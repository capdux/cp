// cap/test/testCapPbkdf2Sha256.cp
// Comprehensive unit tests for @cap/pbkdf2-sha256 (RFC 7914 / RFC 6070 SHA-256 vectors)

import { test, expect_eq } from "cap:test";
import {
    pbkdf2HmacSha256,
    pbkdf2Verify
} from "../../packages/pbkdf2/src/index.cp";

fn bytesEq(a: []u8, b: []u8): bool {
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

fn main() {
    test("1. @cap/pbkdf2-sha256 — Official vector c=1 (password/salt, dkLen=32)", () => {
        let pw: [8]u8 = ['p', 'a', 's', 's', 'w', 'o', 'r', 'd'];
        let salt: [4]u8 = ['s', 'a', 'l', 't'];
        let dk: [32]u8 = undefined;

        expect_eq(pbkdf2HmacSha256(pw[0..8], salt[0..4], 1, dk[0..32]), true);
        let exp: [32]u8 = [
            0x12, 0x0f, 0xb6, 0xcf, 0xfc, 0xf8, 0xb3, 0x2c,
            0x43, 0xe7, 0x22, 0x52, 0x56, 0xc4, 0xf8, 0x37,
            0xa8, 0x65, 0x48, 0xc9, 0x2c, 0xcc, 0x35, 0x48,
            0x08, 0x05, 0x98, 0x7c, 0xb7, 0x0b, 0xe1, 0x7b
        ];
        expect_eq(bytesEq(dk[0..32], exp[0..32]), true);
    });

    test("2. @cap/pbkdf2-sha256 — Official vector c=2 (password/salt, dkLen=32)", () => {
        let pw: [8]u8 = ['p', 'a', 's', 's', 'w', 'o', 'r', 'd'];
        let salt: [4]u8 = ['s', 'a', 'l', 't'];
        let dk: [32]u8 = undefined;

        expect_eq(pbkdf2HmacSha256(pw[0..8], salt[0..4], 2, dk[0..32]), true);
        let exp: [32]u8 = [
            0xae, 0x4d, 0x0c, 0x95, 0xaf, 0x6b, 0x46, 0xd3,
            0x2d, 0x0a, 0xdf, 0xf9, 0x28, 0xf0, 0x6d, 0xd0,
            0x2a, 0x30, 0x3f, 0x8e, 0xf3, 0xc2, 0x51, 0xdf,
            0xd6, 0xe2, 0xd8, 0x5a, 0x95, 0x47, 0x4c, 0x43
        ];
        expect_eq(bytesEq(dk[0..32], exp[0..32]), true);
    });

    test("3. @cap/pbkdf2-sha256 — Official vector c=4096 (password/salt, dkLen=32)", () => {
        let pw: [8]u8 = ['p', 'a', 's', 's', 'w', 'o', 'r', 'd'];
        let salt: [4]u8 = ['s', 'a', 'l', 't'];
        let dk: [32]u8 = undefined;

        expect_eq(pbkdf2HmacSha256(pw[0..8], salt[0..4], 4096, dk[0..32]), true);
        let exp: [32]u8 = [
            0xc5, 0xe4, 0x78, 0xd5, 0x92, 0x88, 0xc8, 0x41,
            0xaa, 0x53, 0x0d, 0xb6, 0x84, 0x5c, 0x4c, 0x8d,
            0x96, 0x28, 0x93, 0xa0, 0x01, 0xce, 0x4e, 0x11,
            0xa4, 0x96, 0x38, 0x73, 0xaa, 0x98, 0x13, 0x4a
        ];
        expect_eq(bytesEq(dk[0..32], exp[0..32]), true);
    });

    test("4. @cap/pbkdf2-sha256 — Multi-block 40-byte dkLen > 32 official vector (c=4096)", () => {
        let pw_str = "passwordPASSWORDpassword";
        let salt_str = "saltSALTsaltSALTsaltSALTsaltSALTsalt";
        let pw: [24]u8 = undefined;
        let salt: [36]u8 = undefined;
        let i: usize = 0;
        while (i < 24) {
            pw[i] = pw_str[i];
            i = i + 1;
        }
        i = 0;
        while (i < 36) {
            salt[i] = salt_str[i];
            i = i + 1;
        }

        let dk: [40]u8 = undefined;
        expect_eq(pbkdf2HmacSha256(pw[0..24], salt[0..36], 4096, dk[0..40]), true);
        let exp: [40]u8 = [
            0x34, 0x8c, 0x89, 0xdb, 0xcb, 0xd3, 0x2b, 0x2f,
            0x32, 0xd8, 0x14, 0xb8, 0x11, 0x6e, 0x84, 0xcf,
            0x2b, 0x17, 0x34, 0x7e, 0xbc, 0x18, 0x00, 0x18,
            0x1c, 0x4e, 0x2a, 0x1f, 0xb8, 0xdd, 0x53, 0xe1,
            0xc6, 0x35, 0x51, 0x8c, 0x7d, 0xac, 0x47, 0xe9
        ];
        expect_eq(bytesEq(dk[0..40], exp[0..40]), true);
    });

    test("5. @cap/pbkdf2-sha256 — pbkdf2Verify accepts valid key & rejects single-bit tamper", () => {
        let pw: [8]u8 = ['p', 'a', 's', 's', 'w', 'o', 'r', 'd'];
        let salt: [4]u8 = ['s', 'a', 'l', 't'];
        let dk: [32]u8 = undefined;

        expect_eq(pbkdf2HmacSha256(pw[0..8], salt[0..4], 10, dk[0..32]), true);
        expect_eq(pbkdf2Verify(pw[0..8], salt[0..4], 10, dk[0..32]), true);

        // Flip 1 bit in expected_dk -> must fail
        dk[31] = dk[31] ^ 0x01;
        expect_eq(pbkdf2Verify(pw[0..8], salt[0..4], 10, dk[0..32]), false);

        // Wrong password -> must fail
        dk[31] = dk[31] ^ 0x01;
        pw[0] = 'P' as u8;
        expect_eq(pbkdf2Verify(pw[0..8], salt[0..4], 10, dk[0..32]), false);
    });

    test("6. @cap/pbkdf2-sha256 — iterations=0 & empty output boundary safety", () => {
        let pw: [4]u8 = ['t', 'e', 's', 't'];
        let salt: [4]u8 = ['s', 'a', 'l', 't'];
        let dk: [32]u8 = undefined;

        expect_eq(pbkdf2HmacSha256(pw[0..4], salt[0..4], 0, dk[0..32]), false);
        expect_eq(pbkdf2Verify(pw[0..4], salt[0..4], 0, dk[0..32]), false);

        // Empty output dkLen == 0 succeeds for derive, returns false for verify
        expect_eq(pbkdf2HmacSha256(pw[0..4], salt[0..4], 1, dk[0..0]), true);
        expect_eq(pbkdf2Verify(pw[0..4], salt[0..4], 1, dk[0..0]), false);
    });
}
