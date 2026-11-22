// Origin: Pure-CP Stdlib Test Suite (lib/vector/colbert_index.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc ColBERT Multi-Vector MaxSim Retrieval Index:
//   1. Exact multi-vector MaxSim scoring vs single-vector mean pooling & negative dot handling
//   2. Exact Top-K descending sort with deterministic doc_id ascending tie-breaking & k truncation
//   3. L2 normalization on insert & cosine equivalence vs raw dot mode (+ zero-norm token safety)
//   4. Logical document removal (remove_document), search exclusion, stats sync & re-insert
//   5. Low-norm noise/stopword token pruning (prune_low_norm_tokens) including >64D SIMD chunking
//   6. Comprehensive boundary guards (empty query, pool full, duplicate ID, dim mismatch, NaN/Inf)
//   7. End-to-end RAG pipeline combining pruning, L2 normalization & Top-K late-interaction search

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    COLBERT_MAX_DOCS,
    ColbertConfig,
    ColbertDocSpan,
    ColbertSearchHit,
    ColbertIndexStats,
    ColbertLiteIndex,
    prune_low_norm_tokens,
} from "cap:vector";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: Multi-Vector MaxSim Exact Scoring vs Single-Vector Mean Pooling
// ============================================================================
test("colbert_index: multi-vector MaxSim exact scoring vs single-vector baseline", () => {
    let pool: [128]f32 = [0.0; 128];
    let pool_s: []f32 = pool[0..];
    const cfg_opt = ColbertConfig.init(4, false);
    expect_eq(cfg_opt != null, true);
    const cfg = cfg_opt ?? return;
    const idx_opt = ColbertLiteIndex.init(cfg, pool_s);
    expect_eq(idx_opt != null, true);
    let idx = idx_opt ?? return;

    // Query has 2 orthogonal semantic concepts: q0 = e0, q1 = e1
    const query: [8]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
    ];

    // Doc 10 (multi-token): contains both concepts explicitly -> MaxSim = 1.0 + 1.0 = 2.0
    const doc_multi: [12]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
    ];
    // Doc 20 (single mean-pooled vector): average of e0 and e1 = [0.5, 0.5, 0, 0]
    // MaxSim = 0.5 + 0.5 = 1.0 (illustrating late-interaction fine-grained advantage)
    const doc_mean: [4]f32 = [0.5, 0.5, 0.0, 0.0];

    // Doc 30 (anti-correlated): negative dot products -> MaxSim must take max(-0.5, -2.0) = -0.5, not clamp to 0
    const doc_neg: [8]f32 = [
        -0.5, -0.25, 0.0, 0.0,
        -2.0, -1.0, 0.0, 0.0,
    ];

    expect_eq(idx.add_document(10, doc_multi[0..], 3), true);
    expect_eq(idx.add_document(20, doc_mean[0..], 1), true);
    expect_eq(idx.add_document(30, doc_neg[0..], 2), true);

    const s_multi = idx.score_document_maxsim(query[0..], 2, 0);
    const s_mean = idx.score_document_maxsim(query[0..], 2, 1);
    const s_neg = idx.score_document_maxsim(query[0..], 2, 2);

    expect_eq(approxEqF32(s_multi, 2.0, 0.0001), true);
    expect_eq(approxEqF32(s_mean, 1.0, 0.0001), true);
    expect_eq(s_multi > s_mean, true);
    // q0 max(-0.5, -2.0) = -0.5; q1 max(-0.25, -1.0) = -0.25 -> total = -0.75
    expect_eq(approxEqF32(s_neg, -0.75, 0.0001), true);

    let hits: [3]ColbertSearchHit = [ColbertSearchHit{}; 3];
    let hits_s: []ColbertSearchHit = hits[0..];
    const n = idx.search_topk(query[0..], 2, 3, hits_s);
    expect_eq(n, 3);
    expect_eq(hits[0].doc_id, 10);
    expect_eq(approxEqF32(hits[0].avg_maxsim_f32, 1.0, 0.0001), true);
    expect_eq(hits[0].matched_tokens, 2);

    expect_eq(hits[1].doc_id, 20);
    expect_eq(approxEqF32(hits[1].avg_maxsim_f32, 0.5, 0.0001), true);
    expect_eq(hits[1].matched_tokens, 2);

    expect_eq(hits[2].doc_id, 30);
    expect_eq(approxEqF32(hits[2].maxsim_score_f32, -0.75, 0.0001), true);
    expect_eq(hits[2].matched_tokens, 0);
});

