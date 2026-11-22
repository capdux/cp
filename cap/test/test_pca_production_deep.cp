// Source: cp/cap/test/test_pca_production_deep.cp (Production deep tests for cap:pca)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    jacobiEigenSymmetric,
    PcaFitResult,
    pcaFit,
    pcaTransform,
    pcaInverseTransform,
    pcaReconstructionMse,
} from "cap:math";

fn absDiff(a: f64, b: f64): f64 {
    const d = a - b;
    return if (d < 0.0) 0.0 - d else d;
}

test("1. 4x4 symmetric Jacobi eigendecomposition: A * v == lambda * v, V^T * V == I & trace conservation", () => {
    const orig: [16]f64 = [
        4.0, 1.0, -2.0, 2.0,
        1.0, 2.0, 0.0, 1.0,
        -2.0, 0.0, 3.0, -2.0,
        2.0, 1.0, -2.0, -1.0,
    ];
    let work: [16]f64 = orig;
    let evals: [4]f64 = std.mem.zeroes([4]f64);
    let evecs: [16]f64 = std.mem.zeroes([16]f64);

    const sweeps = jacobiEigenSymmetric(work[0..], 4, evals[0..], evecs[0..], 100, 1.0e-14) ?? unreachable;
    expect_eq(sweeps > 0 && sweeps < 100, true);

    // Eigenvalues strictly descending
    expect_eq(evals[0] >= evals[1], true);
    expect_eq(evals[1] >= evals[2], true);
    expect_eq(evals[2] >= evals[3], true);

    // Trace conservation: sum(evals) == 4 + 2 + 3 + (-1) = 8.0
    const trace = evals[0] + evals[1] + evals[2] + evals[3];
    expect_eq(absDiff(trace, 8.0) < 1.0e-12, true);

    // Verify A * v_j == lambda_j * v_j for all 4 eigenpairs
    let j: usize = 0;
    while (j < 4) {
        const lam = evals[j];
        let i: usize = 0;
        while (i < 4) {
            let av: f64 = 0.0;
            let k: usize = 0;
            while (k < 4) {
                av = av + orig[i * 4 + k] * evecs[k * 4 + j];
                k += 1;
            }
            const lv = lam * evecs[i * 4 + j];
            expect_eq(absDiff(av, lv) < 1.0e-12, true);
            i += 1;
        }
        j += 1;
    }

    // Verify orthonormality: V^T * V == I
    let a: usize = 0;
    while (a < 4) {
        let b: usize = 0;
        while (b < 4) {
            let dot: f64 = 0.0;
            let r: usize = 0;
            while (r < 4) {
                dot = dot + evecs[r * 4 + a] * evecs[r * 4 + b];
                r += 1;
            }
            const target = if (a == b) 1.0 else 0.0;
            expect_eq(absDiff(dot, target) < 1.0e-12, true);
            b += 1;
        }
        a += 1;
    }
});

