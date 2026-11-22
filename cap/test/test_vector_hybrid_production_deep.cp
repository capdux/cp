// Origin: Pure-CP Stdlib Test Suite (RAG Hybrid Retrieval: RRF, MMR & Int8 Scalar Quantization) | Created: 2026-11-22
// Deep Production Verification Suite for lib/vector/hybrid.cp zero-heap primitives
// Covers:
//   1. 3-way RRF fusion (BM25 + Dense + Sparse), co-occurrence promotion, intra-list dedup & deterministic tie-breaking
//   2. Min-Max / Z-Score normalization & linear weighted score fusion (alpha * dense + (1 - alpha) * sparse)
//   3. MMR diversity reranking on redundant topic clusters (lambda = 0.5 diversification vs lambda = 1.0 pure relevance)
//   4. 128D U8 asymmetric scalar quantization (< 1% reconstruction & dot-product error, exact integer-accumulated match)
//   5. Boundary & malformed input defenses (empty inputs, illegal lambda/alpha/k, buffer underflow, NaN/Inf)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    RankedDoc,
    ScalarQuantParams,
    rrfFuse,
    normalizeScoresMinMax,
    normalizeScoresZScore,
    weightedScoreFuse,
    mmrRerank,
    quantizeVectorU8,
    dequantizeVectorU8,
    quantizedDotU8,
} from "../../lib/vector/index.cp";

const EPS_F64: f64 = 0.000000000001;
const EPS_DOT: f64 = 0.000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testRrfThreeWayFusionAndDeterminism()! {
    // 3 recall lists:
    //   List 0 (BM25):   [100, 42, 200, 42]  (doc 42 duplicated -> only rank 2 counted)
    //   List 1 (Dense):  [300, 42, 100]
    //   List 2 (Sparse): [42, 400, 300]
    // With k = 60.0:
    //   doc 42:  1/62 + 1/62 + 1/61 = 0.0486515...  (promoted to #1 by 3-way co-occurrence)
    //   doc 100: 1/61 + 1/63        = 0.0322664...  (#2)
    //   doc 300: 1/61 + 1/63        = 0.0322664...  (#3, tied with 100 -> doc_id 100 < 300 wins!)
    //   doc 400: 1/62               = 0.0161290...  (#4)
    //   doc 200: 1/63               = 0.0158730...  (#5)
    const lists_ids: [10]u32 = [100, 42, 200, 42, 300, 42, 100, 42, 400, 300];
    const offsets: [4]usize = [0, 4, 7, 10];

    let out_buf: [8]RankedDoc = [RankedDoc{}; 8];
    let work_ids_buf: [8]u32 = [0; 8];
    let work_scores_buf: [8]f64 = [0.0; 8];

    let out_slice: []RankedDoc = out_buf[0..];
    let w_ids: []u32 = work_ids_buf[0..];
    let w_scores: []f64 = work_scores_buf[0..];

    const count_opt = rrfFuse(lists_ids[0..], offsets[0..], 60.0, out_slice, w_ids, w_scores);
    expect_eq(count_opt != null, true);
    const count = count_opt ?? 0;
    expect_eq(count, 5);

    // #1: doc 42
    expect_eq(out_buf[0].doc_id, 42);
    const expected_42 = (1.0 / 62.0) + (1.0 / 62.0) + (1.0 / 61.0);
    expect_eq(approxF64(out_buf[0].score, expected_42, EPS_F64), true);

    // #2 and #3: doc 100 and doc 300 have identical score, deterministic tie-break by doc_id asc
    expect_eq(out_buf[1].doc_id, 100);
    expect_eq(out_buf[2].doc_id, 300);
    const expected_100 = (1.0 / 61.0) + (1.0 / 63.0);
    expect_eq(approxF64(out_buf[1].score, expected_100, EPS_F64), true);
    expect_eq(approxF64(out_buf[2].score, expected_100, EPS_F64), true);

    // #4: doc 400, #5: doc 200
    expect_eq(out_buf[3].doc_id, 400);
    expect_eq(approxF64(out_buf[3].score, 1.0 / 62.0, EPS_F64), true);
    expect_eq(out_buf[4].doc_id, 200);
    expect_eq(approxF64(out_buf[4].score, 1.0 / 63.0, EPS_F64), true);
}

