// Origin: Pure-CP Stdlib Test Suite (lib/vector/matryoshka.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Matryoshka Multi-Resolution Two-Stage SIMD Retrieval:
//   1. truncateAndNormalizeF32: prefix truncation, L2 unit-norm accuracy & non-clobbering tail
//   2. buildCoarseEmbeddingsF32: batch compact coarse embedding bank construction & empty corpus
//   3. searchMatryoshkaTwoStageF32: coarse-to-fine rescoring rank flip, exact Top-K equivalence
//      with full-dim brute-force simdBatchTopKCosineF32, and compute_speedup_ratio verification
//   4. searchMatryoshkaTwoStageL2F32: @l2sq two-stage retrieval equivalence with simdBatchTopKL2F32
//   5. Small corpus, degenerate dimensions (coarse_dim == full_dim) & deterministic tie-breaking
//   6. Comprehensive boundary guards (invalid config, zero vectors, NaN/Inf, undersized buffers)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MatryoshkaConfig,
    MatryoshkaHit,
    MatryoshkaStats,
    truncateAndNormalizeF32,
    buildCoarseEmbeddingsF32,
    searchMatryoshkaTwoStageF32,
    searchMatryoshkaTwoStageL2F32,
    simdBatchTopKCosineF32,
    simdBatchTopKL2F32,
} from "cap:vector";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// Deterministic pseudo-random f32 generator in [-1.0, 1.0] (SplitMix64)
fn nextUnitF32(state: *u64): f32 {
    state.* = state.* +% 0x9E3779B97F4A7C15;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% 0xBF58476D1CE4E5B9;
    z = (z ^ (z >> 27)) *% 0x94D049BB133111EB;
    z = z ^ (z >> 31);
    const bits: u32 = ((z >> 40) & 0xFFFFFF) as u32;
    const u: f32 = (bits as f32) / 16777216.0;
    return u * 2.0 - 1.0;
}

// ============================================================================
// Test 1: Prefix Truncation & L2 Normalization (truncateAndNormalizeF32)
// ============================================================================

fn testTruncateAndNormalizeF32()! {
    const full: [6]f32 = [3.0, 4.0, 100.0, -50.0, 25.0, -12.0];
    let out: [4]f32 = [99.0, 99.0, 99.0, 99.0];
    let out_s: []f32 = out[0..];

    expect_eq(cap.vector.truncateAndNormalizeF32(full[0..], 2, out_s), true);
    expect_eq(approxEqF32(out[0], 0.6, 0.0001), true);
    expect_eq(approxEqF32(out[1], 0.8, 0.0001), true);
    // Tail beyond target_dim must remain untouched
    expect_eq(approxEqF32(out[2], 99.0, 0.0001), true);
    expect_eq(approxEqF32(out[3], 99.0, 0.0001), true);

    const norm_sq: f32 = @dot(out[0..2], out[0..2]);
    expect_eq(approxEqF32(norm_sq, 1.0, 0.0001), true);

    // Full-dimension normalization (target_dim == full.len)
    let out_full: [6]f32 = [0.0; 6];
    let out_full_s: []f32 = out_full[0..];
    expect_eq(truncateAndNormalizeF32(full[0..], 6, out_full_s), true);
    const full_norm_sq: f32 = @dot(out_full[0..], out_full[0..]);
    expect_eq(approxEqF32(full_norm_sq, 1.0, 0.0001), true);
}

// ============================================================================
// Test 2: Batch Compact Coarse Bank Builder (buildCoarseEmbeddingsF32)
// ============================================================================

