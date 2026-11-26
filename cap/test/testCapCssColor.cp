// cap/test/testCapCssColor.cp
// Comprehensive unit tests for @cap/css-color

import { test, expect_eq } from "cap:test";
import {
    hslToRgb,
    rgbToHsl,
    parseCssColor,
    formatRgb,
} from "../../packages/@cap/css-color/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn nearF64(a: f64, b: f64): bool {
    let d: f64 = a - b;
    if (d < (0.0 as f64)) {
        d = -d;
    }
    return d < (0.01 as f64);
}

fn main() {
    test("1. @cap/css-color — Legacy rgb(255, 128, 0) & rgba(10, 20, 30, 0.5)", () => {
        const c1 = parseCssColor("rgb(255, 128, 0)");
        expect_eq(c1.valid, true);
        expect_eq(c1.r, 255 as u8);
        expect_eq(c1.g, 128 as u8);
        expect_eq(c1.b, 0 as u8);
        expect_eq(nearF64(c1.a, 1.0 as f64), true);

        const c2 = parseCssColor("RGBA(10, 20, 30, 0.5)");
        expect_eq(c2.valid, true);
        expect_eq(c2.r, 10 as u8);
        expect_eq(c2.g, 20 as u8);
        expect_eq(c2.b, 30 as u8);
        expect_eq(nearF64(c2.a, 0.5 as f64), true);
    });

    test("2. @cap/css-color — CSS Color 4 space syntax rgb(255 0 128 / 50%)", () => {
        const c = parseCssColor("rgb(255 0 128 / 50%)");
        expect_eq(c.valid, true);
        expect_eq(c.r, 255 as u8);
        expect_eq(c.g, 0 as u8);
        expect_eq(c.b, 128 as u8);
        expect_eq(nearF64(c.a, 0.5 as f64), true);

        const c2 = parseCssColor("hsl(240deg 100% 50% / 0.25)");
        expect_eq(c2.valid, true);
        expect_eq(c2.r, 0 as u8);
        expect_eq(c2.g, 0 as u8);
        expect_eq(c2.b, 255 as u8);
        expect_eq(nearF64(c2.a, 0.25 as f64), true);
    });

    test("3. @cap/css-color — hsl primary red (0), green (120) & blue (240)", () => {
        const red = parseCssColor("hsl(0, 100%, 50%)");
        expect_eq(red.valid, true);
        expect_eq(red.r, 255 as u8);
        expect_eq(red.g, 0 as u8);
        expect_eq(red.b, 0 as u8);

        const green = parseCssColor("hsl(120, 100%, 50%)");
        expect_eq(green.valid, true);
        expect_eq(green.r, 0 as u8);
        expect_eq(green.g, 255 as u8);
        expect_eq(green.b, 0 as u8);

        const blue = parseCssColor("hsla(240, 100%, 50%, 0.5)");
        expect_eq(blue.valid, true);
        expect_eq(blue.r, 0 as u8);
        expect_eq(blue.g, 0 as u8);
        expect_eq(blue.b, 255 as u8);
        expect_eq(nearF64(blue.a, 0.5 as f64), true);
    });

    test("4. @cap/css-color — rgbToHsl <-> hslToRgb round-trip", () => {
        const hsl = rgbToHsl(64 as u8, 128 as u8, 192 as u8, 0.75 as f64);
        expect_eq(hsl.valid, true);
        expect_eq(nearF64(hsl.h, 210.0 as f64), true);
        expect_eq(nearF64(hsl.s, 50.3937 as f64), true);

        const back = hslToRgb(hsl.h, hsl.s, hsl.l, hsl.a);
        expect_eq(back.r, 64 as u8);
        expect_eq(back.g, 128 as u8);
        expect_eq(back.b, 192 as u8);
        expect_eq(nearF64(back.a, 0.75 as f64), true);
    });

    test("5. @cap/css-color — formatRgb opaque vs translucent output", () => {
        const solidCol = parseCssColor("rgb(255, 128, 0)");
        let b1: [32]u8 = undefined;
        const n1 = formatRgb(solidCol, b1[0..32]);
        expect_eq(sliceEq(b1[0..n1], "rgb(255, 128, 0)"), true);

        const semi = parseCssColor("rgb(255 0 128 / 50%)");
        let b2: [32]u8 = undefined;
        const n2 = formatRgb(semi, b2[0..32]);
        expect_eq(sliceEq(b2[0..n2], "rgba(255, 0, 128, 0.5)"), true);
    });

    test("6. @cap/css-color — Reject invalid function names, missing parens & tiny buffer", () => {
        expect_eq(parseCssColor("cmyk(0, 0, 0, 0)").valid, false);
        expect_eq(parseCssColor("rgb(255, 0, 0").valid, false);
        expect_eq(parseCssColor("rgb(255, 0)").valid, false);
        expect_eq(parseCssColor("rgb(255, 0 / 50%)").valid, false);
        expect_eq(parseCssColor("rgb(abc, 0, 0)").valid, false);

        const c = parseCssColor("rgb(255, 128, 0)");
        let tiny: [6]u8 = undefined;
        expect_eq(formatRgb(c, tiny[0..6]), 0 as usize);
    });
}
