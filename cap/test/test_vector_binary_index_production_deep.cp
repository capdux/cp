// Origin: Pure-CP Stdlib Test Suite (lib/vector/binary_index.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc 1-Bit Binary Quantized Vector Index:
//   1. 64D, 256D & 1024D cross-word 1-bit quantization packing & @popCount Hamming accuracy
//   2. Opposite-sign vector boundary (hamming_dist == dim, approx_cosine_f32 == -1.0) & identical (0, +1.0)
//   3. Stage 1 Hamming Top-K sorting & deterministic tie-breaking by doc_id ascending
//   4. Stage 2 @dot SIMD Float32 rescoring promoting coarse runner-up to exact Rank #1
//   5. remove_document exclusion & inline slot reuse
//   6. 32x compression ratio stats verification across 64D / 256D / 1024D
//   7. Invalid dim (0, >1024), duplicate ID, NaN/Inf non-finite guard & 256-slot full pool guard

import { test, expect_eq } from "cap:test";
import {
    BIN_IDX_MAX_DOCS,
    BIN_IDX_MAX_WORDS,
    BinaryIndexConfig,
    BinaryIndexHit,
    BinaryIndexStats,
    BinaryVectorIndex,
} from "cap:vector";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff: f32 = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

// ============================================================================
// Test 1: 64D, 256D & 1024D Cross-Word Quantization & @popCount Hamming Accuracy
// ============================================================================
test("binary_index: 64D/256D/1024D cross-word quantization & Hamming accuracy", () => {
    expect_eq(BIN_IDX_MAX_DOCS, 256);
    expect_eq(BIN_IDX_MAX_WORDS, 16);

    // 1024-D (16 words) cross-word flip test
    let idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 1024, threshold_f32: 0.0 }) ?? return;

    let q: [1024]f32 = [1.0; 1024];
    let d: [1024]f32 = [1.0; 1024];
    // Flip 5 specific bits spanning word 0, word 1, word 4, word 15
    d[0] = -1.0;
    d[63] = -1.0;
    d[64] = -1.0;
    d[255] = -1.0;
    d[1023] = -1.0;

    expect_eq(idx.add_document(100, d[0..]), true);

    let hits: [2]BinaryIndexHit = [BinaryIndexHit{}; 2];
    let hits_s: []BinaryIndexHit = hits[0..];
    expect_eq(idx.search_hamming_topk(q[0..], 2, hits_s), 1);
    expect_eq(hits[0].doc_id, 100);
    expect_eq(hits[0].hamming_dist, 5);

    // 256-D (4 words) exact 3-bit flip across words
    let idx256 = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 256, threshold_f32: 0.0 }) ?? return;
    let q256: [256]f32 = [0.5; 256];
    let d256: [256]f32 = [0.5; 256];
    d256[1] = -0.2;
    d256[127] = -0.2;
    d256[128] = -0.2;
    expect_eq(idx256.add_document(200, d256[0..]), true);
    expect_eq(idx256.search_hamming_topk(q256[0..], 1, hits_s), 1);
    expect_eq(hits[0].hamming_dist, 3);
});

// ============================================================================
// Test 2: Opposite-Sign Vector (hamming == dim, approx_cosine == -1.0)
// ============================================================================
test("binary_index: opposite-sign vector yields hamming == dim & approx_cosine == -1.0", () => {
    let idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 64, threshold_f32: 0.0 }) ?? return;

    let pos: [64]f32 = [1.0; 64];
    let neg: [64]f32 = [-1.0; 64];

    expect_eq(idx.add_document(1, pos[0..]), true);
    expect_eq(idx.add_document(2, neg[0..]), true);

    let hits: [2]BinaryIndexHit = [BinaryIndexHit{}; 2];
    let hits_s: []BinaryIndexHit = hits[0..];
    expect_eq(idx.search_hamming_topk(pos[0..], 2, hits_s), 2);

    expect_eq(hits[0].doc_id, 1);
    expect_eq(hits[0].hamming_dist, 0);
    expect_eq(approxEqF32(hits[0].approx_cosine_f32, 1.0, 0.0001), true);

    expect_eq(hits[1].doc_id, 2);
    expect_eq(hits[1].hamming_dist, 64);
    expect_eq(approxEqF32(hits[1].approx_cosine_f32, -1.0, 0.0001), true);
});

