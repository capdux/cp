// cap/test/test_phase_d_css_lightning_port.cp
// Comprehensive E2E Test Suite for Phase D-1 LightningCSS Industrial Engine (lib/css/engine/)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CssExpandedDecl,
    packBrowserVersion,
    unpackMajor,
    unpackMinor,
    Browsers,
    Feature,
    Features,
    PrefixFlags,
    PrefixFeature,
    Targets,
    Angle,
    AngleUnit,
    Time,
    Resolution,
    AlphaValue,
    Ratio,
    UrlValue,
    CssRect,
    Position,
    StepPosition,
    EasingFunction,
    Gradient,
    RectShorthand,
    BorderStyle,
    BorderShorthand,
    BorderRadius,
    BoxShadowList,
    TransformList,
    Matrix2D,
    Point2D,
    FlexShorthand,
    TransitionItem,
    AnimationItem,
    DisplayValue,
    FilterList,
    SupportsAst,
    MediaRangeQuery,
    ContainerQuery,
    LayerManager,
    FontFaceRule,
    PropertyRule,
    ScopeRule,
    CssCompileOptions,
    CssCompileStats,
    CssModulesHasher,
    CssCompiler,
} from "cap:css";

fn supportsDeclStub(prop: []const u8, val: []const u8): bool {
    * = val;
    return std.ascii.eqlIgnoreCase(prop, "display") || std.ascii.eqlIgnoreCase(prop, "color");
}

fn supportsSelStub(sel: []const u8): bool {
    return std.mem.indexOf(u8, sel, ":is(") != null;
}

