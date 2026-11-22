// Source: cp/cap/test/test_mediaquery_production_deep.cp (Production deep tests for cap:mediaquery)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    MediaType,
    Qualifier,
    MediaComparison,
    ColorScheme,
    ReducedMotion,
    Orientation,
    HoverKind,
    PointerKind,
    MediaEnv,
    parseLengthPx,
    parseResolutionDppx,
    parseRatio,
    evalMediaQueryList,
    parseMediaQuery,
    canonicalizeMediaQuery,
} from "cap:mediaquery";

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

test("1. Unit parsers: length (px/em/rem/in/cm/pt), resolution (dppx/x/dpi/dpcm) & ratio", () => {
    if (v := parseLengthPx("768px", 16.0)) {
        expect_eq(absF64(v - 768.0) < 1.0e-9, true);
    } else { expect_eq(true, false); }

    if (v := parseLengthPx("48em", 16.0)) {
        expect_eq(absF64(v - 768.0) < 1.0e-9, true);
    } else { expect_eq(true, false); }

    if (v := parseLengthPx("1in", 16.0)) {
        expect_eq(absF64(v - 96.0) < 1.0e-9, true);
    } else { expect_eq(true, false); }

    if (v := parseLengthPx("0", 16.0)) {
        expect_eq(v, 0.0);
    } else { expect_eq(true, false); }

    if (r := parseResolutionDppx("192dpi")) {
        expect_eq(absF64(r - 2.0) < 1.0e-9, true);
    } else { expect_eq(true, false); }

    if (r := parseResolutionDppx("3x")) {
        expect_eq(absF64(r - 3.0) < 1.0e-9, true);
    } else { expect_eq(true, false); }

    if (rat := parseRatio("16 / 9")) {
        expect_eq(absF64(rat - (16.0 / 9.0)) < 1.0e-9, true);
    } else { expect_eq(true, false); }

    // Invalid units / zero denominator
    expect_eq(parseLengthPx("100", 16.0) == null, true);
    expect_eq(parseRatio("16/0") == null, true);
    expect_eq(parseResolutionDppx("-2dppx") == null, true);
});

test("2. Traditional min-/max- queries & MQ4 single/double-sided range syntax", () => {
    const desk = MediaEnv.desktop(); // 1440x900, 1dppx
    const mob = MediaEnv.mobile();   // 390x844, 3dppx

    expect_eq(evalMediaQueryList("(min-width: 1024px) and (max-width: 1920px)", desk), true);
    expect_eq(evalMediaQueryList("(min-width: 1024px) and (max-width: 1920px)", mob), false);

    // MQ4 range syntax (single & double-sided)
    expect_eq(evalMediaQueryList("(width >= 1440px)", desk), true);
    expect_eq(evalMediaQueryList("(width > 1440px)", desk), false);
    expect_eq(evalMediaQueryList("(768px <= width < 1600px)", desk), true);
    expect_eq(evalMediaQueryList("(1600px > width >= 768px)", desk), true);
    expect_eq(evalMediaQueryList("(300px <= width <= 450px)", mob), true);

    // Aspect ratio & resolution ranges
    expect_eq(evalMediaQueryList("(aspect-ratio = 16/10)", desk), true);
    expect_eq(evalMediaQueryList("(resolution >= 192dpi)", mob), true);
    expect_eq(evalMediaQueryList("(-webkit-min-device-pixel-ratio: 2)", mob), true);
    expect_eq(evalMediaQueryList("(-webkit-min-device-pixel-ratio: 2)", desk), false);
});

test("3. Media types, qualifiers (only/not), comma OR lists & nested boolean logic", () => {
    const desk = MediaEnv.desktop();
    let printer = MediaEnv.desktop();
    printer.media_type = MediaType.print;

    expect_eq(evalMediaQueryList("", desk), true); // Empty list matches all per W3C MQ4
    expect_eq(evalMediaQueryList("all", desk), true);
    expect_eq(evalMediaQueryList("only screen and (min-width: 800px)", desk), true);
    expect_eq(evalMediaQueryList("only print", desk), false);
    expect_eq(evalMediaQueryList("only print", printer), true);

    // `not` negates the entire media query
    expect_eq(evalMediaQueryList("not print", desk), true);
    expect_eq(evalMediaQueryList("not screen and (min-width: 2000px)", desk), true);
    expect_eq(evalMediaQueryList("not screen and (min-width: 1000px)", desk), false);

    // Comma list (OR semantics)
    expect_eq(evalMediaQueryList("print, (width >= 1200px)", desk), true);
    expect_eq(evalMediaQueryList("print, (width <= 500px)", desk), false);

    // Nested `or` + `not (...)`
    expect_eq(evalMediaQueryList("((width < 500px) or (width >= 1200px)) and (not (orientation: portrait))", desk), true);
    expect_eq(evalMediaQueryList("not (width < 500px)", desk), true);
});

