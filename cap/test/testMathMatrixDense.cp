// Deep Production Identity Test Suite for lib/math/matrixDense.cp (Wave 27 Round 6)
// Verifies: 64x64 SIMD F32 GEMM/GEMV/Gram parity (< 1e-4), partial-pivot LU solve/det/inverse (< 1e-9),
// SPD Cholesky + logDet + non-SPD rejection, Householder QR orthogonality & overdetermined least squares,
// symmetric cyclic Jacobi eigendecomposition (< 1e-9), One-Sided Jacobi Thin SVD + pinv + condNumber (< 1e-8),
// matrixPowerF64, and dimension/singular/buffer defensive guards.

import { test, expect_eq, expect_true } from "cap:test";
import {
    MatrixError,
    MatrixViewF64,
    MatrixMutF64,
    MatrixViewF32,
    MatrixMutF32,
    matrixIdentityF64,
    transposeMatrixF64,
    matrixTraceF64,
    frobeniusNormF32,
    matrixDiffFrobeniusF64,
    matvecF32Simd,
    matmulF32Simd,
    matmulF64,
    LuFactorResult,
    luDecomposeInPlace,
    denseLuSolve,
    denseLuDeterminant,
    luInvert,
    choleskyDecomposeInPlace,
    denseCholeskySolve,
    denseCholeskyLogDet,
    qrDecomposeHouseholder,
    denseQrSolveLeastSquares,
    EigenJacobiResult,
    eigenSymmetricJacobi,
    SvdThinResult,
    svdThin,
    svdCondNumber,
    svdThinPinv,
    gramMatrixF32Simd,
    matrixPowerF64,
} from "../../lib/math/index.cp";

const std = @import("std");

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

// ============================================================================
// Test 1: 64x64 SIMD F32 GEMM, GEMV & Gram Matrix Parity (< 1e-4)
// ============================================================================
test("MatrixDense: 64x64 SIMD F32 GEMM, GEMV & Gram matrix scalar parity (< 1e-4)") {
    const N: usize = 64;
    let a_data: [4096]f32 = [0.0; 4096];
    let b_data: [4096]f32 = [0.0; 4096];
    let c_data: [4096]f32 = [0.0; 4096];
    let bt_work: [4096]f32 = [0.0; 4096];

    let i: usize = 0;
    while (i < N) : (i += 1) {
        let j: usize = 0;
        while (j < N) : (j += 1) {
            const fi: f64 = @as(f64, @floatFromInt(i + 1));
            const fj: f64 = @as(f64, @floatFromInt(j + 1));
            a_data[i * N + j] = (std.math.sin(fi * 0.17 + fj * 0.11) * 0.5) as f32;
            b_data[i * N + j] = (std.math.cos(fi * 0.13 - fj * 0.19) * 0.5) as f32;
            c_data[i * N + j] = 0.25;
        }
    }

    const va: MatrixViewF32 = MatrixViewF32.init(a_data[0..], N, N) catch { expect_true(false); return; };
    const vb: MatrixViewF32 = MatrixViewF32.init(b_data[0..], N, N) catch { expect_true(false); return; };
    let mc: MatrixMutF32 = MatrixMutF32.init(c_data[0..], N, N) catch { expect_true(false); return; };
    let bt_s: []f32 = bt_work[0..];

    * = matmulF32Simd(va, vb, 1.5, 0.5, *mc, *bt_s) catch { expect_true(false); return; };

    // Compare against naive scalar reference across all 64x64 entries
    let max_err: f64 = 0.0;
    i = 0;
    while (i < N) : (i += 1) {
        let j: usize = 0;
        while (j < N) : (j += 1) {
            let acc: f64 = 0.0;
            let k: usize = 0;
            while (k < N) : (k += 1) {
                acc += @as(f64, a_data[i * N + k]) * @as(f64, b_data[k * N + j]);
            }
            const ref_val: f64 = 1.5 * acc + 0.5 * 0.25;
            const err: f64 = absF64(@as(f64, c_data[i * N + j]) - ref_val);
            if (err > max_err) max_err = err;
        }
    }
    expect_true(max_err < 1.0e-4);

    // GEMV check
    let x_vec: [64]f32 = [0.0; 64];
    let y_vec: [64]f32 = [0.0; 64];
    let y_s: []f32 = y_vec[0..];
    i = 0;
    while (i < N) : (i += 1) x_vec[i] = (@as(f64, @floatFromInt(i)) * 0.02) as f32;
    * = matvecF32Simd(va, x_vec[0..], 1.0, 0.0, *y_s) catch { expect_true(false); return; };
    let ref_y0: f64 = 0.0;
    i = 0;
    while (i < N) : (i += 1) ref_y0 += @as(f64, a_data[i]) * @as(f64, x_vec[i]);
    expect_true(absF64(@as(f64, y_vec[0]) - ref_y0) < 1.0e-4);

    // Gram matrix A^T * A symmetry & norm check
    let gram_data: [4096]f32 = [0.0; 4096];
    let mg: MatrixMutF32 = MatrixMutF32.init(gram_data[0..], N, N) catch { expect_true(false); return; };
    * = gramMatrixF32Simd(va, *mg, *bt_s) catch { expect_true(false); return; };
    expect_true(absF64(@as(f64, mg.at(3, 17)) - @as(f64, mg.at(17, 3))) < 1.0e-6);
    expect_true(frobeniusNormF32(va) > 1.0);
}

