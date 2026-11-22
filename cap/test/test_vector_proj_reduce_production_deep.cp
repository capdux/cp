// Origin: Pure-CP Stdlib Test Suite (lib/vector/proj_reduce.cp) | Created: 2026-11-22
// Production Deep Verification Suite for embedding dimensionality reduction & random projection:
// mean centering zero-mean check, power-iteration Top-2 principal axes orthonormality & descending
// variance, @dot projection reconstruction error bound, JL random projection distance preservation
// & determinism, and invalid dimension / empty input defensive guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    computeMeanF32,
    subtractMeanInPlaceF32,
    fitTopKPrincipalAxesF32,
    projectEmbeddingsF32,
    jlRandomProjectF32,
} from "cap:vector";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

fn nextRandU64(state: *u64): u64 {
    const c_add: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
    const c_m1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
    const c_m2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
    state.* = state.* +% c_add;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% c_m1;
    z = (z ^ (z >> 27)) *% c_m2;
    return z ^ (z >> 31);
}

fn nextRandUnitF32(state: *u64): f32 {
    const r: u64 = nextRandU64(state) >> 40;
    const u: f32 = (r as f32) / 16777216.0;
    return (u * 2.0) - 1.0;
}

// ============================================================================
// Test 1: Mean Computation & In-Place Mean Centering (Zero-Mean Verification)
// ============================================================================

fn testMeanCenteringZeroMean()! {
    const n: usize = 6;
    const dim: usize = 4;
    let data: [24]f32 = [
        10.0, -4.0,  2.0,  1.0,
        12.0, -2.0,  4.0,  3.0,
        14.0,  0.0,  6.0,  5.0,
        16.0,  2.0,  8.0,  7.0,
        18.0,  4.0, 10.0,  9.0,
        20.0,  6.0, 12.0, 11.0,
    ];
    let mean: [4]f32 = [0.0; 4];
    let mean_s: []f32 = mean[0..];
    let data_s: []f32 = data[0..];

    expect_eq(cap.vector.computeMeanF32(data[0..], n, dim, mean_s), true);
    expect_eq(absF32(mean[0] - 15.0) < 0.0001, true);
    expect_eq(absF32(mean[1] - 1.0) < 0.0001, true);
    expect_eq(absF32(mean[2] - 7.0) < 0.0001, true);
    expect_eq(absF32(mean[3] - 6.0) < 0.0001, true);

    expect_eq(cap.vector.subtractMeanInPlaceF32(data_s, n, dim, mean[0..]), true);

    let after_mean: [4]f32 = [0.0; 4];
    let after_s: []f32 = after_mean[0..];
    expect_eq(computeMeanF32(data[0..], n, dim, after_s), true);

    let d: usize = 0;
    while (d < dim) : (d += 1) {
        expect_eq(absF32(after_mean[d]) < 0.0001, true);
    }
}

// ============================================================================
// Test 2: Power-Iteration Top-2 Principal Axes Orthonormality & Variance Order
// ============================================================================

