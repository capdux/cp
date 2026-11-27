// cap/test/testCapCbc.cp
// Comprehensive unit tests for @cap/cbc (NIST SP 800-38A F.2.1 & F.2.5 verified)

import { test, expect_eq } from "cap:test";
import {
    aes128CbcEncryptBlocks,
    aes128CbcDecryptBlocks,
    aes128CbcEncryptPkcs7,
    aes128CbcDecryptPkcs7,
    aes256CbcEncryptBlocks,
    aes256CbcDecryptBlocks,
    aes256CbcEncryptPkcs7,
    aes256CbcDecryptPkcs7,
} from "../../packages/cbc/src/index.cp";

fn cbcTestSlicesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/cbc — NIST SP 800-38A F.2.1 AES-128-CBC official 4-block test vector", () => {
        const key: [16]u8 = @as([16]u8, [
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
        ]);
        const iv: [16]u8 = @as([16]u8, [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        ]);
        const pt: [64]u8 = @as([64]u8, [
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
            0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
            0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
            0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
            0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
            0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
            0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
        ]);
        const expectedCt: [64]u8 = @as([64]u8, [
            0x76, 0x49, 0xab, 0xac, 0x81, 0x19, 0xb2, 0x46,
            0xce, 0xe9, 0x8e, 0x9b, 0x12, 0xe9, 0x19, 0x7d,
            0x50, 0x86, 0xcb, 0x9b, 0x50, 0x72, 0x19, 0xee,
            0x95, 0xdb, 0x11, 0x3a, 0x91, 0x76, 0x78, 0xb2,
            0x73, 0xbe, 0xd6, 0xb8, 0xe3, 0xc1, 0x74, 0x3b,
            0x71, 0x16, 0xe6, 0x9e, 0x22, 0x22, 0x95, 0x16,
            0x3f, 0xf1, 0xca, 0xa1, 0x68, 0x1f, 0xac, 0x09,
            0x12, 0x0e, 0xca, 0x30, 0x75, 0x86, 0xe1, 0xa7,
        ]);

        let ct: [64]u8 = undefined;
        let outPt: [64]u8 = undefined;
        const encLen = aes128CbcEncryptBlocks(key, iv, pt[0..64], ct[0..64]);
        expect_eq(encLen, 64 as usize);
        expect_eq(cbcTestSlicesEq(ct[0..64], expectedCt[0..64]), true);

        const decLen = aes128CbcDecryptBlocks(key, iv, ct[0..64], outPt[0..64]);
        expect_eq(decLen, 64 as usize);
        expect_eq(cbcTestSlicesEq(outPt[0..64], pt[0..64]), true);
    });

    test("2. @cap/cbc — NIST SP 800-38A F.2.5 AES-256-CBC official 4-block test vector", () => {
        const key: [32]u8 = @as([32]u8, [
            0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
            0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
            0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
            0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4,
        ]);
        const iv: [16]u8 = @as([16]u8, [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        ]);
        const pt: [64]u8 = @as([64]u8, [
            0x6b, 0xc1, 0xbe, 0xe2, 0x2e, 0x40, 0x9f, 0x96,
            0xe9, 0x3d, 0x7e, 0x11, 0x73, 0x93, 0x17, 0x2a,
            0xae, 0x2d, 0x8a, 0x57, 0x1e, 0x03, 0xac, 0x9c,
            0x9e, 0xb7, 0x6f, 0xac, 0x45, 0xaf, 0x8e, 0x51,
            0x30, 0xc8, 0x1c, 0x46, 0xa3, 0x5c, 0xe4, 0x11,
            0xe5, 0xfb, 0xc1, 0x19, 0x1a, 0x0a, 0x52, 0xef,
            0xf6, 0x9f, 0x24, 0x45, 0xdf, 0x4f, 0x9b, 0x17,
            0xad, 0x2b, 0x41, 0x7b, 0xe6, 0x6c, 0x37, 0x10,
        ]);
        const expectedCt: [64]u8 = @as([64]u8, [
            0xf5, 0x8c, 0x4c, 0x04, 0xd6, 0xe5, 0xf1, 0xba,
            0x77, 0x9e, 0xab, 0xfb, 0x5f, 0x7b, 0xfb, 0xd6,
            0x9c, 0xfc, 0x4e, 0x96, 0x7e, 0xdb, 0x80, 0x8d,
            0x67, 0x9f, 0x77, 0x7b, 0xc6, 0x70, 0x2c, 0x7d,
            0x39, 0xf2, 0x33, 0x69, 0xa9, 0xd9, 0xba, 0xcf,
            0xa5, 0x30, 0xe2, 0x63, 0x04, 0x23, 0x14, 0x61,
            0xb2, 0xeb, 0x05, 0xe2, 0xc3, 0x9b, 0xe9, 0xfc,
            0xda, 0x6c, 0x19, 0x07, 0x8c, 0x6a, 0x9d, 0x1b,
        ]);

        let ct: [64]u8 = undefined;
        let outPt: [64]u8 = undefined;
        const encLen = aes256CbcEncryptBlocks(key, iv, pt[0..64], ct[0..64]);
        expect_eq(encLen, 64 as usize);
        expect_eq(cbcTestSlicesEq(ct[0..64], expectedCt[0..64]), true);

        const decLen = aes256CbcDecryptBlocks(key, iv, ct[0..64], outPt[0..64]);
        expect_eq(decLen, 64 as usize);
        expect_eq(cbcTestSlicesEq(outPt[0..64], pt[0..64]), true);
    });

    test("3. @cap/cbc — AES-128-CBC PKCS#7 round-trip (empty, partial, exact block multiple)", () => {
        const key: [16]u8 = @as([16]u8, [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        ]);
        const iv: [16]u8 = @as([16]u8, [
            0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
            0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        ]);

        let ct: [64]u8 = undefined;
        let ptOut: [64]u8 = undefined;

        // Empty plaintext -> 16-byte padded block (0x10 * 16)
        const emptyCtLen = aes128CbcEncryptPkcs7(key, iv, "", ct[0..64]);
        expect_eq(emptyCtLen, 16 as usize);
        const emptyPtLen = aes128CbcDecryptPkcs7(key, iv, ct[0..emptyCtLen], ptOut[0..64]);
        expect_eq(emptyPtLen, 0 as i64);

        // Partial block (13 bytes) -> 16 bytes
        const msg1 = "Hello, World!";
        const ct1Len = aes128CbcEncryptPkcs7(key, iv, msg1, ct[0..64]);
        expect_eq(ct1Len, 16 as usize);
        const pt1Len = aes128CbcDecryptPkcs7(key, iv, ct[0..ct1Len], ptOut[0..64]);
        expect_eq(pt1Len, 13 as i64);
        expect_eq(cbcTestSlicesEq(ptOut[0..13], msg1), true);

        // Exact 16-byte block -> 32 bytes (full extra padding block)
        const msg2 = "0123456789abcdef";
        const ct2Len = aes128CbcEncryptPkcs7(key, iv, msg2, ct[0..64]);
        expect_eq(ct2Len, 32 as usize);
        const pt2Len = aes128CbcDecryptPkcs7(key, iv, ct[0..ct2Len], ptOut[0..64]);
        expect_eq(pt2Len, 16 as i64);
        expect_eq(cbcTestSlicesEq(ptOut[0..16], msg2), true);
    });

    test("4. @cap/cbc — AES-256-CBC PKCS#7 multi-block round-trip", () => {
        const key: [32]u8 = @as([32]u8, [
            0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
            0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
            0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
            0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4,
        ]);
        const iv: [16]u8 = @as([16]u8, [
            0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
            0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
        ]);
        const msg = "The quick brown fox jumps over the lazy dog!"; // 44 bytes -> 48 bytes
        let ct: [64]u8 = undefined;
        let ptOut: [64]u8 = undefined;

        const ctLen = aes256CbcEncryptPkcs7(key, iv, msg, ct[0..64]);
        expect_eq(ctLen, 48 as usize);
        const ptLen = aes256CbcDecryptPkcs7(key, iv, ct[0..ctLen], ptOut[0..44]);
        expect_eq(ptLen, 44 as i64);
        expect_eq(cbcTestSlicesEq(ptOut[0..44], msg), true);
    });

    test("5. @cap/cbc — Strict PKCS#7 padding rejection on tampered ciphertext", () => {
        const key: [16]u8 = @as([16]u8, [
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c,
        ]);
        const iv: [16]u8 = @as([16]u8, [
            0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        ]);
        const msg = "0123456789abcdef1234"; // 20 bytes -> 32 bytes (pad=12)
        let ct: [32]u8 = undefined;
        let ptOut: [32]u8 = undefined;

        const ctLen = aes128CbcEncryptPkcs7(key, iv, msg, ct[0..32]);
        expect_eq(ctLen, 32 as usize);

        // Tamper previous ciphertext block last byte -> corrupts last padding byte
        ct[15] = ct[15] ^ (0x01 as u8);
        expect_eq(aes128CbcDecryptPkcs7(key, iv, ct[0..32], ptOut[0..32]), -1 as i64);

        // Restore byte 15, tamper byte 14 -> corrupts second-to-last padding byte
        ct[15] = ct[15] ^ (0x01 as u8);
        ct[14] = ct[14] ^ (0x01 as u8);
        expect_eq(aes128CbcDecryptPkcs7(key, iv, ct[0..32], ptOut[0..32]), -1 as i64);

        // Restore -> valid decryption again
        ct[14] = ct[14] ^ (0x01 as u8);
        expect_eq(aes128CbcDecryptPkcs7(key, iv, ct[0..32], ptOut[0..32]), 20 as i64);
    });

    test("6. @cap/cbc — Error boundaries (unaligned input, empty ciphertext, small output buffer)", () => {
        const key128: [16]u8 = @as([16]u8, [
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
        ]);
        const key256: [32]u8 = @as([32]u8, [
            1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
            17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
        ]);
        const iv: [16]u8 = @as([16]u8, [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        ]);
        let smallBuf: [8]u8 = undefined;
        let buf32: [32]u8 = undefined;

        // Unaligned whole-block calls return 0
        expect_eq(aes128CbcEncryptBlocks(key128, iv, "abc", buf32[0..32]), 0 as usize);
        expect_eq(aes128CbcDecryptBlocks(key128, iv, "abc", buf32[0..32]), 0 as usize);
        expect_eq(aes256CbcEncryptBlocks(key256, iv, "", buf32[0..32]), 0 as usize);

        // Output buffer too small
        expect_eq(aes128CbcEncryptPkcs7(key128, iv, "hello", smallBuf[0..8]), 0 as usize);
        expect_eq(aes256CbcEncryptPkcs7(key256, iv, "hello", smallBuf[0..8]), 0 as usize);

        // Empty or unaligned ciphertext to DecryptPkcs7 returns -1
        expect_eq(aes128CbcDecryptPkcs7(key128, iv, "", buf32[0..32]), -1 as i64);
        expect_eq(aes256CbcDecryptPkcs7(key256, iv, "short", buf32[0..32]), -1 as i64);
    });
}
