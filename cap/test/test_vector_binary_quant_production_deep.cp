// Origin: Pure-CP Stdlib Test Suite (1-Bit Binary Vector Quantization & Two-Stage Rescore) | Created: 2026-11-22
// Deep Production Verification Suite for lib/vector/binary_quant.cp
// Covers:
//   1. 128D, 1536D & non-64-multiple (100D) binary packing & exact popcount Hamming distance
//   2. Charikar binaryCosineApprox for identical, orthogonal & opposite vectors
//   3. searchBinaryTopK deterministic (hamming asc, index asc) ranking
//   4. 200-doc 128D corpus twoStageSearch (oversample_m=20, top_k=5) vs brute-force exact Top-5
//   5. dim == 0, buffer underflow, empty inputs & NaN/Inf boundary defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BinaryCandidate,
    RescoredHit,
    binaryWordsForDim,
    quantizeBinary,
    hammingDistanceWords,
    binaryCosineApprox,
    searchBinaryTopK,
    twoStageSearch,
} from "../../lib/vector/index.cp";

const EPS_F64: f64 = 0.0000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn testPackingAndHammingMultiDim()! {
    expect_eq(binaryWordsForDim(0), 0);
    expect_eq(binaryWordsForDim(1), 1);
    expect_eq(binaryWordsForDim(64), 1);
    expect_eq(binaryWordsForDim(65), 2);
    expect_eq(binaryWordsForDim(100), 2);
    expect_eq(binaryWordsForDim(128), 2);
    expect_eq(binaryWordsForDim(1536), 24);

    // 1. Non-64-multiple (100D): 27 flipped coordinates across both u64 words
    let a100: [100]f64 = [1.0; 100];
    let b100: [100]f64 = [1.0; 100];
    let i: usize = 0;
    while (i < 27) : (i += 1) {
        // Spread flips across word 0 and word 1 (indices 0, 3, 6, ..., 78)
        b100[i * 3] = -1.0;
    }
    let wa100_buf: [2]u64 = [999, 999]; // dirty initial values to verify internal zeroing
    let wb100_buf: [2]u64 = [999, 999];
    let wa100: []u64 = wa100_buf[0..];
    let wb100: []u64 = wb100_buf[0..];

    expect_eq(quantizeBinary(a100[0..], 0.0, wa100), true);
    expect_eq(quantizeBinary(b100[0..], 0.0, wb100), true);
    expect_eq(hammingDistanceWords(wa100_buf[0..], wb100_buf[0..]), 27);

    // 2. 1536D (24 u64 words): flip every 3rd coordinate -> exact 512 bits differ
    let a1536: [1536]f64 = [0.5; 1536];
    let b1536: [1536]f64 = [0.5; 1536];
    let k: usize = 0;
    while (k < 1536) : (k += 3) {
        b1536[k] = -0.5;
    }
    let wa1536_buf: [24]u64 = [0; 24];
    let wb1536_buf: [24]u64 = [0; 24];
    let wa1536: []u64 = wa1536_buf[0..];
    let wb1536: []u64 = wb1536_buf[0..];

    expect_eq(quantizeBinary(a1536[0..], 0.0, wa1536), true);
    expect_eq(quantizeBinary(b1536[0..], 0.0, wb1536), true);
    expect_eq(hammingDistanceWords(wa1536_buf[0..], wb1536_buf[0..]), 512);
}

fn testBinaryCosineApproxCharikar()! {
    expect_eq(approxF64(binaryCosineApprox(0, 128), 1.0, EPS_F64), true);
    expect_eq(approxF64(binaryCosineApprox(64, 128), 0.0, EPS_F64), true);
    expect_eq(approxF64(binaryCosineApprox(128, 128), -1.0, EPS_F64), true);
    expect_eq(approxF64(binaryCosineApprox(10, 0), 0.0, EPS_F64), true);
}

fn testSearchBinaryTopKDeterministic()! {
    // 6 single-word vectors compared against query = 0x0:
    //   doc 0: 10 bits set (0x3FF)
    //   doc 1:  2 bits set (0x3)
    //   doc 2:  5 bits set (0x1F)
    //   doc 3:  2 bits set (0xC)   -> ties with doc 1, index 1 < 3 wins
    //   doc 4:  0 bits set (0x0)   -> best
    //   doc 5:  5 bits set (0x3E0) -> ties with doc 2, index 2 < 5 wins
    const q: [1]u64 = [0];
    const corpus: [6]u64 = [0x3FF, 0x3, 0x1F, 0xC, 0x0, 0x3E0];
    let out_buf: [4]BinaryCandidate = [BinaryCandidate{}; 4];
    let out: []BinaryCandidate = out_buf[0..];

    const n_opt = searchBinaryTopK(q[0..], corpus[0..], 6, 1, 4, out);
    expect_eq(n_opt != null, true);
    expect_eq(n_opt ?? 0, 4);

    expect_eq(out_buf[0].index, 4);
    expect_eq(out_buf[0].hamming, 0);
    expect_eq(out_buf[1].index, 1);
    expect_eq(out_buf[1].hamming, 2);
    expect_eq(out_buf[2].index, 3);
    expect_eq(out_buf[2].hamming, 2);
    expect_eq(out_buf[3].index, 2);
    expect_eq(out_buf[3].hamming, 5);
}

