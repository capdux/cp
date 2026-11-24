// Wave 36 R6 Reference-Hardened Regression & Fuzz Suite
// Covers lib/web/{csrfGuard,signedUrl,hstsStore}.cp against actix/actix-web (MIT OR Apache-2.0) & RFC 6797

import { test, expect_eq } from "cap:test";
import {
    scalarWebAuthCtEqual,
    simdWebAuthCtEqual,
    scalarWebAuthAsciiLowerEql,
    simdWebAuthAsciiLowerEql,
    CSRF_TOKEN_LEN,
    CsrfTokenStatus,
    CsrfVerdict,
    CsrfRequest,
    CsrfGuard,
    csrfConstantTimeEq,
    mintCsrfToken,
    verifyCsrfToken,
} from "../../lib/web/csrfGuard.cp";
import {
    SIGNED_URL_SIG_LEN,
    SignedUrlStatus,
    SignedUrlKeyring,
    signedUrlConstantTimeEq,
    signUrl,
    verifySignedUrl,
} from "../../lib/web/signedUrl.cp";
import {
    HstsPolicy,
    HstsStore,
    isHstsEligibleHost,
    parseHstsHeader,
    formatHstsHeader,
} from "../../lib/web/hstsStore.cp";

fn splitmix64(state: u64): u64 {
    let z = state +% 0x6E37_79B9_7F4A_7C15;
    z = (z ^ (z >> 30)) *% 0x3F58_476D_1CE4_E5B9;
    z = (z ^ (z >> 27)) *% 0x14D0_49BB_1331_11EB;
    return z ^ (z >> 31);
}

fn checkCsrfVectors(): u32 {
    const secret: []const u8 = "w36-r6-csrf-hmac-secret-key-0123456789";
    const session: []const u8 = "sess-user-42";
    const nonce: []const u8 = "0123456789abcdef";
    const issued: i64 = 1_700_000_000;
    const ttl: i64 = 600;

    let tok_buf: [CSRF_TOKEN_LEN]u8 = [0; CSRF_TOKEN_LEN];
    const tok_slice: []u8 = tok_buf[0..];
    const tok_len = mintCsrfToken(secret, session, issued, nonce, tok_slice) ?? return 1;
    if (tok_len != CSRF_TOKEN_LEN) return 2;

    // Valid at issued and exact expiry boundary
    if (verifyCsrfToken(secret, session, tok_buf[0..tok_len], issued, ttl) != CsrfTokenStatus.valid) return 3;
    if (verifyCsrfToken(secret, session, tok_buf[0..tok_len], issued + ttl, ttl) != CsrfTokenStatus.valid) return 4;
    if (verifyCsrfToken(secret, session, tok_buf[0..tok_len], issued + ttl + 1, ttl) != CsrfTokenStatus.expired) return 5;

    // Future skew boundary (+30s allowed, +31s rejected)
    if (verifyCsrfToken(secret, session, tok_buf[0..tok_len], issued - 30, ttl) != CsrfTokenStatus.valid) return 6;
    if (verifyCsrfToken(secret, session, tok_buf[0..tok_len], issued - 31, ttl) != CsrfTokenStatus.invalid) return 7;

    // Cross-session and wrong-secret rejection
    if (verifyCsrfToken(secret, "sess-user-99", tok_buf[0..tok_len], issued + 10, ttl) != CsrfTokenStatus.invalid) return 8;
    if (verifyCsrfToken("wrong-secret", session, tok_buf[0..tok_len], issued + 10, ttl) != CsrfTokenStatus.invalid) return 9;

    // Single-bit tamper across all 76 bytes must be rejected
    let idx: usize = 0;
    while (idx < CSRF_TOKEN_LEN) : (idx += 1) {
        let tampered: [CSRF_TOKEN_LEN]u8 = tok_buf;
        tampered[idx] = tampered[idx] ^ 0x01;
        if (verifyCsrfToken(secret, session, tampered[0..], issued + 10, ttl) == CsrfTokenStatus.valid) return 10;
    }

    // CRLF / control-char injection rejection
    if (verifyCsrfToken(secret, session, "bad\r\ntoken", issued + 10, ttl) != CsrfTokenStatus.invalid) return 11;

    // CsrfGuard defense-in-depth checks
    let guard = CsrfGuard.init(ttl, false, true);
    if (!guard.addTrustedOrigin("https://app.example.com")) return 12;

    // Safe method passes
    const d_get = guard.evaluateCsrfRequest(CsrfRequest{ method: "GET" }, secret, issued + 5);
    if (!d_get.allowed || d_get.verdict != CsrfVerdict.allowed_safe_method) return 13;

    // Cross-site blocked
    const d_xsite = guard.evaluateCsrfRequest(CsrfRequest{
        method: "POST",
        session_id: session,
        sec_fetch_site: "cross-site",
        origin: "https://app.example.com",
        host: "app.example.com",
        submitted_token: tok_buf[0..],
    }, secret, issued + 5);
    if (d_xsite.allowed || d_xsite.verdict != CsrfVerdict.denied_cross_site) return 14;

    // Origin mismatch blocked
    const d_orig = guard.evaluateCsrfRequest(CsrfRequest{
        method: "POST",
        session_id: session,
        sec_fetch_site: "same-origin",
        origin: "https://evil.example.com",
        host: "app.example.com",
        submitted_token: tok_buf[0..],
    }, secret, issued + 5);
    if (d_orig.allowed || d_orig.verdict != CsrfVerdict.denied_origin_mismatch) return 15;

    // Double-submit cookie mismatch blocked
    const d_cookie = guard.evaluateCsrfRequest(CsrfRequest{
        method: "POST",
        session_id: session,
        sec_fetch_site: "same-origin",
        origin: "https://app.example.com",
        host: "app.example.com",
        submitted_token: tok_buf[0..],
        cookie_token: "different-cookie-token",
    }, secret, issued + 5);
    if (d_cookie.allowed || d_cookie.verdict != CsrfVerdict.denied_invalid_token) return 16;

    // Valid POST with matching double-submit cookie allowed
    const d_ok = guard.evaluateCsrfRequest(CsrfRequest{
        method: "POST",
        session_id: session,
        sec_fetch_site: "same-origin",
        origin: "https://APP.EXAMPLE.COM",
        host: "app.example.com",
        submitted_token: tok_buf[0..],
        cookie_token: tok_buf[0..],
    }, secret, issued + 5);
    if (!d_ok.allowed || d_ok.verdict != CsrfVerdict.allowed_verified) return 17;

    if (!csrfConstantTimeEq(tok_buf[0..], tok_buf[0..])) return 18;
    return 0;
}

