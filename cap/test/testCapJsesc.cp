// cap/test/testCapJsesc.cp
// Comprehensive unit tests for @cap/jsesc

import { test, expect_eq } from "cap:test";
import {
    JsescOptions,
    jsesc,
    jsescWithOptions,
} from "../../packages/jsesc/src/index.cp";

fn main() {
    test("1. @cap/jsesc — default single-quote & control character escaping", () => {
        let out: [256]u8 = undefined;
        const n1 = jsesc("it's a\\b\n\r\t\x08\x0C", out[0..]);
        expect_eq(out[0..n1], "it\\'s a\\\\b\\n\\r\\t\\b\\f");

        // Null byte: \0 normally, \x00 before digit
        const n2 = jsesc("a\x00b", out[0..]);
        expect_eq(out[0..n2], "a\\0b");

        const n3 = jsesc("a\x001", out[0..]);
        expect_eq(out[0..n3], "a\\x001");
    });

    test("2. @cap/jsesc — non-ASCII, UTF-16 surrogates, and U+2028/U+2029", () => {
        let out: [256]u8 = undefined;
        // Latin-1 (U+00E9 'é') + CJK (U+4E2D '中') + emoji (U+1F600 '😀')
        const n1 = jsesc("é中😀", out[0..]);
        expect_eq(out[0..n1], "\\xE9\\u4E2D\\uD83D\\uDE00");

        // Line separator (U+2028) & Paragraph separator (U+2029)
        const n2 = jsesc("a\u{2028}b\u{2029}c", out[0..]);
        expect_eq(out[0..n2], "a\\u2028b\\u2029c");
    });

    test("3. @cap/jsesc — ES6 code point escapes and lowercaseHex", () => {
        let out: [256]u8 = undefined;
        let opts = JsescOptions.default();
        opts.es6 = true;
        opts.lowercase_hex = true;
        const n1 = jsescWithOptions("😀中", opts, out[0..]);
        expect_eq(out[0..n1], "\\u{1f600}\\u4e2d");

        opts.lowercase_hex = false;
        const n2 = jsescWithOptions("😀", opts, out[0..]);
        expect_eq(out[0..n2], "\\u{1F600}");
    });

    test("4. @cap/jsesc — JSON mode forces double quotes, wrap, and strict \\uXXXX", () => {
        let out: [256]u8 = undefined;
        const opts = JsescOptions.jsonDefault();
        const n1 = jsescWithOptions("hi \"bob\"\x00é😀", opts, out[0..]);
        expect_eq(out[0..n1], "\"hi \\\"bob\\\"\\u0000\\u00E9\\uD83D\\uDE00\"");
    });

    test("5. @cap/jsesc — is_script_context escapes </script and <!--", () => {
        let out: [256]u8 = undefined;
        let opts = JsescOptions.default();
        opts.is_script_context = true;
        const n1 = jsescWithOptions("</ScRiPt><!--x-->", opts, out[0..]);
        expect_eq(out[0..n1], "\\x3C/ScRiPt>\\x3C!--x-->");

        // Regular '<' not followed by /script or !-- stays untouched
        const n2 = jsescWithOptions("<div>a < b</div>", opts, out[0..]);
        expect_eq(out[0..n2], "<div>a < b</div>");
    });

    test("6. @cap/jsesc — wrap, backtick interpolation, minimal, and escape_everything", () => {
        let out: [256]u8 = undefined;

        // Backtick wrap + ${ guard
        let o1 = JsescOptions.default();
        o1.quote = '`';
        o1.wrap = true;
        const n1 = jsescWithOptions("`hi ${x}`", o1, out[0..]);
        expect_eq(out[0..n1], "`\\`hi \\${x}\\``");

        // minimal preserves UTF-8 non-ASCII except U+2028/U+2029
        let o2 = JsescOptions.default();
        o2.minimal = true;
        const n2 = jsescWithOptions("中文\u{2028}!", o2, out[0..]);
        expect_eq(out[0..n2], "中文\\u2028!");

        // escape_everything escapes printable ASCII too
        let o3 = JsescOptions.default();
        o3.escape_everything = true;
        const n3 = jsescWithOptions("AB", o3, out[0..]);
        expect_eq(out[0..n3], "\\x41\\x42");
    });
}
