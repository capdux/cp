// Origin: Pure-CP Stdlib Test Suite (lib/attention/quant_int4.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation Per-Group INT4 quantization (Sym & Asym)
// and scale-hoisted fused GEMV: nibble bit-packing, sym/asym error bounds, zero/outlier group
// isolation, fused GEMV accuracy vs f32 baseline, and invalid dimension/group_size guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Int4QuantMode,
    Int4GroupParams,
    packInt4Pair,
    unpackInt4Lo,
    unpackInt4Hi,
    quantizeGroupInt4Sym,
    quantizeGroupInt4Asym,
    dequantizeGroupInt4,
    gemvInt4Group,
} from "cap:attention";

fn absF32(x: f32): f32 {
    if (x < 0.0) return -x;
    return x;
}

fn testNibblePackUnpackExhaustiveRoundtrip()! {
    let lo: u16 = 0;
    while (lo < 16) : (lo += 1) {
        let hi: u16 = 0;
        while (hi < 16) : (hi += 1) {
            const packed_byte: u8 = cap.attention.packInt4Pair(lo as u8, hi as u8);
            expect_eq(unpackInt4Lo(packed_byte), lo as u8);
            expect_eq(unpackInt4Hi(packed_byte), hi as u8);
        }
    }
    // Upper nibble garbage bits must be masked cleanly.
    const masked: u8 = packInt4Pair(0xF5, 0xCA);
    expect_eq(unpackInt4Lo(masked), 0x05);
    expect_eq(unpackInt4Hi(masked), 0x0A);
}

fn testSymAndAsymQuantizationErrorBounds()! {
    // 1. Symmetric quantization over [-7.0, 7.0]: exact integer grid + half-step bound.
    const sym_src: [8]f32 = [-7.0, -3.5, -1.0, 0.0, 1.0, 2.3, 5.6, 7.0];
    let sym_packed: [4]u8 = undefined;
    let sym_params: [1]Int4GroupParams = undefined;
    let sym_dst: [8]f32 = undefined;

    let sym_packed_s: []u8 = sym_packed[0..];
    let sym_params_s: []Int4GroupParams = sym_params[0..];
    let sym_dst_s: []f32 = sym_dst[0..];

    expect_eq(quantizeGroupInt4Sym(sym_src[0..], 8, sym_packed_s, sym_params_s), true);
    expect_eq(sym_params[0].group_size, 8);
    expect_eq(sym_params[0].zero_point, 8);
    expect_eq(absF32(sym_params[0].scale - 1.0) < 0.0001, true);

    expect_eq(dequantizeGroupInt4(sym_packed[0..], sym_params[0..], .Symmetric, sym_dst_s), true);
    const sym_bound: f32 = sym_params[0].scale * 0.5 + 0.0001;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        expect_eq(absF32(sym_src[i] - sym_dst[i]) <= sym_bound, true);
    }
    expect_eq(sym_dst[3], 0.0);

    // 2. Asymmetric quantization over skewed range [-1.0, 14.0]:
    //    range = 15.0 => scale = 1.0, zero_point = 1 => exact recovery for all integer values,
    //    strictly tighter than symmetric (which requires scale = 14/7 = 2.0).
    const asym_src: [8]f32 = [-1.0, 0.0, 1.0, 3.0, 6.0, 9.0, 12.0, 14.0];
    let asym_packed: [4]u8 = undefined;
    let asym_params: [1]Int4GroupParams = undefined;
    let asym_dst: [8]f32 = undefined;

    let asym_packed_s: []u8 = asym_packed[0..];
    let asym_params_s: []Int4GroupParams = asym_params[0..];
    let asym_dst_s: []f32 = asym_dst[0..];

    expect_eq(quantizeGroupInt4Asym(asym_src[0..], 8, asym_packed_s, asym_params_s), true);
    expect_eq(asym_params[0].group_size, 8);
    expect_eq(asym_params[0].zero_point, 1);
    expect_eq(absF32(asym_params[0].scale - 1.0) < 0.0001, true);

    expect_eq(dequantizeGroupInt4(asym_packed[0..], asym_params[0..], .Asymmetric, asym_dst_s), true);
    i = 0;
    while (i < 8) : (i += 1) {
        expect_eq(absF32(asym_src[i] - asym_dst[i]) < 0.0001, true);
    }
}