test("2. 2D 45-degree tilted ellipse point cloud: exact principal direction & variance ratios", () => {
    // 8 points symmetrically placed along rotated major axis u = (1, 1)/sqrt(2) (scale 4)
    // and minor axis v = (-1, 1)/sqrt(2) (scale 1), shifted by center (10, -5).
    const inv_s2 = 1.0 / std.math.sqrt(2.0);
    const maj: [8]f64 = [4.0, -4.0, 4.0, -4.0, 2.0, -2.0, 0.0, 0.0];
    const min_c: [8]f64 = [0.0, 0.0, 1.0, -1.0, -1.0, 1.0, 1.0, -1.0];
    // Note: sum(maj*min_c) = 0 + 0 + 4 + 4 - 2 - 2 + 0 + 0 = 4 -> let's make them strictly orthogonal!
    // Use exact symmetric pairs so sum(maj * min_c) == 0:
    const maj_orth: [8]f64 = [4.0, -4.0, 4.0, -4.0, 0.0, 0.0, 0.0, 0.0];
    const min_orth: [8]f64 = [1.0, -1.0, -1.0, 1.0, 1.0, -1.0, 1.0, -1.0];
    // sum(maj_orth^2) = 64, sum(min_orth^2) = 8, sum(maj_orth * min_orth) = 0
    // Ratio of variances: 64 / (64 + 8) = 8/9, 8 / 72 = 1/9!

    let data: [16]f64 = std.mem.zeroes([16]f64);
    let i: usize = 0;
    while (i < 8) {
        data[i * 2 + 0] = 10.0 + (maj_orth[i] - min_orth[i]) * inv_s2;
        data[i * 2 + 1] = -5.0 + (maj_orth[i] + min_orth[i]) * inv_s2;
        i += 1;
    }
    * = maj[0];
    * = min_c[0];

    let mean: [2]f64 = std.mem.zeroes([2]f64);
    let comps: [4]f64 = std.mem.zeroes([4]f64);
    let evar: [2]f64 = std.mem.zeroes([2]f64);
    let eratio: [2]f64 = std.mem.zeroes([2]f64);
    let cov: [4]f64 = std.mem.zeroes([4]f64);
    let evals: [2]f64 = std.mem.zeroes([2]f64);
    let evecs: [4]f64 = std.mem.zeroes([4]f64);

    const res = pcaFit(
        data[0..], 8, 2, 2,
        mean[0..], comps[0..], evar[0..], eratio[0..],
        cov[0..], evals[0..], evecs[0..], 50, 1.0e-14,
    ) ?? unreachable;

    expect_eq(res.converged, true);
    expect_eq(absDiff(mean[0], 10.0) < 1.0e-12, true);
    expect_eq(absDiff(mean[1], -5.0) < 1.0e-12, true);
    expect_eq(absDiff(eratio[0], 8.0 / 9.0) < 1.0e-12, true);
    expect_eq(absDiff(eratio[1], 1.0 / 9.0) < 1.0e-12, true);
    expect_eq(absDiff(res.explainedRatioSum, 1.0) < 1.0e-12, true);

    // First component must align with (1/sqrt(2), 1/sqrt(2)) up to sign
    const dot_u = comps[0] * inv_s2 + comps[1] * inv_s2;
    const abs_dot_u = if (dot_u < 0.0) 0.0 - dot_u else dot_u;
    expect_eq(absDiff(abs_dot_u, 1.0) < 1.0e-12, true);
});

test("3. Full-rank PCA (n_components == n_features) achieves near-zero reconstruction MSE < 1e-24", () => {
    const data: [18]f64 = [
        1.2, -0.5, 3.4,
        2.8, 1.1, 2.0,
        -0.4, 2.3, 1.5,
        4.1, -1.7, 0.2,
        0.9, 0.4, -2.1,
        3.5, 2.9, 4.7,
    ];
    let mean: [3]f64 = std.mem.zeroes([3]f64);
    let comps: [9]f64 = std.mem.zeroes([9]f64);
    let evar: [3]f64 = std.mem.zeroes([3]f64);
    let eratio: [3]f64 = std.mem.zeroes([3]f64);
    let cov: [9]f64 = std.mem.zeroes([9]f64);
    let evals: [3]f64 = std.mem.zeroes([3]f64);
    let evecs: [9]f64 = std.mem.zeroes([9]f64);

    const res = pcaFit(
        data[0..], 6, 3, 3,
        mean[0..], comps[0..], evar[0..], eratio[0..],
        cov[0..], evals[0..], evecs[0..], 100, 1.0e-14,
    ) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(absDiff(res.explainedRatioSum, 1.0) < 1.0e-12, true);

    let proj: [18]f64 = std.mem.zeroes([18]f64);
    let recon: [18]f64 = std.mem.zeroes([18]f64);
    expect_eq(pcaTransform(data[0..], 6, 3, mean[0..], comps[0..], 3, proj[0..]), true);
    expect_eq(pcaInverseTransform(proj[0..], 6, 3, mean[0..], comps[0..], 3, recon[0..]), true);

    const mse = pcaReconstructionMse(data[0..], recon[0..], 18);
    expect_eq(mse < 1.0e-24, true);
});

