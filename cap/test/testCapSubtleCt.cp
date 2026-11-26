import { test, expect_eq } from "cap:test";
import {
    Choice,
    ctIsZeroU64,
    ctEqU8,
    ctEqU32,
    ctEqU64,
    ctEqBytes,
    ctLtU64,
    ctGtU64,
    ctSelectU8,
    ctSelectU32,
    ctSelectU64,
    ctSelectBytes,
    ctCondSwapU64,
    zeroizeBytes,
} from "../../packages/@cap/subtle-ct/src/index.cp";

fn main() {
    test("1. @cap/subtle-ct — Choice boolean algebra (fromU8, not, and, or, xor)", () => {
        let c0: Choice = Choice.fromU8(0);
        let c1: Choice = Choice.fromU8(1);
        expect_eq(c0.toBool(), false);
        expect_eq(c1.toBool(), true);
        expect_eq(c0.not().toBool(), true);
        expect_eq(c1.not().toBool(), false);
        expect_eq(c1.and(c1).toBool(), true);
        expect_eq(c1.and(c0).toBool(), false);
        expect_eq(c0.or(c1).toBool(), true);
        expect_eq(c0.or(c0).toBool(), false);
        expect_eq(c1.xor(c1).toBool(), false);
        expect_eq(c1.xor(c0).toBool(), true);
    });

    test("2. @cap/subtle-ct — ctEqU8, ctEqU32, ctEqU64 & ctIsZeroU64 boundaries", () => {
        let max64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        expect_eq(ctIsZeroU64(0).toBool(), true);
        expect_eq(ctIsZeroU64(1).toBool(), false);
        expect_eq(ctIsZeroU64(max64).toBool(), false);

        expect_eq(ctEqU8(0, 0).toBool(), true);
        expect_eq(ctEqU8(255, 255).toBool(), true);
        expect_eq(ctEqU8(255, 254).toBool(), false);

        expect_eq(ctEqU32(0xDEADBEEF, 0xDEADBEEF).toBool(), true);
        expect_eq(ctEqU32(0xDEADBEEF, 0xDEADBEF0).toBool(), false);

        expect_eq(ctEqU64(max64, max64).toBool(), true);
        expect_eq(ctEqU64(max64, max64 - 1).toBool(), false);
        expect_eq(ctEqU64(0, 1).toBool(), false);
    });

    test("3. @cap/subtle-ct — ctEqBytes equal, single-bit head/tail diff & mismatched lengths", () => {
        let a: [4]u8 = [0x11, 0x22, 0x33, 0x44];
        let b: [4]u8 = [0x11, 0x22, 0x33, 0x44];
        let diff_head: [4]u8 = [0x10, 0x22, 0x33, 0x44];
        let diff_tail: [4]u8 = [0x11, 0x22, 0x33, 0x45];
        let shorter: [3]u8 = [0x11, 0x22, 0x33];

        expect_eq(ctEqBytes(a[0..4], b[0..4]).toBool(), true);
        expect_eq(ctEqBytes(a[0..4], diff_head[0..4]).toBool(), false);
        expect_eq(ctEqBytes(a[0..4], diff_tail[0..4]).toBool(), false);
        expect_eq(ctEqBytes(a[0..4], shorter[0..3]).toBool(), false);
        expect_eq(ctEqBytes(a[0..0], b[0..0]).toBool(), true);
    });

    test("4. @cap/subtle-ct — branchless ctLtU64 & ctGtU64 across 0..u64::MAX", () => {
        let max64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        let bit32: u64 = (1 as u64) << 32;

        expect_eq(ctLtU64(0, 1).toBool(), true);
        expect_eq(ctLtU64(1, 0).toBool(), false);
        expect_eq(ctLtU64(5, 5).toBool(), false);
        expect_eq(ctLtU64(0, max64).toBool(), true);
        expect_eq(ctLtU64(max64, 0).toBool(), false);
        expect_eq(ctLtU64(max64, max64).toBool(), false);
        expect_eq(ctLtU64(bit32 - 1, bit32).toBool(), true);
        expect_eq(ctLtU64(bit32, bit32 - 1).toBool(), false);

        expect_eq(ctGtU64(max64, 0).toBool(), true);
        expect_eq(ctGtU64(0, max64).toBool(), false);
        expect_eq(ctGtU64(42, 42).toBool(), false);
    });

    test("5. @cap/subtle-ct — ctSelectU8/U32/U64/Bytes & ctCondSwapU64", () => {
        let c0: Choice = Choice.fromU8(0);
        let c1: Choice = Choice.fromU8(1);

        expect_eq(ctSelectU8(10, 20, c0), 10 as u8);
        expect_eq(ctSelectU8(10, 20, c1), 20 as u8);
        expect_eq(ctSelectU32(1000, 2000, c0), 1000 as u32);
        expect_eq(ctSelectU32(1000, 2000, c1), 2000 as u32);
        expect_eq(ctSelectU64(111, 222, c0), 111 as u64);
        expect_eq(ctSelectU64(111, 222, c1), 222 as u64);

        let a_buf: [4]u8 = [1, 2, 3, 4];
        let b_buf: [4]u8 = [9, 8, 7, 6];
        let dst: [4]u8 = [0; 4];
        expect_eq(ctSelectBytes(dst[0..4], a_buf[0..4], b_buf[0..4], c0), 4 as usize);
        expect_eq(ctEqBytes(dst[0..4], a_buf[0..4]).toBool(), true);
        expect_eq(ctSelectBytes(dst[0..4], a_buf[0..4], b_buf[0..4], c1), 4 as usize);
        expect_eq(ctEqBytes(dst[0..4], b_buf[0..4]).toBool(), true);

        let x: u64 = 12345;
        let y: u64 = 67890;
        ctCondSwapU64(&x, &y, c0);
        expect_eq(x, 12345 as u64);
        expect_eq(y, 67890 as u64);
        ctCondSwapU64(&x, &y, c1);
        expect_eq(x, 67890 as u64);
        expect_eq(y, 12345 as u64);
    });

    test("6. @cap/subtle-ct — zeroizeBytes clears sensitive buffer completely", () => {
        let key: [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44];
        let zeros: [8]u8 = [0; 8];
        expect_eq(ctEqBytes(key[0..8], zeros[0..8]).toBool(), false);
        zeroizeBytes(key[0..8]);
        expect_eq(ctEqBytes(key[0..8], zeros[0..8]).toBool(), true);
    });
}
