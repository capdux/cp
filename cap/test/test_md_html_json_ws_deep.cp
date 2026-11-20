// cap/test/test_md_html_json_ws_deep.cp
// Comprehensive Deep Regression Suite for:
//   cap:md, cap:html, cap:json, cap:ws
// Covers CommonMark/GFM AST + HTML + XSS sanitization, HTML5 Tokenizer + CSS compound selectors + HTMLRewriter,
// RFC 8259 JSON + UTF-16 surrogates + Serializer + RFC 6901 JSON Pointer + equals,
// RFC 6455 WebSocket Frame RSV/Validation + SHA-1/Base64 Handshake + Fragment Assembler + Close/Auto-Pong.

const std = @import("std");
import { test, expect_eq } from "cap:test";

import {
    MarkdownOptions,
    MdOptions,
    MdBlockKind,
    MdBlock,
    MdRenderer,
    MdDocument,
    MdParser,
    Markdown,
    MdTableAlign,
    MdTable,
} from "cap:md";

import {
    HtmlEscape,
    HtmlTokenKind,
    HtmlToken,
    HtmlTokenizer,
    CssSelector,
    HtmlElement,
    HtmlRewriter,
} from "cap:html";

import {
    JSON,
    JsonParser,
    JsonParseOptions,
    JsonValue,
    JsonTag,
    JsonSerializer,
} from "cap:json";

import {
    Opcode,
    CloseCode,
    FrameHeader,
    FrameBuilder,
    ClosePayload,
    Mask,
    WebSocketHandshake,
    WebSocketMessageKind,
    WebSocketMessageAssembler,
} from "cap:ws";

// Helper callbacks for AST traversal & HTMLRewriter tests
struct AstCountCtx {
    headings: usize = 0,
    code_blocks: usize = 0,
    tables: usize = 0,
    quotes: usize = 0,
}

fn onAstEnter(blk: MdBlock, raw_ctx: (*anyopaque)?) {
    if (raw := raw_ctx) {
        unsafe {
            let ctx = raw as *AstCountCtx!;
            if (blk.kind == .Heading) ctx.headings += 1;
            if (blk.kind == .CodeBlock) ctx.code_blocks += 1;
            if (blk.kind == .Table) ctx.tables += 1;
            if (blk.kind == .BlockQuote) ctx.quotes += 1;
        }
    }
}

fn rewriteCardHandler(el: *HtmlElement) {
    el.setTagName("article");
    el.setAttribute("data-enhanced", "true");
    el.removeAttribute("data-legacy");
    el.before("<!-- card-start -->", true);
    el.after("<!-- card-end -->", true);
    el.prepend("<header>Card</header>", true);
    el.append("<footer>End</footer>", true);
}

fn rewriteBadgeHandler(el: *HtmlElement) {
    el.setInnerContent("NEW & IMPROVED", false);
}

fn rewriteDeleteHandler(el: *HtmlElement) {
    el.remove();
}

fn rewriteReplaceHandler(el: *HtmlElement) {
    el.replaceWith("<span>REPLACED</span>", true);
}

