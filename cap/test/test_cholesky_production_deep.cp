// Source: cp/cap/test/test_cholesky_production_deep.cp (Production deep tests for cap:cholesky)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    choleskyDecompose,
    forwardSubstitute,
    backwardSubstituteT,
    choleskySolve,
    choleskyLogDet,
    choleskyDet,
    choleskyInvert,
    ldltDecompose,
    ldltSolve,
} from "cap:math";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

test("1. Classic 3x3 SPD Cholesky L*L^T == A, exact factor & choleskySolve residual < 1e-12", () => {
    // Classic textbook SPD matrix:
    // A = [  4,  12, -16 ]   ->   L = [  2, 0, 0 ]
    //     [ 12,  37, -43 ]            [  6, 1, 0 ]
    //     [-16, -43,  98 ]            [ -8, 5, 3 ]
    const a: [9]f64 = [
        4.0, 12.0, -16.0,
        12.0, 37.0, -43.0,
        -16.0, -43.0, 98.0,
    ];
    let l: [9]f64 = std.mem.zeroes([9]f64);
    expect_eq(choleskyDecompose(a[0..], 3, l[0..]), true);

    expect_eq(absF64(l[0] - 2.0) < 1.0e-12, true);
    expect_eq(absF64(l[1]) < 1.0e-12, true);
    expect_eq(absF64(l[2]) < 1.0e-12, true);
    expect_eq(absF64(l[3] - 6.0) < 1.0e-12, true);
    expect_eq(absF64(l[4] - 1.0) < 1.0e-12, true);
    expect_eq(absF64(l[5]) < 1.0e-12, true);
    expect_eq(absF64(l[6] + 8.0) < 1.0e-12, true);
    expect_eq(absF64(l[7] - 5.0) < 1.0e-12, true);
    expect_eq(absF64(l[8] - 3.0) < 1.0e-12, true);

    // Solve A * x = b for true x = [1.5, -2.0, 0.5]
    const x_true: [3]f64 = [1.5, -2.0, 0.5];
    let b: [3]f64 = std.mem.zeroes([3]f64);
    let r: usize = 0;
    while (r < 3) {
        b[r] = a[r * 3 + 0] * x_true[0] + a[r * 3 + 1] * x_true[1] + a[r * 3 + 2] * x_true[2];
        r += 1;
    }

    let x_out: [3]f64 = std.mem.zeroes([3]f64);
    let scratch_y: [3]f64 = std.mem.zeroes([3]f64);
    expect_eq(choleskySolve(l[0..], 3, b[0..], x_out[0..], scratch_y[0..]), true);

    let i: usize = 0;
    while (i < 3) {
        expect_eq(absF64(x_out[i] - x_true[i]) < 1.0e-12, true);
        i += 1;
    }
});

test("2. choleskyDet & choleskyLogDet accuracy (det = 36, logDet = ln(36))", () => {
    const a: [9]f64 = [
        4.0, 12.0, -16.0,
        12.0, 37.0, -43.0,
        -16.0, -43.0, 98.0,
    ];
    let l: [9]f64 = std.mem.zeroes([9]f64);
    expect_eq(choleskyDecompose(a[0..], 3, l[0..]), true);

    // diag(L) = [2, 1, 3] -> det(A) = (2*1*3)^2 = 36.0
    const det = choleskyDet(l[0..], 3) ?? unreachable;
    expect_eq(absF64(det - 36.0) < 1.0e-12, true);

    // ln(36) = 3.5835189384561100
    const log_det = choleskyLogDet(l[0..], 3) ?? unreachable;
    expect_eq(absF64(log_det - 3.58351893845611) < 1.0e-12, true);
});

