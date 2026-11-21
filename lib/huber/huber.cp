// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Robust Statistics, IRLS Huber/Tukey M-Estimation Regression & Theil-Sen 1D)

const MAD_NORM_FACTOR: f64 = 1.4826;
const PIVOT_EPS: f64 = 1.0e-14;

/// Robust loss family used by `huberRegress`.
pub enum RobustKind {
    Huber,
    Tukey,
}

/// Hyperparameters for IRLS robust linear regression (`huberRegress`).
pub struct HuberRegConfig {
    kind: RobustKind,
    delta: f64,
    l2_reg: f64,
    fit_intercept: bool,
    max_iter: usize,
    tol: f64,

    /// Default Huber IRLS configuration (`delta = 1.345` for 95% Gaussian efficiency).
    static fn defaultHuber(): HuberRegConfig {
        return HuberRegConfig {
            kind: RobustKind.Huber,
            delta: 1.345,
            l2_reg: 0.0,
            fit_intercept: true,
            max_iter: 100,
            tol: 1.0e-7,
        };
    }

    /// Default Tukey biweight IRLS configuration (`delta = 4.685` for 95% Gaussian efficiency).
    static fn defaultTukey(): HuberRegConfig {
        return HuberRegConfig {
            kind: RobustKind.Tukey,
            delta: 4.685,
            l2_reg: 0.0,
            fit_intercept: true,
            max_iter: 100,
            tol: 1.0e-7,
        };
    }
}

/// Summary returned by `huberRegress`.
pub struct HuberRegResult {
    intercept: f64,
    scale: f64,
    iterations: usize,
    converged: bool,
    loss: f64,
}

/// Caller-provided zero-allocation workspace for `huberRegress`.
/// Let `p = n_features + (fit_intercept ? 1 : 0)`.
/// Required lengths: `weights >= n_samples`, `residuals >= n_samples`, `ata >= p * p`,
/// `atb >= p`, `beta >= p`, `prev_beta >= p`, `piv >= p`, `scratch_a >= n_samples`, `scratch_b >= n_samples`.
pub struct HuberRegWorkspace {
    weights: []f64,
    residuals: []f64,
    ata: []f64,
    atb: []f64,
    beta: []f64,
    prev_beta: []f64,
    piv: []usize,
    scratch_a: []f64,
    scratch_b: []f64,
}

