// Origin: Pure-CP Stdlib Test Suite (lib/vector/quant_sq4.cp) | Created: 2026-11-22
// Production Deep Verification Suite for 4-bit Asymmetric Scalar Quantization (SQ4):
// odd/even nibble packing & quantization error bounds, constant-dim zero-division guard,
// asymmetric L2 squared distance equivalence vs decoded @l2sq, 64D batch Top-K recall, and guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Sq4Codebook,
    sq4PackedBytes,
    sq4FitCodebook,
    sq4EncodeVector,
    sq4DecodeVector,
    sq4AsymmetricL2Sq,
    sq4BatchTopK,
    computeGroundTruthL2TopK,
    annRecallAtK,
} from "cap:vector";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

fn nextRandU64(state: *u64): u64 {
    const c_add: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
    const c_m1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
    const c_m2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
    state.* = state.* +% c_add;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% c_m1;
    z = (z ^ (z >> 27)) *% c_m2;
    return z ^ (z >> 31);
}

fn nextRandUnitF32(state: *u64): f32 {
    const r: u64 = nextRandU64(state) >> 40;
    const u: f32 = (r as f32) / 16777216.0;
    return (u * 2.0) - 1.0;
}

// ============================================================================
// Test 1: Odd & Even Dimension Nibble Packing & Uniform Quantization Error Bound
// ============================================================================

fn testOddEvenNibblePackingAndQuantErrorBound()! {
    expect_eq(sq4PackedBytes(0), 0 as usize);
    expect_eq(sq4PackedBytes(1), 1 as usize);
    expect_eq(sq4PackedBytes(7), 4 as usize);
    expect_eq(sq4PackedBytes(8), 4 as usize);
    expect_eq(sq4PackedBytes(64), 32 as usize);

    // Odd dimension (dim = 7) across 3 vectors.
    const dim_odd: usize = 7;
    const corpus_odd: [21]f32 = [
        -3.0, 0.0, 1.0, -5.0, 2.5, 10.0, -1.0,
         3.0, 15.0, 4.0, 5.0, 7.5, 25.0,  2.0,
         0.2, 7.3, 2.7, -1.1, 4.8, 17.4,  0.6,
    ];
    let mins: [7]f32 = [0.0; 7];
    let scales: [7]f32 = [0.0; 7];
    let mins_s: []f32 = mins[0..];
    let scales_s: []f32 = scales[0..];

    expect_eq(cap.vector.sq4FitCodebook(corpus_odd[0..], 3, dim_odd, mins_s, scales_s), true);
    if (cb := Sq4Codebook.init(mins_s, scales_s, dim_odd)) {
        expect_eq(cb.dim, dim_odd);
    } else {
        expect_eq(true, false);
    }

    let packed_buf: [4]u8 = [0xFF; 4];
    let packed_s: []u8 = packed_buf[0..];
    const target_vec: []const f32 = corpus_odd[14..21];
    expect_eq(sq4EncodeVector(target_vec, mins[0..], scales[0..], packed_s), true);
    // High nibble of final byte for odd dim must be cleanly zeroed.
    expect_eq((packed_buf[3] >> 4), 0 as u8);

    let decoded: [7]f32 = [0.0; 7];
    let decoded_s: []f32 = decoded[0..];
    expect_eq(sq4DecodeVector(packed_buf[0..], mins[0..], scales[0..], decoded_s), true);

    // Uniform 4-bit rounding error per coordinate is strictly <= 0.5 * scale + eps.
    let d: usize = 0;
    while (d < dim_odd) : (d += 1) {
        const err: f32 = absF32(decoded[d] - target_vec[d]);
        expect_eq(err <= (0.5 * scales[d]) + 0.0001, true);
    }

    // Even dimension (dim = 8) roundtrip check.
    const dim_even: usize = 8;
    const corpus_even: [16]f32 = [
        0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0,
        15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 21.0, 22.0,
    ];
    let mins8: [8]f32 = [0.0; 8];
    let scales8: [8]f32 = [0.0; 8];
    let mins8_s: []f32 = mins8[0..];
    let scales8_s: []f32 = scales8[0..];
    expect_eq(sq4FitCodebook(corpus_even[0..], 2, dim_even, mins8_s, scales8_s), true);

    let packed8: [4]u8 = [0; 4];
    let packed8_s: []u8 = packed8[0..];
    expect_eq(sq4EncodeVector(corpus_even[8..16], mins8[0..], scales8[0..], packed8_s), true);
    // All max endpoints map to nibble 15 -> every byte == 0xFF!
    expect_eq(packed8[0], 0xFF as u8);
    expect_eq(packed8[3], 0xFF as u8);
}

