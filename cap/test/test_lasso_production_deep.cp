// Source: cp/cap/test/test_lasso_production_deep.cp (Production deep tests for cap:lasso)

import { test, expect_eq } from "cap:test";
import {
    softThreshold,
    LassoConfig,
    LassoResult,
    LassoWorkspace,
    elasticNetFit,
    lassoFit,
    ridgeFit,
    linearPredict,
    mseScore,
    r2Score,
} from "cap:lasso";

fn absF64(v: f64): f64 {
    return if (v < 0.0) (0.0 - v) else v;
}

fn l1Norm(w: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < w.len) {
        s += absF64(w[i]);
        i += 1;
    }
    return s;
}

fn l2Norm(w: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < w.len) {
        s += w[i] * w[i];
        i += 1;
    }
    return @sqrt(s);
}

test("1. High-dimensional sparse truth (10 features, 2 non-zero): Lasso zeroes 8 noise features & R2 > 0.99", () => {
    const n: usize = 40;
    const p: usize = 10;
    let x: [400]f64 = [0.0; 400];
    let y: [40]f64 = [0.0; 40];

    // Deterministic uncorrelated features via SplitMix64 centered in [-1, 1]
    let rng: u64 = 0xCAFEBABE12345678;
    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < p) {
            rng = rng +% 0x9E3779B97F4A7C15;
            let z: u64 = rng;
            z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
            z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
            z = z ^ (z >> 31);
            const u: f64 = (((z >> 11) as f64) * (1.0 / 9007199254740992.0)) * 2.0 - 1.0;
            x[i * p + j] = u;
            j += 1;
        }
        // True sparse model: y = 3.5 * x[1] - 2.5 * x[6] + 1.25
        y[i] = (3.5 * x[i * p + 1]) - (2.5 * x[i * p + 6]) + 1.25;
        i += 1;
    }

    let w_b: [10]f64 = [0.0; 10];
    let r_b: [40]f64 = [0.0; 40];
    let cn_b: [10]f64 = [0.0; 10];
    let xm_b: [10]f64 = [0.0; 10];
    let w: []f64 = w_b[0..];
    let ws = LassoWorkspace { residual: r_b[0..], col_norm_sq: cn_b[0..], x_mean: xm_b[0..] };

    const res: LassoResult = lassoFit(x[0..], y[0..], n, p, 0.08, true, *w, *ws) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.non_zero_count, 2);

    // Features 1 and 6 recovered with right sign & magnitude; all 8 noise features strictly 0.0
    expect_eq(w[1] > 3.0 && w[1] < 3.6, true);
    expect_eq(w[6] < -2.0 && w[6] > -2.6, true);
    let j: usize = 0;
    while (j < p) {
        if (j != 1 && j != 6) {
            expect_eq(w[j], 0.0);
        }
        j += 1;
    }

    // Verify predictions and R^2 > 0.99
    let y_pred: [40]f64 = [0.0; 40];
    let k: usize = 0;
    while (k < n) {
        y_pred[k] = linearPredict(w, res.intercept, x[(k * p)..(k * p + p)]) ?? unreachable;
        k += 1;
    }
    const r2: f64 = r2Score(y[0..], y_pred[0..]) ?? unreachable;
    const mse: f64 = mseScore(y[0..], y_pred[0..]) ?? unreachable;
    expect_eq(r2 > 0.99, true);
    expect_eq(absF64(mse - res.mse) < 1.0e-12, true);
});

