// Source: cp/cap/test/test_svm_production_deep.cp (Production deep tests for cap:svm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    KernelKind,
    KernelParams,
    SmoConfig,
    SmoResult,
    LinearSvmResult,
    evalKernel,
    svmFitSmo,
    svmDecision,
    svmPredict,
    linearSvmFit,
    linearSvmDecision,
} from "cap:svm";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

test("1. Linearly separable 2D dataset: SMO Linear kernel 100% accuracy & KKT constraints", () => {
    // 10 points: 5 negative near (-2, -2), 5 positive near (+2, +2)
    const x: [20]f64 = [
        -2.0, -2.0,  -2.5, -1.5,  -1.5, -2.5,  -3.0, -2.0,  -2.0, -3.0,
         2.0,  2.0,   2.5,  1.5,   1.5,  2.5,   3.0,  2.0,   2.0,  3.0,
    ];
    const y: [10]f64 = [-1.0, -1.0, -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0];

    let alphas_b: [10]f64 = std.mem.zeroes([10]f64);
    let errs_b: [10]f64 = std.mem.zeroes([10]f64);
    let alphas: []f64 = alphas_b[0..];
    let errs: []f64 = errs_b[0..];

    const kp: KernelParams = KernelParams.linear();
    let cfg: SmoConfig = SmoConfig.default();
    cfg.c = 10.0;
    cfg.tol = 1.0e-4;

    const res: SmoResult = svmFitSmo(x[0..], y[0..], 10, 2, kp, cfg, *alphas, *errs) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(res.support_vectors >= 2, true);

    // KKT constraints: 0 <= alpha_i <= C and sum(alpha_i * y_i) == 0
    let ay_sum: f64 = 0.0;
    let i: usize = 0;
    while (i < 10) {
        expect_eq(alphas[i] >= -1.0e-12 && alphas[i] <= cfg.c + 1.0e-12, true);
        ay_sum += alphas[i] * y[i];
        const pred: f64 = svmPredict(x[0..], y[0..], alphas, res.bias, 10, 2, kp, x[(i * 2)..(i * 2 + 2)]) ?? unreachable;
        expect_eq(pred, y[i]);
        i += 1;
    }
    expect_eq(absF64(ay_sum) < 1.0e-9, true);

    // Unseen test points
    const q_neg: [2]f64 = [-1.0, -1.2];
    const q_pos: [2]f64 = [1.2, 1.0];
    expect_eq(svmPredict(x[0..], y[0..], alphas, res.bias, 10, 2, kp, q_neg[0..]) ?? unreachable, -1.0);
    expect_eq(svmPredict(x[0..], y[0..], alphas, res.bias, 10, 2, kp, q_pos[0..]) ?? unreachable, 1.0);
});

