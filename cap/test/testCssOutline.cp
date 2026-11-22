import {
    CssOutlineError,
    CssOutlineStyle,
    CssOutlineWidthKind,
    CssOutlineWidth,
    CssOutline,
} from "cap:css";
import { expect, test } from "cap:test";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("css outline: CssOutlineStyle parse, default, visibility and auto", fn () {
    expect(CssOutlineStyle.default()).toBe(CssOutlineStyle.none);
    expect(CssOutlineStyle.none.isVisible()).toBe(false);
    expect(CssOutlineStyle.hidden.isVisible()).toBe(false);
    expect(CssOutlineStyle.solid.isVisible()).toBe(true);
    expect(CssOutlineStyle.auto.isVisible()).toBe(true);
    expect(CssOutlineStyle.auto.isAuto()).toBe(true);
    expect(CssOutlineStyle.dashed.isAuto()).toBe(false);

    expect(CssOutlineStyle.parse("AUTO") ?? CssOutlineStyle.none).toBe(CssOutlineStyle.auto);
    expect(CssOutlineStyle.parse("Double") ?? CssOutlineStyle.none).toBe(CssOutlineStyle.double);
    expect(CssOutlineStyle.parse("groove") ?? CssOutlineStyle.none).toBe(CssOutlineStyle.groove);
    expect(CssOutlineStyle.parse("invalid") == null).toBe(true);
    expect(sliceEql(CssOutlineStyle.auto.asStr(), "auto")).toBe(true);
    expect(sliceEql(CssOutlineStyle.ridge.asStr(), "ridge")).toBe(true);
});

test("css outline: order-independent shorthand parse and canonical minimal toCss", fn () {
    const o1 = CssOutline.parse("2px solid #09f") catch CssOutline.default();
    const o2 = CssOutline.parse("#09f SOLID 2px") catch CssOutline.default();
    expect(o1.eql(o2)).toBe(true);
    expect(o1.isVisible()).toBe(true);
    expect(o1.isNone()).toBe(false);
    expect(o1.width.kind).toBe(CssOutlineWidthKind.length);
    expect(sliceEql(o1.width.asStr(), "2px")).toBe(true);

    let buf: [64]u8 = [0; 64];
    const n1 = o2.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n1], "2px solid #09f")).toBe(true);

    // Functional color with internal whitespace
    const o3 = CssOutline.parse("rgb(0, 128, 255) dashed thin") catch CssOutline.default();
    const n3 = o3.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n3], "thin dashed rgb(0, 128, 255)")).toBe(true);
});

test("css outline: omits default medium width and currentcolor during minification", fn () {
    let buf: [64]u8 = [0; 64];

    const o_auto = CssOutline.parse("medium auto currentColor") catch CssOutline.default();
    expect(o_auto.width.isDefault()).toBe(true);
    expect(o_auto.isDefaultColor()).toBe(true);
    const n1 = o_auto.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n1], "auto")).toBe(true);

    const o_none = CssOutline.parse("none") catch CssOutline.default();
    expect(o_none.isNone()).toBe(true);
    expect(o_none.isVisible()).toBe(false);
    const n2 = o_none.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n2], "none")).toBe(true);

    const o_width_only = CssOutline.parse("3px") catch CssOutline.default();
    const n3 = o_width_only.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n3], "3px")).toBe(true);
});

test("css outline: CssOutlineWidth keyword and non-negative length validation", fn () {
    const w_thin = (CssOutlineWidth.tryParse("THIN") catch null) ?? CssOutlineWidth.default();
    expect(w_thin.kind).toBe(CssOutlineWidthKind.thin);
    expect(sliceEql(w_thin.asStr(), "thin")).toBe(true);

    const w_zero = (CssOutlineWidth.tryParse("0") catch null) ?? CssOutlineWidth.default();
    expect(w_zero.kind).toBe(CssOutlineWidthKind.length);
    expect(sliceEql(w_zero.asStr(), "0")).toBe(true);

    const w_dec = (CssOutlineWidth.tryParse(".5rem") catch null) ?? CssOutlineWidth.default();
    expect(w_dec.kind).toBe(CssOutlineWidthKind.length);

    let got_neg = false;
    * = CssOutlineWidth.tryParse("-2px") catch |e| {
        got_neg = e == CssOutlineError.NegativeWidth;
    };
    expect(got_neg).toBe(true);
});

test("css outline: rejects empty input, duplicate components, negative width and >3 tokens", fn () {
    let got_empty = false;
    * = CssOutline.parse("   \t ") catch |e| {
        got_empty = e == CssOutlineError.EmptyInput;
    };
    expect(got_empty).toBe(true);

    let got_dup_style = false;
    * = CssOutline.parse("solid dashed") catch |e| {
        got_dup_style = e == CssOutlineError.DuplicateComponent;
    };
    expect(got_dup_style).toBe(true);

    let got_dup_width = false;
    * = CssOutline.parse("thin 2px") catch |e| {
        got_dup_width = e == CssOutlineError.DuplicateComponent;
    };
    expect(got_dup_width).toBe(true);

    let got_dup_color = false;
    * = CssOutline.parse("red blue") catch |e| {
        got_dup_color = e == CssOutlineError.DuplicateComponent;
    };
    expect(got_dup_color).toBe(true);

    let got_neg = false;
    * = CssOutline.parse("-1px solid red") catch |e| {
        got_neg = e == CssOutlineError.NegativeWidth;
    };
    expect(got_neg).toBe(true);

    let got_too_many = false;
    * = CssOutline.parse("1px solid red extra") catch |e| {
        got_too_many = e == CssOutlineError.TooManyTokens;
    };
    expect(got_too_many).toBe(true);
});

test("css outline: buffer too small guard on toCss", fn () {
    const o = CssOutline.parse("2px solid #00aaff") catch CssOutline.default();
    let tiny: [6]u8 = [0; 6];
    let got_small = false;
    * = o.toCss(tiny[0..]) catch |e| {
        got_small = e == CssOutlineError.BufferTooSmall;
    };
    expect(got_small).toBe(true);
});
