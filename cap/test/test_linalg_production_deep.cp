// Source: cp/cap/test/test_linalg_production_deep.cp
// Deep Production Verification Suite for cap:linalg & cap.linalg
// Covers:
//   1. Matrix construction, identity, transpose, mul, add/sub/scale, trace, frobeniusNorm & DimensionMismatch interception
//   2. Partial-pivoting LU solve (including 0-pivot row swap) & back-substitution residual ||Ax - b||_inf < 1e-9
//   3. determinant, inverse (A * A^-1 ≈ I), SingularMatrix & NotSquare error interception
//   4. Householder QR leastSquares overdetermined fit for y = 2x + 3
//   5. `import cap from "cap"; cap.linalg` dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    LinalgError,
    Matrix,
    dot,
    vecNorm1,
    vecNorm2,
    vecNormInf,
    leastSquares,
} from "cap:ml";

fn expectNear(actual: f64, expected: f64, tol: f64) {
    expect_eq(@abs(actual - expected) <= tol, true);
}

test("cap:linalg - Matrix ops, trace, frobeniusNorm & DimensionMismatch interception") {
    const alloc = std.testing.allocator;

    let id = Matrix.identity(alloc, 3) catch unreachable;
    defer id.deinit();
    expectNear(id.trace() catch unreachable, 3.0, 0.000000000001);
    expectNear(id.frobeniusNorm(), @sqrt(3.0), 0.000000000001);

    let a = Matrix.fromSlice(alloc, 2, 3, &[
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
    ]) catch unreachable;
    defer a.deinit();

    let at = a.transpose(alloc) catch unreachable;
    defer at.deinit();
    expect_eq(at.rows, 3);
    expect_eq(at.cols, 2);
    expectNear(at.get(0, 1), 4.0, 0.000000000001);
    expectNear(at.get(2, 0), 3.0, 0.000000000001);

    // (2x3) * (3x2) = (2x2)
    let prod = a.mul(alloc, at) catch unreachable;
    defer prod.deinit();
    expect_eq(prod.rows, 2);
    expect_eq(prod.cols, 2);
    expectNear(prod.get(0, 0), 14.0, 0.000000000001);
    expectNear(prod.get(0, 1), 32.0, 0.000000000001);
    expectNear(prod.get(1, 0), 32.0, 0.000000000001);
    expectNear(prod.get(1, 1), 77.0, 0.000000000001);
    expectNear(prod.trace() catch unreachable, 91.0, 0.000000000001);

    // add / sub / scale / clone / set
    let c = prod.clone(alloc) catch unreachable;
    defer c.deinit();
    let doubled = prod.add(alloc, c) catch unreachable;
    defer doubled.deinit();
    let scaled = prod.scale(alloc, 2.0) catch unreachable;
    defer scaled.deinit();
    expectNear(doubled.maxAbsDiff(scaled) catch unreachable, 0.0, 0.000000000001);

    let diff = doubled.sub(alloc, scaled) catch unreachable;
    defer diff.deinit();
    expectNear(diff.frobeniusNorm(), 0.0, 0.000000000001);

    // dot & vector norms
    const u = [3.0, -4.0];
    const v = [1.0, 2.0];
    expectNear(dot(&u, &v) catch unreachable, -5.0, 0.000000000001);
    expectNear(vecNorm1(&u), 7.0, 0.000000000001);
    expectNear(vecNorm2(&u), 5.0, 0.000000000001);
    expectNear(vecNormInf(&u), 4.0, 0.000000000001);

    // DimensionMismatch & InvalidDimensions interception
    const mul_err = a.mul(alloc, a);
    expect_eq(mul_err == LinalgError.DimensionMismatch, true);
    const add_err = a.add(alloc, at);
    expect_eq(add_err == LinalgError.DimensionMismatch, true);
    const zero_err = Matrix.zeros(alloc, 0, 3);
    expect_eq(zero_err == LinalgError.InvalidDimensions, true);
}

test("cap:linalg - Partial-pivoting LU solve with zero pivot & residual check") {
    const alloc = std.testing.allocator;

    // Matrix with 0.0 on the leading diagonal (requires row swap!)
    // [ 0  2  1 ] [x0]   [ 5]
    // [ 1 -2 -3 ] [x1] = [-6]  => true x = [1, 2, 1]
    // [-1  1  2 ] [x2]   [ 3]
    let a = Matrix.fromSlice(alloc, 3, 3, &[
        0.0, 2.0, 1.0,
        1.0, -2.0, -3.0,
        -1.0, 1.0, 2.0,
    ]) catch unreachable;
    defer a.deinit();

    const b = [5.0, -6.0, 3.0];
    const x = a.solve(alloc, &b) catch unreachable;
    defer alloc.free(x);

    expectNear(x[0], 1.0, 0.000000001);
    expectNear(x[1], 2.0, 0.000000001);
    expectNear(x[2], 1.0, 0.000000001);

    // Verify back-substitution residual ||Ax - b||_inf < 1e-9
    const ax = a.mulVec(alloc, x) catch unreachable;
    defer alloc.free(ax);
    let res = [ax[0] - b[0], ax[1] - b[1], ax[2] - b[2]];
    expect_eq(vecNormInf(&res) < 0.000000001, true);

    // 4x4 system with multiple zero pivots
    let a4 = Matrix.fromSlice(alloc, 4, 4, &[
        0.0, 0.0, 2.0, 1.0,
        0.0, 3.0, -1.0, 2.0,
        4.0, 1.0, 0.0, -1.0,
        1.0, 2.0, 1.0, 1.0,
    ]) catch unreachable;
    defer a4.deinit();

    const x_true = [2.0, -1.0, 3.0, 0.5];
    const b4 = a4.mulVec(alloc, &x_true) catch unreachable;
    defer alloc.free(b4);

    const x4 = a4.solve(alloc, b4) catch unreachable;
    defer alloc.free(x4);

    const ax4 = a4.mulVec(alloc, x4) catch unreachable;
    defer alloc.free(ax4);
    let res4 = [ax4[0] - b4[0], ax4[1] - b4[1], ax4[2] - b4[2], ax4[3] - b4[3]];
    expect_eq(vecNormInf(&res4) < 0.000000001, true);
}