fn testPowerIterTop2OrthonormalityAndDescendingVariance()! {
    const n: usize = 32;
    const dim: usize = 8;

    // Two true orthonormal signal directions in 8D:
    // dir0 = (1, 1, 0, 0, 0, 0, 0, 0) / sqrt(2)
    // dir1 = (0, 0, 1, -1, 0, 0, 0, 0) / sqrt(2)
    const inv_s2: f32 = 1.0 / @sqrt(2.0 as f32);
    const dir0: [8]f32 = [inv_s2, inv_s2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const dir1: [8]f32 = [0.0, 0.0, inv_s2, (0.0 as f32) - inv_s2, 0.0, 0.0, 0.0, 0.0];

    let data: [256]f32 = [0.0; 256];
    let rng: u64 = 0x202611220001;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        // Strong variance along dir0 (~5.0), moderate along dir1 (~2.0), tiny noise elsewhere (~0.05).
        const c0: f32 = (((i % 8) as f32) - 3.5) * 2.0;
        const c1: f32 = ((((i / 8) as f32) - 1.5)) * 1.6;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            data[i * dim + d] = c0 * dir0[d] + c1 * dir1[d] + nextRandUnitF32(&rng) * 0.04;
        }
    }

    let mean: [8]f32 = [0.0; 8];
    let mean_s: []f32 = mean[0..];
    let data_s: []f32 = data[0..];
    expect_eq(computeMeanF32(data[0..], n, dim, mean_s), true);
    expect_eq(subtractMeanInPlaceF32(data_s, n, dim, mean[0..]), true);

    let axes: [16]f32 = [0.0; 16]; // 2 * 8
    let evals: [2]f32 = [0.0; 2];
    let scratch: [8]f32 = [0.0; 8];
    let axes_s: []f32 = axes[0..];
    let evals_s: []f32 = evals[0..];
    let scratch_s: []f32 = scratch[0..];

    expect_eq(cap.vector.fitTopKPrincipalAxesF32(data[0..], n, dim, 2, 50, 42, axes_s, evals_s, scratch_s), true);

    const a0: []const f32 = axes[0..8];
    const a1: []const f32 = axes[8..16];

    // Unit norm & strict orthogonality.
    expect_eq(absF32(@dot(a0, a0) - 1.0) < 0.0005, true);
    expect_eq(absF32(@dot(a1, a1) - 1.0) < 0.0005, true);
    expect_eq(absF32(@dot(a0, a1)) < 0.0001, true);

    // Strictly descending explained variances & true axis recovery.
    const dir0_s: []const f32 = dir0[0..];
    const dir1_s: []const f32 = dir1[0..];
    expect_eq(evals[0] > evals[1], true);
    expect_eq(evals[1] > 1.0, true);
    expect_eq(absF32(@dot(a0, dir0_s)) > 0.99, true);
    expect_eq(absF32(@dot(a1, dir1_s)) > 0.99, true);
}

// ============================================================================
// Test 3: @dot SIMD Projection & Low-Rank Reconstruction Error Bound
// ============================================================================

fn testSimdDotProjectionAndReconstructionErrorBound()! {
    const n: usize = 24;
    const in_dim: usize = 8;
    const out_dim: usize = 2;

    const inv_s2: f32 = 1.0 / @sqrt(2.0 as f32);
    const dir0: [8]f32 = [inv_s2, 0.0, inv_s2, 0.0, 0.0, 0.0, 0.0, 0.0];
    const dir1: [8]f32 = [0.0, inv_s2, 0.0, inv_s2, 0.0, 0.0, 0.0, 0.0];

    let data: [192]f32 = [0.0; 192];
    let rng: u64 = 0x554433221100;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const c0: f32 = nextRandUnitF32(&rng) * 5.0;
        const c1: f32 = nextRandUnitF32(&rng) * 2.5;
        let d: usize = 0;
        while (d < in_dim) : (d += 1) {
            data[i * in_dim + d] = c0 * dir0[d] + c1 * dir1[d] + nextRandUnitF32(&rng) * 0.05;
        }
    }

    let mean: [8]f32 = [0.0; 8];
    let mean_s: []f32 = mean[0..];
    let data_s: []f32 = data[0..];
    expect_eq(computeMeanF32(data[0..], n, in_dim, mean_s), true);
    expect_eq(subtractMeanInPlaceF32(data_s, n, in_dim, mean[0..]), true);

    let axes: [16]f32 = [0.0; 16];
    let evals: [2]f32 = [0.0; 2];
    let scratch: [8]f32 = [0.0; 8];
    let axes_s: []f32 = axes[0..];
    let evals_s: []f32 = evals[0..];
    let scratch_s: []f32 = scratch[0..];
    expect_eq(fitTopKPrincipalAxesF32(data[0..], n, in_dim, out_dim, 50, 99, axes_s, evals_s, scratch_s), true);

    let proj: [48]f32 = [0.0; 48]; // 24 * 2
    let proj_s: []f32 = proj[0..];
    expect_eq(cap.vector.projectEmbeddingsF32(data[0..], n, in_dim, axes[0..], out_dim, proj_s), true);

    // Reconstruct each vector from Top-2 projection and verify residual energy < 1%.
    let total_energy: f32 = 0.0;
    let resid_energy: f32 = 0.0;
    let r: usize = 0;
    while (r < n) : (r += 1) {
        const z0: f32 = proj[r * out_dim];
        const z1: f32 = proj[r * out_dim + 1];
        let d: usize = 0;
        while (d < in_dim) : (d += 1) {
            const orig: f32 = data[r * in_dim + d];
            const recon: f32 = z0 * axes[d] + z1 * axes[in_dim + d];
            const diff: f32 = orig - recon;
            total_energy += orig * orig;
            resid_energy += diff * diff;
        }
    }
    expect_eq((resid_energy / total_energy) < 0.01, true);
}

