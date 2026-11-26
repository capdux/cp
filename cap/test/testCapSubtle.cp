// cap/test/testCapSubtle.cp
// Comprehensive unit tests for @cap/subtle (Rust subtle + zeroize parity)

import { test, expect_eq } from "cap:test";
import {
    Choice,
    ctEqU8,
    ctEqU32,
    ctEqU64,
    ctLtU64,
    ctGtU64,
    ctSelectU8,
    ctSelectU32,
    ctSelectU64,
    ctSwapU64,
    ctEqBytes,
    ctEqFixed32,
    ctCopyIf,
    zeroizeBytes,
    isAllZero,
} from "../../packages/@cap/subtle/src/index.cp";

fn main() {
    test("1. @cap/subtle — Choice boolean algebra & normalization", () => {
        const t = Choice.fromU8(42 as u8);
        const f = Choice.fromU8(0 as u8);
        expect_eq(t.val, 1 as u8);
        expect_eq(f.val, 0 as u8);
        expect_eq(t.toBool(), true);
        expect_eq(f.toBool(), false);
        expect_eq(Choice.fromBool(true).val, 1 as u8);
        expect_eq(Choice.fromBool(false).val, 0 as u8);

        expect_eq(t.ctNot().toBool(), false);
        expect_eq(f.ctNot().toBool(), true);
        expect_eq(t.ctAnd(f).toBool(), false);
        expect_eq(t.ctAnd(t).toBool(), true);
        expect_eq(t.ctOr(f).toBool(), true);
        expect_eq(f.ctOr(f).toBool(), false);
        expect_eq(t.ctXor(t).toBool(), false);
        expect_eq(t.ctXor(f).toBool(), true);
    });

    test("2. @cap/subtle — constant-time integer equality & 64-bit ordering", () => {
        expect_eq(ctEqU8(0xAB as u8, 0xAB as u8).toBool(), true);
        expect_eq(ctEqU8(0xAB as u8, 0xAC as u8).toBool(), false);

        expect_eq(ctEqU32(0xDEADBEEF as u32, 0xDEADBEEF as u32).toBool(), true);
        expect_eq(ctEqU32(0xDEADBEEF as u32, 0xDEADBEEE as u32).toBool(), false);

        const max64: u64 = 0xFFFFFFFFFFFFFFFF as u64;
        expect_eq(ctEqU64(max64, max64).toBool(), true);
        expect_eq(ctEqU64(max64, 0 as u64).toBool(), false);

        expect_eq(ctLtU64(0 as u64, 1 as u64).toBool(), true);
        expect_eq(ctLtU64(1 as u64, 1 as u64).toBool(), false);
        expect_eq(ctLtU64(0 as u64, max64).toBool(), true);
        expect_eq(ctLtU64(max64, 0 as u64).toBool(), false);
        expect_eq(ctLtU64(0x00000000FFFFFFFF as u64, 0x0000000100000000 as u64).toBool(), true);
        expect_eq(ctLtU64(0x0000000100000000 as u64, 0x00000000FFFFFFFF as u64).toBool(), false);

        expect_eq(ctGtU64(max64, 42 as u64).toBool(), true);
        expect_eq(ctGtU64(42 as u64, 42 as u64).toBool(), false);
    });

    test("3. @cap/subtle — branchless ctSelectU8/U32/U64 & ctSwapU64", () => {
        const t = Choice.fromBool(true);
        const f = Choice.fromBool(false);

        expect_eq(ctSelectU8(t, 0xAA as u8, 0x55 as u8), 0xAA as u8);
        expect_eq(ctSelectU8(f, 0xAA as u8, 0x55 as u8), 0x55 as u8);

        expect_eq(ctSelectU32(t, 100 as u32, 200 as u32), 100 as u32);
        expect_eq(ctSelectU32(f, 100 as u32, 200 as u32), 200 as u32);

        expect_eq(ctSelectU64(t, 1234567890123 as u64, 9876543210987 as u64), 1234567890123 as u64);
        expect_eq(ctSelectU64(f, 1234567890123 as u64, 9876543210987 as u64), 9876543210987 as u64);

        let x: u64 = 111;
        let y: u64 = 222;
        ctSwapU64(f, &x, &y);
        expect_eq(x, 111 as u64);
        expect_eq(y, 222 as u64);

        ctSwapU64(t, &x, &y);
        expect_eq(x, 222 as u64);
        expect_eq(y, 111 as u64);
    });

    test("4. @cap/subtle — constant-time ctEqBytes", () => {
        expect_eq(ctEqBytes("hmac-token-12345", "hmac-token-12345").toBool(), true);
        expect_eq(ctEqBytes("hmac-token-12345", "hmac-token-12346").toBool(), false);
        expect_eq(ctEqBytes("short", "longer").toBool(), false);
        expect_eq(ctEqBytes("", "").toBool(), true);
    });

    test("5. @cap/subtle — constant-time 32-byte digest ctEqFixed32", () => {
        let d1: [32]u8 = undefined;
        let d2: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            d1[i] = (i * 7) as u8;
            d2[i] = (i * 7) as u8;
            i += 1;
        }
        expect_eq(ctEqFixed32(d1, d2).toBool(), true);

        // Flip 1 bit in the very last byte
        d2[31] = d2[31] ^ (1 as u8);
        expect_eq(ctEqFixed32(d1, d2).toBool(), false);
    });

    test("6. @cap/subtle — ctCopyIf, zeroizeBytes & isAllZero", () => {
        let dst: [6]u8 = ['s', 'e', 'c', 'r', 'e', 't'];
        ctCopyIf(Choice.fromBool(false), dst[0..6], "PUBLIC");
        expect_eq(dst[0..6], "secret");

        ctCopyIf(Choice.fromBool(true), dst[0..6], "PUBLIC");
        expect_eq(dst[0..6], "PUBLIC");
        expect_eq(isAllZero(dst[0..6]).toBool(), false);

        zeroizeBytes(dst[0..6]);
        expect_eq(isAllZero(dst[0..6]).toBool(), true);
        expect_eq(dst[0], 0 as u8);
        expect_eq(dst[5], 0 as u8);
    });
}
