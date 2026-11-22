// Source: cp/cap/test/test_lu_production_deep.cp (Production deep tests for cap:lu)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    LuInfo,
    luFactorize,
    luSolve,
    luDeterminant,
    luInverse,
    matVecResidualInf,
    solveTridiagonal,
} from "cap:math";

fn absDiff(a: f64, b: f64): f64 {
    const d = a - b;
    return if (d < 0.0) 0.0 - d else d;
}

fn verifyPaEqualsLu(a: []const f64, lu: []const f64, piv: []const usize, n: usize): f64 {
    let max_err: f64 = 0.0;
    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            let sum: f64 = 0.0;
            const k_end = if (i < j) i else j;
            let k: usize = 0;
            while (k <= k_end) {
                const l_ik = if (k == i) 1.0 else lu[i * n + k];
                const u_kj = lu[k * n + j];
                sum = sum + l_ik * u_kj;
                k += 1;
            }
            const pa_ij = a[piv[i] * n + j];
            const err = absDiff(pa_ij, sum);
            if (err > max_err) {
                max_err = err;
            }
            j += 1;
        }
        i += 1;
    }
    return max_err;
}

test("1. 3x3 & 4x4 partial-pivoting PA = LU reconstruction, linear solve & multi-RHS reuse", () => {
    // 3x3 with zero leading pivot A[0,0] == 0.0 forcing row swap
    const a3: [9]f64 = [
        0.0, 2.0, 1.0,
        1.0, -2.0, -3.0,
        -1.0, 1.0, 2.0,
    ];
    let lu3: [9]f64 = a3;
    let piv3: [3]usize = std.mem.zeroes([3]usize);

    const info3 = luFactorize(lu3[0..], 3, piv3[0..], 1.0e-12) ?? unreachable;
    expect_eq(info3.swaps >= 1, true);
    expect_eq(info3.min_pivot > 0.0, true);
    expect_eq(verifyPaEqualsLu(a3[0..], lu3[0..], piv3[0..], 3) < 1.0e-13, true);

    // First RHS: true solution x = [1, 2, -1] => b = [3, 0, -1]
    const b3_a: [3]f64 = [3.0, 0.0, -1.0];
    let x3: [3]f64 = std.mem.zeroes([3]f64);
    expect_eq(luSolve(lu3[0..], piv3[0..], 3, b3_a[0..], x3[0..]), true);
    expect_eq(absDiff(x3[0], 1.0) < 1.0e-12, true);
    expect_eq(absDiff(x3[1], 2.0) < 1.0e-12, true);
    expect_eq(absDiff(x3[2], -1.0) < 1.0e-12, true);
    expect_eq(matVecResidualInf(a3[0..], x3[0..], b3_a[0..], 3) < 1.0e-12, true);

    // Reuse same LU for second RHS: true x = [-2, 3, 5] => b = [11, -23, 15]
    const b3_b: [3]f64 = [11.0, -23.0, 15.0];
    expect_eq(luSolve(lu3[0..], piv3[0..], 3, b3_b[0..], x3[0..]), true);
    expect_eq(absDiff(x3[0], -2.0) < 1.0e-12, true);
    expect_eq(absDiff(x3[1], 3.0) < 1.0e-12, true);
    expect_eq(absDiff(x3[2], 5.0) < 1.0e-12, true);
    expect_eq(matVecResidualInf(a3[0..], x3[0..], b3_b[0..], 3) < 1.0e-12, true);

    // 4x4 non-symmetric matrix with multiple pivots
    const a4: [16]f64 = [
        0.0, 1.0, 2.0, -1.0,
        2.0, 4.0, -2.0, 3.0,
        1.0, -1.0, 3.0, 2.0,
        3.0, 2.0, 1.0, -4.0,
    ];
    let lu4: [16]f64 = a4;
    let piv4: [4]usize = std.mem.zeroes([4]usize);
    * = luFactorize(lu4[0..], 4, piv4[0..], 1.0e-12) ?? unreachable;
    expect_eq(verifyPaEqualsLu(a4[0..], lu4[0..], piv4[0..], 4) < 1.0e-12, true);

    const b4: [4]f64 = [5.0, 12.0, 7.0, -3.0];
    let x4: [4]f64 = std.mem.zeroes([4]f64);
    expect_eq(luSolve(lu4[0..], piv4[0..], 4, b4[0..], x4[0..]), true);
    expect_eq(matVecResidualInf(a4[0..], x4[0..], b4[0..], 4) < 1.0e-12, true);
});

