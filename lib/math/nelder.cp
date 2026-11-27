// Origin: Pure-CP Stdlib (Zero-Alloc Derivative-Free Optimization: Nelder-Mead Simplex, Golden Section 1D Minimization & Brent 1D Root-Finding) | Created: 2026-11-22

/// Objective function type for multi-dimensional Nelder-Mead minimization.
pub type ObjectiveFn = fn([]const f64): f64;

/// Scalar function type for 1D minimization and root-finding.
pub type ScalarFn = fn(f64): f64;

/// Hyperparameters and stopping criteria for `nelderMead`.
pub struct NelderConfig {
    alpha: f64,
    gamma: f64,
    rho: f64,
    sigma: f64,
    tol_f: f64,
    tol_x: f64,
    max_iter: usize,
    init_step: f64,

    /// Standard Nelder-Mead parameters (`alpha=1.0, gamma=2.0, rho=0.5, sigma=0.5`).
    static fn default(): NelderConfig {
        return NelderConfig {
            alpha: 1.0,
            gamma: 2.0,
            rho: 0.5,
            sigma: 0.5,
            tol_f: 1.0e-10,
            tol_x: 1.0e-8,
            max_iter: 2000,
            init_step: 0.05,
        };
    }
}

/// Caller-owned zero-alloc working buffers for `nelderMead` on dimension `n`.
/// Required lengths: `simplex >= (n + 1) * n`, `fvals >= n + 1`, `centroid/xr/xe/xc >= n`.
pub struct NelderWorkspace {
    simplex: []f64,
    fvals: []f64,
    centroid: []f64,
    xr: []f64,
    xe: []f64,
    xc: []f64,
}

/// Summary returned by `nelderMead`.
pub struct NelderResult {
    converged: bool,
    best_value: f64,
    iterations: usize,
    evals: usize,
}

/// Summary returned by `goldenSectionMin`.
pub struct Minimize1DResult {
    x: f64,
    fval: f64,
    iterations: usize,
    converged: bool,
}

/// Summary returned by `brentRoot`.
pub struct Root1DResult {
    root: f64,
    fval: f64,
    iterations: usize,
    converged: bool,
}

fn isFiniteF64(v: f64): bool {
    const diff = v - v;
    return diff == 0.0;
}

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn replaceVertex(ws: *NelderWorkspace, v: usize, n: usize, src: []const f64, fval: f64) {
    const off = v * n;
    let j: usize = 0;
    while (j < n) {
        ws.simplex[off + j] = src[j];
        j += 1;
    }
    ws.fvals[v] = fval;
}

