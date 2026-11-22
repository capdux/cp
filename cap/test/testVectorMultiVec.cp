// Origin: Pure-CP Stdlib Test Suite (lib/vector/multiVec.cp) | Created: 2026-11-23
// Deep production regression tests for cap:vector Multi-Vector Chamfer/MaxSim index (`lib/vector/multiVec.cp`):
//   1. Two-stage centroid coarse pruning + weighted MaxSim rescoring & auto token L2 normalization
//   2. Symmetric Chamfer similarity penalizes off-topic noise tokens where asymmetric MaxSim ties
//   3. Coarse pruning telemetry (`total_coarse_pruned`) & exact Top-K truncation
//   4. Token-level alignment attribution (`explainAlignment`) matching exact `q2d_maxsim` sum
//   5. NaN/Inf, dimension mismatch, negative weights, duplicate doc_id & unknown doc guards
//   6. 64-doc catalog saturation & 16384-float arena exhaustion atomic guards
//   7. Zero-heap `MultiVecIndexStats`, `clear()`, and `cap.vector` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    MULTIVEC_MAX_DOCS,
    MULTIVEC_MAX_DIM,
    MULTIVEC_ARENA_FLOATS,
    MultiVecScoreMode,
    MultiVecHit,
    MultiVecTokenAlign,
    MultiVecIndex,
} from "cap:vector";

fn approxEq(a: f32, b: f32, eps: f32): bool {
    const diff: f32 = if (a >= b) a - b else b - a;
    return diff <= eps;
}

test("multiVec: two-stage weighted MaxSim ranking with auto L2 token normalization") {
    let idx = MultiVecIndex.init(4) ?? return;

    // Doc 1: tokens along e0 and e1 (unnormalized [2,0,0,0] and [0,3,0,0] -> normalized to e0, e1)
    const d1: [8]f32 = [2.0, 0.0, 0.0, 0.0,  0.0, 3.0, 0.0, 0.0];
    // Doc 2: only along e0 and e2
    const d2: [8]f32 = [1.0, 0.0, 0.0, 0.0,  0.0, 0.0, 1.0, 0.0];
    // Doc 3: orthogonal e3
    const d3: [4]f32 = [0.0, 0.0, 0.0, 1.0];

    expect_true(idx.addDocument(1, d1[0..], true));
    expect_true(idx.addDocument(2, d2[0..], true));
    expect_true(idx.addDocument(3, d3[0..], true));

    // Query: e0 (weight 1.0) + e1 (weight 3.0)
    // Doc 1 weighted MaxSim = 1.0*1.0 + 3.0*1.0 = 4.0
    // Doc 2 weighted MaxSim = 1.0*1.0 + 3.0*0.0 = 1.0
    // Doc 3 weighted MaxSim = 0.0
    const q: [8]f32 = [1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0];
    const w: [2]f32 = [1.0, 3.0];

    let hits: [3]MultiVecHit = [MultiVecHit{}; 3];
    let hits_s: []MultiVecHit = hits[0..];
    const n = idx.searchTwoStage(q[0..], w[0..], MultiVecScoreMode.weighted_maxsim, 3, 3, *hits_s) ?? 0;

    expect_eq(n, @as(usize, 3));
    expect_eq(hits[0].doc_id, @as(u64, 1));
    expect_true(approxEq(hits[0].score, 4.0, 0.0001));
    expect_eq(hits[1].doc_id, @as(u64, 2));
    expect_true(approxEq(hits[1].score, 1.0, 0.0001));
    expect_eq(hits[2].doc_id, @as(u64, 3));
    expect_true(approxEq(hits[2].score, 0.0, 0.0001));
}

test("multiVec: symmetric Chamfer penalizes noisy off-topic tokens where MaxSim ties") {
    let idx = MultiVecIndex.init(4) ?? return;

    // Doc 10: concise exact match (2 tokens: e0, e1)
    const d_clean: [8]f32 = [1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0];
    // Doc 20: contains e0, e1 PLUS 4 orthogonal noise tokens along e2/e3
    const d_noisy: [24]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];
    expect_true(idx.addDocument(10, d_clean[0..], false));
    expect_true(idx.addDocument(20, d_noisy[0..], false));

    const q: [8]f32 = [1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0];
    const empty_w: [0]f32 = [];

    let hits: [2]MultiVecHit = [MultiVecHit{}; 2];
    let hits_s: []MultiVecHit = hits[0..];

    // Under asymmetric MaxSim, both score 2.0
    const n1 = idx.searchTwoStage(q[0..], empty_w[0..], MultiVecScoreMode.weighted_maxsim, 0, 2, *hits_s) ?? 0;
    expect_eq(n1, @as(usize, 2));
    expect_true(approxEq(hits[0].score, 2.0, 0.0001));
    expect_true(approxEq(hits[1].score, 2.0, 0.0001));

    // Under symmetric Chamfer:
    //   Doc 10: q2d_avg=1.0, d2q_avg=1.0 => Chamfer = 1.0
    //   Doc 20: q2d_avg=1.0, d2q_avg=(1+1+0+0+0+0)/6=0.3333 => Chamfer = 0.6667
    const n2 = idx.searchTwoStage(q[0..], empty_w[0..], MultiVecScoreMode.chamfer_symmetric, 0, 2, *hits_s) ?? 0;
    expect_eq(n2, @as(usize, 2));
    expect_eq(hits[0].doc_id, @as(u64, 10));
    expect_true(approxEq(hits[0].score, 1.0, 0.0001));
    expect_eq(hits[1].doc_id, @as(u64, 20));
    expect_true(approxEq(hits[1].score, 0.66666, 0.001));
}