// ============================================================================
// Test 3: Stage 1 Hamming Top-K Sorting & Deterministic doc_id Tie-Breaking
// ============================================================================
test("binary_index: Stage 1 Hamming Top-K ordering & doc_id ascending tie-break", () => {
    let idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 16, threshold_f32: 0.0 }) ?? return;

    let q: [16]f32 = [1.0; 16];

    // Insert in reverse doc_id order with ties at hamming_dist == 2
    let d30: [16]f32 = [1.0; 16];
    d30[0] = -1.0; d30[1] = -1.0; // dist = 2

    let d10: [16]f32 = [1.0; 16];
    d10[2] = -1.0; d10[3] = -1.0; // dist = 2

    let d20: [16]f32 = [1.0; 16];
    d20[4] = -1.0; d20[5] = -1.0; // dist = 2

    let d50: [16]f32 = [1.0; 16];
    d50[0] = -1.0; // dist = 1 (best)

    expect_eq(idx.add_document(30, d30[0..]), true);
    expect_eq(idx.add_document(10, d10[0..]), true);
    expect_eq(idx.add_document(20, d20[0..]), true);
    expect_eq(idx.add_document(50, d50[0..]), true);

    let hits: [3]BinaryIndexHit = [BinaryIndexHit{}; 3];
    let hits_s: []BinaryIndexHit = hits[0..];
    expect_eq(idx.search_hamming_topk(q[0..], 3, hits_s), 3);

    // Rank 0: doc 50 (dist 1); Rank 1: doc 10 (dist 2); Rank 2: doc 20 (dist 2)
    expect_eq(hits[0].doc_id, 50);
    expect_eq(hits[0].hamming_dist, 1);
    expect_eq(hits[1].doc_id, 10);
    expect_eq(hits[1].hamming_dist, 2);
    expect_eq(hits[2].doc_id, 20);
    expect_eq(hits[2].hamming_dist, 2);
});

// ============================================================================
// Test 4: Stage 2 @dot SIMD Rescoring Promotes Coarse Runner-Up to Rank #1
// ============================================================================
test("binary_index: Stage 2 @dot rescoring promotes true inner-product winner to Rank #1", () => {
    let idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 8, threshold_f32: 0.0 }) ?? return;

    const query: [8]f32 = [2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 0.1, 0.1];

    // Flat raw_doc_pool for 3 slots (3 * 8 = 24 floats):
    // Slot 0 (doc 101): all positive -> Hamming dist 0, but tiny magnitudes => @dot ~ 1.22
    // Slot 1 (doc 202): 2 tiny dims flipped -> Hamming dist 2 (coarse #2), huge aligned dims => @dot ~ 59.98!
    // Slot 2 (doc 303): all negative -> Hamming dist 8 => @dot < 0
    const raw_pool: [24]f32 = [
        0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1,
        5.0, 5.0, 5.0, 5.0, 5.0, 5.0, -0.1, -0.1,
        -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0, -1.0,
    ];

    expect_eq(idx.add_document(101, raw_pool[0..8]), true);
    expect_eq(idx.add_document(202, raw_pool[8..16]), true);
    expect_eq(idx.add_document(303, raw_pool[16..24]), true);

    // Stage 1 alone ranks doc 101 first (Hamming 0 < Hamming 2)
    let coarse: [2]BinaryIndexHit = [BinaryIndexHit{}; 2];
    let coarse_s: []BinaryIndexHit = coarse[0..];
    expect_eq(idx.search_hamming_topk(query[0..], 2, coarse_s), 2);
    expect_eq(coarse[0].doc_id, 101);
    expect_eq(coarse[1].doc_id, 202);

    // Stage 2 with oversample_k=3 uses @dot SIMD and promotes doc 202 to exact Rank #1!
    let rescored: [2]BinaryIndexHit = [BinaryIndexHit{}; 2];
    let rescored_s: []BinaryIndexHit = rescored[0..];
    expect_eq(idx.search_two_stage_rescore(query[0..], raw_pool[0..], 2, 3, rescored_s), 2);
    expect_eq(rescored[0].doc_id, 202);
    expect_eq(rescored[0].hamming_dist, 2);
    expect_eq(rescored[0].exact_score_f32 > 50.0, true);
    expect_eq(rescored[1].doc_id, 101);
    expect_eq(rescored[1].exact_score_f32 < 2.0, true);
});

