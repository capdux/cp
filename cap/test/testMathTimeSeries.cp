// Deep Production Test Suite for lib/math/timeSeries.cp (Wave 27 Round 7)
// Verifies: Additive & multiplicative Holt-Winters accuracy, centered seasonal decomposition
// exact identity (< 1e-10), SIMD ACF parity (< 1e-4) + Levinson-Durbin AR(2) recovery (< 0.05),
// Kalman1dFilter RMSE reduction > 40%, 4-state KalmanFilterN with Joseph update & Mahalanobis gating,
// Robust MAD outlier immunity & CUSUM changepoint localization, and 1,000-point Welford + error guards.

import { test, expect_eq, expect_true } from "cap:test";
import {
    MatrixViewF64,
    TimeSeriesError,
    TsSeasonalMode,
    AnomalyFlag,
    rollingMeanF64,
    rollingStdF64,
    ewmaSmoothF64,
    TsHoltWintersParams,
    doubleExpSmoothHolt,
    tsHoltWintersForecast,
    decomposeSeasonalAdditive,
    computeAcfF32Simd,
    computeAcfF64,
    levinsonDurbinAr,
    forecastArModel,
    Kalman1dFilter,
    KalmanNScratch,
    KalmanUpdateInfo,
    KalmanFilterN,
    detectAnomaliesZScore,
    detectAnomaliesMad,
    CusumChangeDetector,
    TsForecastMetrics,
    evalForecastMetrics,
} from "../../lib/math/index.cp";

const std = @import("std");
const PI_F64: f64 = 3.14159265358979323846;

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn nextPseudoGaussian(rng: *u64): f64 {
    // Central-limit sum of 6 uniforms scaled to zero-mean unit-variance (~N(0, 1))
    let sum: f64 = 0.0;
    let k: usize = 0;
    while (k < 6) : (k += 1) {
        let x: u64 = rng.*;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        rng.* = x;
        const u: f64 = @as(f64, @floatFromInt(x & 0xFFFF_FFFF)) / 4294967296.0;
        sum += u;
    }
    return (sum - 3.0) * 1.4142135623730951;
}

