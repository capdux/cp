// Origin: Pure-CP Stdlib (Dao-AILab FlashDecoding Split-K KV-Partition Attention & LSE Merge Kernel) | Created: 2026-11-22
// Zero-heap-allocation FlashDecoding Split-K autoregressive decode kernel powered by @dot and @fma:
//   - Splits long KV sequences into independent Split-K partitions with online Softmax + exact LSE
//   - Lossless numerically stable Log-Sum-Exp reduction across arbitrary partition layouts & empty splits
//   - End-to-end single-head Split-K decode and Grouped-Query Attention (GQA) multi-head batch decode

const std = @import("std");

/// Maximum supported attention head dimension for zero-heap-allocation stack partials.
pub const FLASH_DECODE_MAX_HEAD_DIM: usize = 256;

/// Maximum supported Split-K partition count for stack-resident end-to-end Split-K decode.
pub const FLASH_DECODE_MAX_SPLITS: usize = 64;

// ============================================================================
// Internal Numeric & Geometry Validation Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn checkedMul(a: usize, b: usize): usize? {
    if (a == 0 || b == 0) {
        return 0;
    }
    if (a > std.math.maxInt(usize) / b) {
        return null;
    }
    return a * b;
}

// ============================================================================
// Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Per-partition FlashDecoding accumulator holding the normalized partial output vector
/// `o_s = (1 / l_s) * sum_{j in P_s} exp(z_j - m_s) * v_j` and exact Log-Sum-Exp scalar
/// `lse = m_s + ln(l_s)`.
pub struct FlashDecodePartial {
    partial_out: [FLASH_DECODE_MAX_HEAD_DIM]f32 = [0.0; FLASH_DECODE_MAX_HEAD_DIM];
    lse: f64 = 0.0;
    max_logit: f64 = 0.0;
    sum_exp: f64 = 0.0;
    head_dim: usize = 0;
    num_kv: usize = 0;
    valid: bool = false;

    /// Constructs an empty (inactive) partition accumulator that is safely skipped during merge.
    static fn empty(): FlashDecodePartial {
        return FlashDecodePartial{
            partial_out: [0.0; FLASH_DECODE_MAX_HEAD_DIM],
            lse: 0.0 - std.math.inf(f64),
            max_logit: 0.0 - std.math.inf(f64),
            sum_exp: 0.0,
            head_dim: 0,
            num_kv: 0,
            valid: false,
        };
    }

    /// Returns `true` if this partition accumulated at least one KV token with finite LSE.
    fn isValid(): bool {
        return _.valid && _.num_kv > 0 && _.head_dim > 0 && _.head_dim <= FLASH_DECODE_MAX_HEAD_DIM && isFiniteF64(_.lse);
    }

    /// Returns a read-only slice view over the active `head_dim` output elements.
    fn outputSlice(): []const f32 {
        const d: usize = if (_.head_dim <= FLASH_DECODE_MAX_HEAD_DIM) _.head_dim else FLASH_DECODE_MAX_HEAD_DIM;
        return _.partial_out[0..d];
    }
}

/// Diagnostic summary returned by `mergeFlashDecodePartialsWithStats`.
pub struct FlashDecodeMergeStats {
    global_lse: f64 = 0.0;
    active_partitions: usize = 0;
    skipped_empty: usize = 0;
    total_kv: usize = 0;
}

// ============================================================================
// 1. Single-Partition Online Softmax Attention Kernel (Split-K Stage 1)
// ============================================================================

