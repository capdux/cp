import { test, expect, expect_eq } from "cap:test";
import {
    CSS_UNKNOWN_AT_MAX_RULES,
    CSS_UNKNOWN_AT_MAX_PRELUDE_TOKENS,
    isStandardKnownAtRule,
    CssUnknownAtRuleKind,
    CssUnknownPreludeTokens,
    CssUnknownAtRule,
    CssUnknownAtRuleList,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("css unknown_rule: parse statement and block unknown at-rules", fn() {
    const stmt = CssUnknownAtRule.parse("@tailwind utilities;") catch {
        expect(false);
        return;
    };
    expect(stmt.kind() == CssUnknownAtRuleKind.Statement);
    expect(!stmt.hasBlock());
    expect(stmt.hasPrelude());
    expect(!stmt.isKnownStandard());
    expect(!stmt.isVendorPrefixed());
    expect(strEq(stmt.rule_name, "tailwind"));
    expect(strEq(stmt.prelude, "utilities"));

    let out: [128]u8 = [0; 128];
    const sn = stmt.toCss(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..sn], "@tailwind utilities;"));

    const blk = CssUnknownAtRule.parse("@POSITION-TRY --flip-block { top: anchor(bottom); }") catch {
        expect(false);
        return;
    };
    expect(blk.kind() == CssUnknownAtRuleKind.Block);
    expect(blk.hasBlock());
    expect(blk.hasPrelude());
    expect(strEq(blk.prelude, "--flip-block"));

    const bn = blk.toCss(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..bn], "@position-try --flip-block {\n  top: anchor(bottom);\n}"));

    const mn = blk.toCssMinified(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..mn], "@position-try --flip-block{top: anchor(bottom);}"));
});

test("css unknown_rule: prelude tokenizer preserves functions, brackets and quoted strings", fn() {
    const rule = CssUnknownAtRule.parse(
        "@-moz-document url-prefix(\"https://example.com/a;b\") domain('test.org') [attr=\"x y\"] { body { color: blue; } }",
    ) catch {
        expect(false);
        return;
    };
    expect(rule.isVendorPrefixed());
    expect(!rule.isKnownStandard());

    const toks = rule.preludeTokens() catch {
        expect(false);
        return;
    };
    expect_eq(toks.count(), 3);
    if (t0 := toks.at(0)) {
        expect(strEq(t0, "url-prefix(\"https://example.com/a;b\")"));
    } else {
        expect(false);
    }
    if (t1 := toks.at(1)) {
        expect(strEq(t1, "domain('test.org')"));
    } else {
        expect(false);
    }
    if (t2 := toks.at(2)) {
        expect(strEq(t2, "[attr=\"x y\"]"));
    } else {
        expect(false);
    }
    expect(toks.at(3) == null);
});

test("css unknown_rule: standard known vs unknown classification", fn() {
    expect(isStandardKnownAtRule("media"));
    expect(isStandardKnownAtRule("LAYER"));
    expect(isStandardKnownAtRule("-webkit-keyframes"));
    expect(isStandardKnownAtRule("-ms-viewport"));
    expect(isStandardKnownAtRule("starting-style"));

    expect(!isStandardKnownAtRule("tailwind"));
    expect(!isStandardKnownAtRule("theme"));
    expect(!isStandardKnownAtRule("position-try"));
    expect(!isStandardKnownAtRule("view-transition"));
    expect(!isStandardKnownAtRule("-moz-document"));

    const custom_prefixed = CssUnknownAtRule.statement("--custom-at", "foo");
    expect(!custom_prefixed.isVendorPrefixed());
    const vendor_prefixed = CssUnknownAtRule.statement("-moz-document", "url-prefix()");
    expect(vendor_prefixed.isVendorPrefixed());
});

