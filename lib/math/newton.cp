// Origin: Pure-CP Stdlib (Zero-Alloc Damped Multidimensional Newton-Raphson & Levenberg-Marquardt Nonlinear Least Squares) | Created: 2026-11-22

const PIVOT_EPS: f64 = 1.0e-14;
const ARMIJO_C1: f64 = 1.0e-4;
const LM_MAX_INNER: usize = 24;
const LM_LAMBDA_MAX: f64 = 1.0e16;
const LM_LAMBDA_MIN: f64 = 1.0e-15;

/// Vector residual callback `F(x) -> f_out` (for both square systems and least-squares residuals).
pub type ResidualFn = fn(x: []const f64, f_out: *[]f64);

/// Scalar 1D function callback `f(x) -> f64`.
pub type ScalarFn = fn(x: f64): f64;

/// Configuration for damped multidimensional Newton-Raphson solver (`F(x) = 0`).
pub struct NewtonConfig {
    tol_f: f64,
    tol_x: f64,
    fd_eps: f64,
    max_iter: usize,
    max_backtrack: usize,

    static fn default(): NewtonConfig {
        return {
            tol_f: 1.0e-12,
            tol_x: 1.0e-12,
            fd_eps: 1.0e-7,
            max_iter: 100,
            max_backtrack: 20,
        };
    }
}

/// Result summary returned by `newtonSystem`.
pub struct NewtonResult {
    converged: bool,
    residual_norm: f64,
    iterations: usize,
    evals: usize,
}

/// Caller-provided zero-allocation workspace for `newtonSystem` (`n` unknowns).
/// Required lengths: `fx >= n`, `fx_trial >= n`, `jac >= n * n`, `dx >= n`, `x_trial >= n`, `piv >= n`.
pub struct NewtonWorkspace {
    fx: []f64,
    fx_trial: []f64,
    jac: []f64,
    dx: []f64,
    x_trial: []f64,
    piv: []usize,
}

/// Configuration for Levenberg-Marquardt nonlinear least-squares solver (`min 0.5 * ||r(x)||_2^2`).
pub struct LmConfig {
    tol_r: f64,
    tol_x: f64,
    fd_eps: f64,
    lambda_init: f64,
    max_iter: usize,

    static fn default(): LmConfig {
        return {
            tol_r: 1.0e-12,
            tol_x: 1.0e-12,
            fd_eps: 1.0e-7,
            lambda_init: 1.0e-3,
            max_iter: 200,
        };
    }
}

/// Result summary returned by `levenbergMarquardt`.
pub struct LmResult {
    converged: bool,
    cost: f64,
    residual_norm: f64,
    iterations: usize,
    evals: usize,
}

/// Caller-provided zero-allocation workspace for `levenbergMarquardt` (`m` residuals, `n` parameters).
/// Required lengths: `r >= m`, `r_trial >= m`, `jac >= m * n`, `jtj >= n * n`, `jtr >= n`, `dx >= n`, `x_trial >= n`, `piv >= n`.
pub struct LmWorkspace {
    r: []f64,
    r_trial: []f64,
    jac: []f64,
    jtj: []f64,
    jtr: []f64,
    dx: []f64,
    x_trial: []f64,
    piv: []usize,
}

