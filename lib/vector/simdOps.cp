// Origin: Pure-CP Stdlib (AI-Native SIMD Vector Distance & Batch Retrieval Kernels using @dot/@l2sq/@fma) | Created: 2026-11-22
// Pure CP zero-allocation SIMD vector distance, hardware-FMA AXPY, and batch Top-K Cosine/L2
// retrieval kernels powered directly by CP's native 8-lane SIMD intrinsics (@dot, @l2sq, @fma).

const std = @import("std");

// ============================================================================
// Internal Validation Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clampCosineF32(x: f32): f32 {
    if (x > 1.0) {
        return 1.0;
    }
    if (x < -1.0) {
        return -1.0;
    }
    return x;
}

// ============================================================================
// 1. Native SIMD Dot Product, L2 Distance, Cosine Similarity & FMA AXPY
// ============================================================================

/// Computes the 8-lane SIMD inner product of two equal-length non-empty `f32` slices via `@dot`.
/// Returns `null` on empty input, length mismatch, or `NaN` result.
pub fn simdDotF32(a: []const f32, b: []const f32): f32? {
    if (a.len == 0 || a.len != b.len) {
        return null;
    }
    const res: f32 = @dot(a, b);
    if (std.math.isNan(res)) {
        return null;
    }
    return res;
}

/// Computes the SIMD inner product of two equal-length non-empty `f64` slices via `@dot`.
/// Returns `null` on empty input, length mismatch, or `NaN` result.
pub fn simdDotF64(a: []const f64, b: []const f64): f64? {
    if (a.len == 0 || a.len != b.len) {
        return null;
    }
    const res: f64 = @dot(a, b);
    if (std.math.isNan(res)) {
        return null;
    }
    return res;
}

/// Computes the 8-lane SIMD squared Euclidean distance `sum((a[i] - b[i])^2)` via `@l2sq`.
/// Returns `null` on empty input, length mismatch, or `NaN` result.
pub fn simdL2SqF32(a: []const f32, b: []const f32): f32? {
    if (a.len == 0 || a.len != b.len) {
        return null;
    }
    const res: f32 = @l2sq(a, b);
    if (std.math.isNan(res)) {
        return null;
    }
    if (res < 0.0) {
        return 0.0;
    }
    return res;
}

/// Computes the 8-lane SIMD Euclidean (L2) distance `sqrt(@l2sq(a, b))`.
/// Returns `null` on empty input, length mismatch, or `NaN` result.
pub fn simdL2DistF32(a: []const f32, b: []const f32): f32? {
    if (sq := simdL2SqF32(a, b)) {
        return @sqrt(sq);
    }
    return null;
}

/// Computes SIMD cosine similarity `@dot(a, b) / sqrt(@dot(a, a) * @dot(b, b))` in `[-1.0, 1.0]`.
/// Safely returns `0.0` if either input is a zero vector; returns `null` on empty/mismatched/NaN input.
pub fn simdCosineF32(a: []const f32, b: []const f32): f32? {
    if (a.len == 0 || a.len != b.len) {
        return null;
    }
    const na: f32 = @dot(a, a);
    const nb: f32 = @dot(b, b);
    const ab: f32 = @dot(a, b);
    if (std.math.isNan(na) || std.math.isNan(nb) || std.math.isNan(ab)) {
        return null;
    }
    if (na <= 0.0 || nb <= 0.0) {
        return 0.0;
    }
    const denom: f32 = @sqrt(na) * @sqrt(nb);
    if (denom <= 0.0 || std.math.isNan(denom)) {
        return 0.0;
    }
    return clampCosineF32(ab / denom);
}

/// Performs in-place single-rounding hardware FMA AXPY: `y[i] = @fma(alpha, x[i], y[i])`.
/// Returns `false` on empty input, length mismatch, or non-finite `alpha`.
pub fn simdAxpyInPlaceF32(alpha: f32, x: []const f32, y: *[]f32): bool {
    if (x.len == 0 || x.len != y.len || !isFiniteF32(alpha)) {
        return false;
    }
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        y[i] = @fma(alpha, x[i], y[i]);
    }
    return true;
}

// ============================================================================
// 2. Zero-Allocation Batch SIMD Top-K Retrieval Kernels
// ============================================================================

