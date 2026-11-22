// Source: cp/cap/test/test_nelder_production_deep.cp (Production deep tests for cap:nelder)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NelderConfig,
    NelderWorkspace,
    nelderMead,
    goldenSectionMin,
    brentRoot,
} from "cap:math";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

fn rosenbrock2d(x: []const f64): f64 {
    const a = 1.0 - x[0];
    const b = x[1] - x[0] * x[0];
    return a * a + 100.0 * b * b;
}

fn bowl4d(x: []const f64): f64 {
    const d0 = x[0] - 1.5;
    const d1 = x[1] + 2.0;
    const d2 = x[2] - 0.5;
    const d3 = x[3] - 4.0;
    return 2.0 * d0 * d0 + 3.0 * d1 * d1 + 1.5 * d2 * d2 + 4.0 * d3 * d3 + 7.0;
}

fn quad1d(x: f64): f64 {
    const d = x - 2.75;
    return d * d + 1.5;
}

fn cubicRootFn(x: f64): f64 {
    return x * x * x - 2.0 * x - 5.0;
}

fn cosMinusX(x: f64): f64 {
    return @cos(x) - x;
}

fn alwaysPositive(x: f64): f64 {
    return x * x + 1.0;
}

fn nanObjective(x: []const f64): f64 {
    const zero: f64 = 0.0;
    return (x[0] * zero) / zero;
}

test("1. 2D Rosenbrock banana function from (-1.2, 1.0) converges precisely to (1.0, 1.0)", () => {
    const x0 = [-1.2, 1.0];
    let x_out: [2]f64 = std.mem.zeroes([2]f64);
    let simplex: [6]f64 = std.mem.zeroes([6]f64);
    let fvals: [3]f64 = std.mem.zeroes([3]f64);
    let centroid: [2]f64 = std.mem.zeroes([2]f64);
    let xr: [2]f64 = std.mem.zeroes([2]f64);
    let xe: [2]f64 = std.mem.zeroes([2]f64);
    let xc: [2]f64 = std.mem.zeroes([2]f64);

    let ws = NelderWorkspace {
        simplex: simplex[0..],
        fvals: fvals[0..],
        centroid: centroid[0..],
        xr: xr[0..],
        xe: xe[0..],
        xc: xc[0..],
    };
    let xo_s: []f64 = x_out[0..];
    let cfg = NelderConfig.default();
    cfg.tol_f = 1.0e-12;
    cfg.tol_x = 1.0e-8;
    cfg.max_iter = 2000;

    const res = nelderMead(rosenbrock2d, x0[0..], &xo_s, cfg, &ws) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.best_value < 1.0e-10, true);
    expect_eq(absF64(x_out[0] - 1.0) < 1.0e-5, true);
    expect_eq(absF64(x_out[1] - 1.0) < 1.0e-5, true);
    expect_eq(res.evals > res.iterations, true);
});

test("2. 4D quadratic bowl converges accurately to global minimum [1.5, -2.0, 0.5, 4.0]", () => {
    const x0 = [0.0, 0.0, 0.0, 0.0];
    let x_out: [4]f64 = std.mem.zeroes([4]f64);
    let simplex: [20]f64 = std.mem.zeroes([20]f64);
    let fvals: [5]f64 = std.mem.zeroes([5]f64);
    let centroid: [4]f64 = std.mem.zeroes([4]f64);
    let xr: [4]f64 = std.mem.zeroes([4]f64);
    let xe: [4]f64 = std.mem.zeroes([4]f64);
    let xc: [4]f64 = std.mem.zeroes([4]f64);

    let ws = NelderWorkspace {
        simplex: simplex[0..],
        fvals: fvals[0..],
        centroid: centroid[0..],
        xr: xr[0..],
        xe: xe[0..],
        xc: xc[0..],
    };
    let xo_s: []f64 = x_out[0..];
    let cfg = NelderConfig.default();
    cfg.init_step = 0.5;
    cfg.tol_f = 1.0e-12;
    cfg.tol_x = 1.0e-7;
    cfg.max_iter = 4000;

    const res = nelderMead(bowl4d, x0[0..], &xo_s, cfg, &ws) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(absF64(res.best_value - 7.0) < 1.0e-10, true);
    expect_eq(absF64(x_out[0] - 1.5) < 1.0e-5, true);
    expect_eq(absF64(x_out[1] + 2.0) < 1.0e-5, true);
    expect_eq(absF64(x_out[2] - 0.5) < 1.0e-5, true);
    expect_eq(absF64(x_out[3] - 4.0) < 1.0e-5, true);
});

