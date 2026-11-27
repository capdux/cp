// cap/test/testCapColorHsl.cp
// Comprehensive unit tests for @cap/color-hsl

import { test, expect_eq } from "cap:test";
import {
    Rgb,
    Hsl,
    Hsv,
    Cmyk,
    rgbToHsl,
    hslToRgb,
    rgbToHsv,
    hsvToRgb,
    rgbToCmyk,
    cmykToRgb,
    rgbFromU8,
    rotateHue,
    lighten,
    darken,
    mixRgb,
} from "../../packages/palette/src/index.cp";

fn near(a: f64, b: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d <= (0.0001 as f64);
}

fn rgbNear(a: Rgb, b: Rgb): bool {
    return near(a.r, b.r) && near(a.g, b.g) && near(a.b, b.b);
}

fn main() {
    test("1. @cap/color-hsl — Primary Red/Green/Blue RGB <-> HSL exact round-trip", () => {
        const red = rgbFromU8(255 as u8, 0 as u8, 0 as u8);
        const hRed = rgbToHsl(red);
        expect_eq(near(hRed.h, 0.0 as f64), true);
        expect_eq(near(hRed.s, 1.0 as f64), true);
        expect_eq(near(hRed.l, 0.5 as f64), true);
        expect_eq(rgbNear(hslToRgb(hRed), red), true);

        const green = rgbFromU8(0 as u8, 255 as u8, 0 as u8);
        const hGreen = rgbToHsl(green);
        expect_eq(near(hGreen.h, 120.0 as f64), true);
        expect_eq(rgbNear(hslToRgb(hGreen), green), true);

        const blue = rgbFromU8(0 as u8, 0 as u8, 255 as u8);
        const hBlue = rgbToHsl(blue);
        expect_eq(near(hBlue.h, 240.0 as f64), true);
        expect_eq(rgbNear(hslToRgb(hBlue), blue), true);
    });

    test("2. @cap/color-hsl — Achromatic black/white/gray boundaries", () => {
        const black = Rgb{ r: 0.0 as f64, g: 0.0 as f64, b: 0.0 as f64 };
        const white = Rgb{ r: 1.0 as f64, g: 1.0 as f64, b: 1.0 as f64 };
        const gray = Rgb{ r: 0.5 as f64, g: 0.5 as f64, b: 0.5 as f64 };

        const hB = rgbToHsl(black);
        expect_eq(near(hB.s, 0.0 as f64) && near(hB.l, 0.0 as f64), true);
        expect_eq(rgbNear(hslToRgb(hB), black), true);

        const hW = rgbToHsl(white);
        expect_eq(near(hW.s, 0.0 as f64) && near(hW.l, 1.0 as f64), true);
        expect_eq(rgbNear(hslToRgb(hW), white), true);

        const hG = rgbToHsl(gray);
        expect_eq(near(hG.s, 0.0 as f64) && near(hG.l, 0.5 as f64), true);
        expect_eq(rgbNear(hslToRgb(hG), gray), true);
    });

    test("3. @cap/color-hsl — RGB <-> HSV round-trip", () => {
        const sample = Rgb{ r: 0.25 as f64, g: 0.75 as f64, b: 0.5 as f64 };
        const hsv = rgbToHsv(sample);
        expect_eq(near(hsv.h, 150.0 as f64), true);
        expect_eq(near(hsv.v, 0.75 as f64), true);
        expect_eq(rgbNear(hsvToRgb(hsv), sample), true);
    });

    test("4. @cap/color-hsl — RGB <-> CMYK round-trip including pure black (k=1)", () => {
        const black = Rgb{ r: 0.0 as f64, g: 0.0 as f64, b: 0.0 as f64 };
        const cBlack = rgbToCmyk(black);
        expect_eq(near(cBlack.c, 0.0 as f64) && near(cBlack.k, 1.0 as f64), true);
        expect_eq(rgbNear(cmykToRgb(cBlack), black), true);

        const sample = Rgb{ r: 0.8 as f64, g: 0.4 as f64, b: 0.2 as f64 };
        const cmyk = rgbToCmyk(sample);
        expect_eq(near(cmyk.k, 0.2 as f64), true);
        expect_eq(near(cmyk.c, 0.0 as f64), true);
        expect_eq(near(cmyk.m, 0.5 as f64), true);
        expect_eq(near(cmyk.y, 0.75 as f64), true);
        expect_eq(rgbNear(cmykToRgb(cmyk), sample), true);
    });

    test("5. @cap/color-hsl — rotateHue 360-deg wrap & lighten/darken clamping", () => {
        const base = Hsl{ h: 300.0 as f64, s: 0.8 as f64, l: 0.5 as f64 };
        const rPos = rotateHue(base, 90.0 as f64);
        expect_eq(near(rPos.h, 30.0 as f64), true);
        const rNeg = rotateHue(Hsl{ h: 20.0 as f64, s: 0.8 as f64, l: 0.5 as f64 }, -50.0 as f64);
        expect_eq(near(rNeg.h, 330.0 as f64), true);

        expect_eq(near(lighten(base, 0.2 as f64).l, 0.7 as f64), true);
        expect_eq(near(lighten(base, 0.9 as f64).l, 1.0 as f64), true);
        expect_eq(near(darken(base, 0.2 as f64).l, 0.3 as f64), true);
        expect_eq(near(darken(base, 0.9 as f64).l, 0.0 as f64), true);
    });

    test("6. @cap/color-hsl — mixRgb linear interpolation & t clamping", () => {
        const red = Rgb{ r: 1.0 as f64, g: 0.0 as f64, b: 0.0 as f64 };
        const blue = Rgb{ r: 0.0 as f64, g: 0.0 as f64, b: 1.0 as f64 };
        const mid = mixRgb(red, blue, 0.5 as f64);
        expect_eq(rgbNear(mid, Rgb{ r: 0.5 as f64, g: 0.0 as f64, b: 0.5 as f64 }), true);
        expect_eq(rgbNear(mixRgb(red, blue, -0.5 as f64), red), true);
        expect_eq(rgbNear(mixRgb(red, blue, 1.5 as f64), blue), true);
    });
}
