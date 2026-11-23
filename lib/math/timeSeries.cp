// Origin: Pure-CP Stdlib (Unbounded Zero-Heap Time-Series Filtering, Holt-Winters, Kalman N-D, SIMD ACF/PACF & Anomaly Detection) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, caller-slice-driven time-series analysis & forecasting engine:
//   - Rolling Welford statistics (`rollingMeanF64`, `rollingStdF64`), EWMA (`ewmaSmoothF64`),
//     Holt linear trend (`doubleExpSmoothHolt`), and unbounded-period additive/multiplicative
//     Holt-Winters triple exponential smoothing (`tsHoltWintersForecast`)
//   - Centered moving-average additive seasonal decomposition (`decomposeSeasonalAdditive`)
//     preserving exact identity `trend[i] + seasonal[i] + residual[i] == y[i]`
//   - Hardware SIMD `@dot` autocorrelation (`computeAcfF32Simd`) + `computeAcfF64`,
//     O(p^2) Levinson-Durbin Yule-Walker AR(p) & PACF solver (`levinsonDurbinAr`), and `forecastArModel`
//   - Production linear Kalman filters: 2-state constant-velocity `Kalman1dFilter` and
//     arbitrary-dimension `KalmanFilterN` with Joseph-stabilized covariance update & Mahalanobis gating
//   - Monitoring anomaly & changepoint detectors: `detectAnomaliesZScore`, `detectAnomaliesMad` (robust MAD),
//     `CusumChangeDetector` (two-sided CUSUM), and `evalForecastMetrics` (MAE/RMSE/MAPE/sMAPE/R^2)

const std = @import("std");
import {
    MatrixViewF64,
    MatrixMutF64,
    matrixIdentityF64,
    transposeMatrixF64,
    matmulF64,
    matvecF64,
    luDecomposeInPlace,
    luInvert,
} from "./matrixDense.cp";

// ============================================================================
// 1. Error Set, Helpers & Enums
// ============================================================================

/// Error set returned by time-series filtering, forecasting, Kalman, and anomaly APIs.
pub err TimeSeriesError {
    InvalidParameter,
    InsufficientData,
    BufferTooSmall,
    DimensionMismatch,
    SingularSystem,
    NonFiniteInput,
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Seasonal model kind for Holt-Winters triple exponential smoothing.
pub enum TsSeasonalMode {
    additive,
    multiplicative,
}

/// Direction tag emitted by anomaly detectors.
pub enum AnomalyFlag {
    normal,
    high,
    low,
}

// ============================================================================
// 2. Rolling Statistics, EWMA, Holt, Holt-Winters & Seasonal Decomposition
// ============================================================================

/// Computes the causal rolling mean with window size `window` into `out` (`out.len >= data.len`).
/// For early indices `i < window - 1`, averages over the available prefix `0..=i`.
pub fn rollingMeanF64(data: []const f64, window: usize, out: *[]f64): void ! TimeSeriesError {
    if (window == 0) return TimeSeriesError.InvalidParameter;
    if (data.len == 0) return TimeSeriesError.InsufficientData;
    if (out.len < data.len) return TimeSeriesError.BufferTooSmall;

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (!isFiniteF64(data[i])) return TimeSeriesError.NonFiniteInput;
        sum += data[i];
        if (i >= window) {
            sum -= data[i - window];
            out[i] = sum / @as(f64, @floatFromInt(window));
        } else {
            out[i] = sum / @as(f64, @floatFromInt(i + 1));
        }
    }
}

/// Computes numerically stable rolling sample standard deviation (`ddof = 1` when count >= 2, `0.0` at `i == 0`).
/// Uses shifted-origin variance per window to avoid catastrophic cancellation on large baselines.
pub fn rollingStdF64(data: []const f64, window: usize, out: *[]f64): void ! TimeSeriesError {
    if (window == 0) return TimeSeriesError.InvalidParameter;
    if (data.len == 0) return TimeSeriesError.InsufficientData;
    if (out.len < data.len) return TimeSeriesError.BufferTooSmall;

    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const start: usize = if (i + 1 >= window) (i + 1 - window) else 0;
        const cnt: usize = i + 1 - start;
        if (cnt <= 1) {
            out[i] = 0.0;
            continue;
        }
        // Welford single-pass over the current window
        let mean: f64 = 0.0;
        let m2: f64 = 0.0;
        let k: usize = 0;
        while (k < cnt) : (k += 1) {
            const x: f64 = data[start + k];
            if (!isFiniteF64(x)) return TimeSeriesError.NonFiniteInput;
            const kf: f64 = @as(f64, @floatFromInt(k + 1));
            const delta: f64 = x - mean;
            mean += delta / kf;
            const delta2: f64 = x - mean;
            m2 += delta * delta2;
        }
        const variance: f64 = m2 / @as(f64, @floatFromInt(cnt - 1));
        out[i] = std.math.sqrt(if (variance > 0.0) variance else 0.0);
    }
}

/// Applies Exponentially Weighted Moving Average `s[t] = alpha * x[t] + (1 - alpha) * s[t - 1]`.
pub fn ewmaSmoothF64(data: []const f64, alpha: f64, out: *[]f64): void ! TimeSeriesError {
    if (!isFiniteF64(alpha) || alpha <= 0.0 || alpha > 1.0) return TimeSeriesError.InvalidParameter;
    if (data.len == 0) return TimeSeriesError.InsufficientData;
    if (out.len < data.len) return TimeSeriesError.BufferTooSmall;
    if (!isFiniteF64(data[0])) return TimeSeriesError.NonFiniteInput;

    out[0] = data[0];
    let prev: f64 = data[0];
    let i: usize = 1;
    while (i < data.len) : (i += 1) {
        const x: f64 = data[i];
        if (!isFiniteF64(x)) return TimeSeriesError.NonFiniteInput;
        prev = alpha * x + (1.0 - alpha) * prev;
        out[i] = prev;
    }
}