fn testBuildCoarseEmbeddingsF32()! {
    const n_docs: usize = 16;
    const full_dim: usize = 32;
    const coarse_dim: usize = 8;

    let full_corpus: [512]f32 = [0.0; 512]; // 16 * 32
    let rng: u64 = 0x123456789ABCDEF0;
    let i: usize = 0;
    while (i < 512) : (i += 1) {
        full_corpus[i] = nextUnitF32(*rng);
    }

    let coarse_bank: [128]f32 = [0.0; 128]; // 16 * 8
    let coarse_s: []f32 = coarse_bank[0..];
    expect_eq(buildCoarseEmbeddingsF32(full_corpus[0..], n_docs, full_dim, coarse_dim, coarse_s), true);

    // Every coarse row must be unit-norm and match single-vector truncateAndNormalizeF32
    let single_buf: [8]f32 = [0.0; 8];
    let single_s: []f32 = single_buf[0..];
    let d: usize = 0;
    while (d < n_docs) : (d += 1) {
        const f_base = d * full_dim;
        const c_base = d * coarse_dim;
        expect_eq(truncateAndNormalizeF32(full_corpus[f_base .. f_base + full_dim], coarse_dim, single_s), true);

        const row = coarse_bank[c_base .. c_base + coarse_dim];
        const norm_sq: f32 = @dot(row, row);
        expect_eq(approxEqF32(norm_sq, 1.0, 0.0001), true);

        let j: usize = 0;
        while (j < coarse_dim) : (j += 1) {
            expect_eq(approxEqF32(row[j], single_buf[j], 0.00001), true);
        }
    }

    // Empty corpus succeeds cleanly
    expect_eq(buildCoarseEmbeddingsF32(full_corpus[0..0], 0, full_dim, coarse_dim, coarse_s), true);
}

// ============================================================================
// Test 3: Two-Stage Cosine Search vs Brute-Force Top-K Equivalence & Speedup
// ============================================================================

