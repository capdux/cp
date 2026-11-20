// Source: cp/lib/optimize/opt.cp (1D Root-Finding, 1D Minimization & Multi-Dimensional Optimization)
// Pure CP, zero-C-dependency numerical root-finding (Bisection, Brent), 1D Golden-Section minimization,
// derivative-free Nelder-Mead simplex, and first-order Adam optimizer with analytical or central-difference gradients.

const std = @import("std");

const MACHEP: f64 = 0.0000000000000002220446049250313;
const GOLDEN_PHI: f64 = 0.6180339887498949;
const FD_EPS: f64 = 0.000001;

/// Error set for numerical optimization and root-finding operations.
pub err OptError {
    InvalidInterval,
    SameSignBracket,
    MaxIterationsExceeded,
    InvalidDimension,
    InvalidTolerance,
    OutOfMemory,
}

/// Callback type for 1D scalar objective functions: f(ctx, x) -> f64.
pub type ScalarFn = *const fn (*anyopaque, f64) f64;

/// Callback type for N-dimensional vector objective functions: f(ctx, x) -> f64.
pub type VectorFn = *const fn (*anyopaque, []const f64) f64;

/// Callback type for N-dimensional gradient functions: grad(ctx, x, out_grad).
pub type GradFn = *const fn (*anyopaque, []const f64, []f64) void;

/// Result of a 1D root-finding algorithm.
pub struct RootResult {
    root: f64;
    f_val: f64;
    iterations: u32;
    converged: bool;
}

/// Result of a 1D minimization algorithm.
pub struct MinResult1D {
    x: f64;
    f_val: f64;
    iterations: u32;
    converged: bool;
}

/// Result of a multi-dimensional optimization algorithm.
pub struct OptResult {
    x: []f64;
    f_val: f64;
    iterations: u32;
    converged: bool;

    /// Frees the solution vector allocated by the optimizer.
    fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.x.len > 0) {
            alloc.free(_.x);
            _.x = &[];
        }
    }
}

fn sameSign(a: f64, b: f64): bool {
    return (a > 0.0 && b > 0.0) || (a < 0.0 && b < 0.0);
}

fn centralDiffInPlace(ctx: *anyopaque, f: VectorFn, x: []f64, eps: f64, out_grad: []f64) {
    const inv_2eps = 1.0 / (2.0 * eps);
    for (i of 0..x.len) {
        const orig = x[i];
        x[i] = orig + eps;
        const fp = f(ctx, x);
        x[i] = orig - eps;
        const fm = f(ctx, x);
        x[i] = orig;
        out_grad[i] = (fp - fm) * inv_2eps;
    }
}

/// Numerical optimization and root-finding engine.
pub static class Optimizer {
    /// Finds a root of `f` on `[a, b]` using the bisection method.
    pub static fn bisect(
        ctx: *anyopaque,
        f: ScalarFn,
        a: f64,
        b: f64,
        tol: f64,
        max_iter: u32,
    ): RootResult ! OptError {
        if (!(tol > 0.0)) return OptError.InvalidTolerance;
        if (!(a < b)) return OptError.InvalidInterval;

        let lo = a;
        let hi = b;
        let flo = f(ctx, lo);
        let fhi = f(ctx, hi);

        if (flo == 0.0) {
            return RootResult { root: lo, f_val: flo, iterations: 0, converged: true };
        }
        if (fhi == 0.0) {
            return RootResult { root: hi, f_val: fhi, iterations: 0, converged: true };
        }
        if (sameSign(flo, fhi)) {
            return OptError.SameSignBracket;
        }

        let iter: u32 = 0;
        while (iter < max_iter) : (iter += 1) {
            const half = 0.5 * (hi - lo);
            const mid = lo + half;
            const fmid = f(ctx, mid);

            if (fmid == 0.0 || half <= tol) {
                return RootResult {
                    root: mid,
                    f_val: fmid,
                    iterations: iter + 1,
                    converged: true,
                };
            }

            if (sameSign(flo, fmid)) {
                lo = mid;
                flo = fmid;
            } else {
                hi = mid;
                fhi = fmid;
            }
        }

        return OptError.MaxIterationsExceeded;
    }

