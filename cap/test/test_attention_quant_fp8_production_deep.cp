// Origin: Pure-CP Stdlib Test Suite (lib/attention/quant_fp8.cp) | Created: 2026-11-22
// Production Deep Verification Suite for DeepSeek-V3 / H100 block-scaled FP8 E4M3 quantization
// & fused mixed-precision GEMV: outlier isolation via per-block scaling, all-zero block safety,
// fused GEMV < 2% error vs FP32 baseline, non-divisible tail blocks, and invalid/NaN guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FP8_E4M3_MAX_FINITE,
    Fp8BlockScale,
    computeBlockAmaxF32,
    quantizeBlockFp8E4m3,
    dequantizeBlockFp8E4m3,
    gemvBlockFp8E4m3,
} from "cap:attention";

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
// Test 1: Block-Level Dynamic Scaling Prevents Outlier Precision Collapse
// ============================================================================

fn testBlockDynamicScalingPreventsOutlierCollapse()! {
    expect_eq(FP8_E4M3_MAX_FINITE, 448.0 as f32);

    // 64 elements: Block 0 has a huge outlier (100000.0); Block 1 has small weights (0.001..0.032).
    let src: [64]f32 = [0.0; 64];
    src[0] = 100000.0;
    let i: usize = 1;
    while (i < 32) : (i += 1) {
        src[i] = (i as f32) * 10.0;
    }
    let j: usize = 0;
    while (j < 32) : (j += 1) {
        src[32 + j] = ((j + 1) as f32) * 0.001;
    }

    // Verify computeBlockAmaxF32 and Fp8BlockScale.fromAmax.
    let amax: [2]f32 = [0.0; 2];
    let amax_s: []f32 = amax[0..];
    expect_eq(cap.attention.computeBlockAmaxF32(src[0..], 32, amax_s), true);
    expect_eq(absF32(amax[0] - 100000.0) < 0.01, true);
    expect_eq(absF32(amax[1] - 0.032) < 0.0001, true);

    if (bs := Fp8BlockScale.fromAmax(amax[1], 32)) {
        expect_eq(bs.block_size, 32 as u16);
        expect_eq(absF32(bs.scale * bs.inv_scale - 1.0) < 0.0001, true);
    } else {
        expect_eq(true, false);
    }

    // Per-tensor scaling (block_size = 64): Block 1 collapses to 0.0 due to Block 0 outlier!
    let q_tensor: [64]u8 = [0; 64];
    let s_tensor: [1]f32 = [0.0];
    let dec_tensor: [64]f32 = [0.0; 64];
    let q_tensor_s: []u8 = q_tensor[0..];
    let s_tensor_s: []f32 = s_tensor[0..];
    let dec_tensor_s: []f32 = dec_tensor[0..];
    expect_eq(quantizeBlockFp8E4m3(src[0..], 64, q_tensor_s, s_tensor_s), true);
    expect_eq(dequantizeBlockFp8E4m3(q_tensor[0..], s_tensor[0..], 64, dec_tensor_s), true);
    expect_eq(dec_tensor[63], 0.0 as f32); // Collapsed!

    // Block-scaled (block_size = 32): Block 1 preserves precision cleanly!
    let q_blk: [64]u8 = [0; 64];
    let s_blk: [2]f32 = [0.0; 2];
    let dec_blk: [64]f32 = [0.0; 64];
    let q_blk_s: []u8 = q_blk[0..];
    let s_blk_s: []f32 = s_blk[0..];
    let dec_blk_s: []f32 = dec_blk[0..];
    expect_eq(cap.attention.quantizeBlockFp8E4m3(src[0..], 32, q_blk_s, s_blk_s), true);
    expect_eq(cap.attention.dequantizeBlockFp8E4m3(q_blk[0..], s_blk[0..], 32, dec_blk_s), true);

    let k: usize = 32;
    while (k < 64) : (k += 1) {
        const rel_err: f32 = absF32(dec_blk[k] - src[k]) / src[k];
        expect_eq(rel_err < 0.06, true);
    }
}

// ============================================================================
// Test 2: All-Zero Block Zero-Division Safety
// ============================================================================

