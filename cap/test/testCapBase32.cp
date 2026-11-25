// cap/test/testCapBase32.cp
// RFC 4648 §10 official standard & extended hex test vectors, Crockford tolerance,
// z-base-32 round-trip, unpadded round-trip, and strict error-handling tests.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    Base32Error,
    Base32Alphabet,
    encodedLen,
    decodedMaxLen,
    encode,
    decode,
    encodeStd,
    decodeStd,
    encodeStdNoPad,
    encodeHex,
    decodeHex,
    encodeCrockford,
    decodeCrockford,
} from "../../packages/@cap/base32/src/index.cp";

fn expectDecodeError(input: []const u8, alphabet: Base32Alphabet, expected: Base32Error): bool {
    let out: [64]u8 = [0; 64];
    * = decode(input, alphabet, out[0..]) catch |e| {
        return e == expected;
    };
    return false;
}

fn expectEncodeBufferTooSmall(input: []const u8, padded: bool, cap_len: usize): bool {
    let out: [64]u8 = [0; 64];
    * = encode(input, Base32Alphabet.Rfc4648, padded, out[0..cap_len]) catch |e| {
        return e == Base32Error.BufferTooSmall;
    };
    return false;
}

fn expectDecodeBufferTooSmall(input: []const u8, cap_len: usize): bool {
    let out: [64]u8 = [0; 64];
    * = decodeStd(input, out[0..cap_len]) catch |e| {
        return e == Base32Error.BufferTooSmall;
    };
    return false;
}