/// Minimizes a multi-dimensional objective function without derivatives via the Nelder-Mead simplex algorithm.
/// Writes the best solution found into `x_out[0..x0.len]`.
/// Returns `null` if `x0` is empty, config/inputs contain invalid or non-finite values, or workspace slices are undersized.
pub fn nelderMead(
    obj: ObjectiveFn,
    x0: []const f64,
    x_out: *[]f64,
    cfg: NelderConfig,
    ws: *NelderWorkspace,
): NelderResult? {
    const n = x0.len;
    if (n == 0 || x_out.*.len < n) {
        return null;
    }
    const nv = n + 1;
    if (
        ws.simplex.len < nv * n ||
        ws.fvals.len < nv ||
        ws.centroid.len < n ||
        ws.xr.len < n ||
        ws.xe.len < n ||
        ws.xc.len < n
    ) {
        return null;
    }
    if (
        !isFiniteF64(cfg.alpha) || cfg.alpha <= 0.0 ||
        !isFiniteF64(cfg.gamma) || cfg.gamma <= 1.0 ||
        !isFiniteF64(cfg.rho) || cfg.rho <= 0.0 || cfg.rho >= 1.0 ||
        !isFiniteF64(cfg.sigma) || cfg.sigma <= 0.0 || cfg.sigma >= 1.0 ||
        !isFiniteF64(cfg.tol_f) || cfg.tol_f < 0.0 ||
        !isFiniteF64(cfg.tol_x) || cfg.tol_x < 0.0 ||
        !isFiniteF64(cfg.init_step) || cfg.init_step <= 0.0
    ) {
        return null;
    }

    let j: usize = 0;
    while (j < n) {
        if (!isFiniteF64(x0[j])) {
            return null;
        }
        j += 1;
    }

    // Initialize simplex vertices: v=0 is x0; v=i+1 perturbs coordinate i
    let v: usize = 0;
    while (v < nv) {
        const off = v * n;
        j = 0;
        while (j < n) {
            ws.simplex[off + j] = x0[j];
            j += 1;
        }
        if (v > 0) {
            const dim = v - 1;
            const base = x0[dim];
            const delta = if (absF64(base) > 1.0e-12) base * cfg.init_step else cfg.init_step;
            ws.simplex[off + dim] = base + delta;
        }
        const fv = obj(ws.simplex[off .. off + n]);
        if (!isFiniteF64(fv)) {
            return null;
        }
        ws.fvals[v] = fv;
        v += 1;
    }

    let evals: usize = nv;
    let iter: usize = 0;
    let converged = false;

    while (true) {
        // Locate best, worst, and second-worst vertices
        let best: usize = 0;
        let worst: usize = 0;
        v = 1;
        while (v < nv) {
            if (ws.fvals[v] < ws.fvals[best]) {
                best = v;
            }
            if (ws.fvals[v] > ws.fvals[worst]) {
                worst = v;
            }
            v += 1;
        }

        let second_worst: usize = best;
        v = 0;
        while (v < nv) {
            if (v != worst && ws.fvals[v] > ws.fvals[second_worst]) {
                second_worst = v;
            }
            v += 1;
        }

        // Convergence check on both function spread and simplex diameter
        const f_span = absF64(ws.fvals[worst] - ws.fvals[best]);
        let x_span: f64 = 0.0;
        const best_off = best * n;
        v = 0;
        while (v < nv) {
            if (v != best) {
                const v_off = v * n;
                j = 0;
                while (j < n) {
                    const dx = absF64(ws.simplex[v_off + j] - ws.simplex[best_off + j]);
                    if (dx > x_span) {
                        x_span = dx;
                    }
                    j += 1;
                }
            }
            v += 1;
        }

        if (f_span <= cfg.tol_f && x_span <= cfg.tol_x) {
            converged = true;
            break;
        }
        if (iter >= cfg.max_iter) {
            break;
        }

        // Compute centroid of all vertices except worst
        j = 0;
        while (j < n) {
            ws.centroid[j] = 0.0;
            j += 1;
        }
        v = 0;
        while (v < nv) {
            if (v != worst) {
                const v_off = v * n;
                j = 0;
                while (j < n) {
                    ws.centroid[j] += ws.simplex[v_off + j];
                    j += 1;
                }
            }
            v += 1;
        }
        const inv_n = 1.0 / (n as f64);
        j = 0;
        while (j < n) {
            ws.centroid[j] *= inv_n;
            j += 1;
        }

        const worst_off = worst * n;

        // 1. Reflection: xr = centroid + alpha * (centroid - x_worst)
        j = 0;
        while (j < n) {
            ws.xr[j] = ws.centroid[j] + cfg.alpha * (ws.centroid[j] - ws.simplex[worst_off + j]);
            j += 1;
        }
        const fr = obj(ws.xr[0..n]);
        evals += 1;
        if (!isFiniteF64(fr)) {
            return null;
        }

        if (fr >= ws.fvals[best] && fr < ws.fvals[second_worst]) {
            replaceVertex(ws, worst, n, ws.xr[0..n], fr);
        } else if (fr < ws.fvals[best]) {
            // 2. Expansion: xe = centroid + gamma * (xr - centroid)
            j = 0;
            while (j < n) {
                ws.xe[j] = ws.centroid[j] + cfg.gamma * (ws.xr[j] - ws.centroid[j]);
                j += 1;
            }
            const fe = obj(ws.xe[0..n]);
            evals += 1;
            if (!isFiniteF64(fe)) {
                return null;
            }
            if (fe < fr) {
                replaceVertex(ws, worst, n, ws.xe[0..n], fe);
            } else {
                replaceVertex(ws, worst, n, ws.xr[0..n], fr);
            }
        } else {
            // 3. Outside or Inside Contraction
            let do_shrink = false;
            if (fr < ws.fvals[worst]) {
                // Outside contraction
                j = 0;
                while (j < n) {
                    ws.xc[j] = ws.centroid[j] + cfg.rho * (ws.xr[j] - ws.centroid[j]);
                    j += 1;
                }
                const fc = obj(ws.xc[0..n]);
                evals += 1;
                if (!isFiniteF64(fc)) {
                    return null;
                }
                if (fc <= fr) {
                    replaceVertex(ws, worst, n, ws.xc[0..n], fc);
                } else {
                    do_shrink = true;
                }
            } else {
                // Inside contraction
                j = 0;
                while (j < n) {
                    ws.xc[j] = ws.centroid[j] + cfg.rho * (ws.simplex[worst_off + j] - ws.centroid[j]);
                    j += 1;
                }
                const fc = obj(ws.xc[0..n]);
                evals += 1;
                if (!isFiniteF64(fc)) {
                    return null;
                }
                if (fc < ws.fvals[worst]) {
                    replaceVertex(ws, worst, n, ws.xc[0..n], fc);
                } else {
                    do_shrink = true;
                }
            }

            // 4. Shrink toward best vertex
            if (do_shrink) {
                v = 0;
                while (v < nv) {
                    if (v != best) {
                        const v_off = v * n;
                        j = 0;
                        while (j < n) {
                            ws.simplex[v_off + j] = ws.simplex[best_off + j] +
                                cfg.sigma * (ws.simplex[v_off + j] - ws.simplex[best_off + j]);
                            j += 1;
                        }
                        const fv = obj(ws.simplex[v_off .. v_off + n]);
                        evals += 1;
                        if (!isFiniteF64(fv)) {
                            return null;
                        }
                        ws.fvals[v] = fv;
                    }
                    v += 1;
                }
            }
        }

        iter += 1;
    }

    // Extract final best vertex
    let final_best: usize = 0;
    let fv_idx: usize = 1;
    while (fv_idx < nv) {
        if (ws.fvals[fv_idx] < ws.fvals[final_best]) {
            final_best = fv_idx;
        }
        fv_idx += 1;
    }
    const f_off = final_best * n;
    j = 0;
    while (j < n) {
        x_out.*[j] = ws.simplex[f_off + j];
        j += 1;
    }

    return NelderResult {
        converged: converged,
        best_value: ws.fvals[final_best],
        iterations: iter,
        evals: evals,
    };
}

