// Source: cp/cap/test/test_huber_production_deep.cp (Production deep tests for cap:huber)

import { test, expect_eq } from "cap:test";
import {
    RobustKind,
    HuberRegConfig,
    HuberRegResult,
    HuberRegWorkspace,
    TheilSenResult,
    median,
    madScale,
    trimmedMean,
    huberLocation,
    huberLoss,
    huberWeight,
    tukeyBiweight,
    huberRegress,
    theilSen1D,
} from "cap:huber";

fn absF64(v: f64): f64 {
    return if (v < 0.0) (0.0 - v) else v;
}

test("1. Robust location & scale resist extreme +1e6 outlier contamination", () => {
    // Clean core centered around 10.0 with spread ~ 1.0, plus 1 extreme outlier (+1e6)
    const data: [11]f64 = [
        8.8, 9.2, 9.5, 9.8, 10.0, 10.0, 10.2, 10.5, 10.8, 11.2, 1.0e6,
    ];
    let s1_b: [11]f64 = [0.0; 11];
    let s2_b: [11]f64 = [0.0; 11];
    let s1: []f64 = s1_b[0..];
    let s2: []f64 = s2_b[0..];

    const med: f64 = median(data[0..], *s1) ?? unreachable;
    const mad: f64 = madScale(data[0..], *s1, *s2) ?? unreachable;
    const tmean: f64 = trimmedMean(data[0..], 0.2, *s1) ?? unreachable;
    const hloc: f64 = huberLocation(data[0..], 1.345, 50, 1.0e-8, *s1, *s2) ?? unreachable;

    expect_eq(absF64(med - 10.0) < 1.0e-12, true);
    expect_eq(mad > 0.5 && mad < 1.5, true);
    expect_eq(absF64(tmean - 10.0) < 0.25, true);
    expect_eq(absF64(hloc - 10.0) < 0.35, true);

    // Loss and weight primitives
    expect_eq(absF64(huberLoss(1.0, 2.0) - 0.5) < 1.0e-12, true);
    expect_eq(absF64(huberLoss(4.0, 2.0) - 6.0) < 1.0e-12, true);
    expect_eq(huberWeight(1.0, 2.0), 1.0);
    expect_eq(absF64(huberWeight(-4.0, 2.0) - 0.5) < 1.0e-12, true);
    expect_eq(tukeyBiweight(0.0, 4.685), 1.0);
    expect_eq(tukeyBiweight(4.685, 4.685), 0.0);
    expect_eq(tukeyBiweight(-10.0, 4.685), 0.0);
});

test("2. Multivariate IRLS Huber & Tukey regression resist 20% severe y-outliers", () => {
    // 25 samples, 2 features, true model: y = 2.0 * x0 - 3.0 * x1 + 1.5
    // Inject 5 severe outliers (20% contamination) with +/- 200..300 corruption.
    const n: usize = 25;
    const d: usize = 2;
    let x: [50]f64 = [0.0; 50];
    let y: [25]f64 = [0.0; 25];

    let i: usize = 0;
    while (i < n) {
        const x0: f64 = ((i % 5) as f64) - 2.0;
        const x1: f64 = ((i / 5) as f64) - 2.0;
        const tiny_noise: f64 = if ((i % 2) == 0) 0.02 else -0.02;
        x[i * 2 + 0] = x0;
        x[i * 2 + 1] = x1;
        let yi: f64 = (2.0 * x0) - (3.0 * x1) + 1.5 + tiny_noise;
        if (i == 2 || i == 7 || i == 12 || i == 17 || i == 22) {
            yi += if ((i % 2) == 0) 250.0 else -220.0;
        }
        y[i] = yi;
        i += 1;
    }

    let w_h_b: [2]f64 = [0.0; 2];
    let w_t_b: [2]f64 = [0.0; 2];
    let w_h: []f64 = w_h_b[0..];
    let w_t: []f64 = w_t_b[0..];

    let wt_b: [25]f64 = [0.0; 25];
    let res_b: [25]f64 = [0.0; 25];
    let ata_b: [9]f64 = [0.0; 9];
    let atb_b: [3]f64 = [0.0; 3];
    let beta_b: [3]f64 = [0.0; 3];
    let pbeta_b: [3]f64 = [0.0; 3];
    let piv_b: [3]usize = [0; 3];
    let sa_b: [25]f64 = [0.0; 25];
    let sb_b: [25]f64 = [0.0; 25];

    let ws = HuberRegWorkspace {
        weights: wt_b[0..],
        residuals: res_b[0..],
        ata: ata_b[0..],
        atb: atb_b[0..],
        beta: beta_b[0..],
        prev_beta: pbeta_b[0..],
        piv: piv_b[0..],
        scratch_a: sa_b[0..],
        scratch_b: sb_b[0..],
    };

    // (a) Huber IRLS
    const cfg_h: HuberRegConfig = HuberRegConfig.defaultHuber();
    const res_h: HuberRegResult = huberRegress(x[0..], y[0..], n, d, cfg_h, *w_h, *ws) ?? unreachable;
    expect_eq(res_h.converged, true);
    expect_eq(absF64(w_h[0] - 2.0) < 0.05, true);
    expect_eq(absF64(w_h[1] - (-3.0)) < 0.05, true);
    expect_eq(absF64(res_h.intercept - 1.5) < 0.05, true);

    // (b) Tukey Biweight IRLS: completely zeroes outlier weights for near-exact recovery
    const cfg_t: HuberRegConfig = HuberRegConfig.defaultTukey();
    const res_t: HuberRegResult = huberRegress(x[0..], y[0..], n, d, cfg_t, *w_t, *ws) ?? unreachable;
    expect_eq(res_t.converged, true);
    expect_eq(absF64(w_t[0] - 2.0) < 0.02, true);
    expect_eq(absF64(w_t[1] - (-3.0)) < 0.02, true);
    expect_eq(absF64(res_t.intercept - 1.5) < 0.02, true);
    // All 5 severe outliers must have weight 0.0 under Tukey biweight
    expect_eq(ws.weights[2], 0.0);
    expect_eq(ws.weights[7], 0.0);
    expect_eq(ws.weights[12], 0.0);
    expect_eq(ws.weights[17], 0.0);
    expect_eq(ws.weights[22], 0.0);
});

