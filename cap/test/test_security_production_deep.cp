// cap/test/test_security_production_deep.cp
// Deep production-grade test suite for cap:security (Base32, HOTP, TOTP, CORS, CSP, SecurityHeadersPreset, CSRF)

import { test, expect_eq } from "cap:test";
const std = @import("std");

import {
    CSRF,
    TokenFormat,
    DEFAULT_EXPIRATION_MS,
    constantTimeEql,
    Base32,
    OtpAlgorithm,
    Hotp,
    TotpConfig,
    Totp,
    CorsHeader,
    CorsDecisionKind,
    CorsRejectReason,
    CorsConfig,
    CorsPolicy,
    SecurityHeadersPreset,
    CspBuilder,
} from "../../lib/security/index.cp";

fn expectBase32DecodeFail(alloc: std.mem.Allocator, input: []const u8): bool {
    const v = Base32.decode(alloc, input) catch return true;
    alloc.free(v);
    return false;
}

fn expectCspBuildFail(builder: *CspBuilder, alloc: std.mem.Allocator): bool {
    const v = builder.build(alloc) catch return true;
    alloc.free(v);
    return false;
}

fn testBase32Rfc4648AndFlexibleDecode(): void! {
    const alloc = std.heap.page_allocator;

    // RFC 4648 Section 10 test vectors
    const inputs: [7][]const u8 = ["", "f", "fo", "foo", "foob", "fooba", "foobar"];
    const expected: [7][]const u8 = [
        "",
        "MY======",
        "MZXQ====",
        "MZXW6===",
        "MZXW6YQ=",
        "MZXW6YTB",
        "MZXW6YTBOI======",
    ];

    for (i of 0..7) {
        const enc = Base32.encode(alloc, inputs[i])?;
        defer alloc.free(enc);
        expect_eq(std.mem.eql(u8, enc, expected[i]), true);

        const decoded = Base32.decode(alloc, enc)?;
        defer alloc.free(decoded);
        expect_eq(std.mem.eql(u8, decoded, inputs[i]), true);
    }

    // Flexible decoding: lowercase, spaces, optional '=' padding
    const decoded_flex = Base32.decode(alloc, " mzxw 6ytb oi ")?;
    defer alloc.free(decoded_flex);
    expect_eq(std.mem.eql(u8, decoded_flex, "foobar"), true);

    // Unpadded encoding roundtrip
    const enc_nopad = Base32.encodeNoPad(alloc, "foobar")?;
    defer alloc.free(enc_nopad);
    expect_eq(std.mem.eql(u8, enc_nopad, "MZXW6YTBOI"), true);

    // Invalid Base32 characters must fail
    expect_eq(expectBase32DecodeFail(alloc, "MZXW0189"), true);
}

fn testHotpAndTotpRfcVectors(): void! {
    const alloc = std.heap.page_allocator;

    // RFC 4226 Appendix D test vectors (SHA-1, 6 digits)
    const hotp_secret = "12345678901234567890";
    const hotp_expected: [10]u32 = [
        755224,
        287082,
        359152,
        969429,
        338314,
        254676,
        287922,
        162583,
        399871,
        520489,
    ];
    for (c of 0..10) {
        const got = Hotp.generate(hotp_secret, u64(c), 6, .Sha1)?;
        expect_eq(got, hotp_expected[c]);
    }

    // RFC 6238 Appendix B test vectors (period=30, digits=8)
    const timestamps: [6]u64 = [59, 1111111109, 1111111111, 1234567890, 2000000000, 20000000000];

    const sha1_vals: [6]u32 = [94287082, 7081804, 14050471, 89005924, 69279037, 65353130];
    const sha256_secret = "12345678901234567890123456789012";
    const sha256_vals: [6]u32 = [46119246, 68084774, 67062674, 91819424, 90698825, 77737706];
    const sha512_secret = "1234567890123456789012345678901234567890123456789012345678901234";
    const sha512_vals: [6]u32 = [90693936, 25091201, 99943326, 93441116, 38618901, 47863826];

    const cfg_sha1 = TotpConfig{ period_sec: 30, digits: 8, alg: .Sha1 };
    const cfg_sha256 = TotpConfig{ period_sec: 30, digits: 8, alg: .Sha256 };
    const cfg_sha512 = TotpConfig{ period_sec: 30, digits: 8, alg: .Sha512 };

    for (i of 0..6) {
        expect_eq(Totp.generateAt(hotp_secret, timestamps[i], cfg_sha1)?, sha1_vals[i]);
        expect_eq(Totp.generateAt(sha256_secret, timestamps[i], cfg_sha256)?, sha256_vals[i]);
        expect_eq(Totp.generateAt(sha512_secret, timestamps[i], cfg_sha512)?, sha512_vals[i]);
    }

    // Leading-zero formatting & constant-time verifyAt with skew window
    let code_buf: [10]u8 = undefined;
    const formatted = Totp.generateCodeAt(hotp_secret, 1111111109, cfg_sha1, &code_buf)?;
    expect_eq(std.mem.eql(u8, formatted, "07081804"), true);

    expect_eq(Totp.verifyAt(hotp_secret, "07081804", 1111111109, 0, cfg_sha1), true);
    // 1 step later (1111111109 + 30 = 1111111139): fails with skew=0, passes with skew=1
    expect_eq(Totp.verifyAt(hotp_secret, "07081804", 1111111139, 0, cfg_sha1), false);
    expect_eq(Totp.verifyAt(hotp_secret, "07081804", 1111111139, 1, cfg_sha1), true);
    // Wrong code or wrong length must fail
    expect_eq(Totp.verifyAt(hotp_secret, "07081805", 1111111109, 1, cfg_sha1), false);
    expect_eq(Totp.verifyAt(hotp_secret, "7081804", 1111111109, 1, cfg_sha1), false);

    // otpauth:// URI builder
    const uri = Totp.buildOtpAuthUri(
        alloc,
        "CP Cloud",
        "alice@example.com",
        "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ",
        TotpConfig{ period_sec: 30, digits: 6, alg: .Sha256 },
    )?;
    defer alloc.free(uri);
    expect_eq(
        std.mem.eql(
            u8,
            uri,
            "otpauth://totp/CP%20Cloud:alice%40example.com?secret=GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ&issuer=CP%20Cloud&algorithm=SHA256&digits=6&period=30",
        ),
        true,
    );
}

