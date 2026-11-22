// Origin: Pure-CP Stdlib Test Suite (lib/math/simd_stats.cp) | Created: 2026-11-22
// Production Deep Verification Suite for SIMD Vectorized Statistics & Linear Algebra Facade:
//   1. simdSummaryStatsF32 exact values & large-offset Welford anti-cancellation variance
//   2. simdPearsonCorrF32, simdCosineSimF32 & simdEuclideanDistF32 (8D SIMD)
//   3. simdZScoreNormalizeF32 zero-mean / unit-variance & zero-variance guard
//   4. simdSoftmaxF32 normalization & overflow-safe shift invariance on large logits
//   5. simdMatVecF32 (slice.split + @dot GEMV) & simdAxpyF32 (@fma AXPY)
//   6. Empty, zero-norm, zero-variance, NaN/Inf & dimension-mismatch defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SimdSummaryStats,
    simdSummaryStatsF32,
    simdPearsonCorrF32,
    simdCosineSimF32,
    simdEuclideanDistF32,
    simdZScoreNormalizeF32,
    simdSoftmaxF32,
    simdMatVecF32,
    simdAxpyF32,
} from "cap:math";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

// ============================================================================
// Test 1: Descriptive Stats & Welford Anti-Cancellation on Large Offset
// ============================================================================

fn testSummaryStatsAndAntiCancellation()! {
    const data: [4]f32 = [2.0, 4.0, 4.0, 6.0];
    const st = cap.math.simdSummaryStatsF32(data[0..]) ?? return error.StatsFailed;
    expect_eq(st.count, 4);
    expect_eq(absF32(st.min - 2.0) < 0.0001, true);
    expect_eq(absF32(st.max - 6.0) < 0.0001, true);
    expect_eq(absF64(st.mean - 4.0) < 0.00001, true);
    // Population variance of [2, 4, 4, 6] = (4 + 0 + 0 + 4) / 4 = 2.0
    expect_eq(absF64(st.variance - 2.0) < 0.00001, true);
    expect_eq(absF64(st.stddev - std.math.sqrt(2.0 as f64)) < 0.00001, true);
    // L2 norm = sqrt(4 + 16 + 16 + 36) = sqrt(72)
    expect_eq(absF32(st.l2_norm - @sqrt(72.0 as f32)) < 0.0001, true);

    // Large-offset dataset: 1_000_000.0 + {1, 2, 3, 4} (naive E[X^2]-E[X]^2 in f32 would catastrophically cancel!)
    // Exact population variance of {1, 2, 3, 4} = 1.25
    const shifted: [4]f32 = [1000001.0, 1000002.0, 1000003.0, 1000004.0];
    const st_hi = simdSummaryStatsF32(shifted[0..]) ?? return error.StatsFailed;
    expect_eq(absF64(st_hi.mean - 1000002.5) < 0.001, true);
    expect_eq(absF64(st_hi.variance - 1.25) < 0.0001, true);
}

// ============================================================================
// Test 2: Pearson Correlation, SIMD Cosine Similarity & Euclidean Distance
// ============================================================================

