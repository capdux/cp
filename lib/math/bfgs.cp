// Origin: Pure-CP Stdlib (Zero-Alloc Quasi-Newton Optimization: Full BFGS, Limited-Memory L-BFGS, Strong Wolfe Line Search & Central Finite-Diff Gradient) | Created: 2026-11-22

/// Objective + gradient callback: evaluates `f(x)`, writes gradient into `grad_out[0..x.len]`, and returns `f(x)`.
pub type ValueGradFn = fn(x: []const f64, grad_out: *[]f64): f64;

/// Value-only objective callback used by `finiteDiffGradient`.
pub type ValueOnlyFn = fn(x: []const f64): f64;

/// Optimization hyperparameters and line-search constants for BFGS and L-BFGS.
pub struct BfgsConfig {
    grad_tol: f64,
    step_tol: f64,
    max_iter: usize,
    c1_armijo: f64,
    c2_wolfe: f64,
    max_line_search: usize,

    /// Standard Nocedal & Wright defaults (`grad_tol=1e-8`, `c1=1e-4`, `c2=0.9`).
    fn default(): BfgsConfig {
        return BfgsConfig {
            grad_tol: 1.0e-8,
            step_tol: 1.0e-12,
            max_iter: 500,
            c1_armijo: 1.0e-4,
            c2_wolfe: 0.9,
            max_line_search: 25,
        };
    }
}

/// Summary returned by `bfgsMinimize` and `lbfgsMinimize`.
pub struct BfgsResult {
    converged: bool,
    best_value: f64,
    grad_norm: f64,
    iterations: usize,
    evals: usize,
}

/// Caller-owned zero-alloc workspace for full `bfgsMinimize` on dimension `n`.
/// Required lengths: `h_inv >= n * n`, all vector slices `>= n`.
pub struct BfgsWorkspace {
    h_inv: []f64,
    grad: []f64,
    grad_next: []f64,
    dir: []f64,
    x_next: []f64,
    s: []f64,
    y: []f64,
    hy: []f64,
}

/// Caller-owned zero-alloc workspace for `lbfgsMinimize` on dimension `n` with history size `m`.
/// Required lengths: `s_hist >= m * n`, `y_hist >= m * n`, `rho_hist >= m`, `alpha_hist >= m`,
/// `grad/grad_next/dir/x_next >= n`.
pub struct LbfgsWorkspace {
    s_hist: []f64,
    y_hist: []f64,
    rho_hist: []f64,
    alpha_hist: []f64,
    grad: []f64,
    grad_next: []f64,
    dir: []f64,
    x_next: []f64,
}

struct LineSearchOutcome {
    ok: bool,
    alpha: f64,
    f_val: f64,
    evals: usize,
}

