// Source: cp/cap/test/test_naivebayes_production_deep.cp (Production deep tests for cap:naivebayes)

import { test, expect_eq } from "cap:test";
import {
    GaussianNbModel,
    fitGaussianNb,
    gaussianNbPredictProba,
    gaussianNbPredict,
    MultinomialNbModel,
    fitMultinomialNb,
    multinomialNbPredictProba,
    multinomialNbPredict,
    BernoulliNbModel,
    fitBernoulliNb,
    bernoulliNbPredictProba,
    bernoulliNbPredict,
} from "cap:ml";

fn absF64(v: f64): f64 {
    return if (v < 0.0) (0.0 - v) else v;
}

test("1. GaussianNb: 3-class 4-feature continuous data 100% accuracy & normalized posteriors", () => {
    // 18 samples (6 per class), 4 continuous features (Iris-like clusters)
    const x: [72]f64 = [
        // Class 0: near (1.0, 3.5, 1.4, 0.2)
        1.0, 3.5, 1.4, 0.2,   0.9, 3.2, 1.3, 0.2,
        1.1, 3.6, 1.5, 0.3,   0.8, 3.4, 1.4, 0.1,
        1.2, 3.7, 1.5, 0.2,   1.0, 3.3, 1.2, 0.3,
        // Class 1: near (5.8, 2.8, 4.3, 1.3)
        5.8, 2.8, 4.3, 1.3,   5.6, 2.7, 4.1, 1.2,
        6.0, 2.9, 4.5, 1.4,   5.7, 2.6, 4.2, 1.3,
        5.9, 3.0, 4.4, 1.5,   5.5, 2.8, 4.0, 1.2,
        // Class 2: near (6.8, 3.1, 5.7, 2.2)
        6.8, 3.1, 5.7, 2.2,   6.6, 3.0, 5.5, 2.1,
        7.1, 3.2, 5.9, 2.3,   6.9, 2.9, 5.8, 2.1,
        6.7, 3.3, 5.6, 2.4,   7.0, 3.1, 6.0, 2.2,
    ];
    const y: [18]usize = [
        0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2,
    ];

    let lp_b: [3]f64 = [0.0; 3];
    let m_b: [12]f64 = [0.0; 12];
    let v_b: [12]f64 = [0.0; 12];
    let c_b: [3]usize = [0; 3];
    let lp: []f64 = lp_b[0..];
    let m: []f64 = m_b[0..];
    let v: []f64 = v_b[0..];
    let c: []usize = c_b[0..];

    const model: GaussianNbModel = fitGaussianNb(
        x[0..], y[0..], 18, 4, 3, 1.0e-9, *lp, *m, *v, *c,
    ) ?? unreachable;

    let prob_b: [3]f64 = [0.0; 3];
    let prob: []f64 = prob_b[0..];

    let i: usize = 0;
    while (i < 18) {
        const row: []const f64 = x[(i * 4)..(i * 4 + 4)];
        const pred: usize = gaussianNbPredict(model, row, *prob) ?? unreachable;
        expect_eq(pred, y[i]);

        const psum: f64 = prob[0] + prob[1] + prob[2];
        expect_eq(absF64(psum - 1.0) < 1.0e-12, true);
        expect_eq(prob[pred] > 0.99, true);
        i += 1;
    }

    // Unseen queries
    const q0: [4]f64 = [1.05, 3.45, 1.38, 0.22];
    const q1: [4]f64 = [5.75, 2.82, 4.28, 1.32];
    const q2: [4]f64 = [6.85, 3.12, 5.75, 2.25];
    expect_eq(gaussianNbPredict(model, q0[0..], *prob) ?? unreachable, 0);
    expect_eq(gaussianNbPredict(model, q1[0..], *prob) ?? unreachable, 1);
    expect_eq(gaussianNbPredict(model, q2[0..], *prob) ?? unreachable, 2);
});

