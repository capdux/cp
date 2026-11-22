// Origin: Pure-CP Stdlib Test Suite (lib/vector/eval_harness.cp) | Created: 2026-11-22
// Production Deep Verification Suite for ANN offline evaluation harness & latency percentiles:
// exact P50/P95/P99 latency quantiles, @l2sq SIMD ground-truth with tie-breaking, deduplicated
// single/batch Recall@K, end-to-end NavGraphIndex evaluation, and empty/out-of-bounds guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    LatencyPercentiles,
    AnnEvalSummary,
    computeLatencyPercentilesInPlace,
    computeGroundTruthL2TopK,
    annRecallAtK,
    evaluateAnnBatch,
    NavGraphHit,
    NavNodeAdj,
    NavGraphIndex,
} from "cap:vector";

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn nextRandU64(state: *u64): u64 {
    const c_add: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
    const c_m1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
    const c_m2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
    state.* = state.* +% c_add;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% c_m1;
    z = (z ^ (z >> 27)) *% c_m2;
    return z ^ (z >> 31);
}

fn nextRandUnitF32(state: *u64): f32 {
    const r: u64 = nextRandU64(state) >> 40;
    const u: f32 = (r as f32) / 16777216.0;
    return (u * 2.0) - 1.0;
}

// ============================================================================
// Test 1: Exact P50 / P95 / P99 Latency Percentiles (In-Place Heapsort)
// ============================================================================