/// Fitted Holt-Winters level/trend state & diagnostics.
pub struct TsHoltWintersState {
    level: f64 = 0.0;
    trend: f64 = 0.0;
    in_sample_rmse: f64 = 0.0;
}

/// Configuration for `tsHoltWintersForecast`.
pub struct TsHoltWintersParams {
    alpha: f64 = 0.3;
    beta: f64 = 0.1;
    gamma: f64 = 0.2;
    period: usize = 12;
    mode: TsSeasonalMode = TsSeasonalMode.additive;
    damping_phi: f64 = 1.0;

    fn isValid(): bool {
        if (!isFiniteF64(alpha) || alpha <= 0.0 || alpha > 1.0) return false;
        if (!isFiniteF64(beta) || beta < 0.0 || beta > 1.0) return false;
        if (!isFiniteF64(gamma) || gamma < 0.0 || gamma > 1.0) return false;
        if (!isFiniteF64(damping_phi) || damping_phi <= 0.0 || damping_phi > 1.0) return false;
        if (period < 2) return false;
        return true;
    }
}

/// Holt's linear trend double exponential smoothing producing `horizon` multi-step forecasts into `out_forecast`.
pub fn doubleExpSmoothHolt(
    data: []const f64,
    alpha: f64,
    beta: f64,
    horizon: usize,
    out_forecast: *[]f64,
): TsHoltWintersState ! TimeSeriesError {
    if (!isFiniteF64(alpha) || alpha <= 0.0 || alpha > 1.0) return TimeSeriesError.InvalidParameter;
    if (!isFiniteF64(beta) || beta < 0.0 || beta > 1.0) return TimeSeriesError.InvalidParameter;
    if (data.len < 2) return TimeSeriesError.InsufficientData;
    if (out_forecast.len < horizon) return TimeSeriesError.BufferTooSmall;

    let level: f64 = data[0];
    let trend: f64 = data[1] - data[0];
    let sq_err_sum: f64 = 0.0;

    let t: usize = 1;
    while (t < data.len) : (t += 1) {
        const y: f64 = data[t];
        if (!isFiniteF64(y)) return TimeSeriesError.NonFiniteInput;
        const pred: f64 = level + trend;
        const err: f64 = y - pred;
        sq_err_sum += err * err;

        const prev_level: f64 = level;
        level = alpha * y + (1.0 - alpha) * (level + trend);
        trend = beta * (level - prev_level) + (1.0 - beta) * trend;
    }

    let h: usize = 0;
    while (h < horizon) : (h += 1) {
        out_forecast[h] = level + @as(f64, @floatFromInt(h + 1)) * trend;
    }

    return TsHoltWintersState{
        level: level,
        trend: trend,
        in_sample_rmse: std.math.sqrt(sq_err_sum / @as(f64, @floatFromInt(data.len - 1))),
    };
}

/// Unbounded-period Holt-Winters triple exponential smoothing (additive or multiplicative seasonality).
/// Requires `data.len >= 2 * params.period`, `season_work.len >= params.period`, and `out_forecast.len >= horizon`.
pub fn tsHoltWintersForecast(
    data: []const f64,
    params: TsHoltWintersParams,
    horizon: usize,
    season_work: *[]f64,
    out_forecast: *[]f64,
): TsHoltWintersState ! TimeSeriesError {
    if (!params.isValid()) return TimeSeriesError.InvalidParameter;
    const p: usize = params.period;
    if (data.len < 2 * p) return TimeSeriesError.InsufficientData;
    if (season_work.len < p || out_forecast.len < horizon) return TimeSeriesError.BufferTooSmall;

    // Validate finite inputs (and strictly positive for multiplicative mode)
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (!isFiniteF64(data[i])) return TimeSeriesError.NonFiniteInput;
        if (params.mode == TsSeasonalMode.multiplicative && data[i] <= 0.0) {
            return TimeSeriesError.InvalidParameter;
        }
    }

    // Initialize level from first season average and trend from average slope across first two seasons
    let sum_first: f64 = 0.0;
    let trend_sum: f64 = 0.0;
    i = 0;
    while (i < p) : (i += 1) {
        sum_first += data[i];
        trend_sum += (data[i + p] - data[i]) / @as(f64, @floatFromInt(p));
    }
    let level: f64 = sum_first / @as(f64, @floatFromInt(p));
    let trend: f64 = trend_sum / @as(f64, @floatFromInt(p));

    // Initialize seasonal indices from first season
    i = 0;
    while (i < p) : (i += 1) {
        if (params.mode == TsSeasonalMode.additive) {
            season_work[i] = data[i] - level;
        } else {
            season_work[i] = data[i] / level;
        }
    }

    // Filter across full series
    let sq_err: f64 = 0.0;
    let err_cnt: usize = 0;
    let t: usize = p;
    while (t < data.len) : (t += 1) {
        const y: f64 = data[t];
        const s_idx: usize = t % p;
        const s_prev: f64 = season_work[s_idx];
        const damped_t: f64 = params.damping_phi * trend;

        const pred: f64 = if (params.mode == TsSeasonalMode.additive)
            (level + damped_t + s_prev)
        else
            ((level + damped_t) * s_prev);
        const e: f64 = y - pred;
        sq_err += e * e;
        err_cnt += 1;

        const prev_level: f64 = level;
        if (params.mode == TsSeasonalMode.additive) {
            level = params.alpha * (y - s_prev) + (1.0 - params.alpha) * (level + damped_t);
            trend = params.beta * (level - prev_level) + (1.0 - params.beta) * damped_t;
            season_work[s_idx] = params.gamma * (y - level) + (1.0 - params.gamma) * s_prev;
        } else {
            const safe_s: f64 = if (absF64(s_prev) > 1.0e-12) s_prev else 1.0;
            level = params.alpha * (y / safe_s) + (1.0 - params.alpha) * (level + damped_t);
            trend = params.beta * (level - prev_level) + (1.0 - params.beta) * damped_t;
            const safe_l: f64 = if (absF64(level) > 1.0e-12) level else 1.0;
            season_work[s_idx] = params.gamma * (y / safe_l) + (1.0 - params.gamma) * s_prev;
        }
    }

    // Multi-step forecast
    let phi_sum: f64 = 0.0;
    let phi_pow: f64 = 1.0;
    let h: usize = 0;
    while (h < horizon) : (h += 1) {
        phi_pow *= params.damping_phi;
        phi_sum += phi_pow;
        const s_idx: usize = (data.len + h) % p;
        const base: f64 = level + phi_sum * trend;
        out_forecast[h] = if (params.mode == TsSeasonalMode.additive)
            (base + season_work[s_idx])
        else
            (base * season_work[s_idx]);
    }

    return TsHoltWintersState{
        level: level,
        trend: trend,
        in_sample_rmse: if (err_cnt > 0) std.math.sqrt(sq_err / @as(f64, @floatFromInt(err_cnt))) else 0.0,
    };
}

