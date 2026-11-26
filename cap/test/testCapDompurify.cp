import { test, expect_eq } from "cap:test";
import {
    SanitizeHtmlOptions,
    defaultSanitizeOptions,
    sanitizeHtml,
    isSafeHtml,
} from "../../packages/@cap/dompurify/src/index.cp";

fn main() {
    test("1. @cap/dompurify — Preserves safe HTML tags and attributes", () => {
        const opts: SanitizeHtmlOptions = defaultSanitizeOptions();
        let out: [256]u8 = undefined;
        const input: string = "<div class=\"card\"><h2>Title</h2><p>Hello <b>world</b> and <a href=\"https://example.com\" title=\"ok\">link</a></p></div>";
        const n: usize = sanitizeHtml(input, opts, out[0..]);
        expect_eq(out[0..n], input);
        expect_eq(isSafeHtml(input), true);
    });

    test("2. @cap/dompurify — Removes script, style, iframe, object, embed, and form tags", () => {
        const opts: SanitizeHtmlOptions = defaultSanitizeOptions();
        let out: [256]u8 = undefined;
        const input: string = "<p>Hi</p><script>console.log(1)</script><style>body{color:red}</style><iframe src=\"x\">inner</iframe><object>bad</object><embed src=\"y\"><form><input></form><p>End</p>";
        const n: usize = sanitizeHtml(input, opts, out[0..]);
        expect_eq(out[0..n], "<p>Hi</p><p>End</p>");
        expect_eq(isSafeHtml(input), false);
    });

    test("3. @cap/dompurify — Strips all on* event handler attributes", () => {
        const opts: SanitizeHtmlOptions = defaultSanitizeOptions();
        let out: [256]u8 = undefined;
        const input: string = "<img src=\"https://example.com/a.png\" onerror=\"run()\" alt=\"pic\" ONLOAD=\"x()\"/>";
        const n: usize = sanitizeHtml(input, opts, out[0..]);
        expect_eq(out[0..n], "<img src=\"https://example.com/a.png\" alt=\"pic\"/>");
        expect_eq(isSafeHtml(input), false);
        expect_eq(isSafeHtml(out[0..n]), true);
    });

    test("4. @cap/dompurify — Filters javascript:, vbscript:, and data: URIs in href/src", () => {
        const opts: SanitizeHtmlOptions = defaultSanitizeOptions();
        let out: [256]u8 = undefined;
        const input: string = "<a href=\"  JaVaScRiPt:void(0)\">click</a><a href=\"vbscript:msg\">v</a><img src=\"data:text/html,hi\" alt=\"x\">";
        const n: usize = sanitizeHtml(input, opts, out[0..]);
        expect_eq(out[0..n], "<a>click</a><a>v</a><img alt=\"x\">");
        expect_eq(isSafeHtml(input), false);
    });

    test("5. @cap/dompurify — Respects allow_links, allow_images, and strip_comments options", () => {
        const strict: SanitizeHtmlOptions = SanitizeHtmlOptions {
            allow_links: false,
            allow_images: false,
            strip_comments: true,
        };
        let out: [256]u8 = undefined;
        const input: string = "<!-- note --><p><a href=\"https://a.com\">text</a><img src=\"a.png\"></p>";
        const n: usize = sanitizeHtml(input, strict, out[0..]);
        expect_eq(out[0..n], "<p>text</p>");

        const keep_comments: SanitizeHtmlOptions = SanitizeHtmlOptions {
            allow_links: true,
            allow_images: true,
            strip_comments: false,
        };
        const m: usize = sanitizeHtml("<!-- ok --><b>hi</b>", keep_comments, out[0..]);
        expect_eq(out[0..m], "<!-- ok --><b>hi</b>");
    });

    test("6. @cap/dompurify — Safe lists/code/br/hr tags and output buffer truncation safety", () => {
        const opts: SanitizeHtmlOptions = defaultSanitizeOptions();
        let out: [256]u8 = undefined;
        const input: string = "<ul><li><code>x</code></li></ul><br/><hr/>";
        const n: usize = sanitizeHtml(input, opts, out[0..]);
        expect_eq(out[0..n], input);
        expect_eq(isSafeHtml(input), true);

        let small: [8]u8 = undefined;
        const m: usize = sanitizeHtml(input, opts, small[0..]);
        expect_eq(m, 8);
        expect_eq(small[0..m], "<ul><li>");
    });
}
