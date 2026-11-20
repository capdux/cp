// cap/test/test_html_production_deep.cp
// Production Deep Regression Suite for cap:html:
//   1. WHATWG HTML5 Entity Codec (Named, Decimal, Hex, C1 Windows-1252, U+FFFD)
//   2. Allowlist HTML5 Content Sanitizer & URL Protocol Bypass Defense
//   3. AI Agent Single-Pass HTML Metadata, Link, Heading & Normalized Text Extractor
//   4. Malformed Unclosed Tags & Deep Nesting Boundary Protection
//   5. cap:html Aggregate Exports & Backwards Compatibility Zero-Regression

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    HtmlEscape,
    HtmlTokenKind,
    HtmlToken,
    HtmlTokenizer,
    CssSelector,
    HtmlElement,
    HtmlRewriter,
    HtmlEntities,
    SanitizerPolicy,
    HtmlSanitizer,
    HtmlLink,
    HtmlMeta,
    HtmlHeading,
    HtmlDocumentSummary,
    HtmlExtractor,
} from "cap:html";

// ============================================================================
// 1. WHATWG HTML5 Entities Codec
// ============================================================================
fn testHtmlEntitiesCodec(): !void {
    const alloc = std.heap.page_allocator;

    // 1.1 encodeText escapes &, <, > only; encodeAttr additionally escapes " and '
    const raw_str = "5 < 10 & \"hello\" 'world' > 0";
    const text_enc = try HtmlEntities.encodeText(alloc, raw_str);
    defer alloc.free(text_enc);
    expect_eq(std.mem.eql(u8, text_enc, "5 &lt; 10 &amp; \"hello\" 'world' &gt; 0"), true);

    const attr_enc = try HtmlEntities.encodeAttr(alloc, raw_str);
    defer alloc.free(attr_enc);
    expect_eq(std.mem.eql(u8, attr_enc, "5 &lt; 10 &amp; &quot;hello&quot; &#39;world&#39; &gt; 0"), true);

    // 1.2 Named entities (80+ supported, testing punctuation, currency, math, arrows, Greek, Latin)
    const named_in = "&amp;&lt;&gt;&quot;&apos;&nbsp;&copy;&reg;&trade;&mdash;&ndash;&hellip;&euro;&yen;&pound;&times;&divide;&rarr;&larr;&harr;&alpha;&Omega;&eacute;";
    const named_out = try HtmlEntities.decode(alloc, named_in);
    defer alloc.free(named_out);
    const named_expected = "&<>\"'\xC2\xA0\xC2\xA9\xC2\xAE\xE2\x84\xA2\xE2\x80\x94\xE2\x80\x93\xE2\x80\xA6\xE2\x82\xAC\xC2\xA5\xC2\xA3\xC3\x97\xC3\xB7\xE2\x86\x92\xE2\x86\x90\xE2\x86\x94\xCE\xB1\xCE\xA9\xC3\xA9";
    expect_eq(std.mem.eql(u8, named_out, named_expected), true);

    // 1.3 Decimal & Hexadecimal numeric character references (incl. 4-byte emoji)
    const num_in = "&#65;&#x42;&#X43; &#128512; &#x1F600;";
    const num_out = try HtmlEntities.decode(alloc, num_in);
    defer alloc.free(num_out);
    expect_eq(std.mem.eql(u8, num_out, "ABC \xF0\x9F\x98\x80 \xF0\x9F\x98\x80"), true);

    // 1.4 WHATWG C1 control area (0x80..0x9F) Windows-1252 replacements
    const c1_in = "&#x80;&#130;&#x85;&#x91;&#x92;&#x93;&#x94;&#150;&#151;&#x99;&#x9F;";
    const c1_out = try HtmlEntities.decode(alloc, c1_in);
    defer alloc.free(c1_out);
    const c1_expected = "\xE2\x82\xAC\xE2\x80\x9A\xE2\x80\xA6\xE2\x80\x98\xE2\x80\x99\xE2\x80\x9C\xE2\x80\x9D\xE2\x80\x93\xE2\x80\x94\xE2\x84\xA2\xC5\xB8";
    expect_eq(std.mem.eql(u8, c1_out, c1_expected), true);

    // 1.5 Invalid codepoints (0, UTF-16 surrogates, >0x10FFFF, huge overflow) -> U+FFFD
    const bad_in = "&#0;|&#xD800;|&#xDFFF;|&#x110000;|&#99999999999999999999;";
    const bad_out = try HtmlEntities.decode(alloc, bad_in);
    defer alloc.free(bad_out);
    const fffd = "\xEF\xBF\xBD";
    expect_eq(std.mem.eql(u8, bad_out, fffd ++ "|" ++ fffd ++ "|" ++ fffd ++ "|" ++ fffd ++ "|" ++ fffd), true);

    // 1.6 Unknown or non-entity ampersands preserved verbatim
    const passthrough = try HtmlEntities.decode(alloc, "A & B &unknown; &#; &#x;");
    defer alloc.free(passthrough);
    expect_eq(std.mem.eql(u8, passthrough, "A & B &unknown; &#; &#x;"), true);
}

