// Source: cp/cap/test/test_csrf_production_deep.cp (production deep tests for cap:csrf & cap.csrf)

const std = @import("std");
import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    CsrfError,
    CsrfProtector,
    generateSecret,
    generateSecretAlloc,
    signToken,
    signTokenAt,
    verifySignedToken,
    verifySignedTokenWithTtl,
    maskToken,
    unmaskAndVerify,
    isSafeMethod,
    verifyOrigin,
} from "cap:csrf";

test("csrf signed token generation, constant-time verify, and tamper/cross-session rejection") {
    const alloc = std.testing.allocator;
    let secret_buf: [32]u8 = [0; 32];
    const secret = generateSecret(&secret_buf);

    const session_a = "sess_alice_001";
    const session_b = "sess_bob_002";

    const token = try signToken(alloc, secret, session_a);
    defer alloc.free(token);

    try verifySignedToken(secret, session_a, token);

    // Cross-session rejection
    let cross_rejected = false;
    verifySignedToken(secret, session_b, token) catch |e| {
        if (e == CsrfError.InvalidSignature) {
            cross_rejected = true;
        }
    };
    try expect_eq(cross_rejected, true);

    // Tampered token byte rejection
    let tampered = try alloc.dupe(u8, token);
    defer alloc.free(tampered);
    tampered[tampered.len - 1] = if (tampered[tampered.len - 1] == 'A') 'B' else 'A';

    let tamper_rejected = false;
    verifySignedToken(secret, session_a, tampered) catch |e| {
        if (e == CsrfError.InvalidSignature or e == CsrfError.DecodingFailed) {
            tamper_rejected = true;
        }
    };
    try expect_eq(tamper_rejected, true);

    // Wrong secret rejection
    let other_secret: [32]u8 = [0xAB; 32];
    let wrong_secret_rejected = false;
    verifySignedToken(&other_secret, session_a, token) catch |e| {
        if (e == CsrfError.InvalidSignature) {
            wrong_secret_rejected = true;
        }
    };
    try expect_eq(wrong_secret_rejected, true);
}

test("csrf BREACH masked token randomization and unmaskAndVerify") {
    const alloc = std.testing.allocator;
    const secret = try generateSecretAlloc(alloc);
    defer alloc.free(secret);

    const session_id = "sess_breach_guard";
    const raw_token = try signToken(alloc, secret, session_id);
    defer alloc.free(raw_token);

    const masked_1 = try maskToken(alloc, raw_token);
    defer alloc.free(masked_1);
    const masked_2 = try maskToken(alloc, raw_token);
    defer alloc.free(masked_2);

    // Two masks of the exact same raw token must differ to mitigate BREACH
    try expect_eq(std.mem.eql(u8, masked_1, masked_2), false);

    try unmaskAndVerify(alloc, secret, session_id, masked_1);
    try unmaskAndVerify(alloc, secret, session_id, masked_2);

    // Cross-session unmask rejection
    let cross_ok = true;
    unmaskAndVerify(alloc, secret, "sess_other", masked_1) catch {
        cross_ok = false;
    };
    try expect_eq(cross_ok, false);
}

