// Source: cp/cap/test/test_logreg_production_deep.cp (Production deep tests for cap:logreg)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    sigmoid,
    logLossBinary,
    softmax,
    LogRegConfig,
    fitBinary,
    predictProbaBinary,
    predictBinary,
    fitMultinomial,
    predictProbaMultinomial,
    predictClassMultinomial,
    binaryMetrics,
} from "cap:ml";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn l2Norm(xs: []const f64): f64 {
    let s: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        s += xs[i] * xs[i];
        i += 1;
    }
    return @sqrt(s);
}

test("1. sigmoid numerical stability across extreme logits, symmetry & NaN propagation", () => {
    expect_eq(absF64(sigmoid(0.0) - 0.5) < 1.0e-15, true);

    // Symmetry: sigmoid(z) + sigmoid(-z) == 1.0
    const z_list: [4]f64 = [0.5, 2.5, 10.0, 35.0];
    let i: usize = 0;
    while (i < 4) {
        const z = z_list[i];
        const sum_sym = sigmoid(z) + sigmoid(0.0 - z);
        expect_eq(absF64(sum_sym - 1.0) < 1.0e-14, true);
        i += 1;
    }

    // Extreme positive / negative values must not overflow or produce NaN/Inf
    const pos_big = sigmoid(1000.0);
    const pos_huge = sigmoid(1.0e300);
    const pos_inf = sigmoid(std.math.inf(f64));
    expect_eq(pos_big, 1.0);
    expect_eq(pos_huge, 1.0);
    expect_eq(pos_inf, 1.0);

    const neg_big = sigmoid(-1000.0);
    const neg_huge = sigmoid(-1.0e300);
    const neg_inf = sigmoid(-std.math.inf(f64));
    expect_eq(neg_big, 0.0);
    expect_eq(neg_huge, 0.0);
    expect_eq(neg_inf, 0.0);

    // NaN propagates cleanly
    expect_eq(std.math.isNan(sigmoid(std.math.nan(f64))), true);
});

