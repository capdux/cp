// Source: cp/cap/test/test_kmeans_production_deep.cp (Production deep tests for cap:kmeans)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    KmeansResult,
    sqDist,
    kmeansPredictOne,
    kmeansInitPlusPlus,
    kmeansFit,
    kmeansInertia,
    kmeansSilhouetteScore,
} from "cap:ml";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn findClosestTrueCenter(
    cx: []const f64,
    true_centers: []const f64,
    k: usize,
    dim: usize,
): f64 {
    let best: f64 = sqDist(cx, true_centers[0..dim]);
    let c: usize = 1;
    while (c < k) {
        const base: usize = c * dim;
        const d: f64 = sqDist(cx, true_centers[base..(base + dim)]);
        if (d < best) {
            best = d;
        }
        c += 1;
    }
    return std.math.sqrt(best);
}

test("1. 2D well-separated 3-cluster convergence, centroid accuracy < 0.1 & Silhouette > 0.85", () => {
    // 3 clusters of 5 points each around (0, 0), (10, 0), (5, 10)
    const points: [30]f64 = [
        // Cluster A: mean = (0.0, 0.0)
        0.0, 0.0,   0.1, 0.05,  -0.1, -0.05,  0.05, -0.1,  -0.05, 0.1,
        // Cluster B: mean = (10.0, 0.0)
        10.0, 0.0,  10.1, 0.08,  9.9, -0.08,  10.06, -0.04,  9.94, 0.04,
        // Cluster C: mean = (5.0, 10.0)
        5.0, 10.0,  5.08, 10.06,  4.92, 9.94,  5.04, 9.96,  4.96, 10.04,
    ];
    const true_centers: [6]f64 = [0.0, 0.0, 10.0, 0.0, 5.0, 10.0];

    let c_buf: [6]f64 = std.mem.zeroes([6]f64);
    let l_buf: [15]usize = std.mem.zeroes([15]usize);
    let cnt_buf: [3]usize = std.mem.zeroes([3]usize);
    let d_buf: [15]f64 = std.mem.zeroes([15]f64);

    let centroids: []f64 = c_buf[0..];
    let labels: []usize = l_buf[0..];
    let counts: []usize = cnt_buf[0..];
    let dists: []f64 = d_buf[0..];

    const res: KmeansResult = kmeansFit(
        points[0..], 15, 2, 3, 100, 1.0e-6, 12345,
        *centroids, *labels, *counts, *dists,
    ) ?? unreachable;

    expect_eq(res.converged, true);
    expect_eq(res.iterations >= 1 && res.iterations <= 100, true);

    // Points within each true group must share identical label
    const la: usize = labels[0];
    const lb: usize = labels[5];
    const lc: usize = labels[10];
    expect_eq(la != lb && lb != lc && la != lc, true);

    let idx: usize = 0;
    while (idx < 5) {
        expect_eq(labels[idx], la);
        expect_eq(labels[5 + idx], lb);
        expect_eq(labels[10 + idx], lc);
        idx += 1;
    }

    // Every fitted centroid matches its true center within < 0.1
    let c: usize = 0;
    while (c < 3) {
        const err: f64 = findClosestTrueCenter(
            centroids[(c * 2)..(c * 2 + 2)],
            true_centers[0..],
            3,
            2,
        );
        expect_eq(err < 0.1, true);
        c += 1;
    }

    // Inertia matches standalone kmeansInertia
    const sse: f64 = kmeansInertia(points[0..], 15, 2, centroids, 3, labels) ?? unreachable;
    expect_eq(absF64(res.inertia - sse) < 1.0e-12, true);
    expect_eq(sse < 0.2, true);

    // Silhouette score > 0.85
    const sil: f64 = kmeansSilhouetteScore(points[0..], 15, 2, labels, 3) ?? unreachable;
    expect_eq(sil > 0.85 && sil <= 1.0, true);
});