test("2. Exact determinant verification including permutation sign flips", () => {
    // Odd permutation matrix (single swap of rows 0 and 1): det == -1.0
    const perm_odd: [9]f64 = [
        0.0, 1.0, 0.0,
        1.0, 0.0, 0.0,
        0.0, 0.0, 1.0,
    ];
    let lu_p: [9]f64 = perm_odd;
    let piv_p: [3]usize = std.mem.zeroes([3]usize);
    const info_p = luFactorize(lu_p[0..], 3, piv_p[0..], 1.0e-12) ?? unreachable;
    expect_eq(info_p.swaps, 1 as usize);
    expect_eq(info_p.sign, -1.0);
    expect_eq(absDiff(luDeterminant(lu_p[0..], info_p, 3), -1.0) < 1.0e-13, true);

    // Analytic determinant:
    // | 2  1  3 |
    // | 4  1  6 |  => det = 2(1*5 - 6*2) - 1(4*5 - 6*(-1)) + 3(4*2 - 1*(-1))
    // |-1  2  5 |        = 2(-7) - 1(26) + 3(9) = -14 - 26 + 27 = -13
    const m3: [9]f64 = [
        2.0, 1.0, 3.0,
        4.0, 1.0, 6.0,
        -1.0, 2.0, 5.0,
    ];
    let lu_m: [9]f64 = m3;
    let piv_m: [3]usize = std.mem.zeroes([3]usize);
    const info_m = luFactorize(lu_m[0..], 3, piv_m[0..], 1.0e-12) ?? unreachable;
    expect_eq(absDiff(luDeterminant(lu_m[0..], info_m, 3), -13.0) < 1.0e-12, true);
});

test("3. Matrix inverse A * A^-1 == I and A^-1 * A == I", () => {
    const a: [16]f64 = [
        1.0, 2.0, 0.0, 1.0,
        0.0, 1.0, 3.0, -1.0,
        2.0, -1.0, 1.0, 4.0,
        -1.0, 3.0, 2.0, 2.0,
    ];
    let lu: [16]f64 = a;
    let piv: [4]usize = std.mem.zeroes([4]usize);
    const info = luFactorize(lu[0..], 4, piv[0..], 1.0e-12) ?? unreachable;
    const det_a = luDeterminant(lu[0..], info, 4);

    let inv: [16]f64 = std.mem.zeroes([16]f64);
    let rhs_s: [4]f64 = std.mem.zeroes([4]f64);
    let col_s: [4]f64 = std.mem.zeroes([4]f64);
    expect_eq(luInverse(lu[0..], piv[0..], 4, inv[0..], rhs_s[0..], col_s[0..]), true);

    // Check A * A^-1 == I and A^-1 * A == I
    let i: usize = 0;
    while (i < 4) {
        let j: usize = 0;
        while (j < 4) {
            let prod_r: f64 = 0.0;
            let prod_l: f64 = 0.0;
            let k: usize = 0;
            while (k < 4) {
                prod_r = prod_r + a[i * 4 + k] * inv[k * 4 + j];
                prod_l = prod_l + inv[i * 4 + k] * a[k * 4 + j];
                k += 1;
            }
            const target = if (i == j) 1.0 else 0.0;
            expect_eq(absDiff(prod_r, target) < 1.0e-12, true);
            expect_eq(absDiff(prod_l, target) < 1.0e-12, true);
            j += 1;
        }
        i += 1;
    }

    // Check det(A^-1) * det(A) == 1.0
    let lu_inv: [16]f64 = inv;
    let piv_inv: [4]usize = std.mem.zeroes([4]usize);
    const info_inv = luFactorize(lu_inv[0..], 4, piv_inv[0..], 1.0e-12) ?? unreachable;
    const det_inv = luDeterminant(lu_inv[0..], info_inv, 4);
    expect_eq(absDiff(det_a * det_inv, 1.0) < 1.0e-11, true);
});

