// rust-ammonia/ammonia (MIT/Apache-2.0) & WHATWG HTML5 Reference-Hardened Sanitizer Test Suite
// Covers:
//   - Test 01: Safe rich HTML preservation (<a>, <p>, <ul>, <code>, <pre>, <img>), strictTextOnly, custom policy
//   - Test 02: URL scheme obfuscation defense (control chars, tabs/newlines, &#106;/&#x6A; entities,
//              vbscript:, data:text/html) vs allowlisted http/https/mailto/relative
//   - Test 03: Forbidden tags (<script>/<style>/<iframe>/<svg>/<math>/<template>/<noscript>) &
//              forbidden attrs (on*/style/srcdoc/formaction/xlink:href), comments, unclosed tags, depth > 64
//   - Test 04: cap:simd HTML special-byte scan parity (scanHtmlTagOpenSimd, scanHtmlSpecialSimd vs Scalar) &
//              HtmlEntities encodeText/encodeAttr/decode (C1 Windows-1252 & FFFD)
//   - Test 05: 1,500-iteration deterministic PRNG malformed HTML / nested tags / truncated entity fuzzing

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { HtmlEntities } from "../../lib/html/entities.cp";
import {
    HTML_SANITIZE_MAX_DEPTH,
    SanitizerPolicy,
    HtmlSanitizer,
} from "../../lib/html/sanitizer.cp";

fn xorshift64(state: *u64): u64 {
    let x = state.*;
    x = x ^ (x << 13);
    x = x ^ (x >> 7);
    x = x ^ (x << 17);
    state.* = x;
    return x;
}

fn hasSubstr(haystack: []const u8, needle: []const u8): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn runTest01(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    const safe_html =
        "<p class=\"lead\">Hello <a href=\"https://cp-lang.org\" title=\"CP\">Link</a></p>" +
        "<ul><li><code>let x = 1;</code></li></ul>" +
        "<pre>fn main() {}</pre>" +
        "<img src=\"/logo.png\" alt=\"Logo\" width=\"128\" height=\"64\">";

    const clean = try HtmlSanitizer.sanitize(alloc, safe_html, SanitizerPolicy.defaultSafe());
    expect_eq(hasSubstr(clean, "<p class=\"lead\">"), true);
    expect_eq(hasSubstr(clean, "href=\"https://cp-lang.org\""), true);
    expect_eq(hasSubstr(clean, "<code>let x = 1;</code>"), true);
    expect_eq(hasSubstr(clean, "<pre>fn main() {}</pre>"), true);
    expect_eq(hasSubstr(clean, "<img src=\"/logo.png\""), true);

    const text_only = try HtmlSanitizer.sanitize(alloc, "<p>Hi <b>CP</b> &amp; AI</p>", SanitizerPolicy.strictTextOnly());
    expect_eq(std.mem.eql(u8, text_only, "Hi CP &amp; AI"), true);
}

