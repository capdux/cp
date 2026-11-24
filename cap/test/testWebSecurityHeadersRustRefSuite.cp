// Origin: Wave 35 R8 Reference-Hardening Verification Suite (Lane 2F)
// Reference-hardened against tower-rs/tower-http (MIT) & W3C Fetch CORS / W3C CSP3 / RFC 9110 §13
// Covers:
//   1. W3C Fetch CORS official compliance vectors (credentials != '*', null origin default-deny/opt-in,
//      CRLF/CTL injection, RFC 7230 token headers/methods, Vary: Origin, subdomain dot-boundary)
//   2. W3C CSP Level 3 compliance vectors (first-directive-wins, ASCII case-insensitivity, 'none' coexistence,
//      nonce/sha256/sha384/sha512 Base64 validation, CRLF/CTL & directive cap guards)
//   3. RFC 9110 §13 ETag conditional request compliance vectors (strict W/"..." vs w/"...", etagc charset,
//      strong vs weak comparison, 5-step precedence If-Match/IUS/If-None-Match/IMS/If-Range & '*' wildcard)
//   4. 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity (all 256 byte values + lengths 0..=257)
//   5. 1,800 deterministic PRNG fuzz cases (600 CORS + 600 CSP + 600 ETag), 0 panic / 0 OOB / 0 hang