fn checkSignedUrlVectors(): u32 {
    const secret1: []const u8 = "w36-r6-signed-url-key-v1-secret";
    const secret2: []const u8 = "w36-r6-signed-url-key-v2-secret-rotated";
    const exp: i64 = 1_700_005_000;

    let url_buf: [512]u8 = [0; 512];
    const url_s: []u8 = url_buf[0..];
    const n = signUrl(secret1, "GET", "/files/report.pdf", "dl=1&v=2", exp, "k1", url_s) ?? return 1;
    const signed = url_buf[0..n];

    // Valid verification
    const v_ok = verifySignedUrl(secret1, "GET", signed, exp - 100, 30);
    if (!v_ok.valid || v_ok.status != SignedUrlStatus.valid) return 2;
    if (v_ok.exp_epoch_s != exp) return 3;

    // Case-insensitive method matches
    const v_lower = verifySignedUrl(secret1, "get", signed, exp - 100, 30);
    if (!v_lower.valid) return 4;

    // Method mismatch detected
    const v_mm = verifySignedUrl(secret1, "POST", signed, exp - 100, 30);
    if (v_mm.valid || v_mm.status != SignedUrlStatus.method_mismatch) return 5;

    // Expiry & clock skew boundary
    const v_skew = verifySignedUrl(secret1, "GET", signed, exp + 30, 30);
    if (!v_skew.valid) return 6;
    const v_exp = verifySignedUrl(secret1, "GET", signed, exp + 31, 30);
    if (v_exp.valid || v_exp.status != SignedUrlStatus.expired) return 7;

    // Single-bit tamper on signature (last 43 bytes) and path
    let j: usize = 0;
    while (j < SIGNED_URL_SIG_LEN) : (j += 1) {
        let mut_buf: [512]u8 = url_buf;
        mut_buf[n - 1 - j] = mut_buf[n - 1 - j] ^ 0x01;
        const vt = verifySignedUrl(secret1, "GET", mut_buf[0..n], exp - 100, 30);
        if (vt.valid) return 8;
    }
    let path_mut: [512]u8 = url_buf;
    path_mut[2] = 'X';
    if (verifySignedUrl(secret1, "GET", path_mut[0..n], exp - 100, 30).valid) return 9;

    // Small buffer truncation guard must return null
    let tiny: [16]u8 = [0; 16];
    const tiny_s: []u8 = tiny[0..];
    if (signUrl(secret1, "GET", "/files/report.pdf", "dl=1", exp, "k1", tiny_s) != null) return 10;

    // Duplicate reserved params and CRLF rejected
    if (verifySignedUrl(secret1, "GET", "/a?exp=100&exp=200&sig=abc", 50, 0).status != SignedUrlStatus.malformed_url) return 11;
    if (verifySignedUrl(secret1, "GET", "/a\r\n?exp=100&sig=abc", 50, 0).status != SignedUrlStatus.malformed_url) return 12;

    // Multi-key rotation & revocation
    let kr = SignedUrlKeyring.init();
    if (!kr.addKey("k1", secret1, true)) return 13;
    let u1_buf: [512]u8 = [0; 512];
    const u1_s: []u8 = u1_buf[0..];
    const u1_len = kr.signWithPrimary("GET", "/assets/a.bin", "", exp, u1_s) ?? return 14;

    // Rotate to k2 primary
    if (!kr.addKey("k2", secret2, true)) return 15;
    let u2_buf: [512]u8 = [0; 512];
    const u2_s: []u8 = u2_buf[0..];
    const u2_len = kr.signWithPrimary("GET", "/assets/b.bin", "", exp, u2_s) ?? return 16;

    // Both k1 and k2 verify before revocation
    if (!kr.verify("GET", u1_buf[0..u1_len], exp - 10, 0).valid) return 17;
    if (!kr.verify("GET", u2_buf[0..u2_len], exp - 10, 0).valid) return 18;

    // Revoke k1 -> u1 immediately rejected, u2 still valid
    if (!kr.revokeKey("k1")) return 19;
    if (kr.verify("GET", u1_buf[0..u1_len], exp - 10, 0).valid) return 20;
    if (!kr.verify("GET", u2_buf[0..u2_len], exp - 10, 0).valid) return 21;

    if (!signedUrlConstantTimeEq("abcdef0123456789XY", "abcdef0123456789XY")) return 22;
    return 0;
}

