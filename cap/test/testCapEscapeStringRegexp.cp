import { test, expect_eq } from "cap:test";
import {
    escapeStringRegexp, unescapeStringRegexp, escapedRegexpLen, hasRegexpMeta
} from "../../packages/@cap/escape-string-regexp/src/index.cp";

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
    test("1. @cap/escape-string-regexp — Escapes standard regex metacharacters", () => {
        let buf: [128]u8 = undefined;
        let input = "How much $ for a 🦄? (1+2)*3 [a.b]";
        let n = escapeStringRegexp(input, buf[0..128]);
        expect_eq(n, escapedRegexpLen(input));
        expect_eq(eqBytes(buf[0..128], n, "How much \\$ for a 🦄\\? \\(1\\+2\\)\\*3 \\[a\\.b\\]"), true);
    });

    test("2. @cap/escape-string-regexp — Escapes hyphen as \\x2d for character-class safety", () => {
        let buf: [64]u8 = undefined;
        let n = escapeStringRegexp("foo-bar", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n, "foo\\x2dbar"), true);
        expect_eq(n, escapedRegexpLen("foo-bar"));
    });

    test("3. @cap/escape-string-regexp — Escapes backslash, caret, pipe, braces", () => {
        let buf: [64]u8 = undefined;
        let n = escapeStringRegexp("^a|b{1}\\c", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n, "\\^a\\|b\\{1\\}\\\\c"), true);
    });

    test("4. @cap/escape-string-regexp — Round-trip with unescapeStringRegexp", () => {
        let esc: [128]u8 = undefined;
        let out: [128]u8 = undefined;
        let orig = "file-(v1.2)+[test]^$*?\\|{}";
        let ne = escapeStringRegexp(orig, esc[0..128]);
        let nu = unescapeStringRegexp(esc[0..ne], out[0..128]);
        expect_eq(eqBytes(out[0..128], nu, orig), true);
    });

    test("5. @cap/escape-string-regexp — hasRegexpMeta fast detection", () => {
        expect_eq(hasRegexpMeta("plain_identifier_123"), false);
        expect_eq(hasRegexpMeta(""), false);
        expect_eq(hasRegexpMeta("a.b"), true);
        expect_eq(hasRegexpMeta("foo-bar"), true);
        expect_eq(hasRegexpMeta("$100"), true);
    });

    test("6. @cap/escape-string-regexp — Empty input and small buffer safety", () => {
        let buf: [4]u8 = undefined;
        expect_eq(escapeStringRegexp("", buf[0..4]), 0);
        // "a-b": 'a' (1) + "\\x2d" (4) won't fit -> stops cleanly after 'a'
        let n = escapeStringRegexp("a-b", buf[0..4]);
        expect_eq(n, 1);
        expect_eq(buf[0], 97);
    });
}