fn testTwoStageCosineMatchesBruteForceAndSpeedup()! {
    const n_docs: usize = 64;
    const full_dim: usize = 64;
    const coarse_dim: usize = 16;
    const coarse_top_k: usize = 12;
    const final_top_k: usize = 5;

    // Build a Matryoshka-structured corpus where leading 16 dims carry coarse topic energy
    // and trailing 48 dims carry fine-grained semantic discrimination.
    let full_corpus: [4096]f32 = [0.0; 4096]; // 64 * 64
    let rng: u64 = 0xCAFEBABE00000001;

    let d: usize = 0;
    while (d < n_docs) : (d += 1) {
        const base = d * full_dim;
        let k: usize = 0;
        while (k < coarse_dim) : (k += 1) {
            full_corpus[base + k] = nextUnitF32(*rng) * 2.0;
        }
        while (k < full_dim) : (k += 1) {
            full_corpus[base + k] = nextUnitF32(*rng) * 0.35;
        }
    }

    // Plant query and two near-top docs that test Stage-2 fine-rescore rank reversal:
    // - doc 7: slightly stronger in coarse_dim, weaker in fine tail
    // - doc 19: slightly weaker in coarse_dim than doc 7, but near-exact match across full_dim
    let query: [64]f32 = [0.0; 64];
    let qk: usize = 0;
    while (qk < coarse_dim) : (qk += 1) {
        query[qk] = if (qk % 2 == 0) 2.5 else -1.5;
    }
    while (qk < full_dim) : (qk += 1) {
        query[qk] = if (qk % 3 == 0) 0.9 else -0.4;
    }

    // Plant doc 7 (coarse winner, fine runner-up)
    let j: usize = 0;
    while (j < coarse_dim) : (j += 1) {
        full_corpus[7 * full_dim + j] = query[j];
    }
    while (j < full_dim) : (j += 1) {
        full_corpus[7 * full_dim + j] = -0.5 * query[j];
    }

    // Plant doc 19 (slightly perturbed coarse, exact fine tail -> must beat doc 7 after Stage-2!)
    j = 0;
    while (j < coarse_dim) : (j += 1) {
        full_corpus[19 * full_dim + j] = query[j] * 0.95 + (if (j == 0) 0.2 else 0.0);
    }
    while (j < full_dim) : (j += 1) {
        full_corpus[19 * full_dim + j] = query[j];
    }

    // Plant doc 42 (strong 3rd match)
    j = 0;
    while (j < full_dim) : (j += 1) {
        full_corpus[42 * full_dim + j] = query[j] * 0.7 + (if (j % 4 == 0) 0.5 else -0.2);
    }

    let coarse_bank: [1024]f32 = [0.0; 1024]; // 64 * 16
    let coarse_s: []f32 = coarse_bank[0..];
    expect_eq(buildCoarseEmbeddingsF32(full_corpus[0..], n_docs, full_dim, coarse_dim, coarse_s), true);

    const cfg = MatryoshkaConfig.init(full_dim, coarse_dim, coarse_top_k, final_top_k) ?? return error.InvalidConfig;
    let scratch: [12]MatryoshkaHit = [MatryoshkaHit{}; 12];
    let out_hits: [5]MatryoshkaHit = [MatryoshkaHit{}; 5];
    let scratch_s: []MatryoshkaHit = scratch[0..];
    let out_s: []MatryoshkaHit = out_hits[0..];

    const stats: MatryoshkaStats = searchMatryoshkaTwoStageF32(
        query[0..],
        coarse_bank[0..],
        full_corpus[0..],
        n_docs,
        cfg,
        scratch_s,
        out_s,
    ) ?? return error.SearchFailed;

    // Verify stats & compute speedup ratio
    expect_eq(stats.scanned_docs, 64);
    expect_eq(stats.coarse_candidates, 12);
    expect_eq(stats.returned_hits, 5);
    expect_eq(stats.coarse_dim, 16);
    expect_eq(stats.full_dim, 64);
    expect_eq(stats.coarse_macs, 1024);
    expect_eq(stats.fine_macs, 768);
    expect_eq(stats.brute_force_macs, 4096);
    // 4096 / (1024 + 768) = 4096 / 1792 = 2.285714...
    expect_eq(approxEqF32(stats.compute_speedup_ratio, 4096.0 / 1792.0, 0.001), true);
    expect_eq(stats.compute_speedup_ratio > 2.2, true);

    // Stage-2 fine rescore must rank doc 19 #1 ahead of doc 7 (even though doc 7 won Stage-1 coarse)
    expect_eq(out_hits[0].doc_idx, 19);
    expect_eq(out_hits[0].fine_score > out_hits[1].fine_score, true);

    // Cross-check against full-dim brute-force simdBatchTopKCosineF32
    let bf_ids: [5]usize = [0; 5];
    let bf_scores: [5]f32 = [0.0; 5];
    let bf_ids_s: []usize = bf_ids[0..];
    let bf_scores_s: []f32 = bf_scores[0..];
    const bf_n = simdBatchTopKCosineF32(
        query[0..],
        full_corpus[0..],
        n_docs,
        full_dim,
        final_top_k,
        bf_ids_s,
        bf_scores_s,
    ) ?? return error.BruteForceFailed;

    expect_eq(bf_n, 5);
    // Verify top-3 planted winners match brute-force ground truth identically in both doc_idx & score
    let r: usize = 0;
    while (r < 3) : (r += 1) {
        expect_eq(out_hits[r].doc_idx, bf_ids[r]);
        expect_eq(approxEqF32(out_hits[r].fine_score, bf_scores[r], 0.0001), true);
    }
    // Verify descending order of fine_score across all returned hits
    r = 1;
    while (r < final_top_k) : (r += 1) {
        expect_eq(out_hits[r - 1].fine_score >= out_hits[r].fine_score, true);
    }
}

// ============================================================================
// Test 4: Two-Stage @l2sq Retrieval vs Brute-Force simdBatchTopKL2F32
// ============================================================================

