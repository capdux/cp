// Origin: Pure-CP Stdlib Test Suite (lib/css/nestingRule.cp) | Created: 2026-11-22
// Production Deep Verification Suite for CSS Nesting (@nest & implicit lowering):
//   1. hasNestingSelectorAmp & isValidAtNestSelector (quoted '&' safety, comma lists)
//   2. resolveNestedSelector: explicit '&' substitution, implicit descendant combinator & quoted '&'
//   3. resolveNestedSelector: Cartesian product expansion for comma-separated selector lists
//   4. CssNestingRule.toCss context-aware '@nest ' emission, minify vs pretty & !important
//   5. CssNestingRule.toLoweredCss end-to-end browser-compatible flattening
//   6. Error guards: EmptySelector, InvalidAtNestSelector, TooManyDecls (32) & BufferTooSmall

import { test, expect_eq, expect_true } from "cap:test";
import {
    CSS_NESTING_MAX_DECLS,
    CssNestingError,
    CssNestingLoc,
    hasNestingSelectorAmp,
    isValidAtNestSelector,
    resolveNestedSelector,
    CssNestingRule,
} from "cap:css";

test("CssNesting: hasNestingSelectorAmp & isValidAtNestSelector", fn() {
    expect_true(hasNestingSelectorAmp("&:hover"));
    expect_true(hasNestingSelectorAmp(".dark &"));
    expect_true(!hasNestingSelectorAmp(".title"));
    // Quoted '&' inside attribute selector is NOT a nesting selector
    expect_true(!hasNestingSelectorAmp("a[title=\"a&b\"]"));
    expect_true(hasNestingSelectorAmp("&[title=\"a&b\"]"));

    expect_true(isValidAtNestSelector("&:hover, &.active"));
    expect_true(isValidAtNestSelector(".dark &"));
    expect_true(!isValidAtNestSelector("&:hover, .no-amp"));
    expect_true(!isValidAtNestSelector(""));
    expect_true(!isValidAtNestSelector("   "));
});

test("CssNesting: resolveNestedSelector explicit '&', implicit descendant & quoted '&'", fn() {
    let buf: [128]u8 = [0; 128];

    const s1 = resolveNestedSelector(".card", ".title", false, buf[0..]) catch "";
    expect_eq(s1, ".card .title");

    const s2 = resolveNestedSelector(".btn", "&:hover", false, buf[0..]) catch "";
    expect_eq(s2, ".btn:hover");

    const s3 = resolveNestedSelector(".card", ".dark &", false, buf[0..]) catch "";
    expect_eq(s3, ".dark .card");

    const s4 = resolveNestedSelector(".item", "& + &", false, buf[0..]) catch "";
    expect_eq(s4, ".item + .item");

    // Quoted '&' must stay literal while unquoted '&' is substituted
    const s5 = resolveNestedSelector("a", "&[data-op=\"a&b\"]", false, buf[0..]) catch "";
    expect_eq(s5, "a[data-op=\"a&b\"]");
});

test("CssNesting: resolveNestedSelector Cartesian product for selector lists", fn() {
    let buf: [256]u8 = [0; 256];

    const pretty = resolveNestedSelector(".a, .b", "&:hover, &.on", false, buf[0..]) catch "";
    expect_eq(pretty, ".a:hover, .a.on, .b:hover, .b.on");

    const min = resolveNestedSelector(".a, .b", "&:hover, &.on", true, buf[0..]) catch "";
    expect_eq(min, ".a:hover,.a.on,.b:hover,.b.on");
});

test("CssNestingRule: context-aware toCss ('@nest ' omission in parent context)", fn() {
    let rule = CssNestingRule.initAtNest("&:focus", CssNestingLoc{ line: 10, column: 4 }) catch unreachable;
    * = rule.addDecl("outline", "none", false) catch unreachable;
    * = rule.addDecl("color", "red", true) catch unreachable;
    expect_eq(rule.declCount(), 2);

    let buf: [256]u8 = [0; 256];

    // No parent context -> emits "@nest " prefix (mirrors Bun NestingRule.toCss)
    const standalone = rule.toCss(false, false, buf[0..]) catch "";
    expect_eq(standalone, "@nest &:focus { outline: none; color: red !important }");

    // Inside parent context -> omits "@nest "
    const in_ctx = rule.toCss(true, true, buf[0..]) catch "";
    expect_eq(in_ctx, "&:focus{outline:none;color:red!important}");

    // Implicit nested rule never emits "@nest "
    let imp = CssNestingRule.initImplicit(".child", CssNestingLoc{ line: 2, column: 2 }) catch unreachable;
    * = imp.addDecl("display", "block", false) catch unreachable;
    const imp_css = imp.toCss(false, true, buf[0..]) catch "";
    expect_eq(imp_css, ".child{display:block}");
});

test("CssNestingRule: toLoweredCss end-to-end lowering", fn() {
    let rule = CssNestingRule.initAtNest("&:hover, &.active", CssNestingLoc{ line: 5, column: 2 }) catch unreachable;
    * = rule.addDecl("background", "#000", false) catch unreachable;

    let buf: [256]u8 = [0; 256];
    const flat = rule.toLoweredCss(".btn, .link", true, buf[0..]) catch "";
    expect_eq(flat, ".btn:hover,.btn.active,.link:hover,.link.active{background:#000}");
});

test("CssNestingRule: error guards (EmptySelector, InvalidAtNestSelector, TooManyDecls, BufferTooSmall)", fn() {
    let empty_err = false;
    CssNestingRule.initImplicit("   ", CssNestingLoc{}) catch |e| {
        if (e == CssNestingError.EmptySelector) empty_err = true;
    };
    expect_true(empty_err);

    let inv_nest = false;
    CssNestingRule.initAtNest(".no-amp", CssNestingLoc{}) catch |e| {
        if (e == CssNestingError.InvalidAtNestSelector) inv_nest = true;
    };
    expect_true(inv_nest);

    let rule = CssNestingRule.initImplicit(".x", CssNestingLoc{}) catch unreachable;
    let i: usize = 0;
    while (i < CSS_NESTING_MAX_DECLS) {
        * = rule.addDecl("a", "1", false) catch unreachable;
        i += 1;
    }
    let too_many = false;
    rule.addDecl("b", "2", false) catch |e| {
        if (e == CssNestingError.TooManyDecls) too_many = true;
    };
    expect_true(too_many);

    let tiny: [4]u8 = [0; 4];
    let buf_small = false;
    rule.toLoweredCss(".parent", true, tiny[0..]) catch |e| {
        if (e == CssNestingError.BufferTooSmall) buf_small = true;
    };
    expect_true(buf_small);
});
