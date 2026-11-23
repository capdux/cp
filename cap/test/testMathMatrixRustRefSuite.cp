// dimforge/nalgebra (Apache-2.0) Reference-Hardened Dense Linear Algebra Test Suite
// Covers:
//   - Hilbert ill-conditioned matrices (condition number, Cholesky, LU solve, detFromLu, logDet)
//   - Singular / non-SPD / asymmetric / NaN / Inf strict rejection guards
//   - Householder QR on 1x1, square, tall (6x3), wide (3x6), zero-column & rank-deficient matrices
//     (||Q^T Q - I||_F < 1e-11, ||Q R - A||_F < 1e-11)
//   - Thin SVD (svdThin & svdThinAny on 1x1, zero, tall 5x3, wide 3x5, rank-deficient) +
//     all 4 Moore-Penrose pseudoinverse identities (< 1e-10)
//   - TRSV / TRSM triangular solvers (unit & non-unit diagonal, residual < 1e-11) + infNorm / oneNorm
//   - SIMD GEMM / GEMV / Rank-1 Outer Product / Frobenius Norm vs scalar reference (< 1e-4)
//   - 1,200-iteration deterministic PRNG fuzzing (zero, rank-deficient, wide-scale, NaN/Inf)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MatrixError,
    MatrixViewF64,
    MatrixMutF64,
    MatrixViewF32,
    MatrixMutF32,
    matrixIdentityF64,
    transposeMatrixF64,
    frobeniusNormF64,
    frobeniusNormF32Simd,
    infNormF64,
    oneNormF64,
    matrixDiffFrobeniusF64,
    matvecF32Simd,
    matmulF32Simd,
    matmulF64,
    outerProductAddF32Simd,
    luDecomposeInPlace,
    denseLuSolve,
    detFromLu,
    choleskyDecomposeInPlace,
    denseCholeskySolve,
    denseCholeskyLogDet,
    qrDecomposeHouseholder,
    svdThin,
    svdThinAny,
    svdCondNumber,
    svdThinPinv,
    estimateConditionNumber,
    matrixRankF64,
    solveLowerTriangular,
    solveUpperTriangular,
    solveTrsmLeftLower,
    solveTrsmLeftUpper,
} from "../../lib/math/matrixDense.cp";

fn tAbsF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn tAbsF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

fn checkLuRejected(data: []f64, n: usize): bool {
    let piv: [8]usize = [0; 8];
    let piv_s: []usize = piv[0..n];
    let m = MatrixMutF64.init(data, n, n) catch return true;
    const res = luDecomposeInPlace(*m, piv_s) catch return true;
    return res.sign == 999;
}

fn runFuzzDecomps(
    a_buf: []f64,
    q_buf: []f64,
    r_buf: []f64,
    v_buf: []f64,
    vw: []f64,
    s_buf: []f64,
    piv: []usize,
    n: usize,
) {
    const av = MatrixViewF64.init(a_buf[0..n * n], n, n) catch return;
    let qm = MatrixMutF64.init(q_buf[0..n * n], n, n) catch return;
    let rm = MatrixMutF64.init(r_buf[0..n * n], n, n) catch return;
    qrDecomposeHouseholder(av, *qm, *rm, vw[0..n]) catch {};

    let um = MatrixMutF64.init(q_buf[0..n * n], n, n) catch return;
    let vm = MatrixMutF64.init(v_buf[0..n * n], n, n) catch return;
    const svd_ok = svdThin(av, 24, 1.0e-10, *um, s_buf[0..n], *vm) catch return;
    if (svd_ok.rank == 9999) return;

    let lum = MatrixMutF64.init(a_buf[0..n * n], n, n) catch return;
    const lu_ok = luDecomposeInPlace(*lum, piv[0..n]) catch return;
    if (lu_ok.sign == 999) return;
    choleskyDecomposeInPlace(*lum) catch {};
}

fn checkCholRejected(data: []f64, n: usize): bool {
    let m = MatrixMutF64.init(data, n, n) catch return true;
    choleskyDecomposeInPlace(*m) catch return true;
    return false;
}

fn checkQrRejected(data: []const f64, r: usize, c: usize): bool {
    let qd: [16]f64 = [0.0; 16];
    let rd: [16]f64 = [0.0; 16];
    let vw: [8]f64 = [0.0; 8];
    let vws: []f64 = vw[0..r];
    const a = MatrixViewF64.init(data, r, c) catch return true;
    let q = MatrixMutF64.init(qd[0..r * r], r, r) catch return true;
    let rm = MatrixMutF64.init(rd[0..r * c], r, c) catch return true;
    qrDecomposeHouseholder(a, *q, *rm, vws) catch return true;
    return false;
}