fn testAllZeroBlockZeroDivisionSafety()! {
    let src: [96]f32 = [0.0; 96];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        src[i] = ((i as f32) - 16.0) * 0.25;
        src[64 + i] = ((i as f32) + 1.0) * 0.5;
    }
    // Block 1 (indices 32..64) is strictly all zeros.

    let q: [96]u8 = [0xFF; 96];
    let scales: [3]f32 = [1.0; 3];
    let dec_buf: [96]f32 = [9.0; 96];
    let q_s: []u8 = q[0..];
    let scales_s: []f32 = scales[0..];
    let dec_s: []f32 = dec_buf[0..];

    expect_eq(quantizeBlockFp8E4m3(src[0..], 32, q_s, scales_s), true);
    expect_eq(scales[1], 0.0 as f32);
    expect_eq(dequantizeBlockFp8E4m3(q[0..], scales[0..], 32, dec_s), true);

    let z: usize = 32;
    while (z < 64) : (z += 1) {
        expect_eq(q[z], 0 as u8);
        expect_eq(dec_buf[z], 0.0 as f32);
    }
}

// ============================================================================
// Test 3: Fused FP8 GEMV vs FP32 Baseline (< 2% Relative L2 Error)
// ============================================================================

fn testFusedFp8GemvVsFp32BaselineUnder2Percent()! {
    const rows: usize = 8;
    const cols: usize = 64;
    const block_size: usize = 32;
    const bpr: usize = 2;

    let w_fp32: [512]f32 = [0.0; 512]; // 8 * 64
    let x: [64]f32 = [0.0; 64];
    let rng: u64 = 0x202611220707;

    let i: usize = 0;
    while (i < 512) : (i += 1) {
        w_fp32[i] = 0.4 + nextRandUnitF32(&rng) * 1.6;
    }
    let j: usize = 0;
    while (j < cols) : (j += 1) {
        x[j] = 0.5 + nextRandUnitF32(&rng) * 1.2;
    }

    let w_q: [512]u8 = [0; 512];
    let w_scales: [16]f32 = [0.0; 16]; // 8 * 2
    let r: usize = 0;
    while (r < rows) : (r += 1) {
        let rq: []u8 = w_q[r * cols .. (r + 1) * cols];
        let rs: []f32 = w_scales[r * bpr .. (r + 1) * bpr];
        expect_eq(quantizeBlockFp8E4m3(w_fp32[r * cols .. (r + 1) * cols], block_size, rq, rs), true);
    }

    let y_fp8: [8]f32 = [0.0; 8];
    let y_fp8_s: []f32 = y_fp8[0..];
    expect_eq(cap.attention.gemvBlockFp8E4m3(x[0..], w_q[0..], w_scales[0..], rows, cols, block_size, y_fp8_s), true);

    let err_sq: f32 = 0.0;
    let ref_sq: f32 = 0.0;
    let row: usize = 0;
    while (row < rows) : (row += 1) {
        const y_ref: f32 = @dot(w_fp32[row * cols .. (row + 1) * cols], x[0..]);
        const diff: f32 = y_fp8[row] - y_ref;
        err_sq += diff * diff;
        ref_sq += y_ref * y_ref;
    }

    const rel_l2: f32 = @sqrt(err_sq) / @sqrt(ref_sq);
    expect_eq(rel_l2 < 0.02, true);
}

// ============================================================================
// Test 4: Non-Divisible Tail Block Handling (cols = 45, block_size = 32)
// ============================================================================

fn testNonDivisibleTailBlockHandling()! {
    const rows: usize = 4;
    const cols: usize = 45; // 45 = 32 + 13 tail
    const block_size: usize = 32;
    const bpr: usize = 2;

    let w_fp32: [180]f32 = [0.0; 180]; // 4 * 45
    let x: [45]f32 = [0.0; 45];
    let rng: u64 = 0x76543210FEDC;

    let i: usize = 0;
    while (i < 180) : (i += 1) {
        w_fp32[i] = nextRandUnitF32(&rng) * 3.0;
    }
    let j: usize = 0;
    while (j < cols) : (j += 1) {
        x[j] = nextRandUnitF32(&rng) * 2.0;
    }

    let w_q: [180]u8 = [0; 180];
    let w_scales: [8]f32 = [0.0; 8];
    let r: usize = 0;
    while (r < rows) : (r += 1) {
        let rq: []u8 = w_q[r * cols .. (r + 1) * cols];
        let rs: []f32 = w_scales[r * bpr .. (r + 1) * bpr];
        expect_eq(quantizeBlockFp8E4m3(w_fp32[r * cols .. (r + 1) * cols], block_size, rq, rs), true);
    }

    let y_fp8: [4]f32 = [0.0; 4];
    let y_fp8_s: []f32 = y_fp8[0..];
    expect_eq(gemvBlockFp8E4m3(x[0..], w_q[0..], w_scales[0..], rows, cols, block_size, y_fp8_s), true);

    let err_sq: f32 = 0.0;
    let ref_sq: f32 = 0.0;
    let row: usize = 0;
    while (row < rows) : (row += 1) {
        const y_ref: f32 = @dot(w_fp32[row * cols .. (row + 1) * cols], x[0..]);
        const diff: f32 = y_fp8[row] - y_ref;
        err_sq += diff * diff;
        ref_sq += y_ref * y_ref;
    }
    expect_eq((@sqrt(err_sq) / @sqrt(ref_sq)) < 0.02, true);
}

