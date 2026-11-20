// Source: cp/cap/test/test_optimize_production_deep.cp
// Deep Production Verification Suite for cap:optimize & cap.optimize
// Covers:
//   1. bisect & brentRoot on nonlinear equations (cos(x)-x=0, x^3-x-2=0), verifying Brent << bisect iterations & < 1e-10 accuracy
//   2. goldenSectionMin on unimodal functions + InvalidInterval / SameSignBracket / InvalidTolerance error interception
//   3. nelderMead derivative-free optimization on 2D Rosenbrock & elliptic quadratic -> (1, 1) within 1e-4
//   4. adamMinimize (analytical gradient & null numerical gradient) + numericalGradient + std.testing.allocator zero-leak
//   5. `import cap from "cap"; cap.optimize` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    OptError,
    ScalarFn,
    VectorFn,
    GradFn,
    RootResult,
    MinResult1D,
    OptResult,
    Optimizer,
} from "cap:optimize";

fn expectNear(actual: f64, expected: f64, tol: f64) {
    expect_eq(@abs(actual - expected) <= tol, true);
}

fn fnCosMinusX(ctx: *anyopaque, x: f64): f64 {
    * = ctx;
    return @cos(x) - x;
}

fn fnCubic(ctx: *anyopaque, x: f64): f64 {
    * = ctx;
    return x * x * x - x - 2.0;
}

fn fnUnimodalQuad(ctx: *anyopaque, x: f64): f64 {
    * = ctx;
    const d = x - 2.5;
    return d * d + 1.0;
}

fn fnUnimodalPoly(ctx: *anyopaque, x: f64): f64 {
    * = ctx;
    return x * x * x * x - 3.0 * x * x * x + 2.0;
}

fn fnRosenbrock(ctx: *anyopaque, x: []const f64): f64 {
    * = ctx;
    const a = 1.0 - x[0];
    const b = x[1] - x[0] * x[0];
    return a * a + 100.0 * b * b;
}

fn fnEllipticQuad(ctx: *anyopaque, x: []const f64): f64 {
    * = ctx;
    const u = x[0] - 1.0;
    const v = x[1] - 1.0;
    return 4.0 * u * u + v * v + 3.0 * u * v;
}

fn fnQuad3D(ctx: *anyopaque, x: []const f64): f64 {
    * = ctx;
    const d0 = x[0] - 1.0;
    const d1 = x[1] + 2.0;
    const d2 = x[2] - 3.0;
    return d0 * d0 + 2.0 * d1 * d1 + 3.0 * d2 * d2;
}

fn gradQuad3D(ctx: *anyopaque, x: []const f64, out_grad: []f64) {
    * = ctx;
    out_grad[0] = 2.0 * (x[0] - 1.0);
    out_grad[1] = 4.0 * (x[1] + 2.0);
    out_grad[2] = 6.0 * (x[2] - 3.0);
}

test("cap:optimize - bisect & brentRoot nonlinear equations with superlinear speedup") {
    let dummy: u8 = 0;
    const ctx: *anyopaque = unsafe { *dummy as *anyopaque };
    const tol: f64 = 0.000000000001;

    // 1. cos(x) - x = 0 on [0.0, 1.0], Dottie number ≈ 0.7390851332151607
    const r_bis1 = Optimizer.bisect(ctx, fnCosMinusX, 0.0, 1.0, tol, 200) catch unreachable;
    const r_br1 = Optimizer.brentRoot(ctx, fnCosMinusX, 0.0, 1.0, tol, 200) catch unreachable;

    expect_eq(r_bis1.converged, true);
    expect_eq(r_br1.converged, true);
    expectNear(r_bis1.root, 0.7390851332151607, 0.0000000001);
    expectNear(r_br1.root, 0.7390851332151607, 0.0000000001);
    expect_eq(@abs(r_br1.f_val) < 0.0000000001, true);
    expect_eq(r_br1.iterations < r_bis1.iterations, true);

    // 2. x^3 - x - 2 = 0 on [1.0, 2.0], root ≈ 1.5213797068045676
    const r_bis2 = Optimizer.bisect(ctx, fnCubic, 1.0, 2.0, tol, 200) catch unreachable;
    const r_br2 = Optimizer.brentRoot(ctx, fnCubic, 1.0, 2.0, tol, 200) catch unreachable;

    expectNear(r_bis2.root, 1.5213797068045676, 0.0000000001);
    expectNear(r_br2.root, 1.5213797068045676, 0.0000000001);
    expect_eq(@abs(r_br2.f_val) < 0.0000000001, true);
    expect_eq(r_br2.iterations < r_bis2.iterations, true);
}

