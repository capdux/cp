// Origin: Pure-CP Stdlib Test Suite (lib/math/statsDist.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Probability Distributions, Hypothesis Testing & OLS/Ridge Regression:
//   1. High-precision special functions (`logGamma`, `erfApprox`/`erfcApprox`, `normalInvCdf`, incomplete Gamma/Beta)
//   2. 6 probability distributions (Normal, Student-t, Chi-Square, F, Beta, Poisson) CDF/PPF round-trip < 1e-6
//   3. Descriptive statistics, one-sample t-test & Welch two-sample t-test vs R/SciPy reference benchmarks
//   4. Chi-square goodness-of-fit & Pearson correlation hypothesis test
//   5. Simple OLS regression (`fitSimpleOls`) exact & noisy parameter recovery
//   6. 1000-sample multivariate OLS & Ridge regression (coefficient recovery, R^2 > 0.999 & collinearity stabilization)
//   7. Error boundary guards (`StatsDistError`) & `cap.math` facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    STATS_MAX_FEATURES,
    StatsDistError,
    logGamma,
    erfApprox,
    erfcApprox,
    normalInvCdf,
    regularizedIncompleteGammaP,
    regularizedIncompleteGammaQ,
    regularizedIncompleteBeta,
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
} from "cap:math";

fn absDiff(a: f64, b: f64): f64 {
    const d: f64 = a - b;
    return if (d < 0.0) -d else d;
}

test("SpecialFunctions: logGamma, erf/erfc, normalInvCdf, incomplete Gamma & Beta reference accuracy") {
    // logGamma reference values
    expect_true(absDiff(logGamma(1.0), 0.0) < 1.0e-12);
    expect_true(absDiff(logGamma(0.5), 0.5 * std.math.log(f64, std.math.e, 3.14159265358979323846)) < 1.0e-12);
    expect_true(absDiff(logGamma(6.0), std.math.log(f64, std.math.e, 120.0)) < 1.0e-12);
    expect_true(absDiff(logGamma(0.25), 1.2880225246980775) < 1.0e-11);

    // erf & erfc reference values
    expect_true(absDiff(erfApprox(0.0), 0.0) < 1.0e-14);
    expect_true(absDiff(erfApprox(1.0), 0.8427007929497149) < 1.0e-12);
    expect_true(absDiff(erfApprox(-1.0), -0.8427007929497149) < 1.0e-12);
    expect_true(absDiff(erfcApprox(3.0), 0.00002209049699858544) < 1.0e-12);

    // normalInvCdf (Acklam + Halley)
    expect_true(absDiff(normalInvCdf(0.5), 0.0) < 1.0e-14);
    expect_true(absDiff(normalInvCdf(0.975), 1.959963984540054) < 1.0e-10);
    expect_true(absDiff(normalInvCdf(0.001), -3.090232306167813) < 1.0e-10);

    // Incomplete gamma complementarity: P(a, x) + Q(a, x) == 1
    const gp: f64 = regularizedIncompleteGammaP(2.5, 1.5);
    const gq: f64 = regularizedIncompleteGammaQ(2.5, 1.5);
    expect_true(absDiff(gp + gq, 1.0) < 1.0e-13);

    // Incomplete beta exact polynomial: I_x(2, 3) = x^2 * (6 - 8x + 3x^2) => at x=0.4 => 0.5248
    expect_true(absDiff(regularizedIncompleteBeta(2.0, 3.0, 0.4), 0.5248) < 1.0e-12);
    expect_true(absDiff(regularizedIncompleteBeta(3.0, 2.0, 0.6), 1.0 - 0.5248) < 1.0e-12);
}

