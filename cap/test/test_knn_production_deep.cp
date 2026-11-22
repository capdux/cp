// Source: cp/cap/test/test_knn_production_deep.cp (Production deep tests for cap:knn)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DistanceMetric,
    WeightMode,
    Neighbor,
    distanceMetric,
    findKNearest,
    knnClassify,
    knnRegress,
    knnLeaveOneOutAccuracy,
} from "cap:ml";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return (if (d < 0.0) -d else d) <= eps;
}

test("1. All four distance metrics (Euclidean, Manhattan, Chebyshev, Cosine) exact verification", () => {
    const a = [1.0, 2.0, 3.0];
    const b = [4.0, 6.0, 3.0];

    // Euclidean: sqrt(3^2 + 4^2 + 0^2) = 5.0
    expect_eq(approxEq(distanceMetric(a[0..], b[0..], .Euclidean), 5.0, 1e-12), true);
    // Manhattan: |3| + |4| + |0| = 7.0
    expect_eq(approxEq(distanceMetric(a[0..], b[0..], .Manhattan), 7.0, 1e-12), true);
    // Chebyshev: max(3, 4, 0) = 4.0
    expect_eq(approxEq(distanceMetric(a[0..], b[0..], .Chebyshev), 4.0, 1e-12), true);

    // Cosine: identical direction -> 0.0; orthogonal -> 1.0; opposite -> 2.0; zero vectors handled safely
    const u = [1.0, 0.0];
    const v_same = [5.0, 0.0];
    const v_orth = [0.0, 3.0];
    const v_opp = [-2.0, 0.0];
    const v_zero = [0.0, 0.0];

    expect_eq(approxEq(distanceMetric(u[0..], v_same[0..], .Cosine), 0.0, 1e-12), true);
    expect_eq(approxEq(distanceMetric(u[0..], v_orth[0..], .Cosine), 1.0, 1e-12), true);
    expect_eq(approxEq(distanceMetric(u[0..], v_opp[0..], .Cosine), 2.0, 1e-12), true);
    expect_eq(approxEq(distanceMetric(v_zero[0..], v_zero[0..], .Cosine), 0.0, 1e-12), true);
    expect_eq(approxEq(distanceMetric(u[0..], v_zero[0..], .Cosine), 1.0, 1e-12), true);
});

test("2. findKNearest ascending sort order & deterministic index tie-breaking", () => {
    // 6 points at 1D coordinates; indices 1, 3, 5 all lie at distance 2.0 from query 0.0
    const train_x = [
        5.0,  // idx 0: dist 5
        -2.0, // idx 1: dist 2
        1.0,  // idx 2: dist 1
        2.0,  // idx 3: dist 2
        0.5,  // idx 4: dist 0.5
        -2.0, // idx 5: dist 2
    ];
    const query = [0.0];
    let out: [4]Neighbor = std.mem.zeroes([4]Neighbor);
    let out_s: []Neighbor = out[0..];

    const cnt = findKNearest(train_x[0..], 6, 1, query[0..], 4, .Euclidean, &out_s) ?? unreachable;
    expect_eq(cnt, 4 as usize);

    // Expected order: idx 4 (0.5), idx 2 (1.0), idx 1 (2.0), idx 3 (2.0) — idx 5 excluded by tie-break
    expect_eq(out[0].index, 4 as usize);
    expect_eq(approxEq(out[0].distance, 0.5, 1e-12), true);
    expect_eq(out[1].index, 2 as usize);
    expect_eq(approxEq(out[1].distance, 1.0, 1e-12), true);
    expect_eq(out[2].index, 1 as usize);
    expect_eq(approxEq(out[2].distance, 2.0, 1e-12), true);
    expect_eq(out[3].index, 3 as usize);
    expect_eq(approxEq(out[3].distance, 2.0, 1e-12), true);
});

