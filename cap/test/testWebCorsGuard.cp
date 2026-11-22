// Origin: Pure-CP Stdlib Test Suite (lib/web/corsGuard.cp) | Created: 2026-11-22
// Production Deep Verification Suite for W3C Fetch CORS Guard & Header Synthesizer:
//   1. Safe wildcard subdomain matching (scheme + dot-boundary anti-bypass verification)
//   2. W3C Fetch credential invariant: never reflect `*` when allow_credentials=true + Vary: Origin
//   3. OPTIONS preflight 4-way verdicts (allow_204, reject_origin, reject_method, reject_header)
//   4. Actual request evaluation (not_cors_request, actual_allow + Expose-Headers, actual_reject_origin)
//   5. Wire-ready HTTP CORS response header synthesis exact verification
//   6. CRLF / control-char header-splitting injection guards across all entry points
//   7. Undersized formatCorsResponseHeaders buffer guard (returns null safely)
//   8. Full-table capacity guards (32 origins, 32 allowed headers, 32 exposed headers) & overlong rejection

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CORS_MAX_ORIGINS,
    CORS_MAX_HEADERS,
    CORS_METHOD_GET,
    CORS_METHOD_POST,
    CORS_METHOD_PUT,
    CORS_METHOD_DELETE,
    hasControlOrCrlf,
    isValidOriginSyntax,
    matchOriginPattern,
    CorsDecisionKind,
    CorsGuard,
    formatCorsResponseHeaders,
} from "cap:web";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (strEq(haystack[i..(i + needle.len)], needle)) {
            return true;
        }
    }
    return false;
}

test("corsGuard: safe wildcard subdomain matching blocks dot-boundary & scheme bypasses", () => {
    const pat: []const u8 = "https://*.example.com";

    // Valid subdomains
    expect_eq(matchOriginPattern(pat, "https://api.example.com"), true);
    expect_eq(matchOriginPattern(pat, "https://v2.api.example.com"), true);
    expect_eq(matchOriginPattern(pat, "HTTPS://APP.EXAMPLE.COM"), true);

    // Must reject sibling domain without dot boundary, bare apex, scheme downgrade, malformed syntax
    expect_eq(matchOriginPattern(pat, "https://evil-example.com"), false);
    expect_eq(matchOriginPattern(pat, "https://example.com"), false);
    expect_eq(matchOriginPattern(pat, "http://api.example.com"), false);
    expect_eq(matchOriginPattern(pat, "https://.example.com"), false);
    expect_eq(matchOriginPattern(pat, "https://api.example.com/path"), false);
    expect_eq(matchOriginPattern(pat, "https://evil.com@.example.com"), false);
    expect_eq(matchOriginPattern(pat, "https://api.example.com:8443"), false);

    // Port-qualified wildcard matches only that exact port
    expect_eq(matchOriginPattern("https://*.example.com:8443", "https://api.example.com:8443"), true);
    expect_eq(matchOriginPattern("https://*.example.com:8443", "https://api.example.com:9000"), false);
});

test("corsGuard: W3C Fetch credential invariant never reflects * when allow_credentials=true", () => {
    // Case A: wildcard '*' with allow_credentials=false -> emits '*' and no Vary: Origin
    let open_guard = CorsGuard.init();
    expect_eq(open_guard.addAllowedOrigin("*"), true);
    open_guard.setAllowCredentials(false);
    const d_open = open_guard.evaluateActualRequest("https://client.io", "GET");
    expect_eq(d_open.kind, CorsDecisionKind.actual_allow);
    expect_eq(strEq(d_open.allowOriginSlice(), "*"), true);
    expect_eq(d_open.vary_origin, false);

    // Case B: wildcard '*' with allow_credentials=true -> MUST echo exact Origin + Vary: Origin!
    let cred_guard = CorsGuard.init();
    expect_eq(cred_guard.addAllowedOrigin("*"), true);
    cred_guard.setAllowCredentials(true);
    const d_cred = cred_guard.evaluateActualRequest("https://client.io", "GET");
    expect_eq(d_cred.kind, CorsDecisionKind.actual_allow);
    expect_eq(strEq(d_cred.allowOriginSlice(), "https://client.io"), true);
    expect_eq(d_cred.allow_credentials, true);
    expect_eq(d_cred.vary_origin, true);
});

