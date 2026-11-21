// Source: cp/cap/test/test_xpath_production_deep.cp (Production deep tests for cap:xpath)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    XPathTokenKind,
    XPathToken,
    XPathIter,
    XPathStep,
    XPathStepIter,
    validateXPathDelimiters,
} from "cap:xpath";

test("1. XPathIter lexes multi-char tokens, QNames, vars, strings, numbers, and operators", () => {
    const expr = "//svg:path[@id != 'a/b' and $limit <= 10.5] | ../child::* | .5";
    let it = XPathIter.init(expr);

    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.dslash, true);

    const t_qname = it.next() ?? unreachable;
    expect_eq(t_qname.kind == XPathTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t_qname.lexeme, "svg:path"), true);

    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.lbracket, true);
    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.at, true);

    const t_id = it.next() ?? unreachable;
    expect_eq(t_id.kind == XPathTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t_id.lexeme, "id"), true);

    const t_neq = it.next() ?? unreachable;
    expect_eq(t_neq.kind == XPathTokenKind.op, true);
    expect_eq(std.mem.eql(u8, t_neq.lexeme, "!="), true);

    const t_str = it.next() ?? unreachable;
    expect_eq(t_str.kind == XPathTokenKind.string, true);
    expect_eq(std.mem.eql(u8, t_str.lexeme, "'a/b'"), true);

    const t_and = it.next() ?? unreachable;
    expect_eq(t_and.kind == XPathTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t_and.lexeme, "and"), true);

    const t_var = it.next() ?? unreachable;
    expect_eq(t_var.kind == XPathTokenKind.var_ref, true);
    expect_eq(std.mem.eql(u8, t_var.lexeme, "$limit"), true);

    const t_le = it.next() ?? unreachable;
    expect_eq(t_le.kind == XPathTokenKind.op, true);
    expect_eq(std.mem.eql(u8, t_le.lexeme, "<="), true);

    const t_num = it.next() ?? unreachable;
    expect_eq(t_num.kind == XPathTokenKind.number, true);
    expect_eq(std.mem.eql(u8, t_num.lexeme, "10.5"), true);

    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.rbracket, true);
    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.pipe, true);
    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.ddot, true);
    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.slash, true);

    const t_axis_name = it.next() ?? unreachable;
    expect_eq(t_axis_name.kind == XPathTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t_axis_name.lexeme, "child"), true);

    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.axis, true);
    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.star, true);
    expect_eq((it.next() ?? unreachable).kind == XPathTokenKind.pipe, true);

    const t_frac = it.next() ?? unreachable;
    expect_eq(t_frac.kind == XPathTokenKind.number, true);
    expect_eq(std.mem.eql(u8, t_frac.lexeme, ".5"), true);

    expect_eq(it.next() == null, true);
});

test("2. XPathStepIter parses descendant, explicit axis, @attr, node-type tests, and skips '/' inside predicates", () => {
    const expr = "//div[@class=\"card\" and contains(a/b, 'x/y')]/child::ns:item[position() >= 2]/text()";
    let it = XPathStepIter.init(expr);

    const s0 = it.next() ?? unreachable;
    expect_eq(s0.is_descendant, true);
    expect_eq(s0.axis == null, true);
    expect_eq(std.mem.eql(u8, s0.node_test, "div"), true);
    const p0 = s0.predicate ?? unreachable;
    expect_eq(std.mem.eql(u8, p0, "@class=\"card\" and contains(a/b, 'x/y')"), true);

    const s1 = it.next() ?? unreachable;
    expect_eq(s1.is_descendant, false);
    const ax1 = s1.axis ?? unreachable;
    expect_eq(std.mem.eql(u8, ax1, "child"), true);
    expect_eq(std.mem.eql(u8, s1.node_test, "ns:item"), true);
    const p1 = s1.predicate ?? unreachable;
    expect_eq(std.mem.eql(u8, p1, "position() >= 2"), true);

    const s2 = it.next() ?? unreachable;
    expect_eq(s2.is_descendant, false);
    expect_eq(s2.axis == null, true);
    expect_eq(std.mem.eql(u8, s2.node_test, "text()"), true);
    expect_eq(s2.predicate == null, true);

    expect_eq(it.next() == null, true);
});

test("3. XPathStepIter handles @attribute shorthand, wildcards, parent '..', self '.', and nested predicates", () => {
    const expr = "../descendant-or-self::ns:*[child/sub[price > 10]]//@href";
    let it = XPathStepIter.init(expr);

    const s0 = it.next() ?? unreachable;
    expect_eq(s0.is_descendant, false);
    expect_eq(s0.axis == null, true);
    expect_eq(std.mem.eql(u8, s0.node_test, ".."), true);
    expect_eq(s0.predicate == null, true);

    const s1 = it.next() ?? unreachable;
    expect_eq(s1.is_descendant, false);
    const ax1 = s1.axis ?? unreachable;
    expect_eq(std.mem.eql(u8, ax1, "descendant-or-self"), true);
    expect_eq(std.mem.eql(u8, s1.node_test, "ns:*"), true);
    const p1 = s1.predicate ?? unreachable;
    expect_eq(std.mem.eql(u8, p1, "child/sub[price > 10]"), true);

    const s2 = it.next() ?? unreachable;
    expect_eq(s2.is_descendant, true);
    const ax2 = s2.axis ?? unreachable;
    expect_eq(std.mem.eql(u8, ax2, "attribute"), true);
    expect_eq(std.mem.eql(u8, s2.node_test, "href"), true);
    expect_eq(s2.predicate == null, true);

    expect_eq(it.next() == null, true);
});

test("4. XPathIter flags malformed tokens (unterminated strings, stray ':' or '$', bare '!')", () => {
    let it1 = XPathIter.init("//item[@name = \"unclosed]");
    let saw_invalid1 = false;
    while (tok := it1.next()) {
        if (tok.kind == XPathTokenKind.invalid) saw_invalid1 = true;
    }
    expect_eq(saw_invalid1, true);

    let it2 = XPathIter.init("$");
    const t2 = it2.next() ?? unreachable;
    expect_eq(t2.kind == XPathTokenKind.invalid, true);

    let it3 = XPathIter.init("!foo");
    const t3 = it3.next() ?? unreachable;
    expect_eq(t3.kind == XPathTokenKind.invalid, true);
});

test("5. validateXPathDelimiters checks string-aware []/() balance, quotes, and 64-depth limit", () => {
    expect_eq(validateXPathDelimiters("//book[contains(title, '([unbalanced') and price >= 10]/@id"), true);
    expect_eq(validateXPathDelimiters("//a[b(c[d()])]"), true);

    // Mismatched or unclosed delimiters & quotes
    expect_eq(validateXPathDelimiters("//a[b(c])]"), false);
    expect_eq(validateXPathDelimiters("//a[b(c)"), false);
    expect_eq(validateXPathDelimiters("//a[b(c))]"), false);
    expect_eq(validateXPathDelimiters("//a[@x = 'unclosed]"), false);

    // Exact 64-level nesting passes; 65-level nesting is rejected
    let ok64: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        ok64[i] = '[';
        ok64[127 - i] = ']';
        i += 1;
    }
    expect_eq(validateXPathDelimiters(ok64[0..128]), true);

    let bad65: [130]u8 = undefined;
    let j: usize = 0;
    while (j < 65) {
        bad65[j] = '(';
        bad65[129 - j] = ')';
        j += 1;
    }
    expect_eq(validateXPathDelimiters(bad65[0..130]), false);
});