fn isHstsHeaderRejected(hdr: []const u8): bool {
    const _p = parseHstsHeader(hdr) catch return true;
    return _p.max_age_sec == ~@as(u64, 0) && false;
}

fn checkHstsVectors(): u32 {
    // Parse & format round-trip
    const p1 = parseHstsHeader("max-age=31536000; includeSubDomains; preload") catch return 1;
    if (p1.max_age_sec != 31_536_000 || !p1.include_subdomains || !p1.preload) return 2;
    if (!p1.isPreloadEligible()) return 3;

    let fmt_buf: [128]u8 = [0; 128];
    const fmt_s: []u8 = fmt_buf[0..];
    const fmt_n = formatHstsHeader(p1, fmt_s) catch return 4;
    const p1_rt = parseHstsHeader(fmt_buf[0..fmt_n]) catch return 5;
    if (p1_rt.max_age_sec != p1.max_age_sec || !p1_rt.include_subdomains || !p1_rt.preload) return 6;

    // Quoted max-age & case-insensitive directives
    const p2 = parseHstsHeader("MAX-AGE=\"86400\"; INCLUDESUBDOMAINS") catch return 7;
    if (p2.max_age_sec != 86400 || !p2.include_subdomains || p2.preload) return 8;

    // Reject malformed/duplicate/overflow/CRLF headers
    if (!isHstsHeaderRejected("")) return 9;
    if (!isHstsHeaderRejected("includeSubDomains")) return 10;
    if (!isHstsHeaderRejected("max-age=-10")) return 11;
    if (!isHstsHeaderRejected("max-age=99999999999999999999999")) return 12;
    if (!isHstsHeaderRejected("max-age=10; max-age=20")) return 13;
    if (!isHstsHeaderRejected("max-age=10; includeSubDomains; includeSubDomains")) return 14;
    if (!isHstsHeaderRejected("max-age=10\r\nX-Inject: 1")) return 15;
    if (!isHstsHeaderRejected("max-age=10\x7f")) return 16;

    // Host eligibility: reject IPv4, IPv6, empty, malformed
    if (isHstsEligibleHost("127.0.0.1")) return 18;
    if (isHstsEligibleHost("[::1]")) return 19;
    if (isHstsEligibleHost("")) return 20;
    if (isHstsEligibleHost("-bad.example.com")) return 21;
    if (isHstsEligibleHost("bad..example.com")) return 22;
    if (!isHstsEligibleHost("API.Example.COM.")) return 23;

    // HstsStore subdomain dot-boundary & max-age=0 revocation
    let store = HstsStore.init();
    const applied = store.applyHeader("Example.COM", "max-age=3600; includeSubDomains", 1000) catch return 24;
    if (!applied) return 25;

    const r_exact = store.shouldUpgrade("example.com", 2000);
    if (!r_exact.must_upgrade || !r_exact.matched_exact) return 26;

    const r_sub = store.shouldUpgrade("a.b.example.com", 2000);
    if (!r_sub.must_upgrade || !r_sub.matched_via_subdomain) return 27;

    // CRITICAL: evil-example.com MUST NOT match example.com
    const r_evil = store.shouldUpgrade("evil-example.com", 2000);
    if (r_evil.must_upgrade) return 28;
    const r_not = store.shouldUpgrade("notexample.com", 2000);
    if (r_not.must_upgrade) return 29;

    // Expired after 1000 + 3600 = 4600
    const r_exp = store.shouldUpgrade("example.com", 4600);
    if (r_exp.must_upgrade) return 30;

    // Re-apply then clear via max-age=0
    const re_ok = store.applyHeader("example.com", "max-age=3600", 5000) catch return 31;
    if (!re_ok) return 32;
    if (!store.shouldUpgrade("example.com", 5001).must_upgrade) return 33;
    const cleared = store.applyHeader("example.com", "max-age=0", 5002) catch return 34;
    if (!cleared) return 35;
    if (store.shouldUpgrade("example.com", 5003).must_upgrade) return 36;

    // Preload cannot be cleared by dynamic max-age=0
    store.addPreload("secure.org", true) catch return 37;
    const pre_clr = store.applyHeader("secure.org", "max-age=0", 5004) catch return 38;
    if (pre_clr) return 39;
    if (!store.shouldUpgrade("sub.secure.org", 99_999_999).must_upgrade) return 40;
    if (store.shouldUpgrade("evil-secure.org", 99_999_999).must_upgrade) return 41;

    return 0;
}

