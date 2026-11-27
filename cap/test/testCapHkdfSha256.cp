// cap/test/testCapHkdfSha256.cp
// Comprehensive unit tests for @cap/hkdf-sha256 (RFC 5869 Appendix A.1, A.2, A.3)

import { test, expect_eq } from "cap:test";
import {
    HKDF_HASH_LEN,
    HKDF_MAX_OKM_LEN,
    hkdfExtract,
    hkdfExpand,
    hkdfDerive
} from "../../packages/futoin-hkdf/src/index.cp";

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
    test("1. @cap/hkdf-sha256 — RFC 5869 Appendix A.1 (Basic 42-byte OKM vector)", () => {
        let ikm: [22]u8 = undefined;
        let i: usize = 0;
        while (i < 22) {
            ikm[i] = 0x0b;
            i = i + 1;
        }
        let salt: [13]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c];
        let info: [10]u8 = [0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9];

        let prk: [32]u8 = undefined;
        let okm: [42]u8 = undefined;
        expect_eq(hkdfExtract(salt[0..13], ikm[0..22], prk[0..32]), true);
        expect_eq(hkdfExpand(prk[0..32], info[0..10], okm[0..42]), true);

        let exp_prk: [32]u8 = [
            0x07, 0x77, 0x09, 0x36, 0x2c, 0x2e, 0x32, 0xdf,
            0x0d, 0xdc, 0x3f, 0x0d, 0xc4, 0x7b, 0xba, 0x63,
            0x90, 0xb6, 0xc7, 0x3b, 0xb5, 0x0f, 0x9c, 0x31,
            0x22, 0xec, 0x84, 0x4a, 0xd7, 0xc2, 0xb3, 0xe5
        ];
        let exp_okm: [42]u8 = [
            0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a,
            0x90, 0x43, 0x4f, 0x64, 0xd0, 0x36, 0x2f, 0x2a,
            0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a, 0x5a, 0x4c,
            0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf,
            0x34, 0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18,
            0x58, 0x65
        ];
        expect_eq(bytesEq(prk[0..32], exp_prk[0..32]), true);
        expect_eq(bytesEq(okm[0..42], exp_okm[0..42]), true);
    });

    test("2. @cap/hkdf-sha256 — RFC 5869 Appendix A.2 (Long 80B inputs & 82-byte OKM vector)", () => {
        let ikm: [80]u8 = undefined;
        let salt: [80]u8 = undefined;
        let info: [80]u8 = undefined;
        let i: usize = 0;
        while (i < 80) {
            ikm[i] = i as u8;
            salt[i] = (0x60 + i) as u8;
            info[i] = (0xb0 + i) as u8;
            i = i + 1;
        }

        let prk: [32]u8 = undefined;
        let okm: [82]u8 = undefined;
        expect_eq(hkdfExtract(salt[0..80], ikm[0..80], prk[0..32]), true);
        expect_eq(hkdfExpand(prk[0..32], info[0..80], okm[0..82]), true);

        let exp_prk: [32]u8 = [
            0x06, 0xa6, 0xb8, 0x8c, 0x58, 0x53, 0x36, 0x1a,
            0x06, 0x10, 0x4c, 0x9c, 0xeb, 0x35, 0xb4, 0x5c,
            0xef, 0x76, 0x00, 0x14, 0x90, 0x46, 0x71, 0x01,
            0x4a, 0x19, 0x3f, 0x40, 0xc1, 0x5f, 0xc2, 0x44
        ];
        let exp_okm: [82]u8 = [
            0xb1, 0x1e, 0x39, 0x8d, 0xc8, 0x03, 0x27, 0xa1,
            0xc8, 0xe7, 0xf7, 0x8c, 0x59, 0x6a, 0x49, 0x34,
            0x4f, 0x01, 0x2e, 0xda, 0x2d, 0x4e, 0xfa, 0xd8,
            0xa0, 0x50, 0xcc, 0x4c, 0x19, 0xaf, 0xa9, 0x7c,
            0x59, 0x04, 0x5a, 0x99, 0xca, 0xc7, 0x82, 0x72,
            0x71, 0xcb, 0x41, 0xc6, 0x5e, 0x59, 0x0e, 0x09,
            0xda, 0x32, 0x75, 0x60, 0x0c, 0x2f, 0x09, 0xb8,
            0x36, 0x77, 0x93, 0xa9, 0xac, 0xa3, 0xdb, 0x71,
            0xcc, 0x30, 0xc5, 0x81, 0x79, 0xec, 0x3e, 0x87,
            0xc1, 0x4c, 0x01, 0xd5, 0xc1, 0xf3, 0x43, 0x4f,
            0x1d, 0x87
        ];
        expect_eq(bytesEq(prk[0..32], exp_prk[0..32]), true);
        expect_eq(bytesEq(okm[0..82], exp_okm[0..82]), true);
    });

    test("3. @cap/hkdf-sha256 — RFC 5869 Appendix A.3 (Zero-length salt & info vector)", () => {
        let ikm: [22]u8 = undefined;
        let i: usize = 0;
        while (i < 22) {
            ikm[i] = 0x0b;
            i = i + 1;
        }
        let empty: [1]u8 = [0];
        let prk: [32]u8 = undefined;
        let okm: [42]u8 = undefined;

        expect_eq(hkdfExtract(empty[0..0], ikm[0..22], prk[0..32]), true);
        expect_eq(hkdfExpand(prk[0..32], empty[0..0], okm[0..42]), true);

        let exp_prk: [32]u8 = [
            0x19, 0xef, 0x24, 0xa3, 0x2c, 0x71, 0x7b, 0x16,
            0x7f, 0x33, 0xa9, 0x1d, 0x6f, 0x64, 0x8b, 0xdf,
            0x96, 0x59, 0x67, 0x76, 0xaf, 0xdb, 0x63, 0x77,
            0xac, 0x43, 0x4c, 0x1c, 0x29, 0x3c, 0xcb, 0x04
        ];
        let exp_okm: [42]u8 = [
            0x8d, 0xa4, 0xe7, 0x75, 0xa5, 0x63, 0xc1, 0x8f,
            0x71, 0x5f, 0x80, 0x2a, 0x06, 0x3c, 0x5a, 0x31,
            0xb8, 0xa1, 0x1f, 0x5c, 0x5e, 0xe1, 0x87, 0x9e,
            0xc3, 0x45, 0x4e, 0x5f, 0x3c, 0x73, 0x8d, 0x2d,
            0x9d, 0x20, 0x13, 0x95, 0xfa, 0xa4, 0xb6, 0x1a,
            0x96, 0xc8
        ];
        expect_eq(bytesEq(prk[0..32], exp_prk[0..32]), true);
        expect_eq(bytesEq(okm[0..42], exp_okm[0..42]), true);
    });

    test("4. @cap/hkdf-sha256 — hkdfDerive one-shot equals Extract + Expand", () => {
        let ikm: [22]u8 = undefined;
        let i: usize = 0;
        while (i < 22) {
            ikm[i] = 0x0b;
            i = i + 1;
        }
        let salt: [13]u8 = [0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c];
        let info: [10]u8 = [0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9];

        let prk: [32]u8 = undefined;
        let okm1: [42]u8 = undefined;
        let okm2: [42]u8 = undefined;

        expect_eq(hkdfExtract(salt[0..13], ikm[0..22], prk[0..32]), true);
        expect_eq(hkdfExpand(prk[0..32], info[0..10], okm1[0..42]), true);
        expect_eq(hkdfDerive(salt[0..13], ikm[0..22], info[0..10], okm2[0..42]), true);
        expect_eq(bytesEq(okm1[0..42], okm2[0..42]), true);
    });

    test("5. @cap/hkdf-sha256 — Reject L > 8160 (255 * 32) overflow", () => {
        let prk: [32]u8 = undefined;
        let info: [4]u8 = ['t', 'e', 's', 't'];
        let i: usize = 0;
        while (i < 32) {
            prk[i] = 0x42;
            i = i + 1;
        }
        expect_eq(HKDF_HASH_LEN, 32 as usize);
        expect_eq(HKDF_MAX_OKM_LEN, 8160 as usize);

        let too_big: [8161]u8 = undefined;
        expect_eq(hkdfExpand(prk[0..32], info[0..4], too_big[0..8161]), false);
        expect_eq(hkdfDerive(info[0..4], prk[0..32], info[0..4], too_big[0..8161]), false);
    });

    test("6. @cap/hkdf-sha256 — Empty OKM & invalid PRK/output buffer length boundaries", () => {
        let prk: [32]u8 = undefined;
        let info: [4]u8 = ['i', 'n', 'f', 'o'];
        let i: usize = 0;
        while (i < 32) {
            prk[i] = 0x11;
            i = i + 1;
        }

        // L == 0 succeeds cleanly
        let okm: [1]u8 = [0];
        expect_eq(hkdfExpand(prk[0..32], info[0..4], okm[0..0]), true);
        expect_eq(hkdfDerive(info[0..4], prk[0..32], info[0..4], okm[0..0]), true);

        // PRK shorter than 32 bytes fails safely
        expect_eq(hkdfExpand(prk[0..31], info[0..4], okm[0..1]), false);
        // Extract output shorter than 32 bytes fails safely
        expect_eq(hkdfExtract(info[0..4], prk[0..32], prk[0..16]), false);
    });
}
