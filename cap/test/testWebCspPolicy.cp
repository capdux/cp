// Origin: Pure-CP Stdlib Test Suite (lib/web/cspPolicy.cp) | Created: 2026-11-23
// Production Deep Verification Suite for W3C Content-Security-Policy Level 3 Engine:
//   1. Multi-directive parsing & W3C first-directive-wins duplicate handling
//   2. W3C fallback chain (`default-src` for fetch directives vs non-fallback `frame-ancestors`/`base-uri`/`form-action`)
//   3. Resource URL matching: `'none'`, `'self'`, `*`, scheme, exact host & strict `*.example.com` wildcard
//   4. CSP3 inline script rules: `nonce` / `sha256` / `'strict-dynamic'` suppressing `'unsafe-inline'`
//   5. CRLF & control-character header injection rejection
//   6. Builder API, `formatHeader` round-trip, small-buffer guard & `cap.web` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CspDirectiveKind,
    CspEvalVerdict,
    CspPolicy,
} from "cap:web";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("CspPolicy: multi-directive parsing & W3C first-directive-wins duplicate rule") {
    const hdr = "default-src 'none'; script-src 'self'; script-src *; img-src https: data:; upgrade-insecure-requests";
    const pol = CspPolicy.parseHeader(hdr) ?? return expect_true(false);

    expect_true(pol.upgrade_insecure);
    // Duplicate `script-src *` must be ignored -> only `'self'` applies!
    expect_eq(
        pol.checkResourceUrl(CspDirectiveKind.script_src, "https://app.example.com", "https://app.example.com/main.js"),
        CspEvalVerdict.allowed,
    );
    expect_eq(
        pol.checkResourceUrl(CspDirectiveKind.script_src, "https://app.example.com", "https://evil.org/x.js"),
        CspEvalVerdict.blocked_no_match,
    );
    // img-src allows https: and data:
    expect_eq(
        pol.checkResourceUrl(CspDirectiveKind.img_src, "https://app.example.com", "data:image/png;base64,AAA"),
        CspEvalVerdict.allowed,
    );
}

test("CspPolicy: W3C fallback chain (fetch directives vs non-fallback base-uri/form-action/frame-ancestors)") {
    const pol = CspPolicy.parseHeader("default-src 'none'") ?? return expect_true(false);

    // Fetch directives fall back to default-src ('none') -> blocked_by_none
    expect_true(pol.effectiveDirective(CspDirectiveKind.script_src) != null);
    expect_true(pol.effectiveDirective(CspDirectiveKind.img_src) != null);
    expect_true(pol.effectiveDirective(CspDirectiveKind.connect_src) != null);
    expect_true(pol.effectiveDirective(CspDirectiveKind.object_src) != null);
    expect_eq(
        pol.checkResourceUrl(CspDirectiveKind.connect_src, "https://app.test", "https://app.test/api"),
        CspEvalVerdict.blocked_by_none,
    );

    // Document/navigation directives NEVER fall back to default-src!
    expect_true(pol.effectiveDirective(CspDirectiveKind.frame_ancestors) == null);
    expect_true(pol.effectiveDirective(CspDirectiveKind.base_uri) == null);
    expect_true(pol.effectiveDirective(CspDirectiveKind.form_action) == null);
    expect_eq(
        pol.checkResourceUrl(CspDirectiveKind.form_action, "https://app.test", "https://submit.other.test/post"),
        CspEvalVerdict.allowed,
    );
}

test("CspPolicy: resource URL matching ('self', scheme, wildcard, exact host & strict subdomain)") {
    const hdr = "default-src 'none'; script-src 'self' https://api.example.com:8443 *.cdn.example.com; img-src *";
    const pol = CspPolicy.parseHeader(hdr) ?? return expect_true(false);
    const origin = "https://app.example.com";

    // 'self' matches exact scheme+host+port, rejects different port or scheme
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "https://app.example.com/a.js"), CspEvalVerdict.allowed);
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "http://app.example.com/a.js"), CspEvalVerdict.blocked_no_match);
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "https://app.example.com:8443/a.js"), CspEvalVerdict.blocked_no_match);

    // Explicit scheme+host+port
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "https://api.example.com:8443/sdk.js"), CspEvalVerdict.allowed);
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "https://api.example.com/sdk.js"), CspEvalVerdict.blocked_no_match);

    // Strict subdomain wildcard *.cdn.example.com
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "https://static.cdn.example.com/v1.js"), CspEvalVerdict.allowed);
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "https://a.b.cdn.example.com/v1.js"), CspEvalVerdict.allowed);
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "https://cdn.example.com/v1.js"), CspEvalVerdict.blocked_no_match);
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.script_src, origin, "https://evil-cdn.example.com/v1.js"), CspEvalVerdict.blocked_no_match);

    // Wildcard '*' allows network schemes (https) but NOT data:
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.img_src, origin, "https://any.host/pic.png"), CspEvalVerdict.allowed);
    expect_eq(pol.checkResourceUrl(CspDirectiveKind.img_src, origin, "data:image/svg+xml,foo"), CspEvalVerdict.blocked_no_match);
}