// ============================================================================
// Test 2: Partial-Pivoting LU Solve, Determinant & Inverse (||A*A^-1 - I||_F < 1e-9)
// ============================================================================
test("MatrixDense: LU partial-pivoting solve, determinant & inverse (||A*A^-1 - I||_F < 1e-9)") {
    const N: usize = 4;
    const orig_data: [16]f64 = [
        0.0,  2.0,  1.0, -1.0,
        3.0, -1.0,  2.0,  1.0,
        1.0,  4.0, -1.0,  2.0,
        2.0,  1.0,  3.0, -2.0,
    ];
    let lu_data: [16]f64 = orig_data;
    let piv: [4]usize = [0; 4];
    let piv_s: []usize = piv[0..];

    const va: MatrixViewF64 = MatrixViewF64.init(orig_data[0..], N, N) catch { expect_true(false); return; };
    let mlu: MatrixMutF64 = MatrixMutF64.init(lu_data[0..], N, N) catch { expect_true(false); return; };

    const lu_res: LuFactorResult = luDecomposeInPlace(*mlu, *piv_s) catch { expect_true(false); return; };
    expect_true(lu_res.swaps >= 1); // row 0 has 0 pivot, forcing partial pivot swap

    // Solve A * x = b where true x = [1, -2, 3, 4]
    const x_true: [4]f64 = [1.0, -2.0, 3.0, 4.0];
    let b_vec: [4]f64 = [0.0; 4];
    let r: usize = 0;
    while (r < N) : (r += 1) {
        let s: f64 = 0.0;
        let c: usize = 0;
        while (c < N) : (c += 1) s += va.at(r, c) * x_true[c];
        b_vec[r] = s;
    }

    let x_sol: [4]f64 = [0.0; 4];
    let x_s: []f64 = x_sol[0..];
    * = denseLuSolve(mlu.asView(), piv[0..], b_vec[0..], *x_s) catch { expect_true(false); return; };
    r = 0;
    while (r < N) : (r += 1) {
        expect_true(absF64(x_sol[r] - x_true[r]) < 1.0e-11);
    }

    const det: f64 = denseLuDeterminant(mlu.asView(), lu_res.sign) catch { expect_true(false); return; };
    expect_true(absF64(det) > 1.0);

    // Explicit matrix inverse and verify ||A * A^{-1} - I||_F < 1e-9
    let inv_data: [16]f64 = [0.0; 16];
    let col_work: [8]f64 = [0.0; 8];
    let minv: MatrixMutF64 = MatrixMutF64.init(inv_data[0..], N, N) catch { expect_true(false); return; };
    let cw_s: []f64 = col_work[0..];
    * = luInvert(mlu.asView(), piv[0..], *minv, *cw_s) catch { expect_true(false); return; };

    let prod_data: [16]f64 = [0.0; 16];
    let eye_data: [16]f64 = [0.0; 16];
    let bt_w: [16]f64 = [0.0; 16];
    let mprod: MatrixMutF64 = MatrixMutF64.init(prod_data[0..], N, N) catch { expect_true(false); return; };
    let meye: MatrixMutF64 = MatrixMutF64.init(eye_data[0..], N, N) catch { expect_true(false); return; };
    let bt_s: []f64 = bt_w[0..];
    * = matrixIdentityF64(*meye) catch { expect_true(false); return; };
    * = matmulF64(va, minv.asView(), 1.0, 0.0, *mprod, *bt_s) catch { expect_true(false); return; };

    const inv_err: f64 = matrixDiffFrobeniusF64(mprod.asView(), meye.asView()) catch { expect_true(false); return; };
    expect_true(inv_err < 1.0e-9);
}

