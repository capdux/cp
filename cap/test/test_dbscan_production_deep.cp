// Source: cp/cap/test/test_dbscan_production_deep.cp (Production deep tests for cap:dbscan)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    DBSCAN_NOISE,
    DBSCAN_UNVISITED,
    DbscanMetric,
    DbscanResult,
    DbscanWorkspace,
    dbscanCluster,
    dbscanFromDistanceMatrix,
    silhouetteScore,
} from "cap:dbscan";

test("1. Non-convex concentric rings + outliers cleanly separated into 2 clusters and noise", () => {
    // 12 points on inner ring (r = 1.0), 16 points on outer ring (r = 4.0), 3 far outliers => 31 points
    const two_pi: f64 = 6.28318530717958647692;
    let pts: [62]f64 = std.mem.zeroes([62]f64);
    let i: usize = 0;
    while (i < 12) {
        const ang = two_pi * ((i as f64) / 12.0);
        pts[i * 2 + 0] = 1.0 * std.math.cos(ang);
        pts[i * 2 + 1] = 1.0 * std.math.sin(ang);
        i += 1;
    }
    while (i < 28) {
        const ang = two_pi * (((i - 12) as f64) / 16.0);
        pts[i * 2 + 0] = 4.0 * std.math.cos(ang);
        pts[i * 2 + 1] = 4.0 * std.math.sin(ang);
        i += 1;
    }
    // 3 noise points
    pts[28 * 2 + 0] = 0.0;   pts[28 * 2 + 1] = 0.0;
    pts[29 * 2 + 0] = 15.0;  pts[29 * 2 + 1] = 15.0;
    pts[30 * 2 + 0] = -12.0; pts[30 * 2 + 1] = 10.0;

    let labels: [31]i32 = std.mem.zeroes([31]i32);
    let is_core: [31]u8 = std.mem.zeroes([31]u8);
    let q: [31]usize = std.mem.zeroes([31]usize);
    let inq: [31]u8 = std.mem.zeroes([31]u8);
    let nb: [31]usize = std.mem.zeroes([31]usize);
    let ws = DbscanWorkspace { queue: q[0..], in_queue: inq[0..], neighbors_tmp: nb[0..] };

    // Chord lengths: inner ring ~0.517, outer ring ~1.56, ring gap = 3.0 -> eps = 1.8 separates rings cleanly
    // Wait: center (0,0) is distance 1.0 from inner ring! Put noise 0 at (2.5, 0) or (12, -12) so it's > 1.8 from both rings
    pts[28 * 2 + 0] = 12.0; pts[28 * 2 + 1] = -12.0;

    const res = dbscanCluster(pts[0..], 31, 2, 1.8, 3, DbscanMetric.Euclidean, labels[0..], is_core[0..], &ws) ?? unreachable;
    expect_eq(res.num_clusters, 2 as usize);
    expect_eq(res.noise_count, 3 as usize);
    expect_eq(res.core_count, 28 as usize);

    i = 0;
    while (i < 12) {
        expect_eq(labels[i], 0 as i32);
        expect_eq(is_core[i], 1 as u8);
        i += 1;
    }
    while (i < 28) {
        expect_eq(labels[i], 1 as i32);
        expect_eq(is_core[i], 1 as u8);
        i += 1;
    }
    while (i < 31) {
        expect_eq(labels[i], DBSCAN_NOISE);
        expect_eq(is_core[i], 0 as u8);
        i += 1;
    }
});

test("2. Core points vs border points vs noise points strictly distinguished", () => {
    // Points 0, 1, 2 form a tight triangle (pairwise dist <= 0.3) -> all 3 are core points for eps=0.6, min_pts=3
    // Point 3 at (0.85, 0.0) is within 0.6 of point 1 (0.3, 0.0) only (2 points in its eps-ball: {3, 1} < 3) -> BORDER point!
    // Point 4 at (10.0, 10.0) -> NOISE point!
    const pts: [10]f64 = [
        0.0, 0.0,
        0.3, 0.0,
        0.15, 0.2,
        0.85, 0.0,
        10.0, 10.0,
    ];
    let labels: [5]i32 = std.mem.zeroes([5]i32);
    let is_core: [5]u8 = std.mem.zeroes([5]u8);
    let q: [5]usize = std.mem.zeroes([5]usize);
    let inq: [5]u8 = std.mem.zeroes([5]u8);
    let nb: [5]usize = std.mem.zeroes([5]usize);
    let ws = DbscanWorkspace { queue: q[0..], in_queue: inq[0..], neighbors_tmp: nb[0..] };

    const res = dbscanCluster(pts[0..], 5, 2, 0.6, 3, DbscanMetric.Euclidean, labels[0..], is_core[0..], &ws) ?? unreachable;
    expect_eq(res.num_clusters, 1 as usize);
    expect_eq(res.core_count, 3 as usize);
    expect_eq(res.noise_count, 1 as usize);

    expect_eq(labels[0], 0 as i32); expect_eq(is_core[0], 1 as u8);
    expect_eq(labels[1], 0 as i32); expect_eq(is_core[1], 1 as u8);
    expect_eq(labels[2], 0 as i32); expect_eq(is_core[2], 1 as u8);
    // Point 3 is a border point: belongs to cluster 0, but is_core == 0
    expect_eq(labels[3], 0 as i32); expect_eq(is_core[3], 0 as u8);
    // Point 4 is noise
    expect_eq(labels[4], DBSCAN_NOISE); expect_eq(is_core[4], 0 as u8);
});