/// Zero-allocation batch SIMD Top-K Cosine retrieval over a flat row-major `[n_docs, dim]` corpus.
/// Outputs up to `min(k, n_docs)` document indices and cosine similarities sorted descending
/// (tie-broken by ascending `doc_id`).
pub fn simdBatchTopKCosineF32(
    query: []const f32,
    corpus_flat: []const f32,
    n_docs: usize,
    dim: usize,
    k: usize,
    out_ids: *[]usize,
    out_scores: *[]f32,
): usize? {
    if (dim == 0 || query.len != dim) {
        return null;
    }
    if (out_ids.len < k || out_scores.len < k) {
        return null;
    }
    if (n_docs == 0 || k == 0) {
        return 0;
    }
    if (n_docs > std.math.maxInt(usize) / dim) {
        return null;
    }
    if (corpus_flat.len < n_docs * dim) {
        return null;
    }

    const q_norm_sq: f32 = @dot(query, query);
    if (std.math.isNan(q_norm_sq)) {
        return null;
    }
    const q_norm: f32 = if (q_norm_sq > 0.0) @sqrt(q_norm_sq) else 0.0;

    let limit: usize = k;
    if (n_docs < limit) {
        limit = n_docs;
    }

    let count: usize = 0;
    let doc_id: usize = 0;
    while (doc_id < n_docs) : (doc_id += 1) {
        const base: usize = doc_id * dim;
        const doc_vec = corpus_flat[base .. base + dim];
        let score: f32 = 0.0;
        if (q_norm > 0.0) {
            const d_norm_sq: f32 = @dot(doc_vec, doc_vec);
            const ab: f32 = @dot(query, doc_vec);
            if (std.math.isNan(d_norm_sq) || std.math.isNan(ab)) {
                return null;
            }
            if (d_norm_sq > 0.0) {
                const denom: f32 = q_norm * @sqrt(d_norm_sq);
                if (denom > 0.0) {
                    score = clampCosineF32(ab / denom);
                }
            }
        }

        // Insert into sorted descending top-K buffer (tie-break: smaller doc_id wins).
        if (count < limit) {
            let pos: usize = count;
            while (pos > 0 && (score > out_scores[pos - 1] || (score == out_scores[pos - 1] && doc_id < out_ids[pos - 1]))) : (pos -= 1) {
                out_scores[pos] = out_scores[pos - 1];
                out_ids[pos] = out_ids[pos - 1];
            }
            out_scores[pos] = score;
            out_ids[pos] = doc_id;
            count += 1;
        } else if (score > out_scores[limit - 1] || (score == out_scores[limit - 1] && doc_id < out_ids[limit - 1])) {
            let pos: usize = limit - 1;
            while (pos > 0 && (score > out_scores[pos - 1] || (score == out_scores[pos - 1] && doc_id < out_ids[pos - 1]))) : (pos -= 1) {
                out_scores[pos] = out_scores[pos - 1];
                out_ids[pos] = out_ids[pos - 1];
            }
            out_scores[pos] = score;
            out_ids[pos] = doc_id;
        }
    }

    return count;
}

/// Zero-allocation batch SIMD Top-K L2 nearest-neighbor retrieval over a flat `[n_docs, dim]` corpus.
/// Ranks candidates using `@l2sq` and applies `@sqrt` only to the final Top-K winners.
/// Outputs up to `min(k, n_docs)` document indices and L2 distances sorted ascending.
pub fn simdBatchTopKL2F32(
    query: []const f32,
    corpus_flat: []const f32,
    n_docs: usize,
    dim: usize,
    k: usize,
    out_ids: *[]usize,
    out_dists: *[]f32,
): usize? {
    if (dim == 0 || query.len != dim) {
        return null;
    }
    if (out_ids.len < k || out_dists.len < k) {
        return null;
    }
    if (n_docs == 0 || k == 0) {
        return 0;
    }
    if (n_docs > std.math.maxInt(usize) / dim) {
        return null;
    }
    if (corpus_flat.len < n_docs * dim) {
        return null;
    }

    let limit: usize = k;
    if (n_docs < limit) {
        limit = n_docs;
    }

    // Store squared distances during ranking to avoid n_docs - k redundant sqrt calls.
    let count: usize = 0;
    let doc_id: usize = 0;
    while (doc_id < n_docs) : (doc_id += 1) {
        const base: usize = doc_id * dim;
        const doc_vec = corpus_flat[base .. base + dim];
        let d2: f32 = @l2sq(query, doc_vec);
        if (std.math.isNan(d2)) {
            return null;
        }
        if (d2 < 0.0) {
            d2 = 0.0;
        }

        if (count < limit) {
            let pos: usize = count;
            while (pos > 0 && (d2 < out_dists[pos - 1] || (d2 == out_dists[pos - 1] && doc_id < out_ids[pos - 1]))) : (pos -= 1) {
                out_dists[pos] = out_dists[pos - 1];
                out_ids[pos] = out_ids[pos - 1];
            }
            out_dists[pos] = d2;
            out_ids[pos] = doc_id;
            count += 1;
        } else if (d2 < out_dists[limit - 1] || (d2 == out_dists[limit - 1] && doc_id < out_ids[limit - 1])) {
            let pos: usize = limit - 1;
            while (pos > 0 && (d2 < out_dists[pos - 1] || (d2 == out_dists[pos - 1] && doc_id < out_ids[pos - 1]))) : (pos -= 1) {
                out_dists[pos] = out_dists[pos - 1];
                out_ids[pos] = out_ids[pos - 1];
            }
            out_dists[pos] = d2;
            out_ids[pos] = doc_id;
        }
    }

    let j: usize = 0;
    while (j < count) : (j += 1) {
        out_dists[j] = @sqrt(out_dists[j]);
    }
    return count;
}
