// Origin: Pure-CP Stdlib (Zero-Alloc Nonlinear State Estimation: Numerical Jacobians, Joseph-Form EKF & Merwe-Scaled UKF) | Created: 2026-11-22

/// Nonlinear state transition callback: writes `f(x, u, dt)` into `x_next_out[0..x.len]`.
pub type TransitionFn = fn(x: []const f64, u: []const f64, dt: f64, x_next_out: *[]f64);

/// Nonlinear measurement callback: writes `h(x)` into `z_out[0..m]`.
pub type MeasurementFn = fn(x: []const f64, z_out: *[]f64);

/// Caller-owned zero-alloc workspace for `ekfUpdate` (state dim `n`, measurement dim `m`).
/// Required lengths:
/// - `z_pred`, `innov`, `tmp_m`: `>= m`
/// - `ph_t`, `k_gain`, `tmp_nm`: `>= n * m`
/// - `s_cov`, `s_chol`: `>= m * m`
/// - `i_kh`, `tmp_nn`: `>= n * n`
pub struct EkfWorkspace {
    z_pred: []f64,
    innov: []f64,
    ph_t: []f64,
    s_cov: []f64,
    s_chol: []f64,
    k_gain: []f64,
    i_kh: []f64,
    tmp_nn: []f64,
    tmp_nm: []f64,
    tmp_m: []f64,
}

/// Merwe-scaled Unscented Transform parameters (`alpha`, `beta`, `kappa`).
pub struct UkfParams {
    alpha: f64,
    beta: f64,
    kappa: f64,

    /// Standard Merwe defaults (`alpha = 0.001, beta = 2.0, kappa = 0.0`).
    static fn default(): UkfParams {
        return UkfParams {
            alpha: 0.001,
            beta: 2.0,
            kappa: 0.0,
        };
    }
}

fn isFiniteF64(v: f64): bool {
    const diff = v - v;
    return diff == 0.0;
}