// ============================================================================
// Test 5: Invalid Dimensions, Negative Scales & NaN Defensive Guards
// ============================================================================

fn testInvalidDimNegativeScaleAndNanGuards()! {
    expect_eq(Fp8BlockScale.fromAmax(1.0, 0) == null, true);
    expect_eq(Fp8BlockScale.fromAmax(-1.0, 32) == null, true);
    expect_eq(Fp8BlockScale.fromAmax(std.math.nan(f32), 32) == null, true);
    if (z := Fp8BlockScale.fromAmax(0.0, 32)) {
        expect_eq(z.scale, 0.0 as f32);
        expect_eq(z.inv_scale, 0.0 as f32);
    } else {
        expect_eq(true, false);
    }

    const src: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    const src_nan: [4]f32 = [1.0, std.math.nan(f32), 3.0, 4.0];
    let q: [4]u8 = [0; 4];
    let q_nan_byte: [4]u8 = [0, 0x7F, 0, 0]; // 0x7F = FP8 E4M3FN NaN
    let scales: [1]f32 = [1.0];
    let neg_scales: [1]f32 = [-1.0];
    let dst: [4]f32 = [0.0; 4];
    let y: [1]f32 = [0.0];

    let q_s: []u8 = q[0..];
    let scales_s: []f32 = scales[0..];
    let dst_s: []f32 = dst[0..];
    let y_s: []f32 = y[0..];

    expect_eq(computeBlockAmaxF32(src[0..0], 4, scales_s), false);
    expect_eq(computeBlockAmaxF32(src[0..], 0, scales_s), false);
    expect_eq(computeBlockAmaxF32(src_nan[0..], 4, scales_s), false);

    expect_eq(quantizeBlockFp8E4m3(src[0..0], 4, q_s, scales_s), false);
    expect_eq(quantizeBlockFp8E4m3(src[0..], 0, q_s, scales_s), false);
    expect_eq(quantizeBlockFp8E4m3(src_nan[0..], 4, q_s, scales_s), false);

    expect_eq(dequantizeBlockFp8E4m3(q[0..0], scales[0..], 4, dst_s), false);
    expect_eq(dequantizeBlockFp8E4m3(q[0..], neg_scales[0..], 4, dst_s), false);
    expect_eq(dequantizeBlockFp8E4m3(q_nan_byte[0..], scales[0..], 4, dst_s), false);

    expect_eq(gemvBlockFp8E4m3(src[0..], q[0..], scales[0..], 0, 4, 4, y_s), false);
    expect_eq(gemvBlockFp8E4m3(src[0..], q[0..], scales[0..], 1, 0, 4, y_s), false);
    expect_eq(gemvBlockFp8E4m3(src[0..], q[0..], scales[0..], 1, 4, 0, y_s), false);
    expect_eq(gemvBlockFp8E4m3(src_nan[0..], q[0..], scales[0..], 1, 4, 4, y_s), false);
    expect_eq(gemvBlockFp8E4m3(src[0..], q[0..], neg_scales[0..], 1, 4, 4, y_s), false);
    expect_eq(gemvBlockFp8E4m3(src[0..], q_nan_byte[0..], scales[0..], 1, 4, 4, y_s), false);
}

fn main() {
    test("1. attention.quant_fp8: block dynamic scaling prevents outlier precision collapse", () => {
        testBlockDynamicScalingPreventsOutlierCollapse()?;
    });
    test("2. attention.quant_fp8: all-zero block zero-division safety", () => {
        testAllZeroBlockZeroDivisionSafety()?;
    });
    test("3. attention.quant_fp8: fused FP8 GEMV vs FP32 baseline < 2% error", () => {
        testFusedFp8GemvVsFp32BaselineUnder2Percent()?;
    });
    test("4. attention.quant_fp8: non-divisible tail block (cols=45, block_size=32)", () => {
        testNonDivisibleTailBlockHandling()?;
    });
    test("5. attention.quant_fp8: invalid dimensions, negative scales & NaN guards", () => {
        testInvalidDimNegativeScaleAndNanGuards()?;
    });
}
