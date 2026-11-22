// Origin: Pure-CP Stdlib Test Suite (lib/vector/lateChunk.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Late Chunking & Contextual Token-Span Pooling Engine:
//   1. End-to-end Late Chunking pipeline: semantic boundary detection -> mean_pool + L2 norm -> Top-K search -> stats
//   2. All 3 pooling strategies (mean_pool, weighted_attention_pool external & self-norm, max_norm_pool) with & without L2 norm
//   3. Semantic boundary detection min/max chunk token length constraints & valley boundary_sim recording
//   4. In-document Top-K retrieval deterministic descending sort, ascending chunk_idx tie-breaking & K-truncation
//   5. NaN / Inf sanitization across embeddings, weights & queries + zero-vector safe L2 normalization
//   6. Comprehensive boundary & error guards (dim==0, num_tokens==0, invalid spans, undersized output buffers)
//   7. High-dimensional non-multiple-of-8 SIMD + scalar-tail accuracy (dim=19) & cap.vector facade parity

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    LateChunkPoolMode,
    LateChunkSpan,
    LateChunkHit,
    LateChunkStats,
    sanitizeVectorF32,
    normalizeVectorInPlaceF32,
    detectSemanticBoundaries,
    poolLateChunkSpan,
    poolLateChunks,
    searchLateChunks,
    computeLateChunkStats,
} from "cap:vector";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff: f32 = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