test("4. Singular and near-singular matrices rejected cleanly", () => {
    // Row 2 == Row 0 + Row 1 (rank 2)
    let singular: [9]f64 = [
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        5.0, 7.0, 9.0,
    ];
    let piv: [3]usize = std.mem.zeroes([3]usize);
    expect_eq(luFactorize(singular[0..], 3, piv[0..], 1.0e-12) == null, true);

    // Zero column matrix
    let zero_col: [9]f64 = [
        1.0, 0.0, 2.0,
        3.0, 0.0, 4.0,
        5.0, 0.0, 6.0,
    ];
    expect_eq(luFactorize(zero_col[0..], 3, piv[0..], 1.0e-12) == null, true);

    // Near-singular below tolerance 1e-6
    let near_sing: [4]f64 = [
        1.0, 0.0,
        0.0, 1.0e-8,
    ];
    let piv2: [2]usize = std.mem.zeroes([2]usize);
    expect_eq(luFactorize(near_sing[0..], 2, piv2[0..], 1.0e-6) == null, true);
});

test("5. 100-dimensional tridiagonal Thomas algorithm O(n) exact solve & residual < 1e-12", () => {
    const n: usize = 100;
    let lower: [99]f64 = std.mem.zeroes([99]f64);
    let diag: [100]f64 = std.mem.zeroes([100]f64);
    let upper: [99]f64 = std.mem.zeroes([99]f64);
    let x_true: [100]f64 = std.mem.zeroes([100]f64);
    let rhs: [100]f64 = std.mem.zeroes([100]f64);
    let x_out: [100]f64 = std.mem.zeroes([100]f64);
    let c_buf: [100]f64 = std.mem.zeroes([100]f64);
    let d_buf: [100]f64 = std.mem.zeroes([100]f64);

    // Build diagonally dominant non-symmetric tridiagonal system with known ground truth x_true
    let i: usize = 0;
    while (i < n) {
        diag[i] = 4.5 + ((i % 5) as f64) * 0.25;
        x_true[i] = ((i as f64) - 50.0) * 0.1 + 1.0;
        if (i + 1 < n) {
            lower[i] = -1.2;
            upper[i] = -0.8;
        }
        i += 1;
    }

    // Compute exact rhs = T * x_true
    i = 0;
    while (i < n) {
        let v = diag[i] * x_true[i];
        if (i > 0) {
            v = v + lower[i - 1] * x_true[i - 1];
        }
        if (i + 1 < n) {
            v = v + upper[i] * x_true[i + 1];
        }
        rhs[i] = v;
        i += 1;
    }

    expect_eq(
        solveTridiagonal(lower[0..], diag[0..], upper[0..], rhs[0..], x_out[0..], c_buf[0..], d_buf[0..]),
        true,
    );

    // Verify both solution accuracy and residual ||T * x_out - rhs||_inf < 1e-12
    let max_sol_err: f64 = 0.0;
    let max_res: f64 = 0.0;
    i = 0;
    while (i < n) {
        const se = absDiff(x_out[i], x_true[i]);
        if (se > max_sol_err) {
            max_sol_err = se;
        }
        let tx = diag[i] * x_out[i];
        if (i > 0) {
            tx = tx + lower[i - 1] * x_out[i - 1];
        }
        if (i + 1 < n) {
            tx = tx + upper[i] * x_out[i + 1];
        }
        const re = absDiff(tx, rhs[i]);
        if (re > max_res) {
            max_res = re;
        }
        i += 1;
    }
    expect_eq(max_sol_err < 1.0e-12, true);
    expect_eq(max_res < 1.0e-12, true);

    // 1x1 degenerate tridiagonal system
    const empty0: [0]f64 = [];
    const d1: [1]f64 = [5.0];
    const r1: [1]f64 = [15.0];
    let x1: [1]f64 = std.mem.zeroes([1]f64);
    let c1: [1]f64 = std.mem.zeroes([1]f64);
    let s1: [1]f64 = std.mem.zeroes([1]f64);
    expect_eq(solveTridiagonal(empty0[0..], d1[0..], empty0[0..], r1[0..], x1[0..], c1[0..], s1[0..]), true);
    expect_eq(absDiff(x1[0], 3.0) < 1.0e-14, true);
});

