// Source: cp/cap/test/test_isotonic_production_deep.cp (Production deep tests for cap:isotonic)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    PavaWorkspace,
    pavaIncreasing,
    pavaDecreasing,
    fitIsotonic,
    isotonicPredict,
    brierScore,
    expectedCalibrationError,
} from "cap:ml";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

test("1. Classic PAVA [1, 4, 2, 3, 5] -> [1, 3, 3, 3, 5] and multi-level cascade merge", () => {
    const y = [1.0, 4.0, 2.0, 3.0, 5.0];
    const empty_w: [0]f64 = [];
    let out_arr: [5]f64 = std.mem.zeroes([5]f64);
    let swy: [5]f64 = std.mem.zeroes([5]f64);
    let sw: [5]f64 = std.mem.zeroes([5]f64);
    let sc: [5]usize = std.mem.zeroes([5]usize);
    let se: [5]usize = std.mem.zeroes([5]usize);
    let ws = PavaWorkspace {
        block_sum_wy: swy[0..],
        block_sum_w: sw[0..],
        block_count: sc[0..],
        block_end: se[0..],
    };
    let out_s: []f64 = out_arr[0..];

    expect_eq(pavaIncreasing(y[0..], empty_w[0..], &out_s, &ws), true);
    const expected = [1.0, 3.0, 3.0, 3.0, 5.0];
    let i: usize = 0;
    while (i < 5) {
        expect_eq(absF64(out_arr[i] - expected[i]) < 1.0e-12, true);
        i += 1;
    }

    // Cascade merge: [10, 8, 9, 1, 2] -> all merge to mean = 30 / 5 = 6.0
    const y2 = [10.0, 8.0, 9.0, 1.0, 2.0];
    expect_eq(pavaIncreasing(y2[0..], empty_w[0..], &out_s, &ws), true);
    i = 0;
    while (i < 5) {
        expect_eq(absF64(out_arr[i] - 6.0) < 1.0e-12, true);
        i += 1;
    }
});

test("2. Weighted PAVA & KKT optimality verification + pavaDecreasing negation symmetry", () => {
    const y = [2.0, 5.0, 1.0, 3.0, 6.0, 4.0];
    const w = [1.0, 3.0, 1.0, 2.0, 1.0, 4.0];
    let f_inc: [6]f64 = std.mem.zeroes([6]f64);
    let swy: [6]f64 = std.mem.zeroes([6]f64);
    let sw: [6]f64 = std.mem.zeroes([6]f64);
    let sc: [6]usize = std.mem.zeroes([6]usize);
    let se: [6]usize = std.mem.zeroes([6]usize);
    let ws = PavaWorkspace {
        block_sum_wy: swy[0..],
        block_sum_w: sw[0..],
        block_count: sc[0..],
        block_end: se[0..],
    };
    let f_inc_s: []f64 = f_inc[0..];
    expect_eq(pavaIncreasing(y[0..], w[0..], &f_inc_s, &ws), true);

    // Monotonicity & KKT conditions:
    // 1) f[i] <= f[i+1]
    // 2) prefix weighted residual sum <= 0, and == 0 at strict jump points and at n-1.
    let prefix_res: f64 = 0.0;
    let i: usize = 0;
    while (i < 6) {
        prefix_res += w[i] * (f_inc[i] - y[i]);
        expect_eq(prefix_res <= 1.0e-11, true);
        if (i + 1 < 6) {
            expect_eq(f_inc[i] <= f_inc[i + 1] + 1.0e-12, true);
            if (f_inc[i] + 1.0e-12 < f_inc[i + 1]) {
                expect_eq(absF64(prefix_res) < 1.0e-11, true);
            }
        }
        i += 1;
    }
    expect_eq(absF64(prefix_res) < 1.0e-11, true);

    // Decreasing PAVA must equal -pavaIncreasing(-y, w)
    let neg_y: [6]f64 = std.mem.zeroes([6]f64);
    i = 0;
    while (i < 6) {
        neg_y[i] = 0.0 - y[i];
        i += 1;
    }
    let f_dec: [6]f64 = std.mem.zeroes([6]f64);
    let f_neg_inc: [6]f64 = std.mem.zeroes([6]f64);
    let f_dec_s: []f64 = f_dec[0..];
    let f_neg_inc_s: []f64 = f_neg_inc[0..];

    expect_eq(pavaDecreasing(y[0..], w[0..], &f_dec_s, &ws), true);
    expect_eq(pavaIncreasing(neg_y[0..], w[0..], &f_neg_inc_s, &ws), true);
    i = 0;
    while (i < 6) {
        expect_eq(absF64(f_dec[i] + f_neg_inc[i]) < 1.0e-12, true);
        if (i + 1 < 6) {
            expect_eq(f_dec[i] >= f_dec[i + 1] - 1.0e-12, true);
        }
        i += 1;
    }
});

