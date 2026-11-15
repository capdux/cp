// cap/test/test_css.cp
// Comprehensive End-to-End Test Suite for CSS Subsystem (cap:css)
// Covers CssColor (Hex, RGB/RGBA, HSL/HSLA, OKLCH, Named Colors, WCAG Contrast),
// CssLength & CssUnit & CssCalc, CssSelector & CssSpecificity,
// and CssRule / CssMediaQuery / CssKeyframes / CssStylesheetParser.

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
    CssStylesheetParser
} from "cap:css";

fn main() {
    test("1. cap:css — CssColor Hex (#rgb, #rgba, #rrggbb, #rrggbbaa) & Named Color Parsing", () => {
        const c1 = CssColor.parse("#f0a").?;
        expect_eq(c1.r, 255);
        expect_eq(c1.g, 0);
        expect_eq(c1.b, 170);
        expect_eq(c1.a, 255);

        const c2 = CssColor.parse("#ff800080").?;
        expect_eq(c2.r, 255);
        expect_eq(c2.g, 128);
        expect_eq(c2.b, 0);
        expect_eq(c2.a, 128);

        let hex_buf: [16]u8 = [0; 16];
        expect_eq(std.mem.eql(u8, c1.toHex(hex_buf[0..16]), "#ff00aa"), true);
        expect_eq(std.mem.eql(u8, c2.toHex(hex_buf[0..16]), "#ff800080"), true);

        const rebecca = CssColor.parse("rebeccapurple").?;
        expect_eq(rebecca.r, 102);
        expect_eq(rebecca.g, 51);
        expect_eq(rebecca.b, 153);
    });

    test("2. cap:css — CssColor Functional RGB/RGBA, HSL/HSLA, OKLCH & WCAG Contrast Ratio", () => {
        const rgb_legacy = CssColor.parse("rgba(16, 32, 64, 0.5)").?;
        expect_eq(rgb_legacy.r, 16);
        expect_eq(rgb_legacy.g, 32);
        expect_eq(rgb_legacy.b, 64);
        expect_eq(rgb_legacy.a, 128);

        const rgb_modern = CssColor.parse("rgb(255 128 0 / 50%)").?;
        expect_eq(rgb_modern.r, 255);
        expect_eq(rgb_modern.g, 128);
        expect_eq(rgb_modern.b, 0);
        expect_eq(rgb_modern.a, 128);

        // Pure red in HSL: hsl(0, 100%, 50%)
        const hsl_red = CssColor.parse("hsl(0, 100%, 50%)").?;
        expect_eq(hsl_red.r, 255);
        expect_eq(hsl_red.g, 0);
        expect_eq(hsl_red.b, 0);

        // Pure green in HSL: hsl(120deg 100% 50%)
        const hsl_green = CssColor.parse("hsl(120deg 100% 50%)").?;
        expect_eq(hsl_green.r, 0);
        expect_eq(hsl_green.g, 255);
        expect_eq(hsl_green.b, 0);

        // OKLCH color parsing & conversion
        const oklch_col = CssColor.parse("oklch(62.8% 0.258 29.23deg)").?;
        expect_eq(oklch_col.is_oklch, true);
        expect_eq(oklch_col.r > 240, true);
        expect_eq(oklch_col.g < 30, true);
        expect_eq(oklch_col.b < 30, true);

        // WCAG contrast between black and white is 21:1
        const black = CssColor.parse("#000000").?;
        const white = CssColor.parse("#ffffff").?;
        const ratio = white.contrastRatio(black);
        expect_eq(ratio > 20.9 && ratio < 21.1, true);
    });

    test("3. cap:css — CssLength, CssUnit Conversion & CssCalc Expression Evaluation", () => {
        const len_px = CssLength.parse("24px").?;
        expect_eq(len_px.unit == CssUnit.Px, true);
        expect_eq(@as(i64, @intFromFloat(len_px.toPx(16.0, 1920.0, 1080.0))), 24);

        const len_rem = CssLength.parse("1.5rem").?;
        expect_eq(len_rem.unit == CssUnit.Rem, true);
        expect_eq(@as(i64, @intFromFloat(len_rem.toPx(16.0, 1920.0, 1080.0))), 24);

        const len_vw = CssLength.parse("50vw").?;
        expect_eq(@as(i64, @intFromFloat(len_vw.toPx(16.0, 1200.0, 800.0))), 600);

        const dur_sec = CssLength.parse("0.25s").?;
        expect_eq(@as(i64, @intFromFloat(dur_sec.toMs())), 250);

        const angle_turn = CssLength.parse("0.5turn").?;
        expect_eq(@as(i64, @intFromFloat(angle_turn.toDeg())), 180);

        // calc(100vw - 2rem) with 1200px viewport and 16px root font -> 1200 - 32 = 1168px
        const calc_px = CssCalc.evalPx("calc(100vw - 2rem)", 16.0, 1200.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(calc_px)), 1168);

        // Precedence: calc(16px + 2 * 1.5rem) -> 16 + 2 * 24 = 64px
        const calc_prec = CssCalc.evalPx("calc(16px + 2 * 1.5rem)", 16.0, 1200.0, 800.0).?;
        expect_eq(@as(i64, @intFromFloat(calc_prec)), 64);
    });

    test("4. cap:css — CssSelector W3C Specificity Calculation & DOM Element Matching", () => {
        const s1 = CssSelector.parse("div.card");
        expect_eq(s1.specificity.ids, 0);
        expect_eq(s1.specificity.classes, 1);
        expect_eq(s1.specificity.elements, 1);
        expect_eq(s1.matchesElement("div", "", "card featured"), true);
        expect_eq(s1.matchesElement("span", "", "card"), false);

        const s2 = CssSelector.parse("ul#main-nav > li.nav-item[data-active=\"true\"]:hover::after");
        expect_eq(s2.specificity.ids, 1);
        expect_eq(s2.specificity.classes, 3); // .nav-item + [data-active] + :hover
        expect_eq(s2.specificity.elements, 3); // ul + li + ::after
        expect_eq(s2.specificity.compare(s1.specificity), 1);
        expect_eq(std.mem.eql(u8, s2.tag_name, "li"), true);
        expect_eq(std.mem.eql(u8, s2.class_name, "nav-item"), true);
        expect_eq(std.mem.eql(u8, s2.attr_name, "data-active"), true);
        expect_eq(std.mem.eql(u8, s2.attr_val, "true"), true);
        expect_eq(std.mem.eql(u8, s2.pseudo_class, "hover"), true);
        expect_eq(std.mem.eql(u8, s2.pseudo_element, "after"), true);
    });

    test("5. cap:css — CssRule, @media, @keyframes, Stylesheet Parsing & Minification", () => {
        const css_source =
            "/* Header & Layout Styles */\n" ++
            "@import \"reset.css\";\n" ++
            ".btn#submit-btn {\n" ++
            "  background-color: #2563eb;\n" ++
            "  padding: 12px !important;\n" ++
            "  --btn-radius: 6px;\n" ++
            "}\n" ++
            "@media screen and (min-width: 768px) and (max-width: 1440px) {\n" ++
            "  .container { max-width: 1200px; }\n" ++
            "}\n" ++
            "@keyframes fadeIn {\n" ++
            "  from { opacity: 0; }\n" ++
            "  50% { opacity: 0.5; }\n" ++
            "  to { opacity: 1; }\n" ++
            "}\n";

        let rules: [8]CssRule = std.mem.zeroes([8]CssRule);
        const count = CssStylesheetParser.parseStylesheet(css_source, rules[0..8]);
        expect_eq(count, 4);

        // Rule 0: @import
        expect_eq(rules[0].kind == CssRuleKind.Import, true);
        expect_eq(std.mem.eql(u8, rules[0].prelude, "\"reset.css\""), true);

        // Rule 1: .btn#submit-btn style rule
        expect_eq(rules[1].kind == CssRuleKind.Style, true);
        expect_eq(rules[1].prop_count, 3);
        expect_eq(std.mem.eql(u8, rules[1].getProperty("background-color").?, "#2563eb"), true);
        expect_eq(std.mem.eql(u8, rules[1].getProperty("padding").?, "12px"), true);
        expect_eq(rules[1].properties[1].important, true);
        expect_eq(rules[1].properties[2].isCustomProperty(), true);

        let ser_buf: [256]u8 = [0; 256];
        const min_rule: []const u8 = rules[1].formatRule(ser_buf[0..256], true);
        expect_eq(
            std.mem.eql(u8, min_rule, ".btn#submit-btn{background-color:#2563eb;padding:12px!important;--btn-radius:6px}"),
            true
        );

        // Rule 2: @media rule
        expect_eq(rules[2].kind == CssRuleKind.Media, true);
        expect_eq(std.mem.eql(u8, rules[2].media.media_type, "screen"), true);
        expect_eq(rules[2].media.matches(1024.0, false), true);
        expect_eq(rules[2].media.matches(480.0, false), false);
        expect_eq(rules[2].media.matches(1600.0, false), false);

        // Rule 3: @keyframes rule
        expect_eq(rules[3].kind == CssRuleKind.Keyframes, true);
        const kf = CssKeyframes.parse("@keyframes fadeIn { from { opacity: 0; } 50% { opacity: 0.5; } to { opacity: 1; } }").?;
        expect_eq(std.mem.eql(u8, kf.name, "fadeIn"), true);
        expect_eq(kf.step_count, 3);
        expect_eq(@as(i64, @intFromFloat(kf.steps[0].offset_percent)), 0);
        expect_eq(@as(i64, @intFromFloat(kf.steps[1].offset_percent)), 50);
        expect_eq(@as(i64, @intFromFloat(kf.steps[2].offset_percent)), 100);
        expect_eq(std.mem.eql(u8, kf.steps[1].declarations[0].value, "0.5"), true);

        // Stylesheet Minification
        let min_buf: [512]u8 = [0; 512];
        const minified = CssStylesheetParser.minify("/* comment */ .card { color: red; margin: 0px; }", min_buf[0..512]);
        expect_eq(std.mem.eql(u8, minified, ".card{color:red;margin:0px}"), true);
    });
}