test("2. 3D well-separated 3-cluster convergence, centroid accuracy < 0.1 & Silhouette > 0.85", () => {
    // 3 clusters of 4 points each in 3D:
    // A around (0, 0, 0), B around (10, 10, 0), C around (0, 10, 10)
    const points: [36]f64 = [
        // Cluster A
        0.0, 0.0, 0.0,    0.1, -0.1, 0.05,   -0.1, 0.1, -0.05,   0.0, 0.0, 0.0,
        // Cluster B
        10.0, 10.0, 0.0,  10.1, 9.9, 0.05,   9.9, 10.1, -0.05,   10.0, 10.0, 0.0,
        // Cluster C
        0.0, 10.0, 10.0,  0.08, 10.06, 9.95, -0.08, 9.94, 10.05, 0.0, 10.0, 10.0,
    ];
    const true_centers: [9]f64 = [0.0, 0.0, 0.0, 10.0, 10.0, 0.0, 0.0, 10.0, 10.0];

    let c_buf: [9]f64 = std.mem.zeroes([9]f64);
    let l_buf: [12]usize = std.mem.zeroes([12]usize);
    let cnt_buf: [3]usize = std.mem.zeroes([3]usize);
    let d_buf: [12]f64 = std.mem.zeroes([12]f64);

    let centroids: []f64 = c_buf[0..];
    let labels: []usize = l_buf[0..];
    let counts: []usize = cnt_buf[0..];
    let dists: []f64 = d_buf[0..];

    const res: KmeansResult = kmeansFit(
        points[0..], 12, 3, 3, 100, 1.0e-7, 2026,
        *centroids, *labels, *counts, *dists,
    ) ?? unreachable;

    expect_eq(res.converged, true);

    const la: usize = labels[0];
    const lb: usize = labels[4];
    const lc: usize = labels[8];
    expect_eq(la != lb && lb != lc && la != lc, true);

    let idx: usize = 0;
    while (idx < 4) {
        expect_eq(labels[idx], la);
        expect_eq(labels[4 + idx], lb);
        expect_eq(labels[8 + idx], lc);
        idx += 1;
    }

    let c: usize = 0;
    while (c < 3) {
        const err: f64 = findClosestTrueCenter(
            centroids[(c * 3)..(c * 3 + 3)],
            true_centers[0..],
            3,
            3,
        );
        expect_eq(err < 0.1, true);
        c += 1;
    }

    const sil: f64 = kmeansSilhouetteScore(points[0..], 12, 3, labels, 3) ?? unreachable;
    expect_eq(sil > 0.85 && sil <= 1.0, true);
});

test("3. Deterministic seed reproducibility & kmeansPredictOne classification", () => {
    const points: [18]f64 = [
        -10.0, -10.0,  -9.8, -10.2,  -10.2, -9.8,
        0.0, 0.0,       0.2, -0.1,   -0.2, 0.1,
        10.0, 10.0,    10.3, 9.7,     9.7, 10.3,
    ];

    let c1_buf: [6]f64 = std.mem.zeroes([6]f64);
    let l1_buf: [9]usize = std.mem.zeroes([9]usize);
    let cnt1_buf: [3]usize = std.mem.zeroes([3]usize);
    let d1_buf: [9]f64 = std.mem.zeroes([9]f64);

    let c1: []f64 = c1_buf[0..];
    let l1: []usize = l1_buf[0..];
    let cnt1: []usize = cnt1_buf[0..];
    let d1: []f64 = d1_buf[0..];

    let c2_buf: [6]f64 = std.mem.zeroes([6]f64);
    let l2_buf: [9]usize = std.mem.zeroes([9]usize);
    let cnt2_buf: [3]usize = std.mem.zeroes([3]usize);
    let d2_buf: [9]f64 = std.mem.zeroes([9]f64);

    let c2: []f64 = c2_buf[0..];
    let l2: []usize = l2_buf[0..];
    let cnt2: []usize = cnt2_buf[0..];
    let d2: []f64 = d2_buf[0..];

    const seed: u64 = 0xCAFEBABE12345678;
    const r1: KmeansResult = kmeansFit(points[0..], 9, 2, 3, 50, 1.0e-8, seed, *c1, *l1, *cnt1, *d1) ?? unreachable;
    const r2: KmeansResult = kmeansFit(points[0..], 9, 2, 3, 50, 1.0e-8, seed, *c2, *l2, *cnt2, *d2) ?? unreachable;

    expect_eq(r1.iterations, r2.iterations);
    expect_eq(r1.converged, r2.converged);
    expect_eq(r1.inertia, r2.inertia);

    let i: usize = 0;
    while (i < 9) {
        expect_eq(l1[i], l2[i]);
        i += 1;
    }
    let j: usize = 0;
    while (j < 6) {
        expect_eq(c1[j], c2[j]);
        j += 1;
    }

    // Predict unseen points near each cluster
    const q_neg: [2]f64 = [-9.5, -10.4];
    const q_zero: [2]f64 = [0.3, -0.2];
    const q_pos: [2]f64 = [10.5, 9.6];

    const p_neg: usize = kmeansPredictOne(q_neg[0..], c1, 3, 2) ?? unreachable;
    const p_zero: usize = kmeansPredictOne(q_zero[0..], c1, 3, 2) ?? unreachable;
    const p_pos: usize = kmeansPredictOne(q_pos[0..], c1, 3, 2) ?? unreachable;

    expect_eq(p_neg, l1[0]);
    expect_eq(p_zero, l1[3]);
    expect_eq(p_pos, l1[6]);
});