fn allFinite(s: []const f64, len: usize): bool {
    let i: usize = 0;
    while (i < len) {
        if (!isFiniteF64(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn symmetrizeMatrix(p: *[]f64, n: usize) {
    let i: usize = 0;
    while (i < n) {
        let j: usize = i + 1;
        while (j < n) {
            const avg = 0.5 * (p.*[i * n + j] + p.*[j * n + i]);
            p.*[i * n + j] = avg;
            p.*[j * n + i] = avg;
            j += 1;
        }
        i += 1;
    }
}

/// Computes lower-triangular Cholesky factor `L` (`A = L * L^T`) of symmetric positive-definite `n x n` matrix `a`.
/// Returns `false` if `a` is not positive definite or contains non-finite entries.
fn choleskyLower(a: []const f64, n: usize, l_out: *[]f64): bool {
    if (n == 0 || a.len < n * n || l_out.*.len < n * n) {
        return false;
    }
    let idx: usize = 0;
    while (idx < n * n) {
        l_out.*[idx] = 0.0;
        idx += 1;
    }

    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j <= i) {
            let sum = 0.5 * (a[i * n + j] + a[j * n + i]);
            if (!isFiniteF64(sum)) {
                return false;
            }
            let k: usize = 0;
            while (k < j) {
                sum -= l_out.*[i * n + k] * l_out.*[j * n + k];
                k += 1;
            }
            if (i == j) {
                if (sum <= 0.0 || !isFiniteF64(sum)) {
                    return false;
                }
                l_out.*[i * n + j] = @sqrt(sum);
            } else {
                const diag = l_out.*[j * n + j];
                if (diag <= 0.0) {
                    return false;
                }
                l_out.*[i * n + j] = sum / diag;
            }
            j += 1;
        }
        i += 1;
    }
    return true;
}

/// Solves `(L * L^T) * x = b` in-place via forward & backward substitution into `x_out[0..m]`.
fn choleskySolve(l: []const f64, m: usize, b: []const f64, x_out: *[]f64) {
    // Forward substitution: L * y = b
    let i: usize = 0;
    while (i < m) {
        let sum = b[i];
        let k: usize = 0;
        while (k < i) {
            sum -= l[i * m + k] * x_out.*[k];
            k += 1;
        }
        x_out.*[i] = sum / l[i * m + i];
        i += 1;
    }

    // Backward substitution: L^T * x = y
    let row: usize = m;
    while (row > 0) {
        row -= 1;
        let sum = x_out.*[row];
        let k: usize = row + 1;
        while (k < m) {
            sum -= l[k * m + row] * x_out.*[k];
            k += 1;
        }
        x_out.*[row] = sum / l[row * m + row];
    }
}

/// Computes central finite-difference state transition Jacobian `F = df/dx` (`n x n` row-major).
/// Returns `false` on invalid dimensions, `eps <= 0`, undersized buffers, or non-finite values.
pub fn numericalJacobianF(
    f: TransitionFn,
    x: []const f64,
    u: []const f64,
    dt: f64,
    eps: f64,
    x_pert_s: *[]f64,
    f_plus_s: *[]f64,
    f_minus_s: *[]f64,
    j_out: *[]f64,
): bool {
    const n = x.len;
    if (
        n == 0 ||
        !isFiniteF64(dt) ||
        !isFiniteF64(eps) || eps <= 0.0 ||
        x_pert_s.*.len < n ||
        f_plus_s.*.len < n ||
        f_minus_s.*.len < n ||
        j_out.*.len < n * n
    ) {
        return false;
    }
    if (!allFinite(x, n) || !allFinite(u, u.len)) {
        return false;
    }

    let i: usize = 0;
    while (i < n) {
        x_pert_s.*[i] = x[i];
        i += 1;
    }

    const inv_2eps = 0.5 / eps;
    let col: usize = 0;
    while (col < n) {
        const orig = x[col];
        x_pert_s.*[col] = orig + eps;
        f(x_pert_s.*[0..n], u, dt, f_plus_s);
        x_pert_s.*[col] = orig - eps;
        f(x_pert_s.*[0..n], u, dt, f_minus_s);
        x_pert_s.*[col] = orig;

        if (!allFinite(f_plus_s.*[0..n], n) || !allFinite(f_minus_s.*[0..n], n)) {
            return false;
        }
        let row: usize = 0;
        while (row < n) {
            j_out.*[row * n + col] = (f_plus_s.*[row] - f_minus_s.*[row]) * inv_2eps;
            row += 1;
        }
        col += 1;
    }

    return true;
}

/// Computes central finite-difference measurement Jacobian `H = dh/dx` (`m x n` row-major).
/// Returns `false` on invalid dimensions, `eps <= 0`, undersized buffers, or non-finite values.
pub fn numericalJacobianH(
    h: MeasurementFn,
    x: []const f64,
    m: usize,
    eps: f64,
    x_pert_s: *[]f64,
    h_plus_s: *[]f64,
    h_minus_s: *[]f64,
    j_out: *[]f64,
): bool {
    const n = x.len;
    if (
        n == 0 || m == 0 ||
        !isFiniteF64(eps) || eps <= 0.0 ||
        x_pert_s.*.len < n ||
        h_plus_s.*.len < m ||
        h_minus_s.*.len < m ||
        j_out.*.len < m * n
    ) {
        return false;
    }
    if (!allFinite(x, n)) {
        return false;
    }

    let i: usize = 0;
    while (i < n) {
        x_pert_s.*[i] = x[i];
        i += 1;
    }

    const inv_2eps = 0.5 / eps;
    let col: usize = 0;
    while (col < n) {
        const orig = x[col];
        x_pert_s.*[col] = orig + eps;
        h(x_pert_s.*[0..n], h_plus_s);
        x_pert_s.*[col] = orig - eps;
        h(x_pert_s.*[0..n], h_minus_s);
        x_pert_s.*[col] = orig;

        if (!allFinite(h_plus_s.*[0..m], m) || !allFinite(h_minus_s.*[0..m], m)) {
            return false;
        }
        let row: usize = 0;
        while (row < m) {
            j_out.*[row * n + col] = (h_plus_s.*[row] - h_minus_s.*[row]) * inv_2eps;
            row += 1;
        }
        col += 1;
    }

    return true;
}

/// EKF Predict step: `x = f(x, u, dt)`, `P = F * P * F^T + Q`, with symmetry enforcement.
/// Returns `false` on invalid dimensions, undersized buffers, or non-finite inputs.
pub fn ekfPredict(
    x_inout: *[]f64,
    p_inout: *[]f64,
    n: usize,
    f_fn: TransitionFn,
    u: []const f64,
    dt: f64,
    f_jac: []const f64,
    q_cov: []const f64,
    tmp_nn: *[]f64,
    x_next_s: *[]f64,
): bool {
    if (
        n == 0 ||
        !isFiniteF64(dt) ||
        x_inout.*.len < n ||
        p_inout.*.len < n * n ||
        f_jac.len < n * n ||
        q_cov.len < n * n ||
        tmp_nn.*.len < n * n ||
        x_next_s.*.len < n
    ) {
        return false;
    }
    if (
        !allFinite(x_inout.*[0..n], n) ||
        !allFinite(p_inout.*[0..n * n], n * n) ||
        !allFinite(f_jac[0..n * n], n * n) ||
        !allFinite(q_cov[0..n * n], n * n)
    ) {
        return false;
    }

    // State propagation: x_next = f(x, u, dt)
    f_fn(x_inout.*[0..n], u, dt, x_next_s);
    if (!allFinite(x_next_s.*[0..n], n)) {
        return false;
    }

    let i: usize = 0;
    while (i < n) {
        x_inout.*[i] = x_next_s.*[i];
        i += 1;
    }

    // tmp_nn = F * P
    i = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                sum += f_jac[i * n + k] * p_inout.*[k * n + j];
                k += 1;
            }
            tmp_nn.*[i * n + j] = sum;
            j += 1;
        }
        i += 1;
    }

    // P = tmp_nn * F^T + Q
    i = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let sum = q_cov[i * n + j];
            let k: usize = 0;
            while (k < n) {
                sum += tmp_nn.*[i * n + k] * f_jac[j * n + k];
                k += 1;
            }
            p_inout.*[i * n + j] = sum;
            j += 1;
        }
        i += 1;
    }

    symmetrizeMatrix(p_inout, n);
    return true;
}

