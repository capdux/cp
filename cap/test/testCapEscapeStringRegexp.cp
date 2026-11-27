import { test, expect_eq } from "cap:test";
import {
    isRegexpMeta,
    hasRegexpMeta,
    needsRegexEscape,
    escapedRegexpLen,
    escapedLen,
    escapeStringRegexp,
    escapeStringRegexpChecked,
    escapeCharClass,
    unescapedLen,
    unescapeStringRegexp,
    unescapeRegexp,
} from "../../packages/escape-string-regexp/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. escape-string-regexp — Escapes standard regex metacharacters", () => {
        let buf: [128]u8 = undefined;
        let input = "How much $ for a 🦄? (1+2)*3 [a.b]";
        let n = escapeStringRegexp(input, buf[0..128]);
        expect_eq(n, escapedRegexpLen(input));
        expect_eq(n, escapedLen(input));
        expect_eq(eqBytes(buf[0..128], n, "How much \\$ for a 🦄\\? \\(1\\+2\\)\\*3 \\[a\\.b\\]"), true);
    });

    test("2. escape-string-regexp — Escapes hyphen as \\x2d for character-class safety", () => {
        let buf: [64]u8 = undefined;
        let n = escapeStringRegexp("foo-bar", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n, "foo\\x2dbar"), true);
        expect_eq(n, escapedRegexpLen("foo-bar"));
    });

    test("3. escape-string-regexp — Escapes backslash, caret, pipe, braces", () => {
        let buf: [64]u8 = undefined;
        let n = escapeStringRegexp("^a|b{1}\\c", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n, "\\^a\\|b\\{1\\}\\\\c"), true);
    });

    test("4. escape-string-regexp — Round-trip with unescapeStringRegexp & unescapeRegexp", () => {
        let esc: [128]u8 = undefined;
        let out: [128]u8 = undefined;
        let orig = "file-(v1.2)+[test]^$*?\\|{}";
        let ne = escapeStringRegexp(orig, esc[0..128]);
        let nu = unescapeStringRegexp(esc[0..ne], out[0..128]);
        expect_eq(nu, unescapedLen(esc[0..ne]));
        expect_eq(eqBytes(out[0..128], nu, orig), true);

        // unescapeRegexp alias also supports uppercase \x2D and \-
        let n2 = unescapeRegexp("a\\x2Db\\-c\\.d", out[0..128]);
        expect_eq(eqBytes(out[0..128], n2, "a-b-c.d"), true);
    });

    test("5. escape-string-regexp — hasRegexpMeta, needsRegexEscape & isRegexpMeta", () => {
        expect_eq(hasRegexpMeta("plain_identifier_123"), false);
        expect_eq(needsRegexEscape("plain_identifier_123"), false);
        expect_eq(hasRegexpMeta(""), false);
        expect_eq(hasRegexpMeta("a.b"), true);
        expect_eq(needsRegexEscape("foo-bar"), true);
        expect_eq(needsRegexEscape("$100"), true);

        expect_eq(isRegexpMeta('-'), true);
        expect_eq(isRegexpMeta('*'), true);
        expect_eq(isRegexpMeta('?'), true);
        expect_eq(isRegexpMeta('a'), false);
    });

    test("6. escape-string-regexp — Empty input and small buffer safety", () => {
        let buf: [4]u8 = undefined;
        expect_eq(escapeStringRegexp("", buf[0..4]), 0 as usize);
        // "a-b": 'a' (1) + "\\x2d" (4) won't fit -> stops cleanly after 'a'
        let n = escapeStringRegexp("a-b", buf[0..4]);
        expect_eq(n, 1 as usize);
        expect_eq(buf[0], 97 as u8);
    });

    test("7. escape-string-regexp — escapeStringRegexpChecked rejects undersized buffer", () => {
        let small: [4]u8 = undefined;
        expect_eq(escapeStringRegexpChecked("a-b", small[0..4]) == null, true);

        let exact: [6]u8 = undefined;
        const ok = escapeStringRegexpChecked("a-b", exact[0..6]);
        expect_eq(ok ?? 0, 6 as usize);
        expect_eq(eqBytes(exact[0..6], 6, "a\\x2db"), true);
    });

    test("8. escape-string-regexp — escapeCharClass escapes only class-special chars", () => {
        let buf: [64]u8 = undefined;
        const n = escapeCharClass("a.b+c]d^e-f\\g", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n, "a.b+c\\]d\\^e\\x2df\\\\g"), true);
    });
}