test("3. Theil-Sen 1D recovers exact y = 2.5x - 1.0 under 25% severe outlier contamination", () => {
    // 20 points: 15 exact inliers on y = 2.5 * x - 1.0, 5 gross outliers (25% < 29.3% breakdown)
    const n: usize = 20;
    let xs: [20]f64 = [0.0; 20];
    let ys: [20]f64 = [0.0; 20];
    let i: usize = 0;
    while (i < n) {
        const xv: f64 = (i as f64) - 10.0;
        xs[i] = xv;
        if (i == 1 || i == 5 || i == 10 || i == 14 || i == 18) {
            ys[i] = if ((i % 2) == 0) 500.0 else -600.0;
        } else {
            ys[i] = (2.5 * xv) - 1.0;
        }
        i += 1;
    }

    let slopes_b: [190]f64 = [0.0; 190];
    let ints_b: [20]f64 = [0.0; 20];
    let slopes: []f64 = slopes_b[0..];
    let ints: []f64 = ints_b[0..];

    const ts: TheilSenResult = theilSen1D(xs[0..], ys[0..], *slopes, *ints) ?? unreachable;
    expect_eq(absF64(ts.slope - 2.5) < 1.0e-12, true);
    expect_eq(absF64(ts.intercept - (-1.0)) < 1.0e-12, true);
});

test("4. Clean data without outliers: Huber regression matches exact OLS solution", () => {
    // Exact linear data: y = -1.5 * x0 + 4.0 * x1 + 3.25
    const x: [12]f64 = [
        1.0, 0.0,
        0.0, 1.0,
        2.0, 1.0,
        -1.0, 2.0,
        3.0, -1.0,
        -2.0, -2.0,
    ];
    const y: [6]f64 = [1.75, 7.25, 4.25, 12.75, -5.25, -1.75];

    let w_b: [2]f64 = [0.0; 2];
    let w: []f64 = w_b[0..];
    let wt_b: [6]f64 = [0.0; 6];
    let res_b: [6]f64 = [0.0; 6];
    let ata_b: [9]f64 = [0.0; 9];
    let atb_b: [3]f64 = [0.0; 3];
    let beta_b: [3]f64 = [0.0; 3];
    let pbeta_b: [3]f64 = [0.0; 3];
    let piv_b: [3]usize = [0; 3];
    let sa_b: [6]f64 = [0.0; 6];
    let sb_b: [6]f64 = [0.0; 6];

    let ws = HuberRegWorkspace {
        weights: wt_b[0..],
        residuals: res_b[0..],
        ata: ata_b[0..],
        atb: atb_b[0..],
        beta: beta_b[0..],
        prev_beta: pbeta_b[0..],
        piv: piv_b[0..],
        scratch_a: sa_b[0..],
        scratch_b: sb_b[0..],
    };

    const cfg: HuberRegConfig = HuberRegConfig.defaultHuber();
    const res: HuberRegResult = huberRegress(x[0..], y[0..], 6, 2, cfg, *w, *ws) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(absF64(w[0] - (-1.5)) < 1.0e-10, true);
    expect_eq(absF64(w[1] - 4.0) < 1.0e-10, true);
    expect_eq(absF64(res.intercept - 3.25) < 1.0e-10, true);
    expect_eq(res.loss < 1.0e-12, true);
});

