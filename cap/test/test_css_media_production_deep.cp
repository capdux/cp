import { test, expect_eq } from "cap:test";
import {
    CssMediaError,
    MediaType,
    MediaQualifier,
    ColorScheme,
    ReducedMotion,
    HoverCapability,
    PointerCapability,
    MediaEnv,
    MediaFeature,
    MediaQuery,
    MediaQueryList,
} from "cap:css_media";

test("1. MediaType (all/screen/print) & MediaQualifier (only/not) with alwaysMatches/neverMatches", () => {
    const q_all = MediaQuery.parse("all") catch unreachable;
    expect_eq(q_all.alwaysMatches(), true);
    expect_eq(q_all.neverMatches(), false);

    const q_not_all = MediaQuery.parse("not all") catch unreachable;
    expect_eq(q_not_all.alwaysMatches(), false);
    expect_eq(q_not_all.neverMatches(), true);

    const q_only_scr = MediaQuery.parse("only screen") catch unreachable;
    expect_eq(q_only_scr.qualifier == MediaQualifier.only, true);
    expect_eq(q_only_scr.media_type == MediaType.screen, true);
    expect_eq(q_only_scr.evaluate(MediaEnv{ media_type: MediaType.screen }), true);
    expect_eq(q_only_scr.evaluate(MediaEnv{ media_type: MediaType.print }), false);

    const q_not_print = MediaQuery.parse("not print") catch unreachable;
    expect_eq(q_not_print.evaluate(MediaEnv{ media_type: MediaType.screen }), true);
    expect_eq(q_not_print.evaluate(MediaEnv{ media_type: MediaType.print }), false);
});

test("2. MediaFeature parsing & MediaEnv evaluation (width/orientation/color-scheme/motion/hover/pointer)", () => {
    const mobile_dark = MediaEnv{
        media_type: MediaType.screen,
        width: 390.0,
        height: 844.0,
        color_scheme: ColorScheme.dark,
        reduced_motion: ReducedMotion.reduce,
        hover: HoverCapability.none,
        pointer: PointerCapability.coarse,
    };

    expect_eq((MediaFeature.parse("(max-width: 600px)") catch unreachable).evaluate(mobile_dark), true);
    expect_eq((MediaFeature.parse("(min-width: 768px)") catch unreachable).evaluate(mobile_dark), false);
    expect_eq((MediaFeature.parse("(orientation: portrait)") catch unreachable).evaluate(mobile_dark), true);
    expect_eq((MediaFeature.parse("(prefers-color-scheme: dark)") catch unreachable).evaluate(mobile_dark), true);
    expect_eq((MediaFeature.parse("(prefers-reduced-motion: reduce)") catch unreachable).evaluate(mobile_dark), true);
    expect_eq((MediaFeature.parse("(hover: none)") catch unreachable).evaluate(mobile_dark), true);
    expect_eq((MediaFeature.parse("(pointer: coarse)") catch unreachable).evaluate(mobile_dark), true);
    expect_eq((MediaFeature.parse("(pointer: fine)") catch unreachable).evaluate(mobile_dark), false);
});

test("3. Compound MediaQuery with multiple `and` features & `not` negation", () => {
    const desktop = MediaEnv{
        media_type: MediaType.screen,
        width: 1440.0,
        height: 900.0,
        color_scheme: ColorScheme.dark,
        hover: HoverCapability.hover,
        pointer: PointerCapability.fine,
    };

    const q = MediaQuery.parse("screen and (min-width: 1024px) and (prefers-color-scheme: dark) and (hover: hover)") catch unreachable;
    expect_eq(q.feature_count, usize(3));
    expect_eq(q.evaluate(desktop), true);

    const light_desktop = MediaEnv{
        media_type: MediaType.screen,
        width: 1440.0,
        height: 900.0,
        color_scheme: ColorScheme.light,
    };
    expect_eq(q.evaluate(light_desktop), false);

    // `not` negates the entire query
    const q_neg = MediaQuery.parse("not screen and (max-width: 600px)") catch unreachable;
    expect_eq(q_neg.evaluate(desktop), true);
});

test("4. MediaQueryList comma-separated OR evaluation & canonical toCss serialization", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);

    const list = MediaQueryList.parse("@media screen and (min-width: 1200px), print") catch unreachable;
    expect_eq(list.len, usize(2));
    expect_eq(list.evaluate(MediaEnv{ media_type: MediaType.screen, width: 1280.0 }), true);
    expect_eq(list.evaluate(MediaEnv{ media_type: MediaType.screen, width: 800.0 }), false);
    expect_eq(list.evaluate(MediaEnv{ media_type: MediaType.print, width: 800.0 }), true);

    const n = list.toCss(buf[0..]) catch unreachable;
    expect_eq(std.mem.eql(u8, buf[0..n], "@media screen and (min-width: 1200px), print"), true);

    const always_list = MediaQueryList.parse("print, all") catch unreachable;
    expect_eq(always_list.alwaysMatches(), true);
});

test("5. Malformed media queries, invalid types/features & boundary guards", () => {
    std.testing.expectError(CssMediaError.InvalidMediaQuery, MediaQueryList.parse("")) catch unreachable;
    std.testing.expectError(CssMediaError.InvalidMediaQuery, MediaQueryList.parse("screen, ")) catch unreachable;
    std.testing.expectError(CssMediaError.InvalidMediaType, MediaQuery.parse("braille")) catch unreachable;
    std.testing.expectError(CssMediaError.InvalidMediaQuery, MediaQuery.parse("screen and")) catch unreachable;
    std.testing.expectError(CssMediaError.InvalidMediaFeature, MediaFeature.parse("(unknown-feat: 1)")) catch unreachable;
    std.testing.expectError(CssMediaError.InvalidFeatureValue, MediaFeature.parse("(prefers-color-scheme: sepia)")) catch unreachable;
    std.testing.expectError(CssMediaError.InvalidFeatureValue, MediaFeature.parse("(min-width: -100px)")) catch unreachable;
    std.testing.expectError(CssMediaError.InvalidFeatureValue, MediaFeature.parse("(pointer: laser)")) catch unreachable;
});
