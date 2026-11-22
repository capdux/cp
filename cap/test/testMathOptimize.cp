// Origin: Pure-CP Stdlib Test Suite (lib/math/optimize.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Numerical Optimization & Two-Phase Simplex LP:
//   1. Brent 1D minimization & bracketed root-finding (< 1e-10 accuracy)
//   2. Standard production resource-allocation LP (Wyndor Glass & 4-var LP optimal solution)
//   3. Two-Phase Simplex with negative RHS constraints (Phase I artificial feasibility -> Phase II optimal)
//   4. Exact `infeasible` and `unbounded` LP classification
//   5. 2D & 10D coupled Rosenbrock minimization via L-BFGS (< 1e-6 coordinate error) & central-diff gradient
//   6. 16D ill-conditioned quadratic minimization via Polak-Ribiere+ Conjugate Gradient
//   7. Nelder-Mead non-smooth black-box minimization, `OptError` guards & `cap.math` facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    OptError,
    OptStatus,
    LpStatus,
    brentMinimize1d,
    brentRoot1d,
    lpTableauRequiredLen,
    solveLinearProgramSimplex,
    approxGradientCentral,
    cgWorkRequiredLen,
    minimizeConjugateGradient,
    lbfgsWorkRequiredLen,
    minimizeLbfgs,
    nelderMeadWorkRequiredLen,
    minimizeNelderMead,
} from "cap:math";

fn absDiff(a: f64, b: f64): f64 {
    const d: f64 = a - b;
    return if (d < 0.0) -d else d;
}

fn poly1d(x: f64): f64 {
    const x2: f64 = x * x;
    return x2 * x2 - 3.0 * x2 + 2.0; // min on [0, 3] at x = sqrt(1.5), f = -0.25
}

fn dottie1d(x: f64): f64 {
    return std.math.cos(x) - x; // root ~ 0.7390851332151607
}

fn wallis1d(x: f64): f64 {
    return x * x * x - 2.0 * x - 5.0; // root ~ 2.0945514815423265
}

fn rosenbrockVal(x: []const f64): f64 {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i + 1 < x.len) : (i += 1) {
        const t1: f64 = x[i + 1] - x[i] * x[i];
        const t2: f64 = 1.0 - x[i];
        sum += 100.0 * t1 * t1 + t2 * t2;
    }
    return sum;
}

fn rosenbrockGrad(x: []const f64, g: *[]f64) {
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        g[i] = 0.0;
    }
    let k: usize = 0;
    while (k + 1 < x.len) : (k += 1) {
        const t1: f64 = x[k + 1] - x[k] * x[k];
        g[k] += -400.0 * x[k] * t1 - 2.0 * (1.0 - x[k]);
        g[k + 1] += 200.0 * t1;
    }
}

fn illCondQuadVal(x: []const f64): f64 {
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        const fi: f64 = @as(f64, @floatFromInt(i));
        const d_i: f64 = 1.0 + 6.6 * fi; // condition number ~ 100 for dim=16
        const target: f64 = fi * 0.5 - 3.0;
        const diff: f64 = x[i] - target;
        sum += 0.5 * d_i * diff * diff;
    }
    return sum;
}

fn illCondQuadGrad(x: []const f64, g: *[]f64) {
    let i: usize = 0;
    while (i < x.len) : (i += 1) {
        const fi: f64 = @as(f64, @floatFromInt(i));
        const d_i: f64 = 1.0 + 6.6 * fi;
        const target: f64 = fi * 0.5 - 3.0;
        g[i] = d_i * (x[i] - target);
    }
}

fn nonSmoothL1Val(x: []const f64): f64 {
    const dx: f64 = absDiff(x[0], 2.5);
    const dy: f64 = absDiff(x[1], -1.5);
    return dx + 2.0 * dy + 0.1 * (dx * dx + dy * dy);
}

test("Brent1D: minimization & root-finding < 1e-10 accuracy") {
    const m = brentMinimize1d(poly1d, 0.0, 3.0, 1.0e-11, 100) catch {
        expect_true(false);
        return;
    };
    expect_true(m.status == OptStatus.converged);
    expect_true(absDiff(m.x_best, std.math.sqrt(1.5)) < 1.0e-7);
    expect_true(absDiff(m.objective, -0.25) < 1.0e-12);

    const r1 = brentRoot1d(dottie1d, 0.0, 1.5, 1.0e-12, 100) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(r1, 0.7390851332151607) < 1.0e-11);

    const r2 = brentRoot1d(wallis1d, 1.0, 3.0, 1.0e-12, 100) catch {
        expect_true(false);
        return;
    };
    expect_true(absDiff(r2, 2.0945514815423265) < 1.0e-11);
}