test("4. Dimensionality reduction reconstruction error equals exact sum of discarded eigenvalues", () => {
    // 8 samples x 4 features -> compress to 2 components
    const data: [32]f64 = [
        2.1, 4.0, 0.5, -1.2,
        -1.3, -2.5, 1.8, 0.9,
        3.4, 6.2, -0.7, -2.1,
        0.5, 1.1, 2.2, 1.5,
        -2.8, -5.1, -1.4, 0.3,
        1.7, 3.3, 0.9, -0.6,
        -0.9, -1.8, -2.5, 2.0,
        4.0, 7.5, 1.2, -3.0,
    ];
    let mean: [4]f64 = std.mem.zeroes([4]f64);
    let comps: [8]f64 = std.mem.zeroes([8]f64); // 2 x 4
    let evar: [2]f64 = std.mem.zeroes([2]f64);
    let eratio: [2]f64 = std.mem.zeroes([2]f64);
    let cov: [16]f64 = std.mem.zeroes([16]f64);
    let evals: [4]f64 = std.mem.zeroes([4]f64);
    let evecs: [16]f64 = std.mem.zeroes([16]f64);

    const res = pcaFit(
        data[0..], 8, 4, 2,
        mean[0..], comps[0..], evar[0..], eratio[0..],
        cov[0..], evals[0..], evecs[0..], 100, 1.0e-14,
    ) ?? unreachable;
    expect_eq(res.converged, true);

    let proj: [16]f64 = std.mem.zeroes([16]f64); // 8 x 2
    let recon: [32]f64 = std.mem.zeroes([32]f64); // 8 x 4
    expect_eq(pcaTransform(data[0..], 8, 4, mean[0..], comps[0..], 2, proj[0..]), true);
    expect_eq(pcaInverseTransform(proj[0..], 8, 2, mean[0..], comps[0..], 4, recon[0..]), true);

    // Exact spectral identity:
    // (1 / (N - 1)) * sum_i ||x_i - x_hat_i||^2 == sum_{j=k}^{d-1} lambda_j == totalVariance - explainedVarianceSum
    const mse = pcaReconstructionMse(data[0..], recon[0..], 32);
    const recon_var_loss = mse * (32.0 / 7.0);
    const discarded_eigen_sum = res.totalVariance - res.explainedVarianceSum;
    expect_eq(absDiff(recon_var_loss, discarded_eigen_sum) < 1.0e-12, true);
    expect_eq(absDiff(discarded_eigen_sum, evals[2] + evals[3]) < 1.0e-12, true);
});

test("5. Degenerate constant features (zero variance) handled without NaN", () => {
    const data: [12]f64 = [
        7.0, -3.0, 2.5,
        7.0, -3.0, 2.5,
        7.0, -3.0, 2.5,
        7.0, -3.0, 2.5,
    ];
    let mean: [3]f64 = std.mem.zeroes([3]f64);
    let comps: [6]f64 = std.mem.zeroes([6]f64);
    let evar: [2]f64 = std.mem.zeroes([2]f64);
    let eratio: [2]f64 = std.mem.zeroes([2]f64);
    let cov: [9]f64 = std.mem.zeroes([9]f64);
    let evals: [3]f64 = std.mem.zeroes([3]f64);
    let evecs: [9]f64 = std.mem.zeroes([9]f64);

    const res = pcaFit(
        data[0..], 4, 3, 2,
        mean[0..], comps[0..], evar[0..], eratio[0..],
        cov[0..], evals[0..], evecs[0..], 50, 1.0e-12,
    ) ?? unreachable;

    expect_eq(res.converged, true);
    expect_eq(res.totalVariance, 0.0);
    expect_eq(res.explainedVarianceSum, 0.0);
    expect_eq(res.explainedRatioSum, 0.0);
    expect_eq(eratio[0], 0.0);
    expect_eq(eratio[1], 0.0);

    let proj: [8]f64 = std.mem.zeroes([8]f64);
    let recon: [12]f64 = std.mem.zeroes([12]f64);
    expect_eq(pcaTransform(data[0..], 4, 3, mean[0..], comps[0..], 2, proj[0..]), true);
    expect_eq(pcaInverseTransform(proj[0..], 4, 2, mean[0..], comps[0..], 3, recon[0..]), true);
    expect_eq(pcaReconstructionMse(data[0..], recon[0..], 12), 0.0);
});