test("Distributions: Normal, Student-t, Chi-Square, F, Beta & Poisson CDF/PPF round-trip < 1e-6") {
    // 1. Normal
    const npdf = normalPdf(10.0, 10.0, 2.5) catch 0.0;
    expect_true(absDiff(npdf, 1.0 / (2.5 * 2.5066282746310005)) < 1.0e-10);
    const nq = normalPpf(0.95, 10.0, 2.5) catch 0.0;
    const nc = normalCdf(nq, 10.0, 2.5) catch 0.0;
    expect_true(absDiff(nc, 0.95) < 1.0e-10);

    // 2. Student-t (df=1, 2, 10, 30)
    const t975_10 = studentTPpf(0.975, 10.0) catch 0.0;
    expect_true(absDiff(t975_10, 2.2281388519649385) < 1.0e-7);
    const tc_10 = studentTCdf(t975_10, 10.0) catch 0.0;
    expect_true(absDiff(tc_10, 0.975) < 1.0e-9);

    const t05_5 = studentTPpf(0.05, 5.0) catch 0.0;
    expect_true(absDiff(t05_5, -2.0150483726691575) < 1.0e-7);
    expect_true(absDiff(studentTCdf(t05_5, 5.0) catch 0.0, 0.05) < 1.0e-9);
    expect_true((studentTPdf(0.0, 10.0) catch 0.0) > 0.38);

    // 3. Chi-Square (df=4, 15)
    const c95_4 = chiSquarePpf(0.95, 4.0) catch 0.0;
    expect_true(absDiff(c95_4, 9.487729036781154) < 1.0e-7);
    expect_true(absDiff(chiSquareCdf(c95_4, 4.0) catch 0.0, 0.95) < 1.0e-9);

    const c99_15 = chiSquarePpf(0.99, 15.0) catch 0.0;
    expect_true(absDiff(c99_15, 30.57791416689249) < 1.0e-6);
    expect_true(absDiff(chiSquareCdf(c99_15, 15.0) catch 0.0, 0.99) < 1.0e-8);
    expect_true(absDiff(chiSquarePdf(2.0, 2.0) catch 0.0, 0.5 * std.math.exp(-1.0)) < 1.0e-10);

    // 4. F-distribution: F_{0.95}(3, 9) = 3.86254835762476
    const fc = fDistCdf(3.86254835762476, 3.0, 9.0) catch 0.0;
    expect_true(absDiff(fc, 0.95) < 1.0e-8);

    // 5. Beta(2, 3) at x = 0.4: pdf = 12 * 0.4 * 0.36 = 1.728, cdf = 0.5248
    expect_true(absDiff(betaPdf(0.4, 2.0, 3.0) catch 0.0, 1.728) < 1.0e-10);
    expect_true(absDiff(betaCdf(0.4, 2.0, 3.0) catch 0.0, 0.5248) < 1.0e-10);

    // 6. Poisson(lambda = 2.5)
    let sum_pmf: f64 = 0.0;
    let k: u64 = 0;
    while (k <= 3) : (k += 1) {
        sum_pmf += poissonPmf(k, 2.5) catch 0.0;
    }
    const pcdf3 = poissonCdf(3, 2.5) catch 0.0;
    expect_true(absDiff(sum_pmf, pcdf3) < 1.0e-11);
}