test("multiVec: Stage-1 centroid coarse pruning counts pruned docs accurately") {
    let idx = MultiVecIndex.init(4) ?? return;

    const v_pos: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v_near: [4]f32 = [0.8, 0.6, 0.0, 0.0];
    const v_neg: [4]f32 = [-1.0, 0.0, 0.0, 0.0];

    expect_true(idx.addDocument(1, v_pos[0..], true));
    expect_true(idx.addDocument(2, v_near[0..], true));
    expect_true(idx.addDocument(3, v_neg[0..], true));
    expect_true(idx.addDocument(4, v_neg[0..], true));
    expect_true(idx.addDocument(5, v_neg[0..], true));
    expect_true(idx.addDocument(6, v_neg[0..], true));

    const empty_w: [0]f32 = [];
    let hits: [2]MultiVecHit = [MultiVecHit{}; 2];
    let hits_s: []MultiVecHit = hits[0..];

    const n = idx.searchTwoStage(v_pos[0..], empty_w[0..], MultiVecScoreMode.weighted_maxsim, 2, 2, *hits_s) ?? 0;
    expect_eq(n, @as(usize, 2));
    expect_eq(hits[0].doc_id, @as(u64, 1));
    expect_eq(hits[1].doc_id, @as(u64, 2));
    expect_eq(idx.stats().total_coarse_pruned, @as(u64, 4));
}

test("multiVec: token-level alignment attribution matches exact MaxSim contribution sum") {
    let idx = MultiVecIndex.init(4) ?? return;

    // Doc 42 has 4 basis tokens in order: [e3, e1, e0, e2]
    const doc_toks: [16]f32 = [
        0.0, 0.0, 0.0, 1.0, // d[0] = e3
        0.0, 1.0, 0.0, 0.0, // d[1] = e1
        1.0, 0.0, 0.0, 0.0, // d[2] = e0
        0.0, 0.0, 1.0, 0.0, // d[3] = e2
    ];
    expect_true(idx.addDocument(42, doc_toks[0..], false));

    // Query has 3 tokens: [e0, e2, e3] with weights [2.0, 0.5, 1.5]
    const q_toks: [12]f32 = [
        1.0, 0.0, 0.0, 0.0, // matches d[2]
        0.0, 0.0, 1.0, 0.0, // matches d[3]
        0.0, 0.0, 0.0, 1.0, // matches d[0]
    ];
    const q_w: [3]f32 = [2.0, 0.5, 1.5];

    let pairs: [3]MultiVecTokenAlign = [MultiVecTokenAlign{}; 3];
    let pairs_s: []MultiVecTokenAlign = pairs[0..];
    const n = idx.explainAlignment(42, q_toks[0..], q_w[0..], *pairs_s) ?? 0;
    expect_eq(n, @as(usize, 3));

    expect_eq(pairs[0].query_token_idx, @as(usize, 0));
    expect_eq(pairs[0].best_doc_token_idx, @as(usize, 2));
    expect_true(approxEq(pairs[0].weighted_contribution, 2.0, 0.00001));

    expect_eq(pairs[1].query_token_idx, @as(usize, 1));
    expect_eq(pairs[1].best_doc_token_idx, @as(usize, 3));
    expect_true(approxEq(pairs[1].weighted_contribution, 0.5, 0.00001));

    expect_eq(pairs[2].query_token_idx, @as(usize, 2));
    expect_eq(pairs[2].best_doc_token_idx, @as(usize, 0));
    expect_true(approxEq(pairs[2].weighted_contribution, 1.5, 0.00001));

    // Sum of weighted contributions equals Stage-2 q2d_maxsim (4.0)
    let hits: [1]MultiVecHit = [MultiVecHit{}; 1];
    let hits_s: []MultiVecHit = hits[0..];
    * = idx.searchTwoStage(q_toks[0..], q_w[0..], MultiVecScoreMode.weighted_maxsim, 1, 1, *hits_s);
    const contrib_sum: f32 = pairs[0].weighted_contribution + pairs[1].weighted_contribution + pairs[2].weighted_contribution;
    expect_true(approxEq(contrib_sum, hits[0].q2d_maxsim, 0.00001));
}