// ============================================================================
// Test 4: JL Sparse Random Projection Distance Preservation & Determinism
// ============================================================================

fn testJlRandomProjectDistancePreservationAndDeterminism()! {
    const in_dim: usize = 128;
    const out_dim: usize = 64;
    const seed: u64 = 0x20260404;

    let va: [128]f32 = [0.0; 128];
    let v_close: [128]f32 = [0.0; 128];
    let v_far: [128]f32 = [0.0; 128];
    let rng: u64 = 0x778899AABBCC;

    let i: usize = 0;
    while (i < in_dim) : (i += 1) {
        const a: f32 = nextRandUnitF32(&rng);
        va[i] = a;
        v_close[i] = a + nextRandUnitF32(&rng) * 0.1;
        v_far[i] = (0.0 as f32) - a + nextRandUnitF32(&rng) * 0.2;
    }

    let pa1: [64]f32 = [0.0; 64];
    let pa2: [64]f32 = [0.0; 64];
    let p_close: [64]f32 = [0.0; 64];
    let p_far: [64]f32 = [0.0; 64];
    let pa1_s: []f32 = pa1[0..];
    let pa2_s: []f32 = pa2[0..];
    let p_close_s: []f32 = p_close[0..];
    let p_far_s: []f32 = p_far[0..];

    expect_eq(cap.vector.jlRandomProjectF32(va[0..], in_dim, out_dim, seed, pa1_s), true);
    expect_eq(jlRandomProjectF32(va[0..], in_dim, out_dim, seed, pa2_s), true);
    expect_eq(jlRandomProjectF32(v_close[0..], in_dim, out_dim, seed, p_close_s), true);
    expect_eq(jlRandomProjectF32(v_far[0..], in_dim, out_dim, seed, p_far_s), true);

    // Exact determinism with identical seed.
    let j: usize = 0;
    while (j < out_dim) : (j += 1) {
        expect_eq(pa1[j], pa2[j]);
    }

    // Approximate norm & pairwise distance preservation.
    const norm_orig: f32 = @sqrt(@dot(va[0..], va[0..]));
    const norm_proj: f32 = @sqrt(@dot(pa1[0..], pa1[0..]));
    const ratio: f32 = norm_proj / norm_orig;
    expect_eq(ratio > 0.65 && ratio < 1.35, true);

    const dist_close: f32 = @l2sq(pa1[0..], p_close[0..]);
    const dist_far: f32 = @l2sq(pa1[0..], p_far[0..]);
    expect_eq(dist_close * 10.0 < dist_far, true);
}

// ============================================================================
// Test 5: Invalid Dimension, Empty & Non-Finite Defensive Guards
// ============================================================================