fn checkSimdParity(): u32 {
    // 1. All 256 byte values against self and ^ 0x20 trap
    let a1: [1]u8 = [0];
    let b1: [1]u8 = [0];
    let byte_v: usize = 0;
    while (byte_v < 256) : (byte_v += 1) {
        a1[0] = @as(u8, @intCast(byte_v));
        b1[0] = a1[0] ^ 0x20;
        if (simdWebAuthCtEqual(a1[0..], a1[0..]) != scalarWebAuthCtEqual(a1[0..], a1[0..])) return 1;
        if (simdWebAuthCtEqual(a1[0..], b1[0..]) != scalarWebAuthCtEqual(a1[0..], b1[0..])) return 2;
        if (simdWebAuthAsciiLowerEql(a1[0..], a1[0..]) != scalarWebAuthAsciiLowerEql(a1[0..], a1[0..])) return 3;
        if (simdWebAuthAsciiLowerEql(a1[0..], b1[0..]) != scalarWebAuthAsciiLowerEql(a1[0..], b1[0..])) return 4;
    }

    // 2. Lengths 0..=257 exact + single-byte mutation parity
    let buf_a: [257]u8 = [0; 257];
    let buf_b: [257]u8 = [0; 257];
    let k: usize = 0;
    while (k < 257) : (k += 1) {
        buf_a[k] = @as(u8, @intCast((k * 13 + 33) & 0xFF));
        buf_b[k] = buf_a[k];
    }

    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        if (simdWebAuthCtEqual(buf_a[0..len], buf_b[0..len]) != scalarWebAuthCtEqual(buf_a[0..len], buf_b[0..len])) return 5;
        if (simdWebAuthAsciiLowerEql(buf_a[0..len], buf_b[0..len]) != scalarWebAuthAsciiLowerEql(buf_a[0..len], buf_b[0..len])) return 6;
        if (len > 0) {
            // Flip case on alpha or flip bit on last byte
            const orig = buf_b[len - 1];
            buf_b[len - 1] = orig ^ 0x20;
            if (simdWebAuthCtEqual(buf_a[0..len], buf_b[0..len]) != scalarWebAuthCtEqual(buf_a[0..len], buf_b[0..len])) return 7;
            if (simdWebAuthAsciiLowerEql(buf_a[0..len], buf_b[0..len]) != scalarWebAuthAsciiLowerEql(buf_a[0..len], buf_b[0..len])) return 8;
            buf_b[len - 1] = orig ^ 0x01;
            if (simdWebAuthCtEqual(buf_a[0..len], buf_b[0..len]) != scalarWebAuthCtEqual(buf_a[0..len], buf_b[0..len])) return 9;
            if (simdWebAuthAsciiLowerEql(buf_a[0..len], buf_b[0..len]) != scalarWebAuthAsciiLowerEql(buf_a[0..len], buf_b[0..len])) return 10;
            buf_b[len - 1] = orig;
        }
    }

    // Mismatched lengths
    if (simdWebAuthCtEqual(buf_a[0..32], buf_b[0..31])) return 11;
    if (simdWebAuthAsciiLowerEql(buf_a[0..32], buf_b[0..31])) return 12;
    return 0;
}