test("3. 4x4 SPD choleskyInvert satisfies A * A^-1 == I within 1e-11", () => {
    // Symmetric positive-definite 4x4 matrix
    const a: [16]f64 = [
        10.0, 2.0, 1.0, 0.5,
        2.0, 8.0, 1.5, 1.0,
        1.0, 1.5, 9.0, 2.0,
        0.5, 1.0, 2.0, 7.0,
    ];
    let l: [16]f64 = std.mem.zeroes([16]f64);
    let inv: [16]f64 = std.mem.zeroes([16]f64);
    let sc_col: [4]f64 = std.mem.zeroes([4]f64);
    let sc_y: [4]f64 = std.mem.zeroes([4]f64);

    expect_eq(choleskyDecompose(a[0..], 4, l[0..]), true);
    expect_eq(choleskyInvert(l[0..], 4, inv[0..], sc_col[0..], sc_y[0..]), true);

    // Verify A * inv == I_4
    let r: usize = 0;
    while (r < 4) {
        let c: usize = 0;
        while (c < 4) {
            let dot: f64 = 0.0;
            let k: usize = 0;
            while (k < 4) {
                dot = dot + a[r * 4 + k] * inv[k * 4 + c];
                k += 1;
            }
            const target: f64 = if (r == c) 1.0 else 0.0;
            expect_eq(absF64(dot - target) < 1.0e-11, true);
            c += 1;
        }
        r += 1;
    }
});

test("4. Sqrt-free LDL^T solves symmetric indefinite matrix with negative pivot", () => {
    // Symmetric indefinite matrix (negative pivot in D[1]):
    // A = [ 2,  4,  2 ]
    //     [ 4,  5,  7 ]  -> D[0] = 2, L10 = 2, D[1] = 5 - 4*2 = -3 < 0!
    //     [ 2,  7, 10 ]
    const a: [9]f64 = [
        2.0, 4.0, 2.0,
        4.0, 5.0, 7.0,
        2.0, 7.0, 10.0,
    ];
    let l: [9]f64 = std.mem.zeroes([9]f64);
    let d: [3]f64 = std.mem.zeroes([3]f64);

    // Standard Cholesky MUST reject indefinite matrix
    expect_eq(choleskyDecompose(a[0..], 3, l[0..]), false);

    // LDL^T succeeds and captures negative pivot
    expect_eq(ldltDecompose(a[0..], 3, l[0..], d[0..]), true);
    expect_eq(absF64(d[0] - 2.0) < 1.0e-12, true);
    expect_eq(absF64(d[1] + 3.0) < 1.0e-12, true);

    const x_true: [3]f64 = [-1.0, 2.5, 3.0];
    let b: [3]f64 = std.mem.zeroes([3]f64);
    let r: usize = 0;
    while (r < 3) {
        b[r] = a[r * 3 + 0] * x_true[0] + a[r * 3 + 1] * x_true[1] + a[r * 3 + 2] * x_true[2];
        r += 1;
    }

    let x_out: [3]f64 = std.mem.zeroes([3]f64);
    let sc_y: [3]f64 = std.mem.zeroes([3]f64);
    expect_eq(ldltSolve(l[0..], d[0..], 3, b[0..], x_out[0..], sc_y[0..]), true);

    let i: usize = 0;
    while (i < 3) {
        expect_eq(absF64(x_out[i] - x_true[i]) < 1.0e-12, true);
        i += 1;
    }
});

test("5. Non-SPD, singular zero-pivot, zero-dimension & short buffer rejection", () => {
    let l: [9]f64 = std.mem.zeroes([9]f64);
    let d: [3]f64 = std.mem.zeroes([3]f64);
    let v: [3]f64 = std.mem.zeroes([3]f64);
    let sc: [3]f64 = std.mem.zeroes([3]f64);

    // Singular zero-pivot matrix [0, 1; 1, 2]
    const singular: [4]f64 = [0.0, 1.0, 1.0, 2.0];
    expect_eq(choleskyDecompose(singular[0..], 2, l[0..]), false);
    expect_eq(ldltDecompose(singular[0..], 2, l[0..], d[0..]), false);

    // Negative-definite matrix rejected by Cholesky
    const neg_def: [4]f64 = [-4.0, 0.0, 0.0, -9.0];
    expect_eq(choleskyDecompose(neg_def[0..], 2, l[0..]), false);

    // Zero dimension & undersized buffers
    expect_eq(choleskyDecompose(singular[0..], 0, l[0..]), false);
    expect_eq(choleskyDecompose(singular[0..], 3, l[0..]), false); // a.len=4 < 9
    expect_eq(choleskyLogDet(l[0..], 0) == null, true);
    expect_eq(choleskyDet(l[0..], 0) == null, true);
    expect_eq(forwardSubstitute(l[0..], 2, v[0..1], sc[0..]), false);
    expect_eq(backwardSubstituteT(l[0..], 2, v[0..], sc[0..1]), false);
    expect_eq(choleskyInvert(l[0..], 2, l[0..3], v[0..], sc[0..]), false);
});
