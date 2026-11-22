// Source: cp/cap/test/test_gmm_production_deep.cp (Production deep tests for cap:gmm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    GmmConfig,
    gmmFit,
    gmmLogProbSample,
    gmmPredictProba,
    gmmPredict,
    gmmScoreSamples,
} from "cap:ml";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

test("1. 1D bimodal GMM recovers exact weights (0.4/0.6), means (-5/+5) & 100% hard clustering accuracy", () => {
    // 8 points around -5.0 (weight 0.4) + 12 points around +5.0 (weight 0.6)
    const data = [
        -5.3, -5.1, -5.0, -4.9, -4.7, -5.2, -4.8, -5.0,
         4.7,  4.8,  4.9,  5.0,  5.0,  5.1,  5.2,  5.3,  4.85,  5.15,  4.95,  5.05,
    ];
    let weights: [2]f64 = std.mem.zeroes([2]f64);
    let means: [2]f64 = std.mem.zeroes([2]f64);
    let vars: [2]f64 = std.mem.zeroes([2]f64);
    let resp: [40]f64 = std.mem.zeroes([40]f64);

    let w_s: []f64 = weights[0..];
    let m_s: []f64 = means[0..];
    let v_s: []f64 = vars[0..];
    let r_s: []f64 = resp[0..];

    const res = gmmFit(data[0..], 20, 1, 2, GmmConfig.default(), &w_s, &m_s, &v_s, &r_s) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(absF64(weights[0] + weights[1] - 1.0) < 1.0e-12, true);

    // Identify left (-5) and right (+5) components regardless of label permutation
    const left_c: usize = if (means[0] < means[1]) 0 as usize else 1 as usize;
    const right_c: usize = (1 as usize) - left_c;
    expect_eq(absF64(means[left_c] + 5.0) < 0.05, true);
    expect_eq(absF64(means[right_c] - 5.0) < 0.05, true);
    expect_eq(absF64(weights[left_c] - 0.4) < 0.01, true);
    expect_eq(absF64(weights[right_c] - 0.6) < 0.01, true);
    expect_eq(vars[left_c] > 0.0 && vars[left_c] < 0.1, true);
    expect_eq(vars[right_c] > 0.0 && vars[right_c] < 0.1, true);

    let labels: [20]usize = std.mem.zeroes([20]usize);
    let l_s: []usize = labels[0..];
    expect_eq(gmmPredict(data[0..], 20, 1, 2, weights[0..], means[0..], vars[0..], &l_s), true);

    let i: usize = 0;
    while (i < 8) {
        expect_eq(labels[i], left_c);
        i += 1;
    }
    while (i < 20) {
        expect_eq(labels[i], right_c);
        i += 1;
    }
});

test("2. 2D 3-cluster GMM: posterior row sums == 1.0 & argmax matches gmmPredict", () => {
    // 18 samples in 3 well-separated 2D clusters (6 samples each)
    const data = [
        // Cluster A around (0, 0)
        0.0, 0.0,   0.2, -0.1,  -0.2, 0.1,   0.1, 0.2,  -0.1, -0.2,  0.0, 0.1,
        // Cluster B around (10, 0)
        10.0, 0.0,  10.2, 0.1,   9.8, -0.1,  10.1, -0.2, 9.9, 0.2,  10.0, -0.1,
        // Cluster C around (5, 10)
        5.0, 10.0,  5.2, 9.9,    4.8, 10.1,  5.1, 10.2,  4.9, 9.8,   5.0, 10.05,
    ];
    let weights: [3]f64 = std.mem.zeroes([3]f64);
    let means: [6]f64 = std.mem.zeroes([6]f64);
    let vars: [6]f64 = std.mem.zeroes([6]f64);
    let resp: [54]f64 = std.mem.zeroes([54]f64);

    let w_s: []f64 = weights[0..];
    let m_s: []f64 = means[0..];
    let v_s: []f64 = vars[0..];
    let r_s: []f64 = resp[0..];

    const res = gmmFit(data[0..], 18, 2, 3, GmmConfig.default(), &w_s, &m_s, &v_s, &r_s) ?? unreachable;
    expect_eq(res.converged, true);

    let proba: [54]f64 = std.mem.zeroes([54]f64);
    let labels: [18]usize = std.mem.zeroes([18]usize);
    let p_s: []f64 = proba[0..];
    let l_s: []usize = labels[0..];

    expect_eq(gmmPredictProba(data[0..], 18, 2, 3, weights[0..], means[0..], vars[0..], &p_s), true);
    expect_eq(gmmPredict(data[0..], 18, 2, 3, weights[0..], means[0..], vars[0..], &l_s), true);

    // Every posterior row sums to 1.0 and its argmax matches gmmPredict
    let i: usize = 0;
    while (i < 18) {
        const p0 = proba[i * 3 + 0];
        const p1 = proba[i * 3 + 1];
        const p2 = proba[i * 3 + 2];
        expect_eq(absF64((p0 + p1 + p2) - 1.0) < 1.0e-12, true);
        let argmax: usize = 0;
        if (p1 > p0 && p1 >= p2) {
            argmax = 1;
        } else if (p2 > p0 && p2 > p1) {
            argmax = 2;
        }
        expect_eq(argmax, labels[i]);
        i += 1;
    }

    // All 6 samples in each cluster share the same label, and the 3 cluster labels are distinct
    const la = labels[0];
    const lb = labels[6];
    const lc = labels[12];
    expect_eq(la != lb && lb != lc && la != lc, true);
    i = 0;
    while (i < 6) {
        expect_eq(labels[i], la);
        expect_eq(labels[6 + i], lb);
        expect_eq(labels[12 + i], lc);
        i += 1;
    }
});

