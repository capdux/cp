// Origin: Pure-CP Stdlib Test Suite (lib/attention/quantAwq.cp) | Created: 2026-11-22
// Production Deep Verification Suite for AWQ Activation-Aware INT4 Group-Quantized GEMV Kernel:
//   1. Salient channel scale computation (alpha=0.5, alpha=0.0, alpha=1.0 & zero-activity channel neutral 1.0)
//   2. Core AWQ theorem: AWQ channel scaling achieves strictly lower GEMV error than naive RTN on salient channels
//   3. Exact parity between fused awqGemvF32 (tiled @dot/@fma) and explicit awqDequantizeRow + @dot (multi-group + partial tail)
//   4. Quantization error & cosine fidelity diagnostics (evaluateAwqQuantError)
//   5. All-zero & constant weight group zero-division safety
//   6. NaN / Inf sanitization across calibration activations, weights & GEMV input vectors
//   7. Full boundary guards (group_size==0, odd cols, invalid alpha, undersized buffers) & cap.attention facade

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    AwqGroupMeta,
    awqGroupsPerRow,
    awqPackedBytesPerRow,
    computeAwqChannelScales,
    awqQuantizeMatrix,
    awqDequantizeRow,
    awqGemvF32,
    evaluateAwqQuantError,
} from "cap:attention";

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    return absF32(a - b) <= eps;
}

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

// ============================================================================
// Test 1: Salient Channel Scale Computation
// ============================================================================
test("quantAwq: computeAwqChannelScales computes mean(|x|)^alpha with zero-channel safety", () => {
    // 2 calibration samples x 4 channels:
    //   ch 0: |100|, |-100| -> mean=100 -> alpha=0.5 => 10.0
    //   ch 1: |0.25|, |0.25| -> mean=0.25 -> alpha=0.5 => 0.5
    //   ch 2: |4.0|, |4.0| -> mean=4.0 -> alpha=0.5 => 2.0
    //   ch 3: |0.0|, |0.0| -> mean=0.0 -> neutral 1.0
    const acts: [8]f32 = [
        100.0, 0.25, -4.0, 0.0,
        -100.0, 0.25, 4.0, 0.0,
    ];
    let scales: [4]f32 = [0.0; 4];
    let scales_s: []f32 = scales[0..];

    expect_true(computeAwqChannelScales(acts[0..], 2, 4, 0.5, *scales_s));
    expect_true(approxEqF32(scales[0], 10.0, 1.0e-4));
    expect_true(approxEqF32(scales[1], 0.5, 1.0e-4));
    expect_true(approxEqF32(scales[2], 2.0, 1.0e-4));
    expect_true(approxEqF32(scales[3], 1.0, 1.0e-4));

    // alpha = 0.0 -> all scales equal 1.0
    expect_true(computeAwqChannelScales(acts[0..], 2, 4, 0.0, *scales_s));
    let j: usize = 0;
    while (j < 4) : (j += 1) {
        expect_true(approxEqF32(scales[j], 1.0, 1.0e-5));
    }

    // alpha = 1.0 -> mean(|x|) directly
    expect_true(computeAwqChannelScales(acts[0..], 2, 4, 1.0, *scales_s));
    expect_true(approxEqF32(scales[0], 100.0, 1.0e-3));
    expect_true(approxEqF32(scales[1], 0.25, 1.0e-4));
});

