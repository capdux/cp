// cap/test/testCapSha1Crate.cp
// Comprehensive unit tests for @cap/sha1-crate (FIPS 180-4 SHA-1)

import { test, expect_eq } from "cap:test";
import {
    SHA1_DIGEST_LEN,
    SHA1_BLOCK_LEN,
    Sha1Hasher,
    sha1Bytes,
    sha1Hex,
} from "../../packages/sha1/src/index.cp";

fn testHexMatches(actual: []u8, expected: string): bool {
    if (actual.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < expected.len) {
        if (actual[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/sha1-crate — FIPS 180-4 standard test vectors", () => {
        expect_eq(SHA1_DIGEST_LEN, 20 as usize);
        expect_eq(SHA1_BLOCK_LEN, 64 as usize);

        let hex: [40]u8 = undefined;

        let n0 = sha1Hex("", hex[0..40]);
        expect_eq(n0, 40 as usize);
        expect_eq(testHexMatches(hex[0..40], "da39a3ee5e6b4b0d3255bfef95601890afd80709"), true);

        let n1 = sha1Hex("abc", hex[0..40]);
        expect_eq(n1, 40 as usize);
        expect_eq(testHexMatches(hex[0..40], "a9993e364706816aba3e25717850c26c9cd0d89d"), true);

        let n2 = sha1Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", hex[0..40]);
        expect_eq(n2, 40 as usize);
        expect_eq(testHexMatches(hex[0..40], "84983e441c3bd26ebaae4aa1f95129e5e54670f1"), true);
    });

    test("2. @cap/sha1-crate — Streaming chunked updateStr matches one-shot", () => {
        let h = Sha1Hasher.new();
        h.updateStr("abcdbcde");
        h.updateStr("cdefdefgefghfghighij");
        h.updateStr("hijkijkljklm");
        h.updateStr("klmnlmnomnopnopq");

        let hex: [40]u8 = undefined;
        let n = h.finalizeHex(hex[0..40]);
        expect_eq(n, 40 as usize);
        expect_eq(testHexMatches(hex[0..40], "84983e441c3bd26ebaae4aa1f95129e5e54670f1"), true);

        // Single-byte streaming for "abc"
        let h2 = Sha1Hasher.new();
        let b_a: [1]u8 = ['a' as u8];
        let b_b: [1]u8 = ['b' as u8];
        let b_c: [1]u8 = ['c' as u8];
        h2.update(b_a[0..1]);
        h2.update(b_b[0..1]);
        h2.update(b_c[0..1]);
        * = h2.finalizeHex(hex[0..40]);
        expect_eq(testHexMatches(hex[0..40], "a9993e364706816aba3e25717850c26c9cd0d89d"), true);
    });

    test("3. @cap/sha1-crate — Padding boundary lengths (55, 56, 64, 112 bytes)", () => {
        // FIPS 180-4 112-byte multi-block vector
        let msg112 = "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu";
        let hex: [40]u8 = undefined;
        let n = sha1Hex(msg112, hex[0..40]);
        expect_eq(n, 40 as usize);
        expect_eq(testHexMatches(hex[0..40], "a49b2446a02c645bf419f995b67091253a04a259"), true);

        // Verify 55, 56, 64-byte streaming vs one-shot consistency
        let data: [64]u8 = undefined;
        let i: usize = 0;
        while (i < (64 as usize)) {
            data[i] = ('a' as u8) + ((i % (26 as usize)) as u8);
            i += 1;
        }
        let d_one: [20]u8 = undefined;
        let d_str: [20]u8 = undefined;

        sha1Bytes(data[0..55], d_one[0..20]);
        let h55 = Sha1Hasher.new();
        h55.update(data[0..20]);
        h55.update(data[20..55]);
        h55.finalize(d_str[0..20]);
        expect_eq(d_one[0], d_str[0]);
        expect_eq(d_one[19], d_str[19]);

        sha1Bytes(data[0..56], d_one[0..20]);
        let h56 = Sha1Hasher.new();
        h56.update(data[0..30]);
        h56.update(data[30..56]);
        h56.finalize(d_str[0..20]);
        expect_eq(d_one[0], d_str[0]);
        expect_eq(d_one[19], d_str[19]);

        sha1Bytes(data[0..64], d_one[0..20]);
        let h64 = Sha1Hasher.new();
        h64.update(data[0..64]);
        h64.finalize(d_str[0..20]);
        expect_eq(d_one[0], d_str[0]);
        expect_eq(d_one[19], d_str[19]);
    });

    test("4. @cap/sha1-crate — Reset allows clean hasher reuse", () => {
        let h = Sha1Hasher.new();
        let hex: [40]u8 = undefined;

        h.updateStr("abc");
        * = h.finalizeHex(hex[0..40]);
        expect_eq(testHexMatches(hex[0..40], "a9993e364706816aba3e25717850c26c9cd0d89d"), true);

        h.reset();
        * = h.finalizeHex(hex[0..40]);
        expect_eq(testHexMatches(hex[0..40], "da39a3ee5e6b4b0d3255bfef95601890afd80709"), true);

        h.reset();
        h.updateStr("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
        * = h.finalizeHex(hex[0..40]);
        expect_eq(testHexMatches(hex[0..40], "84983e441c3bd26ebaae4aa1f95129e5e54670f1"), true);
    });

    test("5. @cap/sha1-crate — Raw 20-byte digest & binary input verification", () => {
        let raw_abc: [3]u8 = ['a' as u8, 'b' as u8, 'c' as u8];
        let out: [20]u8 = undefined;
        sha1Bytes(raw_abc[0..3], out[0..20]);

        // a9993e36 4706816a ba3e2571 7850c26c 9cd0d89d
        expect_eq(out[0], 0xA9 as u8);
        expect_eq(out[1], 0x99 as u8);
        expect_eq(out[2], 0x3E as u8);
        expect_eq(out[3], 0x36 as u8);
        expect_eq(out[16], 0x9C as u8);
        expect_eq(out[17], 0xD0 as u8);
        expect_eq(out[18], 0xD8 as u8);
        expect_eq(out[19], 0x9D as u8);

        // Short output buffer returns 0 safely
        let small: [10]u8 = undefined;
        expect_eq(sha1Hex("abc", small[0..10]), 0 as usize);
    });

    test("6. @cap/sha1-crate — FIPS 180-4 1,000,000 x 'a' long message vector", () => {
        let chunk: [1000]u8 = undefined;
        let i: usize = 0;
        while (i < (1000 as usize)) {
            chunk[i] = 'a' as u8;
            i += 1;
        }
        let h = Sha1Hasher.new();
        let rep: usize = 0;
        while (rep < (1000 as usize)) {
            h.update(chunk[0..1000]);
            rep += 1;
        }
        let hex: [40]u8 = undefined;
        let n = h.finalizeHex(hex[0..40]);
        expect_eq(n, 40 as usize);
        expect_eq(testHexMatches(hex[0..40], "34aa973cd4c4daa4f61eeb2bdbad27316534016f"), true);
    });
}
