// Origin: Pure-CP Stdlib Test Suite (Multi-Tenant LoRA Low-Rank Adapter & Weight Merge) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/lora.cp
// Covers:
//   1. loraApplyAdd dynamic incremental forward matches static loraMergeIntoBase + W_merged * x (< 1e-12)
//   2. loraMergeIntoBase + loraUnmergeFromBase lossless round-trip recovery of base weights (< 1e-12)
//   3. loraApplyWeightedSum multi-adapter weighted composition (code + style adapters, including zero-weight skip)
//   4. Zero-initialized B matrix preserves exact base identity output (standard LoRA init property)
//   5. Boundary & malformed input defenses (rank == 0, dim mismatch, undersized buffers, NaN/Inf)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    LoraAdapter,
    loraApplyAdd,
    loraApplyWeightedSum,
    loraMergeIntoBase,
    loraUnmergeFromBase,
} from "../../lib/attention/index.cp";

const EPS_F64: f64 = 0.000000000001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn matvec(w: []const f64, x: []const f64, out_dim: usize, in_dim: usize, y_out: *[]f64) {
    let o: usize = 0;
    while (o < out_dim) : (o += 1) {
        let acc: f64 = 0.0;
        let j: usize = 0;
        while (j < in_dim) : (j += 1) {
            acc += w[o * in_dim + j] * x[j];
        }
        y_out[o] = acc;
    }
}