// ============================================================================
// 2. Allowlist HTML5 Sanitizer & URL Bypass Protection
// ============================================================================
fn testHtmlSanitizerSecurity(): !void {
    const alloc = std.heap.page_allocator;

    // 2.1 URL allowlist & control-character/whitespace bypass blocking
    expect_eq(HtmlSanitizer.isSafeUrl("https://cp-lang.org/docs?a=1&b=2"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("http://localhost:8080/api"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("  https://trimmed.example/path  "), true);
    expect_eq(HtmlSanitizer.isSafeUrl("mailto:team@cp-lang.org"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("#section-2"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("/guide/intro.html"), true);
    expect_eq(HtmlSanitizer.isSafeUrl("assets/logo.png"), true);

    expect_eq(HtmlSanitizer.isSafeUrl("javascript:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("  JaVaScRiPt:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("java\tscript:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("java\nscript:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("java script:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("\x01javascript:alert(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("vbscript:msgbox(1)"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("data:text/html,<script>alert(1)</script>"), false);
    expect_eq(HtmlSanitizer.isSafeUrl("data:image/svg+xml,<svg onload=alert(1)>"), false);
    expect_eq(HtmlSanitizer.isSafeUrl(""), false);

    // 2.2 Dangerous tags, comments, event attrs, entity-encoded javascript: href, safe tags kept
    const dirty_html = "<!-- secret comment -->" ++
        "<script>alert('xss')</script>" ++
        "<style>body{display:none}</style>" ++
        "<iframe src=\"https://evil.test\"><p>inside iframe</p></iframe>" ++
        "<svg onload=\"alert(1)\"><circle r=\"10\"/></svg>" ++
        "<math><mi>x</mi></math>" ++
        "<object data=\"x.swf\">bad</object>" ++
        "<embed src=\"x.swf\">" ++
        "<p class=\"intro\" onclick=\"evil()\" onmouseover=\"bad()\">" ++
        "1 < 2 &amp; <a href=\"&#106;&#97;vascript:alert(1)\" ONCLICK=\"x()\">BadLink</a> " ++
        "<a href=\"https://cp-lang.org\" title=\"CP &quot;Home&quot;\">GoodLink</a>" ++
        "<img src=\"/img.png\" onerror=\"hack()\" alt=\"Logo\">" ++
        "</p>";

    const cleaned = try HtmlSanitizer.sanitize(alloc, dirty_html, SanitizerPolicy.defaultSafe());
    defer alloc.free(cleaned);

    expect_eq(std.mem.indexOf(u8, cleaned, "secret comment") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<script") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "alert('xss')") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<style") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "display:none") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<iframe") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "inside iframe") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<svg") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<math") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<object") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<embed") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "onclick") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "onmouseover") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "onerror") == null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "javascript:") == null, true);

    expect_eq(std.mem.indexOf(u8, cleaned, "<p class=\"intro\">1 &lt; 2 &amp; <a>BadLink</a> ") != null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<a href=\"https://cp-lang.org\" title=\"CP &quot;Home&quot;\">GoodLink</a>") != null, true);
    expect_eq(std.mem.indexOf(u8, cleaned, "<img src=\"/img.png\" alt=\"Logo\">") != null, true);

    // 2.3 strictTextOnly() removes all tags and keeps safely escaped text
    const strict_out = try HtmlSanitizer.sanitize(
        alloc,
        "<p>Hello <strong>CP</strong> &amp; 1 < 2</p><script>evil()</script>",
        SanitizerPolicy.strictTextOnly(),
    );
    defer alloc.free(strict_out);
    expect_eq(std.mem.eql(u8, strict_out, "Hello CP &amp; 1 &lt; 2"), true);

    // 2.4 Custom policy allowlist
    const custom_tags: [2][]const u8 = ["b", "script"];
    const custom_attrs: [1][]const u8 = ["id"];
    const empty_urls: []const []const u8 = [];
    const custom_policy = SanitizerPolicy.custom(custom_tags[0..2], custom_attrs[0..1], empty_urls);
    const custom_out = try HtmlSanitizer.sanitize(
        alloc,
        "<div id=\"x\"><b id=\"ok\" class=\"no\">Bold</b><script>bad()</script></div>",
        custom_policy,
    );
    defer alloc.free(custom_out);
    // Even if user passes "script" in custom tags, forbidden tag defense drops <script>
    expect_eq(std.mem.eql(u8, custom_out, "<b id=\"ok\">Bold</b>"), true);
}

// ============================================================================
// 3. AI Agent HTML Visible Text & Metadata Extractor
// ============================================================================
fn testHtmlExtractorMetadataAndText(): !void {
    const alloc = std.heap.page_allocator;

    const doc_html = "<!DOCTYPE html><html><head>" ++
        "<title>  CP   Language &amp; Runtime  </title>" ++
        "<meta name=\"description\" content=\"Systems &amp; AI Agent Language &#x2014; Fast\">" ++
        "<meta property=\"og:title\" content=\"CP Lang Official\">" ++
        "<meta property=\"og:image\" content=\"https://cp-lang.org/og.png\">" ++
        "<link rel=\"alternate canonical\" href=\"https://cp-lang.org/docs\">" ++
        "<style>.hidden { color: red; }</style>" ++
        "<script>const secret_js = 1 < 2;</script>" ++
        "</head><body>" ++
        "<!-- header comment -->" ++
        "<noscript>Enable JS</noscript>" ++
        "<template><p>Template Hidden</p></template>" ++
        "<h1>Welcome to <code>CP</code> &mdash; v1</h1>" ++
        "<p>First   paragraph\n\twith   multiple   spaces &amp; &#x1F600;.</p>" ++
        "<h2>Links Section</h2>" ++
        "<ul><li>Read <a href=\"https://cp-lang.org/spec\"><span>The</span> <b>Spec</b></a></li>" ++
        "<li>Visit <a href=\"/examples\">Examples</a></li></ul>" ++
        "</body></html>";

    let summary = try HtmlExtractor.extract(alloc, doc_html);
    defer summary.deinit();

    expect_eq(std.mem.eql(u8, summary.title, "CP Language & Runtime"), true);
    expect_eq(std.mem.eql(u8, summary.description, "Systems & AI Agent Language \xE2\x80\x94 Fast"), true);
    expect_eq(std.mem.eql(u8, summary.og_title, "CP Lang Official"), true);
    expect_eq(std.mem.eql(u8, summary.og_image, "https://cp-lang.org/og.png"), true);
    expect_eq(std.mem.eql(u8, summary.canonical_url, "https://cp-lang.org/docs"), true);

    expect_eq(summary.headings.len, 2 as usize);
    expect_eq(summary.headings[0].level, 1 as u8);
    expect_eq(std.mem.eql(u8, summary.headings[0].text, "Welcome to CP \xE2\x80\x94 v1"), true);
    expect_eq(summary.headings[1].level, 2 as u8);
    expect_eq(std.mem.eql(u8, summary.headings[1].text, "Links Section"), true);

    expect_eq(summary.links.len, 2 as usize);
    expect_eq(std.mem.eql(u8, summary.links[0].href, "https://cp-lang.org/spec"), true);
    expect_eq(std.mem.eql(u8, summary.links[0].text, "The Spec"), true);
    expect_eq(std.mem.eql(u8, summary.links[1].href, "/examples"), true);
    expect_eq(std.mem.eql(u8, summary.links[1].text, "Examples"), true);

    expect_eq(summary.metas.len, 3 as usize);

    // Script, style, noscript, template, comment, and <title> MUST NOT leak into plain_text
    expect_eq(std.mem.indexOf(u8, summary.plain_text, "secret_js") == null, true);
    expect_eq(std.mem.indexOf(u8, summary.plain_text, "color: red") == null, true);
    expect_eq(std.mem.indexOf(u8, summary.plain_text, "Enable JS") == null, true);
    expect_eq(std.mem.indexOf(u8, summary.plain_text, "Template Hidden") == null, true);
    expect_eq(std.mem.indexOf(u8, summary.plain_text, "header comment") == null, true);
    expect_eq(std.mem.indexOf(u8, summary.plain_text, "CP Language & Runtime") == null, true);

    const expected_plain = "Welcome to CP \xE2\x80\x94 v1\n" ++
        "First paragraph with multiple spaces & \xF0\x9F\x98\x80.\n" ++
        "Links Section\n" ++
        "Read The Spec\n" ++
        "Visit Examples";
    expect_eq(std.mem.eql(u8, summary.plain_text, expected_plain), true);
}

// ============================================================================
// 4. Malformed Unclosed Tags & Deep Nesting Boundary Defense
// ============================================================================
fn testMalformedAndDeepNestingDefense(): !void {
    const alloc = std.heap.page_allocator;

    // 4.1 Unclosed / mismatched tags in Sanitizer auto-close cleanly
    const malformed = "<div><p><b><i>hello</div><span>world";
    const mal_clean = try HtmlSanitizer.sanitize(alloc, malformed, SanitizerPolicy.defaultSafe());
    defer alloc.free(mal_clean);
    expect_eq(std.mem.eql(u8, mal_clean, "<div><p><b><i>hello</i></b></p></div><span>world</span>"), true);

    // 4.2 Unclosed script, unclosed comment, unclosed attribute quote
    const unclosed_script = "<p>Safe</p><script>never closed";
    const clean_script = try HtmlSanitizer.sanitize(alloc, unclosed_script, SanitizerPolicy.defaultSafe());
    defer alloc.free(clean_script);
    expect_eq(std.mem.eql(u8, clean_script, "<p>Safe</p>"), true);

    const unclosed_comment = "<p>Visible</p><!-- trailing unclosed";
    const clean_comment = try HtmlSanitizer.sanitize(alloc, unclosed_comment, SanitizerPolicy.defaultSafe());
    defer alloc.free(clean_comment);
    expect_eq(std.mem.eql(u8, clean_comment, "<p>Visible</p>"), true);

    // 4.3 Malformed unclosed heading & link in Extractor flushed at EOF
    let mal_summary = try HtmlExtractor.extract(alloc, "<h2>Open Heading<p>Body <a href=\"/tail\">Unclosed Link");
    defer mal_summary.deinit();
    expect_eq(mal_summary.headings.len, 1 as usize);
    expect_eq(std.mem.eql(u8, mal_summary.headings[0].text, "Open HeadingBody Unclosed Link"), true);
    expect_eq(mal_summary.links.len, 1 as usize);
    expect_eq(std.mem.eql(u8, mal_summary.links[0].href, "/tail"), true);
    expect_eq(std.mem.eql(u8, mal_summary.links[0].text, "Unclosed Link"), true);

    // 4.4 Deep nesting stress test (600 nested <div> tags) must not overflow or crash
    let deep_buf: std.ArrayList(u8) = .empty;
    defer deep_buf.deinit(alloc);
    let i: usize = 0;
    while (i < 600) : (i += 1) {
        try deep_buf.appendSlice(alloc, "<div>");
    }
    try deep_buf.appendSlice(alloc, "core");
    i = 0;
    while (i < 600) : (i += 1) {
        try deep_buf.appendSlice(alloc, "</div>");
    }

    const deep_clean = try HtmlSanitizer.sanitize(alloc, deep_buf.items, SanitizerPolicy.defaultSafe());
    defer alloc.free(deep_clean);
    expect_eq(std.mem.indexOf(u8, deep_clean, "core") != null, true);

    let deep_summary = try HtmlExtractor.extract(alloc, deep_buf.items);
    defer deep_summary.deinit();
    expect_eq(std.mem.eql(u8, deep_summary.plain_text, "core"), true);
}

// ============================================================================
// 5. Backwards Compatibility & Aggregate cap:html Exports
// ============================================================================
fn addRelHandler(el: *HtmlElement) {
    el.setAttribute("rel", "noopener");
}

fn testBackwardsCompatibility(): !void {
    const alloc = std.heap.page_allocator;

    // 5.1 HtmlEscape remains 100% compatible
    let buf: [64]u8 = [0; 64];
    const esc = HtmlEscape.escapeToBuf("<a>&\"'", &buf);
    expect_eq(std.mem.eql(u8, esc, "&lt;a&gt;&amp;&quot;&#39;"), true);

    // 5.2 HtmlTokenizer + CssSelector + HtmlRewriter remain 100% compatible
    let rw = HtmlRewriter.init(alloc);
    defer rw.deinit();
    try rw.on("a.ext", addRelHandler);
    const rewritten = try rw.transform("<p><a class=\"ext\" href=\"https://cp-lang.org\">CP</a></p>");
    defer alloc.free(rewritten);
    expect_eq(std.mem.eql(u8, rewritten, "<p><a class=\"ext\" href=\"https://cp-lang.org\" rel=\"noopener\">CP</a></p>"), true);

    // 5.3 Pipeline integration: sanitize -> extract
    const dirty = "<div onclick=\"x()\"><script>bad()</script><h1>Title &amp; Guide</h1><p>Read <a href=\"https://cp-lang.org\">Docs</a> and <a href=\"javascript:bad()\">Skip</a></p></div>";
    const safe_html = try HtmlSanitizer.sanitize(alloc, dirty, SanitizerPolicy.defaultSafe());
    defer alloc.free(safe_html);

    let summary = try HtmlExtractor.extract(alloc, safe_html);
    defer summary.deinit();
    expect_eq(summary.headings.len, 1 as usize);
    expect_eq(std.mem.eql(u8, summary.headings[0].text, "Title & Guide"), true);
    expect_eq(summary.links.len, 1 as usize);
    expect_eq(std.mem.eql(u8, summary.links[0].href, "https://cp-lang.org"), true);
    expect_eq(std.mem.eql(u8, summary.links[0].text, "Docs"), true);
}

fn main() {
    test("1. cap:html Entities — WHATWG Named, Decimal/Hex, C1 Windows-1252 & U+FFFD", () => {
        try testHtmlEntitiesCodec();
    });
    test("2. cap:html Sanitizer — Allowlist Tags, on* Stripping & URL Protocol Bypass Defense", () => {
        try testHtmlSanitizerSecurity();
    });
    test("3. cap:html Extractor — Metadata, OG, Canonical, Links, Headings & Clean Plain Text", () => {
        try testHtmlExtractorMetadataAndText();
    });
    test("4. cap:html Boundary — Malformed Unclosed Tags & 600-Deep Nesting Protection", () => {
        try testMalformedAndDeepNestingDefense();
    });
    test("5. cap:html Integration — Aggregate Exports & Existing API Zero Regression", () => {
        try testBackwardsCompatibility();
    });
}