test("3. Monotonic EM log-likelihood progression & gmmScoreSamples exact match", () => {
    const data = [
        -4.2, -3.8, -4.0, -3.9, -4.1,
         3.9,  4.1,  4.0,  3.8,  4.2,
    ];
    let weights: [2]f64 = std.mem.zeroes([2]f64);
    let means: [2]f64 = std.mem.zeroes([2]f64);
    let vars: [2]f64 = std.mem.zeroes([2]f64);
    let resp: [20]f64 = std.mem.zeroes([20]f64);
    let w_s: []f64 = weights[0..];
    let m_s: []f64 = means[0..];
    let v_s: []f64 = vars[0..];
    let r_s: []f64 = resp[0..];

    let cfg = GmmConfig.default();
    cfg.tol = 0.0;

    cfg.max_iter = 1;
    const r1 = gmmFit(data[0..], 10, 1, 2, cfg, &w_s, &m_s, &v_s, &r_s) ?? unreachable;
    cfg.max_iter = 2;
    const r2 = gmmFit(data[0..], 10, 1, 2, cfg, &w_s, &m_s, &v_s, &r_s) ?? unreachable;
    cfg.max_iter = 10;
    const r10 = gmmFit(data[0..], 10, 1, 2, cfg, &w_s, &m_s, &v_s, &r_s) ?? unreachable;

    expect_eq(r2.log_likelihood >= r1.log_likelihood - 1.0e-12, true);
    expect_eq(r10.log_likelihood >= r2.log_likelihood - 1.0e-12, true);

    let log_probs: [10]f64 = std.mem.zeroes([10]f64);
    let lp_s: []f64 = log_probs[0..];
    const scored_ll = gmmScoreSamples(data[0..], 10, 1, 2, weights[0..], means[0..], vars[0..], &lp_s) ?? unreachable;
    expect_eq(absF64(scored_ll - r10.log_likelihood) < 1.0e-11, true);

    const single_lp = gmmLogProbSample(data[0..1], 2, 1, weights[0..], means[0..], vars[0..]) ?? unreachable;
    expect_eq(absF64(single_lp - log_probs[0]) < 1.0e-12, true);
});

