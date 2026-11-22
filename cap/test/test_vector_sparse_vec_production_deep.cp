// Origin: Pure-CP Stdlib Test Suite (lib/vector/sparse_vec.cp) | Created: 2026-11-22
// Production Deep Verification Suite for SPLADE / BGE-M3 learned sparse lexical vectors:
// unsorted/duplicate/zero/NaN canonicalization, O(m+n) merge-join @fma dot & cosine,
// SPLADE Top-K magnitude pruning with term_id ordering, CSR batch Top-K retrieval, and defensive guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    SparseEntry,
    SparseHit,
    sparseNormalizeInPlace,
    sparseDot,
    sparseCosine,
    sparsePruneTopK,
    sparseBatchTopK,
} from "cap:vector";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

// ============================================================================
// Test 1: Unsorted / Duplicate / Zero / Negative / NaN Normalization
// ============================================================================

fn testSparseNormalizeInPlaceDedupSortAndFilter()! {
    let raw: [10]SparseEntry = [
        SparseEntry{ term_id: 50, weight: 1.25 },
        SparseEntry{ term_id: 10, weight: 2.0 },
        SparseEntry{ term_id: 30, weight: 0.0 },                 // zero -> drop
        SparseEntry{ term_id: 50, weight: 0.75 },                // dup 50 -> sum 2.0
        SparseEntry{ term_id: 20, weight: -1.5 },                // negative -> drop
        SparseEntry{ term_id: 10, weight: std.math.nan(f32) },   // NaN must not poison term 10
        SparseEntry{ term_id: 40, weight: 3.0 },
        SparseEntry{ term_id: 40, weight: -3.0 },                // cancels to 0.0 -> drop
        SparseEntry{ term_id: 5, weight: 0.5 },
        SparseEntry{ term_id: 70, weight: std.math.inf(f32) },   // Inf -> drop
    ];
    let raw_s: []SparseEntry = raw[0..];

    const n: usize = cap.vector.sparseNormalizeInPlace(raw_s);
    expect_eq(n, 3 as usize);

    // Expect strictly ascending term_id: (5, 0.5), (10, 2.0), (50, 2.0)
    expect_eq(raw[0].term_id, 5 as u32);
    expect_eq(absF32(raw[0].weight - 0.5) < 0.0001, true);

    expect_eq(raw[1].term_id, 10 as u32);
    expect_eq(absF32(raw[1].weight - 2.0) < 0.0001, true);

    expect_eq(raw[2].term_id, 50 as u32);
    expect_eq(absF32(raw[2].weight - 2.0) < 0.0001, true);
}

// ============================================================================
// Test 2: O(m+n) Two-Pointer Merge-Join Dot Product & Sparse Cosine
// ============================================================================

fn testSparseDotAndCosineMergeJoin()! {
    const a: [4]SparseEntry = [
        SparseEntry{ term_id: 2, weight: 1.0 },
        SparseEntry{ term_id: 5, weight: 2.0 },
        SparseEntry{ term_id: 9, weight: 3.0 },
        SparseEntry{ term_id: 20, weight: 4.0 },
    ];
    const b: [4]SparseEntry = [
        SparseEntry{ term_id: 1, weight: 5.0 },
        SparseEntry{ term_id: 5, weight: 3.0 },
        SparseEntry{ term_id: 12, weight: 7.0 },
        SparseEntry{ term_id: 20, weight: 0.5 },
    ];
    const disjoint: [2]SparseEntry = [
        SparseEntry{ term_id: 3, weight: 1.5 },
        SparseEntry{ term_id: 100, weight: 2.5 },
    ];

    // Overlap: term 5 (2.0 * 3.0 = 6.0) + term 20 (4.0 * 0.5 = 2.0) = 8.0
    const dot_ab: f32 = cap.vector.sparseDot(a[0..], b[0..]);
    expect_eq(absF32(dot_ab - 8.0) < 0.0001, true);

    // Commutativity
    const dot_ba: f32 = sparseDot(b[0..], a[0..]);
    expect_eq(absF32(dot_ba - 8.0) < 0.0001, true);

    // Disjoint dot & cosine == 0.0
    expect_eq(sparseDot(a[0..], disjoint[0..]), 0.0 as f32);
    expect_eq(sparseCosine(a[0..], disjoint[0..]), 0.0 as f32);

    // Self-cosine == 1.0
    const cos_aa: f32 = sparseCosine(a[0..], a[0..]);
    expect_eq(absF32(cos_aa - 1.0) < 0.0001, true);

    // Exact cosine(a, b): 8.0 / (sqrt(1+4+9+16) * sqrt(25+9+49+0.25)) = 8.0 / (sqrt(30) * sqrt(83.25))
    const expected_cos: f32 = 8.0 / (@sqrt(30.0 as f32) * @sqrt(83.25 as f32));
    const cos_ab: f32 = cap.vector.sparseCosine(a[0..], b[0..]);
    expect_eq(absF32(cos_ab - expected_cos) < 0.0002, true);
}

// ============================================================================
// Test 3: SPLADE Top-K Magnitude Pruning Preserving Ascending term_id Order
// ============================================================================