fn testCorrelationCosineAndEuclidean()! {
    const a: [8]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];
    const b_pos: [8]f32 = [10.0, 12.0, 14.0, 16.0, 18.0, 20.0, 22.0, 24.0]; // 2*a + 8 -> Pearson = +1.0
    const b_neg: [8]f32 = [8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0];         // -a + 9  -> Pearson = -1.0

    const r_pos = simdPearsonCorrF32(a[0..], b_pos[0..]) ?? return error.PearsonFailed;
    const r_neg = simdPearsonCorrF32(a[0..], b_neg[0..]) ?? return error.PearsonFailed;
    expect_eq(absF64(r_pos - 1.0) < 0.00001, true);
    expect_eq(absF64(r_neg + 1.0) < 0.00001, true);

    // Cosine similarity: identical -> 1.0, orthogonal -> 0.0, opposite -> -1.0
    const e1: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const e2: [8]f32 = [0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const e3: [8]f32 = [-2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    expect_eq(absF32((simdCosineSimF32(e1[0..], e1[0..]) ?? 99.0) - 1.0) < 0.00001, true);
    expect_eq(absF32(simdCosineSimF32(e1[0..], e2[0..]) ?? 99.0) < 0.00001, true);
    expect_eq(absF32((simdCosineSimF32(e1[0..], e3[0..]) ?? 99.0) + 1.0) < 0.00001, true);

    // Euclidean distance: 3-4-5 triangle
    const p1: [2]f32 = [1.0, 2.0];
    const p2: [2]f32 = [4.0, 6.0];
    const dist = simdEuclideanDistF32(p1[0..], p2[0..]) ?? return error.DistFailed;
    expect_eq(absF32(dist - 5.0) < 0.00001, true);
}

// ============================================================================
// Test 3: Z-Score Standardization (Zero Mean, Unit Variance)
// ============================================================================

fn testZScoreNormalize()! {
    const src: [5]f32 = [10.0, 20.0, 30.0, 40.0, 50.0];
    let out: [5]f32 = [0.0; 5];
    expect_eq(simdZScoreNormalizeF32(src[0..], out[0..]), true);

    const st = simdSummaryStatsF32(out[0..]) ?? return error.StatsFailed;
    expect_eq(absF64(st.mean) < 0.0001, true);
    expect_eq(absF64(st.stddev - 1.0) < 0.0001, true);

    // Zero-variance constant vector returns false
    const constant: [4]f32 = [7.0, 7.0, 7.0, 7.0];
    let out4: [4]f32 = [0.0; 4];
    expect_eq(simdZScoreNormalizeF32(constant[0..], out4[0..]), false);
}

// ============================================================================
// Test 4: Overflow-Safe Softmax (Shift Invariance & Sum-to-1)
// ============================================================================

fn testSoftmaxOverflowSafe()! {
    const small: [3]f32 = [0.0, 1.0, 2.0];
    const huge: [3]f32 = [1000.0, 1001.0, 1002.0];
    let p_small: [3]f32 = [0.0; 3];
    let p_huge: [3]f32 = [0.0; 3];

    expect_eq(simdSoftmaxF32(small[0..], p_small[0..]), true);
    expect_eq(simdSoftmaxF32(huge[0..], p_huge[0..]), true);

    let sum_p: f32 = 0.0;
    let i: usize = 0;
    while (i < 3) : (i += 1) {
        expect_eq(absF32(p_small[i] - p_huge[i]) < 0.00001, true);
        sum_p += p_huge[i];
    }
    expect_eq(absF32(sum_p - 1.0) < 0.00001, true);
    expect_eq(p_huge[2] > p_huge[1] && p_huge[1] > p_huge[0], true);
}

// ============================================================================
// Test 5: Row-Major GEMV (`slice.split` + `@dot`) & Hardware `@fma` AXPY
// ============================================================================

fn testMatVecAndAxpy()! {
    // 3x8 matrix * 8-element vector (directly exercises 8-lane SIMD @dot per row)
    const mat: [24]f32 = [
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, // row 0 dot x = 1 + 8 = 9
        2.0, 2.0, 2.0, 2.0, 0.0, 0.0, 0.0, 0.0, // row 1 dot x = 2*(1+2+3+4) = 20
        0.0, 0.0, 0.0, 0.0, 1.0, -1.0, 1.0, -1.0, // row 2 dot x = 5 - 6 + 7 - 8 = -2
    ];
    const x: [8]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];
    let y: [3]f32 = [0.0; 3];

    expect_eq(simdMatVecF32(mat[0..], 3, 8, x[0..], y[0..]), true);
    expect_eq(absF32(y[0] - 9.0) < 0.0001, true);
    expect_eq(absF32(y[1] - 20.0) < 0.0001, true);
    expect_eq(absF32(y[2] + 2.0) < 0.0001, true);

    // AXPY: y = 0.5 * [2, 4, 6] + [9, 20, -2] = [10, 22, 1]
    const dx: [3]f32 = [2.0, 4.0, 6.0];
    expect_eq(simdAxpyF32(0.5, dx[0..], y[0..]), true);
    expect_eq(absF32(y[0] - 10.0) < 0.0001, true);
    expect_eq(absF32(y[1] - 22.0) < 0.0001, true);
    expect_eq(absF32(y[2] - 1.0) < 0.0001, true);
}