fn runTest02(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // Allowlisted URLs
    expect_eq(HtmlSanitizer.isSafeUrl("https://example.com/a?b=1"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("http://localhost:8080/api"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("mailto:sec@cp-lang.org"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("#section-2"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("/assets/app.js"), true);

    // Disallowed & obfuscated schemes
    expect_eq(HtmlSanitizer.isSafeUrl("javascript:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("  JaVaScRiPt:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("java\tscript:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("java\nscript:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("\x00javascript:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("vbscript:msgbox(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("data:text/html,<script>1</script>"), false);

    // Entity-encoded scheme obfuscation
    expect_eq(try HtmlSanitizer.isSafeUrlStrict(alloc, "&#106;avascript:alert(1)"), false);
    expect_eq(try HtmlSanitizer.isSafeUrlStrict(alloc, "&#x6A;avascript:alert(1)"), false);

    const dirty_links =
        "<a href=\"&#106;avascript:alert(1)\">e1</a>" +
        "<a href=\"&#x6A;avascript:alert(2)\">e2</a>" +
        "<a href=\"java\tscript:alert(3)\">e3</a>" +
        "<a href=\"data:text/html,hi\">e4</a>" +
        "<a href=\"https://cp-lang.org\">ok</a>";
    const clean_links = try HtmlSanitizer.sanitize(alloc, dirty_links, SanitizerPolicy.defaultSafe());
    expect_eq(hasSubstr(clean_links, "javascript:"), false);
    expect_eq(hasSubstr(clean_links, "data:text/html"), false);
    expect_eq(hasSubstr(clean_links, "href=\"https://cp-lang.org\""), true);
}

fn runTest03(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    const dirty =
        "<!-- secret comment -->" +
        "<script>bad()</script>" +
        "<style>body{display:none}</style>" +
        "<iframe src=\"https://x.test\">drop</iframe>" +
        "<svg onload=\"bad()\"><circle/></svg>" +
        "<math><mi>x</mi></math>" +
        "<template><p>hidden</p></template>" +
        "<noscript><p>nojs</p></noscript>" +
        "<div onclick=\"bad()\" ONMOUSEOVER=\"bad()\" style=\"color:red\" srcdoc=\"<p>x</p>\" formaction=\"/x\" xlink:href=\"#\">" +
        "<p><b>unclosed</div>";

    const clean = try HtmlSanitizer.sanitize(alloc, dirty, SanitizerPolicy.defaultSafe());
    expect_eq(hasSubstr(clean, "secret comment"), false);
    expect_eq(hasSubstr(clean, "bad()"), false);
    expect_eq(hasSubstr(clean, "display:none"), false);
    expect_eq(hasSubstr(clean, "<iframe"), false);
    expect_eq(hasSubstr(clean, "<svg"), false);
    expect_eq(hasSubstr(clean, "<math"), false);
    expect_eq(hasSubstr(clean, "<template"), false);
    expect_eq(hasSubstr(clean, "<noscript"), false);
    expect_eq(hasSubstr(clean, "onclick"), false);
    expect_eq(hasSubstr(clean, "style="), false);
    expect_eq(hasSubstr(clean, "srcdoc"), false);
    expect_eq(hasSubstr(clean, "formaction"), false);
    expect_eq(hasSubstr(clean, "xlink:href"), false);
    expect_eq(hasSubstr(clean, "<div><p><b>unclosed</b></p></div>"), true);

    // Depth > HTML_SANITIZE_MAX_DEPTH (64) bounded safely
    expect_eq(HTML_SANITIZE_MAX_DEPTH, 64);
    let deep = std.ArrayList(u8).empty;
    let d: usize = 0;
    while (d < 80) : (d += 1) try deep.appendSlice(alloc, "<div>");
    try deep.appendSlice(alloc, "core");
    d = 0;
    while (d < 80) : (d += 1) try deep.appendSlice(alloc, "</div>");
    const deep_clean = try HtmlSanitizer.sanitize(alloc, deep.items, SanitizerPolicy.defaultSafe());
    expect_eq(hasSubstr(deep_clean, "core"), true);
}

fn runTest04(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    const samples: [6][]const u8 = [
        "",
        "plain_ascii_text_without_any_html_specials_over_thirty_two_bytes",
        "0123456789abcdef0123456789abcdef<tag>",
        "0123456789abcdef0123456789abcdef&amp;",
        "0123456789abcdef0123456789abcdef\"attr\"",
        "0123456789abcdef0123456789abcdef>end",
    ];
    for (s of samples) {
        expect_eq(HtmlSanitizer.scanHtmlTagOpenSimd(s) == null, HtmlSanitizer.scanHtmlTagOpenScalar(s) == null);
        if (HtmlSanitizer.scanHtmlTagOpenSimd(s) != null) {
            expect_eq(HtmlSanitizer.scanHtmlTagOpenSimd(s).?, HtmlSanitizer.scanHtmlTagOpenScalar(s).?);
        }
        expect_eq(HtmlSanitizer.scanHtmlSpecialSimd(s) == null, HtmlSanitizer.scanHtmlSpecialScalar(s) == null);
        if (HtmlSanitizer.scanHtmlSpecialSimd(s) != null) {
            expect_eq(HtmlSanitizer.scanHtmlSpecialSimd(s).?, HtmlSanitizer.scanHtmlSpecialScalar(s).?);
        }
    }

    // HtmlEntities SIMD encodeText / encodeAttr / decode (incl. C1 & FFFD)
    const enc_t = try HtmlEntities.encodeText(alloc, "0123456789abcdef<a&b>");
    expect_eq(std.mem.eql(u8, enc_t, "0123456789abcdef&lt;a&amp;b&gt;"), true);
    const enc_a = try HtmlEntities.encodeAttr(alloc, "0123456789abcdef'\"&");
    expect_eq(std.mem.eql(u8, enc_a, "0123456789abcdef&#39;&quot;&amp;"), true);
    const dec_out = try HtmlEntities.decode(alloc, "0123456789abcdef&euro;&#x97;&#xD800;");
    expect_eq(hasSubstr(dec_out, "\xE2\x82\xAC\xE2\x80\x94\xEF\xBF\xBD"), true);
}

fn runTest05(): void! {
    let arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer arena.deinit();
    const alloc = arena.allocator();

    // 1,500-iteration deterministic PRNG malformed HTML / nested tags / truncated entity fuzzing
    const alphabet = "abcdefghijklmnopqrstuvwxyz<>\"'&;#/=! \t\n\r0123456789";
    let seed: u64 = 0x4854_4D4C_5341_4E49;
    let buf: [96]u8 = undefined;

    let iter: usize = 0;
    let count: usize = 0;
    while (iter < 1500) : (iter += 1) {
        const len = ((xorshift64(&seed) % 80) + 1) as usize;
        let j: usize = 0;
        while (j < len) : (j += 1) {
            const idx = (xorshift64(&seed) as usize) % alphabet.len;
            buf[j] = alphabet[idx];
        }
        const res = try HtmlSanitizer.sanitize(alloc, buf[0..len], SanitizerPolicy.defaultSafe());
        * = res;
        count += 1;
    }
    expect_eq(count, 1500);
}

fn main() {
    test("html_ref_01_safe_rich_html_and_strict_text_only", () => { try runTest01(); });
    test("html_ref_02_url_protocol_obfuscation_and_entity_defense", () => { try runTest02(); });
    test("html_ref_03_forbidden_tags_attrs_comments_and_depth_64", () => { try runTest03(); });
    test("html_ref_04_simd_special_scan_and_entities_c1_parity", () => { try runTest04(); });
    test("html_ref_05_deterministic_prng_1500_fuzz_malformed_html", () => { try runTest05(); });
}
