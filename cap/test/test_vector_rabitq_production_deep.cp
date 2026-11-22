// Origin: Pure-CP Stdlib Test Suite (lib/vector/rabitq.cp) | Created: 2026-11-22
// Production Deep Verification Suite for SIGMOD 2024 RaBitQ unbiased 1-bit random orthogonal
// binary quantization: FWHT/sign-flip norm preservation & involution, 32x compression & geometric
// invariants, 64D dot/L2 unbiased estimation & Top-1/ranking accuracy, zero-residual safety, and guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RabitqCodeMeta,
    rabitqWordsForDim,
    rabitqSignFlipInPlace,
    rabitqFastWalshHadamardInPlace,
    rabitqEncodeUnit,
    rabitqEstimateDot,
    rabitqEstimateL2Sq,
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
// Test 1: FWHT & Sign-Flip Orthogonality (Norm & Dot Preservation) & Involution
// ============================================================================

fn testFwhtAndSignFlipOrthogonalityAndInvolution()! {
    let a: [64]f32 = [0.0; 64];
    let a_orig: [64]f32 = [0.0; 64];
    let b: [64]f32 = [0.0; 64];
    let rng: u64 = 0x1122334455667788;

    let i: usize = 0;
    while (i < 64) : (i += 1) {
        const av: f32 = nextRandUnitF32(&rng) * 3.0;
        const bv: f32 = nextRandUnitF32(&rng) * 2.5;
        a[i] = av;
        a_orig[i] = av;
        b[i] = bv;
    }

    const orig_norm_a: f32 = @dot(a[0..], a[0..]);
    const orig_dot_ab: f32 = @dot(a[0..], b[0..]);

    const seed: u64 = 0x20261122;
    let a_s: []f32 = a[0..];
    let b_s: []f32 = b[0..];
    expect_eq(cap.vector.rabitqSignFlipInPlace(a_s, seed), true);
    expect_eq(cap.vector.rabitqFastWalshHadamardInPlace(a_s), true);
    expect_eq(rabitqSignFlipInPlace(b_s, seed), true);
    expect_eq(rabitqFastWalshHadamardInPlace(b_s), true);

    // SRHT (SignFlip + FWHT) must preserve L2 norm and inner product.
    const rot_norm_a: f32 = @dot(a[0..], a[0..]);
    const rot_dot_ab: f32 = @dot(a[0..], b[0..]);
    expect_eq(absF32(rot_norm_a - orig_norm_a) < 0.001, true);
    expect_eq(absF32(rot_dot_ab - orig_dot_ab) < 0.001, true);

    // Exact inverse: FWHT then SignFlip recovers `a_orig`.
    expect_eq(rabitqFastWalshHadamardInPlace(a_s), true);
    expect_eq(rabitqSignFlipInPlace(a_s, seed), true);

    let j: usize = 0;
    while (j < 64) : (j += 1) {
        expect_eq(absF32(a[j] - a_orig[j]) < 0.0005, true);
    }
}

// ============================================================================
// Test 2: 32x Compression & Geometric Invariants (dot_o_obar > 0, popcount)
// ============================================================================