// ============================================================================
// Test 2: AWQ Strictly Reduces GEMV Error on Salient Channels vs Naive RTN
// ============================================================================
test("quantAwq: AWQ channel scaling strictly outperforms naive RTN on salient activation channels", () => {
    // 2 rows x 16 cols, single group_size=16:
    // Channel 0 is salient (activation = 64.0, small weight = 0.07)
    // Channels 1..15 have small activation (0.05) and large weights (~3.0)
    // Without AWQ, channel 0's weight 0.07 gets crushed by the 3.0 group range -> huge GEMV error!
    let weights: [32]f32 = [0.0; 32];
    let c: usize = 0;
    while (c < 16) : (c += 1) {
        weights[c] = if (c == 0) 0.07 else (if ((c & 1) == 0) 3.0 else -2.5);
        weights[16 + c] = if (c == 0) -0.09 else (if ((c & 1) == 0) -2.8 else 3.2);
    }

    let x: [16]f32 = [0.05; 16];
    x[0] = 64.0;

    // Exact FP32 reference y = W * x
    const ref_y0: f32 = @dot(weights[0..16], x[0..]);
    const ref_y1: f32 = @dot(weights[16..32], x[0..]);

    // 1. Naive RTN (all scales = 1.0)
    const ones: [16]f32 = [1.0; 16];
    let rtn_packed: [16]u8 = [0; 16];
    let rtn_packed_s: []u8 = rtn_packed[0..];
    let rtn_groups: [2]AwqGroupMeta = [AwqGroupMeta{}; 2];
    let rtn_groups_s: []AwqGroupMeta = rtn_groups[0..];
    expect_true(awqQuantizeMatrix(weights[0..], ones[0..], 2, 16, 16, *rtn_packed_s, *rtn_groups_s));

    let scratch: [16]f32 = [0.0; 16];
    let scratch_s: []f32 = scratch[0..];
    let y_rtn: [2]f32 = [0.0; 2];
    let y_rtn_s: []f32 = y_rtn[0..];
    expect_true(awqGemvF32(rtn_packed[0..], rtn_groups[0..], ones[0..], x[0..], 2, 16, 16, *scratch_s, *y_rtn_s));

    // 2. AWQ (activation-aware scales, alpha=0.5)
    let awq_scales: [16]f32 = [1.0; 16];
    let awq_scales_s: []f32 = awq_scales[0..];
    expect_true(computeAwqChannelScales(x[0..], 1, 16, 0.5, *awq_scales_s));

    let awq_packed: [16]u8 = [0; 16];
    let awq_packed_s: []u8 = awq_packed[0..];
    let awq_groups: [2]AwqGroupMeta = [AwqGroupMeta{}; 2];
    let awq_groups_s: []AwqGroupMeta = awq_groups[0..];
    expect_true(awqQuantizeMatrix(weights[0..], awq_scales[0..], 2, 16, 16, *awq_packed_s, *awq_groups_s));

    let y_awq: [2]f32 = [0.0; 2];
    let y_awq_s: []f32 = y_awq[0..];
    expect_true(awqGemvF32(awq_packed[0..], awq_groups[0..], awq_scales[0..], x[0..], 2, 16, 16, *scratch_s, *y_awq_s));

    const err_rtn = absF32(y_rtn[0] - ref_y0) + absF32(y_rtn[1] - ref_y1);
    const err_awq = absF32(y_awq[0] - ref_y0) + absF32(y_awq[1] - ref_y1);
    expect_true(err_awq < err_rtn);
    expect_true(err_awq < 1.0);
});

// ============================================================================
// Test 3: Fused awqGemvF32 Exact Parity with awqDequantizeRow + @dot (Partial Tail Group)
// ============================================================================
test("quantAwq: fused awqGemvF32 matches awqDequantizeRow + @dot across multi-group partial tail", () => {
    // rows=3, cols=40, group_size=16 -> 3 groups/row (16 + 16 + 8), bytes_per_row=20
    expect_eq(awqGroupsPerRow(40, 16), 3 as usize);
    expect_eq(awqPackedBytesPerRow(40), 20 as usize);

    let weights: [120]f32 = [0.0; 120];
    let scales: [40]f32 = [1.0; 40];
    let x: [40]f32 = [0.0; 40];

    let c: usize = 0;
    while (c < 40) : (c += 1) {
        scales[c] = 0.5 + ((c % 5) as f32) * 0.4;
        x[c] = ((c as f32) - 20.0) * 0.1;
        weights[c] = ((c as f32) * 0.07) - 1.2;
        weights[40 + c] = 1.5 - ((c as f32) * 0.08);
        weights[80 + c] = if ((c % 3) == 0) 0.9 else -0.4;
    }

    let q_packed: [60]u8 = [0; 60];
    let packed_s: []u8 = q_packed[0..];
    let groups: [9]AwqGroupMeta = [AwqGroupMeta{}; 9];
    let groups_s: []AwqGroupMeta = groups[0..];
    expect_true(awqQuantizeMatrix(weights[0..], scales[0..], 3, 40, 16, *packed_s, *groups_s));

    let scratch: [40]f32 = [0.0; 40];
    let scratch_s: []f32 = scratch[0..];
    let y_gemv: [3]f32 = [0.0; 3];
    let y_gemv_s: []f32 = y_gemv[0..];
    expect_true(awqGemvF32(q_packed[0..], groups[0..], scales[0..], x[0..], 3, 40, 16, *scratch_s, *y_gemv_s));

    // Compare each row against explicit dequantize + @dot
    let deq_row: [40]f32 = [0.0; 40];
    let deq_s: []f32 = deq_row[0..];
    let r: usize = 0;
    while (r < 3) : (r += 1) {
        expect_true(awqDequantizeRow(q_packed[0..], groups[0..], scales[0..], r, 3, 40, 16, *deq_s));
        const expected: f32 = @dot(deq_row[0..], x[0..]);
        expect_true(approxEqF32(y_gemv[r], expected, 1.0e-3));
    }
});

