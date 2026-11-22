// Source: cp/cap/test/test_newton_production_deep.cp (Production deep tests for cap:newton)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NewtonConfig,
    NewtonWorkspace,
    LmConfig,
    LmWorkspace,
    newtonSystem,
    levenbergMarquardt,
    newton1D,
    secant1D,
} from "cap:math";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

// 2D circle & hyperbola intersection: x^2 + y^2 = 4, x*y = 1
fn circleHyperbola(x: []const f64, f_out: *[]f64) {
    f_out.*[0] = x[0] * x[0] + x[1] * x[1] - 4.0;
    f_out.*[1] = x[0] * x[1] - 1.0;
}

// 3D coupled nonlinear system (Burden & Faires)
fn coupled3D(x: []const f64, f_out: *[]f64) {
    const pi: f64 = 3.14159265358979323846;
    f_out.*[0] = 3.0 * x[0] - @cos(x[1] * x[2]) - 0.5;
    f_out.*[1] = x[0] * x[0] - 81.0 * (x[1] + 0.1) * (x[1] + 0.1) + @sin(x[2]) + 1.06;
    f_out.*[2] = @exp(0.0 - x[0] * x[1]) + 20.0 * x[2] + (10.0 * pi - 3.0) / 3.0;
}

// Singular 2D system: parallel lines (rank-1 Jacobian everywhere)
fn singular2D(x: []const f64, f_out: *[]f64) {
    f_out.*[0] = x[0] + x[1] - 1.0;
    f_out.*[1] = 2.0 * x[0] + 2.0 * x[1] - 3.0;
}

// Exponential decay residual: y(t) = a * exp(-b * t) + c
// True parameters: a = 2.5, b = 0.7, c = 0.4 on t = 0.0, 0.5, ..., 4.5 (10 samples)
fn expDecayResidual(p: []const f64, r_out: *[]f64) {
    const a_true: f64 = 2.5;
    const b_true: f64 = 0.7;
    const c_true: f64 = 0.4;
    let i: usize = 0;
    while (i < 10) {
        const t = (i as f64) * 0.5;
        const y_obs = a_true * @exp((0.0 - b_true) * t) + c_true;
        const y_pred = p[0] * @exp((0.0 - p[1]) * t) + p[2];
        r_out.*[i] = y_pred - y_obs;
        i += 1;
    }
}

// Rosenbrock residual vector: r0 = 1 - x, r1 = 10 * (y - x^2), unique minimum at (1, 1)
fn rosenbrockResidual(x: []const f64, r_out: *[]f64) {
    r_out.*[0] = 1.0 - x[0];
    r_out.*[1] = 10.0 * (x[1] - x[0] * x[0]);
}

// Wallis cubic: f(x) = x^3 - 2x - 5
fn wallisF(x: f64): f64 {
    return x * x * x - 2.0 * x - 5.0;
}

fn wallisDf(x: f64): f64 {
    return 3.0 * x * x - 2.0;
}

// Parabolic test for zero derivative / flat secant defense
fn parabolaF(x: f64): f64 {
    return x * x - 1.0;
}

fn parabolaDf(x: f64): f64 {
    return 2.0 * x;
}