fn testSparsePruneTopKPreservesOrderAndTieBreak()! {
    const src: [8]SparseEntry = [
        SparseEntry{ term_id: 100, weight: 0.4 },
        SparseEntry{ term_id: 20, weight: 2.5 },
        SparseEntry{ term_id: 50, weight: -2.5 },               // |weight| == 2.5 (tie with 20, kept)
        SparseEntry{ term_id: 5, weight: 1.5 },                 // tie with 70 -> term 5 wins over 70!
        SparseEntry{ term_id: 70, weight: 1.5 },
        SparseEntry{ term_id: 10, weight: 4.0 },
        SparseEntry{ term_id: 80, weight: 0.0 },                // zero skipped
        SparseEntry{ term_id: 90, weight: std.math.nan(f32) },  // NaN skipped
    ];

    // Prune to Top-4 by |weight|: should keep term 10 (4.0), 20 (2.5), 50 (-2.5), 5 (1.5)
    // Output must be sorted ascending by term_id: 5, 10, 20, 50.
    let dst_buf: [6]SparseEntry = [SparseEntry{}; 6];
    let dst_s: []SparseEntry = dst_buf[0..];
    const kept: usize = sparsePruneTopK(src[0..], 4, dst_s);
    expect_eq(kept, 4 as usize);

    expect_eq(dst_buf[0].term_id, 5 as u32);
    expect_eq(absF32(dst_buf[0].weight - 1.5) < 0.0001, true);
    expect_eq(dst_buf[1].term_id, 10 as u32);
    expect_eq(absF32(dst_buf[1].weight - 4.0) < 0.0001, true);
    expect_eq(dst_buf[2].term_id, 20 as u32);
    expect_eq(absF32(dst_buf[2].weight - 2.5) < 0.0001, true);
    expect_eq(dst_buf[3].term_id, 50 as u32);
    expect_eq(absF32(dst_buf[3].weight - (-2.5)) < 0.0001, true);

    // Output buffer smaller than k clamps safely to out_dst.len.
    let small_buf: [2]SparseEntry = [SparseEntry{}; 2];
    let small_s: []SparseEntry = small_buf[0..];
    const kept2: usize = sparsePruneTopK(src[0..], 5, small_s);
    expect_eq(kept2, 2 as usize);
    expect_eq(small_buf[0].term_id, 10 as u32);
    expect_eq(small_buf[1].term_id, 20 as u32);
}

// ============================================================================
// Test 4: CSR Corpus Batch Top-K Retrieval & Deterministic Tie-Breaking
// ============================================================================

fn testSparseBatchTopKCsrCorpusRanking()! {
    const query: [3]SparseEntry = [
        SparseEntry{ term_id: 10, weight: 2.0 },
        SparseEntry{ term_id: 20, weight: 1.5 },
        SparseEntry{ term_id: 30, weight: 3.0 },
    ];

    // 6 documents in CSR layout:
    // doc 0: [(10, 1.0), (20, 2.0)]            -> score = 2.0*1.0 + 1.5*2.0 = 5.0
    // doc 1: [] (empty)                        -> score = 0.0 (excluded)
    // doc 2: [(30, 3.0)]                       -> score = 3.0*3.0 = 9.0
    // doc 3: [(10, 2.5)]                       -> score = 2.0*2.5 = 5.0 (tie with doc 0 -> doc 0 ahead)
    // doc 4: [(99, 10.0)] (disjoint)           -> score = 0.0 (excluded)
    // doc 5: [(10, 1.0), (20, 2.0), (30, 1.0)] -> score = 2.0 + 3.0 + 3.0 = 8.0
    const corpus: [8]SparseEntry = [
        SparseEntry{ term_id: 10, weight: 1.0 },
        SparseEntry{ term_id: 20, weight: 2.0 },
        SparseEntry{ term_id: 30, weight: 3.0 },
        SparseEntry{ term_id: 10, weight: 2.5 },
        SparseEntry{ term_id: 99, weight: 10.0 },
        SparseEntry{ term_id: 10, weight: 1.0 },
        SparseEntry{ term_id: 20, weight: 2.0 },
        SparseEntry{ term_id: 30, weight: 1.0 },
    ];
    const offsets: [7]usize = [0, 2, 2, 3, 4, 5, 8];

    // Top-3: doc 2 (9.0), doc 5 (8.0), doc 0 (5.0)
    let hits3: [3]SparseHit = [SparseHit{}; 3];
    let hits3_s: []SparseHit = hits3[0..];
    const n3: usize = cap.vector.sparseBatchTopK(query[0..], corpus[0..], offsets[0..], 3, hits3_s) ?? 0;
    expect_eq(n3, 3 as usize);
    expect_eq(hits3[0].doc_id, 2 as u32);
    expect_eq(absF32(hits3[0].score - 9.0) < 0.0001, true);
    expect_eq(hits3[1].doc_id, 5 as u32);
    expect_eq(absF32(hits3[1].score - 8.0) < 0.0001, true);
    expect_eq(hits3[2].doc_id, 0 as u32);
    expect_eq(absF32(hits3[2].score - 5.0) < 0.0001, true);

    // Top-6: only 4 matching docs returned (doc 2, doc 5, doc 0, doc 3)
    let hits6: [6]SparseHit = [SparseHit{}; 6];
    let hits6_s: []SparseHit = hits6[0..];
    const n6: usize = sparseBatchTopK(query[0..], corpus[0..], offsets[0..], 6, hits6_s) ?? 0;
    expect_eq(n6, 4 as usize);
    expect_eq(hits6[0].doc_id, 2 as u32);
    expect_eq(hits6[1].doc_id, 5 as u32);
    expect_eq(hits6[2].doc_id, 0 as u32);
    expect_eq(hits6[3].doc_id, 3 as u32);
    expect_eq(absF32(hits6[3].score - 5.0) < 0.0001, true);
}