fn isFiniteF64(v: f64): bool {
    const diff = v - v;
    return diff == 0.0;
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn vecAllFinite(v: []const f64, n: usize): bool {
    let i: usize = 0;
    while (i < n) {
        if (!isFiniteF64(v[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn vecDot(a: []const f64, b: []const f64, n: usize): f64 {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        sum += a[i] * b[i];
        i += 1;
    }
    return sum;
}

fn vecNorm2(a: []const f64, n: usize): f64 {
    return @sqrt(vecDot(a, a, n));
}

fn validateConfig(cfg: BfgsConfig): bool {
    return isFiniteF64(cfg.grad_tol) && cfg.grad_tol >= 0.0 &&
        isFiniteF64(cfg.step_tol) && cfg.step_tol >= 0.0 &&
        isFiniteF64(cfg.c1_armijo) && cfg.c1_armijo > 0.0 && cfg.c1_armijo < 1.0 &&
        isFiniteF64(cfg.c2_wolfe) && cfg.c2_wolfe > cfg.c1_armijo && cfg.c2_wolfe < 1.0 &&
        cfg.max_line_search > 0;
}

fn evalAlongRay(
    fg: ValueGradFn,
    x: []const f64,
    dir: []const f64,
    alpha: f64,
    n: usize,
    x_next: *[]f64,
    grad_next: *[]f64,
): f64? {
    let i: usize = 0;
    while (i < n) {
        x_next.*[i] = x[i] + alpha * dir[i];
        i += 1;
    }
    const fv = fg(x_next.*[0..n], grad_next);
    if (!isFiniteF64(fv) || !vecAllFinite(grad_next.*[0..n], n)) {
        return null;
    }
    return fv;
}

/// Nocedal & Wright Algorithm 3.5 / 3.6 strong Wolfe line search with bisection zoom.
fn strongWolfeLineSearch(
    fg: ValueGradFn,
    x: []const f64,
    f0: f64,
    dg0: f64,
    dir: []const f64,
    n: usize,
    cfg: BfgsConfig,
    x_next: *[]f64,
    grad_next: *[]f64,
): LineSearchOutcome {
    const c1 = cfg.c1_armijo;
    const c2 = cfg.c2_wolfe;
    let evals: usize = 0;

    let a_prev: f64 = 0.0;
    let f_prev: f64 = f0;
    let alpha: f64 = 1.0;

    let ls: usize = 0;
    while (ls < cfg.max_line_search) {
        const fv_opt = evalAlongRay(fg, x, dir, alpha, n, x_next, grad_next);
        if (fv_opt == null) {
            // Shrink step on non-finite evaluation
            alpha *= 0.25;
            ls += 1;
            continue;
        }
        const fv = fv_opt ?? f0;
        evals += 1;
        const dg = vecDot(grad_next.*[0..n], dir, n);

        if (fv > f0 + c1 * alpha * dg0 || (ls > 0 && fv >= f_prev)) {
            // Zoom on [a_prev, alpha]
            let z_lo = a_prev;
            let z_hi = alpha;
            let f_lo = f_prev;
            let z_iter: usize = 0;
            let best_a = alpha;
            let best_f = fv;
            while (z_iter < cfg.max_line_search) {
                const a_mid = 0.5 * (z_lo + z_hi);
                const f_mid_opt = evalAlongRay(fg, x, dir, a_mid, n, x_next, grad_next);
                if (f_mid_opt == null) {
                    return LineSearchOutcome { ok: false, alpha: 0.0, f_val: f0, evals: evals };
                }
                const f_mid = f_mid_opt ?? f0;
                evals += 1;
                best_a = a_mid;
                best_f = f_mid;
                const dg_mid = vecDot(grad_next.*[0..n], dir, n);

                if (f_mid > f0 + c1 * a_mid * dg0 || f_mid >= f_lo) {
                    z_hi = a_mid;
                } else {
                    if (absF64(dg_mid) <= -c2 * dg0) {
                        return LineSearchOutcome { ok: true, alpha: a_mid, f_val: f_mid, evals: evals };
                    }
                    if (dg_mid * (z_hi - z_lo) >= 0.0) {
                        z_hi = z_lo;
                    }
                    z_lo = a_mid;
                    f_lo = f_mid;
                }
                if (absF64(z_hi - z_lo) <= 1.0e-15 * (1.0 + absF64(z_lo))) {
                    break;
                }
                z_iter += 1;
            }
            return LineSearchOutcome { ok: true, alpha: best_a, f_val: best_f, evals: evals };
        }

        if (absF64(dg) <= -c2 * dg0) {
            return LineSearchOutcome { ok: true, alpha: alpha, f_val: fv, evals: evals };
        }

        if (dg >= 0.0) {
            // Zoom on [alpha, a_prev]
            let z_lo = alpha;
            let z_hi = a_prev;
            let f_lo = fv;
            let z_iter: usize = 0;
            let best_a = alpha;
            let best_f = fv;
            while (z_iter < cfg.max_line_search) {
                const a_mid = 0.5 * (z_lo + z_hi);
                const f_mid_opt = evalAlongRay(fg, x, dir, a_mid, n, x_next, grad_next);
                if (f_mid_opt == null) {
                    return LineSearchOutcome { ok: false, alpha: 0.0, f_val: f0, evals: evals };
                }
                const f_mid = f_mid_opt ?? f0;
                evals += 1;
                best_a = a_mid;
                best_f = f_mid;
                const dg_mid = vecDot(grad_next.*[0..n], dir, n);

                if (f_mid > f0 + c1 * a_mid * dg0 || f_mid >= f_lo) {
                    z_hi = a_mid;
                } else {
                    if (absF64(dg_mid) <= -c2 * dg0) {
                        return LineSearchOutcome { ok: true, alpha: a_mid, f_val: f_mid, evals: evals };
                    }
                    if (dg_mid * (z_hi - z_lo) >= 0.0) {
                        z_hi = z_lo;
                    }
                    z_lo = a_mid;
                    f_lo = f_mid;
                }
                if (absF64(z_hi - z_lo) <= 1.0e-15 * (1.0 + absF64(z_lo))) {
                    break;
                }
                z_iter += 1;
            }
            return LineSearchOutcome { ok: true, alpha: best_a, f_val: best_f, evals: evals };
        }

        a_prev = alpha;
        f_prev = fv;
        alpha *= 2.0;
        if (alpha > 1.0e6) {
            return LineSearchOutcome { ok: true, alpha: a_prev, f_val: f_prev, evals: evals };
        }
        ls += 1;
    }

    // Fallback: re-evaluate at last accepted step a_prev if non-zero
    if (a_prev > 0.0) {
        const fv_opt = evalAlongRay(fg, x, dir, a_prev, n, x_next, grad_next);
        if (fv_opt == null) {
            return LineSearchOutcome { ok: false, alpha: 0.0, f_val: f0, evals: evals };
        }
        evals += 1;
        return LineSearchOutcome { ok: true, alpha: a_prev, f_val: fv_opt ?? f0, evals: evals };
    }
    return LineSearchOutcome { ok: false, alpha: 0.0, f_val: f0, evals: evals };
}

fn resetIdentity(h_inv: *[]f64, n: usize, scale: f64) {
    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            h_inv.*[i * n + j] = if (i == j) scale else 0.0;
            j += 1;
        }
        i += 1;
    }
}

/// Full BFGS quasi-Newton minimizer maintaining an `n x n` inverse Hessian approximation in `ws.h_inv`.
/// Uses Strong Wolfe line search and curvature guard `s^T y > 0`.
/// Returns `null` on empty `x0`, invalid config, undersized buffers, or non-finite inputs/evaluations.
pub fn bfgsMinimize(
    fg: ValueGradFn,
    x0: []const f64,
    x_out: *[]f64,
    cfg: BfgsConfig,
    ws: *BfgsWorkspace,
): BfgsResult? {
    const n = x0.len;
    if (n == 0 || x_out.*.len < n || !validateConfig(cfg)) {
        return null;
    }
    if (
        ws.h_inv.len < n * n ||
        ws.grad.len < n ||
        ws.grad_next.len < n ||
        ws.dir.len < n ||
        ws.x_next.len < n ||
        ws.s.len < n ||
        ws.y.len < n ||
        ws.hy.len < n
    ) {
        return null;
    }
    if (!vecAllFinite(x0, n)) {
        return null;
    }

    let i: usize = 0;
    while (i < n) {
        x_out.*[i] = x0[i];
        i += 1;
    }

    let h_slice: []f64 = ws.h_inv;
    resetIdentity(&h_slice, n, 1.0);

    let g_slice: []f64 = ws.grad;
    let gn_slice: []f64 = ws.grad_next;
    let xn_slice: []f64 = ws.x_next;

    let f_cur = fg(x_out.*[0..n], &g_slice);
    let evals: usize = 1;
    if (!isFiniteF64(f_cur) || !vecAllFinite(g_slice[0..n], n)) {
        return null;
    }

    let g_norm = vecNorm2(g_slice[0..n], n);
    let iter: usize = 0;
    let converged = g_norm <= cfg.grad_tol;
    let first_update = true;

    while (!converged && iter < cfg.max_iter) {
        // Compute search direction d = -H * grad
        i = 0;
        while (i < n) {
            let sum: f64 = 0.0;
            let j: usize = 0;
            while (j < n) {
                sum += h_slice[i * n + j] * g_slice[j];
                j += 1;
            }
            ws.dir[i] = -sum;
            i += 1;
        }

        let dg = vecDot(g_slice[0..n], ws.dir[0..n], n);
        if (dg >= 0.0) {
            // Reset to steepest descent if numerical drift broke descent property
            resetIdentity(&h_slice, n, 1.0);
            i = 0;
            while (i < n) {
                ws.dir[i] = 0.0 - g_slice[i];
                i += 1;
            }
            dg = vecDot(g_slice[0..n], ws.dir[0..n], n);
        }

        const ls = strongWolfeLineSearch(fg, x_out.*[0..n], f_cur, dg, ws.dir[0..n], n, cfg, &xn_slice, &gn_slice);
        evals += ls.evals;
        if (!ls.ok) {
            break;
        }

        // s = x_next - x, y = grad_next - grad
        i = 0;
        while (i < n) {
            ws.s[i] = xn_slice[i] - x_out.*[i];
            ws.y[i] = gn_slice[i] - g_slice[i];
            x_out.*[i] = xn_slice[i];
            g_slice[i] = gn_slice[i];
            i += 1;
        }

        f_cur = ls.f_val;
        g_norm = vecNorm2(g_slice[0..n], n);
        iter += 1;

        if (g_norm <= cfg.grad_tol) {
            converged = true;
            break;
        }

        const s_norm = vecNorm2(ws.s[0..n], n);
        if (s_norm <= cfg.step_tol) {
            converged = true;
            break;
        }

        const sy = vecDot(ws.s[0..n], ws.y[0..n], n);
        const y_norm = vecNorm2(ws.y[0..n], n);
        if (sy > 1.0e-14 * s_norm * y_norm) {
            if (first_update) {
                const yy = y_norm * y_norm;
                if (yy > 0.0) {
                    resetIdentity(&h_slice, n, sy / yy);
                }
                first_update = false;
            }

            // hy = H * y
            i = 0;
            while (i < n) {
                let sum: f64 = 0.0;
                let j: usize = 0;
                while (j < n) {
                    sum += h_slice[i * n + j] * ws.y[j];
                    j += 1;
                }
                ws.hy[i] = sum;
                i += 1;
            }

            const yhy = vecDot(ws.y[0..n], ws.hy[0..n], n);
            const rho = 1.0 / sy;
            const coeff = (sy + yhy) * rho * rho;

            i = 0;
            while (i < n) {
                const si = ws.s[i];
                const hyi = ws.hy[i];
                let j: usize = 0;
                while (j < n) {
                    h_slice[i * n + j] += coeff * si * ws.s[j] - rho * (hyi * ws.s[j] + si * ws.hy[j]);
                    j += 1;
                }
                i += 1;
            }
        }
    }

    return BfgsResult {
        converged: converged,
        best_value: f_cur,
        grad_norm: g_norm,
        iterations: iter,
        evals: evals,
    };
}

/// Limited-Memory BFGS (L-BFGS) using Two-Loop Recursion with O(m * n) ring-buffer memory.
/// Uses Strong Wolfe line search and curvature guard `s^T y > 0`.
/// Returns `null` on `x0.len == 0`, `m == 0`, invalid config, undersized buffers, or non-finite values.
pub fn lbfgsMinimize(
    fg: ValueGradFn,
    x0: []const f64,
    m: usize,
    x_out: *[]f64,
    cfg: BfgsConfig,
    ws: *LbfgsWorkspace,
): BfgsResult? {
    const n = x0.len;
    if (n == 0 || m == 0 || x_out.*.len < n || !validateConfig(cfg)) {
        return null;
    }
    if (
        ws.s_hist.len < m * n ||
        ws.y_hist.len < m * n ||
        ws.rho_hist.len < m ||
        ws.alpha_hist.len < m ||
        ws.grad.len < n ||
        ws.grad_next.len < n ||
        ws.dir.len < n ||
        ws.x_next.len < n
    ) {
        return null;
    }
    if (!vecAllFinite(x0, n)) {
        return null;
    }

    let i: usize = 0;
    while (i < n) {
        x_out.*[i] = x0[i];
        i += 1;
    }

    let g_slice: []f64 = ws.grad;
    let gn_slice: []f64 = ws.grad_next;
    let xn_slice: []f64 = ws.x_next;

    let f_cur = fg(x_out.*[0..n], &g_slice);
    let evals: usize = 1;
    if (!isFiniteF64(f_cur) || !vecAllFinite(g_slice[0..n], n)) {
        return null;
    }

    let g_norm = vecNorm2(g_slice[0..n], n);
    let iter: usize = 0;
    let converged = g_norm <= cfg.grad_tol;
    let hist_len: usize = 0;
    let hist_head: usize = 0;
    let last_gamma: f64 = 1.0;

    while (!converged && iter < cfg.max_iter) {
        // Two-Loop Recursion: compute q = H_k * grad in ws.dir
        i = 0;
        while (i < n) {
            ws.dir[i] = g_slice[i];
            i += 1;
        }

        // Backward loop from newest to oldest
        let k_back: usize = 0;
        while (k_back < hist_len) {
            const slot = (hist_head + m - 1 - k_back) % m;
            const off = slot * n;
            const dot_sq = vecDot(ws.s_hist[off .. off + n], ws.dir[0..n], n);
            const a_i = ws.rho_hist[slot] * dot_sq;
            ws.alpha_hist[slot] = a_i;
            i = 0;
            while (i < n) {
                ws.dir[i] -= a_i * ws.y_hist[off + i];
                i += 1;
            }
            k_back += 1;
        }

        // Initial Hessian scaling H_k^0 = last_gamma * I
        if (hist_len > 0) {
            i = 0;
            while (i < n) {
                ws.dir[i] *= last_gamma;
                i += 1;
            }
        }

        // Forward loop from oldest to newest
        let k_fwd: usize = 0;
        while (k_fwd < hist_len) {
            const slot = (hist_head + m - hist_len + k_fwd) % m;
            const off = slot * n;
            const dot_yr = vecDot(ws.y_hist[off .. off + n], ws.dir[0..n], n);
            const beta = ws.rho_hist[slot] * dot_yr;
            const diff = ws.alpha_hist[slot] - beta;
            i = 0;
            while (i < n) {
                ws.dir[i] += diff * ws.s_hist[off + i];
                i += 1;
            }
            k_fwd += 1;
        }

        // Negate to form descent direction d = -H_k * grad
        i = 0;
        while (i < n) {
            ws.dir[i] = -ws.dir[i];
            i += 1;
        }

        let dg = vecDot(g_slice[0..n], ws.dir[0..n], n);
        if (dg >= 0.0) {
            hist_len = 0;
            i = 0;
            while (i < n) {
                ws.dir[i] = 0.0 - g_slice[i];
                i += 1;
            }
            dg = vecDot(g_slice[0..n], ws.dir[0..n], n);
        }

        const ls = strongWolfeLineSearch(fg, x_out.*[0..n], f_cur, dg, ws.dir[0..n], n, cfg, &xn_slice, &gn_slice);
        evals += ls.evals;
        if (!ls.ok) {
            break;
        }

        // Record candidate (s, y) into ring slot hist_head
        const slot_off = hist_head * n;
        let sy: f64 = 0.0;
        let ss: f64 = 0.0;
        let yy: f64 = 0.0;
        i = 0;
        while (i < n) {
            const si = xn_slice[i] - x_out.*[i];
            const yi = gn_slice[i] - g_slice[i];
            ws.s_hist[slot_off + i] = si;
            ws.y_hist[slot_off + i] = yi;
            sy += si * yi;
            ss += si * si;
            yy += yi * yi;
            x_out.*[i] = xn_slice[i];
            g_slice[i] = gn_slice[i];
            i += 1;
        }

        f_cur = ls.f_val;
        g_norm = vecNorm2(g_slice[0..n], n);
        iter += 1;

        if (g_norm <= cfg.grad_tol) {
            converged = true;
            break;
        }
        if (@sqrt(ss) <= cfg.step_tol) {
            converged = true;
            break;
        }

        if (sy > 1.0e-14 * @sqrt(ss) * @sqrt(yy) && yy > 0.0) {
            ws.rho_hist[hist_head] = 1.0 / sy;
            last_gamma = sy / yy;
            hist_head = (hist_head + 1) % m;
            if (hist_len < m) {
                hist_len += 1;
            }
        }
    }

    return BfgsResult {
        converged: converged,
        best_value: f_cur,
        grad_norm: g_norm,
        iterations: iter,
        evals: evals,
    };
}

/// Computes the central finite-difference gradient `(f(x + eps*e_i) - f(x - eps*e_i)) / (2*eps)`.
/// Returns `false` on empty `x`, `eps <= 0`, undersized buffers, or non-finite values.
pub fn finiteDiffGradient(
    f: ValueOnlyFn,
    x: []const f64,
    eps: f64,
    x_scratch: *[]f64,
    grad_out: *[]f64,
): bool {
    const n = x.len;
    if (n == 0 || !isFiniteF64(eps) || eps <= 0.0 || x_scratch.*.len < n || grad_out.*.len < n) {
        return false;
    }
    if (!vecAllFinite(x, n)) {
        return false;
    }

    let i: usize = 0;
    while (i < n) {
        x_scratch.*[i] = x[i];
        i += 1;
    }

    const inv_2eps = 0.5 / eps;
    i = 0;
    while (i < n) {
        const orig = x[i];
        x_scratch.*[i] = orig + eps;
        const fp = f(x_scratch.*[0..n]);
        x_scratch.*[i] = orig - eps;
        const fm = f(x_scratch.*[0..n]);
        x_scratch.*[i] = orig;
        if (!isFiniteF64(fp) || !isFiniteF64(fm)) {
            return false;
        }
        grad_out.*[i] = (fp - fm) * inv_2eps;
        i += 1;
    }

    return true;
}