// ============================================================================
// 1. cap:md — Full CommonMark + GFM Blocks, Inlines, AST & XSS Sanitization
// ============================================================================
fn testMarkdownDeep(): !void {
    const allocator = std.heap.page_allocator;

    // 1.1 Setext headings, ATX headings (#..######), headingIds + deduplication
    const opts_ids = MarkdownOptions{
        gfm: true,
        tables: true,
        strikethrough: true,
        tasklists: true,
        autolinks: true,
        sanitizeHtml: true,
        headingIds: true,
    };
    const headings_md = "Setext H1\n===\nSetext H2\n---\n### ATX 3 ###\n# Setext H1\n";
    const headings_html = try Markdown.toHtml(allocator, headings_md, opts_ids);
    defer allocator.free(headings_html);
    expect_eq(std.mem.indexOf(u8, headings_html, "<h1 id=\"setext-h1\">Setext H1</h1>") != null, true);
    expect_eq(std.mem.indexOf(u8, headings_html, "<h2 id=\"setext-h2\">Setext H2</h2>") != null, true);
    expect_eq(std.mem.indexOf(u8, headings_html, "<h3 id=\"atx-3\">ATX 3</h3>") != null, true);
    expect_eq(std.mem.indexOf(u8, headings_html, "<h1 id=\"setext-h1-1\">Setext H1</h1>") != null, true);

    // 1.2 Fenced code blocks (``` and ~~~, nested backtick fence lengths, indented code, HR)
    const code_md = "````md\n```js\nlet a = 1 < 2;\n```\n````\n\n~~~\ntilde code\n~~~\n\n    indented()\n\n---\n***\n___\n";
    const code_html = try Markdown.toHtml(allocator, code_md, opts_ids);
    defer allocator.free(code_html);
    expect_eq(std.mem.indexOf(u8, code_html, "<pre><code class=\"language-md\">```js\nlet a = 1 &lt; 2;\n```\n</code></pre>") != null, true);
    expect_eq(std.mem.indexOf(u8, code_html, "<pre><code>tilde code\n</code></pre>") != null, true);
    expect_eq(std.mem.indexOf(u8, code_html, "<pre><code>indented()\n</code></pre>") != null, true);
    expect_eq(std.mem.indexOf(u8, code_html, "<hr />\n") != null, true);

    // 1.3 Nested blockquotes, ordered lists with custom start, nested lists, tasklists
    const list_md = "> Outer\n> > Inner quote\n\n5. Fifth\n6. Sixth\n\n- [x] Done task\n- [ ] Todo task\n  - Nested bullet\n";
    const list_html = try Markdown.toHtml(allocator, list_md, opts_ids);
    defer allocator.free(list_html);
    expect_eq(std.mem.indexOf(u8, list_html, "<blockquote>\n<p>Outer</p>\n<blockquote>\n<p>Inner quote</p>\n</blockquote>\n</blockquote>") != null, true);
    expect_eq(std.mem.indexOf(u8, list_html, "<ol start=\"5\">\n<li>Fifth</li>\n<li>Sixth</li>\n</ol>") != null, true);
    expect_eq(std.mem.indexOf(u8, list_html, "<li class=\"task-list-item\"><input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled checked /> Done task") != null, true);
    expect_eq(std.mem.indexOf(u8, list_html, "<li class=\"task-list-item\"><input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled /> Todo task") != null, true);
    expect_eq(std.mem.indexOf(u8, list_html, "<ul>\n<li>Nested bullet</li>\n</ul>") != null, true);

    // 1.4 GFM Table with inline formatting and escaped pipes
    const tbl_md = "| Name | Info | Score |\n| :--- | :---: | ---: |\n| **CP** | `a|b` & c | ~~10~~ 99 |\n";
    const tbl_html = try Markdown.toHtml(allocator, tbl_md, opts_ids);
    defer allocator.free(tbl_html);
    expect_eq(std.mem.indexOf(u8, tbl_html, "<th align=\"left\">Name</th><th align=\"center\">Info</th><th align=\"right\">Score</th>") != null, true);
    expect_eq(std.mem.indexOf(u8, tbl_html, "<strong>CP</strong>") != null, true);
    expect_eq(std.mem.indexOf(u8, tbl_html, "<code>a|b</code> &amp; c") != null, true);
    expect_eq(std.mem.indexOf(u8, tbl_html, "<del>10</del> 99") != null, true);

    // 1.5 Inline styles: **, __, *, _, ~~, multi-backtick code, links, ref links, images, autolinks, escapes, XSS
    const inline_md = "**bold** __b2__ *em* _e2_ foo_bar_baz ``use `tick` here`` [CP](https://cp-lang.org \"Home\") [Ref][cpref] ![logo](logo.png \"CP\") <https://example.com> https://gfm.dev/test. \\*esc\\* [xss](javascript:alert(1)) <script>bad()</script>\n\n[cpref]: https://ref.cp.dev \"RefTitle\"\n";
    const inline_html = try Markdown.toHtml(allocator, inline_md, opts_ids);
    defer allocator.free(inline_html);
    expect_eq(std.mem.indexOf(u8, inline_html, "<strong>bold</strong> <strong>b2</strong> <em>em</em> <em>e2</em> foo_bar_baz") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "<code>use `tick` here</code>") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "<a href=\"https://cp-lang.org\" title=\"Home\">CP</a>") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "<a href=\"https://ref.cp.dev\" title=\"RefTitle\">Ref</a>") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "<img src=\"logo.png\" alt=\"logo\" title=\"CP\" />") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "<a href=\"https://example.com\">https://example.com</a>") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "<a href=\"https://gfm.dev/test\">https://gfm.dev/test</a>.") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "*esc*") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "<a href=\"#\">xss</a>") != null, true);
    expect_eq(std.mem.indexOf(u8, inline_html, "&lt;script&gt;bad()&lt;/script&gt;") != null, true);

    // 1.6 CommonMark preset disables GFM tables/strikethrough/autolinks
    const cm_opts = MarkdownOptions.commonmark();
    const cm_html = try Markdown.toHtml(allocator, "~~keep~~ https://raw.test\n", cm_opts);
    defer allocator.free(cm_html);
    expect_eq(std.mem.indexOf(u8, cm_html, "<del>") == null, true);
    expect_eq(std.mem.indexOf(u8, cm_html, "<a href") == null, true);

    // 1.7 AST traversal & custom MdRenderer
    let doc = try Markdown.parse(allocator, "# Title\n> Quote\n```cp\nx\n```\n| A | B |\n| - | - |\n| 1 | 2 |\n", opts_ids);
    defer doc.deinit();
    let counts = AstCountCtx{};
    let ctx_ptr: *!? = null;
    unsafe { ctx_ptr = *counts as *!; }
    doc.walk(MdRenderer{
        enterBlock: onAstEnter,
        ctx: ctx_ptr,
    });
    expect_eq(counts.headings, 1 as usize);
    expect_eq(counts.quotes, 1 as usize);
    expect_eq(counts.code_blocks, 1 as usize);
    expect_eq(counts.tables, 1 as usize);
}