// ============================================================================
// Test 4: Quantization Fidelity Diagnostics (evaluateAwqQuantError)
// ============================================================================
test("quantAwq: evaluateAwqQuantError reports accurate MSE, max_abs_err & high cosine fidelity", () => {
    let weights: [64]f32 = [0.0; 64]; // 4 rows x 16 cols
    let i: usize = 0;
    while (i < 64) : (i += 1) {
        const idx_f = (i % 16) as f32;
        weights[i] = (idx_f - 7.5) * 0.2 + ((i / 16) as f32) * 0.1;
    }
    const scales: [16]f32 = [1.0; 16];
    let q_packed: [32]u8 = [0; 32];
    let packed_s: []u8 = q_packed[0..];
    let groups: [8]AwqGroupMeta = [AwqGroupMeta{}; 8]; // group_size=8 -> 2 groups/row
    let groups_s: []AwqGroupMeta = groups[0..];
    expect_true(awqQuantizeMatrix(weights[0..], scales[0..], 4, 16, 8, *packed_s, *groups_s));

    let row_scratch: [16]f32 = [0.0; 16];
    let row_s: []f32 = row_scratch[0..];
    if (stats := evaluateAwqQuantError(weights[0..], q_packed[0..], groups[0..], scales[0..], 4, 16, 8, *row_s)) {
        expect_eq(stats.rows, 4 as usize);
        expect_eq(stats.cols, 16 as usize);
        expect_true(stats.mse < 0.01);
        expect_true(stats.max_abs_err < 0.25);
        expect_true(stats.min_row_cosine > 0.99);
        expect_true(stats.mean_row_cosine > 0.99);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 5: All-Zero & Constant Weight Group Zero-Division Safety
// ============================================================================
test("quantAwq: all-zero and constant weight groups quantize and multiply without div-by-zero", () => {
    let weights: [24]f32 = [0.0; 24]; // 3 rows x 8 cols
    // Row 0: all zeros
    // Row 1: constant -2.0
    // Row 2: constant +3.0
    let c: usize = 0;
    while (c < 8) : (c += 1) {
        weights[8 + c] = -2.0;
        weights[16 + c] = 3.0;
    }
    const scales: [8]f32 = [1.0; 8];
    let q_packed: [12]u8 = [0; 12];
    let packed_s: []u8 = q_packed[0..];
    let groups: [3]AwqGroupMeta = [AwqGroupMeta{}; 3];
    let groups_s: []AwqGroupMeta = groups[0..];
    expect_true(awqQuantizeMatrix(weights[0..], scales[0..], 3, 8, 8, *packed_s, *groups_s));
    expect_true(approxEqF32(groups[0].scale, 0.0, 1.0e-6));
    expect_eq(groups[0].zero_point, 0 as u8);

    const x: [8]f32 = [1.0; 8];
    let scratch: [8]f32 = [0.0; 8];
    let scratch_s: []f32 = scratch[0..];
    let y: [3]f32 = [0.0; 3];
    let y_s: []f32 = y[0..];
    expect_true(awqGemvF32(q_packed[0..], groups[0..], scales[0..], x[0..], 3, 8, 8, *scratch_s, *y_s));

    expect_true(approxEqF32(y[0], 0.0, 1.0e-5));
    expect_true(approxEqF32(y[1], -16.0, 0.1));
    expect_true(approxEqF32(y[2], 24.0, 0.1));
});

// ============================================================================
// Test 6: NaN / Inf Activation & Weight Sanitization
// ============================================================================
test("quantAwq: NaN and Inf activations and weights are sanitized cleanly", () => {
    const nan_v: f32 = std.math.nan(f32);
    const inf_v: f32 = std.math.inf(f32);

    const dirty_acts: [4]f32 = [nan_v, 4.0, inf_v, 9.0];
    let scales: [4]f32 = [0.0; 4];
    let scales_s: []f32 = scales[0..];
    expect_true(computeAwqChannelScales(dirty_acts[0..], 1, 4, 0.5, *scales_s));
    expect_true(approxEqF32(scales[0], 1.0, 1.0e-5)); // NaN -> 0 -> neutral 1.0
    expect_true(approxEqF32(scales[1], 2.0, 1.0e-5));
    expect_true(approxEqF32(scales[2], 1.0, 1.0e-5)); // Inf -> 0 -> neutral 1.0
    expect_true(approxEqF32(scales[3], 3.0, 1.0e-5));

    const dirty_w: [4]f32 = [nan_v, 1.0, inf_v, -1.0];
    let q_packed: [2]u8 = [0; 2];
    let packed_s: []u8 = q_packed[0..];
    let groups: [1]AwqGroupMeta = [AwqGroupMeta{}];
    let groups_s: []AwqGroupMeta = groups[0..];
    expect_true(awqQuantizeMatrix(dirty_w[0..], scales[0..], 1, 4, 4, *packed_s, *groups_s));

    const dirty_x: [4]f32 = [inf_v, 2.0, nan_v, 1.0];
    let scratch: [4]f32 = [0.0; 4];
    let scratch_s: []f32 = scratch[0..];
    let y: [1]f32 = [0.0];
    let y_s: []f32 = y[0..];
    expect_true(awqGemvF32(q_packed[0..], groups[0..], scales[0..], dirty_x[0..], 1, 4, 4, *scratch_s, *y_s));
    expect_true(isFiniteF32(y[0]));
    // Expected ~ 1.0*2.0 + (-1.0)*1.0 = 1.0
    expect_true(approxEqF32(y[0], 1.0, 0.2));
});

// ============================================================================
// Test 7: Full Boundary Guards & cap.attention Dynamic Facade
// ============================================================================
test("quantAwq: boundary guards reject invalid group_size, odd cols, bad alpha & short buffers", () => {
    const w8: [8]f32 = [1.0; 8];
    const s4: [4]f32 = [1.0; 4];
    let p4: [4]u8 = [0; 4];
    let p4_s: []u8 = p4[0..];
    let g2: [2]AwqGroupMeta = [AwqGroupMeta{}; 2];
    let g2_s: []AwqGroupMeta = g2[0..];
    let sc4: [4]f32 = [0.0; 4];
    let sc4_s: []f32 = sc4[0..];
    let y2: [2]f32 = [0.0; 2];
    let y2_s: []f32 = y2[0..];

    // Invalid alpha
    expect_eq(computeAwqChannelScales(w8[0..], 2, 4, -0.1, *sc4_s), false);
    expect_eq(computeAwqChannelScales(w8[0..], 2, 4, 1.1, *sc4_s), false);
    expect_eq(computeAwqChannelScales(w8[0..], 2, 4, std.math.nan(f32), *sc4_s), false);

    // Invalid group_size == 0, odd cols == 3, rows == 0
    expect_eq(awqQuantizeMatrix(w8[0..], s4[0..], 2, 4, 0, *p4_s, *g2_s), false);
    expect_eq(awqQuantizeMatrix(w8[0..], s4[0..], 2, 3, 2, *p4_s, *g2_s), false);
    expect_eq(awqQuantizeMatrix(w8[0..], s4[0..], 0, 4, 4, *p4_s, *g2_s), false);

    // Undersized packed buffer
    let p1: [1]u8 = [0];
    let p1_s: []u8 = p1[0..];
    expect_eq(awqQuantizeMatrix(w8[0..], s4[0..], 2, 4, 4, *p1_s, *g2_s), false);

    // Valid quantize + dynamic cap.attention.awqGemvF32 facade
    expect_true(cap.attention.awqQuantizeMatrix(w8[0..], s4[0..], 2, 4, 4, *p4_s, *g2_s));
    expect_true(cap.attention.awqGemvF32(p4[0..], g2[0..], s4[0..], s4[0..], 2, 4, 4, *sc4_s, *y2_s));
    expect_true(approxEqF32(y2[0], 4.0, 0.2));

    // Out-of-range row_idx in awqDequantizeRow
    expect_eq(awqDequantizeRow(p4[0..], g2[0..], s4[0..], 2, 2, 4, 4, *sc4_s), false);
});
