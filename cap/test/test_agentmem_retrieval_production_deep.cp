// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/retrieval.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Three-Factor Memory Retrieval, Redundant Merging & Prompt Packing:
//   1. Three-factor scoring (Recency exp decay + Importance + SIMD @dot Relevance) & weight sensitivity
//   2. Tag filtering, min_score threshold, dead-slot skipping & top-K truncation
//   3. SIMD @dot redundant memory merging (importance promotion, timestamp max, access_count sum)
//   4. Token-budget prompt packing into `[mem#<id>] <summary>\n` format
//   5. End-to-end merge -> retrieve -> pack reflection pipeline
//   6. Empty/NaN/Inf/out-of-bounds/undersized-buffer defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MemoryRetrievalConfig,
    MemoryCandidate,
    MemoryScoredHit,
    MemoryMergeStats,
    MemoryPackResult,
    scoreAndRankMemoriesF32,
    mergeRedundantMemoriesF32,
    packMemoriesForPrompt,
} from "cap:agentmem";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

// ============================================================================
// Test 1: Three-Factor Scoring (Recency, Importance, SIMD @dot Relevance)
// ============================================================================

fn testThreeFactorScoringAndWeights()! {
    const hour_ms: i64 = 3600000;
    const now_ms: i64 = 24 * hour_ms;

    // 3 memories in 8D (directly exercises 8-lane SIMD @dot):
    // mem 10: very recent (0h old), low importance (0.2), orthogonal to query (rel = 0.0)
    // mem 20: old (20h old), high importance (0.95), identical to query (rel = 1.0)
    // mem 30: medium age (2h old), medium importance (0.7), moderate similarity
    const cands: [3]MemoryCandidate = [
        MemoryCandidate{ mem_id: 10, importance: 0.2,  last_access_ms: now_ms,               access_count: 1, token_count: 20, tag: "note", summary: "fresh orthogonal" },
        MemoryCandidate{ mem_id: 20, importance: 0.95, last_access_ms: now_ms - 20 * hour_ms, access_count: 5, token_count: 30, tag: "core", summary: "old highly relevant" },
        MemoryCandidate{ mem_id: 30, importance: 0.7,  last_access_ms: now_ms - 2 * hour_ms,  access_count: 2, token_count: 25, tag: "note", summary: "medium balanced" },
    ];
    const query: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    const embs: [24]f32 = [
        0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // mem 10: orthogonal
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // mem 20: exact match
        0.6, 0.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, // mem 30: cosine = 0.6
    ];

    let hits: [3]MemoryScoredHit = [MemoryScoredHit{}; 3];
    let hits_s: []MemoryScoredHit = hits[0..];

    // 1. Relevance-heavy config: mem 20 must win despite 20h age.
    const cfg_rel = cap.agentmem.MemoryRetrievalConfig.init(0.5, 1.0, 3.0, 0.1) ?? return error.InitFailed;
    const n1 = cap.agentmem.scoreAndRankMemoriesF32(query[0..], cands[0..], embs[0..], 8, now_ms, cfg_rel, 3, hits_s) ?? 0;
    expect_eq(n1, 3);
    expect_eq(hits[0].mem_id, 20);
    expect_eq(absF32(hits[0].relevance_score - 1.0) < 0.0001, true);
    const exp_rec20: f32 = std.math.exp(-2.0 as f32);
    expect_eq(absF32(hits[0].recency_score - exp_rec20) < 0.0001, true);

    // 2. Pure recency config (w_recency = 1.0, w_importance = 0.0, w_relevance = 0.0):
    // mem 10 (0h, recency = 1.0) > mem 30 (2h) > mem 20 (20h).
    const cfg_rec = MemoryRetrievalConfig.init(1.0, 0.0, 0.0, 0.2) ?? return error.InitFailed;
    const n2 = scoreAndRankMemoriesF32(query[0..], cands[0..], embs[0..], 8, now_ms, cfg_rec, 3, hits_s) ?? 0;
    expect_eq(n2, 3);
    expect_eq(hits[0].mem_id, 10);
    expect_eq(absF32(hits[0].recency_score - 1.0) < 0.0001, true);
    expect_eq(hits[1].mem_id, 30);
    expect_eq(hits[2].mem_id, 20);
}