/// Classical additive seasonal decomposition (`y[i] = trend[i] + seasonal[i] + residual[i]`)
/// using centered moving average (2xP for even `period`, P for odd `period`) and boundary extension.
/// Guarantees exact additive reconstruction `|trend[i] + seasonal[i] + residual[i] - y[i]| < 1e-12`.
pub fn decomposeSeasonalAdditive(
    data: []const f64,
    period: usize,
    out_trend: *[]f64,
    out_seasonal: *[]f64,
    out_residual: *[]f64,
    season_avg_work: *[]f64,
): void ! TimeSeriesError {
    if (period < 2) return TimeSeriesError.InvalidParameter;
    if (data.len < 2 * period) return TimeSeriesError.InsufficientData;
    if (out_trend.len < data.len || out_seasonal.len < data.len ||
        out_residual.len < data.len || season_avg_work.len < period)
    {
        return TimeSeriesError.BufferTooSmall;
    }

    const n: usize = data.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (!isFiniteF64(data[i])) return TimeSeriesError.NonFiniteInput;
    }

    // 1. Centered moving average trend on interior [half .. n - half)
    const half: usize = period / 2;
    const is_even: bool = (period % 2) == 0;
    const pf: f64 = @as(f64, @floatFromInt(period));

    i = half;
    while (i + half < n) : (i += 1) {
        if (is_even) {
            // 2xP centered moving average: half weight at endpoints i-half and i+half
            let sum: f64 = 0.5 * (data[i - half] + data[i + half]);
            let k: usize = 1;
            while (k < period) : (k += 1) {
                sum += data[i - half + k];
            }
            out_trend[i] = sum / pf;
        } else {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < period) : (k += 1) {
                sum += data[i - half + k];
            }
            out_trend[i] = sum / pf;
        }
    }

    // Linearly extrapolate edge trend values using nearest interior slope
    const first_valid: usize = half;
    const last_valid: usize = n - half - 1;
    const left_slope: f64 = if (first_valid + 1 <= last_valid)
        (out_trend[first_valid + 1] - out_trend[first_valid])
    else
        0.0;
    const right_slope: f64 = if (last_valid > first_valid)
        (out_trend[last_valid] - out_trend[last_valid - 1])
    else
        0.0;

    let li: usize = 0;
    while (li < first_valid) : (li += 1) {
        out_trend[li] = out_trend[first_valid] - left_slope * @as(f64, @floatFromInt(first_valid - li));
    }
    let ri: usize = last_valid + 1;
    while (ri < n) : (ri += 1) {
        out_trend[ri] = out_trend[last_valid] + right_slope * @as(f64, @floatFromInt(ri - last_valid));
    }

    // 2. Average detrended values per seasonal phase and center to zero mean
    let p_i: usize = 0;
    let total_phase_mean: f64 = 0.0;
    while (p_i < period) : (p_i += 1) {
        let sum: f64 = 0.0;
        let cnt: usize = 0;
        let idx: usize = p_i;
        while (idx < n) : (idx += period) {
            sum += data[idx] - out_trend[idx];
            cnt += 1;
        }
        const avg: f64 = if (cnt > 0) sum / @as(f64, @floatFromInt(cnt)) else 0.0;
        season_avg_work[p_i] = avg;
        total_phase_mean += avg;
    }
    const mean_shift: f64 = total_phase_mean / pf;
    p_i = 0;
    while (p_i < period) : (p_i += 1) {
        season_avg_work[p_i] -= mean_shift;
    }

    // 3. Emit seasonal and exact residual
    i = 0;
    while (i < n) : (i += 1) {
        const s: f64 = season_avg_work[i % period];
        out_seasonal[i] = s;
        out_residual[i] = data[i] - out_trend[i] - s;
    }
}

// ============================================================================
// 3. Hardware SIMD `@dot` ACF, Levinson-Durbin PACF & AR(p) Forecasting
// ============================================================================

/// Computes autocorrelation function `acf[0..=max_lag]` on `f32` data using hardware SIMD `@dot`.
/// Requires `centered_work.len >= data.len` and `out_acf.len >= max_lag + 1`.
pub fn computeAcfF32Simd(
    data: []const f32,
    max_lag: usize,
    centered_work: *[]f32,
    out_acf: *[]f32,
): void ! TimeSeriesError {
    if (data.len < 2 || max_lag >= data.len) return TimeSeriesError.InsufficientData;
    if (centered_work.len < data.len || out_acf.len < max_lag + 1) {
        return TimeSeriesError.BufferTooSmall;
    }

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        sum += @as(f64, data[i]);
    }
    const mean: f32 = (sum / @as(f64, @floatFromInt(data.len))) as f32;
    i = 0;
    while (i < data.len) : (i += 1) {
        centered_work[i] = data[i] - mean;
    }

    const c_all: []const f32 = centered_work.*[0..data.len];
    const denom: f32 = @dot(c_all, c_all);
    if (denom <= 1.0e-18) {
        let lag: usize = 0;
        while (lag <= max_lag) : (lag += 1) out_acf[lag] = if (lag == 0) 1.0 else 0.0;
        return;
    }

    out_acf[0] = 1.0;
    let lag: usize = 1;
    while (lag <= max_lag) : (lag += 1) {
        const len: usize = data.len - lag;
        const s1: []const f32 = c_all[0..len];
        const s2: []const f32 = c_all[lag..lag + len];
        const num: f32 = @dot(s1, s2);
        out_acf[lag] = num / denom;
    }
}