test("6. Boundary guards: n == 0, undersized buffers, negative tol, NaN & tridiagonal zero pivot", () => {
    let mat4: [4]f64 = [1.0, 0.0, 0.0, 1.0];
    let mat3: [3]f64 = [1.0, 0.0, 0.0];
    let piv2: [2]usize = std.mem.zeroes([2]usize);
    let piv1: [1]usize = std.mem.zeroes([1]usize);

    expect_eq(luFactorize(mat4[0..], 0, piv2[0..], 1.0e-12) == null, true);
    expect_eq(luFactorize(mat3[0..], 2, piv2[0..], 1.0e-12) == null, true);
    expect_eq(luFactorize(mat4[0..], 2, piv1[0..], 1.0e-12) == null, true);
    expect_eq(luFactorize(mat4[0..], 2, piv2[0..], -1.0) == null, true);

    // NaN matrix rejected
    let nan_mat: [4]f64 = [1.0, std.math.nan(f64), 0.0, 1.0];
    expect_eq(luFactorize(nan_mat[0..], 2, piv2[0..], 1.0e-12) == null, true);

    // Invalid pivot index in luSolve rejected
    const bad_piv: [2]usize = [0, 5];
    const b2: [2]f64 = [1.0, 2.0];
    let x2: [2]f64 = std.mem.zeroes([2]f64);
    expect_eq(luSolve(mat4[0..], bad_piv[0..], 2, b2[0..], x2[0..]), false);

    // Tridiagonal zero pivot & undersized scratch rejected
    const lo: [2]f64 = [1.0, 1.0];
    const dg_zero: [3]f64 = [0.0, 2.0, 2.0];
    const dg_ok: [3]f64 = [2.0, 2.0, 2.0];
    const up: [2]f64 = [1.0, 1.0];
    const rh: [3]f64 = [1.0, 2.0, 3.0];
    let tx: [3]f64 = std.mem.zeroes([3]f64);
    let cs: [3]f64 = std.mem.zeroes([3]f64);
    let ds: [3]f64 = std.mem.zeroes([3]f64);
    let small2: [2]f64 = std.mem.zeroes([2]f64);
    const empty0: [0]f64 = [];

    expect_eq(solveTridiagonal(lo[0..], dg_zero[0..], up[0..], rh[0..], tx[0..], cs[0..], ds[0..]), false);
    expect_eq(solveTridiagonal(lo[0..], empty0[0..], up[0..], rh[0..], tx[0..], cs[0..], ds[0..]), false);
    expect_eq(solveTridiagonal(lo[0..], dg_ok[0..], up[0..], rh[0..], small2[0..], cs[0..], ds[0..]), false);
    expect_eq(solveTridiagonal(lo[0..], dg_ok[0..], up[0..], rh[0..], tx[0..], small2[0..], ds[0..]), false);
});