fn testTwoStageL2MatchesBruteForce()! {
    const n_docs: usize = 32;
    const full_dim: usize = 32;
    const coarse_dim: usize = 8;
    const coarse_top_k: usize = 8;
    const final_top_k: usize = 4;

    let full_corpus: [1024]f32 = [0.0; 1024]; // 32 * 32
    let coarse_bank: [256]f32 = [0.0; 256];   // 32 * 8 (unnormalized prefix for L2)
    let rng: u64 = 0xDEADBEEF11223344;

    let d: usize = 0;
    while (d < n_docs) : (d += 1) {
        const f_base = d * full_dim;
        const c_base = d * coarse_dim;
        let k: usize = 0;
        while (k < full_dim) : (k += 1) {
            const v = nextUnitF32(*rng) * 3.0;
            full_corpus[f_base + k] = v;
            if (k < coarse_dim) {
                coarse_bank[c_base + k] = v;
            }
        }
    }

    // Plant query close to doc 5 and doc 11
    let query: [32]f32 = [0.0; 32];
    let k: usize = 0;
    while (k < full_dim) : (k += 1) {
        query[k] = full_corpus[5 * full_dim + k] + 0.05;
        full_corpus[11 * full_dim + k] = query[k] + (if (k < coarse_dim) 0.01 else 0.4);
        if (k < coarse_dim) {
            coarse_bank[11 * coarse_dim + k] = full_corpus[11 * full_dim + k];
        }
    }

    const cfg = MatryoshkaConfig.init(full_dim, coarse_dim, coarse_top_k, final_top_k) ?? return error.InvalidConfig;
    let scratch: [8]MatryoshkaHit = [MatryoshkaHit{}; 8];
    let out_hits: [4]MatryoshkaHit = [MatryoshkaHit{}; 4];
    let scratch_s: []MatryoshkaHit = scratch[0..];
    let out_s: []MatryoshkaHit = out_hits[0..];

    const stats = searchMatryoshkaTwoStageL2F32(
        query[0..],
        coarse_bank[0..],
        full_corpus[0..],
        n_docs,
        cfg,
        scratch_s,
        out_s,
    ) ?? return error.L2SearchFailed;

    expect_eq(stats.returned_hits, 4);
    expect_eq(stats.compute_speedup_ratio, 2.0); // 1024 / (256 + 256) = 2.0

    // Compare against brute-force L2
    let bf_ids: [4]usize = [0; 4];
    let bf_dists: [4]f32 = [0.0; 4];
    let bf_ids_s: []usize = bf_ids[0..];
    let bf_dists_s: []f32 = bf_dists[0..];
    const bf_n = simdBatchTopKL2F32(query[0..], full_corpus[0..], n_docs, full_dim, final_top_k, bf_ids_s, bf_dists_s) ?? return error.BfFailed;
    expect_eq(bf_n, 4);

    // Doc 5 must be #1 (exact distance ~ sqrt(32 * 0.05^2) = 0.28284) and beat doc 11
    expect_eq(out_hits[0].doc_idx, 5);
    expect_eq(out_hits[0].doc_idx, bf_ids[0]);
    expect_eq(approxEqF32(out_hits[0].fine_score, bf_dists[0], 0.0001), true);
    expect_eq(out_hits[1].doc_idx, 11);
    expect_eq(out_hits[1].doc_idx, bf_ids[1]);
    expect_eq(approxEqF32(out_hits[1].fine_score, bf_dists[1], 0.0001), true);

    // Ascending order check
    let r: usize = 1;
    while (r < 4) : (r += 1) {
        expect_eq(out_hits[r - 1].fine_score <= out_hits[r].fine_score, true);
    }
}

// ============================================================================
// Test 5: Small Corpus, Degenerate Config & Deterministic Tie-Breaking
// ============================================================================

