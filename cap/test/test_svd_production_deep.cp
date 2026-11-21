// Source: cp/cap/test/test_svd_production_deep.cp (Production deep tests for cap:svd)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SvdResult,
    svdJacobi,
    svdSolve,
    svdPseudoinverse,
    svdReconstructionError,
} from "cap:svd";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn maxOrthoError(mat: []const f64, rows: usize, cols: usize): f64 {
    // Computes ||mat^T * mat - I_cols||_inf
    let max_err: f64 = 0.0;
    let i: usize = 0;
    while (i < cols) {
        let j: usize = 0;
        while (j < cols) {
            let dot: f64 = 0.0;
            let r: usize = 0;
            while (r < rows) {
                dot += mat[r * cols + i] * mat[r * cols + j];
                r += 1;
            }
            const target: f64 = if (i == j) 1.0 else 0.0;
            const err: f64 = absF64(dot - target);
            if (err > max_err) {
                max_err = err;
            }
            j += 1;
        }
        i += 1;
    }
    return max_err;
}

fn matMul(a: []const f64, b: []const f64, m: usize, k: usize, n: usize, out: *[]f64) {
    let r: usize = 0;
    while (r < m) {
        let c: usize = 0;
        while (c < n) {
            let s: f64 = 0.0;
            let t: usize = 0;
            while (t < k) {
                s += a[r * k + t] * b[t * n + c];
                t += 1;
            }
            out.*[r * n + c] = s;
            c += 1;
        }
        r += 1;
    }
}

test("1. 3x3 square & 5x3 overdetermined SVD accuracy < 1e-12, orthogonality & descending singular values", () => {
    // 3x3 non-symmetric matrix
    const a3: [9]f64 = [
        3.0, 2.0, 2.0,
        2.0, 3.0, -2.0,
        1.0, -1.0, 4.0,
    ];
    let u3_b: [9]f64 = std.mem.zeroes([9]f64);
    let s3_b: [3]f64 = std.mem.zeroes([3]f64);
    let v3_b: [9]f64 = std.mem.zeroes([9]f64);
    let u3: []f64 = u3_b[0..];
    let s3: []f64 = s3_b[0..];
    let v3: []f64 = v3_b[0..];

    const r3: SvdResult = svdJacobi(a3[0..], 3, 3, *u3, *s3, *v3, 60, 1.0e-14) ?? unreachable;
    expect_eq(r3.converged, true);
    expect_eq(r3.rank, 3 as usize);
    expect_eq(s3[0] >= s3[1] && s3[1] >= s3[2] && s3[2] > 0.0, true);
    expect_eq(svdReconstructionError(a3[0..], u3, s3, v3, 3, 3) < 1.0e-12, true);
    expect_eq(maxOrthoError(u3, 3, 3) < 1.0e-12, true);
    expect_eq(maxOrthoError(v3, 3, 3) < 1.0e-12, true);

    // 5x3 tall matrix
    const a5: [15]f64 = [
        1.0, 2.0, 3.0,
        4.0, 5.0, 6.0,
        7.0, 8.0, 10.0,
        2.0, -1.0, 3.0,
        -3.0, 4.0, 1.0,
    ];
    let u5_b: [15]f64 = std.mem.zeroes([15]f64);
    let s5_b: [3]f64 = std.mem.zeroes([3]f64);
    let v5_b: [9]f64 = std.mem.zeroes([9]f64);
    let u5: []f64 = u5_b[0..];
    let s5: []f64 = s5_b[0..];
    let v5: []f64 = v5_b[0..];

    const r5: SvdResult = svdJacobi(a5[0..], 5, 3, *u5, *s5, *v5, 60, 1.0e-14) ?? unreachable;
    expect_eq(r5.converged, true);
    expect_eq(r5.rank, 3 as usize);
    expect_eq(s5[0] >= s5[1] && s5[1] >= s5[2] && s5[2] > 0.0, true);
    expect_eq(svdReconstructionError(a5[0..], u5, s5, v5, 5, 3) < 1.0e-12, true);
    expect_eq(maxOrthoError(u5, 5, 3) < 1.0e-12, true);
    expect_eq(maxOrthoError(v5, 3, 3) < 1.0e-12, true);
});

