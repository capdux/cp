// Deep production regression tests for cap:css FallbackHandler (lib/css/fallback.cp)

import { test, expect, expectError } from "cap:test";
import {
    CSS_COMPAT_OKLAB_COLORS,
    CSS_COMPAT_P3_COLORS,
    CSS_COMPAT_COLOR_MIX,
    CSS_COMPAT_ALL_MODERN,
    CssFallbackError,
    CssFallbackProp,
    CssFallbackDeclList,
    detectRequiredColorFeatures,
    isCssValueCompatible,
    containsCssVarRef,
    CssFallbackHandler,
} from "cap:css";

test("cssFallback: property parsing and modern color feature detection", () => {
    const p = CssFallbackProp.parse("TEXT-SHADOW") ?? return;
    expect(p == .TextShadow).toBe(true);
    expect(p.asStr()).toEqual("text-shadow");
    expect(CssFallbackProp.parse("display") == null).toBe(true);

    const req = detectRequiredColorFeatures("color-mix(in oklch, oklch(60% 0.2 250), color(display-p3 1 0 0))");
    expect((req & CSS_COMPAT_OKLAB_COLORS) != 0).toBe(true);
    expect((req & CSS_COMPAT_P3_COLORS) != 0).toBe(true);
    expect((req & CSS_COMPAT_COLOR_MIX) != 0).toBe(true);

    expect(isCssValueCompatible("oklch(60% 0.2 250)", 0)).toBe(false);
    expect(isCssValueCompatible("oklch(60% 0.2 250)", CSS_COMPAT_OKLAB_COLORS)).toBe(true);
    expect(isCssValueCompatible("#ff0000", 0)).toBe(true);
    expect(containsCssVarRef("0 2px 4px var(--shadow-color)")).toBe(true);
});

test("cssFallback: emits fallback before incompatible modern color for legacy targets", () => {
    let h = CssFallbackHandler.init(0); // 目标浏览器不支持任何现代色域
    let dest = CssFallbackDeclList.init();
    const fbs: [1][]const u8 = ["#3b82f6"];

    * = h.handleProperty(.Color, "oklch(62% 0.21 259)", fbs[0..], &dest)?;
    expect(dest.count).toBe(2 as usize);
    expect(dest.slice()[0].is_fallback).toBe(true);
    expect(dest.slice()[0].value).toEqual("#3b82f6");
    expect(dest.slice()[1].is_fallback).toBe(false);
    expect(dest.slice()[1].value).toEqual("oklch(62% 0.21 259)");

    let buf: [128]u8 = [0; 128];
    const n = dest.formatCss(buf[0..], true)?;
    expect(buf[0..n]).toEqual("color:#3b82f6;color:oklch(62% 0.21 259)");
});

test("cssFallback: skips redundant fallback when target browser natively supports feature", () => {
    let h = CssFallbackHandler.init(CSS_COMPAT_ALL_MODERN);
    let dest = CssFallbackDeclList.init();
    const fbs: [1][]const u8 = ["#3b82f6"];

    * = h.handleProperty(.Color, "oklch(62% 0.21 259)", fbs[0..], &dest)?;
    expect(dest.count).toBe(1 as usize);
    expect(dest.slice()[0].value).toEqual("oklch(62% 0.21 259)");
});

test("cssFallback: preserves manual progressive fallback chain vs overwrites with compatible value", () => {
    let h = CssFallbackHandler.init(0);
    let dest = CssFallbackDeclList.init();
    const empty_fb: [0][]const u8 = [];

    // 1. 用户先手写 sRGB fallback，再写不兼容的 oklch -> 两条均保留，形成渐进增强链
    * = h.handleProperty(.BackgroundColor, "#ff0000", empty_fb[0..], &dest)?;
    * = h.handleProperty(.BackgroundColor, "oklch(62% 0.25 29)", empty_fb[0..], &dest)?;
    expect(dest.count).toBe(2 as usize);
    expect(dest.slice()[0].value).toEqual("#ff0000");
    expect(dest.slice()[1].value).toEqual("oklch(62% 0.25 29)");

    // 2. 随后再写一个兼容值 `#00ff00` -> 原地覆盖最近槽位（槽位 1）
    * = h.handleProperty(.BackgroundColor, "#00ff00", empty_fb[0..], &dest)?;
    expect(dest.count).toBe(2 as usize);
    expect(dest.slice()[1].value).toEqual("#00ff00");
});

test("cssFallback: handles unparsed var() properties and resets slots on finalize()", () => {
    let h = CssFallbackHandler.init(0);
    let dest = CssFallbackDeclList.init();
    const empty_fb: [0][]const u8 = [];

    * = h.handleProperty(.Color, "#111", empty_fb[0..], &dest)?;
    * = h.handleUnparsed(.Color, "var(--fg)", &dest)?;
    expect(dest.count).toBe(1 as usize);
    expect(dest.slice()[0].is_unparsed).toBe(true);
    expect(dest.slice()[0].value).toEqual("var(--fg)");
    expect(h.unparsed_fallback_count).toBe(1 as usize);

    // finalize() 重置槽位后，新规则块中同属性不会覆盖上一个块的内容
    h.finalize();
    let dest2 = CssFallbackDeclList.init();
    const fbs: [1][]const u8 = ["rgb(0,0,0)"];
    * = h.handleProperty(.Color, "lab(50% 40 59)", fbs[0..], &dest2)?;
    expect(dest2.count).toBe(2 as usize);
});

test("cssFallback: empty value and small buffer guards", () => {
    let h = CssFallbackHandler.init(0);
    let dest = CssFallbackDeclList.init();
    const empty_fb: [0][]const u8 = [];

    expectError(h.handleProperty(.Color, "", empty_fb[0..], &dest), CssFallbackError.EmptyValue);
    expectError(h.handleUnparsed(.Color, "", &dest), CssFallbackError.EmptyValue);

    * = h.handleProperty(.TextShadow, "0 2px 4px #000", empty_fb[0..], &dest)?;
    let tiny: [6]u8 = [0; 6];
    expectError(dest.formatCss(tiny[0..], true), CssFallbackError.BufferTooSmall);
});
