import { test, expect } from "cap:test";
import {
    BoxSizing,
    Size,
    MaxSize,
    AspectRatio,
    BoxInsets,
    LogicalInsets,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("css_sizing: BoxSizing, Size, MaxSize with vendor prefixes and fit-content()", () => {
    let buf: [64]u8 = [0; 64];

    expect(BoxSizing.parse("border-box") == BoxSizing.border_box);
    expect(BoxSizing.parse("content-box") == BoxSizing.content_box);
    expect(BoxSizing.parse("padding-box") == null);

    const s1 = Size.parse("fit-content(250px)");
    expect(s1 != null);
    expect(strEq(s1.?.toCss(buf[0..]), "fit-content(250px)"));

    const s2 = Size.parse("-webkit-fill-available");
    expect(s2 != null);
    expect(strEq(s2.?.toCss(buf[0..]), "-webkit-fill-available"));

    const s3 = Size.parse("-moz-max-content");
    expect(s3 != null);
    expect(strEq(s3.?.toCss(buf[0..]), "-moz-max-content"));

    const s4 = Size.parse("50%");
    expect(s4 != null);
    expect(strEq(s4.?.toCss(buf[0..]), "50%"));

    // Negative size is invalid
    expect(Size.parse("-10px") == null);

    const m1 = MaxSize.parse("none");
    expect(m1 != null);
    expect(strEq(m1.?.toCss(buf[0..]), "none"));

    const m2 = MaxSize.parse("fit-content(80%)");
    expect(m2 != null);
    expect(strEq(m2.?.toCss(buf[0..]), "fit-content(80%)"));

    expect(MaxSize.parse("auto") == null);
});

test("css_sizing: AspectRatio parsing and serialization", () => {
    let buf: [64]u8 = [0; 64];

    const r1 = AspectRatio.parse("16 / 9");
    expect(r1 != null);
    expect(strEq(r1.?.toCss(buf[0..]), "16 / 9"));

    const r2 = AspectRatio.parse("auto 4/3");
    expect(r2 != null);
    expect(strEq(r2.?.toCss(buf[0..]), "auto 4 / 3"));

    const r3 = AspectRatio.parse("2 auto");
    expect(r3 != null);
    expect(strEq(r3.?.toCss(buf[0..]), "auto 2"));

    const r4 = AspectRatio.parse("auto");
    expect(r4 != null);
    expect(strEq(r4.?.toCss(buf[0..]), "auto"));

    expect(AspectRatio.parse("16 / 0") == null);
    expect(AspectRatio.parse("16 /") == null);
    expect(AspectRatio.parse("-1 / 2") == null);
});

test("css_sizing: BoxInsets and LogicalInsets shorthand compression", () => {
    let buf: [64]u8 = [0; 64];

    const bi1 = BoxInsets.parse("10px auto 10px auto");
    expect(bi1 != null);
    expect(strEq(bi1.?.toCss(buf[0..]), "10px auto"));

    const bi2 = BoxInsets.parse("-8px 16px 0px 16px");
    expect(bi2 != null);
    expect(strEq(bi2.?.toCss(buf[0..]), "-8px 16px 0"));

    const bi3 = BoxInsets.parse("1rem 2rem 3rem 4rem");
    expect(bi3 != null);
    expect(strEq(bi3.?.toCss(buf[0..]), "1rem 2rem 3rem 4rem"));

    const li1 = LogicalInsets.parse("12px 12px");
    expect(li1 != null);
    expect(strEq(li1.?.toCss(buf[0..]), "12px"));

    const li2 = LogicalInsets.parse("auto 24px");
    expect(li2 != null);
    expect(strEq(li2.?.toCss(buf[0..]), "auto 24px"));

    expect(BoxInsets.parse("1px 2px 3px 4px 5px") == null);
    expect(LogicalInsets.parse("1px 2px 3px") == null);
});