test("1. 2D circle & hyperbola nonlinear system converges quadratically (< 1e-12)", () => {
    const x0: [2]f64 = [1.8, 0.3];
    let x_buf: [2]f64 = [0.0; 2];
    let x_out: []f64 = x_buf[0..];

    let fx: [2]f64 = [0.0; 2];
    let fxt: [2]f64 = [0.0; 2];
    let jac: [4]f64 = [0.0; 4];
    let dx: [2]f64 = [0.0; 2];
    let xt: [2]f64 = [0.0; 2];
    let piv: [2]usize = [0; 2];

    let ws: NewtonWorkspace = {
        fx: fx[0..],
        fx_trial: fxt[0..],
        jac: jac[0..],
        dx: dx[0..],
        x_trial: xt[0..],
        piv: piv[0..],
    };

    if (res := newtonSystem(circleHyperbola, x0[0..], *x_out, NewtonConfig.default(), *ws)) {
        expect_eq(res.converged, true);
        expect_eq(res.residual_norm < 1.0e-12, true);
        expect_eq(res.iterations <= 15, true);
        expect_eq(res.evals > 0, true);

        const eq1 = x_out[0] * x_out[0] + x_out[1] * x_out[1] - 4.0;
        const eq2 = x_out[0] * x_out[1] - 1.0;
        expect_eq(absF64(eq1) < 1.0e-12, true);
        expect_eq(absF64(eq2) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }
});

test("2. 3D coupled nonlinear system converges with Armijo damping", () => {
    const x0: [3]f64 = [0.5, 0.5, -0.1];
    let x_buf: [3]f64 = [0.0; 3];
    let x_out: []f64 = x_buf[0..];

    let fx: [3]f64 = [0.0; 3];
    let fxt: [3]f64 = [0.0; 3];
    let jac: [9]f64 = [0.0; 9];
    let dx: [3]f64 = [0.0; 3];
    let xt: [3]f64 = [0.0; 3];
    let piv: [3]usize = [0; 3];

    let ws: NewtonWorkspace = {
        fx: fx[0..],
        fx_trial: fxt[0..],
        jac: jac[0..],
        dx: dx[0..],
        x_trial: xt[0..],
        piv: piv[0..],
    };

    if (res := newtonSystem(coupled3D, x0[0..], *x_out, NewtonConfig.default(), *ws)) {
        expect_eq(res.converged, true);
        expect_eq(res.residual_norm < 1.0e-11, true);
        // Known root is approximately (0.5, 0.0, -0.5235987756)
        expect_eq(absF64(x_out[0] - 0.5) < 1.0e-8, true);
        expect_eq(absF64(x_out[1] - 0.0) < 1.0e-8, true);
        expect_eq(absF64(x_out[2] + 0.5235987755982988) < 1.0e-8, true);
    } else {
        expect_eq(true, false);
    }
});

test("3. Levenberg-Marquardt recovers 3 parameters of exponential decay y = a*exp(-b*t) + c (< 1e-8)", () => {
    const x0: [3]f64 = [1.0, 0.2, 0.0];
    let x_buf: [3]f64 = [0.0; 3];
    let x_out: []f64 = x_buf[0..];

    let r: [10]f64 = [0.0; 10];
    let rt: [10]f64 = [0.0; 10];
    let jac: [30]f64 = [0.0; 30];
    let jtj: [9]f64 = [0.0; 9];
    let jtr: [3]f64 = [0.0; 3];
    let dx: [3]f64 = [0.0; 3];
    let xt: [3]f64 = [0.0; 3];
    let piv: [3]usize = [0; 3];

    let ws: LmWorkspace = {
        r: r[0..],
        r_trial: rt[0..],
        jac: jac[0..],
        jtj: jtj[0..],
        jtr: jtr[0..],
        dx: dx[0..],
        x_trial: xt[0..],
        piv: piv[0..],
    };

    if (res := levenbergMarquardt(expDecayResidual, 10, x0[0..], *x_out, LmConfig.default(), *ws)) {
        expect_eq(res.converged, true);
        expect_eq(res.residual_norm < 1.0e-10, true);
        expect_eq(res.cost < 1.0e-20, true);
        expect_eq(absF64(x_out[0] - 2.5) < 1.0e-8, true);
        expect_eq(absF64(x_out[1] - 0.7) < 1.0e-8, true);
        expect_eq(absF64(x_out[2] - 0.4) < 1.0e-8, true);
    } else {
        expect_eq(true, false);
    }
});

test("4. Levenberg-Marquardt solves Rosenbrock residual from (-1.2, 1.0) to (1.0, 1.0)", () => {
    const x0: [2]f64 = [-1.2, 1.0];
    let x_buf: [2]f64 = [0.0; 2];
    let x_out: []f64 = x_buf[0..];

    let r: [2]f64 = [0.0; 2];
    let rt: [2]f64 = [0.0; 2];
    let jac: [4]f64 = [0.0; 4];
    let jtj: [4]f64 = [0.0; 4];
    let jtr: [2]f64 = [0.0; 2];
    let dx: [2]f64 = [0.0; 2];
    let xt: [2]f64 = [0.0; 2];
    let piv: [2]usize = [0; 2];

    let ws: LmWorkspace = {
        r: r[0..],
        r_trial: rt[0..],
        jac: jac[0..],
        jtj: jtj[0..],
        jtr: jtr[0..],
        dx: dx[0..],
        x_trial: xt[0..],
        piv: piv[0..],
    };

    if (res := levenbergMarquardt(rosenbrockResidual, 2, x0[0..], *x_out, LmConfig.default(), *ws)) {
        expect_eq(res.converged, true);
        expect_eq(res.residual_norm < 1.0e-10, true);
        expect_eq(absF64(x_out[0] - 1.0) < 1.0e-9, true);
        expect_eq(absF64(x_out[1] - 1.0) < 1.0e-9, true);
    } else {
        expect_eq(true, false);
    }
});

test("5. newton1D & secant1D solve x^3 - 2x - 5 = 0 to machine precision", () => {
    const true_root: f64 = 2.0945514815423265;

    if (rn := newton1D(wallisF, wallisDf, 2.0, 1.0e-14, 50)) {
        expect_eq(rn.converged, true);
        expect_eq(absF64(rn.fval) < 1.0e-13, true);
        expect_eq(absF64(rn.root - true_root) < 1.0e-13, true);
        expect_eq(rn.iterations <= 10, true);
    } else {
        expect_eq(true, false);
    }

    if (rs := secant1D(wallisF, 2.0, 3.0, 1.0e-14, 50)) {
        expect_eq(rs.converged, true);
        expect_eq(absF64(rs.fval) < 1.0e-13, true);
        expect_eq(absF64(rs.root - true_root) < 1.0e-13, true);
        expect_eq(rs.iterations <= 15, true);
    } else {
        expect_eq(true, false);
    }
});

test("6. Boundary defense: n == 0, m < n, singular Jacobian, short buffers, NaN & zero derivative", () => {
    const nan = std.math.nan(f64);
    const x0: [2]f64 = [1.0, 1.0];
    const x_nan: [2]f64 = [1.0, nan];
    let x_buf: [2]f64 = [0.0; 2];
    let x_out: []f64 = x_buf[0..];

    let fx: [2]f64 = [0.0; 2];
    let fxt: [2]f64 = [0.0; 2];
    let jac: [4]f64 = [0.0; 4];
    let short_f: [1]f64 = [0.0];
    let dx: [2]f64 = [0.0; 2];
    let xt: [2]f64 = [0.0; 2];
    let piv: [2]usize = [0; 2];
    let short_piv: [1]usize = [0];

    let ws_ok: NewtonWorkspace = {
        fx: fx[0..], fx_trial: fxt[0..], jac: jac[0..],
        dx: dx[0..], x_trial: xt[0..], piv: piv[0..],
    };
    let ws_short_jac: NewtonWorkspace = {
        fx: fx[0..], fx_trial: fxt[0..], jac: short_f[0..],
        dx: dx[0..], x_trial: xt[0..], piv: piv[0..],
    };
    let ws_short_piv: NewtonWorkspace = {
        fx: fx[0..], fx_trial: fxt[0..], jac: jac[0..],
        dx: dx[0..], x_trial: xt[0..], piv: short_piv[0..],
    };

    const ncfg = NewtonConfig.default();
    // n == 0, NaN x0, singular Jacobian, short workspace
    expect_eq(newtonSystem(circleHyperbola, x0[0..0], *x_out, ncfg, *ws_ok) == null, true);
    expect_eq(newtonSystem(circleHyperbola, x_nan[0..], *x_out, ncfg, *ws_ok) == null, true);
    expect_eq(newtonSystem(singular2D, x0[0..], *x_out, ncfg, *ws_ok) == null, true);
    expect_eq(newtonSystem(circleHyperbola, x0[0..], *x_out, ncfg, *ws_short_jac) == null, true);
    expect_eq(newtonSystem(circleHyperbola, x0[0..], *x_out, ncfg, *ws_short_piv) == null, true);

    // LM boundary: m < n, n == 0, short workspace, NaN x0
    let r: [2]f64 = [0.0; 2];
    let rt: [2]f64 = [0.0; 2];
    let jtj: [4]f64 = [0.0; 4];
    let jtr: [2]f64 = [0.0; 2];
    let lm_ws_ok: LmWorkspace = {
        r: r[0..], r_trial: rt[0..], jac: jac[0..], jtj: jtj[0..],
        jtr: jtr[0..], dx: dx[0..], x_trial: xt[0..], piv: piv[0..],
    };
    let lm_ws_short: LmWorkspace = {
        r: r[0..], r_trial: rt[0..], jac: jac[0..], jtj: short_f[0..],
        jtr: jtr[0..], dx: dx[0..], x_trial: xt[0..], piv: piv[0..],
    };
    const lcfg = LmConfig.default();
    expect_eq(levenbergMarquardt(rosenbrockResidual, 1, x0[0..], *x_out, lcfg, *lm_ws_ok) == null, true);
    expect_eq(levenbergMarquardt(rosenbrockResidual, 2, x0[0..0], *x_out, lcfg, *lm_ws_ok) == null, true);
    expect_eq(levenbergMarquardt(rosenbrockResidual, 2, x_nan[0..], *x_out, lcfg, *lm_ws_ok) == null, true);
    expect_eq(levenbergMarquardt(rosenbrockResidual, 2, x0[0..], *x_out, lcfg, *lm_ws_short) == null, true);

    // 1D boundary: zero derivative, flat secant, NaN, max_iter == 0
    expect_eq(newton1D(parabolaF, parabolaDf, 0.0, 1.0e-12, 20) == null, true);
    expect_eq(newton1D(wallisF, wallisDf, nan, 1.0e-12, 20) == null, true);
    expect_eq(newton1D(wallisF, wallisDf, 2.0, 1.0e-12, 0) == null, true);
    expect_eq(secant1D(parabolaF, -2.0, 2.0, 1.0e-12, 20) == null, true);
    expect_eq(secant1D(wallisF, nan, 2.0, 1.0e-12, 20) == null, true);
});
