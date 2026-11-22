// Source: cp/cap/test/test_dtree_production_deep.cp (Production deep tests for cap:dtree)

import { test, expect_eq } from "cap:test";
import {
    DTREE_NO_NODE,
    SplitCriterion,
    TreeNode,
    TreeConfig,
    TreeWorkspace,
    DecisionTree,
} from "cap:ml";

fn absF64(v: f64): f64 {
    return if (v < 0.0) (0.0 - v) else v;
}

test("1. 3-class multi-feature classification: Gini & Entropy both achieve 100% accuracy", () => {
    // 24 samples, 3 features, 3 classes:
    //   Class 0: f0 < 0.0, f1 in [-1, 1]
    //   Class 1: f0 > 1.0, f1 > 1.0
    //   Class 2: f0 > 1.0, f1 < -1.0
    //   f2 is a mild auxiliary coordinate.
    const x: [72]f64 = [
        // Class 0 (8 samples)
        -2.5,  0.1,  0.3,   -2.1, -0.4, -0.2,
        -1.8,  0.6,  0.8,   -1.5, -0.8, -0.5,
        -1.2,  0.2,  0.1,   -0.9, -0.3,  0.4,
        -0.6,  0.7, -0.7,   -0.4,  0.0,  0.2,
        // Class 1 (8 samples)
         1.4,  1.5,  0.2,    1.7,  2.1, -0.3,
         2.0,  1.8,  0.5,    2.3,  2.5, -0.1,
         2.6,  1.4,  0.9,    2.9,  2.8,  0.0,
         3.1,  1.9, -0.4,    3.5,  2.3,  0.6,
        // Class 2 (8 samples)
         1.3, -1.6, -0.2,    1.6, -2.2,  0.4,
         1.9, -1.9, -0.6,    2.2, -2.6,  0.1,
         2.5, -1.5, -0.8,    2.8, -2.9,  0.3,
         3.0, -2.0,  0.5,    3.4, -2.4, -0.5,
    ];
    const y: [24]i32 = [
        0, 0, 0, 0, 0, 0, 0, 0,
        1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2,
    ];

    let idx_b: [24]usize = [0; 24];
    let tmp_b: [24]usize = [0; 24];
    let cc_b: [3]usize = [0; 3];
    let lc_b: [3]usize = [0; 3];
    let sn_b: [64]u32 = [0; 64];
    let slo_b: [64]usize = [0; 64];
    let shi_b: [64]usize = [0; 64];
    let sd_b: [64]usize = [0; 64];

    let ws = TreeWorkspace {
        sample_indices: idx_b[0..],
        sort_tmp: tmp_b[0..],
        class_counts: cc_b[0..],
        left_counts: lc_b[0..],
        stack_node: sn_b[0..],
        stack_lo: slo_b[0..],
        stack_hi: shi_b[0..],
        stack_depth: sd_b[0..],
    };

    // (a) Gini criterion
    let pool_g_b: [64]TreeNode = [TreeNode.empty(); 64];
    let pool_g: []TreeNode = pool_g_b[0..];
    const cfg_g: TreeConfig = TreeConfig.defaultClassifier();
    const tree_g: DecisionTree = DecisionTree.fitClassifier(
        x[0..], y[0..], 24, 3, 3, cfg_g, *pool_g, *ws,
    ) ?? unreachable;

    expect_eq(tree_g.nodeCount() >= 5, true);
    expect_eq(tree_g.nodeCount(), (2 * tree_g.leafCount()) - 1);
    expect_eq(tree_g.depth() >= 2, true);

    // (b) Entropy criterion
    let pool_e_b: [64]TreeNode = [TreeNode.empty(); 64];
    let pool_e: []TreeNode = pool_e_b[0..];
    let cfg_e: TreeConfig = TreeConfig.defaultClassifier();
    cfg_e.criterion = SplitCriterion.Entropy;
    const tree_e: DecisionTree = DecisionTree.fitClassifier(
        x[0..], y[0..], 24, 3, 3, cfg_e, *pool_e, *ws,
    ) ?? unreachable;

    expect_eq(tree_e.nodeCount() >= 5, true);
    expect_eq(tree_e.nodeCount(), (2 * tree_e.leafCount()) - 1);

    // Verify 100% training accuracy for both Gini and Entropy
    let i: usize = 0;
    while (i < 24) {
        const row: []const f64 = x[(i * 3)..(i * 3 + 3)];
        const pg: i32 = tree_g.predictClass(row) ?? unreachable;
        const pe: i32 = tree_e.predictClass(row) ?? unreachable;
        expect_eq(pg, y[i]);
        expect_eq(pe, y[i]);
        i += 1;
    }

    // Verify unseen test queries
    const q0: [3]f64 = [-1.7, 0.3, 0.0];
    const q1: [3]f64 = [2.4, 2.0, 0.1];
    const q2: [3]f64 = [2.1, -2.1, -0.2];
    expect_eq(tree_g.predictClass(q0[0..]) ?? unreachable, 0);
    expect_eq(tree_g.predictClass(q1[0..]) ?? unreachable, 1);
    expect_eq(tree_g.predictClass(q2[0..]) ?? unreachable, 2);
    expect_eq(tree_e.predictClass(q0[0..]) ?? unreachable, 0);
    expect_eq(tree_e.predictClass(q1[0..]) ?? unreachable, 1);
    expect_eq(tree_e.predictClass(q2[0..]) ?? unreachable, 2);
});

