// Origin: Pure-CP Stdlib Test Suite (lib/vector/hybrid_rag.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Hybrid RAG pipeline:
//   1. Dual-channel weighted RRF fusion (weights, dual-hit boost, intra-list dedup, rank metadata)
//   2. M-way CSR weighted RRF fusion (Dense + BM25 + SPLADE 3-way fusion & top-K truncation)
//   3. SIMD @dot/@fma MMR diversity reranking (lambda = 0.5 dedup, lambda = 1.0 pure relevance, lambda = 0.0 pure diversity, zero-vector safety)
//   4. SIMD @l2sq near-duplicate pruning
//   5. Token budget pruning (greedy skip vs strict prefix truncation, min_score filter, zero-token chunks)
//   6. End-to-end Hybrid RAG pipeline + empty input / NaN / Inf / undersized buffer defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HybridRagHit,
    HybridRrfFusion,
    RagContextChunk,
    TokenBudgetResult,
    mmrRerankSimdF32,
    dedupNearDuplicatesL2F32,
    selectWithinTokenBudget,
} from "cap:vector";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

// ============================================================================
// Test 1: Dual-Channel Weighted RRF Fusion (Boost, Weights, Intra-List Dedup)
// ============================================================================

fn testDualWeightedRrfFusion()! {
    const rrf = cap.vector.HybridRrfFusion.init(0.7, 0.3, 60.0) ?? return error.InitFailed;

    // Dense: doc 10 (rank 1), doc 20 (rank 2), doc 10 (dup ignored), doc 30 (rank 4)
    // Sparse: doc 20 (rank 1), doc 40 (rank 2), doc 10 (rank 3)
    const dense_ids: [4]u32 = [10, 20, 10, 30];
    const sparse_ids: [3]u32 = [20, 40, 10];

    let work_buf: [8]HybridRagHit = [HybridRagHit{}; 8];
    let out_buf: [4]HybridRagHit = [HybridRagHit{}; 4];
    let work_s: []HybridRagHit = work_buf[0..];
    let out_s: []HybridRagHit = out_buf[0..];

    const n = rrf.fuseDual(dense_ids[0..], sparse_ids[0..], 4, out_s, work_s) ?? 0;
    expect_eq(n, 4);

    // Expected exact scores:
    // doc 10: 0.7 / 61 + 0.3 / 63 = 0.01147541 + 0.00476190 = 0.01623731
    // doc 20: 0.7 / 62 + 0.3 / 61 = 0.01129032 + 0.00491803 = 0.01620835
    // doc 30: 0.7 / 64 = 0.01093750
    // doc 40: 0.3 / 62 = 0.00483871
    // Ordering: 10 > 20 > 30 > 40
    expect_eq(out_buf[0].doc_id, 10);
    expect_eq(out_buf[0].dense_rank, 1);
    expect_eq(out_buf[0].sparse_rank, 3);
    const exp10: f32 = (0.7 as f32) / 61.0 + (0.3 as f32) / 63.0;
    expect_eq(absF32(out_buf[0].score - exp10) < 0.00001, true);

    expect_eq(out_buf[1].doc_id, 20);
    expect_eq(out_buf[1].dense_rank, 2);
    expect_eq(out_buf[1].sparse_rank, 1);
    const exp20: f32 = (0.7 as f32) / 62.0 + (0.3 as f32) / 61.0;
    expect_eq(absF32(out_buf[1].score - exp20) < 0.00001, true);

    expect_eq(out_buf[2].doc_id, 30);
    expect_eq(out_buf[2].dense_rank, 4);
    expect_eq(out_buf[2].sparse_rank, 0);

    expect_eq(out_buf[3].doc_id, 40);
    expect_eq(out_buf[3].dense_rank, 0);
    expect_eq(out_buf[3].sparse_rank, 2);

    // Deterministic tie-breaking: equal weights & symmetric ranks -> smaller doc_id wins.
    const eq_rrf = HybridRrfFusion.init(1.0, 1.0, 60.0) ?? return error.InitFailed;
    const d_tie: [2]u32 = [50, 25];
    const s_tie: [2]u32 = [25, 50];
    const n_tie = eq_rrf.fuseDual(d_tie[0..], s_tie[0..], 2, out_s, work_s) ?? 0;
    expect_eq(n_tie, 2);
    expect_eq(out_buf[0].doc_id, 25);
    expect_eq(out_buf[1].doc_id, 50);
}

// ============================================================================
// Test 2: M-Way CSR Weighted RRF Fusion (Dense + BM25 + SPLADE)
// ============================================================================