// ============================================================================
// Test 5: remove_document Exclusion & Slot Reuse
// ============================================================================
test("binary_index: remove_document filters deleted doc and reuses slot", () => {
    let idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 8, threshold_f32: 0.0 }) ?? return;

    const v1: [8]f32 = [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0];
    const v2: [8]f32 = [1.0, 1.0, 1.0, 1.0, -1.0, -1.0, -1.0, -1.0];

    expect_eq(idx.add_document(1, v1[0..]), true);
    expect_eq(idx.add_document(2, v2[0..]), true);
    expect_eq(idx.stats().active_docs, 2);

    expect_eq(idx.remove_document(1), true);
    expect_eq(idx.remove_document(1), false); // already removed
    expect_eq(idx.stats().active_docs, 1);

    let hits: [2]BinaryIndexHit = [BinaryIndexHit{}; 2];
    let hits_s: []BinaryIndexHit = hits[0..];
    expect_eq(idx.search_hamming_topk(v1[0..], 2, hits_s), 1);
    expect_eq(hits[0].doc_id, 2);

    // Re-add doc 1 cleanly into freed slot
    expect_eq(idx.add_document(1, v1[0..]), true);
    expect_eq(idx.stats().active_docs, 2);
});

// ============================================================================
// Test 6: 32x Compression Ratio Stats Verification
// ============================================================================
test("binary_index: 32x memory compression ratio stats across 64D/256D/1024D", () => {
    let i64_idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 64, threshold_f32: 0.0 }) ?? return;
    const s64: BinaryIndexStats = i64_idx.stats();
    expect_eq(s64.words_per_vec, 1);
    expect_eq(s64.bytes_per_vec, 8);
    expect_eq(approxEqF32(s64.compression_ratio_x, 32.0, 0.001), true);

    let i256_idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 256, threshold_f32: 0.0 }) ?? return;
    const s256 = i256_idx.stats();
    expect_eq(s256.words_per_vec, 4);
    expect_eq(s256.bytes_per_vec, 32);
    expect_eq(approxEqF32(s256.compression_ratio_x, 32.0, 0.001), true);

    let i1024_idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 1024, threshold_f32: 0.0 }) ?? return;
    const s1024 = i1024_idx.stats();
    expect_eq(s1024.words_per_vec, 16);
    expect_eq(s1024.bytes_per_vec, 128);
    expect_eq(approxEqF32(s1024.compression_ratio_x, 32.0, 0.001), true);
});

// ============================================================================
// Test 7: Invalid Dim, Duplicate ID, Non-Finite Values & 256-Slot Full Pool
// ============================================================================
test("binary_index: invalid dim, duplicate ID, NaN/Inf & 256-slot overflow guards", () => {
    expect_eq(BinaryVectorIndex.init(BinaryIndexConfig{ dim: 0, threshold_f32: 0.0 }) == null, true);
    expect_eq(BinaryVectorIndex.init(BinaryIndexConfig{ dim: 1025, threshold_f32: 0.0 }) == null, true);

    let idx = BinaryVectorIndex.init(BinaryIndexConfig{ dim: 4, threshold_f32: 0.0 }) ?? return;

    // Dimension mismatch rejected
    const wrong_dim: [3]f32 = [1.0, 2.0, 3.0];
    expect_eq(idx.add_document(1, wrong_dim[0..]), false);

    // Non-finite NaN / Inf rejected
    const zero_f: f32 = 0.0;
    const one_f: f32 = 1.0;
    const nan_vec: [4]f32 = [1.0, zero_f / zero_f, 1.0, 1.0];
    const inf_vec: [4]f32 = [1.0, one_f / zero_f, 1.0, 1.0];
    expect_eq(idx.add_document(1, nan_vec[0..]), false);
    expect_eq(idx.add_document(1, inf_vec[0..]), false);
    expect_eq(idx.stats().active_docs, 0);

    // Duplicate ID rejected
    const ok_vec: [4]f32 = [1.0, -1.0, 1.0, -1.0];
    expect_eq(idx.add_document(1, ok_vec[0..]), true);
    expect_eq(idx.add_document(1, ok_vec[0..]), false);

    // Fill remaining 255 slots up to BIN_IDX_MAX_DOCS (256)
    let id: u64 = 2;
    while (id <= BIN_IDX_MAX_DOCS as u64) : (id += 1) {
        expect_eq(idx.add_document(id, ok_vec[0..]), true);
    }
    expect_eq(idx.stats().active_docs, 256);

    // 257th document rejected cleanly
    expect_eq(idx.add_document(999, ok_vec[0..]), false);
    expect_eq(idx.stats().active_docs, 256);
});