fn exactCosine128(a: []const f64, b: []const f64): f64 {
    let dot: f64 = 0.0;
    let na: f64 = 0.0;
    let nb: f64 = 0.0;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        dot += a[i] * b[i];
        na += a[i] * a[i];
        nb += b[i] * b[i];
    }
    return dot / (std.math.sqrt(na) * std.math.sqrt(nb));
}

fn testTwoStageSearch200Docs128D()! {
    const N_DOCS: usize = 200;
    const DIM: usize = 128;
    const WORDS: usize = 2;

    let query: [128]f64 = [0.0; 128];
    let d: usize = 0;
    while (d < DIM) : (d += 1) {
        const fd = d as f64;
        query[d] = std.math.sin(fd * 0.23 + 0.4) + 0.5 * std.math.cos(fd * 0.13);
    }

    let q_words_buf: [2]u64 = [0; 2];
    let q_words: []u64 = q_words_buf[0..];
    expect_eq(quantizeBinary(query[0..], 0.0, q_words), true);

    // Allocate 200 * 128 f64 corpus and 200 * 2 u64 binary corpus on stack (zero heap!)
    let corpus_fp: [25600]f64 = [0.0; 25600];
    let corpus_words: [400]u64 = [0; 400];

    let doc: usize = 0;
    while (doc < N_DOCS) : (doc += 1) {
        const base = doc * DIM;
        const fdoc = (doc + 1) as f64;
        let j: usize = 0;
        while (j < DIM) : (j += 1) {
            const fj = j as f64;
            if (doc == 17) {
                // Planted #1 nearest neighbor (almost identical to query)
                corpus_fp[base + j] = query[j] + 0.02 * std.math.sin(fj * 1.7);
            } else if (doc == 63) {
                // Planted #2
                corpus_fp[base + j] = query[j] + 0.12 * std.math.cos(fj * 2.1);
            } else if (doc == 105) {
                // Planted #3
                corpus_fp[base + j] = query[j] + 0.22 * std.math.sin(fj * 2.9);
            } else if (doc == 142) {
                // Planted #4
                corpus_fp[base + j] = query[j] + 0.32 * std.math.cos(fj * 3.3);
            } else if (doc == 188) {
                // Planted #5
                corpus_fp[base + j] = query[j] + 0.42 * std.math.sin(fj * 4.1);
            } else {
                // Diverse background documents
                corpus_fp[base + j] = std.math.sin(fj * (0.71 + 0.031 * fdoc) + fdoc * 1.13)
                    - std.math.cos(fj * (0.43 + 0.019 * fdoc) - fdoc * 0.67);
            }
        }
        let w_slice: []u64 = corpus_words[doc * WORDS .. (doc + 1) * WORDS];
        expect_eq(quantizeBinary(corpus_fp[base .. base + DIM], 0.0, w_slice), true);
    }

    // Brute-force exact Top-1 for reference
    let exact_best_idx: usize = 0;
    let exact_best_cos: f64 = -2.0;
    let bdoc: usize = 0;
    while (bdoc < N_DOCS) : (bdoc += 1) {
        const base = bdoc * DIM;
        const c = exactCosine128(query[0..], corpus_fp[base .. base + DIM]);
        if (c > exact_best_cos) {
            exact_best_cos = c;
            exact_best_idx = bdoc;
        }
    }

    // Two-stage search: oversample_m = 20, top_k = 5
    let scratch_buf: [20]BinaryCandidate = [BinaryCandidate{}; 20];
    let hits_buf: [5]RescoredHit = [RescoredHit{}; 5];
    let scratch: []BinaryCandidate = scratch_buf[0..];
    let hits: []RescoredHit = hits_buf[0..];

    const k_opt = twoStageSearch(
        query[0..],
        q_words_buf[0..],
        corpus_words[0..],
        corpus_fp[0..],
        N_DOCS,
        DIM,
        20,
        5,
        scratch,
        hits,
    );
    expect_eq(k_opt != null, true);
    expect_eq(k_opt ?? 0, 5);

    // Top-1 must match exact brute-force Top-1 (doc 17)
    expect_eq(hits_buf[0].index, exact_best_idx);
    expect_eq(hits_buf[0].index, 17);
    expect_eq(approxF64(hits_buf[0].cosine, exact_best_cos, EPS_F64), true);

    // All 5 planted neighbors recovered in exact descending cosine order
    expect_eq(hits_buf[1].index, 63);
    expect_eq(hits_buf[2].index, 105);
    expect_eq(hits_buf[3].index, 142);
    expect_eq(hits_buf[4].index, 188);
    expect_eq(hits_buf[0].cosine > hits_buf[1].cosine, true);
    expect_eq(hits_buf[1].cosine > hits_buf[2].cosine, true);
    expect_eq(hits_buf[2].cosine > hits_buf[3].cosine, true);
    expect_eq(hits_buf[3].cosine > hits_buf[4].cosine, true);
}