test("2. Non-linear XOR & concentric circles 100% separated by RBF kernel SMO", () => {
    // (a) XOR dataset (8 points: 2 per quadrant)
    const x_xor: [16]f64 = [
        -1.0, -1.0,  -0.8, -1.2,
         1.0,  1.0,   1.2,  0.8,
        -1.0,  1.0,  -1.2,  0.8,
         1.0, -1.0,   0.8, -1.2,
    ];
    const y_xor: [8]f64 = [1.0, 1.0, 1.0, 1.0, -1.0, -1.0, -1.0, -1.0];

    let a_xor_b: [8]f64 = std.mem.zeroes([8]f64);
    let e_xor_b: [8]f64 = std.mem.zeroes([8]f64);
    let a_xor: []f64 = a_xor_b[0..];
    let e_xor: []f64 = e_xor_b[0..];

    const kp_rbf: KernelParams = KernelParams.rbf(1.0);
    let cfg: SmoConfig = SmoConfig.default();
    cfg.c = 10.0;
    const r_xor: SmoResult = svmFitSmo(x_xor[0..], y_xor[0..], 8, 2, kp_rbf, cfg, *a_xor, *e_xor) ?? unreachable;
    expect_eq(r_xor.converged, true);

    let i: usize = 0;
    while (i < 8) {
        const pred: f64 = svmPredict(x_xor[0..], y_xor[0..], a_xor, r_xor.bias, 8, 2, kp_rbf, x_xor[(i * 2)..(i * 2 + 2)]) ?? unreachable;
        expect_eq(pred, y_xor[i]);
        i += 1;
    }

    // (b) Concentric circles: inner radius 0.5 (+1), outer radius 2.5 (-1)
    let x_circ: [32]f64 = std.mem.zeroes([32]f64);
    let y_circ: [16]f64 = std.mem.zeroes([16]f64);
    const step_ang: f64 = 2.0 * 3.14159265358979323846 / 8.0;
    let k: usize = 0;
    while (k < 8) {
        const ang: f64 = (k as f64) * step_ang;
        x_circ[k * 2 + 0] = 0.5 * std.math.cos(ang);
        x_circ[k * 2 + 1] = 0.5 * std.math.sin(ang);
        y_circ[k] = 1.0;

        x_circ[(8 + k) * 2 + 0] = 2.5 * std.math.cos(ang);
        x_circ[(8 + k) * 2 + 1] = 2.5 * std.math.sin(ang);
        y_circ[8 + k] = -1.0;
        k += 1;
    }

    let a_c_b: [16]f64 = std.mem.zeroes([16]f64);
    let e_c_b: [16]f64 = std.mem.zeroes([16]f64);
    let a_c: []f64 = a_c_b[0..];
    let e_c: []f64 = e_c_b[0..];

    const kp_c: KernelParams = KernelParams.rbf(0.5);
    const r_c: SmoResult = svmFitSmo(x_circ[0..], y_circ[0..], 16, 2, kp_c, cfg, *a_c, *e_c) ?? unreachable;
    expect_eq(r_c.converged, true);

    let j: usize = 0;
    while (j < 16) {
        const pred: f64 = svmPredict(x_circ[0..], y_circ[0..], a_c, r_c.bias, 16, 2, kp_c, x_circ[(j * 2)..(j * 2 + 2)]) ?? unreachable;
        expect_eq(pred, y_circ[j]);
        j += 1;
    }
    // Origin is strictly inside (+1), far point is strictly outside (-1)
    const origin: [2]f64 = [0.0, 0.0];
    const far_pt: [2]f64 = [3.0, 0.0];
    expect_eq(svmPredict(x_circ[0..], y_circ[0..], a_c, r_c.bias, 16, 2, kp_c, origin[0..]) ?? unreachable, 1.0);
    expect_eq(svmPredict(x_circ[0..], y_circ[0..], a_c, r_c.bias, 16, 2, kp_c, far_pt[0..]) ?? unreachable, -1.0);
});

test("3. Polynomial degree-2 kernel separates XOR & evalKernel exactness", () => {
    const va: [2]f64 = [1.0, 2.0];
    const vb: [2]f64 = [3.0, -1.0];
    // Linear: 1*3 + 2*(-1) = 1.0
    expect_eq(absF64(evalKernel(va[0..], vb[0..], KernelParams.linear()) - 1.0) < 1.0e-12, true);
    // Poly(2.0, 1.0, 3): (2*1 + 1)^3 = 27.0
    expect_eq(absF64(evalKernel(va[0..], vb[0..], KernelParams.poly(2.0, 1.0, 3)) - 27.0) < 1.0e-12, true);
    // Rbf(0.5): ||a - b||^2 = 4 + 9 = 13 -> exp(-6.5)
    expect_eq(absF64(evalKernel(va[0..], vb[0..], KernelParams.rbf(0.5)) - std.math.exp(-6.5)) < 1.0e-14, true);

    const x_xor: [8]f64 = [-1.0, -1.0, 1.0, 1.0, -1.0, 1.0, 1.0, -1.0];
    const y_xor: [4]f64 = [1.0, 1.0, -1.0, -1.0];
    let a_b: [4]f64 = std.mem.zeroes([4]f64);
    let e_b: [4]f64 = std.mem.zeroes([4]f64);
    let a: []f64 = a_b[0..];
    let e: []f64 = e_b[0..];

    const kp_poly: KernelParams = KernelParams.poly(1.0, 1.0, 2);
    let cfg: SmoConfig = SmoConfig.default();
    cfg.c = 10.0;
    const res: SmoResult = svmFitSmo(x_xor[0..], y_xor[0..], 4, 2, kp_poly, cfg, *a, *e) ?? unreachable;
    expect_eq(res.converged, true);

    let i: usize = 0;
    while (i < 4) {
        const pred: f64 = svmPredict(x_xor[0..], y_xor[0..], a, res.bias, 4, 2, kp_poly, x_xor[(i * 2)..(i * 2 + 2)]) ?? unreachable;
        expect_eq(pred, y_xor[i]);
        i += 1;
    }
});