test("2. Collinear grouping effect (ElasticNet) & Ridge closed-form analytical cross-check", () => {
    // (a) Ridge analytical solution cross-check on 2D correlated design matrix (fit_intercept = false)
    //     Closed form: (X^T X + n * alpha * I) w = X^T y
    const x2: [10]f64 = [
        1.0, 2.0,
        2.0, 1.0,
        3.0, 4.0,
        4.0, 3.0,
        5.0, 6.0,
    ];
    const y2: [5]f64 = [5.0, 4.0, 11.0, 10.0, 17.0];
    const alpha_r: f64 = 0.5;
    const n2: usize = 5;

    // Compute exact 2x2 normal equations (X^T X + n * alpha * I) w = X^T y
    let a00: f64 = (n2 as f64) * alpha_r;
    let a01: f64 = 0.0;
    let a11: f64 = (n2 as f64) * alpha_r;
    let b0: f64 = 0.0;
    let b1: f64 = 0.0;
    let i: usize = 0;
    while (i < n2) {
        const x0: f64 = x2[i * 2];
        const x1: f64 = x2[i * 2 + 1];
        a00 += x0 * x0;
        a01 += x0 * x1;
        a11 += x1 * x1;
        b0 += x0 * y2[i];
        b1 += x1 * y2[i];
        i += 1;
    }
    const det: f64 = (a00 * a11) - (a01 * a01);
    const w0_exact: f64 = ((a11 * b0) - (a01 * b1)) / det;
    const w1_exact: f64 = ((a00 * b1) - (a01 * b0)) / det;

    let w2_b: [2]f64 = [0.0; 2];
    let r2_b: [5]f64 = [0.0; 5];
    let cn2_b: [2]f64 = [0.0; 2];
    let xm2_b: [2]f64 = [0.0; 2];
    let w2: []f64 = w2_b[0..];
    let ws2 = LassoWorkspace { residual: r2_b[0..], col_norm_sq: cn2_b[0..], x_mean: xm2_b[0..] };

    const res_r: LassoResult = ridgeFit(x2[0..], y2[0..], n2, 2, alpha_r, false, *w2, *ws2) ?? unreachable;
    expect_eq(res_r.converged, true);
    expect_eq(res_r.intercept, 0.0);
    expect_eq(absF64(w2[0] - w0_exact) < 1.0e-8, true);
    expect_eq(absF64(w2[1] - w1_exact) < 1.0e-8, true);

    // (b) ElasticNet grouping effect on strongly collinear pair (x0 == x1) + noise x2
    const n_c: usize = 12;
    let xc: [36]f64 = [0.0; 36];
    let yc: [12]f64 = [0.0; 12];
    let k: usize = 0;
    while (k < n_c) {
        const base: f64 = ((k as f64) - 5.5) * 0.5;
        const noise: f64 = if ((k % 2) == 0) 0.3 else -0.3;
        xc[k * 3 + 0] = base;
        xc[k * 3 + 1] = base;
        xc[k * 3 + 2] = noise;
        yc[k] = 4.0 * base;
        k += 1;
    }

    let wc_b: [3]f64 = [0.0; 3];
    let rc_b: [12]f64 = [0.0; 12];
    let cnc_b: [3]f64 = [0.0; 3];
    let xmc_b: [3]f64 = [0.0; 3];
    let wc: []f64 = wc_b[0..];
    let wsc = LassoWorkspace { residual: rc_b[0..], col_norm_sq: cnc_b[0..], x_mean: xmc_b[0..] };

    const cfg_en: LassoConfig = LassoConfig.defaultElasticNet(0.2, 0.5);
    const res_en: LassoResult = elasticNetFit(xc[0..], yc[0..], n_c, 3, cfg_en, *wc, *wsc) ?? unreachable;
    expect_eq(res_en.converged, true);
    // Grouping effect: identical collinear features receive equal shared positive weights, noise zeroed
    expect_eq(wc[0] > 1.0 && wc[1] > 1.0, true);
    expect_eq(absF64(wc[0] - wc[1]) < 1.0e-4, true);
    expect_eq(wc[2], 0.0);
});