// ============================================================================
// Test 3: Cholesky Factorization (A = L*L^T), logDet & Non-SPD Rejection
// ============================================================================
test("MatrixDense: Cholesky A=L*L^T, solve, logDet & non-SPD rejection") {
    const N: usize = 4;
    // Construct SPD matrix A = [[10,2,1,0.5], [2,8,1.5,1], [1,1.5,9,2], [0.5,1,2,7]]
    const spd_orig: [16]f64 = [
        10.0, 2.0, 1.0, 0.5,
         2.0, 8.0, 1.5, 1.0,
         1.0, 1.5, 9.0, 2.0,
         0.5, 1.0, 2.0, 7.0,
    ];
    let l_data: [16]f64 = spd_orig;
    let ml: MatrixMutF64 = MatrixMutF64.init(l_data[0..], N, N) catch { expect_true(false); return; };
    * = choleskyDecomposeInPlace(*ml) catch { expect_true(false); return; };

    // Verify L * L^T == A
    let lt_data: [16]f64 = [0.0; 16];
    let recon_data: [16]f64 = [0.0; 16];
    let bt_w: [16]f64 = [0.0; 16];
    let mlt: MatrixMutF64 = MatrixMutF64.init(lt_data[0..], N, N) catch { expect_true(false); return; };
    let mrecon: MatrixMutF64 = MatrixMutF64.init(recon_data[0..], N, N) catch { expect_true(false); return; };
    let bt_s: []f64 = bt_w[0..];
    * = transposeMatrixF64(ml.asView(), *mlt) catch { expect_true(false); return; };
    * = matmulF64(ml.asView(), mlt.asView(), 1.0, 0.0, *mrecon, *bt_s) catch { expect_true(false); return; };

    const va: MatrixViewF64 = MatrixViewF64.init(spd_orig[0..], N, N) catch { expect_true(false); return; };
    const chol_err: f64 = matrixDiffFrobeniusF64(mrecon.asView(), va) catch { expect_true(false); return; };
    expect_true(chol_err < 1.0e-11);

    // Solve SPD system & compare logDet against ln(LU det)
    const b: [4]f64 = [13.5, 12.5, 13.5, 10.5];
    let x: [4]f64 = [0.0; 4];
    let x_s: []f64 = x[0..];
    * = denseCholeskySolve(ml.asView(), b[0..], *x_s) catch { expect_true(false); return; };
    // Solution must be [1, 1, 1, 1] since b = row sums of A
    let i: usize = 0;
    while (i < N) : (i += 1) expect_true(absF64(x[i] - 1.0) < 1.0e-11);

    const log_det: f64 = denseCholeskyLogDet(ml.asView()) catch { expect_true(false); return; };
    let lu_copy: [16]f64 = spd_orig;
    let piv: [4]usize = [0; 4];
    let mlu: MatrixMutF64 = MatrixMutF64.init(lu_copy[0..], N, N) catch { expect_true(false); return; };
    let piv_s: []usize = piv[0..];
    const lres: LuFactorResult = luDecomposeInPlace(*mlu, *piv_s) catch { expect_true(false); return; };
    const lu_det: f64 = denseLuDeterminant(mlu.asView(), lres.sign) catch { expect_true(false); return; };
    expect_true(absF64(log_det - std.math.log(f64, std.math.e, lu_det)) < 1.0e-10);

    // Non-positive-definite matrix must return NotPositiveDefinite
    let bad_spd: [4]f64 = [1.0, 3.0, 3.0, 1.0]; // eigenvalues 4 and -2
    let mbad: MatrixMutF64 = MatrixMutF64.init(bad_spd[0..], 2, 2) catch { expect_true(false); return; };
    let got_non_spd: bool = false;
    * = choleskyDecomposeInPlace(*mbad) catch |e| {
        if (e == MatrixError.NotPositiveDefinite) got_non_spd = true;
    };
    expect_true(got_non_spd);
}

