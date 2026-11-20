// cap/test/test_css_deep.cp
// Deep End-to-End Test Suite for Zero-Bun Pure CP cap:css Engine
// Covers CssTokenizer, CssColor (148 Named, HWB, OKLAB, OKLCH, Lighten/Darken),
// CssCalc (nested parens, min/max/clamp), CssSelector (:is/:not/:where/:has, attr ops, SelectorList),
// CssVarResolver, CssPrefixer & CssLogical, CssNesting, CssStylesheet AST & Pretty Printer.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CssTokenKind,
    CssToken,
    CssTokenizer,
    CssColor,
    CssUnit,
    CssLength,
    CssCalc,
    CssSpecificity,
    CssSelector,
    CssSelectorList,
    CssVariableMap,
    CssVarResolver,
    CssVendorPrefix,
    CssExpandedDecl,
    CssPrefixer,
    CssLogical,
    CssProperty,
    CssRuleKind,
    CssMediaQuery,
    CssKeyframeStep,
    CssKeyframes,
    CssRule,
    CssStylesheetParser,
    CssNesting,
    CssStylesheet,
} from "cap:css";

fn main() {
    test("1. cap:css — CssTokenizer (Idents, Functions, AtKeywords, Numbers, Dimensions, Strings, Urls, Comments)", () => {
        const css = "/* header */ @media screen { .btn#ok { width: calc(100% - 16px); content: \"a b\"; bg: url(icon.svg); } }";
        let tz = CssTokenizer.init(css);
        let tokens: [64]CssToken = std.mem.zeroes([64]CssToken);
        const n = tz.tokenizeAll(tokens[0..64], true);
        expect_eq(n > 15, true);

        expect_eq(tokens[0].kind == CssTokenKind.AtKeyword, true);
        expect_eq(std.mem.eql(u8, tokens[0].value, "media"), true);
        expect_eq(tokens[1].kind == CssTokenKind.Ident, true);
        expect_eq(std.mem.eql(u8, tokens[1].value, "screen"), true);
        expect_eq(tokens[2].kind == CssTokenKind.LeftBrace, true);

        // Locate specific token kinds
        let found_hash = false;
        let found_func = false;
        let found_pct = false;
        let found_dim = false;
        let found_str = false;
        let found_url = false;
        let i: usize = 0;
        while (i < n) {
            if (tokens[i].kind == CssTokenKind.Hash && std.mem.eql(u8, tokens[i].value, "ok")) {
                found_hash = true;
            } else if (tokens[i].kind == CssTokenKind.Function && std.mem.eql(u8, tokens[i].value, "calc")) {
                found_func = true;
            } else if (tokens[i].kind == CssTokenKind.Percentage && tokens[i].num_val == 100.0) {
                found_pct = true;
            } else if (tokens[i].kind == CssTokenKind.Dimension && std.mem.eql(u8, tokens[i].unit, "px") && tokens[i].num_val == 16.0) {
                found_dim = true;
            } else if (tokens[i].kind == CssTokenKind.String && std.mem.eql(u8, tokens[i].value, "a b")) {
                found_str = true;
            } else if (tokens[i].kind == CssTokenKind.Url && std.mem.eql(u8, tokens[i].value, "icon.svg")) {
                found_url = true;
            }
            i += 1;
        }
        expect_eq(found_hash, true);
        expect_eq(found_func, true);
        expect_eq(found_pct, true);
        expect_eq(found_dim, true);
        expect_eq(found_str, true);
        expect_eq(found_url, true);

        // CDO/CDC & Comment tokens preserved when skip_trivia=false
        let tz2 = CssTokenizer.init("<!-- /* c */ -->");
        let t_all: [8]CssToken = std.mem.zeroes([8]CssToken);
        const n2 = tz2.tokenizeAll(t_all[0..8], false);
        expect_eq(n2 >= 5, true);
        expect_eq(t_all[0].kind == CssTokenKind.Cdo, true);
        expect_eq(t_all[2].kind == CssTokenKind.Comment, true);
        expect_eq(t_all[4].kind == CssTokenKind.Cdc, true);
    });

    test("2. cap:css — CssColor Extended Spaces (HWB, OKLAB, 148 W3C Named Colors, Lighten/Darken, Angle Units)", () => {
        // 148 named colors (binary search)
        const crimson = CssColor.parse("crimson").?;
        expect_eq(crimson.r, 220 as u8);
        expect_eq(crimson.g, 20 as u8);
        expect_eq(crimson.b, 60 as u8);

        const dodger = CssColor.parse("DodgerBlue").?;
        expect_eq(dodger.r, 30 as u8);
        expect_eq(dodger.g, 144 as u8);
        expect_eq(dodger.b, 255 as u8);

        const yg = CssColor.parse("yellowgreen").?;
        expect_eq(yg.r, 154 as u8);
        expect_eq(yg.g, 205 as u8);
        expect_eq(yg.b, 50 as u8);

        // HWB: hwb(0deg 0% 0%) -> pure red; hwb(120deg 20% 20%)
        const hwb_red = CssColor.parse("hwb(0deg 0% 0%)").?;
        expect_eq(hwb_red.r, 255 as u8);
        expect_eq(hwb_red.g, 0 as u8);
        expect_eq(hwb_red.b, 0 as u8);

        const hwb_mix = CssColor.parse("hwb(240 50% 50% / 50%)").?;
        expect_eq(hwb_mix.r, 128 as u8);
        expect_eq(hwb_mix.g, 128 as u8);
        expect_eq(hwb_mix.b, 128 as u8);
        expect_eq(hwb_mix.a, 128 as u8);

        // OKLAB: oklab(62.8% 0.225 0.126) -> vivid red-orange
        const oklab_c = CssColor.parse("oklab(0.628 0.225 0.126)").?;
        expect_eq(oklab_c.r > 230, true);
        expect_eq(oklab_c.g < 40, true);

        // Angle units in HSL: 0.5turn == 180deg (cyan), 200grad == 180deg
        const hsl_turn = CssColor.parse("hsl(0.5turn 100% 50%)").?;
        expect_eq(hsl_turn.r, 0 as u8);
        expect_eq(hsl_turn.g, 255 as u8);
        expect_eq(hsl_turn.b, 255 as u8);

        const hsl_grad = CssColor.parse("hsl(200grad 100% 50%)").?;
        expect_eq(hsl_grad.eql(hsl_turn), true);

        // Lighten / darken / withAlpha / toHsl
        const base = CssColor.fromHsl(210.0, 80.0, 40.0, 255);
        const lighter = base.lighten(20.0);
        const darker = base.darken(20.0);
        expect_eq(lighter.luminance() > base.luminance(), true);
        expect_eq(darker.luminance() < base.luminance(), true);
        expect_eq(base.withAlpha(100).a, 100 as u8);
    });

    test("3. cap:css — CssLength Extended Units & Recursive-Descent CssCalc (parens, min, max, clamp)", () => {
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("2pc").?.toPx(16.0, 1000.0, 800.0))), 32);
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("100grad").?.toDeg())), 90);
        expect_eq(CssLength.px(10.0).scale(2.5).eql(CssLength.px(25.0)), true);

        // Nested parentheses & tightly packed operators
        const c_paren = CssCalc.evalPx("calc((100vw - 2rem) / 2)", 16.0, 1032.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c_paren)), 500);

        const c_tight = CssCalc.evalPx("calc(10px+20px*3)", 16.0, 1000.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c_tight)), 70);

        // Unary sign
        const c_unary = CssCalc.evalPx("calc(-10px + 50px)", 16.0, 1000.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c_unary)), 40);

        // min(), max(), clamp()
        const c_min = CssCalc.evalPx("min(50vw, 300px, 20rem)", 16.0, 1000.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c_min)), 300);

        const c_max = CssCalc.evalPx("max(10vw, 150px, 5rem)", 16.0, 1000.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c_max)), 150);

        const c_clamp = CssCalc.evalPx("clamp(16px, 5vw, 48px)", 16.0, 600.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c_clamp)), 30); // 5% of 600 = 30, within [16, 48]

        const c_clamp_hi = CssCalc.evalPx("clamp(16px, 10vw, 48px)", 16.0, 1200.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c_clamp_hi)), 48); // 120 clamped to 48
    });

    test("4. cap:css — Selectors Level 4 (:is/:not/:has/:where, multi-class, attr ops, CssSelectorList)", () => {
        // Multi-class matching requires ALL classes on subject
        const multi = CssSelector.parse("button.btn.primary");
        expect_eq(multi.specificity.classes, 2 as u32);
        expect_eq(multi.matchesElementAllClasses("button", "", "btn primary large"), true);
        expect_eq(multi.matchesElementAllClasses("button", "", "btn large"), false);

        // :is() & :not() take max specificity of arguments; :where() contributes 0
        const s_is = CssSelector.parse("div:is(#hero, .card)");
        expect_eq(s_is.specificity.ids, 1 as u32);
        expect_eq(s_is.specificity.elements, 1 as u32);

        const s_not = CssSelector.parse("li:not(.disabled, .hidden)");
        expect_eq(s_not.specificity.classes, 1 as u32);
        expect_eq(s_not.specificity.elements, 1 as u32);

        const s_where = CssSelector.parse("section:where(#main, .box)");
        expect_eq(s_where.specificity.ids, 0 as u32);
        expect_eq(s_where.specificity.classes, 0 as u32);
        expect_eq(s_where.specificity.elements, 1 as u32);

        // Attribute operators: =, ~=, |=, ^=, $=, *=
        const attr_prefix = CssSelector.parse("a[href^=\"https\"]");
        expect_eq(attr_prefix.matchesAttribute("href", "https://cp-lang.org"), true);
        expect_eq(attr_prefix.matchesAttribute("href", "http://cp-lang.org"), false);

        const attr_word = CssSelector.parse("div[data-tags~=\"fast\"]");
        expect_eq(attr_word.matchesAttribute("data-tags", "safe fast native"), true);
        expect_eq(attr_word.matchesAttribute("data-tags", "faster"), false);

        const attr_suffix = CssSelector.parse("img[src$=\".png\"]");
        expect_eq(attr_suffix.matchesAttribute("src", "logo.png"), true);
        expect_eq(attr_suffix.matchesAttribute("src", "logo.jpg"), false);

        // SelectorList
        const list = CssSelectorList.parse("h1, .card#featured, nav > a.active");
        expect_eq(list.count, 3 as usize);
        expect_eq(list.maxSpecificity().ids, 1 as u32);
        expect_eq(list.matchesElement("h1", "", ""), true);
        expect_eq(list.matchesElement("div", "featured", "card"), true);
        expect_eq(list.matchesElement("span", "", "other"), false);
    });

    test("5. cap:css — CssVariableMap & Recursive CssVarResolver", () => {
        let vars = CssVariableMap.init();
        expect_eq(vars.set("--primary", "#2563eb"), true);
        expect_eq(vars.set("--btn-bg", "var(--primary)"), true);
        expect_eq(vars.set("--border", "1px solid var(--accent, #cbd5e1)"), true);
        expect_eq(vars.len(), 3 as usize);
        expect_eq(vars.has("--primary"), true);

        let out: [256]u8 = [0; 256];
        const r1 = CssVarResolver.resolve("var(--btn-bg)", vars, out[0..256]);
        expect_eq(std.mem.eql(u8, r1, "#2563eb"), true);

        const r2 = CssVarResolver.resolve("var(--missing, var(--btn-bg))", vars, out[0..256]);
        expect_eq(std.mem.eql(u8, r2, "#2563eb"), true);

        const r3 = CssVarResolver.resolve("border: var(--border)", vars, out[0..256]);
        expect_eq(std.mem.eql(u8, r3, "border: 1px solid #cbd5e1"), true);

        // Cycle protection should terminate cleanly without stack overflow
        expect_eq(vars.set("--loop-a", "var(--loop-b)"), true);
        expect_eq(vars.set("--loop-b", "var(--loop-a)"), true);
        const r_cycle = CssVarResolver.resolve("var(--loop-a)", vars, out[0..256]);
        expect_eq(r_cycle.len > 0, true);
    });

    test("6. cap:css — CssPrefixer & CssLogical Property Expansion", () => {
        const vp = CssPrefixer.prefixesForProperty("user-select");
        expect_eq(vp.hasWebkit(), true);
        expect_eq(vp.hasMoz(), true);
        expect_eq(vp.hasMs(), true);
        expect_eq(vp.hasNone(), true);

        let decls: [8]CssExpandedDecl = std.mem.zeroes([8]CssExpandedDecl);
        let scratch: [256]u8 = [0; 256];
        const n_pref = CssPrefixer.expand("user-select", "none", false, vp, decls[0..8], scratch[0..256]);
        expect_eq(n_pref, 4 as usize);
        expect_eq(std.mem.eql(u8, decls[0].name, "-webkit-user-select"), true);
        expect_eq(std.mem.eql(u8, decls[1].name, "-moz-user-select"), true);
        expect_eq(std.mem.eql(u8, decls[2].name, "-ms-user-select"), true);
        expect_eq(std.mem.eql(u8, decls[3].name, "user-select"), true);

        // Logical properties -> physical properties
        expect_eq(CssLogical.isLogicalProperty("margin-inline"), true);
        const n_mi = CssLogical.expand("margin-inline", "1rem 2rem", false, decls[0..8]);
        expect_eq(n_mi, 2 as usize);
        expect_eq(std.mem.eql(u8, decls[0].name, "margin-left"), true);
        expect_eq(std.mem.eql(u8, decls[0].value, "1rem"), true);
        expect_eq(std.mem.eql(u8, decls[1].name, "margin-right"), true);
        expect_eq(std.mem.eql(u8, decls[1].value, "2rem"), true);

        const n_pb = CssLogical.expand("padding-block", "12px", true, decls[0..8]);
        expect_eq(n_pb, 2 as usize);
        expect_eq(std.mem.eql(u8, decls[0].name, "padding-top"), true);
        expect_eq(std.mem.eql(u8, decls[1].name, "padding-bottom"), true);
        expect_eq(decls[0].important, true);

        const n_sz = CssLogical.expand("inline-size", "320px", false, decls[0..8]);
        expect_eq(n_sz, 1 as usize);
        expect_eq(std.mem.eql(u8, decls[0].name, "width"), true);
    });

    test("7. cap:css — CssNesting Flattening, CssStylesheet AST, MediaQuery Full, Minify & Pretty Print", () => {
        // CSS Nesting flattening
        const nested_css =
            ".card {\n" ++
            "  color: #111;\n" ++
            "  &:hover { color: #2563eb; }\n" ++
            "  & .title { font-size: 1.25rem; }\n" ++
            "  > p { margin: 0; }\n" ++
            "}\n";
        let flat_buf: [1024]u8 = [0; 1024];
        const flat = CssNesting.flatten(nested_css, flat_buf[0..1024]);
        expect_eq(std.mem.indexOf(u8, flat, ".card { color: #111; }") != null, true);
        expect_eq(std.mem.indexOf(u8, flat, ".card:hover { color: #2563eb; }") != null, true);
        expect_eq(std.mem.indexOf(u8, flat, ".card .title { font-size: 1.25rem; }") != null, true);
        expect_eq(std.mem.indexOf(u8, flat, ".card > p { margin: 0; }") != null, true);

        // CssStylesheet AST & variable collection
        const full_css =
            "@namespace svg url(\"http://www.w3.org/2000/svg\");\n" ++
            "@property --accent { syntax: \"<color>\"; initial-value: #2563eb; }\n" ++
            "@scope (.card) { .title { color: var(--accent); } }\n" ++
            "@starting-style { .dialog { opacity: 0; } }\n" ++
            ":root { --accent: #3b82f6; --gap: 16px; }\n" ++
            ".box { padding: var(--gap); content: \"keep   spaces\"; }\n";

        let sheet = CssStylesheet.parse(full_css);
        expect_eq(sheet.count(), 6 as usize);
        expect_eq(sheet.rules[0].kind == CssRuleKind.Namespace, true);
        expect_eq(sheet.rules[1].kind == CssRuleKind.Property, true);
        expect_eq(sheet.rules[2].kind == CssRuleKind.Scope, true);
        expect_eq(sheet.rules[3].kind == CssRuleKind.StartingStyle, true);

        const vars = sheet.collectVariables();
        expect_eq(std.mem.eql(u8, vars.get("--accent").?, "#3b82f6"), true);
        expect_eq(std.mem.eql(u8, vars.get("--gap").?, "16px"), true);

        let box_rule = sheet.findRuleBySelector(".box").?;
        expect_eq(box_rule.hasProperty("padding"), true);
        expect_eq(box_rule.setProperty("margin", "8px", false), true);
        expect_eq(box_rule.hasProperty("margin"), true);
        expect_eq(box_rule.removeProperty("padding"), true);
        expect_eq(box_rule.hasProperty("padding"), false);

        // MediaQuery full evaluation
        const mq_full = CssMediaQuery.parse("@media (min-height: 600px) and (prefers-reduced-motion: reduce) and (orientation: landscape)");
        expect_eq(mq_full.matchesFull(1280.0, 720.0, false, true), true);
        expect_eq(mq_full.matchesFull(1280.0, 720.0, false, false), false);
        expect_eq(mq_full.matchesFull(600.0, 900.0, false, true), false);

        // Minify preserves quoted spaces & strips comments
        let min_buf: [512]u8 = [0; 512];
        const min_str = CssStylesheetParser.minify("/* c */ .tag { content: \"a   b\"; color: red; }", min_buf[0..512]);
        expect_eq(std.mem.eql(u8, min_str, ".tag{content:\"a   b\";color:red}"), true);

        // Pretty printing
        let pretty_buf: [512]u8 = [0; 512];
        const pretty = CssStylesheetParser.formatPretty(".tag{color:red;margin:0}", pretty_buf[0..512]);
        expect_eq(std.mem.indexOf(u8, pretty, ".tag {\n  color: red;\n  margin: 0;\n}") != null, true);
    });
}
