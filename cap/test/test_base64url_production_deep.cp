// Source: cp/cap/test/test_base64url_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    Base64UrlError,
    encodeStandard,
    decodeStandard,
    encodeUrlNoPad,
    decodeUrlNoPad,
    encodeUrlPadded,
    decodeUrlAuto,
} from "cap:codec";

test "base64url: RFC 4648 Section 10 official standard vectors" {
    const alloc = std.testing.allocator;
    const inputs: [7][]const u8 = [ "", "f", "fo", "foo", "foob", "fooba", "foobar" ];
    const expected: [7][]const u8 = [ "", "Zg==", "Zm8=", "Zm9v", "Zm9vYg==", "Zm9vYmE=", "Zm9vYmFy" ];

    let i: usize = 0;
    while (i < inputs.len) : (i += 1) {
        const enc = try encodeStandard(alloc, inputs[i]);
        defer alloc.free(enc);
        try expect_eq(std.mem.eql(u8, enc, expected[i]), true);

        const orig = try decodeStandard(alloc, enc);
        defer alloc.free(orig);
        try expect_eq(std.mem.eql(u8, orig, inputs[i]), true);
    }
}

test "base64url: URL-safe alphabet (- and _) without padding for JWT/JWK" {
    const alloc = std.testing.allocator;
    const binary: [4]u8 = [ 0xFB, 0xFF, 0xBF, 0x3E ];
    const url_enc = try encodeUrlNoPad(alloc, &binary);
    defer alloc.free(url_enc);
    try expect_eq(std.mem.indexOfScalar(u8, url_enc, '+') == null, true);
    try expect_eq(std.mem.indexOfScalar(u8, url_enc, '/') == null, true);
    try expect_eq(std.mem.indexOfScalar(u8, url_enc, '=') == null, true);

    const orig = try decodeUrlNoPad(alloc, url_enc);
    defer alloc.free(orig);
    try expect_eq(std.mem.eql(u8, orig, &binary), true);
}

test "base64url: encodeUrlPadded & decodeUrlAuto handles both padded and unpadded" {
    const alloc = std.testing.allocator;
    const padded = try encodeUrlPadded(alloc, "hi!");
    defer alloc.free(padded);
    const d1 = try decodeUrlAuto(alloc, padded);
    defer alloc.free(d1);
    try expect_eq(std.mem.eql(u8, d1, "hi!"), true);

    const unpadded = try encodeUrlNoPad(alloc, "hi");
    defer alloc.free(unpadded);
    const d2 = try decodeUrlAuto(alloc, unpadded);
    defer alloc.free(d2);
    try expect_eq(std.mem.eql(u8, d2, "hi"), true);
}

test "base64url: rejects invalid chars & bad padding" {
    const alloc = std.testing.allocator;
    try expect_eq(decodeUrlNoPad(alloc, "abc+def") == Base64UrlError.InvalidCharacter, true);
    try expect_eq(decodeStandard(alloc, "Zg=") == Base64UrlError.InvalidPadding, true);
}

test "base64url: cap.base64url dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    const enc = try cap.codec.encodeUrlNoPad(alloc, "cp-stdlib");
    defer alloc.free(enc);
    const orig = try cap.codec.decodeUrlAuto(alloc, enc);
    defer alloc.free(orig);
    try expect_eq(std.mem.eql(u8, orig, "cp-stdlib"), true);
}