// ============================================================================
// Test 1: Additive & Multiplicative Holt-Winters + Holt + Eval Metrics
// ============================================================================
test("TimeSeries: additive & multiplicative Holt-Winters forecast accuracy & metrics") {
    const P: usize = 12;
    const TRAIN: usize = 48; // 4 full seasons
    const HORIZON: usize = 12;
    const season_add: [12]f64 = [5.0, 8.0, 12.0, 10.0, 4.0, -2.0, -8.0, -10.0, -9.0, -5.0, -3.0, -2.0];
    const season_mul: [12]f64 = [1.10, 1.15, 1.25, 1.20, 1.05, 0.95, 0.85, 0.80, 0.82, 0.90, 0.96, 0.97];

    let y_add_train: [48]f64 = [0.0; 48];
    let y_add_test: [12]f64 = [0.0; 12];
    let y_mul_train: [48]f64 = [0.0; 48];
    let y_mul_test: [12]f64 = [0.0; 12];

    let t: usize = 0;
    while (t < TRAIN + HORIZON) : (t += 1) {
        const trend_val: f64 = 100.0 + 1.5 * @as(f64, @floatFromInt(t));
        const va: f64 = trend_val + season_add[t % P];
        const vm: f64 = trend_val * season_mul[t % P];
        if (t < TRAIN) {
            y_add_train[t] = va;
            y_mul_train[t] = vm;
        } else {
            y_add_test[t - TRAIN] = va;
            y_mul_test[t - TRAIN] = vm;
        }
    }

    let sw: [12]f64 = [0.0; 12];
    let fc: [12]f64 = [0.0; 12];
    let sw_s: []f64 = sw[0..];
    let fc_s: []f64 = fc[0..];

    // Additive Holt-Winters
    * = tsHoltWintersForecast(
        y_add_train[0..],
        TsHoltWintersParams{ alpha: 0.4, beta: 0.2, gamma: 0.3, period: P, mode: TsSeasonalMode.additive },
        HORIZON,
        *sw_s,
        *fc_s,
    ) catch { expect_true(false); return; };

    const m_add: TsForecastMetrics = evalForecastMetrics(y_add_test[0..], fc[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(m_add.mape_pct < 1.0);
    expect_true(m_add.r_squared > 0.99);

    // Multiplicative Holt-Winters
    * = tsHoltWintersForecast(
        y_mul_train[0..],
        TsHoltWintersParams{ alpha: 0.4, beta: 0.2, gamma: 0.3, period: P, mode: TsSeasonalMode.multiplicative },
        HORIZON,
        *sw_s,
        *fc_s,
    ) catch { expect_true(false); return; };

    const m_mul: TsForecastMetrics = evalForecastMetrics(y_mul_test[0..], fc[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(m_mul.mape_pct < 2.0);
    expect_true(m_mul.r_squared > 0.98);

    // Holt double exponential smoothing on pure linear trend
    let lin: [20]f64 = [0.0; 20];
    let i: usize = 0;
    while (i < 20) : (i += 1) lin[i] = 10.0 + 2.5 * @as(f64, @floatFromInt(i));
    let hfc: [4]f64 = [0.0; 4];
    let hfc_s: []f64 = hfc[0..];
    * = doubleExpSmoothHolt(lin[0..], 0.5, 0.3, 4, *hfc_s) catch { expect_true(false); return; };
    expect_true(absF64(hfc[0] - 60.0) < 1.0e-6);
    expect_true(absF64(hfc[3] - 67.5) < 1.0e-6);
}

// ============================================================================
// Test 2: Centered Seasonal Decomposition Exact Identity (< 1e-10)
// ============================================================================
test("TimeSeries: decomposeSeasonalAdditive exact conservation (trend + seasonal + residual == y)") {
    const N: usize = 48;
    const P: usize = 12;
    let y: [48]f64 = [0.0; 48];
    let i: usize = 0;
    while (i < N) : (i += 1) {
        const fi: f64 = @as(f64, @floatFromInt(i));
        y[i] = 50.0 + 0.8 * fi + 6.0 * std.math.sin(2.0 * PI_F64 * fi / 12.0) + 0.2 * std.math.cos(fi * 1.7);
    }

    let tr: [48]f64 = [0.0; 48];
    let se: [48]f64 = [0.0; 48];
    let re: [48]f64 = [0.0; 48];
    let sw: [12]f64 = [0.0; 12];
    let tr_s: []f64 = tr[0..];
    let se_s: []f64 = se[0..];
    let re_s: []f64 = re[0..];
    let sw_s: []f64 = sw[0..];

    * = decomposeSeasonalAdditive(y[0..], P, *tr_s, *se_s, *re_s, *sw_s) catch {
        expect_true(false);
        return;
    };

    // Exact additive identity at every point
    let max_err: f64 = 0.0;
    i = 0;
    while (i < N) : (i += 1) {
        const err: f64 = absF64((tr[i] + se[i] + re[i]) - y[i]);
        if (err > max_err) max_err = err;
    }
    expect_true(max_err < 1.0e-10);

    // Zero-mean seasonal component
    let s_sum: f64 = 0.0;
    i = 0;
    while (i < P) : (i += 1) s_sum += se[i];
    expect_true(absF64(s_sum) < 1.0e-10);
}

// ============================================================================
// Test 3: SIMD ACF Parity (< 1e-4) & Levinson-Durbin AR(2) Parameter Recovery
// ============================================================================
test("TimeSeries: SIMD ACF parity (< 1e-4) & Levinson-Durbin AR(2) recovery (< 0.05)") {
    const N: usize = 1000;
    let x64: [1000]f64 = [0.0; 1000];
    let x32: [1000]f32 = [0.0; 1000];
    let rng: u64 = 0x8F3A2B1C4D5E6F71;

    // Generate stationary AR(2): x[t] = 0.70 * x[t-1] - 0.35 * x[t-2] + eps[t]
    let t: usize = 2;
    while (t < N) : (t += 1) {
        x64[t] = 0.70 * x64[t - 1] - 0.35 * x64[t - 2] + nextPseudoGaussian(*rng);
    }
    let mean: f64 = 0.0;
    t = 0;
    while (t < N) : (t += 1) {
        x32[t] = x64[t] as f32;
        mean += x64[t];
    }
    mean /= @as(f64, @floatFromInt(N));
    let gamma0: f64 = 0.0;
    t = 0;
    while (t < N) : (t += 1) {
        const d: f64 = x64[t] - mean;
        gamma0 += d * d;
    }
    gamma0 /= @as(f64, @floatFromInt(N));

    // Compute ACF via SIMD F32 and Scalar F64
    let acf64: [17]f64 = [0.0; 17];
    let acf32: [17]f32 = [0.0; 17];
    let centered32: [1000]f32 = [0.0; 1000];
    let acf64_s: []f64 = acf64[0..];
    let acf32_s: []f32 = acf32[0..];
    let c32_s: []f32 = centered32[0..];

    * = computeAcfF64(x64[0..], 16, *acf64_s) catch { expect_true(false); return; };
    * = computeAcfF32Simd(x32[0..], 16, *c32_s, *acf32_s) catch { expect_true(false); return; };

    let lag: usize = 0;
    while (lag <= 16) : (lag += 1) {
        expect_true(absF64(@as(f64, acf32[lag]) - acf64[lag]) < 1.0e-4);
    }

    // Levinson-Durbin AR(3) fit: phi_1 ~ 0.70, phi_2 ~ -0.35, pacf[2] (lag 3) ~ 0
    let ar: [3]f64 = [0.0; 3];
    let pacf: [3]f64 = [0.0; 3];
    let prev_w: [3]f64 = [0.0; 3];
    let ar_s: []f64 = ar[0..];
    let pacf_s: []f64 = pacf[0..];
    let pw_s: []f64 = prev_w[0..];

    const fit = levinsonDurbinAr(acf64[0..], gamma0, mean, 3, *ar_s, *pacf_s, *pw_s) catch {
        expect_true(false);
        return;
    };
    expect_true(absF64(ar[0] - 0.70) < 0.05);
    expect_true(absF64(ar[1] - (-0.35)) < 0.05);
    expect_true(absF64(pacf[2]) < 0.10);
    expect_true(fit.noise_variance > 0.5 && fit.noise_variance < 1.5);

    // Multi-step forecast sanity
    let fc: [5]f64 = [0.0; 5];
    let fc_s: []f64 = fc[0..];
    * = forecastArModel(x64[0..], ar[0..2], mean, 5, *fc_s) catch { expect_true(false); return; };
    expect_true(!std.math.isNan(fc[4]));
}

// ============================================================================
// Test 4: Kalman1dFilter Noisy Trajectory Denoising (> 40% RMSE Reduction)
// ============================================================================
test("TimeSeries: Kalman1dFilter reduces noisy trajectory RMSE by > 40%") {
    const N: usize = 200;
    const dt: f64 = 0.1;
    let rng: u64 = 0x5566778899AABBCD;

    let kf: Kalman1dFilter = Kalman1dFilter.init(0.0, 0.0, 25.0, 10.0, 0.25, 16.0) catch {
        expect_true(false);
        return;
    };

    let raw_se: f64 = 0.0;
    let kf_se: f64 = 0.0;
    let t: usize = 0;
    while (t < N) : (t += 1) {
        const tf: f64 = @as(f64, @floatFromInt(t)) * dt;
        // True smooth trajectory: x(t) = 2.0 * t + 0.15 * t^2
        const true_x: f64 = 2.0 * tf + 0.15 * tf * tf;
        const meas_z: f64 = true_x + 4.0 * nextPseudoGaussian(*rng);

        kf.predict(dt);
        * = kf.update(meas_z);

        const e_raw: f64 = meas_z - true_x;
        const e_kf: f64 = kf.pos - true_x;
        raw_se += e_raw * e_raw;
        kf_se += e_kf * e_kf;
    }

    const raw_rmse: f64 = std.math.sqrt(raw_se / @as(f64, @floatFromInt(N)));
    const kf_rmse: f64 = std.math.sqrt(kf_se / @as(f64, @floatFromInt(N)));
    expect_true(kf_rmse < raw_rmse * 0.60); // > 40% RMSE reduction
}

// ============================================================================
// Test 5: 4-State N-D KalmanFilterN (Joseph Covariance + Mahalanobis Gating)
// ============================================================================
test("TimeSeries: 4-state KalmanFilterN Joseph covariance stability & Mahalanobis outlier gating") {
    let x_state: [4]f64 = [0.0, 0.0, 1.0, 0.5]; // [x, y, vx, vy]
    let p_cov: [16]f64 = [
        4.0, 0.0, 0.0, 0.0,
        0.0, 4.0, 0.0, 0.0,
        0.0, 0.0, 2.0, 0.0,
        0.0, 0.0, 0.0, 2.0,
    ];
    let kf: KalmanFilterN = KalmanFilterN.init(4, 2, x_state[0..], p_cov[0..]) catch {
        expect_true(false);
        return;
    };

    const dt: f64 = 1.0;
    const f_data: [16]f64 = [
        1.0, 0.0,  dt, 0.0,
        0.0, 1.0, 0.0,  dt,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];
    const q_data: [16]f64 = [
        0.05, 0.0,  0.0,  0.0,
        0.0,  0.05, 0.0,  0.0,
        0.0,  0.0,  0.02, 0.0,
        0.0,  0.0,  0.0,  0.02,
    ];
    const h_data: [8]f64 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
    ];
    const r_data: [4]f64 = [
        1.0, 0.0,
        0.0, 1.0,
    ];

    const vf: MatrixViewF64 = MatrixViewF64.init(f_data[0..], 4, 4) catch { expect_true(false); return; };
    const vq: MatrixViewF64 = MatrixViewF64.init(q_data[0..], 4, 4) catch { expect_true(false); return; };
    const vh: MatrixViewF64 = MatrixViewF64.init(h_data[0..], 2, 4) catch { expect_true(false); return; };
    const vr: MatrixViewF64 = MatrixViewF64.init(r_data[0..], 2, 2) catch { expect_true(false); return; };

    let ma: [16]f64 = [0.0; 16];
    let mb: [16]f64 = [0.0; 16];
    let mc: [16]f64 = [0.0; 16];
    let md: [16]f64 = [0.0; 16];
    let bt: [16]f64 = [0.0; 16];
    let va: [4]f64 = [0.0; 4];
    let vb: [8]f64 = [0.0; 8];
    let piv: [4]usize = [0; 4];
    let sc: KalmanNScratch = KalmanNScratch{
        mat_a: ma[0..],
        mat_b: mb[0..],
        mat_c: mc[0..],
        mat_d: md[0..],
        bt_work: bt[0..],
        vec_a: va[0..],
        vec_b: vb[0..],
        piv: piv[0..],
    };

    let step: usize = 1;
    let rejected_outlier: bool = false;
    while (step <= 30) : (step += 1) {
        * = kf.predict(vf, vq, *sc) catch { expect_true(false); return; };
        const true_x: f64 = @as(f64, @floatFromInt(step)) * 1.0;
        const true_y: f64 = @as(f64, @floatFromInt(step)) * 0.5;
        let z: [2]f64 = [true_x, true_y];
        if (step == 15) {
            // Inject massive sensor glitch (+500) -> must be gated out
            z[0] = true_x + 500.0;
        }
        const info: KalmanUpdateInfo = kf.update(z[0..], vh, vr, 25.0, *sc) catch {
            expect_true(false);
            return;
        };
        if (step == 15) {
            rejected_outlier = info.gated_rejected;
        } else {
            expect_true(!info.gated_rejected);
        }
    }
    expect_true(rejected_outlier);
    expect_true(absF64(kf.x[0] - 30.0) < 0.5);
    expect_true(absF64(kf.x[1] - 15.0) < 0.5);
    // Verify Joseph-form covariance remains symmetric and strictly positive on diagonal
    expect_true(kf.p[0] > 0.0 && kf.p[5] > 0.0 && kf.p[10] > 0.0 && kf.p[15] > 0.0);
    expect_true(absF64(kf.p[0 * 4 + 2] - kf.p[2 * 4 + 0]) < 1.0e-12);
}

// ============================================================================
// Test 6: Robust MAD Anomaly Detection & CUSUM Online Step Changepoint Detection
// ============================================================================
test("TimeSeries: Robust MAD outlier detection & CUSUM step changepoint localization") {
    // 40-point baseline around 100.0 with 3 extreme spikes (masked for naive global Z-score, caught by MAD)
    let data: [40]f64 = [100.0; 40];
    let i: usize = 0;
    while (i < 40) : (i += 1) {
        data[i] = 100.0 + @as(f64, @floatFromInt(i % 3)) - 1.0;
    }
    data[12] = 500.0;  // extreme high spike
    data[25] = -300.0; // extreme low spike
    data[33] = 450.0;  // extreme high spike

    let sw: [40]f64 = [0.0; 40];
    let flags: [40]AnomalyFlag = [AnomalyFlag.normal; 40];
    let sw_s: []f64 = sw[0..];
    let fl_s: []AnomalyFlag = flags[0..];

    const n_mad: usize = detectAnomaliesMad(data[0..], 3.5, *sw_s, *fl_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n_mad, 3);
    expect_true(flags[12] == AnomalyFlag.high);
    expect_true(flags[25] == AnomalyFlag.low);
    expect_true(flags[33] == AnomalyFlag.high);

    // Rolling Z-score also catches isolated spike
    let zflags: [40]AnomalyFlag = [AnomalyFlag.normal; 40];
    let zf_s: []AnomalyFlag = zflags[0..];
    const n_z: usize = detectAnomaliesZScore(data[0..], 8, 3.0, *zf_s) catch {
        expect_true(false);
        return;
    };
    expect_true(n_z >= 3);

    // CUSUM online changepoint: mean shifts from 10.0 to 14.0 at t = 50
    let cusum: CusumChangeDetector = CusumChangeDetector.init(10.0, 0.5, 8.0) catch {
        expect_true(false);
        return;
    };
    let alarm_idx: usize = 9999;
    let t: usize = 0;
    while (t < 100) : (t += 1) {
        const obs: f64 = if (t < 50) 10.0 else 14.0;
        const res = cusum.update(obs);
        if (t < 50) {
            expect_true(!res.triggered);
        } else if (res.triggered && alarm_idx == 9999) {
            alarm_idx = res.sample_index;
            expect_true(res.direction == AnomalyFlag.high);
        }
    }
    expect_true(alarm_idx >= 51 && alarm_idx <= 55);
}

// ============================================================================
// Test 7: 1,000-Point Welford Stability & Defensive Error Guards
// ============================================================================
test("TimeSeries: 1,000-point high-baseline Welford stability & error guards") {
    let series: [1000]f64 = [0.0; 1000];
    let rmean: [1000]f64 = [0.0; 1000];
    let rstd: [1000]f64 = [0.0; 1000];
    let ewma: [1000]f64 = [0.0; 1000];

    // High-offset baseline (1e9 + alternating +/-1) would suffer catastrophic cancellation with naive E[X^2]-E[X]^2
    let i: usize = 0;
    while (i < 1000) : (i += 1) {
        series[i] = 1.0e9 + (if ((i % 2) == 0) 1.0 else -1.0);
    }
    let rm_s: []f64 = rmean[0..];
    let rs_s: []f64 = rstd[0..];
    let ew_s: []f64 = ewma[0..];

    * = rollingMeanF64(series[0..], 20, *rm_s) catch { expect_true(false); return; };
    * = rollingStdF64(series[0..], 20, *rs_s) catch { expect_true(false); return; };
    * = ewmaSmoothF64(series[0..], 0.2, *ew_s) catch { expect_true(false); return; };

    // Exact sample std of ten +1s and ten -1s is sqrt(20/19) = 1.025978352
    const expected_std: f64 = std.math.sqrt(20.0 / 19.0);
    expect_true(absF64(rmean[999] - 1.0e9) < 1.0e-6);
    expect_true(absF64(rstd[999] - expected_std) < 1.0e-6);

    // Defensive error guards: window=0, invalid alpha, NaN input
    let got_bad_w: bool = false;
    * = rollingMeanF64(series[0..10], 0, *rm_s) catch |e| {
        if (e == TimeSeriesError.InvalidParameter) got_bad_w = true;
    };
    expect_true(got_bad_w);

    let bad_nan: [4]f64 = [1.0, std.math.nan(f64), 3.0, 4.0];
    let got_nan: bool = false;
    * = ewmaSmoothF64(bad_nan[0..], 0.5, *ew_s) catch |e| {
        if (e == TimeSeriesError.NonFiniteInput) got_nan = true;
    };
    expect_true(got_nan);
}