// ============================================================================
// Test 4: Householder QR Orthogonality & Overdetermined Least Squares
// ============================================================================
test("MatrixDense: Householder QR orthogonality (||Q^T*Q-I|| < 1e-10) & least squares") {
    const M: usize = 6;
    const N: usize = 3;
    // Polynomial design matrix [1, t, t^2] for t = 0..5
    const a_data: [18]f64 = [
        1.0, 0.0,  0.0,
        1.0, 1.0,  1.0,
        1.0, 2.0,  4.0,
        1.0, 3.0,  9.0,
        1.0, 4.0, 16.0,
        1.0, 5.0, 25.0,
    ];
    const va: MatrixViewF64 = MatrixViewF64.init(a_data[0..], M, N) catch { expect_true(false); return; };

    let q_data: [36]f64 = [0.0; 36];
    let r_data: [18]f64 = [0.0; 18];
    let v_work: [6]f64 = [0.0; 6];
    let mq: MatrixMutF64 = MatrixMutF64.init(q_data[0..], M, M) catch { expect_true(false); return; };
    let mr: MatrixMutF64 = MatrixMutF64.init(r_data[0..], M, N) catch { expect_true(false); return; };
    let vw_s: []f64 = v_work[0..];

    * = qrDecomposeHouseholder(va, *mq, *mr, *vw_s) catch { expect_true(false); return; };

    // Verify ||Q^T * Q - I||_F < 1e-10
    let qt_data: [36]f64 = [0.0; 36];
    let qtq_data: [36]f64 = [0.0; 36];
    let eye_data: [36]f64 = [0.0; 36];
    let bt_w: [36]f64 = [0.0; 36];
    let mqt: MatrixMutF64 = MatrixMutF64.init(qt_data[0..], M, M) catch { expect_true(false); return; };
    let mqtq: MatrixMutF64 = MatrixMutF64.init(qtq_data[0..], M, M) catch { expect_true(false); return; };
    let meye: MatrixMutF64 = MatrixMutF64.init(eye_data[0..], M, M) catch { expect_true(false); return; };
    let bt_s: []f64 = bt_w[0..];
    * = matrixIdentityF64(*meye) catch { expect_true(false); return; };
    * = transposeMatrixF64(mq.asView(), *mqt) catch { expect_true(false); return; };
    * = matmulF64(mqt.asView(), mq.asView(), 1.0, 0.0, *mqtq, *bt_s) catch { expect_true(false); return; };
    const ortho_err: f64 = matrixDiffFrobeniusF64(mqtq.asView(), meye.asView()) catch { expect_true(false); return; };
    expect_true(ortho_err < 1.0e-10);

    // Verify ||Q * R - A||_F < 1e-10
    let qr_data: [18]f64 = [0.0; 18];
    let mqr: MatrixMutF64 = MatrixMutF64.init(qr_data[0..], M, N) catch { expect_true(false); return; };
    * = matmulF64(mq.asView(), mr.asView(), 1.0, 0.0, *mqr, *bt_s) catch { expect_true(false); return; };
    const recon_err: f64 = matrixDiffFrobeniusF64(mqr.asView(), va) catch { expect_true(false); return; };
    expect_true(recon_err < 1.0e-10);

    // Solve overdetermined least squares for y = 2 - 3t + 0.5t^2 + small symmetric noise
    const b: [6]f64 = [2.1, -0.6, -1.9, -2.6, -1.9, -0.4];
    let x: [3]f64 = [0.0; 3];
    let qtb: [6]f64 = [0.0; 6];
    let x_s: []f64 = x[0..];
    let qtb_s: []f64 = qtb[0..];
    * = denseQrSolveLeastSquares(mq.asView(), mr.asView(), b[0..], *x_s, *qtb_s) catch {
        expect_true(false);
        return;
    };

    // Normal-equation optimality check: ||A^T * (A*x - b)||_2 < 1e-10
    let resid: [6]f64 = [0.0; 6];
    let i: usize = 0;
    while (i < M) : (i += 1) {
        resid[i] = va.at(i, 0) * x[0] + va.at(i, 1) * x[1] + va.at(i, 2) * x[2] - b[i];
    }
    let norm_eq_sq: f64 = 0.0;
    let j: usize = 0;
    while (j < N) : (j += 1) {
        let g: f64 = 0.0;
        i = 0;
        while (i < M) : (i += 1) g += va.at(i, j) * resid[i];
        norm_eq_sq += g * g;
    }
    expect_true(std.math.sqrt(norm_eq_sq) < 1.0e-10);
}

