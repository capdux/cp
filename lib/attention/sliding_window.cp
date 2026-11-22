// Origin: Pure-CP Stdlib (AI Long-Context Sliding Window Attention & StreamingLLM Sink Cache) | Created: 2026-11-22
// Pure CP zero-allocation Sliding Window Attention (SWA, Mistral/Gemma-2 style) and
// StreamingLLM Attention Sink fixed-capacity RingKvCache for unbounded streaming decode
// in O((S + W) * d) time and O(1) extra scratch memory via Online Softmax.

const std = @import("std");

/// Configuration for Sliding Window Attention (SWA) and StreamingLLM Attention Sink cache.
pub struct SwaConfig {
    /// Number of initial tokens permanently anchored as attention sinks.
    sink_tokens: usize = 0,
    /// Size of the local sliding attention window.
    window_size: usize = 0,
    /// Per-head embedding dimension.
    head_dim: usize = 0,
    /// If true, masks future tokens strictly after `query_pos`.
    causal: bool = true,
}

// ============================================================================
// Internal Validation & Online Softmax Accumulation Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isValidSwaConfig(cfg: SwaConfig): bool {
    if (cfg.head_dim == 0) {
        return false;
    }
    if (cfg.sink_tokens > std.math.maxInt(usize) - cfg.window_size) {
        return false;
    }
    const cap_tokens: usize = cfg.sink_tokens + cfg.window_size;
    if (cap_tokens == 0) {
        return false;
    }
    if (cap_tokens > std.math.maxInt(usize) / cfg.head_dim) {
        return false;
    }
    return true;
}

fn checkSliceFinite(slice: []const f32): bool {
    let i: usize = 0;
    while (i < slice.len) : (i += 1) {
        if (!isFiniteF32(slice[i])) {
            return false;
        }
    }
    return true;
}

/// Online Softmax step accumulating one key/value token into `out_acc` (`f32` buffer).
fn onlineSoftmaxAttendToken(
    query: []const f32,
    k_tok: []const f32,
    v_tok: []const f32,
    head_dim: usize,
    inv_sqrt_d: f64,
    first_token: bool,
    max_score_ptr: *f64,
    sum_exp_ptr: *f64,
    out_acc: *[]f32,
) {
    let dot: f64 = 0.0;
    let d: usize = 0;
    while (d < head_dim) : (d += 1) {
        dot += (query[d] as f64) * (k_tok[d] as f64);
    }
    const score: f64 = dot * inv_sqrt_d;

    if (first_token) {
        max_score_ptr.* = score;
        sum_exp_ptr.* = 1.0;
        d = 0;
        while (d < head_dim) : (d += 1) {
            out_acc[d] = v_tok[d];
        }
    } else if (score > max_score_ptr.*) {
        const rescale: f64 = std.math.exp(max_score_ptr.* - score);
        sum_exp_ptr.* = sum_exp_ptr.* * rescale + 1.0;
        max_score_ptr.* = score;
        d = 0;
        while (d < head_dim) : (d += 1) {
            out_acc[d] = ((out_acc[d] as f64) * rescale + (v_tok[d] as f64)) as f32;
        }
    } else {
        const w_t: f64 = std.math.exp(score - max_score_ptr.*);
        sum_exp_ptr.* += w_t;
        d = 0;
        while (d < head_dim) : (d += 1) {
            out_acc[d] = ((out_acc[d] as f64) + w_t * (v_tok[d] as f64)) as f32;
        }
    }
}

// ============================================================================
// 1. StreamingLLM Fixed-Capacity RingKvCache
// ============================================================================

/// Zero-allocation StreamingLLM KV-Cache with permanently anchored sink slots
/// `[0 .. sink_tokens)` and a circular overwrite window `[sink_tokens .. sink_tokens + window_size)`.
pub struct RingKvCache {
    cfg: SwaConfig,
    keys: []f32,
    vals: []f32,
    sink_filled: usize = 0,
    window_filled: usize = 0,
    window_write_pos: usize = 0,
    total_seen: u64 = 0,

    /// Initializes a zero-allocation `RingKvCache` over caller-provided `keys_buf` and `vals_buf`.
    /// Requires buffer lengths >= `(cfg.sink_tokens + cfg.window_size) * cfg.head_dim`.
    pub static fn init(
        cfg: SwaConfig,
        keys_buf: *[]f32,
        vals_buf: *[]f32,
    ): RingKvCache? {
        if (!isValidSwaConfig(cfg)) {
            return null;
        }
        const needed: usize = (cfg.sink_tokens + cfg.window_size) * cfg.head_dim;
        if (keys_buf.len < needed || vals_buf.len < needed) {
            return null;
        }
        return RingKvCache{
            cfg: cfg,
            keys: keys_buf,
            vals: vals_buf,
            sink_filled: 0,
            window_filled: 0,
            window_write_pos: 0,
            total_seen: 0,
        };
    }

