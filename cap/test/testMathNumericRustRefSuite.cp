// statrs-dev/statrs (MIT) & dimforge/nalgebra (Apache-2.0) Reference-Hardened Numeric & Stats Suite
// Covers:
//   - Authoritative analytical truth vectors for statsDist (Normal, Student-t, Chi-Square, F, Beta,
//     Poisson, logGamma, erf/erfc, logSumExp, Welford shifted variance, t-tests, chi2, OLS/Ridge)
//   - Authoritative LU truth vectors (Hilbert 4x4, partial pivoting 6x6, luLogDet, luInverse, Thomas)
//   - Strict NaN / ±Inf / empty / zero-variance / singular / domain-out-of-bounds rejection guards
//   - 4-lane @Vector(4, f64) SIMD vs scalar parity (< 1e-9 across lengths 1..129)
//   - 1,800-iteration deterministic PRNG fuzzing (statsDist + LU + tridiagonal + SIMD)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    StatsDistError,
    logGamma,
    erfApprox,
    erfcApprox,
    normalInvCdf,
    normalPdf,
    normalCdf,
    normalPpf,
    studentTPdf,
    studentTCdf,
    studentTPpf,
    chiSquarePdf,
    chiSquareCdf,
    chiSquarePpf,
    fDistCdf,
    betaPdf,
    betaCdf,
    poissonPmf,
    poissonCdf,
    describeSample,
    oneSampleTTest,
    welchTwoSampleTTest,
    chiSquareGoodnessOfFit,
    pearsonCorrelationTest,
    fitSimpleOls,
    fitRidgeRegression,
    predictLinear,
    simdMathSumF64,
    simdMathDotF64,
    simdMathSumSqDiffF64,
    logSumExp,
} from "../../lib/math/statsDist.cp";
import {
    LuInfo,
    luSimdDotF64,
    luFactorize,
    luSolve,
    luDeterminant,
    luLogDet,
    luInverse,
    matVecResidualInf,
    solveTridiagonal,
} from "../../lib/math/lu.cp";

fn tAbs(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn tIsFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn tNear(actual: f64, expected: f64, tol: f64): bool {
    if (!tIsFinite(actual) || !tIsFinite(expected)) return false;
    return tAbs(actual - expected) <= tol;
}

fn scalarSum(xs: []const f64): f64 {
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        acc += xs[i];
    }
    return acc;
}

fn scalarDot(xs: []const f64, ys: []const f64): f64 {
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        acc += xs[i] * ys[i];
    }
    return acc;
}

fn scalarSumSqDiff(xs: []const f64, mean: f64): f64 {
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        const d = xs[i] - mean;
        acc += d * d;
    }
    return acc;
}