fn testMultiWayWeightedRrfFusion()! {
    const rrf = HybridRrfFusion.init(1.0, 1.0, 60.0) ?? return error.InitFailed;

    // 3 channels:
    // Channel 0 (Dense,  w=0.5): [100, 200, 300]
    // Channel 1 (BM25,   w=0.3): [300, 100, 400]
    // Channel 2 (SPLADE, w=0.2): [300, 400, 500]
    const lists_ids: [9]u32 = [100, 200, 300, 300, 100, 400, 300, 400, 500];
    const offsets: [4]usize = [0, 3, 6, 9];
    const weights: [3]f32 = [0.5, 0.3, 0.2];

    let work_buf: [8]HybridRagHit = [HybridRagHit{}; 8];
    let out_buf: [3]HybridRagHit = [HybridRagHit{}; 3];
    let work_s: []HybridRagHit = work_buf[0..];
    let out_s: []HybridRagHit = out_buf[0..];

    const n = rrf.fuseMultiWeighted(
        lists_ids[0..],
        offsets[0..],
        weights[0..],
        3,
        out_s,
        work_s,
    ) ?? 0;
    expect_eq(n, 3);

    // doc 300 appears in all 3 channels: 0.5/63 + 0.3/61 + 0.2/61 = 0.0161333
    // doc 100 appears in ch0 & ch1:     0.5/61 + 0.3/62          = 0.0130354
    // doc 400 appears in ch1 & ch2:     0.3/63 + 0.2/62          = 0.0079877
    // doc 200 appears in ch0 only:      0.5/62                   = 0.0080645
    // Top 3 must be: 300, 100, 200
    expect_eq(out_buf[0].doc_id, 300);
    expect_eq(out_buf[0].dense_rank, 3);
    expect_eq(out_buf[0].sparse_rank, 1);
    const exp300: f32 = (0.5 as f32) / 63.0 + (0.3 as f32) / 61.0 + (0.2 as f32) / 61.0;
    expect_eq(absF32(out_buf[0].score - exp300) < 0.00001, true);

    expect_eq(out_buf[1].doc_id, 100);
    expect_eq(out_buf[2].doc_id, 200);
}

// ============================================================================
// Test 3: SIMD @dot/@fma MMR Diversity Reranking (Dedup, Relevance, Diversity)
// ============================================================================

fn testMmrRerankSimdDiversity()! {
    // 5 candidates in 8D (exercises 8-lane SIMD @dot directly):
    // doc 0: topic A, rel = 0.96
    // doc 1: near-duplicate of doc 0, rel = 0.95
    // doc 2: topic B (orthogonal to A), rel = 0.86
    // doc 3: topic C (orthogonal to A & B), rel = 0.80
    // doc 4: zero vector (edge case), rel = 0.10
    const rels: [5]f32 = [0.96, 0.95, 0.86, 0.80, 0.10];
    const embs: [40]f32 = [
        1.0,  0.0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // doc 0
        0.99, 0.01, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // doc 1 (near-dup of doc 0)
        0.0,  0.0,  1.0, 0.0, 0.0, 0.0, 0.0, 0.0, // doc 2 (topic B)
        0.0,  0.0,  0.0, 1.0, 0.0, 0.0, 0.0, 0.0, // doc 3 (topic C)
        0.0,  0.0,  0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // doc 4 (zero vector)
    ];

    let idx_buf: [4]usize = [0; 4];
    let idx_s: []usize = idx_buf[0..];

    // Balanced MMR (lambda = 0.5): must skip near-duplicate doc 1 and pick [0, 2, 3].
    const k_bal = mmrRerankSimdF32(rels[0..], embs[0..], 5, 8, 3, 0.5, idx_s) ?? 0;
    expect_eq(k_bal, 3);
    expect_eq(idx_buf[0], 0);
    expect_eq(idx_buf[1], 2);
    expect_eq(idx_buf[2], 3);

    // Pure relevance (lambda = 1.0): must preserve strict relevance order [0, 1, 2, 3].
    const k_rel = mmrRerankSimdF32(rels[0..], embs[0..], 5, 8, 4, 1.0, idx_s) ?? 0;
    expect_eq(k_rel, 4);
    expect_eq(idx_buf[0], 0);
    expect_eq(idx_buf[1], 1);
    expect_eq(idx_buf[2], 2);
    expect_eq(idx_buf[3], 3);

    // Pure diversity (lambda = 0.0): picks highest relevance seed (0), then orthogonal/anti-aligned.
    const rels_div: [3]f32 = [0.9, 0.85, 0.5];
    const embs_div: [12]f32 = [
        1.0,  0.0, 0.0, 0.0, // doc 0 (seed)
        0.95, 0.1, 0.0, 0.0, // doc 1 (redundant with 0)
        -1.0, 0.0, 0.0, 0.0, // doc 2 (cosine = -1.0 -> maximum diversity)
    ];
    let div_buf: [2]usize = [0; 2];
    let div_s: []usize = div_buf[0..];
    const k_div = mmrRerankSimdF32(rels_div[0..], embs_div[0..], 3, 4, 2, 0.0, div_s) ?? 0;
    expect_eq(k_div, 2);
    expect_eq(div_buf[0], 0);
    expect_eq(div_buf[1], 2);

    // Requesting top_k > n_docs safely clamps to n_docs (and handles zero vector cleanly).
    let all_buf: [5]usize = [0; 5];
    let all_s: []usize = all_buf[0..];
    const k_all = mmrRerankSimdF32(rels[0..], embs[0..], 5, 8, 10, 0.5, all_s) ?? 0;
    expect_eq(k_all, 5);
}