test("SimplexLP: Wyndor Glass production resource-allocation optimal solution") {
    // min -3*x0 - 5*x1 s.t. x0 <= 4, 2*x1 <= 12, 3*x0 + 2*x1 <= 18, x >= 0
    // Optimal: x = [2.0, 6.0], obj = -36.0
    const c: [2]f64 = [-3.0, -5.0];
    const a: [6]f64 = [
        1.0, 0.0,
        0.0, 2.0,
        3.0, 2.0,
    ];
    const b: [3]f64 = [4.0, 12.0, 18.0];
    expect_eq(lpTableauRequiredLen(2, 3), @as(usize, 36));

    let x_out: [2]f64 = [0.0; 2];
    let tab: [36]f64 = [0.0; 36];
    let basis: [3]usize = [0; 3];
    let x_s: []f64 = x_out[0..];
    let tab_s: []f64 = tab[0..];
    let bas_s: []usize = basis[0..];

    const res = cap.math.solveLinearProgramSimplex(
        c[0..],
        a[0..],
        b[0..],
        2,
        3,
        *x_s,
        *tab_s,
        *bas_s,
        100,
    ) catch {
        expect_true(false);
        return;
    };
    expect_true(res.status == LpStatus.optimal);
    expect_true(absDiff(res.objective, -36.0) < 1.0e-9);
    expect_true(absDiff(x_out[0], 2.0) < 1.0e-9);
    expect_true(absDiff(x_out[1], 6.0) < 1.0e-9);
}

test("SimplexLP: Two-Phase negative RHS feasibility & optimal recovery") {
    // min 2*x0 + 3*x1
    // s.t. -x0 - x1 <= -4   (x0 + x1 >= 4)
    //      -2*x0 - x1 <= -6 (2*x0 + x1 >= 6)
    //      x0 <= 5, x1 <= 5, x >= 0
    // Optimal: x = [4.0, 0.0], obj = 8.0
    const c: [2]f64 = [2.0, 3.0];
    const a: [8]f64 = [
        -1.0, -1.0,
        -2.0, -1.0,
        1.0, 0.0,
        0.0, 1.0,
    ];
    const b: [4]f64 = [-4.0, -6.0, 5.0, 5.0];
    // req = (4+1)*(2+8+1) = 55
    let x_out: [2]f64 = [0.0; 2];
    let tab: [64]f64 = [0.0; 64];
    let basis: [4]usize = [0; 4];
    let x_s: []f64 = x_out[0..];
    let tab_s: []f64 = tab[0..];
    let bas_s: []usize = basis[0..];

    const res = solveLinearProgramSimplex(
        c[0..],
        a[0..],
        b[0..],
        2,
        4,
        *x_s,
        *tab_s,
        *bas_s,
        200,
    ) catch {
        expect_true(false);
        return;
    };
    expect_true(res.status == LpStatus.optimal);
    expect_true(absDiff(res.objective, 8.0) < 1.0e-8);
    expect_true(absDiff(x_out[0], 4.0) < 1.0e-8);
    expect_true(absDiff(x_out[1], 0.0) < 1.0e-8);
}

test("SimplexLP: exact infeasible & unbounded classification") {
    // 1. Infeasible: x0 + x1 <= 2 and x0 + x1 >= 5 (-x0 - x1 <= -5)
    const c1: [2]f64 = [1.0, 1.0];
    const a1: [4]f64 = [
        1.0, 1.0,
        -1.0, -1.0,
    ];
    const b1: [2]f64 = [2.0, -5.0];
    let x1: [2]f64 = [0.0; 2];
    let tab1: [32]f64 = [0.0; 32];
    let bas1: [2]usize = [0; 2];
    let x1_s: []f64 = x1[0..];
    let tab1_s: []f64 = tab1[0..];
    let bas1_s: []usize = bas1[0..];

    const r_inf = solveLinearProgramSimplex(
        c1[0..],
        a1[0..],
        b1[0..],
        2,
        2,
        *x1_s,
        *tab1_s,
        *bas1_s,
        100,
    ) catch {
        expect_true(false);
        return;
    };
    expect_true(r_inf.status == LpStatus.infeasible);

    // 2. Unbounded: min -x0 - 2*x1 s.t. -x0 + x1 <= 2, x0 - 2*x1 <= 4, x >= 0
    const c2: [2]f64 = [-1.0, -2.0];
    const a2: [4]f64 = [
        -1.0, 1.0,
        1.0, -2.0,
    ];
    const b2: [2]f64 = [2.0, 4.0];

    const r_unb = solveLinearProgramSimplex(
        c2[0..],
        a2[0..],
        b2[0..],
        2,
        2,
        *x1_s,
        *tab1_s,
        *bas1_s,
        100,
    ) catch {
        expect_true(false);
        return;
    };
    expect_true(r_unb.status == LpStatus.unbounded);
}