/// Computes single-query online Softmax attention over a single KV partition `[num_kv, head_dim]`,
/// writing the normalized partial output vector and exact `lse = max_logit + ln(sum_exp)` into `out_acc`.
/// - `scale <= 0.0` defaults to `1.0 / sqrt(head_dim)`.
/// - When `num_kv == 0`, resets `out_acc` to an empty inactive partition (`valid = false`) and returns `true`
///   so callers can seamlessly merge partitions even when `num_splits > total_kv`.
/// - Returns `false` on invalid `head_dim`, non-finite `scale`, undersized slices, or NaN/Inf inputs.
pub fn flashDecodePartition(
    q: []const f32,
    k_part: []const f32,
    v_part: []const f32,
    num_kv: usize,
    head_dim: usize,
    scale: f32,
    out_acc: *FlashDecodePartial,
): bool {
    if (head_dim == 0 || head_dim > FLASH_DECODE_MAX_HEAD_DIM) {
        return false;
    }
    if (!isFiniteF32(scale)) {
        return false;
    }
    if (q.len < head_dim) {
        return false;
    }

    // Validate query elements are finite
    let iq: usize = 0;
    while (iq < head_dim) : (iq += 1) {
        if (!isFiniteF32(q[iq])) {
            return false;
        }
    }

    // Empty partition: emit a well-formed inactive partial that merge skips automatically
    if (num_kv == 0) {
        out_acc.lse = 0.0 - std.math.inf(f64);
        out_acc.max_logit = 0.0 - std.math.inf(f64);
        out_acc.sum_exp = 0.0;
        out_acc.head_dim = head_dim;
        out_acc.num_kv = 0;
        out_acc.valid = false;
        let zd: usize = 0;
        while (zd < head_dim) : (zd += 1) {
            out_acc.partial_out[zd] = 0.0;
        }
        return true;
    }

    const kv_elems: usize = checkedMul(num_kv, head_dim) ?? return false;
    if (k_part.len < kv_elems || v_part.len < kv_elems) {
        return false;
    }

    // Validate K and V elements are finite
    let ikv: usize = 0;
    while (ikv < kv_elems) : (ikv += 1) {
        if (!isFiniteF32(k_part[ikv]) || !isFiniteF32(v_part[ikv])) {
            return false;
        }
    }

    const eff_scale: f64 = if (scale > 0.0) (scale as f64) else (1.0 / std.math.sqrt(head_dim as f64));
    const q_slice: []const f32 = q[0..head_dim];

    // Stack accumulator for unnormalized weighted V sum
    let acc: [FLASH_DECODE_MAX_HEAD_DIM]f32 = [0.0; FLASH_DECODE_MAX_HEAD_DIM];
    let max_logit: f64 = 0.0 - std.math.inf(f64);
    let sum_exp: f64 = 0.0;

    let j: usize = 0;
    while (j < num_kv) : (j += 1) {
        const kv_base: usize = j * head_dim;
        const k_row: []const f32 = k_part[kv_base .. kv_base + head_dim];
        const dot_val: f32 = @dot(q_slice, k_row);
        if (!isFiniteF32(dot_val)) {
            return false;
        }
        const score: f64 = (dot_val as f64) * eff_scale;
        if (!isFiniteF64(score)) {
            return false;
        }

        let alpha: f64 = 1.0;
        let weight: f64 = 1.0;
        if (sum_exp == 0.0) {
            max_logit = score;
            sum_exp = 1.0;
            alpha = 0.0;
            weight = 1.0;
        } else if (score > max_logit) {
            alpha = std.math.exp(max_logit - score);
            sum_exp = sum_exp * alpha + 1.0;
            max_logit = score;
            weight = 1.0;
        } else {
            weight = std.math.exp(score - max_logit);
            sum_exp += weight;
            alpha = 1.0;
        }

        const alpha_f32: f32 = alpha as f32;
        const weight_f32: f32 = weight as f32;
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            const scaled_prev: f32 = acc[d] * alpha_f32;
            acc[d] = @fma(weight_f32, v_part[kv_base + d], scaled_prev);
        }
    }

    if (!isFiniteF64(sum_exp) || sum_exp <= 0.0) {
        return false;
    }
    const lse: f64 = max_logit + @log(sum_exp);
    if (!isFiniteF64(lse)) {
        return false;
    }

    const inv_sum: f32 = (1.0 / sum_exp) as f32;
    let dn: usize = 0;
    while (dn < head_dim) : (dn += 1) {
        const val: f32 = acc[dn] * inv_sum;
        if (!isFiniteF32(val)) {
            return false;
        }
        out_acc.partial_out[dn] = val;
    }
    out_acc.lse = lse;
    out_acc.max_logit = max_logit;
    out_acc.sum_exp = sum_exp;
    out_acc.head_dim = head_dim;
    out_acc.num_kv = num_kv;
    out_acc.valid = true;
    return true;
}

// ============================================================================
// 2. Numerically Stable Log-Sum-Exp Partition Reduction (Split-K Stage 2)
// ============================================================================

