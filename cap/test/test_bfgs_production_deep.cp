// Source: cp/cap/test/test_bfgs_production_deep.cp (Production deep tests for cap:bfgs)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BfgsConfig,
    BfgsWorkspace,
    LbfgsWorkspace,
    bfgsMinimize,
    lbfgsMinimize,
    finiteDiffGradient,
} from "cap:bfgs";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn rosenbrock2dFg(x: []const f64, g: *[]f64): f64 {
    const a = 1.0 - x[0];
    const b = x[1] - x[0] * x[0];
    g.*[0] = -2.0 * a - 400.0 * x[0] * b;
    g.*[1] = 200.0 * b;
    return a * a + 100.0 * b * b;
}

fn rosenbrock2dVal(x: []const f64): f64 {
    const a = 1.0 - x[0];
    const b = x[1] - x[0] * x[0];
    return a * a + 100.0 * b * b;
}

/// 16D coupled tridiagonal positive-definite quadratic:
/// f(x) = 0.5 * sum_i (2*(x_i - t_i)^2) + 0.5 * sum_{i=0..n-2} (x_i - t_i - (x_{i+1} - t_{i+1}))^2
/// Unique global minimum at x_i = t_i = (i + 1) * 0.25 with f* = 0.
fn coupledQuad16Fg(x: []const f64, g: *[]f64): f64 {
    const n = x.len;
    let val: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        g.*[i] = 0.0;
        i += 1;
    }
    i = 0;
    while (i < n) {
        const t_i = ((i + 1) as f64) * 0.25;
        const di = x[i] - t_i;
        val += di * di;
        g.*[i] += 2.0 * di;
        if (i + 1 < n) {
            const t_next = ((i + 2) as f64) * 0.25;
            const d_next = x[i + 1] - t_next;
            const diff = di - d_next;
            val += 0.5 * diff * diff;
            g.*[i] += diff;
            g.*[i + 1] -= diff;
        }
        i += 1;
    }
    return val;
}

/// Generalized N-dimensional Rosenbrock: sum_{i=0..n-2} [100*(x_{i+1} - x_i^2)^2 + (1 - x_i)^2]
fn generalizedRosenbrockFg(x: []const f64, g: *[]f64): f64 {
    const n = x.len;
    let val: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        g.*[i] = 0.0;
        i += 1;
    }
    i = 0;
    while (i + 1 < n) {
        const a = 1.0 - x[i];
        const b = x[i + 1] - x[i] * x[i];
        val += a * a + 100.0 * b * b;
        g.*[i] += -2.0 * a - 400.0 * x[i] * b;
        g.*[i + 1] += 200.0 * b;
        i += 1;
    }
    return val;
}

/// L2-regularized binary logistic regression on a clean 8-sample 2D dataset (3 params: w0, w1, bias).
fn logisticL2Fg(w: []const f64, g: *[]f64): f64 {
    const xs: [16]f64 = [
        -2.0, -1.5,  -1.8, -0.5,  -1.2, -2.0,  -2.5, -1.0,
         2.0,  1.5,   1.8,  0.5,   1.2,  2.0,   2.5,  1.0,
    ];
    const ys: [8]f64 = [0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0];
    const reg: f64 = 0.05;

    let loss: f64 = 0.5 * reg * (w[0] * w[0] + w[1] * w[1]);
    g.*[0] = reg * w[0];
    g.*[1] = reg * w[1];
    g.*[2] = 0.0;

    let i: usize = 0;
    while (i < 8) {
        const x0 = xs[i * 2];
        const x1 = xs[i * 2 + 1];
        const z = w[0] * x0 + w[1] * x1 + w[2];
        // Numerically stable log(1 + exp(z)) - y*z and sigmoid
        let p: f64 = 0.0;
        if (z >= 0.0) {
            const ez = @exp(-z);
            loss += z + @log(1.0 + ez) - ys[i] * z;
            p = 1.0 / (1.0 + ez);
        } else {
            const ez = @exp(z);
            loss += @log(1.0 + ez) - ys[i] * z;
            p = ez / (1.0 + ez);
        }
        const diff = p - ys[i];
        g.*[0] += diff * x0;
        g.*[1] += diff * x1;
        g.*[2] += diff;
        i += 1;
    }

    return loss;
}