test("2. Rank-deficient 4x3 matrix: exact rank == 2 & minimum-norm least-squares solution", () => {
    // col2 = col0 + 2 * col1 -> true rank == 2
    const a: [12]f64 = [
        1.0, 0.0, 1.0,
        0.0, 1.0, 2.0,
        1.0, 1.0, 3.0,
        2.0, -1.0, 0.0,
    ];
    let u_b: [12]f64 = std.mem.zeroes([12]f64);
    let s_b: [3]f64 = std.mem.zeroes([3]f64);
    let v_b: [9]f64 = std.mem.zeroes([9]f64);
    let u: []f64 = u_b[0..];
    let s: []f64 = s_b[0..];
    let v: []f64 = v_b[0..];

    const res: SvdResult = svdJacobi(a[0..], 4, 3, *u, *s, *v, 60, 1.0e-14) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.rank, 2 as usize);
    expect_eq(s[2] < 1.0e-12, true);
    expect_eq(svdReconstructionError(a[0..], u, s, v, 4, 3) < 1.0e-12, true);

    // Solve A * x = b where b = A * [1, 2, 0]^T = [1, 2, 3, 0]^T
    // Minimum-norm solution must satisfy A * x == b AND x orthogonal to nullspace vector [1, 2, -1]
    const b: [4]f64 = [1.0, 2.0, 3.0, 0.0];
    let x_b: [3]f64 = std.mem.zeroes([3]f64);
    let sc_b: [3]f64 = std.mem.zeroes([3]f64);
    let x: []f64 = x_b[0..];
    let sc: []f64 = sc_b[0..];

    expect_eq(svdSolve(u, s, v, 4, 3, b[0..], *x, *sc, 1.0e-12), true);

    // Residual ||A*x - b||_inf < 1e-12
    let r: usize = 0;
    while (r < 4) {
        const ax: f64 = a[r * 3 + 0] * x[0] + a[r * 3 + 1] * x[1] + a[r * 3 + 2] * x[2];
        expect_eq(absF64(ax - b[r]) < 1.0e-12, true);
        r += 1;
    }

    // Nullspace orthogonality: x . [1, 2, -1] == 0
    const null_dot: f64 = x[0] * 1.0 + x[1] * 2.0 - x[2] * 1.0;
    expect_eq(absF64(null_dot) < 1.0e-12, true);
});

test("3. Moore-Penrose pseudoinverse satisfies all 4 Penrose axioms on rank-deficient 4x3 matrix", () => {
    const a: [12]f64 = [
        1.0, 2.0, 3.0,
        2.0, 4.0, 6.0,
        1.0, 0.0, 1.0,
        0.0, 2.0, 2.0,
    ];
    let u_b: [12]f64 = std.mem.zeroes([12]f64);
    let s_b: [3]f64 = std.mem.zeroes([3]f64);
    let v_b: [9]f64 = std.mem.zeroes([9]f64);
    let u: []f64 = u_b[0..];
    let s: []f64 = s_b[0..];
    let v: []f64 = v_b[0..];

    const res: SvdResult = svdJacobi(a[0..], 4, 3, *u, *s, *v, 60, 1.0e-14) ?? unreachable;
    expect_eq(res.rank, 2 as usize);

    let pinv_b: [12]f64 = std.mem.zeroes([12]f64);
    let pinv: []f64 = pinv_b[0..];
    expect_eq(svdPseudoinverse(u, s, v, 4, 3, *pinv, 1.0e-12), true);

    // AA^+ (4x4) and A^+A (3x3)
    let aap_b: [16]f64 = std.mem.zeroes([16]f64);
    let apa_b: [9]f64 = std.mem.zeroes([9]f64);
    let aapa_b: [12]f64 = std.mem.zeroes([12]f64);
    let apaa_b: [12]f64 = std.mem.zeroes([12]f64);
    let aap: []f64 = aap_b[0..];
    let apa: []f64 = apa_b[0..];
    let aapa: []f64 = aapa_b[0..];
    let apaa: []f64 = apaa_b[0..];

    matMul(a[0..], pinv, 4, 3, 4, *aap);
    matMul(pinv, a[0..], 3, 4, 3, *apa);
    matMul(aap, a[0..], 4, 4, 3, *aapa);
    matMul(apa, pinv, 3, 3, 4, *apaa);

    // Axiom 1: A A^+ A == A
    let i1: usize = 0;
    while (i1 < 12) {
        expect_eq(absF64(aapa[i1] - a[i1]) < 1.0e-11, true);
        i1 += 1;
    }

    // Axiom 2: A^+ A A^+ == A^+
    let i2: usize = 0;
    while (i2 < 12) {
        expect_eq(absF64(apaa[i2] - pinv[i2]) < 1.0e-11, true);
        i2 += 1;
    }

    // Axiom 3: (A A^+)^T == A A^+
    let r3: usize = 0;
    while (r3 < 4) {
        let c3: usize = 0;
        while (c3 < 4) {
            expect_eq(absF64(aap[r3 * 4 + c3] - aap[c3 * 4 + r3]) < 1.0e-11, true);
            c3 += 1;
        }
        r3 += 1;
    }

    // Axiom 4: (A^+ A)^T == A^+ A
    let r4: usize = 0;
    while (r4 < 3) {
        let c4: usize = 0;
        while (c4 < 3) {
            expect_eq(absF64(apa[r4 * 3 + c4] - apa[c4 * 3 + r4]) < 1.0e-11, true);
            c4 += 1;
        }
        r4 += 1;
    }
});