test("6. Boundary guards: n_samples < 2, n_components > n_features, undersized buffers & NaN rejection", () => {
    const data: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0]; // 3 x 2
    let mean: [2]f64 = std.mem.zeroes([2]f64);
    let comps: [4]f64 = std.mem.zeroes([4]f64);
    let evar: [2]f64 = std.mem.zeroes([2]f64);
    let eratio: [2]f64 = std.mem.zeroes([2]f64);
    let cov: [4]f64 = std.mem.zeroes([4]f64);
    let evals: [2]f64 = std.mem.zeroes([2]f64);
    let evecs: [4]f64 = std.mem.zeroes([4]f64);
    let small1: [1]f64 = std.mem.zeroes([1]f64);

    // n_samples < 2 rejected
    expect_eq(pcaFit(data[0..], 1, 2, 1, mean[0..], comps[0..], evar[0..], eratio[0..], cov[0..], evals[0..], evecs[0..], 50, 1.0e-12) == null, true);
    // n_components == 0 or > n_features rejected
    expect_eq(pcaFit(data[0..], 3, 2, 0, mean[0..], comps[0..], evar[0..], eratio[0..], cov[0..], evals[0..], evecs[0..], 50, 1.0e-12) == null, true);
    expect_eq(pcaFit(data[0..], 3, 2, 3, mean[0..], comps[0..], evar[0..], eratio[0..], cov[0..], evals[0..], evecs[0..], 50, 1.0e-12) == null, true);
    // tol <= 0 rejected
    expect_eq(pcaFit(data[0..], 3, 2, 1, mean[0..], comps[0..], evar[0..], eratio[0..], cov[0..], evals[0..], evecs[0..], 50, 0.0) == null, true);
    // Undersized buffers rejected
    expect_eq(pcaFit(data[0..], 3, 2, 2, small1[0..], comps[0..], evar[0..], eratio[0..], cov[0..], evals[0..], evecs[0..], 50, 1.0e-12) == null, true);
    expect_eq(pcaFit(data[0..], 3, 2, 2, mean[0..], comps[0..], small1[0..], eratio[0..], cov[0..], evals[0..], evecs[0..], 50, 1.0e-12) == null, true);

    // NaN data rejected
    const nan_data: [6]f64 = [1.0, std.math.nan(f64), 3.0, 4.0, 5.0, 6.0];
    expect_eq(pcaFit(nan_data[0..], 3, 2, 1, mean[0..], comps[0..], evar[0..], eratio[0..], cov[0..], evals[0..], evecs[0..], 50, 1.0e-12) == null, true);

    // Jacobi NaN & n == 0 rejected
    let nan_sym: [4]f64 = [1.0, std.math.nan(f64), 0.0, 1.0];
    expect_eq(jacobiEigenSymmetric(nan_sym[0..], 2, evals[0..], evecs[0..], 50, 1.0e-12) == null, true);
    expect_eq(jacobiEigenSymmetric(cov[0..], 0, evals[0..], evecs[0..], 50, 1.0e-12) == null, true);
    expect_eq(jacobiEigenSymmetric(cov[0..], 2, evals[0..], evecs[0..], 50, 0.0) == null, true);

    // pcaTransform / pcaInverseTransform undersized buffer rejected
    expect_eq(pcaTransform(data[0..], 3, 2, mean[0..], comps[0..], 2, small1[0..]), false);
    expect_eq(pcaInverseTransform(data[0..], 3, 2, mean[0..], comps[0..], 2, small1[0..]), false);
});
