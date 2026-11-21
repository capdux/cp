// Source: native-cp-stdlib (NIST SP 800-38D AES-GCM & AES-GCM-SIV)
import { test, expect_eq } from "cap:test";
import {
    AesGcmError,
    encrypt128,
    decrypt128,
    encrypt256,
    decrypt256,
    encrypt256Siv,
    decrypt256Siv,
    seal256Alloc,
    open256Alloc,
} from "cap:aes_gcm";
import cap from "cap";

const std = @import("std");

fn expectBytesEq(actual: []const u8, expected: []const u8) !void {
    try std.testing.expectEqualSlices(u8, expected, actual);
}

test "NIST CAVP AES-128-GCM and AES-256-GCM official vectors" {
    // NIST SP 800-38D Test Case 2 (AES-128-GCM)
    const key128 = [_]u8{0} ** 16;
    const nonce = [_]u8{0} ** 12;
    const pt16 = [_]u8{0} ** 16;
    const empty_ad: []const u8 = "";

    let ct128: [16]u8 = undefined;
    let tag128: [16]u8 = undefined;
    try encrypt128(&ct128, &tag128, &pt16, empty_ad, nonce, key128);

    const exp_ct128: [16]u8 = [
        0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
        0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78,
    ];
    const exp_tag128: [16]u8 = [
        0xab, 0x6e, 0x47, 0xd4, 0x2c, 0xec, 0x13, 0xbd,
        0xf5, 0x3a, 0x67, 0xb2, 0x12, 0x57, 0xbd, 0xdf,
    ];
    try expectBytesEq(&ct128, &exp_ct128);
    try expectBytesEq(&tag128, &exp_tag128);

    let dec128_buf: [16]u8 = undefined;
    try decrypt128(&dec128_buf, &ct128, tag128, empty_ad, nonce, key128);
    try expectBytesEq(&dec128_buf, &pt16);

    // NIST SP 800-38D Test Case 14 (AES-256-GCM)
    const key256 = [_]u8{0} ** 32;
    let ct256: [16]u8 = undefined;
    let tag256: [16]u8 = undefined;
    try encrypt256(&ct256, &tag256, &pt16, empty_ad, nonce, key256);

    const exp_ct256: [16]u8 = [
        0xce, 0xa7, 0x40, 0x3d, 0x4d, 0x60, 0x6b, 0x6e,
        0x07, 0x4e, 0xc5, 0xd3, 0xba, 0xf3, 0x9d, 0x18,
    ];
    const exp_tag256: [16]u8 = [
        0xd0, 0xd1, 0xc8, 0xa7, 0x99, 0x99, 0x6b, 0xf0,
        0x26, 0x5b, 0x98, 0xb5, 0xd4, 0x8a, 0xb9, 0x19,
    ];
    try expectBytesEq(&ct256, &exp_ct256);
    try expectBytesEq(&tag256, &exp_tag256);

    let dec256_buf: [16]u8 = undefined;
    try decrypt256(&dec256_buf, &ct256, tag256, empty_ad, nonce, key256);
    try expectBytesEq(&dec256_buf, &pt16);

    // NIST SP 800-38D Test Case 16 (AES-256-GCM with AAD)
    const key16: [32]u8 = [
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
        0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c,
        0x6d, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08,
    ];
    const nonce16: [12]u8 = [
        0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad,
        0xde, 0xca, 0xf8, 0x88,
    ];
    const pt16_60: [60]u8 = [
        0xd9, 0x31, 0x32, 0x25, 0xf8, 0x84, 0x06, 0xe5,
        0xa5, 0x59, 0x09, 0xc5, 0xaf, 0xf5, 0x26, 0x9a,
        0x86, 0xa7, 0xa9, 0x53, 0x15, 0x34, 0xf7, 0xda,
        0x2e, 0x4c, 0x30, 0x3d, 0x8a, 0x31, 0x8a, 0x72,
        0x1c, 0x3c, 0x0c, 0x95, 0x95, 0x68, 0x09, 0x53,
        0x2f, 0xcf, 0x0e, 0x24, 0x49, 0xa6, 0xb5, 0x25,
        0xb1, 0x6a, 0xed, 0xf5, 0xaa, 0x0d, 0xe6, 0x57,
        0xba, 0x63, 0x7b, 0x39,
    ];
    const aad16: [20]u8 = [
        0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
        0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
        0xab, 0xad, 0xda, 0xd2,
    ];
    const exp_ct16: [60]u8 = [
        0x52, 0x2d, 0xc1, 0xf0, 0x99, 0x56, 0x7d, 0x07,
        0xf4, 0x7f, 0x37, 0xa3, 0x2a, 0x84, 0x42, 0x7d,
        0x64, 0x3a, 0x8c, 0xdc, 0xbf, 0xe5, 0xc0, 0xc9,
        0x75, 0x98, 0xa2, 0xbd, 0x25, 0x55, 0xd1, 0xaa,
        0x8c, 0xb0, 0x8e, 0x48, 0x59, 0x0d, 0xbb, 0x3d,
        0xa7, 0xb0, 0x8b, 0x10, 0x56, 0x82, 0x88, 0x38,
        0xc5, 0xf6, 0x1e, 0x63, 0x93, 0xba, 0x7a, 0x0a,
        0xbc, 0xc9, 0xf6, 0x62,
    ];
    const exp_tag16: [16]u8 = [
        0x76, 0xfc, 0x6e, 0xce, 0x0f, 0x4e, 0x17, 0x68,
        0xcd, 0xdf, 0x88, 0x53, 0xbb, 0x2d, 0x55, 0x1b,
    ];
    let ct16: [60]u8 = undefined;
    let tag16: [16]u8 = undefined;
    try encrypt256(&ct16, &tag16, &pt16_60, &aad16, nonce16, key16);
    try expectBytesEq(&ct16, &exp_ct16);
    try expectBytesEq(&tag16, &exp_tag16);

    let pt16_out: [60]u8 = undefined;
    try decrypt256(&pt16_out, &ct16, tag16, &aad16, nonce16, key16);
    try expectBytesEq(&pt16_out, &pt16_60);
}