// ============================================================================
// Test 2: Constant Dimension (min == max) Zero-Division Guard
// ============================================================================

fn testConstantDimMinEqMaxZeroDivGuard()! {
    const dim: usize = 4;
    const corpus: [12]f32 = [
        3.5, -2.0, -7.0, 1.0,
        3.5,  4.0, -7.0, 5.0,
        3.5,  1.0, -7.0, 9.0,
    ];
    let mins: [4]f32 = [0.0; 4];
    let scales: [4]f32 = [0.0; 4];
    let mins_s: []f32 = mins[0..];
    let scales_s: []f32 = scales[0..];

    expect_eq(sq4FitCodebook(corpus[0..], 3, dim, mins_s, scales_s), true);
    expect_eq(scales[0], 0.0 as f32);
    expect_eq(scales[2], 0.0 as f32);
    expect_eq(scales[1] > 0.0, true);
    expect_eq(scales[3] > 0.0, true);

    let packed_buf: [2]u8 = [0; 2];
    let packed_s: []u8 = packed_buf[0..];
    expect_eq(sq4EncodeVector(corpus[4..8], mins[0..], scales[0..], packed_s), true);

    let decoded: [4]f32 = [0.0; 4];
    let decoded_s: []f32 = decoded[0..];
    expect_eq(sq4DecodeVector(packed_buf[0..], mins[0..], scales[0..], decoded_s), true);

    // Constant dimensions reconstruct with zero error and zero NaN.
    expect_eq(absF32(decoded[0] - 3.5) < 0.00001, true);
    expect_eq(absF32(decoded[2] - (-7.0)) < 0.00001, true);
}

// ============================================================================
// Test 3: Asymmetric L2 Squared Distance Equivalence vs Decoded @l2sq
// ============================================================================

fn testAsymmetricL2SqMatchesDecodedSimdL2Sq()! {
    const dim: usize = 19; // Odd non-multiple of 8
    let rng: u64 = 0x13579BDF2468ACE0;

    let corpus: [190]f32 = [0.0; 190]; // 10 docs * 19 dims
    let i: usize = 0;
    while (i < 190) : (i += 1) {
        corpus[i] = nextRandUnitF32(&rng) * 6.0;
    }

    let mins: [19]f32 = [0.0; 19];
    let scales: [19]f32 = [0.0; 19];
    let mins_s: []f32 = mins[0..];
    let scales_s: []f32 = scales[0..];
    expect_eq(sq4FitCodebook(corpus[0..], 10, dim, mins_s, scales_s), true);

    let query: [19]f32 = [0.0; 19];
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        query[d] = nextRandUnitF32(&rng) * 5.0;
    }

    let packed_buf: [10]u8 = [0; 10]; // sq4PackedBytes(19) == 10
    let packed_s: []u8 = packed_buf[0..];
    let decoded: [19]f32 = [0.0; 19];
    let decoded_s: []f32 = decoded[0..];

    let doc: usize = 0;
    while (doc < 10) : (doc += 1) {
        const v: []const f32 = corpus[doc * dim .. (doc + 1) * dim];
        expect_eq(sq4EncodeVector(v, mins[0..], scales[0..], packed_s), true);
        expect_eq(sq4DecodeVector(packed_buf[0..], mins[0..], scales[0..], decoded_s), true);

        const ref_l2sq: f32 = @l2sq(query[0..], decoded[0..]);
        if (asym := cap.vector.sq4AsymmetricL2Sq(query[0..], packed_buf[0..], mins[0..], scales[0..])) {
            expect_eq(absF32(asym - ref_l2sq) < 0.001, true);
        } else {
            expect_eq(true, false);
        }
    }
}

