// Origin: Pure-CP Stdlib Test Suite (lib/vector/queryFusion.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Zero-Alloc Multi-Query HyDE Centroid & Multi-View RRF/CombMNZ Fusion:
//   1. `fuseQueryVectorsF32` spherical weighted centroid & L2 normalization precision (scale-invariant)
//   2. NaN/zero-vector/non-positive-weight filtering & opposite-vector cancellation guard
//   3. `MultiQueryFusion` `weighted_rrf` ranking, per-view weighting & `consensus_mask`/`best_rank` attribution
//   4. `comb_mnz` consensus-amplified fusion promoting multi-view agreement over single-view top-1
//   5. Intra-view duplicate `doc_id` dedup, `reset()` reuse & diversity/unanimous coverage stats
//   6. 8-view capacity, invalid weight rejection & truncation-safe Top-K output
//   7. `cap.vector` namespace facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    QFUSION_MAX_VIEWS,
    QueryFusionStrategy,
    QueryViewHit,
    FusedDocHit,
    QueryFusionConfig,
    fuseQueryVectorsF32,
    MultiQueryFusion,
} from "cap:vector";

fn approxEq(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) (a - b) else (b - a);
    return diff <= eps;
}

test("QueryFusion: HyDE weighted spherical centroid & L2 normalization precision") {
    // Different input scales [10, 0] and [0, 2] normalize to [1, 0] and [0, 1], weighted 3:4 -> [0.6, 0.8]!
    const vecs: [4]f32 = [10.0, 0.0, 0.0, 2.0];
    const weights: [2]f32 = [3.0, 4.0];
    let out: [2]f32 = [0.0; 2];
    let out_s: []f32 = out[0..];

    const rep = fuseQueryVectorsF32(vecs[0..], weights[0..], 2, 2, out_s);
    expect_true(rep.valid);
    expect_eq(rep.used_views, 2 as usize);
    expect_eq(rep.skipped_views, 0 as usize);
    expect_true(approxEq(rep.total_weight, 7.0, 0.0001));
    expect_true(approxEq(out[0], 0.6, 0.0001));
    expect_true(approxEq(out[1], 0.8, 0.0001));
}

test("QueryFusion: NaN, zero-vector, non-positive weight filtering & cancellation guard") {
    const zero_f: f32 = 0.0;
    const nan_val: f32 = zero_f / zero_f;

    // View 0: valid [1, 0], View 1: zero [0, 0], View 2: NaN [nan, 1], View 3: valid [0, 1] with weight 0
    const vecs: [8]f32 = [
        1.0, 0.0,
        0.0, 0.0,
        nan_val, 1.0,
        0.0, 1.0,
    ];
    const weights: [4]f32 = [2.0, 1.0, 1.0, 0.0];
    let out: [2]f32 = [0.0; 2];
    let out_s: []f32 = out[0..];

    const rep = fuseQueryVectorsF32(vecs[0..], weights[0..], 4, 2, out_s);
    expect_true(rep.valid);
    expect_eq(rep.used_views, 1 as usize);
    expect_eq(rep.skipped_views, 3 as usize);
    expect_true(approxEq(out[0], 1.0, 0.0001));
    expect_true(approxEq(out[1], 0.0, 0.0001));

    // Equal-weight opposite vectors cancel out to zero -> valid == false
    const opp: [4]f32 = [1.0, 0.0, -1.0, 0.0];
    const eq_w: [2]f32 = [1.0, 1.0];
    expect_true(!fuseQueryVectorsF32(opp[0..], eq_w[0..], 2, 2, out_s).valid);

    // Invalid dim == 0 or num_views > 8
    expect_true(!fuseQueryVectorsF32(opp[0..], eq_w[0..], 2, 0, out_s).valid);
    expect_true(!fuseQueryVectorsF32(opp[0..], eq_w[0..], QFUSION_MAX_VIEWS + 1, 2, out_s).valid);
}