    /// Appends one token's `(k, v)` pair.
    /// Fills the permanent sink zone first, then overwrites the oldest slot in the circular window.
    /// Returns `false` on dimension mismatch, non-finite inputs, or full sink-only cache (`window_size == 0`).
    pub fn appendToken(*_, k: []const f32, v: []const f32): bool {
        const d: usize = _.cfg.head_dim;
        if (k.len != d || v.len != d) {
            return false;
        }
        if (!checkSliceFinite(k) || !checkSliceFinite(v)) {
            return false;
        }

        let target_slot: usize = 0;
        if (_.sink_filled < _.cfg.sink_tokens) {
            target_slot = _.sink_filled;
            _.sink_filled += 1;
        } else if (_.cfg.window_size == 0) {
            return false;
        } else {
            target_slot = _.cfg.sink_tokens + _.window_write_pos;
            _.window_write_pos = (_.window_write_pos + 1) % _.cfg.window_size;
            if (_.window_filled < _.cfg.window_size) {
                _.window_filled += 1;
            }
        }

        const base: usize = target_slot * d;
        let i: usize = 0;
        while (i < d) : (i += 1) {
            _.keys[base + i] = k[i];
            _.vals[base + i] = v[i];
        }
        _.total_seen += 1;
        return true;
    }

    /// Returns the number of active KV slots currently retained (`<= sink_tokens + window_size`).
    pub fn activeLen(): usize {
        return _.sink_filled + _.window_filled;
    }

    /// Returns the total number of tokens ever appended since `init` or last `clear`.
    pub fn totalSeen(): u64 {
        return _.total_seen;
    }

    /// Resets occupancy and stream counters without touching underlying buffer ownership.
    pub fn clear(*_) {
        _.sink_filled = 0;
        _.window_filled = 0;
        _.window_write_pos = 0;
        _.total_seen = 0;
    }
}

// ============================================================================
// 2. Sliding Window Attention (SWA + Attention Sink) over Contiguous KV Tensors
// ============================================================================

/// Computes single-query Sliding Window Attention with optional Attention Sink tokens
/// over contiguous `[seq_len, head_dim]` `keys` and `vals` tensors in $O((S + W) \cdot d)$ time.
///
/// In causal mode (`cfg.causal == true`), attends strictly to the disjoint union:
/// `[0, min(sink_tokens, query_pos + 1))` U `[max(sink_tokens, query_pos + 1 - window_size), query_pos]`.
pub fn swaAttentionSingleQuery(
    query: []const f32,
    keys: []const f32,
    vals: []const f32,
    seq_len: usize,
    query_pos: usize,
    cfg: SwaConfig,
    out: *[]f32,
): bool {
    if (!isValidSwaConfig(cfg) || seq_len == 0 || query_pos >= seq_len) {
        return false;
    }
    const d: usize = cfg.head_dim;
    if (seq_len > std.math.maxInt(usize) / d) {
        return false;
    }
    const total_elems: usize = seq_len * d;
    if (query.len < d || out.len < d || keys.len < total_elems || vals.len < total_elems) {
        return false;
    }
    if (!checkSliceFinite(query[0..d])) {
        return false;
    }

    // Compute strictly disjoint Sink [0, sink_end) and Window [win_start, win_end) ranges.
    let sink_end: usize = 0;
    let win_start: usize = 0;
    let win_end: usize = 0;

    if (cfg.causal) {
        const end_excl: usize = query_pos + 1;
        sink_end = if (cfg.sink_tokens < end_excl) cfg.sink_tokens else end_excl;
        if (cfg.window_size > 0) {
            let raw_start: usize = 0;
            if (end_excl > cfg.window_size) {
                raw_start = end_excl - cfg.window_size;
            }
            win_start = if (raw_start > sink_end) raw_start else sink_end;
            win_end = end_excl;
        } else {
            win_start = sink_end;
            win_end = sink_end;
        }
    } else {
        sink_end = if (cfg.sink_tokens < seq_len) cfg.sink_tokens else seq_len;
        if (cfg.window_size > 0) {
            const half: usize = cfg.window_size / 2;
            let raw_start: usize = 0;
            if (query_pos > half) {
                raw_start = query_pos - half;
            }
            let raw_end: usize = raw_start + cfg.window_size;
            if (raw_end > seq_len) {
                raw_end = seq_len;
                if (seq_len > cfg.window_size) {
                    raw_start = seq_len - cfg.window_size;
                } else {
                    raw_start = 0;
                }
            }
            win_start = if (raw_start > sink_end) raw_start else sink_end;
            win_end = if (raw_end > win_start) raw_end else win_start;
        } else {
            win_start = sink_end;
            win_end = sink_end;
        }
    }

    const total_attended: usize = sink_end + (win_end - win_start);
    if (total_attended == 0) {
        return false;
    }

    // Atomic finite check over attended K/V slices before mutating `out`.
    let t: usize = 0;
    while (t < sink_end) : (t += 1) {
        const base: usize = t * d;
        if (!checkSliceFinite(keys[base .. base + d]) || !checkSliceFinite(vals[base .. base + d])) {
            return false;
        }
    }
    t = win_start;
    while (t < win_end) : (t += 1) {
        const base: usize = t * d;
        if (!checkSliceFinite(keys[base .. base + d]) || !checkSliceFinite(vals[base .. base + d])) {
            return false;
        }
    }

    const inv_sqrt_d: f64 = 1.0 / @sqrt(d as f64);
    let max_score: f64 = -(std.math.inf(f64));
    let sum_exp: f64 = 0.0;
    let attended: usize = 0;

    // 1. Attend permanent Attention Sink range [0, sink_end).
    let s_idx: usize = 0;
    while (s_idx < sink_end) : (s_idx += 1) {
        const base: usize = s_idx * d;
        onlineSoftmaxAttendToken(
            query[0..d],
            keys[base .. base + d],
            vals[base .. base + d],
            d,
            inv_sqrt_d,
            attended == 0,
            *max_score,
            *sum_exp,
            out,
        );
        attended += 1;
    }

    // 2. Attend local sliding window range [win_start, win_end).
    let w_idx: usize = win_start;
    while (w_idx < win_end) : (w_idx += 1) {
        const base: usize = w_idx * d;
        onlineSoftmaxAttendToken(
            query[0..d],
            keys[base .. base + d],
            vals[base .. base + d],
            d,
            inv_sqrt_d,
            attended == 0,
            *max_score,
            *sum_exp,
            out,
        );
        attended += 1;
    }

    // 3. Final normalization.
    const inv_sum: f64 = 1.0 / sum_exp;
    let i: usize = 0;
    while (i < d) : (i += 1) {
        out[i] = ((out[i] as f64) * inv_sum) as f32;
    }
    return true;
}

