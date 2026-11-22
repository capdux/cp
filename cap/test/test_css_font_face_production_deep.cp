// cap/test/test_css_font_face_production_deep.cp
// Production-deep regression tests for cap:css font_face module
// Source: /data/dev/bun/src/css/rules/font_face.cp & /data/dev/bun/docs/bundler/css.mdx

import { test, expect } from "cap:test";
import {
    CSS_UNICODE_MAX_CODEPOINT,
    CSS_FONT_FACE_MAX_SOURCES,
    CSS_FONT_FACE_MAX_RANGES,
    CssFontFaceError,
    CssFontFormat,
    CssFontDisplay,
    CssFontSourceKind,
    CssUnicodeRange,
    CssFontFaceSource,
    CssFontFaceRule,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("css font_face: CssUnicodeRange parsing & wildcard '?' compression round-trip") {
    let buf: [32]u8 = [0; 32];

    // U+0000-00FF -> U+??
    const r1 = CssUnicodeRange.parse("U+0000-00FF") catch { expect(false); return; };
    expect(r1.start == 0x0000 && r1.end == 0x00FF);
    expect(r1.contains(0x0041));
    expect(!r1.contains(0x0100));
    const n1 = r1.toCss(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "U+??"));
    const rt1 = CssUnicodeRange.parse(buf[0..n1]) catch { expect(false); return; };
    expect(rt1.eql(r1));

    // U+4000-4FFF -> U+4???
    const r2 = CssUnicodeRange.parse("U+4???") catch { expect(false); return; };
    expect(r2.start == 0x4000 && r2.end == 0x4FFF);
    const n2 = r2.toCss(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "U+4???"));

    // Single code point
    const r3 = CssUnicodeRange.parse("u+0041") catch { expect(false); return; };
    expect(r3.start == 0x41 && r3.end == 0x41);
    const n3 = r3.toCss(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n3], "U+41"));

    // Non-aligned explicit range stays explicit
    const r4 = CssUnicodeRange.parse("U+0025-00FF") catch { expect(false); return; };
    const n4 = r4.toCss(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n4], "U+25-ff"));
}

test("css font_face: CssFontFaceSource parsing (local, url + format + tech)") {
    let buf: [128]u8 = [0; 128];

    const s1 = CssFontFaceSource.parse("local('Inter Regular')") catch { expect(false); return; };
    expect(s1.kind == CssFontSourceKind.local);
    expect(strEq(s1.value, "Inter Regular"));
    const n1 = s1.toCss(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n1], "local(\"Inter Regular\")"));

    const s2 = CssFontFaceSource.parse("url('./inter.woff2') format('woff2') tech(variations)") catch { expect(false); return; };
    expect(s2.kind == CssFontSourceKind.url);
    expect(strEq(s2.value, "./inter.woff2"));
    expect(s2.format == CssFontFormat.woff2);
    expect(strEq(s2.tech, "variations"));
    const n2 = s2.toCss(buf[0..]) catch { expect(false); return; };
    expect(strEq(buf[0..n2], "url(\"./inter.woff2\") format(\"woff2\") tech(variations)"));
}

test("css font_face: full @font-face rule parse, code-point matching & round-trip serialization") {
    const raw =
        "@font-face {" ++
        "  font-family: 'Inter';" ++
        "  src: local('Inter'), url('./inter.woff2') format('woff2'), url('./inter.woff') format('woff');" ++
        "  font-weight: 100 900;" ++
        "  font-style: normal;" ++
        "  font-display: swap;" ++
        "  unicode-range: U+0000-00FF, U+4E00-4FFF;" ++
        "}";

    const rule = CssFontFaceRule.parse(raw) catch { expect(false); return; };
    expect(strEq(rule.font_family, "Inter"));
    expect(rule.source_count == 3);
    expect(rule.range_count == 2);
    expect(rule.has_font_display);
    expect(rule.font_display == CssFontDisplay.swap);
    expect(strEq(rule.font_weight, "100 900"));
    expect(strEq(rule.font_style, "normal"));

    expect(rule.matchesCodePoint(0x0041));
    expect(rule.matchesCodePoint(0x4E2D));
    expect(!rule.matchesCodePoint(0x3042));
    expect(!rule.matchesCodePoint(CSS_UNICODE_MAX_CODEPOINT + 1));

    let out: [512]u8 = [0; 512];
    const n = rule.toCss(out[0..]) catch { expect(false); return; };

    // Round-trip re-parse serialized CSS
    const rt = CssFontFaceRule.parse(out[0..n]) catch { expect(false); return; };
    expect(strEq(rt.font_family, "Inter"));
    expect(rt.source_count == 3);
    expect(rt.range_count == 2);
    expect(rt.font_display == CssFontDisplay.swap);
    expect(rt.matchesCodePoint(0x4E2D));
}

