// Origin: Pure-CP Stdlib (Zero-Alloc Sparse Regularized Regression: Lasso, Ridge & ElasticNet via Cyclic Coordinate Descent) | Created: 2026-11-22

/// Soft-thresholding proximal operator `S(z, gamma) = sign(z) * max(|z| - gamma, 0)`.
pub fn softThreshold(z: f64, gamma: f64): f64 {
    if (gamma <= 0.0) {
        return z;
    }
    if (z > gamma) {
        return z - gamma;
    }
    if (z < (0.0 - gamma)) {
        return z + gamma;
    }
    return 0.0;
}

/// Hyperparameters for ElasticNet / Lasso / Ridge coordinate descent.
/// Minimizes `(1 / (2*n)) * ||y - Xw - b||_2^2 + alpha * l1_ratio * ||w||_1 + 0.5 * alpha * (1 - l1_ratio) * ||w||_2^2`.
pub struct LassoConfig {
    alpha: f64,
    l1_ratio: f64,
    fit_intercept: bool,
    max_iter: usize,
    tol: f64,

    /// Default pure Lasso configuration (`l1_ratio = 1.0`).
    static fn defaultLasso(alpha: f64): LassoConfig {
        return LassoConfig {
            alpha: alpha,
            l1_ratio: 1.0,
            fit_intercept: true,
            max_iter: 2000,
            tol: 1.0e-8,
        };
    }

    /// Default pure Ridge configuration (`l1_ratio = 0.0`).
    static fn defaultRidge(alpha: f64): LassoConfig {
        return LassoConfig {
            alpha: alpha,
            l1_ratio: 0.0,
            fit_intercept: true,
            max_iter: 2000,
            tol: 1.0e-10,
        };
    }

    /// Default ElasticNet configuration for arbitrary `l1_ratio` in `[0.0, 1.0]`.
    static fn defaultElasticNet(alpha: f64, l1_ratio: f64): LassoConfig {
        return LassoConfig {
            alpha: alpha,
            l1_ratio: l1_ratio,
            fit_intercept: true,
            max_iter: 2000,
            tol: 1.0e-8,
        };
    }
}

/// Summary returned by `elasticNetFit`, `lassoFit`, and `ridgeFit`.
pub struct LassoResult {
    intercept: f64,
    iterations: usize,
    converged: bool,
    mse: f64,
    non_zero_count: usize,
}

/// Caller-provided zero-allocation workspace for cyclic coordinate descent.
/// Required lengths: `residual >= n_samples`, `col_norm_sq >= n_features`, `x_mean >= n_features`.
pub struct LassoWorkspace {
    residual: []f64,
    col_norm_sq: []f64,
    x_mean: []f64,
}

fn isFiniteF64(v: f64): bool {
    return (v == v) && ((v - v) == 0.0);
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) (0.0 - v) else v;
}

fn checkMatrixBounds(buf_len: usize, rows: usize, cols: usize): bool {
    return rows > 0 && cols > 0 && (buf_len / cols) >= rows;
}