test("3. knnClassify: Uniform vs InverseDistance weighting & exact zero-distance match", () => {
    // Query at (0.0, 0.0):
    //   sample 0: (0.1, 0.0) -> class 10 (dist 0.1, weight 10.0)
    //   sample 1: (5.0, 0.0) -> class 20 (dist 5.0, weight 0.2)
    //   sample 2: (-5.0, 0.0) -> class 20 (dist 5.0, weight 0.2)
    // With k = 3:
    //   Uniform voting: class 20 wins (2 votes vs 1 vote)
    //   InverseDistance voting: class 10 wins (10.0 vs 0.4)
    const train_x = [
        0.1, 0.0,
        5.0, 0.0,
        -5.0, 0.0,
        20.0, 20.0,
    ];
    const labels: [4]i32 = [10, 20, 20, 30];
    const query = [0.0, 0.0];

    let scratch: [4]Neighbor = std.mem.zeroes([4]Neighbor);
    let s: []Neighbor = scratch[0..];

    const pred_unif = knnClassify(train_x[0..], labels[0..], 4, 2, query[0..], 3, .Euclidean, .Uniform, &s) ?? unreachable;
    expect_eq(pred_unif, 20 as i32);

    const pred_inv = knnClassify(train_x[0..], labels[0..], 4, 2, query[0..], 3, .Euclidean, .InverseDistance, &s) ?? unreachable;
    expect_eq(pred_inv, 10 as i32);

    // Exact zero-distance match on sample 3 returns 30 immediately even under Uniform k=3
    const exact_q = [20.0, 20.0];
    const pred_exact = knnClassify(train_x[0..], labels[0..], 4, 2, exact_q[0..], 3, .Euclidean, .Uniform, &s) ?? unreachable;
    expect_eq(pred_exact, 30 as i32);
});

test("4. knnRegress: linear interpolation, InverseDistance accuracy & multi-point zero-distance average", () => {
    // Samples on y = 2x + 1: x = [0, 1, 4, 10], y = [1, 3, 9, 21]
    const train_x = [0.0, 1.0, 4.0, 10.0];
    const train_y = [1.0, 3.0, 9.0, 21.0];
    let scratch: [4]Neighbor = std.mem.zeroes([4]Neighbor);
    let s: []Neighbor = scratch[0..];

    // Query x = 2.0 with k = 2 -> neighbors x=1 (d=1, y=3) and x=4 (d=2, y=9)
    const q = [2.0];
    // Uniform: (3 + 9) / 2 = 6.0
    const y_unif = knnRegress(train_x[0..], train_y[0..], 4, 1, q[0..], 2, .Euclidean, .Uniform, &s) ?? unreachable;
    expect_eq(approxEq(y_unif, 6.0, 1e-12), true);

    // InverseDistance: (1*3 + 0.5*9) / (1 + 0.5) = 7.5 / 1.5 = 5.0 == exact 2*(2.0)+1!
    const y_inv = knnRegress(train_x[0..], train_y[0..], 4, 1, q[0..], 2, .Euclidean, .InverseDistance, &s) ?? unreachable;
    expect_eq(approxEq(y_inv, 5.0, 1e-12), true);

    // Duplicate zero-distance points averaged directly: two samples at x=5.0 with y=10.0 and y=20.0 -> 15.0
    const dup_x = [5.0, 5.0, 100.0];
    const dup_y = [10.0, 20.0, 999.0];
    const q_dup = [5.0];
    const y_zero = knnRegress(dup_x[0..], dup_y[0..], 3, 1, q_dup[0..], 3, .Euclidean, .InverseDistance, &s) ?? unreachable;
    expect_eq(approxEq(y_zero, 15.0, 1e-12), true);
});