/// EKF Update step using Joseph stabilized covariance update:
/// `P = (I - K H) P (I - K H)^T + K R K^T`.
/// Returns the Normalized Innovation Squared (NIS = `y^T S^-1 y`), or `null` on singular/non-PD `S` or invalid inputs.
pub fn ekfUpdate(
    x_inout: *[]f64,
    p_inout: *[]f64,
    n: usize,
    m: usize,
    h_fn: MeasurementFn,
    z: []const f64,
    h_jac: []const f64,
    r_cov: []const f64,
    ws: *EkfWorkspace,
): f64? {
    if (
        n == 0 || m == 0 ||
        x_inout.*.len < n ||
        p_inout.*.len < n * n ||
        z.len < m ||
        h_jac.len < m * n ||
        r_cov.len < m * m ||
        ws.z_pred.len < m ||
        ws.innov.len < m ||
        ws.ph_t.len < n * m ||
        ws.s_cov.len < m * m ||
        ws.s_chol.len < m * m ||
        ws.k_gain.len < n * m ||
        ws.i_kh.len < n * n ||
        ws.tmp_nn.len < n * n ||
        ws.tmp_nm.len < n * m ||
        ws.tmp_m.len < m
    ) {
        return null;
    }
    if (
        !allFinite(x_inout.*[0..n], n) ||
        !allFinite(p_inout.*[0..n * n], n * n) ||
        !allFinite(z[0..m], m) ||
        !allFinite(h_jac[0..m * n], m * n) ||
        !allFinite(r_cov[0..m * m], m * m)
    ) {
        return null;
    }

    // 1. Innovation: y = z - h(x)
    let zp_s: []f64 = ws.z_pred;
    h_fn(x_inout.*[0..n], &zp_s);
    if (!allFinite(zp_s[0..m], m)) {
        return null;
    }
    let i: usize = 0;
    while (i < m) {
        ws.innov[i] = z[i] - zp_s[i];
        i += 1;
    }

    // 2. PH^T (n x m)
    i = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < m) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                sum += p_inout.*[i * n + k] * h_jac[j * n + k];
                k += 1;
            }
            ws.ph_t[i * m + j] = sum;
            j += 1;
        }
        i += 1;
    }

    // 3. S = H * (PH^T) + R (m x m)
    i = 0;
    while (i < m) {
        let j: usize = 0;
        while (j < m) {
            let sum = r_cov[i * m + j];
            let k: usize = 0;
            while (k < n) {
                sum += h_jac[i * n + k] * ws.ph_t[k * m + j];
                k += 1;
            }
            ws.s_cov[i * m + j] = sum;
            j += 1;
        }
        i += 1;
    }

    // 4. Cholesky factor S = L * L^T (rejects singular or non-positive-definite S)
    let sc_s: []f64 = ws.s_chol;
    if (!choleskyLower(ws.s_cov[0..m * m], m, &sc_s)) {
        return null;
    }

    // 5. NIS = y^T * S^-1 * y
    let tm_s: []f64 = ws.tmp_m;
    choleskySolve(sc_s[0..m * m], m, ws.innov[0..m], &tm_s);
    let nis: f64 = 0.0;
    i = 0;
    while (i < m) {
        nis += ws.innov[i] * tm_s[i];
        i += 1;
    }

    // 6. Kalman gain K = (PH^T) * S^-1 row-by-row: solve S * K_row^T = (PH^T)_row^T
    i = 0;
    while (i < n) {
        const row_off = i * m;
        choleskySolve(sc_s[0..m * m], m, ws.ph_t[row_off .. row_off + m], &tm_s);
        let j: usize = 0;
        while (j < m) {
            ws.k_gain[row_off + j] = tm_s[j];
            j += 1;
        }
        i += 1;
    }

    // 7. State update: x = x + K * y
    i = 0;
    while (i < n) {
        let dx: f64 = 0.0;
        let j: usize = 0;
        while (j < m) {
            dx += ws.k_gain[i * m + j] * ws.innov[j];
            j += 1;
        }
        x_inout.*[i] += dx;
        i += 1;
    }

    // 8. Joseph covariance update: P = (I - K H) P (I - K H)^T + K R K^T
    // i_kh = I - K * H (n x n)
    i = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let prod: f64 = 0.0;
            let k: usize = 0;
            while (k < m) {
                prod += ws.k_gain[i * m + k] * h_jac[k * n + j];
                k += 1;
            }
            ws.i_kh[i * n + j] = (if (i == j) 1.0 else 0.0) - prod;
            j += 1;
        }
        i += 1;
    }

    // tmp_nn = i_kh * P
    i = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                sum += ws.i_kh[i * n + k] * p_inout.*[k * n + j];
                k += 1;
            }
            ws.tmp_nn[i * n + j] = sum;
            j += 1;
        }
        i += 1;
    }

    // tmp_nm = K * R
    i = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < m) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < m) {
                sum += ws.k_gain[i * m + k] * r_cov[k * m + j];
                k += 1;
            }
            ws.tmp_nm[i * m + j] = sum;
            j += 1;
        }
        i += 1;
    }

    // P = tmp_nn * i_kh^T + tmp_nm * K^T
    i = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let sum: f64 = 0.0;
            let k: usize = 0;
            while (k < n) {
                sum += ws.tmp_nn[i * n + k] * ws.i_kh[j * n + k];
                k += 1;
            }
            let km: usize = 0;
            while (km < m) {
                sum += ws.tmp_nm[i * m + km] * ws.k_gain[j * m + km];
                km += 1;
            }
            p_inout.*[i * n + j] = sum;
            j += 1;
        }
        i += 1;
    }

    symmetrizeMatrix(p_inout, n);
    return nis;
}