test("multiVec: NaN/Inf, dimension mismatch, negative weight & unknown doc guards") {
    expect_true(MultiVecIndex.init(0) == null);
    expect_true(MultiVecIndex.init(MULTIVEC_MAX_DIM + 1) == null);

    let idx = MultiVecIndex.init(4) ?? return;
    const good: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const bad_dim: [3]f32 = [1.0, 0.0, 0.0];
    const zero_f: f32 = 0.0;
    const nan_val: f32 = zero_f / zero_f;
    const bad_nan: [4]f32 = [nan_val, 0.0, 0.0, 0.0];

    expect_eq(idx.addDocument(0, good[0..], true), false);
    expect_eq(idx.addDocument(1, bad_dim[0..], true), false);
    expect_eq(idx.addDocument(1, bad_nan[0..], true), false);
    expect_true(idx.addDocument(1, good[0..], true));
    expect_eq(idx.addDocument(1, good[0..], true), false); // duplicate doc_id

    let hits: [2]MultiVecHit = [MultiVecHit{}; 2];
    let hits_s: []MultiVecHit = hits[0..];
    const empty_w: [0]f32 = [];
    const neg_w: [1]f32 = [-1.0];
    expect_true(idx.searchTwoStage(bad_dim[0..], empty_w[0..], MultiVecScoreMode.weighted_maxsim, 2, 2, *hits_s) == null);
    expect_true(idx.searchTwoStage(bad_nan[0..], empty_w[0..], MultiVecScoreMode.weighted_maxsim, 2, 2, *hits_s) == null);
    expect_true(idx.searchTwoStage(good[0..], neg_w[0..], MultiVecScoreMode.weighted_maxsim, 2, 2, *hits_s) == null);

    let pairs: [2]MultiVecTokenAlign = [MultiVecTokenAlign{}; 2];
    let pairs_s: []MultiVecTokenAlign = pairs[0..];
    expect_true(idx.explainAlignment(999, good[0..], empty_w[0..], *pairs_s) == null);
}

test("multiVec: 64-doc catalog saturation and 16384-float arena exhaustion guards") {
    // Part A: 64 docs saturation
    let idx_docs = MultiVecIndex.init(4) ?? return;
    const tok: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    let did: u64 = 1;
    while (did <= @as(u64, MULTIVEC_MAX_DOCS)) {
        expect_true(idx_docs.addDocument(did, tok[0..], false));
        did += 1;
    }
    expect_eq(idx_docs.stats().doc_count, MULTIVEC_MAX_DOCS);
    expect_eq(idx_docs.addDocument(999, tok[0..], false), false);

    // Part B: 16384-float arena exhaustion
    let idx_arena = MultiVecIndex.init(32) ?? return;
    const big_doc: [2048]f32 = [1.0; 2048]; // 64 tokens * 32 dim = 2048 floats
    let k: u64 = 1;
    while (k <= 8) {
        expect_true(idx_arena.addDocument(k, big_doc[0..], false));
        k += 1;
    }
    expect_eq(idx_arena.stats().arena_used_floats, MULTIVEC_ARENA_FLOATS);
    // 9th doc overflows arena -> rejected atomically
    expect_eq(idx_arena.addDocument(9, big_doc[0..], false), false);
    expect_eq(idx_arena.stats().doc_count, @as(usize, 8));
}

test("multiVec: stats snapshot, clear, and cap.vector facade parity") {
    let idx = cap.vector.MultiVecIndex.init(4) ?? return;
    const d1: [8]f32 = [1.0, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0, 0.0];
    expect_true(idx.addDocument(7, d1[0..], true));

    const st = idx.stats();
    expect_eq(st.dim, @as(usize, 4));
    expect_eq(st.doc_count, @as(usize, 1));
    expect_eq(st.total_tokens, @as(usize, 2));
    expect_eq(st.arena_used_floats, @as(usize, 8));

    idx.clear();
    const st2 = idx.stats();
    expect_eq(st2.doc_count, @as(usize, 0));
    expect_eq(st2.total_tokens, @as(usize, 0));
    expect_eq(st2.arena_used_floats, @as(usize, 0));
}