// ============================================================================
// Test 2: Top-K Descending Sort & Deterministic doc_id Ascending Tie-Breaking
// ============================================================================
test("colbert_index: Top-K sort order, deterministic tie-breaking and truncation", () => {
    let pool: [128]f32 = [0.0; 128];
    let pool_s: []f32 = pool[0..];
    const cfg_opt = ColbertConfig.init(4, false);
    expect_eq(cfg_opt != null, true);
    const cfg = cfg_opt ?? return;
    const idx_opt = ColbertLiteIndex.init(cfg, pool_s);
    expect_eq(idx_opt != null, true);
    let idx = idx_opt ?? return;

    const q: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    // Insert documents out of doc_id order with intentional score ties
    const d_low: [4]f32 = [0.25, 0.0, 0.0, 0.0];
    const d_tie_a: [4]f32 = [0.75, 0.0, 0.0, 0.0];
    const d_tie_b: [4]f32 = [0.75, 0.0, 0.0, 0.0];
    const d_tie_c: [4]f32 = [0.75, 0.0, 0.0, 0.0];
    const d_high: [4]f32 = [1.5, 0.0, 0.0, 0.0];

    expect_eq(idx.add_document(500, d_tie_a[0..], 1), true);
    expect_eq(idx.add_document(100, d_low[0..], 1), true);
    expect_eq(idx.add_document(300, d_tie_b[0..], 1), true);
    expect_eq(idx.add_document(900, d_high[0..], 1), true);
    expect_eq(idx.add_document(200, d_tie_c[0..], 1), true);

    // Full Top-5 check
    let hits5: [5]ColbertSearchHit = [ColbertSearchHit{}; 5];
    let hits5_s: []ColbertSearchHit = hits5[0..];
    const n5 = idx.search_topk(q[0..], 1, 5, hits5_s);
    expect_eq(n5, 5);
    expect_eq(hits5[0].doc_id, 900);
    // Tie group (score 0.75): strictly ascending doc_id -> 200, 300, 500
    expect_eq(hits5[1].doc_id, 200);
    expect_eq(hits5[2].doc_id, 300);
    expect_eq(hits5[3].doc_id, 500);
    expect_eq(hits5[4].doc_id, 100);

    // Truncated Top-3 check (k < active_docs): keeps 900, 200, 300 (drops 500 on tie-break)
    let hits3: [3]ColbertSearchHit = [ColbertSearchHit{}; 3];
    let hits3_s: []ColbertSearchHit = hits3[0..];
    const n3 = idx.search_topk(q[0..], 1, 3, hits3_s);
    expect_eq(n3, 3);
    expect_eq(hits3[0].doc_id, 900);
    expect_eq(hits3[1].doc_id, 200);
    expect_eq(hits3[2].doc_id, 300);

    // Output buffer smaller than requested k -> safely bounded by out_hits.len
    let hits2: [2]ColbertSearchHit = [ColbertSearchHit{}; 2];
    let hits2_s: []ColbertSearchHit = hits2[0..];
    const n2 = idx.search_topk(q[0..], 1, 10, hits2_s);
    expect_eq(n2, 2);
    expect_eq(hits2[0].doc_id, 900);
    expect_eq(hits2[1].doc_id, 200);
});