/// 1D Golden-Section Search minimization on `[a, b]` (`a < b`).
/// Returns `null` on invalid bracket, non-positive tolerance, or non-finite evaluations.
pub fn goldenSectionMin(
    f: ScalarFn,
    a: f64,
    b: f64,
    tol: f64,
    max_iter: usize,
): Minimize1DResult? {
    if (!isFiniteF64(a) || !isFiniteF64(b) || a >= b || !isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }

    const inv_phi = (@sqrt(5.0) - 1.0) * 0.5;
    const inv_phi2 = 1.0 - inv_phi;

    let lo = a;
    let hi = b;
    let x1 = lo + inv_phi2 * (hi - lo);
    let x2 = lo + inv_phi * (hi - lo);
    let f1 = f(x1);
    let f2 = f(x2);
    if (!isFiniteF64(f1) || !isFiniteF64(f2)) {
        return null;
    }

    let iter: usize = 0;
    while (hi - lo > tol && iter < max_iter) {
        if (f1 < f2) {
            hi = x2;
            x2 = x1;
            f2 = f1;
            x1 = lo + inv_phi2 * (hi - lo);
            f1 = f(x1);
            if (!isFiniteF64(f1)) {
                return null;
            }
        } else {
            lo = x1;
            x1 = x2;
            f1 = f2;
            x2 = lo + inv_phi * (hi - lo);
            f2 = f(x2);
            if (!isFiniteF64(f2)) {
                return null;
            }
        }
        iter += 1;
    }

    const mid = 0.5 * (lo + hi);
    const f_mid = f(mid);
    if (!isFiniteF64(f_mid)) {
        return null;
    }

    return Minimize1DResult {
        x: mid,
        fval: f_mid,
        iterations: iter,
        converged: (hi - lo) <= tol,
    };
}

