import { test, expect_eq } from "cap:test";
import {
    escapeHtml,
    escapedHtmlLen,
    needsHtmlEscape,
    unescapeHtml,
} from "../../packages/escape-html/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/escape-html — Escapes all 5 special HTML characters", () => {
        const src: string = "&<>\"'";
        expect_eq(needsHtmlEscape(src), true);
        expect_eq(escapedHtmlLen(src), 24); // 5 + 4 + 4 + 6 + 5

        let buf: [64]u8 = undefined;
        const n = escapeHtml(src, &buf);
        expect_eq(n, 24);
        expect_eq(strEq(buf[0..n], "&amp;&lt;&gt;&quot;&#39;"), true);
    });

    test("2. @cap/escape-html — Plain text passes through unchanged", () => {
        const plain: string = "Hello, CP 2026! (a + b = c)";
        expect_eq(needsHtmlEscape(plain), false);
        expect_eq(escapedHtmlLen(plain), plain.len);

        let buf: [64]u8 = undefined;
        const n = escapeHtml(plain, &buf);
        expect_eq(n, plain.len);
        expect_eq(strEq(buf[0..n], plain), true);
    });

    test("3. @cap/escape-html — Markup tag neutralization & escapedHtmlLen parity", () => {
        const markup: string = "<div data-msg=\"x\" class='a'>1 & 2</div>";
        const expected: string = "&lt;div data-msg=&quot;x&quot; class=&#39;a&#39;&gt;1 &amp; 2&lt;/div&gt;";
        expect_eq(escapedHtmlLen(markup), expected.len);

        let buf: [128]u8 = undefined;
        const n = escapeHtml(markup, &buf);
        expect_eq(n, expected.len);
        expect_eq(strEq(buf[0..n], expected), true);
    });

    test("4. @cap/escape-html — unescapeHtml round-trip including &#39;, &#x27; & &apos;", () => {
        const orig: string = "<a href=\"/test?a=1&b=2\">It's ok</a>";
        let esc: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;
        const ne = escapeHtml(orig, &esc);
        const nd = unescapeHtml(esc[0..ne], &dec_buf);
        expect_eq(strEq(dec_buf[0..nd], orig), true);

        const quotes: string = "&#39; &#x27; &#X27; &apos;";
        const nq = unescapeHtml(quotes, &dec_buf);
        expect_eq(strEq(dec_buf[0..nq], "' ' ' '"), true);
    });

    test("5. @cap/escape-html — Unknown or malformed entities preserved verbatim", () => {
        const raw: string = "a &foo; b &amp; c &123 d &";
        let buf: [64]u8 = undefined;
        const n = unescapeHtml(raw, &buf);
        expect_eq(strEq(buf[0..n], "a &foo; b & c &123 d &"), true);
    });

    test("6. @cap/escape-html — Empty input & small buffer truncation safety", () => {
        let buf: [8]u8 = undefined;
        expect_eq(needsHtmlEscape(""), false);
        expect_eq(escapedHtmlLen(""), 0);
        expect_eq(escapeHtml("", &buf), 0);
        expect_eq(unescapeHtml("", &buf), 0);

        const empty_out: []u8 = buf[0..0];
        expect_eq(escapeHtml("<a>", empty_out), 0);
        expect_eq(unescapeHtml("&lt;", empty_out), 0);

        // Truncates cleanly at out.len without writing out of bounds
        let small: [6]u8 = undefined;
        const ns = escapeHtml("<b>hi</b>", &small);
        expect_eq(ns, 6);
        expect_eq(strEq(small[0..ns], "&lt;b&"), true);
    });
}
