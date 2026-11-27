// cap/test/testCapHalf.cp
// Comprehensive unit tests for @cap/half (IEEE 754 F16 & BFloat16)

import { test, expect_eq } from "cap:test";
import {
    F16,
    Bf16,
    f32ToBits,
    bitsToF32,
    f32SliceToF16,
    f16SliceToF32,
} from "../../packages/half/src/index.cp";

fn main() {
    test("1. @cap/half — F16 exact bit vectors, special values & LE bytes", () => {
        expect_eq(F16.fromF32(0.0).bits, 0x0000 as u16);
        expect_eq(F16.fromF32(1.0).bits, 0x3C00 as u16);
        expect_eq(F16.fromF32(-2.0).bits, 0xC000 as u16);
        expect_eq(F16.fromBits(0x3C00 as u16).toF32(), 1.0 as f32);
        expect_eq(F16.fromBits(0x7C00 as u16).isInfinite(), true);
        expect_eq(F16.fromF32(bitsToF32(0x7FC00000 as u32)).isNan(), true);

        const le: [2]u8 = F16.fromBits(0x3C00 as u16).toLeBytes();
        expect_eq(le[0], 0x00 as u8);
        expect_eq(le[1], 0x3C as u8);
        expect_eq(F16.fromLeBytes(le[0], le[1]).bits, 0x3C00 as u16);
    });

    test("2. @cap/half — Bf16 conversion & slice batch conversion", () => {
        expect_eq(Bf16.fromF32(1.0).bits, 0x3F80 as u16);
        expect_eq(Bf16.fromF32(-2.0).bits, 0xC000 as u16);
        expect_eq(Bf16.fromBits(0x3F80 as u16).toF32(), 1.0 as f32);
        expect_eq(f32ToBits(1.0 as f32), 0x3F800000 as u32);

        let src: [3]f32 = [1.0, -2.0, 0.5];
        let mid: [3]u16 = [0; 3];
        let dst: [3]f32 = [0.0; 3];
        expect_eq(f32SliceToF16(src[0..3], mid[0..3]), 3 as usize);
        expect_eq(f16SliceToF32(mid[0..3], dst[0..3]), 3 as usize);
        expect_eq(dst[0], 1.0 as f32);
        expect_eq(dst[1], -2.0 as f32);
        expect_eq(dst[2], 0.5 as f32);
    });
}