test("css unknown_rule: extract unknown vs all at-rules from stylesheet with line/col tracking", fn() {
    const sheet =
        "@import \"./reset.css\";\n" ++
        "/* @fake-in-comment { a: b; } */\n" ++
        ".btn { content: \"@fake-in-str;\"; }\n" ++
        "@tailwind base;\n" ++
        "@media (min-width: 768px) { .btn { display: flex; } }\n" ++
        "  @theme { --color-brand: #3b82f6; }\n" ++
        "@view-transition { navigation: auto; }\n";

    const unknown_only = CssUnknownAtRuleList.extractUnknownFromStylesheet(sheet) catch {
        expect(false);
        return;
    };
    expect_eq(unknown_only.count(), 3);

    if (r0 := unknown_only.at(0)) {
        expect(strEq(r0.rule_name, "tailwind"));
        expect(strEq(r0.prelude, "base"));
        expect_eq(r0.line, 4);
        expect_eq(r0.column, 1);
    } else {
        expect(false);
    }

    if (r1 := unknown_only.at(1)) {
        expect(strEq(r1.rule_name, "theme"));
        expect(r1.hasBlock());
        expect_eq(r1.line, 6);
        expect_eq(r1.column, 3);
    } else {
        expect(false);
    }

    if (r2 := unknown_only.at(2)) {
        expect(strEq(r2.rule_name, "view-transition"));
        expect_eq(r2.line, 7);
        expect_eq(r2.column, 1);
    } else {
        expect(false);
    }

    const all_rules = CssUnknownAtRuleList.extractAllAtRulesFromStylesheet(sheet) catch {
        expect(false);
        return;
    };
    expect_eq(all_rules.count(), 5);
});

test("css unknown_rule: empty block, no prelude & equality semantics", fn() {
    const empty_blk = CssUnknownAtRule.parse("@view-transition { }") catch {
        expect(false);
        return;
    };
    expect(empty_blk.hasBlock());
    expect(!empty_blk.hasPrelude());

    let out: [64]u8 = [0; 64];
    const n1 = empty_blk.toCss(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n1], "@view-transition {}"));

    const n2 = empty_blk.toCssMinified(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n2], "@view-transition{}"));

    const a = CssUnknownAtRule.withBlock("Theme", "", "--x: 1;");
    const b = CssUnknownAtRule.parse("@theme { --x: 1; }") catch {
        expect(false);
        return;
    };
    expect(a.eql(b));
    expect(!a.eql(CssUnknownAtRule.statement("theme", "")));
});

test("css unknown_rule: malformed input, unbalanced delimiters & trailing content guards", fn() {
    // 缺少 @
    let ok1 = true;
    * = CssUnknownAtRule.parse("tailwind utilities;") catch {
        ok1 = false;
    };
    expect(!ok1);

    // 语句缺少分号
    let ok2 = true;
    * = CssUnknownAtRule.parse("@tailwind utilities") catch {
        ok2 = false;
    };
    expect(!ok2);

    // 块未闭合
    let ok3 = true;
    * = CssUnknownAtRule.parse("@theme { --a: 1;") catch {
        ok3 = false;
    };
    expect(!ok3);

    // 括号不平衡
    let ok4 = true;
    * = CssUnknownAtRule.parse("@custom (a + b { x: 1; }") catch {
        ok4 = false;
    };
    expect(!ok4);

    // 块内括号不平衡
    let ok5 = true;
    * = CssUnknownAtRule.parse("@custom { color: rgb(1, 2; }") catch {
        ok5 = false;
    };
    expect(!ok5);

    // 尾随非法内容
    let ok6 = true;
    * = CssUnknownAtRule.parse("@tailwind base; @tailwind components;") catch {
        ok6 = false;
    };
    expect(!ok6);
});

test("css unknown_rule: buffer-too-small and capacity constants verification", fn() {
    expect_eq(CSS_UNKNOWN_AT_MAX_RULES, 32);
    expect_eq(CSS_UNKNOWN_AT_MAX_PRELUDE_TOKENS, 32);

    const rule = CssUnknownAtRule.statement("tailwind", "components");
    let tiny: [5]u8 = [0; 5];
    let ok_buf = true;
    * = rule.toCss(tiny[0..]) catch {
        ok_buf = false;
    };
    expect(!ok_buf);

    let ok_min = true;
    * = rule.toCssMinified(tiny[0..]) catch {
        ok_min = false;
    };
    expect(!ok_min);

    // Prelude token 不平衡括号防护
    let ok_tok = true;
    * = CssUnknownPreludeTokens.parse("url(\"unclosed)") catch {
        ok_tok = false;
    };
    expect(!ok_tok);
});
