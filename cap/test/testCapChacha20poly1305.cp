// cap/test/testCapChacha20poly1305.cp
// Comprehensive unit tests for @cap/chacha20poly1305 (RFC 8439 AEAD)

import { test, expect_eq } from "cap:test";
import {
    AEAD_KEY_LEN,
    AEAD_NONCE_LEN,
    AEAD_TAG_LEN,
    ChaCha20Poly1305,
} from "../../packages/chacha20poly1305/src/index.cp";

fn aeadTestHexNibble(ch: u8): u8 {
    if (ch >= ('0' as u8) && ch <= ('9' as u8)) {
        return ch - ('0' as u8);
    }
    return (ch - ('a' as u8)) + (10 as u8);
}

fn aeadTestMatchesHex(bytes: []u8, expected_hex: string): bool {
    if (bytes.len * (2 as usize) != expected_hex.len) {
        return false;
    }
    let i: usize = 0;
    while (i < bytes.len) {
        let hi: u8 = aeadTestHexNibble(expected_hex[i * (2 as usize)]);
        let lo: u8 = aeadTestHexNibble(expected_hex[i * (2 as usize) + (1 as usize)]);
        if (bytes[i] != ((hi << 4) | lo)) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn aeadFillRfcKey(key: []u8) {
    let i: usize = 0;
    while (i < (32 as usize)) {
        key[i] = (0x80 as u8) + (i as u8);
        i += 1;
    }
}

fn main() {
    test("1. @cap/chacha20poly1305 — RFC 8439 Section 2.8.2 official AEAD encryption vector", () => {
        expect_eq(AEAD_KEY_LEN, 32 as usize);
        expect_eq(AEAD_NONCE_LEN, 12 as usize);
        expect_eq(AEAD_TAG_LEN, 16 as usize);

        let key: [32]u8 = undefined;
        aeadFillRfcKey(key[0..32]);

        let nonce: [12]u8 = [
            0x07 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
            0x40 as u8, 0x41 as u8, 0x42 as u8, 0x43 as u8,
            0x44 as u8, 0x45 as u8, 0x46 as u8, 0x47 as u8,
        ];
        let aad: [12]u8 = [
            0x50 as u8, 0x51 as u8, 0x52 as u8, 0x53 as u8,
            0xc0 as u8, 0xc1 as u8, 0xc2 as u8, 0xc3 as u8,
            0xc4 as u8, 0xc5 as u8, 0xc6 as u8, 0xc7 as u8,
        ];

        let pt_str = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
        let buf: [114]u8 = undefined;
        let i: usize = 0;
        while (i < (114 as usize)) {
            buf[i] = pt_str[i];
            i += 1;
        }

        let aead = ChaCha20Poly1305.new(key[0..32]).?;
        let tag: [16]u8 = undefined;
        let ok = aead.encryptInPlaceDetached(nonce[0..12], aad[0..12], buf[0..114], tag[0..16]);
        expect_eq(ok, true);

        let exp_ct = "d31a8d34648e60db7b86afbc53ef7ec2a4aded51296e08fea9e2b5a736ee62d63dbea45e8ca9671282fafb69da92728b1a71de0a9e060b2905d6a5b67ecd3b3692ddbd7f2d778b8c9803aee328091b58fab324e4fad675945585808b4831d7bc3ff4def08e4b7a9de576d26586cec64b6116";
        expect_eq(aeadTestMatchesHex(buf[0..114], exp_ct), true);
        expect_eq(aeadTestMatchesHex(tag[0..16], "1ae10b594f09e26a7e902ecbd0600691"), true);
    });

    test("2. @cap/chacha20poly1305 — RFC 8439 Section 2.8.2 AEAD decryption roundtrip", () => {
        let key: [32]u8 = undefined;
        aeadFillRfcKey(key[0..32]);
        let nonce: [12]u8 = [
            0x07 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
            0x40 as u8, 0x41 as u8, 0x42 as u8, 0x43 as u8,
            0x44 as u8, 0x45 as u8, 0x46 as u8, 0x47 as u8,
        ];
        let aad: [12]u8 = [
            0x50 as u8, 0x51 as u8, 0x52 as u8, 0x53 as u8,
            0xc0 as u8, 0xc1 as u8, 0xc2 as u8, 0xc3 as u8,
            0xc4 as u8, 0xc5 as u8, 0xc6 as u8, 0xc7 as u8,
        ];

        let pt_str = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
        let buf: [114]u8 = undefined;
        let i: usize = 0;
        while (i < (114 as usize)) {
            buf[i] = pt_str[i];
            i += 1;
        }

        let aead = ChaCha20Poly1305.new(key[0..32]).?;
        let tag: [16]u8 = undefined;
        * = aead.encryptInPlaceDetached(nonce[0..12], aad[0..12], buf[0..114], tag[0..16]);

        let ok = aead.decryptInPlaceDetached(nonce[0..12], aad[0..12], buf[0..114], tag[0..16]);
        expect_eq(ok, true);

        let same: bool = true;
        i = 0;
        while (i < (114 as usize)) {
            if (buf[i] != pt_str[i]) {
                same = false;
            }
            i += 1;
        }
        expect_eq(same, true);
    });

    test("3. @cap/chacha20poly1305 — Tampered ciphertext rejected without decrypting", () => {
        let key: [32]u8 = undefined;
        aeadFillRfcKey(key[0..32]);
        let nonce: [12]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
        let aad: [4]u8 = [0xAA as u8, 0xBB as u8, 0xCC as u8, 0xDD as u8];
        let buf: [16]u8 = [
            's' as u8, 'e' as u8, 'c' as u8, 'r' as u8,
            'e' as u8, 't' as u8, '-' as u8, 'p' as u8,
            'a' as u8, 'y' as u8, 'l' as u8, 'o' as u8,
            'a' as u8, 'd' as u8, '!' as u8, '!' as u8,
        ];

        let aead = ChaCha20Poly1305.new(key[0..32]).?;
        let tag: [16]u8 = undefined;
        * = aead.encryptInPlaceDetached(nonce[0..12], aad[0..4], buf[0..16], tag[0..16]);

        // Flip 1 bit in ciphertext
        buf[5] = buf[5] ^ (0x01 as u8);
        let saved_b0: u8 = buf[0];
        let ok = aead.decryptInPlaceDetached(nonce[0..12], aad[0..4], buf[0..16], tag[0..16]);
        expect_eq(ok, false);
        // Buffer was not decrypted on tag mismatch
        expect_eq(buf[0], saved_b0);
    });

    test("4. @cap/chacha20poly1305 — Tampered AAD or Tag rejected", () => {
        let key: [32]u8 = undefined;
        aeadFillRfcKey(key[0..32]);
        let nonce: [12]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12];
        let aad: [8]u8 = [10, 20, 30, 40, 50, 60, 70, 80];
        let buf: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];

        let aead = ChaCha20Poly1305.new(key[0..32]).?;
        let tag: [16]u8 = undefined;
        * = aead.encryptInPlaceDetached(nonce[0..12], aad[0..8], buf[0..8], tag[0..16]);

        // Tamper AAD
        aad[3] = aad[3] ^ (0x80 as u8);
        expect_eq(aead.decryptInPlaceDetached(nonce[0..12], aad[0..8], buf[0..8], tag[0..16]), false);
        aad[3] = aad[3] ^ (0x80 as u8);

        // Tamper Tag
        tag[15] = tag[15] ^ (0x01 as u8);
        expect_eq(aead.decryptInPlaceDetached(nonce[0..12], aad[0..8], buf[0..8], tag[0..16]), false);
        tag[15] = tag[15] ^ (0x01 as u8);

        // Valid again
        expect_eq(aead.decryptInPlaceDetached(nonce[0..12], aad[0..8], buf[0..8], tag[0..16]), true);
        expect_eq(buf[0], 1 as u8);
        expect_eq(buf[7], 8 as u8);
    });

    test("5. @cap/chacha20poly1305 — Empty plaintext (AAD-only) & empty AAD aligned lengths", () => {
        let key: [32]u8 = undefined;
        aeadFillRfcKey(key[0..32]);
        let nonce: [12]u8 = [9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2];
        let aad: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
        let empty_buf: [1]u8 = [0];

        let aead = ChaCha20Poly1305.new(key[0..32]).?;
        let tag: [16]u8 = undefined;
        expect_eq(aead.encryptInPlaceDetached(nonce[0..12], aad[0..16], empty_buf[0..0], tag[0..16]), true);
        expect_eq(aead.decryptInPlaceDetached(nonce[0..12], aad[0..16], empty_buf[0..0], tag[0..16]), true);

        // Empty AAD with 32-byte aligned plaintext
        let msg32: [32]u8 = undefined;
        let i: usize = 0;
        while (i < (32 as usize)) {
            msg32[i] = (i as u8) + (1 as u8);
            i += 1;
        }
        expect_eq(aead.encryptInPlaceDetached(nonce[0..12], empty_buf[0..0], msg32[0..32], tag[0..16]), true);
        expect_eq(aead.decryptInPlaceDetached(nonce[0..12], empty_buf[0..0], msg32[0..32], tag[0..16]), true);
        expect_eq(msg32[0], 1 as u8);
        expect_eq(msg32[31], 32 as u8);
    });

    test("6. @cap/chacha20poly1305 — Invalid key, nonce, and tag length bounds checks", () => {
        let short_key: [16]u8 = undefined;
        let key: [32]u8 = undefined;
        aeadFillRfcKey(key[0..32]);
        let short_nonce: [8]u8 = undefined;
        let nonce: [12]u8 = undefined;
        let short_tag: [8]u8 = undefined;
        let tag: [16]u8 = undefined;
        let buf: [4]u8 = [1, 2, 3, 4];

        expect_eq(ChaCha20Poly1305.new(short_key[0..16]) == null, true);

        let aead = ChaCha20Poly1305.new(key[0..32]).?;
        expect_eq(aead.encryptInPlaceDetached(short_nonce[0..8], buf[0..0], buf[0..4], tag[0..16]), false);
        expect_eq(aead.encryptInPlaceDetached(nonce[0..12], buf[0..0], buf[0..4], short_tag[0..8]), false);
        expect_eq(aead.decryptInPlaceDetached(short_nonce[0..8], buf[0..0], buf[0..4], tag[0..16]), false);
        expect_eq(aead.decryptInPlaceDetached(nonce[0..12], buf[0..0], buf[0..4], short_tag[0..8]), false);
    });
}