test("2. softmax max-shift stability, shift-invariance & logLossBinary accuracy", () => {
    const base_logits: [4]f64 = [1.0, 2.0, 0.5, -1.0];
    const shifted_pos: [4]f64 = [1001.0, 1002.0, 1000.5, 999.0];
    const shifted_neg: [4]f64 = [-999.0, -998.0, -999.5, -1001.0];

    let p_base_buf: [4]f64 = [0.0; 4];
    let p_pos_buf: [4]f64 = [0.0; 4];
    let p_neg_buf: [4]f64 = [0.0; 4];
    let p_base: []f64 = p_base_buf[0..];
    let p_pos: []f64 = p_pos_buf[0..];
    let p_neg: []f64 = p_neg_buf[0..];

    expect_eq(softmax(base_logits[0..], *p_base), true);
    expect_eq(softmax(shifted_pos[0..], *p_pos), true);
    expect_eq(softmax(shifted_neg[0..], *p_neg), true);

    let sum_base: f64 = 0.0;
    let sum_pos: f64 = 0.0;
    let sum_neg: f64 = 0.0;
    let i: usize = 0;
    while (i < 4) {
        expect_eq(p_base[i] > 0.0 && p_base[i] < 1.0, true);
        expect_eq(absF64(p_base[i] - p_pos[i]) < 1.0e-12, true);
        expect_eq(absF64(p_base[i] - p_neg[i]) < 1.0e-12, true);
        sum_base += p_base[i];
        sum_pos += p_pos[i];
        sum_neg += p_neg[i];
        i += 1;
    }
    expect_eq(absF64(sum_base - 1.0) < 1.0e-12, true);
    expect_eq(absF64(sum_pos - 1.0) < 1.0e-12, true);
    expect_eq(absF64(sum_neg - 1.0) < 1.0e-12, true);

    // logLossBinary exact check & extreme probability clipping (no Inf/NaN)
    const probs_ok: [2]f64 = [0.9, 0.1];
    const y_ok: [2]f64 = [1.0, 0.0];
    if (ll := logLossBinary(probs_ok[0..], y_ok[0..])) {
        const expected = 0.0 - @log(0.9);
        expect_eq(absF64(ll - expected) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }

    const probs_ext: [2]f64 = [1.0, 0.0];
    if (ll_ext := logLossBinary(probs_ext[0..], y_ok[0..])) {
        expect_eq(ll_ext >= 0.0 && ll_ext < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }
});

test("3. fitBinary converges on linearly separable 2D dataset with 100% accuracy & logLoss < 0.1", () => {
    // 10 samples, 2 features: class 0 in bottom-left, class 1 in top-right
    const x: [20]f64 = [
        -3.0, -2.5,
        -2.5, -1.8,
        -2.0, -2.2,
        -1.5, -1.0,
        -1.2, -2.4,
         2.5,  3.0,
         2.0,  1.8,
         1.8,  2.4,
         1.5,  1.2,
         3.0,  2.2,
    ];
    const y: [10]f64 = [0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0];

    let w_buf: [2]f64 = [0.0; 2];
    let g_buf: [2]f64 = [0.0; 2];
    let w_s: []f64 = w_buf[0..];
    let g_s: []f64 = g_buf[0..];

    const cfg: LogRegConfig = {
        learning_rate: 0.5,
        l2_reg: 1.0e-3,
        max_iter: 1000,
        tol: 1.0e-5,
    };

    if (res := fitBinary(x[0..], y[0..], 10, 2, cfg, *w_s, *g_s)) {
        expect_eq(res.converged, true);
        expect_eq(res.iterations > 0, true);
        expect_eq(res.loss < 0.1, true);

        let probs: [10]f64 = [0.0; 10];
        let correct: usize = 0;
        let i: usize = 0;
        while (i < 10) {
            const sample = x[(i * 2)..(i * 2 + 2)];
            if (p := predictProbaBinary(w_s, res.bias, sample)) {
                probs[i] = p;
            } else {
                expect_eq(true, false);
            }
            if (pred := predictBinary(w_s, res.bias, sample, 0.5)) {
                const expected_cls: u8 = if (y[i] == 1.0) 1 else 0;
                if (pred == expected_cls) {
                    correct += 1;
                }
            } else {
                expect_eq(true, false);
            }
            i += 1;
        }
        expect_eq(correct, 10);

        if (ll := logLossBinary(probs[0..], y[0..])) {
            expect_eq(ll < 0.1, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("4. Increasing L2 regularization strictly shrinks learned weight L2 norm", () => {
    const x: [16]f64 = [
        -2.2, -1.4,
        -1.8, -2.1,
        -1.0, -0.8,
        -0.5, -1.5,
         2.1,  1.6,
         1.7,  2.3,
         1.1,  0.9,
         0.8,  1.7,
    ];
    const y: [8]f64 = [0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0];

    let w_weak_buf: [2]f64 = [0.0; 2];
    let w_mid_buf: [2]f64 = [0.0; 2];
    let w_strong_buf: [2]f64 = [0.0; 2];
    let g_buf: [2]f64 = [0.0; 2];

    let w_weak: []f64 = w_weak_buf[0..];
    let w_mid: []f64 = w_mid_buf[0..];
    let w_strong: []f64 = w_strong_buf[0..];
    let g_s: []f64 = g_buf[0..];

    const cfg_weak: LogRegConfig = { learning_rate: 0.5, l2_reg: 1.0e-3, max_iter: 1500, tol: 1.0e-6 };
    const cfg_mid: LogRegConfig = { learning_rate: 0.5, l2_reg: 0.1, max_iter: 1500, tol: 1.0e-6 };
    const cfg_strong: LogRegConfig = { learning_rate: 0.5, l2_reg: 2.0, max_iter: 1500, tol: 1.0e-6 };

    const r1 = fitBinary(x[0..], y[0..], 8, 2, cfg_weak, *w_weak, *g_s);
    const r2 = fitBinary(x[0..], y[0..], 8, 2, cfg_mid, *w_mid, *g_s);
    const r3 = fitBinary(x[0..], y[0..], 8, 2, cfg_strong, *w_strong, *g_s);

    expect_eq(r1 != null, true);
    expect_eq(r2 != null, true);
    expect_eq(r3 != null, true);

    const n_weak = l2Norm(w_weak);
    const n_mid = l2Norm(w_mid);
    const n_strong = l2Norm(w_strong);

    expect_eq(n_weak > n_mid, true);
    expect_eq(n_mid > n_strong, true);
    expect_eq(n_strong > 0.0, true);
});

test("5. fitMultinomial 3-class 2D dataset reaches 100% accuracy & normalized probabilities", () => {
    // 12 samples, 2 features, 3 well-separated clusters:
    // Class 0 around (-3, -1), Class 1 around (3, -1), Class 2 around (0, 3.5)
    const x: [24]f64 = [
        -3.2, -1.1,
        -2.8, -0.7,
        -3.5, -1.5,
        -2.5, -1.3,
         3.1, -1.0,
         2.7, -0.6,
         3.4, -1.4,
         2.6, -1.2,
        -0.3,  3.4,
         0.2,  3.8,
         0.5,  3.1,
        -0.4,  3.6,
    ];
    const y_cls: [12]usize = [0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2];

    let w_buf: [6]f64 = [0.0; 6];
    let b_buf: [3]f64 = [0.0; 3];
    let gw_buf: [6]f64 = [0.0; 6];
    let gb_buf: [3]f64 = [0.0; 3];
    let prob_buf: [3]f64 = [0.0; 3];

    let w_s: []f64 = w_buf[0..];
    let b_s: []f64 = b_buf[0..];
    let gw_s: []f64 = gw_buf[0..];
    let gb_s: []f64 = gb_buf[0..];
    let prob_s: []f64 = prob_buf[0..];

    const cfg: LogRegConfig = {
        learning_rate: 0.5,
        l2_reg: 1.0e-3,
        max_iter: 1000,
        tol: 1.0e-5,
    };

    if (res := fitMultinomial(x[0..], y_cls[0..], 12, 2, 3, cfg, *w_s, *b_s, *gw_s, *gb_s, *prob_s)) {
        expect_eq(res.converged, true);
        expect_eq(res.loss < 0.1, true);

        let correct: usize = 0;
        let i: usize = 0;
        while (i < 12) {
            const sample = x[(i * 2)..(i * 2 + 2)];
            expect_eq(predictProbaMultinomial(w_s, b_s, 3, 2, sample, *prob_s), true);
            const row_sum = prob_s[0] + prob_s[1] + prob_s[2];
            expect_eq(absF64(row_sum - 1.0) < 1.0e-12, true);
            expect_eq(prob_s[0] >= 0.0 && prob_s[1] >= 0.0 && prob_s[2] >= 0.0, true);

            if (pred := predictClassMultinomial(w_s, b_s, 3, 2, sample, *prob_s)) {
                if (pred == y_cls[i]) {
                    correct += 1;
                }
            } else {
                expect_eq(true, false);
            }
            i += 1;
        }
        expect_eq(correct, 12);

        // Generalization check on unseen cluster centers
        const q0: [2]f64 = [-3.0, -1.0];
        const q1: [2]f64 = [3.0, -1.0];
        const q2: [2]f64 = [0.0, 3.5];
        expect_eq(predictClassMultinomial(w_s, b_s, 3, 2, q0[0..], *prob_s), 0);
        expect_eq(predictClassMultinomial(w_s, b_s, 3, 2, q1[0..], *prob_s), 1);
        expect_eq(predictClassMultinomial(w_s, b_s, 3, 2, q2[0..], *prob_s), 2);
    } else {
        expect_eq(true, false);
    }
});

test("6. binaryMetrics exact TP/FP/TN/FN, accuracy, precision, recall & F1", () => {
    // threshold = 0.5:
    // idx 0: p=0.9, y=1 -> TP
    // idx 1: p=0.8, y=1 -> TP
    // idx 2: p=0.6, y=0 -> FP
    // idx 3: p=0.4, y=1 -> FN
    // idx 4: p=0.2, y=0 -> TN
    // idx 5: p=0.1, y=0 -> TN
    // idx 6: p=0.5, y=1 -> TP (boundary >= threshold)
    // idx 7: p=0.3, y=0 -> TN
    // Totals: TP=3, FP=1, TN=3, FN=1 => acc=6/8=0.75, prec=3/4=0.75, rec=3/4=0.75, f1=0.75
    const probs: [8]f64 = [0.9, 0.8, 0.6, 0.4, 0.2, 0.1, 0.5, 0.3];
    const y: [8]f64 = [1.0, 1.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0];

    if (m := binaryMetrics(probs[0..], y[0..], 0.5)) {
        expect_eq(m.tp, 3);
        expect_eq(m.fp, 1);
        expect_eq(m.tn, 3);
        expect_eq(m.fn_count, 1);
        expect_eq(absF64(m.accuracy - 0.75) < 1.0e-12, true);
        expect_eq(absF64(m.precision - 0.75) < 1.0e-12, true);
        expect_eq(absF64(m.recall - 0.75) < 1.0e-12, true);
        expect_eq(absF64(m.f1 - 0.75) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }

    // Zero positive predictions edge case (no division by zero)
    const low_p: [3]f64 = [0.1, 0.2, 0.3];
    const all_zero_y: [3]f64 = [0.0, 0.0, 0.0];
    if (m0 := binaryMetrics(low_p[0..], all_zero_y[0..], 0.5)) {
        expect_eq(m0.tp, 0);
        expect_eq(m0.fp, 0);
        expect_eq(m0.tn, 3);
        expect_eq(m0.fn_count, 0);
        expect_eq(absF64(m0.accuracy - 1.0) < 1.0e-12, true);
        expect_eq(m0.precision, 0.0);
        expect_eq(m0.recall, 0.0);
        expect_eq(m0.f1, 0.0);
    } else {
        expect_eq(true, false);
    }
});

test("7. Boundary defense: invalid labels, short buffers, NaN/Inf & bad configs", () => {
    const nan = std.math.nan(f64);
    const inf = std.math.inf(f64);
    const cfg = LogRegConfig.default();

    // softmax boundary: empty, short buffer, NaN, Inf
    let out1_buf: [1]f64 = [0.0];
    let out1: []f64 = out1_buf[0..];
    const logits2: [2]f64 = [1.0, 2.0];
    const logits_nan: [2]f64 = [1.0, nan];
    const logits_inf: [2]f64 = [1.0, inf];
    let out2_buf: [2]f64 = [0.0; 2];
    let out2: []f64 = out2_buf[0..];

    expect_eq(softmax(logits2[0..0], *out2), false);
    expect_eq(softmax(logits2[0..], *out1), false);
    expect_eq(softmax(logits_nan[0..], *out2), false);
    expect_eq(softmax(logits_inf[0..], *out2), false);

    // logLossBinary boundary: invalid labels, out-of-range probs, NaN, mismatch
    const bad_y: [2]f64 = [0.0, 2.0];
    const good_p: [2]f64 = [0.2, 0.8];
    const bad_p: [2]f64 = [-0.1, 0.8];
    const nan_p: [2]f64 = [nan, 0.5];
    const good_y: [2]f64 = [0.0, 1.0];
    expect_eq(logLossBinary(good_p[0..], bad_y[0..]) == null, true);
    expect_eq(logLossBinary(bad_p[0..], good_y[0..]) == null, true);
    expect_eq(logLossBinary(nan_p[0..], good_y[0..]) == null, true);
    expect_eq(logLossBinary(good_p[0..1], good_y[0..]) == null, true);

    // fitBinary boundary: invalid label, NaN feature, short w/grad buffer, invalid config
    const x2: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const x_nan: [4]f64 = [1.0, nan, 3.0, 4.0];
    let w2_buf: [2]f64 = [0.0; 2];
    let g2_buf: [2]f64 = [0.0; 2];
    let w1_buf: [1]f64 = [0.0];
    let w2: []f64 = w2_buf[0..];
    let g2: []f64 = g2_buf[0..];
    let w1: []f64 = w1_buf[0..];

    expect_eq(fitBinary(x2[0..], bad_y[0..], 2, 2, cfg, *w2, *g2) == null, true);
    expect_eq(fitBinary(x_nan[0..], good_y[0..], 2, 2, cfg, *w2, *g2) == null, true);
    expect_eq(fitBinary(x2[0..], good_y[0..], 2, 2, cfg, *w1, *g2) == null, true);
    expect_eq(fitBinary(x2[0..], good_y[0..], 2, 2, cfg, *w2, *w1) == null, true);

    const bad_cfg_lr: LogRegConfig = { learning_rate: -0.1, l2_reg: 0.0, max_iter: 100, tol: 1.0e-6 };
    const bad_cfg_reg: LogRegConfig = { learning_rate: 0.1, l2_reg: -1.0, max_iter: 100, tol: 1.0e-6 };
    const bad_cfg_iter: LogRegConfig = { learning_rate: 0.1, l2_reg: 0.0, max_iter: 0, tol: 1.0e-6 };
    expect_eq(fitBinary(x2[0..], good_y[0..], 2, 2, bad_cfg_lr, *w2, *g2) == null, true);
    expect_eq(fitBinary(x2[0..], good_y[0..], 2, 2, bad_cfg_reg, *w2, *g2) == null, true);
    expect_eq(fitBinary(x2[0..], good_y[0..], 2, 2, bad_cfg_iter, *w2, *g2) == null, true);

    // fitMultinomial boundary: out-of-range class label, n_classes < 2, short buffers
    const y_bad_cls: [2]usize = [0, 3];
    const y_ok_cls: [2]usize = [0, 1];
    let mw_buf: [6]f64 = [0.0; 6];
    let mb_buf: [3]f64 = [0.0; 3];
    let mgw_buf: [6]f64 = [0.0; 6];
    let mgb_buf: [3]f64 = [0.0; 3];
    let mp_buf: [3]f64 = [0.0; 3];
    let mw: []f64 = mw_buf[0..];
    let mb: []f64 = mb_buf[0..];
    let mgw: []f64 = mgw_buf[0..];
    let mgb: []f64 = mgb_buf[0..];
    let mp: []f64 = mp_buf[0..];

    expect_eq(fitMultinomial(x2[0..], y_bad_cls[0..], 2, 2, 3, cfg, *mw, *mb, *mgw, *mgb, *mp) == null, true);
    expect_eq(fitMultinomial(x2[0..], y_ok_cls[0..], 2, 2, 1, cfg, *mw, *mb, *mgw, *mgb, *mp) == null, true);
    expect_eq(fitMultinomial(x2[0..], y_ok_cls[0..], 2, 2, 3, cfg, *w2, *mb, *mgw, *mgb, *mp) == null, true);
    expect_eq(fitMultinomial(x2[0..], y_ok_cls[0..], 2, 2, 3, cfg, *mw, *w1, *mgw, *mgb, *mp) == null, true);

    // predict & metrics invalid threshold / NaN
    expect_eq(predictBinary(w2, 0.0, x2[0..2], 1.5) == null, true);
    expect_eq(predictBinary(w2, nan, x2[0..2], 0.5) == null, true);
    expect_eq(predictProbaMultinomial(mw, mb, 3, 2, x_nan[0..2], *mp), false);
    expect_eq(binaryMetrics(good_p[0..], good_y[0..], -0.1) == null, true);
    expect_eq(binaryMetrics(good_p[0..], bad_y[0..], 0.5) == null, true);
});