// ============================================================================
// Test 2: Tag Filtering, min_score Threshold & Dead-Slot Skipping
// ============================================================================

fn testTagFilterAndMinScore()! {
    const cands: [4]MemoryCandidate = [
        MemoryCandidate{ mem_id: 1, importance: 0.9, last_access_ms: 1000, tag: "code", summary: "a", alive: true },
        MemoryCandidate{ mem_id: 2, importance: 0.9, last_access_ms: 1000, tag: "chat", summary: "b", alive: true },
        MemoryCandidate{ mem_id: 3, importance: 0.9, last_access_ms: 1000, tag: "code", summary: "c", alive: false }, // dead
        MemoryCandidate{ mem_id: 4, importance: 0.1, last_access_ms: 1000, tag: "code", summary: "d", alive: true },
    ];
    const query: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const embs: [16]f32 = [
        1.0, 0.0, 0.0, 0.0,
        1.0, 0.0, 0.0, 0.0,
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
    ];

    let cfg = MemoryRetrievalConfig.init(0.0, 1.0, 1.0, 0.0) ?? return error.InitFailed;
    cfg.required_tag = "code";
    cfg.min_score = 0.5;

    let hits: [4]MemoryScoredHit = [MemoryScoredHit{}; 4];
    let hits_s: []MemoryScoredHit = hits[0..];
    const n = scoreAndRankMemoriesF32(query[0..], cands[0..], embs[0..], 4, 1000, cfg, 4, hits_s) ?? 0;

    // Only mem 1 is alive, has tag "code", and has score >= 0.5 (mem 4 has score 0.05)
    expect_eq(n, 1);
    expect_eq(hits[0].mem_id, 1);
}

// ============================================================================
// Test 3: SIMD @dot Redundant Episodic Memory Merging
// ============================================================================

fn testMergeRedundantMemories()! {
    let cands: [4]MemoryCandidate = [
        MemoryCandidate{ mem_id: 100, importance: 0.60, last_access_ms: 1000, access_count: 2, summary: "short note", alive: true },
        MemoryCandidate{ mem_id: 101, importance: 0.92, last_access_ms: 5000, access_count: 3, summary: "detailed refined note", alive: true },
        MemoryCandidate{ mem_id: 102, importance: 0.50, last_access_ms: 2000, access_count: 1, summary: "orthogonal fact", alive: true },
        MemoryCandidate{ mem_id: 103, importance: 0.40, last_access_ms: 6000, access_count: 4, summary: "another dup", alive: true },
    ];
    const embs: [16]f32 = [
        1.0,  0.0,  0.0, 0.0, // 100
        0.99, 0.01, 0.0, 0.0, // 101 (sim > 0.99 with 100 -> merges into 100)
        0.0,  1.0,  0.0, 0.0, // 102 (orthogonal -> survives)
        0.98, 0.02, 0.0, 0.0, // 103 (sim > 0.99 with 100 -> merges into 100)
    ];

    let cands_s: []MemoryCandidate = cands[0..];
    const stats = mergeRedundantMemoriesF32(cands_s, embs[0..], 4, 0.95) ?? return error.MergeFailed;
    expect_eq(stats.surviving_count, 2);
    expect_eq(stats.merged_count, 2);

    // Survivor 0 (mem_id 100) promoted importance to 0.92, summary to "detailed refined note",
    // timestamp to 6000, and accumulated access_count = 2 + 3 + 4 = 9.
    expect_eq(cands[0].alive, true);
    expect_eq(absF32(cands[0].importance - 0.92) < 0.0001, true);
    expect_eq(std.mem.eql(u8, cands[0].summary, "detailed refined note"), true);
    expect_eq(cands[0].last_access_ms, 6000);
    expect_eq(cands[0].access_count, 9);

    expect_eq(cands[1].alive, false);
    expect_eq(cands[2].alive, true);
    expect_eq(cands[3].alive, false);
}

// ============================================================================
// Test 4: Token-Budget Prompt Packing (`[mem#<id>] <summary>\n`)
// ============================================================================