// ============================================================================
// 2. cap:html — HTML5 Tokenizer, 40+ Entities + Numeric, Compound Selectors & HTMLRewriter
// ============================================================================
fn testHtmlDeep(): !void {
    const allocator = std.heap.page_allocator;

    // 2.1 Named & numeric HTML5 entities (including 4-byte UTF-8 Emoji &#x1F600;)
    let unesc_buf: [256]u8 = [0; 256];
    const ent_in = "&copy; 2026 &mdash; &laquo;CP&raquo; &#65; &#x42; &#x1F600; &euro;&nbsp;&rarr;&hearts;";
    const ent_out = HtmlEscape.unescapeToBuf(ent_in, &unesc_buf);
    const expected_ent = "\xC2\xA9 2026 \xE2\x80\x94 \xC2\xABCP\xC2\xBB A B \xF0\x9F\x98\x80 \xE2\x82\xAC\xC2\xA0\xE2\x86\x92\xE2\x99\xA5";
    expect_eq(std.mem.eql(u8, ent_out, expected_ent), true);

    // 2.2 HTML5 Tokenizer: Doctype, Comment, CDATA, RawText <script>/<style>, quotes/boolean attrs
    const raw_html = "<!DOCTYPE html><!-- <b>ignored</b> --><div id=\"app\" class=\"box active\" disabled data-val='x>y'><script>if (a < b && c > d) {}</script><style>.a > .b { color: red; }</style><br/><![CDATA[<raw>]]></div>";
    const tokens = try HtmlTokenizer.tokenizeAll(allocator, raw_html);
    defer {
        for (t of tokens) {
            if (t.attrs.len > 0) allocator.free(t.attrs);
        }
        allocator.free(tokens);
    }

    expect_eq(tokens.len >= 9, true);
    expect_eq(tokens[0].kind == .Doctype, true);
    expect_eq(std.mem.eql(u8, tokens[0].text, "html"), true);
    expect_eq(tokens[1].kind == .Comment, true);
    expect_eq(std.mem.eql(u8, tokens[1].text, " <b>ignored</b> "), true);
    expect_eq(tokens[2].kind == .StartTag, true);
    expect_eq(std.mem.eql(u8, tokens[2].tag_name, "div"), true);
    expect_eq(tokens[2].hasAttribute("disabled"), true);
    expect_eq(std.mem.eql(u8, tokens[2].getAttribute("data-val").?, "x>y"), true);
    // Script body MUST remain a single Text token without splitting `a < b`!
    expect_eq(tokens[3].kind == .StartTag, true);
    expect_eq(tokens[4].kind == .Text, true);
    expect_eq(std.mem.eql(u8, tokens[4].text, "if (a < b && c > d) {}"), true);
    expect_eq(tokens[5].kind == .EndTag, true);
    expect_eq(tokens[6].kind == .StartTag, true);
    expect_eq(tokens[7].kind == .Text, true);
    expect_eq(std.mem.eql(u8, tokens[7].text, ".a > .b { color: red; }"), true);

    // 2.3 Compound CSS Selector matching
    const sel_compound = CssSelector.parse("div.card.primary#hero[data-role=\"main\"][open]");
    expect_eq(std.mem.eql(u8, sel_compound.tag_name, "div"), true);
    expect_eq(std.mem.eql(u8, sel_compound.id_name, "hero"), true);
    expect_eq(sel_compound.class_count, 2 as usize);
    expect_eq(sel_compound.attr_count, 2 as usize);
    expect_eq(sel_compound.matchesToken(tokens[2]), false);

    const card_html = "<div id=\"hero\" class=\"extra primary card\" data-role=\"main\" open>ok</div>";
    const card_toks = try HtmlTokenizer.tokenizeAll(allocator, card_html);
    defer {
        for (t of card_toks) {
            if (t.attrs.len > 0) allocator.free(t.attrs);
        }
        allocator.free(card_toks);
    }
    expect_eq(sel_compound.matchesToken(card_toks[0]), true);

    // Prefix / suffix / contains attribute selectors
    const link_html = "<a href=\"https://cp-lang.org/spec.pdf\">Spec</a>";
    const link_toks = try HtmlTokenizer.tokenizeAll(allocator, link_html);
    defer {
        for (t of link_toks) {
            if (t.attrs.len > 0) allocator.free(t.attrs);
        }
        allocator.free(link_toks);
    }
    expect_eq(CssSelector.parse("a[href^=\"https://\"][href$=\".pdf\"][href*=\"cp-lang\"]").matchesToken(link_toks[0]), true);
    expect_eq(CssSelector.parse("a[href^=\"http://\"]").matchesToken(link_toks[0]), false);

    // 2.4 Full HTMLRewriter element manipulation
    let rw = HtmlRewriter.init(allocator);
    defer rw.deinit();
    try rw.on("div.card#hero", rewriteCardHandler);
    try rw.on("span.badge", rewriteBadgeHandler);
    try rw.on("p.remove-me", rewriteDeleteHandler);
    try rw.on("em.swap", rewriteReplaceHandler);

    const input_doc = "<section><div id=\"hero\" class=\"card\" data-legacy=\"1\"><span class=\"badge\">old</span><p class=\"remove-me\"><span>nested</span>gone</p><em class=\"swap\">bye</em></div></section>";
    const rewritten = try rw.transform(input_doc);
    defer allocator.free(rewritten);

    expect_eq(std.mem.indexOf(u8, rewritten, "<!-- card-start --><article id=\"hero\" class=\"card\" data-enhanced=\"true\"><header>Card</header>") != null, true);
    expect_eq(std.mem.indexOf(u8, rewritten, "<span class=\"badge\">NEW &amp; IMPROVED</span>") != null, true);
    expect_eq(std.mem.indexOf(u8, rewritten, "gone") == null, true);
    expect_eq(std.mem.indexOf(u8, rewritten, "<span>REPLACED</span>") != null, true);
    expect_eq(std.mem.indexOf(u8, rewritten, "<footer>End</footer></article><!-- card-end -->") != null, true);
}

