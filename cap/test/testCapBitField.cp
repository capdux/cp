import { test, expect_eq } from "cap:test";
import {
    getBitU64,
    setBitU64,
    getBitsU64,
    setBitsU64,
    getBitU32,
    setBitU32,
    getBitsU32,
    setBitsU32,
    getBitSlice,
    setBitSlice,
} from "../../packages/@cap/bit-field/src/index.cp";

fn main() {
    test("1. @cap/bit-field — getBitU64 & setBitU64 single bit operations", () => {
        let v: u64 = 0;
        v = setBitU64(v, 0, true);
        v = setBitU64(v, 31, true);
        v = setBitU64(v, 63, true);

        expect_eq(getBitU64(v, 0), true);
        expect_eq(getBitU64(v, 1), false);
        expect_eq(getBitU64(v, 31), true);
        expect_eq(getBitU64(v, 62), false);
        expect_eq(getBitU64(v, 63), true);

        v = setBitU64(v, 31, false);
        expect_eq(getBitU64(v, 31), false);
        expect_eq(getBitU64(v, 0), true);
        expect_eq(getBitU64(v, 63), true);
    });

    test("2. @cap/bit-field — getBitsU64 & setBitsU64 bit-range extraction and insertion", () => {
        const hi: u64 = 0x12345678 as u64;
        const lo: u64 = 0x9ABCDEF0 as u64;
        const val: u64 = (hi << 32) | lo;

        expect_eq(getBitsU64(val, 0, 8), 0xF0 as u64);
        expect_eq(getBitsU64(val, 8, 16), 0xDE as u64);
        expect_eq(getBitsU64(val, 32, 48), 0x5678 as u64);
        expect_eq(getBitsU64(val, 4, 12), 0xEF as u64);

        // Overwrite bits [8..16) with 0xAB (extra high bits in 0xFFAB masked off)
        const updated = setBitsU64(val, 8, 16, 0xFFAB as u64);
        expect_eq(getBitsU64(updated, 8, 16), 0xAB as u64);
        expect_eq(getBitsU64(updated, 0, 8), 0xF0 as u64);
        expect_eq(getBitsU64(updated, 16, 24), 0xBC as u64);
    });

    test("3. @cap/bit-field — getBitU32 & setBitU32 single bit operations", () => {
        let v: u32 = 0;
        v = setBitU32(v, 0, true);
        v = setBitU32(v, 15, true);
        v = setBitU32(v, 31, true);

        expect_eq(getBitU32(v, 0), true);
        expect_eq(getBitU32(v, 14), false);
        expect_eq(getBitU32(v, 15), true);
        expect_eq(getBitU32(v, 31), true);

        v = setBitU32(v, 15, false);
        expect_eq(getBitU32(v, 15), false);
        expect_eq(v, (0x80000001 as u32));
    });

    test("4. @cap/bit-field — getBitsU32 & setBitsU32 bit-range operations", () => {
        const val: u32 = 0xCAFEBABE as u32;
        expect_eq(getBitsU32(val, 0, 8), 0xBE as u32);
        expect_eq(getBitsU32(val, 8, 16), 0xBA as u32);
        expect_eq(getBitsU32(val, 16, 28), 0xAFE as u32);

        const updated = setBitsU32(val, 8, 20, 0x123 as u32);
        expect_eq(getBitsU32(updated, 8, 20), 0x123 as u32);
        expect_eq(getBitsU32(updated, 0, 8), 0xBE as u32);
        expect_eq(getBitsU32(updated, 20, 31), (val >> 20) & 0x7FF);
    });

    test("5. @cap/bit-field — getBitSlice & setBitSlice across multi-byte buffer", () => {
        let buf: [4]u8 = [0, 0, 0, 0];
        setBitSlice(buf[0..4], 0, true);
        setBitSlice(buf[0..4], 7, true);
        setBitSlice(buf[0..4], 9, true);
        setBitSlice(buf[0..4], 31, true);

        expect_eq(buf[0], 0x81 as u8);
        expect_eq(buf[1], 0x02 as u8);
        expect_eq(buf[2], 0x00 as u8);
        expect_eq(buf[3], 0x80 as u8);

        expect_eq(getBitSlice(buf[0..4], 0), true);
        expect_eq(getBitSlice(buf[0..4], 1), false);
        expect_eq(getBitSlice(buf[0..4], 7), true);
        expect_eq(getBitSlice(buf[0..4], 9), true);
        expect_eq(getBitSlice(buf[0..4], 31), true);

        setBitSlice(buf[0..4], 7, false);
        expect_eq(getBitSlice(buf[0..4], 7), false);
        expect_eq(buf[0], 0x01 as u8);
    });

    test("6. @cap/bit-field — Edge cases: empty/inverted ranges & out-of-bounds slice indices", () => {
        expect_eq(getBitsU64(0xFFFF as u64, 8, 8), 0 as u64);
        expect_eq(getBitsU64(0xFFFF as u64, 12, 4), 0 as u64);
        expect_eq(setBitsU64(0x1234 as u64, 8, 8, 0xFF as u64), 0x1234 as u64);
        expect_eq(setBitsU64(0x1234 as u64, 10, 5, 0xFF as u64), 0x1234 as u64);

        expect_eq(getBitsU32(0xFFFF as u32, 5, 5), 0 as u32);
        expect_eq(getBitsU32(0xFFFF as u32, 9, 3), 0 as u32);
        expect_eq(setBitsU32(0x1234 as u32, 5, 5, 0xFF as u32), 0x1234 as u32);

        let small: [2]u8 = [0xFF, 0xFF];
        expect_eq(getBitSlice(small[0..2], 16), false);
        expect_eq(getBitSlice(small[0..2], 100), false);
        setBitSlice(small[0..2], 16, false);
        expect_eq(small[0], 0xFF as u8);
        expect_eq(small[1], 0xFF as u8);
    });
}