test("2. CART regression: nonlinear piecewise & quadratic y = x^2 high-accuracy fitting", () => {
    // (a) Piecewise constant function with 3 plateaus:
    //   x < -1 => -5.0, -1 <= x < 1 => 2.0, x >= 1 => 9.0
    const n_pw: usize = 30;
    let x_pw: [30]f64 = [0.0; 30];
    let y_pw: [30]f64 = [0.0; 30];
    let i: usize = 0;
    while (i < n_pw) {
        const xv: f64 = -3.0 + (6.0 * (i as f64) / 29.0);
        x_pw[i] = xv;
        if (xv < -1.0) {
            y_pw[i] = -5.0;
        } else if (xv < 1.0) {
            y_pw[i] = 2.0;
        } else {
            y_pw[i] = 9.0;
        }
        i += 1;
    }

    let pool_b: [128]TreeNode = [TreeNode.empty(); 128];
    let pool: []TreeNode = pool_b[0..];
    let idx_b: [41]usize = [0; 41];
    let tmp_b: [41]usize = [0; 41];
    let empty_cc: [1]usize = [0; 1];
    let empty_lc: [1]usize = [0; 1];
    let sn_b: [128]u32 = [0; 128];
    let slo_b: [128]usize = [0; 128];
    let shi_b: [128]usize = [0; 128];
    let sd_b: [128]usize = [0; 128];

    let ws = TreeWorkspace {
        sample_indices: idx_b[0..],
        sort_tmp: tmp_b[0..],
        class_counts: empty_cc[0..],
        left_counts: empty_lc[0..],
        stack_node: sn_b[0..],
        stack_lo: slo_b[0..],
        stack_hi: shi_b[0..],
        stack_depth: sd_b[0..],
    };

    const cfg: TreeConfig = TreeConfig.defaultRegressor();
    const tree_pw: DecisionTree = DecisionTree.fitRegressor(
        x_pw[0..], y_pw[0..], n_pw, 1, cfg, *pool, *ws,
    ) ?? unreachable;

    expect_eq(tree_pw.leafCount(), 3);
    expect_eq(tree_pw.nodeCount(), 5);

    const q_lo: [1]f64 = [-2.2];
    const q_mid: [1]f64 = [0.15];
    const q_hi: [1]f64 = [2.4];
    expect_eq(absF64((tree_pw.predictValue(q_lo[0..]) ?? unreachable) - (-5.0)) < 1.0e-9, true);
    expect_eq(absF64((tree_pw.predictValue(q_mid[0..]) ?? unreachable) - 2.0) < 1.0e-9, true);
    expect_eq(absF64((tree_pw.predictValue(q_hi[0..]) ?? unreachable) - 9.0) < 1.0e-9, true);

    // (b) Smooth quadratic y = x^2 over [-2.0, 2.0] with 41 points
    const n_q: usize = 41;
    let x_q: [41]f64 = [0.0; 41];
    let y_q: [41]f64 = [0.0; 41];
    let j: usize = 0;
    while (j < n_q) {
        const xv: f64 = -2.0 + 0.1 * (j as f64);
        x_q[j] = xv;
        y_q[j] = xv * xv;
        j += 1;
    }

    let pool_q_b: [128]TreeNode = [TreeNode.empty(); 128];
    let pool_q: []TreeNode = pool_q_b[0..];
    let cfg_q: TreeConfig = TreeConfig.defaultRegressor();
    cfg_q.max_depth = 8;
    const tree_q: DecisionTree = DecisionTree.fitRegressor(
        x_q[0..], y_q[0..], n_q, 1, cfg_q, *pool_q, *ws,
    ) ?? unreachable;

    // Training MSE should be virtually zero, and interpolated midpoints accurate
    let train_sq_err: f64 = 0.0;
    let k: usize = 0;
    while (k < n_q) {
        const pv: f64 = tree_q.predictValue(x_q[k..(k + 1)]) ?? unreachable;
        const err: f64 = pv - y_q[k];
        train_sq_err += err * err;
        k += 1;
    }
    expect_eq((train_sq_err / (n_q as f64)) < 1.0e-6, true);

    const t1: [1]f64 = [-1.55];
    const t2: [1]f64 = [0.05];
    const t3: [1]f64 = [1.45];
    expect_eq(absF64((tree_q.predictValue(t1[0..]) ?? unreachable) - (1.55 * 1.55)) < 0.25, true);
    expect_eq(absF64((tree_q.predictValue(t2[0..]) ?? unreachable) - (0.05 * 0.05)) < 0.05, true);
    expect_eq(absF64((tree_q.predictValue(t3[0..]) ?? unreachable) - (1.45 * 1.45)) < 0.25, true);
});