fn main() {
    test("1. Targets & Browsers — Version Packing, Feature Matrix & Vendor Prefixes", () => {
        const v14_1 = packBrowserVersion(14, 1, 0);
        expect_eq(unpackMajor(v14_1), 14 as u32);
        expect_eq(unpackMinor(v14_1), 1 as u32);

        const def_targets = Targets.browserDefault(); // Chrome 87, Edge 88, Firefox 78, Safari 14
        expect_eq(def_targets.shouldCompile(Feature.Nesting), true);
        expect_eq(def_targets.shouldCompile(Feature.MediaRangeSyntax), true);
        expect_eq(def_targets.shouldCompile(Feature.LogicalProperties), true);
        expect_eq(def_targets.shouldCompile(Feature.OklabColors), true);
        expect_eq(def_targets.shouldCompile(Feature.IsSelector), true); // Chrome 87 < 88
        expect_eq(def_targets.shouldCompile(Feature.ClampFunction), false); // All >= min versions
        expect_eq(def_targets.shouldCompile(Feature.CascadeLayers), true);

        const modern = Targets.fromBrowsers(Browsers.modern()); // Chrome 120, Firefox 121, Safari 17.2, Edge 120
        expect_eq(modern.isCompatible(Feature.Nesting), true);
        expect_eq(modern.isCompatible(Feature.MediaRangeSyntax), true);
        expect_eq(modern.isCompatible(Feature.LogicalProperties), true);
        expect_eq(modern.isCompatible(Feature.OklabColors), true);
        expect_eq(modern.isCompatible(Feature.HasSelector), true);
        expect_eq(modern.isCompatible(Feature.CascadeLayers), true);
        expect_eq(modern.isCompatible(Feature.LightDark), false); // Needs Chrome 123, Safari 17.5

        // Parse esbuild-style targets
        const list: [2][]const u8 = ["es2018", "safari12"];
        const es_targets = Targets.fromList(list[0..2]);
        expect_eq(es_targets.shouldCompile(Feature.DoublePositionGradients), true); // Safari 12 < 12.1

        // Vendor prefixes
        const p_us = def_targets.prefixes(PrefixFeature.UserSelect);
        expect_eq(p_us.hasWebkit(), true); // Safari 14 <= 17
        expect_eq(p_us.hasNone(), true);

        const p_bf = def_targets.prefixes(PrefixFeature.BackdropFilter);
        expect_eq(p_bf.hasWebkit(), true);

        const pre_2014 = Targets.fromBrowsers(Browsers {
            chrome: packBrowserVersion(35, 0, 0),
            firefox: packBrowserVersion(15, 0, 0),
            safari: packBrowserVersion(8, 0, 0),
            ie: packBrowserVersion(9, 0, 0),
        });
        const p_tr_legacy = pre_2014.prefixes(PrefixFeature.Transform);
        expect_eq(p_tr_legacy.hasWebkit(), true);
        expect_eq(p_tr_legacy.hasMoz(), true);
        expect_eq(p_tr_legacy.hasMs(), true);

        // Include / exclude overrides
        let custom = Targets.fromBrowsers(Browsers.modern());
        custom.include.insert(Feature.Nesting);
        custom.exclude.insert(Feature.LightDark);
        expect_eq(custom.shouldCompile(Feature.Nesting), true);
        expect_eq(custom.shouldCompile(Feature.LightDark), false);
    });

    test("2. Values — Angle, Time, Resolution, Ratio, Alpha, Url, Position & CssRect", () => {
        const a_turn = Angle.parse("0.25turn").?;
        expect_eq(@as(i64, @intFromFloat(@round(a_turn.toDegrees()))), 90);
        const a_grad = Angle.parse("100grad").?;
        expect_eq(a_turn.eql(a_grad), true);
        const a_neg = Angle.deg(-90.0);
        expect_eq(@as(i64, @intFromFloat(@round(a_neg.normalizeDeg()))), 270);

        let buf: [128]u8 = [0; 128];
        const a_rad = Angle.rad(3.141592653589793);
        expect_eq(a_rad.toDegrees() > 179.9 && a_rad.toDegrees() < 180.1, true);
        expect_eq(a_rad.format(buf[0..128], true).len > 0, true);

        // Time canonical shortest formatting
        const t100 = Time.parse("100ms").?;
        expect_eq(std.mem.eql(u8, t100.format(buf[0..128], true), ".1s"), true);
        const t50 = Time.parse("0.05s").?;
        expect_eq(std.mem.eql(u8, t50.format(buf[0..128], true), "50ms"), true);

        // Resolution
        const r2x = Resolution.parse("2x").?;
        expect_eq(@as(i64, @intFromFloat(@round(r2x.toDpi()))), 192);
        expect_eq(std.mem.eql(u8, r2x.format(buf[0..128], true), "2x"), true);
        expect_eq(std.mem.eql(u8, r2x.format(buf[0..128], false), "2dppx"), true);

        // Ratio, Alpha, Url, CssRect
        const rat = Ratio.parse("16 / 9").?;
        expect_eq(rat.value() > 1.77 && rat.value() < 1.78, true);
        const alp = AlphaValue.parse("50%").?;
        expect_eq(std.mem.eql(u8, alp.format(buf[0..128]), "0.5"), true);
        const uv = UrlValue.parse("url(\"assets/icon.svg\")").?;
        expect_eq(std.mem.eql(u8, uv.format(buf[0..128], true), "url(assets/icon.svg)"), true);

        const rect = CssRect.parse("10px 20px 10px 20px").?;
        expect_eq(std.mem.eql(u8, rect.format(buf[0..128]), "10px 20px"), true);

        // Position 1/2/4 value forms
        const p_center = Position.parse("center").?;
        expect_eq(std.mem.eql(u8, p_center.format(buf[0..128], true), "50%"), true);
        const p_4 = Position.parse("right 10px bottom 20px").?;
        expect_eq(std.mem.eql(u8, p_4.format(buf[0..128], false), "right 10px bottom 20px"), true);
    });

    test("3. Values — EasingFunction (CubicBezier Solver, Steps, Validation) & Gradients", () => {
        let buf: [256]u8 = [0; 256];

        // Canonicalization of cubic-bezier to keywords
        const cb_ease = EasingFunction.parse("cubic-bezier(0.25, 0.1, 0.25, 1)").?;
        expect_eq(std.mem.eql(u8, cb_ease.format(buf[0..256], true), "ease"), true);
        const cb_inout = EasingFunction.parse("cubic-bezier(0.42, 0, 0.58, 1)").?;
        expect_eq(std.mem.eql(u8, cb_inout.format(buf[0..256], true), "ease-in-out"), true);

        // Numerical evaluation check
        const mid = cb_inout.eval(0.5);
        expect_eq(mid > 0.49 && mid < 0.51, true);
        expect_eq(cb_ease.eval(0.0) == 0.0, true);
        expect_eq(cb_ease.eval(1.0) == 1.0, true);

        // Validation rejects out-of-range x1/x2 and invalid jump-none
        expect_eq(EasingFunction.parse("cubic-bezier(1.5, 0, 0.5, 1)") == null, true);
        expect_eq(EasingFunction.parse("steps(1, jump-none)") == null, true);

        const st = EasingFunction.parse("steps(4, jump-start)").?;
        expect_eq(std.mem.eql(u8, st.format(buf[0..256], true), "steps(4,start)"), true);
        expect_eq(st.eval(0.1) > 0.24 && st.eval(0.1) < 0.26, true);

        // Gradients & double-position stop lowering
        const grad = Gradient.parse("linear-gradient(to bottom, red 20% 50%, #00ff00)").?;
        expect_eq(grad.hasDoublePositionStops(), true);
        const lowered = grad.format(buf[0..256], true, true);
        expect_eq(std.mem.eql(u8, lowered, "linear-gradient(red 20%,red 50%,#00ff00)"), true);

        const conic = Gradient.parse("conic-gradient(from 90deg at 50% 50%, red, blue)").?;
        expect_eq(conic.stop_count, 2 as usize);
    });

    test("4. Properties — Shorthand Expander & Minifier (Margin/Border/Radius/BoxShadow/Transform/Flex/Display/Filter)", () => {
        let buf: [256]u8 = [0; 256];
        let decls: [4]CssExpandedDecl = std.mem.zeroes([4]CssExpandedDecl);

        // RectShorthand expansion & minification
        const n_margin = RectShorthand.expand("margin", "10px 20px 30px", false, decls[0..4]);
        expect_eq(n_margin, 4 as usize);
        expect_eq(std.mem.eql(u8, decls[0].name, "margin-top"), true);
        expect_eq(std.mem.eql(u8, decls[0].value, "10px"), true);
        expect_eq(std.mem.eql(u8, decls[3].name, "margin-left"), true);
        expect_eq(std.mem.eql(u8, decls[3].value, "20px"), true);
        expect_eq(std.mem.eql(u8, RectShorthand.minifyFour("8px", "16px", "8px", "16px", buf[0..256]), "8px 16px"), true);

        // BorderShorthand
        const bdr = BorderShorthand.parse("solid 2px #333").?;
        expect_eq(bdr.style == BorderStyle.Solid, true);
        expect_eq(std.mem.eql(u8, bdr.width, "2px"), true);
        expect_eq(std.mem.eql(u8, bdr.color, "#333"), true);
        expect_eq(bdr.expandToLonghands("top", true, decls[0..4]), 3 as usize);
        expect_eq(std.mem.eql(u8, decls[1].name, "border-top-style"), true);
        expect_eq(decls[1].important, true);

        // BorderRadius (elliptical & collapsed)
        const br_ell = BorderRadius.parse("10px 20px 10px 20px / 5px 5px 5px 5px").?;
        expect_eq(br_ell.isElliptical(), true);
        expect_eq(std.mem.eql(u8, br_ell.format(buf[0..256], true), "10px 20px/5px"), true);
        const br_eq = BorderRadius.parse("8px 8px 8px 8px / 8px 8px 8px 8px").?;
        expect_eq(br_eq.isElliptical(), false);
        expect_eq(std.mem.eql(u8, br_eq.format(buf[0..256], true), "8px"), true);

        // BoxShadowList
        const bsl = BoxShadowList.parse("inset 0 2px 4px 0 rgba(0, 0, 0, 0.2), 0 4px 8px #000").?;
        expect_eq(bsl.count, 2 as usize);
        expect_eq(bsl.items[0].inset, true);
        expect_eq(std.mem.eql(u8, bsl.format(buf[0..256], true), "inset 0 2px 4px rgba(0, 0, 0, 0.2),0 4px 8px #000"), true);

        // TransformList & Matrix2D composition
        const tf = TransformList.parse("translate(10px, 20px) rotate(90deg) scale(2)").?;
        const mat = tf.toMatrix2D().?;
        const pt: Point2D = mat.applyPoint(1.0, 0.0);
        // translate(10, 20) * rotate(90deg) * scale(2): (1,0) -> (2,0) -> (0,2) -> (10, 22)
        expect_eq(@as(i64, @intFromFloat(@round(pt.x))), 10);
        expect_eq(@as(i64, @intFromFloat(@round(pt.y))), 22);

        // FlexShorthand
        const fl1 = FlexShorthand.parse("1 1 0%").?;
        expect_eq(std.mem.eql(u8, fl1.format(buf[0..256], true), "1"), true);
        const fl_auto = FlexShorthand.parse("1 1 auto").?;
        expect_eq(std.mem.eql(u8, fl_auto.format(buf[0..256], true), "auto"), true);

        // Transition, Animation, Display, FilterList
        const tr = TransitionItem.parse("0.2s cubic-bezier(0.25, 0.1, 0.25, 1) 50ms opacity").?;
        expect_eq(std.mem.eql(u8, tr.format(buf[0..256], true), "opacity .2s 50ms"), true);

        const anim = AnimationItem.parse("fadeIn 100ms ease-out infinite alternate forwards").?;
        expect_eq(std.mem.eql(u8, anim.format(buf[0..256], true), ".1s ease-out infinite alternate forwards fadeIn"), true);

        const disp = DisplayValue.parse("inline flow-root").?;
        expect_eq(std.mem.eql(u8, disp.format(buf[0..256]), "inline-block"), true);

        const flt = FilterList.parse("blur(4px) brightness(120%)").?;
        expect_eq(flt.count, 2 as usize);
        expect_eq(std.mem.eql(u8, flt.format(buf[0..256], true), "blur(4px)brightness(120%)"), true);
    });

    test("5. Rules — @supports AST, MediaRange L4 Lowering, @container, @layer, @font-face, @property & @scope", () => {
        let buf: [256]u8 = [0; 256];

        // @supports AST & evaluation
        const sup = SupportsAst.parse("@supports (display: grid) and (not (selector(:has(*))))").?;
        expect_eq(sup.eval(supportsDeclStub, supportsSelStub), true);
        const sup_false = SupportsAst.parse("(display: grid) and (selector(:has(*)))").?;
        expect_eq(sup_false.eval(supportsDeclStub, supportsSelStub), false);

        // Media Queries Level 4 range & interval syntax lowering
        const mrq = MediaRangeQuery.parse("(400px <= width < 900px)").?;
        expect_eq(mrq.matches(400.0, 800.0), true);
        expect_eq(mrq.matches(899.0, 800.0), true);
        expect_eq(mrq.matches(900.0, 800.0), false);
        expect_eq(mrq.matches(399.0, 800.0), false);
        expect_eq(std.mem.eql(u8, mrq.lowerToLegacy(buf[0..256]), "(min-width: 400px) and (max-width: 899.98px)"), true);

        // @container
        const cq = ContainerQuery.parse("@container card (300px <= width < 700px)").?;
        expect_eq(cq.matches("card", 500.0, 400.0), true);
        expect_eq(cq.matches("other", 500.0, 400.0), false);
        expect_eq(cq.matches("card", 700.0, 400.0), false);
        expect_eq(std.mem.eql(u8, cq.format(buf[0..256], true), "@container card (min-width: 300px) and (max-width: 699.98px)"), true);

        // @layer priority manager
        let lm = LayerManager.init();
        expect_eq(lm.registerStatement("@layer base, components, utilities;"), 3 as usize);
        expect_eq(lm.compareWeight("base", "utilities"), -1);
        expect_eq(lm.compareWeight("utilities", "components"), 1);
        expect_eq(lm.compareWeight("", "utilities"), 1); // Unlayered CSS beats all layers

        // @font-face, @property, @scope
        const ff = FontFaceRule.parse("@font-face { font-family: \"Inter\"; src: url(inter.woff2); font-weight: 400 700; font-display: swap; }").?;
        expect_eq(std.mem.eql(u8, ff.font_family, "\"Inter\""), true);
        expect_eq(ff.format(buf[0..256], true).len > 20, true);

        const prop = PropertyRule.parse("@property --accent { syntax: \"<color>\"; inherits: true; initial-value: #2563eb; }").?;
        expect_eq(prop.inherits, true);
        expect_eq(PropertyRule.parse("@property --bad { syntax: \"<length>\"; inherits: false; }") == null, true);

        const sc = ScopeRule.parse("@scope (.card) to (.content) { p { color: red; } }").?;
        expect_eq(std.mem.eql(u8, sc.scope_start, ".card"), true);
        expect_eq(std.mem.eql(u8, sc.scope_end, ".content"), true);
    });

    test("6. CssCompiler — Full Pipeline (Nesting + Media Ranges + Logical + Prefixes + Shorthands + CSS Modules + Unused Pruning)", () => {
        const input_css =
            "/* Component CSS */\n" ++
            ".dead-unused { color: red; }\n" ++
            ".card {\n" ++
            "  margin: 10px 20px 10px 20px;\n" ++
            "  margin-inline: 8px 16px;\n" ++
            "  user-select: none;\n" ++
            "  color: #ffffff;\n" ++
            "  flex: 1 1 0%;\n" ++
            "  &:hover { border-radius: 4px 4px 4px 4px; }\n" ++
            "}\n" ++
            "@media (400px <= width < 900px) {\n" ++
            "  .card :global(.active) { display: block flow; }\n" ++
            "}\n" ++
            "@keyframes pulse { from { opacity: 0px; } to { opacity: 1; } }\n";

        let opts = CssCompileOptions {
            targets: Targets.fromBrowsers(Browsers {
                chrome: packBrowserVersion(35, 0, 0),
                firefox: packBrowserVersion(45, 0, 0),
                safari: packBrowserVersion(8, 0, 0),
                ie: packBrowserVersion(10, 0, 0),
            }),
            minify: true,
            css_modules: true,
            module_hash_seed: "card.module.css",
        };
        opts.addUnusedSymbol("dead-unused");

        let out_buf: [8192]u8 = [0; 8192];
        let stats = CssCompileStats {};
        const out = CssCompiler.compile(input_css, opts, out_buf[0..8192], &stats);

        // Verify unused rule pruned
        expect_eq(stats.unused_rules_dropped >= 1, true);
        expect_eq(std.mem.indexOf(u8, out, "dead-unused") == null, true);

        // Verify nesting flattened
        expect_eq(std.mem.indexOf(u8, out, ":hover") != null, true);

        // Verify logical margin-inline expanded to margin-left / margin-right
        expect_eq(std.mem.indexOf(u8, out, "margin-left:8px") != null, true);
        expect_eq(std.mem.indexOf(u8, out, "margin-right:16px") != null, true);

        // Verify vendor prefixes injected for legacy target
        expect_eq(std.mem.indexOf(u8, out, "-webkit-user-select:none") != null, true);
        expect_eq(std.mem.indexOf(u8, out, "-moz-user-select:none") != null, true);
        expect_eq(std.mem.indexOf(u8, out, "-ms-user-select:none") != null, true);
        expect_eq(std.mem.indexOf(u8, out, "@-webkit-keyframes pulse") != null, true);

        // Verify shorthand & value minification
        expect_eq(std.mem.indexOf(u8, out, "margin:10px 20px") != null, true);
        expect_eq(std.mem.indexOf(u8, out, "color:#fff") != null, true);
        expect_eq(std.mem.indexOf(u8, out, "flex:1") != null, true);
        expect_eq(std.mem.indexOf(u8, out, "border-radius:4px") != null, true);
        expect_eq(std.mem.indexOf(u8, out, "display:block") != null, true);

        // Verify Media Queries L4 range lowered
        expect_eq(std.mem.indexOf(u8, out, "(min-width: 400px) and (max-width: 899.98px)") != null, true);

        // Verify CSS Modules scoping (.card -> .card_<hash>, :global(.active) -> .active)
        let scoped_card: [64]u8 = [0; 64];
        const expected_ident = CssModulesHasher.scopeIdent("card", "card.module.css", scoped_card[0..64]);
        expect_eq(std.mem.indexOf(u8, out, expected_ident) != null, true);
        expect_eq(std.mem.indexOf(u8, out, ".active") != null, true);
        expect_eq(stats.prefixes_injected > 0, true);
        expect_eq(stats.logical_expanded > 0, true);
        expect_eq(stats.media_ranges_lowered > 0, true);
    });
}