test("4. Zero matrix orthonormal completion & signed diagonal singular value sorting", () => {
    // 4x3 zero matrix -> rank 0, s == [0, 0, 0], U^T U == I_3, recon error == 0
    const z_mat: [12]f64 = std.mem.zeroes([12]f64);
    let u_b: [12]f64 = std.mem.zeroes([12]f64);
    let s_b: [3]f64 = std.mem.zeroes([3]f64);
    let v_b: [9]f64 = std.mem.zeroes([9]f64);
    let u: []f64 = u_b[0..];
    let s: []f64 = s_b[0..];
    let v: []f64 = v_b[0..];

    const rz: SvdResult = svdJacobi(z_mat[0..], 4, 3, *u, *s, *v, 30, 1.0e-14) ?? unreachable;
    expect_eq(rz.converged, true);
    expect_eq(rz.rank, 0 as usize);
    expect_eq(s[0] == 0.0 && s[1] == 0.0 && s[2] == 0.0, true);
    expect_eq(svdReconstructionError(z_mat[0..], u, s, v, 4, 3) == 0.0, true);
    expect_eq(maxOrthoError(u, 4, 3) < 1.0e-13, true);
    expect_eq(maxOrthoError(v, 3, 3) < 1.0e-13, true);

    // Unsorted signed diagonal matrix diag(-5, 2, -9) -> S == [9, 5, 2], cond == 4.5
    const diag_mat: [9]f64 = [
        -5.0, 0.0,  0.0,
         0.0, 2.0,  0.0,
         0.0, 0.0, -9.0,
    ];
    let ud_b: [9]f64 = std.mem.zeroes([9]f64);
    let sd_b: [3]f64 = std.mem.zeroes([3]f64);
    let vd_b: [9]f64 = std.mem.zeroes([9]f64);
    let ud: []f64 = ud_b[0..];
    let sd: []f64 = sd_b[0..];
    let vd: []f64 = vd_b[0..];

    const rd: SvdResult = svdJacobi(diag_mat[0..], 3, 3, *ud, *sd, *vd, 30, 1.0e-14) ?? unreachable;
    expect_eq(rd.rank, 3 as usize);
    expect_eq(absF64(sd[0] - 9.0) < 1.0e-14, true);
    expect_eq(absF64(sd[1] - 5.0) < 1.0e-14, true);
    expect_eq(absF64(sd[2] - 2.0) < 1.0e-14, true);
    expect_eq(absF64(rd.conditionNumber - 4.5) < 1.0e-13, true);
    expect_eq(svdReconstructionError(diag_mat[0..], ud, sd, vd, 3, 3) < 1.0e-14, true);
});

