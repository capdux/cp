// cap/test/testCapAes.cp
// Comprehensive unit tests for @cap/aes (FIPS-197 Appendix C.1 / C.2 / C.3 verified)

import { test, expect_eq } from "cap:test";
import {
    Aes128,
    Aes192,
    Aes256,
} from "../../packages/aes/src/index.cp";

fn blocksEq(a: [16]u8, b: [16]u8): bool {
    let i: usize = 0;
    while (i < 16) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/aes — FIPS-197 Appendix C.1 AES-128 official test vector", () => {
        const key: [16]u8 = @as([16]u8, [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        ]);
        const pt: [16]u8 = @as([16]u8, [
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
        ]);
        const expectedCt: [16]u8 = @as([16]u8, [
            0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
            0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a,
        ]);

        const cipher = Aes128.new(key);
        const ct = cipher.encryptBlock(pt);
        expect_eq(blocksEq(ct, expectedCt), true);

        const decPt = cipher.decryptBlock(ct);
        expect_eq(blocksEq(decPt, pt), true);
    });

    test("2. @cap/aes — FIPS-197 Appendix C.2 AES-192 official test vector", () => {
        const key: [24]u8 = @as([24]u8, [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        ]);
        const pt: [16]u8 = @as([16]u8, [
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
        ]);
        const expectedCt: [16]u8 = @as([16]u8, [
            0xdd, 0xa9, 0x7c, 0xa4, 0x86, 0x4c, 0xdf, 0xe0,
            0x6e, 0xaf, 0x70, 0xa0, 0xec, 0x0d, 0x71, 0x91,
        ]);

        const cipher = Aes192.new(key);
        const ct = cipher.encryptBlock(pt);
        expect_eq(blocksEq(ct, expectedCt), true);

        const decPt = cipher.decryptBlock(ct);
        expect_eq(blocksEq(decPt, pt), true);
    });

    test("3. @cap/aes — FIPS-197 Appendix C.3 AES-256 official test vector", () => {
        const key: [32]u8 = @as([32]u8, [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        ]);
        const pt: [16]u8 = @as([16]u8, [
            0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
        ]);
        const expectedCt: [16]u8 = @as([16]u8, [
            0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
            0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89,
        ]);

        const cipher = Aes256.new(key);
        const ct = cipher.encryptBlock(pt);
        expect_eq(blocksEq(ct, expectedCt), true);

        const decPt = cipher.decryptBlock(ct);
        expect_eq(blocksEq(decPt, pt), true);
    });

    test("4. @cap/aes — FIPS-197 Appendix B AES-128 cipher example vector", () => {
        const key: [16]u8 = @as([16]u8, [
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
        ]);
        const pt: [16]u8 = @as([16]u8, [
            0x32, 0x43, 0xf6, 0xa8, 0x88, 0x5a, 0x30, 0x8d,
            0x31, 0x31, 0x98, 0xa2, 0xe0, 0x37, 0x07, 0x34,
        ]);
        const expectedCt: [16]u8 = @as([16]u8, [
            0x39, 0x25, 0x84, 0x1d, 0x02, 0xdc, 0x09, 0xfb,
            0xdc, 0x11, 0x85, 0x97, 0x19, 0x6a, 0x0b, 0x32,
        ]);

        const cipher = Aes128.new(key);
        const ct = cipher.encryptBlock(pt);
        expect_eq(blocksEq(ct, expectedCt), true);
        expect_eq(blocksEq(cipher.decryptBlock(ct), pt), true);
    });

    test("5. @cap/aes — All-zero key & plaintext round-trip across 128/192/256", () => {
        const z16: [16]u8 = @as([16]u8, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]);
        const z24: [24]u8 = @as([24]u8, [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ]);
        const z32: [32]u8 = @as([32]u8, [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ]);
        // Known NIST KAT: AES-128(0^128, 0^128) = 66e94bd4ef8a2c3b884cfa59ca342b2e
        const expected128: [16]u8 = @as([16]u8, [
            0x66, 0xe9, 0x4b, 0xd4, 0xef, 0x8a, 0x2c, 0x3b,
            0x88, 0x4c, 0xfa, 0x59, 0xca, 0x34, 0x2b, 0x2e,
        ]);

        const c128 = Aes128.new(z16);
        const ct128 = c128.encryptBlock(z16);
        expect_eq(blocksEq(ct128, expected128), true);
        expect_eq(blocksEq(c128.decryptBlock(ct128), z16), true);

        const c192 = Aes192.new(z24);
        const ct192 = c192.encryptBlock(z16);
        expect_eq(blocksEq(c192.decryptBlock(ct192), z16), true);

        const c256 = Aes256.new(z32);
        const ct256 = c256.encryptBlock(z16);
        expect_eq(blocksEq(c256.decryptBlock(ct256), z16), true);
    });

    test("6. @cap/aes — Single-bit avalanche & key sensitivity", () => {
        const keyA: [16]u8 = @as([16]u8, [
            0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
            0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x00,
        ]);
        const keyB: [16]u8 = @as([16]u8, [
            0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80,
            0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x01,
        ]);
        const ptA: [16]u8 = @as([16]u8, [
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
        ]);
        const ptB: [16]u8 = @as([16]u8, [
            0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
            0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x11,
        ]);

        const cA = Aes128.new(keyA);
        const cB = Aes128.new(keyB);
        const ctA = cA.encryptBlock(ptA);
        const ctB = cA.encryptBlock(ptB);
        const ctKeyDiff = cB.encryptBlock(ptA);

        let diffPtBytes: usize = 0;
        let diffKeyBytes: usize = 0;
        let i: usize = 0;
        while (i < 16) {
            if (ctA[i] != ctB[i]) {
                diffPtBytes += 1;
            }
            if (ctA[i] != ctKeyDiff[i]) {
                diffKeyBytes += 1;
            }
            i += 1;
        }
        expect_eq(diffPtBytes >= (12 as usize), true);
        expect_eq(diffKeyBytes >= (12 as usize), true);
    });
}
