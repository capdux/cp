// Source: bun/src/base64/base64.cp
// cap/test/test_encoding_complete.cp
import { test, expect_eq } from "cap:test";
import {
    Base64,
    Base64Encoder,
    Base64Decoder,
    standard,
    standard_no_pad,
    url_safe,
    url_safe_no_pad,
} from "cap:encoding";
const std = @import("std");

fn testDecodeInPlace(): !void {
    // 1. In-place decode "TWFu" -> "Man"
    let buf1: [4]u8 = undefined;
    @memcpy(&buf1, "TWFu");
    const len1 = try Base64.decodeInPlace(&buf1);
    expect_eq(len1, 3);
    expect_eq(std.mem.eql(u8, buf1[0..len1], "Man"), true);

    // 2. In-place decode with padding "TQ==" -> "M"
    let buf2: [4]u8 = undefined;
    @memcpy(&buf2, "TQ==");
    const len2 = try Base64.decodeInPlace(&buf2);
    expect_eq(len2, 1);
    expect_eq(std.mem.eql(u8, buf2[0..len2], "M"), true);

    // 3. In-place decode with single padding "TWE=" -> "Ma"
    let buf3: [4]u8 = undefined;
    @memcpy(&buf3, "TWE=");
    const len3 = try Base64.decodeInPlace(&buf3);
    expect_eq(len3, 2);
    expect_eq(std.mem.eql(u8, buf3[0..len3], "Ma"), true);

    // 4. In-place round trip for longer string
    const original = "Concurrent Plus (CP) High-Performance Base64";
    let encoded_buf: [128]u8 = undefined;
    const enc_len = Base64.encode(&encoded_buf, original);

    // decodeInPlace on the encoded slice
    const dec_len = try Base64.decodeInPlace(encoded_buf[0..enc_len]);
    expect_eq(dec_len, original.len);
    expect_eq(std.mem.eql(u8, encoded_buf[0..dec_len], original), true);
}

fn testLengthCalculations(): !void {
    // encodeLen standard
    expect_eq(Base64.encodeLen(0), 0);
    expect_eq(Base64.encodeLen(1), 4);
    expect_eq(Base64.encodeLen(2), 4);
    expect_eq(Base64.encodeLen(3), 4);
    expect_eq(Base64.encodeLen(4), 8);

    // encodeLenURLSafe (no padding)
    expect_eq(Base64.encodeLenURLSafe(0), 0);
    expect_eq(Base64.encodeLenURLSafe(1), 2);
    expect_eq(Base64.encodeLenURLSafe(2), 3);
    expect_eq(Base64.encodeLenURLSafe(3), 4);
    expect_eq(Base64.encodeLenURLSafe(4), 6);

    // exactDecodedLen
    expect_eq(Base64.exactDecodedLen("TWFu"), 3);
    expect_eq(Base64.exactDecodedLen("TWE="), 2);
    expect_eq(Base64.exactDecodedLen("TQ=="), 1);
}

fn testCodecsAndValidation(): !void {
    let out: [64]u8 = undefined;

    // standard encoder (padded)
    const n1 = standard.encoder.encode(&out, "hello");
    expect_eq(n1, 8);
    expect_eq(std.mem.eql(u8, out[0..n1], "aGVsbG8="), true);

    // standard_no_pad encoder
    const n2 = standard_no_pad.encoder.encode(&out, "hello");
    expect_eq(n2, 7);
    expect_eq(std.mem.eql(u8, out[0..n2], "aGVsbG8"), true);

    // url_safe_no_pad encoder
    const raw_bytes = "\xFB\xFF\xFE";
    const n3 = url_safe_no_pad.encoder.encode(&out, raw_bytes);
    expect_eq(std.mem.eql(u8, out[0..n3], "-__-"), true);

    // Validation
    expect_eq(Base64.isValid("aGVsbG8="), true);
    expect_eq(Base64.isValid("TWFu"), true);
    expect_eq(Base64.isValid("invalid!chars"), false);
    expect_eq(Base64.isValidURLSafe("-_--"), true);
    expect_eq(Base64.isValidURLSafe("-__-"), true);
}

fn testErrorHandling(): !void {
    let dec_buf: [32]u8 = undefined;

    // Invalid character
    let failed1 = false;
    * = Base64.decode(&dec_buf, "TQ*=") catch e => {
        failed1 = true;
    };
    expect_eq(failed1, true);

    // Invalid character in decodeInPlace
    let bad_buf: [4]u8 = undefined;
    @memcpy(&bad_buf, "TQ*=");
    let failed2 = false;
    * = Base64.decodeInPlace(&bad_buf) catch e => {
        failed2 = true;
    };
    expect_eq(failed2, true);
}

fn main() {
    test("Base64 in-place decoding decodeInPlace", () => {
        try testDecodeInPlace();
    });
    test("Base64 length calculations and exact capacity", () => {
        try testLengthCalculations();
    });
    test("Base64 codecs, unpadded URL-safe and validation", () => {
        try testCodecsAndValidation();
    });
    test("Base64 error handling on invalid characters", () => {
        try testErrorHandling();
    });
}