test("4. AIC & BIC model selection strongly prefers true k=2 over k=1 on bimodal data", () => {
    const data = [
        -6.0, -5.8, -6.2, -5.9, -6.1, -6.0, -5.7, -6.3,
         6.0,  5.8,  6.2,  5.9,  6.1,  6.0,  5.7,  6.3,
    ];
    let w1: [1]f64 = std.mem.zeroes([1]f64);
    let m1: [1]f64 = std.mem.zeroes([1]f64);
    let v1: [1]f64 = std.mem.zeroes([1]f64);
    let r1: [16]f64 = std.mem.zeroes([16]f64);
    let w1_s: []f64 = w1[0..];
    let m1_s: []f64 = m1[0..];
    let v1_s: []f64 = v1[0..];
    let r1_s: []f64 = r1[0..];

    let w2: [2]f64 = std.mem.zeroes([2]f64);
    let m2: [2]f64 = std.mem.zeroes([2]f64);
    let v2: [2]f64 = std.mem.zeroes([2]f64);
    let r2: [32]f64 = std.mem.zeroes([32]f64);
    let w2_s: []f64 = w2[0..];
    let m2_s: []f64 = m2[0..];
    let v2_s: []f64 = v2[0..];
    let r2_s: []f64 = r2[0..];

    const fit1 = gmmFit(data[0..], 16, 1, 1, GmmConfig.default(), &w1_s, &m1_s, &v1_s, &r1_s) ?? unreachable;
    const fit2 = gmmFit(data[0..], 16, 1, 2, GmmConfig.default(), &w2_s, &m2_s, &v2_s, &r2_s) ?? unreachable;

    // Lower AIC and lower BIC indicate the better model
    expect_eq(fit2.aic < fit1.aic, true);
    expect_eq(fit2.bic < fit1.bic, true);
    expect_eq(fit2.log_likelihood > fit1.log_likelihood, true);
});

test("5. Variance floor reg_var prevents singularity collapse on singleton outlier cluster", () => {
    // 5 tightly grouped points at 0.0 + 1 isolated singleton point at 100.0
    const data = [0.0, 0.1, -0.1, 0.05, -0.05, 100.0];
    let weights: [2]f64 = std.mem.zeroes([2]f64);
    let means: [2]f64 = std.mem.zeroes([2]f64);
    let vars: [2]f64 = std.mem.zeroes([2]f64);
    let resp: [12]f64 = std.mem.zeroes([12]f64);
    let w_s: []f64 = weights[0..];
    let m_s: []f64 = means[0..];
    let v_s: []f64 = vars[0..];
    let r_s: []f64 = resp[0..];

    let cfg = GmmConfig.default();
    cfg.reg_var = 1.0e-4;

    const res = gmmFit(data[0..], 6, 1, 2, cfg, &w_s, &m_s, &v_s, &r_s) ?? unreachable;
    expect_eq(res.converged, true);
    // Every component variance stays strictly >= reg_var
    expect_eq(vars[0] >= cfg.reg_var, true);
    expect_eq(vars[1] >= cfg.reg_var, true);
    expect_eq((res.log_likelihood - res.log_likelihood) == 0.0, true);
});

test("6. Defensive validation: k==0, k>n_samples, undersized buffers, reg_var<=0 & NaN rejected", () => {
    const data = [1.0, 2.0, 3.0];
    let w: [3]f64 = std.mem.zeroes([3]f64);
    let m: [3]f64 = std.mem.zeroes([3]f64);
    let v: [3]f64 = std.mem.zeroes([3]f64);
    let r: [9]f64 = std.mem.zeroes([9]f64);
    let w_s: []f64 = w[0..];
    let m_s: []f64 = m[0..];
    let v_s: []f64 = v[0..];
    let r_s: []f64 = r[0..];
    let short_r: []f64 = r[0..2];
    const cfg = GmmConfig.default();

    expect_eq(gmmFit(data[0..], 3, 1, 0, cfg, &w_s, &m_s, &v_s, &r_s) == null, true);
    expect_eq(gmmFit(data[0..], 3, 1, 4, cfg, &w_s, &m_s, &v_s, &r_s) == null, true);
    expect_eq(gmmFit(data[0..], 3, 1, 2, cfg, &w_s, &m_s, &v_s, &short_r) == null, true);

    let bad_cfg = GmmConfig.default();
    bad_cfg.reg_var = 0.0;
    expect_eq(gmmFit(data[0..], 3, 1, 2, bad_cfg, &w_s, &m_s, &v_s, &r_s) == null, true);

    const zero: f64 = 0.0;
    const nan_data = [1.0, zero / zero, 3.0];
    expect_eq(gmmFit(nan_data[0..], 3, 1, 2, cfg, &w_s, &m_s, &v_s, &r_s) == null, true);
    expect_eq(gmmLogProbSample(nan_data[0..1], 1, 1, w[0..1], m[0..1], v[0..1]) == null, true);
});