/// Result summary returned by 1D root finders `newton1D` and `secant1D`.
pub struct RootResult {
    root: f64,
    fval: f64,
    iterations: usize,
    converged: bool,
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn isAllFinite(xs: []const f64, len: usize): bool {
    let i: usize = 0;
    while (i < len) {
        if (!isFinite(xs[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn l2Norm(xs: []const f64, len: usize): f64 {
    let sq: f64 = 0.0;
    let i: usize = 0;
    while (i < len) {
        sq += xs[i] * xs[i];
        i += 1;
    }
    return @sqrt(sq);
}

// In-place LU factorization with partial pivoting using row permutation `piv[0..n]`.
// Solves `A * x_out = rhs` where `A` is `n x n` row-major (overwritten with LU factors).
fn solveLinearSystem(
    a: []f64,
    rhs: []const f64,
    x_out: []f64,
    piv: []usize,
    n: usize,
): bool {
    let i_init: usize = 0;
    while (i_init < n) {
        piv[i_init] = i_init;
        i_init += 1;
    }

    let col: usize = 0;
    while (col < n) {
        let best_row = col;
        let best_abs = absF64(a[piv[col] * n + col]);
        let r_scan = col + 1;
        while (r_scan < n) {
            const v_abs = absF64(a[piv[r_scan] * n + col]);
            if (v_abs > best_abs) {
                best_abs = v_abs;
                best_row = r_scan;
            }
            r_scan += 1;
        }

        if (!isFinite(best_abs) || best_abs < PIVOT_EPS) {
            return false;
        }

        if (best_row != col) {
            const tmp = piv[col];
            piv[col] = piv[best_row];
            piv[best_row] = tmp;
        }

        const piv_row = piv[col];
        const diag = a[piv_row * n + col];
        let row = col + 1;
        while (row < n) {
            const r_idx = piv[row];
            const factor = a[r_idx * n + col] / diag;
            a[r_idx * n + col] = factor;
            let j = col + 1;
            while (j < n) {
                a[r_idx * n + j] -= factor * a[piv_row * n + j];
                j += 1;
            }
            row += 1;
        }
        col += 1;
    }

    // Forward substitution: L * y = P * rhs (stored in x_out)
    let i: usize = 0;
    while (i < n) {
        const r_idx = piv[i];
        let sum = rhs[r_idx];
        let j: usize = 0;
        while (j < i) {
            sum -= a[r_idx * n + j] * x_out[j];
            j += 1;
        }
        x_out[i] = sum;
        i += 1;
    }

    // Backward substitution: U * x = y
    let back: usize = n;
    while (back > 0) {
        back -= 1;
        const r_idx = piv[back];
        let sum = x_out[back];
        let j = back + 1;
        while (j < n) {
            sum -= a[r_idx * n + j] * x_out[j];
            j += 1;
        }
        const diag = a[r_idx * n + back];
        if (!isFinite(diag) || absF64(diag) < PIVOT_EPS) {
            return false;
        }
        const val = sum / diag;
        if (!isFinite(val)) {
            return false;
        }
        x_out[back] = val;
    }
    return true;
}

// Computes central-difference Jacobian `jac` (`m x n`, row-major) using only `x_trial` & `f_scratch` (zero extra alloc).
fn computeJacobianCentral(
    f: ResidualFn,
    x: []const f64,
    m: usize,
    n: usize,
    fd_eps: f64,
    x_trial: []f64,
    f_scratch: []f64,
    jac: []f64,
    evals: *usize,
): bool {
    let c_copy: usize = 0;
    while (c_copy < n) {
        x_trial[c_copy] = x[c_copy];
        c_copy += 1;
    }

    let f_view: []f64 = f_scratch[0..m];
    let j: usize = 0;
    while (j < n) {
        const xj = x[j];
        const scale = if (absF64(xj) > 1.0) absF64(xj) else 1.0;
        const h = fd_eps * scale;

        // Forward evaluation: store f(x + h e_j) in column j of jac
        x_trial[j] = xj + h;
        f(x_trial[0..n], *f_view);
        evals.* += 1;
        if (!isAllFinite(f_view, m)) {
            return false;
        }
        let i_pos: usize = 0;
        while (i_pos < m) {
            jac[i_pos * n + j] = f_view[i_pos];
            i_pos += 1;
        }

        // Backward evaluation: (f(x + h e_j) - f(x - h e_j)) / (2 h)
        x_trial[j] = xj - h;
        f(x_trial[0..n], *f_view);
        evals.* += 1;
        if (!isAllFinite(f_view, m)) {
            return false;
        }
        const inv_2h = 0.5 / h;
        let i_neg: usize = 0;
        while (i_neg < m) {
            const deriv = (jac[i_neg * n + j] - f_view[i_neg]) * inv_2h;
            if (!isFinite(deriv)) {
                return false;
            }
            jac[i_neg * n + j] = deriv;
            i_neg += 1;
        }

        x_trial[j] = xj;
        j += 1;
    }
    return true;
}

/// Damped multidimensional Newton-Raphson solver for square nonlinear systems `F(x) = 0`.
/// Uses central-difference Jacobian, partial-pivoting LU linear solver, and Armijo backtracking damping.
/// Returns `null` on invalid dimensions/config, short workspace buffers, non-finite evaluations, or singular Jacobian.
pub fn newtonSystem(
    f: ResidualFn,
    x0: []const f64,
    x_out: *[]f64,
    cfg: NewtonConfig,
    ws: *NewtonWorkspace,
): NewtonResult? {
    const n = x0.len;
    if (n == 0 || x_out.*.len < n) {
        return null;
    }
    if (!isFinite(cfg.tol_f) || cfg.tol_f < 0.0 || !isFinite(cfg.tol_x) || cfg.tol_x < 0.0) {
        return null;
    }
    if (!isFinite(cfg.fd_eps) || cfg.fd_eps <= 0.0 || cfg.max_iter == 0) {
        return null;
    }
    if (ws.*.fx.len < n || ws.*.fx_trial.len < n || ws.*.jac.len / n < n) {
        return null;
    }
    if (ws.*.dx.len < n || ws.*.x_trial.len < n || ws.*.piv.len < n) {
        return null;
    }
    if (!isAllFinite(x0, n)) {
        return null;
    }

    let i_init: usize = 0;
    while (i_init < n) {
        x_out.*[i_init] = x0[i_init];
        i_init += 1;
    }

    let fx_view: []f64 = ws.*.fx[0..n];
    let fx_trial_view: []f64 = ws.*.fx_trial[0..n];
    let evals: usize = 0;

    f(x_out.*[0..n], *fx_view);
    evals += 1;
    if (!isAllFinite(fx_view, n)) {
        return null;
    }
    let norm_f = l2Norm(fx_view, n);
    let iters: usize = 0;
    let converged = false;

    while (iters < cfg.max_iter) {
        if (norm_f <= cfg.tol_f) {
            converged = true;
            break;
        }

        if (!computeJacobianCentral(
            f,
            x_out.*[0..n],
            n,
            n,
            cfg.fd_eps,
            ws.*.x_trial,
            ws.*.fx_trial,
            ws.*.jac,
            *evals,
        )) {
            return null;
        }

        // Reuse x_trial temporarily as rhs = -fx
        let k_rhs: usize = 0;
        while (k_rhs < n) {
            ws.*.x_trial[k_rhs] = 0.0 - fx_view[k_rhs];
            k_rhs += 1;
        }

        if (!solveLinearSystem(ws.*.jac, ws.*.x_trial[0..n], ws.*.dx, ws.*.piv, n)) {
            return null;
        }

        // Armijo backtracking line search along Newton direction dx
        let alpha: f64 = 1.0;
        let accepted = false;
        let trial_norm: f64 = norm_f;
        let bt: usize = 0;
        while (bt <= cfg.max_backtrack) {
            let j: usize = 0;
            while (j < n) {
                ws.*.x_trial[j] = x_out.*[j] + alpha * ws.*.dx[j];
                j += 1;
            }
            f(ws.*.x_trial[0..n], *fx_trial_view);
            evals += 1;
            if (isAllFinite(fx_trial_view, n)) {
                trial_norm = l2Norm(fx_trial_view, n);
                if (trial_norm <= (1.0 - ARMIJO_C1 * alpha) * norm_f || trial_norm <= cfg.tol_f) {
                    accepted = true;
                    break;
                }
            }
            alpha *= 0.5;
            bt += 1;
        }

        if (!accepted) {
            break;
        }

        let u: usize = 0;
        while (u < n) {
            x_out.*[u] = ws.*.x_trial[u];
            fx_view[u] = fx_trial_view[u];
            u += 1;
        }
        norm_f = trial_norm;
        iters += 1;

        const step_norm = alpha * l2Norm(ws.*.dx, n);
        const x_norm = l2Norm(x_out.*, n);
        if (norm_f <= cfg.tol_f || (step_norm <= cfg.tol_x * (1.0 + x_norm) && norm_f <= cfg.tol_f * 1.0e4)) {
            converged = true;
            break;
        }
    }

    if (norm_f <= cfg.tol_f) {
        converged = true;
    }

    return {
        converged: converged,
        residual_norm: norm_f,
        iterations: iters,
        evals: evals,
    };
}

/// Zero-allocation Levenberg-Marquardt nonlinear least-squares solver (`min 0.5 * ||r(x)||_2^2`, `m >= n`).
/// Solves `(J^T J + lambda * diag(J^T J)) dx = -J^T r` with adaptive Marquardt damping.
/// Returns `null` on invalid dimensions (`n == 0` or `m < n`), bad config, short workspace buffers, or non-finite values.
pub fn levenbergMarquardt(
    r_fn: ResidualFn,
    m: usize,
    x0: []const f64,
    x_out: *[]f64,
    cfg: LmConfig,
    ws: *LmWorkspace,
): LmResult? {
    const n = x0.len;
    if (n == 0 || m < n || x_out.*.len < n) {
        return null;
    }
    if (!isFinite(cfg.tol_r) || cfg.tol_r < 0.0 || !isFinite(cfg.tol_x) || cfg.tol_x < 0.0) {
        return null;
    }
    if (!isFinite(cfg.fd_eps) || cfg.fd_eps <= 0.0 || !isFinite(cfg.lambda_init) || cfg.lambda_init <= 0.0 || cfg.max_iter == 0) {
        return null;
    }
    if (ws.*.r.len < m || ws.*.r_trial.len < m || ws.*.jac.len / m < n) {
        return null;
    }
    if (ws.*.jtj.len / n < n || ws.*.jtr.len < n || ws.*.dx.len < n || ws.*.x_trial.len < n || ws.*.piv.len < n) {
        return null;
    }
    if (!isAllFinite(x0, n)) {
        return null;
    }

    let i_init: usize = 0;
    while (i_init < n) {
        x_out.*[i_init] = x0[i_init];
        i_init += 1;
    }

    let r_view: []f64 = ws.*.r[0..m];
    let r_trial_view: []f64 = ws.*.r_trial[0..m];
    let evals: usize = 0;

    r_fn(x_out.*[0..n], *r_view);
    evals += 1;
    if (!isAllFinite(r_view, m)) {
        return null;
    }

    let norm_r = l2Norm(r_view, m);
    let cost = 0.5 * norm_r * norm_r;
    let lambda = cfg.lambda_init;
    let iters: usize = 0;
    let converged = false;

    while (iters < cfg.max_iter) {
        if (norm_r <= cfg.tol_r) {
            converged = true;
            break;
        }

        if (!computeJacobianCentral(
            r_fn,
            x_out.*[0..n],
            m,
            n,
            cfg.fd_eps,
            ws.*.x_trial,
            ws.*.r_trial,
            ws.*.jac,
            *evals,
        )) {
            return null;
        }

        // Compute rhs = -J^T * r and infinity norm of gradient
        let grad_inf: f64 = 0.0;
        let j_g: usize = 0;
        while (j_g < n) {
            let acc: f64 = 0.0;
            let i_r: usize = 0;
            while (i_r < m) {
                acc += ws.*.jac[i_r * n + j_g] * r_view[i_r];
                i_r += 1;
            }
            ws.*.jtr[j_g] = 0.0 - acc;
            const abs_g = absF64(acc);
            if (abs_g > grad_inf) {
                grad_inf = abs_g;
            }
            j_g += 1;
        }

        if (grad_inf <= cfg.tol_r) {
            converged = true;
            break;
        }

        // Adaptive Marquardt damping loop
        let accepted = false;
        let inner: usize = 0;
        while (inner < LM_MAX_INNER) {
            // Form damped normal matrix: J^T J + lambda * diag(J^T J)
            let a: usize = 0;
            while (a < n) {
                let b: usize = a;
                while (b < n) {
                    let sum: f64 = 0.0;
                    let i_k: usize = 0;
                    while (i_k < m) {
                        const row_off = i_k * n;
                        sum += ws.*.jac[row_off + a] * ws.*.jac[row_off + b];
                        i_k += 1;
                    }
                    ws.*.jtj[a * n + b] = sum;
                    ws.*.jtj[b * n + a] = sum;
                    b += 1;
                }
                const diag_v = ws.*.jtj[a * n + a];
                const scale_d = if (diag_v > 1.0e-6) diag_v else 1.0e-6;
                ws.*.jtj[a * n + a] = diag_v + lambda * scale_d;
                a += 1;
            }

            if (solveLinearSystem(ws.*.jtj, ws.*.jtr[0..n], ws.*.dx, ws.*.piv, n)) {
                let k: usize = 0;
                while (k < n) {
                    ws.*.x_trial[k] = x_out.*[k] + ws.*.dx[k];
                    k += 1;
                }
                r_fn(ws.*.x_trial[0..n], *r_trial_view);
                evals += 1;
                if (isAllFinite(r_trial_view, m)) {
                    const trial_norm = l2Norm(r_trial_view, m);
                    const trial_cost = 0.5 * trial_norm * trial_norm;
                    if (trial_cost < cost) {
                        let u: usize = 0;
                        while (u < n) {
                            x_out.*[u] = ws.*.x_trial[u];
                            u += 1;
                        }
                        let ur: usize = 0;
                        while (ur < m) {
                            r_view[ur] = r_trial_view[ur];
                            ur += 1;
                        }
                        norm_r = trial_norm;
                        cost = trial_cost;
                        const shrunk = lambda * 0.3;
                        lambda = if (shrunk > LM_LAMBDA_MIN) shrunk else LM_LAMBDA_MIN;
                        accepted = true;
                        iters += 1;

                        const dx_norm = l2Norm(ws.*.dx, n);
                        const x_norm = l2Norm(x_out.*, n);
                        if (norm_r <= cfg.tol_r || dx_norm <= cfg.tol_x * (1.0 + x_norm)) {
                            converged = true;
                        }
                        break;
                    }
                }
            }

            lambda *= 5.0;
            if (lambda > LM_LAMBDA_MAX) {
                break;
            }
            inner += 1;
        }

        if (converged || !accepted) {
            break;
        }
    }

    if (norm_r <= cfg.tol_r) {
        converged = true;
    }

    return {
        converged: converged,
        cost: cost,
        residual_norm: norm_r,
        iterations: iters,
        evals: evals,
    };
}

/// 1D Newton-Raphson root finder using analytic derivative `df`.
/// Returns `null` on non-finite inputs, invalid `tol`/`max_iter`, or vanishing derivative.
pub fn newton1D(f: ScalarFn, df: ScalarFn, x0: f64, tol: f64, max_iter: usize): RootResult? {
    if (!isFinite(x0) || !isFinite(tol) || tol < 0.0 || max_iter == 0) {
        return null;
    }
    let x = x0;
    let fx = f(x);
    if (!isFinite(fx)) {
        return null;
    }
    let iters: usize = 0;
    while (iters < max_iter) {
        if (absF64(fx) <= tol) {
            return {
                root: x,
                fval: fx,
                iterations: iters,
                converged: true,
            };
        }
        const dfx = df(x);
        if (!isFinite(dfx) || absF64(dfx) < PIVOT_EPS) {
            return null;
        }
        const step = fx / dfx;
        const x_next = x - step;
        if (!isFinite(x_next)) {
            return null;
        }
        const fx_next = f(x_next);
        if (!isFinite(fx_next)) {
            return null;
        }
        x = x_next;
        fx = fx_next;
        iters += 1;
        if (absF64(fx) <= tol || absF64(step) <= tol * (1.0 + absF64(x))) {
            return {
                root: x,
                fval: fx,
                iterations: iters,
                converged: true,
            };
        }
    }
    return {
        root: x,
        fval: fx,
        iterations: iters,
        converged: absF64(fx) <= tol,
    };
}

/// 1D derivative-free secant method root finder starting from `(x0, x1)`.
/// Returns `null` on non-finite inputs, invalid `tol`/`max_iter`, or vanishing secant denominator.
pub fn secant1D(f: ScalarFn, x0: f64, x1: f64, tol: f64, max_iter: usize): RootResult? {
    if (!isFinite(x0) || !isFinite(x1) || !isFinite(tol) || tol < 0.0 || max_iter == 0) {
        return null;
    }
    let a = x0;
    let b = x1;
    let fa = f(a);
    let fb = f(b);
    if (!isFinite(fa) || !isFinite(fb)) {
        return null;
    }
    if (absF64(fa) <= tol) {
        return { root: a, fval: fa, iterations: 0, converged: true };
    }
    if (absF64(fb) <= tol) {
        return { root: b, fval: fb, iterations: 0, converged: true };
    }

    let iters: usize = 0;
    while (iters < max_iter) {
        const denom = fb - fa;
        if (!isFinite(denom) || absF64(denom) < PIVOT_EPS) {
            return null;
        }
        const step = fb * (b - a) / denom;
        const c = b - step;
        if (!isFinite(c)) {
            return null;
        }
        const fc = f(c);
        if (!isFinite(fc)) {
            return null;
        }
        a = b;
        fa = fb;
        b = c;
        fb = fc;
        iters += 1;
        if (absF64(fb) <= tol || absF64(step) <= tol * (1.0 + absF64(b))) {
            return {
                root: b,
                fval: fb,
                iterations: iters,
                converged: true,
            };
        }
    }
    return {
        root: b,
        fval: fb,
        iterations: iters,
        converged: absF64(fb) <= tol,
    };
}