fn testSmallCorpusAndTieBreaking()! {
    // 3 identical documents -> tie-break must order by ascending doc_idx: 0, 1, 2
    const full_corpus: [24]f32 = [
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
        1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    ];
    let coarse_bank: [12]f32 = [0.0; 12]; // 3 * 4
    let coarse_s: []f32 = coarse_bank[0..];
    expect_eq(buildCoarseEmbeddingsF32(full_corpus[0..], 3, 8, 4, coarse_s), true);

    const query: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0];
    // Ask for coarse_top_k=5, final_top_k=4 when only 3 docs exist
    const cfg = MatryoshkaConfig.init(8, 4, 5, 4) ?? return error.InvalidConfig;
    let scratch: [5]MatryoshkaHit = [MatryoshkaHit{}; 5];
    let out_hits: [4]MatryoshkaHit = [MatryoshkaHit{}; 4];
    let scratch_s: []MatryoshkaHit = scratch[0..];
    let out_s: []MatryoshkaHit = out_hits[0..];

    const stats = searchMatryoshkaTwoStageF32(query[0..], coarse_bank[0..], full_corpus[0..], 3, cfg, scratch_s, out_s) ?? return error.SearchFailed;
    expect_eq(stats.scanned_docs, 3);
    expect_eq(stats.coarse_candidates, 3);
    expect_eq(stats.returned_hits, 3);
    expect_eq(out_hits[0].doc_idx, 0);
    expect_eq(out_hits[1].doc_idx, 1);
    expect_eq(out_hits[2].doc_idx, 2);
    expect_eq(approxEqF32(out_hits[0].fine_score, 1.0, 0.0001), true);

    // Empty corpus (n_docs == 0) returns 0 hits and speedup == 1.0
    const empty_stats = searchMatryoshkaTwoStageF32(query[0..], coarse_bank[0..0], full_corpus[0..0], 0, cfg, scratch_s, out_s) ?? return error.EmptyFailed;
    expect_eq(empty_stats.scanned_docs, 0);
    expect_eq(empty_stats.returned_hits, 0);
    expect_eq(approxEqF32(empty_stats.compute_speedup_ratio, 1.0, 0.0001), true);
}

// ============================================================================
// Test 6: Boundary & Invalid Input Guards
// ============================================================================