/// Generates `2*n + 1` Merwe-scaled UKF sigma points and mean/covariance weights (`wm_out`, `wc_out`).
/// Returns `false` if `p` is not positive definite, params are invalid, or buffers are undersized.
pub fn generateSigmaPoints(
    x: []const f64,
    p: []const f64,
    n: usize,
    params: UkfParams,
    sigma_out: *[]f64,
    wm_out: *[]f64,
    wc_out: *[]f64,
    chol_scratch: *[]f64,
): bool {
    const n_sig = 2 * n + 1;
    if (
        n == 0 ||
        x.len < n ||
        p.len < n * n ||
        sigma_out.*.len < n_sig * n ||
        wm_out.*.len < n_sig ||
        wc_out.*.len < n_sig ||
        chol_scratch.*.len < n * n
    ) {
        return false;
    }
    if (
        !isFiniteF64(params.alpha) || params.alpha <= 0.0 ||
        !isFiniteF64(params.beta) ||
        !isFiniteF64(params.kappa) ||
        !allFinite(x, n)
    ) {
        return false;
    }

    const nf = n as f64;
    const lambda = params.alpha * params.alpha * (nf + params.kappa) - nf;
    const denom = nf + lambda;
    if (denom <= 0.0 || !isFiniteF64(denom)) {
        return false;
    }

    if (!choleskyLower(p, n, chol_scratch)) {
        return false;
    }

    const gamma = @sqrt(denom);

    // Sigma point 0 = x
    let i: usize = 0;
    while (i < n) {
        sigma_out.*[i] = x[i];
        i += 1;
    }

    // Sigma points 1..n (x + gamma * col_k(L)) and n+1..2n (x - gamma * col_k(L))
    let k: usize = 0;
    while (k < n) {
        const pos_off = (1 + k) * n;
        const neg_off = (1 + n + k) * n;
        i = 0;
        while (i < n) {
            const delta = gamma * chol_scratch.*[i * n + k];
            sigma_out.*[pos_off + i] = x[i] + delta;
            sigma_out.*[neg_off + i] = x[i] - delta;
            i += 1;
        }
        k += 1;
    }

    // Weights
    const w0_m = lambda / denom;
    const w0_c = w0_m + (1.0 - params.alpha * params.alpha + params.beta);
    const wi = 0.5 / denom;

    wm_out.*[0] = w0_m;
    wc_out.*[0] = w0_c;
    let s_idx: usize = 1;
    while (s_idx < n_sig) {
        wm_out.*[s_idx] = wi;
        wc_out.*[s_idx] = wi;
        s_idx += 1;
    }

    return true;
}