fn testZeroVectorAndOutlierGroupIsolation()! {
    // Group 0: all zeros
    // Group 1: large outlier (70.0)
    // Group 2: fine-grained small weights ([-0.7, 0.7]) that must NOT be degraded by Group 1's outlier
    const src: [12]f32 = [
        0.0, 0.0, 0.0, 0.0,
        70.0, -30.0, 10.0, 0.0,
        -0.7, -0.3, 0.4, 0.7,
    ];
    let packed_buf: [6]u8 = undefined;
    let params: [3]Int4GroupParams = undefined;
    let dst: [12]f32 = undefined;

    let packed_s: []u8 = packed_buf[0..];
    let params_s: []Int4GroupParams = params[0..];
    let dst_s: []f32 = dst[0..];

    expect_eq(quantizeGroupInt4Sym(src[0..], 4, packed_s, params_s), true);
    expect_eq(params[0].scale, 0.0);
    expect_eq(absF32(params[1].scale - 10.0) < 0.001, true);
    expect_eq(absF32(params[2].scale - 0.1) < 0.001, true);

    expect_eq(dequantizeGroupInt4(packed_buf[0..], params[0..], .Symmetric, dst_s), true);
    // Group 0 strictly zero
    let j: usize = 0;
    while (j < 4) : (j += 1) {
        expect_eq(dst[j], 0.0);
    }
    // Group 2 isolated precision: exact 0.1 grid recovery despite 70.0 outlier in Group 1
    expect_eq(absF32(dst[8] - (-0.7)) < 0.001, true);
    expect_eq(absF32(dst[9] - (-0.3)) < 0.001, true);
    expect_eq(absF32(dst[10] - 0.4) < 0.001, true);
    expect_eq(absF32(dst[11] - 0.7) < 0.001, true);

    // Also verify all-zero asymmetric group produces clean 0.0 output.
    const z_src: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    let z_packed: [2]u8 = undefined;
    let z_params: [1]Int4GroupParams = undefined;
    let z_dst: [4]f32 = undefined;
    let z_packed_s: []u8 = z_packed[0..];
    let z_params_s: []Int4GroupParams = z_params[0..];
    let z_dst_s: []f32 = z_dst[0..];
    expect_eq(quantizeGroupInt4Asym(z_src[0..], 4, z_packed_s, z_params_s), true);
    expect_eq(z_params[0].scale, 0.0);
    expect_eq(dequantizeGroupInt4(z_packed[0..], z_params[0..], .Asymmetric, z_dst_s), true);
    expect_eq(z_dst[0], 0.0);
    expect_eq(z_dst[3], 0.0);
}