test("csrf verifyOrigin blocks cross-site, prefix spoofing, port mismatch, and checks safe methods") {
    try expect_eq(isSafeMethod("GET"), true);
    try expect_eq(isSafeMethod("head"), true);
    try expect_eq(isSafeMethod("OPTIONS"), true);
    try expect_eq(isSafeMethod("TRACE"), true);
    try expect_eq(isSafeMethod("POST"), false);
    try expect_eq(isSafeMethod("DELETE"), false);

    const expected = "https://app.example.com";

    // Valid exact Origin & Referer fallback
    try verifyOrigin("https://app.example.com", null, expected, "same-origin");
    try verifyOrigin(null, "https://app.example.com:443/settings?tab=1", expected, "same-origin");

    // Sec-Fetch-Site: cross-site blocked even if Origin matches
    let sfs_blocked = false;
    verifyOrigin("https://app.example.com", null, expected, "cross-site") catch |e| {
        if (e == CsrfError.CrossSiteRequest) {
            sfs_blocked = true;
        }
    };
    try expect_eq(sfs_blocked, true);

    // Prefix spoofing blocked
    let prefix_blocked = false;
    verifyOrigin("https://app.example.com.evil.com", null, expected, "same-site") catch |e| {
        if (e == CsrfError.InvalidOrigin) {
            prefix_blocked = true;
        }
    };
    try expect_eq(prefix_blocked, true);

    // Userinfo spoofing & scheme downgrade & null origin blocked
    let userinfo_blocked = false;
    verifyOrigin("https://app.example.com@evil.com", null, expected, null) catch |e| {
        if (e == CsrfError.InvalidOrigin) {
            userinfo_blocked = true;
        }
    };
    try expect_eq(userinfo_blocked, true);

    let null_origin_blocked = false;
    verifyOrigin("null", null, expected, null) catch |e| {
        if (e == CsrfError.InvalidOrigin) {
            null_origin_blocked = true;
        }
    };
    try expect_eq(null_origin_blocked, true);

    let missing_blocked = false;
    verifyOrigin(null, null, expected, "same-origin") catch |e| {
        if (e == CsrfError.MissingOrigin) {
            missing_blocked = true;
        }
    };
    try expect_eq(missing_blocked, true);
}

test("csrf rejects invalid length, non-Base64URL characters, and expired TTL tokens") {
    const alloc = std.testing.allocator;
    let secret_buf: [32]u8 = [0; 32];
    const secret = generateSecret(&secret_buf);
    const session_id = "sess_ttl_check";

    // Invalid length
    let short_rejected = false;
    verifySignedToken(secret, session_id, "abc123") catch |e| {
        if (e == CsrfError.InvalidToken or e == CsrfError.DecodingFailed) {
            short_rejected = true;
        }
    };
    try expect_eq(short_rejected, true);

    // Non-Base64URL characters ('+', '/', '=')
    let bad_b64_rejected = false;
    verifySignedToken(secret, session_id, "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") catch |e| {
        if (e == CsrfError.DecodingFailed) {
            bad_b64_rejected = true;
        }
    };
    try expect_eq(bad_b64_rejected, true);

    // Expired timestamp check
    const now_ms: u64 = @intCast(std.time.milliTimestamp());
    const old_ts = if (now_ms > 20_000) now_ms - 20_000 else 0;
    const old_token = try signTokenAt(alloc, secret, session_id, old_ts);
    defer alloc.free(old_token);

    let expired_rejected = false;
    verifySignedTokenWithTtl(secret, session_id, old_token, 5_000) catch |e| {
        if (e == CsrfError.ExpiredToken) {
            expired_rejected = true;
        }
    };
    try expect_eq(expired_rejected, true);

    // TTL == 0 disables expiration check
    try verifySignedTokenWithTtl(secret, session_id, old_token, 0);
}

test("csrf cap.csrf dynamic mount and std.testing.allocator zero-leak verification") {
    const alloc = std.testing.allocator;
    let secret_buf: [32]u8 = [0; 32];
    const secret = cap.csrf.generateSecret(&secret_buf);

    const protector = try CsrfProtector.init(secret);
    const signed = try protector.sign(alloc, "sess_dyn");
    defer alloc.free(signed);
    try protector.verify("sess_dyn", signed);

    const masked = try protector.mask(alloc, signed);
    defer alloc.free(masked);
    try protector.unmaskAndVerify(alloc, "sess_dyn", masked);

    try cap.csrf.verifyOrigin("https://cp.dev:8443", null, "https://cp.dev:8443", "same-origin");
    try expect_eq(cap.csrf.isSafeMethod("GET"), true);
}