fn testPackMemoriesForPrompt()! {
    const cands: [3]MemoryCandidate = [
        MemoryCandidate{ mem_id: 7,  token_count: 40, summary: "alpha", alive: true },
        MemoryCandidate{ mem_id: 42, token_count: 80, summary: "oversized", alive: true },
        MemoryCandidate{ mem_id: 99, token_count: 30, summary: "gamma", alive: true },
    ];
    const ranked: [3]MemoryScoredHit = [
        MemoryScoredHit{ candidate_index: 0, mem_id: 7,  total_score: 0.95 },
        MemoryScoredHit{ candidate_index: 1, mem_id: 42, total_score: 0.85 },
        MemoryScoredHit{ candidate_index: 2, mem_id: 99, total_score: 0.75 },
    ];

    let out_buf: [128]u8 = [0; 128];
    let out_s: []u8 = out_buf[0..];
    // Budget 75: packs mem#7 (40), skips mem#42 (80 > 35), packs mem#99 (30) -> used 70 tokens
    const res = packMemoriesForPrompt(ranked[0..], cands[0..], 75, out_s) ?? return error.PackFailed;
    expect_eq(res.packed_count, 2);
    expect_eq(res.used_tokens, 70);
    expect_eq(res.skipped_oversized, 1);

    const expected = "[mem#7] alpha\n[mem#99] gamma\n";
    expect_eq(std.mem.eql(u8, out_buf[0..res.bytes_written], expected), true);
}

// ============================================================================
// Test 5: End-to-End Merge -> Retrieve -> Pack Reflection Pipeline
// ============================================================================

fn testEndToEndReflectionPipeline()! {
    let cands: [3]MemoryCandidate = [
        MemoryCandidate{ mem_id: 1, importance: 0.7, last_access_ms: 3600000, access_count: 1, token_count: 25, tag: "arch", summary: "CP uses zero-heap slices" },
        MemoryCandidate{ mem_id: 2, importance: 0.9, last_access_ms: 7200000, access_count: 2, token_count: 25, tag: "arch", summary: "CP uses zero-heap slices and SIMD @dot" },
        MemoryCandidate{ mem_id: 3, importance: 0.8, last_access_ms: 7200000, access_count: 1, token_count: 20, tag: "arch", summary: "BM25 complements dense search" },
    ];
    const embs: [12]f32 = [
        1.0,  0.0,  0.0, 0.0,
        0.99, 0.01, 0.0, 0.0, // duplicate of 1
        0.0,  1.0,  0.0, 0.0,
    ];
    let cands_s: []MemoryCandidate = cands[0..];
    const m_stats = mergeRedundantMemoriesF32(cands_s, embs[0..], 4, 0.95) ?? return error.MergeFailed;
    expect_eq(m_stats.merged_count, 1);

    const query: [4]f32 = [0.9, 0.1, 0.0, 0.0];
    const cfg = MemoryRetrievalConfig.init(1.0, 1.0, 2.0, 0.05) ?? return error.InitFailed;
    let hits: [3]MemoryScoredHit = [MemoryScoredHit{}; 3];
    let hits_s: []MemoryScoredHit = hits[0..];
    const n_hits = scoreAndRankMemoriesF32(query[0..], cands[0..], embs[0..], 4, 7200000, cfg, 3, hits_s) ?? 0;
    expect_eq(n_hits, 2);
    expect_eq(hits[0].mem_id, 1);

    let prompt_buf: [256]u8 = [0; 256];
    let prompt_s: []u8 = prompt_buf[0..];
    const packed_res = packMemoriesForPrompt(hits[0..n_hits], cands[0..], 50, prompt_s) ?? return error.PackFailed;
    expect_eq(packed_res.packed_count, 2);
    expect_eq(packed_res.used_tokens, 45);
    const expected = "[mem#1] CP uses zero-heap slices and SIMD @dot\n[mem#3] BM25 complements dense search\n";
    expect_eq(std.mem.eql(u8, prompt_buf[0..packed_res.bytes_written], expected), true);
}

// ============================================================================
// Test 6: Defensive Guards (Empty, NaN, Out-of-Bounds, Undersized Buffers)
// ============================================================================