/// UKF Predict step: generates `2*n + 1` Merwe sigma points, propagates each through `f_fn`,
/// and reconstructs predicted mean `x_inout` and symmetric covariance `p_inout = Σ wc_k (y_k - x)(y_k - x)^T + Q`.
/// Returns `false` if `p_inout` is not positive definite, buffers are undersized, or non-finite values occur.
pub fn ukfPredict(
    x_inout: *[]f64,
    p_inout: *[]f64,
    n: usize,
    f_fn: TransitionFn,
    u: []const f64,
    dt: f64,
    q_cov: []const f64,
    params: UkfParams,
    sigma_in: *[]f64,
    sigma_out: *[]f64,
    wm_s: *[]f64,
    wc_s: *[]f64,
    chol_s: *[]f64,
): bool {
    const n_sig = 2 * n + 1;
    if (
        n == 0 ||
        !isFiniteF64(dt) ||
        x_inout.*.len < n ||
        p_inout.*.len < n * n ||
        q_cov.len < n * n ||
        sigma_in.*.len < n_sig * n ||
        sigma_out.*.len < n_sig * n ||
        wm_s.*.len < n_sig ||
        wc_s.*.len < n_sig ||
        chol_s.*.len < n * n
    ) {
        return false;
    }
    if (!allFinite(q_cov[0..n * n], n * n) || !allFinite(u, u.len)) {
        return false;
    }

    if (!generateSigmaPoints(x_inout.*[0..n], p_inout.*[0..n * n], n, params, sigma_in, wm_s, wc_s, chol_s)) {
        return false;
    }

    // Propagate each sigma point through nonlinear f_fn
    let k: usize = 0;
    while (k < n_sig) {
        const off = k * n;
        let out_slice: []f64 = sigma_out.*[off .. off + n];
        f_fn(sigma_in.*[off .. off + n], u, dt, &out_slice);
        if (!allFinite(out_slice, n)) {
            return false;
        }
        k += 1;
    }

    // Reconstruct predicted mean
    let i: usize = 0;
    while (i < n) {
        let mean_i: f64 = 0.0;
        k = 0;
        while (k < n_sig) {
            mean_i += wm_s.*[k] * sigma_out.*[k * n + i];
            k += 1;
        }
        x_inout.*[i] = mean_i;
        i += 1;
    }

    // Reconstruct predicted covariance + Q
    i = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let cov_ij = q_cov[i * n + j];
            k = 0;
            while (k < n_sig) {
                const di = sigma_out.*[k * n + i] - x_inout.*[i];
                const dj = sigma_out.*[k * n + j] - x_inout.*[j];
                cov_ij += wc_s.*[k] * di * dj;
                k += 1;
            }
            p_inout.*[i * n + j] = cov_ij;
            j += 1;
        }
        i += 1;
    }

    symmetrizeMatrix(p_inout, n);
    return true;
}
