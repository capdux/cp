// Source: cp/cap/test/test_base32_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { Base32Error, Variant, encodedLen, encodeAlloc, decodeAlloc } from "cap:base32";

test "base32: RFC 4648 Section 10 official test vectors" {
    const alloc = std.testing.allocator;
    const inputs: [7][]const u8 = [ "", "f", "fo", "foo", "foob", "fooba", "foobar" ];
    const expected: [7][]const u8 = [
        "",
        "MY======",
        "MZXQ====",
        "MZXW6===",
        "MZXW6YQ=",
        "MZXW6YTB",
        "MZXW6YTBOI======",
    ];
    let i: usize = 0;
    while (i < inputs.len) : (i += 1) {
        const enc = try encodeAlloc(alloc, inputs[i], .Standard, true);
        defer alloc.free(enc);
        try expect_eq(std.mem.eql(u8, enc, expected[i]), true);

        const orig = try decodeAlloc(alloc, enc, .Standard);
        defer alloc.free(orig);
        try expect_eq(std.mem.eql(u8, orig, inputs[i]), true);
    }
}

test "base32: RFC 4648 Base32hex test vector & unpadded mode" {
    const alloc = std.testing.allocator;
    const enc = try encodeAlloc(alloc, "foobar", .Hex, true);
    defer alloc.free(enc);
    try expect_eq(std.mem.eql(u8, enc, "CPNMUOJ1E8======"), true);

    const unpadded = try encodeAlloc(alloc, "foo", .Standard, false);
    defer alloc.free(unpadded);
    try expect_eq(std.mem.eql(u8, unpadded, "MZXW6"), true);
    try expect_eq(encodedLen(3, false), 5);
    try expect_eq(encodedLen(3, true), 8);
}

test "base32: Crockford normalization (O->0, I/L->1, hyphens, lowercase)" {
    const alloc = std.testing.allocator;
    const enc = try encodeAlloc(alloc, "Hi!", .Crockford, false);
    defer alloc.free(enc);
    const orig = try decodeAlloc(alloc, enc, .Crockford);
    defer alloc.free(orig);
    try expect_eq(std.mem.eql(u8, orig, "Hi!"), true);

    // 0/O and 1/I/L equivalence
    const d1 = try decodeAlloc(alloc, "0101-abcd", .Crockford);
    defer alloc.free(d1);
    const d2 = try decodeAlloc(alloc, "OILo-ABCD", .Crockford);
    defer alloc.free(d2);
    try expect_eq(std.mem.eql(u8, d1, d2), true);
}

test "base32: rejects invalid characters & mid-stream padding" {
    const alloc = std.testing.allocator;
    try expect_eq(decodeAlloc(alloc, "MZXW6==A", .Standard) == Base32Error.InvalidPadding, true);
    try expect_eq(decodeAlloc(alloc, "MZXW6!==", .Standard) == Base32Error.InvalidCharacter, true);
}

test "base32: cap.base32 dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    const enc = try cap.base32.encodeAlloc(alloc, "cp-lang", .Standard, true);
    defer alloc.free(enc);
    const orig = try cap.base32.decodeAlloc(alloc, enc, .Standard);
    defer alloc.free(orig);
    try expect_eq(std.mem.eql(u8, orig, "cp-lang"), true);
}
