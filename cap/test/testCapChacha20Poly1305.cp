// cap/test/testCapChacha20Poly1305.cp
// Comprehensive unit tests for @cap/chacha20-poly1305
// Covers RFC 8439 §2.8.2 official AEAD vector, tamper rejection & zeroization,
// empty plaintext/AAD, ChaCha20 block/XOR/Poly1305 keygen, HChaCha20 & XChaCha20-Poly1305.

import { test, expect_eq } from "cap:test";
import {
    CHACHA20_KEY_LEN,
    CHACHA20_NONCE_LEN,
    CHACHA20_BLOCK_LEN,
    CHACHA20_TAG_LEN,
    XCHACHA20_NONCE_LEN,
    chacha20Block,
    chacha20Xor,
    poly1305KeyGen,
    aeadEncrypt,
    aeadDecrypt,
    xchacha20HChaCha20,
    xchacha20Poly1305Encrypt,
    xchacha20Poly1305Decrypt,
} from "../../packages/@cap/chacha20-poly1305/src/index.cp";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn allZero(a: []const u8): bool {
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != 0) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn fillRfc8439Key(out: []u8) {
    let i: usize = 0;
    while (i < 32) {
        out[i] = (0x80 + i) as u8;
        i = i + 1;
    }
}

fn main() {
    test("1. @cap/chacha20-poly1305 — RFC 8439 Section 2.8.2 official AEAD test vector", () => {
        let key: [32]u8 = undefined;
        fillRfc8439Key(key[0..32]);

        let nonce: [12]u8 = [
            0x07, 0x00, 0x00, 0x00,
            0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47
        ];
        let aad: [12]u8 = [
            0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3,
            0xc4, 0xc5, 0xc6, 0xc7
        ];

        let pt_str = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
        expect_eq(pt_str.len, 114 as usize);
        let pt: [114]u8 = undefined;
        let i: usize = 0;
        while (i < 114) {
            pt[i] = pt_str[i];
            i = i + 1;
        }

        let ct: [114]u8 = undefined;
        let tag: [16]u8 = undefined;
        expect_eq(aeadEncrypt(key[0..32], nonce[0..12], aad[0..12], pt[0..114], ct[0..114], tag[0..16]), true);

        let expected_ct: [114]u8 = [
            0xd3, 0x1a, 0x8d, 0x34, 0x64, 0x8e, 0x60, 0xdb, 0x7b, 0x86, 0xaf, 0xbc, 0x53, 0xef, 0x7e, 0xc2,
            0xa4, 0xad, 0xed, 0x51, 0x29, 0x6e, 0x08, 0xfe, 0xa9, 0xe2, 0xb5, 0xa7, 0x36, 0xee, 0x62, 0xd6,
            0x3d, 0xbe, 0xa4, 0x5e, 0x8c, 0xa9, 0x67, 0x12, 0x82, 0xfa, 0xfb, 0x69, 0xda, 0x92, 0x72, 0x8b,
            0x1a, 0x71, 0xde, 0x0a, 0x9e, 0x06, 0x0b, 0x29, 0x05, 0xd6, 0xa5, 0xb6, 0x7e, 0xcd, 0x3b, 0x36,
            0x92, 0xdd, 0xbd, 0x7f, 0x2d, 0x77, 0x8b, 0x8c, 0x98, 0x03, 0xae, 0xe3, 0x28, 0x09, 0x1b, 0x58,
            0xfa, 0xb3, 0x24, 0xe4, 0xfa, 0xd6, 0x75, 0x94, 0x55, 0x85, 0x80, 0x8b, 0x48, 0x31, 0xd7, 0xbc,
            0x3f, 0xf4, 0xde, 0xf0, 0x8e, 0x4b, 0x7a, 0x9d, 0xe5, 0x76, 0xd2, 0x65, 0x86, 0xce, 0xc6, 0x4b,
            0x61, 0x16
        ];
        let expected_tag: [16]u8 = [
            0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a,
            0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91
        ];

        expect_eq(bytesEq(ct[0..114], expected_ct[0..114]), true);
        expect_eq(bytesEq(tag[0..16], expected_tag[0..16]), true);

        let dec_buf: [114]u8 = undefined;
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], aad[0..12], ct[0..114], tag[0..16], dec_buf[0..114]), true);
        expect_eq(bytesEq(dec_buf[0..114], pt[0..114]), true);
    });

    test("2. @cap/chacha20-poly1305 — Tampered ciphertext, AAD, or tag rejected & plaintext zeroized", () => {
        let key: [32]u8 = undefined;
        fillRfc8439Key(key[0..32]);
        let nonce: [12]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
        let aad: [8]u8 = [0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44];
        let pt: [24]u8 = [
            0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x43, 0x50,
            0x20, 0x41, 0x45, 0x41, 0x44, 0x20, 0x53, 0x65,
            0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x21, 0x21
        ];

        let ct: [24]u8 = undefined;
        let tag: [16]u8 = undefined;
        expect_eq(aeadEncrypt(key[0..32], nonce[0..12], aad[0..8], pt[0..24], ct[0..24], tag[0..16]), true);

        // Pre-fill plaintext buffer with sentinel bytes to verify zeroization on failure
        let out: [24]u8 = undefined;
        let i: usize = 0;
        while (i < 24) {
            out[i] = 0xEE;
            i = i + 1;
        }

        // Flip 1 bit in ciphertext
        ct[10] = ct[10] ^ 0x01;
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], aad[0..8], ct[0..24], tag[0..16], out[0..24]), false);
        expect_eq(allZero(out[0..24]), true);
        ct[10] = ct[10] ^ 0x01;

        // Flip 1 bit in AAD
        i = 0;
        while (i < 24) {
            out[i] = 0xDD;
            i = i + 1;
        }
        aad[3] = aad[3] ^ 0x80;
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], aad[0..8], ct[0..24], tag[0..16], out[0..24]), false);
        expect_eq(allZero(out[0..24]), true);
        aad[3] = aad[3] ^ 0x80;

        // Flip 1 bit in tag
        i = 0;
        while (i < 24) {
            out[i] = 0xCC;
            i = i + 1;
        }
        tag[15] = tag[15] ^ 0x04;
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], aad[0..8], ct[0..24], tag[0..16], out[0..24]), false);
        expect_eq(allZero(out[0..24]), true);
        tag[15] = tag[15] ^ 0x04;

        // Untampered decrypt succeeds
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], aad[0..8], ct[0..24], tag[0..16], out[0..24]), true);
        expect_eq(bytesEq(out[0..24], pt[0..24]), true);
    });

    test("3. @cap/chacha20-poly1305 — Empty plaintext and empty AAD combinations", () => {
        let key: [32]u8 = undefined;
        fillRfc8439Key(key[0..32]);
        let nonce: [12]u8 = [0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b];
        let empty: [1]u8 = [0];
        let aad: [5]u8 = [0x61, 0x61, 0x64, 0x2d, 0x31];
        let pt: [7]u8 = [0x70, 0x61, 0x79, 0x6c, 0x6f, 0x61, 0x64];

        // Case A: empty AAD + empty plaintext
        let tag_a: [16]u8 = undefined;
        expect_eq(aeadEncrypt(key[0..32], nonce[0..12], empty[0..0], empty[0..0], empty[0..0], tag_a[0..16]), true);
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], empty[0..0], empty[0..0], tag_a[0..16], empty[0..0]), true);

        // Case B: non-empty AAD + empty plaintext
        let tag_b: [16]u8 = undefined;
        expect_eq(aeadEncrypt(key[0..32], nonce[0..12], aad[0..5], empty[0..0], empty[0..0], tag_b[0..16]), true);
        expect_eq(bytesEq(tag_a[0..16], tag_b[0..16]), false);
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], aad[0..5], empty[0..0], tag_b[0..16], empty[0..0]), true);

        // Case C: empty AAD + non-empty plaintext
        let ct_c: [7]u8 = undefined;
        let dec_c: [7]u8 = undefined;
        let tag_c: [16]u8 = undefined;
        expect_eq(aeadEncrypt(key[0..32], nonce[0..12], empty[0..0], pt[0..7], ct_c[0..7], tag_c[0..16]), true);
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], empty[0..0], ct_c[0..7], tag_c[0..16], dec_c[0..7]), true);
        expect_eq(bytesEq(dec_c[0..7], pt[0..7]), true);
    });

    test("4. @cap/chacha20-poly1305 — ChaCha20 block, stream XOR & Poly1305 keygen primitives", () => {
        expect_eq(CHACHA20_KEY_LEN, 32 as usize);
        expect_eq(CHACHA20_NONCE_LEN, 12 as usize);
        expect_eq(CHACHA20_BLOCK_LEN, 64 as usize);
        expect_eq(CHACHA20_TAG_LEN, 16 as usize);
        expect_eq(XCHACHA20_NONCE_LEN, 24 as usize);

        // RFC 8439 Section 2.6.2 Poly1305 keygen test vector
        let key: [32]u8 = undefined;
        fillRfc8439Key(key[0..32]);
        let nonce: [12]u8 = [0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07];

        let blk: [64]u8 = undefined;
        let otk: [32]u8 = undefined;
        expect_eq(chacha20Block(key[0..32], 0 as u32, nonce[0..12], blk[0..64]), true);
        expect_eq(poly1305KeyGen(key[0..32], nonce[0..12], otk[0..32]), true);
        expect_eq(bytesEq(blk[0..32], otk[0..32]), true);

        let expected_otk: [32]u8 = [
            0x8a, 0xd5, 0xa0, 0x8b, 0x90, 0x5f, 0x81, 0xcc,
            0x81, 0x50, 0x40, 0x27, 0x4a, 0xb2, 0x94, 0x71,
            0xa8, 0x33, 0xb6, 0x37, 0xe3, 0xfd, 0x0d, 0xa5,
            0x08, 0xdb, 0xb8, 0xe2, 0xfd, 0xd1, 0xa6, 0x46
        ];
        expect_eq(bytesEq(otk[0..32], expected_otk[0..32]), true);

        // Multi-block ChaCha20 XOR involution (130 bytes > 2 blocks)
        let msg: [130]u8 = undefined;
        let i: usize = 0;
        while (i < 130) {
            msg[i] = ((i * 7 + 13) & 0xFF) as u8;
            i = i + 1;
        }
        let enc: [130]u8 = undefined;
        let dec_buf: [130]u8 = undefined;
        expect_eq(chacha20Xor(key[0..32], 1 as u32, nonce[0..12], msg[0..130], enc[0..130]), true);
        expect_eq(bytesEq(msg[0..130], enc[0..130]), false);
        expect_eq(chacha20Xor(key[0..32], 1 as u32, nonce[0..12], enc[0..130], dec_buf[0..130]), true);
        expect_eq(bytesEq(msg[0..130], dec_buf[0..130]), true);
    });

    test("5. @cap/chacha20-poly1305 — HChaCha20 official vector & XChaCha20-Poly1305 round-trip", () => {
        // draft-irtf-cfrg-xchacha Section 2.2.1 official HChaCha20 vector
        let hkey: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            hkey[i] = i as u8;
            i = i + 1;
        }
        let hnonce: [16]u8 = [
            0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4a,
            0x00, 0x00, 0x00, 0x00, 0x31, 0x41, 0x59, 0x27
        ];
        let subkey: [32]u8 = undefined;
        expect_eq(xchacha20HChaCha20(hkey[0..32], hnonce[0..16], subkey[0..32]), true);

        let expected_subkey: [32]u8 = [
            0x82, 0x41, 0x3b, 0x42, 0x27, 0xb2, 0x7b, 0xfe,
            0xd3, 0x0e, 0x42, 0x50, 0x8a, 0x87, 0x7d, 0x73,
            0xa0, 0xf9, 0xe4, 0xd5, 0x8a, 0x74, 0xa8, 0x53,
            0xc1, 0x2e, 0xc4, 0x13, 0x26, 0xd3, 0xec, 0xdc
        ];
        expect_eq(bytesEq(subkey[0..32], expected_subkey[0..32]), true);

        // XChaCha20-Poly1305 192-bit nonce AEAD round-trip + tamper rejection
        let key: [32]u8 = undefined;
        fillRfc8439Key(key[0..32]);
        let nonce24: [24]u8 = undefined;
        i = 0;
        while (i < 24) {
            nonce24[i] = (0x40 + i) as u8;
            i = i + 1;
        }
        let aad: [12]u8 = [0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7];
        let pt: [32]u8 = undefined;
        i = 0;
        while (i < 32) {
            pt[i] = (0x30 + i) as u8;
            i = i + 1;
        }

        let ct: [32]u8 = undefined;
        let tag: [16]u8 = undefined;
        let dec_buf: [32]u8 = undefined;
        expect_eq(xchacha20Poly1305Encrypt(key[0..32], nonce24[0..24], aad[0..12], pt[0..32], ct[0..32], tag[0..16]), true);
        expect_eq(bytesEq(ct[0..32], pt[0..32]), false);
        expect_eq(xchacha20Poly1305Decrypt(key[0..32], nonce24[0..24], aad[0..12], ct[0..32], tag[0..16], dec_buf[0..32]), true);
        expect_eq(bytesEq(dec_buf[0..32], pt[0..32]), true);

        // Tamper tag -> must fail and zeroize dec_buf
        tag[0] = tag[0] ^ 0xFF;
        expect_eq(xchacha20Poly1305Decrypt(key[0..32], nonce24[0..24], aad[0..12], ct[0..32], tag[0..16], dec_buf[0..32]), false);
        expect_eq(allZero(dec_buf[0..32]), true);
    });

    test("6. @cap/chacha20-poly1305 — Invalid buffer lengths safely return false", () => {
        let key: [32]u8 = undefined;
        let nonce12: [12]u8 = undefined;
        let nonce24: [24]u8 = undefined;
        let buf64: [64]u8 = undefined;
        let tag: [16]u8 = undefined;

        expect_eq(chacha20Block(key[0..31], 0 as u32, nonce12[0..12], buf64[0..64]), false);
        expect_eq(chacha20Block(key[0..32], 0 as u32, nonce12[0..11], buf64[0..64]), false);
        expect_eq(chacha20Block(key[0..32], 0 as u32, nonce12[0..12], buf64[0..63]), false);

        expect_eq(poly1305KeyGen(key[0..31], nonce12[0..12], buf64[0..32]), false);
        expect_eq(poly1305KeyGen(key[0..32], nonce12[0..12], buf64[0..31]), false);

        expect_eq(aeadEncrypt(key[0..31], nonce12[0..12], buf64[0..4], buf64[0..8], buf64[0..8], tag[0..16]), false);
        expect_eq(aeadEncrypt(key[0..32], nonce12[0..12], buf64[0..4], buf64[0..8], buf64[0..7], tag[0..16]), false);
        expect_eq(aeadEncrypt(key[0..32], nonce12[0..12], buf64[0..4], buf64[0..8], buf64[0..8], tag[0..15]), false);
        expect_eq(aeadDecrypt(key[0..32], nonce12[0..12], buf64[0..4], buf64[0..8], tag[0..15], buf64[0..8]), false);

        expect_eq(xchacha20HChaCha20(key[0..32], nonce24[0..15], buf64[0..32]), false);
        expect_eq(xchacha20Poly1305Encrypt(key[0..32], nonce24[0..23], buf64[0..4], buf64[0..8], buf64[0..8], tag[0..16]), false);
        expect_eq(xchacha20Poly1305Decrypt(key[0..32], nonce24[0..23], buf64[0..4], buf64[0..8], tag[0..16], buf64[0..8]), false);
    });
}