/// Losslessly merges any number of `FlashDecodePartial` results into the final attention
/// output vector using overflow-proof Log-Sum-Exp reduction, returning merge diagnostics.
/// Automatically skips empty/inactive partitions (`!valid || num_kv == 0`).
/// Returns `null` if `head_dim` is invalid, `out_vec` is undersized, no active partition exists,
/// or any active partition has mismatched `head_dim` / non-finite `lse`.
pub fn mergeFlashDecodePartialsWithStats(
    partials: []const FlashDecodePartial,
    head_dim: usize,
    out_vec: *[]f32,
): FlashDecodeMergeStats? {
    if (head_dim == 0 || head_dim > FLASH_DECODE_MAX_HEAD_DIM) {
        return null;
    }
    if (partials.len == 0 || out_vec.len < head_dim) {
        return null;
    }

    // Pass 1: find global max LSE among active partitions and validate metadata
    let max_lse: f64 = 0.0 - std.math.inf(f64);
    let active_count: usize = 0;
    let skipped_empty: usize = 0;
    let total_kv: usize = 0;

    let i: usize = 0;
    while (i < partials.len) : (i += 1) {
        const p = partials[i];
        if (!p.valid || p.num_kv == 0) {
            skipped_empty += 1;
            continue;
        }
        if (p.head_dim != head_dim || !isFiniteF64(p.lse)) {
            return null;
        }
        if (active_count == 0 || p.lse > max_lse) {
            max_lse = p.lse;
        }
        active_count += 1;
        total_kv += p.num_kv;
    }

    if (active_count == 0) {
        return null;
    }

    // Pass 2: compute global LSE = max_lse + ln(sum_s exp(lse_s - max_lse))
    let sum_exp_lse: f64 = 0.0;
    let j: usize = 0;
    while (j < partials.len) : (j += 1) {
        const p = partials[j];
        if (!p.valid || p.num_kv == 0) {
            continue;
        }
        sum_exp_lse += std.math.exp(p.lse - max_lse);
    }

    if (!isFiniteF64(sum_exp_lse) || sum_exp_lse <= 0.0) {
        return null;
    }
    const global_lse: f64 = max_lse + @log(sum_exp_lse);
    if (!isFiniteF64(global_lse)) {
        return null;
    }

    // Pass 3: weighted combination using @fma (weights exp(lse_s - global_lse) in (0, 1])
    let zd: usize = 0;
    while (zd < head_dim) : (zd += 1) {
        out_vec[zd] = 0.0;
    }

    let k: usize = 0;
    while (k < partials.len) : (k += 1) {
        const p = partials[k];
        if (!p.valid || p.num_kv == 0) {
            continue;
        }
        const w: f32 = std.math.exp(p.lse - global_lse) as f32;
        if (!isFiniteF32(w)) {
            return null;
        }
        let d: usize = 0;
        while (d < head_dim) : (d += 1) {
            const pv: f32 = p.partial_out[d];
            if (!isFiniteF32(pv)) {
                return null;
            }
            out_vec[d] = @fma(w, pv, out_vec[d]);
        }
    }

    return FlashDecodeMergeStats{
        global_lse: global_lse,
        active_partitions: active_count,
        skipped_empty: skipped_empty,
        total_kv: total_kv,
    };
}

/// Convenience boolean wrapper around `mergeFlashDecodePartialsWithStats`.
/// Merges all active `FlashDecodePartial` entries into `out_vec[0..head_dim]` via stable LSE reduction.
pub fn mergeFlashDecodePartials(
    partials: []const FlashDecodePartial,
    head_dim: usize,
    out_vec: *[]f32,
): bool {
    if (stats := mergeFlashDecodePartialsWithStats(partials, head_dim, out_vec)) {
        * = stats;
        return true;
    }
    return false;
}

// ============================================================================
// 3. End-to-End Split-K Decode & Multi-Head GQA Batch Reduction
// ============================================================================

