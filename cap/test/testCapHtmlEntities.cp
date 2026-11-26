// cap/test/testCapHtmlEntities.cp
// Comprehensive unit tests for @cap/html-entities

import { test, expect_eq } from "cap:test";
import {
    escapeHtml,
    encodeNonAscii,
    decodeHtmlEntities,
    needsHtmlEscape,
} from "../../packages/@cap/html-entities/src/index.cp";

fn main() {
    test("1. @cap/html-entities — XSS 5-character escape & needsHtmlEscape", () => {
        expect_eq(needsHtmlEscape("hello world 123"), false);
        expect_eq(needsHtmlEscape("a < b"), true);
        expect_eq(needsHtmlEscape("a & b"), true);
        expect_eq(needsHtmlEscape("it's"), true);
        expect_eq(needsHtmlEscape("\"quoted\""), true);

        let buf: [256]u8 = undefined;
        const out = escapeHtml("<div class=\"box\" data-x='1'>A & B</div>", buf[0..256]);
        expect_eq(out, "&lt;div class=&quot;box&quot; data-x=&#39;1&#39;&gt;A &amp; B&lt;/div&gt;");
    });

    test("2. @cap/html-entities — high-frequency HTML5 named entities decoding", () => {
        let buf: [256]u8 = undefined;
        const out = decodeHtmlEntities(
            "&lt;a&gt; &amp; &quot;hi&quot; &apos;ok&apos; &copy; &reg; &trade; &mdash; &euro; &rarr; &infin; &hearts;",
            buf[0..256],
        );
        expect_eq(out, "<a> & \"hi\" 'ok' © ® ™ — € → ∞ ♥");

        let buf2: [128]u8 = undefined;
        const math = decodeHtmlEntities("1 &plusmn; 2 &times; 3 &divide; 4 &ne; 5 &le; 6 &ge; 7 &check; &cross;", buf2[0..128]);
        expect_eq(math, "1 ± 2 × 3 ÷ 4 ≠ 5 ≤ 6 ≥ 7 ✓ ✗");
    });

    test("3. @cap/html-entities — decimal & hexadecimal Unicode / Emoji UTF-8 decoding", () => {
        let buf: [256]u8 = undefined;
        // Rocket emoji U+1F680 (decimal 128640, hex 1F680 / 1f680) + CJK U+4E2D (中)
        const out = decodeHtmlEntities("&#128640; &#x1F680; &#X1f680; &#20013; &#x4E2D; &#65;", buf[0..256]);
        expect_eq(out, "🚀 🚀 🚀 中 中 A");
    });

    test("4. @cap/html-entities — encodeNonAscii & round-trip decoding", () => {
        let enc_buf: [256]u8 = undefined;
        const orig = "Hi <b>世界</b> 🚀 & café";
        const encoded = encodeNonAscii(orig, enc_buf[0..256]);
        expect_eq(encoded, "Hi &lt;b&gt;&#x4E16;&#x754C;&lt;/b&gt; &#x1F680; &amp; caf&#xE9;");

        let dec_buf: [256]u8 = undefined;
        const decoded = decodeHtmlEntities(encoded, dec_buf[0..256]);
        expect_eq(decoded, orig);
    });

    test("5. @cap/html-entities — stray '&' & unknown entities preserved verbatim", () => {
        let buf: [256]u8 = undefined;
        const raw = "A & B &unknownEntity; &; &#; &#x; &notclosed &amp; trailing &";
        const out = decodeHtmlEntities(raw, buf[0..256]);
        expect_eq(out, "A & B &unknownEntity; &; &#; &#x; &notclosed & trailing &");
    });

    test("6. @cap/html-entities — empty string & invalid/surrogate codepoint U+FFFD fallback", () => {
        let buf: [128]u8 = undefined;
        expect_eq(escapeHtml("", buf[0..128]), "");
        expect_eq(encodeNonAscii("", buf[0..128]), "");
        expect_eq(decodeHtmlEntities("", buf[0..128]), "");

        // Zero (&#0;), surrogate (&#xD800;, &#xDFFF;), out-of-range (&#x110000;, &#99999999;) -> U+FFFD (EF BF BD)
        const bad = decodeHtmlEntities("&#0;|&#xD800;|&#xDFFF;|&#x110000;|&#99999999;", buf[0..128]);
        // 5 * 3 bytes (U+FFFD) + 4 '|' separators = 19 bytes
        expect_eq(bad.len, 19 as usize);
        expect_eq(bad[0], 0xEF as u8);
        expect_eq(bad[1], 0xBF as u8);
        expect_eq(bad[2], 0xBD as u8);
        expect_eq(bad[3], '|' as u8);
        expect_eq(bad[16], 0xEF as u8);
        expect_eq(bad[17], 0xBF as u8);
        expect_eq(bad[18], 0xBD as u8);
    });
}