test("3. Feature importances: informative features outweigh pure noise and sum to 1.0", () => {
    // 32 samples, 4 features:
    //   f0: primary signal (splits class 0 vs {1, 2})
    //   f1: secondary signal (splits class 1 vs 2)
    //   f2, f3: deterministic pseudo-noise uncorrelated with y
    const n: usize = 32;
    let x: [128]f64 = [0.0; 128];
    let y: [32]i32 = [0; 32];

    let state: u64 = 0x123456789ABCDEF0;
    let i: usize = 0;
    while (i < n) {
        const cls: i32 = (i % 3) as i32;
        y[i] = cls;

        let f0: f64 = 0.0;
        let f1: f64 = 0.0;
        if (cls == 0) {
            f0 = -3.0 + 0.05 * (i as f64);
            f1 = 0.0 + 0.03 * (i as f64);
        } else if (cls == 1) {
            f0 = 3.0 + 0.05 * (i as f64);
            f1 = -3.0 + 0.03 * (i as f64);
        } else {
            f0 = 3.0 + 0.05 * (i as f64);
            f1 = 3.0 + 0.03 * (i as f64);
        }

        state = (state *% 6364136223846793005) +% 1442695040888963407;
        const n2: f64 = (((state >> 32) % 1000) as f64) * 0.001;
        state = (state *% 6364136223846793005) +% 1442695040888963407;
        const n3: f64 = (((state >> 32) % 1000) as f64) * 0.001;

        x[i * 4 + 0] = f0;
        x[i * 4 + 1] = f1;
        x[i * 4 + 2] = n2;
        x[i * 4 + 3] = n3;
        i += 1;
    }

    let pool_b: [64]TreeNode = [TreeNode.empty(); 64];
    let pool: []TreeNode = pool_b[0..];
    let idx_b: [32]usize = [0; 32];
    let tmp_b: [32]usize = [0; 32];
    let cc_b: [3]usize = [0; 3];
    let lc_b: [3]usize = [0; 3];
    let sn_b: [64]u32 = [0; 64];
    let slo_b: [64]usize = [0; 64];
    let shi_b: [64]usize = [0; 64];
    let sd_b: [64]usize = [0; 64];

    let ws = TreeWorkspace {
        sample_indices: idx_b[0..],
        sort_tmp: tmp_b[0..],
        class_counts: cc_b[0..],
        left_counts: lc_b[0..],
        stack_node: sn_b[0..],
        stack_lo: slo_b[0..],
        stack_hi: shi_b[0..],
        stack_depth: sd_b[0..],
    };

    const cfg: TreeConfig = TreeConfig.defaultClassifier();
    const tree: DecisionTree = DecisionTree.fitClassifier(
        x[0..], y[0..], n, 4, 3, cfg, *pool, *ws,
    ) ?? unreachable;

    let imp_b: [4]f64 = [0.0; 4];
    let imp: []f64 = imp_b[0..];
    expect_eq(tree.featureImportances(*imp), true);

    const imp_sum: f64 = imp[0] + imp[1] + imp[2] + imp[3];
    expect_eq(absF64(imp_sum - 1.0) < 1.0e-9, true);
    expect_eq(imp[0] > 0.3, true);
    expect_eq(imp[1] > 0.3, true);
    expect_eq(imp[2] < 1.0e-9, true);
    expect_eq(imp[3] < 1.0e-9, true);

    // Short output slice rejected
    let short_imp_b: [2]f64 = [0.0; 2];
    let short_imp: []f64 = short_imp_b[0..];
    expect_eq(tree.featureImportances(*short_imp), false);
});

