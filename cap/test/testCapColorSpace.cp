// cap/test/testCapColorSpace.cp
// Comprehensive unit tests for @cap/color-convert (RGB, HSL, HSV, CMYK, Hex)

import { test, expect_eq } from "cap:test";
import {
    rgbToHsv,
    hsvToRgb,
    rgbToHsl,
    hslToRgb,
    rgbToCmyk,
    cmykToRgb,
    hexToRgb,
} from "../../packages/color-convert/src/index.cp";

fn csNear(a: f64, b: f64, eps: f64): bool {
    let d: f64 = a - b;
    if (d < 0.0) { d = -d; }
    return d <= eps;
}

fn main() {
    test("1. @cap/color-convert — RGB <-> HSL & HSV round-trip", () => {
        const hsl = rgbToHsl(255 as u8, 0 as u8, 0 as u8);
        expect_eq(csNear(hsl.h, 0.0, 0.5), true);
        expect_eq(csNear(hsl.s, 100.0, 0.5), true);
        expect_eq(csNear(hsl.l, 50.0, 0.5), true);
        const back1 = hslToRgb(hsl.h, hsl.s, hsl.l);
        expect_eq(back1.r, 255 as u8);
        expect_eq(back1.g, 0 as u8);
        expect_eq(back1.b, 0 as u8);

        const hsv = rgbToHsv(0 as u8, 255 as u8, 0 as u8);
        expect_eq(csNear(hsv.h, 120.0, 0.5), true);
        const back2 = hsvToRgb(hsv.h, hsv.s, hsv.v);
        expect_eq(back2.g, 255 as u8);
    });

    test("2. @cap/color-convert — CMYK & Hex conversions", () => {
        const cmyk = rgbToCmyk(0 as u8, 0 as u8, 255 as u8);
        const back = cmykToRgb(cmyk.c, cmyk.m, cmyk.y, cmyk.k);
        expect_eq(back.r, 0 as u8);
        expect_eq(back.g, 0 as u8);
        expect_eq(back.b, 255 as u8);

        const h = hexToRgb("#ff8000");
        expect_eq(h.found, true);
        expect_eq(h.r, 255 as u8);
        expect_eq(h.g, 128 as u8);
        expect_eq(h.b, 0 as u8);
    });
}
