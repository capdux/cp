import { test, expect_eq } from "cap:test";
import {
    F16,
    Bf16,
    f32ToBits,
    bitsToF32,
    f32SliceToF16,
    f16SliceToF32,
} from "../../packages/@cap/half-f16/src/index.cp";

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

fn main() {
    test("1. @cap/half-f16 — IEEE 754 f16 standard bit vectors & exact round-trip", () => {
        expect_eq(F16.fromF32(0.0).bits, 0x0000);
        expect_eq(F16.fromF32(bitsToF32(0x80000000)).bits, 0x8000); // -0.0
        expect_eq(F16.fromF32(1.0).bits, 0x3C00);
        expect_eq(F16.fromF32(-2.0).bits, 0xC000);
        expect_eq(F16.fromF32(65504.0).bits, 0x7BFF);
        expect_eq(F16.fromF32(bitsToF32(0x7F800000)).bits, 0x7C00); // +Inf

        const h1: F16 = F16.fromBits(0x3C00);
        const h2: F16 = F16.fromBits(0xC000);
        const hmax: F16 = F16.fromBits(0x7BFF);
        expect_eq(h1.toF32(), 1.0 as f32);
        expect_eq(h2.toF32(), -2.0 as f32);
        expect_eq(hmax.toF32(), 65504.0 as f32);
    });

    test("2. @cap/half-f16 — f16 subnormals, underflow & overflow to Inf", () => {
        // Smallest positive f16 subnormal: 0x0001 = 2^-24 (~5.9604645e-8)
        const min_sub: F16 = F16.fromBits(0x0001);
        const sub_f32: f32 = min_sub.toF32();
        expect_eq(F16.fromF32(sub_f32).bits, 0x0001);

        // Largest f16 subnormal: 0x03FF
        const max_sub: F16 = F16.fromBits(0x03FF);
        expect_eq(F16.fromF32(max_sub.toF32()).bits, 0x03FF);

        // Overflow >= 65520 -> +Inf (0x7C00), <= -65520 -> -Inf (0xFC00)
        expect_eq(F16.fromF32(65520.0).bits, 0x7C00);
        expect_eq(F16.fromF32(100000.0).bits, 0x7C00);
        expect_eq(F16.fromF32(-100000.0).bits, 0xFC00);

        // Extreme underflow -> signed zero
        expect_eq(F16.fromF32(bitsToF32(0x00800000)).bits, 0x0000);
        expect_eq(F16.fromF32(bitsToF32(0x80800000)).bits, 0x8000);
    });

    test("3. @cap/half-f16 — Bf16 exact vectors & 3.14 RTNE rounding", () => {
        expect_eq(Bf16.fromF32(0.0).bits, 0x0000);
        expect_eq(Bf16.fromF32(1.0).bits, 0x3F80);
        expect_eq(Bf16.fromF32(-2.0).bits, 0xC000);

        const b1: Bf16 = Bf16.fromBits(0x3F80);
        expect_eq(b1.toF32(), 1.0 as f32);

        // 3.14 as f32 is 0x4048F5C3 -> RTNE rounds low 0xF5C3 up -> 0x4049 (3.140625)
        const b_pi: Bf16 = Bf16.fromF32(3.14 as f32);
        expect_eq(b_pi.bits, 0x4049);
        const diff: f32 = absF32(b_pi.toF32() - (3.140625 as f32));
        expect_eq(diff < (0.0001 as f32), true);
    });

    test("4. @cap/half-f16 — isNan, isInfinite, isZero, neg & abs for F16 & Bf16", () => {
        const h_nan: F16 = F16.fromF32(bitsToF32(0x7FC00000));
        const h_inf: F16 = F16.fromBits(0x7C00);
        const h_ninf: F16 = h_inf.neg();
        const h_zero: F16 = F16.fromBits(0x8000);
        const h_neg: F16 = F16.fromF32(-3.5);
        const h_abs: F16 = h_neg.abs();

        expect_eq(h_nan.isNan(), true);
        expect_eq(h_nan.isInfinite(), false);
        expect_eq(h_inf.isInfinite(), true);
        expect_eq(h_ninf.bits, 0xFC00);
        expect_eq(h_ninf.isInfinite(), true);
        expect_eq(h_zero.isZero(), true);
        expect_eq(h_abs.toF32(), 3.5 as f32);

        const b_nan: Bf16 = Bf16.fromF32(bitsToF32(0x7FC00001));
        const b_inf: Bf16 = Bf16.fromBits(0x7F80);
        const b_neg: Bf16 = Bf16.fromF32(-4.0);
        const b_abs: Bf16 = b_neg.abs();
        expect_eq(b_nan.isNan(), true);
        expect_eq(b_inf.isInfinite(), true);
        expect_eq(b_abs.toF32(), 4.0 as f32);
        expect_eq(f32ToBits(bitsToF32(0x3F800000)), 0x3F800000);
    });

    test("5. @cap/half-f16 — Little-endian byte serialization round-trip", () => {
        const h: F16 = F16.fromF32(1.5); // 0x3E00 -> LE [0x00, 0x3E]
        expect_eq(h.bits, 0x3E00);
        const le: [2]u8 = h.toLeBytes();
        expect_eq(le[0], 0x00);
        expect_eq(le[1], 0x3E);

        const decoded: F16 = F16.fromLeBytes(le[0], le[1]);
        expect_eq(decoded.bits, 0x3E00);
        expect_eq(decoded.toF32(), 1.5 as f32);
    });

    test("6. @cap/half-f16 — Batch f32SliceToF16 & f16SliceToF32 tensor conversion", () => {
        const src: [5]f32 = [0.0, 1.0, -2.0, 0.5, 16.25];
        let f16_buf: [5]u16 = undefined;
        let unpacked: [5]f32 = undefined;

        const n1 = f32SliceToF16(&src, &f16_buf);
        expect_eq(n1, 5);
        expect_eq(f16_buf[1], 0x3C00);
        expect_eq(f16_buf[2], 0xC000);

        const n2 = f16SliceToF32(&f16_buf, &unpacked);
        expect_eq(n2, 5);
        expect_eq(unpacked[0], 0.0 as f32);
        expect_eq(unpacked[1], 1.0 as f32);
        expect_eq(unpacked[2], -2.0 as f32);
        expect_eq(unpacked[3], 0.5 as f32);
        expect_eq(unpacked[4], 16.25 as f32);

        // Truncated dst slice safety
        let short_dst: [2]u16 = undefined;
        expect_eq(f32SliceToF16(&src, &short_dst), 2);
    });
}
