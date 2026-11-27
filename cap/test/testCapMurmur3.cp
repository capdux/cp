// cap/test/testCapMurmur3.cp
// Comprehensive unit tests for @cap/murmur3

import { test, expect_eq } from "cap:test";
import {
    Hash128,
    murmur3_32,
    murmur3_128,
} from "../../packages/murmur3/src/index.cp";

fn main() {
    test("1. @cap/murmur3 — official MurmurHash3 32-bit reference vectors", () => {
        expect_eq(murmur3_32("", 0), 0x00000000 as u32);
        expect_eq(murmur3_32("", 1), 0x514e28b7 as u32);
        expect_eq(murmur3_32("hello", 0), 0x248bfa47 as u32);
        expect_eq(murmur3_32("hello, world", 0), 0x149bbb7f as u32);
        expect_eq(murmur3_32("The quick brown fox jumps over the lazy dog", 0), 0x2e4ff723 as u32);
    });

    test("2. @cap/murmur3 — 32-bit seed sensitivity & tail lengths 1..3", () => {
        expect_eq(murmur3_32("a", 0) != murmur3_32("a", 42), true);
        expect_eq(murmur3_32("ab", 0) != murmur3_32("abc", 0), true);
        expect_eq(murmur3_32("abcd", 0) != murmur3_32("abcde", 0), true);
    });

    test("3. @cap/murmur3 — 128-bit empty & seeded vectors", () => {
        const z: Hash128 = murmur3_128("", 0);
        expect_eq(z.lo, 0 as u64);
        expect_eq(z.hi, 0 as u64);

        const s1: Hash128 = murmur3_128("", 1);
        expect_eq(s1.lo != (0 as u64), true);
        expect_eq(s1.hi != (0 as u64), true);
    });

    test("4. @cap/murmur3 — 128-bit deterministic & multi-block (>16 bytes)", () => {
        const msg: string = "The quick brown fox jumps over the lazy dog";
        const a: Hash128 = murmur3_128(msg, 0);
        const b: Hash128 = murmur3_128(msg, 0);
        const c: Hash128 = murmur3_128(msg, 123);
        expect_eq(a.lo, b.lo);
        expect_eq(a.hi, b.hi);
        expect_eq(a.lo != c.lo, true);
    });

    test("5. @cap/murmur3 — 128-bit avalanche on 1-bit input flip", () => {
        const h1: Hash128 = murmur3_128("capdux-partition-0", 0);
        const h2: Hash128 = murmur3_128("capdux-partition-1", 0);
        expect_eq(h1.lo != h2.lo, true);
        expect_eq(h1.hi != h2.hi, true);
    });

    test("6. @cap/murmur3 — 128-bit tail length 9..15 coverage", () => {
        const h9: Hash128 = murmur3_128("123456789", 0);
        const h15: Hash128 = murmur3_128("123456789012345", 0);
        const h16: Hash128 = murmur3_128("1234567890123456", 0);
        expect_eq(h9.lo != h15.lo, true);
        expect_eq(h15.lo != h16.lo, true);
    });
}