fn nanFg(x: []const f64, g: *[]f64): f64 {
    const zero: f64 = 0.0;
    g.*[0] = 0.0;
    return (x[0] * zero) / zero;
}

test("1. 2D Rosenbrock banana: bfgsMinimize converges to < 1e-10 within 40 iterations", () => {
    const x0 = [-1.2, 1.0];
    let x_out: [2]f64 = std.mem.zeroes([2]f64);
    let h_inv: [4]f64 = std.mem.zeroes([4]f64);
    let grad: [2]f64 = std.mem.zeroes([2]f64);
    let grad_next: [2]f64 = std.mem.zeroes([2]f64);
    let dir: [2]f64 = std.mem.zeroes([2]f64);
    let x_next: [2]f64 = std.mem.zeroes([2]f64);
    let s: [2]f64 = std.mem.zeroes([2]f64);
    let y: [2]f64 = std.mem.zeroes([2]f64);
    let hy: [2]f64 = std.mem.zeroes([2]f64);

    let ws = BfgsWorkspace {
        h_inv: h_inv[0..],
        grad: grad[0..],
        grad_next: grad_next[0..],
        dir: dir[0..],
        x_next: x_next[0..],
        s: s[0..],
        y: y[0..],
        hy: hy[0..],
    };
    let xo_s: []f64 = x_out[0..];
    let cfg = BfgsConfig.default();
    cfg.grad_tol = 1.0e-8;
    cfg.max_iter = 40;

    const res = bfgsMinimize(rosenbrock2dFg, x0[0..], &xo_s, cfg, &ws) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.iterations <= 40, true);
    expect_eq(res.best_value < 1.0e-10, true);
    expect_eq(absF64(x_out[0] - 1.0) < 1.0e-6, true);
    expect_eq(absF64(x_out[1] - 1.0) < 1.0e-6, true);
});

test("2. High-dimensional 16D coupled quadratic: lbfgsMinimize (m=5) high-precision convergence", () => {
    const n: usize = 16;
    const m: usize = 5;
    let x0: [16]f64 = std.mem.zeroes([16]f64);
    let x_out: [16]f64 = std.mem.zeroes([16]f64);
    let s_hist: [80]f64 = std.mem.zeroes([80]f64);
    let y_hist: [80]f64 = std.mem.zeroes([80]f64);
    let rho_hist: [5]f64 = std.mem.zeroes([5]f64);
    let alpha_hist: [5]f64 = std.mem.zeroes([5]f64);
    let grad: [16]f64 = std.mem.zeroes([16]f64);
    let grad_next: [16]f64 = std.mem.zeroes([16]f64);
    let dir: [16]f64 = std.mem.zeroes([16]f64);
    let x_next: [16]f64 = std.mem.zeroes([16]f64);

    let ws = LbfgsWorkspace {
        s_hist: s_hist[0..],
        y_hist: y_hist[0..],
        rho_hist: rho_hist[0..],
        alpha_hist: alpha_hist[0..],
        grad: grad[0..],
        grad_next: grad_next[0..],
        dir: dir[0..],
        x_next: x_next[0..],
    };
    let xo_s: []f64 = x_out[0..];
    let cfg = BfgsConfig.default();
    cfg.grad_tol = 1.0e-9;

    const res = lbfgsMinimize(coupledQuad16Fg, x0[0..], m, &xo_s, cfg, &ws) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.best_value < 1.0e-14, true);

    let i: usize = 0;
    while (i < n) {
        const expected = ((i + 1) as f64) * 0.25;
        expect_eq(absF64(x_out[i] - expected) < 1.0e-7, true);
        i += 1;
    }
});

