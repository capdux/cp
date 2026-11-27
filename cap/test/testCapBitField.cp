import { test, expect_eq } from "cap:test";
import {
    bitLengthU8, bitLengthU16, bitLengthU32, bitLengthU64, bitLengthUsize, bitLengthSlice,
    getBitU8, setBitU8, getBitsU8, setBitsU8,
    getBitU16, setBitU16, getBitsU16, setBitsU16,
    getBitU32, setBitU32, getBitsU32, setBitsU32,
    getBitU64, setBitU64, getBitsU64, setBitsU64,
    getBitsRangeU64, setBitsRangeU64,
    getBitUsize, setBitUsize, getBitsUsize, setBitsUsize,
    getBitSlice, setBitSlice, toggleBitSlice, getBitsSlice, setBitsSlice, countOnesSlice,
    BitArray,
} from "../../packages/bit_field/src/index.cp";

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

    test("7. @cap/bit-field — u8, u16, usize BitField & bitLength helpers", () => {
        expect_eq(bitLengthU8(), 8 as usize);
        expect_eq(bitLengthU16(), 16 as usize);
        expect_eq(bitLengthU32(), 32 as usize);
        expect_eq(bitLengthU64(), 64 as usize);
        expect_eq(bitLengthUsize() >= 32, true);

        let b8: u8 = setBitU8(0, 7, true);
        b8 = setBitsU8(b8, 1, 5, 0x0A);
        expect_eq(getBitU8(b8, 7), true);
        expect_eq(getBitsU8(b8, 1, 5), 0x0A as u8);

        let b16: u16 = setBitsU16(0, 4, 12, 0xAB);
        b16 = setBitU16(b16, 15, true);
        expect_eq(getBitU16(b16, 15), true);
        expect_eq(getBitsU16(b16, 4, 12), 0xAB as u16);

        let bu: usize = setBitUsize(0, 10, true);
        bu = setBitsUsize(bu, 16, 24, 0x5C);
        expect_eq(getBitUsize(bu, 10), true);
        expect_eq(getBitsUsize(bu, 16, 24), 0x5C as usize);

        // Full [0..64) range via getBitsRangeU64 / setBitsRangeU64
        const full = setBitsRangeU64(0, 56, 64, 0xAB);
        expect_eq(getBitsRangeU64(full, 56, 64), 0xAB as u64);
    });

    test("8. @cap/bit-field — BitArray & cross-byte getBitsSlice/setBitsSlice/toggle/countOnes", () => {
        let raw: [4]u8 = [0, 0, 0, 0];
        let ba = BitArray.fromSlice(raw[0..4]);
        expect_eq(ba.bitLength(), 32 as usize);
        expect_eq(bitLengthSlice(raw[0..4]), 32 as usize);

        ba.setBits(5, 17, 0xABC);
        expect_eq(ba.getBits(5, 17), 0xABC as u64);
        expect_eq(getBitsSlice(raw[0..4], 5, 17), 0xABC as u64);

        ba.toggleBit(0);
        expect_eq(ba.getBit(0), true);
        toggleBitSlice(raw[0..4], 0);
        expect_eq(ba.getBit(0), false);

        ba.setAll();
        expect_eq(ba.countOnes(), 32 as usize);
        expect_eq(countOnesSlice(raw[0..4]), 32 as usize);
        ba.clearAll();
        expect_eq(ba.countOnes(), 0 as usize);
    });
}