test("corsGuard: OPTIONS preflight 4-way verdicts (allow_204, reject origin/method/header)", () => {
    let guard = CorsGuard.init();
    expect_eq(guard.addAllowedOrigin("https://*.app.dev"), true);
    guard.setAllowedMethods(CORS_METHOD_GET | CORS_METHOD_POST | CORS_METHOD_PUT);
    expect_eq(guard.addAllowedHeader("Authorization"), true);
    expect_eq(guard.addAllowedHeader("Content-Type"), true);
    guard.setMaxAgeSecs(3600);

    // 1. Allowed preflight (case-insensitive headers with spaces)
    const ok = guard.evaluatePreflight("https://web.app.dev", "POST", "authorization, content-type");
    expect_eq(ok.kind, CorsDecisionKind.preflight_allow_204);
    expect_eq(ok.http_status, 204 as u16);
    expect_eq(ok.isAllowed(), true);
    expect_eq(strEq(ok.allowOriginSlice(), "https://web.app.dev"), true);

    // 2. Rejected origin
    const bad_org = guard.evaluatePreflight("https://evil-app.dev", "POST", "Authorization");
    expect_eq(bad_org.kind, CorsDecisionKind.preflight_reject_origin);
    expect_eq(bad_org.http_status, 403 as u16);
    expect_eq(bad_org.isAllowed(), false);

    // 3. Rejected method (DELETE not in allowed mask)
    const bad_m = guard.evaluatePreflight("https://web.app.dev", "DELETE", "Authorization");
    expect_eq(bad_m.kind, CorsDecisionKind.preflight_reject_method);
    expect_eq(bad_m.http_status, 403 as u16);

    // 4. Rejected header (X-Secret not whitelisted)
    const bad_h = guard.evaluatePreflight("https://web.app.dev", "POST", "Authorization, X-Secret");
    expect_eq(bad_h.kind, CorsDecisionKind.preflight_reject_header);
    expect_eq(bad_h.http_status, 403 as u16);
});

test("corsGuard: actual request evaluation & Access-Control-Expose-Headers", () => {
    let guard = CorsGuard.init();
    expect_eq(guard.addAllowedOrigin("https://console.example.com"), true);
    guard.setAllowedMethods(CORS_METHOD_GET | CORS_METHOD_POST);
    expect_eq(guard.addExposedHeader("X-Request-Id"), true);
    expect_eq(guard.addExposedHeader("X-RateLimit-Remaining"), true);

    // Same-origin / non-CORS request (no Origin header)
    const non_cors = guard.evaluateActualRequest("", "GET");
    expect_eq(non_cors.kind, CorsDecisionKind.not_cors_request);
    expect_eq(non_cors.isAllowed(), true);
    expect_eq(non_cors.allow_origin_len, 0 as usize);

    // Allowed cross-origin request with exposed headers
    const allowed = guard.evaluateActualRequest("https://console.example.com", "GET");
    expect_eq(allowed.kind, CorsDecisionKind.actual_allow);
    expect_eq(allowed.http_status, 200 as u16);
    expect_eq(strEq(allowed.exposeHeadersSlice(), "X-Request-Id, X-RateLimit-Remaining"), true);

    // Rejected origin on actual request
    const rejected = guard.evaluateActualRequest("https://other.example.com", "GET");
    expect_eq(rejected.kind, CorsDecisionKind.actual_reject_origin);
    expect_eq(rejected.http_status, 403 as u16);
});