// ============================================================================
// Test 4: SIMD @l2sq Near-Duplicate Chunk Pruning
// ============================================================================

fn testDedupNearDuplicatesL2()! {
    const embs: [16]f32 = [
        1.0,  0.0,  0.0, 0.0, // doc 0: kept
        1.02, 0.01, 0.0, 0.0, // doc 1: l2sq = 0.0005 < 0.05 -> pruned
        0.0,  1.0,  0.0, 0.0, // doc 2: l2sq = 2.0 >= 0.05 -> kept
        0.01, 0.99, 0.0, 0.0, // doc 3: l2sq to doc 2 = 0.0002 < 0.05 -> pruned
    ];
    let out_buf: [4]usize = [0; 4];
    let out_s: []usize = out_buf[0..];

    const kept = dedupNearDuplicatesL2F32(embs[0..], 4, 4, 0.05, 4, out_s) ?? 0;
    expect_eq(kept, 2);
    expect_eq(out_buf[0], 0);
    expect_eq(out_buf[1], 2);

    // max_keep = 1 caps output at 1
    let one_buf: [1]usize = [0];
    let one_s: []usize = one_buf[0..];
    const kept1 = dedupNearDuplicatesL2F32(embs[0..], 4, 4, 0.05, 1, one_s) ?? 0;
    expect_eq(kept1, 1);
    expect_eq(one_buf[0], 0);
}

// ============================================================================
// Test 5: Context Token Budget Pruning (Greedy Skip vs Strict Prefix)
// ============================================================================

fn testTokenBudgetPruning()! {
    const chunks: [5]RagContextChunk = [
        RagContextChunk{ doc_id: 1, score: 0.95, token_count: 300 },
        RagContextChunk{ doc_id: 2, score: 0.88, token_count: 500 }, // exceeds remaining 300
        RagContextChunk{ doc_id: 3, score: 0.40, token_count: 100 }, // below min_score 0.50
        RagContextChunk{ doc_id: 4, score: 0.80, token_count: 250 }, // fits in remaining 300
        RagContextChunk{ doc_id: 5, score: 0.70, token_count: 0 },   // zero-token metadata chunk
    ];

    let out_buf: [5]RagContextChunk = [RagContextChunk{}; 5];
    let out_s: []RagContextChunk = out_buf[0..];

    // 1. Greedy packing mode (max_tokens = 600, min_score = 0.5):
    // Picks doc 1 (300), skips oversized doc 2 (500), filters low-score doc 3 (0.40),
    // packs doc 4 (250) and doc 5 (0) -> total 550 tokens.
    const greedy = selectWithinTokenBudget(chunks[0..], 600, 0.5, true, out_s) ?? return error.BudgetFailed;
    expect_eq(greedy.selected_count, 3);
    expect_eq(greedy.used_tokens, 550);
    expect_eq(greedy.skipped_oversized, 1);
    expect_eq(greedy.filtered_low_score, 1);
    expect_eq(greedy.budget_limit, 600);
    expect_eq(out_buf[0].doc_id, 1);
    expect_eq(out_buf[1].doc_id, 4);
    expect_eq(out_buf[2].doc_id, 5);

    // 2. Strict prefix truncation mode (greedy_skip_oversized = false):
    // Picks doc 1 (300), hits oversized doc 2 (500) and stops immediately.
    const strict = selectWithinTokenBudget(chunks[0..], 600, 0.5, false, out_s) ?? return error.BudgetFailed;
    expect_eq(strict.selected_count, 1);
    expect_eq(strict.used_tokens, 300);
    expect_eq(strict.skipped_oversized, 1);
    expect_eq(strict.filtered_low_score, 0);
    expect_eq(out_buf[0].doc_id, 1);

    // 3. Output buffer capacity cap (only 1 slot):
    let cap1_buf: [1]RagContextChunk = [RagContextChunk{}; 1];
    let cap1_s: []RagContextChunk = cap1_buf[0..];
    const capped = selectWithinTokenBudget(chunks[0..], 1000, 0.5, true, cap1_s) ?? return error.BudgetFailed;
    expect_eq(capped.selected_count, 1);
    expect_eq(capped.used_tokens, 300);
}