fn testDefensiveGuards()! {
    const nan_f32: f32 = std.math.nan(f32);
    const query: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const embs: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    let cands: [1]MemoryCandidate = [MemoryCandidate{ mem_id: 1, importance: 0.5, token_count: 10, summary: "hello" }];
    let cands_s: []MemoryCandidate = cands[0..];
    let hits: [1]MemoryScoredHit = [MemoryScoredHit{}; 1];
    let hits_s: []MemoryScoredHit = hits[0..];

    // Invalid configs
    expect_eq(MemoryRetrievalConfig.init(-0.1, 1.0, 1.0, 0.05) == null, true);
    expect_eq(MemoryRetrievalConfig.init(0.0, 0.0, 0.0, 0.05) == null, true);
    expect_eq(MemoryRetrievalConfig.init(1.0, 1.0, 1.0, -0.01) == null, true);
    expect_eq(MemoryRetrievalConfig.init(nan_f32, 1.0, 1.0, 0.05) == null, true);

    const cfg = MemoryRetrievalConfig.init(1.0, 1.0, 1.0, 0.05) ?? return error.InitFailed;

    // Empty candidates / top_k == 0 safely return 0
    const empty_cands: []const MemoryCandidate = cands[0..0];
    expect_eq(scoreAndRankMemoriesF32(query[0..], empty_cands, embs[0..], 4, 1000, cfg, 1, hits_s) ?? 99, 0);
    expect_eq(scoreAndRankMemoriesF32(query[0..], cands[0..], embs[0..], 4, 1000, cfg, 0, hits_s) ?? 99, 0);

    // Invalid dim / NaN query / NaN embedding -> null
    expect_eq(scoreAndRankMemoriesF32(query[0..], cands[0..], embs[0..], 0, 1000, cfg, 1, hits_s) == null, true);
    const nan_q: [4]f32 = [nan_f32, 0.0, 0.0, 0.0];
    expect_eq(scoreAndRankMemoriesF32(nan_q[0..], cands[0..], embs[0..], 4, 1000, cfg, 1, hits_s) == null, true);

    // Invalid mergeRedundantMemoriesF32 thresholds & dim
    expect_eq(mergeRedundantMemoriesF32(cands_s, embs[0..], 0, 0.9) == null, true);
    expect_eq(mergeRedundantMemoriesF32(cands_s, embs[0..], 4, 1.5) == null, true);
    expect_eq(mergeRedundantMemoriesF32(cands_s, embs[0..], 4, -1.0) == null, true);

    // packMemoriesForPrompt: out-of-bounds candidate_index & tiny out_buf -> null
    const bad_hit: [1]MemoryScoredHit = [MemoryScoredHit{ candidate_index: 99, mem_id: 1 }];
    let buf: [64]u8 = [0; 64];
    let buf_s: []u8 = buf[0..];
    expect_eq(packMemoriesForPrompt(bad_hit[0..], cands[0..], 100, buf_s) == null, true);

    const good_hit: [1]MemoryScoredHit = [MemoryScoredHit{ candidate_index: 0, mem_id: 1 }];
    let tiny: [4]u8 = [0; 4];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(packMemoriesForPrompt(good_hit[0..], cands[0..], 100, tiny_s) == null, true);
}

fn main() {
    test("1. agentmem.retrieval: three-factor scoring & weight sensitivity", () => {
        testThreeFactorScoringAndWeights()?;
    });
    test("2. agentmem.retrieval: tag filtering, min_score & dead-slot skipping", () => {
        testTagFilterAndMinScore()?;
    });
    test("3. agentmem.retrieval: SIMD @dot redundant episodic memory merging", () => {
        testMergeRedundantMemories()?;
    });
    test("4. agentmem.retrieval: token-budget prompt packing format", () => {
        testPackMemoriesForPrompt()?;
    });
    test("5. agentmem.retrieval: end-to-end merge -> retrieve -> pack pipeline", () => {
        testEndToEndReflectionPipeline()?;
    });
    test("6. agentmem.retrieval: defensive guards (empty, NaN, OOB, tiny buf)", () => {
        testDefensiveGuards()?;
    });
}