test("QueryFusion: MultiQueryFusion weighted_rrf ranking & consensus_mask attribution") {
    let f = MultiQueryFusion.init(QueryFusionConfig{
        strategy: QueryFusionStrategy.weighted_rrf,
        rrf_k: 60.0,
    });

    // View 0 (original query, weight 2.0): [doc 100, doc 200, doc 300]
    const v0: [3]QueryViewHit = [
        QueryViewHit{ doc_id: 100, score: 0.9 },
        QueryViewHit{ doc_id: 200, score: 0.8 },
        QueryViewHit{ doc_id: 300, score: 0.7 },
    ];
    // View 1 (HyDE rewrite 1, weight 1.0): [doc 200, doc 400]
    const v1: [2]QueryViewHit = [
        QueryViewHit{ doc_id: 200, score: 0.95 },
        QueryViewHit{ doc_id: 400, score: 0.6 },
    ];
    // View 2 (HyDE rewrite 2, weight 1.0): [doc 200, doc 100]
    const v2: [2]QueryViewHit = [
        QueryViewHit{ doc_id: 200, score: 0.88 },
        QueryViewHit{ doc_id: 100, score: 0.85 },
    ];

    expect_true(f.addView(2.0, v0[0..]));
    expect_true(f.addView(1.0, v1[0..]));
    expect_true(f.addView(1.0, v2[0..]));

    let out: [8]FusedDocHit = [FusedDocHit{}; 8];
    let out_s: []FusedDocHit = out[0..];
    const rep = f.fuse(out_s);

    expect_eq(rep.views_added, 3 as usize);
    expect_eq(rep.total_raw_hits, 7 as usize);
    expect_eq(rep.unique_docs, 4 as usize);
    expect_eq(rep.consensus_docs, 2 as usize); // docs 200 and 100
    expect_eq(rep.unanimous_docs, 1 as usize); // doc 200 appears in all 3 views!

    // Doc 200: 2/62 + 1/61 + 1/61 ~= 0.06504; Doc 100: 2/61 + 1/62 ~= 0.04892 -> Doc 200 wins!
    expect_eq(out[0].doc_id, 200 as u64);
    expect_eq(out[0].consensus_mask, 0b111 as u8);
    expect_eq(out[0].consensus_count, 3 as u8);
    expect_eq(out[0].best_rank, 0 as u16);

    expect_eq(out[1].doc_id, 100 as u64);
    expect_eq(out[1].consensus_mask, 0b101 as u8);
    expect_eq(out[1].consensus_count, 2 as u8);
    expect_eq(out[1].best_rank, 0 as u16);
}

test("QueryFusion: CombMNZ amplifies multi-view consensus over single-view top-1") {
    let f = MultiQueryFusion.init(QueryFusionConfig{
        strategy: QueryFusionStrategy.comb_mnz,
    });

    // Doc 1 is top-1 in view 0 only (norm 1.0 * count 1 = 1.0)
    // Doc 2 is mid-ranked in all 3 views (norm ~0.5 * 3 views = 1.5 * count 3 = 4.5!)
    const v0: [3]QueryViewHit = [
        QueryViewHit{ doc_id: 1, score: 1.0 },
        QueryViewHit{ doc_id: 2, score: 0.5 },
        QueryViewHit{ doc_id: 3, score: 0.0 },
    ];
    const v1: [3]QueryViewHit = [
        QueryViewHit{ doc_id: 4, score: 1.0 },
        QueryViewHit{ doc_id: 2, score: 0.5 },
        QueryViewHit{ doc_id: 5, score: 0.0 },
    ];
    const v2: [3]QueryViewHit = [
        QueryViewHit{ doc_id: 6, score: 1.0 },
        QueryViewHit{ doc_id: 2, score: 0.5 },
        QueryViewHit{ doc_id: 7, score: 0.0 },
    ];

    * = f.addView(1.0, v0[0..]);
    * = f.addView(1.0, v1[0..]);
    * = f.addView(1.0, v2[0..]);

    let out: [8]FusedDocHit = [FusedDocHit{}; 8];
    let out_s: []FusedDocHit = out[0..];
    const rep = f.fuse(out_s);
    expect_eq(rep.unique_docs, 7 as usize);
    expect_eq(out[0].doc_id, 2 as u64);
    expect_eq(out[0].consensus_count, 3 as u8);
    expect_true(approxEq(out[0].fused_score, 4.5, 0.0001));
}