test("4. High-dimensional (8D) Pegasos linearSvmFit achieves 100% accuracy & deterministic reproducibility", () => {
    // 20 samples in 8D: first 10 centered around +1.5 on dims 0..3, next 10 around -1.5
    let x: [160]f64 = std.mem.zeroes([160]f64);
    let y: [20]f64 = std.mem.zeroes([20]f64);

    let i: usize = 0;
    while (i < 10) {
        const offset: f64 = ((i as f64) - 4.5) * 0.05;
        let d: usize = 0;
        while (d < 8) {
            x[i * 8 + d] = if (d < 4) (1.5 + offset) else offset;
            x[(10 + i) * 8 + d] = if (d < 4) (-1.5 + offset) else offset;
            d += 1;
        }
        y[i] = 1.0;
        y[10 + i] = -1.0;
        i += 1;
    }

    let w1_b: [8]f64 = std.mem.zeroes([8]f64);
    let w2_b: [8]f64 = std.mem.zeroes([8]f64);
    let w1: []f64 = w1_b[0..];
    let w2: []f64 = w2_b[0..];

    const r1: LinearSvmResult = linearSvmFit(x[0..], y[0..], 20, 8, 0.01, 50, 12345, *w1) ?? unreachable;
    const r2: LinearSvmResult = linearSvmFit(x[0..], y[0..], 20, 8, 0.01, 50, 12345, *w2) ?? unreachable;

    expect_eq(r1.iterations, 1000 as usize);
    expect_eq(r1.bias, r2.bias);
    let d_chk: usize = 0;
    while (d_chk < 8) {
        expect_eq(w1[d_chk], w2[d_chk]);
        d_chk += 1;
    }

    // 100% training classification accuracy
    let s: usize = 0;
    while (s < 20) {
        const score: f64 = linearSvmDecision(w1, r1.bias, x[(s * 8)..(s * 8 + 8)]) ?? unreachable;
        expect_eq(score * y[s] > 0.0, true);
        s += 1;
    }
});

test("5. Soft-margin SMO respects box constraint 0 <= alpha_i <= C on overlapping noisy data", () => {
    const x: [12]f64 = [
        -2.0, 0.0,  -1.5, 0.0,   0.5, 0.0, // noisy positive-side point labeled -1
         2.0, 0.0,   1.5, 0.0,  -0.5, 0.0, // noisy negative-side point labeled +1
    ];
    const y: [6]f64 = [-1.0, -1.0, -1.0, 1.0, 1.0, 1.0];

    let a_b: [6]f64 = std.mem.zeroes([6]f64);
    let e_b: [6]f64 = std.mem.zeroes([6]f64);
    let a: []f64 = a_b[0..];
    let e: []f64 = e_b[0..];

    let cfg: SmoConfig = SmoConfig.default();
    cfg.c = 0.5;
    const res: SmoResult = svmFitSmo(x[0..], y[0..], 6, 2, KernelParams.linear(), cfg, *a, *e) ?? unreachable;
    expect_eq(res.support_vectors >= 2, true);

    let sum_ay: f64 = 0.0;
    let i: usize = 0;
    while (i < 6) {
        expect_eq(a[i] >= -1.0e-12 && a[i] <= 0.5 + 1.0e-12, true);
        sum_ay += a[i] * y[i];
        i += 1;
    }
    expect_eq(absF64(sum_ay) < 1.0e-9, true);
    const dec0: f64 = svmDecision(x[0..], y[0..], a, res.bias, 6, 2, KernelParams.linear(), x[0..2]) ?? unreachable;
    expect_eq(dec0 < 0.0, true);
});