test("3. Monotonic norm shrinkage with alpha & exact zero solution at alpha >= alpha_max", () => {
    const n: usize = 10;
    const p: usize = 3;
    let x: [30]f64 = [0.0; 30];
    let y: [10]f64 = [0.0; 10];
    let i: usize = 0;
    while (i < n) {
        const t: f64 = ((i as f64) - 4.5) * 0.4;
        x[i * 3 + 0] = t;
        x[i * 3 + 1] = t * t - 1.0;
        x[i * 3 + 2] = if ((i % 2) == 0) 0.5 else -0.5;
        y[i] = 2.0 * x[i * 3 + 0] - 1.0 * x[i * 3 + 1] + 0.5 * x[i * 3 + 2];
        i += 1;
    }

    let w_b: [3]f64 = [0.0; 3];
    let r_b: [10]f64 = [0.0; 10];
    let cn_b: [3]f64 = [0.0; 3];
    let xm_b: [3]f64 = [0.0; 3];
    let w: []f64 = w_b[0..];
    let ws = LassoWorkspace { residual: r_b[0..], col_norm_sq: cn_b[0..], x_mean: xm_b[0..] };

    // Monotonic L1 norm shrinkage for Lasso & L2 norm shrinkage for Ridge
    const alphas: [5]f64 = [0.001, 0.05, 0.2, 1.0, 5.0];
    let prev_l1: f64 = 1.0e30;
    let prev_l2: f64 = 1.0e30;
    let ai: usize = 0;
    while (ai < 5) {
        const rl: LassoResult = lassoFit(x[0..], y[0..], n, p, alphas[ai], true, *w, *ws) ?? unreachable;
        expect_eq(rl.converged, true);
        const cur_l1: f64 = l1Norm(w);
        expect_eq(cur_l1 <= prev_l1 + 1.0e-9, true);
        prev_l1 = cur_l1;

        const rr: LassoResult = ridgeFit(x[0..], y[0..], n, p, alphas[ai], true, *w, *ws) ?? unreachable;
        expect_eq(rr.converged, true);
        const cur_l2: f64 = l2Norm(w);
        expect_eq(cur_l2 <= prev_l2 + 1.0e-9, true);
        prev_l2 = cur_l2;
        ai += 1;
    }

    // Compute exact alpha_max = max_j |(1/n) * sum_i (x_ij - mean_x_j)(y_i - mean_y)|
    let y_mean: f64 = 0.0;
    let s: usize = 0;
    while (s < n) {
        y_mean += y[s];
        s += 1;
    }
    y_mean /= n as f64;

    let alpha_max: f64 = 0.0;
    let j: usize = 0;
    while (j < p) {
        let xm: f64 = 0.0;
        let k: usize = 0;
        while (k < n) {
            xm += x[k * p + j];
            k += 1;
        }
        xm /= n as f64;
        let dot: f64 = 0.0;
        let k2: usize = 0;
        while (k2 < n) {
            dot += (x[k2 * p + j] - xm) * (y[k2] - y_mean);
            k2 += 1;
        }
        const cand: f64 = absF64(dot) / (n as f64);
        if (cand > alpha_max) {
            alpha_max = cand;
        }
        j += 1;
    }

    const res_zero: LassoResult = lassoFit(x[0..], y[0..], n, p, alpha_max * 1.01, true, *w, *ws) ?? unreachable;
    expect_eq(res_zero.converged, true);
    expect_eq(res_zero.non_zero_count, 0);
    expect_eq(w[0], 0.0);
    expect_eq(w[1], 0.0);
    expect_eq(w[2], 0.0);
});

test("4. Exact intercept recovery (fit_intercept true vs false) & softThreshold properties", () => {
    expect_eq(softThreshold(3.5, 1.0), 2.5);
    expect_eq(softThreshold(-3.5, 1.0), -2.5);
    expect_eq(softThreshold(0.8, 1.0), 0.0);
    expect_eq(softThreshold(-0.8, 1.0), 0.0);
    expect_eq(softThreshold(2.0, 0.0), 2.0);

    // y = 2.0 * x0 - 3.0 * x1 + 12.5
    const x: [12]f64 = [
        1.0, 0.0,
        0.0, 1.0,
        2.0, 1.0,
        -1.0, 2.0,
        3.0, -1.0,
        -2.0, -2.0,
    ];
    const y: [6]f64 = [14.5, 9.5, 13.5, 4.5, 21.5, 14.5];

    let w_b: [2]f64 = [0.0; 2];
    let r_b: [6]f64 = [0.0; 6];
    let cn_b: [2]f64 = [0.0; 2];
    let xm_b: [2]f64 = [0.0; 2];
    let w: []f64 = w_b[0..];
    let ws = LassoWorkspace { residual: r_b[0..], col_norm_sq: cn_b[0..], x_mean: xm_b[0..] };

    // fit_intercept = true with tiny regularization recovers exact weights and intercept = 12.5
    const res_t: LassoResult = lassoFit(x[0..], y[0..], 6, 2, 0.0, true, *w, *ws) ?? unreachable;
    expect_eq(res_t.converged, true);
    expect_eq(absF64(w[0] - 2.0) < 1.0e-7, true);
    expect_eq(absF64(w[1] - (-3.0)) < 1.0e-7, true);
    expect_eq(absF64(res_t.intercept - 12.5) < 1.0e-7, true);
    expect_eq(res_t.mse < 1.0e-12, true);

    // fit_intercept = false on origin-passing line y = 2*x0 - 3*x1
    const y0: [6]f64 = [2.0, -3.0, 1.0, -8.0, 9.0, 2.0];
    const res_f: LassoResult = lassoFit(x[0..], y0[0..], 6, 2, 0.0, false, *w, *ws) ?? unreachable;
    expect_eq(res_f.converged, true);
    expect_eq(res_f.intercept, 0.0);
    expect_eq(absF64(w[0] - 2.0) < 1.0e-7, true);
    expect_eq(absF64(w[1] - (-3.0)) < 1.0e-7, true);
});

