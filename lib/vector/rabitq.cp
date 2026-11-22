// Origin: Pure-CP Stdlib (AI Vector Search RaBitQ Unbiased 1-bit Binary Quantization & Estimator) | Created: 2026-11-22
// Pure CP zero-heap-allocation RaBitQ (SIGMOD 2024) unbiased 1-bit random orthogonal binary
// quantization: 32x memory compression for f32 vectors, O(D log D) normalized Fast Walsh-Hadamard
// Transform + deterministic Rademacher sign-flip, exact geometric correction metadata, and
// zero-temp-buffer unbiased inner product / squared L2 distance estimators.

const std = @import("std");

/// Geometric correction metadata stored alongside a RaBitQ 1-bit binary code.
pub struct RabitqCodeMeta {
    /// L2 norm of the residual vector `||x - c||_2`.
    norm_l2: f32 = 0.0;
    /// Exact inner product `<o, o_bar>` between unit residual `o` and quantized vertex `o_bar`.
    dot_o_obar: f32 = 0.0;
    /// Number of set bits (`popcount`) in the packed 1-bit code.
    sum_bits: u32 = 0;
}

// ============================================================================
// Internal Numeric & Deterministic SplitMix64 Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteSliceF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) return false;
    }
    return true;
}

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

fn isPowerOfTwo(n: usize): bool {
    return n > 0 && (n & (n - 1)) == 0;
}

fn nextSplitMix64(state: *u64): u64 {
    const c_add: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
    const c_m1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
    const c_m2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
    state.* = state.* +% c_add;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% c_m1;
    z = (z ^ (z >> 27)) *% c_m2;
    return z ^ (z >> 31);
}

// ============================================================================
// Public RaBitQ API
// ============================================================================

/// Returns the number of `u64` words needed to pack `dim` 1-bit RaBitQ codes (`ceil(dim / 64)`).
pub fn rabitqWordsForDim(dim: usize): usize {
    if (dim == 0) return 0;
    return ((dim - 1) >> 6) + 1;
}

/// Applies a deterministic Rademacher (`±1`) coordinate sign flip in-place keyed by `seed`.
/// Applying twice with the same `seed` is an exact involution.
/// Returns `false` if `vec` is empty or contains non-finite values.
pub fn rabitqSignFlipInPlace(vec: *[]f32, seed: u64): bool {
    if (vec.len == 0) return false;
    let state: u64 = seed;
    let i: usize = 0;
    while (i < vec.len) : (i += 1) {
        const val: f32 = vec[i];
        if (!isFiniteF32(val)) return false;
        if ((nextSplitMix64(&state) & (1 as u64)) == (1 as u64)) {
            vec[i] = (0.0 as f32) - val;
        }
    }
    return true;
}

/// Applies the normalized O(D log D) in-place Fast Walsh-Hadamard Transform (`H_D / sqrt(D)`).
/// Requires `vec.len` to be a power of 2 and all elements finite.
/// Orthogonal and symmetric (`H_D^2 = I`): preserves L2 norms and inner products, and is self-inverse.
pub fn rabitqFastWalshHadamardInPlace(vec: *[]f32): bool {
    const n: usize = vec.len;
    if (!isPowerOfTwo(n)) return false;

    let check: usize = 0;
    while (check < n) : (check += 1) {
        if (!isFiniteF32(vec[check])) return false;
    }

    let h: usize = 1;
    while (h < n) : (h <<= 1) {
        const step: usize = h << 1;
        let i: usize = 0;
        while (i < n) : (i += step) {
            let j: usize = i;
            const limit: usize = i + h;
            while (j < limit) : (j += 1) {
                const u: f32 = vec[j];
                const v: f32 = vec[j + h];
                vec[j] = u + v;
                vec[j + h] = u - v;
            }
        }
    }

    const inv_sqrt_n: f32 = 1.0 / @sqrt(n as f32);
    let k: usize = 0;
    while (k < n) : (k += 1) {
        vec[k] = vec[k] * inv_sqrt_n;
    }

    return true;
}