fn testNormalizationAndWeightedScoreFuse()! {
    // 1. Min-Max normalization (including negative inputs)
    const raw: [4]f64 = [-2.0, 0.0, 3.0, 8.0];
    let norm_buf: [4]f64 = [0.0; 4];
    let norm_slice: []f64 = norm_buf[0..];
    expect_eq(normalizeScoresMinMax(raw[0..], norm_slice), true);
    expect_eq(approxF64(norm_buf[0], 0.0, EPS_F64), true);
    expect_eq(approxF64(norm_buf[1], 0.2, EPS_F64), true);
    expect_eq(approxF64(norm_buf[2], 0.5, EPS_F64), true);
    expect_eq(approxF64(norm_buf[3], 1.0, EPS_F64), true);

    // Constant vector -> all 0.0
    const const_scores: [3]f64 = [7.5, 7.5, 7.5];
    let const_out_buf: [3]f64 = [9.0; 3];
    let const_out: []f64 = const_out_buf[0..];
    expect_eq(normalizeScoresMinMax(const_scores[0..], const_out), true);
    expect_eq(approxF64(const_out_buf[0], 0.0, EPS_F64), true);
    expect_eq(approxF64(const_out_buf[2], 0.0, EPS_F64), true);

    // 2. Z-Score standardization
    const z_in: [4]f64 = [2.0, 4.0, 4.0, 6.0];
    let z_buf: [4]f64 = [0.0; 4];
    let z_slice: []f64 = z_buf[0..];
    expect_eq(normalizeScoresZScore(z_in[0..], z_slice), true);
    // mean = 4.0, variance = (4 + 0 + 0 + 4) / 4 = 2.0, std = sqrt(2)
    const inv_sqrt2 = 1.0 / std.math.sqrt(2.0);
    expect_eq(approxF64(z_buf[0], -2.0 * inv_sqrt2, EPS_F64), true);
    expect_eq(approxF64(z_buf[1], 0.0, EPS_F64), true);
    expect_eq(approxF64(z_buf[3], 2.0 * inv_sqrt2, EPS_F64), true);

    // 3. Weighted score fusion (alpha = 0.7 dense-heavy vs alpha = 0.2 sparse-heavy)
    const dense_docs: [3]RankedDoc = [
        RankedDoc{ doc_id: 10, score: 1.0 },
        RankedDoc{ doc_id: 20, score: 0.6 },
        RankedDoc{ doc_id: 30, score: 0.2 },
    ];
    const sparse_docs: [3]RankedDoc = [
        RankedDoc{ doc_id: 20, score: 0.9 },
        RankedDoc{ doc_id: 40, score: 1.0 },
        RankedDoc{ doc_id: 10, score: 0.1 },
    ];

    let fused_buf: [8]RankedDoc = [RankedDoc{}; 8];
    let fused_slice: []RankedDoc = fused_buf[0..];

    // alpha = 0.7:
    //   doc 10: 0.7*1.0 + 0.3*0.1 = 0.73
    //   doc 20: 0.7*0.6 + 0.3*0.9 = 0.69
    //   doc 40: 0.7*0.0 + 0.3*1.0 = 0.30
    //   doc 30: 0.7*0.2 + 0.3*0.0 = 0.14
    const n1 = weightedScoreFuse(dense_docs[0..], sparse_docs[0..], 0.7, fused_slice) ?? 0;
    expect_eq(n1, 4);
    expect_eq(fused_buf[0].doc_id, 10);
    expect_eq(approxF64(fused_buf[0].score, 0.73, EPS_F64), true);
    expect_eq(fused_buf[1].doc_id, 20);
    expect_eq(approxF64(fused_buf[1].score, 0.69, EPS_F64), true);
    expect_eq(fused_buf[2].doc_id, 40);
    expect_eq(approxF64(fused_buf[2].score, 0.30, EPS_F64), true);
    expect_eq(fused_buf[3].doc_id, 30);
    expect_eq(approxF64(fused_buf[3].score, 0.14, EPS_F64), true);

    // alpha = 0.2 (sparse-heavy):
    //   doc 20: 0.2*0.6 + 0.8*0.9 = 0.84 (#1)
    //   doc 40: 0.2*0.0 + 0.8*1.0 = 0.80 (#2)
    //   doc 10: 0.2*1.0 + 0.8*0.1 = 0.28 (#3)
    //   doc 30: 0.2*0.2 + 0.8*0.0 = 0.04 (#4)
    const n2 = weightedScoreFuse(dense_docs[0..], sparse_docs[0..], 0.2, fused_slice) ?? 0;
    expect_eq(n2, 4);
    expect_eq(fused_buf[0].doc_id, 20);
    expect_eq(approxF64(fused_buf[0].score, 0.84, EPS_F64), true);
    expect_eq(fused_buf[1].doc_id, 40);
    expect_eq(approxF64(fused_buf[1].score, 0.80, EPS_F64), true);
    expect_eq(fused_buf[2].doc_id, 10);
    expect_eq(fused_buf[3].doc_id, 30);
}