test("3. Metric differentiation: Euclidean, Manhattan, Chebyshev & Cosine", () => {
    // Two points at (0,0) and (0.8, 0.8):
    // Chebyshev = 0.8 <= 1.0 (1 cluster); Euclidean = 1.131 > 1.0 (noise); Manhattan = 1.6 > 1.0 (noise)
    const pair: [4]f64 = [0.0, 0.0, 0.8, 0.8];
    let labels: [4]i32 = std.mem.zeroes([4]i32);
    let core: [4]u8 = std.mem.zeroes([4]u8);
    let q: [4]usize = std.mem.zeroes([4]usize);
    let inq: [4]u8 = std.mem.zeroes([4]u8);
    let nb: [4]usize = std.mem.zeroes([4]usize);
    let ws = DbscanWorkspace { queue: q[0..], in_queue: inq[0..], neighbors_tmp: nb[0..] };

    const r_cheb = dbscanCluster(pair[0..], 2, 2, 1.0, 2, DbscanMetric.Chebyshev, labels[0..], core[0..], &ws) ?? unreachable;
    expect_eq(r_cheb.num_clusters, 1 as usize);

    const r_euc = dbscanCluster(pair[0..], 2, 2, 1.0, 2, DbscanMetric.Euclidean, labels[0..], core[0..], &ws) ?? unreachable;
    expect_eq(r_euc.num_clusters, 0 as usize);
    expect_eq(r_euc.noise_count, 2 as usize);

    const r_man = dbscanCluster(pair[0..], 2, 2, 1.0, 2, DbscanMetric.Manhattan, labels[0..], core[0..], &ws) ?? unreachable;
    expect_eq(r_man.num_clusters, 0 as usize);

    // Cosine metric groups collinear rays regardless of magnitude
    const rays: [8]f64 = [
        1.0, 0.0,
        100.0, 0.0,
        0.0, 1.0,
        0.0, 50.0,
    ];
    const r_cos = dbscanCluster(rays[0..], 4, 2, 0.05, 2, DbscanMetric.Cosine, labels[0..], core[0..], &ws) ?? unreachable;
    expect_eq(r_cos.num_clusters, 2 as usize);
    expect_eq(labels[0], labels[1]);
    expect_eq(labels[2], labels[3]);
    expect_eq(labels[0] != labels[2], true);
});

test("4. dbscanFromDistanceMatrix produces 100% identical results to dbscanCluster", () => {
    const pts: [16]f64 = [
        0.0, 0.0,  0.2, 0.1,  0.1, 0.25,
        5.0, 5.0,  5.2, 4.9,  4.85, 5.15,
        12.0, -3.0,
        -8.0, 9.0,
    ];
    // Precompute exact Euclidean distance matrix (8x8)
    let dmat: [64]f64 = std.mem.zeroes([64]f64);
    let i: usize = 0;
    while (i < 8) {
        let j: usize = 0;
        while (j < 8) {
            const dx = pts[i * 2 + 0] - pts[j * 2 + 0];
            const dy = pts[i * 2 + 1] - pts[j * 2 + 1];
            dmat[i * 8 + j] = std.math.sqrt(dx * dx + dy * dy);
            j += 1;
        }
        i += 1;
    }

    let l1: [8]i32 = std.mem.zeroes([8]i32);
    let c1: [8]u8 = std.mem.zeroes([8]u8);
    let l2: [8]i32 = std.mem.zeroes([8]i32);
    let c2: [8]u8 = std.mem.zeroes([8]u8);
    let q: [8]usize = std.mem.zeroes([8]usize);
    let inq: [8]u8 = std.mem.zeroes([8]u8);
    let nb: [8]usize = std.mem.zeroes([8]usize);
    let ws = DbscanWorkspace { queue: q[0..], in_queue: inq[0..], neighbors_tmp: nb[0..] };

    const r1 = dbscanCluster(pts[0..], 8, 2, 0.5, 2, DbscanMetric.Euclidean, l1[0..], c1[0..], &ws) ?? unreachable;
    const r2 = dbscanFromDistanceMatrix(dmat[0..], 8, 0.5, 2, l2[0..], c2[0..], &ws) ?? unreachable;

    expect_eq(r1.num_clusters, r2.num_clusters);
    expect_eq(r1.noise_count, r2.noise_count);
    expect_eq(r1.core_count, r2.core_count);
    i = 0;
    while (i < 8) {
        expect_eq(l1[i], l2[i]);
        expect_eq(c1[i], c2[i]);
        i += 1;
    }
});