/// Encodes the residual `r = vec - centroid` into a 1-bit packed RaBitQ code (`out_words`)
/// and returns its exact geometric correction factors (`RabitqCodeMeta`).
///
/// - Bit `i` is `1` iff `vec[i] - centroid[i] >= 0.0` (representing `+1/sqrt(D)`), else `0` (`-1/sqrt(D)`).
/// - Zero residual (`||vec - centroid||_2 == 0`) safely returns `norm_l2 = 0.0, dot_o_obar = 0.0, sum_bits = 0`.
/// - Returns `null` on empty/mismatched/non-finite inputs or undersized `out_words`.
pub fn rabitqEncodeUnit(
    vec: []const f32,
    centroid: []const f32,
    out_words: *[]u64,
): RabitqCodeMeta? {
    const dim: usize = vec.len;
    if (dim == 0 || centroid.len != dim) return null;
    const words_needed: usize = rabitqWordsForDim(dim);
    if (out_words.len < words_needed) return null;

    let w_clear: usize = 0;
    while (w_clear < words_needed) : (w_clear += 1) {
        out_words[w_clear] = 0;
    }

    let norm_sq: f32 = 0.0;
    let l1_sum: f32 = 0.0;
    let i: usize = 0;
    while (i < dim) : (i += 1) {
        const xv: f32 = vec[i];
        const cv: f32 = centroid[i];
        if (!isFiniteF32(xv) || !isFiniteF32(cv)) return null;
        const r: f32 = xv - cv;
        norm_sq = @fma(r, r, norm_sq);
        l1_sum += absF32(r);
        if (r >= 0.0) {
            const word_idx: usize = i >> 6;
            const bit_idx: u6 = @intCast(i & 63);
            out_words[word_idx] = out_words[word_idx] | ((1 as u64) << bit_idx);
        }
    }

    if (!isFiniteF32(norm_sq) || !isFiniteF32(l1_sum)) return null;
    if (norm_sq <= 0.0) {
        let z: usize = 0;
        while (z < words_needed) : (z += 1) {
            out_words[z] = 0;
        }
        return RabitqCodeMeta{
            norm_l2: 0.0,
            dot_o_obar: 0.0,
            sum_bits: 0,
        };
    }

    const norm_l2: f32 = @sqrt(norm_sq);
    const inv_sqrt_d: f32 = 1.0 / @sqrt(dim as f32);
    // Exact <o, o_bar> = sum(|r_i|) / (||r||_2 * sqrt(D)) in [1/sqrt(D), 1.0].
    const dot_o_obar: f32 = (l1_sum / norm_l2) * inv_sqrt_d;

    let sum_bits: u32 = 0;
    let w: usize = 0;
    while (w < words_needed) : (w += 1) {
        sum_bits += @popCount(out_words[w]) as u32;
    }

    return RabitqCodeMeta{
        norm_l2: norm_l2,
        dot_o_obar: dot_o_obar,
        sum_bits: sum_bits,
    };
}

/// Computes the unbiased RaBitQ inner product estimate of `<query_residual, x - c>`:
/// `est = meta.norm_l2 * <query_residual, o_bar> / meta.dot_o_obar`,
/// directly from packed 1-bit `code_words` without allocating a temporary decoded vector.
/// Safely returns `0.0` for zero-residual vectors; returns `null` on invalid/non-finite inputs.
pub fn rabitqEstimateDot(
    query_residual: []const f32,
    code_words: []const u64,
    meta: RabitqCodeMeta,
): f32? {
    const dim: usize = query_residual.len;
    if (dim == 0) return null;
    if (code_words.len < rabitqWordsForDim(dim)) return null;
    if (!isFiniteF32(meta.norm_l2) || !isFiniteF32(meta.dot_o_obar) || meta.norm_l2 < 0.0 || meta.dot_o_obar < 0.0) {
        return null;
    }
    if (!isFiniteSliceF32(query_residual)) return null;

    if (meta.norm_l2 == 0.0 || meta.dot_o_obar == 0.0) {
        return 0.0;
    }

    // Compute signed sum: sum_i (2 * b_i - 1) * query_residual[i]
    let signed_sum: f32 = 0.0;
    let i: usize = 0;
    while (i < dim) : (i += 1) {
        const word: u64 = code_words[i >> 6];
        const bit_idx: u6 = @intCast(i & 63);
        const qv: f32 = query_residual[i];
        if ((word & ((1 as u64) << bit_idx)) != 0) {
            signed_sum += qv;
        } else {
            signed_sum -= qv;
        }
    }

    const inv_sqrt_d: f32 = 1.0 / @sqrt(dim as f32);
    const dot_q_obar: f32 = signed_sum * inv_sqrt_d;
    const est: f32 = meta.norm_l2 * (dot_q_obar / meta.dot_o_obar);
    if (!isFiniteF32(est)) return null;
    return est;
}

/// Computes the unbiased RaBitQ squared L2 distance estimate `||q - x||_2^2`:
/// `est_l2sq = query_norm_sq + meta.norm_l2^2 - 2 * rabitqEstimateDot(...)`, clamped to `>= 0.0`.
/// Returns `null` on invalid/non-finite inputs or negative `query_norm_sq`.
pub fn rabitqEstimateL2Sq(
    query_residual: []const f32,
    query_norm_sq: f32,
    code_words: []const u64,
    meta: RabitqCodeMeta,
): f32? {
    if (!isFiniteF32(query_norm_sq) || query_norm_sq < 0.0) return null;
    if (est_dot := rabitqEstimateDot(query_residual, code_words, meta)) {
        const x_norm_sq: f32 = meta.norm_l2 * meta.norm_l2;
        const est_l2sq: f32 = @fma(-2.0 as f32, est_dot, query_norm_sq + x_norm_sq);
        if (!isFiniteF32(est_l2sq)) return null;
        if (est_l2sq < 0.0) return 0.0;
        return est_l2sq;
    }
    return null;
}