fn main() {
    test("1. @cap/base32 — RFC 4648 §10 Standard Test Vectors", () => {
        let enc: [64]u8 = [0; 64];
        let out: [64]u8 = [0; 64];

        const e0 = encodeStd("", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e0, ""), true);
        expect_eq(std.mem.eql(u8, decodeStd(e0, out[0..]) catch unreachable, ""), true);

        const e1 = encodeStd("f", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e1, "MY======"), true);
        expect_eq(std.mem.eql(u8, decodeStd(e1, out[0..]) catch unreachable, "f"), true);

        const e2 = encodeStd("fo", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e2, "MZXQ===="), true);
        expect_eq(std.mem.eql(u8, decodeStd(e2, out[0..]) catch unreachable, "fo"), true);

        const e3 = encodeStd("foo", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e3, "MZXW6==="), true);
        expect_eq(std.mem.eql(u8, decodeStd(e3, out[0..]) catch unreachable, "foo"), true);

        const e4 = encodeStd("foob", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e4, "MZXW6YQ="), true);
        expect_eq(std.mem.eql(u8, decodeStd(e4, out[0..]) catch unreachable, "foob"), true);

        const e5 = encodeStd("fooba", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e5, "MZXW6YTB"), true);
        expect_eq(std.mem.eql(u8, decodeStd(e5, out[0..]) catch unreachable, "fooba"), true);

        const e6 = encodeStd("foobar", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e6, "MZXW6YTBOI======"), true);
        expect_eq(std.mem.eql(u8, decodeStd(e6, out[0..]) catch unreachable, "foobar"), true);

        // Case-insensitive decoding
        expect_eq(std.mem.eql(u8, decodeStd("mzxw6ytboi======", out[0..]) catch unreachable, "foobar"), true);
    });

    test("2. @cap/base32 — RFC 4648 §10 Extended Hex Test Vectors", () => {
        let enc: [64]u8 = [0; 64];
        let out: [64]u8 = [0; 64];

        const e0 = encodeHex("", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e0, ""), true);
        expect_eq(std.mem.eql(u8, decodeHex(e0, out[0..]) catch unreachable, ""), true);

        const e1 = encodeHex("f", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e1, "CO======"), true);
        expect_eq(std.mem.eql(u8, decodeHex(e1, out[0..]) catch unreachable, "f"), true);

        const e2 = encodeHex("fo", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e2, "CPNG===="), true);
        expect_eq(std.mem.eql(u8, decodeHex(e2, out[0..]) catch unreachable, "fo"), true);

        const e3 = encodeHex("foo", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e3, "CPNMU==="), true);
        expect_eq(std.mem.eql(u8, decodeHex(e3, out[0..]) catch unreachable, "foo"), true);

        const e4 = encodeHex("foob", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e4, "CPNMUOG="), true);
        expect_eq(std.mem.eql(u8, decodeHex(e4, out[0..]) catch unreachable, "foob"), true);

        const e5 = encodeHex("fooba", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e5, "CPNMUOJ1"), true);
        expect_eq(std.mem.eql(u8, decodeHex(e5, out[0..]) catch unreachable, "fooba"), true);

        const e6 = encodeHex("foobar", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, e6, "CPNMUOJ1E8======"), true);
        expect_eq(std.mem.eql(u8, decodeHex(e6, out[0..]) catch unreachable, "foobar"), true);

        // Case-insensitive hex decoding
        expect_eq(std.mem.eql(u8, decodeHex("cpnmuoj1e8======", out[0..]) catch unreachable, "foobar"), true);
    });

    test("3. @cap/base32 — Unpadded Round-Trip, Length Helpers & TOTP Secret", () => {
        expect_eq(encodedLen(0, true), 0);
        expect_eq(encodedLen(0, false), 0);
        expect_eq(encodedLen(1, true), 8);
        expect_eq(encodedLen(1, false), 2);
        expect_eq(encodedLen(2, false), 4);
        expect_eq(encodedLen(3, false), 5);
        expect_eq(encodedLen(4, false), 7);
        expect_eq(encodedLen(5, false), 8);
        expect_eq(encodedLen(6, false), 10);
        expect_eq(decodedMaxLen(0), 0);
        expect_eq(decodedMaxLen(2), 1);
        expect_eq(decodedMaxLen(4), 2);
        expect_eq(decodedMaxLen(5), 3);
        expect_eq(decodedMaxLen(7), 4);
        expect_eq(decodedMaxLen(8), 5);
        expect_eq(decodedMaxLen(16), 10);

        let enc: [64]u8 = [0; 64];
        let out: [64]u8 = [0; 64];

        const np1 = encodeStdNoPad("f", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, np1, "MY"), true);
        expect_eq(std.mem.eql(u8, decodeStd(np1, out[0..]) catch unreachable, "f"), true);

        const np6 = encodeStdNoPad("foobar", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, np6, "MZXW6YTBOI"), true);
        expect_eq(std.mem.eql(u8, decodeStd(np6, out[0..]) catch unreachable, "foobar"), true);

        // RFC 6238 TOTP 20-byte ASCII test secret
        const totp = encodeStdNoPad("12345678901234567890", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, totp, "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"), true);
        expect_eq(std.mem.eql(u8, decodeStd(totp, out[0..]) catch unreachable, "12345678901234567890"), true);
    });

    test("4. @cap/base32 — Crockford Tolerance & z-base-32 Round-Trip", () => {
        let enc: [64]u8 = [0; 64];
        let out: [64]u8 = [0; 64];

        const cf = encodeCrockford("foobar", enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, cf, "CSQPYRK1E8"), true);
        expect_eq(std.mem.eql(u8, decodeCrockford(cf, out[0..]) catch unreachable, "foobar"), true);

        // Crockford tolerance: I/i/L/l -> 1, O/o -> 0, '-' ignored, case-insensitive
        const d_tol = decodeCrockford("csqp-yrki-e8", out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, d_tol, "foobar"), true);
        const d_tol2 = decodeCrockford("CSQP-YRKL-E8", out[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, d_tol2, "foobar"), true);

        // O/o -> 0 check: [0x00] -> "00" -> "Oo" / "o-O"
        const zero_byte: [1]u8 = [0x00];
        const cf_zero = encodeCrockford(zero_byte[0..], enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, cf_zero, "00"), true);
        const d_zero = decodeCrockford("o-O", out[0..]) catch unreachable;
        expect_eq(d_zero.len, 1);
        expect_eq(d_zero[0], 0x00);

        // z-base-32 round-trip (including uppercase input tolerance)
        const zb = encode("foobar", Base32Alphabet.ZBase32, false, enc[0..]) catch unreachable;
        expect_eq(std.mem.eql(u8, zb, "c3zs6aubqe"), true);
        expect_eq(std.mem.eql(u8, decode(zb, Base32Alphabet.ZBase32, out[0..]) catch unreachable, "foobar"), true);
        expect_eq(std.mem.eql(u8, decode("C3ZS6AUBQE", Base32Alphabet.ZBase32, out[0..]) catch unreachable, "foobar"), true);
    });

    test("5. @cap/base32 — Strict Error Handling (InvalidChar, Length, Padding, BufferTooSmall)", () => {
        // Invalid characters
        expect_eq(expectDecodeError("0189", Base32Alphabet.Rfc4648, Base32Error.InvalidCharacter), true);
        expect_eq(expectDecodeError("WXYZ", Base32Alphabet.Rfc4648Hex, Base32Error.InvalidCharacter), true);
        expect_eq(expectDecodeError("UU", Base32Alphabet.Crockford, Base32Error.InvalidCharacter), true);
        expect_eq(expectDecodeError("02lv", Base32Alphabet.ZBase32, Base32Error.InvalidCharacter), true);

        // Invalid lengths (rem == 1, 3, 6)
        expect_eq(expectDecodeError("M", Base32Alphabet.Rfc4648, Base32Error.InvalidLength), true);
        expect_eq(expectDecodeError("MZX", Base32Alphabet.Rfc4648, Base32Error.InvalidLength), true);
        expect_eq(expectDecodeError("MZXW6Y", Base32Alphabet.Rfc4648, Base32Error.InvalidLength), true);

        // Invalid padding counts & mid-stream padding
        expect_eq(expectDecodeError("MY=====", Base32Alphabet.Rfc4648, Base32Error.InvalidPadding), true);
        expect_eq(expectDecodeError("MY=A====", Base32Alphabet.Rfc4648, Base32Error.InvalidPadding), true);
        expect_eq(expectDecodeError("========", Base32Alphabet.Rfc4648, Base32Error.InvalidPadding), true);

        // Non-zero canonical trailing bits (RFC 4648 §3.5): 'Z' = 25 = 0b11001 -> low 2 bits = 01 != 0
        expect_eq(expectDecodeError("MZ======", Base32Alphabet.Rfc4648, Base32Error.InvalidPadding), true);
        expect_eq(expectDecodeError("MZ", Base32Alphabet.Rfc4648, Base32Error.InvalidPadding), true);

        // BufferTooSmall on encode & decode
        expect_eq(expectEncodeBufferTooSmall("foobar", true, 15), true);
        expect_eq(expectEncodeBufferTooSmall("foobar", false, 9), true);
        expect_eq(expectDecodeBufferTooSmall("MZXW6YTBOI======", 5), true);
        expect_eq(expectDecodeBufferTooSmall("MY======", 0), true);
    });
}