/// End-to-end zero-heap-allocation FlashDecoding Split-K single-query attention.
/// Evenly partitions `[0, num_kv)` into `num_splits` contiguous KV chunks (supporting `num_splits > num_kv`
/// via automatic empty-partition skipping), runs `flashDecodePartition` per split, and merges with LSE.
/// - `scale <= 0.0` defaults to `1.0 / sqrt(head_dim)`.
/// - Returns `false` if `num_kv == 0`, `head_dim == 0`, `head_dim > FLASH_DECODE_MAX_HEAD_DIM`,
///   `num_splits == 0`, `num_splits > FLASH_DECODE_MAX_SPLITS`, buffers are undersized, or inputs contain NaN/Inf.
pub fn flashDecodeSplitK(
    q: []const f32,
    k_all: []const f32,
    v_all: []const f32,
    num_kv: usize,
    head_dim: usize,
    num_splits: usize,
    scale: f32,
    out_vec: *[]f32,
): bool {
    if (num_kv == 0 || head_dim == 0 || head_dim > FLASH_DECODE_MAX_HEAD_DIM) {
        return false;
    }
    if (num_splits == 0 || num_splits > FLASH_DECODE_MAX_SPLITS) {
        return false;
    }
    if (!isFiniteF32(scale)) {
        return false;
    }
    if (q.len < head_dim || out_vec.len < head_dim) {
        return false;
    }
    const kv_elems: usize = checkedMul(num_kv, head_dim) ?? return false;
    if (k_all.len < kv_elems || v_all.len < kv_elems) {
        return false;
    }

    let partials: [FLASH_DECODE_MAX_SPLITS]FlashDecodePartial = [FlashDecodePartial.empty(); FLASH_DECODE_MAX_SPLITS];
    const base_chunk: usize = num_kv / num_splits;
    const rem: usize = num_kv % num_splits;

    let offset_kv: usize = 0;
    let s: usize = 0;
    while (s < num_splits) : (s += 1) {
        const part_kv: usize = base_chunk + (if (s < rem) 1 else 0);
        const start_elem: usize = offset_kv * head_dim;
        const end_elem: usize = (offset_kv + part_kv) * head_dim;
        let part_acc: FlashDecodePartial = FlashDecodePartial.empty();

        if (!flashDecodePartition(
            q[0..head_dim],
            k_all[start_elem..end_elem],
            v_all[start_elem..end_elem],
            part_kv,
            head_dim,
            scale,
            *part_acc,
        )) {
            return false;
        }
        partials[s] = part_acc;
        offset_kv += part_kv;
    }

    return mergeFlashDecodePartials(partials[0..num_splits], head_dim, out_vec);
}

/// Multi-head Grouped-Query Attention (GQA) batch Split-K FlashDecoding kernel.
/// Layout:
/// - `q_heads`: `[num_q_heads, head_dim]`
/// - `k_heads`: `[num_kv_heads, num_kv, head_dim]`
/// - `v_heads`: `[num_kv_heads, num_kv, head_dim]`
/// - `out_heads`: `[num_q_heads, head_dim]`
/// Maps query head `qh` to KV head `qh / (num_q_heads / num_kv_heads)`.
/// Returns `false` if `num_q_heads == 0`, `num_kv_heads == 0`, `num_q_heads % num_kv_heads != 0`,
/// or any dimension / buffer / numeric constraint is violated.
pub fn flashDecodeSplitKGqa(
    q_heads: []const f32,
    k_heads: []const f32,
    v_heads: []const f32,
    num_q_heads: usize,
    num_kv_heads: usize,
    num_kv: usize,
    head_dim: usize,
    num_splits: usize,
    scale: f32,
    out_heads: *[]f32,
): bool {
    if (num_q_heads == 0 || num_kv_heads == 0) {
        return false;
    }
    if ((num_q_heads % num_kv_heads) != 0) {
        return false;
    }
    if (num_kv == 0 || head_dim == 0 || head_dim > FLASH_DECODE_MAX_HEAD_DIM) {
        return false;
    }
    if (num_splits == 0 || num_splits > FLASH_DECODE_MAX_SPLITS) {
        return false;
    }
    if (!isFiniteF32(scale)) {
        return false;
    }

    const q_total: usize = checkedMul(num_q_heads, head_dim) ?? return false;
    const kv_per_head: usize = checkedMul(num_kv, head_dim) ?? return false;
    const kv_total: usize = checkedMul(num_kv_heads, kv_per_head) ?? return false;

    if (q_heads.len < q_total || out_heads.len < q_total) {
        return false;
    }
    if (k_heads.len < kv_total || v_heads.len < kv_total) {
        return false;
    }

    const group_size: usize = num_q_heads / num_kv_heads;
    let qh: usize = 0;
    while (qh < num_q_heads) : (qh += 1) {
        const kvh: usize = qh / group_size;
        const q_start: usize = qh * head_dim;
        const kv_start: usize = kvh * kv_per_head;

        let out_slice: []f32 = out_heads[q_start .. q_start + head_dim];
        if (!flashDecodeSplitK(
            q_heads[q_start .. q_start + head_dim],
            k_heads[kv_start .. kv_start + kv_per_head],
            v_heads[kv_start .. kv_start + kv_per_head],
            num_kv,
            head_dim,
            num_splits,
            scale,
            *out_slice,
        )) {
            return false;
        }
    }
    return true;
}