fn test32xCompressionAndGeometricInvariants()! {
    expect_eq(rabitqWordsForDim(0), 0 as usize);
    expect_eq(rabitqWordsForDim(1), 1 as usize);
    expect_eq(rabitqWordsForDim(64), 1 as usize);  // 64 * 4B = 256B -> 8B (32x compression)
    expect_eq(rabitqWordsForDim(65), 2 as usize);
    expect_eq(rabitqWordsForDim(128), 2 as usize);

    let x: [64]f32 = [0.0; 64];
    let c: [64]f32 = [0.0; 64];
    let rng: u64 = 0x33445566778899AA;
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        x[i] = nextRandUnitF32(&rng) * 4.0;
        c[i] = nextRandUnitF32(&rng) * 1.0;
    }

    let words: [1]u64 = [0];
    let words_s: []u64 = words[0..];
    if (m := cap.vector.rabitqEncodeUnit(x[0..], c[0..], words_s)) {
        const ref_norm: f32 = @sqrt(@l2sq(x[0..], c[0..]));
        expect_eq(absF32(m.norm_l2 - ref_norm) < 0.001, true);
        // Geometric invariant: <o, o_bar> in [1/sqrt(64), 1.0] = [0.125, 1.0]
        expect_eq(m.dot_o_obar >= 0.124, true);
        expect_eq(m.dot_o_obar <= 1.0001, true);
        expect_eq(m.sum_bits, @popCount(words[0]) as u32);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 3: 64D RaBitQ Unbiased Dot & L2 Estimation Ranking & Top-1 Accuracy
// ============================================================================

fn test64DRabitqDotAndL2EstimationRankingAccuracy()! {
    const dim: usize = 64;
    const n_cands: usize = 16;
    const seed: u64 = 0x55AA33CC11224466;
    let rng: u64 = 0x1020304050607080;

    // Shared centroid and query vector.
    let centroid: [64]f32 = [0.0; 64];
    let query: [64]f32 = [0.0; 64];
    let d: usize = 0;
    while (d < dim) : (d += 1) {
        centroid[d] = nextRandUnitF32(&rng) * 0.5;
        query[d] = centroid[d] + nextRandUnitF32(&rng) * 2.0;
    }

    // Rotate centroid and query with SRHT.
    let c_rot: [64]f32 = centroid;
    let q_rot: [64]f32 = query;
    let c_rot_s: []f32 = c_rot[0..];
    let q_rot_s: []f32 = q_rot[0..];
    expect_eq(rabitqSignFlipInPlace(c_rot_s, seed), true);
    expect_eq(rabitqFastWalshHadamardInPlace(c_rot_s), true);
    expect_eq(rabitqSignFlipInPlace(q_rot_s, seed), true);
    expect_eq(rabitqFastWalshHadamardInPlace(q_rot_s), true);

    let q_res: [64]f32 = [0.0; 64];
    let qi: usize = 0;
    while (qi < dim) : (qi += 1) {
        q_res[qi] = q_rot[qi] - c_rot[qi];
    }
    const q_norm_sq: f32 = @dot(q_res[0..], q_res[0..]);

    // Build 16 candidates with progressive distance from query:
    // cand 0 is very close to query; cand 15 is far/opposite.
    let best_true_id: usize = 0;
    let best_true_l2: f32 = 1.0e30;
    let best_est_id: usize = 0;
    let best_est_l2: f32 = 1.0e30;

    let close_est_dot: f32 = 0.0;
    let far_est_dot: f32 = 0.0;
    let close_est_l2: f32 = 0.0;
    let far_est_l2: f32 = 0.0;

    let idx: usize = 0;
    while (idx < n_cands) : (idx += 1) {
        let cand: [64]f32 = [0.0; 64];
        const mix: f32 = 1.0 - ((idx as f32) * 0.12); // +1.0 down to -0.8
        let j: usize = 0;
        while (j < dim) : (j += 1) {
            cand[j] = centroid[j] + mix * (query[j] - centroid[j]) + nextRandUnitF32(&rng) * 0.35;
        }

        const true_l2: f32 = @l2sq(query[0..], cand[0..]);
        if (true_l2 < best_true_l2) {
            best_true_l2 = true_l2;
            best_true_id = idx;
        }

        let cand_rot: [64]f32 = cand;
        let cand_rot_s: []f32 = cand_rot[0..];
        expect_eq(rabitqSignFlipInPlace(cand_rot_s, seed), true);
        expect_eq(rabitqFastWalshHadamardInPlace(cand_rot_s), true);

        let code: [1]u64 = [0];
        let code_s: []u64 = code[0..];
        if (m := rabitqEncodeUnit(cand_rot[0..], c_rot[0..], code_s)) {
            const est_dot: f32 = cap.vector.rabitqEstimateDot(q_res[0..], code[0..], m) ?? -1.0e30;
            const est_l2: f32 = cap.vector.rabitqEstimateL2Sq(q_res[0..], q_norm_sq, code[0..], m) ?? 1.0e30;

            if (est_l2 < best_est_l2) {
                best_est_l2 = est_l2;
                best_est_id = idx;
            }
            if (idx == 0) {
                close_est_dot = est_dot;
                close_est_l2 = est_l2;
            }
            if (idx == n_cands - 1) {
                far_est_dot = est_dot;
                far_est_l2 = est_l2;
            }
        } else {
            expect_eq(true, false);
        }
    }

    // Top-1 nearest neighbor must match exact brute-force Top-1, and ranking between close/far must hold strongly.
    expect_eq(best_est_id, best_true_id);
    expect_eq(close_est_dot > far_est_dot, true);
    expect_eq(close_est_l2 < far_est_l2, true);
}

// ============================================================================
// Test 4: Zero-Residual Vector Safety (No Zero-Division NaN)
// ============================================================================

fn testZeroResidualVectorSafety()! {
    const c: [8]f32 = [1.0, -2.0, 3.0, -4.0, 0.5, -0.5, 2.5, -1.5];
    const q_res: [8]f32 = [1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0];
    const q_norm_sq: f32 = @dot(q_res[0..], q_res[0..]); // 4.0

    let code: [1]u64 = [0xFFFF];
    let code_s: []u64 = code[0..];
    if (m := rabitqEncodeUnit(c[0..], c[0..], code_s)) {
        expect_eq(m.norm_l2, 0.0 as f32);
        expect_eq(m.dot_o_obar, 0.0 as f32);
        expect_eq(m.sum_bits, 0 as u32);
        expect_eq(code[0], 0 as u64);

        const est_dot: f32 = rabitqEstimateDot(q_res[0..], code[0..], m) ?? -1.0;
        expect_eq(est_dot, 0.0 as f32);

        const est_l2: f32 = rabitqEstimateL2Sq(q_res[0..], q_norm_sq, code[0..], m) ?? -1.0;
        expect_eq(absF32(est_l2 - q_norm_sq) < 0.0001, true);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 5: Non-Power-of-Two Dimensions, Empty & Invalid Input Guards
// ============================================================================

fn testNonPowerOfTwoAndEmptyInvalidGuards()! {
    let v3: [3]f32 = [1.0, 2.0, 3.0];
    let v6: [6]f32 = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0];
    let v4_nan: [4]f32 = [1.0, std.math.nan(f32), 2.0, 3.0];
    let empty_f32: []f32 = v3[0..0];
    let v3_s: []f32 = v3[0..];
    let v6_s: []f32 = v6[0..];
    let v4_nan_s: []f32 = v4_nan[0..];

    // FWHT requires power-of-two finite slice.
    expect_eq(rabitqFastWalshHadamardInPlace(empty_f32), false);
    expect_eq(rabitqFastWalshHadamardInPlace(v3_s), false);
    expect_eq(rabitqFastWalshHadamardInPlace(v6_s), false);
    expect_eq(rabitqFastWalshHadamardInPlace(v4_nan_s), false);

    // SignFlip rejects empty and NaN slices.
    expect_eq(rabitqSignFlipInPlace(empty_f32, 42), false);
    expect_eq(rabitqSignFlipInPlace(v4_nan_s, 42), false);

    // EncodeUnit guards.
    let words: [1]u64 = [0];
    let words_s: []u64 = words[0..];
    let words_empty: []u64 = words[0..0];
    expect_eq(rabitqEncodeUnit(v3[0..0], v3[0..0], words_s) == null, true);
    expect_eq(rabitqEncodeUnit(v3[0..], v6[0..], words_s) == null, true);
    expect_eq(rabitqEncodeUnit(v3[0..], v3[0..], words_empty) == null, true);
    expect_eq(rabitqEncodeUnit(v4_nan[0..], v6[0..4], words_s) == null, true);

    // EstimateDot / EstimateL2Sq guards.
    const valid_meta = RabitqCodeMeta{ norm_l2: 1.0, dot_o_obar: 0.8, sum_bits: 2 };
    const neg_meta = RabitqCodeMeta{ norm_l2: -1.0, dot_o_obar: 0.8, sum_bits: 2 };
    expect_eq(rabitqEstimateDot(v3[0..0], words[0..], valid_meta) == null, true);
    expect_eq(rabitqEstimateDot(v3[0..], words[0..0], valid_meta) == null, true);
    expect_eq(rabitqEstimateDot(v3[0..], words[0..], neg_meta) == null, true);
    expect_eq(rabitqEstimateDot(v4_nan[0..], words[0..], valid_meta) == null, true);
    expect_eq(rabitqEstimateL2Sq(v3[0..], -0.1, words[0..], valid_meta) == null, true);
}

fn main() {
    test("1. vector.rabitq: FWHT & sign-flip orthogonality and exact involution", () => {
        testFwhtAndSignFlipOrthogonalityAndInvolution()?;
    });
    test("2. vector.rabitq: 32x compression & dot_o_obar / popcount geometric invariants", () => {
        test32xCompressionAndGeometricInvariants()?;
    });
    test("3. vector.rabitq: 64D unbiased dot/L2 estimation ranking & Top-1 accuracy", () => {
        test64DRabitqDotAndL2EstimationRankingAccuracy()?;
    });
    test("4. vector.rabitq: zero-residual vector safety (no zero-division NaN)", () => {
        testZeroResidualVectorSafety()?;
    });
    test("5. vector.rabitq: non-power-of-two, empty and invalid input guards", () => {
        testNonPowerOfTwoAndEmptyInvalidGuards()?;
    });
}