fn checkPrngFuzz(): u32 {
    let rng: u64 = 0x3606_5A5A_1234_5678;

    // 1. 600 CSRF fuzz cases
    let i: usize = 0;
    while (i < 600) : (i += 1) {
        let tok: [96]u8 = [0; 96];
        rng = splitmix64(rng);
        const len = @as(usize, @intCast(rng % 96));
        let k: usize = 0;
        while (k < len) : (k += 1) {
            rng = splitmix64(rng);
            tok[k] = @as(u8, @intCast(rng & 0xFF));
        }
        const st = verifyCsrfToken("fuzz-secret", "fuzz-sess", tok[0..len], 1_700_000_000, 3600);
        if (st == CsrfTokenStatus.valid) return 1;
    }

    // 2. 600 SignedUrl fuzz cases
    let j: usize = 0;
    while (j < 600) : (j += 1) {
        let url: [160]u8 = [0; 160];
        rng = splitmix64(rng);
        const len = @as(usize, @intCast(rng % 160));
        let k: usize = 0;
        while (k < len) : (k += 1) {
            rng = splitmix64(rng);
            url[k] = @as(u8, @intCast(rng & 0xFF));
        }
        if (len > 0 && (j % 3 == 0)) url[0] = '/';
        const res = verifySignedUrl("fuzz-secret", "GET", url[0..len], 1_700_000_000, 30);
        if (res.valid) return 2;
    }

    // 3. 600 HSTS fuzz cases
    let store = HstsStore.init();
    let m: usize = 0;
    while (m < 600) : (m += 1) {
        let hdr: [96]u8 = [0; 96];
        let host: [64]u8 = [0; 64];
        rng = splitmix64(rng);
        const hlen = @as(usize, @intCast(rng % 96));
        rng = splitmix64(rng);
        const dlen = @as(usize, @intCast(rng % 64));
        let k: usize = 0;
        while (k < hlen) : (k += 1) {
            rng = splitmix64(rng);
            hdr[k] = @as(u8, @intCast(rng & 0xFF));
        }
        let d: usize = 0;
        while (d < dlen) : (d += 1) {
            rng = splitmix64(rng);
            host[d] = @as(u8, @intCast(rng & 0xFF));
        }
        const _p = parseHstsHeader(hdr[0..hlen]) catch HstsPolicy{};
        const _e = isHstsEligibleHost(host[0..dlen]);
        const _l = store.shouldUpgrade(host[0..dlen], 1_700_000_000);
        if (_p.max_age_sec == 1 && !_e && _l.must_upgrade) return 3;
    }

    return 0;
}

fn main() {
    test("w36r6_csrf_vectors", () => {
        expect_eq(checkCsrfVectors(), @as(u32, 0));
    });
    test("w36r6_signed_url_vectors", () => {
        expect_eq(checkSignedUrlVectors(), @as(u32, 0));
    });
    test("w36r6_hsts_vectors", () => {
        expect_eq(checkHstsVectors(), @as(u32, 0));
    });
    test("w36r6_simd_parity", () => {
        expect_eq(checkSimdParity(), @as(u32, 0));
    });
    test("w36r6_prng_fuzz_1800", () => {
        expect_eq(checkPrngFuzz(), @as(u32, 0));
    });
}