fn testFusedGemvInt4AgainstF32AndDequantBaseline()! {
    // 3x8 matrix quantized with group_size = 4 (2 groups/row, 6 groups total, 12 packed bytes).
    const w_f32: [24]f32 = [
         0.5, -1.0,  1.5,  2.0,   -0.2,  0.4,  0.6, -0.8,
         3.0,  6.0,  9.0, 12.0,    0.0,  0.0,  0.0,  0.0,
        -1.0,  0.0,  2.0,  5.0,   -3.0, -1.5,  0.0,  1.5,
    ];
    const x: [8]f32 = [1.0, -0.5, 0.25, 1.5, -1.0, 2.0, 0.5, -0.25];

    // Exact f32 reference y_ref = W * x.
    let y_ref: [3]f32 = [0.0, 0.0, 0.0];
    let r: usize = 0;
    while (r < 3) : (r += 1) {
        let sum: f32 = 0.0;
        let c: usize = 0;
        while (c < 8) : (c += 1) {
            sum += w_f32[r * 8 + c] * x[c];
        }
        y_ref[r] = sum;
    }

    // --- Symmetric fused GEMV ---
    let sym_packed: [12]u8 = undefined;
    let sym_params: [6]Int4GroupParams = undefined;
    let sym_deq: [24]f32 = undefined;
    let y_sym: [3]f32 = undefined;

    let sym_packed_s: []u8 = sym_packed[0..];
    let sym_params_s: []Int4GroupParams = sym_params[0..];
    let sym_deq_s: []f32 = sym_deq[0..];
    let y_sym_s: []f32 = y_sym[0..];

    expect_eq(quantizeGroupInt4Sym(w_f32[0..], 4, sym_packed_s, sym_params_s), true);
    expect_eq(dequantizeGroupInt4(sym_packed[0..], sym_params[0..], .Symmetric, sym_deq_s), true);
    expect_eq(
        cap.attention.gemvInt4Group(x[0..], sym_packed[0..], sym_params[0..], 3, 8, 4, .Symmetric, y_sym_s),
        true,
    );

    r = 0;
    while (r < 3) : (r += 1) {
        let deq_dot: f32 = 0.0;
        let c: usize = 0;
        while (c < 8) : (c += 1) {
            deq_dot += sym_deq[r * 8 + c] * x[c];
        }
        // Fused GEMV must match dequantized reference within float rounding tolerance.
        expect_eq(absF32(y_sym[r] - deq_dot) < 0.0005, true);
        // And stay close to unquantized f32 baseline.
        expect_eq(absF32(y_sym[r] - y_ref[r]) < 0.75, true);
    }

    // --- Asymmetric fused GEMV ---
    let asym_packed: [12]u8 = undefined;
    let asym_params: [6]Int4GroupParams = undefined;
    let asym_deq: [24]f32 = undefined;
    let y_asym: [3]f32 = undefined;

    let asym_packed_s: []u8 = asym_packed[0..];
    let asym_params_s: []Int4GroupParams = asym_params[0..];
    let asym_deq_s: []f32 = asym_deq[0..];
    let y_asym_s: []f32 = y_asym[0..];

    expect_eq(quantizeGroupInt4Asym(w_f32[0..], 4, asym_packed_s, asym_params_s), true);
    expect_eq(dequantizeGroupInt4(asym_packed[0..], asym_params[0..], .Asymmetric, asym_deq_s), true);
    expect_eq(
        gemvInt4Group(x[0..], asym_packed[0..], asym_params[0..], 3, 8, 4, .Asymmetric, y_asym_s),
        true,
    );

    r = 0;
    while (r < 3) : (r += 1) {
        let deq_dot: f32 = 0.0;
        let c: usize = 0;
        while (c < 8) : (c += 1) {
            deq_dot += asym_deq[r * 8 + c] * x[c];
        }
        expect_eq(absF32(y_asym[r] - deq_dot) < 0.0005, true);
        expect_eq(absF32(y_asym[r] - y_ref[r]) < 0.70, true);
    }
}

