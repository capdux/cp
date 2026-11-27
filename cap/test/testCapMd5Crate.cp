// cap/test/testCapMd5Crate.cp
// Comprehensive unit tests for @cap/md5-crate (RFC 1321 MD5)

import { test, expect_eq } from "cap:test";
import {
    MD5_DIGEST_LEN,
    MD5_BLOCK_LEN,
    Md5Hasher,
    md5Bytes,
    md5Hex,
} from "../../packages/md-5/src/index.cp";

fn md5TestHexMatches(actual: []u8, expected: string): bool {
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
    test("1. @cap/md5-crate — RFC 1321 official test vectors", () => {
        expect_eq(MD5_DIGEST_LEN, 16 as usize);
        expect_eq(MD5_BLOCK_LEN, 64 as usize);

        let hex: [32]u8 = undefined;

        expect_eq(md5Hex("", hex[0..32]), 32 as usize);
        expect_eq(md5TestHexMatches(hex[0..32], "d41d8cd98f00b204e9800998ecf8427e"), true);

        expect_eq(md5Hex("a", hex[0..32]), 32 as usize);
        expect_eq(md5TestHexMatches(hex[0..32], "0cc175b9c0f1b6a831c399e269772661"), true);

        expect_eq(md5Hex("abc", hex[0..32]), 32 as usize);
        expect_eq(md5TestHexMatches(hex[0..32], "900150983cd24fb0d6963f7d28e17f72"), true);

        expect_eq(md5Hex("message digest", hex[0..32]), 32 as usize);
        expect_eq(md5TestHexMatches(hex[0..32], "f96b697d7cb7938d525a2f31aaf161d0"), true);

        expect_eq(md5Hex("abcdefghijklmnopqrstuvwxyz", hex[0..32]), 32 as usize);
        expect_eq(md5TestHexMatches(hex[0..32], "c3fcd3d76192e4007dfb496cca67e13b"), true);
    });

    test("2. @cap/md5-crate — RFC 1321 multi-block extended vectors", () => {
        let hex: [32]u8 = undefined;

        expect_eq(
            md5Hex("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", hex[0..32]),
            32 as usize
        );
        expect_eq(md5TestHexMatches(hex[0..32], "d174ab98d277d9f5a5611c2c9f419d9f"), true);

        expect_eq(
            md5Hex("12345678901234567890123456789012345678901234567890123456789012345678901234567890", hex[0..32]),
            32 as usize
        );
        expect_eq(md5TestHexMatches(hex[0..32], "57edf4a22be3c955ac49da2e2107b67a"), true);
    });

    test("3. @cap/md5-crate — Streaming chunked updateStr & single-byte update", () => {
        let h = Md5Hasher.new();
        h.updateStr("abcdef");
        h.updateStr("ghijklm");
        h.updateStr("nopqrst");
        h.updateStr("uvwxyz");

        let hex: [32]u8 = undefined;
        let n = h.finalizeHex(hex[0..32]);
        expect_eq(n, 32 as usize);
        expect_eq(md5TestHexMatches(hex[0..32], "c3fcd3d76192e4007dfb496cca67e13b"), true);

        let h2 = Md5Hasher.new();
        let b0: [1]u8 = ['a' as u8];
        let b1: [1]u8 = ['b' as u8];
        let b2: [1]u8 = ['c' as u8];
        h2.update(b0[0..1]);
        h2.update(b1[0..1]);
        h2.update(b2[0..1]);
        * = h2.finalizeHex(hex[0..32]);
        expect_eq(md5TestHexMatches(hex[0..32], "900150983cd24fb0d6963f7d28e17f72"), true);
    });

    test("4. @cap/md5-crate — Padding boundary lengths (55, 56, 64 bytes)", () => {
        let data: [64]u8 = undefined;
        let i: usize = 0;
        while (i < (64 as usize)) {
            data[i] = ('a' as u8) + ((i % (26 as usize)) as u8);
            i += 1;
        }
        let d_one: [16]u8 = undefined;
        let d_str: [16]u8 = undefined;

        md5Bytes(data[0..55], d_one[0..16]);
        let h55 = Md5Hasher.new();
        h55.update(data[0..25]);
        h55.update(data[25..55]);
        h55.finalize(d_str[0..16]);
        expect_eq(d_one[0], d_str[0]);
        expect_eq(d_one[15], d_str[15]);

        md5Bytes(data[0..56], d_one[0..16]);
        let h56 = Md5Hasher.new();
        h56.update(data[0..32]);
        h56.update(data[32..56]);
        h56.finalize(d_str[0..16]);
        expect_eq(d_one[0], d_str[0]);
        expect_eq(d_one[15], d_str[15]);

        md5Bytes(data[0..64], d_one[0..16]);
        let h64 = Md5Hasher.new();
        h64.update(data[0..64]);
        h64.finalize(d_str[0..16]);
        expect_eq(d_one[0], d_str[0]);
        expect_eq(d_one[15], d_str[15]);
    });

    test("5. @cap/md5-crate — Reset allows clean hasher reuse", () => {
        let h = Md5Hasher.new();
        let hex: [32]u8 = undefined;

        h.updateStr("abc");
        * = h.finalizeHex(hex[0..32]);
        expect_eq(md5TestHexMatches(hex[0..32], "900150983cd24fb0d6963f7d28e17f72"), true);

        h.reset();
        * = h.finalizeHex(hex[0..32]);
        expect_eq(md5TestHexMatches(hex[0..32], "d41d8cd98f00b204e9800998ecf8427e"), true);

        h.reset();
        h.updateStr("message digest");
        * = h.finalizeHex(hex[0..32]);
        expect_eq(md5TestHexMatches(hex[0..32], "f96b697d7cb7938d525a2f31aaf161d0"), true);
    });

    test("6. @cap/md5-crate — Raw 16-byte digest & short buffer safety", () => {
        let raw_abc: [3]u8 = ['a' as u8, 'b' as u8, 'c' as u8];
        let out: [16]u8 = undefined;
        md5Bytes(raw_abc[0..3], out[0..16]);

        // 90015098 3cd24fb0 d6963f7d 28e17f72
        expect_eq(out[0], 0x90 as u8);
        expect_eq(out[1], 0x01 as u8);
        expect_eq(out[2], 0x50 as u8);
        expect_eq(out[3], 0x98 as u8);
        expect_eq(out[12], 0x28 as u8);
        expect_eq(out[13], 0xE1 as u8);
        expect_eq(out[14], 0x7F as u8);
        expect_eq(out[15], 0x72 as u8);

        let small: [16]u8 = undefined;
        expect_eq(md5Hex("abc", small[0..16]), 0 as usize);
    });
}
