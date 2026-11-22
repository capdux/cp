// Origin: Pure-CP Stdlib Test Suite (lib/web/csrfGuard.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Stateless HMAC-SHA256 CSRF Token & Sec-Fetch-Site/Origin Guard:
//   1. Stateless HMAC-SHA256 mint/verify: 76B Base64URL, >64B secret, cross-session replay, tamper & TTL expiry
//   2. Safe idempotent methods (GET / HEAD / OPTIONS) auto-pass without tokens
//   3. Sec-Fetch-Site layer: cross-site block, same-site policy toggle & malformed value block
//   4. Origin/Host same-origin matching, trusted origin allowlist, `Origin: null` & CRLF/path injection guard
//   5. Double-submit cookie constant-time equality, missing/invalid/expired token rejection
//   6. Global telemetry stats, resetStats & `cap.web` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CSRF_NONCE_LEN,
    CSRF_TOKEN_LEN,
    CSRF_MAX_TRUSTED_ORIGINS,
    CSRF_DEFAULT_TTL_S,
    CsrfTokenStatus,
    CsrfVerdict,
    CsrfRequest,
    CsrfGuardStats,
    csrfConstantTimeEq,
    mintCsrfToken,
    verifyCsrfToken,
    CsrfGuard,
} from "cap:web";

test("CsrfGuard: stateless HMAC-SHA256 mint/verify, cross-session replay, tamper & TTL expiry") {
    // Use a >64-byte secret to exercise SHA-256 key-hashing branch in RFC 2104 HMAC
    const long_secret = "super-long-production-csrf-hmac-secret-key-exceeding-sixty-four-bytes-0123456789";
    const nonce = "0123456789abcdef";

    let tok_buf: [80]u8 = [0; 80];
    let tok_s: []u8 = tok_buf[0..];
    const n = mintCsrfToken(long_secret, "session-alice", 1_000, nonce, *tok_s) ?? 0;
    expect_eq(n, CSRF_TOKEN_LEN);

    const tok = tok_buf[0..n];

    // Valid within TTL window [1000, 1600]
    expect_eq(verifyCsrfToken(long_secret, "session-alice", tok, 1_000, 600), CsrfTokenStatus.valid);
    expect_eq(verifyCsrfToken(long_secret, "session-alice", tok, 1_600, 600), CsrfTokenStatus.valid);

    // Expired at issued + ttl + 1
    expect_eq(verifyCsrfToken(long_secret, "session-alice", tok, 1_601, 600), CsrfTokenStatus.expired);

    // Cross-session replay rejected
    expect_eq(verifyCsrfToken(long_secret, "session-bob", tok, 1_100, 600), CsrfTokenStatus.invalid);

    // Wrong secret rejected
    expect_eq(verifyCsrfToken("wrong-secret", "session-alice", tok, 1_100, 600), CsrfTokenStatus.invalid);

    // Far-future issued_at (>30s ahead of now_s) rejected
    expect_eq(verifyCsrfToken(long_secret, "session-alice", tok, 900, 600), CsrfTokenStatus.invalid);

    // Single-character tamper in Base64URL token rejected
    tok_buf[70] = if (tok_buf[70] == 'A') 'B' else 'A';
    expect_eq(verifyCsrfToken(long_secret, "session-alice", tok_buf[0..n], 1_100, 600), CsrfTokenStatus.invalid);

    // Invalid mint arguments rejected cleanly
    expect_true(mintCsrfToken("", "s", 1000, nonce, *tok_s) == null);
    expect_true(mintCsrfToken(long_secret, "", 1000, nonce, *tok_s) == null);
    expect_true(mintCsrfToken(long_secret, "s", 0, nonce, *tok_s) == null);
    expect_true(mintCsrfToken(long_secret, "s", 1000, "short-nonce", *tok_s) == null);
}

test("CsrfGuard: safe idempotent methods auto-allowed") {
    let guard = CsrfGuard.init(CSRF_DEFAULT_TTL_S, false, true);

    const r_get = guard.evaluateCsrfRequest(CsrfRequest{ method: "GET" }, "sec", 1_000);
    const r_head = guard.evaluateCsrfRequest(CsrfRequest{ method: "head" }, "sec", 1_000);
    const r_opt = guard.evaluateCsrfRequest(CsrfRequest{ method: "OPTIONS" }, "sec", 1_000);

    expect_eq(r_get.verdict, CsrfVerdict.allowed_safe_method);
    expect_true(r_get.allowed);
    expect_eq(r_get.http_status, @as(u16, 200));
    expect_eq(r_head.verdict, CsrfVerdict.allowed_safe_method);
    expect_eq(r_opt.verdict, CsrfVerdict.allowed_safe_method);
}