test("cap:optimize - goldenSectionMin & SameSignBracket / InvalidInterval interception") {
    let dummy: u8 = 0;
    const ctx: *anyopaque = unsafe { *dummy as *anyopaque };

    // 1. Quadratic unimodal: (x - 2.5)^2 + 1.0 on [0, 5] -> x = 2.5, f = 1.0
    const m1 = Optimizer.goldenSectionMin(ctx, fnUnimodalQuad, 0.0, 5.0, 0.000000001, 200) catch unreachable;
    expect_eq(m1.converged, true);
    expectNear(m1.x, 2.5, 0.00000001);
    expectNear(m1.f_val, 1.0, 0.00000001);

    // 2. Quartic unimodal on [1.0, 3.0]: x^4 - 3x^3 + 2 -> min at x = 2.25
    const m2 = Optimizer.goldenSectionMin(ctx, fnUnimodalPoly, 1.0, 3.0, 0.000000001, 200) catch unreachable;
    expect_eq(m2.converged, true);
    expectNear(m2.x, 2.25, 0.0000001);

    // 3. Error interceptions
    const same_err = Optimizer.bisect(ctx, fnCubic, 2.0, 3.0, 0.000001, 100);
    expect_eq(same_err == OptError.SameSignBracket, true);

    const same_brent_err = Optimizer.brentRoot(ctx, fnCubic, 2.0, 3.0, 0.000001, 100);
    expect_eq(same_brent_err == OptError.SameSignBracket, true);

    const inv_bis_err = Optimizer.bisect(ctx, fnCubic, 2.0, 1.0, 0.000001, 100);
    expect_eq(inv_bis_err == OptError.InvalidInterval, true);

    const inv_gold_err = Optimizer.goldenSectionMin(ctx, fnUnimodalQuad, 3.0, 3.0, 0.000001, 100);
    expect_eq(inv_gold_err == OptError.InvalidInterval, true);

    const tol_err = Optimizer.goldenSectionMin(ctx, fnUnimodalQuad, 0.0, 5.0, 0.0, 100);
    expect_eq(tol_err == OptError.InvalidTolerance, true);
}

test("cap:optimize - nelderMead derivative-free Rosenbrock & elliptic quadratic") {
    const alloc = std.testing.allocator;
    let dummy: u8 = 0;
    const ctx: *anyopaque = unsafe { *dummy as *anyopaque };

    // 1. 2D Rosenbrock banana function from [-1.2, 1.0] -> (1.0, 1.0)
    const x0 = [-1.2, 1.0];
    let res1 = Optimizer.nelderMead(alloc, ctx, fnRosenbrock, &x0, 0.25, 0.00000001, 3000) catch unreachable;
    defer res1.deinit(alloc);

    expect_eq(res1.converged, true);
    expectNear(res1.x[0], 1.0, 0.0001);
    expectNear(res1.x[1], 1.0, 0.0001);
    expect_eq(res1.f_val < 0.000001, true);

    // 2. Elliptic quadratic from [-3.0, 4.0] -> (1.0, 1.0)
    const x1 = [-3.0, 4.0];
    let res2 = Optimizer.nelderMead(alloc, ctx, fnEllipticQuad, &x1, 0.5, 0.00000001, 2000) catch unreachable;
    defer res2.deinit(alloc);

    expect_eq(res2.converged, true);
    expectNear(res2.x[0], 1.0, 0.0001);
    expectNear(res2.x[1], 1.0, 0.0001);
    expect_eq(res2.f_val < 0.000001, true);
}

test("cap:optimize - adamMinimize (analytical & numerical gradient) + zero leak") {
    const alloc = std.testing.allocator;
    let dummy: u8 = 0;
    const ctx: *anyopaque = unsafe { *dummy as *anyopaque };

    // Direct numericalGradient check at [2.0, -1.0, 5.0]:
    // grad = [2*(2-1), 4*(-1+2), 6*(5-3)] = [2.0, 4.0, 12.0]
    const pt = [2.0, -1.0, 5.0];
    let g: [3]f64 = [0.0, 0.0, 0.0];
    Optimizer.numericalGradient(ctx, fnQuad3D, &pt, 0.000001, &g) catch unreachable;
    expectNear(g[0], 2.0, 0.000001);
    expectNear(g[1], 4.0, 0.000001);
    expectNear(g[2], 12.0, 0.000001);

    const x0 = [-4.0, 5.0, -2.0];

    // 1. Analytical gradient mode
    let res_ana = Optimizer.adamMinimize(alloc, ctx, fnQuad3D, gradQuad3D, &x0, 0.05, 0.00001, 5000) catch unreachable;
    defer res_ana.deinit(alloc);
    expect_eq(res_ana.converged, true);
    expectNear(res_ana.x[0], 1.0, 0.0001);
    expectNear(res_ana.x[1], -2.0, 0.0001);
    expectNear(res_ana.x[2], 3.0, 0.0001);
    expect_eq(res_ana.f_val < 0.000001, true);

    // 2. Automatic central-difference numerical gradient mode (grad_fn_opt = null)
    let res_num = Optimizer.adamMinimize(alloc, ctx, fnQuad3D, null, &x0, 0.05, 0.00001, 5000) catch unreachable;
    defer res_num.deinit(alloc);
    expect_eq(res_num.converged, true);
    expectNear(res_num.x[0], 1.0, 0.0001);
    expectNear(res_num.x[1], -2.0, 0.0001);
    expectNear(res_num.x[2], 3.0, 0.0001);
    expect_eq(res_num.f_val < 0.000001, true);
}

test("cap:optimize - dynamic mounting via cap.optimize") {
    const alloc = std.testing.allocator;
    let dummy: u8 = 0;
    const ctx: *anyopaque = unsafe { *dummy as *anyopaque };

    const r = cap.optimize.Optimizer.brentRoot(ctx, fnCosMinusX, 0.0, 1.0, 0.000000000001, 100) catch unreachable;
    expect_eq(r.converged, true);
    expectNear(r.root, 0.7390851332151607, 0.0000000001);

    const x0 = [0.0, 0.0];
    let nm = cap.optimize.Optimizer.nelderMead(alloc, ctx, fnRosenbrock, &x0, 0.2, 0.00000001, 2000) catch unreachable;
    defer nm.deinit(alloc);
    expect_eq(nm.converged, true);
    expectNear(nm.x[0], 1.0, 0.0001);
    expectNear(nm.x[1], 1.0, 0.0001);
}