fn testMmrDiversityVsPureRelevance()! {
    // 6 candidate documents in 3D topic space:
    //   Docs 0, 1, 2: Topic A (near-identical along axis 0), query_sims = 0.95, 0.94, 0.93
    //   Docs 3, 4:    Topic B (along axis 1, orthogonal to A), query_sims = 0.85, 0.84
    //   Doc  5:       Topic C (along axis 2, orthogonal to A & B), query_sim = 0.80
    const query_sims: [6]f64 = [0.95, 0.94, 0.93, 0.85, 0.84, 0.80];
    const embeddings: [18]f64 = [
        1.0, 0.0, 0.0,    // doc 0 (Topic A)
        0.999, 0.01, 0.0, // doc 1 (Topic A duplicate)
        0.998, 0.0, 0.01, // doc 2 (Topic A duplicate)
        0.0, 1.0, 0.0,    // doc 3 (Topic B)
        0.01, 0.999, 0.0, // doc 4 (Topic B duplicate)
        0.0, 0.0, 1.0,    // doc 5 (Topic C)
    ];

    let out_idx_buf: [6]usize = [99; 6];
    let mask_buf: [6]u8 = [1; 6]; // intentionally dirty to verify internal zeroing
    let out_idx: []usize = out_idx_buf[0..];
    let mask: []u8 = mask_buf[0..];

    // 1. lambda = 0.5 -> balances relevance & diversity, picking 1 doc per topic: [0, 3, 5]
    const k_div = mmrRerank(query_sims[0..], embeddings[0..], 6, 3, 3, 0.5, out_idx, mask) ?? 0;
    expect_eq(k_div, 3);
    expect_eq(out_idx_buf[0], 0);
    expect_eq(out_idx_buf[1], 3);
    expect_eq(out_idx_buf[2], 5);

    // 2. lambda = 1.0 -> degenerates to pure relevance ranking: [0, 1, 2]
    const k_rel = mmrRerank(query_sims[0..], embeddings[0..], 6, 3, 3, 1.0, out_idx, mask) ?? 0;
    expect_eq(k_rel, 3);
    expect_eq(out_idx_buf[0], 0);
    expect_eq(out_idx_buf[1], 1);
    expect_eq(out_idx_buf[2], 2);

    // 3. top_k > n_docs -> clamps to n_docs (6)
    const k_all = mmrRerank(query_sims[0..], embeddings[0..], 6, 3, 20, 0.5, out_idx, mask) ?? 0;
    expect_eq(k_all, 6);
}

