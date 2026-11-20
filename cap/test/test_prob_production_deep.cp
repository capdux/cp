// Source: cp/cap/test/test_prob_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:prob & cap.prob

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ProbError,
    Rng,
    NormalDist,
    ExponentialDist,
    BinomialDist,
    PoissonDist,
    AbTestResult,
    HypothesisTest,
} from "cap:prob";

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testNormalDistPrecision(): !void {
    const std_norm = NormalDist.standard();

    expect_eq(approxF64(std_norm.cdf(0.0), 0.5, 1.0e-12), true);
    expect_eq(approxF64(std_norm.cdf(1.959963984540054), 0.975, 1.0e-7), true);
    expect_eq(approxF64(std_norm.pdf(0.0), 0.3989422804014327, 1.0e-12), true);

    const xs: [7]f64 = [-3.5, -1.96, -0.5, 0.0, 0.75, 1.96, 3.2];
    for (x of xs) {
        const p = std_norm.cdf(x);
        const rx = std_norm.invCdf(p)?;
        expect_eq(approxF64(rx, x, 1.0e-6), true);
    }

    const shifted = NormalDist.init(10.0, 2.5)?;
    expect_eq(approxF64(shifted.cdf(10.0), 0.5, 1.0e-12), true);
    const q95 = shifted.invCdf(0.975)?;
    expect_eq(approxF64(q95, 10.0 + 2.5 * 1.959963984540054, 1.0e-6), true);

    let got_inv_param = false;
    * = NormalDist.init(0.0, 0.0) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidParameter")) got_inv_param = true;
    };
    expect_eq(got_inv_param, true);

    let got_inv_prob = false;
    * = std_norm.invCdf(1.2) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidProbability")) got_inv_prob = true;
    };
    expect_eq(got_inv_prob, true);
}

fn testExpBinomialPoissonAndOverflow(): !void {
    const exp_d = ExponentialDist.init(2.0)?;
    expect_eq(approxF64(exp_d.pdf(0.0), 2.0, 1.0e-12), true);
    expect_eq(approxF64(exp_d.pdf(-1.0), 0.0, 1.0e-12), true);
    expect_eq(approxF64(exp_d.cdf(0.0), 0.0, 1.0e-12), true);
    const p_exp = exp_d.cdf(1.5);
    const x_exp = exp_d.invCdf(p_exp)?;
    expect_eq(approxF64(x_exp, 1.5, 1.0e-12), true);

    // Binomial(10, 0.3) total PMF sum == 1.0
    const bin_small = BinomialDist.init(10, 0.3)?;
    let sum_bin: f64 = 0.0;
    let k: u64 = 0;
    while (k <= 10) : (k += 1) {
        sum_bin += bin_small.pmf(k);
    }
    expect_eq(approxF64(sum_bin, 1.0, 1.0e-10), true);
    expect_eq(approxF64(bin_small.cdf(10), 1.0, 1.0e-10), true);

    // Large Binomial(1000, 0.5) must not overflow and peak near k = 500
    const bin_large = BinomialDist.init(1000, 0.5)?;
    const p500 = bin_large.pmf(500);
    expect_eq(std.math.isFinite(p500), true);
    expect_eq(p500 > 0.025 && p500 < 0.026, true);
    expect_eq(approxF64(bin_large.cdf(500), 0.5 + 0.5 * p500, 1.0e-4), true);

    // Poisson(4.0) partial sum & CDF consistency
    const pois = PoissonDist.init(4.0)?;
    let sum_pois: f64 = 0.0;
    let j: u64 = 0;
    while (j <= 30) : (j += 1) {
        sum_pois += pois.pmf(j);
    }
    expect_eq(approxF64(sum_pois, 1.0, 1.0e-10), true);
    expect_eq(approxF64(pois.cdf(4), pois.pmf(0) + pois.pmf(1) + pois.pmf(2) + pois.pmf(3) + pois.pmf(4), 1.0e-12), true);
}

