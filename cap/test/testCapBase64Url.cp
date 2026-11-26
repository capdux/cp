import { test, expect_eq } from "cap:test";
import {
    encodeBase64Url, decodeBase64Url, encodedBase64UrlLen,
    fromStandardBase64, toStandardBase64, isValidBase64Url
} from "../../packages/@cap/base64-url/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/base64-url — JWT header encode & decode round-trip", () => {
        let hdr = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
        let enc: [64]u8 = undefined;
        let en = encodeBase64Url(hdr, enc[0..64]);
        expect_eq(en, encodedBase64UrlLen(hdr.len));
        expect_eq(eqBuf(enc[0..en], en, "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9"), true);
        let dec_buf: [64]u8 = undefined;
        let r = decodeBase64Url(enc[0..en], dec_buf[0..64]);
        expect_eq(r.ok, true);
        expect_eq(eqBuf(dec_buf[0..r.len], r.len, hdr), true);
    });

    test("2. @cap/base64-url — URL-safe alphabet uses '-' and '_' without '='", () => {
        let raw: [3]u8 = [0xfb, 0xff, 0xfe];
        let enc: [16]u8 = undefined;
        let en = encodeBase64Url(raw[0..3], enc[0..16]);
        expect_eq(eqBuf(enc[0..en], en, "-__-"), true);
        expect_eq(isValidBase64Url("-__-"), true);
    });

    test("3. @cap/base64-url — fromStandardBase64 converts +/= to -_", () => {
        let out: [32]u8 = undefined;
        let n = fromStandardBase64("ab+c/d==", out[0..32]);
        expect_eq(eqBuf(out[0..n], n, "ab-c_d"), true);
    });

    test("4. @cap/base64-url — toStandardBase64 restores +/ and '=' padding", () => {
        let out: [32]u8 = undefined;
        let n = toStandardBase64("ab-c_d", out[0..32]);
        expect_eq(eqBuf(out[0..n], n, "ab+c/d=="), true);
        let n2 = toStandardBase64("abcd", out[0..32]);
        expect_eq(eqBuf(out[0..n2], n2, "abcd"), true);
    });

    test("5. @cap/base64-url — isValidBase64Url rejects +, /, = and len%4==1", () => {
        expect_eq(isValidBase64Url(""), true);
        expect_eq(isValidBase64Url("Zg"), true);
        expect_eq(isValidBase64Url("a"), false);
        expect_eq(isValidBase64Url("ab+c"), false);
        expect_eq(isValidBase64Url("ab/c"), false);
        expect_eq(isValidBase64Url("Zg=="), false);
    });

    test("6. @cap/base64-url — Empty input & invalid decode rejection", () => {
        let out: [16]u8 = undefined;
        expect_eq(encodeBase64Url("", out[0..16]), 0);
        let r = decodeBase64Url("@@@", out[0..16]);
        expect_eq(r.ok, false);
    });
}
