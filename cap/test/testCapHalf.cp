// cap/test/testCapHalf.cp
// Comprehensive unit tests for @cap/half

import { test, expect_eq } from "cap:test";
import {
    F16,
    Bf16,
    f32BitsToF16Bits,
    f16BitsToF32Bits,
    f32BitsToBf16Bits,
    bf16BitsToF32Bits,
} from "../../packages/half/src/index.cp";

fn main() {
    test("1. @cap/half — f16 exact canonical values (0, -0, 1, -2, 65504)", () => {
        // 0.0 -> 0x0000, -0.0 -> 0x8000
        expect_eq(f32BitsToF16Bits(0x00000000), 0x0000 as u16);
        expect_eq(f32BitsToF16Bits(0x80000000), 0x8000 as u16);
        // 1.0f32 (0x3F800000) -> 0x3C00
        expect_eq(f32BitsToF16Bits(0x3F800000), 0x3C00 as u16);
        expect_eq(f16BitsToF32Bits(0x3C00), 0x3F800000 as u32);
        // -2.0f32 (0xC0000000) -> 0xC000
        expect_eq(f32BitsToF16Bits(0xC0000000), 0xC000 as u16);
        expect_eq(f16BitsToF32Bits(0xC000), 0xC0000000 as u32);
        // 65504.0f32 (0x477FE000) -> max finite f16 0x7BFF
        expect_eq(f32BitsToF16Bits(0x477FE000), 0x7BFF as u16);
        expect_eq(f16BitsToF32Bits(0x7BFF), 0x477FE000 as u32);
    });

    test("2. @cap/half — f16 infinity, NaN & overflow handling", () => {
        expect_eq(f32BitsToF16Bits(0x7F800000), 0x7C00 as u16); // +Inf
        expect_eq(f32BitsToF16Bits(0xFF800000), 0xFC00 as u16); // -Inf
        expect_eq(f16BitsToF32Bits(0x7C00), 0x7F800000 as u32);
        // 100000.0f32 (0x47C35000) overflows f16 -> +Inf
        expect_eq(f32BitsToF16Bits(0x47C35000), 0x7C00 as u16);

        const nanH: F16 = F16.fromBits(f32BitsToF16Bits(0x7FC00000));
        expect_eq(nanH.isNan(), true);
        expect_eq(nanH.isFinite(), false);
        expect_eq(nanH.isInfinite(), false);
    });

    test("3. @cap/half — f16 subnormal round-trip (smallest positive 2^-24)", () => {
        // 2^-24 in f32 = 0x33800000 -> smallest f16 subnormal 0x0001
        expect_eq(f32BitsToF16Bits(0x33800000), 0x0001 as u16);
        expect_eq(f16BitsToF32Bits(0x0001), 0x33800000 as u32);
        // Largest f16 subnormal 0x03FF = (1023/1024) * 2^-14 -> f32 0x387FC000
        expect_eq(f16BitsToF32Bits(0x03FF), 0x387FC000 as u32);
    });

    test("4. @cap/half — F16 helper methods (abs, neg, sign, zero)", () => {
        const negOne: F16 = F16.fromBits(0xBC00); // -1.0
        expect_eq(negOne.isSignNegative(), true);
        expect_eq(negOne.isFinite(), true);
        expect_eq(negOne.abs().bits, 0x3C00 as u16);
        expect_eq(negOne.neg().bits, 0x3C00 as u16);
        expect_eq(F16.fromBits(0x8000).isZero(), true);
    });

    test("5. @cap/half — bfloat16 exact conversion & round-to-nearest-even", () => {
        // 1.0f32 (0x3F800000) -> 0x3F80
        expect_eq(f32BitsToBf16Bits(0x3F800000), 0x3F80 as u16);
        expect_eq(bf16BitsToF32Bits(0x3F80), 0x3F800000 as u32);
        // Tie to even: 0x3F808000 (even LSB 0) -> 0x3F80; 0x3F818000 (odd LSB 1) -> 0x3F82
        expect_eq(f32BitsToBf16Bits(0x3F808000), 0x3F80 as u16);
        expect_eq(f32BitsToBf16Bits(0x3F818000), 0x3F82 as u16);
    });

    test("6. @cap/half — Bf16 special values (Inf, NaN, neg, abs)", () => {
        const inf: Bf16 = Bf16.fromBits(f32BitsToBf16Bits(0x7F800000));
        expect_eq(inf.isInfinite(), true);
        expect_eq(inf.isNan(), false);
        expect_eq(inf.neg().bits, 0xFF80 as u16);
        expect_eq(inf.neg().abs().bits, 0x7F80 as u16);

        const qnan: Bf16 = Bf16.fromBits(f32BitsToBf16Bits(0x7FC00001));
        expect_eq(qnan.isNan(), true);
    });
}
