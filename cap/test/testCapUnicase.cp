import { test, expect_eq } from "cap:test";
import {
    Ascii, UniCase, eqAscii, cmpAscii, hashAscii,
    eqUnicode, cmpUnicode, hashUnicode, caseFold,
    startsWithIgnoreCase, endsWithIgnoreCase, containsIgnoreCase
} from "../../packages/unicase/src/index.cp";

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
    test("1. @cap/unicase — Ascii wrapper equality, ordering & consistent hashing", () => {
        let a = Ascii.new("Content-Type");
        let b = Ascii.new("content-type");
        let c = Ascii.new("CONTENT-LENGTH");
        expect_eq(a.eq(b), true);
        expect_eq(a.cmp(b), 0);
        expect_eq(a.hash(), b.hash());
        expect_eq(c.cmp(a), -1);
        expect_eq(a.cmp(c), 1);
        expect_eq(a.intoInner().len, 12);
    });

    test("2. @cap/unicase — Unicode German sharp s (ß == SS) & multi-codepoint folding", () => {
        let a = UniCase.new("Straße");
        let b = UniCase.new("STRASSE");
        let c = UniCase.new("strasse");
        expect_eq(a.eq(b), true);
        expect_eq(b.eq(c), true);
        expect_eq(a.cmp(b), 0);
        expect_eq(a.hash(), b.hash());
        expect_eq(a.hash(), c.hash());
    });

    test("3. @cap/unicase — Latin-1, Greek final sigma & Cyrillic case folding", () => {
        expect_eq(eqUnicode("CAFÉ", "café"), true);
        expect_eq(eqUnicode("ÜBER", "über"), true);
        expect_eq(eqUnicode("ΟΔΥΣΣΕΎΣ", "οδυσσεύς"), true);
        expect_eq(eqUnicode("ПРИВЕТ", "привет"), true);
        expect_eq(eqUnicode("ЁЖИК", "ёжик"), true);
        expect_eq(eqUnicode("Привет", "Пока"), false);
    });

    test("4. @cap/unicase — caseFold UTF-8 output buffer", () => {
        let out: [64]u8 = undefined;
        let n = caseFold("Groß-CAFÉ", out[0..64]);
        expect_eq(eqBytes(out[0..n], n, "gross-café"), true);
        let n2 = caseFold("Hello World!", out[0..64]);
        expect_eq(eqBytes(out[0..n2], n2, "hello world!"), true);
    });

    test("5. @cap/unicase — startsWith/endsWith/containsIgnoreCase helpers", () => {
        expect_eq(startsWithIgnoreCase("Authorization: Bearer xyz", "authorization:"), true);
        expect_eq(endsWithIgnoreCase("index.HTML", ".html"), true);
        expect_eq(containsIgnoreCase("X-Custom-Header", "custom"), true);
        expect_eq(containsIgnoreCase("abc", "abcd"), false);
        expect_eq(containsIgnoreCase("abc", ""), true);
    });

    test("6. @cap/unicase — Unicode ordering & Kelvin/Angstrom special signs", () => {
        expect_eq(eqUnicode("\u{212A}", "k"), true);
        expect_eq(eqUnicode("\u{212B}", "å"), true);
        expect_eq(cmpUnicode("alpha", "BETA"), -1);
        expect_eq(cmpUnicode("ZETA", "beta"), 1);
        expect_eq(eqAscii("aBc", "AbC"), true);
        expect_eq(cmpAscii("ab", "abc"), -1);
        expect_eq(hashAscii("HTTP"), hashAscii("http"));
    });
}
