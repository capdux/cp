import { test, expect } from "cap:test";
import {
    LineStyle,
    BorderSideWidth,
    BorderColorValue,
    Border,
    BorderWidth,
    CssBorderStyle as BorderStyle,
    BorderColor,
    BorderLogicalWidth,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("css_border: Border shorthand order-independent parse & default elision", () => {
    let buf: [64]u8 = [0; 64];

    const b1 = Border.parse("#ff0000 solid 2px");
    expect(b1 != null);
    expect(strEq(b1.?.toCss(buf[0..]), "2px solid #ff0000"));

    // Defaults omitted: medium + currentcolor -> only "dashed"
    const b2 = Border.parse("dashed currentcolor medium");
    expect(b2 != null);
    expect(strEq(b2.?.toCss(buf[0..]), "dashed"));

    // All default -> "none"
    const b3 = Border.parse("medium none currentcolor");
    expect(b3 != null);
    expect(strEq(b3.?.toCss(buf[0..]), "none"));

    // Hex 3-digit + 0 width
    const b4 = Border.parse("0 solid #0f8");
    expect(b4 != null);
    expect(strEq(b4.?.toCss(buf[0..]), "0 solid #00ff88"));

    // Invalid: duplicate style, negative width, unknown token
    expect(Border.parse("solid dashed") == null);
    expect(Border.parse("-2px solid red") == null);
    expect(Border.parse("2px bogus red") == null);
    expect(Border.parse("") == null);
});

test("css_border: BorderWidth/BorderStyle/BorderColor 1-4 value compression", () => {
    let buf: [128]u8 = [0; 128];

    const w1 = BorderWidth.parse("1px 2px 1px 2px");
    expect(w1 != null);
    expect(strEq(w1.?.toCss(buf[0..]), "1px 2px"));

    const w2 = BorderWidth.parse("thin thick thin");
    expect(w2 != null);
    expect(strEq(w2.?.toCss(buf[0..]), "thin thick"));

    const w3 = BorderWidth.parse("1px 2px 3px 2px");
    expect(w3 != null);
    expect(strEq(w3.?.toCss(buf[0..]), "1px 2px 3px"));

    const st = BorderStyle.parse("solid dashed dotted double");
    expect(st != null);
    expect(strEq(st.?.toCss(buf[0..]), "solid dashed dotted double"));

    const st_same = BorderStyle.parse("groove groove groove groove");
    expect(st_same != null);
    expect(strEq(st_same.?.toCss(buf[0..]), "groove"));

    const col = BorderColor.parse("red #00ff00 blue transparent");
    expect(col != null);
    expect(strEq(col.?.toCss(buf[0..]), "#ff0000 #00ff00 #0000ff transparent"));

    expect(BorderWidth.parse("1px 2px 3px 4px 5px") == null);
    expect(BorderStyle.parse("solid unknown") == null);
});

test("css_border: BorderLogicalWidth and BorderSideWidth/Color helpers", () => {
    let buf: [64]u8 = [0; 64];

    const lw1 = BorderLogicalWidth.parse("1.5rem 1.5rem");
    expect(lw1 != null);
    expect(strEq(lw1.?.toCss(buf[0..]), "1.5rem"));

    const lw2 = BorderLogicalWidth.parse("thin 4px");
    expect(lw2 != null);
    expect(strEq(lw2.?.toCss(buf[0..]), "thin 4px"));

    const c4 = BorderColorValue.parse("#f0a8");
    expect(c4 != null);
    expect(strEq(c4.?.toCss(buf[0..]), "#ff00aa88"));

    expect(LineStyle.parse("DOUBLE") == LineStyle.double);
    expect(BorderSideWidth.parse("2.25em") != null);
    expect(BorderLogicalWidth.parse("1px 2px 3px") == null);
});