test("HypothesisTests: describeSample, one-sample t-test & Welch two-sample t-test benchmarks") {
    // 1. Descriptive stats
    const s_arr: [8]f64 = [2.3, 2.9, 3.1, 2.7, 3.4, 2.8, 3.0, 3.2];
    const desc = describeSample(s_arr[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(desc.count, @as(usize, 8));
    expect_true(absDiff(desc.mean, 2.925) < 1.0e-12);
    expect_true(absDiff(desc.min, 2.3) < 1.0e-12);
    expect_true(absDiff(desc.max, 3.4) < 1.0e-12);
    expect_true(desc.variance > 0.1 && desc.variance < 0.13);

    // 2. One-sample t-test against mu0 = 2.5
    const t1 = oneSampleTTest(s_arr[0..], 2.5) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(t1.df, 7.0) < 1.0e-12);
    expect_true(absDiff(t1.mean_diff, 0.425) < 1.0e-12);
    expect_true(t1.t_stat > 3.5 && t1.t_stat < 3.7);
    expect_true(t1.p_value_two_tailed > 0.005 && t1.p_value_two_tailed < 0.012);
    expect_true(t1.ci95_low > 0.1 && t1.ci95_high < 0.75);
    expect_true(t1.cohens_d > 1.2);

    // 3. Welch two-sample t-test against R/SciPy reference dataset:
    //    t = -1.5379, df = 18.137, p = 0.1413
    const gx: [11]f64 = [14.0, 15.0, 15.0, 15.0, 16.0, 18.0, 22.0, 23.0, 24.0, 25.0, 25.0];
    const gy: [13]f64 = [10.0, 12.0, 14.0, 15.0, 18.0, 22.0, 24.0, 27.0, 31.0, 33.0, 34.0, 34.0, 34.0];
    const tw = welchTwoSampleTTest(gx[0..], gy[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(tw.t_stat, -1.53790) < 0.002);
    expect_true(absDiff(tw.df, 18.137) < 0.05);
    expect_true(absDiff(tw.p_value_two_tailed, 0.1413) < 0.002);
    expect_true(tw.ci95_low < 0.0 && tw.ci95_high > 0.0);
}

test("HypothesisTests: chi-square goodness-of-fit & Pearson correlation test") {
    // 1. Fair vs biased 6-sided die
    const expected: [6]f64 = [100.0, 100.0, 100.0, 100.0, 100.0, 100.0];
    const fair_obs: [6]f64 = [98.0, 103.0, 97.0, 102.0, 99.0, 101.0];
    const biased_obs: [6]f64 = [140.0, 90.0, 85.0, 95.0, 90.0, 100.0];

    const c_fair = chiSquareGoodnessOfFit(fair_obs[0..], expected[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(c_fair.df, 5.0) < 1.0e-12);
    expect_true(c_fair.p_value > 0.95);

    const c_bias = chiSquareGoodnessOfFit(biased_obs[0..], expected[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(c_bias.chi2_stat, 20.5) < 1.0e-9);
    expect_true(c_bias.p_value < 0.002);

    // 2. Pearson correlation test
    const x: [10]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0];
    const y_pos: [10]f64 = [2.1, 3.9, 6.2, 7.8, 10.1, 12.2, 13.9, 16.1, 17.8, 20.2];
    const corr = pearsonCorrelationTest(x[0..], y_pos[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(corr.n, @as(usize, 10));
    expect_true(corr.r > 0.998);
    expect_true(corr.t_stat > 40.0);
    expect_true(corr.p_value < 1.0e-8);
}

test("Regression: simple OLS exact & noisy fit") {
    const x: [10]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0];
    const y_exact: [10]f64 = [5.5, 8.0, 10.5, 13.0, 15.5, 18.0, 20.5, 23.0, 25.5, 28.0];

    const r_exact = fitSimpleOls(x[0..], y_exact[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(r_exact.slope, 2.5) < 1.0e-12);
    expect_true(absDiff(r_exact.intercept, 3.0) < 1.0e-12);
    expect_true(absDiff(r_exact.r_squared, 1.0) < 1.0e-12);
    expect_true(r_exact.rmse < 1.0e-12);
    expect_true(r_exact.p_value_slope < 1.0e-12);

    const y_noisy: [10]f64 = [5.6, 7.8, 10.7, 12.9, 15.7, 17.8, 20.6, 22.9, 25.7, 27.9];
    const r_noisy = fitSimpleOls(x[0..], y_noisy[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(r_noisy.slope, 2.5) < 0.05);
    expect_true(absDiff(r_noisy.intercept, 3.0) < 0.25);
    expect_true(r_noisy.r_squared > 0.999);
    expect_true(r_noisy.std_err_slope > 0.0);
    expect_true(r_noisy.p_value_slope < 1.0e-8);
}

test("Regression: 1000-sample multivariate OLS & Ridge collinearity stabilization") {
    // Ground truth: y = 1.5 + 2.5*x0 - 1.2*x1 + 0.8*x2 + 3.1*x3 + tiny deterministic noise
    const N: usize = 1000;
    const F: usize = 4;
    let x_mat: [4000]f64 = [0.0; 4000];
    let y_vec: [1000]f64 = [0.0; 1000];

    let i: usize = 0;
    while (i < N) : (i += 1) {
        const fi: f64 = @as(f64, @floatFromInt(i));
        const x0: f64 = std.math.sin(fi * 0.13) * 3.0;
        const x1: f64 = std.math.cos(fi * 0.29) * 2.5;
        const x2: f64 = std.math.sin(fi * 0.57 + 1.1) * 4.0;
        const x3: f64 = std.math.cos(fi * 0.83 - 0.7) * 2.0;
        const noise: f64 = 0.02 * std.math.sin(fi * 1.91);

        x_mat[i * F + 0] = x0;
        x_mat[i * F + 1] = x1;
        x_mat[i * F + 2] = x2;
        x_mat[i * F + 3] = x3;
        y_vec[i] = 1.5 + 2.5 * x0 - 1.2 * x1 + 0.8 * x2 + 3.1 * x3 + noise;
    }

    const model = cap.math.fitRidgeRegression(x_mat[0..], N, F, y_vec[0..], 0.0) catch {
        expect_true(false);
        return;
    };
    expect_eq(model.n_features, @as(usize, 4));
    expect_true(absDiff(model.intercept, 1.5) < 0.01);
    expect_true(absDiff(model.coeffs[0], 2.5) < 0.01);
    expect_true(absDiff(model.coeffs[1], -1.2) < 0.01);
    expect_true(absDiff(model.coeffs[2], 0.8) < 0.01);
    expect_true(absDiff(model.coeffs[3], 3.1) < 0.01);
    expect_true(model.r_squared > 0.9999);
    expect_true(model.adj_r_squared > 0.9999);
    expect_true(model.rmse < 0.02);

    const probe: [4]f64 = [1.0, -2.0, 0.5, 1.0];
    const pred = predictLinear(model, probe[0..]) catch 0.0;
    // Expected: 1.5 + 2.5 + 2.4 + 0.4 + 3.1 = 9.9
    expect_true(absDiff(pred, 9.9) < 0.02);

    // Collinear features (x1 = 2 * x0): pure OLS singular, Ridge lambda=1.0 succeeds
    const col_x: [12]f64 = [
        1.0, 2.0,
        2.0, 4.0,
        3.0, 6.0,
        4.0, 8.0,
        5.0, 10.0,
        6.0, 12.0,
    ];
    const col_y: [6]f64 = [4.0, 7.0, 10.0, 13.0, 16.0, 19.0];

    let got_singular: bool = false;
    * = fitRidgeRegression(col_x[0..], 6, 2, col_y[0..], 0.0) catch |e| {
        got_singular = (e == StatsDistError.SingularMatrix);
    };
    expect_true(got_singular);

    const ridge_m = fitRidgeRegression(col_x[0..], 6, 2, col_y[0..], 1.0) catch {
        expect_true(false);
        return;
    };
    expect_true(ridge_m.r_squared > 0.99);
}

test("StatsDistError: boundary guards & cap.math facade parity") {
    expect_eq(STATS_MAX_FEATURES, @as(usize, 16));

    const empty: [0]f64 = [];
    let got_empty: bool = false;
    * = describeSample(empty[0..]) catch |e| {
        got_empty = (e == StatsDistError.EmptySample);
    };
    expect_true(got_empty);

    const single: [1]f64 = [42.0];
    let got_insuff: bool = false;
    * = oneSampleTTest(single[0..], 40.0) catch |e| {
        got_insuff = (e == StatsDistError.InsufficientSamples);
    };
    expect_true(got_insuff);

    const const_s: [4]f64 = [5.0, 5.0, 5.0, 5.0];
    let got_zero_var: bool = false;
    * = oneSampleTTest(const_s[0..], 4.0) catch |e| {
        got_zero_var = (e == StatsDistError.ZeroVariance);
    };
    expect_true(got_zero_var);

    const a3: [3]f64 = [1.0, 2.0, 3.0];
    const b4: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    let got_mismatch: bool = false;
    * = pearsonCorrelationTest(a3[0..], b4[0..]) catch |e| {
        got_mismatch = (e == StatsDistError.LengthMismatch);
    };
    expect_true(got_mismatch);

    let got_bad_param: bool = false;
    * = cap.math.normalPpf(1.0, 0.0, 1.0) catch |e| {
        got_bad_param = (e == StatsDistError.InvalidParameter);
    };
    expect_true(got_bad_param);

    expect_true(absDiff(cap.math.studentTCdf(0.0, 10.0) catch -1.0, 0.5) < 1.0e-12);
}