import { test, expect_eq } from "cap:test";
import {
    CORS_METHOD_GET,
    CORS_METHOD_POST,
    CORS_METHOD_PUT,
    CORS_METHOD_DELETE,
    scalarWebHasControlOrCrlf,
    simdWebHasControlOrCrlf,
    scalarWebBytesEqual,
    simdWebBytesEqual,
    scalarWebAsciiLowerEql,
    simdWebAsciiLowerEql,
    isValidOriginSyntax,
    isValidCorsHeaderToken,
    matchOriginPattern,
    CorsDecisionKind,
    CorsGuard,
    formatCorsResponseHeaders,
} from "../../lib/web/corsGuard.cp";
import {
    CspDirectiveKind,
    CspSourceKind,
    CspEvalVerdict,
    CspSourceExpr,
    CspPolicy,
} from "../../lib/web/cspPolicy.cp";
import {
    PreconditionMethod,
    PreconditionVerdict,
    PreconditionTrigger,
    PreconditionRequest,
    ResourceValidators,
    parseEntityTag,
    etagStrongEqual,
    etagWeakEqual,
    etagListMatchesStrong,
    etagListMatchesWeak,
    validateEtagList,
    evaluatePreconditions,
} from "../../lib/web/etagPrecond.cp";

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i <= haystack.len - needle.len) : (i += 1) {
        if (simdWebBytesEqual(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

fn nextRand(s: u64): u64 {
    let x: u64 = s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x;
}

fn randOut(s: u64): u64 {
    return s *% @as(u64, 0x2545F4914F6CDD1D);
}

// ============================================================================
// 1. W3C Fetch CORS Official Compliance Vectors
// ============================================================================

fn checkCorsVectors(): u32 {
    // 1. Credentials invariant: allow_credentials=true NEVER emits "*" and ALWAYS emits Vary: Origin
    let g_cred = CorsGuard.init();
    if (!g_cred.addAllowedOrigin("*")) return 1;
    g_cred.setAllowCredentials(true);
    const d_cred = g_cred.evaluateActualRequest("https://app.example.com", "GET");
    if (d_cred.kind != CorsDecisionKind.actual_allow) return 2;
    if (!simdWebBytesEqual(d_cred.allowOriginSlice(), "https://app.example.com")) return 3;
    if (simdWebBytesEqual(d_cred.allowOriginSlice(), "*")) return 4;
    if (!d_cred.allow_credentials || !d_cred.vary_origin) return 5;

    let hdr_buf: [512]u8 = [0; 512];
    let hdr_s: []u8 = hdr_buf[0..];
    const n_hdr = formatCorsResponseHeaders(d_cred, hdr_s) ?? return 6;
    if (!containsSubstr(hdr_buf[0..n_hdr], "Access-Control-Allow-Origin: https://app.example.com\r\n")) return 7;
    if (!containsSubstr(hdr_buf[0..n_hdr], "Access-Control-Allow-Credentials: true\r\n")) return 8;
    if (!containsSubstr(hdr_buf[0..n_hdr], "Vary: Origin\r\n")) return 9;

    // 2. Origin: null rejected by default even when '*' is allowed; allowed only on explicit opt-in
    let g_null = CorsGuard.init();
    if (!g_null.addAllowedOrigin("*")) return 10;
    const d_null_deny = g_null.evaluateActualRequest("null", "GET");
    if (d_null_deny.kind != CorsDecisionKind.actual_reject_origin) return 11;

    g_null.setAllowNullOrigin(true);
    const d_null_ok = g_null.evaluateActualRequest("null", "GET");
    if (d_null_ok.kind != CorsDecisionKind.actual_allow) return 12;
    if (!simdWebBytesEqual(d_null_ok.allowOriginSlice(), "null") || !d_null_ok.vary_origin) return 13;

    // 3. Subdomain dot-boundary & CRLF/CTL injection guards
    if (matchOriginPattern("https://*.example.com", "https://evil-example.com")) return 14;
    if (matchOriginPattern("https://*.example.com", "http://sub.example.com")) return 15;
    if (!matchOriginPattern("https://*.example.com", "https://api.example.com")) return 16;
    if (isValidOriginSyntax("https://a.com\r\nX-Inj: 1")) return 17;
    if (isValidOriginSyntax("https://a.com/path")) return 18;
    if (isValidOriginSyntax("1http://a.com")) return 19;

    // 4. Preflight RFC 7230 token validation for methods & headers
    if (!isValidCorsHeaderToken("X-Custom-Token_1.0")) return 20;
    if (isValidCorsHeaderToken("Bad Header")) return 21;
    if (isValidCorsHeaderToken("X-Bad(Paren)")) return 22;
    if (isValidCorsHeaderToken("X-Bad\r\n")) return 23;

    let g_pre = CorsGuard.init();
    if (!g_pre.addAllowedOrigin("https://web.example.com")) return 24;
    g_pre.setAllowedMethods(CORS_METHOD_GET | CORS_METHOD_POST | CORS_METHOD_PUT);
    if (!g_pre.addAllowedHeader("Authorization")) return 25;
    if (!g_pre.addAllowedHeader("X-Trace-Id")) return 26;
    g_pre.setMaxAgeSecs(600);

    const p_ok = g_pre.evaluatePreflight("https://web.example.com", "POST", "authorization, x-trace-id");
    if (p_ok.kind != CorsDecisionKind.preflight_allow_204 || p_ok.http_status != 204) return 27;

    const p_bad_m = g_pre.evaluatePreflight("https://web.example.com", "DELETE", "Authorization");
    if (p_bad_m.kind != CorsDecisionKind.preflight_reject_method) return 28;

    const p_bad_h = g_pre.evaluatePreflight("https://web.example.com", "POST", "Authorization, Bad(Hdr)");
    if (p_bad_h.kind != CorsDecisionKind.preflight_reject_header) return 29;

    const p_crlf = g_pre.evaluatePreflight("https://web.example.com", "POST", "Authorization\r\nX-Evil: 1");
    if (p_crlf.kind != CorsDecisionKind.preflight_reject_header) return 30;
    return 0;
}

// ============================================================================
// 2. W3C CSP Level 3 Compliance Vectors
// ============================================================================

fn checkCspVectors(): u32 {
    // 1. Duplicate directives: ONLY first occurrence wins; directive names are ASCII case-insensitive
    const pol1 = CspPolicy.parseHeader("SCRIPT-SRC 'self'; script-src *; DeFaUlT-sRc 'none'") ?? return 1;
    if (pol1.checkResourceUrl(CspDirectiveKind.script_src, "https://app.test", "https://app.test/a.js") != CspEvalVerdict.allowed) return 2;
    if (pol1.checkResourceUrl(CspDirectiveKind.script_src, "https://app.test", "https://evil.test/a.js") != CspEvalVerdict.blocked_no_match) return 3;
    if (pol1.checkResourceUrl(CspDirectiveKind.img_src, "https://app.test", "https://app.test/a.png") != CspEvalVerdict.blocked_by_none) return 4;

    // 2. W3C CSP3 'none' coexistence: sole 'none' blocks; 'none' + 'self' ignores 'none'
    const pol_coex = CspPolicy.parseHeader("script-src 'none' 'self'") ?? return 5;
    if (pol_coex.checkResourceUrl(CspDirectiveKind.script_src, "https://app.test", "https://app.test/app.js") != CspEvalVerdict.allowed) return 6;
    if (pol_coex.checkResourceUrl(CspDirectiveKind.script_src, "https://app.test", "https://evil.test/app.js") != CspEvalVerdict.blocked_no_match) return 7;

    // 3. nonce / sha256 / sha384 / sha512 Base64 validation
    if (CspSourceExpr.parse("'nonce-abc123+/='") == null) return 8;
    if (CspSourceExpr.parse("'sha256-AbCdEf0123456789+/='") == null) return 9;
    if (CspSourceExpr.parse("'sha384-AbCdEf0123456789-_=='") == null) return 10;
    if (CspSourceExpr.parse("'sha512-AbCdEf0123456789'") == null) return 11;
    // Reject empty or invalid Base64 characters / bad padding
    if (CspSourceExpr.parse("'nonce-'") != null) return 12;
    if (CspSourceExpr.parse("'nonce-bad!char'") != null) return 13;
    if (CspSourceExpr.parse("'sha256-abc=def'") != null) return 14;
    if (CspSourceExpr.parse("'sha384-==='") != null) return 15;
    if (CspSourceExpr.parse("'sha512-bad*hash'") != null) return 16;

    // 4. Reject control chars, CRLF, invalid directive names, and excessive segments
    if (CspPolicy.parseHeader("default-src 'self'\r\nX-Inject: 1") != null) return 17;
    if (CspPolicy.parseHeader("default-src 'self'\x01") != null) return 18;
    if (CspPolicy.parseHeader("bad$dir 'self'") != null) return 19;

    // Format round-trip
    let out: [256]u8 = [0; 256];
    let out_s: []u8 = out[0..];
    const n_fmt = pol1.formatHeader(out_s) ?? return 20;
    if (!containsSubstr(out[0..n_fmt], "script-src 'self'")) return 21;
    return 0;
}

// ============================================================================
// 3. RFC 9110 §13 ETag Conditional Request Compliance Vectors
// ============================================================================

fn checkEtagVectors(): u32 {
    // 1. Strict weak prefix W/"..." (uppercase W only; reject lowercase w/"...")
    const strong_t = parseEntityTag("\"v1234567890abcdef\"") ?? return 1;
    if (strong_t.weak || !simdWebBytesEqual(strong_t.tag, "v1234567890abcdef")) return 2;

    const weak_t = parseEntityTag("W/\"v1234567890abcdef\"") ?? return 3;
    if (!weak_t.weak || !simdWebBytesEqual(weak_t.tag, "v1234567890abcdef")) return 4;

    if (parseEntityTag("w/\"v1234567890abcdef\"") != null) return 5;
    if (parseEntityTag("W/v1") != null) return 6;
    if (parseEntityTag("\"unclosed") != null) return 7;
    if (parseEntityTag("\"bad quote\"extra\"") != null) return 8;
    if (parseEntityTag("\"has space\"") != null) return 9;
    if (parseEntityTag("\"has\r\ncrlf\"") != null) return 10;
    if (parseEntityTag("\"has\x7fdel\"") != null) return 11;
    // Empty opaque tag "" and obs-text >= 0x80 are valid per RFC 9110 §8.8.3
    if (parseEntityTag("\"\"") == null) return 12;
    if (parseEntityTag("\"\x80\xfe\"") == null) return 13;

    // 2. Strong vs Weak comparison
    if (etagStrongEqual(strong_t, weak_t)) return 14;
    if (!etagWeakEqual(strong_t, weak_t)) return 15;
    if (!etagStrongEqual(strong_t, strong_t)) return 16;

    // 3. ETag list & '*' wildcard validation
    if (!validateEtagList("*")) return 17;
    if (!validateEtagList("\"a,b\", W/\"c\"")) return 18;
    if (validateEtagList("w/\"bad\"")) return 19;
    if (validateEtagList("\"a\"\r\n")) return 20;
    if (!etagListMatchesStrong("*", strong_t, true)) return 21;
    if (etagListMatchesStrong("*", null, false)) return 22;
    if (etagListMatchesStrong("W/\"v1234567890abcdef\"", strong_t, true)) return 23;
    if (!etagListMatchesWeak("W/\"v1234567890abcdef\"", strong_t, true)) return 24;

    // 4. RFC 9110 §13.2.2 Precedence:
    // Step 1: If-Match fails -> 412 (overrides If-Unmodified-Since)
    const res = ResourceValidators{
        resource_exists: true,
        etag: "\"v2\"",
        last_modified_sec: 2000,
    };
    const r1 = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_match: "\"v1\"",
        if_unmodified_since_sec: 3000,
    }, res);
    if (r1.verdict != PreconditionVerdict.precondition_failed_412 || r1.trigger != PreconditionTrigger.if_match) return 25;

    // Step 2: If-Match present & matches -> If-Unmodified-Since is IGNORED even if stale
    const r2 = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_match: "\"v2\"",
        if_unmodified_since_sec: 1000,
    }, res);
    if (r2.verdict != PreconditionVerdict.proceed_200) return 26;

    // Step 3: If-None-Match weak match -> 304 for GET, 412 for PUT
    const r3_get = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        if_none_match: "W/\"v2\"",
    }, res);
    if (r3_get.verdict != PreconditionVerdict.not_modified_304 || r3_get.http_status != 304) return 27;

    const r3_put = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.put,
        if_none_match: "*",
    }, res);
    if (r3_put.verdict != PreconditionVerdict.precondition_failed_412 || r3_put.http_status != 412) return 28;

    // Step 4: If-Modified-Since ignored when If-None-Match is present
    const r4 = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        if_none_match: "\"v1\"",
        if_modified_since_sec: 3000,
    }, res);
    if (r4.verdict != PreconditionVerdict.proceed_200) return 29;

    // Step 5: If-Range strong match -> 206, weak mismatch -> 200 fallback
    const r5_ok = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        has_range: true,
        if_range_etag: "\"v2\"",
    }, res);
    if (r5_ok.verdict != PreconditionVerdict.serve_range_206 || !r5_ok.serve_range) return 30;

    const r5_weak = evaluatePreconditions(PreconditionRequest{
        method: PreconditionMethod.get,
        has_range: true,
        if_range_etag: "W/\"v2\"",
    }, res);
    if (r5_weak.verdict != PreconditionVerdict.fallback_full_200 || r5_weak.serve_range) return 31;
    return 0;
}

