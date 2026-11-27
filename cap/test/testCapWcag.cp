// cap/test/testCapWcag.cp
// Comprehensive unit tests for @cap/wcag

import { test, expect_eq } from "cap:test";
import {
    Rgb,
    WcagLevel,
    parseHex,
    formatHex,
    relativeLuminance,
    contrastRatio,
    contrastRatioHex,
    meetsAA,
    meetsAAA,
    gradeContrast,
    bestTextColor,
    suggestAccessibleHex,
} from "../../packages/wcag-contrast/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    const diff = if (a > b) { a - b } else { b - a };
    return diff <= eps;
}

fn main() {
    test("1. @cap/wcag — black/white 21:1 & identical color 1:1 contrast", () => {
        const black = Rgb{ r: 0, g: 0, b: 0 };
        const white = Rgb{ r: 255, g: 255, b: 255 };
        const mid = Rgb{ r: 118, g: 118, b: 118 };

        expect_eq(approxEq(relativeLuminance(black), 0.0, 0.000000001), true);
        expect_eq(approxEq(relativeLuminance(white), 1.0, 0.000000001), true);

        expect_eq(approxEq(contrastRatio(black, white), 21.0, 0.000000001), true);
        expect_eq(approxEq(contrastRatio(white, black), 21.0, 0.000000001), true);
        expect_eq(approxEq(contrastRatio(black, black), 1.0, 0.000000001), true);
        expect_eq(approxEq(contrastRatio(white, white), 1.0, 0.000000001), true);
        expect_eq(approxEq(contrastRatio(mid, mid), 1.0, 0.000000001), true);

        // #767676 on #ffffff is the classic WCAG AA boundary (~4.54:1)
        const r_mid = contrastRatio(mid, white);
        expect_eq(approxEq(r_mid, 4.54, 0.02), true);
    });

    test("2. @cap/wcag — 3-digit & 6-digit HEX parsing and formatting", () => {
        const c1 = parseHex("#f0A");
        expect_eq(c1 != null, true);
        expect_eq(c1.?.r, 255 as u8);
        expect_eq(c1.?.g, 0 as u8);
        expect_eq(c1.?.b, 170 as u8);

        const c2 = parseHex("0f8");
        expect_eq(c2 != null, true);
        expect_eq(c2.?.r, 0 as u8);
        expect_eq(c2.?.g, 255 as u8);
        expect_eq(c2.?.b, 136 as u8);

        const c3 = parseHex("#1E293B");
        expect_eq(c3 != null, true);
        expect_eq(c3.?.r, 30 as u8);
        expect_eq(c3.?.g, 41 as u8);
        expect_eq(c3.?.b, 59 as u8);

        const c4 = parseHex("1e293b");
        expect_eq(c4 != null, true);
        expect_eq(c4.?.r, 30 as u8);

        // Invalid HEX inputs
        expect_eq(parseHex("") == null, true);
        expect_eq(parseHex("#12") == null, true);
        expect_eq(parseHex("#12345") == null, true);
        expect_eq(parseHex("#gggggg") == null, true);

        // formatHex
        let buf: [8]u8 = undefined;
        expect_eq(formatHex(c3.?, buf[0..8]), "#1e293b");
        expect_eq(formatHex(Rgb{ r: 0, g: 15, b: 255 }, buf[0..8]), "#000fff");
    });

    test("3. @cap/wcag — contrastRatioHex convenience & error handling", () => {
        const bw = contrastRatioHex("#000", "#fff");
        expect_eq(bw != null, true);
        expect_eq(approxEq(bw.?, 21.0, 0.000000001), true);

        // Pure red #ff0000 on white #ffffff -> L(red) = 0.2126 -> 1.05 / 0.2626 ≈ 3.998
        const rw = contrastRatioHex("ff0000", "ffffff");
        expect_eq(rw != null, true);
        expect_eq(approxEq(rw.?, 3.998, 0.01), true);

        expect_eq(contrastRatioHex("#zzz", "#fff") == null, true);
        expect_eq(contrastRatioHex("#000", "xyz") == null, true);
    });

    test("4. @cap/wcag — AA and AAA normal/large text threshold boundaries", () => {
        // AA: normal >= 4.5, large >= 3.0
        expect_eq(meetsAA(2.99, true), false);
        expect_eq(meetsAA(3.0, true), true);
        expect_eq(meetsAA(4.49, false), false);
        expect_eq(meetsAA(4.5, false), true);

        // AAA: normal >= 7.0, large >= 4.5
        expect_eq(meetsAAA(4.49, true), false);
        expect_eq(meetsAAA(4.5, true), true);
        expect_eq(meetsAAA(6.99, false), false);
        expect_eq(meetsAAA(7.0, false), true);
    });

    test("5. @cap/wcag — gradeContrast levels (Fail / AALarge / AA / AAA)", () => {
        expect_eq(gradeContrast(1.0), WcagLevel.Fail);
        expect_eq(gradeContrast(2.99), WcagLevel.Fail);
        expect_eq(gradeContrast(3.0), WcagLevel.AALarge);
        expect_eq(gradeContrast(4.49), WcagLevel.AALarge);
        expect_eq(gradeContrast(4.5), WcagLevel.AA);
        expect_eq(gradeContrast(6.99), WcagLevel.AA);
        expect_eq(gradeContrast(7.0), WcagLevel.AAA);
        expect_eq(gradeContrast(21.0), WcagLevel.AAA);
    });

    test("6. @cap/wcag — bestTextColor & suggestAccessibleHex for dark/light backgrounds", () => {
        // Dark backgrounds -> white text
        const dark_bg = Rgb{ r: 15, g: 23, b: 42 }; // slate-900
        const best_on_dark = bestTextColor(dark_bg);
        expect_eq(best_on_dark.r, 255 as u8);
        expect_eq(best_on_dark.g, 255 as u8);
        expect_eq(best_on_dark.b, 255 as u8);
        expect_eq(suggestAccessibleHex("#0f172a"), "#ffffff");
        expect_eq(suggestAccessibleHex("#000000"), "#ffffff");
        expect_eq(suggestAccessibleHex("#1d4ed8"), "#ffffff");

        // Light backgrounds -> black text
        const light_bg = Rgb{ r: 253, g: 224, b: 71 }; // yellow-300
        const best_on_light = bestTextColor(light_bg);
        expect_eq(best_on_light.r, 0 as u8);
        expect_eq(best_on_light.g, 0 as u8);
        expect_eq(best_on_light.b, 0 as u8);
        expect_eq(suggestAccessibleHex("#fde047"), "#000000");
        expect_eq(suggestAccessibleHex("#ffffff"), "#000000");
        expect_eq(suggestAccessibleHex("#e2e8f0"), "#000000");

        // Invalid HEX falls back safely to "#000000"
        expect_eq(suggestAccessibleHex("not-a-color"), "#000000");
    });
}
