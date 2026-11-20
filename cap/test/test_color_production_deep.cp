// Source: cp/cap/test/test_color_production_deep.cp
// Deep Production Verification Suite for cap:color & cap.color

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ColorError,
    Rgba,
    Hsl,
    Hsv,
    Oklab,
    Oklch,
    rgbToHsl,
    hslToRgb,
    rgbToHsv,
    hsvToRgb,
    rgbToOklab,
    oklabToRgb,
    oklabToOklch,
    oklchToOklab,
    mixOklab,
    rotateHue,
    lighten,
    darken,
    saturate,
    invert,
    Wcag,
    Palette,
    AnsiColorCodec,
} from "cap:color";

const EPS_F64: f64 = 0.0001;

fn approxF64(a: f64, b: f64, eps: f64): bool {
    return @abs(a - b) <= eps;
}

fn strEql(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn testHexAndHslHsvRoundTrip(): !void {
    // 1. #RGB, #RGBA, #RRGGBB, #RRGGBBAA parsing
    const c3 = Rgba.parseHex("#f80")?;
    expect_eq(c3.eql(Rgba.rgba(255, 136, 0, 255)), true);

    const c4 = Rgba.parseHex("#0af8")?;
    expect_eq(c4.eql(Rgba.rgba(0, 170, 255, 136)), true);

    const c6 = Rgba.parseHex("#3B82F6")?;
    expect_eq(c6.eql(Rgba.rgba(59, 130, 246, 255)), true);

    const c8 = Rgba.parseHex("  #10203080  ")?;
    expect_eq(c8.eql(Rgba.rgba(16, 32, 48, 128)), true);

    // Invalid hex rejection
    let got_invalid = false;
    * = Rgba.parseHex("#12345") catch |e| {
        if (e == ColorError.InvalidHex) got_invalid = true;
    };
    expect_eq(got_invalid, true);

    let got_missing_hash = false;
    * = Rgba.parseHex("ff0000") catch |e| {
        if (e == ColorError.InvalidHex) got_missing_hash = true;
    };
    expect_eq(got_missing_hash, true);

    // 2. toHex & toHexAlpha formatting + OutOfMemory guard
    let small_buf: [6]u8 = undefined;
    let got_oom = false;
    * = c6.toHex(&small_buf) catch |e| {
        if (e == ColorError.OutOfMemory) got_oom = true;
    };
    expect_eq(got_oom, true);

    let hex_buf: [16]u8 = undefined;
    const s6 = c6.toHex(&hex_buf)?;
    expect_eq(strEql(s6, "#3b82f6"), true);

    const s8 = c8.toHexAlpha(&hex_buf)?;
    expect_eq(strEql(s8, "#10203080"), true);

    // 3. sRGB <-> HSL & HSV round-trip across diverse colors
    const samples: [6]Rgba = [
        Rgba.rgb(0, 0, 0),
        Rgba.rgb(255, 255, 255),
        Rgba.rgb(255, 0, 0),
        Rgba.rgb(0, 255, 128),
        Rgba.rgba(59, 130, 246, 200),
        Rgba.rgba(180, 95, 210, 77),
    ];
    for (c of samples) {
        const hsl = rgbToHsl(c);
        const back_hsl = hslToRgb(hsl);
        expect_eq(back_hsl.eql(c), true);

        const hsv = rgbToHsv(c);
        const back_hsv = hsvToRgb(hsv);
        expect_eq(back_hsv.eql(c), true);
    }

    // 4. Hue rotation, lighten, darken, saturate, invert
    const pure_red = Rgba.rgb(255, 0, 0);
    expect_eq(rotateHue(pure_red, 120.0).eql(Rgba.rgb(0, 255, 0)), true);
    expect_eq(rotateHue(pure_red, 240.0).eql(Rgba.rgb(0, 0, 255)), true);
    expect_eq(invert(Rgba.rgba(10, 20, 30, 99)).eql(Rgba.rgba(245, 235, 225, 99)), true);

    const mid_gray = Rgba.rgb(128, 128, 128);
    expect_eq(lighten(mid_gray, 0.2).r > mid_gray.r, true);
    expect_eq(darken(mid_gray, 0.2).r < mid_gray.r, true);

    const muted = Rgba.rgb(160, 120, 120);
    const vivid = saturate(muted, 0.3);
    expect_eq(vivid.toHsl().s > muted.toHsl().s, true);
}

fn testOklabOklchAndMixing(): !void {
    // 1. Anchors: Black (0, 0, 0) & White (1, 0, 0)
    const black_lab = rgbToOklab(Rgba.rgb(0, 0, 0));
    expect_eq(approxF64(black_lab.l, 0.0, EPS_F64), true);
    expect_eq(approxF64(black_lab.a, 0.0, EPS_F64), true);
    expect_eq(approxF64(black_lab.b, 0.0, EPS_F64), true);

    const white_lab = rgbToOklab(Rgba.rgb(255, 255, 255));
    expect_eq(approxF64(white_lab.l, 1.0, EPS_F64), true);
    expect_eq(approxF64(white_lab.a, 0.0, EPS_F64), true);
    expect_eq(approxF64(white_lab.b, 0.0, EPS_F64), true);

    // 2. Round-trip sRGB -> OKLab -> OKLCH -> OKLab -> sRGB
    const palette: [5]Rgba = [
        Rgba.rgb(255, 0, 0),
        Rgba.rgb(0, 255, 0),
        Rgba.rgb(0, 0, 255),
        Rgba.rgba(59, 130, 246, 210),
        Rgba.rgba(245, 158, 11, 140),
    ];
    for (c of palette) {
        const lab = rgbToOklab(c);
        const lch = oklabToOklch(lab);
        const lab2 = oklchToOklab(lch);
        expect_eq(lab.approxEql(lab2, EPS_F64), true);
        const back = oklabToRgb(lab2);
        expect_eq(back.eql(c), true);
    }

    // 3. Pure red OKLab reference values (~L=0.627955, a=0.224863, b=0.125846)
    const red_lab = rgbToOklab(Rgba.rgb(255, 0, 0));
    expect_eq(approxF64(red_lab.l, 0.627955, 0.001), true);
    expect_eq(approxF64(red_lab.a, 0.224863, 0.001), true);
    expect_eq(approxF64(red_lab.b, 0.125846, 0.001), true);

    // 4. Perceptual OKLab mixing preserves lightness without gray dead-zone
    const blue = Rgba.rgba(0, 0, 255, 100);
    const yellow = Rgba.rgba(255, 255, 0, 200);
    expect_eq(mixOklab(blue, yellow, 0.0).eql(blue), true);
    expect_eq(mixOklab(blue, yellow, 1.0).eql(yellow), true);

    const mid = mixOklab(blue, yellow, 0.5);
    expect_eq(mid.a, 150);
    const mid_lab = rgbToOklab(mid);
    const expected_l = (rgbToOklab(blue).l + rgbToOklab(yellow).l) * 0.5;
    expect_eq(approxF64(mid_lab.l, expected_l, 0.01), true);
}

fn testWcagAndPalette(): !void {
    const black = Rgba.rgb(0, 0, 0);
    const white = Rgba.rgb(255, 255, 255);

    // 1. Relative luminance & 21:1 max contrast ratio
    expect_eq(approxF64(Wcag.relativeLuminance(black), 0.0, EPS_F64), true);
    expect_eq(approxF64(Wcag.relativeLuminance(white), 1.0, EPS_F64), true);
    expect_eq(approxF64(Wcag.contrastRatio(black, white), 21.0, EPS_F64), true);
    expect_eq(approxF64(Wcag.contrastRatio(white, white), 1.0, EPS_F64), true);

    // 2. AA (4.5), AA Large (3.0), AAA (7.0)
    expect_eq(Wcag.meetsAA(black, white), true);
    expect_eq(Wcag.meetsAALarge(black, white), true);
    expect_eq(Wcag.meetsAAA(black, white), true);

    const mid_gray = Rgba.rgb(118, 118, 118); // ~4.54:1 against white
    expect_eq(Wcag.meetsAA(mid_gray, white), true);
    expect_eq(Wcag.meetsAAA(mid_gray, white), false);

    const light_gray = Rgba.rgb(148, 148, 148); // ~3.03:1 against white
    expect_eq(Wcag.meetsAALarge(light_gray, white), true);
    expect_eq(Wcag.meetsAA(light_gray, white), false);

    // 3. bestTextColor
    expect_eq(Wcag.bestTextColor(Rgba.rgb(15, 23, 42)).eql(white), true);
    expect_eq(Wcag.bestTextColor(Rgba.rgb(248, 250, 252)).eql(black), true);

    // 4. Palette harmonies
    const red = Rgba.rgb(255, 0, 0);
    const comp = Palette.complementary(red);
    expect_eq(comp[0].eql(red), true);
    expect_eq(comp[1].eql(Rgba.rgb(0, 255, 255)), true);

    const triad = Palette.triadic(red);
    expect_eq(triad[0].eql(red), true);
    expect_eq(triad[1].eql(Rgba.rgb(0, 255, 0)), true);
    expect_eq(triad[2].eql(Rgba.rgb(0, 0, 255)), true);

    const analog = Palette.analogous(red);
    expect_eq(analog[1].eql(red), true);
    expect_eq(approxF64(analog[0].toHsl().h, 330.0, 1.0), true);
    expect_eq(approxF64(analog[2].toHsl().h, 30.0, 1.0), true);
}

fn testAnsiCodec(): !void {
    const alloc = std.heap.page_allocator;

    // 1. TrueColor foreground & background SGR sequences
    const coral = Rgba.rgb(255, 127, 80);
    const fg = AnsiColorCodec.fgTrueColor(alloc, coral)?;
    defer alloc.free(fg);
    expect_eq(strEql(fg, "\x1b[38;2;255;127;80m"), true);

    const bg = AnsiColorCodec.bgTrueColor(alloc, Rgba.rgb(9, 12, 200))?;
    defer alloc.free(bg);
    expect_eq(strEql(bg, "\x1b[48;2;9;12;200m"), true);

    // 2. xterm-256 quantization (6x6x6 cube + 24-step grayscale)
    expect_eq(AnsiColorCodec.rgbToAnsi256(Rgba.rgb(0, 0, 0)), 16);
    expect_eq(AnsiColorCodec.rgbToAnsi256(Rgba.rgb(255, 255, 255)), 231);
    expect_eq(AnsiColorCodec.rgbToAnsi256(Rgba.rgb(255, 0, 0)), 196);
    expect_eq(AnsiColorCodec.rgbToAnsi256(Rgba.rgb(0, 255, 0)), 46);
    expect_eq(AnsiColorCodec.rgbToAnsi256(Rgba.rgb(0, 0, 255)), 21);
    expect_eq(AnsiColorCodec.rgbToAnsi256(Rgba.rgb(128, 128, 128)), 244); // 8 + 10*12 = 128 -> 232+12

    // 3. stripAnsi preserving UTF-8 content
    const styled = "\x1b[1m\x1b[38;2;255;0;0mHello\x1b[0m \x1b]8;;https://cp-lang.org\x07色彩\x1b]8;;\x07 World\x1b[2K!";
    const plain = AnsiColorCodec.stripAnsi(alloc, styled)?;
    defer alloc.free(plain);
    expect_eq(strEql(plain, "Hello 色彩 World!"), true);
}

fn testCapColorDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    const primary = cap.color.Rgba.parseHex("#2563eb")?;
    const surface = cap.color.Rgba.parseHex("#ffffff")?;
    expect_eq(cap.color.Wcag.meetsAA(primary, surface), true);
    expect_eq(cap.color.Wcag.bestTextColor(primary).eql(surface), true);

    const lab = cap.color.rgbToOklab(primary);
    const lch = cap.color.oklabToOklch(lab);
    expect_eq(cap.color.oklchToOklab(lch).toRgba().eql(primary), true);

    const blended = cap.color.mixOklab(primary, surface, 0.5);
    expect_eq(blended.r > primary.r && blended.r < surface.r, true);

    const fg = cap.color.AnsiColorCodec.fgTrueColor(alloc, primary)?;
    defer alloc.free(fg);
    const stripped = cap.color.AnsiColorCodec.stripAnsi(alloc, fg)?;
    defer alloc.free(stripped);
    expect_eq(stripped.len, 0);
}

fn main() {
    test("1. Hex (#RGB/#RGBA/#RRGGBB/#RRGGBBAA) & sRGB<->HSL/HSV Round-Trip", () => {
        testHexAndHslHsvRoundTrip()?;
    });
    test("2. OKLab/OKLCH Conversion Precision & Perceptual mixOklab", () => {
        testOklabOklchAndMixing()?;
    });
    test("3. WCAG 2.1 Luminance, 21:1 Contrast, AA/AAA, bestTextColor & Palette", () => {
        testWcagAndPalette()?;
    });
    test("4. ANSI TrueColor SGR, xterm-256 Quantization & stripAnsi", () => {
        testAnsiCodec()?;
    });
    test("5. import cap from \"cap\"; cap.color Automatic Dynamic Mounting", () => {
        testCapColorDynamicMount()?;
    });
}
