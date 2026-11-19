// cap/test/test_css_full.cp
// Full-Coverage E2E Regression Suite for cap:css Industrial CSS Engine
// Covers:
// 1. Color Spaces (Hex 3/4/6/8, RGB/RGBA legacy & modern, HSL/HSLA, OKLCH, Named, Mix, WCAG Contrast)
// 2. Units & Values (18 CssUnit variants, Length/Time/Angle conversions, formatting, CssCalc precedence & edge cases)
// 3. Selectors Level 4 & W3C Specificity (combinators, attrs, :where(), ::pseudo-element, DOM matching)
// 4. Declarations, @media, @keyframes, All At-Rules, Stylesheet Parsing & Minification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CssColor,
    CssUnit,
    CssLength,
    CssCalc,
    CssSpecificity,
    CssSelector,
    CssProperty,
    CssRuleKind,
    CssMediaQuery,
    CssKeyframeStep,
    CssKeyframes,
    CssRule,
    CssStylesheetParser,
} from "cap:css";

fn main() {
    test("1. cap:css — Color Spaces (Hex, RGB/RGBA, HSL/HSLA, OKLCH, Named, Mix, WCAG)", () => {
        // Hex 3/4/6/8
        const h3 = CssColor.parse("#0f8").?;
        expect_eq(h3.r, 0 as u8);
        expect_eq(h3.g, 255 as u8);
        expect_eq(h3.b, 136 as u8);
        expect_eq(h3.a, 255 as u8);

        const h4 = CssColor.parse("#f088").?;
        expect_eq(h4.r, 255 as u8);
        expect_eq(h4.g, 0 as u8);
        expect_eq(h4.b, 136 as u8);
        expect_eq(h4.a, 136 as u8);

        const h6 = CssColor.parse("#1e293b").?;
        expect_eq(h6.r, 30 as u8);
        expect_eq(h6.g, 41 as u8);
        expect_eq(h6.b, 59 as u8);

        const h8 = CssColor.parse("#3b82f680").?;
        expect_eq(h8.a, 128 as u8);

        // Invalid hex rejected
        expect_eq(CssColor.parse("#12345") == null, true);
        expect_eq(CssColor.parse("#zzzzzz") == null, true);

        // RGB legacy & modern space/slash + percentage channels
        const rgb_pct = CssColor.parse("rgb(100%, 50%, 0%)").?;
        expect_eq(rgb_pct.r, 255 as u8);
        expect_eq(rgb_pct.g, 128 as u8);
        expect_eq(rgb_pct.b, 0 as u8);

        const rgb_mod = CssColor.parse("rgb(10 20 30 / 25%)").?;
        expect_eq(rgb_mod.r, 10 as u8);
        expect_eq(rgb_mod.g, 20 as u8);
        expect_eq(rgb_mod.b, 30 as u8);
        expect_eq(rgb_mod.a, 64 as u8);

        // HSL & HSLA
        const hsl_blue = CssColor.parse("hsla(240deg, 100%, 50%, 0.5)").?;
        expect_eq(hsl_blue.r, 0 as u8);
        expect_eq(hsl_blue.g, 0 as u8);
        expect_eq(hsl_blue.b, 255 as u8);
        expect_eq(hsl_blue.a, 128 as u8);

        // Grayscale HSL (saturation == 0)
        const hsl_gray = CssColor.fromHsl(0.0, 0.0, 50.0, 255);
        expect_eq(hsl_gray.r, 128 as u8);
        expect_eq(hsl_gray.g, 128 as u8);
        expect_eq(hsl_gray.b, 128 as u8);

        // OKLCH
        const oklch_red = CssColor.parse("oklch(0.628 0.258 29.23deg)").?;
        expect_eq(oklch_red.is_oklch, true);
        expect_eq(oklch_red.r > 240, true);
        expect_eq(oklch_red.g < 30, true);

        // Named colors & formatting
        const transp = CssColor.parse("transparent").?;
        expect_eq(transp.a, 0 as u8);
        const coral = CssColor.parse("coral").?;
        expect_eq(coral.r, 255 as u8);
        expect_eq(coral.g, 127 as u8);
        expect_eq(coral.b, 80 as u8);

        let fmt_buf: [32]u8 = [0; 32];
        expect_eq(std.mem.eql(u8, coral.toHex(fmt_buf[0..32]), "#ff7f50"), true);
        expect_eq(std.mem.eql(u8, coral.toRgbString(fmt_buf[0..32]), "rgb(255, 127, 80)"), true);
        expect_eq(std.mem.eql(u8, h8.toRgbString(fmt_buf[0..32]), "rgba(59, 130, 246, 128)"), true);

        // Color mix & WCAG contrast
        const black = CssColor.rgb(0, 0, 0);
        const white = CssColor.rgb(255, 255, 255);
        const mid = black.mix(white, 0.5);
        expect_eq(mid.r, 128 as u8);
        expect_eq(mid.eql(CssColor.rgb(128, 128, 128)), true);
        const cr = white.contrastRatio(black);
        expect_eq(cr > 20.9 && cr < 21.1, true);
    });

    test("2. cap:css — Units (Length/Time/Angle), Formatting & CssCalc Evaluation", () => {
        // Zero normalizes to 0px
        const z = CssLength.parse("0").?;
        expect_eq(z.unit == CssUnit.Px, true);
        let fmt_buf: [32]u8 = [0; 32];
        expect_eq(std.mem.eql(u8, z.format(fmt_buf[0..32]), "0px"), true);

        // Absolute & relative lengths
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("1in").?.toPx(16.0, 1000.0, 800.0))), 96);
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("72pt").?.toPx(16.0, 1000.0, 800.0))), 96);
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("10vmin").?.toPx(16.0, 1000.0, 800.0))), 80);
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("10vmax").?.toPx(16.0, 1000.0, 800.0))), 100);
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("25vh").?.toPx(16.0, 1000.0, 800.0))), 200);
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("2.5rem").?.toPx(16.0, 1000.0, 800.0))), 40);

        // Time & Angle
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("1.5s").?.toMs())), 1500);
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("250ms").?.toMs())), 250);
        expect_eq(@as(i64, @intFromFloat(CssLength.parse("0.25turn").?.toDeg())), 90);
        const rad_len = CssLength.parse("3.14159265rad").?;
        const pi_rad: f64 = rad_len.toDeg();
        expect_eq(pi_rad > 179.9 && pi_rad < 180.1, true);

        // Format integer & float lengths
        expect_eq(std.mem.eql(u8, CssLength.px(24.0).format(fmt_buf[0..32]), "24px"), true);
        expect_eq(std.mem.eql(u8, CssLength.rem(1.25).format(fmt_buf[0..32]), "1.25rem"), true);

        // Invalid unit rejected
        expect_eq(CssLength.parse("12foo") == null, true);

        // CssCalc precedence: +, -, *, /
        const c1 = CssCalc.evalPx("calc(100vw - 2 * 2rem + 16px)", 16.0, 1000.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c1)), 952); // 1000 - 64 + 16 = 952

        const c2 = CssCalc.evalPx("calc(50vh / 2 + 10px)", 16.0, 1000.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(c2)), 210); // 400 / 2 + 10 = 210

        // Division by zero safely returns null
        expect_eq(CssCalc.evalPx("calc(100px / 0)", 16.0, 1000.0, 800.0) == null, true);
    });

    test("3. cap:css — Selectors Level 4, Combinators, :where() & W3C Specificity", () => {
        const s_univ = CssSelector.parse("*");
        expect_eq(s_univ.specificity.score(), 0 as u32);
        expect_eq(s_univ.matchesTag("section"), true);

        // :where() contributes 0 specificity
        const s_where = CssSelector.parse("div:where(.ignored)");
        expect_eq(s_where.specificity.ids, 0 as u32);
        expect_eq(s_where.specificity.classes, 0 as u32);
        expect_eq(s_where.specificity.elements, 1 as u32);

        // Complex selector with combinators, attribute operator ^=, pseudo-class & pseudo-element
        const s_complex = CssSelector.parse("main#app > section + a.cta.large[href^=\"https\"]:focus::before");
        expect_eq(s_complex.combinator_count, 2 as u32);
        expect_eq(s_complex.specificity.ids, 1 as u32);
        expect_eq(s_complex.specificity.classes, 4 as u32); // .cta + .large + [href^=...] + :focus
        expect_eq(s_complex.specificity.elements, 4 as u32); // main + section + a + ::before
        expect_eq(std.mem.eql(u8, s_complex.tag_name, "a"), true);
        expect_eq(std.mem.eql(u8, s_complex.class_name, "cta"), true);
        expect_eq(std.mem.eql(u8, s_complex.attr_name, "href"), true);
        expect_eq(std.mem.eql(u8, s_complex.attr_val, "https"), true);
        expect_eq(std.mem.eql(u8, s_complex.pseudo_class, "focus"), true);
        expect_eq(std.mem.eql(u8, s_complex.pseudo_element, "before"), true);

        expect_eq(s_complex.matchesElement("a", "", "btn cta primary"), true);
        expect_eq(s_complex.matchesElement("a", "", "btn primary"), false);
        expect_eq(s_complex.specificity.compare(s_where.specificity), 1 as i32);
        expect_eq(s_where.specificity.compare(s_complex.specificity), -1 as i32);
    });

    test("4. cap:css — Declarations, @media Dark/Breakpoints, @keyframes & Full At-Rules", () => {
        // Property parse & formatting
        const p_imp = CssProperty.parse("display: flex !important;").?;
        expect_eq(p_imp.important, true);
        expect_eq(p_imp.isCustomProperty(), false);
        let p_buf: [64]u8 = [0; 64];
        expect_eq(std.mem.eql(u8, p_imp.format(p_buf[0..64], true), "display:flex!important"), true);
        expect_eq(std.mem.eql(u8, p_imp.format(p_buf[0..64], false), "display: flex !important;"), true);

        const p_var = CssProperty.parse("--accent-color: #2563eb").?;
        expect_eq(p_var.isCustomProperty(), true);

        // @media with dark color scheme
        const mq = CssMediaQuery.parse("@media screen and (min-width: 640px) and (max-width: 1280px) and (prefers-color-scheme: dark)");
        expect_eq(std.mem.eql(u8, mq.media_type, "screen"), true);
        expect_eq(mq.prefers_dark, true);
        expect_eq(mq.matches(1024.0, true), true);
        expect_eq(mq.matches(1024.0, false), false);
        expect_eq(mq.matches(375.0, true), false);

        // Full multi-rule stylesheet with @import, @layer, @supports, @container, @font-face, @keyframes, style
        const sheet =
            "/* Global Sheet */\n" ++
            "@import \"tokens.css\";\n" ++
            "@layer base { body { margin: 0; } }\n" ++
            "@supports (display: grid) { .grid { display: grid; } }\n" ++
            "@container (min-width: 400px) { .card { padding: 2rem; } }\n" ++
            "@font-face { font-family: Inter; font-display: swap; }\n" ++
            "@keyframes pulse { from { opacity: 0.2; } 50% { opacity: 1; } to { opacity: 0.2; } }\n" ++
            "button#go.primary { color: #fff; background: #000 !important; }\n";

        let rules: [12]CssRule = std.mem.zeroes([12]CssRule);
        const count = CssStylesheetParser.parseStylesheet(sheet, rules[0..12]);
        expect_eq(count, 7 as usize);

        expect_eq(rules[0].kind == CssRuleKind.Import, true);
        expect_eq(rules[1].kind == CssRuleKind.Layer, true);
        expect_eq(rules[2].kind == CssRuleKind.Supports, true);
        expect_eq(rules[3].kind == CssRuleKind.Container, true);
        expect_eq(rules[4].kind == CssRuleKind.FontFace, true);
        expect_eq(std.mem.eql(u8, rules[4].getProperty("font-family").?, "Inter"), true);
        expect_eq(rules[5].kind == CssRuleKind.Keyframes, true);
        expect_eq(rules[6].kind == CssRuleKind.Style, true);
        expect_eq(rules[6].selector.specificity.ids, 1 as u32);
        expect_eq(rules[6].selector.specificity.classes, 1 as u32);
        expect_eq(rules[6].selector.specificity.elements, 1 as u32);

        const kf = CssKeyframes.parse("@keyframes pulse { from { opacity: 0.2; } 50% { opacity: 1; } to { opacity: 0.2; } }").?;
        expect_eq(std.mem.eql(u8, kf.name, "pulse"), true);
        expect_eq(kf.step_count, 3 as usize);
        expect_eq(@as(i64, @intFromFloat(kf.steps[1].offset_percent)), 50);

        // Minification strips comments, whitespace, and trailing semicolons before '}'
        let min_buf: [512]u8 = [0; 512];
        const min_out = CssStylesheetParser.minify(
            "/* banner */ .hero, .card { color: #fff; margin: 0px 12px; }\n",
            min_buf[0..512],
        );
        expect_eq(std.mem.eql(u8, min_out, ".hero,.card{color:#fff;margin:0px 12px}"), true);
    });
}