/// Computes double-precision sample autocorrelation `acf[0..=max_lag]`.
pub fn computeAcfF64(data: []const f64, max_lag: usize, out_acf: *[]f64): void ! TimeSeriesError {
    if (data.len < 2 || max_lag >= data.len) return TimeSeriesError.InsufficientData;
    if (out_acf.len < max_lag + 1) return TimeSeriesError.BufferTooSmall;

    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (!isFiniteF64(data[i])) return TimeSeriesError.NonFiniteInput;
        sum += data[i];
    }
    const mean: f64 = sum / @as(f64, @floatFromInt(data.len));

    let denom: f64 = 0.0;
    i = 0;
    while (i < data.len) : (i += 1) {
        const d: f64 = data[i] - mean;
        denom += d * d;
    }
    if (denom <= 1.0e-18) {
        let lag: usize = 0;
        while (lag <= max_lag) : (lag += 1) out_acf[lag] = if (lag == 0) 1.0 else 0.0;
        return;
    }

    out_acf[0] = 1.0;
    let lag: usize = 1;
    while (lag <= max_lag) : (lag += 1) {
        let num: f64 = 0.0;
        let t: usize = 0;
        while (t + lag < data.len) : (t += 1) {
            num += (data[t] - mean) * (data[t + lag] - mean);
        }
        out_acf[lag] = num / denom;
    }
}

/// Summary returned by `levinsonDurbinAr`.
pub struct ArModelFit {
    order: usize = 0;
    mean: f64 = 0.0;
    noise_variance: f64 = 0.0;
}

/// Solves Yule-Walker equations in O(p^2) time via Levinson-Durbin recursion, producing:
///   - `out_ar[0..order]`: AR(p) coefficients `phi_1 .. phi_p`
///   - `out_pacf[0..order]`: partial autocorrelations `pacf[k - 1]` for lags `1..=order`
/// Requires `acf[0..=order]` (with `acf[0] == 1.0`), `prev_work.len >= order`, and sample variance `gamma0`.
pub fn levinsonDurbinAr(
    acf: []const f64,
    gamma0: f64,
    mean: f64,
    order: usize,
    out_ar: *[]f64,
    out_pacf: *[]f64,
    prev_work: *[]f64,
): ArModelFit ! TimeSeriesError {
    if (order == 0 || !isFiniteF64(gamma0) || gamma0 < 0.0) return TimeSeriesError.InvalidParameter;
    if (acf.len < order + 1) return TimeSeriesError.InsufficientData;
    if (out_ar.len < order || out_pacf.len < order || prev_work.len < order) {
        return TimeSeriesError.BufferTooSmall;
    }

    let err_var: f64 = 1.0; // normalized error variance (starts at acf[0] = 1)
    let m: usize = 1;
    while (m <= order) : (m += 1) {
        let num: f64 = acf[m];
        let j: usize = 1;
        while (j < m) : (j += 1) {
            num -= prev_work[j - 1] * acf[m - j];
        }
        if (absF64(err_var) <= 1.0e-15) return TimeSeriesError.SingularSystem;
        let k_m: f64 = num / err_var;
        if (k_m > 0.999999) k_m = 0.999999;
        if (k_m < -0.999999) k_m = -0.999999;

        out_pacf[m - 1] = k_m;
        out_ar[m - 1] = k_m;
        j = 1;
        while (j < m) : (j += 1) {
            out_ar[j - 1] = prev_work[j - 1] - k_m * prev_work[m - 1 - j];
        }
        j = 0;
        while (j < m) : (j += 1) {
            prev_work[j] = out_ar[j];
        }
        err_var *= (1.0 - k_m * k_m);
        if (err_var < 0.0) err_var = 0.0;
    }

    return ArModelFit{
        order: order,
        mean: mean,
        noise_variance: gamma0 * err_var,
    };
}

/// Generates `horizon` multi-step forecasts from an AR(p) model fitted by `levinsonDurbinAr`.
pub fn forecastArModel(
    history: []const f64,
    ar_coeffs: []const f64,
    mean: f64,
    horizon: usize,
    out_forecast: *[]f64,
): void ! TimeSeriesError {
    const p: usize = ar_coeffs.len;
    if (p == 0) return TimeSeriesError.InvalidParameter;
    if (history.len < p) return TimeSeriesError.InsufficientData;
    if (out_forecast.len < horizon) return TimeSeriesError.BufferTooSmall;

    let h: usize = 0;
    while (h < horizon) : (h += 1) {
        let pred: f64 = mean;
        let k: usize = 0;
        while (k < p) : (k += 1) {
            const lag_val: f64 = if (h >= k + 1)
                out_forecast[h - 1 - k]
            else
                history[history.len - (k + 1 - h)];
            pred += ar_coeffs[k] * (lag_val - mean);
        }
        out_forecast[h] = pred;
    }
}

// ============================================================================
// 4. Production Linear Kalman Filters (2-State 1D Tracker & Arbitrary N-D Filter)
// ============================================================================

/// 2-state constant-velocity linear Kalman filter (`[position, velocity]`) with Joseph-form update.
pub struct Kalman1dFilter {
    pos: f64 = 0.0;
    vel: f64 = 0.0;
    p00: f64 = 1.0;
    p01: f64 = 0.0;
    p10: f64 = 0.0;
    p11: f64 = 1.0;
    process_accel_var: f64 = 0.1;
    meas_var: f64 = 1.0;