// ============================================================================
// Test 3: L2 Normalization on Insert & Cosine Equivalence (+ Zero Token Safety)
// ============================================================================
test("colbert_index: L2 normalization on insert achieves cosine equivalence", () => {
    let pool_norm: [128]f32 = [0.0; 128];
    let pool_norm_s: []f32 = pool_norm[0..];
    const cfg_norm_opt = ColbertConfig.init(4, true);
    expect_eq(cfg_norm_opt != null, true);
    const cfg_norm = cfg_norm_opt ?? return;
    const idx_norm_opt = ColbertLiteIndex.init(cfg_norm, pool_norm_s);
    expect_eq(idx_norm_opt != null, true);
    let idx_norm = idx_norm_opt ?? return;

    // Unnormalized tokens with large differing magnitudes:
    // Token 0: [3.0, 4.0, 0.0, 0.0] -> norm = 5.0 -> unit = [0.6, 0.8, 0.0, 0.0]
    // Token 1: [0.0, 0.0, 0.0, 0.0] -> zero padding token -> safely stays [0, 0, 0, 0]
    const doc_a: [8]f32 = [
        3.0, 4.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
    ];
    // Doc B has smaller magnitude [0.8, 0.6, 0.0, 0.0] -> unit = [0.8, 0.6, 0.0, 0.0]
    const doc_b: [4]f32 = [0.8, 0.6, 0.0, 0.0];

    expect_eq(idx_norm.add_document(1, doc_a[0..], 2), true);
    expect_eq(idx_norm.add_document(2, doc_b[0..], 1), true);

    // Unit query along e0: cosine(q, doc_a_t0) = 0.6, cosine(q, doc_b_t0) = 0.8
    const q_unit: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const score_a = idx_norm.score_document_maxsim(q_unit[0..], 1, 0);
    const score_b = idx_norm.score_document_maxsim(q_unit[0..], 1, 1);

    expect_eq(approxEqF32(score_a, 0.6, 0.0001), true);
    expect_eq(approxEqF32(score_b, 0.8, 0.0001), true);

    // Under L2 normalization, doc 2 (cos 0.8) outranks doc 1 (cos 0.6) despite smaller raw magnitude
    let hits: [2]ColbertSearchHit = [ColbertSearchHit{}; 2];
    let hits_s: []ColbertSearchHit = hits[0..];
    expect_eq(idx_norm.search_topk(q_unit[0..], 1, 2, hits_s), 2);
    expect_eq(hits[0].doc_id, 2);
    expect_eq(hits[1].doc_id, 1);

    // Contrast with raw dot mode (l2_normalize_on_insert = false): doc 1 (dot = 3.0) > doc 2 (dot = 0.8)
    let pool_raw: [128]f32 = [0.0; 128];
    let pool_raw_s: []f32 = pool_raw[0..];
    const cfg_raw_opt = ColbertConfig.init(4, false);
    expect_eq(cfg_raw_opt != null, true);
    const cfg_raw = cfg_raw_opt ?? return;
    const idx_raw_opt = ColbertLiteIndex.init(cfg_raw, pool_raw_s);
    expect_eq(idx_raw_opt != null, true);
    let idx_raw = idx_raw_opt ?? return;

    expect_eq(idx_raw.add_document(1, doc_a[0..], 2), true);
    expect_eq(idx_raw.add_document(2, doc_b[0..], 1), true);
    expect_eq(idx_raw.search_topk(q_unit[0..], 1, 2, hits_s), 2);
    expect_eq(hits[0].doc_id, 1);
    expect_eq(approxEqF32(hits[0].maxsim_score_f32, 3.0, 0.0001), true);
});