/// Result returned by `theilSen1D`.
pub struct TheilSenResult {
    slope: f64,
    intercept: f64,
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

/// In-place Shell sort over `buf[0..n]` in ascending order (zero extra memory, non-recursive).
fn sortF64InPlace(buf: []f64, n: usize) {
    let gap: usize = n / 2;
    while (gap > 0) {
        let i: usize = gap;
        while (i < n) {
            const key: f64 = buf[i];
            let j: usize = i;
            while (j >= gap && buf[j - gap] > key) {
                buf[j] = buf[j - gap];
                j -= gap;
            }
            buf[j] = key;
            i += 1;
        }
        gap = gap / 2;
    }
}

fn medianSorted(sorted: []const f64, n: usize): f64 {
    const mid: usize = n / 2;
    if ((n % 2) == 1) {
        return sorted[mid];
    }
    return 0.5 * (sorted[mid - 1] + sorted[mid]);
}

/// Computes the exact sample median using caller-provided `scratch` (length >= `data.len`).
pub fn median(data: []const f64, scratch: *[]f64): f64? {
    const n: usize = data.len;
    if (n == 0 || scratch.*.len < n || !checkFinitePrefix(data, n)) {
        return null;
    }
    let s: []f64 = scratch.*;
    let i: usize = 0;
    while (i < n) {
        s[i] = data[i];
        i += 1;
    }
    sortF64InPlace(s, n);
    return medianSorted(s, n);
}

/// Computes the normalized Median Absolute Deviation scale estimate:
/// `1.4826 * median(|x - median(x)|)` (consistent with standard deviation under normality).
pub fn madScale(data: []const f64, scratch: *[]f64, dev_scratch: *[]f64): f64? {
    const n: usize = data.len;
    if (n == 0 || scratch.*.len < n || dev_scratch.*.len < n) {
        return null;
    }
    const med: f64 = median(data, scratch) ?? return null;
    let devs: []f64 = dev_scratch.*;
    let i: usize = 0;
    while (i < n) {
        devs[i] = absF64(data[i] - med);
        i += 1;
    }
    sortF64InPlace(devs, n);
    return MAD_NORM_FACTOR * medianSorted(devs, n);
}

/// Computes the symmetric trimmed mean discarding `floor(n * trim_fraction)` values from each tail.
/// Requires `trim_fraction` in `[0.0, 0.5)`.
pub fn trimmedMean(data: []const f64, trim_fraction: f64, scratch: *[]f64): f64? {
    const n: usize = data.len;
    if (
        n == 0 ||
        scratch.*.len < n ||
        !isFiniteF64(trim_fraction) ||
        trim_fraction < 0.0 ||
        trim_fraction >= 0.5 ||
        !checkFinitePrefix(data, n)
    ) {
        return null;
    }
    let s: []f64 = scratch.*;
    let i: usize = 0;
    while (i < n) {
        s[i] = data[i];
        i += 1;
    }
    sortF64InPlace(s, n);

    let trim_k: usize = ((n as f64) * trim_fraction) as usize;
    if ((2 * trim_k) >= n) {
        trim_k = (n - 1) / 2;
    }
    const lo: usize = trim_k;
    const hi: usize = n - trim_k;
    let sum: f64 = 0.0;
    let idx: usize = lo;
    while (idx < hi) {
        sum += s[idx];
        idx += 1;
    }
    return sum / ((hi - lo) as f64);
}

/// Huber loss function: `0.5 * r^2` for `|r| <= delta`, else `delta * (|r| - 0.5 * delta)`.
pub fn huberLoss(r: f64, delta: f64): f64 {
    if (!isFiniteF64(r) || !isFiniteF64(delta) || delta <= 0.0) {
        return 0.0;
    }
    const a: f64 = absF64(r);
    if (a <= delta) {
        return 0.5 * a * a;
    }
    return delta * (a - (0.5 * delta));
}

/// Huber IRLS weight function: `1.0` for `|r| <= delta`, else `delta / |r|`.
pub fn huberWeight(r: f64, delta: f64): f64 {
    if (!isFiniteF64(r) || !isFiniteF64(delta) || delta <= 0.0) {
        return 0.0;
    }
    const a: f64 = absF64(r);
    if (a <= delta) {
        return 1.0;
    }
    return delta / a;
}

/// Tukey's biweight (bisquare) IRLS weight: `(1 - (r/c)^2)^2` for `|r| < c`, else `0.0`.
pub fn tukeyBiweight(r: f64, c: f64): f64 {
    if (!isFiniteF64(r) || !isFiniteF64(c) || c <= 0.0) {
        return 0.0;
    }
    const a: f64 = absF64(r);
    if (a >= c) {
        return 0.0;
    }
    const u: f64 = r / c;
    const q: f64 = 1.0 - (u * u);
    return q * q;
}

fn tukeyLoss(r: f64, c: f64): f64 {
    if (!isFiniteF64(r) || !isFiniteF64(c) || c <= 0.0) {
        return 0.0;
    }
    const c2_over_6: f64 = (c * c) / 6.0;
    const a: f64 = absF64(r);
    if (a >= c) {
        return c2_over_6;
    }
    const u: f64 = r / c;
    const q: f64 = 1.0 - (u * u);
    return c2_over_6 * (1.0 - (q * q * q));
}

/// Computes the Huber M-estimator of location using MAD scale and iterative reweighting.
pub fn huberLocation(
    data: []const f64,
    k: f64,
    max_iter: usize,
    tol: f64,
    scratch: *[]f64,
    dev_scratch: *[]f64,
): f64? {
    const n: usize = data.len;
    if (
        n == 0 ||
        !isFiniteF64(k) || k <= 0.0 ||
        max_iter == 0 ||
        !isFiniteF64(tol) || tol < 0.0
    ) {
        return null;
    }
    let mu: f64 = median(data, scratch) ?? return null;
    const scale: f64 = madScale(data, scratch, dev_scratch) ?? return null;
    if (scale <= 1.0e-15) {
        return mu;
    }

    const step_scale: f64 = if (scale > 1.0) scale else 1.0;
    let it: usize = 0;
    while (it < max_iter) {
        let sum_w: f64 = 0.0;
        let sum_wx: f64 = 0.0;
        let i: usize = 0;
        while (i < n) {
            const w: f64 = huberWeight((data[i] - mu) / scale, k);
            sum_w += w;
            sum_wx += w * data[i];
            i += 1;
        }
        if (sum_w <= 0.0) {
            break;
        }
        const mu_next: f64 = sum_wx / sum_w;
        const delta_mu: f64 = absF64(mu_next - mu);
        mu = mu_next;
        if (delta_mu <= (tol * step_scale)) {
            break;
        }
        it += 1;
    }
    return mu;
}

/// Solves dense `p x p` linear system `A * x = b` via Gaussian elimination with partial pivoting.
fn solveDenseSystem(a: []f64, b: []f64, x_out: []f64, piv: []usize, p: usize): bool {
    let idx0: usize = 0;
    while (idx0 < p) {
        piv[idx0] = idx0;
        idx0 += 1;
    }

    let col: usize = 0;
    while (col < p) {
        let best_row: usize = col;
        let best_abs: f64 = absF64(a[piv[col] * p + col]);
        let r: usize = col + 1;
        while (r < p) {
            const cand_abs: f64 = absF64(a[piv[r] * p + col]);
            if (cand_abs > best_abs) {
                best_abs = cand_abs;
                best_row = r;
            }
            r += 1;
        }
        if (!isFiniteF64(best_abs) || best_abs <= PIVOT_EPS) {
            return false;
        }
        if (best_row != col) {
            const tmp_p: usize = piv[col];
            piv[col] = piv[best_row];
            piv[best_row] = tmp_p;
        }

        const prow: usize = piv[col];
        const pivot_val: f64 = a[prow * p + col];
        let r2: usize = col + 1;
        while (r2 < p) {
            const crow: usize = piv[r2];
            const factor: f64 = a[crow * p + col] / pivot_val;
            a[crow * p + col] = 0.0;
            let c2: usize = col + 1;
            while (c2 < p) {
                a[crow * p + c2] -= factor * a[prow * p + c2];
                c2 += 1;
            }
            b[crow] -= factor * b[prow];
            r2 += 1;
        }
        col += 1;
    }

    // Back substitution
    let k: usize = p;
    while (k > 0) {
        k -= 1;
        const row: usize = piv[k];
        let sum: f64 = b[row];
        let c: usize = k + 1;
        while (c < p) {
            sum -= a[row * p + c] * x_out[c];
            c += 1;
        }
        const diag: f64 = a[row * p + k];
        if (absF64(diag) <= PIVOT_EPS) {
            return false;
        }
        x_out[k] = sum / diag;
    }
    return true;
}

/// Assembles and solves weighted normal equations `(A^T W A + l2_reg * D) beta = A^T W y`.
fn solveWeightedNormalEq(
    x: []const f64,
    y: []const f64,
    weights: []const f64,
    n_samples: usize,
    n_features: usize,
    fit_intercept: bool,
    l2_reg: f64,
    ws: *HuberRegWorkspace,
): bool {
    const p: usize = n_features + (if (fit_intercept) 1 else 0);
    let ata: []f64 = ws.*.ata;
    let atb: []f64 = ws.*.atb;

    let z: usize = 0;
    while (z < (p * p)) {
        ata[z] = 0.0;
        z += 1;
    }
    let z2: usize = 0;
    while (z2 < p) {
        atb[z2] = 0.0;
        z2 += 1;
    }

    let i: usize = 0;
    while (i < n_samples) {
        const w: f64 = weights[i];
        if (w > 0.0) {
            const x_row: usize = i * n_features;
            const yi: f64 = y[i];
            let a_col: usize = 0;
            while (a_col < p) {
                const va: f64 = if (a_col < n_features) x[x_row + a_col] else 1.0;
                const w_va: f64 = w * va;
                atb[a_col] += w_va * yi;
                let b_col: usize = a_col;
                while (b_col < p) {
                    const vb: f64 = if (b_col < n_features) x[x_row + b_col] else 1.0;
                    ata[a_col * p + b_col] += w_va * vb;
                    b_col += 1;
                }
                a_col += 1;
            }
        }
        i += 1;
    }

    // Mirror upper triangle to lower triangle and add L2 regularization on feature weights only.
    let r: usize = 0;
    while (r < p) {
        let c: usize = r + 1;
        while (c < p) {
            ata[c * p + r] = ata[r * p + c];
            c += 1;
        }
        if (r < n_features && l2_reg > 0.0) {
            ata[r * p + r] += l2_reg;
        }
        r += 1;
    }

    return solveDenseSystem(ata, atb, ws.*.beta, ws.*.piv, p);
}

fn computeResiduals(
    x: []const f64,
    y: []const f64,
    beta: []const f64,
    n_samples: usize,
    n_features: usize,
    fit_intercept: bool,
    res_out: []f64,
): f64 {
    const intercept: f64 = if (fit_intercept) beta[n_features] else 0.0;
    let max_abs_r: f64 = 0.0;
    let i: usize = 0;
    while (i < n_samples) {
        let pred: f64 = intercept;
        const row: usize = i * n_features;
        let j: usize = 0;
        while (j < n_features) {
            pred += x[row + j] * beta[j];
            j += 1;
        }
        const r: f64 = y[i] - pred;
        res_out[i] = r;
        const ar: f64 = absF64(r);
        if (ar > max_abs_r) {
            max_abs_r = ar;
        }
        i += 1;
    }
    return max_abs_r;
}

/// Fits a multivariate Huber or Tukey biweight robust linear regression model via IRLS.
/// For non-convex Tukey regression, performs a convex Huber warm-start before biweight refinement.
pub fn huberRegress(
    x: []const f64,
    y: []const f64,
    n_samples: usize,
    n_features: usize,
    cfg: HuberRegConfig,
    w_out: *[]f64,
    ws: *HuberRegWorkspace,
): HuberRegResult? {
    if (n_samples == 0 || n_features == 0 || cfg.max_iter == 0) {
        return null;
    }
    if (!checkMatrixBounds(x.len, n_samples, n_features) || y.len < n_samples) {
        return null;
    }
    if (
        !isFiniteF64(cfg.delta) || cfg.delta <= 0.0 ||
        !isFiniteF64(cfg.l2_reg) || cfg.l2_reg < 0.0 ||
        !isFiniteF64(cfg.tol) || cfg.tol < 0.0
    ) {
        return null;
    }
    const p: usize = n_features + (if (cfg.fit_intercept) 1 else 0);
    if (
        w_out.*.len < n_features ||
        ws.*.weights.len < n_samples ||
        ws.*.residuals.len < n_samples ||
        !checkMatrixBounds(ws.*.ata.len, p, p) ||
        ws.*.atb.len < p ||
        ws.*.beta.len < p ||
        ws.*.prev_beta.len < p ||
        ws.*.piv.len < p ||
        ws.*.scratch_a.len < n_samples ||
        ws.*.scratch_b.len < n_samples
    ) {
        return null;
    }
    if (!checkFinitePrefix(x, n_samples * n_features) || !checkFinitePrefix(y, n_samples)) {
        return null;
    }

    // Initial OLS fit with uniform weights.
    let idx0: usize = 0;
    while (idx0 < n_samples) {
        ws.*.weights[idx0] = 1.0;
        idx0 += 1;
    }
    if (!solveWeightedNormalEq(x, y, ws.*.weights, n_samples, n_features, cfg.fit_intercept, cfg.l2_reg, ws)) {
        return null;
    }

    let sa: []f64 = ws.*.scratch_a;
    let sb: []f64 = ws.*.scratch_b;

    // Convex Huber warm-start for non-convex Tukey biweight regression.
    const is_tukey: bool = match (cfg.kind) {
        .Huber => false,
        .Tukey => true,
    };
    if (is_tukey) {
        let warm: usize = 0;
        while (warm < 15) {
            const max_r: f64 = computeResiduals(x, y, ws.*.beta, n_samples, n_features, cfg.fit_intercept, ws.*.residuals);
            if (max_r <= 1.0e-12) {
                break;
            }
            const sc: f64 = madScale(ws.*.residuals[0..n_samples], *sa, *sb) ?? return null;
            const eff_sc: f64 = if (sc > 1.0e-12) sc else 1.0e-12;
            let k: usize = 0;
            while (k < n_samples) {
                ws.*.weights[k] = huberWeight(ws.*.residuals[k] / eff_sc, 1.345);
                k += 1;
            }
            if (!solveWeightedNormalEq(x, y, ws.*.weights, n_samples, n_features, cfg.fit_intercept, cfg.l2_reg, ws)) {
                break;
            }
            warm += 1;
        }
    }

    let converged: bool = false;
    let iters: usize = 0;
    let final_scale: f64 = 0.0;

    while (iters < cfg.max_iter) {
        iters += 1;
        let b_copy: usize = 0;
        while (b_copy < p) {
            ws.*.prev_beta[b_copy] = ws.*.beta[b_copy];
            b_copy += 1;
        }

        const max_r: f64 = computeResiduals(x, y, ws.*.beta, n_samples, n_features, cfg.fit_intercept, ws.*.residuals);
        if (max_r <= 1.0e-12) {
            final_scale = 0.0;
            converged = true;
            break;
        }

        const raw_sc: f64 = madScale(ws.*.residuals[0..n_samples], *sa, *sb) ?? return null;
        final_scale = raw_sc;
        const eff_sc: f64 = if (raw_sc > 1.0e-12) raw_sc else 1.0e-12;

        let active_w: usize = 0;
        let i: usize = 0;
        while (i < n_samples) {
            const u: f64 = ws.*.residuals[i] / eff_sc;
            const wv: f64 = if (is_tukey) tukeyBiweight(u, cfg.delta) else huberWeight(u, cfg.delta);
            ws.*.weights[i] = wv;
            if (wv > 0.0) {
                active_w += 1;
            }
            i += 1;
        }
        if (active_w < p) {
            // Retain previous robust estimate if biweight zeroed too many points.
            let r_back: usize = 0;
            while (r_back < p) {
                ws.*.beta[r_back] = ws.*.prev_beta[r_back];
                r_back += 1;
            }
            break;
        }

        if (!solveWeightedNormalEq(x, y, ws.*.weights, n_samples, n_features, cfg.fit_intercept, cfg.l2_reg, ws)) {
            let r_back: usize = 0;
            while (r_back < p) {
                ws.*.beta[r_back] = ws.*.prev_beta[r_back];
                r_back += 1;
            }
            break;
        }

        let max_diff: f64 = 0.0;
        let max_beta: f64 = 0.0;
        let j: usize = 0;
        while (j < p) {
            const d: f64 = absF64(ws.*.beta[j] - ws.*.prev_beta[j]);
            const ab: f64 = absF64(ws.*.beta[j]);
            if (d > max_diff) {
                max_diff = d;
            }
            if (ab > max_beta) {
                max_beta = ab;
            }
            j += 1;
        }

        const norm_ref: f64 = if (max_beta > 1.0) max_beta else 1.0;
        if (max_diff <= (cfg.tol * norm_ref)) {
            converged = true;
            break;
        }
    }

    // Recompute final residuals, scale, and mean robust loss.
    const final_max_r: f64 = computeResiduals(x, y, ws.*.beta, n_samples, n_features, cfg.fit_intercept, ws.*.residuals);
    let mean_loss: f64 = 0.0;
    if (final_max_r > 1.0e-12) {
        final_scale = madScale(ws.*.residuals[0..n_samples], *sa, *sb) ?? return null;
        const loss_sc: f64 = if (final_scale > 1.0e-12) final_scale else 1.0;
        let sum_loss: f64 = 0.0;
        let k: usize = 0;
        while (k < n_samples) {
            const u: f64 = ws.*.residuals[k] / loss_sc;
            sum_loss += if (is_tukey) tukeyLoss(u, cfg.delta) else huberLoss(u, cfg.delta);
            k += 1;
        }
        mean_loss = sum_loss / (n_samples as f64);
    }

    let w_dst: []f64 = w_out.*;
    let f: usize = 0;
    while (f < n_features) {
        w_dst[f] = ws.*.beta[f];
        f += 1;
    }
    const intercept: f64 = if (cfg.fit_intercept) ws.*.beta[n_features] else 0.0;

    return HuberRegResult {
        intercept: intercept,
        scale: final_scale,
        iterations: iters,
        converged: converged,
        loss: mean_loss,
    };
}

/// Non-parametric 1D Theil-Sen median-of-slopes regression (29.3% breakdown point).
/// Required scratch lengths: `slopes_scratch >= n * (n - 1) / 2`, `intercepts_scratch >= n`.
pub fn theilSen1D(
    xs: []const f64,
    ys: []const f64,
    slopes_scratch: *[]f64,
    intercepts_scratch: *[]f64,
): TheilSenResult? {
    const n: usize = xs.len;
    if (n < 2 || ys.len != n) {
        return null;
    }
    const max_pairs: usize = (n * (n - 1)) / 2;
    if (slopes_scratch.*.len < max_pairs || intercepts_scratch.*.len < n) {
        return null;
    }
    if (!checkFinitePrefix(xs, n) || !checkFinitePrefix(ys, n)) {
        return null;
    }

    let slopes: []f64 = slopes_scratch.*;
    let count: usize = 0;
    let i: usize = 0;
    while (i < n) {
        let j: usize = i + 1;
        while (j < n) {
            const dx: f64 = xs[j] - xs[i];
            if (absF64(dx) > 1.0e-15) {
                slopes[count] = (ys[j] - ys[i]) / dx;
                count += 1;
            }
            j += 1;
        }
        i += 1;
    }
    if (count == 0) {
        return null;
    }

    sortF64InPlace(slopes, count);
    const slope: f64 = medianSorted(slopes, count);

    let intercepts: []f64 = intercepts_scratch.*;
    let k: usize = 0;
    while (k < n) {
        intercepts[k] = ys[k] - (slope * xs[k]);
        k += 1;
    }
    sortF64InPlace(intercepts, n);
    const intercept: f64 = medianSorted(intercepts, n);

    return TheilSenResult {
        slope: slope,
        intercept: intercept,
    };
}