// ============================================================================
// Test 6: Defensive Guards (Empty, Zero-Norm, Zero-Var, NaN, Mismatched Dims)
// ============================================================================

fn testDefensiveGuards()! {
    const nan_f32: f32 = std.math.nan(f32);
    const v3: [3]f32 = [1.0, 2.0, 3.0];
    const v2: [2]f32 = [1.0, 2.0];
    const zeros: [3]f32 = [0.0, 0.0, 0.0];
    const consts: [3]f32 = [5.0, 5.0, 5.0];
    const nan_v: [3]f32 = [1.0, nan_f32, 3.0];
    const empty: []const f32 = v3[0..0];
    let out3: [3]f32 = [0.0; 3];
    let out1: [1]f32 = [0.0];

    // Empty / NaN summary stats
    expect_eq(simdSummaryStatsF32(empty) == null, true);
    expect_eq(simdSummaryStatsF32(nan_v[0..]) == null, true);

    // Zero-norm cosine -> null; length mismatch -> null
    expect_eq(simdCosineSimF32(zeros[0..], v3[0..]) == null, true);
    expect_eq(simdCosineSimF32(v3[0..], v2[0..]) == null, true);
    expect_eq(simdCosineSimF32(nan_v[0..], v3[0..]) == null, true);

    // Zero-variance Pearson -> null; length mismatch -> null
    expect_eq(simdPearsonCorrF32(consts[0..], v3[0..]) == null, true);
    expect_eq(simdPearsonCorrF32(v3[0..], v2[0..]) == null, true);

    // Euclidean distance guards
    expect_eq(simdEuclideanDistF32(empty, empty) == null, true);
    expect_eq(simdEuclideanDistF32(v3[0..], v2[0..]) == null, true);
    expect_eq(simdEuclideanDistF32(nan_v[0..], v3[0..]) == null, true);

    // Softmax, GEMV, AXPY guards
    expect_eq(simdSoftmaxF32(empty, out3[0..]), false);
    expect_eq(simdSoftmaxF32(v3[0..], out1[0..]), false);
    expect_eq(simdSoftmaxF32(nan_v[0..], out3[0..]), false);
    expect_eq(simdMatVecF32(v3[0..], 0, 3, v3[0..], out3[0..]), false);
    expect_eq(simdMatVecF32(v3[0..], 2, 3, v3[0..], out3[0..]), false);
    expect_eq(simdAxpyF32(nan_f32, v3[0..], out3[0..]), false);
    expect_eq(simdAxpyF32(1.0, v2[0..], out3[0..]), false);
}

fn main() {
    test("1. math.simd_stats: Welford descriptive stats & anti-cancellation", () => {
        testSummaryStatsAndAntiCancellation()?;
    });
    test("2. math.simd_stats: Pearson correlation, SIMD cosine & Euclidean distance", () => {
        testCorrelationCosineAndEuclidean()?;
    });
    test("3. math.simd_stats: Z-Score standardization & zero-variance guard", () => {
        testZScoreNormalize()?;
    });
    test("4. math.simd_stats: overflow-safe Softmax & shift invariance", () => {
        testSoftmaxOverflowSafe()?;
    });
    test("5. math.simd_stats: slice.split + @dot GEMV & @fma AXPY", () => {
        testMatVecAndAxpy()?;
    });
    test("6. math.simd_stats: defensive guards (empty, zero-norm, NaN, mismatch)", () => {
        testDefensiveGuards()?;
    });
}