test("4. Degenerate exact cases: k == 1 equals global mean, k == n gives zero inertia", () => {
    const points: [8]f64 = [
        1.0, 2.0,
        3.0, 4.0,
        5.0, 6.0,
        7.0, 8.0,
    ];

    // k = 1 -> centroid must equal exact global mean (4.0, 5.0)
    let c1_buf: [2]f64 = std.mem.zeroes([2]f64);
    let l1_buf: [4]usize = std.mem.zeroes([4]usize);
    let cnt1_buf: [1]usize = std.mem.zeroes([1]usize);
    let d1_buf: [4]f64 = std.mem.zeroes([4]f64);

    let c1: []f64 = c1_buf[0..];
    let l1: []usize = l1_buf[0..];
    let cnt1: []usize = cnt1_buf[0..];
    let d1: []f64 = d1_buf[0..];

    const r1: KmeansResult = kmeansFit(points[0..], 4, 2, 1, 20, 1.0e-12, 7, *c1, *l1, *cnt1, *d1) ?? unreachable;
    expect_eq(r1.converged, true);
    expect_eq(absF64(c1[0] - 4.0) < 1.0e-12, true);
    expect_eq(absF64(c1[1] - 5.0) < 1.0e-12, true);
    // Exact SSE = 2 * ((-3)^2 + (-1)^2 + 1^2 + 3^2) = 40.0
    expect_eq(absF64(r1.inertia - 40.0) < 1.0e-12, true);

    // k == n -> every distinct sample is its own centroid, inertia == 0.0
    let cn_buf: [8]f64 = std.mem.zeroes([8]f64);
    let ln_buf: [4]usize = std.mem.zeroes([4]usize);
    let cntn_buf: [4]usize = std.mem.zeroes([4]usize);
    let dn_buf: [4]f64 = std.mem.zeroes([4]f64);

    let cn: []f64 = cn_buf[0..];
    let ln: []usize = ln_buf[0..];
    let cntn: []usize = cntn_buf[0..];
    let dn: []f64 = dn_buf[0..];

    const rn: KmeansResult = kmeansFit(points[0..], 4, 2, 4, 20, 1.0e-12, 99, *cn, *ln, *cntn, *dn) ?? unreachable;
    expect_eq(rn.converged, true);
    expect_eq(absF64(rn.inertia) < 1.0e-12, true);
    // All 4 labels are distinct
    expect_eq(ln[0] != ln[1] && ln[0] != ln[2] && ln[0] != ln[3], true);
    expect_eq(ln[1] != ln[2] && ln[1] != ln[3] && ln[2] != ln[3], true);
});

