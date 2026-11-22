// Source: cp/cap/test/test_selector_production_deep.cp
// Deep Production Test Suite for cap:selector
// Covers:
//   1. Basic selectors (tag, #id, .class.multi, *) & all 6 attribute operators ([a], =, ^=, $=, *=, ~=)
//   2. All 4 combinators (' ', '>', '+', '~') & matches(node, root)
//   3. Structural & negation pseudo-classes (:first-child, :last-child, :only-child, :empty, :nth-child, :not) & comma union deduplication
//   4. Malformed CSS selector rejection (EmptySelector, UnclosedBracket, UnclosedParen, InvalidNthExpr, UnsupportedPseudo, UnexpectedToken)
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.selector`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { XmlParser } from "cap:xml";
import { CssSelector, SelectorError, AttrOp, Combinator } from "cap:web";
import cap from "cap";

fn testBasicAndAttributeSelectors()! {
    const alloc = std.heap.page_allocator;

    const xml =
        "<div id=\"main\" class=\"container\" data-env=\"PROD\">" ++
        "  <a id=\"link-pdf\" class=\"btn primary large\" href=\"https://cp-lang.org/guide.pdf\" rel=\"external noopener\" data-tags=\"cp,agent,fast\">Guide</a>" ++
        "  <a id=\"link-sec\" class=\"btn secondary\" href=\"http://example.org/home\" rel=\"noop\">Home</a>" ++
        "  <input id=\"name-in\" type=\"text\" name=\"username\"/>" ++
        "  <span class=\"badge\">v1</span>" ++
        "</div>";

    let doc = XmlParser.parse(alloc, xml) catch unreachable;
    defer doc.deinit();

    // Tag selector
    let s_a = CssSelector.compile(alloc, "a") catch unreachable;
    defer s_a.deinit();
    const a_hits = s_a.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(a_hits);
    expect_eq(a_hits.len, 2 as usize);

    // #id selector
    let s_id = CssSelector.compile(alloc, "#main") catch unreachable;
    defer s_id.deinit();
    const id_first = s_id.queryFirst(&doc.root).?;
    expect_eq(std.mem.eql(u8, id_first.tag, "div"), true);

    // Multi-class .btn.primary
    let s_cls = CssSelector.compile(alloc, ".btn.primary") catch unreachable;
    defer s_cls.deinit();
    const cls_hits = s_cls.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(cls_hits);
    expect_eq(cls_hits.len, 1 as usize);
    expect_eq(std.mem.eql(u8, cls_hits[0].getAttr("id").?, "link-pdf"), true);

    // Universal *
    let s_all = CssSelector.compile(alloc, "*") catch unreachable;
    defer s_all.deinit();
    const all_hits = s_all.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(all_hits);
    expect_eq(all_hits.len, 5 as usize);

    // [href] Exists
    let s_exists = CssSelector.compile(alloc, "a[href]") catch unreachable;
    defer s_exists.deinit();
    const exists_hits = s_exists.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(exists_hits);
    expect_eq(exists_hits.len, 2 as usize);

    // [type="text"] Equals
    let s_eq = CssSelector.compile(alloc, "input[type=\"text\"]") catch unreachable;
    defer s_eq.deinit();
    expect_eq(std.mem.eql(u8, s_eq.queryFirst(&doc.root).?.getAttr("id").?, "name-in"), true);

    // [href^="https://"] Prefix
    let s_pre = CssSelector.compile(alloc, "[href^=\"https://\"]") catch unreachable;
    defer s_pre.deinit();
    const pre_hits = s_pre.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(pre_hits);
    expect_eq(pre_hits.len, 1 as usize);
    expect_eq(std.mem.eql(u8, pre_hits[0].getAttr("id").?, "link-pdf"), true);

    // [href$=".pdf"] Suffix
    let s_suf = CssSelector.compile(alloc, "[href$='.pdf']") catch unreachable;
    defer s_suf.deinit();
    const suf_hits = s_suf.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(suf_hits);
    expect_eq(suf_hits.len, 1 as usize);

    // [data-tags*="agent"] Substring
    let s_sub = CssSelector.compile(alloc, "[data-tags*=\"agent\"]") catch unreachable;
    defer s_sub.deinit();
    const sub_hits = s_sub.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(sub_hits);
    expect_eq(sub_hits.len, 1 as usize);

    // [rel~="noopener"] Word (must NOT match rel="noop")
    let s_word = CssSelector.compile(alloc, "[rel~=\"noopener\"]") catch unreachable;
    defer s_word.deinit();
    const word_hits = s_word.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(word_hits);
    expect_eq(word_hits.len, 1 as usize);
    expect_eq(std.mem.eql(u8, word_hits[0].getAttr("id").?, "link-pdf"), true);

    // Case-insensitive attribute flag [data-env="prod" i]
    let s_ci = CssSelector.compile(alloc, "[data-env=\"prod\" i]") catch unreachable;
    defer s_ci.deinit();
    expect_eq(std.mem.eql(u8, s_ci.queryFirst(&doc.root).?.getAttr("id").?, "main"), true);

    // Verify exported enum variants are usable
    const op_check: AttrOp = .Word;
    const comb_check: Combinator = .Child;
    expect_eq(op_check == .Word, true);
    expect_eq(comb_check == .Child, true);
}

fn testAllFourCombinatorsAndMatches()! {
    const alloc = std.heap.page_allocator;

    const xml =
        "<section>" ++
        "  <p id=\"p0\">Before</p>" ++
        "  <h2>Heading</h2>" ++
        "  text between elements" ++
        "  <p id=\"p1\">Adjacent</p>" ++
        "  <div class=\"box\"><p id=\"p2\">Nested</p></div>" ++
        "  <p id=\"p3\">Later sibling</p>" ++
        "  <ul>" ++
        "    <li id=\"li1\">One</li>" ++
        "    <li id=\"li2\">Two<ol><li id=\"nested-li\">Sub</li></ol></li>" ++
        "  </ul>" ++
        "</section>";

    let doc = XmlParser.parse(alloc, xml) catch unreachable;
    defer doc.deinit();

    // Descendant: section p -> p0, p1, p2, p3
    let s_desc = CssSelector.compile(alloc, "section p") catch unreachable;
    defer s_desc.deinit();
    const desc_hits = s_desc.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(desc_hits);
    expect_eq(desc_hits.len, 4 as usize);

    // Direct child: ul > li -> li1, li2 (NOT nested-li)
    let s_child = CssSelector.compile(alloc, "ul > li") catch unreachable;
    defer s_child.deinit();
    const child_hits = s_child.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(child_hits);
    expect_eq(child_hits.len, 2 as usize);
    expect_eq(std.mem.eql(u8, child_hits[0].getAttr("id").?, "li1"), true);
    expect_eq(std.mem.eql(u8, child_hits[1].getAttr("id").?, "li2"), true);

    // Adjacent sibling: h2 + p -> p1 only
    let s_adj = CssSelector.compile(alloc, "h2 + p") catch unreachable;
    defer s_adj.deinit();
    const adj_hits = s_adj.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(adj_hits);
    expect_eq(adj_hits.len, 1 as usize);
    expect_eq(std.mem.eql(u8, adj_hits[0].getAttr("id").?, "p1"), true);

    // General sibling: h2 ~ p -> p1, p3 (NOT p0 which precedes h2, NOT p2 which is inside div)
    let s_gen = CssSelector.compile(alloc, "h2 ~ p") catch unreachable;
    defer s_gen.deinit();
    const gen_hits = s_gen.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(gen_hits);
    expect_eq(gen_hits.len, 2 as usize);
    expect_eq(std.mem.eql(u8, gen_hits[0].getAttr("id").?, "p1"), true);
    expect_eq(std.mem.eql(u8, gen_hits[1].getAttr("id").?, "p3"), true);

    // matches(node, root) right-to-left check
    expect_eq(s_gen.matches(gen_hits[1], &doc.root), true);
    expect_eq(s_adj.matches(gen_hits[1], &doc.root), false);
    expect_eq(s_child.matches(child_hits[0], &doc.root), true);
}

fn testPseudosAndCommaUnionDedup()! {
    const alloc = std.heap.page_allocator;

    const xml =
        "<root>" ++
        "  <ul id=\"list\">" ++
        "    <li class=\"item\">A</li>" ++
        "    <li class=\"item disabled\">B</li>" ++
        "    <li class=\"item\">C</li>" ++
        "    <li class=\"item\">D</li>" ++
        "  </ul>" ++
        "  <div id=\"solo\"><span id=\"only-s\"></span></div>" ++
        "</root>";

    let doc = XmlParser.parse(alloc, xml) catch unreachable;
    defer doc.deinit();

    // :first-child & :last-child
    let s_first = CssSelector.compile(alloc, "ul > li:first-child") catch unreachable;
    defer s_first.deinit();
    const first_txt = s_first.queryFirst(&doc.root).?.innerText(alloc) catch unreachable;
    defer alloc.free(first_txt);
    expect_eq(std.mem.eql(u8, first_txt, "A"), true);

    let s_last = CssSelector.compile(alloc, "ul > li:last-child") catch unreachable;
    defer s_last.deinit();
    const last_txt = s_last.queryFirst(&doc.root).?.innerText(alloc) catch unreachable;
    defer alloc.free(last_txt);
    expect_eq(std.mem.eql(u8, last_txt, "D"), true);

    // :only-child & :empty
    let s_only = CssSelector.compile(alloc, "span:only-child:empty") catch unreachable;
    defer s_only.deinit();
    expect_eq(std.mem.eql(u8, s_only.queryFirst(&doc.root).?.getAttr("id").?, "only-s"), true);

    // :nth-child(odd) & :nth-child(even) & :nth-child(2n+1)
    let s_odd = CssSelector.compile(alloc, "ul > li:nth-child(odd)") catch unreachable;
    defer s_odd.deinit();
    const odd_hits = s_odd.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(odd_hits);
    expect_eq(odd_hits.len, 2 as usize);

    let s_2np1 = CssSelector.compile(alloc, "ul > li:nth-child(2n+1)") catch unreachable;
    defer s_2np1.deinit();
    const np1_hits = s_2np1.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(np1_hits);
    expect_eq(np1_hits.len, 2 as usize);

    let s_even = CssSelector.compile(alloc, "ul > li:nth-child(even)") catch unreachable;
    defer s_even.deinit();
    const even_hits = s_even.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(even_hits);
    expect_eq(even_hits.len, 2 as usize);

    // :not(.disabled)
    let s_not = CssSelector.compile(alloc, "ul > li:not(.disabled)") catch unreachable;
    defer s_not.deinit();
    const not_hits = s_not.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(not_hits);
    expect_eq(not_hits.len, 3 as usize);

    // Comma union deduplication + document order:
    // Both "li.item" and "ul > li:nth-child(1)" match the 1st <li>, and "#only-s" comes later.
    let s_union = CssSelector.compile(alloc, "#only-s, li.item, ul > li:first-child") catch unreachable;
    defer s_union.deinit();
    const union_hits = s_union.queryAll(alloc, &doc.root) catch unreachable;
    defer alloc.free(union_hits);
    expect_eq(union_hits.len, 5 as usize);
    expect_eq(std.mem.eql(u8, union_hits[0].tag, "li"), true);
    expect_eq(std.mem.eql(u8, union_hits[4].getAttr("id").?, "only-s"), true);

    // selectXml one-shot helper
    const texts = CssSelector.selectXml(alloc, xml, "ul > li:not(.disabled)") catch unreachable;
    defer CssSelector.freeTexts(alloc, texts);
    expect_eq(texts.len, 3 as usize);
    expect_eq(std.mem.eql(u8, texts[0], "A"), true);
    expect_eq(std.mem.eql(u8, texts[1], "C"), true);
    expect_eq(std.mem.eql(u8, texts[2], "D"), true);
}

fn expectCompileErr(alloc: std.mem.Allocator, expr: []const u8, expected: SelectorError)! {
    let hit = false;
    let res = CssSelector.compile(alloc, expr) catch e => {
        hit = (e == expected);
        expect_eq(hit, true);
        return;
    };
    res.deinit();
    expect_eq(hit, true);
}

fn testMalformedSelectorGuards()! {
    const alloc = std.heap.page_allocator;

    // Empty selectors
    expectCompileErr(alloc, "", SelectorError.EmptySelector)?;
    expectCompileErr(alloc, "   ", SelectorError.EmptySelector)?;
    expectCompileErr(alloc, "div,", SelectorError.EmptySelector)?;
    expectCompileErr(alloc, ",div", SelectorError.EmptySelector)?;

    // Unclosed bracket: div[id=
    expectCompileErr(alloc, "div[id=", SelectorError.UnclosedBracket)?;
    expectCompileErr(alloc, "div[id='abc", SelectorError.UnclosedBracket)?;

    // Invalid :nth-child(abc)
    expectCompileErr(alloc, "li:nth-child(abc)", SelectorError.InvalidNthExpr)?;
    expectCompileErr(alloc, "li:nth-child()", SelectorError.InvalidNthExpr)?;
    expectCompileErr(alloc, "li:nth-child(2n+)", SelectorError.InvalidNthExpr)?;

    // Unclosed :not( and :nth-child(
    expectCompileErr(alloc, "div:not(.disabled", SelectorError.UnclosedParen)?;
    expectCompileErr(alloc, "li:nth-child(2", SelectorError.UnclosedParen)?;

    // Unsupported pseudo
    expectCompileErr(alloc, "div:hover", SelectorError.UnsupportedPseudo)?;

    // Trailing combinator / unexpected token
    expectCompileErr(alloc, "div >", SelectorError.UnexpectedToken)?;
    expectCompileErr(alloc, "div..foo", SelectorError.UnexpectedToken)?;
}

fn testCapSelectorLazyMount()! {
    const alloc = std.heap.page_allocator;

    const feed =
        "<feed>" ++
        "  <entry status=\"ok\"><title>Alpha</title></entry>" ++
        "  <entry status=\"draft\"><title>Beta</title></entry>" ++
        "</feed>";

    const first_title = cap.web.CssSelector.selectXmlFirst(alloc, feed, "entry[status=\"ok\"] > title") catch unreachable;
    defer alloc.free(first_title.?);
    expect_eq(std.mem.eql(u8, first_title.?, "Alpha"), true);

    const all_titles = cap.web.CssSelector.selectXml(alloc, feed, "feed > entry > title") catch unreachable;
    defer cap.web.CssSelector.freeTexts(alloc, all_titles);
    expect_eq(all_titles.len, 2 as usize);
    expect_eq(std.mem.eql(u8, all_titles[1], "Beta"), true);
}

fn main() {
    test("1. Basic Selectors (tag, #id, .class.multi, *) & All 6 Attribute Operators", () => {
        testBasicAndAttributeSelectors()?;
    });

    test("2. All 4 Combinators (' ', '>', '+', '~') & matches(node, root)", () => {
        testAllFourCombinatorsAndMatches()?;
    });

    test("3. Structural & Negation Pseudo-Classes + Comma Union Document-Order Deduplication", () => {
        testPseudosAndCommaUnionDedup()?;
    });

    test("4. Malformed CSS Selector Syntax Rejection Guards", () => {
        testMalformedSelectorGuards()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.selector`", () => {
        testCapSelectorLazyMount()?;
    });
}