test("6. Defensive guards: invalid labels, single-class, bad hyperparams, short buffers & NaN", () => {
    const x: [8]f64 = [0.0, 0.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0];
    const y_ok: [4]f64 = [-1.0, -1.0, 1.0, 1.0];
    const y_bad: [4]f64 = [0.0, 1.0, -1.0, 1.0];
    const y_one: [4]f64 = [1.0, 1.0, 1.0, 1.0];

    let a_b: [4]f64 = std.mem.zeroes([4]f64);
    let e_b: [4]f64 = std.mem.zeroes([4]f64);
    let w_b: [2]f64 = std.mem.zeroes([2]f64);
    let a: []f64 = a_b[0..];
    let e: []f64 = e_b[0..];
    let w: []f64 = w_b[0..];

    const kp: KernelParams = KernelParams.linear();
    const cfg: SmoConfig = SmoConfig.default();

    // Invalid labels & single class
    expect_eq(svmFitSmo(x[0..], y_bad[0..], 4, 2, kp, cfg, *a, *e) == null, true);
    expect_eq(svmFitSmo(x[0..], y_one[0..], 4, 2, kp, cfg, *a, *e) == null, true);
    expect_eq(linearSvmFit(x[0..], y_bad[0..], 4, 2, 0.01, 10, 1, *w) == null, true);
    expect_eq(linearSvmFit(x[0..], y_one[0..], 4, 2, 0.01, 10, 1, *w) == null, true);

    // Invalid C / gamma / lambda / epochs
    let bad_c: SmoConfig = SmoConfig.default();
    bad_c.c = 0.0;
    expect_eq(svmFitSmo(x[0..], y_ok[0..], 4, 2, kp, bad_c, *a, *e) == null, true);
    expect_eq(svmFitSmo(x[0..], y_ok[0..], 4, 2, KernelParams.rbf(-1.0), cfg, *a, *e) == null, true);
    expect_eq(linearSvmFit(x[0..], y_ok[0..], 4, 2, 0.0, 10, 1, *w) == null, true);
    expect_eq(linearSvmFit(x[0..], y_ok[0..], 4, 2, 0.01, 0, 1, *w) == null, true);

    // Short buffers
    let short_a: []f64 = a_b[0..3];
    let short_w: []f64 = w_b[0..1];
    expect_eq(svmFitSmo(x[0..], y_ok[0..], 4, 2, kp, cfg, *short_a, *e) == null, true);
    expect_eq(linearSvmFit(x[0..], y_ok[0..], 4, 2, 0.01, 10, 1, *short_w) == null, true);

    // NaN guard
    const zero: f64 = 0.0;
    const nan_x: [8]f64 = [zero / zero, 0.0, 1.0, 1.0, 2.0, 2.0, 3.0, 3.0];
    expect_eq(svmFitSmo(nan_x[0..], y_ok[0..], 4, 2, kp, cfg, *a, *e) == null, true);
    expect_eq(linearSvmFit(nan_x[0..], y_ok[0..], 4, 2, 0.01, 10, 1, *w) == null, true);
    expect_eq(linearSvmDecision(w, zero / zero, x[0..2]) == null, true);
});
