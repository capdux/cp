import { test, expect_eq } from "cap:test";
import {
    HtmlTokenizer, HtmlAttr, parseAttributes, extractTextContent, isVoidElement,
    HTML_TOK_DOCTYPE, HTML_TOK_START_TAG, HTML_TOK_END_TAG, HTML_TOK_SELF_CLOSING,
    HTML_TOK_COMMENT, HTML_TOK_TEXT, HTML_TOK_EOF
} from "../../packages/@cap/parse5/src/index.cp";

fn eqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/parse5 — Tokenizes DOCTYPE, start/end tags, text & EOF", () => {
        let tok = HtmlTokenizer.new("<!DOCTYPE html><div>Hello</div>");
        let t1 = tok.next();
        expect_eq(t1.kind, HTML_TOK_DOCTYPE);
        let t2 = tok.next();
        expect_eq(t2.kind, HTML_TOK_START_TAG);
        expect_eq(t2.name, "div");
        let t3 = tok.next();
        expect_eq(t3.kind, HTML_TOK_TEXT);
        expect_eq(t3.text, "Hello");
        let t4 = tok.next();
        expect_eq(t4.kind, HTML_TOK_END_TAG);
        expect_eq(t4.name, "div");
        expect_eq(tok.next().kind, HTML_TOK_EOF);
    });

    test("2. @cap/parse5 — Self-closing tags & HTML comments", () => {
        let tok = HtmlTokenizer.new("<!-- note --><img src=\"a.png\" />");
        let c = tok.next();
        expect_eq(c.kind, HTML_TOK_COMMENT);
        expect_eq(c.text, " note ");
        let img = tok.next();
        expect_eq(img.kind, HTML_TOK_SELF_CLOSING);
        expect_eq(img.name, "img");
    });

    test("3. @cap/parse5 — parseAttributes double/single/unquoted & boolean attrs", () => {
        let tok = HtmlTokenizer.new("<input type=\"text\" name='user' size=20 disabled>");
        let t = tok.next();
        let attrs: [8]HtmlAttr = undefined;
        let n = parseAttributes(t.raw_attrs, attrs[0..8]);
        expect_eq(n, 4);
        expect_eq(attrs[0].name, "type");
        expect_eq(attrs[0].value, "text");
        expect_eq(attrs[1].name, "name");
        expect_eq(attrs[1].value, "user");
        expect_eq(attrs[2].name, "size");
        expect_eq(attrs[2].value, "20");
        expect_eq(attrs[3].name, "disabled");
        expect_eq(attrs[3].value, "");
    });

    test("4. @cap/parse5 — Quoted '>' inside attribute value does not terminate tag early", () => {
        let tok = HtmlTokenizer.new("<a title=\"1 > 0\" href=\"/\">ok</a>");
        let a = tok.next();
        expect_eq(a.kind, HTML_TOK_START_TAG);
        expect_eq(a.name, "a");
        let attrs: [4]HtmlAttr = undefined;
        let n = parseAttributes(a.raw_attrs, attrs[0..4]);
        expect_eq(n, 2);
        expect_eq(attrs[0].value, "1 > 0");
        expect_eq(attrs[1].value, "/");
    });

    test("5. @cap/parse5 — extractTextContent strips markup & comments cleanly", () => {
        let out: [128]u8 = undefined;
        let n = extractTextContent("<h1>Hi</h1><!-- skip --><p> World!</p>", out[0..128]);
        expect_eq(eqBuf(out[0..n], n, "Hi World!"), true);
    });

    test("6. @cap/parse5 — isVoidElement case-insensitive check via @cap/unicase", () => {
        expect_eq(isVoidElement("IMG"), true);
        expect_eq(isVoidElement("br"), true);
        expect_eq(isVoidElement("meta"), true);
        expect_eq(isVoidElement("div"), false);
        expect_eq(isVoidElement("span"), false);
    });
}