// ============================================================================
// Test 1: End-to-End Late Chunking Pipeline (Detect -> Pool -> Search -> Stats)
// ============================================================================
test("lateChunk: end-to-end semantic boundary detection, mean pooling & Top-K search", () => {
    // 9 tokens, dim=4 representing 3 semantic segments:
    //   Segment 0 [0..3): Topic A around [1, 0, 0, 0]
    //   Segment 1 [3..6): Topic B around [0, 1, 0, 0] with slight coreference bleed (0.2 on dim 0)
    //   Segment 2 [6..9): Topic C around [0, 0, 1, 0]
    const tokens: [36]f32 = [
        1.0, 0.05, 0.0, 0.0,
        0.98, 0.10, 0.0, 0.0,
        0.95, 0.08, 0.0, 0.0,
        0.20, 0.96, 0.0, 0.0,
        0.18, 0.98, 0.0, 0.0,
        0.15, 0.97, 0.0, 0.0,
        0.0, 0.05, 0.99, 0.0,
        0.0, 0.02, 1.00, 0.0,
        0.0, 0.04, 0.98, 0.0,
    ];

    let spans_buf: [8]LateChunkSpan = [LateChunkSpan{}; 8];
    let spans_s: []LateChunkSpan = spans_buf[0..];
    const n_spans = detectSemanticBoundaries(tokens[0..], 9, 4, 2, 5, 0.5, *spans_s) ?? 0;
    expect_eq(n_spans, 3 as usize);
    expect_eq(spans_buf[0].start_token, 0 as usize);
    expect_eq(spans_buf[0].end_token, 3 as usize);
    expect_eq(spans_buf[1].start_token, 3 as usize);
    expect_eq(spans_buf[1].end_token, 6 as usize);
    expect_eq(spans_buf[2].start_token, 6 as usize);
    expect_eq(spans_buf[2].end_token, 9 as usize);
    expect_true(spans_buf[0].boundary_sim < 0.5);
    expect_true(spans_buf[1].boundary_sim < 0.5);

    // Pool all 3 chunks with mean_pool + L2 normalization
    let chunks_buf: [12]f32 = [0.0; 12];
    let chunks_s: []f32 = chunks_buf[0..];
    const empty_w: [0]f32 = [];
    const n_pooled = poolLateChunks(
        tokens[0..],
        9,
        4,
        spans_buf[0..n_spans],
        LateChunkPoolMode.mean_pool,
        empty_w[0..],
        true,
        *chunks_s,
    ) ?? 0;
    expect_eq(n_pooled, 3 as usize);

    // Verify every pooled chunk is L2 unit-normalized
    let c: usize = 0;
    while (c < n_pooled) : (c += 1) {
        const row = chunks_buf[c * 4 .. (c + 1) * 4];
        const norm_sq: f32 = @dot(row, row);
        expect_true(approxEqF32(norm_sq, 1.0, 1.0e-5));
    }

    // Query targeting Topic B + coreference context from Topic A
    const query: [4]f32 = [0.2, 0.98, 0.0, 0.0];
    let hits_buf: [3]LateChunkHit = [LateChunkHit{}; 3];
    let hits_s: []LateChunkHit = hits_buf[0..];
    const n_hits = searchLateChunks(query[0..], chunks_buf[0..], n_pooled, 4, *hits_s) ?? 0;
    expect_eq(n_hits, 3 as usize);
    expect_eq(hits_buf[0].chunk_idx, 1 as usize);
    expect_true(hits_buf[0].score > 0.99);
    expect_eq(hits_buf[1].chunk_idx, 0 as usize);
    expect_eq(hits_buf[2].chunk_idx, 2 as usize);

    // Verify stats
    if (st := computeLateChunkStats(spans_buf[0..n_spans], 9, 4, chunks_buf[0..])) {
        expect_eq(st.num_tokens, 9 as usize);
        expect_eq(st.num_chunks, 3 as usize);
        expect_eq(st.dim, 4 as usize);
        expect_eq(st.min_chunk_len, 3 as usize);
        expect_eq(st.max_chunk_len, 3 as usize);
        expect_eq(st.zero_norm_chunks, 0 as usize);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 2: All 3 Pooling Strategies (Mean, Weighted Attention, Max-Norm)
// ============================================================================
test("lateChunk: mean_pool, weighted_attention_pool (external & auto-norm) & max_norm_pool accuracy", () => {
    // 3 tokens, dim=4:
    //   tok 0: [1, 0, 0, 0] (norm=1)
    //   tok 1: [0, 4, 0, 0] (norm=4, largest norm!)
    //   tok 2: [0, 0, 2, 0] (norm=2)
    const tokens: [12]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 4.0, 0.0, 0.0,
        0.0, 0.0, 2.0, 0.0,
    ];
    const span = LateChunkSpan{ start_token: 0, end_token: 3 };
    const empty_w: [0]f32 = [];

    // 2A. mean_pool without L2 normalization -> [1/3, 4/3, 2/3, 0]
    let out_mean: [4]f32 = [0.0; 4];
    let out_mean_s: []f32 = out_mean[0..];
    expect_true(poolLateChunkSpan(
        tokens[0..], 3, 4, span, LateChunkPoolMode.mean_pool, empty_w[0..], false, *out_mean_s,
    ));
    expect_true(approxEqF32(out_mean[0], 1.0 / 3.0, 1.0e-5));
    expect_true(approxEqF32(out_mean[1], 4.0 / 3.0, 1.0e-5));
    expect_true(approxEqF32(out_mean[2], 2.0 / 3.0, 1.0e-5));
    expect_true(approxEqF32(out_mean[3], 0.0, 1.0e-5));

    // 2B. weighted_attention_pool with external weights [3.0, 1.0, 0.0] -> [3/4, 4/4, 0, 0] = [0.75, 1.0, 0, 0]
    const weights: [3]f32 = [3.0, 1.0, 0.0];
    let out_w: [4]f32 = [0.0; 4];
    let out_w_s: []f32 = out_w[0..];
    expect_true(poolLateChunkSpan(
        tokens[0..], 3, 4, span, LateChunkPoolMode.weighted_attention_pool, weights[0..], false, *out_w_s,
    ));
    expect_true(approxEqF32(out_w[0], 0.75, 1.0e-5));
    expect_true(approxEqF32(out_w[1], 1.0, 1.0e-5));
    expect_true(approxEqF32(out_w[2], 0.0, 1.0e-5));

    // 2C. weighted_attention_pool with empty weights (auto L2-norm salience weights: 1, 4, 2; sum=7)
    //     unnormalized -> [1/7, 16/7, 4/7, 0]
    let out_auto: [4]f32 = [0.0; 4];
    let out_auto_s: []f32 = out_auto[0..];
    expect_true(poolLateChunkSpan(
        tokens[0..], 3, 4, span, LateChunkPoolMode.weighted_attention_pool, empty_w[0..], false, *out_auto_s,
    ));
    expect_true(approxEqF32(out_auto[0], 1.0 / 7.0, 1.0e-5));
    expect_true(approxEqF32(out_auto[1], 16.0 / 7.0, 1.0e-5));
    expect_true(approxEqF32(out_auto[2], 4.0 / 7.0, 1.0e-5));

    // 2D. max_norm_pool with L2 normalization -> selects tok 1 ([0, 4, 0, 0]) -> normalized [0, 1, 0, 0]
    let out_max: [4]f32 = [0.0; 4];
    let out_max_s: []f32 = out_max[0..];
    expect_true(poolLateChunkSpan(
        tokens[0..], 3, 4, span, LateChunkPoolMode.max_norm_pool, empty_w[0..], true, *out_max_s,
    ));
    expect_true(approxEqF32(out_max[0], 0.0, 1.0e-5));
    expect_true(approxEqF32(out_max[1], 1.0, 1.0e-5));
    expect_true(approxEqF32(out_max[2], 0.0, 1.0e-5));
    expect_true(approxEqF32(out_max[3], 0.0, 1.0e-5));
});

// ============================================================================
// Test 3: Boundary Detection min_chunk_tokens & max_chunk_tokens Constraints
// ============================================================================
test("lateChunk: boundary detection respects min_chunk_tokens and max_chunk_tokens bounds", () => {
    // Case A: Alternating orthogonal tokens (sim=0.0 at every step), min_chunk_tokens=3, max_chunk_tokens=5
    // Must NOT split at t=1 or t=2; splits at t=3 and t=6 -> [0..3), [3..6), [6..8)
    const ortho_tokens: [16]f32 = [
        1.0, 0.0,
        0.0, 1.0,
        1.0, 0.0,
        0.0, 1.0,
        1.0, 0.0,
        0.0, 1.0,
        1.0, 0.0,
        0.0, 1.0,
    ];
    let spans: [8]LateChunkSpan = [LateChunkSpan{}; 8];
    let spans_s: []LateChunkSpan = spans[0..];
    const n1 = detectSemanticBoundaries(ortho_tokens[0..], 8, 2, 3, 5, 0.5, *spans_s) ?? 0;
    expect_eq(n1, 3 as usize);
    expect_eq(spans[0].start_token, 0 as usize);
    expect_eq(spans[0].end_token, 3 as usize);
    expect_eq(spans[1].start_token, 3 as usize);
    expect_eq(spans[1].end_token, 6 as usize);
    expect_eq(spans[2].start_token, 6 as usize);
    expect_eq(spans[2].end_token, 8 as usize);

    // Case B: All identical tokens (sim=1.0 everywhere), max_chunk_tokens=3 -> forced splits [0..3), [3..6), [6..7)
    const same_tokens: [14]f32 = [
        1.0, 0.0,
        1.0, 0.0,
        1.0, 0.0,
        1.0, 0.0,
        1.0, 0.0,
        1.0, 0.0,
        1.0, 0.0,
    ];
    const n2 = detectSemanticBoundaries(same_tokens[0..], 7, 2, 2, 3, 0.1, *spans_s) ?? 0;
    expect_eq(n2, 3 as usize);
    expect_eq(spans[0].tokenCount(), 3 as usize);
    expect_eq(spans[1].tokenCount(), 3 as usize);
    expect_eq(spans[2].tokenCount(), 1 as usize);
    expect_true(approxEqF32(spans[0].boundary_sim, 1.0, 1.0e-5));
});

// ============================================================================
// Test 4: Top-K Search Deterministic Tie-Breaking & K-Truncation
// ============================================================================
test("lateChunk: searchLateChunks deterministic tie-breaking and Top-K truncation", () => {
    // 4 chunks, dim=2:
    //   chunk 0: [0.0, 1.0] (cos with [1,0] = 0.0)
    //   chunk 1: [1.0, 0.0] (cos = 1.0)
    //   chunk 2: [0.6, 0.8] (cos = 0.6)
    //   chunk 3: [1.0, 0.0] (cos = 1.0, tied with chunk 1 -> chunk 1 must win tie)
    const chunks: [8]f32 = [
        0.0, 1.0,
        1.0, 0.0,
        0.6, 0.8,
        1.0, 0.0,
    ];
    const query: [2]f32 = [1.0, 0.0];

    // Request Top-2 only
    let hits2: [2]LateChunkHit = [LateChunkHit{}; 2];
    let hits2_s: []LateChunkHit = hits2[0..];
    const k = searchLateChunks(query[0..], chunks[0..], 4, 2, *hits2_s) ?? 0;
    expect_eq(k, 2 as usize);
    expect_eq(hits2[0].chunk_idx, 1 as usize);
    expect_eq(hits2[1].chunk_idx, 3 as usize);
    expect_true(approxEqF32(hits2[0].score, 1.0, 1.0e-5));
    expect_true(approxEqF32(hits2[1].score, 1.0, 1.0e-5));

    // Request Top-6 (> num_chunks=4) -> returns exactly 4 in sorted order [1, 3, 2, 0]
    let hits6: [6]LateChunkHit = [LateChunkHit{}; 6];
    let hits6_s: []LateChunkHit = hits6[0..];
    const k_all = searchLateChunks(query[0..], chunks[0..], 4, 2, *hits6_s) ?? 0;
    expect_eq(k_all, 4 as usize);
    expect_eq(hits6[0].chunk_idx, 1 as usize);
    expect_eq(hits6[1].chunk_idx, 3 as usize);
    expect_eq(hits6[2].chunk_idx, 2 as usize);
    expect_eq(hits6[3].chunk_idx, 0 as usize);
    expect_true(approxEqF32(hits6[2].score, 0.6, 1.0e-5));
    expect_true(approxEqF32(hits6[3].score, 0.0, 1.0e-5));
});

// ============================================================================
// Test 5: NaN / Inf Sanitization & Zero-Vector Safe L2 Normalization
// ============================================================================
test("lateChunk: NaN/Inf sanitization and zero-vector safe normalization", () => {
    const nan_val: f32 = std.math.nan(f32);
    const inf_val: f32 = std.math.inf(f32);

    // 5A. Direct sanitizeVectorF32
    const dirty: [4]f32 = [nan_val, 3.0, inf_val, 4.0];
    let clean: [4]f32 = [0.0; 4];
    let clean_s: []f32 = clean[0..];
    const cleaned_cnt = sanitizeVectorF32(dirty[0..], *clean_s) ?? 99;
    expect_eq(cleaned_cnt, 2 as usize);
    expect_true(approxEqF32(clean[0], 0.0, 1.0e-6));
    expect_true(approxEqF32(clean[1], 3.0, 1.0e-6));
    expect_true(approxEqF32(clean[2], 0.0, 1.0e-6));
    expect_true(approxEqF32(clean[3], 4.0, 1.0e-6));

    // 5B. normalizeVectorInPlaceF32 on [0, 3, 0, 4] -> [0, 0.6, 0, 0.8]
    expect_true(normalizeVectorInPlaceF32(*clean_s));
    expect_true(approxEqF32(clean[1], 0.6, 1.0e-5));
    expect_true(approxEqF32(clean[3], 0.8, 1.0e-5));

    // 5C. Zero-vector and all-NaN safe normalization -> returns false, stays finite 0.0
    const neg_inf: f32 = 0.0 - inf_val;
    let zvec: [4]f32 = [0.0, nan_val, 0.0, neg_inf];
    let zvec_s: []f32 = zvec[0..];
    expect_eq(normalizeVectorInPlaceF32(*zvec_s), false);
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_true(isFiniteF32(zvec[i]));
        expect_true(approxEqF32(zvec[i], 0.0, 1.0e-6));
    }

    // 5D. Pooling & searching with NaN-contaminated tokens and zero-only span
    const dirty_tokens: [12]f32 = [
        nan_val, 1.0, 0.0, 0.0,
        0.0, 0.0, 0.0, 0.0,
        0.0, 0.0, inf_val, 0.0,
    ];
    const spans: [2]LateChunkSpan = [
        LateChunkSpan{ start_token: 0, end_token: 1 }, // becomes [0, 1, 0, 0]
        LateChunkSpan{ start_token: 1, end_token: 3 }, // becomes [0, 0, 0, 0] (zero norm)
    ];
    let pooled: [8]f32 = [0.0; 8];
    let pooled_s: []f32 = pooled[0..];
    const empty_w: [0]f32 = [];
    const n_p = poolLateChunks(
        dirty_tokens[0..], 3, 4, spans[0..], LateChunkPoolMode.mean_pool, empty_w[0..], true, *pooled_s,
    ) ?? 0;
    expect_eq(n_p, 2 as usize);
    expect_true(approxEqF32(pooled[1], 1.0, 1.0e-5));
    expect_true(approxEqF32(pooled[4], 0.0, 1.0e-6));
    expect_true(approxEqF32(pooled[6], 0.0, 1.0e-6));

    // Search with NaN-contaminated query: [nan, 1.0, 0.0, 0.0] -> cleaned to [0, 1, 0, 0]
    const dirty_q: [4]f32 = [nan_val, 1.0, 0.0, 0.0];
    let hits: [2]LateChunkHit = [LateChunkHit{}; 2];
    let hits_s: []LateChunkHit = hits[0..];
    const n_h = searchLateChunks(dirty_q[0..], pooled[0..], 2, 4, *hits_s) ?? 0;
    expect_eq(n_h, 2 as usize);
    expect_eq(hits[0].chunk_idx, 0 as usize);
    expect_true(approxEqF32(hits[0].score, 1.0, 1.0e-5));
    expect_eq(hits[1].chunk_idx, 1 as usize);
    expect_true(approxEqF32(hits[1].score, 0.0, 1.0e-5));

    if (st := computeLateChunkStats(spans[0..], 3, 4, pooled[0..])) {
        expect_eq(st.zero_norm_chunks, 1 as usize);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 6: Comprehensive Boundary & Malformed Input Guards
// ============================================================================
test("lateChunk: boundary guards reject invalid dims, spans, weights and undersized buffers", () => {
    const tokens: [8]f32 = [1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0];
    let spans: [4]LateChunkSpan = [LateChunkSpan{}; 4];
    let spans_s: []LateChunkSpan = spans[0..];
    let out_vec: [4]f32 = [0.0; 4];
    let out_vec_s: []f32 = out_vec[0..];
    const empty_w: [0]f32 = [];

    // Invalid dim == 0
    expect_eq(detectSemanticBoundaries(tokens[0..], 4, 0, 1, 2, 0.5, *spans_s) == null, true);
    // num_tokens == 0 returns 0 spans cleanly
    expect_eq(detectSemanticBoundaries(tokens[0..], 0, 2, 1, 2, 0.5, *spans_s) ?? 99, 0 as usize);
    // min_chunk_tokens > max_chunk_tokens
    expect_eq(detectSemanticBoundaries(tokens[0..], 4, 2, 3, 2, 0.5, *spans_s) == null, true);
    // Non-finite drop_threshold
    expect_eq(detectSemanticBoundaries(tokens[0..], 4, 2, 1, 2, std.math.nan(f32), *spans_s) == null, true);
    // Undersized out_spans buffer (needs 4 spans of len 1, only 1 slot given)
    let tiny_spans: [1]LateChunkSpan = [LateChunkSpan{}];
    let tiny_spans_s: []LateChunkSpan = tiny_spans[0..];
    expect_eq(detectSemanticBoundaries(tokens[0..], 4, 2, 1, 1, 0.5, *tiny_spans_s) == null, true);

    // Invalid spans in poolLateChunkSpan: start >= end, end > num_tokens
    expect_eq(poolLateChunkSpan(
        tokens[0..], 4, 2, LateChunkSpan{ start_token: 2, end_token: 2 },
        LateChunkPoolMode.mean_pool, empty_w[0..], true, *out_vec_s,
    ), false);
    expect_eq(poolLateChunkSpan(
        tokens[0..], 4, 2, LateChunkSpan{ start_token: 3, end_token: 2 },
        LateChunkPoolMode.mean_pool, empty_w[0..], true, *out_vec_s,
    ), false);
    expect_eq(poolLateChunkSpan(
        tokens[0..], 4, 2, LateChunkSpan{ start_token: 2, end_token: 5 },
        LateChunkPoolMode.mean_pool, empty_w[0..], true, *out_vec_s,
    ), false);

    // Undersized external weights for weighted_attention_pool
    const short_w: [2]f32 = [1.0, 1.0];
    expect_eq(poolLateChunkSpan(
        tokens[0..], 4, 2, LateChunkSpan{ start_token: 0, end_token: 2 },
        LateChunkPoolMode.weighted_attention_pool, short_w[0..], true, *out_vec_s,
    ), false);

    // Undersized out_chunk_vecs in poolLateChunks
    const valid_spans: [2]LateChunkSpan = [
        LateChunkSpan{ start_token: 0, end_token: 2 },
        LateChunkSpan{ start_token: 2, end_token: 4 },
    ];
    let tiny_chunks: [3]f32 = [0.0; 3]; // needs 4 floats (2 * 2)
    let tiny_chunks_s: []f32 = tiny_chunks[0..];
    expect_eq(poolLateChunks(
        tokens[0..], 4, 2, valid_spans[0..], LateChunkPoolMode.mean_pool, empty_w[0..], true, *tiny_chunks_s,
    ) == null, true);

    // searchLateChunks guards: dim mismatch, empty out_hits with non-empty chunks, num_chunks == 0
    const q2: [2]f32 = [1.0, 0.0];
    const q3: [3]f32 = [1.0, 0.0, 0.0];
    let hits: [2]LateChunkHit = [LateChunkHit{}; 2];
    let hits_s: []LateChunkHit = hits[0..];
    let empty_hits: [0]LateChunkHit = [];
    let empty_hits_s: []LateChunkHit = empty_hits[0..];
    expect_eq(searchLateChunks(q3[0..], tokens[0..], 4, 2, *hits_s) == null, true);
    expect_eq(searchLateChunks(q2[0..], tokens[0..], 4, 2, *empty_hits_s) == null, true);
    expect_eq(searchLateChunks(q2[0..], tokens[0..], 0, 2, *hits_s) ?? 99, 0 as usize);
});

// ============================================================================
// Test 7: High-Dimensional SIMD + Scalar-Tail Accuracy (dim=19) & cap.vector Facade
// ============================================================================
test("lateChunk: high-dim (dim=19) SIMD @dot/@fma accuracy & cap.vector dynamic facade", () => {
    // 6 tokens x 19D (crosses 8-lane SIMD boundary twice + 3-element scalar tail)
    // Tokens 0..3 emphasize dims 0..9; Tokens 3..6 emphasize dims 10..18
    let tokens: [114]f32 = [0.0; 114];
    let t: usize = 0;
    while (t < 3) : (t += 1) {
        let d: usize = 0;
        while (d < 10) : (d += 1) {
            tokens[t * 19 + d] = 1.0 + (t as f32) * 0.1;
        }
    }
    while (t < 6) : (t += 1) {
        let d: usize = 10;
        while (d < 19) : (d += 1) {
            tokens[t * 19 + d] = 1.0 + ((t - 3) as f32) * 0.1;
        }
    }

    let spans: [4]LateChunkSpan = [LateChunkSpan{}; 4];
    let spans_s: []LateChunkSpan = spans[0..];
    // Exercise dynamic cap.vector.* facade as well
    const n_spans = cap.vector.detectSemanticBoundaries(tokens[0..], 6, 19, 2, 4, 0.3, *spans_s) ?? 0;
    expect_eq(n_spans, 2 as usize);
    expect_eq(spans[0].start_token, 0 as usize);
    expect_eq(spans[0].end_token, 3 as usize);
    expect_eq(spans[1].start_token, 3 as usize);
    expect_eq(spans[1].end_token, 6 as usize);

    let chunks: [38]f32 = [0.0; 38];
    let chunks_s: []f32 = chunks[0..];
    const empty_w: [0]f32 = [];
    const n_pooled = cap.vector.poolLateChunks(
        tokens[0..], 6, 19, spans[0..n_spans], LateChunkPoolMode.mean_pool, empty_w[0..], true, *chunks_s,
    ) ?? 0;
    expect_eq(n_pooled, 2 as usize);

    // Check unit norms on both 19D pooled chunks
    expect_true(approxEqF32(@dot(chunks[0..19], chunks[0..19]), 1.0, 1.0e-5));
    expect_true(approxEqF32(@dot(chunks[19..38], chunks[19..38]), 1.0, 1.0e-5));

    // Query matching dims 10..18 should rank chunk 1 first with score ~1.0
    let query: [19]f32 = [0.0; 19];
    let d: usize = 10;
    while (d < 19) : (d += 1) {
        query[d] = 1.0;
    }
    let hits: [2]LateChunkHit = [LateChunkHit{}; 2];
    let hits_s: []LateChunkHit = hits[0..];
    const n_hits = cap.vector.searchLateChunks(query[0..], chunks[0..], 2, 19, *hits_s) ?? 0;
    expect_eq(n_hits, 2 as usize);
    expect_eq(hits[0].chunk_idx, 1 as usize);
    expect_true(approxEqF32(hits[0].score, 1.0, 1.0e-5));
    expect_eq(hits[1].chunk_idx, 0 as usize);
    expect_true(approxEqF32(hits[1].score, 0.0, 1.0e-5));
});