fn testInvalidDimAndEmptyGuards()! {
    let data: [8]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];
    let data_nan: [4]f32 = [1.0, std.math.nan(f32), 3.0, 4.0];
    let mean: [4]f32 = [0.0; 4];
    let axes: [8]f32 = [0.0; 8];
    let evals: [2]f32 = [0.0; 2];
    let scratch: [4]f32 = [0.0; 4];
    let proj: [4]f32 = [0.0; 4];

    let data_s: []f32 = data[0..];
    let mean_s: []f32 = mean[0..];
    let axes_s: []f32 = axes[0..];
    let evals_s: []f32 = evals[0..];
    let scratch_s: []f32 = scratch[0..];
    let proj_s: []f32 = proj[0..];

    // computeMeanF32 / subtractMeanInPlaceF32 guards
    expect_eq(computeMeanF32(data[0..], 0, 4, mean_s), false);
    expect_eq(computeMeanF32(data[0..], 2, 0, mean_s), false);
    expect_eq(computeMeanF32(data[0..7], 2, 4, mean_s), false);
    expect_eq(computeMeanF32(data_nan[0..], 1, 4, mean_s), false);
    expect_eq(subtractMeanInPlaceF32(data_s, 0, 4, mean[0..]), false);
    expect_eq(subtractMeanInPlaceF32(data_s, 2, 4, data_nan[0..]), false);

    // fitTopKPrincipalAxesF32 guards
    expect_eq(fitTopKPrincipalAxesF32(data[0..], 0, 4, 2, 10, 1, axes_s, evals_s, scratch_s), false);
    expect_eq(fitTopKPrincipalAxesF32(data[0..], 2, 4, 0, 10, 1, axes_s, evals_s, scratch_s), false);
    expect_eq(fitTopKPrincipalAxesF32(data[0..], 2, 4, 5, 10, 1, axes_s, evals_s, scratch_s), false); // k > dim
    expect_eq(fitTopKPrincipalAxesF32(data[0..], 2, 4, 2, 0, 1, axes_s, evals_s, scratch_s), false);  // max_iters == 0
    expect_eq(fitTopKPrincipalAxesF32(data_nan[0..], 1, 4, 1, 10, 1, axes_s, evals_s, scratch_s), false);

    // projectEmbeddingsF32 guards
    expect_eq(projectEmbeddingsF32(data[0..], 0, 4, axes[0..], 2, proj_s), false);
    expect_eq(projectEmbeddingsF32(data[0..], 2, 0, axes[0..], 2, proj_s), false);
    expect_eq(projectEmbeddingsF32(data[0..], 2, 4, axes[0..], 0, proj_s), false);
    expect_eq(projectEmbeddingsF32(data[0..], 2, 4, axes[0..7], 2, proj_s), false);

    // jlRandomProjectF32 guards
    expect_eq(jlRandomProjectF32(data[0..4], 0, 2, 1, proj_s), false);
    expect_eq(jlRandomProjectF32(data[0..4], 4, 0, 1, proj_s), false);
    expect_eq(jlRandomProjectF32(data[0..3], 4, 2, 1, proj_s), false);
    expect_eq(jlRandomProjectF32(data_nan[0..], 4, 2, 1, proj_s), false);
}

fn main() {
    test("1. vector.proj_reduce: mean computation & in-place zero-mean centering", () => {
        testMeanCenteringZeroMean()?;
    });
    test("2. vector.proj_reduce: power-iteration Top-2 axes orthonormality & descending variance", () => {
        testPowerIterTop2OrthonormalityAndDescendingVariance()?;
    });
    test("3. vector.proj_reduce: @dot SIMD projection & low-rank reconstruction error bound", () => {
        testSimdDotProjectionAndReconstructionErrorBound()?;
    });
    test("4. vector.proj_reduce: Achlioptas JL sparse random projection distance preservation", () => {
        testJlRandomProjectDistancePreservationAndDeterminism()?;
    });
    test("5. vector.proj_reduce: invalid dimension, empty and non-finite defensive guards", () => {
        testInvalidDimAndEmptyGuards()?;
    });
}
