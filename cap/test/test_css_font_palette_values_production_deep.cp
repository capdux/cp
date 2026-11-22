// Production Deep Tests for cap:css @font-palette-values (lib/css/font_palette_values.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_PALETTE_OVERRIDES,
    CssFontPaletteError,
    BasePaletteKind,
    BasePalette,
    PaletteOverrideColor,
    CssFontPaletteValuesRule,
} from "cap:css";

test("css font_palette_values: full rule parse, lookup, pretty & minified roundtrip", fn() {
    const rule = CssFontPaletteValuesRule.parse(
        "@font-palette-values --brand-spice { font-family: \"Bungee Spice\"; base-palette: 1; override-colors: 0 #ff6b6b, 1 rgb(25, 113, 194), 2 gold; }"
    ) catch unreachable;

    expect_eq(rule.name, "--brand-spice");
    expect_eq(rule.font_family ?? "", "\"Bungee Spice\"");
    if (bp := rule.base_palette) {
        expect_eq(bp.kind == .index, true);
        expect_eq(bp.index, 1);
    } else {
        unreachable;
    }
    expect_eq(rule.override_len, 3);
    expect_eq(rule.findOverride(0) ?? "", "#ff6b6b");
    expect_eq(rule.findOverride(1) ?? "", "rgb(25, 113, 194)");
    expect_eq(rule.findOverride(2) ?? "", "gold");
    expect_eq(rule.findOverride(99) == null, true);

    let pretty_buf: [256]u8 = undefined;
    const pretty = rule.toCss(pretty_buf[0..]) catch unreachable;
    const expected_pretty =
        "@font-palette-values --brand-spice {\n  font-family: \"Bungee Spice\";\n  base-palette: 1;\n  override-colors: 0 #ff6b6b, 1 rgb(25, 113, 194), 2 gold;\n}";
    expect_eq(pretty, expected_pretty);

    let min_buf: [256]u8 = undefined;
    const minified = rule.toCssMinified(min_buf[0..]) catch unreachable;
    expect_eq(
        minified,
        "@font-palette-values --brand-spice{font-family:\"Bungee Spice\";base-palette:1;override-colors:0 #ff6b6b,1 rgb(25, 113, 194),2 gold}"
    );

    const reparsed = CssFontPaletteValuesRule.parse(minified) catch unreachable;
    expect_eq(rule.eql(reparsed), true);
});

test("css font_palette_values: light/dark base-palette, comments, and duplicate override last-wins", fn() {
    const dark_rule = CssFontPaletteValuesRule.parse(
        "/* dark theme */ @FONT-PALETTE-VALUES --dark-emoji { FONT-FAMILY: Rocher; BASE-PALETTE: DARK; OVERRIDE-COLORS: 0 #111, 0 #222; }"
    ) catch unreachable;

    expect_eq(dark_rule.name, "--dark-emoji");
    expect_eq(dark_rule.font_family ?? "", "Rocher");
    if (bp := dark_rule.base_palette) {
        expect_eq(bp.kind == .dark, true);
    } else {
        unreachable;
    }
    // Same index repeated: last-wins lookup
    expect_eq(dark_rule.findOverride(0) ?? "", "#222");

    const light_bp = BasePalette.parse("light") catch unreachable;
    expect_eq(light_bp.kind == .light, true);
    let bp_buf: [16]u8 = undefined;
    expect_eq(light_bp.toCss(bp_buf[0..]) catch unreachable, "light");

    // Quoted generic name like "serif" is a custom family name, allowed by spec
    const quoted_ok = CssFontPaletteValuesRule.parse(
        "@font-palette-values --custom { font-family: \"serif\"; base-palette: light; }"
    ) catch unreachable;
    expect_eq(quoted_ok.font_family ?? "", "\"serif\"");
});