test("4. Modern user preference & pointer features (dark mode, reduced motion, hover, pointer, boolean context)", () => {
    let env = MediaEnv.desktop();
    env.color_scheme = ColorScheme.dark;
    env.reduced_motion = ReducedMotion.reduce;

    expect_eq(evalMediaQueryList("(prefers-color-scheme: dark)", env), true);
    expect_eq(evalMediaQueryList("(prefers-color-scheme: light)", env), false);
    expect_eq(evalMediaQueryList("(prefers-reduced-motion: reduce)", env), true);
    expect_eq(evalMediaQueryList("(orientation: landscape)", env), true);
    expect_eq(evalMediaQueryList("(hover: hover) and (pointer: fine)", env), true);
    expect_eq(evalMediaQueryList("(color) and (hover)", env), true);

    const mob = MediaEnv.mobile();
    expect_eq(mob.orientation(), Orientation.portrait);
    expect_eq(evalMediaQueryList("(orientation: portrait) and (hover: none) and (pointer: coarse)", mob), true);
    expect_eq(evalMediaQueryList("(hover)", mob), false);
});

test("5. parseMediaQuery summary, alwaysMatches & canonicalizeMediaQuery", () => {
    if (s1 := parseMediaQuery("all")) {
        expect_eq(s1.alwaysMatches(), true);
        expect_eq(s1.feature_count, 0);
    } else { expect_eq(true, false); }

    if (s2 := parseMediaQuery("only screen and (min-width: 768px) and (prefers-color-scheme: dark)")) {
        expect_eq(s2.alwaysMatches(), false);
        expect_eq(s2.qualifier, Qualifier.only);
        expect_eq(s2.media_type, MediaType.screen);
        expect_eq(s2.has_condition, true);
        expect_eq(s2.feature_count, 2);
    } else { expect_eq(true, false); }

    let out_buf: [128]u8 = [0; 128];
    let out_s: []u8 = out_buf[0..];
    if (canon := canonicalizeMediaQuery("  SCREEN   AND  (Min-Width:768px) ,  PRINT  ", *out_s)) {
        expect_eq(std.mem.eql(u8, canon, "screen and (min-width: 768px), print"), true);
    } else { expect_eq(true, false); }

    if (canon_empty := canonicalizeMediaQuery("   ", *out_s)) {
        expect_eq(std.mem.eql(u8, canon_empty, "all"), true);
    } else { expect_eq(true, false); }
});

test("6. Boundary defense: mixed and/or, contradictory double range, unbalanced parens, bad env & short buffer", () => {
    const desk = MediaEnv.desktop();

    // Mixing `and` and `or` at same level is forbidden by W3C MQ4
    expect_eq(evalMediaQueryList("(width >= 600px) and (color) or (hover)", desk) == null, true);

    // Contradictory double-sided range directions
    expect_eq(evalMediaQueryList("(400px < width > 200px)", desk) == null, true);
    expect_eq(evalMediaQueryList("(400px = width = 800px)", desk) == null, true);

    // Unbalanced parentheses & empty feature parens
    expect_eq(evalMediaQueryList("(min-width: 600px", desk) == null, true);
    expect_eq(evalMediaQueryList("min-width: 600px)", desk) == null, true);
    expect_eq(evalMediaQueryList("()", desk) == null, true);
    expect_eq(evalMediaQueryList("screen and (min-width: )", desk) == null, true);

    // Invalid MediaEnv (NaN width / non-positive dppx)
    let bad_env = MediaEnv.desktop();
    bad_env.width_px = std.math.nan(f64);
    expect_eq(evalMediaQueryList("screen", bad_env) == null, true);

    let bad_env2 = MediaEnv.desktop();
    bad_env2.dppx = 0.0;
    expect_eq(evalMediaQueryList("screen", bad_env2) == null, true);

    // Short output buffer in canonicalizeMediaQuery
    let tiny_buf: [4]u8 = [0; 4];
    let tiny: []u8 = tiny_buf[0..];
    expect_eq(canonicalizeMediaQuery("screen and (min-width: 768px)", *tiny) == null, true);
});