    static fn init(
        init_pos: f64,
        init_vel: f64,
        init_pos_var: f64,
        init_vel_var: f64,
        process_accel_var: f64,
        meas_var: f64,
    ): Kalman1dFilter ! TimeSeriesError {
        if (init_pos_var <= 0.0 || init_vel_var <= 0.0 || process_accel_var < 0.0 || meas_var <= 0.0) {
            return TimeSeriesError.InvalidParameter;
        }
        return Kalman1dFilter{
            pos: init_pos,
            vel: init_vel,
            p00: init_pos_var,
            p01: 0.0,
            p10: 0.0,
            p11: init_vel_var,
            process_accel_var: process_accel_var,
            meas_var: meas_var,
        };
    }

    /// Advances state & covariance by time step `dt` under continuous white-noise acceleration model.
    fn predict(*_, dt: f64) {
        const dt2: f64 = dt * dt;
        const dt3: f64 = dt2 * dt;
        const dt4: f64 = dt2 * dt2;

        _.pos += _.vel * dt;

        const q00: f64 = 0.25 * dt4 * _.process_accel_var;
        const q01: f64 = 0.5 * dt3 * _.process_accel_var;
        const q11: f64 = dt2 * _.process_accel_var;

        // P' = F * P * F^T + Q, where F = [[1, dt], [0, 1]]
        const np00: f64 = _.p00 + dt * (_.p10 + _.p01) + dt2 * _.p11 + q00;
        const np01: f64 = _.p01 + dt * _.p11 + q01;
        const np10: f64 = _.p10 + dt * _.p11 + q01;
        const np11: f64 = _.p11 + q11;

        _.p00 = np00;
        _.p01 = 0.5 * (np01 + np10);
        _.p10 = _.p01;
        _.p11 = np11;
    }

    /// Assimilates scalar position measurement `z`, returning normalized innovation squared (Mahalanobis^2).
    fn update(*_, z: f64): f64 {
        const y: f64 = z - _.pos;
        const s: f64 = _.p00 + _.meas_var;
        const k0: f64 = _.p00 / s;
        const k1: f64 = _.p10 / s;

        _.pos += k0 * y;
        _.vel += k1 * y;

        // Joseph stabilized covariance update: P = (I - K H) P (I - K H)^T + K R K^T
        const a00: f64 = 1.0 - k0;
        const a10: f64 = -k1;
        const t00: f64 = a00 * _.p00;
        const t01: f64 = a00 * _.p01;
        const t10: f64 = a10 * _.p00 + _.p10;
        const t11: f64 = a10 * _.p01 + _.p11;

        const np00: f64 = t00 * a00 + k0 * k0 * _.meas_var;
        const np01: f64 = t00 * a10 + t01 + k0 * k1 * _.meas_var;
        const np11: f64 = t10 * a10 + t11 + k1 * k1 * _.meas_var;

        _.p00 = np00;
        _.p01 = np01;
        _.p10 = np01;
        _.p11 = np11;

        return (y * y) / s;
    }
}

/// Diagnostics returned by `KalmanFilterN.update`.
pub struct KalmanUpdateInfo {
    mahalanobis_sq: f64 = 0.0;
    gated_rejected: bool = false;
}

/// Caller-owned scratch buffers for `KalmanFilterN` (`max_dim = max(n, m)`).
pub struct KalmanNScratch {
    mat_a: []f64 = []; // >= max_dim * max_dim
    mat_b: []f64 = []; // >= max_dim * max_dim
    mat_c: []f64 = []; // >= max_dim * max_dim
    mat_d: []f64 = []; // >= max_dim * max_dim
    bt_work: []f64 = []; // >= max_dim * max_dim
    vec_a: []f64 = []; // >= max_dim
    vec_b: []f64 = []; // >= max_dim
    piv: []usize = []; // >= max_dim
}

/// Unbounded N-dimensional linear Kalman filter with Joseph-stabilized covariance update
/// and optional Mahalanobis innovation gating (zero dynamic heap allocation).
pub struct KalmanFilterN {
    n: usize = 0; // state dimension
    m: usize = 0; // measurement dimension
    x: []f64 = []; // state vector [n]
    p: []f64 = []; // state covariance [n x n]

    static fn init(n: usize, m: usize, x_buf: []f64, p_buf: []f64): KalmanFilterN ! TimeSeriesError {
        if (n == 0 || m == 0) return TimeSeriesError.InvalidParameter;
        if (x_buf.len < n || p_buf.len < n * n) return TimeSeriesError.BufferTooSmall;
        return KalmanFilterN{
            n: n,
            m: m,
            x: x_buf[0..n],
            p: p_buf[0..n * n],
        };
    }

