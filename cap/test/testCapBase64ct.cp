// cap/test/testCapBase64ct.cp
// Comprehensive unit tests for @cap/base64ct

import { test, expect_eq } from "cap:test";
import {
    DecodeResult,
    encodedLen,
    encodeStd,
    encodeStdUnpadded,
    encodeUrl,
    encodeUrlUnpadded,
    decodeStd,
    decodeUrlUnpadded
} from "../../packages/base64ct/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
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

fn copyStr(s: string, dst: []u8): usize {
    let i: usize = 0;
    while (i < s.len) {
        dst[i] = s[i];
        i = i + 1;
    }
    return s.len;
}

fn main() {
    test("1. @cap/base64ct — RFC 4648 Section 10 official test vectors", () => {
        let src: [16]u8 = undefined;
        let enc: [32]u8 = undefined;
        let out_dec: [16]u8 = undefined;

        // "" -> ""
        expect_eq(encodedLen(0, true), 0 as usize);
        expect_eq(encodeStd(src[0..0], enc[0..32]), 0 as usize);
        let d0 = decodeStd("", out_dec[0..16]);
        expect_eq(d0.ok, true);
        expect_eq(d0.len, 0 as usize);

        // "f" -> "Zg=="
        let n1 = copyStr("f", src[0..16]);
        let e1 = encodeStd(src[0..n1], enc[0..32]);
        expect_eq(sliceEq(enc[0..e1], "Zg=="), true);
        let d1 = decodeStd("Zg==", out_dec[0..16]);
        expect_eq(d1.ok, true);
        expect_eq(sliceEq(out_dec[0..d1.len], "f"), true);

        // "fo" -> "Zm8="
        let n2 = copyStr("fo", src[0..16]);
        let e2 = encodeStd(src[0..n2], enc[0..32]);
        expect_eq(sliceEq(enc[0..e2], "Zm8="), true);
        let d2 = decodeStd("Zm8=", out_dec[0..16]);
        expect_eq(d2.ok, true);
        expect_eq(sliceEq(out_dec[0..d2.len], "fo"), true);

        // "foo" -> "Zm9v"
        let n3 = copyStr("foo", src[0..16]);
        let e3 = encodeStd(src[0..n3], enc[0..32]);
        expect_eq(sliceEq(enc[0..e3], "Zm9v"), true);
        let d3 = decodeStd("Zm9v", out_dec[0..16]);
        expect_eq(d3.ok, true);
        expect_eq(sliceEq(out_dec[0..d3.len], "foo"), true);

        // "foob" -> "Zm9vYg=="
        let n4 = copyStr("foob", src[0..16]);
        let e4 = encodeStd(src[0..n4], enc[0..32]);
        expect_eq(sliceEq(enc[0..e4], "Zm9vYg=="), true);
        let d4 = decodeStd("Zm9vYg==", out_dec[0..16]);
        expect_eq(d4.ok, true);
        expect_eq(sliceEq(out_dec[0..d4.len], "foob"), true);

        // "fooba" -> "Zm9vYmE="
        let n5 = copyStr("fooba", src[0..16]);
        let e5 = encodeStd(src[0..n5], enc[0..32]);
        expect_eq(sliceEq(enc[0..e5], "Zm9vYmE="), true);
        let d5 = decodeStd("Zm9vYmE=", out_dec[0..16]);
        expect_eq(d5.ok, true);
        expect_eq(sliceEq(out_dec[0..d5.len], "fooba"), true);

        // "foobar" -> "Zm9vYmFy"
        let n6 = copyStr("foobar", src[0..16]);
        let e6 = encodeStd(src[0..n6], enc[0..32]);
        expect_eq(sliceEq(enc[0..e6], "Zm9vYmFy"), true);
        let d6 = decodeStd("Zm9vYmFy", out_dec[0..16]);
        expect_eq(d6.ok, true);
        expect_eq(sliceEq(out_dec[0..d6.len], "foobar"), true);

        // Unpadded standard encoding
        let eu = encodeStdUnpadded(src[0..n4], enc[0..32]);
        expect_eq(sliceEq(enc[0..eu], "Zm9vYg"), true);
    });

    test("2. @cap/base64ct — Full 64-symbol alphabet round-trip (all 6-bit values 0..63)", () => {
        // 48 bytes pack all 64 6-bit values 0..63 in order (16 groups of 3 bytes = 64 symbols)
        let raw: [48]u8 = undefined;
        let g: usize = 0;
        while (g < 16) {
            let s0: u8 = (g * 4) as u8;
            let s1: u8 = (g * 4 + 1) as u8;
            let s2: u8 = (g * 4 + 2) as u8;
            let s3: u8 = (g * 4 + 3) as u8;
            raw[g * 3] = (s0 << 2) | (s1 >> 4);
            raw[g * 3 + 1] = ((s1 & 0x0F) << 4) | (s2 >> 2);
            raw[g * 3 + 2] = ((s2 & 0x03) << 6) | s3;
            g = g + 1;
        }

        let enc: [64]u8 = undefined;
        let elen = encodeStd(raw[0..48], enc[0..64]);
        expect_eq(elen, 64 as usize);
        expect_eq(
            sliceEq(enc[0..64], "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"),
            true
        );

        let out_dec: [48]u8 = undefined;
        let dr = decodeStd(enc[0..64], out_dec[0..48]);
        expect_eq(dr.ok, true);
        expect_eq(dr.len, 48 as usize);
        expect_eq(sliceEq(out_dec[0..48], raw[0..48]), true);
    });

    test("3. @cap/base64ct — URL-Safe (- and _) padded & unpadded (JWT) round-trip", () => {
        // Bytes 0xFB, 0xEF, 0xFF encode to "++//" in Std and "--__" in URL-Safe
        let raw: [4]u8 = [0xFB, 0xEF, 0xFF, 0xFC];
        let enc_url: [16]u8 = undefined;
        let enc_unpad: [16]u8 = undefined;

        let n_pad = encodeUrl(raw[0..4], enc_url[0..16]);
        expect_eq(n_pad, 8 as usize);
        expect_eq(sliceEq(enc_url[0..n_pad], "--___A=="), true);

        let n_unpad = encodeUrlUnpadded(raw[0..4], enc_unpad[0..16]);
        expect_eq(n_unpad, 6 as usize);
        expect_eq(sliceEq(enc_unpad[0..n_unpad], "--___A"), true);

        let out_dec: [8]u8 = undefined;
        let dr = decodeUrlUnpadded(enc_unpad[0..n_unpad], out_dec[0..8]);
        expect_eq(dr.ok, true);
        expect_eq(dr.len, 4 as usize);
        expect_eq(sliceEq(out_dec[0..dr.len], raw[0..4]), true);

        // Reject '+' or '/' or '=' in URL-Safe unpadded decoder
        expect_eq(decodeUrlUnpadded("++//", out_dec[0..8]).ok, false);
        expect_eq(decodeUrlUnpadded("--___A==", out_dec[0..8]).ok, false);
    });

    test("4. @cap/base64ct — Reject illegal characters & malformed padding", () => {
        let out_dec: [16]u8 = undefined;

        // Invalid length (not multiple of 4 for std, rem == 1 for unpadded)
        expect_eq(decodeStd("Zg=", out_dec[0..16]).ok, false);
        expect_eq(decodeStd("A", out_dec[0..16]).ok, false);
        expect_eq(decodeUrlUnpadded("A", out_dec[0..16]).ok, false);

        // Illegal characters (whitespace, symbols, URL chars in std)
        expect_eq(decodeStd("Zm9!", out_dec[0..16]).ok, false);
        expect_eq(decodeStd("Zm 9", out_dec[0..16]).ok, false);
        expect_eq(decodeStd("--__", out_dec[0..16]).ok, false);

        // Misplaced or excessive padding
        expect_eq(decodeStd("====", out_dec[0..16]).ok, false);
        expect_eq(decodeStd("A===", out_dec[0..16]).ok, false);
        expect_eq(decodeStd("Zg==Zm9v", out_dec[0..16]).ok, false);
        expect_eq(decodeStd("=m9v", out_dec[0..16]).ok, false);
    });

    test("5. @cap/base64ct — Strict RFC 4648 Section 3.5 non-zero trailing bits rejection", () => {
        let out_dec: [16]u8 = undefined;

        // "Zg==" is canonical ('g' = 32 = 0b100000, low 4 bits = 0000)
        // "Zh==" has 'h' = 33 = 0b100001 (low 4 bits != 0) -> MUST reject!
        expect_eq(decodeStd("Zg==", out_dec[0..16]).ok, true);
        expect_eq(decodeStd("Zh==", out_dec[0..16]).ok, false);
        expect_eq(decodeUrlUnpadded("Zg", out_dec[0..16]).ok, true);
        expect_eq(decodeUrlUnpadded("Zh", out_dec[0..16]).ok, false);

        // "Zm8=" is canonical ('8' = 60 = 0b111100, low 2 bits = 00)
        // "Zm9=" has '9' = 61 = 0b111101 (low 2 bits != 0) -> MUST reject!
        expect_eq(decodeStd("Zm8=", out_dec[0..16]).ok, true);
        expect_eq(decodeStd("Zm9=", out_dec[0..16]).ok, false);
        expect_eq(decodeUrlUnpadded("Zm8", out_dec[0..16]).ok, true);
        expect_eq(decodeUrlUnpadded("Zm9", out_dec[0..16]).ok, false);
    });

    test("6. @cap/base64ct — Small buffer boundary safety & encodedLen", () => {
        expect_eq(encodedLen(1, true), 4 as usize);
        expect_eq(encodedLen(1, false), 2 as usize);
        expect_eq(encodedLen(2, true), 4 as usize);
        expect_eq(encodedLen(2, false), 3 as usize);
        expect_eq(encodedLen(3, true), 4 as usize);
        expect_eq(encodedLen(3, false), 4 as usize);

        let raw: [3]u8 = ['f', 'o', 'o'];
        let tiny_enc: [3]u8 = undefined;
        // Needs 4 bytes -> returns 0 safely
        expect_eq(encodeStd(raw[0..3], tiny_enc[0..3]), 0 as usize);
        expect_eq(encodeUrlUnpadded(raw[0..3], tiny_enc[0..3]), 0 as usize);

        let tiny_dec: [2]u8 = undefined;
        // "Zm9v" decodes to 3 bytes -> buffer of 2 returns ok=false
        let dr1 = decodeStd("Zm9v", tiny_dec[0..2]);
        expect_eq(dr1.ok, false);
        expect_eq(dr1.len, 0 as usize);

        let dr2 = decodeUrlUnpadded("Zm9v", tiny_dec[0..2]);
        expect_eq(dr2.ok, false);
        expect_eq(dr2.len, 0 as usize);
    });
}
