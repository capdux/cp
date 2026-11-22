// Origin: Pure-CP Stdlib (AI Inference FlashAttention-2 Tiled Online Softmax & Sliding Window) | Created: 2026-11-22
// Zero-heap-allocation FlashAttention-2 forward kernel with O(1) extra memory (never materializes
// the N x N attention score matrix), tiled online softmax rescaling, causal masking, Mistral-style
// sliding window attention, and single-query autoregressive decode fast path.

const std = @import("std");

/// Running online Softmax state `(max_val, sum_exp)` (Milakov & Gimelshein 2018 / FlashAttention-2).
pub struct OnlineSoftmaxState {
    max_val: f64 = 0.0;
    sum_exp: f64 = 0.0;

    /// Initializes an empty online Softmax accumulator (`max_val = -Inf`, `sum_exp = 0.0`).
    static fn init(): OnlineSoftmaxState {
        return OnlineSoftmaxState{
            max_val: 0.0 - std.math.inf(f64),
            sum_exp: 0.0,
        };
    }

    /// Updates `(max_val, sum_exp)` with a new logit `score` and returns the rescaling factor
    /// `exp(old_max - new_max)` for existing unnormalized output accumulators (`0.0` on first element).
    fn update(*_, score: f64): f64 {
        if (_.sum_exp == 0.0) {
            _.max_val = score;
            _.sum_exp = 1.0;
            return 0.0;
        }
        if (score > _.max_val) {
            const alpha = std.math.exp(_.max_val - score);
            _.sum_exp = _.sum_exp * alpha + 1.0;
            _.max_val = score;
            return alpha;
        }
        _.sum_exp += std.math.exp(score - _.max_val);
        return 1.0;
    }
}

/// Configuration for `flashAttentionForward`.
/// - `scale <= 0.0`: defaults to `1.0 / sqrt(head_dim)`.
/// - `causal`: enables lower-triangular autoregressive masking.
/// - `window_size`: `0` = full context; `> 0` = Mistral-style sliding window `[i - window + 1, i]`.
/// - `block_kv`: KV tile size (default `32`; `0` falls back to `32`).
pub struct FlashAttnConfig {
    scale: f64 = 0.0;
    causal: bool = false;
    window_size: usize = 0;
    block_kv: usize = 32;

    /// Returns default non-causal full-context configuration (`scale = auto`, `block_kv = 32`).
    static fn default(): FlashAttnConfig {
        return FlashAttnConfig{
            scale: 0.0,
            causal: false,
            window_size: 0,
            block_kv: 32,
        };
    }
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Zero-heap-allocation FlashAttention-2 forward pass with O(1) extra memory.
/// Computes `out = softmax(mask(Q * K^T * scale)) * V` without allocating any `seq_q x seq_kv` buffer.
/// `q` and `out` have shape `[seq_q, head_dim]`; `k` and `v` have shape `[seq_kv, head_dim]`.
/// Returns `false` on zero dimensions, buffer underflow, or NaN/Inf inputs.
pub fn flashAttentionForward(
    q: []const f64,
    k: []const f64,
    v: []const f64,
    seq_q: usize,
    seq_kv: usize,
    head_dim: usize,
    cfg: FlashAttnConfig,
    out: *[]f64,
): bool {
    if (seq_q == 0 || seq_kv == 0 || head_dim == 0) return false;
    if (!isFiniteF64(cfg.scale)) return false;
    if (seq_q > std.math.maxInt(usize) / head_dim) return false;
    if (seq_kv > std.math.maxInt(usize) / head_dim) return false;

    const q_elems = seq_q * head_dim;
    const kv_elems = seq_kv * head_dim;
    if (q.len < q_elems || k.len < kv_elems || v.len < kv_elems || out.len < q_elems) return false;

    let iq: usize = 0;
    while (iq < q_elems) : (iq += 1) {
        if (!isFiniteF64(q[iq])) return false;
    }
    let ikv: usize = 0;
    while (ikv < kv_elems) : (ikv += 1) {
        if (!isFiniteF64(k[ikv]) || !isFiniteF64(v[ikv])) return false;
    }

    const eff_scale = if (cfg.scale > 0.0) cfg.scale else 1.0 / std.math.sqrt(head_dim as f64);
    const tile_kv: usize = if (cfg.block_kv == 0) 32 else cfg.block_kv;

    let qi: usize = 0;
    while (qi < seq_q) : (qi += 1) {
        const q_base = qi * head_dim;

        // Zero output accumulator for row `qi`
        let zd: usize = 0;
        while (zd < head_dim) : (zd += 1) {
            out[q_base + zd] = 0.0;
        }

        // Determine active KV index range [k_start, k_end) for query `qi`
        let anchor_end: usize = seq_kv;
        if (seq_kv >= seq_q) {
            anchor_end = (seq_kv - seq_q) + qi + 1;
        } else {
            anchor_end = if (qi + 1 < seq_kv) qi + 1 else seq_kv;
        }

        let k_start: usize = 0;
        let k_end: usize = seq_kv;
        if (cfg.window_size > 0) {
            k_end = if (anchor_end < seq_kv) anchor_end else seq_kv;
            k_start = if (k_end > cfg.window_size) k_end - cfg.window_size else 0;
        } else if (cfg.causal) {
            k_end = if (anchor_end < seq_kv) anchor_end else seq_kv;
        }

        let state = OnlineSoftmaxState.init();

        // Tiled KV loop with online Softmax rescaling
        let tile_start = k_start;
        while (tile_start < k_end) {
            const tile_end = if (tile_start + tile_kv < k_end) tile_start + tile_kv else k_end;
            let kj = tile_start;
            while (kj < tile_end) : (kj += 1) {
                const kv_base = kj * head_dim;
                let dot: f64 = 0.0;
                let d: usize = 0;
                while (d < head_dim) : (d += 1) {
                    dot += q[q_base + d] * k[kv_base + d];
                }
                const score = dot * eff_scale;
                if (!isFiniteF64(score)) return false;

                const alpha = state.update(score);
                const weight = std.math.exp(score - state.max_val);

                let d2: usize = 0;
                while (d2 < head_dim) : (d2 += 1) {
                    out[q_base + d2] = out[q_base + d2] * alpha + weight * v[kv_base + d2];
                }
            }
            tile_start = tile_end;
        }

        // Final normalization by running denominator `sum_exp`
        if (state.sum_exp > 0.0) {
            const inv_l = 1.0 / state.sum_exp;
            let dn: usize = 0;
            while (dn < head_dim) : (dn += 1) {
                out[q_base + dn] *= inv_l;
            }
        }
    }

    return true;
}

/// Autoregressive single-token decode fast path: attends `q_vec[0..head_dim]` over all
/// `seq_kv` cached KV entries using O(1)-memory online Softmax.
pub fn flashAttentionSingleQuery(
    q_vec: []const f64,
    k: []const f64,
    v: []const f64,
    seq_kv: usize,
    head_dim: usize,
    scale: f64,
    out_vec: *[]f64,
): bool {
    if (head_dim == 0 || q_vec.len != head_dim || out_vec.len < head_dim) return false;
    const cfg = FlashAttnConfig{
        scale: scale,
        causal: false,
        window_size: 0,
        block_kv: 32,
    };
    return flashAttentionForward(q_vec, k, v, 1, seq_kv, head_dim, cfg, out_vec);
}