// ============================================================================
// Test 4: remove_document Logical Removal, Stats Sync & Re-Insertion
// ============================================================================
test("colbert_index: remove_document excludes removed docs and updates stats", () => {
    let pool: [128]f32 = [0.0; 128];
    let pool_s: []f32 = pool[0..];
    const cfg_opt = ColbertConfig.init(4, true);
    expect_eq(cfg_opt != null, true);
    const cfg = cfg_opt ?? return;
    const idx_opt = ColbertLiteIndex.init(cfg, pool_s);
    expect_eq(idx_opt != null, true);
    let idx = idx_opt ?? return;

    const d1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const d2: [4]f32 = [0.8, 0.6, 0.0, 0.0];
    const d3: [4]f32 = [0.6, 0.8, 0.0, 0.0];

    expect_eq(idx.add_document(11, d1[0..], 1), true);
    expect_eq(idx.add_document(22, d2[0..], 1), true);
    expect_eq(idx.add_document(33, d3[0..], 1), true);

    const st0 = idx.stats();
    expect_eq(st0.active_docs, 3);
    expect_eq(st0.total_stored_tokens, 3);
    expect_eq(st0.token_capacity, 32);
    expect_eq(st0.dim, 4);

    // Remove top-scoring doc 11
    expect_eq(idx.remove_document(11), true);
    // Double remove returns false; unknown ID returns false
    expect_eq(idx.remove_document(11), false);
    expect_eq(idx.remove_document(999), false);

    const st1 = idx.stats();
    expect_eq(st1.active_docs, 2);
    expect_eq(st1.total_stored_tokens, 3);

    // Direct slot scoring on removed doc returns 0.0
    const q: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    expect_eq(approxEqF32(idx.score_document_maxsim(q[0..], 1, 0), 0.0, 0.0001), true);

    let hits: [3]ColbertSearchHit = [ColbertSearchHit{}; 3];
    let hits_s: []ColbertSearchHit = hits[0..];
    const n = idx.search_topk(q[0..], 1, 3, hits_s);
    expect_eq(n, 2);
    expect_eq(hits[0].doc_id, 22);
    expect_eq(hits[1].doc_id, 33);

    // Re-inserting previously removed doc_id 11 with new content succeeds
    const d1_new: [4]f32 = [0.0, 0.0, 1.0, 0.0];
    expect_eq(idx.add_document(11, d1_new[0..], 1), true);
    expect_eq(idx.stats().active_docs, 3);
    expect_eq(idx.stats().total_stored_tokens, 4);
});

// ============================================================================
// Test 5: prune_low_norm_tokens Noise Pruning (+ High-Dim >64D SIMD Chunking)
// ============================================================================
test("colbert_index: prune_low_norm_tokens filters noise, NaN and supports >64D", () => {
    // 5 tokens of dim 4:
    // t0: norm = 1.0 (keep)
    // t1: norm = 0.05 (prune)
    // t2: norm = 0.0 (prune)
    // t3: norm = 5.0 (keep)
    // t4: contains NaN (prune)
    const nan_f32: f32 = std.math.nan(f32);
    const raw: [20]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.03, 0.04, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
        3.0, 4.0, 0.0, 0.0,
        nan_f32, 1.0, 0.0, 0.0,
    ];
    let out: [20]f32 = [0.0; 20];
    let out_s: []f32 = out[0..];

    const kept = prune_low_norm_tokens(raw[0..], 5, 4, 0.5, out_s);
    expect_eq(kept, 2);
    // Retained token 0 == t0
    expect_eq(approxEqF32(out[0], 1.0, 0.0001), true);
    expect_eq(approxEqF32(out[1], 0.0, 0.0001), true);
    // Retained token 1 == t3
    expect_eq(approxEqF32(out[4], 3.0, 0.0001), true);
    expect_eq(approxEqF32(out[5], 4.0, 0.0001), true);

    // Output buffer cap: space for only 1 token
    let out_small: [4]f32 = [0.0; 4];
    let out_small_s: []f32 = out_small[0..];
    expect_eq(prune_low_norm_tokens(raw[0..], 5, 4, 0.5, out_small_s), 1);

    // High-dimensional (72D > 64D SIMD chunk) pruning verification
    // 2 tokens x 72D: token 0 has norm^2 = 72 * 0.01 = 0.72 (norm ~0.8485 > 0.5 -> keep),
    // token 1 has norm^2 = 72 * 0.0001 = 0.0072 (norm ~0.0848 < 0.5 -> prune)
    let hi_in: [144]f32 = [0.0; 144];
    let i: usize = 0;
    while (i < 72) : (i += 1) {
        hi_in[i] = 0.1;
        hi_in[72 + i] = 0.01;
    }
    let hi_out: [144]f32 = [0.0; 144];
    let hi_out_s: []f32 = hi_out[0..];
    const hi_kept = ColbertLiteIndex.prune_low_norm_tokens(hi_in[0..], 2, 72, 0.5, hi_out_s);
    expect_eq(hi_kept, 1);
    expect_eq(approxEqF32(hi_out[0], 0.1, 0.0001), true);
    expect_eq(approxEqF32(hi_out[71], 0.1, 0.0001), true);

    // Invalid threshold / dim guards
    expect_eq(prune_low_norm_tokens(raw[0..], 5, 4, -0.1, out_s), 0);
    expect_eq(prune_low_norm_tokens(raw[0..], 5, 4, nan_f32, out_s), 0);
    expect_eq(prune_low_norm_tokens(raw[0..], 0, 4, 0.5, out_s), 0);
    expect_eq(prune_low_norm_tokens(raw[0..], 5, 0, 0.5, out_s), 0);
});

