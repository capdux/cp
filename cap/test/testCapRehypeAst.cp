// cap/test/testCapRehypeAst.cp
// Comprehensive unit tests for @cap/rehype-ast (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    HAST_ROOT,
    HAST_ELEMENT,
    HAST_TEXT,
    HAST_COMMENT,
    HAST_DOCTYPE,
    HastTree,
    isVoidElement,
} from "../../packages/rehype/src/index.cp";

fn main() {
    test("1. @cap/rehype-ast — full HTML5 document construction & serialization", () => {
        let tree = HastTree.new();
        expect_eq(tree.addDoctype(0) > 0, true);
        const html = tree.addElement(0, "html");
        expect_eq(tree.setAttr(html as usize, "lang", "en"), true);
        const body = tree.addElement(html as usize, "body");
        const h1 = tree.addElement(body as usize, "h1");
        expect_eq(tree.addText(h1 as usize, "Hello CP") > 0, true);

        let out: [512]u8 = undefined;
        const n = tree.toHtml(out[0..512]);
        expect_eq(
            out[0..n],
            "<!doctype html><html lang=\"en\"><body><h1>Hello CP</h1></body></html>",
        );

        expect_eq(HAST_ROOT, 0 as u8);
        expect_eq(HAST_ELEMENT, 1 as u8);
        expect_eq(HAST_TEXT, 2 as u8);
        expect_eq(HAST_COMMENT, 3 as u8);
        expect_eq(HAST_DOCTYPE, 4 as u8);
    });

    test("2. @cap/rehype-ast — all 14 HTML5 void elements serialize without closing tags", () => {
        expect_eq(isVoidElement("area"), true);
        expect_eq(isVoidElement("base"), true);
        expect_eq(isVoidElement("br"), true);
        expect_eq(isVoidElement("col"), true);
        expect_eq(isVoidElement("embed"), true);
        expect_eq(isVoidElement("hr"), true);
        expect_eq(isVoidElement("img"), true);
        expect_eq(isVoidElement("input"), true);
        expect_eq(isVoidElement("link"), true);
        expect_eq(isVoidElement("meta"), true);
        expect_eq(isVoidElement("param"), true);
        expect_eq(isVoidElement("source"), true);
        expect_eq(isVoidElement("track"), true);
        expect_eq(isVoidElement("wbr"), true);
        expect_eq(isVoidElement("div"), false);
        expect_eq(isVoidElement("span"), false);

        let tree = HastTree.new();
        const meta = tree.addElement(0, "meta");
        tree.setAttr(meta as usize, "charset", "utf-8");
        const br = tree.addElement(0, "br");
        const img = tree.addElement(0, "img");
        tree.setAttr(img as usize, "src", "logo.png");
        tree.setAttr(img as usize, "alt", "CP");
        expect_eq(br > 0, true);

        let out: [512]u8 = undefined;
        const n = tree.toHtml(out[0..512]);
        expect_eq(out[0..n], "<meta charset=\"utf-8\"><br><img src=\"logo.png\" alt=\"CP\">");
    });

    test("3. @cap/rehype-ast — XSS-safe escaping of text and attribute values via @cap/escape-html", () => {
        let tree = HastTree.new();
        const a = tree.addElement(0, "a");
        tree.setAttr(a as usize, "title", "1 < 2 & \"quote\"");
        tree.addText(a as usize, "<script>alert('xss')</script>");

        let out: [512]u8 = undefined;
        const n = tree.toHtml(out[0..512]);
        expect_eq(
            out[0..n],
            "<a title=\"1 &lt; 2 &amp; &quot;quote&quot;\">&lt;script&gt;alert(&#39;xss&#39;)&lt;/script&gt;</a>",
        );
    });

    test("4. @cap/rehype-ast — comment nodes and attribute overwrite updates", () => {
        let tree = HastTree.new();
        tree.addComment(0, " header ");
        const div = tree.addElement(0, "div");
        expect_eq(tree.setAttr(div as usize, "id", "old"), true);
        expect_eq(tree.setAttr(div as usize, "class", "box"), true);
        // Overwrite existing `id` attribute
        expect_eq(tree.setAttr(div as usize, "id", "main"), true);
        tree.addText(div as usize, "ok");

        let out: [512]u8 = undefined;
        const n = tree.toHtml(out[0..512]);
        expect_eq(out[0..n], "<!-- header --><div id=\"main\" class=\"box\">ok</div>");
    });

    test("5. @cap/rehype-ast — toText() plain text extraction and countByTag()", () => {
        let tree = HastTree.new();
        tree.addDoctype(0);
        tree.addComment(0, "ignored comment");
        const ul = tree.addElement(0, "ul");
        const li1 = tree.addElement(ul as usize, "li");
        tree.addText(li1 as usize, "Alpha ");
        const li2 = tree.addElement(ul as usize, "li");
        const strong = tree.addElement(li2 as usize, "strong");
        tree.addText(strong as usize, "Beta");
        const li3 = tree.addElement(ul as usize, "li");
        tree.addText(li3 as usize, " Gamma");

        expect_eq(tree.countByTag("li"), 3 as usize);
        expect_eq(tree.countByTag("LI"), 3 as usize);
        expect_eq(tree.countByTag("strong"), 1 as usize);
        expect_eq(tree.countByTag("table"), 0 as usize);
        expect_eq(tree.nodeCount(), 11 as usize);

        let txt: [256]u8 = undefined;
        const tlen = tree.toText(txt[0..256]);
        expect_eq(txt[0..tlen], "Alpha Beta Gamma");
    });

    test("6. @cap/rehype-ast — invalid parent_id, empty tree, and small buffer safety", () => {
        let empty = HastTree.new();
        expect_eq(empty.nodeCount(), 1 as usize);
        let buf: [64]u8 = undefined;
        expect_eq(empty.toHtml(buf[0..64]), 0 as usize);
        expect_eq(empty.toText(buf[0..64]), 0 as usize);

        // Out-of-bounds parent_id & invalid operations rejected safely
        expect_eq(empty.addElement(99, "div"), -1 as i32);
        expect_eq(empty.addText(99, "hi"), -1 as i32);
        expect_eq(empty.setAttr(0, "id", "root"), false); // cannot set attr on root

        // Adding child under void element rejected
        const br = empty.addElement(0, "br");
        expect_eq(empty.addElement(br as usize, "span"), -1 as i32);

        // Small output buffer safely truncates without panic/overflow
        const p = empty.addElement(0, "p");
        empty.addText(p as usize, "hello world");
        let tiny: [6]u8 = undefined;
        const nTiny = empty.toHtml(tiny[0..6]);
        expect_eq(nTiny, 6 as usize);
        expect_eq(tiny[0..nTiny], "<br><p");
    });
}