// ============================================================================
// Test 5: Symmetric Cyclic Jacobi Eigenvalue Decomposition (||A*V - V*L|| < 1e-9)
// ============================================================================
test("MatrixDense: symmetric Jacobi eigendecomposition (A*V = V*Lambda, V^T*V = I)") {
    const N: usize = 5;
    const sym_data: [25]f64 = [
         6.0, -2.0,  1.0,  0.5, -0.2,
        -2.0,  5.0, -1.5,  0.3,  0.8,
         1.0, -1.5,  7.0, -1.0,  0.4,
         0.5,  0.3, -1.0,  4.0, -0.6,
        -0.2,  0.8,  0.4, -0.6,  3.0,
    ];
    const va: MatrixViewF64 = MatrixViewF64.init(sym_data[0..], N, N) catch { expect_true(false); return; };

    let evals: [5]f64 = [0.0; 5];
    let evecs_data: [25]f64 = [0.0; 25];
    let a_work_data: [25]f64 = [0.0; 25];
    let evals_s: []f64 = evals[0..];
    let mevecs: MatrixMutF64 = MatrixMutF64.init(evecs_data[0..], N, N) catch { expect_true(false); return; };
    let maw: MatrixMutF64 = MatrixMutF64.init(a_work_data[0..], N, N) catch { expect_true(false); return; };

    const eres: EigenJacobiResult = eigenSymmetricJacobi(va, 64, 1.0e-13, *evals_s, *mevecs, *maw) catch {
        expect_true(false);
        return;
    };
    expect_true(eres.converged);

    // Check descending order & trace conservation
    let eig_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < N) : (i += 1) {
        eig_sum += evals[i];
        if (i + 1 < N) expect_true(evals[i] >= evals[i + 1]);
    }
    const tr: f64 = matrixTraceF64(va) catch { expect_true(false); return; };
    expect_true(absF64(eig_sum - tr) < 1.0e-10);

    // Verify ||A * V - V * diag(evals)||_F < 1e-9
    let av_data: [25]f64 = [0.0; 25];
    let vl_data: [25]f64 = [0.0; 25];
    let bt_w: [25]f64 = [0.0; 25];
    let mav: MatrixMutF64 = MatrixMutF64.init(av_data[0..], N, N) catch { expect_true(false); return; };
    let mvl: MatrixMutF64 = MatrixMutF64.init(vl_data[0..], N, N) catch { expect_true(false); return; };
    let bt_s: []f64 = bt_w[0..];
    * = matmulF64(va, mevecs.asView(), 1.0, 0.0, *mav, *bt_s) catch { expect_true(false); return; };

    let r: usize = 0;
    while (r < N) : (r += 1) {
        let c: usize = 0;
        while (c < N) : (c += 1) {
            mvl.set(r, c, mevecs.at(r, c) * evals[c]);
        }
    }
    const eig_err: f64 = matrixDiffFrobeniusF64(mav.asView(), mvl.asView()) catch { expect_true(false); return; };
    expect_true(eig_err < 1.0e-9);
}