test("css font_palette_values: rejects generic font-family & currentColor per W3C CSS Fonts 4", fn() {
    // Unquoted generic font-family must fail
    let gen_serif = false;
    * = CssFontPaletteValuesRule.parse(
        "@font-palette-values --bad { font-family: serif; }"
    ) catch { gen_serif = true; };
    expect_eq(gen_serif, true);

    let gen_sans = false;
    * = CssFontPaletteValuesRule.parse(
        "@font-palette-values --bad { font-family: sans-serif; }"
    ) catch { gen_sans = true; };
    expect_eq(gen_sans, true);

    let gen_mono = false;
    * = CssFontPaletteValuesRule.parse(
        "@font-palette-values --bad { font-family: monospace; }"
    ) catch { gen_mono = true; };
    expect_eq(gen_mono, true);

    // `currentColor` is explicitly forbidden in `override-colors`
    let cur_color = false;
    * = CssFontPaletteValuesRule.parse(
        "@font-palette-values --bad { override-colors: 0 currentColor; }"
    ) catch { cur_color = true; };
    expect_eq(cur_color, true);
});

test("css font_palette_values: rejects negative indices, invalid base-palette, and bad override-colors", fn() {
    // Negative base-palette index
    let neg_bp = false;
    * = BasePalette.parse("-1") catch { neg_bp = true; };
    expect_eq(neg_bp, true);

    // Non-keyword base-palette
    let bad_bp = false;
    * = BasePalette.parse("auto") catch { bad_bp = true; };
    expect_eq(bad_bp, true);

    // Negative override-colors index
    let neg_ov = false;
    * = PaletteOverrideColor.parse("-2 red") catch { neg_ov = true; };
    expect_eq(neg_ov, true);

    // Missing color in override-colors
    let no_color = false;
    * = PaletteOverrideColor.parse("0") catch { no_color = true; };
    expect_eq(no_color, true);

    // Trailing comma in override-colors
    let trail_comma = false;
    * = CssFontPaletteValuesRule.parse(
        "@font-palette-values --p { override-colors: 0 red, ; }"
    ) catch { trail_comma = true; };
    expect_eq(trail_comma, true);
});

test("css font_palette_values: malformed header, non-dashed ident, unknown descriptor & unclosed block", fn() {
    let empty_err = false;
    * = CssFontPaletteValuesRule.parse("   ") catch { empty_err = true; };
    expect_eq(empty_err, true);

    let wrong_at = false;
    * = CssFontPaletteValuesRule.parse("@font-face --x { font-family: Foo; }") catch { wrong_at = true; };
    expect_eq(wrong_at, true);

    let bad_name = false;
    * = CssFontPaletteValuesRule.parse("@font-palette-values brand { font-family: Foo; }") catch { bad_name = true; };
    expect_eq(bad_name, true);

    let unknown_desc = false;
    * = CssFontPaletteValuesRule.parse("@font-palette-values --x { color: red; }") catch { unknown_desc = true; };
    expect_eq(unknown_desc, true);

    let unclosed = false;
    * = CssFontPaletteValuesRule.parse("@font-palette-values --x { base-palette: dark;") catch { unclosed = true; };
    expect_eq(unclosed, true);
});

test("css font_palette_values: buffer-too-small guard & empty block serialization", fn() {
    expect_eq(MAX_PALETTE_OVERRIDES, 32);

    const empty_rule = CssFontPaletteValuesRule.parse("@font-palette-values --empty { }") catch unreachable;
    let min_buf: [64]u8 = undefined;
    const min_out = empty_rule.toCssMinified(min_buf[0..]) catch unreachable;
    expect_eq(min_out, "@font-palette-values --empty{}");

    let tiny: [8]u8 = undefined;
    let pretty_oob = false;
    * = empty_rule.toCss(tiny[0..]) catch { pretty_oob = true; };
    expect_eq(pretty_oob, true);

    let min_oob = false;
    * = empty_rule.toCssMinified(tiny[0..]) catch { min_oob = true; };
    expect_eq(min_oob, true);
});