fn checkFinitePrefix(data: []const f64, count: usize): bool {
    let i: usize = 0;
    while (i < count) {
        if (!isFiniteF64(data[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Fits an ElasticNet (Lasso / Ridge / hybrid) linear model via zero-allocation Cyclic Coordinate Descent.
/// Maintains residual `r = y - Xw - b` incrementally in O(n) per coordinate update without forming `X^T X`.
pub fn elasticNetFit(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    cfg: LassoConfig,
    w_out: *[]f64,
    ws: *LassoWorkspace,
): LassoResult? {
    if (n_samples == 0 || n_features == 0 || cfg.max_iter == 0) {
        return null;
    }
    if (!checkMatrixBounds(x.len, n_samples, n_features) || y.len < n_samples) {
        return null;
    }
    if (
        !isFiniteF64(cfg.alpha) || cfg.alpha < 0.0 ||
        !isFiniteF64(cfg.l1_ratio) || cfg.l1_ratio < 0.0 || cfg.l1_ratio > 1.0 ||
        !isFiniteF64(cfg.tol) || cfg.tol < 0.0
    ) {
        return null;
    }
    if (
        w_out.*.len < n_features ||
        ws.*.residual.len < n_samples ||
        ws.*.col_norm_sq.len < n_features ||
        ws.*.x_mean.len < n_features
    ) {
        return null;
    }
    if (!checkFinitePrefix(x, n_samples * n_features) || !checkFinitePrefix(y, n_samples)) {
        return null;
    }

    let w: []f64 = w_out.*;
    let res: []f64 = ws.*.residual;
    let col_norm_sq: []f64 = ws.*.col_norm_sq;
    let x_mean: []f64 = ws.*.x_mean;

    const n_f: f64 = n_samples as f64;
    const inv_n: f64 = 1.0 / n_f;

    // Initialize weights and compute feature/target means when fitting intercept.
    let y_mean: f64 = 0.0;
    let j0: usize = 0;
    while (j0 < n_features) {
        w[j0] = 0.0;
        x_mean[j0] = 0.0;
        col_norm_sq[j0] = 0.0;
        j0 += 1;
    }

    if (cfg.fit_intercept) {
        let i: usize = 0;
        while (i < n_samples) {
            y_mean += y[i];
            const row: usize = i * n_features;
            let j: usize = 0;
            while (j < n_features) {
                x_mean[j] += x[row + j];
                j += 1;
            }
            i += 1;
        }
        y_mean = y_mean * inv_n;
        let j: usize = 0;
        while (j < n_features) {
            x_mean[j] = x_mean[j] * inv_n;
            j += 1;
        }
    }

    // Initialize centered residual `r = y - y_mean` and precompute column squared norms.
    let i_init: usize = 0;
    while (i_init < n_samples) {
        res[i_init] = y[i_init] - y_mean;
        const row: usize = i_init * n_features;
        let j: usize = 0;
        while (j < n_features) {
            const x_c: f64 = x[row + j] - x_mean[j];
            col_norm_sq[j] += x_c * x_c;
            j += 1;
        }
        i_init += 1;
    }

    const n_l1: f64 = n_f * cfg.alpha * cfg.l1_ratio;
    const n_l2: f64 = n_f * cfg.alpha * (1.0 - cfg.l1_ratio);

    let converged: bool = false;
    let iters: usize = 0;

    while (iters < cfg.max_iter) {
        iters += 1;
        let max_delta: f64 = 0.0;
        let max_w: f64 = 0.0;

        let j: usize = 0;
        while (j < n_features) {
            const norm_sq: f64 = col_norm_sq[j];
            const w_old: f64 = w[j];
            let w_new: f64 = 0.0;

            if (norm_sq > 0.0) {
                const xm: f64 = x_mean[j];
                let dot_xr: f64 = 0.0;
                let i: usize = 0;
                while (i < n_samples) {
                    dot_xr += (x[i * n_features + j] - xm) * res[i];
                    i += 1;
                }
                const rho: f64 = dot_xr + (w_old * norm_sq);
                const denom: f64 = norm_sq + n_l2;
                if (denom > 0.0) {
                    w_new = softThreshold(rho, n_l1) / denom;
                }
            }

            const dw: f64 = w_new - w_old;
            if (dw != 0.0) {
                w[j] = w_new;
                const xm: f64 = x_mean[j];
                let i: usize = 0;
                while (i < n_samples) {
                    res[i] -= dw * (x[i * n_features + j] - xm);
                    i += 1;
                }
                const abs_dw: f64 = absF64(dw);
                if (abs_dw > max_delta) {
                    max_delta = abs_dw;
                }
            }

            const abs_w: f64 = absF64(w_new);
            if (abs_w > max_w) {
                max_w = abs_w;
            }
            j += 1;
        }

        const scale: f64 = if (max_w > 1.0) max_w else 1.0;
        if (max_delta <= (cfg.tol * scale)) {
            converged = true;
            break;
        }
    }

    // Recover intercept from centered solution: b = y_mean - dot(x_mean, w).
    let intercept: f64 = 0.0;
    let non_zero: usize = 0;
    if (cfg.fit_intercept) {
        let dot_mw: f64 = 0.0;
        let j: usize = 0;
        while (j < n_features) {
            dot_mw += x_mean[j] * w[j];
            if (w[j] != 0.0) {
                non_zero += 1;
            }
            j += 1;
        }
        intercept = y_mean - dot_mw;
    } else {
        let j: usize = 0;
        while (j < n_features) {
            if (w[j] != 0.0) {
                non_zero += 1;
            }
            j += 1;
        }
    }

    // Compute exact final MSE directly from maintained residual vector.
    let sse: f64 = 0.0;
    let k: usize = 0;
    while (k < n_samples) {
        sse += res[k] * res[k];
        k += 1;
    }

    return LassoResult {
        intercept: intercept,
        iterations: iters,
        converged: converged,
        mse: sse * inv_n,
        non_zero_count: non_zero,
    };
}

/// Convenience wrapper for pure L1 Lasso regression (`l1_ratio = 1.0`).
pub fn lassoFit(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    alpha: f64,
    fit_intercept: bool,
    w_out: *[]f64,
    ws: *LassoWorkspace,
): LassoResult? {
    let cfg: LassoConfig = LassoConfig.defaultLasso(alpha);
    cfg.fit_intercept = fit_intercept;
    return elasticNetFit(x, y, n_samples, n_features, cfg, w_out, ws);
}

/// Convenience wrapper for pure L2 Ridge regression (`l1_ratio = 0.0`).
pub fn ridgeFit(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    alpha: f64,
    fit_intercept: bool,
    w_out: *[]f64,
    ws: *LassoWorkspace,
): LassoResult? {
    let cfg: LassoConfig = LassoConfig.defaultRidge(alpha);
    cfg.fit_intercept = fit_intercept;
    return elasticNetFit(x, y, n_samples, n_features, cfg, w_out, ws);
}

/// Evaluates linear prediction `intercept + <w, sample[0..w.len]>`.
pub fn linearPredict(w: []const f64, intercept: f64, sample: []const f64): f64? {
    if (w.len == 0 || sample.len < w.len || !isFiniteF64(intercept)) {
        return null;
    }
    let acc: f64 = intercept;
    let j: usize = 0;
    while (j < w.len) {
        const wj: f64 = w[j];
        const xj: f64 = sample[j];
        if (!isFiniteF64(wj) || !isFiniteF64(xj)) {
            return null;
        }
        acc += wj * xj;
        j += 1;
    }
    return if (isFiniteF64(acc)) acc else null;
}

/// Computes Mean Squared Error `(1/n) * sum((y_true - y_pred)^2)`.
pub fn mseScore(y_true: []const f64, y_pred: []const f64): f64? {
    const n: usize = y_true.len;
    if (n == 0 || y_pred.len != n) {
        return null;
    }
    let sse: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const yt: f64 = y_true[i];
        const yp: f64 = y_pred[i];
        if (!isFiniteF64(yt) || !isFiniteF64(yp)) {
            return null;
        }
        const diff: f64 = yt - yp;
        sse += diff * diff;
        i += 1;
    }
    return sse / (n as f64);
}

/// Computes the coefficient of determination `R^2 = 1 - SS_res / SS_tot`.
pub fn r2Score(y_true: []const f64, y_pred: []const f64): f64? {
    const n: usize = y_true.len;
    if (n == 0 || y_pred.len != n) {
        return null;
    }
    let mean_y: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        const yt: f64 = y_true[i];
        const yp: f64 = y_pred[i];
        if (!isFiniteF64(yt) || !isFiniteF64(yp)) {
            return null;
        }
        mean_y += yt;
        i += 1;
    }
    mean_y = mean_y / (n as f64);

    let ss_res: f64 = 0.0;
    let ss_tot: f64 = 0.0;
    let k: usize = 0;
    while (k < n) {
        const diff_r: f64 = y_true[k] - y_pred[k];
        const diff_t: f64 = y_true[k] - mean_y;
        ss_res += diff_r * diff_r;
        ss_tot += diff_t * diff_t;
        k += 1;
    }

    if (ss_tot == 0.0) {
        return if (ss_res == 0.0) 1.0 else 0.0;
    }
    return 1.0 - (ss_res / ss_tot);
}