    /// Predicts `x <- F * x` and `P <- F * P * F^T + Q`.
    fn predict(*_, f_mat: MatrixViewF64, q_mat: MatrixViewF64, sc: *KalmanNScratch): void ! TimeSeriesError {
        const n: usize = _.n;
        if (f_mat.rows != n || f_mat.cols != n || q_mat.rows != n || q_mat.cols != n) {
            return TimeSeriesError.DimensionMismatch;
        }
        if (sc.mat_a.len < n * n || sc.mat_b.len < n * n || sc.bt_work.len < n * n || sc.vec_a.len < n) {
            return TimeSeriesError.BufferTooSmall;
        }

        // x_new = F * x
        matvecF64(f_mat, _.x, 1.0, 0.0, *sc.vec_a) catch return TimeSeriesError.DimensionMismatch;
        let i: usize = 0;
        while (i < n) : (i += 1) _.x[i] = sc.vec_a[i];

        // mat_a = F * P
        const p_view: MatrixViewF64 = MatrixViewF64.init(_.p, n, n) catch return TimeSeriesError.DimensionMismatch;
        let ma: MatrixMutF64 = MatrixMutF64.init(sc.mat_a, n, n) catch return TimeSeriesError.BufferTooSmall;
        let mb: MatrixMutF64 = MatrixMutF64.init(sc.mat_b, n, n) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(f_mat, p_view, 1.0, 0.0, *ma, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;

        // mb = F^T
        transposeMatrixF64(f_mat, *mb) catch return TimeSeriesError.DimensionMismatch;

        // P = (F * P) * F^T + Q
        let mp: MatrixMutF64 = MatrixMutF64.init(_.p, n, n) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(ma.asView(), mb.asView(), 1.0, 0.0, *mp, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;
        i = 0;
        while (i < n * n) : (i += 1) {
            _.p[i] += q_mat.data[i];
        }
    }

    /// Assimilates measurement `z` (`[m]`) with observation matrix `H` (`m x n`) and noise `R` (`m x m`).
    /// Uses Joseph-stabilized covariance update `P = (I-KH)P(I-KH)^T + K R K^T`.
    /// If `mahalanobis_gate_sq > 0` and innovation `y^T S^{-1} y > mahalanobis_gate_sq`, rejects update.
    fn update(
        *_,
        z: []const f64,
        h_mat: MatrixViewF64,
        r_mat: MatrixViewF64,
        mahalanobis_gate_sq: f64,
        sc: *KalmanNScratch,
    ): KalmanUpdateInfo ! TimeSeriesError {
        const n: usize = _.n;
        const m: usize = _.m;
        const max_d: usize = if (n > m) n else m;
        if (z.len < m || h_mat.rows != m || h_mat.cols != n || r_mat.rows != m || r_mat.cols != m) {
            return TimeSeriesError.DimensionMismatch;
        }
        if (sc.mat_a.len < max_d * max_d || sc.mat_b.len < max_d * max_d ||
            sc.mat_c.len < max_d * max_d || sc.mat_d.len < max_d * max_d ||
            sc.bt_work.len < max_d * max_d || sc.vec_a.len < max_d ||
            sc.vec_b.len < 2 * max_d || sc.piv.len < max_d)
        {
            return TimeSeriesError.BufferTooSmall;
        }

        // 1. Innovation y = z - H * x  (stored in sc.vec_a[0..m])
        matvecF64(h_mat, _.x, 1.0, 0.0, *sc.vec_a) catch return TimeSeriesError.DimensionMismatch;
        let i: usize = 0;
        while (i < m) : (i += 1) sc.vec_a[i] = z[i] - sc.vec_a[i];

        // 2. mat_a (n x m) = P * H^T
        let ht: MatrixMutF64 = MatrixMutF64.init(sc.mat_b, n, m) catch return TimeSeriesError.BufferTooSmall;
        transposeMatrixF64(h_mat, *ht) catch return TimeSeriesError.DimensionMismatch;
        const p_view: MatrixViewF64 = MatrixViewF64.init(_.p, n, n) catch return TimeSeriesError.DimensionMismatch;
        let pht: MatrixMutF64 = MatrixMutF64.init(sc.mat_a, n, m) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(p_view, ht.asView(), 1.0, 0.0, *pht, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;

        // 3. Innovation covariance S (m x m) = H * (P * H^T) + R  (stored in sc.mat_c)
        let s_mat: MatrixMutF64 = MatrixMutF64.init(sc.mat_c, m, m) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(h_mat, pht.asView(), 1.0, 0.0, *s_mat, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;
        i = 0;
        while (i < m * m) : (i += 1) s_mat.data[i] += r_mat.data[i];

        // 4. Invert S -> s_inv (m x m) in sc.mat_d
        * = luDecomposeInPlace(*s_mat, *sc.piv) catch return TimeSeriesError.SingularSystem;
        let s_inv: MatrixMutF64 = MatrixMutF64.init(sc.mat_d, m, m) catch return TimeSeriesError.BufferTooSmall;
        luInvert(s_mat.asView(), sc.piv, *s_inv, *sc.vec_b) catch return TimeSeriesError.SingularSystem;

        // 5. Mahalanobis distance squared: d^2 = y^T * S^{-1} * y
        matvecF64(s_inv.asView(), sc.vec_a[0..m], 1.0, 0.0, *sc.vec_b) catch return TimeSeriesError.DimensionMismatch;
        let mahal_sq: f64 = 0.0;
        i = 0;
        while (i < m) : (i += 1) mahal_sq += sc.vec_a[i] * sc.vec_b[i];

        if (mahalanobis_gate_sq > 0.0 && mahal_sq > mahalanobis_gate_sq) {
            return KalmanUpdateInfo{ mahalanobis_sq: mahal_sq, gated_rejected: true };
        }

        // 6. Kalman gain K (n x m) = (P * H^T) * S^{-1}  (stored in sc.mat_b)
        let k_mat: MatrixMutF64 = MatrixMutF64.init(sc.mat_b, n, m) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(pht.asView(), s_inv.asView(), 1.0, 0.0, *k_mat, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;

        // 7. State update: x <- x + K * y
        matvecF64(k_mat.asView(), sc.vec_a[0..m], 1.0, 0.0, *sc.vec_b) catch return TimeSeriesError.DimensionMismatch;
        i = 0;
        while (i < n) : (i += 1) _.x[i] += sc.vec_b[i];

        // 8. Joseph-form covariance update:
        //    ikh (n x n) = I - K * H   (stored in sc.mat_c)
        let ikh: MatrixMutF64 = MatrixMutF64.init(sc.mat_c, n, n) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(k_mat.asView(), h_mat, -1.0, 0.0, *ikh, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;
        i = 0;
        while (i < n) : (i += 1) ikh.set(i, i, ikh.at(i, i) + 1.0);

        //    t1 (n x n) = ikh * P  (stored in sc.mat_a)
        let t1: MatrixMutF64 = MatrixMutF64.init(sc.mat_a, n, n) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(ikh.asView(), p_view, 1.0, 0.0, *t1, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;

        //    ikh_t (n x n) = ikh^T (stored in sc.mat_d)
        let ikh_t: MatrixMutF64 = MatrixMutF64.init(sc.mat_d, n, n) catch return TimeSeriesError.BufferTooSmall;
        transposeMatrixF64(ikh.asView(), *ikh_t) catch return TimeSeriesError.DimensionMismatch;

        //    P <- t1 * ikh_t
        let mp: MatrixMutF64 = MatrixMutF64.init(_.p, n, n) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(t1.asView(), ikh_t.asView(), 1.0, 0.0, *mp, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;

        //    kr (n x m) = K * R (stored in sc.mat_a)
        let kr: MatrixMutF64 = MatrixMutF64.init(sc.mat_a, n, m) catch return TimeSeriesError.BufferTooSmall;
        matmulF64(k_mat.asView(), r_mat, 1.0, 0.0, *kr, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;

        //    kt (m x n) = K^T (stored in sc.mat_d)
        let kt: MatrixMutF64 = MatrixMutF64.init(sc.mat_d, m, n) catch return TimeSeriesError.BufferTooSmall;
        transposeMatrixF64(k_mat.asView(), *kt) catch return TimeSeriesError.DimensionMismatch;

        //    P <- P + kr * kt
        matmulF64(kr.asView(), kt.asView(), 1.0, 1.0, *mp, *sc.bt_work) catch return TimeSeriesError.DimensionMismatch;

        return KalmanUpdateInfo{ mahalanobis_sq: mahal_sq, gated_rejected: false };
    }
}

// ============================================================================
// 5. Statistical Anomaly Detection (Z-Score, Robust MAD), CUSUM & Metrics
// ============================================================================

/// Detects anomalies via causal rolling Z-score `|x[t] - mean| / std > z_thresh` (using trailing window).
/// Returns total count of anomalous points written to `out_flags`.
pub fn detectAnomaliesZScore(
    data: []const f64,
    window: usize,
    z_thresh: f64,
    out_flags: *[]AnomalyFlag,
): usize ! TimeSeriesError {
    if (window < 3 || !isFiniteF64(z_thresh) || z_thresh <= 0.0) return TimeSeriesError.InvalidParameter;
    if (data.len == 0) return TimeSeriesError.InsufficientData;
    if (out_flags.len < data.len) return TimeSeriesError.BufferTooSmall;

    let count: usize = 0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (!isFiniteF64(data[i])) return TimeSeriesError.NonFiniteInput;
        if (i < window) {
            out_flags[i] = AnomalyFlag.normal;
            continue;
        }
        // Compute mean & std strictly over prior window [i - window .. i)
        let mean: f64 = 0.0;
        let m2: f64 = 0.0;
        let k: usize = 0;
        while (k < window) : (k += 1) {
            const x: f64 = data[i - window + k];
            const kf: f64 = @as(f64, @floatFromInt(k + 1));
            const d: f64 = x - mean;
            mean += d / kf;
            m2 += d * (x - mean);
        }
        const std_dev: f64 = std.math.sqrt(m2 / @as(f64, @floatFromInt(window - 1)));
        if (std_dev <= 1.0e-12) {
            out_flags[i] = AnomalyFlag.normal;
            continue;
        }
        const z: f64 = (data[i] - mean) / std_dev;
        if (z > z_thresh) {
            out_flags[i] = AnomalyFlag.high;
            count += 1;
        } else if (z < -z_thresh) {
            out_flags[i] = AnomalyFlag.low;
            count += 1;
        } else {
            out_flags[i] = AnomalyFlag.normal;
        }
    }
    return count;
}

fn sortF64InPlace(arr: *[]f64, n: usize) {
    let i: usize = 1;
    while (i < n) : (i += 1) {
        const key: f64 = arr[i];
        let j: usize = i;
        while (j > 0 && arr[j - 1] > key) : (j -= 1) {
            arr[j] = arr[j - 1];
        }
        arr[j] = key;
    }
}

fn medianSorted(sorted: []const f64, n: usize): f64 {
    if (n == 0) return 0.0;
    if ((n % 2) == 1) return sorted[n / 2];
    return 0.5 * (sorted[n / 2 - 1] + sorted[n / 2]);
}

/// Robust anomaly detection using Median Absolute Deviation (MAD):
/// `modified_z = 0.67448975 * (x - median) / MAD`. Immune to outlier masking even when >20%
/// of the baseline is contaminated. Requires `sort_work.len >= data.len`.
pub fn detectAnomaliesMad(
    data: []const f64,
    thresh: f64,
    sort_work: *[]f64,
    out_flags: *[]AnomalyFlag,
): usize ! TimeSeriesError {
    if (!isFiniteF64(thresh) || thresh <= 0.0) return TimeSeriesError.InvalidParameter;
    if (data.len < 3) return TimeSeriesError.InsufficientData;
    if (sort_work.len < data.len || out_flags.len < data.len) return TimeSeriesError.BufferTooSmall;

    const n: usize = data.len;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (!isFiniteF64(data[i])) return TimeSeriesError.NonFiniteInput;
        sort_work[i] = data[i];
    }
    sortF64InPlace(sort_work, n);
    const med: f64 = medianSorted(sort_work.*, n);

    i = 0;
    while (i < n) : (i += 1) {
        sort_work[i] = absF64(data[i] - med);
    }
    sortF64InPlace(sort_work, n);
    const mad: f64 = medianSorted(sort_work.*, n);

    let count: usize = 0;
    const scale: f64 = if (mad > 1.0e-12) (0.6744897501960817 / mad) else 0.0;
    i = 0;
    while (i < n) : (i += 1) {
        const diff: f64 = data[i] - med;
        if (scale == 0.0) {
            if (absF64(diff) > 1.0e-12) {
                out_flags[i] = if (diff > 0.0) AnomalyFlag.high else AnomalyFlag.low;
                count += 1;
            } else {
                out_flags[i] = AnomalyFlag.normal;
            }
        } else {
            const mz: f64 = diff * scale;
            if (mz > thresh) {
                out_flags[i] = AnomalyFlag.high;
                count += 1;
            } else if (mz < -thresh) {
                out_flags[i] = AnomalyFlag.low;
                count += 1;
            } else {
                out_flags[i] = AnomalyFlag.normal;
            }
        }
    }
    return count;
}

/// Online changepoint alarm state emitted by `CusumChangeDetector.update`.
pub struct CusumChangeResult {
    triggered: bool = false;
    direction: AnomalyFlag = AnomalyFlag.normal;
    g_pos: f64 = 0.0;
    g_neg: f64 = 0.0;
    sample_index: usize = 0;
}

/// Two-sided online Page-Hinkley / CUSUM mean-shift changepoint detector.
pub struct CusumChangeDetector {
    target_mean: f64 = 0.0;
    drift_k: f64 = 0.5;
    threshold_h: f64 = 5.0;
    g_pos: f64 = 0.0;
    g_neg: f64 = 0.0;
    count: usize = 0;

    static fn init(target_mean: f64, drift_k: f64, threshold_h: f64): CusumChangeDetector ! TimeSeriesError {
        if (!isFiniteF64(target_mean) || !isFiniteF64(drift_k) || drift_k < 0.0 ||
            !isFiniteF64(threshold_h) || threshold_h <= 0.0)
        {
            return TimeSeriesError.InvalidParameter;
        }
        return CusumChangeDetector{
            target_mean: target_mean,
            drift_k: drift_k,
            threshold_h: threshold_h,
        };
    }

    fn reset(*_) {
        _.g_pos = 0.0;
        _.g_neg = 0.0;
        _.count = 0;
    }

    /// Processes the next observation `x` and returns alarm status (auto-resetting accumulators on alarm).
    fn update(*_, x: f64): CusumChangeResult {
        const idx: usize = _.count;
        _.count += 1;
        const diff: f64 = x - _.target_mean;

        const next_pos: f64 = _.g_pos + diff - _.drift_k;
        _.g_pos = if (next_pos > 0.0) next_pos else 0.0;

        const next_neg: f64 = _.g_neg - diff - _.drift_k;
        _.g_neg = if (next_neg > 0.0) next_neg else 0.0;

        if (_.g_pos > _.threshold_h) {
            const res: CusumChangeResult = CusumChangeResult{
                triggered: true,
                direction: AnomalyFlag.high,
                g_pos: _.g_pos,
                g_neg: _.g_neg,
                sample_index: idx,
            };
            _.g_pos = 0.0;
            _.g_neg = 0.0;
            return res;
        }
        if (_.g_neg > _.threshold_h) {
            const res: CusumChangeResult = CusumChangeResult{
                triggered: true,
                direction: AnomalyFlag.low,
                g_pos: _.g_pos,
                g_neg: _.g_neg,
                sample_index: idx,
            };
            _.g_pos = 0.0;
            _.g_neg = 0.0;
            return res;
        }
        return CusumChangeResult{
            triggered: false,
            direction: AnomalyFlag.normal,
            g_pos: _.g_pos,
            g_neg: _.g_neg,
            sample_index: idx,
        };
    }
}

/// Comprehensive forecast accuracy metrics returned by `evalForecastMetrics`.
pub struct TsForecastMetrics {
    mae: f64 = 0.0;
    rmse: f64 = 0.0;
    mape_pct: f64 = 0.0;
    smape_pct: f64 = 0.0;
    r_squared: f64 = 0.0;
}

/// Computes MAE, RMSE, MAPE (%), sMAPE (%), and R^2 between `actual` and `predicted` in a single pass.
pub fn evalForecastMetrics(actual: []const f64, predicted: []const f64): TsForecastMetrics ! TimeSeriesError {
    if (actual.len == 0) return TimeSeriesError.InsufficientData;
    if (actual.len != predicted.len) return TimeSeriesError.DimensionMismatch;

    const n: usize = actual.len;
    const nf: f64 = @as(f64, @floatFromInt(n));
    let mean_y: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (!isFiniteF64(actual[i]) || !isFiniteF64(predicted[i])) return TimeSeriesError.NonFiniteInput;
        mean_y += actual[i];
    }
    mean_y /= nf;

    let sum_ae: f64 = 0.0;
    let sum_se: f64 = 0.0;
    let sum_tot: f64 = 0.0;
    let sum_mape: f64 = 0.0;
    let mape_cnt: usize = 0;
    let sum_smape: f64 = 0.0;
    let smape_cnt: usize = 0;

    i = 0;
    while (i < n) : (i += 1) {
        const y: f64 = actual[i];
        const p: f64 = predicted[i];
        const e: f64 = y - p;
        const ae: f64 = absF64(e);
        sum_ae += ae;
        sum_se += e * e;
        const dy: f64 = y - mean_y;
        sum_tot += dy * dy;

        const ay: f64 = absF64(y);
        if (ay > 1.0e-12) {
            sum_mape += ae / ay;
            mape_cnt += 1;
        }
        const denom: f64 = ay + absF64(p);
        if (denom > 1.0e-12) {
            sum_smape += (2.0 * ae) / denom;
            smape_cnt += 1;
        }
    }

    return TsForecastMetrics{
        mae: sum_ae / nf,
        rmse: std.math.sqrt(sum_se / nf),
        mape_pct: if (mape_cnt > 0) (100.0 * sum_mape / @as(f64, @floatFromInt(mape_cnt))) else 0.0,
        smape_pct: if (smape_cnt > 0) (100.0 * sum_smape / @as(f64, @floatFromInt(smape_cnt))) else 0.0,
        r_squared: if (sum_tot > 1.0e-15) (1.0 - sum_se / sum_tot) else 1.0,
    };
}