fn testInvalidDimensionsOddGroupSizeAndNanGuards()! {
    const valid_src: [8]f32 = [1.0, 2.0, 3.0, 4.0, -1.0, -2.0, -3.0, -4.0];
    const bad_nan: [4]f32 = [1.0, std.math.nan(f32), 2.0, 3.0];
    const bad_inf: [4]f32 = [1.0, std.math.inf(f32), 2.0, 3.0];

    let packed_buf: [4]u8 = undefined;
    let params: [2]Int4GroupParams = undefined;
    let dst: [8]f32 = undefined;
    let y: [2]f32 = undefined;

    let packed_s: []u8 = packed_buf[0..];
    let params_s: []Int4GroupParams = params[0..];
    let dst_s: []f32 = dst[0..];
    let y_s: []f32 = y[0..];

    // Odd or zero group_size rejected.
    expect_eq(quantizeGroupInt4Sym(valid_src[0..], 0, packed_s, params_s), false);
    expect_eq(quantizeGroupInt4Sym(valid_src[0..], 1, packed_s, params_s), false);
    expect_eq(quantizeGroupInt4Sym(valid_src[0..], 3, packed_s, params_s), false);
    expect_eq(quantizeGroupInt4Asym(valid_src[0..], 5, packed_s, params_s), false);

    // Misaligned src.len % group_size != 0 or empty src rejected.
    expect_eq(quantizeGroupInt4Sym(valid_src[0..6], 4, packed_s, params_s), false);
    expect_eq(quantizeGroupInt4Asym(valid_src[0..0], 4, packed_s, params_s), false);

    // Insufficient output buffers rejected.
    expect_eq(quantizeGroupInt4Sym(valid_src[0..], 4, packed_buf[0..3], params_s), false);
    expect_eq(quantizeGroupInt4Sym(valid_src[0..], 4, packed_s, params[0..1]), false);

    // NaN / Inf inputs rejected.
    expect_eq(quantizeGroupInt4Sym(bad_nan[0..], 4, packed_s, params_s), false);
    expect_eq(quantizeGroupInt4Asym(bad_inf[0..], 4, packed_s, params_s), false);

    // Prepare valid quantized data for dequantize / GEMV negative checks.
    expect_eq(quantizeGroupInt4Asym(valid_src[0..], 4, packed_s, params_s), true);

    // dequantizeGroupInt4 guards: invalid zero_point > 15, negative scale, truncated dst.
    let bad_params: [2]Int4GroupParams = [params[0], params[1]];
    bad_params[0].zero_point = 16;
    expect_eq(dequantizeGroupInt4(packed_buf[0..], bad_params[0..], .Asymmetric, dst_s), false);
    bad_params[0].zero_point = 0;
    bad_params[0].scale = -1.0;
    expect_eq(dequantizeGroupInt4(packed_buf[0..], bad_params[0..], .Asymmetric, dst_s), false);
    expect_eq(dequantizeGroupInt4(packed_buf[0..], params[0..], .Asymmetric, dst[0..4]), false);

    // gemvInt4Group guards: odd group_size, cols % group_size != 0, mismatched param group_size, NaN x.
    const x_ok: [4]f32 = [1.0, 1.0, 1.0, 1.0];
    expect_eq(gemvInt4Group(x_ok[0..], packed_buf[0..], params[0..], 0, 4, 4, .Asymmetric, y_s), false);
    expect_eq(gemvInt4Group(x_ok[0..], packed_buf[0..], params[0..], 2, 4, 3, .Asymmetric, y_s), false);
    expect_eq(gemvInt4Group(x_ok[0..], packed_buf[0..], params[0..], 2, 4, 8, .Asymmetric, y_s), false);
    expect_eq(gemvInt4Group(bad_nan[0..], packed_buf[0..], params[0..], 2, 4, 4, .Asymmetric, y_s), false);
}

fn main() {
    test("1. attention/quant_int4: exhaustive nibble pack/unpack roundtrip & bit masking", () => {
        testNibblePackUnpackExhaustiveRoundtrip()?;
    });

    test("2. attention/quant_int4: symmetric & asymmetric per-group quantization error bounds", () => {
        testSymAndAsymQuantizationErrorBounds()?;
    });

    test("3. attention/quant_int4: zero-vector handling & outlier group precision isolation", () => {
        testZeroVectorAndOutlierGroupIsolation()?;
    });

    test("4. attention/quant_int4: fused INT4 GEMV vs dequantized & f32 baseline accuracy", () => {
        testFusedGemvInt4AgainstF32AndDequantBaseline()?;
    });

    test("5. attention/quant_int4: invalid dimensions, odd group_size & NaN/Inf guards", () => {
        testInvalidDimensionsOddGroupSizeAndNanGuards()?;
    });
}