test("5. knnLeaveOneOutAccuracy: 100% accuracy on 3 well-separated clusters & exact drop on label flip", () => {
    // 15 2D samples in 3 tight, well-separated clusters (5 samples each)
    const train_x = [
        // Cluster 0 around (0, 0)
        0.0, 0.0,   0.1, 0.2,   -0.1, 0.1,   0.2, -0.1,   -0.2, -0.1,
        // Cluster 1 around (20, 0)
        20.0, 0.0,  20.1, 0.2,  19.9, 0.1,   20.2, -0.1,  19.8, -0.1,
        // Cluster 2 around (10, 20)
        10.0, 20.0, 10.1, 20.2, 9.9, 20.1,   10.2, 19.9,  9.8, 19.9,
    ];
    let labels: [15]i32 = [
        0, 0, 0, 0, 0,
        1, 1, 1, 1, 1,
        2, 2, 2, 2, 2,
    ];

    let scratch: [5]Neighbor = std.mem.zeroes([5]Neighbor);
    let s: []Neighbor = scratch[0..];

    const acc_clean = knnLeaveOneOutAccuracy(
        train_x[0..],
        labels[0..],
        15,
        2,
        3,
        .Euclidean,
        .InverseDistance,
        &s,
    ) ?? unreachable;
    expect_eq(approxEq(acc_clean, 1.0, 1e-12), true);

    // Flip 1 label inside cluster 0 -> that 1 point is misclassified by its 3 neighbors => 14/15
    labels[0] = 2;
    const acc_noisy = knnLeaveOneOutAccuracy(
        train_x[0..],
        labels[0..],
        15,
        2,
        3,
        .Euclidean,
        .Uniform,
        &s,
    ) ?? unreachable;
    expect_eq(approxEq(acc_noisy, 14.0 / 15.0, 1e-12), true);
});

test("6. Defensive validation: k==0, k>n, dimension mismatch, undersized buffer & NaN rejection", () => {
    const train_x = [1.0, 2.0, 3.0, 4.0];
    const labels: [2]i32 = [0, 1];
    const targets = [1.0, 2.0];
    const q = [1.0, 2.0];
    const q_bad_dim = [1.0];

    let scratch: [2]Neighbor = std.mem.zeroes([2]Neighbor);
    let s: []Neighbor = scratch[0..];
    let short_s: []Neighbor = scratch[0..1];

    // k == 0 or k > n_samples => null
    expect_eq(findKNearest(train_x[0..], 2, 2, q[0..], 0, .Euclidean, &s) == null, true);
    expect_eq(findKNearest(train_x[0..], 2, 2, q[0..], 3, .Euclidean, &s) == null, true);
    // Feature mismatch & undersized buffer => null
    expect_eq(findKNearest(train_x[0..], 2, 2, q_bad_dim[0..], 1, .Euclidean, &s) == null, true);
    expect_eq(findKNearest(train_x[0..], 2, 2, q[0..], 2, .Euclidean, &short_s) == null, true);
    // LOO requires k <= n_samples - 1
    expect_eq(knnLeaveOneOutAccuracy(train_x[0..], labels[0..], 2, 2, 2, .Euclidean, .Uniform, &s) == null, true);
    expect_eq(knnLeaveOneOutAccuracy(train_x[0..], labels[0..], 1, 2, 1, .Euclidean, .Uniform, &s) == null, true);

    // NaN in query, training features, or regression targets => rejected
    const zero: f64 = 0.0;
    const nan_v = zero / zero;
    const q_nan = [nan_v, 2.0];
    const y_nan = [1.0, nan_v];

    expect_eq(distanceMetric(q[0..], q_nan[0..], .Euclidean), -1.0);
    expect_eq(knnClassify(train_x[0..], labels[0..], 2, 2, q_nan[0..], 1, .Euclidean, .Uniform, &s) == null, true);
    expect_eq(knnRegress(train_x[0..], y_nan[0..], 2, 2, q[0..], 2, .Euclidean, .Uniform, &s) == null, true);
    // Valid regression call succeeds
    const ok_reg = knnRegress(train_x[0..], targets[0..], 2, 2, q[0..], 1, .Euclidean, .Uniform, &s) ?? unreachable;
    expect_eq(approxEq(ok_reg, 1.0, 1e-12), true);
});
