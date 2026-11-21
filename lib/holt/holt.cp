// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Exponential Smoothing: SES, Holt Linear/Damped & Holt-Winters Seasonal)

const SAFE_EPS: f64 = 1.0e-15;

/// Seasonal decomposition model for Holt-Winters triple exponential smoothing.
pub enum SeasonalityKind {
    Additive,
    Multiplicative,
}

/// Result summary returned by `sesForecast`.
pub struct SesResult {
    level: f64,
    sse: f64,
    rmse: f64,
}

/// Hyperparameters for Holt double exponential smoothing (with optional damping `phi ∈ (0, 1]`).
pub struct HoltConfig {
    alpha: f64,
    beta: f64,
    phi: f64,

    fn default(): HoltConfig {
        return {
            alpha: 0.3,
            beta: 0.1,
            phi: 1.0,
        };
    }
}

/// Result summary returned by `holtForecast`.
pub struct HoltResult {
    level: f64,
    trend: f64,
    sse: f64,
    rmse: f64,
}

/// Hyperparameters for Holt-Winters triple exponential smoothing.
pub struct HoltWintersConfig {
    alpha: f64,
    beta: f64,
    gamma: f64,
    phi: f64,
    period: usize,
    kind: SeasonalityKind,
}

/// Result summary returned by `holtWintersForecast`.
pub struct HoltWintersResult {
    level: f64,
    trend: f64,
    sse: f64,
    rmse: f64,
}