test("3. Unsorted fitIsotonic with duplicate x knots, linear interpolation, and boundary clamping", () => {
    // Unsorted with duplicate x = 2.0 (y = 1.0, 5.0 -> merged mean = 3.0, weight = 2)
    // Sorted unique x: [0.0, 2.0, 4.0, 6.0], merged y: [1.0, 3.0, 2.0, 7.0], weights: [1, 2, 1, 1]
    // PAVA on (2.0, 4.0): (2*3 + 1*2)/(2+1) = 8/3 = 2.666666...
    const x = [4.0, 2.0, 6.0, 0.0, 2.0];
    const y = [2.0, 1.0, 7.0, 1.0, 5.0];
    const empty_w: [0]f64 = [];

    let xk: [5]f64 = std.mem.zeroes([5]f64);
    let yk: [5]f64 = std.mem.zeroes([5]f64);
    let ord: [5]usize = std.mem.zeroes([5]usize);
    let ys: [5]f64 = std.mem.zeroes([5]f64);
    let ws_arr: [5]f64 = std.mem.zeroes([5]f64);
    let swy: [5]f64 = std.mem.zeroes([5]f64);
    let sw: [5]f64 = std.mem.zeroes([5]f64);
    let sc: [5]usize = std.mem.zeroes([5]usize);
    let se: [5]usize = std.mem.zeroes([5]usize);
    let pava_ws = PavaWorkspace {
        block_sum_wy: swy[0..],
        block_sum_w: sw[0..],
        block_count: sc[0..],
        block_end: se[0..],
    };

    let xk_s: []f64 = xk[0..];
    let yk_s: []f64 = yk[0..];
    let ord_s: []usize = ord[0..];
    let ys_s: []f64 = ys[0..];
    let ws_s: []f64 = ws_arr[0..];

    const model = fitIsotonic(x[0..], y[0..], empty_w[0..], true, &xk_s, &yk_s, &ord_s, &ys_s, &ws_s, &pava_ws) ?? unreachable;
    expect_eq(model.knot_count, 4);
    expect_eq(model.increasing, true);
    expect_eq(absF64(model.y_min - 1.0) < 1.0e-12, true);
    expect_eq(absF64(model.y_max - 7.0) < 1.0e-12, true);

    const expected_mid: f64 = 8.0 / 3.0;
    expect_eq(absF64(isotonicPredict(model, 0.0) - 1.0) < 1.0e-12, true);
    expect_eq(absF64(isotonicPredict(model, 2.0) - expected_mid) < 1.0e-12, true);
    expect_eq(absF64(isotonicPredict(model, 4.0) - expected_mid) < 1.0e-12, true);
    expect_eq(absF64(isotonicPredict(model, 6.0) - 7.0) < 1.0e-12, true);

    // Linear interpolation at x = 1.0 (midpoint between (0, 1) and (2, 8/3)) -> 11/6
    expect_eq(absF64(isotonicPredict(model, 1.0) - (11.0 / 6.0)) < 1.0e-12, true);
    // Linear interpolation at x = 5.0 (midpoint between (4, 8/3) and (6, 7)) -> 29/6
    expect_eq(absF64(isotonicPredict(model, 5.0) - (29.0 / 6.0)) < 1.0e-12, true);

    // Out-of-range clamping
    expect_eq(absF64(isotonicPredict(model, -10.0) - 1.0) < 1.0e-12, true);
    expect_eq(absF64(isotonicPredict(model, 100.0) - 7.0) < 1.0e-12, true);
});