// ============================================================================
// 3. cap:json — RFC 8259 Escapes/Surrogates/Strict Validation, Serializer, JSON Pointer & equals
// ============================================================================
fn testJsonDeep(): !void {
    const allocator = std.heap.page_allocator;

    // 3.1 Full escape decoding + UTF-16 surrogate pair \uD83D\uDE00 -> 4-byte UTF-8 Emoji (😀)
    const esc_json = "{\"msg\":\"line1\\n\\t\\\"quoted\\\"\\\\slash\\/\\b\\f\\r\\u0041\",\"emoji\":\"\\uD83D\\uDE00\",\"sci\":-1.23e-4,\"big\":1E+10}";
    const parsed = try JSON.parse(allocator, esc_json);
    const msg_s = parsed.get("msg").?.asString().?;
    expect_eq(std.mem.eql(u8, msg_s, "line1\n\t\"quoted\"\\slash/\x08\x0C\rA"), true);

    const emoji_s = parsed.get("emoji").?.asString().?;
    expect_eq(std.mem.eql(u8, emoji_s, "\xF0\x9F\x98\x80"), true);

    const sci_v = parsed.get("sci").?.asNumber().?;
    expect_eq(sci_v < -0.000122 && sci_v > -0.000124, true);
    const big_v = parsed.get("big").?.asNumber().?;
    expect_eq(big_v > 9999999999.0 && big_v < 10000000001.0, true);

    // 3.2 Strict RFC 8259 rejections (trailing comma, leading zero, '+', unclosed string, control char, max depth, trailing tokens)
    let bad_trailing_comma = false;
    * = JSON.parse(allocator, "[1, 2,]") catch { bad_trailing_comma = true; };
    expect_eq(bad_trailing_comma, true);

    let bad_obj_comma = false;
    * = JSON.parse(allocator, "{\"a\":1,}") catch { bad_obj_comma = true; };
    expect_eq(bad_obj_comma, true);

    let bad_leading_zero = false;
    * = JSON.parse(allocator, "0123") catch { bad_leading_zero = true; };
    expect_eq(bad_leading_zero, true);

    let bad_plus = false;
    * = JSON.parse(allocator, "+42") catch { bad_plus = true; };
    expect_eq(bad_plus, true);

    let bad_ctrl = false;
    * = JSON.parse(allocator, "\"a\nb\"") catch { bad_ctrl = true; };
    expect_eq(bad_ctrl, true);

    let bad_eof = false;
    * = JSON.parse(allocator, "\"unclosed") catch { bad_eof = true; };
    expect_eq(bad_eof, true);

    let bad_trailing = false;
    * = JSON.parse(allocator, "{} []") catch { bad_trailing = true; };
    expect_eq(bad_trailing, true);

    let bad_depth = false;
    * = JSON.parseWithOptions(allocator, "[[[1]]]", JsonParseOptions{ max_depth: 2 }) catch { bad_depth = true; };
    expect_eq(bad_depth, true);

    let bad_lone_surr = false;
    * = JSON.parse(allocator, "\"\\uD800\"") catch { bad_lone_surr = true; };
    expect_eq(bad_lone_surr, true);

    let bad_esc = false;
    * = JSON.parse(allocator, "\"\\x41\"") catch { bad_esc = true; };
    expect_eq(bad_esc, true);

    // 3.3 RFC 6901 JSON Pointer & equals
    const ptr_src = "{\"users\":[{\"name\":\"Alice\"},{\"name\":\"Bob\"}],\"a/b\":{\"c~d\":42}}";
    const ptr_doc = try JSON.parse(allocator, ptr_src);
    expect_eq(ptr_doc.pointer("").?.isObject(), true);
    expect_eq(std.mem.eql(u8, ptr_doc.pointer("/users/0/name").?.asString().?, "Alice"), true);
    expect_eq(std.mem.eql(u8, ptr_doc.pointer("/users/1/name").?.asString().?, "Bob"), true);
    expect_eq(ptr_doc.pointer("/a~1b/c~0d").?.asNumber().?, 42.0);
    expect_eq(ptr_doc.pointer("/users/01/name") == null, true); // leading zero forbidden in RFC 6901
    expect_eq(ptr_doc.pointer("/missing") == null, true);

    // Deep equality (object key-order independent)
    const eq_a = try JSON.parse(allocator, "{\"x\":1,\"y\":[true,null,\"hi\\n\"]}");
    const eq_b = try JSON.parse(allocator, "{\"y\":[true,null,\"hi\\n\"],\"x\":1}");
    const eq_c = try JSON.parse(allocator, "{\"y\":[false,null,\"hi\\n\"],\"x\":1}");
    expect_eq(JSON.equals(eq_a, eq_b), true);
    expect_eq(JSON.equals(eq_a, eq_c), false);

    // 3.4 Compact & Pretty Serializer roundtrip
    const compact = try JSON.stringifyValue(allocator, eq_a);
    defer allocator.free(compact);
    const re_parsed = try JSON.parse(allocator, compact);
    expect_eq(JSON.equals(eq_a, re_parsed), true);

    const pretty = try JSON.stringifyPretty(allocator, eq_a, 2);
    defer allocator.free(pretty);
    expect_eq(std.mem.indexOf(u8, pretty, "\n  \"x\": 1") != null, true);
    const re_parsed_pretty = try JSON.parse(allocator, pretty);
    expect_eq(JSON.equals(eq_a, re_parsed_pretty), true);
}