/// Forecast accuracy metrics (`mape` and `smape` reported in percentage `[0, +inf)`).
pub struct ForecastMetrics {
    mae: f64,
    rmse: f64,
    mape: f64,
    smape: f64,
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn isAllFinite(xs: []const f64): bool {
    let i: usize = 0;
    while (i < xs.len) {
        if (!isFinite(xs[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isUnitOpenLeft(x: f64): bool {
    return isFinite(x) && x > 0.0 && x <= 1.0;
}

fn isUnitClosed(x: f64): bool {
    return isFinite(x) && x >= 0.0 && x <= 1.0;
}

/// Simple Exponential Smoothing (SES) with level parameter `alpha ∈ (0, 1]`.
/// Pass an empty slice for `fitted_out` to skip writing in-sample fitted values.
pub fn sesForecast(
    series: []const f64,
    alpha: f64,
    horizon: usize,
    fitted_out: *[]f64,
    forecast_out: *[]f64,
): SesResult? {
    const n = series.len;
    if (n == 0 || !isUnitOpenLeft(alpha)) {
        return null;
    }
    if (fitted_out.*.len != 0 && fitted_out.*.len < n) {
        return null;
    }
    if (forecast_out.*.len < horizon) {
        return null;
    }
    if (!isAllFinite(series)) {
        return null;
    }

    const write_fitted = fitted_out.*.len >= n;
    let level = series[0];
    if (write_fitted) {
        fitted_out.*[0] = level;
    }

    let sse: f64 = 0.0;
    let t: usize = 1;
    while (t < n) {
        const y_hat = level;
        if (write_fitted) {
            fitted_out.*[t] = y_hat;
        }
        const err = series[t] - y_hat;
        sse += err * err;
        level = alpha * series[t] + (1.0 - alpha) * level;
        t += 1;
    }

    let h: usize = 0;
    while (h < horizon) {
        forecast_out.*[h] = level;
        h += 1;
    }

    return {
        level: level,
        sse: sse,
        rmse: @sqrt(sse / (n as f64)),
    };
}

/// Holt double exponential smoothing with linear or damped trend (`phi ∈ (0, 1]`).
/// Requires `series.len >= 2`. Pass an empty slice for `fitted_out` to skip writing fitted values.
pub fn holtForecast(
    series: []const f64,
    cfg: HoltConfig,
    horizon: usize,
    fitted_out: *[]f64,
    forecast_out: *[]f64,
): HoltResult? {
    const n = series.len;
    if (n < 2) {
        return null;
    }
    if (!isUnitOpenLeft(cfg.alpha) || !isUnitClosed(cfg.beta) || !isUnitOpenLeft(cfg.phi)) {
        return null;
    }
    if (fitted_out.*.len != 0 && fitted_out.*.len < n) {
        return null;
    }
    if (forecast_out.*.len < horizon) {
        return null;
    }
    if (!isAllFinite(series)) {
        return null;
    }

    const write_fitted = fitted_out.*.len >= n;
    let level = series[0];
    let trend = series[1] - series[0];
    if (write_fitted) {
        fitted_out.*[0] = level;
    }

    let sse: f64 = 0.0;
    let t: usize = 1;
    while (t < n) {
        const damped_trend = cfg.phi * trend;
        const y_hat = level + damped_trend;
        if (write_fitted) {
            fitted_out.*[t] = y_hat;
        }
        const err = series[t] - y_hat;
        sse += err * err;

        const prev_level = level;
        level = cfg.alpha * series[t] + (1.0 - cfg.alpha) * y_hat;
        trend = cfg.beta * (level - prev_level) + (1.0 - cfg.beta) * damped_trend;
        t += 1;
    }

    let phi_pow: f64 = 1.0;
    let phi_sum: f64 = 0.0;
    let h: usize = 0;
    while (h < horizon) {
        phi_pow *= cfg.phi;
        phi_sum += phi_pow;
        forecast_out.*[h] = level + phi_sum * trend;
        h += 1;
    }

    return {
        level: level,
        trend: trend,
        sse: sse,
        rmse: @sqrt(sse / (n as f64)),
    };
}

/// Holt-Winters triple exponential smoothing supporting Additive and Multiplicative seasonality with optional damping.
/// Requires `period >= 2`, `series.len >= 2 * period`, and `seasonal_buf.*.len >= period`.
/// For `Multiplicative` seasonality, all series entries must be strictly positive (`> 0.0`).
pub fn holtWintersForecast(
    series: []const f64,
    cfg: HoltWintersConfig,
    horizon: usize,
    seasonal_buf: *[]f64,
    fitted_out: *[]f64,
    forecast_out: *[]f64,
): HoltWintersResult? {
    const n = series.len;
    const m = cfg.period;
    if (m < 2 || n < 2 * m) {
        return null;
    }
    if (!isUnitOpenLeft(cfg.alpha) || !isUnitClosed(cfg.beta) || !isUnitClosed(cfg.gamma) || !isUnitOpenLeft(cfg.phi)) {
        return null;
    }
    if (seasonal_buf.*.len < m || forecast_out.*.len < horizon) {
        return null;
    }
    if (fitted_out.*.len != 0 && fitted_out.*.len < n) {
        return null;
    }

    let i_chk: usize = 0;
    while (i_chk < n) {
        const v = series[i_chk];
        if (!isFinite(v)) {
            return null;
        }
        if (cfg.kind == SeasonalityKind.Multiplicative && v <= 0.0) {
            return null;
        }
        i_chk += 1;
    }

    // Classic two-period initialization for level, trend, and seasonal indices
    let sum1: f64 = 0.0;
    let sum2: f64 = 0.0;
    let k_m: usize = 0;
    while (k_m < m) {
        sum1 += series[k_m];
        sum2 += series[m + k_m];
        k_m += 1;
    }
    const mf = m as f64;
    const mean1 = sum1 / mf;
    const mean2 = sum2 / mf;
    let trend = (mean2 - mean1) / mf;
    const half_span = (mf - 1.0) * 0.5;
    let l0 = mean1 - trend * half_span;

    match (cfg.kind) {
        SeasonalityKind.Additive => {
            let s_sum: f64 = 0.0;
            let k: usize = 0;
            while (k < m) {
                const off = (k as f64) - half_span;
                const d1 = series[k] - (mean1 + trend * off);
                const d2 = series[m + k] - (mean2 + trend * off);
                const s_val = 0.5 * (d1 + d2);
                seasonal_buf.*[k] = s_val;
                s_sum += s_val;
                k += 1;
            }
            const s_avg = s_sum / mf;
            let k2: usize = 0;
            while (k2 < m) {
                seasonal_buf.*[k2] -= s_avg;
                k2 += 1;
            }
        },
        SeasonalityKind.Multiplicative => {
            let s_sum: f64 = 0.0;
            let k: usize = 0;
            while (k < m) {
                const off = (k as f64) - half_span;
                const b1 = mean1 + trend * off;
                const b2 = mean2 + trend * off;
                const r1 = if (b1 > SAFE_EPS) series[k] / b1 else series[k] / mean1;
                const r2 = if (b2 > SAFE_EPS) series[m + k] / b2 else series[m + k] / mean2;
                const s_val = 0.5 * (r1 + r2);
                seasonal_buf.*[k] = s_val;
                s_sum += s_val;
                k += 1;
            }
            const s_avg = s_sum / mf;
            if (s_avg > SAFE_EPS) {
                let k2: usize = 0;
                while (k2 < m) {
                    seasonal_buf.*[k2] = seasonal_buf.*[k2] / s_avg;
                    k2 += 1;
                }
            }
        },
    }

    // Align pre-step level so step t = 0 predicts l0
    let level = l0 - cfg.phi * trend;
    const write_fitted = fitted_out.*.len >= n;
    let sse: f64 = 0.0;

    let t: usize = 0;
    while (t < n) {
        const s_idx = t % m;
        const s_old = seasonal_buf.*[s_idx];
        const damped_trend = cfg.phi * trend;
        const base_pred = level + damped_trend;
        const yt = series[t];

        let y_hat: f64 = 0.0;
        const prev_level = level;

        match (cfg.kind) {
            SeasonalityKind.Additive => {
                y_hat = base_pred + s_old;
                level = cfg.alpha * (yt - s_old) + (1.0 - cfg.alpha) * base_pred;
                trend = cfg.beta * (level - prev_level) + (1.0 - cfg.beta) * damped_trend;
                seasonal_buf.*[s_idx] = cfg.gamma * (yt - level) + (1.0 - cfg.gamma) * s_old;
            },
            SeasonalityKind.Multiplicative => {
                y_hat = base_pred * s_old;
                const safe_s = if (absF64(s_old) > SAFE_EPS) s_old else SAFE_EPS;
                level = cfg.alpha * (yt / safe_s) + (1.0 - cfg.alpha) * base_pred;
                trend = cfg.beta * (level - prev_level) + (1.0 - cfg.beta) * damped_trend;
                const safe_l = if (absF64(level) > SAFE_EPS) level else SAFE_EPS;
                seasonal_buf.*[s_idx] = cfg.gamma * (yt / safe_l) + (1.0 - cfg.gamma) * s_old;
            },
        }

        if (write_fitted) {
            fitted_out.*[t] = y_hat;
        }
        const err = yt - y_hat;
        sse += err * err;
        t += 1;
    }

    // Multi-step forecast
    let phi_pow: f64 = 1.0;
    let phi_sum: f64 = 0.0;
    let h: usize = 0;
    while (h < horizon) {
        phi_pow *= cfg.phi;
        phi_sum += phi_pow;
        const s_idx = (n + h) % m;
        const trend_proj = level + phi_sum * trend;
        forecast_out.*[h] = match (cfg.kind) {
            SeasonalityKind.Additive => trend_proj + seasonal_buf.*[s_idx],
            SeasonalityKind.Multiplicative => trend_proj * seasonal_buf.*[s_idx],
        };
        h += 1;
    }

    return {
        level: level,
        trend: trend,
        sse: sse,
        rmse: @sqrt(sse / (n as f64)),
    };
}

/// Evaluates MAE, RMSE, MAPE (%), and sMAPE (%) between `actual` and `predicted`.
/// Returns `null` on empty/mismatched slices or non-finite values.
pub fn evaluateForecast(actual: []const f64, predicted: []const f64): ForecastMetrics? {
    const n = actual.len;
    if (n == 0 || n != predicted.len) {
        return null;
    }

    let abs_sum: f64 = 0.0;
    let sq_sum: f64 = 0.0;
    let mape_sum: f64 = 0.0;
    let mape_cnt: usize = 0;
    let smape_sum: f64 = 0.0;

    let i: usize = 0;
    while (i < n) {
        const a = actual[i];
        const p = predicted[i];
        if (!isFinite(a) || !isFinite(p)) {
            return null;
        }
        const err = absF64(a - p);
        abs_sum += err;
        sq_sum += err * err;

        const abs_a = absF64(a);
        if (abs_a > SAFE_EPS) {
            mape_sum += err / abs_a;
            mape_cnt += 1;
        }

        const denom_s = abs_a + absF64(p);
        if (denom_s > SAFE_EPS) {
            smape_sum += (2.0 * err) / denom_s;
        }
        i += 1;
    }

    const nf = n as f64;
    return {
        mae: abs_sum / nf,
        rmse: @sqrt(sq_sum / nf),
        mape: if (mape_cnt > 0) (100.0 * mape_sum) / (mape_cnt as f64) else 0.0,
        smape: (100.0 * smape_sum) / nf,
    };
}
