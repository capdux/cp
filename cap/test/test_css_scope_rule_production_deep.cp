// Production Deep Tests for cap:css @scope Rule & Extractor (lib/css/scope_rule.cp)
import { test, expect_eq } from "cap:test";
import {
    CSS_SCOPE_MAX_SELECTORS,
    CSS_SCOPE_MAX_RULES,
    CssScopeError,
    ScopeSelectorList,
    ScopeRule,
    ScopeRuleList,
} from "cap:css";

test("css scope_rule: implicit, root-only, and limit-only @scope parsing", fn() {
    expect_eq(CSS_SCOPE_MAX_SELECTORS, @as(usize, 16));
    expect_eq(CSS_SCOPE_MAX_RULES, @as(usize, 32));

    const r_implicit = ScopeRule.parse("@scope { p { color: red; } }") catch unreachable;
    expect_eq(r_implicit.isImplicit(), true);
    expect_eq(r_implicit.hasStart(), false);
    expect_eq(r_implicit.hasEnd(), false);
    expect_eq(r_implicit.isDonut(), false);
    expect_eq(r_implicit.startCount(), @as(usize, 0));
    expect_eq(r_implicit.endCount(), @as(usize, 0));
    expect_eq(r_implicit.body, "p { color: red; }");

    const r_root = ScopeRule.parse("@scope (.card) { .title { font-weight: 700; } }") catch unreachable;
    expect_eq(r_root.isImplicit(), false);
    expect_eq(r_root.hasStart(), true);
    expect_eq(r_root.hasEnd(), false);
    expect_eq(r_root.isDonut(), false);
    expect_eq(r_root.startCount(), @as(usize, 1));
    expect_eq(r_root.start.at(0) ?? "", ".card");
    expect_eq(r_root.body, ".title { font-weight: 700; }");

    const r_limit = ScopeRule.parse("@scope to (.nested-widget) { img { border: 0; } }") catch unreachable;
    expect_eq(r_limit.isImplicit(), true);
    expect_eq(r_limit.hasStart(), false);
    expect_eq(r_limit.hasEnd(), true);
    expect_eq(r_limit.isDonut(), false);
    expect_eq(r_limit.endCount(), @as(usize, 1));
    expect_eq(r_limit.end.at(0) ?? "", ".nested-widget");
    expect_eq(r_limit.body, "img { border: 0; }");
});

test("css scope_rule: donut scope, case-insensitivity, and equality", fn() {
    const r1 = ScopeRule.parse(
        "@SCOPE (.card, .panel) TO (.card-footer, .aside) { a { color: blue; } }"
    ) catch unreachable;
    expect_eq(r1.isDonut(), true);
    expect_eq(r1.isImplicit(), false);
    expect_eq(r1.startCount(), @as(usize, 2));
    expect_eq(r1.endCount(), @as(usize, 2));
    expect_eq(r1.start.at(0) ?? "", ".card");
    expect_eq(r1.start.at(1) ?? "", ".panel");
    expect_eq(r1.end.at(0) ?? "", ".card-footer");
    expect_eq(r1.end.at(1) ?? "", ".aside");
    expect_eq(r1.start.at(2) == null, true);

    const r2 = ScopeRule.parse(
        "  @scope ( .card , .panel ) to ( .card-footer , .aside ) {   a { color: blue; }   } "
    ) catch unreachable;
    expect_eq(r1.eql(r2), true);

    const r3 = ScopeRule.parse(
        "@scope (.card, .panel) to (.card-footer) { a { color: blue; } }"
    ) catch unreachable;
    expect_eq(r1.eql(r3), false);
});

test("css scope_rule: nested parens, attribute quotes, and comments inside selectors", fn() {
    const raw = "@scope (:is(.card, .modal), [data-role=\"a, b) to (c\"]) to (:not(.keep, .pin) /* comment */) { color: green; }";
    const rule = ScopeRule.parse(raw) catch unreachable;
    expect_eq(rule.isDonut(), true);
    expect_eq(rule.startCount(), @as(usize, 2));
    expect_eq(rule.start.at(0) ?? "", ":is(.card, .modal)");
    expect_eq(rule.start.at(1) ?? "", "[data-role=\"a, b) to (c\"]");
    expect_eq(rule.endCount(), @as(usize, 1));
    expect_eq(rule.end.at(0) ?? "", ":not(.keep, .pin) /* comment */");
    expect_eq(rule.body, "color: green;");
});

test("css scope_rule: pretty toCss, minified toCssMinified, and BufferTooSmall", fn() {
    const donut = ScopeRule.parse(
        "@scope (.card, .panel) to (.footer, .meta) { a { color: blue; } }"
    ) catch unreachable;

    let buf: [256]u8 = [0; 256];
    const n_pretty = donut.toCss(buf[0..]) catch unreachable;
    expect_eq(
        buf[0..n_pretty],
        "@scope (.card, .panel) to (.footer, .meta) {\n  a { color: blue; }\n}"
    );

    const n_min = donut.toCssMinified(buf[0..]) catch unreachable;
    // Aligns with Bun scope.cp: preserves space before `to (` even when minified
    expect_eq(
        buf[0..n_min],
        "@scope(.card,.panel) to (.footer,.meta){a { color: blue; }}"
    );

    const empty_implicit = ScopeRule.parse("@scope {   }") catch unreachable;
    const n_empty = empty_implicit.toCss(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_empty], "@scope {}");
    const n_empty_min = empty_implicit.toCssMinified(buf[0..]) catch unreachable;
    expect_eq(buf[0..n_empty_min], "@scope{}");

    // Small buffer triggers BufferTooSmall
    let tiny: [8]u8 = [0; 8];
    let got_small = false;
    * = donut.toCss(tiny[0..]) catch |e| blk: {
        expect_eq(e, CssScopeError.BufferTooSmall);
        got_small = true;
        break :blk 0;
    };
    expect_eq(got_small, true);
});