test("QueryFusion: intra-view duplicate dedup & reset() reuse") {
    let f = MultiQueryFusion.init(QueryFusionConfig{});

    // View 0 contains duplicate doc_id 42 -> must count once with best_rank == 0
    const dup_view: [3]QueryViewHit = [
        QueryViewHit{ doc_id: 42, score: 0.9 },
        QueryViewHit{ doc_id: 42, score: 0.4 },
        QueryViewHit{ doc_id: 99, score: 0.5 },
    ];
    expect_true(f.addView(1.0, dup_view[0..]));

    let out: [4]FusedDocHit = [FusedDocHit{}; 4];
    let out_s: []FusedDocHit = out[0..];
    const r1 = f.fuse(out_s);
    expect_eq(r1.total_raw_hits, 2 as usize);
    expect_eq(r1.unique_docs, 2 as usize);
    expect_eq(out[0].doc_id, 42 as u64);
    expect_eq(out[0].consensus_count, 1 as u8);

    // Reset clears all views cleanly
    f.reset();
    expect_eq(f.viewCount(), 0 as usize);
    const r2 = f.fuse(out_s);
    expect_eq(r2.unique_docs, 0 as usize);
}

test("QueryFusion: 8-view capacity, invalid weight rejection & Top-K truncation") {
    let f = MultiQueryFusion.init(QueryFusionConfig{});
    const single: [2]QueryViewHit = [
        QueryViewHit{ doc_id: 1, score: 1.0 },
        QueryViewHit{ doc_id: 2, score: 0.5 },
    ];

    expect_true(!f.addView(0.0, single[0..]));
    expect_true(!f.addView(-1.0, single[0..]));

    let v: usize = 0;
    while (v < QFUSION_MAX_VIEWS) : (v += 1) {
        expect_true(f.addView(1.0, single[0..]));
    }
    // 9th view rejected
    expect_true(!f.addView(1.0, single[0..]));

    // Top-1 truncation safety
    let top1: [1]FusedDocHit = [FusedDocHit{}; 1];
    let top1_s: []FusedDocHit = top1[0..];
    const rep = f.fuse(top1_s);
    expect_eq(rep.unique_docs, 2 as usize);
    expect_eq(rep.output_count, 1 as usize);
    expect_eq(top1[0].doc_id, 1 as u64);
    expect_eq(top1[0].consensus_mask, 0xFF as u8);
}

test("QueryFusion: cap.vector namespace facade parity") {
    const vecs: [4]f32 = [1.0, 0.0, 0.0, 1.0];
    const w: [2]f32 = [1.0, 1.0];
    let c: [2]f32 = [0.0; 2];
    let c_s: []f32 = c[0..];
    const crep = cap.vector.fuseQueryVectorsF32(vecs[0..], w[0..], 2, 2, c_s);
    expect_true(crep.valid);
    expect_true(approxEq(c[0], c[1], 0.0001));

    let mf = cap.vector.MultiQueryFusion.init(cap.vector.QueryFusionConfig{});
    const h: [1]cap.vector.QueryViewHit = [cap.vector.QueryViewHit{ doc_id: 7, score: 0.9 }];
    expect_true(mf.addView(1.0, h[0..]));
    let out: [2]cap.vector.FusedDocHit = [cap.vector.FusedDocHit{}; 2];
    let out_s: []cap.vector.FusedDocHit = out[0..];
    expect_eq(mf.fuse(out_s).output_count, 1 as usize);
    expect_eq(out[0].doc_id, 7 as u64);
}