test("5. Overdetermined quadratic least-squares regression via svdSolve", () => {
    // Fit y = c0 + c1*t + c2*t^2 to exact samples of 1.5 - 2.0*t + 0.5*t^2 at t = -2,-1,0,1,2
    const a: [15]f64 = [
        1.0, -2.0, 4.0,
        1.0, -1.0, 1.0,
        1.0,  0.0, 0.0,
        1.0,  1.0, 1.0,
        1.0,  2.0, 4.0,
    ];
    const b: [5]f64 = [7.5, 4.0, 1.5, 0.0, -0.5];

    let u_b: [15]f64 = std.mem.zeroes([15]f64);
    let s_b: [3]f64 = std.mem.zeroes([3]f64);
    let v_b: [9]f64 = std.mem.zeroes([9]f64);
    let u: []f64 = u_b[0..];
    let s: []f64 = s_b[0..];
    let v: []f64 = v_b[0..];

    const res: SvdResult = svdJacobi(a[0..], 5, 3, *u, *s, *v, 60, 1.0e-14) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.rank, 3 as usize);

    let x_b: [3]f64 = std.mem.zeroes([3]f64);
    let sc_b: [3]f64 = std.mem.zeroes([3]f64);
    let x: []f64 = x_b[0..];
    let sc: []f64 = sc_b[0..];
    expect_eq(svdSolve(u, s, v, 5, 3, b[0..], *x, *sc, 1.0e-12), true);
    expect_eq(absF64(x[0] - 1.5) < 1.0e-12, true);
    expect_eq(absF64(x[1] + 2.0) < 1.0e-12, true);
    expect_eq(absF64(x[2] - 0.5) < 1.0e-12, true);
});

test("6. Input guards: m < n, empty dims, short buffers, negative tol/rcond & NaN/Inf", () => {
    const ok: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    let u_b: [6]f64 = std.mem.zeroes([6]f64);
    let s_b: [3]f64 = std.mem.zeroes([3]f64);
    let v_b: [9]f64 = std.mem.zeroes([9]f64);
    let u: []f64 = u_b[0..];
    let s: []f64 = s_b[0..];
    let v: []f64 = v_b[0..];

    // m < n rejected
    expect_eq(svdJacobi(ok[0..], 2, 3, *u, *s, *v, 30, 1.0e-12) == null, true);
    // m == 0 or n == 0 rejected
    expect_eq(svdJacobi(ok[0..], 0, 2, *u, *s, *v, 30, 1.0e-12) == null, true);
    expect_eq(svdJacobi(ok[0..], 2, 0, *u, *s, *v, 30, 1.0e-12) == null, true);
    // Negative tol rejected
    expect_eq(svdJacobi(ok[0..], 3, 2, *u, *s, *v, 30, -1.0e-6) == null, true);

    // Short buffers rejected
    let short_u: []f64 = u_b[0..5];
    let short_s: []f64 = s_b[0..1];
    let short_v: []f64 = v_b[0..3];
    expect_eq(svdJacobi(ok[0..5], 3, 2, *u, *s, *v, 30, 1.0e-12) == null, true);
    expect_eq(svdJacobi(ok[0..], 3, 2, *short_u, *s, *v, 30, 1.0e-12) == null, true);
    expect_eq(svdJacobi(ok[0..], 3, 2, *u, *short_s, *v, 30, 1.0e-12) == null, true);
    expect_eq(svdJacobi(ok[0..], 3, 2, *u, *s, *short_v, 30, 1.0e-12) == null, true);

    // NaN / Inf rejected
    const zero: f64 = 0.0;
    const nan_a: [6]f64 = [1.0, zero / zero, 3.0, 4.0, 5.0, 6.0];
    const inf_a: [6]f64 = [1.0, 2.0, 1.0 / zero, 4.0, 5.0, 6.0];
    expect_eq(svdJacobi(nan_a[0..], 3, 2, *u, *s, *v, 30, 1.0e-12) == null, true);
    expect_eq(svdJacobi(inf_a[0..], 3, 2, *u, *s, *v, 30, 1.0e-12) == null, true);

    // svdReconstructionError returns -1.0 on invalid shape
    expect_eq(svdReconstructionError(ok[0..], u, s, v, 2, 3) < 0.0, true);

    // svdSolve & svdPseudoinverse reject negative rcond / short output
    let pinv_short: []f64 = u_b[0..5];
    expect_eq(svdPseudoinverse(u, s, v, 3, 2, *pinv_short, 1.0e-12), false);
    expect_eq(svdPseudoinverse(u, s, v, 3, 2, *u, -0.1), false);
});
