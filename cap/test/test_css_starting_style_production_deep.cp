import { test, expect } from "cap:test";
import {
    CssStartingStyleRule,
    CssStartingStyleRuleList,
} from "cap:css";

fn eqBytes(a: []const u8, b: []const u8): bool {
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

test("css starting_style: top-level style rules, nested at-rules and comments") {
    const src =
        "/* entry animation */\n" ++
        "@STARTING-STYLE {\n" ++
        "  dialog[open] { opacity: 0; transform: scale(0.95); }\n" ++
        "  @media (prefers-reduced-motion: no-preference) { .toast { translate: 0 12px; } }\n" ++
        "}";

    const rule = CssStartingStyleRule.parse(src)?;
    expect(!rule.isEmpty());
    expect(!rule.hasDirectDeclarations());
    expect(rule.count() == 2);

    if (e0 := rule.get(0)) {
        expect(e0.kind == .style_rule);
        expect(eqBytes(e0.head, "dialog[open]"));
        expect(eqBytes(e0.body, "opacity: 0; transform: scale(0.95);"));
    } else {
        expect(false);
    }

    if (e1 := rule.get(1)) {
        expect(e1.kind == .nested_at_rule);
        expect(eqBytes(e1.head, "@media (prefers-reduced-motion: no-preference)"));
        expect(eqBytes(e1.body, ".toast { translate: 0 12px; }"));
    } else {
        expect(false);
    }

    expect(rule.get(2) == null);
}

test("css starting_style: nested declarations with !important and empty block") {
    const empty_rule = CssStartingStyleRule.parse("@starting-style { /* empty */ }")?;
    expect(empty_rule.isEmpty());
    expect(!empty_rule.hasDirectDeclarations());

    const decl_rule = CssStartingStyleRule.parse(
        "@starting-style { opacity: 0 !important; transform: translateY(8px); }"
    )?;
    expect(decl_rule.hasDirectDeclarations());
    expect(decl_rule.count() == 2);

    if (d0 := decl_rule.get(0)) {
        expect(d0.kind == .declaration);
        expect(eqBytes(d0.head, "opacity"));
        expect(eqBytes(d0.body, "0"));
        expect(d0.important);
    } else {
        expect(false);
    }

    if (d1 := decl_rule.get(1)) {
        expect(d1.kind == .declaration);
        expect(eqBytes(d1.head, "transform"));
        expect(eqBytes(d1.body, "translateY(8px)"));
        expect(!d1.important);
    } else {
        expect(false);
    }
}

test("css starting_style: liftForParent flattens nested declarations, & and descendants") {
    const nested = CssStartingStyleRule.parse(
        "@starting-style {\n" ++
        "  opacity: 0;\n" ++
        "  transform: scale(0.9) !important;\n" ++
        "  &:hover { opacity: 0.5; }\n" ++
        "  .title { color: transparent; }\n" ++
        "  @media (min-width: 640px) { translate: 0 16px; }\n" ++
        "}"
    )?;

    let buf: [512]u8 = [0; 512];
    const n = nested.liftForParent(".popover", buf[0..])?;
    expect(eqBytes(
        buf[0..n],
        "@starting-style { .popover { opacity: 0; transform: scale(0.9) !important; } .popover:hover { opacity: 0.5; } .popover .title { color: transparent; } @media (min-width: 640px) { .popover { translate: 0 16px; } } }"
    ));

    // 空父选择器拒绝
    let empty_err: usize = 0;
    * = nested.liftForParent("   ", buf[0..]) catch { empty_err += 1; 0; };
    expect(empty_err == 1);
}

test("css starting_style: canonical and minified serialization with buffer guard") {
    const rule = CssStartingStyleRule.parse(
        "@starting-style { opacity: 0 !important; dialog[open] { scale: 0.9; } }"
    )?;

    let buf: [256]u8 = [0; 256];
    const n_pretty = rule.toCss(buf[0..])?;
    expect(eqBytes(
        buf[0..n_pretty],
        "@starting-style { opacity: 0 !important; dialog[open] { scale: 0.9; } }"
    ));

    const n_min = rule.toCssMinified(buf[0..])?;
    expect(eqBytes(
        buf[0..n_min],
        "@starting-style{opacity:0!important;dialog[open]{scale:0.9;}}"
    ));

    // 缓冲区不足防护
    let tiny: [12]u8 = [0; 12];
    const small_res = rule.toCss(tiny[0..]) catch {
        expect(true);
        return;
    };
    * = small_res;
    expect(false);
}

test("css starting_style: stylesheet extraction with accurate line/column tracking") {
    const css =
        ".popover { opacity: 1; transition: opacity 0.2s; }\n" ++
        "@starting-style {\n" ++
        "  .popover { opacity: 0; }\n" ++
        "}\n" ++
        "/* mid comment */\n" ++
        "@media screen { .a { color: red; } }\n" ++
        "@starting-style { dialog[open] { transform: translateY(10px); } }\n";

    const list = CssStartingStyleRuleList.extractFromStylesheet(css)?;
    expect(list.count() == 2);

    if (r0 := list.get(0)) {
        expect(r0.line == 2);
        expect(r0.column == 1);
        expect(r0.count() == 1);
    } else {
        expect(false);
    }

    if (r1 := list.get(1)) {
        expect(r1.line == 7);
        expect(r1.column == 1);
        expect(r1.count() == 1);
    } else {
        expect(false);
    }

    expect(list.get(2) == null);
}

test("css starting_style: malformed input guards and round-trip idempotency") {
    let rejected: usize = 0;

    // 非 @starting-style / 非空 prelude / 未闭合块 / 尾随垃圾
    * = CssStartingStyleRule.parse("@media screen { }") catch { rejected += 1; CssStartingStyleRule.empty(); };
    * = CssStartingStyleRule.parse("@starting-style screen { opacity: 0; }") catch { rejected += 1; CssStartingStyleRule.empty(); };
    * = CssStartingStyleRule.parse("@starting-style { opacity: 0;") catch { rejected += 1; CssStartingStyleRule.empty(); };
    * = CssStartingStyleRule.parse("@starting-style { } extra") catch { rejected += 1; CssStartingStyleRule.empty(); };
    // 非法嵌套 @import / @charset / @namespace
    * = CssStartingStyleRule.parse("@starting-style { @import \"a.css\"; }") catch { rejected += 1; CssStartingStyleRule.empty(); };
    * = CssStartingStyleRule.parse("@starting-style { @charset \"utf-8\"; }") catch { rejected += 1; CssStartingStyleRule.empty(); };
    * = CssStartingStyleRule.parse("@starting-style { @namespace svg url(http://www.w3.org/2000/svg); }") catch { rejected += 1; CssStartingStyleRule.empty(); };
    // 非法声明（缺冒号、空属性名、空值）
    * = CssStartingStyleRule.parse("@starting-style { opacity; }") catch { rejected += 1; CssStartingStyleRule.empty(); };
    * = CssStartingStyleRule.parse("@starting-style { : 0; }") catch { rejected += 1; CssStartingStyleRule.empty(); };
    * = CssStartingStyleRule.parse("@starting-style { opacity: ; }") catch { rejected += 1; CssStartingStyleRule.empty(); };

    expect(rejected == 10);

    // Round-trip 幂等性
    const src = "@starting-style { opacity: 0; dialog[open] { scale: 0.95; } }";
    const r1 = CssStartingStyleRule.parse(src)?;
    let buf1: [256]u8 = [0; 256];
    const n1 = r1.toCss(buf1[0..])?;
    const r2 = CssStartingStyleRule.parse(buf1[0..n1])?;
    expect(r1.eql(r2));

    let buf2: [256]u8 = [0; 256];
    const n2 = r2.toCss(buf2[0..])?;
    expect(eqBytes(buf1[0..n1], buf2[0..n2]));
}