// ============================================================================
// Test 6: Boundary Guards (Invalid Config, Empty Query, Pool Full, Dup ID, Dim Mismatch)
// ============================================================================
test("colbert_index: comprehensive boundary and malformed input guards", () => {
    // 1. Invalid config (dim == 0)
    const bad_cfg = ColbertConfig{ dim: 0, l2_normalize_on_insert: true };
    expect_eq(bad_cfg.isValid(), false);
    expect_eq(ColbertConfig.init(0, true) == null, true);

    let tiny_pool: [2]f32 = [0.0; 2];
    let tiny_pool_s: []f32 = tiny_pool[0..];
    const cfg4_opt = ColbertConfig.init(4, true);
    expect_eq(cfg4_opt != null, true);
    const cfg4 = cfg4_opt ?? return;
    // Pool too small for even 1 token of dim 4 -> init returns null
    expect_eq(ColbertLiteIndex.init(cfg4, tiny_pool_s) == null, true);

    // 2. Pool with capacity for exactly 3 tokens of dim 4
    let pool: [12]f32 = [0.0; 12];
    let pool_s: []f32 = pool[0..];
    const idx_opt = ColbertLiteIndex.init(cfg4, pool_s);
    expect_eq(idx_opt != null, true);
    let idx = idx_opt ?? return;
    expect_eq(idx.stats().token_capacity, 3);

    const d_two: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0];
    const d_one: [4]f32 = [0.0, 0.0, 1.0, 0.0];

    // Empty doc rejected
    expect_eq(idx.add_document(1, d_one[0..0], 0), false);
    // Dimension mismatch (slice len != num_tokens * dim) rejected
    expect_eq(idx.add_document(1, d_two[0..6], 2), false);
    expect_eq(idx.add_document(1, d_one[0..], 2), false);

    // Non-finite doc rejected
    const nan_f32: f32 = std.math.nan(f32);
    const inf_f32: f32 = std.math.inf(f32);
    const d_nan: [4]f32 = [nan_f32, 0.0, 0.0, 0.0];
    const d_inf: [4]f32 = [inf_f32, 0.0, 0.0, 0.0];
    expect_eq(idx.add_document(1, d_nan[0..], 1), false);
    expect_eq(idx.add_document(1, d_inf[0..], 1), false);
    expect_eq(idx.stats().active_docs, 0);

    // Valid insert of 2 tokens (used = 2 / 3)
    expect_eq(idx.add_document(42, d_two[0..], 2), true);
    // Duplicate active ID rejected
    expect_eq(idx.add_document(42, d_one[0..], 1), false);
    // Pool overflow (needs 2 tokens, only 1 left) rejected
    expect_eq(idx.add_document(43, d_two[0..], 2), false);
    // Exact fit (needs 1 token, 1 left) succeeds
    expect_eq(idx.add_document(43, d_one[0..], 1), true);
    expect_eq(idx.stats().total_stored_tokens, 3);
    // Further insert fails cleanly
    expect_eq(idx.add_document(44, d_one[0..], 1), false);

    // Query guards: empty query, k == 0, dim mismatch, NaN query, out-of-range doc_idx
    let hits: [4]ColbertSearchHit = [ColbertSearchHit{}; 4];
    let hits_s: []ColbertSearchHit = hits[0..];
    expect_eq(idx.search_topk(d_one[0..0], 0, 2, hits_s), 0);
    expect_eq(idx.search_topk(d_one[0..], 1, 0, hits_s), 0);
    expect_eq(idx.search_topk(d_two[0..6], 2, 2, hits_s), 0);
    expect_eq(idx.search_topk(d_nan[0..], 1, 2, hits_s), 0);
    expect_eq(approxEqF32(idx.score_document_maxsim(d_one[0..], 1, 99), 0.0, 0.0001), true);
    expect_eq(approxEqF32(idx.score_document_maxsim(d_nan[0..], 1, 0), 0.0, 0.0001), true);
    expect_eq(COLBERT_MAX_DOCS, 128);
});