test("5. Invalid parameters and insufficient buffers safely rejected", () => {
    const points: [6]f64 = [0.0, 0.0, 1.0, 1.0, 2.0, 2.0];
    let c_buf: [6]f64 = std.mem.zeroes([6]f64);
    let l_buf: [3]usize = std.mem.zeroes([3]usize);
    let cnt_buf: [3]usize = std.mem.zeroes([3]usize);
    let d_buf: [3]f64 = std.mem.zeroes([3]f64);

    let centroids: []f64 = c_buf[0..];
    let labels: []usize = l_buf[0..];
    let counts: []usize = cnt_buf[0..];
    let dists: []f64 = d_buf[0..];

    // k == 0
    expect_eq(kmeansInitPlusPlus(points[0..], 3, 2, 0, 1, *centroids, *dists), false);
    expect_eq(kmeansFit(points[0..], 3, 2, 0, 10, 1.0e-5, 1, *centroids, *labels, *counts, *dists) == null, true);
    expect_eq(kmeansPredictOne(points[0..2], centroids, 0, 2) == null, true);

    // k > n
    expect_eq(kmeansInitPlusPlus(points[0..], 3, 2, 4, 1, *centroids, *dists), false);
    expect_eq(kmeansFit(points[0..], 3, 2, 4, 10, 1.0e-5, 1, *centroids, *labels, *counts, *dists) == null, true);

    // dim == 0
    expect_eq(kmeansInitPlusPlus(points[0..], 3, 0, 2, 1, *centroids, *dists), false);
    expect_eq(kmeansFit(points[0..], 3, 0, 2, 10, 1.0e-5, 1, *centroids, *labels, *counts, *dists) == null, true);
    expect_eq(kmeansPredictOne(points[0..2], centroids, 2, 0) == null, true);

    // Negative tol
    expect_eq(kmeansFit(points[0..], 3, 2, 2, 10, -0.1, 1, *centroids, *labels, *counts, *dists) == null, true);

    // Insufficient buffers
    let short_c: []f64 = c_buf[0..3]; // needs 2 * 2 = 4
    let short_l: []usize = l_buf[0..2]; // needs 3
    let short_cnt: []usize = cnt_buf[0..1]; // needs 2
    let short_d: []f64 = d_buf[0..2]; // needs 3

    expect_eq(kmeansFit(points[0..4], 3, 2, 2, 10, 1.0e-5, 1, *centroids, *labels, *counts, *dists) == null, true);
    expect_eq(kmeansFit(points[0..], 3, 2, 2, 10, 1.0e-5, 1, *short_c, *labels, *counts, *dists) == null, true);
    expect_eq(kmeansFit(points[0..], 3, 2, 2, 10, 1.0e-5, 1, *centroids, *short_l, *counts, *dists) == null, true);
    expect_eq(kmeansFit(points[0..], 3, 2, 2, 10, 1.0e-5, 1, *centroids, *labels, *short_cnt, *dists) == null, true);
    expect_eq(kmeansFit(points[0..], 3, 2, 2, 10, 1.0e-5, 1, *centroids, *labels, *counts, *short_d) == null, true);

    // Silhouette rejected for k == 1, k > n, or all-same-label degenerate assignment
    const all_zero: [3]usize = [0, 0, 0];
    expect_eq(kmeansSilhouetteScore(points[0..], 3, 2, all_zero[0..], 1) == null, true);
    expect_eq(kmeansSilhouetteScore(points[0..], 3, 2, all_zero[0..], 2) == null, true);
    expect_eq(kmeansSilhouetteScore(points[0..], 3, 2, all_zero[0..], 4) == null, true);

    // Out-of-range label rejected by inertia and silhouette
    const bad_labels: [3]usize = [0, 5, 1];
    expect_eq(kmeansInertia(points[0..], 3, 2, centroids, 2, bad_labels[0..]) == null, true);
    expect_eq(kmeansSilhouetteScore(points[0..], 3, 2, bad_labels[0..], 2) == null, true);
});

test("6. High-dimensional (D = 70 > 64 stack block) clustering & sqDist accuracy", () => {
    // Verify sqDist exactness
    const va: [4]f64 = [1.0, -2.0, 3.0, 4.0];
    const vb: [4]f64 = [4.0, 2.0, 3.0, -1.0];
    // (3)^2 + (-4)^2 + 0 + (5)^2 = 9 + 16 + 0 + 25 = 50.0
    expect_eq(absF64(sqDist(va[0..], vb[0..]) - 50.0) < 1.0e-12, true);

    // 4 samples in 70 dimensions: 2 near 0.0, 2 near 10.0
    let pts: [280]f64 = std.mem.zeroes([280]f64);
    let d: usize = 0;
    while (d < 70) {
        pts[0 * 70 + d] = 0.0;
        pts[1 * 70 + d] = 0.1;
        pts[2 * 70 + d] = 10.0;
        pts[3 * 70 + d] = 10.1;
        d += 1;
    }

    let c_buf: [140]f64 = std.mem.zeroes([140]f64);
    let l_buf: [4]usize = std.mem.zeroes([4]usize);
    let cnt_buf: [2]usize = std.mem.zeroes([2]usize);
    let d_buf: [4]f64 = std.mem.zeroes([4]f64);

    let centroids: []f64 = c_buf[0..];
    let labels: []usize = l_buf[0..];
    let counts: []usize = cnt_buf[0..];
    let dists: []f64 = d_buf[0..];

    const res: KmeansResult = kmeansFit(
        pts[0..], 4, 70, 2, 30, 1.0e-9, 777,
        *centroids, *labels, *counts, *dists,
    ) ?? unreachable;

    expect_eq(res.converged, true);
    expect_eq(labels[0], labels[1]);
    expect_eq(labels[2], labels[3]);
    expect_eq(labels[0] != labels[2], true);

    // Check dim 69 (second stack block) converged to 0.05 and 10.05
    const c_low: usize = labels[0];
    const c_high: usize = labels[2];
    expect_eq(absF64(centroids[c_low * 70 + 69] - 0.05) < 1.0e-9, true);
    expect_eq(absF64(centroids[c_high * 70 + 69] - 10.05) < 1.0e-9, true);

    const sil: f64 = kmeansSilhouetteScore(pts[0..], 4, 70, labels, 2) ?? unreachable;
    expect_eq(sil > 0.95, true);
});
