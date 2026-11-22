// Source: cp/cap/test/test_holt_production_deep.cp (Production deep tests for cap:holt)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SeasonalityKind,
    HoltConfig,
    HoltWintersConfig,
    sesForecast,
    holtForecast,
    holtWintersForecast,
    evaluateForecast,
} from "cap:math";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

test("1. SES tracks constant & step series accurately (including empty fitted_out skip)", () => {
    // Constant series
    const constant: [10]f64 = [42.0; 10];
    let empty_arr: [0]f64 = [];
    let empty_fit: []f64 = empty_arr[0..];
    let fc_buf: [4]f64 = [0.0; 4];
    let fc_s: []f64 = fc_buf[0..];

    if (res := sesForecast(constant[0..], 0.4, 4, *empty_fit, *fc_s)) {
        expect_eq(absF64(res.level - 42.0) < 1.0e-12, true);
        expect_eq(res.sse < 1.0e-24, true);
        expect_eq(res.rmse < 1.0e-12, true);
        let h: usize = 0;
        while (h < 4) {
            expect_eq(absF64(fc_s[h] - 42.0) < 1.0e-12, true);
            h += 1;
        }
    } else {
        expect_eq(true, false);
    }

    // Step series: 10.0 for 10 steps, then 20.0 for 20 steps
    let step_s: [30]f64 = [10.0; 30];
    let i: usize = 10;
    while (i < 30) {
        step_s[i] = 20.0;
        i += 1;
    }
    let fit_buf: [30]f64 = [0.0; 30];
    let fit_s: []f64 = fit_buf[0..];
    if (res2 := sesForecast(step_s[0..], 0.5, 3, *fit_s, *fc_s)) {
        expect_eq(absF64(res2.level - 20.0) < 1.0e-4, true);
        expect_eq(absF64(fc_s[0] - res2.level) < 1.0e-12, true);
        expect_eq(absF64(fit_s[0] - 10.0) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }
});

test("2. Holt linear trend predicts pure ramp y_t = 10 + 2.5*t 5 steps ahead with < 1e-10 error", () => {
    let ramp: [12]f64 = [0.0; 12];
    let t: usize = 0;
    while (t < 12) {
        ramp[t] = 10.0 + 2.5 * (t as f64);
        t += 1;
    }

    let fit_buf: [12]f64 = [0.0; 12];
    let fc_buf: [5]f64 = [0.0; 5];
    let fit_s: []f64 = fit_buf[0..];
    let fc_s: []f64 = fc_buf[0..];

    if (res := holtForecast(ramp[0..], HoltConfig.default(), 5, *fit_s, *fc_s)) {
        expect_eq(res.rmse < 1.0e-12, true);
        expect_eq(absF64(res.trend - 2.5) < 1.0e-12, true);
        let h: usize = 0;
        while (h < 5) {
            const expected = 10.0 + 2.5 * ((12 + h) as f64);
            expect_eq(absF64(fc_s[h] - expected) < 1.0e-10, true);
            h += 1;
        }
    } else {
        expect_eq(true, false);
    }
});

test("3. Damped Holt (phi = 0.8) asymptotically converges to a finite plateau", () => {
    let ramp: [12]f64 = [0.0; 12];
    let t: usize = 0;
    while (t < 12) {
        ramp[t] = 10.0 + 2.0 * (t as f64);
        t += 1;
    }

    let empty_arr: [0]f64 = [];
    let empty_fit: []f64 = empty_arr[0..];
    let fc_buf: [60]f64 = [0.0; 60];
    let fc_s: []f64 = fc_buf[0..];

    const cfg: HoltConfig = { alpha: 0.5, beta: 0.3, phi: 0.8 };
    if (res := holtForecast(ramp[0..], cfg, 60, *empty_fit, *fc_s)) {
        // Monotonically increasing with strictly shrinking increments
        const d0 = fc_s[1] - fc_s[0];
        const d10 = fc_s[11] - fc_s[10];
        const d50 = fc_s[51] - fc_s[50];
        expect_eq(d0 > d10 && d10 > d50 && d50 > 0.0, true);

        // Converges to analytic asymptote: level + (phi / (1 - phi)) * trend = level + 4 * trend
        const plateau = res.level + 4.0 * res.trend;
        expect_eq(absF64(fc_s[59] - plateau) < 1.0e-4, true);
    } else {
        expect_eq(true, false);
    }
});

test("4. Holt-Winters Additive & Multiplicative forecast next full cycle accurately (< 1e-6)", () => {
    const period: usize = 4;

    // 4a. Additive: y_t = (50 + 3*t) + s[t % 4], sum(s) = 0
    const s_add: [4]f64 = [-6.0, 10.0, 2.0, -6.0];
    let y_add: [16]f64 = [0.0; 16];
    let t: usize = 0;
    while (t < 16) {
        y_add[t] = 50.0 + 3.0 * (t as f64) + s_add[t % period];
        t += 1;
    }

    let seas_buf: [4]f64 = [0.0; 4];
    let fit_buf: [16]f64 = [0.0; 16];
    let fc_buf: [4]f64 = [0.0; 4];
    let seas_s: []f64 = seas_buf[0..];
    let fit_s: []f64 = fit_buf[0..];
    let fc_s: []f64 = fc_buf[0..];

    const cfg_add: HoltWintersConfig = {
        alpha: 0.4,
        beta: 0.2,
        gamma: 0.3,
        phi: 1.0,
        period: period,
        kind: SeasonalityKind.Additive,
    };

    if (res_add := holtWintersForecast(y_add[0..], cfg_add, period, *seas_s, *fit_s, *fc_s)) {
        expect_eq(res_add.rmse < 1.0e-10, true);
        let h: usize = 0;
        while (h < period) {
            const expected = 50.0 + 3.0 * ((16 + h) as f64) + s_add[(16 + h) % period];
            expect_eq(absF64(fc_s[h] - expected) < 1.0e-10, true);
            h += 1;
        }
    } else {
        expect_eq(true, false);
    }

    // 4b. Multiplicative: y_t = (100 + 2*t) * s[t % 4], mean(s) = 1.0 (4 cycles = 16 pts)
    const s_mul: [4]f64 = [0.8, 1.2, 1.1, 0.9];
    let y_mul: [16]f64 = [0.0; 16];
    let t2: usize = 0;
    while (t2 < 16) {
        y_mul[t2] = (100.0 + 2.0 * (t2 as f64)) * s_mul[t2 % period];
        t2 += 1;
    }

    const cfg_mul: HoltWintersConfig = {
        alpha: 0.6,
        beta: 0.4,
        gamma: 0.6,
        phi: 1.0,
        period: period,
        kind: SeasonalityKind.Multiplicative,
    };

    if (res_mul := holtWintersForecast(y_mul[0..], cfg_mul, period, *seas_s, *fit_s, *fc_s)) {
        expect_eq(res_mul.rmse < 0.1, true);
        let h: usize = 0;
        while (h < period) {
            const expected = (100.0 + 2.0 * ((16 + h) as f64)) * s_mul[(16 + h) % period];
            expect_eq(absF64(fc_s[h] - expected) < 1.0e-6, true);
            h += 1;
        }
    } else {
        expect_eq(true, false);
    }
});

test("5. evaluateForecast computes exact MAE, RMSE, MAPE & sMAPE", () => {
    const actual: [4]f64 = [100.0, 200.0, 50.0, 100.0];
    const pred: [4]f64 = [110.0, 180.0, 50.0, 120.0];

    if (m := evaluateForecast(actual[0..], pred[0..])) {
        expect_eq(absF64(m.mae - 12.5) < 1.0e-12, true);
        expect_eq(absF64(m.rmse - 15.0) < 1.0e-12, true);
        expect_eq(absF64(m.mape - 10.0) < 1.0e-12, true);
        const expected_smape = 25.0 * (20.0 / 210.0 + 40.0 / 380.0 + 40.0 / 220.0);
        expect_eq(absF64(m.smape - expected_smape) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }

    // Zero-actual edge case (no division by zero)
    const z_act: [2]f64 = [0.0, 0.0];
    const z_prd: [2]f64 = [0.0, 0.0];
    if (mz := evaluateForecast(z_act[0..], z_prd[0..])) {
        expect_eq(mz.mae, 0.0);
        expect_eq(mz.rmse, 0.0);
        expect_eq(mz.mape, 0.0);
        expect_eq(mz.smape, 0.0);
    } else {
        expect_eq(true, false);
    }
});

test("6. Boundary defense: out-of-range params, short series (< 2*period), non-positive multiplicative, short buffers & NaN", () => {
    const nan = std.math.nan(f64);
    const s_ok: [8]f64 = [10.0, 12.0, 11.0, 9.0, 12.0, 14.0, 13.0, 11.0];
    const s_nan: [4]f64 = [1.0, nan, 3.0, 4.0];
    const s_zero: [8]f64 = [10.0, 0.0, 11.0, 9.0, 12.0, 14.0, 13.0, 11.0];

    let empty_arr: [0]f64 = [];
    let empty_fit: []f64 = empty_arr[0..];
    let short_buf: [2]f64 = [0.0; 2];
    let short_s: []f64 = short_buf[0..];
    let fc_buf: [4]f64 = [0.0; 4];
    let fc_s: []f64 = fc_buf[0..];
    let seas_buf: [4]f64 = [0.0; 4];
    let seas_s: []f64 = seas_buf[0..];

    // SES invalid inputs
    expect_eq(sesForecast(s_ok[0..0], 0.5, 2, *empty_fit, *fc_s) == null, true);
    expect_eq(sesForecast(s_ok[0..], 0.0, 2, *empty_fit, *fc_s) == null, true);
    expect_eq(sesForecast(s_ok[0..], 1.2, 2, *empty_fit, *fc_s) == null, true);
    expect_eq(sesForecast(s_nan[0..], 0.5, 2, *empty_fit, *fc_s) == null, true);
    expect_eq(sesForecast(s_ok[0..], 0.5, 4, *empty_fit, *short_s) == null, true);
    expect_eq(sesForecast(s_ok[0..], 0.5, 2, *short_s, *fc_s) == null, true);

    // Holt invalid inputs
    const h_bad_phi: HoltConfig = { alpha: 0.3, beta: 0.1, phi: 0.0 };
    const h_bad_beta: HoltConfig = { alpha: 0.3, beta: -0.1, phi: 1.0 };
    expect_eq(holtForecast(s_ok[0..1], HoltConfig.default(), 2, *empty_fit, *fc_s) == null, true);
    expect_eq(holtForecast(s_ok[0..], h_bad_phi, 2, *empty_fit, *fc_s) == null, true);
    expect_eq(holtForecast(s_ok[0..], h_bad_beta, 2, *empty_fit, *fc_s) == null, true);

    // Holt-Winters invalid inputs: short series (< 2*period), period < 2, non-positive multiplicative, short seasonal_buf
    const hw_ok: HoltWintersConfig = { alpha: 0.3, beta: 0.1, gamma: 0.2, phi: 1.0, period: 4, kind: SeasonalityKind.Additive };
    const hw_mul: HoltWintersConfig = { alpha: 0.3, beta: 0.1, gamma: 0.2, phi: 1.0, period: 4, kind: SeasonalityKind.Multiplicative };
    const hw_p1: HoltWintersConfig = { alpha: 0.3, beta: 0.1, gamma: 0.2, phi: 1.0, period: 1, kind: SeasonalityKind.Additive };
    expect_eq(holtWintersForecast(s_ok[0..7], hw_ok, 2, *seas_s, *empty_fit, *fc_s) == null, true);
    expect_eq(holtWintersForecast(s_ok[0..], hw_p1, 2, *seas_s, *empty_fit, *fc_s) == null, true);
    expect_eq(holtWintersForecast(s_zero[0..], hw_mul, 2, *seas_s, *empty_fit, *fc_s) == null, true);
    expect_eq(holtWintersForecast(s_ok[0..], hw_ok, 2, *short_s, *empty_fit, *fc_s) == null, true);

    // evaluateForecast invalid inputs
    expect_eq(evaluateForecast(s_ok[0..0], s_ok[0..0]) == null, true);
    expect_eq(evaluateForecast(s_ok[0..3], s_ok[0..4]) == null, true);
    expect_eq(evaluateForecast(s_nan[0..], s_ok[0..4]) == null, true);
});
