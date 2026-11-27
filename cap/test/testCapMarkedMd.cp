import { test, expect_eq } from "cap:test";
import { strEq } from "cap:ascii";
import {
    MarkedOptions,
    defaultMarkedOptions,
    parseMarkdown,
    parseMarkdownWithOptions,
    parseInlineMarkdown,
    stripMarkdown,
} from "../../packages/marked/src/index.cp";

fn main() {
    test("1. @cap/marked-md — ATX headings, horizontal rules, paragraphs & hard breaks", () => {
        let out: [2048]u8 = undefined;
        const md = "# Title\n## Subtitle ##\n###### Small\n\n---\n\nLine 1  \nLine 2\\\nLine 3\n";
        const n = parseMarkdown(md, &out);
        const expected = "<h1>Title</h1>\n<h2>Subtitle</h2>\n<h6>Small</h6>\n<hr>\n<p>Line 1<br>\nLine 2<br>\nLine 3</p>\n";
        expect_eq(strEq(out[0..n], expected), true);

        // With opts.breaks = true, single newline becomes <br>\n
        let opts = defaultMarkedOptions();
        opts.breaks = true;
        const n2 = parseMarkdownWithOptions("Alpha\nBeta", opts, &out);
        expect_eq(strEq(out[0..n2], "<p>Alpha<br>\nBeta</p>\n"), true);
    });

    test("2. @cap/marked-md — Fenced code blocks with language class & HTML escaping", () => {
        let out: [2048]u8 = undefined;
        const md = "```ts\nconst x = a < b && c > d;\n```\n\n~~~\nplain & \"quoted\"\n~~~\n";
        const n = parseMarkdown(md, &out);
        const expected = "<pre><code class=\"language-ts\">const x = a &lt; b &amp;&amp; c &gt; d;\n</code></pre>\n<pre><code>plain &amp; &quot;quoted&quot;\n</code></pre>\n";
        expect_eq(strEq(out[0..n], expected), true);
    });

    test("3. @cap/marked-md — Blockquotes, unordered/task lists & ordered lists", () => {
        let out: [2048]u8 = undefined;
        const md = "> **Note:** Read carefully\n> Second line\n\n- [x] Done\n- [ ] Todo\n* Plain *item*\n\n3. Third\n4. Fourth\n";
        const n = parseMarkdown(md, &out);
        const expected = "<blockquote>\n<p><strong>Note:</strong> Read carefully\nSecond line</p>\n</blockquote>\n<ul>\n<li><input checked=\"\" disabled=\"\" type=\"checkbox\"> Done</li>\n<li><input disabled=\"\" type=\"checkbox\"> Todo</li>\n<li>Plain <em>item</em></li>\n</ul>\n<ol start=\"3\">\n<li>Third</li>\n<li>Fourth</li>\n</ol>\n";
        expect_eq(strEq(out[0..n], expected), true);
    });

    test("4. @cap/marked-md — Inline syntax (code, bold, italic, strikethrough, escapes)", () => {
        let out: [2048]u8 = undefined;
        const src = "Use `a < b` with **bold**, __also__, *em*, _ital_, ***both***, ~~old~~, snake_case_id, and \\*lit\\*";
        const n = parseInlineMarkdown(src, &out);
        const expected = "Use <code>a &lt; b</code> with <strong>bold</strong>, <strong>also</strong>, <em>em</em>, <em>ital</em>, <strong><em>both</em></strong>, <del>old</del>, snake_case_id, and *lit*";
        expect_eq(strEq(out[0..n], expected), true);

        // Double backtick code span containing a single backtick
        const n2 = parseInlineMarkdown("``use `tick` here``", &out);
        expect_eq(strEq(out[0..n2], "<code>use `tick` here</code>"), true);
    });

    test("5. @cap/marked-md — Links, images, XSS protocol sanitization & GFM option toggle", () => {
        let out: [2048]u8 = undefined;
        const md = "[**Cap**](https://cap.dev?a=1&b=2) ![Logo & \"Co\"](/img/logo.png)";
        const n = parseInlineMarkdown(md, &out);
        const expected = "<a href=\"https://cap.dev?a=1&amp;b=2\"><strong>Cap</strong></a> <img src=\"/img/logo.png\" alt=\"Logo &amp; &quot;Co&quot;\">";
        expect_eq(strEq(out[0..n], expected), true);

        // Dangerous schemes sanitized by default
        const xss = "[click](  JaVaScRiPt:alert(1)) [vb](vbscript:msg) [d](data:text/html,<script>)";
        const nx = parseInlineMarkdown(xss, &out);
        const expected_xss = "<a href=\"\">click</a> <a href=\"\">vb</a> <a href=\"\">d</a>";
        expect_eq(strEq(out[0..nx], expected_xss), true);

        // Disabling gfm keeps ~~text~~ literal
        let opts = defaultMarkedOptions();
        opts.gfm = false;
        const ng = parseMarkdownWithOptions("~~kept~~", opts, &out);
        expect_eq(strEq(out[0..ng], "<p>~~kept~~</p>\n"), true);
    });

    test("6. @cap/marked-md — stripMarkdown plain-text extraction & small-buffer safety", () => {
        let out: [2048]u8 = undefined;
        const md = "# Hello **World**\n\n> Quoted *text* with `code` and ~~old~~\n\n- [Cap](https://cap.dev) ![Icon](/i.png)\n\n```\nraw()\n```\n";
        const n = stripMarkdown(md, &out);
        const expected = "Hello World\nQuoted text with code and old\nCap Icon\nraw()";
        expect_eq(strEq(out[0..n], expected), true);

        // Small buffer truncation never overflows
        let tiny: [8]u8 = undefined;
        const nt = parseMarkdown("# Long Heading", &tiny);
        expect_eq(nt, 8);
        expect_eq(strEq(tiny[0..nt], "<h1>Long"), true);
    });
}