    /// Finds a root of `f` on `[a, b]` using Brent's superlinear method.
    pub static fn brentRoot(
        ctx: *anyopaque,
        f: ScalarFn,
        a: f64,
        b: f64,
        tol: f64,
        max_iter: u32,
    ): RootResult ! OptError {
        if (!(tol > 0.0)) return OptError.InvalidTolerance;
        if (!(a < b)) return OptError.InvalidInterval;

        let ax = a;
        let bx = b;
        let fa = f(ctx, ax);
        let fb = f(ctx, bx);

        if (fa == 0.0) {
            return RootResult { root: ax, f_val: fa, iterations: 0, converged: true };
        }
        if (fb == 0.0) {
            return RootResult { root: bx, f_val: fb, iterations: 0, converged: true };
        }
        if (sameSign(fa, fb)) {
            return OptError.SameSignBracket;
        }

        let cx = bx;
        let fc = fb;
        let d = bx - ax;
        let e = d;

        let iter: u32 = 0;
        while (iter < max_iter) : (iter += 1) {
            if (sameSign(fb, fc)) {
                cx = ax;
                fc = fa;
                d = bx - ax;
                e = d;
            }
            if (@abs(fc) < @abs(fb)) {
                ax = bx;
                bx = cx;
                cx = ax;
                fa = fb;
                fb = fc;
                fc = fa;
            }

            const tol1 = 2.0 * MACHEP * @abs(bx) + 0.5 * tol;
            const xm = 0.5 * (cx - bx);

            if (@abs(xm) <= tol1 || fb == 0.0) {
                return RootResult {
                    root: bx,
                    f_val: fb,
                    iterations: iter,
                    converged: true,
                };
            }

            if (@abs(e) >= tol1 && @abs(fa) > @abs(fb)) {
                const s = fb / fa;
                let p: f64 = 0.0;
                let q: f64 = 0.0;
                if (ax == cx) {
                    p = 2.0 * xm * s;
                    q = 1.0 - s;
                } else {
                    const q_val = fa / fc;
                    const r_val = fb / fc;
                    p = s * (2.0 * xm * q_val * (q_val - r_val) - (bx - ax) * (r_val - 1.0));
                    q = (q_val - 1.0) * (r_val - 1.0) * (s - 1.0);
                }

                if (p > 0.0) {
                    q = -q;
                }
                p = @abs(p);

                const min1 = 3.0 * xm * q - @abs(tol1 * q);
                const min2 = @abs(e * q);
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

            ax = bx;
            fa = fb;
            if (@abs(d) > tol1) {
                bx += d;
            } else {
                bx += if (xm >= 0.0) tol1 else -tol1;
            }
            fb = f(ctx, bx);
        }

        return OptError.MaxIterationsExceeded;
    }

    /// Minimizes a unimodal 1D function `f` on `[a, b]` using Golden-Section Search.
    pub static fn goldenSectionMin(
        ctx: *anyopaque,
        f: ScalarFn,
        a: f64,
        b: f64,
        tol: f64,
        max_iter: u32,
    ): MinResult1D ! OptError {
        if (!(tol > 0.0)) return OptError.InvalidTolerance;
        if (!(a < b)) return OptError.InvalidInterval;

        let lo = a;
        let hi = b;
        let x1 = hi - GOLDEN_PHI * (hi - lo);
        let x2 = lo + GOLDEN_PHI * (hi - lo);
        let f1 = f(ctx, x1);
        let f2 = f(ctx, x2);

        let iter: u32 = 0;
        while (iter < max_iter) : (iter += 1) {
            if ((hi - lo) <= tol) {
                const x_best = 0.5 * (lo + hi);
                const f_best = f(ctx, x_best);
                return MinResult1D {
                    x: x_best,
                    f_val: f_best,
                    iterations: iter,
                    converged: true,
                };
            }

            if (f1 < f2) {
                hi = x2;
                x2 = x1;
                f2 = f1;
                x1 = hi - GOLDEN_PHI * (hi - lo);
                f1 = f(ctx, x1);
            } else {
                lo = x1;
                x1 = x2;
                f1 = f2;
                x2 = lo + GOLDEN_PHI * (hi - lo);
                f2 = f(ctx, x2);
            }
        }

        return OptError.MaxIterationsExceeded;
    }

    /// Computes the central-difference numerical gradient of `f` at `x` into `out_grad`.
    pub static fn numericalGradient(
        ctx: *anyopaque,
        f: VectorFn,
        x: []const f64,
        eps: f64,
        out_grad: []f64,
    ): void ! OptError {
        if (x.len == 0 || x.len != out_grad.len) return OptError.InvalidDimension;
        if (!(eps > 0.0)) return OptError.InvalidTolerance;

        if (x.len <= 256) {
            let buf: [256]f64 = undefined;
            let work = buf[0..x.len];
            @memcpy(work, x);
            centralDiffInPlace(ctx, f, work, eps, out_grad);
        } else {
            const alloc = std.heap.page_allocator;
            let work = alloc.alloc(f64, x.len) catch return OptError.OutOfMemory;
            defer alloc.free(work);
            @memcpy(work, x);
            centralDiffInPlace(ctx, f, work, eps, out_grad);
        }
    }

    /// Minimizes an N-dimensional function `f` using the derivative-free Nelder-Mead simplex method.
    pub static fn nelderMead(
        alloc: std.mem.Allocator,
        ctx: *anyopaque,
        f: VectorFn,
        x0: []const f64,
        initial_step: f64,
        tol: f64,
        max_iter: u32,
    ): OptResult ! OptError {
        const n = x0.len;
        if (n == 0) return OptError.InvalidDimension;
        if (!(tol > 0.0) || initial_step == 0.0) return OptError.InvalidTolerance;

        const m = n + 1;
        let simplex = alloc.alloc(f64, m * n) catch return OptError.OutOfMemory;
        defer alloc.free(simplex);

        let f_vals = alloc.alloc(f64, m) catch return OptError.OutOfMemory;
        defer alloc.free(f_vals);

        let order = alloc.alloc(usize, m) catch return OptError.OutOfMemory;
        defer alloc.free(order);

        let xc = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
        defer alloc.free(xc);

        let xr = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
        defer alloc.free(xr);

        let xe = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
        defer alloc.free(xe);

        let xcon = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
        defer alloc.free(xcon);

        // Initialize simplex: vertex 0 is x0, vertex i perturbs coordinate i-1
        for (i of 0..m) {
            const base = i * n;
            @memcpy(simplex[base..(base + n)], x0);
            if (i > 0) {
                simplex[base + (i - 1)] += initial_step;
            }
            f_vals[i] = f(ctx, simplex[base..(base + n)]);
            order[i] = i;
        }

        const alpha: f64 = 1.0;
        const gamma: f64 = 2.0;
        const rho: f64 = 0.5;
        const sigma: f64 = 0.5;

        let iter: u32 = 0;
        while (iter < max_iter) : (iter += 1) {
            // Insertion sort indices by f_vals ascending
            for (i of 1..m) {
                const key_idx = order[i];
                const key_val = f_vals[key_idx];
                let j = i;
                while (j > 0 && f_vals[order[j - 1]] > key_val) : (j -= 1) {
                    order[j] = order[j - 1];
                }
                order[j] = key_idx;
            }

            const best = order[0];
            const second_worst = order[m - 2];
            const worst = order[m - 1];

            // Convergence check: both function value spread and simplex diameter <= tol
            const f_spread = @abs(f_vals[worst] - f_vals[best]);
            let max_dist: f64 = 0.0;
            const best_base = best * n;
            for (i of 1..m) {
                const v_base = order[i] * n;
                for (k of 0..n) {
                    const d = @abs(simplex[v_base + k] - simplex[best_base + k]);
                    if (d > max_dist) max_dist = d;
                }
            }
            if (f_spread <= tol && max_dist <= tol) {
                const out_x = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
                @memcpy(out_x, simplex[best_base..(best_base + n)]);
                return OptResult {
                    x: out_x,
                    f_val: f_vals[best],
                    iterations: iter,
                    converged: true,
                };
            }

            // Compute centroid of all vertices except worst
            @memset(xc, 0.0);
            for (i of 0..(m - 1)) {
                const v_base = order[i] * n;
                for (k of 0..n) {
                    xc[k] += simplex[v_base + k];
                }
            }
            const inv_n = 1.0 / f64(n);
            for (k of 0..n) {
                xc[k] *= inv_n;
            }

            const worst_base = worst * n;

            // Reflection
            for (k of 0..n) {
                xr[k] = xc[k] + alpha * (xc[k] - simplex[worst_base + k]);
            }
            const fr = f(ctx, xr);

            if (f_vals[best] <= fr && fr < f_vals[second_worst]) {
                @memcpy(simplex[worst_base..(worst_base + n)], xr);
                f_vals[worst] = fr;
                continue;
            }

            // Expansion
            if (fr < f_vals[best]) {
                for (k of 0..n) {
                    xe[k] = xc[k] + gamma * (xr[k] - xc[k]);
                }
                const fe = f(ctx, xe);
                if (fe < fr) {
                    @memcpy(simplex[worst_base..(worst_base + n)], xe);
                    f_vals[worst] = fe;
                } else {
                    @memcpy(simplex[worst_base..(worst_base + n)], xr);
                    f_vals[worst] = fr;
                }
                continue;
            }

            // Contraction
            if (fr < f_vals[worst]) {
                // Outside contraction
                for (k of 0..n) {
                    xcon[k] = xc[k] + rho * (xr[k] - xc[k]);
                }
                const fc = f(ctx, xcon);
                if (fc <= fr) {
                    @memcpy(simplex[worst_base..(worst_base + n)], xcon);
                    f_vals[worst] = fc;
                    continue;
                }
            } else {
                // Inside contraction
                for (k of 0..n) {
                    xcon[k] = xc[k] + rho * (simplex[worst_base + k] - xc[k]);
                }
                const fc = f(ctx, xcon);
                if (fc < f_vals[worst]) {
                    @memcpy(simplex[worst_base..(worst_base + n)], xcon);
                    f_vals[worst] = fc;
                    continue;
                }
            }

            // Shrink all vertices except best
            for (i of 1..m) {
                const v_base = order[i] * n;
                for (k of 0..n) {
                    simplex[v_base + k] = simplex[best_base + k] + sigma * (simplex[v_base + k] - simplex[best_base + k]);
                }
                f_vals[order[i]] = f(ctx, simplex[v_base..(v_base + n)]);
            }
        }

        return OptError.MaxIterationsExceeded;
    }

    /// Minimizes an N-dimensional function `f` using the Adam first-order optimizer.
    /// When `grad_fn_opt` is `null`, central finite differences are computed automatically.
    pub static fn adamMinimize(
        alloc: std.mem.Allocator,
        ctx: *anyopaque,
        f: VectorFn,
        grad_fn_opt: GradFn?,
        x0: []const f64,
        lr: f64,
        tol: f64,
        max_iter: u32,
    ): OptResult ! OptError {
        const n = x0.len;
        if (n == 0) return OptError.InvalidDimension;
        if (!(tol > 0.0) || !(lr > 0.0)) return OptError.InvalidTolerance;

        let x = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
        errdefer alloc.free(x);
        @memcpy(x, x0);

        let m_vec = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
        defer alloc.free(m_vec);
        @memset(m_vec, 0.0);

        let v_vec = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
        defer alloc.free(v_vec);
        @memset(v_vec, 0.0);

        let grad = alloc.alloc(f64, n) catch return OptError.OutOfMemory;
        defer alloc.free(grad);

        const beta1: f64 = 0.9;
        const beta2: f64 = 0.999;
        const eps: f64 = 0.00000001;

        let beta1_t: f64 = 1.0;
        let beta2_t: f64 = 1.0;

        let iter: u32 = 0;
        while (iter < max_iter) : (iter += 1) {
            if (grad_fn := grad_fn_opt) {
                grad_fn(ctx, x, grad);
            } else {
                centralDiffInPlace(ctx, f, x, FD_EPS, grad);
            }

            let gnorm_sq: f64 = 0.0;
            for (k of 0..n) {
                gnorm_sq += grad[k] * grad[k];
            }
            if (@sqrt(gnorm_sq) <= tol) {
                const f_val = f(ctx, x);
                return OptResult {
                    x: x,
                    f_val: f_val,
                    iterations: iter,
                    converged: true,
                };
            }

            beta1_t *= beta1;
            beta2_t *= beta2;
            const inv_bias1 = 1.0 / (1.0 - beta1_t);
            const inv_bias2 = 1.0 / (1.0 - beta2_t);

            for (k of 0..n) {
                const g = grad[k];
                m_vec[k] = beta1 * m_vec[k] + (1.0 - beta1) * g;
                v_vec[k] = beta2 * v_vec[k] + (1.0 - beta2) * g * g;
                const m_hat = m_vec[k] * inv_bias1;
                const v_hat = v_vec[k] * inv_bias2;
                x[k] -= lr * m_hat / (@sqrt(v_hat) + eps);
            }
        }

        return OptError.MaxIterationsExceeded;
    }
}
