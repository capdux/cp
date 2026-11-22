// Source: cp/cap/test/test_qr_production_deep.cp (Production deep tests for cap:qr)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    QR_DEFAULT_TOL,
    qrFactorize,
    qrApplyQt,
    qrFormQ,
    qrExtractR,
    qrSolveLeastSquares,
    qrOrthogonalityError,
    qrReconstructionError,
} from "cap:math";

fn absF64(v: f64): f64 {
    return if (v < 0.0) -v else v;
}

test("1. 3x3 square matrix QR factorization: orthogonality, reconstruction & implicit Q^T y accuracy", () => {
    // Classic Golub & Van Loan 3x3 matrix
    const a_orig = [
        12.0, -51.0, 4.0,
        6.0, 167.0, -68.0,
        -4.0, 24.0, -41.0,
    ];
    let a_qr = a_orig;
    let tau: [3]f64 = std.mem.zeroes([3]f64);
    let a_s: []f64 = a_qr[0..];
    let tau_s: []f64 = tau[0..];

    expect_eq(qrFactorize(&a_s, 3, 3, &tau_s), true);

    let q: [9]f64 = std.mem.zeroes([9]f64);
    let r: [9]f64 = std.mem.zeroes([9]f64);
    let q_s: []f64 = q[0..];
    let r_s: []f64 = r[0..];

    expect_eq(qrFormQ(a_qr[0..], tau[0..], 3, 3, &q_s), true);
    expect_eq(qrExtractR(a_qr[0..], 3, 3, &r_s), true);

    // Strictly upper triangular check on R
    expect_eq(r[3], 0.0);
    expect_eq(r[6], 0.0);
    expect_eq(r[7], 0.0);

    const ortho_err = qrOrthogonalityError(q[0..], 3, 3);
    const recon_err = qrReconstructionError(a_orig[0..], q[0..], r[0..], 3, 3);
    expect_eq(ortho_err >= 0.0 && ortho_err < 1.0e-12, true);
    expect_eq(recon_err >= 0.0 && recon_err < 1.0e-12, true);

    // Compare implicit qrApplyQt(y) against explicit Q^T * y
    const y_orig = [3.5, -2.25, 7.75];
    let y_imp = y_orig;
    let y_s: []f64 = y_imp[0..];
    expect_eq(qrApplyQt(a_qr[0..], tau[0..], 3, 3, &y_s), true);

    let j: usize = 0;
    while (j < 3) {
        let dot: f64 = 0.0;
        let i: usize = 0;
        while (i < 3) {
            dot += q[i * 3 + j] * y_orig[i];
            i += 1;
        }
        expect_eq(absF64(y_imp[j] - dot) < 1.0e-12, true);
        j += 1;
    }
});

test("2. 6x3 overdetermined QR & linear least squares with normal-equation residual orthogonality A^T(Ax - b) == 0", () => {
    // Quadratic regression design matrix A[i] = [1, t_i, t_i^2] for t = 0..5
    const a_orig = [
        1.0, 0.0, 0.0,
        1.0, 1.0, 1.0,
        1.0, 2.0, 4.0,
        1.0, 3.0, 9.0,
        1.0, 4.0, 16.0,
        1.0, 5.0, 25.0,
    ];
    // Noisy observations around y = 2 - 3t + 0.5 t^2
    const b = [2.1, -0.6, -1.9, -2.4, -2.1, -0.3];

    let a_qr = a_orig;
    let tau: [3]f64 = std.mem.zeroes([3]f64);
    let a_s: []f64 = a_qr[0..];
    let tau_s: []f64 = tau[0..];
    expect_eq(qrFactorize(&a_s, 6, 3, &tau_s), true);

    let q: [18]f64 = std.mem.zeroes([18]f64);
    let r: [9]f64 = std.mem.zeroes([9]f64);
    let q_s: []f64 = q[0..];
    let r_s: []f64 = r[0..];
    expect_eq(qrFormQ(a_qr[0..], tau[0..], 6, 3, &q_s), true);
    expect_eq(qrExtractR(a_qr[0..], 6, 3, &r_s), true);

    const ortho_err = qrOrthogonalityError(q[0..], 6, 3);
    const recon_err = qrReconstructionError(a_orig[0..], q[0..], r[0..], 6, 3);
    expect_eq(ortho_err >= 0.0 && ortho_err < 1.0e-12, true);
    expect_eq(recon_err >= 0.0 && recon_err < 1.0e-12, true);

    let x: [3]f64 = std.mem.zeroes([3]f64);
    let qty: [6]f64 = std.mem.zeroes([6]f64);
    let x_s: []f64 = x[0..];
    let qty_s: []f64 = qty[0..];
    expect_eq(qrSolveLeastSquares(a_qr[0..], tau[0..], 6, 3, b[0..], &x_s, &qty_s, QR_DEFAULT_TOL), true);

    // Verify normal equations: A^T * (A * x - b) == 0 to machine precision
    let resid: [6]f64 = std.mem.zeroes([6]f64);
    let i: usize = 0;
    while (i < 6) {
        const ax_i = a_orig[i * 3 + 0] * x[0] + a_orig[i * 3 + 1] * x[1] + a_orig[i * 3 + 2] * x[2];
        resid[i] = ax_i - b[i];
        i += 1;
    }

    let j: usize = 0;
    while (j < 3) {
        let at_r: f64 = 0.0;
        i = 0;
        while (i < 6) {
            at_r += a_orig[i * 3 + j] * resid[i];
            i += 1;
        }
        expect_eq(absF64(at_r) < 1.0e-11, true);
        j += 1;
    }
});