test("2. MultinomialNb: spam bag-of-words 100% accuracy & unseen word Laplace smoothing", () => {
    // Vocabulary of 6 words:
    //   0: "free", 1: "win", 2: "cash", 3: "meeting", 4: "report", 5: "unseen_rare"
    // Class 0 = Ham, Class 1 = Spam. Notice word 5 has 0 count across all training docs.
    const counts: [48]f64 = [
        // Ham (4 docs)
        0.0, 0.0, 0.0, 3.0, 2.0, 0.0,
        0.0, 0.0, 0.0, 2.0, 4.0, 0.0,
        1.0, 0.0, 0.0, 4.0, 3.0, 0.0,
        0.0, 0.0, 0.0, 1.0, 5.0, 0.0,
        // Spam (4 docs)
        4.0, 3.0, 2.0, 0.0, 0.0, 0.0,
        3.0, 2.0, 4.0, 0.0, 1.0, 0.0,
        5.0, 4.0, 3.0, 0.0, 0.0, 0.0,
        2.0, 3.0, 3.0, 1.0, 0.0, 0.0,
    ];
    const y: [8]usize = [0, 0, 0, 0, 1, 1, 1, 1];

    let lp_b: [2]f64 = [0.0; 2];
    let flp_b: [12]f64 = [0.0; 12];
    let cc_b: [2]usize = [0; 2];
    let lp: []f64 = lp_b[0..];
    let flp: []f64 = flp_b[0..];
    let cc: []usize = cc_b[0..];

    const model: MultinomialNbModel = fitMultinomialNb(
        counts[0..], y[0..], 8, 6, 2, 1.0, *lp, *flp, *cc,
    ) ?? unreachable;

    let prob_b: [2]f64 = [0.0; 2];
    let prob: []f64 = prob_b[0..];

    // 100% training accuracy
    let i: usize = 0;
    while (i < 8) {
        const doc: []const f64 = counts[(i * 6)..(i * 6 + 6)];
        expect_eq(multinomialNbPredict(model, doc, *prob) ?? unreachable, y[i]);
        expect_eq(absF64((prob[0] + prob[1]) - 1.0) < 1.0e-12, true);
        i += 1;
    }

    // Unseen word ("unseen_rare" at index 5) must have finite non-zero smoothed probability
    const unseen_doc: [6]f64 = [0.0, 0.0, 0.0, 0.0, 0.0, 3.0];
    expect_eq(multinomialNbPredictProba(model, unseen_doc[0..], *prob), true);
    expect_eq(prob[0] > 0.0 && prob[1] > 0.0, true);
    expect_eq(absF64((prob[0] + prob[1]) - 1.0) < 1.0e-12, true);

    // Spam doc containing unseen word still predicted as Spam
    const mixed_spam: [6]f64 = [2.0, 1.0, 1.0, 0.0, 0.0, 2.0];
    expect_eq(multinomialNbPredict(model, mixed_spam[0..], *prob) ?? unreachable, 1);
    expect_eq(prob[1] > 0.9, true);
});

test("3. BernoulliNb: explicit absence penalty & threshold binarization", () => {
    // 2 classes, 3 features:
    //   Class 0: features 0 and 1 are almost always present (1, 1, 0)
    //   Class 1: features 0 and 1 are absent, feature 2 is optional (0, 0, 0/1)
    // Values are continuous and binarized with threshold = 0.5.
    const x: [24]f64 = [
        // Class 0 (4 samples)
        0.9, 0.8, 0.1,
        1.2, 0.7, 0.2,
        0.8, 1.1, 0.0,
        1.0, 0.9, 0.3,
        // Class 1 (4 samples)
        0.1, 0.2, 0.9,
        0.0, 0.1, 0.8,
        0.2, 0.0, 0.1,
        0.3, 0.2, 0.9,
    ];
    const y: [8]usize = [0, 0, 0, 0, 1, 1, 1, 1];

    let lp_b: [2]f64 = [0.0; 2];
    let p1_b: [6]f64 = [0.0; 6];
    let p0_b: [6]f64 = [0.0; 6];
    let cc_b: [2]usize = [0; 2];
    let lp: []f64 = lp_b[0..];
    let p1: []f64 = p1_b[0..];
    let p0: []f64 = p0_b[0..];
    let cc: []usize = cc_b[0..];

    const model: BernoulliNbModel = fitBernoulliNb(
        x[0..], y[0..], 8, 3, 2, 1.0, 0.5, *lp, *p1, *p0, *cc,
    ) ?? unreachable;

    let prob_b: [2]f64 = [0.0; 2];
    let prob: []f64 = prob_b[0..];

    // All-zero document (0, 0, 0): Multinomial would give equal 0.5/0.5,
    // but Bernoulli explicitly penalizes the absence of features 0 and 1 in Class 0 -> predicts Class 1!
    const all_absent: [3]f64 = [0.0, 0.0, 0.0];
    const pred_absent: usize = bernoulliNbPredict(model, all_absent[0..], *prob) ?? unreachable;
    expect_eq(pred_absent, 1);
    expect_eq(prob[1] > 0.8, true);
    expect_eq(absF64((prob[0] + prob[1]) - 1.0) < 1.0e-12, true);

    // Document with features 0 and 1 active above threshold 0.5 -> predicts Class 0
    const active_01: [3]f64 = [0.6, 0.7, 0.0];
    expect_eq(bernoulliNbPredict(model, active_01[0..], *prob) ?? unreachable, 0);
    expect_eq(prob[0] > 0.8, true);
});