// ============================================================================
// Test 4: SQ4 Batch Top-K High Recall on 64D Clustered Corpus (8x Compression)
// ============================================================================

fn testSq4BatchTopKHighRecallOn64DClusters()! {
    const dim: usize = 64;
    const n_clusters: usize = 4;
    const per_cluster: usize = 16;
    const n_docs: usize = 64;
    const stride: usize = 32; // sq4PackedBytes(64) == 32 (8x smaller than 256B f32)

    let corpus: [4096]f32 = [0.0; 4096]; // 64 * 64
    let centers: [256]f32 = [0.0; 256];  // 4 * 64
    let rng: u64 = 0x2468ACE013579BDF;

    let c: usize = 0;
    while (c < n_clusters) : (c += 1) {
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            centers[c * dim + d] = nextRandUnitF32(&rng) * 4.0;
        }
    }

    let doc: usize = 0;
    while (doc < n_docs) : (doc += 1) {
        const cl: usize = doc % n_clusters;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            corpus[doc * dim + d] = centers[cl * dim + d] + nextRandUnitF32(&rng) * 0.7;
        }
    }

    let mins: [64]f32 = [0.0; 64];
    let scales: [64]f32 = [0.0; 64];
    let mins_s: []f32 = mins[0..];
    let scales_s: []f32 = scales[0..];
    expect_eq(sq4FitCodebook(corpus[0..], n_docs, dim, mins_s, scales_s), true);

    let packed_corpus: [2048]u8 = [0; 2048]; // 64 * 32
    let p_idx: usize = 0;
    while (p_idx < n_docs) : (p_idx += 1) {
        let dst: []u8 = packed_corpus[p_idx * stride .. (p_idx + 1) * stride];
        expect_eq(sq4EncodeVector(corpus[p_idx * dim .. (p_idx + 1) * dim], mins[0..], scales[0..], dst), true);
    }

    const k: usize = 5;
    const num_queries: usize = 12;
    let sum_recall: f64 = 0.0;

    let out_ids: [5]usize = [0; 5];
    let out_dists: [5]f32 = [0.0; 5];
    let out_ids_s: []usize = out_ids[0..];
    let out_dists_s: []f32 = out_dists[0..];
    let pred_u32: [5]u32 = [0; 5];
    let gt_u32: [5]u32 = [0; 5];
    let gt_u32_s: []u32 = gt_u32[0..];

    let qi: usize = 0;
    while (qi < num_queries) : (qi += 1) {
        const cl: usize = qi % n_clusters;
        let query: [64]f32 = [0.0; 64];
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            query[d] = centers[cl * dim + d] + nextRandUnitF32(&rng) * 0.75;
        }

        const n_found: usize = cap.vector.sq4BatchTopK(
            query[0..],
            packed_corpus[0..],
            n_docs,
            mins[0..],
            scales[0..],
            k,
            out_ids_s,
            out_dists_s,
        ) ?? 0;
        expect_eq(n_found, k);

        let r: usize = 1;
        while (r < k) : (r += 1) {
            expect_eq(out_dists[r - 1] <= out_dists[r], true);
        }

        let j: usize = 0;
        while (j < k) : (j += 1) {
            pred_u32[j] = out_ids[j] as u32;
        }

        const gt_n: usize = computeGroundTruthL2TopK(query[0..], corpus[0..], n_docs, dim, k, gt_u32_s) ?? 0;
        expect_eq(gt_n, k);
        sum_recall += annRecallAtK(pred_u32[0..], gt_u32[0..], k) ?? 0.0;
    }

    const mean_recall: f64 = sum_recall / (num_queries as f64);
    expect_eq(mean_recall >= 0.85, true);
}

// ============================================================================
// Test 5: Empty / Invalid Dimension / Non-Finite Defensive Guards
// ============================================================================