// ============================================================================
// Test 7: End-to-End Pruned Multi-Vector RAG Pipeline
// ============================================================================
test("colbert_index: end-to-end pruned multi-vector ColBERT RAG workflow", () => {
    let pool: [256]f32 = [0.0; 256];
    let pool_s: []f32 = pool[0..];
    const cfg_opt = ColbertConfig.init(8, true);
    expect_eq(cfg_opt != null, true);
    const cfg = cfg_opt ?? return;
    const idx_opt = ColbertLiteIndex.init(cfg, pool_s);
    expect_eq(idx_opt != null, true);
    let idx = idx_opt ?? return;

    // Raw document 701: 2 high-signal tokens + 2 stopword noise tokens (dim = 8)
    const raw_doc701: [32]f32 = [
        2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // signal 0
        0.02, 0.01, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // stopword noise
        0.0, 3.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // signal 1
        0.01, 0.0, 0.01, 0.0, 0.0, 0.0, 0.0, 0.0, // stopword noise
    ];
    let clean_doc: [32]f32 = [0.0; 32];
    let clean_doc_s: []f32 = clean_doc[0..];
    const kept_doc = prune_low_norm_tokens(raw_doc701[0..], 4, 8, 0.25, clean_doc_s);
    expect_eq(kept_doc, 2);
    expect_eq(idx.add_document(701, clean_doc[0 .. kept_doc * 8], kept_doc), true);

    // Document 702: only matches signal 0
    const doc702: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    expect_eq(idx.add_document(702, doc702[0..], 1), true);

    // Query with 1 noise token + 2 signal tokens
    const raw_q: [24]f32 = [
        0.01, 0.01, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // noise
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // signal 0
        0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,   // signal 1
    ];
    let clean_q: [24]f32 = [0.0; 24];
    let clean_q_s: []f32 = clean_q[0..];
    const kept_q = prune_low_norm_tokens(raw_q[0..], 3, 8, 0.25, clean_q_s);
    expect_eq(kept_q, 2);

    let hits: [2]ColbertSearchHit = [ColbertSearchHit{}; 2];
    let hits_s: []ColbertSearchHit = hits[0..];
    const n = idx.search_topk(clean_q[0 .. kept_q * 8], kept_q, 2, hits_s);
    expect_eq(n, 2);
    expect_eq(hits[0].doc_id, 701);
    expect_eq(approxEqF32(hits[0].maxsim_score_f32, 2.0, 0.0001), true);
    expect_eq(hits[0].matched_tokens, 2);
    expect_eq(hits[1].doc_id, 702);
    expect_eq(approxEqF32(hits[1].maxsim_score_f32, 1.0, 0.0001), true);
    expect_eq(hits[1].matched_tokens, 1);

    // Verify token pool savings (only 3 total tokens stored instead of 5)
    expect_eq(idx.stats().total_stored_tokens, 3);
});