fn testScalarQuantization128DAccuracy()! {
    // Construct two realistic 128D embeddings with mixed signs and smooth structure
    let vec_a: [128]f64 = [0.0; 128];
    let vec_b: [128]f64 = [0.0; 128];
    let exact_dot: f64 = 0.0;
    let norm_a_sq: f64 = 0.0;

    let i: usize = 0;
    while (i < 128) : (i += 1) {
        const fi = i as f64;
        vec_a[i] = std.math.sin(fi * 0.17) * 1.5 + std.math.cos(fi * 0.07) * 0.5;
        vec_b[i] = std.math.sin(fi * 0.17 + 0.25) * 1.2 + 0.3 * std.math.cos(fi * 0.11);
        exact_dot += vec_a[i] * vec_b[i];
        norm_a_sq += vec_a[i] * vec_a[i];
    }

    let qa_buf: [128]u8 = [0; 128];
    let qb_buf: [128]u8 = [0; 128];
    let qa: []u8 = qa_buf[0..];
    let qb: []u8 = qb_buf[0..];

    const pa_opt = quantizeVectorU8(vec_a[0..], qa);
    const pb_opt = quantizeVectorU8(vec_b[0..], qb);
    expect_eq(pa_opt != null, true);
    expect_eq(pb_opt != null, true);
    const pa = pa_opt ?? ScalarQuantParams{};
    const pb = pb_opt ?? ScalarQuantParams{};

    // Dequantize vec_a and verify L2 relative reconstruction error < 1%
    let recon_a_buf: [128]f64 = [0.0; 128];
    let recon_b_buf: [128]f64 = [0.0; 128];
    let recon_a: []f64 = recon_a_buf[0..];
    let recon_b: []f64 = recon_b_buf[0..];

    expect_eq(dequantizeVectorU8(qa_buf[0..], pa, recon_a), true);
    expect_eq(dequantizeVectorU8(qb_buf[0..], pb, recon_b), true);

    let err_sq: f64 = 0.0;
    let dequant_dot: f64 = 0.0;
    let j: usize = 0;
    while (j < 128) : (j += 1) {
        const diff = vec_a[j] - recon_a_buf[j];
        err_sq += diff * diff;
        dequant_dot += recon_a_buf[j] * recon_b_buf[j];
    }
    const rel_recon_err = std.math.sqrt(err_sq) / std.math.sqrt(norm_a_sq);
    expect_eq(rel_recon_err < 0.01, true);

    // Verify fast integer-accumulated quantizedDotU8 matches dequantized dot to ~1e-9
    // and approximates exact float dot within < 1%
    const q_dot_opt = quantizedDotU8(qa_buf[0..], pa, qb_buf[0..], pb);
    expect_eq(q_dot_opt != null, true);
    const q_dot = q_dot_opt ?? 0.0;

    expect_eq(approxF64(q_dot, dequant_dot, EPS_DOT), true);
    const rel_dot_err = @abs(q_dot - exact_dot) / @abs(exact_dot);
    expect_eq(rel_dot_err < 0.01, true);

    // Constant vector quantization round-trips with zero error
    const c_vec: [4]f64 = [-3.25, -3.25, -3.25, -3.25];
    let qc_buf: [4]u8 = [99; 4];
    let qc: []u8 = qc_buf[0..];
    const pc_opt = quantizeVectorU8(c_vec[0..], qc);
    expect_eq(pc_opt != null, true);
    const pc = pc_opt ?? ScalarQuantParams{};
    expect_eq(pc.scale == 0.0, true);
    let rc_buf: [4]f64 = [0.0; 4];
    let rc: []f64 = rc_buf[0..];
    expect_eq(dequantizeVectorU8(qc_buf[0..], pc, rc), true);
    expect_eq(approxF64(rc_buf[0], -3.25, EPS_F64), true);
    expect_eq(approxF64(rc_buf[3], -3.25, EPS_F64), true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_v = std.math.nan(f64);
    const inf_v = std.math.inf(f64);

    // 1. rrfFuse defenses: empty, negative/NaN k_const, invalid offsets, small workspace/output
    const two_ids: [2]u32 = [1, 2];
    const ok_offs: [2]usize = [0, 2];
    const empty_ids: []const u32 = two_ids[0..0];
    const empty_offs: []const usize = ok_offs[0..0];
    const zero_off: [1]usize = [0];

    let out_small_buf: [1]RankedDoc = [RankedDoc{}; 1];
    let w_ids_small_buf: [1]u32 = [0; 1];
    let w_sc_small_buf: [1]f64 = [0.0; 1];
    let out_small: []RankedDoc = out_small_buf[0..];
    let w_ids_small: []u32 = w_ids_small_buf[0..];
    let w_sc_small: []f64 = w_sc_small_buf[0..];

    expect_eq(rrfFuse(empty_ids, empty_offs, 60.0, out_small, w_ids_small, w_sc_small), 0);
    expect_eq(rrfFuse(empty_ids, zero_off[0..], 60.0, out_small, w_ids_small, w_sc_small), 0);
    expect_eq(rrfFuse(empty_ids, empty_offs, -1.0, out_small, w_ids_small, w_sc_small) == null, true);
    expect_eq(rrfFuse(empty_ids, empty_offs, nan_v, out_small, w_ids_small, w_sc_small) == null, true);
    expect_eq(rrfFuse(empty_ids, empty_offs, inf_v, out_small, w_ids_small, w_sc_small) == null, true);

    const bad_offs_order: [3]usize = [0, 2, 1];
    const bad_offs_oob: [2]usize = [0, 5];
    expect_eq(rrfFuse(two_ids[0..], bad_offs_order[0..], 60.0, out_small, w_ids_small, w_sc_small) == null, true);
    expect_eq(rrfFuse(two_ids[0..], bad_offs_oob[0..], 60.0, out_small, w_ids_small, w_sc_small) == null, true);
    // Workspace has len 1, but 2 unique docs -> null
    expect_eq(rrfFuse(two_ids[0..], ok_offs[0..], 60.0, out_small, w_ids_small, w_sc_small) == null, true);

    // 2. normalizeScoresMinMax / ZScore / weightedScoreFuse defenses
    const nan_scores: [2]f64 = [1.0, nan_v];
    const ok_scores: [2]f64 = [1.0, 2.0];
    const empty_f64: []const f64 = ok_scores[0..0];
    let f64_out1_buf: [1]f64 = [0.0; 1];
    let f64_out2_buf: [2]f64 = [0.0; 2];
    let f64_out1: []f64 = f64_out1_buf[0..];
    let f64_out2: []f64 = f64_out2_buf[0..];
    expect_eq(normalizeScoresMinMax(nan_scores[0..], f64_out2), false);
    expect_eq(normalizeScoresZScore(nan_scores[0..], f64_out2), false);
    expect_eq(normalizeScoresMinMax(ok_scores[0..], f64_out1), false); // buffer too small
    expect_eq(normalizeScoresMinMax(empty_f64, f64_out1), true);

    const bad_docs: [1]RankedDoc = [RankedDoc{ doc_id: 1, score: nan_v }];
    const ok_docs: [2]RankedDoc = [
        RankedDoc{ doc_id: 1, score: 0.5 },
        RankedDoc{ doc_id: 2, score: 0.3 },
    ];
    expect_eq(weightedScoreFuse(ok_docs[0..], ok_docs[0..], -0.1, out_small) == null, true);
    expect_eq(weightedScoreFuse(ok_docs[0..], ok_docs[0..], 1.5, out_small) == null, true);
    expect_eq(weightedScoreFuse(ok_docs[0..], ok_docs[0..], nan_v, out_small) == null, true);
    expect_eq(weightedScoreFuse(bad_docs[0..], ok_docs[0..], 0.5, out_small) == null, true);
    expect_eq(weightedScoreFuse(ok_docs[0..], ok_docs[0..], 0.5, out_small) == null, true); // out_small len 1 < 2

    // 3. mmrRerank defenses: illegal lambda, NaN, dim=0, buffer underflow, empty
    const sims2: [2]f64 = [0.9, 0.8];
    const sims_nan: [2]f64 = [0.9, nan_v];
    const emb2: [4]f64 = [1.0, 0.0, 0.0, 1.0];
    const emb_nan: [4]f64 = [1.0, inf_v, 0.0, 1.0];
    let idx_buf: [2]usize = [0; 2];
    let idx_small_buf: [1]usize = [0; 1];
    let mask_buf: [2]u8 = [0; 2];
    let mask_small_buf: [1]u8 = [0; 1];
    let idx_s: []usize = idx_buf[0..];
    let idx_small: []usize = idx_small_buf[0..];
    let mask_s: []u8 = mask_buf[0..];
    let mask_small: []u8 = mask_small_buf[0..];

    expect_eq(mmrRerank(sims2[0..], emb2[0..], 0, 2, 2, 0.5, idx_s, mask_s), 0);
    expect_eq(mmrRerank(sims2[0..], emb2[0..], 2, 2, 0, 0.5, idx_s, mask_s), 0);
    expect_eq(mmrRerank(sims2[0..], emb2[0..], 2, 0, 2, 0.5, idx_s, mask_s) == null, true);
    expect_eq(mmrRerank(sims2[0..], emb2[0..], 2, 2, 2, -0.01, idx_s, mask_s) == null, true);
    expect_eq(mmrRerank(sims2[0..], emb2[0..], 2, 2, 2, 1.01, idx_s, mask_s) == null, true);
    expect_eq(mmrRerank(sims2[0..], emb2[0..], 2, 2, 2, nan_v, idx_s, mask_s) == null, true);
    expect_eq(mmrRerank(sims_nan[0..], emb2[0..], 2, 2, 2, 0.5, idx_s, mask_s) == null, true);
    expect_eq(mmrRerank(sims2[0..], emb_nan[0..], 2, 2, 2, 0.5, idx_s, mask_s) == null, true);
    expect_eq(mmrRerank(sims2[0..], emb2[0..], 2, 2, 2, 0.5, idx_small, mask_s) == null, true);
    expect_eq(mmrRerank(sims2[0..], emb2[0..], 2, 2, 2, 0.5, idx_s, mask_small) == null, true);

    // 4. Scalar quantization defenses: empty, NaN, buffer too small, length mismatch, bad params
    let q_small_buf: [1]u8 = [0; 1];
    let q2_buf: [2]u8 = [0; 2];
    let q_small: []u8 = q_small_buf[0..];
    let q2: []u8 = q2_buf[0..];
    expect_eq(quantizeVectorU8(empty_f64, q2) == null, true);
    expect_eq(quantizeVectorU8(ok_scores[0..], q_small) == null, true);
    expect_eq(quantizeVectorU8(nan_scores[0..], q2) == null, true);

    const good_p = ScalarQuantParams{ min_val: 0.0, scale: 0.1, inv_scale: 10.0 };
    const bad_p = ScalarQuantParams{ min_val: 0.0, scale: -1.0, inv_scale: -1.0 };
    expect_eq(dequantizeVectorU8(q2_buf[0..], good_p, f64_out1), false);
    expect_eq(dequantizeVectorU8(q2_buf[0..], bad_p, f64_out2), false);
    expect_eq(quantizedDotU8(q2_buf[0..], good_p, q_small_buf[0..], good_p) == null, true);
    expect_eq(quantizedDotU8(q2_buf[0..], bad_p, q2_buf[0..], good_p) == null, true);
}

fn main() {
    test("1. vector hybrid: 3-way RRF fusion, co-occurrence promotion & tie-breaking", () => {
        testRrfThreeWayFusionAndDeterminism()?;
    });
    test("2. vector hybrid: Min-Max / Z-Score normalization & linear weighted fusion", () => {
        testNormalizationAndWeightedScoreFuse()?;
    });
    test("3. vector hybrid: MMR diversity reranking (lambda=0.5 vs lambda=1.0)", () => {
        testMmrDiversityVsPureRelevance()?;
    });
    test("4. vector hybrid: 128D U8 scalar quantization (<1% recon & dot error)", () => {
        testScalarQuantization128DAccuracy()?;
    });
    test("5. vector hybrid: empty/NaN/illegal-param/buffer-underflow boundary defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