test("corsGuard: formatCorsResponseHeaders wire-ready CRLF header synthesis", () => {
    let guard = CorsGuard.init();
    expect_eq(guard.addAllowedOrigin("https://*.example.com"), true);
    guard.setAllowedMethods(CORS_METHOD_GET | CORS_METHOD_POST | CORS_METHOD_DELETE);
    expect_eq(guard.addAllowedHeader("Authorization"), true);
    expect_eq(guard.addExposedHeader("X-Trace-Id"), true);
    guard.setAllowCredentials(true);
    guard.setMaxAgeSecs(600);

    // Preflight header synthesis
    const pf = guard.evaluatePreflight("https://api.example.com", "POST", "Authorization");
    let buf: [512]u8 = [0; 512];
    let buf_s: []u8 = buf[0..];
    const n = formatCorsResponseHeaders(pf, *buf_s) ?? unreachable;
    const wire: []const u8 = buf[0..n];

    expect_eq(containsSubstr(wire, "Access-Control-Allow-Origin: https://api.example.com\r\n"), true);
    expect_eq(containsSubstr(wire, "Access-Control-Allow-Credentials: true\r\n"), true);
    expect_eq(containsSubstr(wire, "Access-Control-Allow-Methods: GET, POST, DELETE\r\n"), true);
    expect_eq(containsSubstr(wire, "Access-Control-Allow-Headers: Authorization\r\n"), true);
    expect_eq(containsSubstr(wire, "Access-Control-Max-Age: 600\r\n"), true);
    expect_eq(containsSubstr(wire, "Vary: Origin, Access-Control-Request-Method, Access-Control-Request-Headers\r\n"), true);

    // Actual request header synthesis
    const act = guard.evaluateActualRequest("https://api.example.com", "GET");
    const n2 = formatCorsResponseHeaders(act, *buf_s) ?? unreachable;
    const wire2: []const u8 = buf[0..n2];
    expect_eq(containsSubstr(wire2, "Access-Control-Expose-Headers: X-Trace-Id\r\n"), true);
    expect_eq(containsSubstr(wire2, "Vary: Origin\r\n"), true);
});

test("corsGuard: CRLF header-splitting & malformed syntax injection guards", () => {
    expect_eq(hasControlOrCrlf("normal-value"), false);
    expect_eq(hasControlOrCrlf("evil\r\nSet-Cookie: a=1"), true);
    expect_eq(isValidOriginSyntax("https://app.example.com\r\nX-Evil: 1"), false);
    expect_eq(isValidOriginSyntax("not-an-origin"), false);
    expect_eq(isValidOriginSyntax(""), false);

    let guard = CorsGuard.init();
    expect_eq(guard.addAllowedOrigin("https://ok.com\r\nX-Inject: 1"), false);
    expect_eq(guard.addAllowedOrigin("https://*.com"), true);
    expect_eq(guard.addAllowedOrigin("https://*..com"), false);
    expect_eq(guard.addAllowedHeader("X-Bad\r\nHeader"), false);
    expect_eq(guard.addExposedHeader("X-Bad:Colon"), false);

    // CRLF in preflight Origin or req_headers_csv must be rejected cleanly
    expect_eq(guard.addAllowedHeader("*"), true);
    const r1 = guard.evaluatePreflight("https://a.com\r\nInject: 1", "GET", "X-Ok");
    expect_eq(r1.kind, CorsDecisionKind.preflight_reject_origin);
    const r2 = guard.evaluatePreflight("https://a.com", "GET", "X-Ok\r\nInject: 1");
    expect_eq(r2.kind, CorsDecisionKind.preflight_reject_header);
});

test("corsGuard: undersized formatCorsResponseHeaders buffer returns null safely", () => {
    let guard = CorsGuard.init();
    * = guard.addAllowedOrigin("https://app.example.com");
    const decision = guard.evaluateActualRequest("https://app.example.com", "GET");

    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_eq(formatCorsResponseHeaders(decision, *tiny_s) == null, true);
});

test("corsGuard: full rule-table capacity guards (32 origins & 32 headers)", () => {
    let guard = CorsGuard.init();
    let i: usize = 0;
    while (i < CORS_MAX_ORIGINS) : (i += 1) {
        expect_eq(guard.addAllowedOrigin("https://ok.example.com"), true);
    }
    expect_eq(guard.addAllowedOrigin("https://overflow.example.com"), false);

    i = 0;
    while (i < CORS_MAX_HEADERS) : (i += 1) {
        expect_eq(guard.addAllowedHeader("X-Custom"), true);
        expect_eq(guard.addExposedHeader("X-Expose"), true);
    }
    expect_eq(guard.addAllowedHeader("X-Overflow"), false);
    expect_eq(guard.addExposedHeader("X-Overflow"), false);
});
