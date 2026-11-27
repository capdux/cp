import { test, expect_eq } from "cap:test";
import {
    encodeI8, decodeU8, decodeI8,
    encodeI16, decodeU16, decodeI16,
    encodeI32, decodeI32, decodeU32,
    encodeI64, decodeI64, decodeU64,
    encodeI8Slice, decodeU8Slice,
    encodeI16Slice, decodeU16Slice,
    encodeI32Slice, decodeU32Slice,
    encodeI64Slice, decodeU64Slice,
    varintLen,
    encodeZigzagVarint,
    decodeZigzagVarint,
    encodeZigzagVarintSlice,
    decodeZigzagVarintSlice,
} from "../../packages/zigzag/src/index.cp";

fn main() {
    test("1. @cap/zigzag — Official Protobuf ZigZag 32-bit mapping table", () => {
        expect_eq(encodeI32(0), 0 as u32);
        expect_eq(encodeI32(-1), 1 as u32);
        expect_eq(encodeI32(1), 2 as u32);
        expect_eq(encodeI32(-2), 3 as u32);
        expect_eq(encodeI32(2), 4 as u32);
        expect_eq(encodeI32(2147483647), 4294967294 as u32);

        const i32_min: i32 = @bitCast(0x80000000 as u32);
        expect_eq(encodeI32(i32_min), 4294967295 as u32);
    });

    test("2. @cap/zigzag — 32-bit round-trip across negatives, zero & extremes", () => {
        const i32_min: i32 = @bitCast(0x80000000 as u32);
        const samples: [7]i32 = [i32_min, -1000000, -1, 0, 1, 1000000, 2147483647];
        let i: usize = 0;
        while (i < 7) {
            expect_eq(decodeI32(encodeI32(samples[i])), samples[i]);
            i += 1;
        }
    });

    test("3. @cap/zigzag — 64-bit round-trip including i64::MIN & i64::MAX", () => {
        const u64_max: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        const i64_min: i64 = @bitCast((1 as u64) << 63);
        const i64_max: i64 = @bitCast(u64_max >> 1);

        expect_eq(encodeI64(i64_max), u64_max - 1);
        expect_eq(encodeI64(i64_min), u64_max);
        expect_eq(decodeI64(encodeI64(i64_min)), i64_min);
        expect_eq(decodeI64(encodeI64(i64_max)), i64_max);
        expect_eq(decodeI64(encodeI64(-9876543210123)), -9876543210123 as i64);
        expect_eq(decodeI64(encodeI64(9876543210123)), 9876543210123 as i64);
    });

    test("4. @cap/zigzag — encodeZigzagVarint & decodeZigzagVarint 1..10 byte roundtrip", () => {
        const u64_max: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        const i64_min: i64 = @bitCast((1 as u64) << 63);
        const i64_max: i64 = @bitCast(u64_max >> 1);
        const vals: [7]i64 = [0, -1, 63, -64, 123456, i64_max, i64_min];

        let stream: [128]u8 = undefined;
        let w: usize = 0;
        let i: usize = 0;
        while (i < 7) {
            const n = encodeZigzagVarint(vals[i], stream[w..128]);
            expect_eq(n >= 1 && n <= 10, true);
            w += n;
            i += 1;
        }

        // Decode entire stream sequentially
        let pos: usize = 0;
        let j: usize = 0;
        while (j < 7) {
            const r = decodeZigzagVarint(stream[0..w], pos);
            expect_eq(r.valid, true);
            expect_eq(r.value, vals[j]);
            pos += r.consumed;
            j += 1;
        }
        expect_eq(pos, w);
    });

    test("5. @cap/zigzag — Truncated varint & 10-byte overflow rejection", () => {
        // Truncated (continuation bit set on final byte)
        const trunc: [2]u8 = [0x80, 0x81];
        expect_eq(decodeZigzagVarint(trunc[0..2], 0).valid, false);
        expect_eq(decodeZigzagVarint(trunc[0..0], 0).valid, false);

        // 10th byte with value 0x02 overflows 64-bit unsigned integer
        const over10: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02];
        expect_eq(decodeZigzagVarint(over10[0..10], 0).valid, false);

        // 11-byte unending continuation stream overflows
        const over11: [11]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01];
        expect_eq(decodeZigzagVarint(over11[0..11], 0).valid, false);
    });

    test("6. @cap/zigzag — Undersized encode buffer protection", () => {
        let tiny: [2]u8 = undefined;
        // 100000 zigzag = 200000 needs 3 bytes
        expect_eq(encodeZigzagVarint(100000, tiny[0..2]), 0 as usize);
        expect_eq(encodeZigzagVarint(0, tiny[0..0]), 0 as usize);
        expect_eq(encodeZigzagVarint(-1, tiny[0..2]), 1 as usize);
    });

    test("7. @cap/zigzag — encodeI8/I16 & decodeU8/U16/U32/U64 extremes", () => {
        expect_eq(encodeI8(0), 0 as u8);
        expect_eq(encodeI8(-1), 1 as u8);
        expect_eq(encodeI8(1), 2 as u8);
        expect_eq(encodeI8(127), 254 as u8);
        const i8_min: i8 = @bitCast(0x80 as u8);
        expect_eq(encodeI8(i8_min), 255 as u8);
        expect_eq(decodeU8(255 as u8), i8_min);
        expect_eq(decodeI8(254 as u8), 127 as i8);

        expect_eq(encodeI16(0), 0 as u16);
        expect_eq(encodeI16(-1), 1 as u16);
        expect_eq(encodeI16(32767), 65534 as u16);
        const i16_min: i16 = @bitCast(0x8000 as u16);
        expect_eq(encodeI16(i16_min), 65535 as u16);
        expect_eq(decodeU16(65535 as u16), i16_min);
        expect_eq(decodeI16(65534 as u16), 32767 as i16);

        expect_eq(decodeU32(encodeI32(-42)), -42);
        expect_eq(decodeU64(encodeI64(-999999999)), -999999999 as i64);
    });

    test("8. @cap/zigzag — batch slice codecs & varintLen", () => {
        const s8: [4]i8 = [-128, -1, 0, 127];
        let u8buf: [4]u8 = [0, 0, 0, 0];
        let o8: [4]i8 = [0, 0, 0, 0];
        expect_eq(encodeI8Slice(s8[0..4], u8buf[0..4]), 4);
        expect_eq(decodeU8Slice(u8buf[0..4], o8[0..4]), 4);
        expect_eq(o8[0], -128);
        expect_eq(o8[3], 127);

        const s16: [3]i16 = [-32768, 0, 32767];
        let u16buf: [3]u16 = [0, 0, 0];
        let o16: [3]i16 = [0, 0, 0];
        expect_eq(encodeI16Slice(s16[0..3], u16buf[0..3]), 3);
        expect_eq(decodeU16Slice(u16buf[0..3], o16[0..3]), 3);
        expect_eq(o16[0], -32768);

        const s32: [3]i32 = [-1000, 0, 1000];
        let u32buf: [3]u32 = [0, 0, 0];
        let o32: [3]i32 = [0, 0, 0];
        expect_eq(encodeI32Slice(s32[0..3], u32buf[0..3]), 3);
        expect_eq(decodeU32Slice(u32buf[0..3], o32[0..3]), 3);
        expect_eq(o32[0], -1000);
        expect_eq(o32[2], 1000);

        const s64: [4]i64 = [-100000, -1, 0, 100000];
        let u64buf: [4]u64 = [0, 0, 0, 0];
        let o64: [4]i64 = [0, 0, 0, 0];
        expect_eq(encodeI64Slice(s64[0..4], u64buf[0..4]), 4);
        expect_eq(decodeU64Slice(u64buf[0..4], o64[0..4]), 4);
        expect_eq(o64[0], -100000);

        expect_eq(varintLen(0), 1);
        expect_eq(varintLen(-64), 1);
        expect_eq(varintLen(64), 2);
        let vbuf: [64]u8 = undefined;
        let vout: [4]i64 = [0, 0, 0, 0];
        const written = encodeZigzagVarintSlice(s64[0..4], vbuf[0..64]);
        expect_eq(written > 0, true);
        expect_eq(decodeZigzagVarintSlice(vbuf[0..written], vout[0..4]), 4);
        expect_eq(vout[0], -100000);
        expect_eq(vout[3], 100000);
    });
}