test("3. 10D Generalized Rosenbrock: lbfgsMinimize (m=5) converges to all-ones global minimum", () => {
    const n: usize = 10;
    const m: usize = 5;
    let x0: [10]f64 = [-1.2, 1.0, -1.2, 1.0, -1.2, 1.0, -1.2, 1.0, -1.2, 1.0];
    let x_out: [10]f64 = std.mem.zeroes([10]f64);
    let s_hist: [50]f64 = std.mem.zeroes([50]f64);
    let y_hist: [50]f64 = std.mem.zeroes([50]f64);
    let rho_hist: [5]f64 = std.mem.zeroes([5]f64);
    let alpha_hist: [5]f64 = std.mem.zeroes([5]f64);
    let grad: [10]f64 = std.mem.zeroes([10]f64);
    let grad_next: [10]f64 = std.mem.zeroes([10]f64);
    let dir: [10]f64 = std.mem.zeroes([10]f64);
    let x_next: [10]f64 = std.mem.zeroes([10]f64);

    let ws = LbfgsWorkspace {
        s_hist: s_hist[0..],
        y_hist: y_hist[0..],
        rho_hist: rho_hist[0..],
        alpha_hist: alpha_hist[0..],
        grad: grad[0..],
        grad_next: grad_next[0..],
        dir: dir[0..],
        x_next: x_next[0..],
    };
    let xo_s: []f64 = x_out[0..];
    let cfg = BfgsConfig.default();
    cfg.grad_tol = 1.0e-7;
    cfg.max_iter = 300;

    const res = lbfgsMinimize(generalizedRosenbrockFg, x0[0..], m, &xo_s, cfg, &ws) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.best_value < 1.0e-10, true);
    let i: usize = 0;
    while (i < n) {
        expect_eq(absF64(x_out[i] - 1.0) < 1.0e-5, true);
        i += 1;
    }
});

test("4. L2-regularized logistic regression convex optimization reaches exact stationary point", () => {
    const w0 = [0.0, 0.0, 0.0];
    let w_out: [3]f64 = std.mem.zeroes([3]f64);
    let h_inv: [9]f64 = std.mem.zeroes([9]f64);
    let grad: [3]f64 = std.mem.zeroes([3]f64);
    let grad_next: [3]f64 = std.mem.zeroes([3]f64);
    let dir: [3]f64 = std.mem.zeroes([3]f64);
    let x_next: [3]f64 = std.mem.zeroes([3]f64);
    let s: [3]f64 = std.mem.zeroes([3]f64);
    let y: [3]f64 = std.mem.zeroes([3]f64);
    let hy: [3]f64 = std.mem.zeroes([3]f64);

    let ws = BfgsWorkspace {
        h_inv: h_inv[0..],
        grad: grad[0..],
        grad_next: grad_next[0..],
        dir: dir[0..],
        x_next: x_next[0..],
        s: s[0..],
        y: y[0..],
        hy: hy[0..],
    };
    let wo_s: []f64 = w_out[0..];
    let cfg = BfgsConfig.default();
    cfg.grad_tol = 1.0e-9;

    const res = bfgsMinimize(logisticL2Fg, w0[0..], &wo_s, cfg, &ws) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.grad_norm < 1.0e-8, true);
    // Symmetric dataset => positive weights w0, w1 > 0 and near-zero bias w2 ~ 0
    expect_eq(w_out[0] > 0.5, true);
    expect_eq(w_out[1] > 0.5, true);
    expect_eq(absF64(w_out[2]) < 1.0e-6, true);
});

test("5. finiteDiffGradient matches analytic Rosenbrock gradient within < 1e-9", () => {
    const pt = [-0.7, 1.3];
    let g_analytic: [2]f64 = std.mem.zeroes([2]f64);
    let g_fd: [2]f64 = std.mem.zeroes([2]f64);
    let scratch: [2]f64 = std.mem.zeroes([2]f64);
    let ga_s: []f64 = g_analytic[0..];
    let gfd_s: []f64 = g_fd[0..];
    let sc_s: []f64 = scratch[0..];

    * = rosenbrock2dFg(pt[0..], &ga_s);
    expect_eq(finiteDiffGradient(rosenbrock2dVal, pt[0..], 1.0e-6, &sc_s, &gfd_s), true);
    expect_eq(absF64(g_analytic[0] - g_fd[0]) < 1.0e-9, true);
    expect_eq(absF64(g_analytic[1] - g_fd[1]) < 1.0e-9, true);
});