// ============================================================================
// 4. cap:ws — RFC 6455 RSV, 64-bit Length, Control Validation, Handshake & Assembler
// ============================================================================
fn testWebSocketDeep(): !void {
    const allocator = std.heap.page_allocator;

    // 4.1 RFC 6455 §1.3 exact SHA-1 + Base64 Sec-WebSocket-Accept test vector
    let acc_buf: [32]u8 = undefined;
    const accept = try WebSocketHandshake.computeAcceptKey("dGhlIHNhbXBsZSBub25jZQ==", &acc_buf);
    expect_eq(std.mem.eql(u8, accept, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="), true);

    // Client key + Upgrade request + Server 101 response roundtrip validation
    let key_buf: [24]u8 = undefined;
    const client_key = try WebSocketHandshake.generateClientKey(&key_buf);
    expect_eq(client_key.len, 24 as usize);

    let req_buf: [512]u8 = undefined;
    const req_http = try WebSocketHandshake.buildClientUpgradeRequest(&req_buf, "ws.cp-lang.org", "/chat", client_key, "json");
    const parsed_req = try WebSocketHandshake.validateClientUpgradeRequest(req_http);
    expect_eq(std.mem.eql(u8, parsed_req.key, client_key), true);
    expect_eq(std.mem.eql(u8, parsed_req.protocol, "json"), true);

    let resp_buf: [512]u8 = undefined;
    const resp_http = try WebSocketHandshake.buildServerUpgradeResponse(&resp_buf, parsed_req.key, "json");
    const parsed_resp = try WebSocketHandshake.validateServerUpgradeResponse(resp_http, client_key);
    expect_eq(std.mem.eql(u8, parsed_resp.protocol, "json"), true);

    // 4.2 RSV1-3 + 64-bit extended length + control frame validation errors
    const rsv_hdr = FrameHeader{
        fin: true,
        rsv1: true,
        rsv2: false,
        rsv3: true,
        opcode: .Binary,
        masked: false,
        payload_len: 70000,
    };
    let hbuf: [16]u8 = undefined;
    const hlen = try rsv_hdr.encode(&hbuf);
    expect_eq(hlen, 10 as usize); // 2 + 8 (64-bit)
    const dec_rsv = try FrameHeader.decode(hbuf[0..hlen]);
    expect_eq(dec_rsv.rsv1, true);
    expect_eq(dec_rsv.rsv2, false);
    expect_eq(dec_rsv.rsv3, true);
    expect_eq(dec_rsv.payload_len, 70000 as usize);

    // Reject fragmented control frame and >125B control frame and reserved opcode
    let bad_frag_ping = false;
    let raw_frag_ping: [2]u8 = [0x09, 0x00]; // fin=0, Ping
    * = FrameHeader.decode(&raw_frag_ping) catch { bad_frag_ping = true; };
    expect_eq(bad_frag_ping, true);

    let bad_big_ping = false;
    let raw_big_ping: [4]u8 = [0x89, 126, 0x00, 0x7E]; // Ping len=126
    * = FrameHeader.decode(&raw_big_ping) catch { bad_big_ping = true; };
    expect_eq(bad_big_ping, true);

    let bad_opcode = false;
    let raw_res_op: [2]u8 = [0x83, 0x00]; // opcode 0x3 reserved
    * = FrameHeader.decode(&raw_res_op) catch { bad_opcode = true; };
    expect_eq(bad_opcode, true);

    // 4.3 Fragmented message reassembly + interleaved Ping (Auto-Pong) + Close code/reason
    let asm_box = WebSocketMessageAssembler.init(allocator, 65536);
    defer asm_box.deinit();

    let f1: [128]u8 = undefined;
    let f2: [128]u8 = undefined;
    let f3: [128]u8 = undefined;
    let f_ping: [128]u8 = undefined;
    let mask_k: [4]u8 = [0x11, 0x22, 0x33, 0x44];

    const l1 = try FrameBuilder.buildFragment(&f1, false, .Text, "Hello ", mask_k);
    const l_ping = try FrameBuilder.buildPing(&f_ping, "ping-1", mask_k);
    const l2 = try FrameBuilder.buildContinue(&f2, false, "Web", mask_k);
    const l3 = try FrameBuilder.buildContinue(&f3, true, "Socket \xF0\x9F\x9A\x80!", mask_k);

    const r1 = try asm_box.feedBytes(f1[0..l1]);
    expect_eq(r1.bytes_consumed, l1);
    expect_eq(r1.message == null, true);

    // Interleaved Ping mid-fragmentation!
    const r_ping = try asm_box.feedBytes(f_ping[0..l_ping]);
    expect_eq(r_ping.message.?.kind == .Ping, true);
    expect_eq(std.mem.eql(u8, r_ping.message.?.data, "ping-1"), true);
    expect_eq(asm_box.hasPendingPong(), true);

    let pong_wire: [64]u8 = undefined;
    const pong_len = try asm_box.takePongFrame(&pong_wire, null);
    const pong_hdr = try FrameHeader.decode(pong_wire[0..pong_len]);
    expect_eq(pong_hdr.opcode == .Pong, true);
    expect_eq(std.mem.eql(u8, pong_wire[pong_hdr.header_len..pong_len], "ping-1"), true);

    const r2 = try asm_box.feedBytes(f2[0..l2]);
    expect_eq(r2.message == null, true);

    const r3 = try asm_box.feedBytes(f3[0..l3]);
    expect_eq(r3.message != null, true);
    expect_eq(r3.message.?.kind == .Text, true);
    expect_eq(std.mem.eql(u8, r3.message.?.data, "Hello WebSocket \xF0\x9F\x9A\x80!"), true);

    // Close frame 2-byte status code (1000..4999) + reason
    let f_close: [128]u8 = undefined;
    const lc = try FrameBuilder.buildCloseCode(&f_close, 4001, "custom-shutdown", mask_k);
    const rc = try asm_box.feedBytes(f_close[0..lc]);
    expect_eq(rc.message.?.kind == .Close, true);
    expect_eq(rc.message.?.close_code, 4001 as u16);
    expect_eq(std.mem.eql(u8, rc.message.?.close_reason, "custom-shutdown"), true);

    // Reject invalid UTF-8 in Text frame
    let bad_utf8_buf: [64]u8 = undefined;
    const bad_u8_len = try FrameBuilder.buildText(&bad_utf8_buf, "\xFF\xFE", null);
    let caught_utf8 = false;
    * = asm_box.feedBytes(bad_utf8_buf[0..bad_u8_len]) catch { caught_utf8 = true; };
    expect_eq(caught_utf8, true);

    // Reject invalid close code (1005 reserved wire code) and 1-byte close payload
    let bad_close_code = false;
    let close_1005: [2]u8 = [0x03, 0xED];
    * = ClosePayload.parse(&close_1005) catch { bad_close_code = true; };
    expect_eq(bad_close_code, true);

    let bad_close_1b = false;
    let close_1b: [1]u8 = [0x03];
    * = ClosePayload.parse(&close_1b) catch { bad_close_1b = true; };
    expect_eq(bad_close_1b, true);

    // SIMD Mask.apply roundtrip on >= 32 bytes payload
    let big_mask_data: [40]u8 = undefined;
    for (idx of 0..40) {
        big_mask_data[idx] = (idx as u8) +% 65;
    }
    let orig_mask_data: [40]u8 = undefined;
    @memcpy(&orig_mask_data, &big_mask_data);
    Mask.apply(&big_mask_data, mask_k);
    expect_eq(std.mem.eql(u8, &big_mask_data, &orig_mask_data), false);
    Mask.apply(&big_mask_data, mask_k);
    expect_eq(std.mem.eql(u8, &big_mask_data, &orig_mask_data), true);
}

fn main() {
    test("1. cap:md Deep — CommonMark + GFM Blocks, Inlines, AST Walk & XSS Sanitization", () => {
        try testMarkdownDeep();
    });
    test("2. cap:html Deep — HTML5 Tokenizer, 70+ Entities, Compound Selectors & HTMLRewriter", () => {
        try testHtmlDeep();
    });
    test("3. cap:json Deep — RFC 8259 Escapes, UTF-16 Surrogates, Serializer & JSON Pointer", () => {
        try testJsonDeep();
    });
    test("4. cap:ws Deep — RFC 6455 RSV/64-bit/Validation, Handshake SHA-1 & Fragment Assembler", () => {
        try testWebSocketDeep();
    });
}