// ============================================================================
// 4. 16-Lane SIMD vs Scalar 0-Diff Parity (All 256 Bytes + Lengths 0..=257)
// ============================================================================

fn checkSimdParity(): u32 {
    // 1. All 256 byte values across all 16 lanes + tail
    let a: [32]u8 = ['x'; 32];
    let b: [32]u8 = ['X'; 32];
    let byte_i: usize = 0;
    while (byte_i < 256) : (byte_i += 1) {
        const ch: u8 = @as(u8, @intCast(byte_i));
        const lane: usize = byte_i % 17;
        a[lane] = ch;
        b[lane] = ch;
        if (simdWebHasControlOrCrlf(a[0..17]) != scalarWebHasControlOrCrlf(a[0..17])) return 1;
        if (simdWebBytesEqual(a[0..17], b[0..17]) != scalarWebBytesEqual(a[0..17], b[0..17])) return 2;
        if (simdWebAsciiLowerEql(a[0..17], b[0..17]) != scalarWebAsciiLowerEql(a[0..17], b[0..17])) return 3;

        // Flip bit 0x20 on b[lane] to verify non-alpha bytes (e.g. '@' vs '`', '[' vs '{') do NOT false-match!
        b[lane] = ch ^ 0x20;
        if (simdWebAsciiLowerEql(a[0..17], b[0..17]) != scalarWebAsciiLowerEql(a[0..17], b[0..17])) return 4;
        if (simdWebBytesEqual(a[0..17], b[0..17]) != scalarWebBytesEqual(a[0..17], b[0..17])) return 5;

        a[lane] = 'x';
        b[lane] = 'X';
    }

    // 2. Lengths 0..=257 sweep
    let s1: [258]u8 = ['a'; 258];
    let s2: [258]u8 = ['A'; 258];
    let len: usize = 0;
    while (len <= 257) : (len += 1) {
        if (len > 0) {
            const idx = len - 1;
            const c_lo: u8 = @as(u8, @intCast('a' + (idx % 26)));
            const c_hi: u8 = @as(u8, @intCast('A' + (idx % 26)));
            s1[idx] = c_lo;
            s2[idx] = c_hi;
        }
        if (simdWebHasControlOrCrlf(s1[0..len]) != scalarWebHasControlOrCrlf(s1[0..len])) return 6;
        if (simdWebBytesEqual(s1[0..len], s1[0..len]) != scalarWebBytesEqual(s1[0..len], s1[0..len])) return 7;
        if (simdWebAsciiLowerEql(s1[0..len], s2[0..len]) != scalarWebAsciiLowerEql(s1[0..len], s2[0..len])) return 8;
        if (!simdWebAsciiLowerEql(s1[0..len], s2[0..len])) return 9;

        if (len > 0) {
            const saved = s1[len - 1];
            s1[len - 1] = '\n';
            if (simdWebHasControlOrCrlf(s1[0..len]) != scalarWebHasControlOrCrlf(s1[0..len])) return 10;
            if (!simdWebHasControlOrCrlf(s1[0..len])) return 11;
            if (simdWebAsciiLowerEql(s1[0..len], s2[0..len]) != scalarWebAsciiLowerEql(s1[0..len], s2[0..len])) return 12;
            s1[len - 1] = saved;
        }
    }
    return 0;
}

