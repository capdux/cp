// cap/test/testCapMarkdownStrip.cp
// Comprehensive unit tests for @cap/markdown-strip

import { test, expect_eq } from "cap:test";
import {
    StripOptions,
    stripMarkdown,
    stripMarkdownWith,
    stripToSingleLine,
    excerpt,
} from "../../packages/remove-markdown/src/index.cp";

fn main() {
    test("1. @cap/markdown-strip — ATX headings, Setext underlines & horizontal rules", () => {
        const md = "# Title One\n\nSubheading\n==========\n\n---\n\n### Third Level ###\n\n***\n\nParagraph text.";
        let buf: [512]u8 = undefined;
        const plain = stripMarkdown(md, buf[0..512]);
        expect_eq(plain, "Title One\n\nSubheading\n\nThird Level\n\nParagraph text.");
    });

    test("2. @cap/markdown-strip — inline styles while preserving snake_case & operators", () => {
        const md = "Use **bold**, __strong__, *italic*, _em_, ~~old~~, `inline_code`, snake_case_var, and 2 * 3 < 10.";
        let buf: [512]u8 = undefined;
        const plain = stripMarkdown(md, buf[0..512]);
        expect_eq(plain, "Use bold, strong, italic, em, old, inline_code, snake_case_var, and 2 * 3 < 10.");
    });

    test("3. @cap/markdown-strip — links, images (keep_img_alt) & reference link definitions", () => {
        const md = "See [CapDux](https://capdux.org \"Home\"), [Docs][doc-ref], and ![Logo](logo.png).\n\n[doc-ref]: https://capdux.org/docs";
        let buf: [512]u8 = undefined;
        expect_eq(stripMarkdown(md, buf[0..512]), "See CapDux, Docs, and Logo.");

        let opts = StripOptions.defaults();
        opts.keep_img_alt = false;
        expect_eq(stripMarkdownWith(md, opts, buf[0..512]), "See CapDux, Docs, and .");
    });

    test("4. @cap/markdown-strip — fenced code blocks (keep_code_block_text true vs false)", () => {
        const md = "Intro:\n```cp\nconst x = 1;\n```\nDone.";
        let buf: [512]u8 = undefined;
        expect_eq(stripMarkdown(md, buf[0..512]), "Intro:\nconst x = 1;\nDone.");

        let opts = StripOptions.defaults();
        opts.keep_code_block_text = false;
        expect_eq(stripMarkdownWith(md, opts, buf[0..512]), "Intro:\nDone.");
    });

    test("5. @cap/markdown-strip — blockquotes, lists, task lists & inline HTML stripping", () => {
        const md = "> Quoted **line**\n\n- [x] Done task\n- [ ] Todo task\n1. First item\n2) Second <b>bold</b> item";
        let buf: [512]u8 = undefined;
        const plain = stripMarkdown(md, buf[0..512]);
        expect_eq(plain, "Quoted line\n\nDone task\nTodo task\nFirst item\nSecond bold item");

        let opts = StripOptions.defaults();
        opts.strip_html = false;
        expect_eq(stripMarkdownWith("Hello <span>world</span>", opts, buf[0..512]), "Hello <span>world</span>");
    });

    test("6. @cap/markdown-strip — stripToSingleLine & UTF-8 safe excerpt", () => {
        const md = "# Release Notes\n\n\n\n- **Zero** heap alloc\n- [Fast](https://capdux.org)   indexing";
        let buf: [512]u8 = undefined;
        expect_eq(stripToSingleLine(md, buf[0..512]), "Release Notes Zero heap alloc Fast indexing");

        // Short enough -> not truncated
        expect_eq(excerpt("# Short", 20 as usize, buf[0..512]), "Short");

        // Word-boundary truncation with ellipsis
        expect_eq(excerpt(md, 25 as usize, buf[0..512]), "Release Notes Zero...");

        // Multi-byte UTF-8 safe truncation (Chinese characters are 3 bytes each)
        const zh = "# 中文标题\n\n这是纯文本摘要测试内容";
        const ex_zh = excerpt(zh, 15 as usize, buf[0..512]);
        expect_eq(ex_zh.len <= (15 as usize), true);
        expect_eq(ex_zh[(ex_zh.len - 3)..ex_zh.len], "...");
    });
}
