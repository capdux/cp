// Origin: Pure-CP Stdlib (SIMD-Accelerated Vectorized Statistics & Linear Algebra Kernels using @dot/@l2sq/@fma/split) | Created: 2026-11-22
// Pure CP zero-heap-allocation numerical statistics and linear algebra facade powered by CP's native
// 8-lane SIMD intrinsics (@dot, @l2sq, @fma) and zero-copy slice splitting (`slice.split`):
//   1. `SimdSummaryStats` / `simdSummaryStatsF32`: Welford cancellation-safe descriptive stats + SIMD `@dot` L2 norm
//   2. `simdPearsonCorrF32`, `simdCosineSimF32`, `simdEuclideanDistF32`: correlation & SIMD distance metrics
//   3. `simdZScoreNormalizeF32`, `simdSoftmaxF32`, `simdMatVecF32`, `simdAxpyF32`: normalization, overflow-safe softmax, GEMV & FMA AXPY

const std = @import("std");

// ============================================================================
// Internal Numeric Validation Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteSliceF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) return false;
    }
    return true;
}

fn clampUnitF32(x: f32): f32 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

fn clampUnitF64(x: f64): f64 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

// ============================================================================
// 1. Cancellation-Safe Descriptive Statistics + SIMD L2 Norm
// ============================================================================

/// Descriptive statistics summary computed by `simdSummaryStatsF32`.
pub struct SimdSummaryStats {
    count: usize = 0;
    min: f32 = 0.0;
    max: f32 = 0.0;
    mean: f64 = 0.0;
    variance: f64 = 0.0;
    stddev: f64 = 0.0;
    l2_norm: f32 = 0.0;
}

/// Computes descriptive statistics (`count`, `min`, `max`, `mean`, population `variance`, `stddev`,
/// and 8-lane SIMD `@dot` `l2_norm`) in zero heap space.
///
/// Uses Welford's online algorithm in `f64` to eliminate catastrophic cancellation on large-offset
/// inputs (e.g. `1e7 + {1, 2, 3}`). Returns `null` on empty input or any `NaN`/`Inf` value.
pub fn simdSummaryStatsF32(data: []const f32): SimdSummaryStats? {
    if (data.len == 0) return null;

    let min_v: f32 = data[0];
    let max_v: f32 = data[0];
    let mean: f64 = 0.0;
    let m2: f64 = 0.0;

    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const v: f32 = data[i];
        if (!isFiniteF32(v)) return null;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;

        const vf: f64 = v as f64;
        const n_f: f64 = (i + 1) as f64;
        const delta: f64 = vf - mean;
        mean += delta / n_f;
        const delta2: f64 = vf - mean;
        m2 = @fma(delta, delta2, m2);
    }

    const norm_sq: f32 = @dot(data, data);
    if (!isFiniteF32(norm_sq)) return null;
    const l2_norm: f32 = if (norm_sq > 0.0) @sqrt(norm_sq) else 0.0;

    let variance: f64 = m2 / (data.len as f64);
    if (variance < 0.0) variance = 0.0;
    const stddev: f64 = std.math.sqrt(variance);

    return SimdSummaryStats{
        count: data.len,
        min: min_v,
        max: max_v,
        mean: mean,
        variance: variance,
        stddev: stddev,
        l2_norm: l2_norm,
    };
}

// ============================================================================
// 2. Correlation, Cosine Similarity & Euclidean Distance
// ============================================================================

/// Computes Pearson correlation coefficient `r in [-1.0, 1.0]` between two equal-length `f32` slices
/// using cancellation-safe centered `f64` `@fma` accumulation.
/// Returns `null` on empty input, length mismatch, `NaN`/`Inf`, or zero-variance input.
pub fn simdPearsonCorrF32(a: []const f32, b: []const f32): f64? {
    if (a.len == 0 || a.len != b.len) return null;

    let sum_a: f64 = 0.0;
    let sum_b: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (!isFiniteF32(a[i]) || !isFiniteF32(b[i])) return null;
        sum_a += a[i] as f64;
        sum_b += b[i] as f64;
    }

    const n_f: f64 = a.len as f64;
    const mean_a: f64 = sum_a / n_f;
    const mean_b: f64 = sum_b / n_f;

    let cov: f64 = 0.0;
    let var_a: f64 = 0.0;
    let var_b: f64 = 0.0;
    i = 0;
    while (i < a.len) : (i += 1) {
        const da: f64 = (a[i] as f64) - mean_a;
        const db: f64 = (b[i] as f64) - mean_b;
        cov = @fma(da, db, cov);
        var_a = @fma(da, da, var_a);
        var_b = @fma(db, db, var_b);
    }

    if (var_a <= 0.0 || var_b <= 0.0) return null;
    const denom: f64 = std.math.sqrt(var_a) * std.math.sqrt(var_b);
    if (!(denom > 0.0) || !isFiniteF64(denom)) return null;
    return clampUnitF64(cov / denom);
}