test("4. Probability calibration: isotonic calibration significantly reduces Brier score & ECE", () => {
    // Miscalibrated raw probabilities (systematically shifted high / overconfident)
    const raw_probs: [16]f64 = [
        0.35, 0.40, 0.45, 0.50,
        0.55, 0.60, 0.65, 0.70,
        0.75, 0.80, 0.85, 0.88,
        0.90, 0.92, 0.95, 0.98,
    ];
    const y_bin: [16]f64 = [
        0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 1.0, 0.0, 1.0,
        1.0, 1.0, 1.0, 1.0,
    ];
    const empty_w: [0]f64 = [];

    let bin_cnt: [5]usize = std.mem.zeroes([5]usize);
    let bin_prob: [5]f64 = std.mem.zeroes([5]f64);
    let bin_acc: [5]f64 = std.mem.zeroes([5]f64);
    let bc_s: []usize = bin_cnt[0..];
    let bp_s: []f64 = bin_prob[0..];
    let ba_s: []f64 = bin_acc[0..];

    const raw_brier = brierScore(raw_probs[0..], y_bin[0..]) ?? unreachable;
    const raw_ece = expectedCalibrationError(raw_probs[0..], y_bin[0..], 5, &bc_s, &bp_s, &ba_s) ?? unreachable;

    let xk: [16]f64 = std.mem.zeroes([16]f64);
    let yk: [16]f64 = std.mem.zeroes([16]f64);
    let ord: [16]usize = std.mem.zeroes([16]usize);
    let ys: [16]f64 = std.mem.zeroes([16]f64);
    let ws_arr: [16]f64 = std.mem.zeroes([16]f64);
    let swy: [16]f64 = std.mem.zeroes([16]f64);
    let sw: [16]f64 = std.mem.zeroes([16]f64);
    let sc: [16]usize = std.mem.zeroes([16]usize);
    let se: [16]usize = std.mem.zeroes([16]usize);
    let pava_ws = PavaWorkspace {
        block_sum_wy: swy[0..],
        block_sum_w: sw[0..],
        block_count: sc[0..],
        block_end: se[0..],
    };
    let xk_s: []f64 = xk[0..];
    let yk_s: []f64 = yk[0..];
    let ord_s: []usize = ord[0..];
    let ys_s: []f64 = ys[0..];
    let ws_s: []f64 = ws_arr[0..];

    const cal = fitIsotonic(raw_probs[0..], y_bin[0..], empty_w[0..], true, &xk_s, &yk_s, &ord_s, &ys_s, &ws_s, &pava_ws) ?? unreachable;
    let cal_probs: [16]f64 = std.mem.zeroes([16]f64);
    let i: usize = 0;
    while (i < 16) {
        cal_probs[i] = isotonicPredict(cal, raw_probs[i]);
        expect_eq(cal_probs[i] >= 0.0 && cal_probs[i] <= 1.0, true);
        i += 1;
    }

    const cal_brier = brierScore(cal_probs[0..], y_bin[0..]) ?? unreachable;
    const cal_ece = expectedCalibrationError(cal_probs[0..], y_bin[0..], 5, &bc_s, &bp_s, &ba_s) ?? unreachable;

    expect_eq(cal_brier < raw_brier * 0.6, true);
    expect_eq(cal_ece < raw_ece * 0.4, true);
});

test("5. Defensive guards: negative/zero weights, empty inputs, short buffers, and NaN", () => {
    const empty_f: [0]f64 = [];
    const y_ok = [1.0, 2.0, 3.0];
    const w_neg = [1.0, -0.5, 1.0];
    const w_zero = [1.0, 0.0, 1.0];
    const zero_f: f64 = 0.0;
    const nan_f: f64 = zero_f / zero_f;
    const y_nan = [1.0, nan_f, 3.0];

    let out_arr: [3]f64 = std.mem.zeroes([3]f64);
    let out_short_arr: [2]f64 = std.mem.zeroes([2]f64);
    let swy: [3]f64 = std.mem.zeroes([3]f64);
    let sw: [3]f64 = std.mem.zeroes([3]f64);
    let sc: [3]usize = std.mem.zeroes([3]usize);
    let se: [3]usize = std.mem.zeroes([3]usize);
    let ws = PavaWorkspace {
        block_sum_wy: swy[0..],
        block_sum_w: sw[0..],
        block_count: sc[0..],
        block_end: se[0..],
    };
    let out_s: []f64 = out_arr[0..];
    let out_short: []f64 = out_short_arr[0..];

    expect_eq(pavaIncreasing(empty_f[0..], empty_f[0..], &out_s, &ws), false);
    expect_eq(pavaIncreasing(y_ok[0..], w_neg[0..], &out_s, &ws), false);
    expect_eq(pavaIncreasing(y_ok[0..], w_zero[0..], &out_s, &ws), false);
    expect_eq(pavaIncreasing(y_nan[0..], empty_f[0..], &out_s, &ws), false);
    expect_eq(pavaIncreasing(y_ok[0..], empty_f[0..], &out_short, &ws), false);

    // brierScore & ECE guards
    expect_eq(brierScore(empty_f[0..], empty_f[0..]) == null, true);
    expect_eq(brierScore(y_nan[0..], y_ok[0..]) == null, true);
    expect_eq(brierScore([0.2, 0.8][0..], y_ok[0..]) == null, true);

    let bc: [2]usize = std.mem.zeroes([2]usize);
    let bp: [2]f64 = std.mem.zeroes([2]f64);
    let ba: [2]f64 = std.mem.zeroes([2]f64);
    let bc_s: []usize = bc[0..];
    let bp_s: []f64 = bp[0..];
    let ba_s: []f64 = ba[0..];

    expect_eq(expectedCalibrationError(y_ok[0..], y_ok[0..], 0, &bc_s, &bp_s, &ba_s) == null, true);
    expect_eq(expectedCalibrationError(y_ok[0..], y_ok[0..], 5, &bc_s, &bp_s, &ba_s) == null, true);
    expect_eq(expectedCalibrationError(y_nan[0..], y_ok[0..], 2, &bc_s, &bp_s, &ba_s) == null, true);
});