test("css font_face: empty unicode-range matches all valid Unicode code points") {
    const rule = CssFontFaceRule.parse("@font-face { font-family: 'Mono'; src: url('./m.woff2'); }") catch {
        expect(false);
        return;
    };
    expect(rule.range_count == 0);
    expect(rule.matchesCodePoint(0x0000));
    expect(rule.matchesCodePoint(0x1F600));
    expect(rule.matchesCodePoint(CSS_UNICODE_MAX_CODEPOINT));
    expect(!rule.matchesCodePoint(0x110000));
}

test("css font_face: malformed unicode-range & source defenses") {
    // Missing U+ prefix
    let e1 = false;
    * = CssUnicodeRange.parse("0041") catch |e| {
        expect(e == CssFontFaceError.InvalidUnicodeRange);
        e1 = true;
    };
    expect(e1);

    // Mixing '?' and '-'
    let e2 = false;
    * = CssUnicodeRange.parse("U+4??-500") catch |e| {
        expect(e == CssFontFaceError.InvalidUnicodeRange);
        e2 = true;
    };
    expect(e2);

    // Code point exceeding 0x10FFFF
    let e3 = false;
    * = CssUnicodeRange.parse("U+110000") catch |e| {
        expect(e == CssFontFaceError.CodePointOutOfRange);
        e3 = true;
    };
    expect(e3);

    // Reversed range start > end
    let e4 = false;
    * = CssUnicodeRange.parse("U+00FF-0010") catch |e| {
        expect(e == CssFontFaceError.ReversedUnicodeRange);
        e4 = true;
    };
    expect(e4);

    // Invalid font-display keyword
    let e5 = false;
    * = CssFontDisplay.parse("instant") catch |e| {
        expect(e == CssFontFaceError.InvalidFontDisplay);
        e5 = true;
    };
    expect(e5);

    // Malformed src
    let e6 = false;
    * = CssFontFaceSource.parse("url('./unclosed.woff2'") catch |e| {
        expect(e == CssFontFaceError.InvalidSource);
        e6 = true;
    };
    expect(e6);
}

test("css font_face: malformed rule syntax, BufferTooSmall & constants sanity") {
    let e1 = false;
    * = CssFontFaceRule.parse("@media screen {}") catch |e| {
        expect(e == CssFontFaceError.InvalidRuleSyntax);
        e1 = true;
    };
    expect(e1);

    let e2 = false;
    * = CssFontFaceRule.parse("@font-face { font-family: ''; }") catch |e| {
        expect(e == CssFontFaceError.EmptyFontFamily);
        e2 = true;
    };
    expect(e2);

    const r = CssUnicodeRange.parse("U+0025-00FF") catch { expect(false); return; };
    let tiny: [3]u8 = [0; 3];
    let e3 = false;
    * = r.toCss(tiny[0..]) catch |e| {
        expect(e == CssFontFaceError.BufferTooSmall);
        e3 = true;
    };
    expect(e3);

    expect(CSS_UNICODE_MAX_CODEPOINT == 0x10FFFF);
    expect(CSS_FONT_FACE_MAX_SOURCES == 16);
    expect(CSS_FONT_FACE_MAX_RANGES == 32);
}