fn testDynamicApplyEqualsStaticMerge()! {
    const IN_DIM: usize = 4;
    const OUT_DIM: usize = 3;
    const RANK: usize = 2;

    const base_w_orig: [12]f64 = [
        0.5, -0.25, 1.1, 0.3,
        -0.8, 0.6, 0.2, -0.4,
        0.15, 0.9, -0.7, 0.5,
    ];
    const a_mat: [8]f64 = [
        0.4, -0.3, 0.8, 0.1,
        -0.2, 0.5, -0.1, 0.6,
    ];
    const b_mat: [6]f64 = [
        1.2, -0.5,
        -0.4, 0.9,
        0.7, 0.3,
    ];
    const x: [4]f64 = [1.5, -0.75, 2.0, 0.5];

    if (ad := LoraAdapter.init(7, IN_DIM, OUT_DIM, RANK, 6.0, a_mat[0..], b_mat[0..])) {
        expect_eq(ad.id, 7);
        expect_eq(approxF64(ad.scaling(), 3.0, EPS_F64), true);

        // Path A: base matvec + dynamic loraApplyAdd
        let y_dyn_buf: [3]f64 = [0.0; 3];
        let y_dyn: []f64 = y_dyn_buf[0..];
        matvec(base_w_orig[0..], x[0..], OUT_DIM, IN_DIM, y_dyn);

        let scratch_buf: [2]f64 = [0.0; 2];
        let scratch: []f64 = scratch_buf[0..];
        expect_eq(loraApplyAdd(ad, x[0..], y_dyn, scratch), true);

        // Path B: static loraMergeIntoBase + direct W_merged * x
        let w_merged_buf: [12]f64 = base_w_orig;
        let w_merged: []f64 = w_merged_buf[0..];
        expect_eq(loraMergeIntoBase(w_merged, ad), true);

        let y_static_buf: [3]f64 = [0.0; 3];
        let y_static: []f64 = y_static_buf[0..];
        matvec(w_merged_buf[0..], x[0..], OUT_DIM, IN_DIM, y_static);

        let o: usize = 0;
        while (o < OUT_DIM) : (o += 1) {
            expect_eq(approxF64(y_dyn_buf[o], y_static_buf[o], EPS_F64), true);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testMergeAndUnmergeLosslessRoundTrip()! {
    const base_w_orig: [12]f64 = [
        1.25, -0.5, 0.75, -1.0,
        0.3, 0.8, -0.2, 0.4,
        -0.6, 0.1, 1.5, -0.9,
    ];
    const a_mat: [8]f64 = [
        0.25, 0.5, -0.75, 0.2,
        -0.4, 0.3, 0.6, -0.1,
    ];
    const b_mat: [6]f64 = [
        0.8, -0.3,
        0.5, 0.7,
        -0.6, 0.4,
    ];

    if (ad := LoraAdapter.init(42, 4, 3, 2, 8.0, a_mat[0..], b_mat[0..])) {
        let w_buf: [12]f64 = base_w_orig;
        let w: []f64 = w_buf[0..];

        expect_eq(loraMergeIntoBase(w, ad), true);
        // Confirm weights actually changed after merge
        expect_eq(approxF64(w_buf[0], base_w_orig[0], EPS_F64), false);

        expect_eq(loraUnmergeFromBase(w, ad), true);
        let i: usize = 0;
        while (i < 12) : (i += 1) {
            expect_eq(approxF64(w_buf[i], base_w_orig[i], EPS_F64), true);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testWeightedSumMultiAdapterComposition()! {
    const x: [4]f64 = [1.0, -2.0, 0.5, 1.5];

    // Adapter 1 (code specialist): rank = 2, alpha = 4.0 (scaling = 2.0)
    const a1: [8]f64 = [
        0.5, 0.2, -0.1, 0.3,
        -0.4, 0.6, 0.2, -0.5,
    ];
    const b1: [6]f64 = [
        0.9, -0.3,
        0.4, 0.8,
        -0.5, 0.6,
    ];

    // Adapter 2 (style specialist): rank = 1, alpha = 3.0 (scaling = 3.0)
    const a2: [4]f64 = [0.3, -0.7, 0.4, 0.2];
    const b2: [3]f64 = [1.1, -0.6, 0.5];

    if (ad1 := LoraAdapter.init(1, 4, 3, 2, 4.0, a1[0..], b1[0..])) {
        if (ad2 := LoraAdapter.init(2, 4, 3, 1, 3.0, a2[0..], b2[0..])) {
            const adapters: [2]LoraAdapter = [ad1, ad2];
            const weights: [2]f64 = [0.7, 0.3];

            let y_mix_buf: [3]f64 = [1.0, 2.0, 3.0];
            let y_mix: []f64 = y_mix_buf[0..];
            let scratch_buf: [2]f64 = [0.0; 2];
            let scratch: []f64 = scratch_buf[0..];
            expect_eq(loraApplyWeightedSum(adapters[0..], weights[0..], x[0..], y_mix, scratch), true);

            // Reference: compute each adapter delta separately and combine with weights
            let d1_buf: [3]f64 = [0.0; 3];
            let d2_buf: [3]f64 = [0.0; 3];
            let d1: []f64 = d1_buf[0..];
            let d2: []f64 = d2_buf[0..];
            expect_eq(loraApplyAdd(ad1, x[0..], d1, scratch), true);
            expect_eq(loraApplyAdd(ad2, x[0..], d2, scratch), true);

            const base_y: [3]f64 = [1.0, 2.0, 3.0];
            let o: usize = 0;
            while (o < 3) : (o += 1) {
                const expected = base_y[o] + 0.7 * d1_buf[o] + 0.3 * d2_buf[o];
                expect_eq(approxF64(y_mix_buf[o], expected, EPS_F64), true);
            }

            // Zero weight skips adapter cleanly
            const zero_w: [2]f64 = [0.0, 0.0];
            let y_zero_buf: [3]f64 = [1.0, 2.0, 3.0];
            let y_zero: []f64 = y_zero_buf[0..];
            expect_eq(loraApplyWeightedSum(adapters[0..], zero_w[0..], x[0..], y_zero, scratch), true);
            let o2: usize = 0;
            while (o2 < 3) : (o2 += 1) {
                expect_eq(approxF64(y_zero_buf[o2], base_y[o2], EPS_F64), true);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

fn testZeroInitBPreservesIdentity()! {
    const a_mat: [8]f64 = [0.9, -0.4, 1.3, 0.2, -0.7, 0.5, 0.8, -0.6];
    const b_zero: [6]f64 = [0.0; 6];
    const x: [4]f64 = [2.0, -1.0, 0.5, 3.0];

    if (ad := LoraAdapter.init(99, 4, 3, 2, 16.0, a_mat[0..], b_zero[0..])) {
        let y_buf: [3]f64 = [4.5, -2.25, 1.75];
        let y: []f64 = y_buf[0..];
        let scratch_buf: [2]f64 = [0.0; 2];
        let scratch: []f64 = scratch_buf[0..];

        expect_eq(loraApplyAdd(ad, x[0..], y, scratch), true);
        expect_eq(approxF64(y_buf[0], 4.5, EPS_F64), true);
        expect_eq(approxF64(y_buf[1], -2.25, EPS_F64), true);
        expect_eq(approxF64(y_buf[2], 1.75, EPS_F64), true);
    } else {
        expect_eq(true, false);
    }
}

fn testBoundaryAndMalformedInputDefenses()! {
    const a_ok: [8]f64 = [0.1; 8];
    const b_ok: [6]f64 = [0.2; 6];
    const nan_val = std.math.nan(f64);
    const inf_val = std.math.inf(f64);
    const a_nan: [8]f64 = [0.1, nan_val, 0.1, 0.1, 0.1, 0.1, 0.1, 0.1];
    const b_inf: [6]f64 = [0.2, 0.2, inf_val, 0.2, 0.2, 0.2];

    // LoraAdapter.init defenses
    expect_eq(LoraAdapter.init(1, 0, 3, 2, 4.0, a_ok[0..], b_ok[0..]) == null, true); // in_dim == 0
    expect_eq(LoraAdapter.init(1, 4, 0, 2, 4.0, a_ok[0..], b_ok[0..]) == null, true); // out_dim == 0
    expect_eq(LoraAdapter.init(1, 4, 3, 0, 4.0, a_ok[0..], b_ok[0..]) == null, true); // rank == 0
    expect_eq(LoraAdapter.init(1, 4, 3, 2, nan_val, a_ok[0..], b_ok[0..]) == null, true); // alpha NaN
    expect_eq(LoraAdapter.init(1, 4, 3, 2, 4.0, a_ok[0..4], b_ok[0..]) == null, true); // short a_mat
    expect_eq(LoraAdapter.init(1, 4, 3, 2, 4.0, a_ok[0..], b_ok[0..4]) == null, true); // short b_mat
    expect_eq(LoraAdapter.init(1, 4, 3, 2, 4.0, a_nan[0..], b_ok[0..]) == null, true); // NaN in A
    expect_eq(LoraAdapter.init(1, 4, 3, 2, 4.0, a_ok[0..], b_inf[0..]) == null, true); // Inf in B

    if (ad := LoraAdapter.init(1, 4, 3, 2, 4.0, a_ok[0..], b_ok[0..])) {
        const x_ok: [4]f64 = [1.0, 2.0, 3.0, 4.0];
        const x_nan: [4]f64 = [1.0, nan_val, 3.0, 4.0];
        let y_buf: [3]f64 = [0.0; 3];
        let y_nan_buf: [3]f64 = [0.0, nan_val, 0.0];
        let scratch_buf: [2]f64 = [0.0; 2];
        let y: []f64 = y_buf[0..];
        let y_short: []f64 = y_buf[0..2];
        let y_nan: []f64 = y_nan_buf[0..];
        let scratch: []f64 = scratch_buf[0..];
        let scratch_short: []f64 = scratch_buf[0..1];

        // loraApplyAdd defenses
        expect_eq(loraApplyAdd(ad, x_ok[0..3], y, scratch), false);
        expect_eq(loraApplyAdd(ad, x_ok[0..], y_short, scratch), false);
        expect_eq(loraApplyAdd(ad, x_ok[0..], y, scratch_short), false);
        expect_eq(loraApplyAdd(ad, x_nan[0..], y, scratch), false);
        expect_eq(loraApplyAdd(ad, x_ok[0..], y_nan, scratch), false);

        // loraApplyWeightedSum defenses (dim mismatch, empty, NaN weight, atomic no-mutation)
        const a_other: [6]f64 = [0.1; 6];
        const b_other: [6]f64 = [0.2; 6];
        if (ad_mismatch := LoraAdapter.init(2, 3, 3, 2, 4.0, a_other[0..], b_other[0..])) {
            const pair_mismatch: [2]LoraAdapter = [ad, ad_mismatch];
            const w_ok: [2]f64 = [0.5, 0.5];
            const w_nan: [2]f64 = [0.5, nan_val];
            const pair_ok: [2]LoraAdapter = [ad, ad];

            y_buf[0] = 7.0;
            expect_eq(loraApplyWeightedSum(pair_ok[0..0], w_ok[0..], x_ok[0..], y, scratch), false);
            expect_eq(loraApplyWeightedSum(pair_ok[0..], w_ok[0..1], x_ok[0..], y, scratch), false);
            expect_eq(loraApplyWeightedSum(pair_mismatch[0..], w_ok[0..], x_ok[0..], y, scratch), false);
            expect_eq(loraApplyWeightedSum(pair_ok[0..], w_nan[0..], x_ok[0..], y, scratch), false);
            // Confirm y_buf[0] was untouched on failed validation
            expect_eq(approxF64(y_buf[0], 7.0, EPS_F64), true);
        } else {
            expect_eq(true, false);
        }

        // loraMergeIntoBase / loraUnmergeFromBase defenses
        let w_short_buf: [10]f64 = [0.0; 10];
        let w_short: []f64 = w_short_buf[0..];
        let w_nan_buf: [12]f64 = [0.0; 12];
        w_nan_buf[5] = nan_val;
        let w_nan: []f64 = w_nan_buf[0..];
        expect_eq(loraMergeIntoBase(w_short, ad), false);
        expect_eq(loraUnmergeFromBase(w_short, ad), false);
        expect_eq(loraMergeIntoBase(w_nan, ad), false);
        expect_eq(loraUnmergeFromBase(w_nan, ad), false);
    } else {
        expect_eq(true, false);
    }
}

fn main() {
    test("1. lora: dynamic loraApplyAdd matches static loraMergeIntoBase + matvec (<1e-12)", () => {
        testDynamicApplyEqualsStaticMerge()?;
    });
    test("2. lora: loraMergeIntoBase + loraUnmergeFromBase lossless round-trip (<1e-12)", () => {
        testMergeAndUnmergeLosslessRoundTrip()?;
    });
    test("3. lora: loraApplyWeightedSum multi-adapter composition & zero-weight skip", () => {
        testWeightedSumMultiAdapterComposition()?;
    });
    test("4. lora: zero-initialized B matrix preserves exact base identity output", () => {
        testZeroInitBPreservesIdentity()?;
    });
    test("5. lora: rank==0, dim mismatch, undersized buffers & NaN/Inf defenses", () => {
        testBoundaryAndMalformedInputDefenses()?;
    });
}