fn testEmptyAndInvalidDimGuards()! {
    let mins: [4]f32 = [0.0; 4];
    let scales: [4]f32 = [1.0; 4];
    let neg_scales: [4]f32 = [1.0, -0.5, 1.0, 1.0];
    let mins_s: []f32 = mins[0..];
    let scales_s: []f32 = scales[0..];

    expect_eq(Sq4Codebook.init(mins_s, scales_s, 0) == null, true);
    expect_eq(Sq4Codebook.init(mins_s, scales_s, 5) == null, true);

    const corpus: [8]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0];
    const corpus_nan: [4]f32 = [1.0, std.math.nan(f32), 3.0, 4.0];
    expect_eq(sq4FitCodebook(corpus[0..], 0, 4, mins_s, scales_s), false);
    expect_eq(sq4FitCodebook(corpus[0..], 2, 0, mins_s, scales_s), false);
    expect_eq(sq4FitCodebook(corpus[0..7], 2, 4, mins_s, scales_s), false);
    expect_eq(sq4FitCodebook(corpus_nan[0..], 1, 4, mins_s, scales_s), false);

    let packed_buf: [2]u8 = [0; 2];
    let packed_s: []u8 = packed_buf[0..];
    let packed_short: []u8 = packed_buf[0..1];
    let decoded_buf: [4]f32 = [0.0; 4];
    let dec_s: []f32 = decoded_buf[0..];
    let dec_short: []f32 = decoded_buf[0..3];

    expect_eq(sq4EncodeVector(corpus[0..0], mins[0..], scales[0..], packed_s), false);
    expect_eq(sq4EncodeVector(corpus[0..4], mins[0..], scales[0..], packed_short), false);
    expect_eq(sq4EncodeVector(corpus_nan[0..], mins[0..], scales[0..], packed_s), false);
    expect_eq(sq4EncodeVector(corpus[0..4], mins[0..], neg_scales[0..], packed_s), false);

    expect_eq(sq4DecodeVector(packed_buf[0..1], mins[0..], scales[0..], dec_s), false);
    expect_eq(sq4DecodeVector(packed_buf[0..], mins[0..], scales[0..], dec_short), false);

    expect_eq(sq4AsymmetricL2Sq(corpus[0..0], packed_buf[0..], mins[0..], scales[0..]) == null, true);
    expect_eq(sq4AsymmetricL2Sq(corpus_nan[0..], packed_buf[0..], mins[0..], scales[0..]) == null, true);
    expect_eq(sq4AsymmetricL2Sq(corpus[0..4], packed_buf[0..1], mins[0..], scales[0..]) == null, true);

    let ids: [2]usize = [0; 2];
    let dists: [2]f32 = [0.0; 2];
    let ids_s: []usize = ids[0..];
    let dists_s: []f32 = dists[0..];

    expect_eq(sq4BatchTopK(corpus[0..4], packed_buf[0..], 2, mins[0..], scales[0..], 1, ids_s, dists_s) == null, true); // packed len 2 < 2*2
    expect_eq(sq4BatchTopK(corpus[0..4], packed_buf[0..], 1, mins[0..], scales[0..], 3, ids_s, dists_s) == null, true); // out_ids len 2 < k=3
    expect_eq(sq4BatchTopK(corpus[0..4], packed_buf[0..], 0, mins[0..], scales[0..], 2, ids_s, dists_s) ?? 99, 0 as usize);
    expect_eq(sq4BatchTopK(corpus[0..4], packed_buf[0..], 1, mins[0..], scales[0..], 0, ids_s, dists_s) ?? 99, 0 as usize);
}

fn main() {
    test("1. vector.quant_sq4: odd/even nibble packing & uniform quantization error bound", () => {
        testOddEvenNibblePackingAndQuantErrorBound()?;
    });
    test("2. vector.quant_sq4: constant dimension (min == max) zero-division guard", () => {
        testConstantDimMinEqMaxZeroDivGuard()?;
    });
    test("3. vector.quant_sq4: asymmetric L2 squared distance matches decoded @l2sq", () => {
        testAsymmetricL2SqMatchesDecodedSimdL2Sq()?;
    });
    test("4. vector.quant_sq4: 64D clustered corpus batch Top-K high recall (8x compression)", () => {
        testSq4BatchTopKHighRecallOn64DClusters()?;
    });
    test("5. vector.quant_sq4: empty, invalid dimension and non-finite defensive guards", () => {
        testEmptyAndInvalidDimGuards()?;
    });
}