fn testBoundaryAndInvalidInputGuards()! {
    // 1. MatryoshkaConfig validation
    expect_eq(MatryoshkaConfig.init(0, 4, 4, 2) == null, true);   // full_dim == 0
    expect_eq(MatryoshkaConfig.init(8, 0, 4, 2) == null, true);   // coarse_dim == 0
    expect_eq(MatryoshkaConfig.init(8, 12, 4, 2) == null, true);  // coarse_dim > full_dim
    expect_eq(MatryoshkaConfig.init(8, 4, 4, 0) == null, true);   // final_top_k == 0
    expect_eq(MatryoshkaConfig.init(8, 4, 2, 4) == null, true);   // coarse_top_k < final_top_k

    // 2. truncateAndNormalizeF32 guards
    const valid_v: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    const zero_v: [4]f32 = [0.0, 0.0, 5.0, 6.0];
    const nan_v: [4]f32 = [std.math.nan(f32), 1.0, 2.0, 3.0];
    const inf_v: [4]f32 = [std.math.inf(f32), 1.0, 2.0, 3.0];
    let out_buf: [4]f32 = [0.0; 4];
    let out_s: []f32 = out_buf[0..];
    let small_out: [1]f32 = [0.0; 1];
    let small_out_s: []f32 = small_out[0..];

    expect_eq(truncateAndNormalizeF32(valid_v[0..], 0, out_s), false);      // target_dim == 0
    expect_eq(truncateAndNormalizeF32(valid_v[0..], 5, out_s), false);      // target_dim > full_vec.len
    expect_eq(truncateAndNormalizeF32(valid_v[0..], 2, small_out_s), false); // out_vec too small
    expect_eq(truncateAndNormalizeF32(zero_v[0..], 2, out_s), false);       // zero prefix vector
    expect_eq(truncateAndNormalizeF32(nan_v[0..], 2, out_s), false);        // NaN in prefix
    expect_eq(truncateAndNormalizeF32(inf_v[0..], 2, out_s), false);        // Inf in prefix

    // 3. buildCoarseEmbeddingsF32 guards
    const corpus_with_zero: [8]f32 = [1.0, 2.0, 3.0, 4.0, 0.0, 0.0, 5.0, 6.0];
    let coarse_out: [4]f32 = [0.0; 4];
    let coarse_out_s: []f32 = coarse_out[0..];
    expect_eq(buildCoarseEmbeddingsF32(corpus_with_zero[0..], 2, 4, 2, coarse_out_s), false); // 2nd doc has zero coarse prefix
    expect_eq(buildCoarseEmbeddingsF32(corpus_with_zero[0..], 2, 4, 5, coarse_out_s), false); // coarse_dim > full_dim
    expect_eq(buildCoarseEmbeddingsF32(corpus_with_zero[0..4], 2, 4, 2, coarse_out_s), false); // truncated input
    expect_eq(buildCoarseEmbeddingsF32(corpus_with_zero[0..], 2, 4, 2, coarse_out_s[0..2]), false); // truncated output

    // 4. searchMatryoshkaTwoStageF32 & L2 guards
    const ok_corpus: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0];
    let ok_coarse: [4]f32 = [0.0; 4];
    let ok_coarse_s: []f32 = ok_coarse[0..];
    expect_eq(buildCoarseEmbeddingsF32(ok_corpus[0..], 2, 4, 2, ok_coarse_s), true);

    const cfg = MatryoshkaConfig.init(4, 2, 2, 1) ?? return error.InvalidConfig;
    const bad_cfg = MatryoshkaConfig{ full_dim: 4, coarse_dim: 6, coarse_top_k: 2, final_top_k: 1 };
    let scratch: [2]MatryoshkaHit = [MatryoshkaHit{}; 2];
    let final_h: [1]MatryoshkaHit = [MatryoshkaHit{}; 1];
    let scratch_s: []MatryoshkaHit = scratch[0..];
    let final_s: []MatryoshkaHit = final_h[0..];
    const ok_q: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    // Invalid config
    expect_eq(searchMatryoshkaTwoStageF32(ok_q[0..], ok_coarse[0..], ok_corpus[0..], 2, bad_cfg, scratch_s, final_s) == null, true);
    // Query length mismatch
    expect_eq(searchMatryoshkaTwoStageF32(ok_q[0..3], ok_coarse[0..], ok_corpus[0..], 2, cfg, scratch_s, final_s) == null, true);
    // Zero coarse prefix query
    expect_eq(searchMatryoshkaTwoStageF32(zero_v[0..], ok_coarse[0..], ok_corpus[0..], 2, cfg, scratch_s, final_s) == null, true);
    // NaN query
    expect_eq(searchMatryoshkaTwoStageF32(nan_v[0..], ok_coarse[0..], ok_corpus[0..], 2, cfg, scratch_s, final_s) == null, true);
    // Undersized scratch buffer
    expect_eq(searchMatryoshkaTwoStageF32(ok_q[0..], ok_coarse[0..], ok_corpus[0..], 2, cfg, scratch_s[0..1], final_s) == null, true);
    // Undersized output buffer
    expect_eq(searchMatryoshkaTwoStageF32(ok_q[0..], ok_coarse[0..], ok_corpus[0..], 2, cfg, scratch_s, final_s[0..0]) == null, true);
    // Truncated coarse bank
    expect_eq(searchMatryoshkaTwoStageF32(ok_q[0..], ok_coarse[0..2], ok_corpus[0..], 2, cfg, scratch_s, final_s) == null, true);
    // Truncated full corpus
    expect_eq(searchMatryoshkaTwoStageF32(ok_q[0..], ok_coarse[0..], ok_corpus[0..4], 2, cfg, scratch_s, final_s) == null, true);
    // L2 NaN query guard
    expect_eq(searchMatryoshkaTwoStageL2F32(nan_v[0..], ok_coarse[0..], ok_corpus[0..], 2, cfg, scratch_s, final_s) == null, true);
}

test("Matryoshka truncateAndNormalizeF32 prefix truncation & L2 unit norm", testTruncateAndNormalizeF32);
test("Matryoshka buildCoarseEmbeddingsF32 batch compact coarse bank", testBuildCoarseEmbeddingsF32);
test("Matryoshka searchMatryoshkaTwoStageF32 rank flip, brute-force equivalence & speedup", testTwoStageCosineMatchesBruteForceAndSpeedup);
test("Matryoshka searchMatryoshkaTwoStageL2F32 @l2sq equivalence & speedup", testTwoStageL2MatchesBruteForce);
test("Matryoshka small corpus, empty corpus & deterministic tie-breaking", testSmallCorpusAndTieBreaking);
test("Matryoshka boundary & invalid input guards", testBoundaryAndInvalidInputGuards);
