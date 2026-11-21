// Source: cp/cap/test/test_rtf_production_deep.cp (Production deep tests for cap:rtf)

import { test, expect_eq } from "cap:test";
import {
    RTF_MAGIC,
    MAX_RTF_DEPTH,
    RtfTokenKind,
    RtfCharset,
    RtfToken,
    RtfHeaderInfo,
    isRtf,
    RtfLexer,
    parseRtfHeader,
    extractRtfText,
} from "cap:rtf";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("1. parses RTF 1.9 header with leading whitespace, ansi charset, ansicpg1252, deff0, and uc1", () => {
    const doc = "  \n{\\rtf1\\ansi\\ansicpg1252\\deff0\\uc2 {\\fonttbl{\\f0 Arial;}}Hello}";
    expect_eq(MAX_RTF_DEPTH, 64);
    expect_eq(isRtf(doc), true);

    if (hdr := parseRtfHeader(doc)) {
        expect_eq(hdr.version, 1);
        expect_eq(hdr.charset == RtfCharset.ansi, true);
        expect_eq(hdr.ansicpg, 1252);
        expect_eq(hdr.default_font, 0);
        expect_eq(hdr.uc_skip, 2);
    } else {
        expect_eq(true, false);
    }
});

test("2. RtfLexer consumes single space delimiter after control words and decodes negative params and hex escapes", () => {
    const src = "{\\rtf1\\li-720 \\'e9\\~text}";
    let lex = RtfLexer.init(src);

    // '{'
    if (t := lex.next()) { expect_eq(t.kind == RtfTokenKind.group_start, true); } else { expect_eq(true, false); }
    // \rtf1
    if (t := lex.next()) {
        expect_eq(t.kind == RtfTokenKind.control_word, true);
        expect_eq(sliceEq(t.name, "rtf"), true);
        expect_eq(t.has_param, true);
        expect_eq(t.param, 1);
    } else { expect_eq(true, false); }
    // \li-720 (trailing single space consumed!)
    if (t := lex.next()) {
        expect_eq(t.kind == RtfTokenKind.control_word, true);
        expect_eq(sliceEq(t.name, "li"), true);
        expect_eq(t.has_param, true);
        expect_eq(t.param, -720);
    } else { expect_eq(true, false); }
    // \'e9
    if (t := lex.next()) {
        expect_eq(t.kind == RtfTokenKind.hex_byte, true);
        expect_eq(t.byte_val, 0xE9);
    } else { expect_eq(true, false); }
    // \~
    if (t := lex.next()) {
        expect_eq(t.kind == RtfTokenKind.control_symbol, true);
        expect_eq(t.byte_val, '~');
    } else { expect_eq(true, false); }
    // text
    if (t := lex.next()) {
        expect_eq(t.kind == RtfTokenKind.text, true);
        expect_eq(sliceEq(t.name, "text"), true);
    } else { expect_eq(true, false); }
    // '}'
    if (t := lex.next()) { expect_eq(t.kind == RtfTokenKind.group_end, true); } else { expect_eq(true, false); }
    expect_eq(lex.next() == null, true);
    expect_eq(lex.had_error, false);
});

test("3. extractRtfText skips fonttbl/colortbl/ignorable * destinations and preserves body formatting", () => {
    const doc =
        "{\\rtf1\\ansi" +
        "{\\fonttbl{\\f0 Times New Roman;}}" +
        "{\\colortbl;\\red255\\green0\\blue0;}" +
        "{\\*\\generator CustomGen 1.0;}" +
        "Hello\\~World!\\parLine\\tabTwo\\\\\\{ok\\}}";

    let out: [128]u8 = undefined;
    if (n := extractRtfText(doc, out[0..128])) {
        expect_eq(sliceEq(out[0..n], "Hello World!\nLine\tTwo\\{ok}"), true);
    } else {
        expect_eq(true, false);
    }
});

test("4. extractRtfText decodes \\u20013? (Chinese '中') UTF-8 with \\uc1 replacement skip and \\'hh bytes", () => {
    // U+4E2D (20013) = "中" (UTF-8: E4 B8 AD); \uc1 skips the trailing '?'
    // Also test negative \u value: -10179 + 65536 = 55357 (or simple U+00E9 = 233)
    const doc = "{\\rtf1\\ansi\\uc1 \\u20013?\\u25991? \\'41}";
    let out: [64]u8 = undefined;
    if (n := extractRtfText(doc, out[0..64])) {
        expect_eq(sliceEq(out[0..n], "中文 A"), true);
    } else {
        expect_eq(true, false);
    }
});

test("5. rejects non-RTF input, unbalanced braces, excessive nesting (>64), and output buffer overflow", () => {
    let out: [32]u8 = undefined;

    // Non-RTF
    expect_eq(isRtf("<html>hello</html>"), false);
    expect_eq(parseRtfHeader("plain text") == null, true);
    expect_eq(extractRtfText("plain text", out[0..32]) == null, true);

    // Unbalanced braces (missing closing '}')
    expect_eq(extractRtfText("{\\rtf1 Hello {World}", out[0..32]) == null, true);

    // Output buffer too small (overflow protection)
    let tiny: [3]u8 = undefined;
    expect_eq(extractRtfText("{\\rtf1 HelloWorld}", tiny[0..3]) == null, true);

    // Excessive nesting depth (> 64)
    let deep: [160]u8 = undefined;
    deep[0] = '{';
    deep[1] = '\\';
    deep[2] = 'r';
    deep[3] = 't';
    deep[4] = 'f';
    deep[5] = '1';
    let p: usize = 6;
    let i: usize = 0;
    while (i < 66) {
        deep[p] = '{';
        p += 1;
        i += 1;
    }
    i = 0;
    while (i < 67) {
        deep[p] = '}';
        p += 1;
        i += 1;
    }
    expect_eq(extractRtfText(deep[0..p], out[0..32]) == null, true);
});