fn testLatencyPercentilesExact()! {
    // 100 reverse-ordered samples 100..1 -> sorted 1..100
    let samples: [100]u64 = [0; 100];
    let i: usize = 0;
    while (i < 100) : (i += 1) {
        samples[i] = (100 - i) as u64;
    }
    let samples_s: []u64 = samples[0..];

    if (pct := cap.vector.computeLatencyPercentilesInPlace(samples_s)) {
        expect_eq(pct.count, 100 as usize);
        expect_eq(pct.min_us, 1 as u64);
        expect_eq(pct.p50_us, 50 as u64);
        expect_eq(pct.p95_us, 95 as u64);
        expect_eq(pct.p99_us, 99 as u64);
        expect_eq(pct.max_us, 100 as u64);
        expect_eq(absF64(pct.mean_us - 50.5) < 0.0001, true);
    } else {
        expect_eq(true, false);
    }

    // Verify slice was sorted ascending in-place.
    expect_eq(samples[0], 1 as u64);
    expect_eq(samples[99], 100 as u64);

    // Single-element edge case.
    let single: [1]u64 = [42];
    let single_s: []u64 = single[0..];
    if (p1 := computeLatencyPercentilesInPlace(single_s)) {
        expect_eq(p1.count, 1 as usize);
        expect_eq(p1.min_us, 42 as u64);
        expect_eq(p1.p50_us, 42 as u64);
        expect_eq(p1.p95_us, 42 as u64);
        expect_eq(p1.p99_us, 42 as u64);
        expect_eq(p1.max_us, 42 as u64);
        expect_eq(absF64(p1.mean_us - 42.0) < 0.0001, true);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 2: Native @l2sq SIMD Ground-Truth Generation & Deterministic Tie-Breaking
// ============================================================================

fn testSimdGroundTruthL2TopKAndTieBreaking()! {
    const dim: usize = 8;
    const query: [8]f32 = [0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];

    // 6 vectors in 8D:
    // doc 0: dist_sq = 4.0
    // doc 1: dist_sq = 1.0 (tie with doc 4 -> doc 1 wins)
    // doc 2: dist_sq = 9.0
    // doc 3: dist_sq = 0.0 (exact match -> rank 0)
    // doc 4: dist_sq = 1.0 (tie with doc 1 -> rank 2)
    // doc 5: dist_sq = 0.25 (rank 1)
    const corpus: [48]f32 = [
        2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // doc 0: 4.0
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // doc 1: 1.0
        3.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // doc 2: 9.0
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // doc 3: 0.0
        0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,  // doc 4: 1.0
        0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // doc 5: 0.25
    ];

    let gt_buf: [4]u32 = [0; 4];
    let gt_s: []u32 = gt_buf[0..];
    const n4: usize = cap.vector.computeGroundTruthL2TopK(query[0..], corpus[0..], 6, dim, 4, gt_s) ?? 0;
    expect_eq(n4, 4 as usize);
    expect_eq(gt_buf[0], 3 as u32);
    expect_eq(gt_buf[1], 5 as u32);
    expect_eq(gt_buf[2], 1 as u32);
    expect_eq(gt_buf[3], 4 as u32);

    // k > n_docs safely clamps to n_docs = 6.
    let gt_all: [8]u32 = [0; 8];
    let gt_all_s: []u32 = gt_all[0..];
    const n_all: usize = computeGroundTruthL2TopK(query[0..], corpus[0..], 6, dim, 8, gt_all_s) ?? 0;
    expect_eq(n_all, 6 as usize);
    expect_eq(gt_all[4], 0 as u32);
    expect_eq(gt_all[5], 2 as u32);
}

// ============================================================================
// Test 3: Single & Batch Recall@K with Duplicate Prediction Deduplication
// ============================================================================

fn testSingleAndBatchRecallWithDuplicatePredDedup()! {
    const gt: [4]u32 = [10, 20, 30, 40];
    const pred_exact: [4]u32 = [10, 20, 30, 40];
    const pred_dups: [4]u32 = [10, 10, 10, 10]; // Duplicate predictions must NOT inflate recall > 0.25!
    const empty_arr: [1]u32 = [0];
    const empty_pred: []const u32 = empty_arr[0..0];

    expect_eq(absF64((annRecallAtK(pred_exact[0..], gt[0..], 4) ?? -1.0) - 1.0) < 0.0001, true);
    expect_eq(absF64((annRecallAtK(pred_dups[0..], gt[0..], 4) ?? -1.0) - 0.25) < 0.0001, true);
    expect_eq(absF64((annRecallAtK(empty_pred, gt[0..], 4) ?? -1.0) - 0.0) < 0.0001, true);

    // Batch of 4 queries, k = 4:
    // q0: exact match (recall=1.0, top1=hit)
    // q1: 3/4 match, top1 miss (recall=0.75, top1=miss)
    // q2: 2/4 match, top1 hit (recall=0.50, top1=hit)
    // q3: duplicate [1, 1, 99, 98] against [1, 2, 3, 4] -> 1/4 match, top1 hit (recall=0.25, top1=hit)
    // Wait: let's make q3 top1 miss so exact_top1_rate == 0.50: [99, 1, 1, 98]
    const pred_flat: [16]u32 = [
        1, 2, 3, 4,
        2, 1, 3, 99,
        1, 99, 3, 98,
        99, 1, 1, 98,
    ];
    const gt_flat: [16]u32 = [
        1, 2, 3, 4,
        1, 2, 3, 4,
        1, 2, 3, 4,
        1, 2, 3, 4,
    ];

    if (summary := cap.vector.evaluateAnnBatch(pred_flat[0..], gt_flat[0..], 4, 4)) {
        expect_eq(summary.queries, 4 as usize);
        expect_eq(summary.k, 4 as usize);
        expect_eq(absF64(summary.mean_recall_at_k - 0.625) < 0.0001, true);
        expect_eq(absF64(summary.min_recall_at_k - 0.25) < 0.0001, true);
        expect_eq(absF64(summary.exact_top1_rate - 0.50) < 0.0001, true);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 4: End-to-End ANN Evaluation with NavGraphIndex + Latency Percentiles
// ============================================================================

fn testEndToEndEvalWithNavGraphIndex()! {
    const dim: usize = 32;
    const n_clusters: usize = 4;
    const per_cluster: usize = 16;
    const n_nodes: usize = 64;
    const num_queries: usize = 12;
    const k: usize = 5;

    let vecs_buf: [2048]f32 = [0.0; 2048]; // 64 * 32
    let adj_buf: [64]NavNodeAdj = [NavNodeAdj{}; 64];
    let vecs_s: []f32 = vecs_buf[0..];
    let adj_s: []NavNodeAdj = adj_buf[0..];

    if (idx_init := NavGraphIndex.init(vecs_s, adj_s, dim, 12)) {
        let idx = idx_init;
        let rng: u64 = 0x2026112233445566;
        let centers: [128]f32 = [0.0; 128]; // 4 * 32
        let c: usize = 0;
        while (c < n_clusters) : (c += 1) {
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                centers[c * dim + d] = nextRandUnitF32(&rng) * 4.0;
            }
        }

        let p: usize = 0;
        while (p < per_cluster) : (p += 1) {
            let cl: usize = 0;
            while (cl < n_clusters) : (cl += 1) {
                let v: [32]f32 = [0.0; 32];
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    v[d] = centers[cl * dim + d] + nextRandUnitF32(&rng) * 0.5;
                }
                expect_eq(idx.insert(v[0..], 48) != null, true);
            }
        }

        let pred_flat: [60]u32 = [0; 60]; // 12 * 5
        let gt_flat: [60]u32 = [0; 60];
        let lat_samples: [12]u64 = [0; 12];
        let hits_buf: [5]NavGraphHit = [NavGraphHit{}; 5];
        let hits_s: []NavGraphHit = hits_buf[0..];

        let q: usize = 0;
        while (q < num_queries) : (q += 1) {
            const cl: usize = q % n_clusters;
            let query: [32]f32 = [0.0; 32];
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                query[d] = centers[cl * dim + d] + nextRandUnitF32(&rng) * 0.55;
            }

            let gt_slice: []u32 = gt_flat[q * k .. (q + 1) * k];
            const gt_n: usize = computeGroundTruthL2TopK(query[0..], vecs_buf[0..], n_nodes, dim, k, gt_slice) ?? 0;
            expect_eq(gt_n, k);

            const found: usize = idx.search(query[0..], k, 48, hits_s) ?? 0;
            expect_eq(found, k);

            let j: usize = 0;
            while (j < k) : (j += 1) {
                pred_flat[q * k + j] = hits_buf[j].node_id;
            }
            lat_samples[q] = (20 + (q * 7)) as u64;
        }

        if (summary := evaluateAnnBatch(pred_flat[0..], gt_flat[0..], num_queries, k)) {
            expect_eq(summary.queries, num_queries);
            expect_eq(summary.k, k);
            expect_eq(summary.mean_recall_at_k >= 0.90, true);
            expect_eq(summary.exact_top1_rate >= 0.90, true);
        } else {
            expect_eq(true, false);
        }

        let lat_s: []u64 = lat_samples[0..];
        if (pct := computeLatencyPercentilesInPlace(lat_s)) {
            expect_eq(pct.count, num_queries);
            expect_eq(pct.min_us <= pct.p50_us, true);
            expect_eq(pct.p50_us <= pct.p95_us, true);
            expect_eq(pct.p95_us <= pct.p99_us, true);
            expect_eq(pct.p99_us <= pct.max_us, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 5: Empty Input & Out-of-Bounds Defensive Guards
// ============================================================================

fn testEmptyAndOutOfBoundsDefensiveGuards()! {
    // Empty latency samples -> null
    let empty_u64_arr: [1]u64 = [0];
    let empty_u64: []u64 = empty_u64_arr[0..0];
    expect_eq(computeLatencyPercentilesInPlace(empty_u64) == null, true);

    // Ground-truth guards
    const q: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    const q_nan: [4]f32 = [1.0, std.math.nan(f32), 0.0, 0.0];
    const corpus: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0];
    let gt_buf: [2]u32 = [0; 2];
    let gt_s: []u32 = gt_buf[0..];

    expect_eq(computeGroundTruthL2TopK(q[0..], corpus[0..], 2, 0, 1, gt_s) == null, true);        // dim == 0
    expect_eq(computeGroundTruthL2TopK(q[0..3], corpus[0..], 2, 4, 1, gt_s) == null, true);      // query len mismatch
    expect_eq(computeGroundTruthL2TopK(q_nan[0..], corpus[0..], 2, 4, 1, gt_s) == null, true);   // NaN query
    expect_eq(computeGroundTruthL2TopK(q[0..], corpus[0..7], 2, 4, 1, gt_s) == null, true);      // undersized corpus
    expect_eq(computeGroundTruthL2TopK(q[0..], corpus[0..], 2, 4, 3, gt_s) == null, true);       // out_gt_ids.len < k
    expect_eq(computeGroundTruthL2TopK(q[0..], corpus[0..], 0, 4, 2, gt_s) ?? 99, 0 as usize);   // n_docs == 0 -> 0
    expect_eq(computeGroundTruthL2TopK(q[0..], corpus[0..], 2, 4, 0, gt_s) ?? 99, 0 as usize);   // k == 0 -> 0

    // Recall@K & Batch guards
    const ids: [2]u32 = [1, 2];
    const empty_u32: []const u32 = ids[0..0];
    expect_eq(annRecallAtK(ids[0..], ids[0..], 0) == null, true);
    expect_eq(annRecallAtK(ids[0..], empty_u32, 2) == null, true);

    expect_eq(evaluateAnnBatch(ids[0..], ids[0..], 0, 2) == null, true);  // num_queries == 0
    expect_eq(evaluateAnnBatch(ids[0..], ids[0..], 1, 0) == null, true);  // k == 0
    expect_eq(evaluateAnnBatch(ids[0..], ids[0..], 2, 2) == null, true);  // pred/gt len (2) < 4
}

fn main() {
    test("1. vector.eval_harness: exact P50/P95/P99 latency percentiles via in-place heapsort", () => {
        testLatencyPercentilesExact()?;
    });
    test("2. vector.eval_harness: @l2sq SIMD ground-truth Top-K and deterministic tie-breaking", () => {
        testSimdGroundTruthL2TopKAndTieBreaking()?;
    });
    test("3. vector.eval_harness: single/batch Recall@K with duplicate prediction deduplication", () => {
        testSingleAndBatchRecallWithDuplicatePredDedup()?;
    });
    test("4. vector.eval_harness: end-to-end ANN evaluation with NavGraphIndex", () => {
        testEndToEndEvalWithNavGraphIndex()?;
    });
    test("5. vector.eval_harness: empty input and out-of-bounds defensive guards", () => {
        testEmptyAndOutOfBoundsDefensiveGuards()?;
    });
}