test("cap:linalg - determinant, inverse (A * A^-1 ≈ I), SingularMatrix & NotSquare") {
    const alloc = std.testing.allocator;

    let a = Matrix.fromSlice(alloc, 3, 3, &[
        1.0, 2.0, 3.0,
        0.0, 1.0, 4.0,
        5.0, 6.0, 0.0,
    ]) catch unreachable;
    defer a.deinit();

    // det = 1*(0 - 24) - 2*(0 - 20) + 3*(0 - 5) = -24 + 40 - 15 = 1
    const det = a.determinant(alloc) catch unreachable;
    expectNear(det, 1.0, 0.000000001);

    let inv = a.inverse(alloc) catch unreachable;
    defer inv.deinit();

    let prod = a.mul(alloc, inv) catch unreachable;
    defer prod.deinit();

    let id = Matrix.identity(alloc, 3) catch unreachable;
    defer id.deinit();

    const err_norm = prod.maxAbsDiff(id) catch unreachable;
    expect_eq(err_norm < 0.000000001, true);

    // Singular matrix interception
    let sing = Matrix.fromSlice(alloc, 2, 2, &[
        1.0, 2.0,
        2.0, 4.0,
    ]) catch unreachable;
    defer sing.deinit();

    expectNear(sing.determinant(alloc) catch unreachable, 0.0, 0.000000001);
    const rhs = [1.0, 2.0];
    expect_eq(sing.solve(alloc, &rhs) == LinalgError.SingularMatrix, true);
    expect_eq(sing.inverse(alloc) == LinalgError.SingularMatrix, true);

    // NotSquare interception
    let rect = Matrix.zeros(alloc, 2, 3) catch unreachable;
    defer rect.deinit();
    expect_eq(rect.trace() == LinalgError.NotSquare, true);
    expect_eq(rect.determinant(alloc) == LinalgError.NotSquare, true);
    expect_eq(rect.inverse(alloc) == LinalgError.NotSquare, true);
    expect_eq(rect.solve(alloc, &rhs) == LinalgError.NotSquare, true);
}

test("cap:linalg - leastSquares overdetermined linear fit y = 2x + 3") {
    const alloc = std.testing.allocator;

    // Exact overdetermined fit: 5 points on y = 2x + 3
    let design = Matrix.fromSlice(alloc, 5, 2, &[
        0.0, 1.0,
        1.0, 1.0,
        2.0, 1.0,
        3.0, 1.0,
        4.0, 1.0,
    ]) catch unreachable;
    defer design.deinit();

    const y = [3.0, 5.0, 7.0, 9.0, 11.0];
    const beta = leastSquares(alloc, design, &y) catch unreachable;
    defer alloc.free(beta);

    expectNear(beta[0], 2.0, 0.000000001);
    expectNear(beta[1], 3.0, 0.000000001);

    // Noisy overdetermined fit: x = [1, 2, 3, 4], y = [5.1, 6.9, 9.1, 10.9]
    // Analytical OLS: mean_x=2.5, mean_y=8.0, sum((x-mx)^2)=5.0, sum((x-mx)(y-my))=9.9 => slope=1.98, intercept=3.05
    let design2 = Matrix.fromSlice(alloc, 4, 2, &[
        1.0, 1.0,
        2.0, 1.0,
        3.0, 1.0,
        4.0, 1.0,
    ]) catch unreachable;
    defer design2.deinit();

    const y2 = [5.1, 6.9, 9.1, 10.9];
    const beta2 = Matrix.leastSquares(alloc, design2, &y2) catch unreachable;
    defer alloc.free(beta2);

    expectNear(beta2[0], 1.98, 0.000000001);
    expectNear(beta2[1], 3.05, 0.000000001);
}

test("cap:linalg - dynamic mounting via cap.linalg") {
    const alloc = std.testing.allocator;

    let m = cap.ml.Matrix.fromSlice(alloc, 2, 2, &[
        2.0, 1.0,
        1.0, 3.0,
    ]) catch unreachable;
    defer m.deinit();

    const b = [5.0, 5.0];
    const x = m.solve(alloc, &b) catch unreachable;
    defer alloc.free(x);

    expectNear(x[0], 2.0, 0.000000001);
    expectNear(x[1], 1.0, 0.000000001);
    expectNear(cap.ml.vecNorm2(x), @sqrt(5.0), 0.000000001);
    expectNear(cap.ml.dot(x, &b) catch unreachable, 15.0, 0.000000001);
}