test("3. Ill-conditioned 6x6 Hilbert matrix stability: machine-precision Q orthogonality & QR reconstruction", () => {
    const n: usize = 6;
    let h_orig: [36]f64 = std.mem.zeroes([36]f64);
    let i: usize = 0;
    while (i < n) {
        let j: usize = 0;
        while (j < n) {
            h_orig[i * n + j] = 1.0 / ((i + j + 1) as f64);
            j += 1;
        }
        i += 1;
    }

    let h_qr = h_orig;
    let tau: [6]f64 = std.mem.zeroes([6]f64);
    let h_s: []f64 = h_qr[0..];
    let tau_s: []f64 = tau[0..];
    expect_eq(qrFactorize(&h_s, n, n, &tau_s), true);

    let q: [36]f64 = std.mem.zeroes([36]f64);
    let r: [36]f64 = std.mem.zeroes([36]f64);
    let q_s: []f64 = q[0..];
    let r_s: []f64 = r[0..];
    expect_eq(qrFormQ(h_qr[0..], tau[0..], n, n, &q_s), true);
    expect_eq(qrExtractR(h_qr[0..], n, n, &r_s), true);

    // Householder QR preserves < 1e-12 orthogonality & reconstruction even when cond(H_6) ~ 1.5e7
    const ortho_err = qrOrthogonalityError(q[0..], n, n);
    const recon_err = qrReconstructionError(h_orig[0..], q[0..], r[0..], n, n);
    expect_eq(ortho_err >= 0.0 && ortho_err < 1.0e-12, true);
    expect_eq(recon_err >= 0.0 && recon_err < 1.0e-12, true);

    // Solve H * x = H * [1, 2, 3, 4, 5, 6]^T and verify residual ||H x - b||_inf < 1e-10
    const x_true = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    let b: [6]f64 = std.mem.zeroes([6]f64);
    i = 0;
    while (i < n) {
        let s: f64 = 0.0;
        let j: usize = 0;
        while (j < n) {
            s += h_orig[i * n + j] * x_true[j];
            j += 1;
        }
        b[i] = s;
        i += 1;
    }

    let x_sol: [6]f64 = std.mem.zeroes([6]f64);
    let qty: [6]f64 = std.mem.zeroes([6]f64);
    let xs: []f64 = x_sol[0..];
    let qtys: []f64 = qty[0..];
    expect_eq(qrSolveLeastSquares(h_qr[0..], tau[0..], n, n, b[0..], &xs, &qtys, 1.0e-14), true);

    i = 0;
    while (i < n) {
        let hx_i: f64 = 0.0;
        let j: usize = 0;
        while (j < n) {
            hx_i += h_orig[i * n + j] * x_sol[j];
            j += 1;
        }
        expect_eq(absF64(hx_i - b[i]) < 1.0e-10, true);
        i += 1;
    }
});