test("CspPolicy: CSP3 inline script nonce/hash/'strict-dynamic' suppresses 'unsafe-inline'") {
    // 1. 'unsafe-inline' alone allows inline scripts
    const p_legacy = CspPolicy.parseHeader("script-src 'unsafe-inline'") ?? return expect_true(false);
    expect_eq(p_legacy.checkInlineScript("", ""), CspEvalVerdict.allowed);

    // 2. 'unsafe-inline' paired with a nonce -> 'unsafe-inline' is IGNORED in CSP3!
    const p_nonce = CspPolicy.parseHeader("script-src 'unsafe-inline' 'nonce-TokEn123'") ?? return expect_true(false);
    expect_eq(p_nonce.checkInlineScript("", ""), CspEvalVerdict.blocked_unsafe_inline);
    expect_eq(p_nonce.checkInlineScript("wrong", ""), CspEvalVerdict.blocked_unsafe_inline);
    expect_eq(p_nonce.checkInlineScript("TokEn123", ""), CspEvalVerdict.allowed);

    // 3. 'unsafe-inline' paired with sha256 hash -> only matching hash allowed
    const p_hash = CspPolicy.parseHeader("script-src 'unsafe-inline' 'sha256-AbCdEf12345='") ?? return expect_true(false);
    expect_eq(p_hash.checkInlineScript("", ""), CspEvalVerdict.blocked_unsafe_inline);
    expect_eq(p_hash.checkInlineScript("", "AbCdEf12345="), CspEvalVerdict.allowed);

    // 4. 'unsafe-inline' paired with 'strict-dynamic' -> 'unsafe-inline' suppressed
    const p_dyn = CspPolicy.parseHeader("script-src 'unsafe-inline' 'strict-dynamic'") ?? return expect_true(false);
    expect_eq(p_dyn.checkInlineScript("", ""), CspEvalVerdict.blocked_unsafe_inline);

    // 5. 'none' blocks inline scripts with blocked_by_none
    const p_none = CspPolicy.parseHeader("default-src 'none'") ?? return expect_true(false);
    expect_eq(p_none.checkInlineScript("TokEn123", ""), CspEvalVerdict.blocked_by_none);
}

test("CspPolicy: CRLF & control-character header injection rejection") {
    expect_true(CspPolicy.parseHeader("default-src 'self'\r\nX-Injected: 1") == null);
    expect_true(CspPolicy.parseHeader("default-src 'self'\nSet-Cookie: a=b") == null);

    let pol = CspPolicy.init();
    expect_true(!pol.addSource(CspDirectiveKind.script_src, "https://a.com\r\n"));
    expect_true(!pol.addSource(CspDirectiveKind.script_src, "'unknown-keyword'"));

    expect_eq(
        pol.checkResourceUrl(CspDirectiveKind.script_src, "https://a.com", "https://a.com/\r\nbad"),
        CspEvalVerdict.invalid_input,
    );
    expect_eq(pol.checkInlineScript("bad\nnonce", ""), CspEvalVerdict.invalid_input);
}

test("CspPolicy: builder API, formatHeader round-trip, small-buffer guard & cap.web facade parity") {
    let pol = cap.web.CspPolicy.init();
    expect_true(pol.addSource(cap.web.CspDirectiveKind.default_src, "'self'"));
    expect_true(pol.addSource(cap.web.CspDirectiveKind.script_src, "'nonce-abc'"));
    expect_true(pol.addSource(cap.web.CspDirectiveKind.script_src, "'strict-dynamic'"));
    expect_true(pol.addSource(cap.web.CspDirectiveKind.frame_ancestors, "'none'"));
    pol.setUpgradeInsecure(true);

    let buf: [256]u8 = [0; 256];
    let buf_s: []u8 = buf[0..];
    const n = pol.formatHeader(*buf_s) ?? 0;
    expect_true(n > 0);
    const expected = "default-src 'self'; script-src 'nonce-abc' 'strict-dynamic'; frame-ancestors 'none'; upgrade-insecure-requests";
    expect_true(bytesEq(buf[0..n], expected));

    // Round-trip re-parse matches original behavior
    const reparsed = CspPolicy.parseHeader(buf[0..n]) ?? return expect_true(false);
    expect_true(reparsed.upgrade_insecure);
    expect_eq(reparsed.checkInlineScript("abc", ""), CspEvalVerdict.allowed);
    expect_eq(
        reparsed.checkResourceUrl(CspDirectiveKind.frame_ancestors, "https://a.com", "https://a.com"),
        CspEvalVerdict.blocked_by_none,
    );

    // Small buffer returns null cleanly
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    expect_true(pol.formatHeader(*tiny_s) == null);
}