test("4. High-dimensional 100-feature log-sum-exp stability without underflow", () => {
    const n: usize = 20;
    const d: usize = 100;
    let x_g: [2000]f64 = [0.0; 2000];
    let x_m: [2000]f64 = [0.0; 2000];
    let y: [20]usize = [0; 20];

    let i: usize = 0;
    while (i < n) {
        const cls: usize = if (i < 10) 0 else 1;
        y[i] = cls;
        let j: usize = 0;
        while (j < d) {
            const jitter: f64 = ((i % 5) as f64) * 0.05;
            if (cls == 0) {
                x_g[i * d + j] = -1.0 + jitter;
                x_m[i * d + j] = if ((j % 2) == 0) 3.0 else 0.5;
            } else {
                x_g[i * d + j] = 1.0 + jitter;
                x_m[i * d + j] = if ((j % 2) == 1) 3.0 else 0.5;
            }
            j += 1;
        }
        i += 1;
    }

    // (a) 100-D GaussianNb
    let lp_b: [2]f64 = [0.0; 2];
    let m_b: [200]f64 = [0.0; 200];
    let v_b: [200]f64 = [0.0; 200];
    let c_b: [2]usize = [0; 2];
    let lp: []f64 = lp_b[0..];
    let m: []f64 = m_b[0..];
    let v: []f64 = v_b[0..];
    let c: []usize = c_b[0..];

    const g_model: GaussianNbModel = fitGaussianNb(
        x_g[0..], y[0..], n, d, 2, 1.0e-6, *lp, *m, *v, *c,
    ) ?? unreachable;

    let prob_b: [2]f64 = [0.0; 2];
    let prob: []f64 = prob_b[0..];

    // Query far from both means (log-likelihood ~ -10000) still normalizes cleanly without underflow
    let far_q: [100]f64 = [15.0; 100];
    expect_eq(gaussianNbPredictProba(g_model, far_q[0..], *prob), true);
    expect_eq(absF64((prob[0] + prob[1]) - 1.0) < 1.0e-12, true);
    expect_eq(prob[1] > 0.99, true);

    // (b) 100-D MultinomialNb
    let mlp_b: [2]f64 = [0.0; 2];
    let mflp_b: [200]f64 = [0.0; 200];
    let mcc_b: [2]usize = [0; 2];
    let mlp: []f64 = mlp_b[0..];
    let mflp: []f64 = mflp_b[0..];
    let mcc: []usize = mcc_b[0..];

    const m_model: MultinomialNbModel = fitMultinomialNb(
        x_m[0..], y[0..], n, d, 2, 1.0, *mlp, *mflp, *mcc,
    ) ?? unreachable;

    expect_eq(multinomialNbPredict(m_model, x_m[0..d], *prob) ?? unreachable, 0);
    expect_eq(absF64((prob[0] + prob[1]) - 1.0) < 1.0e-12, true);
    expect_eq(multinomialNbPredict(m_model, x_m[(10 * d)..(11 * d)], *prob) ?? unreachable, 1);
    expect_eq(absF64((prob[0] + prob[1]) - 1.0) < 1.0e-12, true);
});

test("5. GaussianNb constant feature var_smoothing prevents division by zero", () => {
    // All features are strictly constant within each class (zero empirical variance)
    const x_const: [8]f64 = [
        5.0, 1.0,
        5.0, 1.0,
        5.0, 9.0,
        5.0, 9.0,
    ];
    const y: [4]usize = [0, 0, 1, 1];

    let lp_b: [2]f64 = [0.0; 2];
    let m_b: [4]f64 = [0.0; 4];
    let v_b: [4]f64 = [0.0; 4];
    let c_b: [2]usize = [0; 2];
    let lp: []f64 = lp_b[0..];
    let m: []f64 = m_b[0..];
    let v: []f64 = v_b[0..];
    let c: []usize = c_b[0..];

    // With var_smoothing = 0.0 and zero empirical variance -> rejected safely
    expect_eq(fitGaussianNb(x_const[0..], y[0..], 4, 2, 2, 0.0, *lp, *m, *v, *c) == null, true);

    // With positive var_smoothing -> succeeds cleanly and predicts accurately
    const model: GaussianNbModel = fitGaussianNb(
        x_const[0..], y[0..], 4, 2, 2, 1.0e-6, *lp, *m, *v, *c,
    ) ?? unreachable;

    let prob_b: [2]f64 = [0.0; 2];
    let prob: []f64 = prob_b[0..];
    const q0: [2]f64 = [5.0, 1.2];
    const q1: [2]f64 = [5.0, 8.7];
    expect_eq(gaussianNbPredict(model, q0[0..], *prob) ?? unreachable, 0);
    expect_eq(gaussianNbPredict(model, q1[0..], *prob) ?? unreachable, 1);
});