fn testRngDeterministicSampling(): !void {
    let r1 = Rng.init(123456789);
    let r2 = Rng.init(123456789);

    for (i of 0..20) {
        * = i;
        expect_eq(r1.nextU64(), r2.nextU64());
    }

    // Sample 5000 Normal(10.0, 2.0) and verify sample mean and stddev convergence
    let rng = Rng.init(42);
    const n: usize = 5000;
    let sum: f64 = 0.0;
    let sum_sq: f64 = 0.0;
    for (i of 0..n) {
        * = i;
        const v = rng.normal(10.0, 2.0)?;
        sum += v;
        sum_sq += v * v;
    }
    const mean = sum / (n as f64);
    const variance = (sum_sq / (n as f64)) - mean * mean;
    const stddev = @sqrt(variance);
    expect_eq(approxF64(mean, 10.0, 0.15), true);
    expect_eq(approxF64(stddev, 2.0, 0.15), true);

    // Weighted choice empirical distribution
    const weights: [3]f64 = [1.0, 2.0, 7.0];
    let counts: [3]u64 = [0, 0, 0];
    for (i of 0..2000) {
        * = i;
        const idx = rng.weightedChoice(&weights)?;
        counts[idx] += 1;
    }
    expect_eq(counts[2] > counts[1] && counts[1] > counts[0], true);

    // Empty weights error guard
    const empty_w: []const f64 = [];
    let got_empty_w = false;
    * = rng.weightedChoice(empty_w) catch e => {
        if (std.mem.eql(u8, @errorName(e), "EmptyWeights")) got_empty_w = true;
    };
    expect_eq(got_empty_w, true);

    // In-place Fisher-Yates shuffle preserves elements and permutes
    let arr: [8]i32 = [1, 2, 3, 4, 5, 6, 7, 8];
    rng.shuffle(arr[0..8]);
    let arr_sum: i32 = 0;
    for (v of arr) {
        arr_sum += v;
    }
    expect_eq(arr_sum, 36);

    // Poisson sampling for both Knuth (lambda=5) and PTRS (lambda=50)
    const pk = rng.poisson(5.0)?;
    const pp = rng.poisson(50.0)?;
    expect_eq(pk < 100 && pp < 200, true);
}

fn testHypothesisTesting(): !void {
    // Significant lift: 100/1000 (10%) vs 160/1000 (16%)
    const sig_res = HypothesisTest.twoProportionZTest(100, 1000, 160, 1000, 0.05)?;
    expect_eq(sig_res.significant, true);
    expect_eq(sig_res.p_value < 0.01, true);
    expect_eq(sig_res.z_score > 3.5, true);
    expect_eq(approxF64(sig_res.relative_lift, 0.6, 1.0e-12), true);
    expect_eq(sig_res.ci_low > 0.0 && sig_res.ci_high > sig_res.ci_low, true);

    // Non-significant difference: 100/1000 vs 105/1000
    const nonsig_res = HypothesisTest.twoProportionZTest(100, 1000, 105, 1000, 0.05)?;
    expect_eq(nonsig_res.significant, false);
    expect_eq(nonsig_res.p_value > 0.5, true);
    expect_eq(nonsig_res.ci_low < 0.0 && nonsig_res.ci_high > 0.0, true);

    // Two-sample Z-test
    const z_res = HypothesisTest.twoSampleZTest(50.0, 5.0, 400, 52.0, 5.0, 400, 0.05)?;
    expect_eq(z_res.significant, true);
    expect_eq(z_res.p_value < 0.001, true);

    // Invalid parameter & sample size guards
    let got_small = false;
    * = HypothesisTest.twoProportionZTest(0, 0, 10, 100, 0.05) catch e => {
        if (std.mem.eql(u8, @errorName(e), "SampleSizeTooSmall")) got_small = true;
    };
    expect_eq(got_small, true);

    let got_bad_conv = false;
    * = HypothesisTest.twoProportionZTest(120, 100, 10, 100, 0.05) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidParameter")) got_bad_conv = true;
    };
    expect_eq(got_bad_conv, true);

    let got_bad_alpha = false;
    * = HypothesisTest.twoSampleZTest(1.0, 1.0, 10, 2.0, 1.0, 10, 1.5) catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidProbability")) got_bad_alpha = true;
    };
    expect_eq(got_bad_alpha, true);
}

fn testCapProbLazyMount(): !void {
    const n = cap.prob.NormalDist.standard();
    expect_eq(approxF64(n.cdf(0.0), 0.5, 1.0e-12), true);

    let r = cap.prob.Rng.init(999);
    const u = r.uniform(10.0, 20.0)?;
    expect_eq(u >= 10.0 && u < 20.0, true);

    const b = cap.prob.BinomialDist.init(20, 0.5)?;
    expect_eq(approxF64(b.cdf(20), 1.0, 1.0e-10), true);

    const ab = cap.prob.HypothesisTest.twoProportionZTest(50, 500, 90, 500, 0.05)?;
    expect_eq(ab.significant, true);
}

fn main() {
    test("1. NormalDist pdf/cdf/invCdf Precision (< 1e-6) & Error Guards", () => {
        testNormalDistPrecision()?;
    });
    test("2. ExponentialDist / BinomialDist(1000,0.5) / PoissonDist Anti-Overflow", () => {
        testExpBinomialPoissonAndOverflow()?;
    });
    test("3. Rng Deterministic Sampling, Normal(10,2) Convergence, WeightedChoice & Shuffle", () => {
        testRngDeterministicSampling()?;
    });
    test("4. HypothesisTest twoProportionZTest / twoSampleZTest & Guards", () => {
        testHypothesisTesting()?;
    });
    test("5. import cap from \"cap\"; cap.prob Dynamic Lazy Mounting", () => {
        testCapProbLazyMount()?;
    });
}
