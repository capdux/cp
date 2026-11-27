// cap/test/testCapChacha20poly1305.cp
// Comprehensive unit tests for @cap/chacha20poly1305 (RFC 8439 AEAD & XChaCha20-Poly1305)

import { test, expect_eq } from "cap:test";
import {
    CHACHA20_KEY_LEN,
    CHACHA20_NONCE_LEN,
    CHACHA20_TAG_LEN,
    aeadEncrypt,
    aeadDecrypt,
    xchacha20Poly1305Encrypt,
    xchacha20Poly1305Decrypt,
} from "../../packages/chacha20poly1305/src/index.cp";

fn aeadEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/chacha20poly1305 — constants & AEAD seal/open round-trip", () => {
        expect_eq(CHACHA20_KEY_LEN, 32 as usize);
        expect_eq(CHACHA20_NONCE_LEN, 12 as usize);
        expect_eq(CHACHA20_TAG_LEN, 16 as usize);

        let key: [32]u8 = [0; 32];
        let i: usize = 0;
        while (i < 32) { key[i] = (0x80 + i) as u8; i += 1; }
        let nonce: [12]u8 = [0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47];
        const aad = "header-auth";
        const pt = "Hello ChaCha20-Poly1305 AEAD!";

        let ct: [64]u8 = [0; 64];
        let tag: [16]u8 = [0; 16];
        expect_eq(aeadEncrypt(key[0..32], nonce[0..12], aad, pt, ct[0..pt.len], tag[0..16]), true);
        expect_eq(aeadEq(ct[0..pt.len], pt), false);

        let dec_buf: [64]u8 = [0; 64];
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], aad, ct[0..pt.len], tag[0..16], dec_buf[0..pt.len]), true);
        expect_eq(aeadEq(dec_buf[0..pt.len], pt), true);
    });

    test("2. @cap/chacha20poly1305 — rejects tampered ciphertext, AAD or tag", () => {
        let key: [32]u8 = [0x42; 32];
        let nonce: [12]u8 = [0x11; 12];
        const pt = "sensitive payload";
        let ct: [32]u8 = [0; 32];
        let tag: [16]u8 = [0; 16];
        let out: [32]u8 = [0; 32];

        expect_eq(aeadEncrypt(key[0..32], nonce[0..12], "aad1", pt, ct[0..pt.len], tag[0..16]), true);
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], "aad2", ct[0..pt.len], tag[0..16], out[0..pt.len]), false);

        ct[0] = ct[0] ^ 0x01;
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], "aad1", ct[0..pt.len], tag[0..16], out[0..pt.len]), false);
        ct[0] = ct[0] ^ 0x01;

        tag[15] = tag[15] ^ 0x80;
        expect_eq(aeadDecrypt(key[0..32], nonce[0..12], "aad1", ct[0..pt.len], tag[0..16], out[0..pt.len]), false);
    });

    test("3. @cap/chacha20poly1305 — XChaCha20-Poly1305 24-byte nonce round-trip", () => {
        let key: [32]u8 = [0xAB; 32];
        let nonce24: [24]u8 = [0xCD; 24];
        const pt = "extended nonce message";
        let ct: [32]u8 = [0; 32];
        let tag: [16]u8 = [0; 16];
        let out: [32]u8 = [0; 32];

        expect_eq(xchacha20Poly1305Encrypt(key[0..32], nonce24[0..24], "ctx", pt, ct[0..pt.len], tag[0..16]), true);
        expect_eq(xchacha20Poly1305Decrypt(key[0..32], nonce24[0..24], "ctx", ct[0..pt.len], tag[0..16], out[0..pt.len]), true);
        expect_eq(aeadEq(out[0..pt.len], pt), true);
    });
}
