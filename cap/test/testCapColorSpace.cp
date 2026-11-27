// cap/test/testCapColorSpace.cp
// Comprehensive unit tests for @cap/color-space

import { test, expect_eq } from "cap:test";
import {
    Rgb,
    srgbToLinear,
    linearToSrgb,
    rgbToHsl,
    hslToRgb,
    rgbToHsv,
    hsvToRgb,
    rgbToOklab,
    oklabToRgb,
    oklabToOklch,
    oklchToOklab,
    oklabDistance,
    mixOklab,
} from "../../packages/color-convert/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff: f64 = a - b;
    if (diff < 0.0) { diff = -diff; }
    return diff <= eps;
}

fn main() {
    test("1. @cap/color-space — Primary colors & black/white in HSL, HSV & OKLab", () => {
        const red = Rgb{ r: 1.0, g: 0.0, b: 0.0 };
        const redHsl = rgbToHsl(red);
        expect_eq(approxEq(redHsl.h, 0.0, 0.0001), true);
        expect_eq(approxEq(redHsl.s, 1.0, 0.0001), true);
        expect_eq(approxEq(redHsl.l, 0.5, 0.0001), true);

        const green = Rgb{ r: 0.0, g: 1.0, b: 0.0 };
        expect_eq(approxEq(rgbToHsv(green).h, 120.0, 0.0001), true);

        const blue = Rgb{ r: 0.0, g: 0.0, b: 1.0 };
        expect_eq(approxEq(rgbToHsl(blue).h, 240.0, 0.0001), true);

        const whiteLab = rgbToOklab(Rgb{ r: 1.0, g: 1.0, b: 1.0 });
        expect_eq(approxEq(whiteLab.l, 1.0, 0.0001), true);
        expect_eq(approxEq(whiteLab.a, 0.0, 0.0001), true);
        expect_eq(approxEq(whiteLab.b, 0.0, 0.0001), true);

        const blackLab = rgbToOklab(Rgb{ r: 0.0, g: 0.0, b: 0.0 });
        expect_eq(approxEq(blackLab.l, 0.0, 0.0001), true);
    });

    test("2. @cap/color-space — rgb <-> hsl round-trip (< 1e-6 error)", () => {
        const c = Rgb{ r: 0.25, g: 0.62, b: 0.88 };
        const back = hslToRgb(rgbToHsl(c));
        expect_eq(approxEq(back.r, c.r, 0.000001), true);
        expect_eq(approxEq(back.g, c.g, 0.000001), true);
        expect_eq(approxEq(back.b, c.b, 0.000001), true);
    });

    test("3. @cap/color-space — rgb <-> hsv round-trip (< 1e-6 error)", () => {
        const c = Rgb{ r: 0.91, g: 0.45, b: 0.17 };
        const back = hsvToRgb(rgbToHsv(c));
        expect_eq(approxEq(back.r, c.r, 0.000001), true);
        expect_eq(approxEq(back.g, c.g, 0.000001), true);
        expect_eq(approxEq(back.b, c.b, 0.000001), true);
    });

    test("4. @cap/color-space — rgb <-> oklab <-> oklch round-trip (< 1e-4 error)", () => {
        const c = Rgb{ r: 0.3, g: 0.7, b: 0.5 };
        const lab = rgbToOklab(c);
        const lch = oklabToOklch(lab);
        const lab2 = oklchToOklab(lch);
        const back = oklabToRgb(lab2);

        expect_eq(approxEq(back.r, c.r, 0.0001), true);
        expect_eq(approxEq(back.g, c.g, 0.0001), true);
        expect_eq(approxEq(back.b, c.b, 0.0001), true);
    });

    test("5. @cap/color-space — sRGB <-> Linear gamma curve exact round-trip", () => {
        expect_eq(approxEq(linearToSrgb(srgbToLinear(0.02)), 0.02, 0.0000001), true);
        expect_eq(approxEq(linearToSrgb(srgbToLinear(0.5)), 0.5, 0.0000001), true);
        expect_eq(approxEq(linearToSrgb(srgbToLinear(0.85)), 0.85, 0.0000001), true);
        expect_eq(approxEq(srgbToLinear(0.5), 0.21404114, 0.00001), true);
    });

    test("6. @cap/color-space — oklabDistance symmetry & mixOklab endpoints/midpoint", () => {
        const red = Rgb{ r: 1.0, g: 0.0, b: 0.0 };
        const blue = Rgb{ r: 0.0, g: 0.0, b: 1.0 };

        expect_eq(approxEq(oklabDistance(red, red), 0.0, 0.000001), true);
        expect_eq(approxEq(oklabDistance(red, blue), oklabDistance(blue, red), 0.000001), true);
        expect_eq(oklabDistance(red, blue) > 0.2, true);

        const m0 = mixOklab(red, blue, 0.0);
        const m1 = mixOklab(red, blue, 1.0);
        const mid = mixOklab(red, blue, 0.5);
        expect_eq(approxEq(m0.r, 1.0, 0.0001), true);
        expect_eq(approxEq(m1.b, 1.0, 0.0001), true);
        // Midpoint should be roughly equidistant in OKLab
        expect_eq(approxEq(oklabDistance(red, mid), oklabDistance(mid, blue), 0.02), true);
    });
}