fn testBoundaryAndMalformedInputDefenses()! {
    const nan_v = std.math.nan(f64);
    const inf_v = std.math.inf(f64);

    const v2: [2]f64 = [1.0, -1.0];
    const v_nan: [2]f64 = [1.0, nan_v];
    const v_empty: []const f64 = v2[0..0];
    let w1_buf: [1]u64 = [0];
    let w1: []u64 = w1_buf[0..];
    let w0: []u64 = w1_buf[0..0];

    // 1. quantizeBinary defenses
    expect_eq(quantizeBinary(v_empty, 0.0, w1), false);
    expect_eq(quantizeBinary(v2[0..], 0.0, w0), false);
    expect_eq(quantizeBinary(v2[0..], nan_v, w1), false);
    expect_eq(quantizeBinary(v2[0..], inf_v, w1), false);
    expect_eq(quantizeBinary(v_nan[0..], 0.0, w1), false);

    // 2. searchBinaryTopK defenses
    const qw: [1]u64 = [1];
    const cw: [2]u64 = [1, 2];
    let c_buf: [2]BinaryCandidate = [BinaryCandidate{}; 2];
    let c2: []BinaryCandidate = c_buf[0..];
    let c1: []BinaryCandidate = c_buf[0..1];

    expect_eq(searchBinaryTopK(qw[0..], cw[0..], 0, 1, 2, c2), 0);
    expect_eq(searchBinaryTopK(qw[0..], cw[0..], 2, 1, 0, c2), 0);
    expect_eq(searchBinaryTopK(qw[0..], cw[0..], 2, 0, 2, c2) == null, true);
    expect_eq(searchBinaryTopK(qw[0..], cw[0..], 2, 1, 2, c1) == null, true);

    // 3. twoStageSearch defenses
    const cfp: [4]f64 = [1.0, 0.0, 0.0, 1.0];
    const cfp_nan: [4]f64 = [1.0, nan_v, 0.0, 1.0];
    let h_buf: [2]RescoredHit = [RescoredHit{}; 2];
    let h2: []RescoredHit = h_buf[0..];
    let h1: []RescoredHit = h_buf[0..1];

    expect_eq(twoStageSearch(v2[0..], qw[0..], cw[0..], cfp[0..], 0, 2, 2, 2, c2, h2), 0);
    expect_eq(twoStageSearch(v2[0..], qw[0..], cw[0..], cfp[0..], 2, 2, 2, 0, c2, h2), 0);
    expect_eq(twoStageSearch(v2[0..], qw[0..], cw[0..], cfp[0..], 2, 0, 2, 2, c2, h2) == null, true);
    expect_eq(twoStageSearch(v_nan[0..], qw[0..], cw[0..], cfp[0..], 2, 2, 2, 2, c2, h2) == null, true);
    expect_eq(twoStageSearch(v2[0..], qw[0..], cw[0..], cfp_nan[0..], 2, 2, 2, 2, c2, h2) == null, true);
    expect_eq(twoStageSearch(v2[0..], qw[0..], cw[0..], cfp[0..], 2, 2, 2, 2, c1, h2) == null, true);
    expect_eq(twoStageSearch(v2[0..], qw[0..], cw[0..], cfp[0..], 2, 2, 2, 2, c2, h1) == null, true);
}

fn main() {
    test("1. binary quant: 100D/128D/1536D bit packing & exact popcount Hamming", () => {
        testPackingAndHammingMultiDim()?;
    });
    test("2. binary quant: Charikar binaryCosineApprox (identical/orthogonal/opposite)", () => {
        testBinaryCosineApproxCharikar()?;
    });
    test("3. binary quant: searchBinaryTopK deterministic (hamming asc, index asc)", () => {
        testSearchBinaryTopKDeterministic()?;
    });
    test("4. binary quant: 200-doc 128D twoStageSearch (M=20, K=5) matches exact Top-5", () => {
        testTwoStageSearch200Docs128D()?;
    });
    test("5. binary quant: dim=0, buffer underflow, empty & NaN/Inf defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