// ============================================================================
// Test 5: Empty Vectors, Zero-Norm & Invalid CSR Offset Guards
// ============================================================================

fn testEmptyVectorsAndInvalidCsrOffsetGuards()! {
    let empty_buf: [1]SparseEntry = [SparseEntry{}];
    let empty_mut: []SparseEntry = empty_buf[0..0];
    const empty_const: []const SparseEntry = empty_buf[0..0];

    expect_eq(sparseNormalizeInPlace(empty_mut), 0 as usize);
    expect_eq(sparseDot(empty_const, empty_const), 0.0 as f32);
    expect_eq(sparseCosine(empty_const, empty_const), 0.0 as f32);

    const zero_vec: [2]SparseEntry = [
        SparseEntry{ term_id: 1, weight: 0.0 },
        SparseEntry{ term_id: 2, weight: 0.0 },
    ];
    expect_eq(sparseCosine(zero_vec[0..], zero_vec[0..]), 0.0 as f32);

    let out_prune_buf: [2]SparseEntry = [SparseEntry{}; 2];
    let out_prune_s: []SparseEntry = out_prune_buf[0..];
    expect_eq(sparsePruneTopK(empty_const, 2, out_prune_s), 0 as usize);
    expect_eq(sparsePruneTopK(zero_vec[0..], 2, out_prune_s), 0 as usize);
    expect_eq(sparsePruneTopK(zero_vec[0..], 0, out_prune_s), 0 as usize);

    // CSR batch Top-K defensive guards
    const valid_q: [1]SparseEntry = [SparseEntry{ term_id: 1, weight: 1.0 }];
    const corpus: [2]SparseEntry = [
        SparseEntry{ term_id: 1, weight: 2.0 },
        SparseEntry{ term_id: 2, weight: 3.0 },
    ];
    let hits_buf: [2]SparseHit = [SparseHit{}; 2];
    let hits_s: []SparseHit = hits_buf[0..];

    const empty_offsets_arr: [1]usize = [0];
    const empty_offsets: []const usize = empty_offsets_arr[0..0];
    const non_mono_offsets: [3]usize = [0, 2, 1];
    const oob_offsets: [3]usize = [0, 1, 5];
    const zero_docs_offsets: [1]usize = [0];
    const valid_offsets: [2]usize = [0, 2];

    // Invalid offsets & undersized out_hits return null.
    expect_eq(sparseBatchTopK(valid_q[0..], corpus[0..], empty_offsets, 1, hits_s) == null, true);
    expect_eq(sparseBatchTopK(valid_q[0..], corpus[0..], non_mono_offsets[0..], 1, hits_s) == null, true);
    expect_eq(sparseBatchTopK(valid_q[0..], corpus[0..], oob_offsets[0..], 1, hits_s) == null, true);
    expect_eq(sparseBatchTopK(valid_q[0..], corpus[0..], valid_offsets[0..], 3, hits_s) == null, true);

    // Empty query, zero docs, or k == 0 safely return 0.
    expect_eq(sparseBatchTopK(empty_const, corpus[0..], valid_offsets[0..], 2, hits_s) ?? 99, 0 as usize);
    expect_eq(sparseBatchTopK(valid_q[0..], corpus[0..], zero_docs_offsets[0..], 2, hits_s) ?? 99, 0 as usize);
    expect_eq(sparseBatchTopK(valid_q[0..], corpus[0..], valid_offsets[0..], 0, hits_s) ?? 99, 0 as usize);
}

fn main() {
    test("1. vector.sparse_vec: unsorted/duplicate/zero/NaN canonicalization", () => {
        testSparseNormalizeInPlaceDedupSortAndFilter()?;
    });
    test("2. vector.sparse_vec: O(m+n) merge-join @fma dot and sparse cosine", () => {
        testSparseDotAndCosineMergeJoin()?;
    });
    test("3. vector.sparse_vec: SPLADE Top-K magnitude pruning preserves term_id order", () => {
        testSparsePruneTopKPreservesOrderAndTieBreak()?;
    });
    test("4. vector.sparse_vec: CSR corpus batch Top-K ranking and tie-breaking", () => {
        testSparseBatchTopKCsrCorpusRanking()?;
    });
    test("5. vector.sparse_vec: empty vectors, zero-norm and invalid CSR offset guards", () => {
        testEmptyVectorsAndInvalidCsrOffsetGuards()?;
    });
}