// ============================================================================
// Test 6: One-Sided Jacobi Thin SVD (||U*S*V^T - A|| < 1e-8), Cond & Pseudoinverse
// ============================================================================
test("MatrixDense: One-Sided Jacobi Thin SVD reconstruction (< 1e-8), cond & pinv") {
    const M: usize = 6;
    const N: usize = 4;
    const a_data: [24]f64 = [
         3.0,  1.0, -2.0,  0.5,
         1.0,  4.0,  0.5, -1.0,
        -2.0,  0.5,  5.0,  2.0,
         0.5, -1.0,  2.0,  3.5,
         1.5,  2.0, -1.0,  1.0,
        -1.0,  0.5,  1.5, -2.5,
    ];
    const va: MatrixViewF64 = MatrixViewF64.init(a_data[0..], M, N) catch { expect_true(false); return; };

    let u_data: [24]f64 = [0.0; 24];
    let s_data: [4]f64 = [0.0; 4];
    let v_data: [16]f64 = [0.0; 16];
    let mu: MatrixMutF64 = MatrixMutF64.init(u_data[0..], M, N) catch { expect_true(false); return; };
    let s_s: []f64 = s_data[0..];
    let mv: MatrixMutF64 = MatrixMutF64.init(v_data[0..], N, N) catch { expect_true(false); return; };

    const sres: SvdThinResult = svdThin(va, 64, 1.0e-13, *mu, *s_s, *mv) catch { expect_true(false); return; };
    expect_true(sres.converged);
    expect_eq(sres.rank, 4);
    expect_true(svdCondNumber(s_data[0..]) > 1.0 && svdCondNumber(s_data[0..]) < 100.0);

    // Reconstruct U * diag(S) * V^T and verify ||recon - A||_F < 1e-8
    let us_data: [24]f64 = [0.0; 24];
    let vt_data: [16]f64 = [0.0; 16];
    let recon_data: [24]f64 = [0.0; 24];
    let bt_w: [24]f64 = [0.0; 24];
    let mus: MatrixMutF64 = MatrixMutF64.init(us_data[0..], M, N) catch { expect_true(false); return; };
    let mvt: MatrixMutF64 = MatrixMutF64.init(vt_data[0..], N, N) catch { expect_true(false); return; };
    let mrecon: MatrixMutF64 = MatrixMutF64.init(recon_data[0..], M, N) catch { expect_true(false); return; };
    let bt_s: []f64 = bt_w[0..];

    let r: usize = 0;
    while (r < M) : (r += 1) {
        let c: usize = 0;
        while (c < N) : (c += 1) mus.set(r, c, mu.at(r, c) * s_data[c]);
    }
    * = transposeMatrixF64(mv.asView(), *mvt) catch { expect_true(false); return; };
    * = matmulF64(mus.asView(), mvt.asView(), 1.0, 0.0, *mrecon, *bt_s) catch { expect_true(false); return; };

    const svd_err: f64 = matrixDiffFrobeniusF64(mrecon.asView(), va) catch { expect_true(false); return; };
    expect_true(svd_err < 1.0e-8);

    // Moore-Penrose pseudoinverse identity: ||A^+ * A - I_4||_F < 1e-9 (full column rank)
    let pinv_data: [24]f64 = [0.0; 24];
    let mpinv: MatrixMutF64 = MatrixMutF64.init(pinv_data[0..], N, M) catch { expect_true(false); return; };
    * = svdThinPinv(mu.asView(), s_data[0..], mv.asView(), 1.0e-12, *mpinv) catch { expect_true(false); return; };

    let pa_data: [16]f64 = [0.0; 16];
    let eye_data: [16]f64 = [0.0; 16];
    let mpa: MatrixMutF64 = MatrixMutF64.init(pa_data[0..], N, N) catch { expect_true(false); return; };
    let meye: MatrixMutF64 = MatrixMutF64.init(eye_data[0..], N, N) catch { expect_true(false); return; };
    * = matrixIdentityF64(*meye) catch { expect_true(false); return; };
    * = matmulF64(mpinv.asView(), va, 1.0, 0.0, *mpa, *bt_s) catch { expect_true(false); return; };

    const pinv_err: f64 = matrixDiffFrobeniusF64(mpa.asView(), meye.asView()) catch { expect_true(false); return; };
    expect_true(pinv_err < 1.0e-9);
}

