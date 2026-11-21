import { test, expect } from "cap:test";
import { CssPosition, HorizontalSide, VerticalSide } from "cap:css_position";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("css_position: 1-value and 2-value forms with canonical serialization", () => {
    let buf: [64]u8 = [0; 64];

    const p1 = CssPosition.parse("center");
    expect(p1 != null && p1.?.isCenter());
    expect(strEq(p1.?.toCss(buf[0..]), "50%"));

    const p2 = CssPosition.parse("bottom right");
    expect(p2 != null);
    expect(p2.?.x.side == HorizontalSide.right && p2.?.y.side == VerticalSide.bottom);
    expect(strEq(p2.?.toCss(buf[0..]), "100% 100%"));

    const p3 = CssPosition.parse("left");
    expect(p3 != null);
    expect(strEq(p3.?.toCss(buf[0..]), "0"));

    const p4 = CssPosition.parse("25% 75%");
    expect(p4 != null);
    expect(strEq(p4.?.toCss(buf[0..]), "25% 75%"));

    // Invalid: vertical keyword followed by length in 2-value form
    expect(CssPosition.parse("top 20px") == null);
    expect(CssPosition.parse("left right") == null);
});

test("css_position: 3-value and 4-value side-offset forms", () => {
    let buf: [64]u8 = [0; 64];

    const p1 = CssPosition.parse("right 20px bottom 10px");
    expect(p1 != null);
    expect(strEq(p1.?.toCss(buf[0..]), "right 20px bottom 10px"));

    const p2 = CssPosition.parse("bottom 10px left 15px");
    expect(p2 != null);
    expect(strEq(p2.?.toCss(buf[0..]), "left 15px bottom 10px"));

    const p3 = CssPosition.parse("right 12px center");
    expect(p3 != null);
    expect(strEq(p3.?.toCss(buf[0..]), "right 12px center"));

    const p4 = CssPosition.parse("center bottom 8px");
    expect(p4 != null);
    expect(strEq(p4.?.toCss(buf[0..]), "center bottom 8px"));

    expect(CssPosition.parse("10px 20px 30px") == null);
    expect(CssPosition.parse("left 1px right 2px") == null);
});
