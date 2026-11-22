// Deep production regression tests for cap:css Box Alignment Level 3 (lib/css/boxAlign.cp)

import { test, expect, expectError } from "cap:test";
import {
    CssBoxAlignError,
    CssOverflowPosition,
    CssBaselinePosition,
    CssContentDistribution,
    CssContentPosition,
    CssSelfPosition,
    CssLegacyJustify,
    CssBoxContentAlign,
    CssBoxItemAlign,
    CssPlaceContent,
    CssPlaceItems,
    CssPlaceSelf,
} from "cap:css";

test("cssBoxAlign: align-content and justify-content with overflow modifiers", () => {
    let buf: [64]u8 = [0; 64];

    const ac1 = CssBoxContentAlign.parseAlignContent("safe flex-end")?;
    expect(ac1.kind == .Position).toBe(true);
    expect(ac1.position == .FlexEnd).toBe(true);
    const n1 = ac1.toCss(buf[0..])?;
    expect(buf[0..n1]).toEqual("safe flex-end");

    const ac2 = CssBoxContentAlign.parseAlignContent("last baseline")?;
    expect(ac2.kind == .Baseline).toBe(true);
    expect(ac2.baseline == .Last).toBe(true);
    const n2 = ac2.toCss(buf[0..])?;
    expect(buf[0..n2]).toEqual("last baseline");

    // first baseline 规范化输出为 baseline
    const ac3 = CssBoxContentAlign.parseAlignContent("first baseline")?;
    const n3 = ac3.toCss(buf[0..])?;
    expect(buf[0..n3]).toEqual("baseline");

    // justify-content 允许 left/right 且支持 unsafe 修饰符
    const jc = CssBoxContentAlign.parseJustifyContent("unsafe right")?;
    expect(jc.position == .Right).toBe(true);
    const n4 = jc.toCss(buf[0..])?;
    expect(buf[0..n4]).toEqual("unsafe right");

    // align-content 禁止 left/right
    expectError(CssBoxContentAlign.parseAlignContent("left"), CssBoxAlignError.InvalidKeyword);
    // justify-content 禁止 baseline
    expectError(CssBoxContentAlign.parseJustifyContent("baseline"), CssBoxAlignError.InvalidKeyword);
});

test("cssBoxAlign: align-items, justify-items and legacy keyword ordering", () => {
    let buf: [64]u8 = [0; 64];

    const ai = CssBoxItemAlign.parseAlignItems("unsafe self-start")?;
    expect(ai.kind == .SelfPosition).toBe(true);
    expect(ai.self_pos == .SelfStart).toBe(true);
    const n1 = ai.toCss(buf[0..])?;
    expect(buf[0..n1]).toEqual("unsafe self-start");

    // justify-items 支持 `legacy center` 与 `center legacy` 两种书写顺序
    const ji1 = CssBoxItemAlign.parseJustifyItems("legacy center")?;
    const ji2 = CssBoxItemAlign.parseJustifyItems("center legacy")?;
    expect(ji1.eql(ji2)).toBe(true);
    expect(ji1.legacy == .Center).toBe(true);
    const n2 = ji1.toCss(buf[0..])?;
    expect(buf[0..n2]).toEqual("legacy center");

    const ji3 = CssBoxItemAlign.parseJustifyItems("legacy")?;
    expect(ji3.legacy == .LegacyOnly).toBe(true);

    // align-items 禁止 auto 与 legacy
    expectError(CssBoxItemAlign.parseAlignItems("auto"), CssBoxAlignError.InvalidKeyword);
    expectError(CssBoxItemAlign.parseAlignItems("legacy left"), CssBoxAlignError.InvalidKeyword);
});

test("cssBoxAlign: place-content W3C baseline fallback and shorthand collapsing", () => {
    let buf: [64]u8 = [0; 64];

    // W3C 规范：单值 `place-content: baseline` 时 justify-content 必须回退为 start
    const pc_bl = CssPlaceContent.parse("baseline")?;
    expect(pc_bl.align_val.kind == .Baseline).toBe(true);
    expect(pc_bl.justify_val.kind == .Position).toBe(true);
    expect(pc_bl.justify_val.position == .Start).toBe(true);
    const n1 = pc_bl.toCss(buf[0..])?;
    expect(buf[0..n1]).toEqual("baseline start");

    // 相等双值折叠为单值
    const pc_eq = CssPlaceContent.parse("safe center safe center")?;
    const n2 = pc_eq.toCss(buf[0..])?;
    expect(buf[0..n2]).toEqual("safe center");

    // 不等双值完整保留
    const pc_diff = CssPlaceContent.parse("space-between safe right")?;
    const n3 = pc_diff.toCss(buf[0..])?;
    expect(buf[0..n3]).toEqual("space-between safe right");
});

test("cssBoxAlign: place-items shorthand expansion and minimal serialization", () => {
    let buf: [64]u8 = [0; 64];

    const pi1 = CssPlaceItems.parse("center center")?;
    const n1 = pi1.toCss(buf[0..])?;
    expect(buf[0..n1]).toEqual("center");

    const pi2 = CssPlaceItems.parse("stretch legacy right")?;
    expect(pi2.align_val.kind == .Stretch).toBe(true);
    expect(pi2.justify_val.kind == .Legacy).toBe(true);
    const n2 = pi2.toCss(buf[0..])?;
    expect(buf[0..n2]).toEqual("stretch legacy right");

    const pi3 = CssPlaceItems.parse("last baseline")?;
    expect(pi3.align_val.eql(pi3.justify_val)).toBe(true);
    const n3 = pi3.toCss(buf[0..])?;
    expect(buf[0..n3]).toEqual("last baseline");
});

test("cssBoxAlign: align-self, justify-self and place-self auto/position rules", () => {
    let buf: [64]u8 = [0; 64];

    const as1 = CssBoxItemAlign.parseAlignSelf("auto")?;
    expect(as1.kind == .Auto).toBe(true);

    const js1 = CssBoxItemAlign.parseJustifySelf("safe left")?;
    expect(js1.self_pos == .Left).toBe(true);

    const ps1 = CssPlaceSelf.parse("auto auto")?;
    const n1 = ps1.toCss(buf[0..])?;
    expect(buf[0..n1]).toEqual("auto");

    const ps2 = CssPlaceSelf.parse("self-end safe right")?;
    const n2 = ps2.toCss(buf[0..])?;
    expect(buf[0..n2]).toEqual("self-end safe right");

    // place-self 不允许 legacy
    expectError(CssPlaceSelf.parse("center legacy left"), CssBoxAlignError.InvalidKeyword);
});

test("cssBoxAlign: malformed syntax, trailing tokens and buffer overflow guards", () => {
    expectError(CssBoxContentAlign.parseAlignContent("   "), CssBoxAlignError.EmptyInput);
    expectError(CssBoxContentAlign.parseAlignContent("safe"), CssBoxAlignError.InvalidSyntax);
    expectError(CssBoxContentAlign.parseAlignContent("first"), CssBoxAlignError.InvalidSyntax);
    expectError(CssBoxContentAlign.parseAlignContent("last center"), CssBoxAlignError.InvalidSyntax);
    expectError(CssBoxContentAlign.parseAlignContent("center extra"), CssBoxAlignError.TrailingTokens);
    expectError(CssPlaceItems.parse("center start extra"), CssBoxAlignError.TrailingTokens);

    const pc = CssPlaceContent.parse("space-between safe right")?;
    let tiny: [6]u8 = [0; 6];
    expectError(pc.toCss(tiny[0..]), CssBoxAlignError.BufferTooSmall);
});