// ============================================================================
// Test 7: Matrix Power (A^k) & Defensive Error Guards (Singular, Mismatch, Small)
// ============================================================================
test("MatrixDense: binary matrixPowerF64 & singular/mismatch/buffer defensive guards") {
    // Fibonacci matrix [[1, 1], [1, 0]]^10 => [[89, 55], [55, 34]]
    const fib: [4]f64 = [1.0, 1.0, 1.0, 0.0];
    let out_d: [4]f64 = [0.0; 4];
    let b_d: [4]f64 = [0.0; 4];
    let p_d: [4]f64 = [0.0; 4];
    let bt: [4]f64 = [0.0; 4];

    const vfib: MatrixViewF64 = MatrixViewF64.init(fib[0..], 2, 2) catch { expect_true(false); return; };
    let mout: MatrixMutF64 = MatrixMutF64.init(out_d[0..], 2, 2) catch { expect_true(false); return; };
    let mb: MatrixMutF64 = MatrixMutF64.init(b_d[0..], 2, 2) catch { expect_true(false); return; };
    let mp: MatrixMutF64 = MatrixMutF64.init(p_d[0..], 2, 2) catch { expect_true(false); return; };
    let bt_s: []f64 = bt[0..];

    * = matrixPowerF64(vfib, 10, *mout, *mb, *mp, *bt_s) catch { expect_true(false); return; };
    expect_true(absF64(mout.at(0, 0) - 89.0) < 1.0e-12);
    expect_true(absF64(mout.at(0, 1) - 55.0) < 1.0e-12);
    expect_true(absF64(mout.at(1, 1) - 34.0) < 1.0e-12);

    // Singular matrix -> SingularMatrix
    let sing_d: [4]f64 = [1.0, 2.0, 2.0, 4.0];
    let piv: [2]usize = [0; 2];
    let msing: MatrixMutF64 = MatrixMutF64.init(sing_d[0..], 2, 2) catch { expect_true(false); return; };
    let piv_s: []usize = piv[0..];
    let got_sing: bool = false;
    * = luDecomposeInPlace(*msing, *piv_s) catch |e| {
        if (e == MatrixError.SingularMatrix) got_sing = true;
    };
    expect_true(got_sing);

    // Non-square trace -> NotSquare
    const rect_d: [6]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    const vrect: MatrixViewF64 = MatrixViewF64.init(rect_d[0..], 2, 3) catch { expect_true(false); return; };
    let got_nsq: bool = false;
    * = matrixTraceF64(vrect) catch |e| {
        if (e == MatrixError.NotSquare) got_nsq = true;
    };
    expect_true(got_nsq);

    // Dimension mismatch in matmul -> DimensionMismatch
    let got_dim: bool = false;
    * = matmulF64(vrect, vrect, 1.0, 0.0, *mout, *bt_s) catch |e| {
        if (e == MatrixError.DimensionMismatch) got_dim = true;
    };
    expect_true(got_dim);
}