test("6. Boundary defense: negative counts, OOB labels, short buffers, negative alpha & NaN/Inf", () => {
    const x_ok: [4]f64 = [1.0, 2.0, 3.0, 4.0];
    const y_ok: [2]usize = [0, 1];

    let lp_b: [2]f64 = [0.0; 2];
    let m_b: [4]f64 = [0.0; 4];
    let v_b: [4]f64 = [0.0; 4];
    let c_b: [2]usize = [0; 2];
    let lp: []f64 = lp_b[0..];
    let m: []f64 = m_b[0..];
    let v: []f64 = v_b[0..];
    let c: []usize = c_b[0..];

    // Negative counts in MultinomialNb
    const neg_counts: [4]f64 = [1.0, -0.5, 2.0, 3.0];
    expect_eq(fitMultinomialNb(neg_counts[0..], y_ok[0..], 2, 2, 2, 1.0, *lp, *m, *c) == null, true);

    // Out-of-bounds class label
    const y_oob: [2]usize = [0, 2];
    expect_eq(fitGaussianNb(x_ok[0..], y_oob[0..], 2, 2, 2, 1.0e-9, *lp, *m, *v, *c) == null, true);
    expect_eq(fitMultinomialNb(x_ok[0..], y_oob[0..], 2, 2, 2, 1.0, *lp, *m, *c) == null, true);
    expect_eq(fitBernoulliNb(x_ok[0..], y_oob[0..], 2, 2, 2, 1.0, 0.5, *lp, *m, *v, *c) == null, true);

    // Negative smoothing / var_smoothing
    expect_eq(fitGaussianNb(x_ok[0..], y_ok[0..], 2, 2, 2, -1.0e-6, *lp, *m, *v, *c) == null, true);
    expect_eq(fitMultinomialNb(x_ok[0..], y_ok[0..], 2, 2, 2, -0.1, *lp, *m, *c) == null, true);
    expect_eq(fitBernoulliNb(x_ok[0..], y_ok[0..], 2, 2, 2, -0.1, 0.5, *lp, *m, *v, *c) == null, true);

    // Short output buffers
    let short_lp: []f64 = lp_b[0..1];
    let short_m: []f64 = m_b[0..3];
    expect_eq(fitGaussianNb(x_ok[0..], y_ok[0..], 2, 2, 2, 1.0e-9, *short_lp, *m, *v, *c) == null, true);
    expect_eq(fitMultinomialNb(x_ok[0..], y_ok[0..], 2, 2, 2, 1.0, *lp, *short_m, *c) == null, true);
    expect_eq(fitBernoulliNb(x_ok[0..], y_ok[0..], 2, 2, 2, 1.0, 0.5, *lp, *short_m, *v, *c) == null, true);

    // NaN / Inf in features and prediction queries
    const zero: f64 = 0.0;
    const nan_v: f64 = zero / zero;
    const inf_v: f64 = 1.0 / zero;
    const x_nan: [4]f64 = [1.0, nan_v, 3.0, 4.0];
    const x_inf: [4]f64 = [1.0, 2.0, inf_v, 4.0];
    expect_eq(fitGaussianNb(x_nan[0..], y_ok[0..], 2, 2, 2, 1.0e-9, *lp, *m, *v, *c) == null, true);
    expect_eq(fitMultinomialNb(x_inf[0..], y_ok[0..], 2, 2, 2, 1.0, *lp, *m, *c) == null, true);
    expect_eq(fitBernoulliNb(x_nan[0..], y_ok[0..], 2, 2, 2, 1.0, 0.5, *lp, *m, *v, *c) == null, true);

    // Query-time negative count / NaN / short proba_out defense
    const m_model: MultinomialNbModel = fitMultinomialNb(
        x_ok[0..], y_ok[0..], 2, 2, 2, 1.0, *lp, *m, *c,
    ) ?? unreachable;
    let prob_b: [2]f64 = [0.0; 2];
    let prob: []f64 = prob_b[0..];
    let prob_short: []f64 = prob_b[0..1];
    const q_neg: [2]f64 = [-1.0, 2.0];
    const q_nan: [2]f64 = [nan_v, 1.0];
    expect_eq(multinomialNbPredictProba(m_model, q_neg[0..], *prob), false);
    expect_eq(multinomialNbPredictProba(m_model, q_nan[0..], *prob), false);
    expect_eq(multinomialNbPredictProba(m_model, x_ok[0..2], *prob_short), false);
    expect_eq(multinomialNbPredict(m_model, q_neg[0..], *prob) == null, true);
});
