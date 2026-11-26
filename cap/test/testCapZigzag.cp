import { test, expect_eq } from "cap:test";
import {
    encodeI32,
    decodeI32,
    encodeI64,
    decodeI64,
    encodeZigzagVarint,
    decodeZigzagVarint,
} from "../../packages/@cap/zigzag/src/index.cp";

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
}