test("3. goldenSectionMin achieves high precision on unimodal 1D objective", () => {
    const res = goldenSectionMin(quad1d, -10.0, 10.0, 1.0e-9, 200) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(absF64(res.x - 2.75) < 1.0e-8, true);
    expect_eq(absF64(res.fval - 1.5) < 1.0e-12, true);
    expect_eq(res.iterations < 100, true);
});

test("4. brentRoot solves cubic & transcendental cos(x) - x = 0 to < 1e-12 accuracy", () => {
    // Cubic: x^3 - 2x - 5 = 0, exact root ~ 2.0945514815423265
    const r_cub = brentRoot(cubicRootFn, 2.0, 3.0, 1.0e-13, 100) ?? unreachable;
    expect_eq(r_cub.converged, true);
    expect_eq(absF64(r_cub.fval) < 1.0e-12, true);
    expect_eq(absF64(r_cub.root - 2.0945514815423265) < 1.0e-12, true);
    expect_eq(r_cub.iterations < 15, true);

    // Transcendental: cos(x) - x = 0 (Dottie number ~ 0.7390851332151607)
    const r_cos = brentRoot(cosMinusX, 0.0, 1.0, 1.0e-13, 100) ?? unreachable;
    expect_eq(r_cos.converged, true);
    expect_eq(absF64(r_cos.fval) < 1.0e-12, true);
    expect_eq(absF64(r_cos.root - 0.7390851332151607) < 1.0e-12, true);
    expect_eq(r_cos.iterations < 15, true);
});

test("5. brentRoot endpoint exact root fast-path & same-sign bracket rejection", () => {
    // Same-sign endpoints cleanly rejected with null
    expect_eq(brentRoot(alwaysPositive, -5.0, 5.0, 1.0e-10, 100) == null, true);
    expect_eq(brentRoot(cubicRootFn, 0.0, 1.0, 1.0e-10, 100) == null, true);

    // Reversed bracket [1.0, 0.0] also works cleanly
    const r_rev = brentRoot(cosMinusX, 1.0, 0.0, 1.0e-12, 100) ?? unreachable;
    expect_eq(r_rev.converged, true);
    expect_eq(absF64(r_rev.fval) < 1.0e-11, true);
});

test("6. Defensive validation: n==0, undersized workspace, invalid brackets & NaN rejection", () => {
    const empty_x0: [0]f64 = [];
    const x0 = [1.0, 2.0];
    let x_out: [2]f64 = std.mem.zeroes([2]f64);
    let simplex_ok: [6]f64 = std.mem.zeroes([6]f64);
    let simplex_short: [4]f64 = std.mem.zeroes([4]f64);
    let fvals: [3]f64 = std.mem.zeroes([3]f64);
    let c: [2]f64 = std.mem.zeroes([2]f64);
    let xr: [2]f64 = std.mem.zeroes([2]f64);
    let xe: [2]f64 = std.mem.zeroes([2]f64);
    let xc: [2]f64 = std.mem.zeroes([2]f64);

    let ws_ok = NelderWorkspace {
        simplex: simplex_ok[0..],
        fvals: fvals[0..],
        centroid: c[0..],
        xr: xr[0..],
        xe: xe[0..],
        xc: xc[0..],
    };
    let ws_short = NelderWorkspace {
        simplex: simplex_short[0..],
        fvals: fvals[0..],
        centroid: c[0..],
        xr: xr[0..],
        xe: xe[0..],
        xc: xc[0..],
    };
    let xo_s: []f64 = x_out[0..];
    const cfg = NelderConfig.default();

    // n == 0 rejected
    expect_eq(nelderMead(rosenbrock2d, empty_x0[0..], &xo_s, cfg, &ws_ok) == null, true);
    // Undersized simplex buffer rejected
    expect_eq(nelderMead(rosenbrock2d, x0[0..], &xo_s, cfg, &ws_short) == null, true);
    // NaN returned by objective rejected
    expect_eq(nelderMead(nanObjective, x0[0..], &xo_s, cfg, &ws_ok) == null, true);
    // NaN initial guess rejected
    const zero: f64 = 0.0;
    const nan_x0 = [zero / zero, 1.0];
    expect_eq(nelderMead(rosenbrock2d, nan_x0[0..], &xo_s, cfg, &ws_ok) == null, true);

    // goldenSectionMin & brentRoot invalid brackets / tolerances rejected
    expect_eq(goldenSectionMin(quad1d, 5.0, 2.0, 1.0e-6, 100) == null, true);
    expect_eq(goldenSectionMin(quad1d, 0.0, 5.0, 0.0, 100) == null, true);
    expect_eq(brentRoot(cosMinusX, 1.0, 1.0, 1.0e-6, 100) == null, true);
    expect_eq(brentRoot(cosMinusX, 0.0, 1.0, -1.0, 100) == null, true);
});
