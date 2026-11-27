// cap/test/testCapColorNames.cp
// Comprehensive unit tests for @cap/color-names

import { test, expect_eq } from "cap:test";
import {
    Rgb,
    Hsl,
    Hsv,
    cssColorCount,
    rgbFromCssName,
    cssNameFromRgb,
    rgbToHsl,
    hslToRgb,
    rgbToHsv,
    hsvToRgb,
    rgbToHex,
    hexToRgb,
    mixRgb,
    lighten,
    darken,
} from "../../packages/color-name/src/index.cp";

fn main() {
    test("1. @cap/color-names — 148 W3C CSS Color 4 named colors & case-insensitive lookup", () => {
        expect_eq(cssColorCount(), 148 as usize);

        const rp = rgbFromCssName("  RebeccaPurple  ");
        expect_eq(rp != null, true);
        expect_eq(rp.?.r, 102 as u8);
        expect_eq(rp.?.g, 51 as u8);
        expect_eq(rp.?.b, 153 as u8);

        const cb = rgbFromCssName("cornflowerblue");
        expect_eq(cb != null, true);
        expect_eq(cb.?.r, 100 as u8);
        expect_eq(cb.?.g, 149 as u8);
        expect_eq(cb.?.b, 237 as u8);

        const tomato = rgbFromCssName("TOMATO");
        expect_eq(tomato != null, true);
        expect_eq(tomato.?.r, 255 as u8);
        expect_eq(tomato.?.g, 99 as u8);
        expect_eq(tomato.?.b, 71 as u8);

        // Aliases: aqua/cyan, fuchsia/magenta, gray/grey
        const aqua = rgbFromCssName("aqua").?;
        const cyan = rgbFromCssName("cyan").?;
        expect_eq(aqua.r == cyan.r && aqua.g == cyan.g && aqua.b == cyan.b, true);

        const gray = rgbFromCssName("gray").?;
        const grey = rgbFromCssName("grey").?;
        expect_eq(gray.r == grey.r && gray.g == grey.g && gray.b == grey.b, true);

        expect_eq(rgbFromCssName("notacolor") == null, true);
        expect_eq(rgbFromCssName("") == null, true);
    });

    test("2. @cap/color-names — Reverse lookup cssNameFromRgb", () => {
        expect_eq(cssNameFromRgb(Rgb.from(102, 51, 153)) ?? "", "rebeccapurple");
        expect_eq(cssNameFromRgb(Rgb.from(255, 99, 71)) ?? "", "tomato");
        expect_eq(cssNameFromRgb(Rgb.from(240, 248, 255)) ?? "", "aliceblue");
        expect_eq(cssNameFromRgb(Rgb.from(154, 205, 50)) ?? "", "yellowgreen");
        expect_eq(cssNameFromRgb(Rgb.from(1, 2, 3)) == null, true);
    });

    test("3. @cap/color-names — RGB <-> HSL exact round-trip accuracy", () => {
        const samples: [6]Rgb = [
            Rgb.from(255, 0, 0),
            Rgb.from(0, 255, 0),
            Rgb.from(0, 0, 255),
            Rgb.from(102, 51, 153),
            Rgb.from(100, 149, 237),
            Rgb.from(128, 128, 128),
        ];
        let i: usize = 0;
        while (i < samples.len) {
            const orig = samples[i];
            const hsl = rgbToHsl(orig);
            const back = hslToRgb(hsl);
            expect_eq(back.r, orig.r);
            expect_eq(back.g, orig.g);
            expect_eq(back.b, orig.b);
            i += 1;
        }
    });

    test("4. @cap/color-names — RGB <-> HSV exact round-trip accuracy", () => {
        const samples: [6]Rgb = [
            Rgb.from(255, 99, 71),
            Rgb.from(64, 224, 208),
            Rgb.from(255, 215, 0),
            Rgb.from(75, 0, 130),
            Rgb.from(0, 0, 0),
            Rgb.from(255, 255, 255),
        ];
        let i: usize = 0;
        while (i < samples.len) {
            const orig = samples[i];
            const hsv = rgbToHsv(orig);
            const back = hsvToRgb(hsv);
            expect_eq(back.r, orig.r);
            expect_eq(back.g, orig.g);
            expect_eq(back.b, orig.b);
            i += 1;
        }
    });

    test("5. @cap/color-names — HEX encoding & decoding (#rrggbb, #rgb, invalid)", () => {
        let out: [16]u8 = undefined;
        expect_eq(rgbToHex(Rgb.from(102, 51, 153), out[0..16]), "#663399");
        expect_eq(rgbToHex(Rgb.from(0, 15, 255), out[0..16]), "#000fff");

        const parsed6 = hexToRgb("#6495ED");
        expect_eq(parsed6 != null, true);
        expect_eq(parsed6.?.r, 100 as u8);
        expect_eq(parsed6.?.g, 149 as u8);
        expect_eq(parsed6.?.b, 237 as u8);

        const parsed3 = hexToRgb("#f0a");
        expect_eq(parsed3 != null, true);
        expect_eq(parsed3.?.r, 255 as u8);
        expect_eq(parsed3.?.g, 0 as u8);
        expect_eq(parsed3.?.b, 170 as u8);

        expect_eq(hexToRgb("#12345") == null, true);
        expect_eq(hexToRgb("#zzzzzz") == null, true);
    });

    test("6. @cap/color-names — mixRgb, lighten & darken utilities", () => {
        const black = Rgb.from(0, 0, 0);
        const white = Rgb.from(255, 255, 255);
        const mid = mixRgb(black, white, 0.5);
        expect_eq(mid.r, 128 as u8);
        expect_eq(mid.g, 128 as u8);
        expect_eq(mid.b, 128 as u8);

        const base = Rgb.from(102, 51, 153); // L = 0.4
        const lighter = lighten(base, 0.2);
        const darker = darken(base, 0.2);
        expect_eq(rgbToHsl(lighter).l > rgbToHsl(base).l, true);
        expect_eq(rgbToHsl(darker).l < rgbToHsl(base).l, true);

        // Clamp bounds: lighten to pure white, darken to pure black
        const pure_white = lighten(base, 1.0);
        expect_eq(pure_white.r, 255 as u8);
        expect_eq(pure_white.g, 255 as u8);
        expect_eq(pure_white.b, 255 as u8);

        const pure_black = darken(base, 1.0);
        expect_eq(pure_black.r, 0 as u8);
        expect_eq(pure_black.g, 0 as u8);
        expect_eq(pure_black.b, 0 as u8);
    });
}