test("5. Boundary defense: negative alpha, invalid l1_ratio, zero dims, short buffers & NaN/Inf", () => {
    const x_ok: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const y_ok: [2]f64 = [1.0, 2.0];
    let w_b: [2]f64 = [0.0; 2];
    let r_b: [2]f64 = [0.0; 2];
    let cn_b: [2]f64 = [0.0; 2];
    let xm_b: [2]f64 = [0.0; 2];
    let w: []f64 = w_b[0..];
    let ws = LassoWorkspace { residual: r_b[0..], col_norm_sq: cn_b[0..], x_mean: xm_b[0..] };

    // Negative alpha & out-of-range l1_ratio
    expect_eq(lassoFit(x_ok[0..], y_ok[0..], 2, 2, -0.1, true, *w, *ws) == null, true);
    expect_eq(elasticNetFit(x_ok[0..], y_ok[0..], 2, 2, LassoConfig.defaultElasticNet(0.1, -0.01), *w, *ws) == null, true);
    expect_eq(elasticNetFit(x_ok[0..], y_ok[0..], 2, 2, LassoConfig.defaultElasticNet(0.1, 1.01), *w, *ws) == null, true);

    // Zero samples / features / max_iter
    expect_eq(lassoFit(x_ok[0..], y_ok[0..], 0, 2, 0.1, true, *w, *ws) == null, true);
    expect_eq(lassoFit(x_ok[0..], y_ok[0..], 2, 0, 0.1, true, *w, *ws) == null, true);
    let zero_iter: LassoConfig = LassoConfig.defaultLasso(0.1);
    zero_iter.max_iter = 0;
    expect_eq(elasticNetFit(x_ok[0..], y_ok[0..], 2, 2, zero_iter, *w, *ws) == null, true);

    // Short buffers
    let short_w: []f64 = w_b[0..1];
    let short_ws = LassoWorkspace { residual: r_b[0..1], col_norm_sq: cn_b[0..2], x_mean: xm_b[0..2] };
    expect_eq(lassoFit(x_ok[0..], y_ok[0..], 2, 2, 0.1, true, *short_w, *ws) == null, true);
    expect_eq(lassoFit(x_ok[0..], y_ok[0..], 2, 2, 0.1, true, *w, *short_ws) == null, true);

    // NaN / Inf in X, y, predict, scores
    const z: f64 = 0.0;
    const nan_v: f64 = z / z;
    const inf_v: f64 = 1.0 / z;
    const x_nan: [4]f64 = [1.0, nan_v, 3.0, 4.0];
    const y_inf: [2]f64 = [1.0, inf_v];
    expect_eq(lassoFit(x_nan[0..], y_ok[0..], 2, 2, 0.1, true, *w, *ws) == null, true);
    expect_eq(lassoFit(x_ok[0..], y_inf[0..], 2, 2, 0.1, true, *w, *ws) == null, true);
    expect_eq(linearPredict(w, nan_v, x_ok[0..2]) == null, true);
    expect_eq(linearPredict(w, 0.0, x_ok[0..1]) == null, true);
    expect_eq(mseScore(y_ok[0..], y_inf[0..]) == null, true);
    expect_eq(r2Score(y_ok[0..], y_inf[0..]) == null, true);
    expect_eq(r2Score(y_ok[0..1], y_ok[0..2]) == null, true);

    // Constant y_true R^2 edge case (no div-by-zero)
    const c_true: [3]f64 = [5.0, 5.0, 5.0];
    const c_pred: [3]f64 = [5.0, 5.0, 5.0];
    expect_eq(r2Score(c_true[0..], c_pred[0..]) ?? unreachable, 1.0);
});
