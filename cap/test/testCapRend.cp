import { test, expect_eq } from "cap:test";
import {
    U16Be,
    U16Le,
    U32Be,
    U32Le,
    I32Be,
    I32Le,
    U64Be,
    U64Le,
    I64Be,
    I64Le,
    readU16Be,
    readU16Le,
    readU32Be,
    readU32Le,
    readU64Be,
    readU64Le,
} from "../../packages/@cap/rend/src/index.cp";

fn main() {
    test("1. @cap/rend — U16Be & U16Le exact wire byte layout & round-trip", () => {
        const be: U16Be = U16Be.fromNative(0x1234);
        const be_b: [2]u8 = be.asBytes();
        expect_eq(be_b[0], 0x12 as u8);
        expect_eq(be_b[1], 0x34 as u8);
        expect_eq(be.toNative(), 0x1234 as u16);

        const le: U16Le = U16Le.fromNative(0x1234);
        const le_b: [2]u8 = le.asBytes();
        expect_eq(le_b[0], 0x34 as u8);
        expect_eq(le_b[1], 0x12 as u8);
        expect_eq(le.toNative(), 0x1234 as u16);

        const from_b: U16Be = U16Be.fromBytes(be_b);
        expect_eq(from_b.toNative(), 0x1234 as u16);
    });

    test("2. @cap/rend — U32Be & U32Le exact wire byte layout & round-trip", () => {
        const be: U32Be = U32Be.fromNative(0xDEADBEEF);
        const be_b: [4]u8 = be.asBytes();
        expect_eq(be_b[0], 0xDE as u8);
        expect_eq(be_b[1], 0xAD as u8);
        expect_eq(be_b[2], 0xBE as u8);
        expect_eq(be_b[3], 0xEF as u8);
        expect_eq(be.toNative(), 0xDEADBEEF as u32);

        const le: U32Le = U32Le.fromNative(0xDEADBEEF);
        const le_b: [4]u8 = le.asBytes();
        expect_eq(le_b[0], 0xEF as u8);
        expect_eq(le_b[1], 0xBE as u8);
        expect_eq(le_b[2], 0xAD as u8);
        expect_eq(le_b[3], 0xDE as u8);
        expect_eq(le.toNative(), 0xDEADBEEF as u32);
    });

    test("3. @cap/rend — I32Be & I32Le signed two's complement negative values", () => {
        const neg: i32 = -123456789;
        const be: I32Be = I32Be.fromNative(neg);
        const le: I32Le = I32Le.fromNative(neg);
        expect_eq(be.toNative(), neg);
        expect_eq(le.toNative(), neg);

        const minus_one_be: I32Be = I32Be.fromNative(-1);
        const b: [4]u8 = minus_one_be.asBytes();
        expect_eq(b[0], 0xFF as u8);
        expect_eq(b[3], 0xFF as u8);
        expect_eq(minus_one_be.toNative(), -1 as i32);
    });

    test("4. @cap/rend — U64Be & U64Le high-bit 64-bit round-trip", () => {
        const val: u64 = ((0x89ABCDEF as u64) << 32) | (0x01234567 as u64);
        const be: U64Be = U64Be.fromNative(val);
        const be_b: [8]u8 = be.asBytes();
        expect_eq(be_b[0], 0x89 as u8);
        expect_eq(be_b[3], 0xEF as u8);
        expect_eq(be_b[4], 0x01 as u8);
        expect_eq(be_b[7], 0x67 as u8);
        expect_eq(be.toNative(), val);

        const le: U64Le = U64Le.fromNative(val);
        const le_b: [8]u8 = le.asBytes();
        expect_eq(le_b[0], 0x67 as u8);
        expect_eq(le_b[7], 0x89 as u8);
        expect_eq(le.toNative(), val);
    });

    test("5. @cap/rend — I64Be & I64Le signed 64-bit negative values", () => {
        const neg: i64 = -9876543210123;
        const be: I64Be = I64Be.fromNative(neg);
        const le: I64Le = I64Le.fromNative(neg);
        expect_eq(be.toNative(), neg);
        expect_eq(le.toNative(), neg);

        const from_le: I64Le = I64Le.fromBytes(le.asBytes());
        expect_eq(from_le.toNative(), neg);
    });

    test("6. @cap/rend — bounds-checked slice readers (Be/Le & out-of-bounds null)", () => {
        const wire: [10]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A];

        expect_eq(readU16Be(wire[0..10], 0).?, 0x0102 as u16);
        expect_eq(readU16Le(wire[0..10], 0).?, 0x0201 as u16);
        expect_eq(readU32Be(wire[0..10], 1).?, 0x02030405 as u32);
        expect_eq(readU32Le(wire[0..10], 1).?, 0x05040302 as u32);

        const expected_u64_be: u64 = ((0x03040506 as u64) << 32) | (0x0708090A as u64);
        const expected_u64_le: u64 = ((0x0A090807 as u64) << 32) | (0x06050403 as u64);
        expect_eq(readU64Be(wire[0..10], 2).?, expected_u64_be);
        expect_eq(readU64Le(wire[0..10], 2).?, expected_u64_le);

        // Out-of-bounds checks
        expect_eq(readU16Be(wire[0..10], 9) == null, true);
        expect_eq(readU32Le(wire[0..10], 7) == null, true);
        expect_eq(readU64Be(wire[0..10], 3) == null, true);
        expect_eq(readU64Le(wire[0..10], 999) == null, true);
    });
}
