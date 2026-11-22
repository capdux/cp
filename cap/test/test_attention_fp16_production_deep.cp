// Origin: Pure-CP Stdlib Test Suite (IEEE-754 FP16, BF16 & FP8 E4M3FN Low-Precision Float Kernels) | Created: 2026-11-22
// Deep Production Verification Suite for lib/attention/fp16.cp
// Covers:
//   1. FP16 classic bit vectors (1.0, -2.0, 65504.0, 2^-24 subnormal, +-0, +-Inf, NaN)
//   2. Exhaustive 65,536-codepoint FP16 round-trip: f16 -> f32 -> f16 bit-exact for all non-NaN values
//   3. BF16 exact conversion (0x3F80 == 1.0) & Round-to-Nearest-Even (RNE) halfway tie-to-even verification
//   4. FP8 E4M3FN bit vectors (0x38 == 1.0, 0x7E == 448.0, 0x7F/0xFF == NaN), saturation clamping & exhaustive 256-codepoint round-trip
//   5. Bulk LE slice decode/encode (FP16/BF16/scaled FP8) + odd-byte, undersized buffer & non-finite scale defenses

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    f16BitsToF32,
    f32ToF16Bits,
    decodeF16SliceLe,
    encodeF16SliceLe,
    bf16BitsToF32,
    f32ToBf16Bits,
    decodeBf16SliceLe,
    encodeBf16SliceLe,
    fp8E4m3ToF32,
    f32ToFp8E4m3,
    decodeFp8E4m3Slice,
} from "../../lib/attention/index.cp";

fn testFp16ClassicVectorsAndSpecials()! {
    expect_eq(f16BitsToF32(0x3C00), 1.0 as f32);
    expect_eq(f16BitsToF32(0xC000), -2.0 as f32);
    expect_eq(f16BitsToF32(0x7BFF), 65504.0 as f32);
    // Smallest positive subnormal: 2^-24
    const sub_min: f32 = f16BitsToF32(0x0001);
    const expected_sub_min: f32 = @bitCast(0x33800000 as u32); // 2^-24 in f32
    expect_eq(sub_min, expected_sub_min);

    // +0.0 and -0.0 sign preservation
    const pos0_bits: u32 = @bitCast(f16BitsToF32(0x0000));
    const neg0_bits: u32 = @bitCast(f16BitsToF32(0x8000));
    expect_eq(pos0_bits, 0x00000000);
    expect_eq(neg0_bits, 0x80000000);
    expect_eq(f32ToF16Bits(f16BitsToF32(0x8000)), 0x8000);

    // +-Inf and NaN
    expect_eq(std.math.isPositiveInf(f16BitsToF32(0x7C00)), true);
    expect_eq(std.math.isNegativeInf(f16BitsToF32(0xFC00)), true);
    expect_eq(std.math.isNan(f16BitsToF32(0x7E00)), true);
    expect_eq(f32ToF16Bits(std.math.inf(f32)), 0x7C00);
    expect_eq(f32ToF16Bits(0.0 - std.math.inf(f32)), 0xFC00);
    expect_eq(std.math.isNan(f16BitsToF32(f32ToF16Bits(std.math.nan(f32)))), true);

    // Overflow to +-Inf
    expect_eq(f32ToF16Bits(100000.0 as f32), 0x7C00);
    expect_eq(f32ToF16Bits(-100000.0 as f32), 0xFC00);
}

fn testFp16Exhaustive65536RoundTrip()! {
    let code: u32 = 0;
    while (code < 65536) : (code += 1) {
        const h: u16 = code as u16;
        const f: f32 = f16BitsToF32(h);
        const rt: u16 = f32ToF16Bits(f);
        const is_nan = ((h & 0x7C00) == 0x7C00) && ((h & 0x03FF) != 0);
        if (is_nan) {
            expect_eq(std.math.isNan(f), true);
            expect_eq(((rt & 0x7C00) == 0x7C00) && ((rt & 0x03FF) != 0), true);
        } else {
            expect_eq(rt, h);
        }
    }
}

fn testBf16ConversionAndRneRounding()! {
    expect_eq(bf16BitsToF32(0x3F80), 1.0 as f32);
    expect_eq(bf16BitsToF32(0xC000), -2.0 as f32);
    expect_eq(f32ToBf16Bits(1.0 as f32), 0x3F80);
    expect_eq(f32ToBf16Bits(-2.0 as f32), 0xC000);

    // Round-to-Nearest-Even (RNE) halfway check:
    // 0x3F80_8000 is exact midpoint between 0x3F80 (even) and 0x3F81 (odd) -> rounds to even 0x3F80
    const tie_even: f32 = @bitCast(0x3F808000 as u32);
    expect_eq(f32ToBf16Bits(tie_even), 0x3F80);

    // 0x3F81_8000 is exact midpoint between 0x3F81 (odd) and 0x3F82 (even) -> rounds to even 0x3F82
    const tie_odd: f32 = @bitCast(0x3F818000 as u32);
    expect_eq(f32ToBf16Bits(tie_odd), 0x3F82);

    // Strictly above midpoint rounds up
    const above_half: f32 = @bitCast(0x3F808001 as u32);
    expect_eq(f32ToBf16Bits(above_half), 0x3F81);

    // Inf and NaN preservation
    expect_eq(f32ToBf16Bits(std.math.inf(f32)), 0x7F80);
    expect_eq(f32ToBf16Bits(0.0 - std.math.inf(f32)), 0xFF80);
    expect_eq(std.math.isNan(bf16BitsToF32(f32ToBf16Bits(std.math.nan(f32)))), true);
}