// ============================================================================
// Test 6: End-to-End Hybrid RAG Pipeline & Defensive Guards
// ============================================================================

fn testEndToEndPipelineAndGuards()! {
    // End-to-end: Dense + Sparse RRF -> MMR Diversity Rerank -> Token Budget Pruning
    const rrf = HybridRrfFusion.init(0.6, 0.4, 60.0) ?? return error.InitFailed;
    const dense_ids: [4]u32 = [0, 1, 2, 3];
    const sparse_ids: [4]u32 = [1, 0, 3, 2];
    let work_hits: [4]HybridRagHit = [HybridRagHit{}; 4];
    let fused_hits: [4]HybridRagHit = [HybridRagHit{}; 4];
    let work_s: []HybridRagHit = work_hits[0..];
    let fused_s: []HybridRagHit = fused_hits[0..];
    const n_fused = rrf.fuseDual(dense_ids[0..], sparse_ids[0..], 4, fused_s, work_s) ?? 0;
    expect_eq(n_fused, 4);

    // Doc 0 and doc 1 have nearly identical embeddings; doc 2 and doc 3 are distinct.
    const corpus_embs: [16]f32 = [
        1.0,  0.0,  0.0, 0.0, // doc 0
        0.99, 0.01, 0.0, 0.0, // doc 1 (duplicate of 0)
        0.0,  1.0,  0.0, 0.0, // doc 2
        0.0,  0.0,  1.0, 0.0, // doc 3
    ];
    const token_lens: [4]u32 = [200, 200, 250, 180];

    let cand_rels: [4]f32 = [0.0; 4];
    let cand_embs: [16]f32 = [0.0; 16];
    let i: usize = 0;
    while (i < n_fused) : (i += 1) {
        const d_id: usize = fused_hits[i].doc_id as usize;
        cand_rels[i] = fused_hits[i].score;
        let d: usize = 0;
        while (d < 4) : (d += 1) {
            cand_embs[i * 4 + d] = corpus_embs[d_id * 4 + d];
        }
    }

    let mmr_idx: [3]usize = [0; 3];
    let mmr_s: []usize = mmr_idx[0..];
    const n_mmr = mmrRerankSimdF32(cand_rels[0..], cand_embs[0..], n_fused, 4, 3, 0.5, mmr_s) ?? 0;
    expect_eq(n_mmr, 3);

    let budget_in: [3]RagContextChunk = [RagContextChunk{}; 3];
    let m_i: usize = 0;
    while (m_i < n_mmr) : (m_i += 1) {
        const hit = fused_hits[mmr_idx[m_i]];
        budget_in[m_i] = RagContextChunk{
            doc_id: hit.doc_id,
            score: hit.score,
            token_count: token_lens[hit.doc_id as usize],
        };
    }

    let packed_buf: [3]RagContextChunk = [RagContextChunk{}; 3];
    let packed_s: []RagContextChunk = packed_buf[0..];
    const budget = selectWithinTokenBudget(budget_in[0..], 400, 0.0, true, packed_s) ?? return error.BudgetFailed;
    // First pick is doc 0 (200 tokens), second diverse pick is doc 2 (250 -> exceeds 200 rem, skipped),
    // third diverse pick is doc 3 (180 -> fits in 200 rem) -> total 380 tokens, 0 duplicates!
    expect_eq(budget.selected_count, 2);
    expect_eq(budget.used_tokens, 380);
    expect_eq(packed_buf[0].doc_id, 0);
    expect_eq(packed_buf[1].doc_id, 3);

    // --- Defensive Guards ---
    const nan_f32: f32 = std.math.nan(f32);
    const inf_f32: f32 = std.math.inf(f32);

    // Invalid HybridRrfFusion configs
    expect_eq(HybridRrfFusion.init(-0.1, 1.0, 60.0) == null, true);
    expect_eq(HybridRrfFusion.init(1.0, -0.1, 60.0) == null, true);
    expect_eq(HybridRrfFusion.init(0.0, 0.0, 60.0) == null, true);
    expect_eq(HybridRrfFusion.init(1.0, 1.0, -1.0) == null, true);
    expect_eq(HybridRrfFusion.init(nan_f32, 1.0, 60.0) == null, true);

    // Empty inputs return 0 safely
    const empty_u32: []const u32 = dense_ids[0..0];
    expect_eq(rrf.fuseDual(empty_u32, empty_u32, 5, fused_s, work_s) ?? 99, 0);
    expect_eq(rrf.fuseDual(dense_ids[0..], sparse_ids[0..], 0, fused_s, work_s) ?? 99, 0);

    // Undersized work_hits returns null
    let tiny_work: [1]HybridRagHit = [HybridRagHit{}; 1];
    let tiny_s: []HybridRagHit = tiny_work[0..];
    expect_eq(rrf.fuseDual(dense_ids[0..], sparse_ids[0..], 2, fused_s, tiny_s) == null, true);

    // Invalid CSR offsets / weights mismatch in fuseMultiWeighted
    const bad_offs: [3]usize = [0, 3, 2]; // decreasing
    const w2: [2]f32 = [0.5, 0.5];
    expect_eq(rrf.fuseMultiWeighted(dense_ids[0..], bad_offs[0..], w2[0..], 2, fused_s, work_s) == null, true);

    // Invalid MMR inputs: lambda out of range, NaN, dim = 0, empty returns 0
    const empty_f32: []const f32 = cand_rels[0..0];
    expect_eq(mmrRerankSimdF32(empty_f32, empty_f32, 0, 4, 2, 0.5, mmr_s) ?? 99, 0);
    expect_eq(mmrRerankSimdF32(cand_rels[0..], cand_embs[0..], 4, 4, 0, 0.5, mmr_s) ?? 99, 0);
    expect_eq(mmrRerankSimdF32(cand_rels[0..], cand_embs[0..], 4, 4, 2, -0.01, mmr_s) == null, true);
    expect_eq(mmrRerankSimdF32(cand_rels[0..], cand_embs[0..], 4, 4, 2, 1.01, mmr_s) == null, true);
    expect_eq(mmrRerankSimdF32(cand_rels[0..], cand_embs[0..], 4, 0, 2, 0.5, mmr_s) == null, true);

    let nan_rels: [4]f32 = [0.5, nan_f32, 0.3, 0.2];
    expect_eq(mmrRerankSimdF32(nan_rels[0..], cand_embs[0..], 4, 4, 2, 0.5, mmr_s) == null, true);

    // Invalid dedupNearDuplicatesL2F32 inputs
    expect_eq(dedupNearDuplicatesL2F32(empty_f32, 0, 4, 0.1, 2, mmr_s) ?? 99, 0);
    expect_eq(dedupNearDuplicatesL2F32(cand_embs[0..], 4, 4, -0.1, 2, mmr_s) == null, true);

    // Invalid selectWithinTokenBudget inputs: NaN/Inf scores & empty candidates
    const empty_chunks: []const RagContextChunk = budget_in[0..0];
    const empty_res = selectWithinTokenBudget(empty_chunks, 500, 0.0, true, packed_s) ?? return error.BudgetFailed;
    expect_eq(empty_res.selected_count, 0);
    expect_eq(empty_res.used_tokens, 0);
    expect_eq(selectWithinTokenBudget(budget_in[0..], 500, nan_f32, true, packed_s) == null, true);
    const bad_chunks: [1]RagContextChunk = [RagContextChunk{ doc_id: 1, score: inf_f32, token_count: 10 }];
    expect_eq(selectWithinTokenBudget(bad_chunks[0..], 500, 0.0, true, packed_s) == null, true);
}

fn main() {
    test("1. vector.hybrid_rag: dual weighted RRF fusion", () => {
        testDualWeightedRrfFusion()?;
    });
    test("2. vector.hybrid_rag: M-way CSR weighted RRF fusion", () => {
        testMultiWayWeightedRrfFusion()?;
    });
    test("3. vector.hybrid_rag: SIMD @dot/@fma MMR diversity reranking", () => {
        testMmrRerankSimdDiversity()?;
    });
    test("4. vector.hybrid_rag: SIMD @l2sq near-duplicate pruning", () => {
        testDedupNearDuplicatesL2()?;
    });
    test("5. vector.hybrid_rag: token budget pruning greedy & strict", () => {
        testTokenBudgetPruning()?;
    });
    test("6. vector.hybrid_rag: end-to-end pipeline & defensive guards", () => {
        testEndToEndPipelineAndGuards()?;
    });
}