/// 1D Brent-Dekker root-finding combining bisection, secant, and inverse quadratic interpolation.
/// Requires `f(a)` and `f(b)` to bracket a root (opposite signs); returns `null` if same-sign or non-finite.
pub fn brentRoot(
    f: ScalarFn,
    a_init: f64,
    b_init: f64,
    tol: f64,
    max_iter: usize,
): Root1DResult? {
    if (!isFiniteF64(a_init) || !isFiniteF64(b_init) || a_init == b_init || !isFiniteF64(tol) || tol <= 0.0) {
        return null;
    }

    let a = a_init;
    let b = b_init;
    let fa = f(a);
    let fb = f(b);
    if (!isFiniteF64(fa) || !isFiniteF64(fb)) {
        return null;
    }

    if (fa == 0.0) {
        return Root1DResult { root: a, fval: 0.0, iterations: 0, converged: true };
    }
    if (fb == 0.0) {
        return Root1DResult { root: b, fval: 0.0, iterations: 0, converged: true };
    }
    if ((fa > 0.0 && fb > 0.0) || (fa < 0.0 && fb < 0.0)) {
        return null;
    }

    let c = a;
    let fc = fa;
    let d = b - a;
    let e = d;
    const eps: f64 = 2.220446049250313e-16;

    let iter: usize = 0;
    while (iter < max_iter) {
        if ((fb > 0.0 && fc > 0.0) || (fb < 0.0 && fc < 0.0)) {
            c = a;
            fc = fa;
            d = b - a;
            e = d;
        }
        if (absF64(fc) < absF64(fb)) {
            a = b;
            b = c;
            c = a;
            fa = fb;
            fb = fc;
            fc = fa;
        }

        const tol1 = 2.0 * eps * absF64(b) + 0.5 * tol;
        const xm = 0.5 * (c - b);

        if (absF64(xm) <= tol1 || fb == 0.0) {
            return Root1DResult {
                root: b,
                fval: fb,
                iterations: iter,
                converged: true,
            };
        }

        if (absF64(e) >= tol1 && absF64(fa) > absF64(fb)) {
            const s = fb / fa;
            let p: f64 = 0.0;
            let q: f64 = 0.0;
            if (a == c) {
                // Linear secant step
                p = 2.0 * xm * s;
                q = 1.0 - s;
            } else {
                // Inverse quadratic interpolation
                const q_ratio = fa / fc;
                const r_ratio = fb / fc;
                p = s * (2.0 * xm * q_ratio * (q_ratio - r_ratio) - (b - a) * (r_ratio - 1.0));
                q = (q_ratio - 1.0) * (r_ratio - 1.0) * (s - 1.0);
            }
            if (p > 0.0) {
                q = -q;
            }
            p = absF64(p);
            const min1 = 3.0 * xm * q - absF64(tol1 * q);
            const min2 = absF64(e * q);
            const bound = if (min1 < min2) min1 else min2;
            if (2.0 * p < bound) {
                e = d;
                d = p / q;
            } else {
                d = xm;
                e = d;
            }
        } else {
            d = xm;
            e = d;
        }

        a = b;
        fa = fb;
        if (absF64(d) > tol1) {
            b += d;
        } else {
            b += if (xm >= 0.0) tol1 else -tol1;
        }
        fb = f(b);
        if (!isFiniteF64(fb)) {
            return null;
        }
        iter += 1;
    }

    return Root1DResult {
        root: b,
        fval: fb,
        iterations: iter,
        converged: false,
    };
}