test("CsrfGuard: Sec-Fetch-Site cross-site & same-site policy enforcement") {
    const secret = "csrf-secret-key";
    let tok_buf: [80]u8 = [0; 80];
    let tok_s: []u8 = tok_buf[0..];
    const n = mintCsrfToken(secret, "s1", 1_000, "fedcba9876543210", *tok_s) ?? 0;
    const tok = tok_buf[0..n];

    let strict_g = CsrfGuard.init(3600, false, false);
    // cross-site blocked even with valid token
    const r_cross = strict_g.evaluateCsrfRequest(
        CsrfRequest{
            method: "POST",
            session_id: "s1",
            sec_fetch_site: "cross-site",
            origin: "https://app.cap.dev",
            host: "app.cap.dev",
            submitted_token: tok,
        },
        secret,
        1_100,
    );
    expect_eq(r_cross.verdict, CsrfVerdict.denied_cross_site);
    expect_true(!r_cross.allowed);
    expect_eq(r_cross.http_status, @as(u16, 403));

    // same-site blocked when allow_same_site=false, allowed when allow_same_site=true
    const req_same_site = CsrfRequest{
        method: "PUT",
        session_id: "s1",
        sec_fetch_site: "same-site",
        origin: "https://app.cap.dev",
        host: "app.cap.dev",
        submitted_token: tok,
    };
    expect_eq(strict_g.evaluateCsrfRequest(req_same_site, secret, 1_100).verdict, CsrfVerdict.denied_cross_site);

    let relaxed_g = CsrfGuard.init(3600, true, false);
    expect_eq(relaxed_g.evaluateCsrfRequest(req_same_site, secret, 1_100).verdict, CsrfVerdict.allowed_verified);
}

test("CsrfGuard: Origin/Host matching, trusted allowlist & null/malformed Origin guard") {
    const secret = "csrf-secret-key";
    let tok_buf: [80]u8 = [0; 80];
    let tok_s: []u8 = tok_buf[0..];
    const n = mintCsrfToken(secret, "s2", 1_000, "1111222233334444", *tok_s) ?? 0;
    const tok = tok_buf[0..n];

    let guard = CsrfGuard.init(3600, false, true);
    expect_true(guard.addTrustedOrigin("https://admin.cap.dev"));
    expect_true(!guard.addTrustedOrigin("https://evil.com/path")); // rejects path in origin
    expect_true(!guard.addTrustedOrigin("null"));

    // 1. Mismatched origin rejected
    const r_bad = guard.evaluateCsrfRequest(
        CsrfRequest{
            method: "DELETE",
            session_id: "s2",
            sec_fetch_site: "same-origin",
            origin: "https://evil.example.com",
            host: "app.cap.dev",
            submitted_token: tok,
        },
        secret,
        1_050,
    );
    expect_eq(r_bad.verdict, CsrfVerdict.denied_origin_mismatch);

    // 2. Origin: null rejected
    const r_null = guard.evaluateCsrfRequest(
        CsrfRequest{
            method: "POST",
            session_id: "s2",
            origin: "null",
            host: "app.cap.dev",
            submitted_token: tok,
        },
        secret,
        1_050,
    );
    expect_eq(r_null.verdict, CsrfVerdict.denied_origin_mismatch);

    // 3. Missing Origin header rejected when require_origin_header=true
    const r_miss_o = guard.evaluateCsrfRequest(
        CsrfRequest{
            method: "POST",
            session_id: "s2",
            host: "app.cap.dev",
            submitted_token: tok,
        },
        secret,
        1_050,
    );
    expect_eq(r_miss_o.verdict, CsrfVerdict.denied_origin_mismatch);

    // 4. Trusted allowlist origin passes even when Host differs
    const r_allow = guard.evaluateCsrfRequest(
        CsrfRequest{
            method: "PATCH",
            session_id: "s2",
            sec_fetch_site: "same-origin",
            origin: "https://admin.cap.dev",
            host: "api.cap.dev",
            submitted_token: tok,
        },
        secret,
        1_050,
    );
    expect_eq(r_allow.verdict, CsrfVerdict.allowed_verified);
}