test("6. Defensive validation: n==0, m==0, undersized buffers, invalid config & NaN rejection", () => {
    const empty: [0]f64 = [];
    const x0 = [1.0, 2.0];
    let x_out: [2]f64 = std.mem.zeroes([2]f64);
    let h_inv: [4]f64 = std.mem.zeroes([4]f64);
    let h_short: [2]f64 = std.mem.zeroes([2]f64);
    let v2: [2]f64 = std.mem.zeroes([2]f64);
    let v2b: [2]f64 = std.mem.zeroes([2]f64);
    let v2c: [2]f64 = std.mem.zeroes([2]f64);
    let v2d: [2]f64 = std.mem.zeroes([2]f64);
    let v2e: [2]f64 = std.mem.zeroes([2]f64);
    let v2f: [2]f64 = std.mem.zeroes([2]f64);
    let v2g: [2]f64 = std.mem.zeroes([2]f64);

    let ws_ok = BfgsWorkspace {
        h_inv: h_inv[0..],
        grad: v2[0..],
        grad_next: v2b[0..],
        dir: v2c[0..],
        x_next: v2d[0..],
        s: v2e[0..],
        y: v2f[0..],
        hy: v2g[0..],
    };
    let ws_short = BfgsWorkspace {
        h_inv: h_short[0..],
        grad: v2[0..],
        grad_next: v2b[0..],
        dir: v2c[0..],
        x_next: v2d[0..],
        s: v2e[0..],
        y: v2f[0..],
        hy: v2g[0..],
    };
    let xo_s: []f64 = x_out[0..];
    const cfg = BfgsConfig.default();

    expect_eq(bfgsMinimize(rosenbrock2dFg, empty[0..], &xo_s, cfg, &ws_ok) == null, true);
    expect_eq(bfgsMinimize(rosenbrock2dFg, x0[0..], &xo_s, cfg, &ws_short) == null, true);
    expect_eq(bfgsMinimize(nanFg, x0[0..], &xo_s, cfg, &ws_ok) == null, true);

    const zero: f64 = 0.0;
    const x_nan = [zero / zero, 1.0];
    expect_eq(bfgsMinimize(rosenbrock2dFg, x_nan[0..], &xo_s, cfg, &ws_ok) == null, true);

    // L-BFGS m == 0 rejected
    let s_h: [4]f64 = std.mem.zeroes([4]f64);
    let y_h: [4]f64 = std.mem.zeroes([4]f64);
    let r_h: [2]f64 = std.mem.zeroes([2]f64);
    let a_h: [2]f64 = std.mem.zeroes([2]f64);
    let lws = LbfgsWorkspace {
        s_hist: s_h[0..],
        y_hist: y_h[0..],
        rho_hist: r_h[0..],
        alpha_hist: a_h[0..],
        grad: v2[0..],
        grad_next: v2b[0..],
        dir: v2c[0..],
        x_next: v2d[0..],
    };
    expect_eq(lbfgsMinimize(rosenbrock2dFg, x0[0..], 0, &xo_s, cfg, &lws) == null, true);
    expect_eq(lbfgsMinimize(rosenbrock2dFg, x0[0..], 3, &xo_s, cfg, &lws) == null, true);

    // finiteDiffGradient invalid eps or empty x rejected
    let sc_s: []f64 = v2[0..];
    let gd_s: []f64 = v2b[0..];
    expect_eq(finiteDiffGradient(rosenbrock2dVal, empty[0..], 1.0e-6, &sc_s, &gd_s), false);
    expect_eq(finiteDiffGradient(rosenbrock2dVal, x0[0..], 0.0, &sc_s, &gd_s), false);
});
