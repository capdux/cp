// Source: cp/cap/test/test_hkdf_kdf_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HkdfError,
    HkdfHash,
    extractSha256,
    expandSha256,
    deriveSha256,
    deriveSha384,
    deriveSha512,
    deriveAlloc,
    expandLabelSha256,
} from "cap:hkdf_kdf";

test "hkdf: RFC 5869 Appendix A.1 SHA-256 basic vector" {
    let ikm: [22]u8 = undefined;
    @memset(&ikm, 0x0b);
    const salt: [13]u8 = [ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c ];
    const info: [10]u8 = [ 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9 ];

    const prk = extractSha256(&salt, &ikm);
    const expected_prk: [32]u8 = [
        0x07, 0x77, 0x09, 0x36, 0x2c, 0x2e, 0x32, 0xdf,
        0x0d, 0xdc, 0x3f, 0x0d, 0xc4, 0x7b, 0xba, 0x63,
        0x90, 0xb6, 0xc7, 0x3b, 0xb5, 0x0f, 0x9c, 0x31,
        0x22, 0xec, 0x84, 0x4a, 0xd7, 0xc2, 0xb3, 0xe5,
    ];
    try expect_eq(std.mem.eql(u8, &prk, &expected_prk), true);

    let okm: [42]u8 = undefined;
    try expandSha256(prk, &info, &okm);
    const expected_okm: [42]u8 = [
        0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a,
        0x90, 0x43, 0x4f, 0x64, 0xd0, 0x36, 0x2f, 0x2a,
        0x2d, 0x2d, 0x0a, 0x90, 0xcf, 0x1a, 0x5a, 0x4c,
        0x5d, 0xb0, 0x2d, 0x56, 0xec, 0xc4, 0xc5, 0xbf,
        0x34, 0x00, 0x72, 0x08, 0xd5, 0xb8, 0x87, 0x18,
        0x58, 0x65,
    ];
    try expect_eq(std.mem.eql(u8, &okm, &expected_okm), true);
}

test "hkdf: RFC 5869 Appendix A.3 zero-length salt & info" {
    let ikm: [22]u8 = undefined;
    @memset(&ikm, 0x0b);
    let okm: [42]u8 = undefined;
    try deriveSha256("", &ikm, "", &okm);
    const expected_okm: [42]u8 = [
        0x8d, 0xa4, 0xe7, 0x75, 0xa5, 0x63, 0xc1, 0x8f,
        0x71, 0x5f, 0x80, 0x2a, 0x06, 0x3c, 0x5a, 0x31,
        0xb8, 0xa1, 0x1f, 0x5c, 0x5e, 0xe1, 0x87, 0x9e,
        0xc3, 0x45, 0x4e, 0x5f, 0x3c, 0x73, 0x8d, 0x2d,
        0x9d, 0x20, 0x13, 0x95, 0xfa, 0xa4, 0xb6, 0x1a,
        0x96, 0xc8,
    ];
    try expect_eq(std.mem.eql(u8, &okm, &expected_okm), true);
}

test "hkdf: SHA-384, SHA-512, and TLS 1.3 expandLabelSha256" {
    let k384_a: [48]u8 = undefined;
    let k384_b: [48]u8 = undefined;
    try deriveSha384("s", "ikm", "ctx1", &k384_a);
    try deriveSha384("s", "ikm", "ctx2", &k384_b);
    try expect_eq(std.mem.eql(u8, &k384_a, &k384_b), false);

    let k512: [64]u8 = undefined;
    try deriveSha512("s", "ikm", "ctx1", &k512);

    const prk = extractSha256("salt", "ikm");
    let key1: [16]u8 = undefined;
    let key2: [16]u8 = undefined;
    try expandLabelSha256(prk, "key", "", &key1);
    try expandLabelSha256(prk, "iv", "", &key2);
    try expect_eq(std.mem.eql(u8, &key1, &key2), false);
}

test "hkdf: OutputTooLong guard for > 255 * HashLen" {
    const alloc = std.testing.allocator;
    const res = deriveAlloc(alloc, .Sha256, "s", "ikm", "i", 255 * 32 + 1);
    try expect_eq(res == HkdfError.OutputTooLong, true);
    try expect_eq(HkdfHash.Sha256.maxOutputLen(), 255 * 32);
    try expect_eq(HkdfHash.Sha384.maxOutputLen(), 255 * 48);
    try expect_eq(HkdfHash.Sha512.maxOutputLen(), 255 * 64);
}

test "hkdf: cap.hkdf_kdf dynamic mount & zero leak" {
    const alloc = std.testing.allocator;
    const out = try cap.hkdf_kdf.deriveAlloc(alloc, .Sha256, "salt", "secret", "app", 64);
    defer alloc.free(out);
    try expect_eq(out.len, 64);
}