test "RFC 8452 AES-256-GCM-SIV official vectors" {
    const key: [32]u8 = [
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    ];
    const nonce: [12]u8 = [
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
    ];
    const pt: [8]u8 = [ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 ];
    const exp_ct: [8]u8 = [ 0xc2, 0xef, 0x32, 0x8e, 0x5c, 0x71, 0xc8, 0x3b ];
    const exp_tag: [16]u8 = [
        0x84, 0x31, 0x22, 0x13, 0x0f, 0x73, 0x64, 0xb7,
        0x61, 0xe0, 0xb9, 0x74, 0x27, 0xe3, 0xdf, 0x28,
    ];
    const empty_ad: []const u8 = "";

    let ct: [8]u8 = undefined;
    let tag: [16]u8 = undefined;
    try encrypt256Siv(&ct, &tag, &pt, empty_ad, nonce, key);
    try expectBytesEq(&ct, &exp_ct);
    try expectBytesEq(&tag, &exp_tag);

    let pt_out: [8]u8 = undefined;
    try decrypt256Siv(&pt_out, &ct, tag, empty_ad, nonce, key);
    try expectBytesEq(&pt_out, &pt);
}

test "tamper detection on AAD, ciphertext, and tag returns AuthenticationFailed" {
    const key = [_]u8{0x5a} ** 32;
    const nonce = [_]u8{0x11} ** 12;
    const pt = "authenticated payload";
    const aad = "context-v1";

    let ct: [21]u8 = undefined;
    let tag: [16]u8 = undefined;
    try encrypt256(&ct, &tag, pt, aad, nonce, key);

    let out: [21]u8 = undefined;

    // Tamper AAD
    try std.testing.expectError(
        AesGcmError.AuthenticationFailed,
        decrypt256(&out, &ct, tag, "context-v2", nonce, key),
    );

    // Tamper ciphertext
    let bad_ct = ct;
    bad_ct[0] = bad_ct[0] ^ 0x01;
    try std.testing.expectError(
        AesGcmError.AuthenticationFailed,
        decrypt256(&out, &bad_ct, tag, aad, nonce, key),
    );

    // Tamper tag
    let bad_tag = tag;
    bad_tag[15] = bad_tag[15] ^ 0x80;
    try std.testing.expectError(
        AesGcmError.AuthenticationFailed,
        decrypt256(&out, &ct, bad_tag, aad, nonce, key),
    );

    // BufferTooSmall check
    let tiny: [4]u8 = undefined;
    try std.testing.expectError(
        AesGcmError.BufferTooSmall,
        encrypt256(&tiny, &tag, pt, aad, nonce, key),
    );
}

test "seal256Alloc and open256Alloc with random 12B nonce and zero leaks" {
    const alloc = std.testing.allocator;
    const key = [_]u8{0x7f} ** 32;
    const pt = "zero-leak sealed payload";
    const aad = "session-42";

    const sealed1 = try seal256Alloc(alloc, pt, aad, key);
    defer alloc.free(sealed1);

    const sealed2 = try seal256Alloc(alloc, pt, aad, key);
    defer alloc.free(sealed2);

    try expect_eq(sealed1.len, 12 + pt.len + 16);
    try std.testing.expect(!std.mem.eql(u8, sealed1[0..12], sealed2[0..12]));

    const opened = try open256Alloc(alloc, sealed1, aad, key);
    defer alloc.free(opened);
    try expectBytesEq(opened, pt);

    // Tampered sealed blob must fail cleanly without leaking
    let bad_sealed = try alloc.dupe(u8, sealed1);
    defer alloc.free(bad_sealed);
    bad_sealed[bad_sealed.len - 1] = bad_sealed[bad_sealed.len - 1] ^ 0xff;
    try std.testing.expectError(
        AesGcmError.AuthenticationFailed,
        open256Alloc(alloc, bad_sealed, aad, key),
    );
}

test "empty plaintext AAD-only auth mode and cap.aes_gcm dynamic mount" {
    const alloc = std.testing.allocator;
    const key = [_]u8{0x33} ** 32;
    const nonce = [_]u8{0x99} ** 12;
    const empty_pt: []const u8 = "";
    const aad = "header-only-integrity-check";

    let empty_ct: [0]u8 = undefined;
    let tag: [16]u8 = undefined;
    try cap.aes_gcm.encrypt256(&empty_ct, &tag, empty_pt, aad, nonce, key);

    let empty_out: [0]u8 = undefined;
    try cap.aes_gcm.decrypt256(&empty_out, &empty_ct, tag, aad, nonce, key);

    try std.testing.expectError(
        AesGcmError.AuthenticationFailed,
        cap.aes_gcm.decrypt256(&empty_out, &empty_ct, tag, "tampered-header", nonce, key),
    );

    const sealed = try cap.aes_gcm.seal256Alloc(alloc, empty_pt, aad, key);
    defer alloc.free(sealed);
    try expect_eq(sealed.len, 28);

    const opened = try cap.aes_gcm.open256Alloc(alloc, sealed, aad, key);
    defer alloc.free(opened);
    try expect_eq(opened.len, 0);
}