fn testCorsPolicyAndSecurityGuards(): void! {
    // 1. Guard: allow_credentials=true + allow_any_origin=true must be strictly rejected
    const bad_cfg_any = CorsConfig{
        allow_credentials: true,
        allow_any_origin: true,
    };
    const bad_policy_any = CorsPolicy.init(bad_cfg_any);
    const bad_res_any = bad_policy_any.evaluate("https://app.example.com", "GET", []);
    expect_eq(bad_res_any.allowed, false);
    expect_eq(bad_res_any.kind == CorsDecisionKind.Rejected, true);
    expect_eq(bad_res_any.reject_reason == CorsRejectReason.InvalidConfigCredentialsWithWildcard, true);
    expect_eq(bad_res_any.status_code, 403);

    // 2. Guard: allow_credentials=true + "*" in allowed_origins must also be strictly rejected
    const star_origins: [1][]const u8 = ["*"];
    const bad_policy_star = CorsPolicy.init(CorsConfig{
        allowed_origins: &star_origins,
        allow_credentials: true,
    });
    const bad_res_star = bad_policy_star.evaluate("https://app.example.com", "GET", []);
    expect_eq(bad_res_star.allowed, false);
    expect_eq(bad_res_star.reject_reason == CorsRejectReason.InvalidConfigCredentialsWithWildcard, true);

    // 3. Exact + safe subdomain matching with preflight and simple headers
    const origins: [2][]const u8 = ["https://app.cp-lang.org", "https://*.trusted.cp-lang.org"];
    const methods: [3][]const u8 = ["GET", "POST", "DELETE"];
    const req_hdrs: [2][]const u8 = ["Authorization", "Content-Type"];
    const exp_hdrs: [1][]const u8 = ["X-Request-Id"];
    const policy = CorsPolicy.init(CorsConfig{
        allowed_origins: &origins,
        allowed_methods: &methods,
        allowed_headers: &req_hdrs,
        exposed_headers: &exp_hdrs,
        allow_credentials: true,
        max_age_sec: 3600,
        allow_any_origin: false,
    });

    // Safe subdomain match + preflight
    const pre_req: [1][]const u8 = ["Authorization"];
    const preflight = policy.evaluate("https://api.v1.trusted.cp-lang.org", "OPTIONS", &pre_req);
    expect_eq(preflight.allowed, true);
    expect_eq(preflight.is_preflight, true);
    expect_eq(preflight.status_code, 204);
    expect_eq(std.mem.eql(u8, preflight.getHeader("Access-Control-Allow-Origin").?, "https://api.v1.trusted.cp-lang.org"), true);
    expect_eq(std.mem.eql(u8, preflight.getHeader("Access-Control-Allow-Credentials").?, "true"), true);
    expect_eq(std.mem.eql(u8, preflight.getHeader("Access-Control-Allow-Methods").?, "GET, POST, DELETE"), true);
    expect_eq(std.mem.eql(u8, preflight.getHeader("Access-Control-Allow-Headers").?, "Authorization, Content-Type"), true);
    expect_eq(std.mem.eql(u8, preflight.getHeader("Access-Control-Max-Age").?, "3600"), true);

    // Simple request with exposed headers
    const simple = policy.evaluate("https://app.cp-lang.org", "POST", []);
    expect_eq(simple.allowed, true);
    expect_eq(simple.status_code, 200);
    expect_eq(std.mem.eql(u8, simple.getHeader("Access-Control-Expose-Headers").?, "X-Request-Id"), true);

    // Lookalike & scheme downgrade & userinfo spoofing must all be rejected
    expect_eq(policy.evaluate("https://evil-trusted.cp-lang.org", "GET", []).allowed, false);
    expect_eq(policy.evaluate("http://api.trusted.cp-lang.org", "GET", []).allowed, false);
    expect_eq(policy.evaluate("https://evil.com@app.cp-lang.org", "GET", []).allowed, false);
    expect_eq(policy.evaluate("https://trusted.cp-lang.org", "GET", []).allowed, false);

    // Disallowed preflight header must be rejected
    const bad_hdr: [1][]const u8 = ["X-Evil-Header"];
    expect_eq(policy.evaluate("https://app.cp-lang.org", "OPTIONS", &bad_hdr).allowed, false);
}