test("5. Boundary defense: delta <= 0, empty inputs, invalid trim, short buffers & NaN/Inf", () => {
    const empty_f: [0]f64 = [];
    const ok_data: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    let s1_b: [4]f64 = [0.0; 4];
    let s2_b: [4]f64 = [0.0; 4];
    let s1: []f64 = s1_b[0..];
    let s2: []f64 = s2_b[0..];
    let s_short: []f64 = s1_b[0..2];

    const z: f64 = 0.0;
    const nan_v: f64 = z / z;
    const inf_v: f64 = 1.0 / z;
    const nan_data: [4]f64 = [1.0, nan_v, 3.0, 4.0];

    // Location & scale boundary checks
    expect_eq(median(empty_f[0..], *s1) == null, true);
    expect_eq(median(ok_data[0..], *s_short) == null, true);
    expect_eq(median(nan_data[0..], *s1) == null, true);
    expect_eq(madScale(empty_f[0..], *s1, *s2) == null, true);
    expect_eq(trimmedMean(ok_data[0..], 0.5, *s1) == null, true);
    expect_eq(trimmedMean(ok_data[0..], -0.1, *s1) == null, true);
    expect_eq(huberLocation(ok_data[0..], 0.0, 20, 1.0e-6, *s1, *s2) == null, true);

    // Regression boundary checks
    const x_ok: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const y_ok: [4]f64 = [2.0, 4.0, 6.0, 8.0];
    let w_b: [1]f64 = [0.0; 1];
    let w: []f64 = w_b[0..];
    let wt_b: [4]f64 = [0.0; 4];
    let res_b: [4]f64 = [0.0; 4];
    let ata_b: [4]f64 = [0.0; 4];
    let atb_b: [2]f64 = [0.0; 2];
    let beta_b: [2]f64 = [0.0; 2];
    let pbeta_b: [2]f64 = [0.0; 2];
    let piv_b: [2]usize = [0; 2];

    let ws = HuberRegWorkspace {
        weights: wt_b[0..],
        residuals: res_b[0..],
        ata: ata_b[0..],
        atb: atb_b[0..],
        beta: beta_b[0..],
        prev_beta: pbeta_b[0..],
        piv: piv_b[0..],
        scratch_a: s1_b[0..],
        scratch_b: s2_b[0..],
    };

    let bad_delta: HuberRegConfig = HuberRegConfig.defaultHuber();
    bad_delta.delta = 0.0;
    expect_eq(huberRegress(x_ok[0..], y_ok[0..], 4, 1, bad_delta, *w, *ws) == null, true);

    let bad_l2: HuberRegConfig = HuberRegConfig.defaultHuber();
    bad_l2.l2_reg = -1.0;
    expect_eq(huberRegress(x_ok[0..], y_ok[0..], 4, 1, bad_l2, *w, *ws) == null, true);

    const y_inf: [4]f64 = [2.0, inf_v, 6.0, 8.0];
    const cfg_ok: HuberRegConfig = HuberRegConfig.defaultHuber();
    expect_eq(huberRegress(x_ok[0..], y_inf[0..], 4, 1, cfg_ok, *w, *ws) == null, true);
    expect_eq(huberRegress(x_ok[0..], y_ok[0..], 0, 1, cfg_ok, *w, *ws) == null, true);

    // Theil-Sen boundary checks: vertical line (identical xs), short scratch, < 2 samples
    const vert_x: [3]f64 = [2.0, 2.0, 2.0];
    const vert_y: [3]f64 = [1.0, 2.0, 3.0];
    let sl_b: [6]f64 = [0.0; 6];
    let in_b: [4]f64 = [0.0; 4];
    let sl: []f64 = sl_b[0..];
    let ints: []f64 = in_b[0..];
    let sl_short: []f64 = sl_b[0..2];
    expect_eq(theilSen1D(vert_x[0..], vert_y[0..], *sl, *ints) == null, true);
    expect_eq(theilSen1D(x_ok[0..], y_ok[0..], *sl_short, *ints) == null, true);
    expect_eq(theilSen1D(x_ok[0..1], y_ok[0..1], *sl, *ints) == null, true);
});
