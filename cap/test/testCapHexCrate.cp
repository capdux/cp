import { test, expect_eq } from "cap:test";
import {
    encodeHex, encodeHexUpper, decodeHex, decodeHexPrefixed,
    hexEncodedLen, hexDecodedLen, isValidHex
} from "../../packages/@cap/hex-crate/src/index.cp";

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
    test("1. @cap/hex-crate — encodeHex & encodeHexUpper", () => {
        let out: [32]u8 = undefined;
        let n1 = encodeHex("Hello", out[0..32]);
        expect_eq(n1, hexEncodedLen(5));
        expect_eq(eqBuf(out[0..n1], n1, "48656c6c6f"), true);
        let n2 = encodeHexUpper("Hello", out[0..32]);
        expect_eq(eqBuf(out[0..n2], n2, "48656C6C6F"), true);
    });

    test("2. @cap/hex-crate — decodeHex round-trip mixed case", () => {
        let out: [32]u8 = undefined;
        let r = decodeHex("48656C6c6F", out[0..32]);
        expect_eq(r.ok, true);
        expect_eq(r.len, 5);
        expect_eq(eqBuf(out[0..r.len], r.len, "Hello"), true);
    });

    test("3. @cap/hex-crate — decodeHexPrefixed handles 0x and 0X", () => {
        let out: [16]u8 = undefined;
        let r1 = decodeHexPrefixed("0xdeadbeef", out[0..16]);
        expect_eq(r1.ok, true);
        expect_eq(r1.len, 4);
        expect_eq(out[0], 0xde);
        expect_eq(out[3], 0xef);
        let r2 = decodeHexPrefixed("0XCAFE", out[0..16]);
        expect_eq(r2.ok, true);
        expect_eq(out[0], 0xca);
        expect_eq(out[1], 0xfe);
    });

    test("4. @cap/hex-crate — isValidHex & hexDecodedLen", () => {
        expect_eq(isValidHex("0123456789abcdefABCDEF"), true);
        expect_eq(isValidHex(""), true);
        expect_eq(isValidHex("abc"), false);
        expect_eq(isValidHex("zzzz"), false);
        expect_eq(hexDecodedLen(10) ?? 99, 5);
        expect_eq(hexDecodedLen(9) == null, true);
    });

    test("5. @cap/hex-crate — Rejects odd length & invalid hex chars", () => {
        let out: [16]u8 = undefined;
        expect_eq(decodeHex("abc", out[0..16]).ok, false);
        expect_eq(decodeHex("0g", out[0..16]).ok, false);
    });

    test("6. @cap/hex-crate — Empty input round-trip", () => {
        let out: [8]u8 = undefined;
        expect_eq(encodeHex("", out[0..8]), 0);
        let r = decodeHex("", out[0..8]);
        expect_eq(r.ok, true);
        expect_eq(r.len, 0);
    });
}