fn testCspBuilderAndSecurityPresets(): void! {
    const alloc = std.heap.page_allocator;

    // 1. Chainable CspBuilder + nonces + sha256
    let builder = CspBuilder.init();
    * = builder
        .defaultSrc("'self'")
        .scriptSrc("'self'")
        .addNonce("script-src", "r4nd0mN0nc3")
        .addSha256("script-src", "47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=")
        .styleSrc("'self'")
        .imgSrc("'self' data:")
        .connectSrc("'self' https://api.cp-lang.org")
        .frameAncestors("'none'");

    expect_eq(builder.isValid(), true);
    const csp_header = builder.build(alloc)?;
    defer alloc.free(csp_header);

    const expected_csp = "default-src 'self'; script-src 'self' 'nonce-r4nd0mN0nc3' 'sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU='; style-src 'self'; img-src 'self' data:; connect-src 'self' https://api.cp-lang.org; frame-ancestors 'none'";
    expect_eq(std.mem.eql(u8, csp_header, expected_csp), true);

    // 2. CRLF & semicolon injection protection
    let crlf_builder = CspBuilder.init();
    * = crlf_builder.defaultSrc("'self'\r\nX-Injected: evil");
    expect_eq(crlf_builder.isValid(), false);
    expect_eq(expectCspBuildFail(&crlf_builder, alloc), true);

    let nonce_inject_builder = CspBuilder.init();
    * = nonce_inject_builder.addNonce("script-src", "abc';script-src 'unsafe-inline");
    expect_eq(nonce_inject_builder.isValid(), false);

    // 3. SecurityHeadersPreset strictApi & strictWeb
    const api_hdrs = SecurityHeadersPreset.strictApi();
    expect_eq(api_hdrs.count() >= 5, true);
    expect_eq(std.mem.eql(u8, api_hdrs.get("Strict-Transport-Security").?, "max-age=63072000; includeSubDomains; preload"), true);
    expect_eq(std.mem.eql(u8, api_hdrs.get("X-Content-Type-Options").?, "nosniff"), true);
    expect_eq(std.mem.eql(u8, api_hdrs.get("X-Frame-Options").?, "DENY"), true);
    expect_eq(std.mem.eql(u8, api_hdrs.get("Referrer-Policy").?, "no-referrer"), true);
    expect_eq(api_hdrs.get("Permissions-Policy") != null, true);

    const web_hdrs = SecurityHeadersPreset.strictWeb();
    expect_eq(web_hdrs.count() >= 5, true);
    expect_eq(std.mem.eql(u8, web_hdrs.get("Referrer-Policy").?, "strict-origin-when-cross-origin"), true);
    expect_eq(std.mem.eql(u8, web_hdrs.get("X-Content-Type-Options").?, "nosniff"), true);
}

fn testExistingCsrfZeroRegression(): void! {
    const alloc = std.heap.page_allocator;
    const secret = "csrf_zero_regression_secret_2026";

    expect_eq(constantTimeEql("abc123", "abc123"), true);
    expect_eq(constantTimeEql("abc123", "abc124"), false);

    const token = CSRF.generate(secret, DEFAULT_EXPIRATION_MS, TokenFormat.Base64Url, alloc)?;
    defer alloc.free(token);
    expect_eq(CSRF.verify(token, secret, DEFAULT_EXPIRATION_MS, TokenFormat.Base64Url, alloc), true);
    expect_eq(CSRF.verify(token, "wrong_secret", DEFAULT_EXPIRATION_MS, TokenFormat.Base64Url, alloc), false);
}

fn main() {
    test("RFC 4648 Base32 encode/decode & flexible input handling", () => {
        testBase32Rfc4648AndFlexibleDecode()?;
    });

    test("RFC 4226 HOTP & RFC 6238 TOTP official test vectors + otpauth URI", () => {
        testHotpAndTotpRfcVectors()?;
    });

    test("Fetch CORS Policy credentials/wildcard guard & safe subdomain matching", () => {
        testCorsPolicyAndSecurityGuards()?;
    });

    test("CspBuilder chaining, CRLF/injection guard & SecurityHeadersPreset", () => {
        testCspBuilderAndSecurityPresets()?;
    });

    test("Existing CSRF & constantTimeEql zero-regression check", () => {
        testExistingCsrfZeroRegression()?;
    });
}