test("css scope_rule: extractFromStylesheet skips comments/strings and extracts multiple rules", fn() {
    const sheet =
        "/* @scope (.fake-in-comment) { a { color: red; } } */\n" +
        ".banner::after { content: \"@scope (.fake-in-string) { b { color: red; } }\"; }\n" +
        "@scoped { color: gray; }\n" +
        "@scope (.article) to (.comments) { p { line-height: 1.6; } }\n" +
        "body { margin: 0; }\n" +
        "@scope { h2 { font-size: 1.5rem; } }\n";

    const list = ScopeRuleList.extractFromStylesheet(sheet) catch unreachable;
    expect_eq(list.count(), @as(usize, 2));

    const first = list.at(0) ?? unreachable;
    expect_eq(first.isDonut(), true);
    expect_eq(first.start.at(0) ?? "", ".article");
    expect_eq(first.end.at(0) ?? "", ".comments");
    expect_eq(first.body, "p { line-height: 1.6; }");

    const second = list.at(1) ?? unreachable;
    expect_eq(second.isImplicit(), true);
    expect_eq(second.body, "h2 { font-size: 1.5rem; }");
    expect_eq(list.at(2) == null, true);
});

test("css scope_rule: malformed syntax error handling", fn() {
    // @scoped is not @scope
    let err_scoped = false;
    * = ScopeRule.parse("@scoped (.a) { color: red; }") catch |e| blk: {
        expect_eq(e, CssScopeError.InvalidPrelude);
        err_scoped = true;
        break :blk ScopeRule{};
    };
    expect_eq(err_scoped, true);

    // Unclosed parenthesis
    let err_paren = false;
    * = ScopeRule.parse("@scope (.a { color: red; }") catch |e| blk: {
        expect_eq(e, CssScopeError.UnclosedParenthesis);
        err_paren = true;
        break :blk ScopeRule{};
    };
    expect_eq(err_paren, true);

    // Unclosed block
    let err_block = false;
    * = ScopeRule.parse("@scope (.a) { p { color: red; }") catch |e| blk: {
        expect_eq(e, CssScopeError.UnclosedBlock);
        err_block = true;
        break :blk ScopeRule{};
    };
    expect_eq(err_block, true);

    // Empty selector list
    let err_empty = false;
    * = ScopeRule.parse("@scope (   ) { p { color: red; } }") catch |e| blk: {
        expect_eq(e, CssScopeError.EmptySelector);
        err_empty = true;
        break :blk ScopeRule{};
    };
    expect_eq(err_empty, true);

    // Trailing comma
    let err_comma = false;
    * = ScopeRule.parse("@scope (.a,) { p { color: red; } }") catch |e| blk: {
        expect_eq(e, CssScopeError.EmptySelector);
        err_comma = true;
        break :blk ScopeRule{};
    };
    expect_eq(err_comma, true);

    // `to` without parenthesis
    let err_to = false;
    * = ScopeRule.parse("@scope (.a) to .b { p { color: red; } }") catch |e| blk: {
        expect_eq(e, CssScopeError.InvalidPrelude);
        err_to = true;
        break :blk ScopeRule{};
    };
    expect_eq(err_to, true);

    // Trailing unexpected token after closing brace
    let err_trail = false;
    * = ScopeRule.parse("@scope (.a) { p { color: red; } } extra") catch |e| blk: {
        expect_eq(e, CssScopeError.UnexpectedToken);
        err_trail = true;
        break :blk ScopeRule{};
    };
    expect_eq(err_trail, true);
});

test("css scope_rule: selector list capacity boundary (TooManySelectors)", fn() {
    // 16 selectors -> OK
    const ok16 = ScopeSelectorList.parse(
        ".a1,.a2,.a3,.a4,.a5,.a6,.a7,.a8,.a9,.a10,.a11,.a12,.a13,.a14,.a15,.a16"
    ) catch unreachable;
    expect_eq(ok16.count(), @as(usize, 16));
    expect_eq(ok16.at(15) ?? "", ".a16");

    // 17 selectors -> TooManySelectors
    let overflow = false;
    * = ScopeSelectorList.parse(
        ".a1,.a2,.a3,.a4,.a5,.a6,.a7,.a8,.a9,.a10,.a11,.a12,.a13,.a14,.a15,.a16,.a17"
    ) catch |e| blk: {
        expect_eq(e, CssScopeError.TooManySelectors);
        overflow = true;
        break :blk ScopeSelectorList{};
    };
    expect_eq(overflow, true);
});
