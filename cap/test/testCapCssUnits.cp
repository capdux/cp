// cap/test/testCapCssUnits.cp
// Comprehensive unit tests for @cap/css-units

import { test, expect_eq } from "cap:test";
import {
    CssUnitKind,
    CssContext,
    parseCssDimension,
    toPixels,
    toDegrees,
    toMilliseconds,
    unitCategory,
} from "../../packages/postcss-value-parser/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    const diff = if (a > b) { a - b } else { b - a };
    return diff <= eps;
}

fn main() {
    test("1. @cap/css-units — Absolute length parsing & 96dpi pixel conversion", () => {
        const ctx = CssContext.defaultCtx();

        const dPx = parseCssDimension("16PX");
        expect_eq(dPx.valid, true);
        expect_eq(dPx.unit == CssUnitKind.Px, true);
        expect_eq(approxEq(toPixels(dPx, ctx) ?? -1.0, 16.0, 0.0001), true);

        const dIn = parseCssDimension("1in");
        expect_eq(approxEq(toPixels(dIn, ctx) ?? -1.0, 96.0, 0.0001), true);

        const dPt = parseCssDimension("72pt");
        expect_eq(approxEq(toPixels(dPt, ctx) ?? -1.0, 96.0, 0.0001), true);

        const dPc = parseCssDimension("2pc");
        expect_eq(approxEq(toPixels(dPc, ctx) ?? -1.0, 32.0, 0.0001), true);

        const dCm = parseCssDimension("2.54cm");
        expect_eq(approxEq(toPixels(dCm, ctx) ?? -1.0, 96.0, 0.0001), true);

        const dMm = parseCssDimension("25.4mm");
        expect_eq(approxEq(toPixels(dMm, ctx) ?? -1.0, 96.0, 0.0001), true);

        const dQ = parseCssDimension("101.6q");
        expect_eq(approxEq(toPixels(dQ, ctx) ?? -1.0, 96.0, 0.0001), true);
    });

    test("2. @cap/css-units — Relative lengths (rem, em, vh, vw, vmin, vmax, ch) & context", () => {
        const ctx = CssContext{
            root_font_px: 16.0,
            parent_font_px: 20.0,
            viewport_w: 1920.0,
            viewport_h: 1080.0,
        };

        expect_eq(approxEq(toPixels(parseCssDimension(".5rem"), ctx) ?? -1.0, 8.0, 0.0001), true);
        expect_eq(approxEq(toPixels(parseCssDimension("1.5em"), ctx) ?? -1.0, 30.0, 0.0001), true);
        expect_eq(approxEq(toPixels(parseCssDimension("50vw"), ctx) ?? -1.0, 960.0, 0.0001), true);
        expect_eq(approxEq(toPixels(parseCssDimension("10vh"), ctx) ?? -1.0, 108.0, 0.0001), true);
        expect_eq(approxEq(toPixels(parseCssDimension("10vmin"), ctx) ?? -1.0, 108.0, 0.0001), true);
        expect_eq(approxEq(toPixels(parseCssDimension("10vmax"), ctx) ?? -1.0, 192.0, 0.0001), true);
        expect_eq(approxEq(toPixels(parseCssDimension("4ch"), ctx) ?? -1.0, 40.0, 0.0001), true);

        const pct = parseCssDimension("100%");
        expect_eq(pct.valid, true);
        expect_eq(pct.unit == CssUnitKind.Percent, true);
        expect_eq(unitCategory(pct.unit), "percentage");
        expect_eq(toPixels(pct, ctx) == null, true);
    });

    test("3. @cap/css-units — Angle parsing & toDegrees conversion", () => {
        expect_eq(approxEq(toDegrees(parseCssDimension("-45deg")) ?? 0.0, -45.0, 0.0001), true);
        expect_eq(approxEq(toDegrees(parseCssDimension("0.25turn")) ?? 0.0, 90.0, 0.0001), true);
        expect_eq(approxEq(toDegrees(parseCssDimension("200grad")) ?? 0.0, 180.0, 0.0001), true);
        expect_eq(approxEq(toDegrees(parseCssDimension("3.141592653589793rad")) ?? 0.0, 180.0, 0.0001), true);
        expect_eq(unitCategory(CssUnitKind.Turn), "angle");
        expect_eq(toDegrees(parseCssDimension("16px")) == null, true);
    });

    test("4. @cap/css-units — Time parsing & toMilliseconds conversion", () => {
        expect_eq(approxEq(toMilliseconds(parseCssDimension("250ms")) ?? -1.0, 250.0, 0.0001), true);
        expect_eq(approxEq(toMilliseconds(parseCssDimension("1.5S")) ?? -1.0, 1500.0, 0.0001), true);
        expect_eq(approxEq(toMilliseconds(parseCssDimension("1e2ms")) ?? -1.0, 100.0, 0.0001), true);
        expect_eq(unitCategory(CssUnitKind.Ms), "time");
        expect_eq(toMilliseconds(parseCssDimension("90deg")) == null, true);
    });

    test("5. @cap/css-units — Resolution units, scientific notation & unitless", () => {
        const rDpi = parseCssDimension("300dpi");
        expect_eq(rDpi.valid, true);
        expect_eq(rDpi.unit == CssUnitKind.Dpi, true);
        expect_eq(unitCategory(rDpi.unit), "resolution");

        const rDppx = parseCssDimension("2dppx");
        expect_eq(rDppx.valid, true);
        expect_eq(rDppx.unit == CssUnitKind.Dppx, true);

        const rDpcm = parseCssDimension("118.11dpcm");
        expect_eq(rDpcm.valid, true);
        expect_eq(rDpcm.unit == CssUnitKind.Dpcm, true);

        const sci = parseCssDimension("2.5e-1rem");
        expect_eq(sci.valid, true);
        expect_eq(sci.unit == CssUnitKind.Rem, true);
        expect_eq(approxEq(sci.value, 0.25, 0.0001), true);

        const zero = parseCssDimension("0");
        expect_eq(zero.valid, true);
        expect_eq(zero.unit == CssUnitKind.Unitless, true);
        expect_eq(unitCategory(zero.unit), "none");
        expect_eq(approxEq(toPixels(zero, CssContext.defaultCtx()) ?? -1.0, 0.0, 0.0001), true);
    });

    test("6. @cap/css-units — Invalid inputs & unknown unit handling", () => {
        expect_eq(parseCssDimension("").valid, false);
        expect_eq(parseCssDimension("   ").valid, false);
        expect_eq(parseCssDimension("px").valid, false);
        expect_eq(parseCssDimension(".rem").valid, false);
        expect_eq(parseCssDimension("16 px").valid, false);
        expect_eq(parseCssDimension("10px!").valid, false);

        const unk = parseCssDimension("12foo");
        expect_eq(unk.valid, true);
        expect_eq(unk.unit == CssUnitKind.Unknown, true);
        expect_eq(unitCategory(unk.unit), "none");
        expect_eq(toPixels(unk, CssContext.defaultCtx()) == null, true);
    });
}