test("4. Rank-deficient matrices (linearly dependent column & zero column) gracefully rejected by solver", () => {
    // Column 2 = 2 * Column 0 + Column 1 -> rank 2 < 3
    let a_dep = [
        1.0, 2.0, 4.0,
        3.0, 1.0, 7.0,
        2.0, 5.0, 9.0,
        4.0, 3.0, 11.0,
    ];
    let tau: [3]f64 = std.mem.zeroes([3]f64);
    let a_s: []f64 = a_dep[0..];
    let tau_s: []f64 = tau[0..];
    expect_eq(qrFactorize(&a_s, 4, 3, &tau_s), true);

    const b = [1.0, 2.0, 3.0, 4.0];
    let x: [3]f64 = std.mem.zeroes([3]f64);
    let qty: [4]f64 = std.mem.zeroes([4]f64);
    let x_s: []f64 = x[0..];
    let qty_s: []f64 = qty[0..];
    expect_eq(qrSolveLeastSquares(a_dep[0..], tau[0..], 4, 3, b[0..], &x_s, &qty_s, QR_DEFAULT_TOL), false);

    // All-zero column 1
    let a_zero = [
        1.0, 0.0,
        2.0, 0.0,
        3.0, 0.0,
    ];
    let tau2: [2]f64 = std.mem.zeroes([2]f64);
    let az_s: []f64 = a_zero[0..];
    let t2_s: []f64 = tau2[0..];
    expect_eq(qrFactorize(&az_s, 3, 2, &t2_s), true);
    expect_eq(qrSolveLeastSquares(a_zero[0..], tau2[0..], 3, 2, b[0..3], &x_s, &qty_s, QR_DEFAULT_TOL), false);
});

test("5. Defensive boundaries: m < n, empty dims, undersized buffers, NaN & Inf rejection", () => {
    let buf: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    let tau: [3]f64 = std.mem.zeroes([3]f64);
    let b_s: []f64 = buf[0..];
    let t_s: []f64 = tau[0..];

    // m < n (2 x 3) rejected
    expect_eq(qrFactorize(&b_s, 2, 3, &t_s), false);
    // Zero dimensions rejected
    expect_eq(qrFactorize(&b_s, 0, 0, &t_s), false);
    expect_eq(qrFactorize(&b_s, 3, 0, &t_s), false);

    // Undersized a_qr or tau rejected
    let short_a: []f64 = buf[0..5];
    let short_tau: []f64 = tau[0..1];
    expect_eq(qrFactorize(&short_a, 3, 2, &t_s), false);
    expect_eq(qrFactorize(&b_s, 3, 2, &short_tau), false);

    // NaN and Inf inputs rejected
    const zero: f64 = 0.0;
    const nan_val = zero / zero;
    const inf_val = 1.0 / zero;

    let nan_mat: [4]f64 = [1.0, nan_val, 3.0, 4.0];
    let nan_s: []f64 = nan_mat[0..];
    expect_eq(qrFactorize(&nan_s, 2, 2, &t_s), false);

    let inf_mat: [4]f64 = [1.0, 2.0, inf_val, 4.0];
    let inf_s: []f64 = inf_mat[0..];
    expect_eq(qrFactorize(&inf_s, 2, 2, &t_s), false);

    // Invalid error metric calls return -1.0
    expect_eq(qrOrthogonalityError(buf[0..], 2, 3), -1.0);
    expect_eq(qrReconstructionError(buf[0..], buf[0..], buf[0..], 0, 2), -1.0);
});

test("6. Extreme scaling robustness (1e100 & 1e-100) without overflow or underflow", () => {
    const big = 1.0e100;
    const a_big_orig = [
        3.0 * big, 4.0 * big,
        4.0 * big, -3.0 * big,
        0.0, 5.0 * big,
    ];
    let a_big = a_big_orig;
    let tau: [2]f64 = std.mem.zeroes([2]f64);
    let a_s: []f64 = a_big[0..];
    let tau_s: []f64 = tau[0..];
    expect_eq(qrFactorize(&a_s, 3, 2, &tau_s), true);

    let q: [6]f64 = std.mem.zeroes([6]f64);
    let r: [4]f64 = std.mem.zeroes([4]f64);
    let q_s: []f64 = q[0..];
    let r_s: []f64 = r[0..];
    expect_eq(qrFormQ(a_big[0..], tau[0..], 3, 2, &q_s), true);
    expect_eq(qrExtractR(a_big[0..], 3, 2, &r_s), true);

    const ortho_big = qrOrthogonalityError(q[0..], 3, 2);
    expect_eq(ortho_big >= 0.0 && ortho_big < 1.0e-12, true);

    const tiny = 1.0e-100;
    const a_tiny_orig = [
        3.0 * tiny, 4.0 * tiny,
        4.0 * tiny, -3.0 * tiny,
        0.0, 5.0 * tiny,
    ];
    let a_tiny = a_tiny_orig;
    let at_s: []f64 = a_tiny[0..];
    expect_eq(qrFactorize(&at_s, 3, 2, &tau_s), true);
    expect_eq(qrFormQ(a_tiny[0..], tau[0..], 3, 2, &q_s), true);

    const ortho_tiny = qrOrthogonalityError(q[0..], 3, 2);
    expect_eq(ortho_tiny >= 0.0 && ortho_tiny < 1.0e-12, true);
});