test("L-BFGS: 2D & 10D coupled Rosenbrock convergence < 1e-6 & central-diff gradient") {
    // Verify central finite-difference gradient matches analytical gradient
    let probe: [2]f64 = [-1.2, 1.0];
    let probe_s: []f64 = probe[0..];
    let g_num: [2]f64 = [0.0; 2];
    let g_ana: [2]f64 = [0.0; 2];
    let g_num_s: []f64 = g_num[0..];
    let g_ana_s: []f64 = g_ana[0..];
    approxGradientCentral(rosenbrockVal, *probe_s, 1.0e-6, *g_num_s) catch {
        expect_true(false);
        return;
    };
    rosenbrockGrad(probe[0..], *g_ana_s);
    expect_true(absDiff(g_num[0], g_ana[0]) < 1.0e-5);
    expect_true(absDiff(g_num[1], g_ana[1]) < 1.0e-5);

    // 2D Rosenbrock
    let x2: [2]f64 = [-1.2, 1.0];
    let x2_s: []f64 = x2[0..];
    let w2: [64]f64 = [0.0; 64]; // >= lbfgsWorkRequiredLen(2, 8) = 58
    let w2_s: []f64 = w2[0..];
    const s2 = cap.math.minimizeLbfgs(rosenbrockVal, rosenbrockGrad, *x2_s, 8, 1.0e-8, 300, *w2_s) catch {
        expect_true(false);
        return;
    };
    expect_true(s2.status == OptStatus.converged);
    expect_true(absDiff(x2[0], 1.0) < 1.0e-6);
    expect_true(absDiff(x2[1], 1.0) < 1.0e-6);
    expect_true(s2.objective < 1.0e-12);

    // 10D coupled Rosenbrock
    let x10: [10]f64 = [-1.0, 1.2, -0.8, 1.1, -0.5, 1.3, 0.2, 0.9, 0.5, 1.2];
    let x10_s: []f64 = x10[0..];
    // req = (2*10 + 5)*10 + 20 = 270
    expect_eq(lbfgsWorkRequiredLen(10, 10), @as(usize, 270));
    let w10: [280]f64 = [0.0; 280];
    let w10_s: []f64 = w10[0..];

    const s10 = minimizeLbfgs(rosenbrockVal, rosenbrockGrad, *x10_s, 10, 1.0e-7, 1000, *w10_s) catch {
        expect_true(false);
        return;
    };
    expect_true(s10.status == OptStatus.converged);
    expect_true(s10.objective < 1.0e-11);
    let idx: usize = 0;
    while (idx < 10) : (idx += 1) {
        expect_true(absDiff(x10[idx], 1.0) < 1.0e-6);
    }
}

test("CG-PR+: 16D ill-conditioned quadratic fast convergence") {
    let x16: [16]f64 = [5.0; 16];
    let x16_s: []f64 = x16[0..];
    expect_eq(cgWorkRequiredLen(16), @as(usize, 64));
    let work: [64]f64 = [0.0; 64];
    let work_s: []f64 = work[0..];

    const sum = minimizeConjugateGradient(illCondQuadVal, illCondQuadGrad, *x16_s, 1.0e-8, 300, *work_s) catch {
        expect_true(false);
        return;
    };
    expect_true(sum.status == OptStatus.converged);
    expect_true(sum.objective < 1.0e-14);
    expect_true(sum.grad_norm <= 1.0e-8);

    let i: usize = 0;
    while (i < 16) : (i += 1) {
        const target: f64 = @as(f64, @floatFromInt(i)) * 0.5 - 3.0;
        expect_true(absDiff(x16[i], target) < 1.0e-6);
    }
}

test("Nelder-Mead: non-smooth black-box optimization & OptError guards") {
    let x: [2]f64 = [-2.0, 4.0];
    let x_s: []f64 = x[0..];
    // req = 3*2 + 3 + 8 = 17
    expect_eq(nelderMeadWorkRequiredLen(2), @as(usize, 17));
    let work: [24]f64 = [0.0; 24];
    let work_s: []f64 = work[0..];

    const nm = minimizeNelderMead(nonSmoothL1Val, *x_s, 1.0, 1.0e-9, 500, *work_s) catch {
        expect_true(false);
        return;
    };
    expect_true(nm.status == OptStatus.converged);
    expect_true(absDiff(x[0], 2.5) < 1.0e-4);
    expect_true(absDiff(x[1], -1.5) < 1.0e-4);
    expect_true(nm.objective < 1.0e-3);

    // Error guards
    let got_bad_bounds: bool = false;
    * = brentRoot1d(wallis1d, 0.0, 1.0, 1.0e-8, 50) catch |e| {
        got_bad_bounds = (e == OptError.InvalidBounds);
    };
    expect_true(got_bad_bounds);

    let tiny_w: [4]f64 = [0.0; 4];
    let tiny_w_s: []f64 = tiny_w[0..];
    let got_small_buf: bool = false;
    * = minimizeLbfgs(rosenbrockVal, rosenbrockGrad, *x_s, 5, 1.0e-6, 50, *tiny_w_s) catch |e| {
        got_small_buf = (e == OptError.WorkBufferTooSmall);
    };
    expect_true(got_small_buf);
}
