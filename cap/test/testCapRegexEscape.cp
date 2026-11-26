// cap/test/testCapRegexEscape.cp
// Comprehensive unit tests for @cap/regex-escape (escape-string-regexp / regex::escape parity)

import { test, expect_eq } from "cap:test";
import {
    isMetaChar,
    needsEscape,
    escapedLen,
    escapeRegex,
    escapeCharClass,
    unescapeRegexLiteral,
} from "../../packages/@cap/regex-escape/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/regex-escape — escapes all 15 metacharacters plus '/'", () => {
        const all_meta = "\\.+*?()|[]{}^$-/";
        let i: usize = 0;
        while (i < 15) {
            expect_eq(isMetaChar(all_meta[i]), true);
            i += 1;
        }
        expect_eq(isMetaChar('a' as u8), false);

        let out: [64]u8 = undefined;
        const n: usize = escapeRegex(all_meta, out[0..64]);
        expect_eq(n, 32 as usize);
        expect_eq(
            sliceEq(out[0..n], "\\\\\\.\\+\\*\\?\\(\\)\\|\\[\\]\\{\\}\\^\\$\\-\\/"),
            true,
        );
    });

    test("2. @cap/regex-escape — escapedLen matches exact output length", () => {
        const s = "https://capdux.org/v1?q=a+b*(c)";
        let out: [128]u8 = undefined;
        const n: usize = escapeRegex(s, out[0..128]);
        expect_eq(n, escapedLen(s));
        expect_eq(n > s.len, true);
    });

    test("3. @cap/regex-escape — needsEscape fast-path check", () => {
        expect_eq(needsEscape("hello_world123"), false);
        expect_eq(needsEscape(""), false);
        expect_eq(needsEscape("file.txt"), true);
        expect_eq(needsEscape("a/b"), true);
        expect_eq(needsEscape("a-b"), true);
    });

    test("4. @cap/regex-escape — escapeCharClass escapes \\, ], -, ^ only", () => {
        let out: [64]u8 = undefined;
        const n: usize = escapeCharClass("a-z^0\\]1.*+", out[0..64]);
        expect_eq(sliceEq(out[0..n], "a\\-z\\^0\\\\\\]1.*+"), true);
    });

    test("5. @cap/regex-escape — escapeRegex -> unescapeRegexLiteral round-trip", () => {
        const orig = "price: $9.99 (USD) [a-z]+ /path\\dir";
        let esc: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;

        const en: usize = escapeRegex(orig, esc[0..128]);
        expect_eq(en, escapedLen(orig));

        const un: usize = unescapeRegexLiteral(esc[0..en], dec_buf[0..128]);
        expect_eq(sliceEq(dec_buf[0..un], orig), true);
    });

    test("6. @cap/regex-escape — empty input & tiny buffer overflow protection", () => {
        let buf: [16]u8 = undefined;
        expect_eq(escapeRegex("", buf[0..16]), 0 as usize);
        expect_eq(escapeCharClass("", buf[0..16]), 0 as usize);
        expect_eq(unescapeRegexLiteral("", buf[0..16]), 0 as usize);
        expect_eq(escapedLen(""), 0 as usize);

        let tiny: [4]u8 = undefined;
        expect_eq(escapeRegex("a.b.c", tiny[0..4]), 0 as usize);
        expect_eq(escapeCharClass("a-b-c", tiny[0..4]), 0 as usize);
        expect_eq(unescapeRegexLiteral("a\\.b\\.c", tiny[0..4]), 0 as usize);
    });
}