/// Computes 8-lane SIMD cosine similarity via `@dot(a, b) / (sqrt(@dot(a, a)) * sqrt(@dot(b, b)))`.
/// Returns `null` on empty input, length mismatch, `NaN`/`Inf`, or zero-norm vector.
pub fn simdCosineSimF32(a: []const f32, b: []const f32): f32? {
    if (a.len == 0 || a.len != b.len) return null;
    if (!isFiniteSliceF32(a) || !isFiniteSliceF32(b)) return null;

    const na: f32 = @dot(a, a);
    const nb: f32 = @dot(b, b);
    if (!(na > 0.0) || !(nb > 0.0) || !isFiniteF32(na) || !isFiniteF32(nb)) return null;

    const ab: f32 = @dot(a, b);
    const denom: f32 = @sqrt(na) * @sqrt(nb);
    if (!(denom > 0.0) || !isFiniteF32(ab) || !isFiniteF32(denom)) return null;
    return clampUnitF32(ab / denom);
}

/// Computes 8-lane SIMD Euclidean (L2) distance `sqrt(@l2sq(a, b))`.
/// Returns `null` on empty input, length mismatch, or `NaN`/`Inf`.
pub fn simdEuclideanDistF32(a: []const f32, b: []const f32): f32? {
    if (a.len == 0 || a.len != b.len) return null;
    if (!isFiniteSliceF32(a) || !isFiniteSliceF32(b)) return null;

    let d2: f32 = @l2sq(a, b);
    if (!isFiniteF32(d2)) return null;
    if (d2 < 0.0) d2 = 0.0;
    return @sqrt(d2);
}

// ============================================================================
// 3. Vector Transforms, Overflow-Safe Softmax, Row-Major GEMV & FMA AXPY
// ============================================================================

/// Zero-heap Z-Score standardization: `out[i] = (src[i] - mean) / stddev`.
/// Returns `false` on empty input, undersized `out`, `NaN`/`Inf`, or zero-variance input.
pub fn simdZScoreNormalizeF32(src: []const f32, out: *[]f32): bool {
    if (src.len == 0 || out.len < src.len) return false;
    const st = simdSummaryStatsF32(src) ?? return false;
    if (!(st.stddev > 0.0) || !isFiniteF64(st.stddev)) return false;

    const inv_std: f64 = 1.0 / st.stddev;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[i] = (((src[i] as f64) - st.mean) * inv_std) as f32;
    }
    return true;
}

/// Zero-heap numerically stable Softmax (`exp(x[i] - max(x)) / sum(exp(x - max(x)))`).
/// Prevents `exp` overflow on large logits (e.g. `1000.0`). Returns `false` on empty/undersized/non-finite input.
pub fn simdSoftmaxF32(logits: []const f32, out: *[]f32): bool {
    if (logits.len == 0 || out.len < logits.len) return false;

    let max_v: f32 = logits[0];
    if (!isFiniteF32(max_v)) return false;
    let i: usize = 1;
    while (i < logits.len) : (i += 1) {
        const v: f32 = logits[i];
        if (!isFiniteF32(v)) return false;
        if (v > max_v) max_v = v;
    }

    let sum_exp: f64 = 0.0;
    i = 0;
    while (i < logits.len) : (i += 1) {
        const e: f64 = std.math.exp((logits[i] as f64) - (max_v as f64));
        out[i] = e as f32;
        sum_exp += e;
    }

    if (!(sum_exp > 0.0) || !isFiniteF64(sum_exp)) return false;
    const inv_sum: f64 = 1.0 / sum_exp;
    i = 0;
    while (i < logits.len) : (i += 1) {
        out[i] = ((out[i] as f64) * inv_sum) as f32;
    }
    return true;
}

/// Zero-heap row-major matrix-vector multiply (GEMV): `out_y[r] = @dot(mat[r, :], x)`.
/// Uses CP's native `slice.split(cols)` for zero-overhead row traversal and 8-lane SIMD `@dot`.
/// Returns `false` on zero dimensions, length/buffer mismatch, overflow, or `NaN`/`Inf`.
pub fn simdMatVecF32(
    mat_flat: []const f32,
    rows: usize,
    cols: usize,
    x: []const f32,
    out_y: *[]f32,
): bool {
    if (rows == 0 || cols == 0) return false;
    if (x.len != cols || out_y.len < rows) return false;
    if (rows > std.math.maxInt(usize) / cols) return false;
    const total: usize = rows * cols;
    if (mat_flat.len < total) return false;
    if (!isFiniteSliceF32(x)) return false;

    let rem: []const f32 = mat_flat[0..total];
    let r: usize = 0;
    while (r < rows) : (r += 1) {
        const [row, next_rem] = rem.split(cols);
        if (!isFiniteSliceF32(row)) return false;
        const dot_val: f32 = @dot(row, x);
        if (!isFiniteF32(dot_val)) return false;
        out_y[r] = dot_val;
        rem = next_rem;
    }
    return true;
}

/// In-place single-rounding hardware FMA AXPY: `y[i] = @fma(alpha, x[i], y[i])`.
/// Returns `false` on empty input, length mismatch, or non-finite `alpha`/`x`/`y`.
pub fn simdAxpyF32(alpha: f32, x: []const f32, y: *[]f32): bool {
    if (x.len == 0 || x.len != y.len || !isFiniteF32(alpha)) return false;
    if (!isFiniteSliceF32(x)) return false;
    let chk: usize = 0;
    while (chk < y.len) : (chk += 1) {
        if (!isFiniteF32(y[chk])) return false;
    }

    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        y[i] = @fma(alpha, x[i], y[i]);
    }
    return true;
}