test("4. Pruning constraints: max_depth and min_samples_leaf strictly enforced", () => {
    const n: usize = 32;
    let x: [32]f64 = [0.0; 32];
    let y: [32]f64 = [0.0; 32];
    let i: usize = 0;
    while (i < n) {
        const xv: f64 = i as f64;
        x[i] = xv;
        y[i] = xv * xv;
        i += 1;
    }

    let idx_b: [32]usize = [0; 32];
    let tmp_b: [32]usize = [0; 32];
    let cc_b: [1]usize = [0; 1];
    let lc_b: [1]usize = [0; 1];
    let sn_b: [64]u32 = [0; 64];
    let slo_b: [64]usize = [0; 64];
    let shi_b: [64]usize = [0; 64];
    let sd_b: [64]usize = [0; 64];

    let ws = TreeWorkspace {
        sample_indices: idx_b[0..],
        sort_tmp: tmp_b[0..],
        class_counts: cc_b[0..],
        left_counts: lc_b[0..],
        stack_node: sn_b[0..],
        stack_lo: slo_b[0..],
        stack_hi: shi_b[0..],
        stack_depth: sd_b[0..],
    };

    // (a) max_depth = 0 => single root leaf
    let p0_b: [64]TreeNode = [TreeNode.empty(); 64];
    let p0: []TreeNode = p0_b[0..];
    let cfg0: TreeConfig = TreeConfig.defaultRegressor();
    cfg0.max_depth = 0;
    const t0: DecisionTree = DecisionTree.fitRegressor(x[0..], y[0..], n, 1, cfg0, *p0, *ws) ?? unreachable;
    expect_eq(t0.depth(), 0);
    expect_eq(t0.nodeCount(), 1);
    expect_eq(t0.leafCount(), 1);

    // Single-leaf tree feature importances are all 0.0
    let imp0_b: [1]f64 = [99.0];
    let imp0: []f64 = imp0_b[0..];
    expect_eq(t0.featureImportances(*imp0), true);
    expect_eq(imp0[0], 0.0);

    // (b) max_depth = 2 => depth <= 2, leafCount <= 4
    let p2_b: [64]TreeNode = [TreeNode.empty(); 64];
    let p2: []TreeNode = p2_b[0..];
    let cfg2: TreeConfig = TreeConfig.defaultRegressor();
    cfg2.max_depth = 2;
    const t2: DecisionTree = DecisionTree.fitRegressor(x[0..], y[0..], n, 1, cfg2, *p2, *ws) ?? unreachable;
    expect_eq(t2.depth(), 2);
    expect_eq(t2.leafCount() <= 4, true);

    // (c) min_samples_leaf = 8 => every leaf holds >= 8 samples, so at most 4 leaves
    let p_leaf_b: [64]TreeNode = [TreeNode.empty(); 64];
    let p_leaf: []TreeNode = p_leaf_b[0..];
    let cfg_leaf: TreeConfig = TreeConfig.defaultRegressor();
    cfg_leaf.max_depth = 10;
    cfg_leaf.min_samples_leaf = 8;
    const t_leaf: DecisionTree = DecisionTree.fitRegressor(x[0..], y[0..], n, 1, cfg_leaf, *p_leaf, *ws) ?? unreachable;
    expect_eq(t_leaf.leafCount() <= 4, true);

    let k: usize = 0;
    while (k < t_leaf.nodeCount()) {
        if (p_leaf[k].is_leaf) {
            expect_eq(p_leaf[k].sample_count >= 8, true);
        }
        k += 1;
    }
});