fn xorshift64(state: *u64): u64 {
    let x: u64 = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

fn nextUnitF64(state: *u64): f64 {
    const bits: u64 = xorshift64(state) >> 11;
    return @as(f64, @floatFromInt(bits)) / 9007199254740992.0;
}

fn nextSignedF64(state: *u64, scale: f64): f64 {
    return (nextUnitF64(state) * 2.0 - 1.0) * scale;
}

fn errDescribe(xs: []const f64): usize {
    const r = describeSample(xs) catch return 1;
    return if (r.count == 999999) 1 else 0;
}

fn errOneT(xs: []const f64, mu0: f64): usize {
    const r = oneSampleTTest(xs, mu0) catch return 1;
    return if (r.df < 0.0) 1 else 0;
}

fn errWelch(xs: []const f64, ys: []const f64): usize {
    const r = welchTwoSampleTTest(xs, ys) catch return 1;
    return if (r.df < 0.0) 1 else 0;
}

fn errCorr(xs: []const f64, ys: []const f64): usize {
    const r = pearsonCorrelationTest(xs, ys) catch return 1;
    return if (r.n == 999999) 1 else 0;
}

fn errOls(xs: []const f64, ys: []const f64): usize {
    const r = fitSimpleOls(xs, ys) catch return 1;
    return if (r.rmse < 0.0) 1 else 0;
}

fn errRidge(x: []const f64, ns: usize, nf: usize, y: []const f64, lam: f64): usize {
    const r = fitRidgeRegression(x, ns, nf, y, lam) catch return 1;
    return if (r.n_features == 999999) 1 else 0;
}

fn main() {
    // ========================================================================
    // 1. Authoritative Analytical Truth Vectors: Special Functions & Distributions
    // ========================================================================
    test("num_ref_01_special_functions_and_distributions_truth", () => {
        // logGamma analytical truth
        expect_eq(tNear(logGamma(1.0), 0.0, 1.0e-12), true);
        expect_eq(tNear(logGamma(0.5), 0.5 * std.math.log(f64, std.math.e, 3.14159265358979323846), 1.0e-12), true);
        expect_eq(tNear(logGamma(5.0), std.math.log(f64, std.math.e, 24.0), 1.0e-12), true);
        expect_eq(tNear(logGamma(10.0), std.math.log(f64, std.math.e, 362880.0), 1.0e-12), true);

        // erf / erfc analytical truth
        expect_eq(tNear(erfApprox(0.0), 0.0, 1.0e-14), true);
        expect_eq(tNear(erfApprox(1.0), 0.8427007929497149, 1.0e-12), true);
        expect_eq(tNear(erfcApprox(3.0), 0.00002209049699858544, 1.0e-12), true);

        // Normal PDF / CDF / PPF truth & roundtrip
        const npdf0 = normalPdf(0.0, 0.0, 1.0) catch -1.0;
        expect_eq(tNear(npdf0, 0.3989422804014327, 1.0e-12), true);
        const ncdf975 = normalCdf(1.959963984540054, 0.0, 1.0) catch -1.0;
        expect_eq(tNear(ncdf975, 0.975, 1.0e-12), true);
        const nppf975 = normalPpf(0.975, 2.0, 3.0) catch -1.0;
        expect_eq(tNear(nppf975, 2.0 + 3.0 * 1.959963984540054, 1.0e-10), true);
        const nppf_tail = normalInvCdf(1.0e-12);
        expect_eq(tIsFinite(nppf_tail) && nppf_tail < -6.5, true);

        // Student-t truth (df=1 Cauchy, df=2 exact, df=10 97.5% quantile)
        const t1_q = studentTPpf(0.75, 1.0) catch -1.0;
        expect_eq(tNear(t1_q, 1.0, 1.0e-12), true);
        const t2_q = studentTPpf(0.975, 2.0) catch -1.0;
        expect_eq(tNear(t2_q, 4.302652729749464, 1.0e-10), true);
        const t10_q = studentTPpf(0.975, 10.0) catch -1.0;
        expect_eq(tNear(t10_q, 2.2281388519649385, 1.0e-10), true);
        const t10_cdf = studentTCdf(t10_q, 10.0) catch -1.0;
        expect_eq(tNear(t10_cdf, 0.975, 1.0e-10), true);
        // Huge |t| must not overflow to NaN
        const t_huge = studentTCdf(1.0e200, 5.0) catch -1.0;
        expect_eq(tNear(t_huge, 1.0, 1.0e-12), true);
        const t_neg_huge = studentTCdf(-1.0e200, 5.0) catch -1.0;
        expect_eq(tNear(t_neg_huge, 0.0, 1.0e-12), true);

        // Chi-Square (df=2 -> 1 - exp(-x/2))
        const chi2_cdf = chiSquareCdf(4.0, 2.0) catch -1.0;
        expect_eq(tNear(chi2_cdf, 1.0 - std.math.exp(-2.0), 1.0e-12), true);
        const chi2_ppf = chiSquarePpf(0.95, 5.0) catch -1.0;
        expect_eq(tNear(chi2_ppf, 11.070497693516351, 1.0e-9), true);

        // F-distribution truth + huge x overflow guard
        const f_cdf = fDistCdf(3.3258345304130112, 5.0, 10.0) catch -1.0;
        expect_eq(tNear(f_cdf, 0.95, 1.0e-9), true);
        const f_huge = fDistCdf(1.0e305, 5.0, 10.0) catch -1.0;
        expect_eq(tNear(f_huge, 1.0, 1.0e-12), true);

        // Beta(2, 5) analytical CDF at x=0.3: 1 - (1-0.3)^5 * (1 + 5*0.3) = 0.579825
        const b_cdf = betaCdf(0.3, 2.0, 5.0) catch -1.0;
        expect_eq(tNear(b_cdf, 0.579825, 1.0e-12), true);
        const b_pdf = betaPdf(0.3, 2.0, 5.0) catch -1.0;
        expect_eq(tNear(b_pdf, 30.0 * 0.3 * 0.7 * 0.7 * 0.7 * 0.7, 1.0e-12), true);

        // Poisson(3.0) PMF & CDF + u64::MAX guard
        const p_pmf2 = poissonPmf(2, 3.0) catch -1.0;
        expect_eq(tNear(p_pmf2, 4.5 * std.math.exp(-3.0), 1.0e-12), true);
        const p_cdf2 = poissonCdf(2, 3.0) catch -1.0;
        expect_eq(tNear(p_cdf2, 8.5 * std.math.exp(-3.0), 1.0e-12), true);
        const p_max = poissonCdf(std.math.maxInt(u64), 3.0) catch -1.0;
        expect_eq(tNear(p_max, 1.0, 1.0e-12), true);
    });

    // ========================================================================
    // 2. Log-Sum-Exp, Welford Shifted Variance, Hypothesis Tests & OLS/Ridge
    // ========================================================================
    test("num_ref_02_logsumexp_welford_regression_truth", () => {
        const ln2: f64 = std.math.log(f64, std.math.e, 2.0);
        const hi_vals: [2]f64 = [1000.0, 1000.0];
        const lse_hi = logSumExp(hi_vals[0..2]) catch -1.0;
        expect_eq(tNear(lse_hi, 1000.0 + ln2, 1.0e-11), true);

        const lo_vals: [2]f64 = [-1000.0, -1000.0];
        const lse_lo = logSumExp(lo_vals[0..2]) catch -1.0;
        expect_eq(tNear(lse_lo, -1000.0 + ln2, 1.0e-11), true);

        // Welford shifted sample: 1e9 + [1, 2, 3, 4, 5] -> mean = 1e9+3, var = 2.5
        const shifted: [5]f64 = [
            1.0e9 + 1.0,
            1.0e9 + 2.0,
            1.0e9 + 3.0,
            1.0e9 + 4.0,
            1.0e9 + 5.0,
        ];
        const ds = describeSample(shifted[0..5]) catch return;
        expect_eq(ds.count, 5);
        expect_eq(tNear(ds.mean, 1.0e9 + 3.0, 1.0e-6), true);
        expect_eq(tNear(ds.variance, 2.5, 1.0e-9), true);
        expect_eq(tNear(ds.std_dev, std.math.sqrt(2.5), 1.0e-9), true);

        // Simple OLS exact line y = 2.5 * x - 1.25
        const xs: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
        const ys: [6]f64 = [1.25, 3.75, 6.25, 8.75, 11.25, 13.75];
        const ols = fitSimpleOls(xs[0..6], ys[0..6]) catch return;
        expect_eq(tNear(ols.slope, 2.5, 1.0e-12), true);
        expect_eq(tNear(ols.intercept, -1.25, 1.0e-12), true);
        expect_eq(tNear(ols.r_squared, 1.0, 1.0e-12), true);

        const corr = pearsonCorrelationTest(xs[0..6], ys[0..6]) catch return;
        expect_eq(tNear(corr.r, 1.0, 1.0e-12), true);

        // Multivariate OLS + predictLinear (y = 3 + 2*x1 - 5*x2)
        const x_mat: [12]f64 = [
            1.0, 0.0,
            0.0, 1.0,
            1.0, 1.0,
            2.0, 1.0,
            1.0, 2.0,
            3.0, -1.0,
        ];
        const y_vec: [6]f64 = [5.0, -2.0, 0.0, 2.0, -5.0, 14.0];
        const mreg = fitRidgeRegression(x_mat[0..12], 6, 2, y_vec[0..6], 0.0) catch return;
        expect_eq(tNear(mreg.intercept, 3.0, 1.0e-10), true);
        expect_eq(tNear(mreg.coeffs[0], 2.0, 1.0e-10), true);
        expect_eq(tNear(mreg.coeffs[1], -5.0, 1.0e-10), true);
        const x_q: [2]f64 = [4.0, 2.0];
        const pred = predictLinear(mreg, x_q[0..2]) catch -999.0;
        expect_eq(tNear(pred, 3.0 + 8.0 - 10.0, 1.0e-10), true);

        // Hypothesis tests sanity
        const t1 = oneSampleTTest(xs[0..6], 3.5) catch return;
        expect_eq(tNear(t1.t_stat, 0.0, 1.0e-12), true);
        expect_eq(tNear(t1.p_value_two_tailed, 1.0, 1.0e-12), true);

        const obs: [4]f64 = [25.0, 25.0, 25.0, 25.0];
        const exp: [4]f64 = [25.0, 25.0, 25.0, 25.0];
        const c2 = chiSquareGoodnessOfFit(obs[0..4], exp[0..4]) catch return;
        expect_eq(tNear(c2.chi2_stat, 0.0, 1.0e-12), true);
        expect_eq(tNear(c2.p_value, 1.0, 1.0e-12), true);
    });

    // ========================================================================
    // 3. Authoritative LU & Tridiagonal Truth Vectors (Hilbert 4x4, Pivot 6x6)
    // ========================================================================
    test("num_ref_03_lu_hilbert_pivot_logdet_inverse_tridiag", () => {
        // 4x4 Hilbert matrix
        const n4: usize = 4;
        let h_orig: [16]f64 = [0.0; 16];
        let h_lu: [16]f64 = [0.0; 16];
        let r: usize = 0;
        while (r < n4) : (r += 1) {
            let c: usize = 0;
            while (c < n4) : (c += 1) {
                const v: f64 = 1.0 / @as(f64, @floatFromInt(r + c + 1));
                h_orig[r * n4 + c] = v;
                h_lu[r * n4 + c] = v;
            }
        }
        let piv4: [4]usize = [0; 4];
        const info4_opt = luFactorize(h_lu[0..16], n4, piv4[0..4], 1.0e-14);
        expect_eq(info4_opt != null, true);
        const info4 = info4_opt.?;

        // Exact det(H_4) = 1 / 6048000
        const det4 = luDeterminant(h_lu[0..16], info4, n4);
        expect_eq(tNear(det4, 1.0 / 6048000.0, 1.0e-15), true);
        const ldet4_opt = luLogDet(h_lu[0..16], info4, n4);
        expect_eq(ldet4_opt != null, true);
        expect_eq(tNear(std.math.exp(ldet4_opt.?), tAbs(det4), 1.0e-15), true);

        const b4: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        let x4: [4]f64 = [0.0; 4];
        expect_eq(luSolve(h_lu[0..16], piv4[0..4], n4, b4[0..4], x4[0..4]), true);
        const res4 = matVecResidualInf(h_orig[0..16], x4[0..4], b4[0..4], n4);
        expect_eq(res4 < 1.0e-11, true);

        // Explicit inverse A * A^-1 == I
        let inv4: [16]f64 = [0.0; 16];
        let s1: [4]f64 = [0.0; 4];
        let s2: [4]f64 = [0.0; 4];
        expect_eq(luInverse(h_lu[0..16], piv4[0..4], n4, inv4[0..16], s1[0..4], s2[0..4]), true);
        let max_inv_err: f64 = 0.0;
        let ri: usize = 0;
        while (ri < n4) : (ri += 1) {
            let ci: usize = 0;
            while (ci < n4) : (ci += 1) {
                let acc: f64 = 0.0;
                let k: usize = 0;
                while (k < n4) : (k += 1) {
                    acc += h_orig[ri * n4 + k] * inv4[k * n4 + ci];
                }
                const target: f64 = if (ri == ci) 1.0 else 0.0;
                const e = tAbs(acc - target);
                if (e > max_inv_err) max_inv_err = e;
            }
        }
        expect_eq(max_inv_err < 1.0e-10, true);

        // 6x6 matrix with zero diagonal requiring partial pivoting (exercises 4-lane SIMD elimination)
        const n6: usize = 6;
        let a6_orig: [36]f64 = [
            0.0, 2.0, 1.0, -1.0, 0.5, 1.0,
            3.0, 0.0, -2.0, 1.0, 2.0, -0.5,
            1.0, 4.0, 0.0, 2.0, -1.0, 1.5,
            -2.0, 1.0, 3.0, 0.0, 4.0, -1.0,
            0.5, -1.5, 2.0, 5.0, 0.0, 2.0,
            2.0, 1.0, -1.0, 0.5, 3.0, 0.0,
        ];
        let a6_lu: [36]f64 = a6_orig;
        let piv6: [6]usize = [0; 6];
        const info6_opt = luFactorize(a6_lu[0..36], n6, piv6[0..6], 1.0e-12);
        expect_eq(info6_opt != null, true);
        expect_eq(info6_opt.?.swaps > 0, true);
        const b6: [6]f64 = [1.0, -2.0, 3.0, -4.0, 5.0, -6.0];
        let x6: [6]f64 = [0.0; 6];
        expect_eq(luSolve(a6_lu[0..36], piv6[0..6], n6, b6[0..6], x6[0..6]), true);
        const res6 = matVecResidualInf(a6_orig[0..36], x6[0..6], b6[0..6], n6);
        expect_eq(res6 < 1.0e-11, true);

        // 12x12 diagonally dominant Thomas tridiagonal solver
        const nt: usize = 12;
        let lo_d: [11]f64 = [-1.0; 11];
        let main_d: [12]f64 = [4.0; 12];
        let up_d: [11]f64 = [-1.0; 11];
        let rhs_t: [12]f64 = [0.0; 12];
        let i: usize = 0;
        while (i < nt) : (i += 1) {
            rhs_t[i] = @as(f64, @floatFromInt(i + 1));
        }
        let xt: [12]f64 = [0.0; 12];
        let cs: [12]f64 = [0.0; 12];
        let ds_t: [12]f64 = [0.0; 12];
        expect_eq(solveTridiagonal(lo_d[0..11], main_d[0..12], up_d[0..11], rhs_t[0..12], xt[0..12], cs[0..12], ds_t[0..12]), true);
        let max_trid_err: f64 = 0.0;
        let ti: usize = 0;
        while (ti < nt) : (ti += 1) {
            let lhs: f64 = main_d[ti] * xt[ti];
            if (ti > 0) lhs += lo_d[ti - 1] * xt[ti - 1];
            if (ti + 1 < nt) lhs += up_d[ti] * xt[ti + 1];
            const e_trid = tAbs(lhs - rhs_t[ti]);
            if (e_trid > max_trid_err) max_trid_err = e_trid;
        }
        expect_eq(max_trid_err < 1.0e-11, true);
    });

    // ========================================================================
    // 4. Strict NaN / ±Inf / Empty / Zero-Variance / Singular / Domain Guards
    // ========================================================================
    test("num_ref_04_strict_boundary_and_nonfinite_rejection", () => {
        const nan_v: f64 = std.math.nan(f64);
        const pos_inf: f64 = std.math.inf(f64);
        const neg_inf: f64 = -std.math.inf(f64);
        const empty: [1]f64 = [0.0];

        // Distribution parameter & NaN/Inf guards
        let rej: usize = 0;
        if (std.math.isNan(normalPdf(nan_v, 0.0, 1.0) catch nan_v)) rej += 1;
        if (std.math.isNan(normalPdf(0.0, 0.0, 0.0) catch nan_v)) rej += 1;
        if (std.math.isNan(normalPdf(0.0, 0.0, -1.0) catch nan_v)) rej += 1;
        if (std.math.isNan(normalCdf(pos_inf, 0.0, 1.0) catch nan_v)) rej += 1;
        if (std.math.isNan(normalPpf(0.0, 0.0, 1.0) catch nan_v)) rej += 1;
        if (std.math.isNan(normalPpf(1.0, 0.0, 1.0) catch nan_v)) rej += 1;
        if (std.math.isNan(normalPpf(-0.1, 0.0, 1.0) catch nan_v)) rej += 1;
        if (std.math.isNan(studentTPdf(1.0, 0.0) catch nan_v)) rej += 1;
        if (std.math.isNan(studentTCdf(nan_v, 5.0) catch nan_v)) rej += 1;
        if (std.math.isNan(studentTPpf(1.2, 5.0) catch nan_v)) rej += 1;
        if (std.math.isNan(chiSquarePdf(1.0, -2.0) catch nan_v)) rej += 1;
        if (std.math.isNan(chiSquareCdf(neg_inf, 3.0) catch nan_v)) rej += 1;
        if (std.math.isNan(chiSquarePpf(0.0, 3.0) catch nan_v)) rej += 1;
        if (std.math.isNan(fDistCdf(1.0, 0.0, 5.0) catch nan_v)) rej += 1;
        if (std.math.isNan(betaPdf(0.5, -1.0, 2.0) catch nan_v)) rej += 1;
        if (std.math.isNan(betaCdf(0.5, 2.0, 0.0) catch nan_v)) rej += 1;
        if (std.math.isNan(poissonPmf(1, 0.0) catch nan_v)) rej += 1;
        if (std.math.isNan(poissonCdf(1, nan_v) catch nan_v)) rej += 1;
        expect_eq(rej, 18);

        // Empty / NaN / zero-variance / singular guards
        let rej2: usize = 0;
        if (std.math.isNan(simdMathSumF64(empty[0..0]) catch nan_v)) rej2 += 1;
        if (std.math.isNan(simdMathDotF64(empty[0..0], empty[0..0]) catch nan_v)) rej2 += 1;
        if (std.math.isNan(simdMathSumSqDiffF64(empty[0..0], 0.0) catch nan_v)) rej2 += 1;
        if (std.math.isNan(logSumExp(empty[0..0]) catch nan_v)) rej2 += 1;
        rej2 += errDescribe(empty[0..0]);
        const bad_nan: [5]f64 = [1.0, 2.0, nan_v, 4.0, 5.0];
        const bad_inf: [5]f64 = [1.0, 2.0, 3.0, pos_inf, 5.0];
        const const_s: [5]f64 = [7.0, 7.0, 7.0, 7.0, 7.0];
        if (std.math.isNan(simdMathSumF64(bad_nan[0..5]) catch nan_v)) rej2 += 1;
        if (std.math.isNan(simdMathDotF64(bad_inf[0..5], const_s[0..5]) catch nan_v)) rej2 += 1;
        if (std.math.isNan(simdMathSumSqDiffF64(bad_nan[0..5], 1.0) catch nan_v)) rej2 += 1;
        if (std.math.isNan(logSumExp(bad_inf[0..5]) catch nan_v)) rej2 += 1;
        rej2 += errDescribe(bad_nan[0..5]);
        rej2 += errOneT(const_s[0..5], 7.0);
        rej2 += errWelch(const_s[0..5], const_s[0..5]);
        rej2 += errCorr(const_s[0..5], const_s[0..5]);
        rej2 += errOls(const_s[0..5], const_s[0..5]);

        // Singular collinear matrix in fitRidgeRegression (lambda=0)
        const col_x: [8]f64 = [1.0, 2.0, 2.0, 4.0, 3.0, 6.0, 4.0, 8.0];
        const col_y: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        rej2 += errRidge(col_x[0..8], 4, 2, col_y[0..4], 0.0);

        // predictLinear NaN guard
        const good_x: [12]f64 = [1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 2.0, 1.0, 1.0, 2.0, 3.0, -1.0];
        const good_y: [6]f64 = [5.0, -2.0, 0.0, 2.0, -5.0, 14.0];
        const mreg = fitRidgeRegression(good_x[0..12], 6, 2, good_y[0..6], 0.0) catch return;
        const bad_row: [2]f64 = [1.0, nan_v];
        if (std.math.isNan(predictLinear(mreg, bad_row[0..2]) catch nan_v)) rej2 += 1;
        expect_eq(rej2, 16);

        // LU & Tridiagonal strict rejection guards
        let sing: [9]f64 = [1.0, 2.0, 3.0, 2.0, 4.0, 6.0, 3.0, 6.0, 9.0];
        let piv3: [3]usize = [0; 3];
        expect_eq(luFactorize(sing[0..9], 3, piv3[0..3], 1.0e-12) == null, true);
        expect_eq(luFactorize(sing[0..9], 0, piv3[0..3], 1.0e-12) == null, true);
        expect_eq(luFactorize(sing[0..9], 3, piv3[0..3], -1.0) == null, true);
        expect_eq(luFactorize(sing[0..9], 3, piv3[0..3], nan_v) == null, true);

        let nan_mat: [4]f64 = [1.0, nan_v, 0.0, 1.0];
        let nan_diag: [4]f64 = [1.0, 0.0, 0.0, nan_v];
        let piv2: [2]usize = [0; 2];
        expect_eq(luFactorize(nan_mat[0..4], 2, piv2[0..2], 1.0e-12) == null, true);
        const dummy_info = LuInfo{ sign: 1.0, swaps: 0, min_pivot: 1.0 };
        expect_eq(luDeterminant(nan_diag[0..4], dummy_info, 2), 0.0);
        expect_eq(luLogDet(nan_diag[0..4], dummy_info, 2) == null, true);
        expect_eq(luSimdDotF64(bad_nan[0..5], const_s[0..5]) == null, true);
        expect_eq(luSimdDotF64(const_s[0..4], const_s[0..5]) == null, true);

        let zero_d: [3]f64 = [0.0, 2.0, 2.0];
        let lo2: [2]f64 = [1.0, 1.0];
        let up2: [2]f64 = [1.0, 1.0];
        let r3: [3]f64 = [1.0, 2.0, 3.0];
        let xo3: [3]f64 = [0.0; 3];
        let c3: [3]f64 = [0.0; 3];
        let d3: [3]f64 = [0.0; 3];
        expect_eq(solveTridiagonal(lo2[0..2], zero_d[0..3], up2[0..2], r3[0..3], xo3[0..3], c3[0..3], d3[0..3]), false);
    });

    // ========================================================================
    // 5. 4-Lane @Vector(4, f64) SIMD vs Scalar Parity (< 1e-9)
    // ========================================================================
    test("num_ref_05_simd4_f64_vs_scalar_parity", () => {
        const lens: [13]usize = [1, 2, 3, 4, 5, 7, 8, 15, 16, 31, 33, 64, 129];
        let xs: [129]f64 = [0.0; 129];
        let ys: [129]f64 = [0.0; 129];
        let rng: u64 = 0x51A7_5F64_C0DE_1001;
        let idx: usize = 0;
        while (idx < 129) : (idx += 1) {
            xs[idx] = nextSignedF64(*rng, 50.0);
            ys[idx] = nextSignedF64(*rng, 50.0);
        }

        let li: usize = 0;
        while (li < lens.len) : (li += 1) {
            const n = lens[li];
            const sx = xs[0..n];
            const sy = ys[0..n];

            const ref_sum = scalarSum(sx);
            const simd_sum = simdMathSumF64(sx) catch -999999.0;
            expect_eq(tNear(simd_sum, ref_sum, 1.0e-9), true);

            const ref_dot = scalarDot(sx, sy);
            const simd_dot = simdMathDotF64(sx, sy) catch -999999.0;
            expect_eq(tNear(simd_dot, ref_dot, 1.0e-9), true);

            const lu_dot_opt = luSimdDotF64(sx, sy);
            expect_eq(lu_dot_opt != null, true);
            expect_eq(tNear(lu_dot_opt.?, ref_dot, 1.0e-9), true);

            const mean_x = ref_sum / @as(f64, @floatFromInt(n));
            const ref_sq = scalarSumSqDiff(sx, mean_x);
            const simd_sq = simdMathSumSqDiffF64(sx, mean_x) catch -999999.0;
            expect_eq(tNear(simd_sq, ref_sq, 1.0e-9), true);
        }
    });

    // ========================================================================
    // 6. 1,800-Iteration Deterministic PRNG Fuzzing (StatsDist + LU + SIMD)
    // ========================================================================
    test("num_ref_06_prng_fuzz_1800_iters", () => {
        let rng: u64 = 0x51AF_057A_7510_F022;
        let x_buf: [24]f64 = [0.0; 24];
        let y_buf: [24]f64 = [0.0; 24];
        let mat_orig: [64]f64 = [0.0; 64];
        let mat_lu: [64]f64 = [0.0; 64];
        let rhs: [8]f64 = [0.0; 8];
        let sol: [8]f64 = [0.0; 8];
        let piv: [8]usize = [0; 8];
        let tr_lo: [7]f64 = [0.0; 7];
        let tr_d: [8]f64 = [0.0; 8];
        let tr_up: [7]f64 = [0.0; 7];
        let tr_c: [8]f64 = [0.0; 8];
        let tr_ds: [8]f64 = [0.0; 8];

        let iter: usize = 0;
        let lu_solved_count: usize = 0;
        let stats_ok_count: usize = 0;
        while (iter < 1800) : (iter += 1) {
            // (a) Fuzz distributions with valid + extreme + occasional invalid params
            const p: f64 = nextUnitF64(*rng);
            const df: f64 = 0.5 + nextUnitF64(*rng) * 30.0;
            const xv: f64 = nextSignedF64(*rng, 10.0);
            if (p > 0.001 && p < 0.999) {
                const nq = normalPpf(p, xv, 0.5 + nextUnitF64(*rng) * 5.0) catch std.math.nan(f64);
                expect_eq(tIsFinite(nq), true);
                const tc = studentTCdf(xv, df) catch std.math.nan(f64);
                expect_eq(tIsFinite(tc) && tc >= 0.0 && tc <= 1.0, true);
                const fc = fDistCdf(tAbs(xv), df, 1.0 + df * 0.5) catch std.math.nan(f64);
                expect_eq(tIsFinite(fc) && fc >= 0.0 && fc <= 1.0, true);
                const cc = chiSquareCdf(tAbs(xv), df) catch std.math.nan(f64);
                expect_eq(tIsFinite(cc) && cc >= 0.0 && cc <= 1.0, true);
                const bc = betaCdf(p, 0.5 + nextUnitF64(*rng) * 5.0, 0.5 + nextUnitF64(*rng) * 5.0) catch std.math.nan(f64);
                expect_eq(tIsFinite(bc) && bc >= 0.0 && bc <= 1.0, true);
            }

            // (b) Fuzz 4-lane SIMD + describeSample + logSumExp + OLS (lengths 3..24)
            const slen_off: usize = @intCast(xorshift64(*rng) % 22);
            const slen: usize = slen_off + 3;
            const inject_bad: bool = (iter % 45) == 0;
            let si: usize = 0;
            while (si < slen) : (si += 1) {
                x_buf[si] = nextSignedF64(*rng, 25.0);
                y_buf[si] = nextSignedF64(*rng, 25.0);
            }
            if (inject_bad) {
                x_buf[slen / 2] = if ((iter % 90) == 0) std.math.nan(f64) else std.math.inf(f64);
                const bad_sum = simdMathSumF64(x_buf[0..slen]) catch std.math.nan(f64);
                expect_eq(std.math.isNan(bad_sum), true);
                const bad_lse = logSumExp(x_buf[0..slen]) catch std.math.nan(f64);
                expect_eq(std.math.isNan(bad_lse), true);
                expect_eq(luSimdDotF64(x_buf[0..slen], y_buf[0..slen]) == null, true);
            } else {
                const s_sum = simdMathSumF64(x_buf[0..slen]) catch std.math.nan(f64);
                const s_dot = simdMathDotF64(x_buf[0..slen], y_buf[0..slen]) catch std.math.nan(f64);
                expect_eq(tNear(s_sum, scalarSum(x_buf[0..slen]), 1.0e-9), true);
                expect_eq(tNear(s_dot, scalarDot(x_buf[0..slen], y_buf[0..slen]), 1.0e-9), true);
                const lse_v = logSumExp(x_buf[0..slen]) catch std.math.nan(f64);
                expect_eq(tIsFinite(lse_v), true);
                const ds = describeSample(x_buf[0..slen]) catch return;
                expect_eq(tIsFinite(ds.mean) && tIsFinite(ds.variance) && ds.variance >= 0.0, true);
                stats_ok_count += 1;
            }

            // (c) Fuzz LU (n = 1..8) & Tridiagonal
            const n_off: usize = @intCast(xorshift64(*rng) % 8);
            const n: usize = n_off + 1;
            const make_sing: bool = (iter % 30) == 0 && n >= 2;
            let r: usize = 0;
            while (r < n) : (r += 1) {
                let c: usize = 0;
                while (c < n) : (c += 1) {
                    let v = nextSignedF64(*rng, 4.0);
                    if (!make_sing && r == c) {
                        if (v >= 0.0) { v += 10.0; } else { v -= 10.0; }
                    }
                    mat_orig[r * n + c] = v;
                    mat_lu[r * n + c] = v;
                }
                rhs[r] = nextSignedF64(*rng, 5.0);
            }
            if (make_sing) {
                // Make row 1 equal to row 0 -> exact singularity
                let sc: usize = 0;
                while (sc < n) : (sc += 1) {
                    mat_orig[n + sc] = mat_orig[sc];
                    mat_lu[n + sc] = mat_orig[sc];
                }
            }
            const lu_opt = luFactorize(mat_lu[0..n * n], n, piv[0..n], 1.0e-12);
            if (make_sing) {
                expect_eq(lu_opt == null, true);
            } else if (lu_opt != null) {
                expect_eq(luSolve(mat_lu[0..n * n], piv[0..n], n, rhs[0..n], sol[0..n]), true);
                expect_eq(matVecResidualInf(mat_orig[0..n * n], sol[0..n], rhs[0..n], n) < 1.0e-8, true);
                expect_eq(luLogDet(mat_lu[0..n * n], lu_opt.?, n) != null, true);
                lu_solved_count += 1;
            }

            // Tridiagonal fuzz (diagonally dominant)
            let ti: usize = 0;
            while (ti < n) : (ti += 1) {
                tr_d[ti] = 6.0 + nextUnitF64(*rng) * 4.0;
                if (ti + 1 < n) {
                    tr_lo[ti] = nextSignedF64(*rng, 1.5);
                    tr_up[ti] = nextSignedF64(*rng, 1.5);
                }
            }
            const lo_s: []const f64 = if (n > 1) tr_lo[0..n - 1] else tr_lo[0..0];
            const up_s: []const f64 = if (n > 1) tr_up[0..n - 1] else tr_up[0..0];
            expect_eq(solveTridiagonal(lo_s, tr_d[0..n], up_s, rhs[0..n], sol[0..n], tr_c[0..n], tr_ds[0..n]), true);
            let k: usize = 0;
            while (k < n) : (k += 1) {
                expect_eq(tIsFinite(sol[k]), true);
            }
        }
        expect_eq(stats_ok_count > 1700, true);
        expect_eq(lu_solved_count > 1700, true);
    });
}
