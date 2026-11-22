// Deep production regression tests for cap:css marginPad.cp
// Covers: 55-property recognition across 5 families, 1/2/3/4-value rect & axis shorthand minification,
// physical/logical interleaving cascade flush, modern vs legacy vs safari14 browser target lowering,
// symmetric inline optimization (no LTR/RTL branch), asymmetric & var() LTR/RTL fallback emission,
// inset shorthand gating, and malformed input / buffer-too-small error guards.

import { test, expect, expectEq } from "cap:test";
import {
    CssBoxSpacingFamily,
    CssSpacingSide,
    CssSpacingPropRef,
    CssSpacingRect,
    CssSpacingAxis,
    CssSpacingTarget,
    CssSpacingOutput,
    CssBoxSpacingHandler,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("marginPad: recognizes all 5 families and physical/logical sides case-insensitively", fn () {
    const p1 = CssSpacingPropRef.parse("MARGIN-INLINE-START").?;
    expectEq(p1.family, CssBoxSpacingFamily.margin);
    expectEq(p1.side, CssSpacingSide.inline_start);
    expect(strEq(p1.propertyName(), "margin-inline-start"));

    const p2 = CssSpacingPropRef.parse("  padding-block  ").?;
    expectEq(p2.family, CssBoxSpacingFamily.padding);
    expectEq(p2.side, CssSpacingSide.block_shorthand);
    expect(strEq(p2.propertyName(), "padding-block"));

    const p3 = CssSpacingPropRef.parse("top").?;
    expectEq(p3.family, CssBoxSpacingFamily.inset);
    expectEq(p3.side, CssSpacingSide.top);
    expect(strEq(p3.propertyName(), "top"));

    const p4 = CssSpacingPropRef.parse("scroll-margin-block-end").?;
    expectEq(p4.family, CssBoxSpacingFamily.scroll_margin);
    expectEq(p4.side, CssSpacingSide.block_end);

    const p5 = CssSpacingPropRef.parse("scroll-padding-left").?;
    expectEq(p5.family, CssBoxSpacingFamily.scroll_padding);
    expectEq(p5.side, CssSpacingSide.left);

    expect(CssSpacingPropRef.parse("inset-top") == null);
    expect(CssSpacingPropRef.parse("border-top") == null);
    expect(CssSpacingPropRef.parse("") == null);
});

test("marginPad: CssSpacingRect and CssSpacingAxis shorthand parse & minimal serialization", fn () {
    let buf: [128]u8 = [0; 128];

    // 1-value collapse
    const r1 = CssSpacingRect.parse("10px 10px 10px 10px") catch unreachable;
    const n1 = r1.formatCss(buf[0..]) catch unreachable;
    expect(strEq(buf[0..n1], "10px"));

    // 2-value collapse
    const r2 = CssSpacingRect.parse("10px 20px 10px 20px") catch unreachable;
    const n2 = r2.formatCss(buf[0..]) catch unreachable;
    expect(strEq(buf[0..n2], "10px 20px"));

    // 3-value collapse with calc() preserving internal whitespace
    const r3 = CssSpacingRect.parse("calc(10px + 2px) 1rem 0 1rem") catch unreachable;
    const n3 = r3.formatCss(buf[0..]) catch unreachable;
    expect(strEq(buf[0..n3], "calc(10px + 2px) 1rem 0"));

    // 4-value distinct
    const r4 = CssSpacingRect.parse("1px 2px 3px 4px") catch unreachable;
    const n4 = r4.formatCss(buf[0..]) catch unreachable;
    expect(strEq(buf[0..n4], "1px 2px 3px 4px"));

    // Axis 1-value & 2-value collapse
    const a1 = CssSpacingAxis.parse("1.5rem 1.5rem") catch unreachable;
    const na1 = a1.formatCss(buf[0..]) catch unreachable;
    expect(strEq(buf[0..na1], "1.5rem"));

    const a2 = CssSpacingAxis.parse("1rem 2rem") catch unreachable;
    const na2 = a2.formatCss(buf[0..]) catch unreachable;
    expect(strEq(buf[0..na2], "1rem 2rem"));
});

test("marginPad: modern target merges 4 physical sides and 2 logical axis sides into shorthands", fn () {
    let out = CssSpacingOutput.init();
    let h = CssBoxSpacingHandler.init(.margin, CssSpacingTarget.modern());

    const ok1 = h.handleProperty("margin-top", "8px", *out) catch unreachable;
    const ok2 = h.handleProperty("margin-right", "16px", *out) catch unreachable;
    const ok3 = h.handleProperty("margin-bottom", "8px", *out) catch unreachable;
    const ok4 = h.handleProperty("margin-left", "16px", *out) catch unreachable;
    const wrong_fam = h.handleProperty("padding-top", "4px", *out) catch unreachable;
    expect(ok1 && ok2 && ok3 && ok4);
    expect(!wrong_fam);

    * = h.finalize(*out) catch unreachable;
    let buf: [128]u8 = [0; 128];
    const n = out.formatDeclsCss(buf[0..], true) catch unreachable;
    expect(strEq(buf[0..n], "margin:8px 16px"));
    expect(!out.hasDirectionalRules());

    // Modern logical shorthand merge: margin-block-start + margin-block-end -> margin-block
    out.reset();
    * = h.handleProperty("margin-block-start", "1rem", *out) catch unreachable;
    * = h.handleProperty("margin-block-end", "2rem", *out) catch unreachable;
    * = h.handleProperty("margin-inline-start", "auto", *out) catch unreachable;
    * = h.handleProperty("margin-inline-end", "auto", *out) catch unreachable;
    * = h.finalize(*out) catch unreachable;
    const n2 = out.formatDeclsCss(buf[0..], true) catch unreachable;
    expect(strEq(buf[0..n2], "margin-block:1rem 2rem;margin-inline:auto"));
});

test("marginPad: legacy target lowers block to top/bottom and asymmetric inline to LTR/RTL rules", fn () {
    let out = CssSpacingOutput.init();
    let h = CssBoxSpacingHandler.init(.margin, CssSpacingTarget.legacy());

    * = h.handleProperty("margin-block", "1rem 2rem", *out) catch unreachable;
    * = h.handleProperty("margin-inline-start", "1rem", *out) catch unreachable;
    * = h.finalize(*out) catch unreachable;

    let main_buf: [128]u8 = [0; 128];
    let ltr_buf: [128]u8 = [0; 128];
    let rtl_buf: [128]u8 = [0; 128];

    const nm = out.formatDeclsCss(main_buf[0..], true) catch unreachable;
    const nl = out.formatLtrCss(ltr_buf[0..], true) catch unreachable;
    const nr = out.formatRtlCss(rtl_buf[0..], true) catch unreachable;

    expect(strEq(main_buf[0..nm], "margin-top:1rem;margin-bottom:2rem"));
    expect(strEq(ltr_buf[0..nl], "margin-left:1rem"));
    expect(strEq(rtl_buf[0..nr], "margin-right:1rem"));
    expect(out.hasDirectionalRules());
});

test("marginPad: legacy target optimizes symmetric inline-start/end directly to left/right without LTR/RTL", fn () {
    let out = CssSpacingOutput.init();
    let h = CssBoxSpacingHandler.init(.padding, CssSpacingTarget.legacy());

    * = h.handleProperty("padding-inline", "16px", *out) catch unreachable;
    * = h.finalize(*out) catch unreachable;

    let buf: [128]u8 = [0; 128];
    const n = out.formatDeclsCss(buf[0..], true) catch unreachable;
    expect(strEq(buf[0..n], "padding-left:16px;padding-right:16px"));
    expect(!out.hasDirectionalRules());
});

test("marginPad: inset shorthand gating on safari14 vs modern target", fn () {
    // safari14: supports logical longhands, does NOT support `inset` shorthand or `inset-block` shorthand
    let out_s14 = CssSpacingOutput.init();
    let h_s14 = CssBoxSpacingHandler.init(.inset, CssSpacingTarget.safari14());
    * = h_s14.handleProperty("inset", "0 10px", *out_s14) catch unreachable;
    * = h_s14.finalize(*out_s14) catch unreachable;

    let buf: [128]u8 = [0; 128];
    const ns = out_s14.formatDeclsCss(buf[0..], true) catch unreachable;
    expect(strEq(buf[0..ns], "top:0;bottom:0;left:10px;right:10px"));

    // modern: collapses 4 inset sides back into `inset:0 10px`
    let out_mod = CssSpacingOutput.init();
    let h_mod = CssBoxSpacingHandler.init(.inset, CssSpacingTarget.modern());
    * = h_mod.handleProperty("top", "0", *out_mod) catch unreachable;
    * = h_mod.handleProperty("right", "10px", *out_mod) catch unreachable;
    * = h_mod.handleProperty("bottom", "0", *out_mod) catch unreachable;
    * = h_mod.handleProperty("left", "10px", *out_mod) catch unreachable;
    * = h_mod.finalize(*out_mod) catch unreachable;

    const nm = out_mod.formatDeclsCss(buf[0..], true) catch unreachable;
    expect(strEq(buf[0..nm], "inset:0 10px"));
});

test("marginPad: interleaving physical & logical flushes to preserve cascade and var() lowering", fn () {
    let out = CssSpacingOutput.init();
    let h = CssBoxSpacingHandler.init(.margin, CssSpacingTarget.legacy());

    // Interleaved physical -> logical -> physical must preserve cascade order
    * = h.handleProperty("margin-left", "4px", *out) catch unreachable;
    * = h.handleProperty("margin-inline-start", "var(--gap)", *out) catch unreachable;
    * = h.handleProperty("margin-top", "12px", *out) catch unreachable;
    * = h.finalize(*out) catch unreachable;

    let main_buf: [128]u8 = [0; 128];
    let ltr_buf: [128]u8 = [0; 128];
    let rtl_buf: [128]u8 = [0; 128];
    const nm = out.formatDeclsCss(main_buf[0..], true) catch unreachable;
    const nl = out.formatLtrCss(ltr_buf[0..], true) catch unreachable;
    const nr = out.formatRtlCss(rtl_buf[0..], true) catch unreachable;

    expect(strEq(main_buf[0..nm], "margin-left:4px;margin-top:12px"));
    expect(strEq(ltr_buf[0..nl], "margin-left:var(--gap)"));
    expect(strEq(rtl_buf[0..nr], "margin-right:var(--gap)"));
});

test("marginPad: error guards for empty, too many values, unclosed parens, and tiny output buffer", fn () {
    let failed_empty = false;
    * = CssSpacingRect.parse("   ") catch {
        failed_empty = true;
    };
    expect(failed_empty);

    let failed_many = false;
    * = CssSpacingRect.parse("1px 2px 3px 4px 5px") catch {
        failed_many = true;
    };
    expect(failed_many);

    let failed_unclosed = false;
    * = CssSpacingRect.parse("calc(10px + 2px") catch {
        failed_unclosed = true;
    };
    expect(failed_unclosed);

    let failed_axis_many = false;
    * = CssSpacingAxis.parse("1px 2px 3px") catch {
        failed_axis_many = true;
    };
    expect(failed_axis_many);

    const r = CssSpacingRect.parse("10px 20px 30px 40px") catch unreachable;
    let tiny: [4]u8 = [0; 4];
    let failed_buf = false;
    * = r.formatCss(tiny[0..]) catch {
        failed_buf = true;
    };
    expect(failed_buf);
});