test("5. Fixed node pool exhaustion gracefully degrades active nodes to leaves", () => {
    const n: usize = 20;
    let x: [20]f64 = [0.0; 20];
    let y: [20]f64 = [0.0; 20];
    let i: usize = 0;
    while (i < n) {
        x[i] = i as f64;
        y[i] = (i as f64) * 3.0;
        i += 1;
    }

    let idx_b: [20]usize = [0; 20];
    let tmp_b: [20]usize = [0; 20];
    let cc_b: [1]usize = [0; 1];
    let lc_b: [1]usize = [0; 1];
    let sn_b: [32]u32 = [0; 32];
    let slo_b: [32]usize = [0; 32];
    let shi_b: [32]usize = [0; 32];
    let sd_b: [32]usize = [0; 32];

    let ws = TreeWorkspace {
        sample_indices: idx_b[0..],
        sort_tmp: tmp_b[0..],
        class_counts: cc_b[0..],
        left_counts: lc_b[0..],
        stack_node: sn_b[0..],
        stack_lo: slo_b[0..],
        stack_hi: shi_b[0..],
        stack_depth: sd_b[0..],
    };

    const cfg: TreeConfig = TreeConfig.defaultRegressor();

    // Pool capacity 1: only root leaf fits
    let tiny1_b: [1]TreeNode = [TreeNode.empty(); 1];
    let tiny1: []TreeNode = tiny1_b[0..];
    const t1: DecisionTree = DecisionTree.fitRegressor(x[0..], y[0..], n, 1, cfg, *tiny1, *ws) ?? unreachable;
    expect_eq(t1.nodeCount(), 1);
    expect_eq(t1.leafCount(), 1);
    expect_eq(tiny1[0].left, DTREE_NO_NODE);
    expect_eq(tiny1[0].right, DTREE_NO_NODE);
    const pred1: f64 = t1.predictValue(x[0..1]) ?? unreachable;
    expect_eq(absF64(pred1 - 28.5) < 1.0e-9, true);

    // Pool capacity 4: root + 2 children = 3 nodes (1 slot left cannot fit another pair of children)
    let tiny4_b: [4]TreeNode = [TreeNode.empty(); 4];
    let tiny4: []TreeNode = tiny4_b[0..];
    const t4: DecisionTree = DecisionTree.fitRegressor(x[0..], y[0..], n, 1, cfg, *tiny4, *ws) ?? unreachable;
    expect_eq(t4.nodeCount(), 3);
    expect_eq(t4.leafCount(), 2);
    expect_eq(t4.depth(), 1);

    // Every sample still predicts a valid finite value without crashing
    let j: usize = 0;
    while (j < n) {
        const pv: f64 = t4.predictValue(x[j..(j + 1)]) ?? unreachable;
        expect_eq(pv == pv, true);
        j += 1;
    }
});

