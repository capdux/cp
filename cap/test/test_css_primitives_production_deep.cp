import { test, expect } from "cap:test";
import {
    Percentage,
    NumberOrPercentage,
    AlphaValue,
    isAbsoluteCssUrl,
    isDataCssUrl,
} from "cap:css_primitives";

test("css_primitives: Percentage and NumberOrPercentage parse, math, and toCss", () => {
    const p = Percentage.parse("75%").?;
    expect(p.v).toBe(0.75);
    expect(p.asPercent()).toBe(75.0);
    expect(p.add(Percentage.fromPercent(25.0)).v).toBe(1.0);
    expect(Percentage.parse("50px") == null).toBeTrue();

    let buf: [32]u8 = undefined;
    expect(p.toCss(buf[0..])).toBe("75%");

    const np1 = NumberOrPercentage.parse("0.5").?;
    expect(np1.kind == .number).toBeTrue();
    expect(np1.asUnitValue()).toBe(0.5);

    const np2 = NumberOrPercentage.parse("50%").?;
    expect(np2.kind == .percentage).toBeTrue();
    expect(np2.asUnitValue()).toBe(0.5);
});

test("css_primitives: AlphaValue parsing, clamping, toByte, and CSS URL classification", () => {
    const a1 = AlphaValue.parse("50%").?;
    expect(a1.v).toBe(0.5);
    expect(a1.toByte()).toBe(128);

    const a2 = AlphaValue.parse("1.5").?;
    expect(a2.clamped().v).toBe(1.0);
    expect(a2.toByte()).toBe(255);

    const a3 = AlphaValue.parse("-0.2").?;
    expect(a3.toByte()).toBe(0);

    expect(isAbsoluteCssUrl("https://cp-lang.org/a.png")).toBeTrue();
    expect(isAbsoluteCssUrl("data:image/png;base64,AA==")).toBeTrue();
    expect(isAbsoluteCssUrl("/static/logo.svg")).toBeTrue();
    expect(isAbsoluteCssUrl("#clip-mask")).toBeTrue();
    expect(isAbsoluteCssUrl("./img.png")).toBeFalse();
    expect(isAbsoluteCssUrl("../img.png")).toBeFalse();
    expect(isAbsoluteCssUrl("assets/img.png")).toBeFalse();
    expect(isDataCssUrl("DATA:image/svg+xml,<svg/>")).toBeTrue();
    expect(isDataCssUrl("https://example.com")).toBeFalse();
});