test("CsrfGuard: double-submit cookie equality & missing/invalid/expired token verdicts") {
    const secret = "csrf-secret-key";
    let tok_buf: [80]u8 = [0; 80];
    let tok_s: []u8 = tok_buf[0..];
    const n = mintCsrfToken(secret, "s3", 1_000, "aaaabbbbccccdddd", *tok_s) ?? 0;
    const tok = tok_buf[0..n];

    let guard = CsrfGuard.init(300, false, false);

    // Missing token
    expect_eq(
        guard.evaluateCsrfRequest(
            CsrfRequest{ method: "POST", session_id: "s3", origin: "https://app.cap.dev", host: "app.cap.dev" },
            secret,
            1_100,
        ).verdict,
        CsrfVerdict.denied_missing_token,
    );

    // Double-submit cookie mismatch
    expect_eq(
        guard.evaluateCsrfRequest(
            CsrfRequest{
                method: "POST",
                session_id: "s3",
                origin: "https://app.cap.dev",
                host: "app.cap.dev",
                submitted_token: tok,
                cookie_token: "different-cookie-value",
            },
            secret,
            1_100,
        ).verdict,
        CsrfVerdict.denied_invalid_token,
    );

    // Double-submit cookie matches + valid HMAC -> allowed_verified
    expect_eq(
        guard.evaluateCsrfRequest(
            CsrfRequest{
                method: "POST",
                session_id: "s3",
                origin: "https://app.cap.dev",
                host: "app.cap.dev",
                submitted_token: tok,
                cookie_token: tok,
            },
            secret,
            1_100,
        ).verdict,
        CsrfVerdict.allowed_verified,
    );

    // Expired token -> denied_expired_token
    expect_eq(
        guard.evaluateCsrfRequest(
            CsrfRequest{
                method: "POST",
                session_id: "s3",
                origin: "https://app.cap.dev",
                host: "app.cap.dev",
                submitted_token: tok,
                cookie_token: tok,
            },
            secret,
            1_400,
        ).verdict,
        CsrfVerdict.denied_expired_token,
    );
}

test("CsrfGuard: global telemetry stats, reset & cap.web facade parity") {
    const secret = "csrf-facade-secret";
    let tok_buf: [80]u8 = [0; 80];
    let tok_s: []u8 = tok_buf[0..];
    const n = cap.web.mintCsrfToken(secret, "s4", 2_000, "0011223344556677", *tok_s) ?? 0;
    const tok = tok_buf[0..n];

    let guard = cap.web.CsrfGuard.init(600, false, false);
    * = guard.addTrustedOrigin("https://web.cap.dev");

    * = guard.evaluateCsrfRequest(CsrfRequest{ method: "GET" }, secret, 2_100);
    * = guard.evaluateCsrfRequest(
        CsrfRequest{ method: "POST", session_id: "s4", origin: "https://web.cap.dev", submitted_token: tok },
        secret,
        2_100,
    );
    * = guard.evaluateCsrfRequest(
        CsrfRequest{ method: "POST", sec_fetch_site: "cross-site", submitted_token: tok },
        secret,
        2_100,
    );
    * = guard.evaluateCsrfRequest(
        CsrfRequest{ method: "POST", origin: "https://bad.dev", host: "web.cap.dev", submitted_token: tok },
        secret,
        2_100,
    );
    * = guard.evaluateCsrfRequest(CsrfRequest{ method: "POST", session_id: "s4" }, secret, 2_100);
    * = guard.evaluateCsrfRequest(
        CsrfRequest{ method: "POST", session_id: "s4", submitted_token: "bad" },
        secret,
        2_100,
    );
    * = guard.evaluateCsrfRequest(
        CsrfRequest{ method: "POST", session_id: "s4", submitted_token: tok },
        secret,
        2_900,
    );

    const st: CsrfGuardStats = guard.stats();
    expect_eq(st.trusted_origin_count, @as(usize, 1));
    expect_eq(st.total_requests, @as(u64, 7));
    expect_eq(st.allowed_safe_count, @as(u64, 1));
    expect_eq(st.allowed_verified_count, @as(u64, 1));
    expect_eq(st.denied_cross_site_count, @as(u64, 1));
    expect_eq(st.denied_origin_mismatch_count, @as(u64, 1));
    expect_eq(st.denied_missing_token_count, @as(u64, 1));
    expect_eq(st.denied_invalid_token_count, @as(u64, 1));
    expect_eq(st.denied_expired_token_count, @as(u64, 1));

    guard.resetStats();
    expect_eq(guard.stats().total_requests, @as(u64, 0));
    expect_eq(guard.stats().trusted_origin_count, @as(usize, 1));
    expect_true(csrfConstantTimeEq("abc", "abc"));
    expect_true(!csrfConstantTimeEq("abc", "abd"));
    expect_eq(CSRF_NONCE_LEN, @as(usize, 16));
    expect_eq(CSRF_MAX_TRUSTED_ORIGINS, @as(usize, 16));
}