test("6. Boundary defense: empty data, out-of-range labels, short buffers, criterion mismatch & NaN", () => {
    const x_ok: [4]f64 = [0.0, 1.0, 2.0, 3.0];
    const y_cls_ok: [2]i32 = [0, 1];
    const y_reg_ok: [2]f64 = [1.0, 2.0];

    let pool_b: [16]TreeNode = [TreeNode.empty(); 16];
    let pool: []TreeNode = pool_b[0..];
    let idx_b: [4]usize = [0; 4];
    let tmp_b: [4]usize = [0; 4];
    let cc_b: [2]usize = [0; 2];
    let lc_b: [2]usize = [0; 2];
    let sn_b: [16]u32 = [0; 16];
    let slo_b: [16]usize = [0; 16];
    let shi_b: [16]usize = [0; 16];
    let sd_b: [16]usize = [0; 16];

    let ws = TreeWorkspace {
        sample_indices: idx_b[0..],
        sort_tmp: tmp_b[0..],
        class_counts: cc_b[0..],
        left_counts: lc_b[0..],
        stack_node: sn_b[0..],
        stack_lo: slo_b[0..],
        stack_hi: shi_b[0..],
        stack_depth: sd_b[0..],
    };

    const cfg_c: TreeConfig = TreeConfig.defaultClassifier();
    const cfg_r: TreeConfig = TreeConfig.defaultRegressor();

    // Empty samples / features / classes
    expect_eq(DecisionTree.fitClassifier(x_ok[0..], y_cls_ok[0..], 0, 2, 2, cfg_c, *pool, *ws) == null, true);
    expect_eq(DecisionTree.fitClassifier(x_ok[0..], y_cls_ok[0..], 2, 0, 2, cfg_c, *pool, *ws) == null, true);
    expect_eq(DecisionTree.fitClassifier(x_ok[0..], y_cls_ok[0..], 2, 2, 0, cfg_c, *pool, *ws) == null, true);
    expect_eq(DecisionTree.fitRegressor(x_ok[0..], y_reg_ok[0..], 0, 2, cfg_r, *pool, *ws) == null, true);

    // Criterion mismatch
    expect_eq(DecisionTree.fitClassifier(x_ok[0..], y_cls_ok[0..], 2, 2, 2, cfg_r, *pool, *ws) == null, true);
    expect_eq(DecisionTree.fitRegressor(x_ok[0..], y_reg_ok[0..], 2, 2, cfg_c, *pool, *ws) == null, true);

    // Out-of-range labels (< 0 and >= num_classes)
    const y_neg: [2]i32 = [-1, 0];
    const y_oob: [2]i32 = [0, 2];
    expect_eq(DecisionTree.fitClassifier(x_ok[0..], y_neg[0..], 2, 2, 2, cfg_c, *pool, *ws) == null, true);
    expect_eq(DecisionTree.fitClassifier(x_ok[0..], y_oob[0..], 2, 2, 2, cfg_c, *pool, *ws) == null, true);

    // NaN / Inf in features and regression targets
    const zero: f64 = 0.0;
    const nan_val: f64 = zero / zero;
    const inf_val: f64 = 1.0 / zero;
    const x_nan: [4]f64 = [0.0, nan_val, 2.0, 3.0];
    const x_inf: [4]f64 = [0.0, 1.0, inf_val, 3.0];
    const y_nan: [2]f64 = [1.0, nan_val];
    expect_eq(DecisionTree.fitClassifier(x_nan[0..], y_cls_ok[0..], 2, 2, 2, cfg_c, *pool, *ws) == null, true);
    expect_eq(DecisionTree.fitClassifier(x_inf[0..], y_cls_ok[0..], 2, 2, 2, cfg_c, *pool, *ws) == null, true);
    expect_eq(DecisionTree.fitRegressor(x_nan[0..], y_reg_ok[0..], 2, 2, cfg_r, *pool, *ws) == null, true);
    expect_eq(DecisionTree.fitRegressor(x_ok[0..], y_nan[0..], 2, 2, cfg_r, *pool, *ws) == null, true);

    // Empty node buffer
    let empty_pool: []TreeNode = pool_b[0..0];
    expect_eq(DecisionTree.fitClassifier(x_ok[0..], y_cls_ok[0..], 2, 2, 2, cfg_c, *empty_pool, *ws) == null, true);

    // Short workspace buffers
    let ws_short = TreeWorkspace {
        sample_indices: idx_b[0..1],
        sort_tmp: tmp_b[0..4],
        class_counts: cc_b[0..2],
        left_counts: lc_b[0..2],
        stack_node: sn_b[0..16],
        stack_lo: slo_b[0..16],
        stack_hi: shi_b[0..16],
        stack_depth: sd_b[0..16],
    };
    expect_eq(DecisionTree.fitClassifier(x_ok[0..], y_cls_ok[0..], 2, 2, 2, cfg_c, *pool, *ws_short) == null, true);

    // Cross-mode prediction and NaN query defense
    const cls_tree: DecisionTree = DecisionTree.fitClassifier(
        x_ok[0..], y_cls_ok[0..], 2, 2, 2, cfg_c, *pool, *ws,
    ) ?? unreachable;
    const q_ok: [2]f64 = [0.5, 1.5];
    const q_nan: [2]f64 = [nan_val, 1.5];
    const q_short: [1]f64 = [0.5];
    expect_eq(cls_tree.predictValue(q_ok[0..]) == null, true);
    expect_eq(cls_tree.predictClass(q_nan[0..]) == null, true);
    expect_eq(cls_tree.predictClass(q_short[0..]) == null, true);

    let pool_r_b: [16]TreeNode = [TreeNode.empty(); 16];
    let pool_r: []TreeNode = pool_r_b[0..];
    const reg_tree: DecisionTree = DecisionTree.fitRegressor(
        x_ok[0..], y_reg_ok[0..], 2, 2, cfg_r, *pool_r, *ws,
    ) ?? unreachable;
    expect_eq(reg_tree.predictClass(q_ok[0..]) == null, true);
    expect_eq(reg_tree.predictValue(q_nan[0..]) == null, true);
    expect_eq(reg_tree.predictValue(q_short[0..]) == null, true);
});
