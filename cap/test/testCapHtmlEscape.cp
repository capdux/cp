import { test, expect_eq } from "cap:test";
import {
    needsEscape,
    escapeHtml,
    unescapeHtml,
} from "../../packages/html-escape/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/html-escape — escapeHtml XSS payload & needsEscape", () => {
        expect_eq(needsEscape("plain text 123"), false);
        expect_eq(needsEscape("<script>alert('x')</script>"), true);

        let buf: [128]u8 = undefined;
        let n: usize = escapeHtml("<a href=\"/x?a=1&b=2\">'hi'</a>", buf[0..]);
        expect_eq(eqStr(buf[0..n], "&lt;a href=&quot;/x?a=1&amp;b=2&quot;&gt;&#39;hi&#39;&lt;/a&gt;"), true);
    });

    test("2. @cap/html-escape — escapeHtml/unescapeHtml roundtrip", () => {
        let orig: string = "1 < 2 && 3 > 1 \"quote\" 'single'";
        let esc: [128]u8 = undefined;
        let en: usize = escapeHtml(orig, esc[0..]);
        let dec_buf: [128]u8 = undefined;
        let dn: usize = unescapeHtml(esc[0..en], dec_buf[0..]);
        expect_eq(eqStr(dec_buf[0..dn], orig), true);
    });

    test("3. @cap/html-escape — decimal & hex numeric character references (incl. UTF-8)", () => {
        let out: [64]u8 = undefined;
        let n1: usize = unescapeHtml("&#65;&#x42;&#39;&#x27;", out[0..]);
        expect_eq(eqStr(out[0..n1], "AB''"), true);

        // Euro sign U+20AC = E2 82 AC
        let n2: usize = unescapeHtml("&#x20AC;100", out[0..]);
        expect_eq(n2, 6);
        expect_eq(out[0], 0xe2);
        expect_eq(out[1], 0x82);
        expect_eq(out[2], 0xac);
        expect_eq(out[3], '1');
    });

    test("4. @cap/html-escape — named HTML entities (&copy;, &nbsp;, &mdash;)", () => {
        let out: [64]u8 = undefined;
        let n: usize = unescapeHtml("&copy;2026&nbsp;CP", out[0..]);
        // © is C2 A9, nbsp is C2 A0
        expect_eq(n, 10);
        expect_eq(out[0], 0xc2);
        expect_eq(out[1], 0xa9);
        expect_eq(out[6], 0xc2);
        expect_eq(out[7], 0xa0);
    });

    test("5. @cap/html-escape — unknown & malformed entities passed through verbatim", () => {
        let out: [64]u8 = undefined;
        let n: usize = unescapeHtml("a &unknown; & &#99999999; &#xD800; ok", out[0..]);
        expect_eq(eqStr(out[0..n], "a &unknown; & &#99999999; &#xD800; ok"), true);
    });

    test("6. @cap/html-escape — empty input & tiny buffer protection", () => {
        let tiny: [4]u8 = undefined;
        expect_eq(escapeHtml("", tiny[0..]), 0);
        expect_eq(escapeHtml("<script>", tiny[0..]), 0);
        expect_eq(unescapeHtml("&amp;hello", tiny[0..]), 0);
    });
}