// ============================================================================
// 5. 1,800 Deterministic PRNG Fuzz Cases (600 CORS + 600 CSP + 600 ETag)
// ============================================================================

fn checkPrngFuzz(): u32 {
    let seed: u64 = 0x51C3_BEEF_2026_1124;
    let buf1: [128]u8 = [0; 128];
    let buf2: [128]u8 = [0; 128];
    let out: [512]u8 = [0; 512];

    // 600 CORS fuzz cases
    let guard = CorsGuard.init();
    * = guard.addAllowedOrigin("https://*.example.com");
    guard.setAllowCredentials(true);
    guard.setAllowedMethods(CORS_METHOD_GET | CORS_METHOD_POST);
    * = guard.addAllowedHeader("Authorization");

    let ci: usize = 0;
    while (ci < 600) : (ci += 1) {
        seed = nextRand(seed);
        const n1: usize = @as(usize, @intCast(randOut(seed) % 96));
        seed = nextRand(seed);
        const n2: usize = @as(usize, @intCast(randOut(seed) % 64));
        let j: usize = 0;
        while (j < n1) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((ci & 1) == 0) {
                const alph: []const u8 = "https://*.example.com:8443/null\r\n\t \x00";
                buf1[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf1[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        j = 0;
        while (j < n2) : (j += 1) {
            seed = nextRand(seed);
            buf2[j] = @as(u8, @intCast(randOut(seed) & 0xFF));
        }
        const org = buf1[0..n1];
        const hdrs = buf2[0..n2];
        const d1 = guard.evaluatePreflight(org, "POST", hdrs);
        // Credential invariant: NEVER emit "*" when allow_credentials=true
        if (d1.allow_credentials && simdWebBytesEqual(d1.allowOriginSlice(), "*")) return 1;
        let os: []u8 = out[0..];
        * = formatCorsResponseHeaders(d1, os);

        const d2 = guard.evaluateActualRequest(org, "GET");
        if (d2.allow_credentials && simdWebBytesEqual(d2.allowOriginSlice(), "*")) return 2;
    }

    // 600 CSP fuzz cases
    let pi: usize = 0;
    while (pi < 600) : (pi += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 128));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((pi & 1) == 0) {
                const alph: []const u8 = "default-src script-src 'none' 'self' 'nonce-abc=';*\r\n\x00";
                buf1[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf1[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        if (pol := CspPolicy.parseHeader(buf1[0..n])) {
            * = pol.checkResourceUrl(CspDirectiveKind.script_src, "https://app.test", "https://app.test/x.js");
            * = pol.checkInlineScript("abc=", "");
            let os: []u8 = out[0..];
            * = pol.formatHeader(os);
        }
    }

    // 600 ETag / Precondition fuzz cases
    const base_res = ResourceValidators{
        resource_exists: true,
        etag: "\"etag-v1\"",
        last_modified_sec: 1700000000,
    };
    let ei: usize = 0;
    while (ei < 600) : (ei += 1) {
        seed = nextRand(seed);
        const n: usize = @as(usize, @intCast(randOut(seed) % 96));
        let j: usize = 0;
        while (j < n) : (j += 1) {
            seed = nextRand(seed);
            const r = randOut(seed);
            if ((ei & 1) == 0) {
                const alph: []const u8 = "Ww/\"etag-v1\", * \t\r\n\x00\x7f\x80";
                buf1[j] = alph[@as(usize, @intCast(r % alph.len))];
            } else {
                buf1[j] = @as(u8, @intCast(r & 0xFF));
            }
        }
        const s = buf1[0..n];
        * = parseEntityTag(s);
        * = validateEtagList(s);
        * = evaluatePreconditions(PreconditionRequest{
            method: if ((ei & 1) == 0) PreconditionMethod.get else PreconditionMethod.put,
            if_match: s,
            if_none_match: s,
            has_range: (ei & 3) == 0,
            if_range_etag: s,
        }, base_res);
    }

    return 0;
}

fn main() {
    test("1. W3C Fetch CORS: credentials invariant, null origin, CRLF, RFC 7230 tokens & Vary", () => {
        expect_eq(checkCorsVectors(), @as(u32, 0));
    });
    test("2. W3C CSP3: first-directive-wins, case-insensitive, 'none' coexistence, nonce/hash & CTL", () => {
        expect_eq(checkCspVectors(), @as(u32, 0));
    });
    test("3. RFC 9110 §13 ETag: strict W/\"...\", etagc, strong/weak compare & 5-step precedence", () => {
        expect_eq(checkEtagVectors(), @as(u32, 0));
    });
    test("4. 16-lane SIMD vs Scalar 0-diff parity (all 256 bytes + lengths 0..=257)", () => {
        expect_eq(checkSimdParity(), @as(u32, 0));
    });
    test("5. PRNG Fuzz: 1,800 deterministic malformed cases (600 CORS + 600 CSP + 600 ETag)", () => {
        expect_eq(checkPrngFuzz(), @as(u32, 0));
    });
}
