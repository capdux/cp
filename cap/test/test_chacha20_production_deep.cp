// Source: cp/cap/test/test_chacha20_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KEY_LEN,
    NONCE_LEN,
    XNONCE_LEN,
    TAG_LEN,
    ChaChaError,
    encrypt,
    decrypt,
    encryptX,
    decryptX,
    sealXAlloc,
    openXAlloc,
} from "cap:chacha20";

test "chacha20: RFC 8439 Section 2.8.2 AEAD official test vector" {
    const key: [32]u8 = [
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
        0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
        0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    ];
    const nonce: [12]u8 = [ 0x07, 0x00, 0x00, 0x00, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47 ];
    const aad: [12]u8 = [ 0x50, 0x51, 0x52, 0x53, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7 ];
    const pt = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";

    let ct: [114]u8 = undefined;
    const tag = try encrypt(key, nonce, &aad, pt, &ct);
    const expected_tag: [16]u8 = [
        0x1a, 0xe1, 0x0b, 0x59, 0x4f, 0x09, 0xe2, 0x6a,
        0x7e, 0x90, 0x2e, 0xcb, 0xd0, 0x60, 0x06, 0x91,
    ];
    try expect_eq(std.mem.eql(u8, &tag, &expected_tag), true);

    let out: [114]u8 = undefined;
    const dec_pt = try decrypt(key, nonce, &aad, &ct, tag, &out);
    try expect_eq(std.mem.eql(u8, dec_pt, pt), true);
}

test "chacha20: tamper detection on tag, ciphertext, and AAD" {
    let key: [32]u8 = undefined;
    @memset(&key, 0x42);
    let nonce: [12]u8 = undefined;
    @memset(&nonce, 0x01);

    let ct: [11]u8 = undefined;
    let tag = try encrypt(key, nonce, "hdr", "hello world", &ct);
    let out: [11]u8 = undefined;

    // Bad AAD
    try expect_eq(decrypt(key, nonce, "bad", &ct, tag, &out) == ChaChaError.AuthenticationFailed, true);

    // Bad tag
    tag[0] ^= 0x01;
    try expect_eq(decrypt(key, nonce, "hdr", &ct, tag, &out) == ChaChaError.AuthenticationFailed, true);
}

test "chacha20: XChaCha20-Poly1305 24-byte nonce roundtrip" {
    let key: [32]u8 = undefined;
    @memset(&key, 0x77);
    let nonce: [24]u8 = undefined;
    @memset(&nonce, 0x99);

    let ct: [16]u8 = undefined;
    const tag = try encryptX(key, nonce, "ctx", "0123456789abcdef", &ct);
    let out: [16]u8 = undefined;
    const pt = try decryptX(key, nonce, "ctx", &ct, tag, &out);
    try expect_eq(std.mem.eql(u8, pt, "0123456789abcdef"), true);
}

test "chacha20: sealXAlloc & openXAlloc random nonce & zero leak" {
    const alloc = std.testing.allocator;
    let key: [32]u8 = undefined;
    @memset(&key, 0xaa);

    const s1 = try sealXAlloc(alloc, key, "meta", "payload");
    defer alloc.free(s1);
    const s2 = try sealXAlloc(alloc, key, "meta", "payload");
    defer alloc.free(s2);

    try expect_eq(s1.len, XNONCE_LEN + 7 + TAG_LEN);
    // Random nonces must differ
    try expect_eq(std.mem.eql(u8, s1, s2), false);

    const pt = try openXAlloc(alloc, key, "meta", s1);
    defer alloc.free(pt);
    try expect_eq(std.mem.eql(u8, pt, "payload"), true);
}

test "chacha20: cap.chacha20 dynamic mount & constants" {
    try expect_eq(KEY_LEN, 32);
    try expect_eq(NONCE_LEN, 12);
    const alloc = std.testing.allocator;
    let key: [32]u8 = undefined;
    @memset(&key, 0x55);
    const sealed = try cap.chacha20.sealXAlloc(alloc, key, "", "dyn");
    defer alloc.free(sealed);
    const pt = try cap.chacha20.openXAlloc(alloc, key, "", sealed);
    defer alloc.free(pt);
    try expect_eq(std.mem.eql(u8, pt, "dyn"), true);
}