fn testFp8E4m3VectorsSaturationAndExhaustiveRoundTrip()! {
    expect_eq(fp8E4m3ToF32(0x38), 1.0 as f32);
    expect_eq(fp8E4m3ToF32(0xB8), -1.0 as f32);
    expect_eq(fp8E4m3ToF32(0x7E), 448.0 as f32);
    expect_eq(fp8E4m3ToF32(0xFE), -448.0 as f32);
    expect_eq(std.math.isNan(fp8E4m3ToF32(0x7F)), true);
    expect_eq(std.math.isNan(fp8E4m3ToF32(0xFF)), true);

    // Smallest positive subnormal: 2^-9 = 0.001953125
    expect_eq(fp8E4m3ToF32(0x01), 0.001953125 as f32);

    // Saturation clamping for out-of-range finite values and +-Inf
    expect_eq(f32ToFp8E4m3(1000.0 as f32), 0x7E);
    expect_eq(f32ToFp8E4m3(-1000.0 as f32), 0xFE);
    expect_eq(f32ToFp8E4m3(std.math.inf(f32)), 0x7E);
    expect_eq(f32ToFp8E4m3(0.0 - std.math.inf(f32)), 0xFE);
    expect_eq(f32ToFp8E4m3(std.math.nan(f32)) & 0x7F, 0x7F);

    // Exhaustive 256-codepoint round-trip
    let code: u16 = 0;
    while (code < 256) : (code += 1) {
        const b: u8 = code as u8;
        const f: f32 = fp8E4m3ToF32(b);
        const rt: u8 = f32ToFp8E4m3(f);
        if ((b & 0x7F) == 0x7F) {
            expect_eq(std.math.isNan(f), true);
            expect_eq(rt & 0x7F, 0x7F);
        } else {
            expect_eq(rt, b);
        }
    }
}

fn testBulkSlicesAndBoundaryDefenses()! {
    const src: [4]f32 = [1.0, -2.0, 0.5, 448.0];
    let bytes_buf: [8]u8 = [0; 8];
    let bytes: []u8 = bytes_buf[0..];
    let dst_buf: [4]f32 = [0.0; 4];
    let dst: []f32 = dst_buf[0..];

    // FP16 LE round-trip
    expect_eq(encodeF16SliceLe(src[0..], bytes), true);
    expect_eq(bytes_buf[0], 0x00); // 1.0 = 0x3C00 LE -> [0x00, 0x3C]
    expect_eq(bytes_buf[1], 0x3C);
    expect_eq(decodeF16SliceLe(bytes_buf[0..], dst), true);
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_eq(dst_buf[i], src[i]);
    }

    // BF16 LE round-trip
    expect_eq(encodeBf16SliceLe(src[0..], bytes), true);
    expect_eq(bytes_buf[0], 0x80); // 1.0 = 0x3F80 LE -> [0x80, 0x3F]
    expect_eq(bytes_buf[1], 0x3F);
    expect_eq(decodeBf16SliceLe(bytes_buf[0..], dst), true);
    let j: usize = 0;
    while (j < 4) : (j += 1) {
        expect_eq(dst_buf[j], src[j]);
    }

    // FP8 E4M3 scaled bulk decode
    const fp8_src: [4]u8 = [0x38, 0xB8, 0x40, 0x7E]; // [1.0, -1.0, 2.0, 448.0]
    expect_eq(decodeFp8E4m3Slice(fp8_src[0..], 0.25 as f32, dst), true);
    expect_eq(dst_buf[0], 0.25 as f32);
    expect_eq(dst_buf[1], -0.25 as f32);
    expect_eq(dst_buf[2], 0.5 as f32);
    expect_eq(dst_buf[3], 112.0 as f32);

    // Boundary defenses: odd byte length, undersized buffers, NaN/Inf scale
    let short_dst: []f32 = dst_buf[0..2];
    let short_bytes: []u8 = bytes_buf[0..6];
    expect_eq(decodeF16SliceLe(bytes_buf[0..7], dst), false);
    expect_eq(decodeF16SliceLe(bytes_buf[0..], short_dst), false);
    expect_eq(encodeF16SliceLe(src[0..], short_bytes), false);
    expect_eq(decodeBf16SliceLe(bytes_buf[0..7], dst), false);
    expect_eq(decodeBf16SliceLe(bytes_buf[0..], short_dst), false);
    expect_eq(encodeBf16SliceLe(src[0..], short_bytes), false);
    expect_eq(decodeFp8E4m3Slice(fp8_src[0..], 1.0 as f32, short_dst), false);
    expect_eq(decodeFp8E4m3Slice(fp8_src[0..], std.math.nan(f32), dst), false);
    expect_eq(decodeFp8E4m3Slice(fp8_src[0..], std.math.inf(f32), dst), false);
}

fn main() {
    test("1. fp16: classic vectors (1.0, -2.0, 65504.0, 2^-24 subnormal, +-0, +-Inf, NaN)", () => {
        testFp16ClassicVectorsAndSpecials()?;
    });
    test("2. fp16: exhaustive 65,536-codepoint f16 -> f32 -> f16 bit-exact round-trip", () => {
        testFp16Exhaustive65536RoundTrip()?;
    });
    test("3. bf16: exact conversion & Round-to-Nearest-Even (RNE) halfway tie-to-even", () => {
        testBf16ConversionAndRneRounding()?;
    });
    test("4. fp8: E4M3FN vectors, saturation to +-448.0 & exhaustive 256-codepoint round-trip", () => {
        testFp8E4m3VectorsSaturationAndExhaustiveRoundTrip()?;
    });
    test("5. fp16/bf16/fp8: LE bulk slice decode/encode & odd-byte/buffer/scale defenses", () => {
        testBulkSlicesAndBoundaryDefenses()?;
    });
}