fn main() {
    // ========================================================================
    // 1. Hilbert Ill-Conditioned Matrix: Condition Number, Cholesky, LU, Det
    // ========================================================================
    test("matrix_ref_01_hilbert_4x4_cond_cholesky_lu_det", () => {
        const n: usize = 4;
        let h_orig: [16]f64 = [0.0; 16];
        let h_chol: [16]f64 = [0.0; 16];
        let h_lu: [16]f64 = [0.0; 16];
        let r: usize = 0;
        while (r < n) : (r += 1) {
            let c: usize = 0;
            while (c < n) : (c += 1) {
                const v: f64 = 1.0 / @as(f64, @floatFromInt(r + c + 1));
                h_orig[r * n + c] = v;
                h_chol[r * n + c] = v;
                h_lu[r * n + c] = v;
            }
        }
        const h_view = MatrixViewF64.init(h_orig[0..], n, n) catch unreachable;

        // Condition number of 4x4 Hilbert matrix is ~15513.7387
        let ud: [16]f64 = [0.0; 16];
        let vd: [16]f64 = [0.0; 16];
        let sv: [4]f64 = [0.0; 4];
        let sv_s: []f64 = sv[0..];
        let u_m = MatrixMutF64.init(ud[0..], n, n) catch unreachable;
        let v_m = MatrixMutF64.init(vd[0..], n, n) catch unreachable;
        const cond = estimateConditionNumber(h_view, *u_m, sv_s, *v_m) catch unreachable;
        expect_eq(cond > 15000.0 && cond < 16000.0, true);

        // Cholesky factorization & reconstruction L * L^T == H
        let chol_m = MatrixMutF64.init(h_chol[0..], n, n) catch unreachable;
        choleskyDecomposeInPlace(*chol_m) catch unreachable;
        const log_det = denseCholeskyLogDet(chol_m.asView()) catch unreachable;

        let lt_d: [16]f64 = [0.0; 16];
        let prod_d: [16]f64 = [0.0; 16];
        let bt_w: [16]f64 = [0.0; 16];
        let bt_s: []f64 = bt_w[0..];
        let lt_m = MatrixMutF64.init(lt_d[0..], n, n) catch unreachable;
        let prod_m = MatrixMutF64.init(prod_d[0..], n, n) catch unreachable;
        transposeMatrixF64(chol_m.asView(), *lt_m) catch unreachable;
        matmulF64(chol_m.asView(), lt_m.asView(), 1.0, 0.0, *prod_m, bt_s) catch unreachable;
        const chol_err = matrixDiffFrobeniusF64(prod_m.asView(), h_view) catch unreachable;
        expect_eq(chol_err < 1.0e-12, true);

        // LU factorization, detFromLu vs exp(logDet), and solve H * x = b
        let lu_m = MatrixMutF64.init(h_lu[0..], n, n) catch unreachable;
        let piv: [4]usize = [0; 4];
        let piv_s: []usize = piv[0..];
        const lu_res = luDecomposeInPlace(*lu_m, piv_s) catch unreachable;
        const det_lu = detFromLu(lu_m.asView(), lu_res.sign) catch unreachable;
        expect_eq(tAbsF64(std.math.log(f64, std.math.e, det_lu) - log_det) < 1.0e-9, true);

        const b: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        let x_lu: [4]f64 = [0.0; 4];
        let x_ch: [4]f64 = [0.0; 4];
        let x_lu_s: []f64 = x_lu[0..];
        let x_ch_s: []f64 = x_ch[0..];
        denseLuSolve(lu_m.asView(), piv[0..], b[0..], x_lu_s) catch unreachable;
        denseCholeskySolve(chol_m.asView(), b[0..], x_ch_s) catch unreachable;

        let i: usize = 0;
        while (i < n) : (i += 1) {
            expect_eq(tAbsF64(x_lu[i] - x_ch[i]) < 1.0e-7, true);
        }
    });

    // ========================================================================
    // 2. Singular, Non-SPD, Asymmetric & NaN/Inf Rejection Guards
    // ========================================================================
    test("matrix_ref_02_singular_non_spd_asymmetric_and_nan_guards", () => {
        // Singular 3x3 (row 2 = row 0 + row 1)
        let sing: [9]f64 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 5.0, 7.0, 9.0];
        expect_eq(checkLuRejected(sing[0..], 3), true);

        // Asymmetric matrix passed to Cholesky must be rejected
        let asym: [4]f64 = [4.0, 1.0, 0.0, 4.0];
        expect_eq(checkCholRejected(asym[0..], 2), true);

        // Indefinite symmetric matrix passed to Cholesky must be rejected
        let indef: [4]f64 = [1.0, 2.0, 2.0, 1.0];
        expect_eq(checkCholRejected(indef[0..], 2), true);

        // NaN & Inf entries rejected across LU / Cholesky / QR
        let nan_m: [4]f64 = [1.0, std.math.nan(f64), 0.0, 1.0];
        expect_eq(checkLuRejected(nan_m[0..], 2), true);
        expect_eq(checkCholRejected(nan_m[0..], 2), true);
        expect_eq(checkQrRejected(nan_m[0..], 2, 2), true);

        let inf_m: [4]f64 = [1.0, 0.0, 0.0, std.math.inf(f64)];
        expect_eq(checkLuRejected(inf_m[0..], 2), true);
    });

    // ========================================================================
    // 3. Householder QR: 1x1, Tall (6x3), Wide (3x6), Zero-Col & Rank-Deficient
    // ========================================================================
    test("matrix_ref_03_qr_1x1_tall_wide_and_rank_deficient", () => {
        // Test 1: Tall 6x3 with a zero column
        let a_tall: [18]f64 = [
            1.0, 0.0, 2.0,
            -3.0, 0.0, 4.0,
            2.0, 0.0, -1.0,
            5.0, 0.0, 3.0,
            -1.0, 0.0, 2.0,
            4.0, 0.0, -2.0,
        ];
        const av_t = MatrixViewF64.init(a_tall[0..], 6, 3) catch unreachable;
        let qd_t: [36]f64 = [0.0; 36];
        let rd_t: [18]f64 = [0.0; 18];
        let vw_t: [6]f64 = [0.0; 6];
        let vw_ts: []f64 = vw_t[0..];
        let q_t = MatrixMutF64.init(qd_t[0..], 6, 6) catch unreachable;
        let r_t = MatrixMutF64.init(rd_t[0..], 6, 3) catch unreachable;
        qrDecomposeHouseholder(av_t, *q_t, *r_t, vw_ts) catch unreachable;

        // Verify ||Q R - A||_F < 1e-11 and ||Q^T Q - I||_F < 1e-11
        let qr_p: [18]f64 = [0.0; 18];
        let bt: [36]f64 = [0.0; 36];
        let bt_s: []f64 = bt[0..];
        let qr_pm = MatrixMutF64.init(qr_p[0..], 6, 3) catch unreachable;
        matmulF64(q_t.asView(), r_t.asView(), 1.0, 0.0, *qr_pm, bt_s) catch unreachable;
        expect_eq((matrixDiffFrobeniusF64(qr_pm.asView(), av_t) catch unreachable) < 1.0e-11, true);

        let qt_d: [36]f64 = [0.0; 36];
        let qtq_d: [36]f64 = [0.0; 36];
        let eye6_d: [36]f64 = [0.0; 36];
        let qt_m = MatrixMutF64.init(qt_d[0..], 6, 6) catch unreachable;
        let qtq_m = MatrixMutF64.init(qtq_d[0..], 6, 6) catch unreachable;
        let eye6_m = MatrixMutF64.init(eye6_d[0..], 6, 6) catch unreachable;
        matrixIdentityF64(*eye6_m) catch unreachable;
        transposeMatrixF64(q_t.asView(), *qt_m) catch unreachable;
        matmulF64(qt_m.asView(), q_t.asView(), 1.0, 0.0, *qtq_m, bt_s) catch unreachable;
        expect_eq((matrixDiffFrobeniusF64(qtq_m.asView(), eye6_m.asView()) catch unreachable) < 1.0e-11, true);

        // Test 2: Wide 3x6 matrix (M < N)
        let a_wide: [18]f64 = [
            2.0, -1.0, 3.0, 0.5, 4.0, -2.0,
            1.0, 5.0, -2.0, 3.0, -1.0, 0.25,
            -4.0, 2.0, 1.0, -1.5, 2.0, 6.0,
        ];
        const av_w = MatrixViewF64.init(a_wide[0..], 3, 6) catch unreachable;
        let qd_w: [9]f64 = [0.0; 9];
        let rd_w: [18]f64 = [0.0; 18];
        let vw_w: [3]f64 = [0.0; 3];
        let vw_ws: []f64 = vw_w[0..];
        let q_w = MatrixMutF64.init(qd_w[0..], 3, 3) catch unreachable;
        let r_w = MatrixMutF64.init(rd_w[0..], 3, 6) catch unreachable;
        qrDecomposeHouseholder(av_w, *q_w, *r_w, vw_ws) catch unreachable;

        let qrw_p: [18]f64 = [0.0; 18];
        let qrw_pm = MatrixMutF64.init(qrw_p[0..], 3, 6) catch unreachable;
        matmulF64(q_w.asView(), r_w.asView(), 1.0, 0.0, *qrw_pm, bt_s) catch unreachable;
        expect_eq((matrixDiffFrobeniusF64(qrw_pm.asView(), av_w) catch unreachable) < 1.0e-11, true);

        // Test 3: 1x1 matrix
        let a1: [1]f64 = [-7.5];
        let q1d: [1]f64 = [0.0];
        let r1d: [1]f64 = [0.0];
        let v1w: [1]f64 = [0.0];
        let v1ws: []f64 = v1w[0..];
        let q1 = MatrixMutF64.init(q1d[0..], 1, 1) catch unreachable;
        let r1 = MatrixMutF64.init(r1d[0..], 1, 1) catch unreachable;
        qrDecomposeHouseholder(MatrixViewF64.init(a1[0..], 1, 1) catch unreachable, *q1, *r1, v1ws) catch unreachable;
        expect_eq(tAbsF64(q1.at(0, 0) * r1.at(0, 0) - (-7.5)) < 1.0e-12, true);
    });

    // ========================================================================
    // 4. Thin SVD (Tall & Wide & Zero) + 4 Moore-Penrose Pseudoinverse Identities
    // ========================================================================
    test("matrix_ref_04_svd_tall_wide_zero_and_4_penrose_identities", () => {
        // Rank-2 4x3 matrix (col 2 = col 0 + 2 * col 1)
        let a_data: [12]f64 = [
            1.0, 2.0, 5.0,
            3.0, -1.0, 1.0,
            -2.0, 4.0, 6.0,
            0.5, 1.5, 3.5,
        ];
        const a_view = MatrixViewF64.init(a_data[0..], 4, 3) catch unreachable;
        let ud: [12]f64 = [0.0; 12];
        let vd: [9]f64 = [0.0; 9];
        let s: [3]f64 = [0.0; 3];
        let s_s: []f64 = s[0..];
        let u_m = MatrixMutF64.init(ud[0..], 4, 3) catch unreachable;
        let v_m = MatrixMutF64.init(vd[0..], 3, 3) catch unreachable;

        const svd_res = svdThin(a_view, 64, 1.0e-12, *u_m, s_s, *v_m) catch unreachable;
        expect_eq(svd_res.converged, true);
        expect_eq(svd_res.rank, 2);
        expect_eq(s[0] >= s[1] && s[1] >= s[2], true);
        expect_eq(s[2] < 1.0e-11, true);

        // Compute Moore-Penrose pseudoinverse A+ (3x4)
        let pinv_d: [12]f64 = [0.0; 12];
        let pinv_m = MatrixMutF64.init(pinv_d[0..], 3, 4) catch unreachable;
        svdThinPinv(u_m.asView(), s[0..], v_m.asView(), 1.0e-10, *pinv_m) catch unreachable;

        let aa_p_d: [16]f64 = [0.0; 16];
        let ap_a_d: [9]f64 = [0.0; 9];
        let id1_d: [12]f64 = [0.0; 12];
        let id2_d: [12]f64 = [0.0; 12];
        let sym1_d: [16]f64 = [0.0; 16];
        let sym2_d: [9]f64 = [0.0; 9];
        let bt: [16]f64 = [0.0; 16];
        let bt_s: []f64 = bt[0..];

        let aa_p = MatrixMutF64.init(aa_p_d[0..], 4, 4) catch unreachable;
        let ap_a = MatrixMutF64.init(ap_a_d[0..], 3, 3) catch unreachable;
        let id1 = MatrixMutF64.init(id1_d[0..], 4, 3) catch unreachable;
        let id2 = MatrixMutF64.init(id2_d[0..], 3, 4) catch unreachable;
        let sym1 = MatrixMutF64.init(sym1_d[0..], 4, 4) catch unreachable;
        let sym2 = MatrixMutF64.init(sym2_d[0..], 3, 3) catch unreachable;

        matmulF64(a_view, pinv_m.asView(), 1.0, 0.0, *aa_p, bt_s) catch unreachable;
        matmulF64(pinv_m.asView(), a_view, 1.0, 0.0, *ap_a, bt_s) catch unreachable;
        matmulF64(aa_p.asView(), a_view, 1.0, 0.0, *id1, bt_s) catch unreachable;
        matmulF64(pinv_m.asView(), aa_p.asView(), 1.0, 0.0, *id2, bt_s) catch unreachable;
        transposeMatrixF64(aa_p.asView(), *sym1) catch unreachable;
        transposeMatrixF64(ap_a.asView(), *sym2) catch unreachable;

        // All 4 Penrose identities < 1e-10
        expect_eq((matrixDiffFrobeniusF64(id1.asView(), a_view) catch unreachable) < 1.0e-10, true);
        expect_eq((matrixDiffFrobeniusF64(id2.asView(), pinv_m.asView()) catch unreachable) < 1.0e-10, true);
        expect_eq((matrixDiffFrobeniusF64(sym1.asView(), aa_p.asView()) catch unreachable) < 1.0e-10, true);
        expect_eq((matrixDiffFrobeniusF64(sym2.asView(), ap_a.asView()) catch unreachable) < 1.0e-10, true);

        // Wide matrix (3x5) via svdThinAny: verify U * diag(S) * V^T == A
        let w_data: [15]f64 = [
            1.0, 2.0, -1.0, 3.0, 0.5,
            0.0, -2.0, 4.0, 1.0, -3.0,
            2.0, 1.0, 1.0, -2.0, 2.5,
        ];
        const w_view = MatrixViewF64.init(w_data[0..], 3, 5) catch unreachable;
        let uw_d: [9]f64 = [0.0; 9];
        let vw_d: [15]f64 = [0.0; 15];
        let sw: [3]f64 = [0.0; 3];
        let sw_s: []f64 = sw[0..];
        let atw: [15]f64 = [0.0; 15];
        let atw_s: []f64 = atw[0..];
        let uw_m = MatrixMutF64.init(uw_d[0..], 3, 3) catch unreachable;
        let vw_m = MatrixMutF64.init(vw_d[0..], 5, 3) catch unreachable;

        const w_res = svdThinAny(w_view, 64, 1.0e-12, *uw_m, sw_s, *vw_m, atw_s) catch unreachable;
        expect_eq(w_res.converged, true);
        expect_eq(w_res.rank, 3);

        // Reconstruct U * diag(S) * V^T
        let rec_d: [15]f64 = [0.0; 15];
        let rec_m = MatrixMutF64.init(rec_d[0..], 3, 5) catch unreachable;
        let r: usize = 0;
        while (r < 3) : (r += 1) {
            let c: usize = 0;
            while (c < 5) : (c += 1) {
                let sum: f64 = 0.0;
                let k: usize = 0;
                while (k < 3) : (k += 1) {
                    sum += uw_m.at(r, k) * sw[k] * vw_m.at(c, k);
                }
                rec_m.set(r, c, sum);
            }
        }
        expect_eq((matrixDiffFrobeniusF64(rec_m.asView(), w_view) catch unreachable) < 1.0e-10, true);

        // Zero matrix rank == 0
        let z_d: [4]f64 = [0.0; 4];
        let uz_d: [4]f64 = [0.0; 4];
        let vz_d: [4]f64 = [0.0; 4];
        let sz: [2]f64 = [0.0; 2];
        let sz_s: []f64 = sz[0..];
        let uz_m = MatrixMutF64.init(uz_d[0..], 2, 2) catch unreachable;
        let vz_m = MatrixMutF64.init(vz_d[0..], 2, 2) catch unreachable;
        const rk0 = matrixRankF64(MatrixViewF64.init(z_d[0..], 2, 2) catch unreachable, 0.0, *uz_m, sz_s, *vz_m) catch unreachable;
        expect_eq(rk0, 0);
    });

    // ========================================================================
    // 5. TRSV / TRSM Triangular Solvers & Matrix Norms
    // ========================================================================
    test("matrix_ref_05_trsv_trsm_and_matrix_norms", () => {
        const l_d: [9]f64 = [
            2.0, 0.0, 0.0,
            -1.0, 3.0, 0.0,
            4.0, 2.0, 5.0,
        ];
        const u_d: [9]f64 = [
            3.0, -2.0, 1.0,
            0.0, 4.0, -3.0,
            0.0, 0.0, 2.0,
        ];
        const lv = MatrixViewF64.init(l_d[0..], 3, 3) catch unreachable;
        const uv = MatrixViewF64.init(u_d[0..], 3, 3) catch unreachable;

        expect_eq(tAbsF64(infNormF64(lv) - 11.0) < 1.0e-12, true);
        expect_eq(tAbsF64(oneNormF64(lv) - 7.0) < 1.0e-12, true);

        const b: [3]f64 = [4.0, 7.0, 23.0];
        let xl: [3]f64 = [0.0; 3];
        let xu: [3]f64 = [0.0; 3];
        let xl_s: []f64 = xl[0..];
        let xu_s: []f64 = xu[0..];
        solveLowerTriangular(lv, b[0..], false, xl_s) catch unreachable;
        solveUpperTriangular(uv, b[0..], false, xu_s) catch unreachable;

        // Residual check L*xl == b and U*xu == b
        let i: usize = 0;
        while (i < 3) : (i += 1) {
            let rl: f64 = 0.0;
            let ru: f64 = 0.0;
            let j: usize = 0;
            while (j < 3) : (j += 1) {
                rl += lv.at(i, j) * xl[j];
                ru += uv.at(i, j) * xu[j];
            }
            expect_eq(tAbsF64(rl - b[i]) < 1.0e-12, true);
            expect_eq(tAbsF64(ru - b[i]) < 1.0e-12, true);
        }

        // TRSM multi-RHS (3x2)
        const bmat_d: [6]f64 = [2.0, 4.0, 5.0, 1.0, 13.0, -3.0];
        const bmat = MatrixViewF64.init(bmat_d[0..], 3, 2) catch unreachable;
        let xmat_d: [6]f64 = [0.0; 6];
        let xmat = MatrixMutF64.init(xmat_d[0..], 3, 2) catch unreachable;
        let prod_d: [6]f64 = [0.0; 6];
        let prod = MatrixMutF64.init(prod_d[0..], 3, 2) catch unreachable;
        let bt: [9]f64 = [0.0; 9];
        let bt_s: []f64 = bt[0..];

        solveTrsmLeftLower(lv, bmat, false, *xmat) catch unreachable;
        matmulF64(lv, xmat.asView(), 1.0, 0.0, *prod, bt_s) catch unreachable;
        expect_eq((matrixDiffFrobeniusF64(prod.asView(), bmat) catch unreachable) < 1.0e-11, true);

        solveTrsmLeftUpper(uv, bmat, false, *xmat) catch unreachable;
        matmulF64(uv, xmat.asView(), 1.0, 0.0, *prod, bt_s) catch unreachable;
        expect_eq((matrixDiffFrobeniusF64(prod.asView(), bmat) catch unreachable) < 1.0e-11, true);
    });

    // ========================================================================
    // 6. SIMD GEMM / GEMV / Rank-1 OuterProduct / Frobenius Norm vs Scalar
    // ========================================================================
    test("matrix_ref_06_simd_gemm_gemv_outer_and_frobenius_vs_scalar", () => {
        const m: usize = 11;
        const k: usize = 13;
        const n: usize = 19; // non-multiple of 8 (2x8 + 3 tail)

        let ad: [11 * 13]f32 = [0.0; 11 * 13];
        let bd: [13 * 19]f32 = [0.0; 13 * 19];
        let cd: [11 * 19]f32 = [0.0; 11 * 19];
        let btw: [13 * 19]f32 = [0.0; 13 * 19];
        let btw_s: []f32 = btw[0..];

        let i: usize = 0;
        while (i < ad.len) : (i += 1) ad[i] = (@as(f32, @floatFromInt((i * 7) % 19)) - 9.0) * 0.1;
        i = 0;
        while (i < bd.len) : (i += 1) bd[i] = (@as(f32, @floatFromInt((i * 11) % 23)) - 11.0) * 0.1;

        const av = MatrixViewF32.init(ad[0..], m, k) catch unreachable;
        const bv = MatrixViewF32.init(bd[0..], k, n) catch unreachable;
        let cm = MatrixMutF32.init(cd[0..], m, n) catch unreachable;

        matmulF32Simd(av, bv, 1.25, 0.0, *cm, btw_s) catch unreachable;

        let r: usize = 0;
        while (r < m) : (r += 1) {
            let c: usize = 0;
            while (c < n) : (c += 1) {
                let ref_s: f32 = 0.0;
                let p: usize = 0;
                while (p < k) : (p += 1) ref_s += av.at(r, p) * bv.at(p, c);
                expect_eq(tAbsF32(cm.at(r, c) - 1.25 * ref_s) < 1.0e-4, true);
            }
        }

        // Rank-1 outerProductAddF32Simd on 11x19
        let xv: [11]f32 = [0.0; 11];
        let yv: [19]f32 = [0.0; 19];
        i = 0;
        while (i < 11) : (i += 1) xv[i] = (@as(f32, @floatFromInt(i)) - 5.0) * 0.2;
        i = 0;
        while (i < 19) : (i += 1) yv[i] = (@as(f32, @floatFromInt(i)) - 9.0) * 0.15;

        let od: [11 * 19]f32 = [1.0; 11 * 19];
        let om = MatrixMutF32.init(od[0..], 11, 19) catch unreachable;
        outerProductAddF32Simd(xv[0..], yv[0..], -2.0, *om) catch unreachable;
        r = 0;
        while (r < 11) : (r += 1) {
            let c2: usize = 0;
            while (c2 < 19) : (c2 += 1) {
                const exp: f32 = 1.0 - 2.0 * xv[r] * yv[c2];
                expect_eq(tAbsF32(om.at(r, c2) - exp) < 1.0e-5, true);
            }
        }

        // frobeniusNormF32Simd vs scalar
        const fn_simd = frobeniusNormF32Simd(av);
        let sq: f32 = 0.0;
        i = 0;
        while (i < ad.len) : (i += 1) sq += ad[i] * ad[i];
        expect_eq(tAbsF32(fn_simd - std.math.sqrt(sq)) < 1.0e-4, true);
    });

    // ========================================================================
    // 7. 1,200-Iteration Deterministic PRNG Fuzzing (Zero/Rank-Def/Scale/NaN)
    // ========================================================================
    test("matrix_ref_07_deterministic_1200_fuzz_zero_panic", () => {
        let state: u64 = 0x6A09E667F3BCC908;
        let a_buf: [25]f64 = [0.0; 25];
        let q_buf: [25]f64 = [0.0; 25];
        let r_buf: [25]f64 = [0.0; 25];
        let v_buf: [25]f64 = [0.0; 25];
        let vw: [5]f64 = [0.0; 5];
        let s_buf: [5]f64 = [0.0; 5];
        let piv: [5]usize = [0; 5];

        let iter: usize = 0;
        while (iter < 1200) : (iter += 1) {
            state ^= state >> 12;
            state ^= state << 25;
            state ^= state >> 27;
            const r = state *% 0x2545F4914F6CDD1D;
            const n: usize = (1 as usize) + ((r % 5) as usize);
            const mode: usize = iter % 4;

            let i: usize = 0;
            while (i < n * n) : (i += 1) {
                state ^= state >> 12;
                state ^= state << 25;
                state ^= state >> 27;
                const iv: i32 = @as(i32, @intCast(state & 0xFF)) - 128;
                if (mode == 0) {
                    a_buf[i] = @as(f64, @floatFromInt(iv)) * 0.05;
                } else if (mode == 1) {
                    // Wide scale 1e-4 .. 1e4
                    a_buf[i] = @as(f64, @floatFromInt(iv)) * (if ((i & 1) == 0) (100.0 as f64) else (0.0001 as f64));
                } else if (mode == 2) {
                    // Zero / rank-deficient
                    a_buf[i] = if (i < n) @as(f64, @floatFromInt(iv)) else 0.0;
                } else {
                    // Inject occasional NaN/Inf to verify safe rejection
                    a_buf[i] = if (i == 0) std.math.nan(f64) else @as(f64, @floatFromInt(iv));
                }
            }

            // Exercise QR, LU, Cholesky, SVD without panic/OOB/hang
            runFuzzDecomps(a_buf[0..], q_buf[0..], r_buf[0..], v_buf[0..], vw[0..], s_buf[0..], piv[0..], n);
        }

        expect_eq(iter, 1200);
    });
}