// ============================================================================
// 3. StreamingLLM Single-Query Attention directly over RingKvCache
// ============================================================================

/// Computes single-query StreamingLLM attention directly over active slots in `cache`
/// (chronological order: anchored sink tokens followed by oldest-to-newest window tokens).
pub fn ringKvAttentionSingleQuery(
    query: []const f32,
    cache: RingKvCache,
    out: *[]f32,
): bool {
    const active: usize = cache.activeLen();
    if (active == 0 || !isValidSwaConfig(cache.cfg)) {
        return false;
    }
    const d: usize = cache.cfg.head_dim;
    if (query.len < d || out.len < d) {
        return false;
    }
    if (!checkSliceFinite(query[0..d])) {
        return false;
    }

    const inv_sqrt_d: f64 = 1.0 / @sqrt(d as f64);
    let max_score: f64 = -(std.math.inf(f64));
    let sum_exp: f64 = 0.0;
    let attended: usize = 0;

    // 1. Attend anchored Attention Sink slots [0 .. sink_filled).
    let s: usize = 0;
    while (s < cache.sink_filled) : (s += 1) {
        const base: usize = s * d;
        onlineSoftmaxAttendToken(
            query[0..d],
            cache.keys[base .. base + d],
            cache.vals[base .. base + d],
            d,
            inv_sqrt_d,
            attended == 0,
            *max_score,
            *sum_exp,
            out,
        );
        attended += 1;
    }

    // 2. Attend circular sliding window in strict chronological order (oldest -> newest).
    if (cache.window_filled > 0 && cache.cfg.window_size > 0) {
        let start_offset: usize = 0;
        if (cache.window_filled == cache.cfg.window_size) {
            start_offset = cache.window_write_pos;
        }
        let k: usize = 0;
        while (k < cache.window_filled) : (k += 1) {
            const ring_idx: usize = (start_offset + k) % cache.cfg.window_size;
            const phys_slot: usize = cache.cfg.sink_tokens + ring_idx;
            const base: usize = phys_slot * d;
            onlineSoftmaxAttendToken(
                query[0..d],
                cache.keys[base .. base + d],
                cache.vals[base .. base + d],
                d,
                inv_sqrt_d,
                attended == 0,
                *max_score,
                *sum_exp,
                out,
            );
            attended += 1;
        }
    }

    const inv_sum: f64 = 1.0 / sum_exp;
    let i: usize = 0;
    while (i < d) : (i += 1) {
        out[i] = ((out[i] as f64) * inv_sum) as f32;
    }
    return true;
}