test("5. silhouetteScore > 0.85 on well-separated clusters (ignoring noise) & null for single cluster", () => {
    const pts: [20]f64 = [
        // Cluster 0 around (0, 0)
        0.0, 0.0,  0.2, 0.1,  -0.1, 0.2,
        // Cluster 1 around (10, 0)
        10.0, 0.0, 10.2, -0.1, 9.9, 0.2,
        // Cluster 2 around (5, 10)
        5.0, 10.0, 5.1, 10.2, 4.8, 9.9,
        // Noise point
        50.0, 50.0,
    ];
    let labels: [10]i32 = std.mem.zeroes([10]i32);
    let core: [10]u8 = std.mem.zeroes([10]u8);
    let q: [10]usize = std.mem.zeroes([10]usize);
    let inq: [10]u8 = std.mem.zeroes([10]u8);
    let nb: [10]usize = std.mem.zeroes([10]usize);
    let ws = DbscanWorkspace { queue: q[0..], in_queue: inq[0..], neighbors_tmp: nb[0..] };

    const res = dbscanCluster(pts[0..], 10, 2, 1.0, 2, DbscanMetric.Euclidean, labels[0..], core[0..], &ws) ?? unreachable;
    expect_eq(res.num_clusters, 3 as usize);
    expect_eq(res.noise_count, 1 as usize);

    const sil = silhouetteScore(pts[0..], 10, 2, labels[0..], DbscanMetric.Euclidean) ?? unreachable;
    expect_eq(sil > 0.85 && sil <= 1.0, true);

    // Single-cluster labels return null for silhouetteScore
    const single: [10]i32 = [0, 0, 0, 0, 0, 0, 0, 0, 0, -1];
    expect_eq(silhouetteScore(pts[0..], 10, 2, single[0..], DbscanMetric.Euclidean) == null, true);
});

test("6. Boundary guards: eps <= 0, min_pts == 0, empty data, undersized buffers, negative dist & NaN", () => {
    expect_eq(DBSCAN_NOISE, -1 as i32);
    expect_eq(DBSCAN_UNVISITED, -2 as i32);

    const pts: [4]f64 = [0.0, 0.0, 1.0, 1.0];
    const nan_pts: [4]f64 = [0.0, std.math.nan(f64), 1.0, 1.0];
    let labels: [2]i32 = std.mem.zeroes([2]i32);
    let core: [2]u8 = std.mem.zeroes([2]u8);
    let q: [2]usize = std.mem.zeroes([2]usize);
    let inq: [2]u8 = std.mem.zeroes([2]u8);
    let nb: [2]usize = std.mem.zeroes([2]usize);
    let small1: [1]usize = std.mem.zeroes([1]usize);
    let ws = DbscanWorkspace { queue: q[0..], in_queue: inq[0..], neighbors_tmp: nb[0..] };
    let bad_ws = DbscanWorkspace { queue: small1[0..], in_queue: inq[0..], neighbors_tmp: nb[0..] };

    expect_eq(dbscanCluster(pts[0..], 0, 2, 1.0, 1, DbscanMetric.Euclidean, labels[0..], core[0..], &ws) == null, true);
    expect_eq(dbscanCluster(pts[0..], 2, 0, 1.0, 1, DbscanMetric.Euclidean, labels[0..], core[0..], &ws) == null, true);
    expect_eq(dbscanCluster(pts[0..], 2, 2, 0.0, 1, DbscanMetric.Euclidean, labels[0..], core[0..], &ws) == null, true);
    expect_eq(dbscanCluster(pts[0..], 2, 2, -0.5, 1, DbscanMetric.Euclidean, labels[0..], core[0..], &ws) == null, true);
    expect_eq(dbscanCluster(pts[0..], 2, 2, 1.0, 0, DbscanMetric.Euclidean, labels[0..], core[0..], &ws) == null, true);
    expect_eq(dbscanCluster(pts[0..], 2, 2, 1.0, 1, DbscanMetric.Euclidean, labels[0..], core[0..], &bad_ws) == null, true);
    expect_eq(dbscanCluster(nan_pts[0..], 2, 2, 1.0, 1, DbscanMetric.Euclidean, labels[0..], core[0..], &ws) == null, true);

    const neg_dmat: [4]f64 = [0.0, -1.0, -1.0, 0.0];
    expect_eq(dbscanFromDistanceMatrix(neg_dmat[0..], 2, 1.0, 1, labels[0..], core[0..], &ws) == null, true);
    expect_eq(silhouetteScore(nan_pts[0..], 2, 2, labels[0..], DbscanMetric.Euclidean) == null, true);
});
