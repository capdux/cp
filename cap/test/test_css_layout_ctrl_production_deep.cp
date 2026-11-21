import { test, expect } from "cap:test";
import {
    OverflowKeyword,
    Overflow,
    TextOverflow,
    Position,
    ContainerType,
    ContainFlags,
    ZIndex,
} from "cap:css_layout_ctrl";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("css_layout_ctrl: Overflow, TextOverflow, Position, ContainerType", () => {
    let buf: [64]u8 = [0; 64];

    const o1 = Overflow.parse("hidden auto");
    expect(o1 != null && o1.?.x == OverflowKeyword.hidden && o1.?.y == OverflowKeyword.auto);
    expect(strEq(o1.?.toCss(buf[0..]), "hidden auto"));

    const o2 = Overflow.parse("clip clip");
    expect(o2 != null);
    expect(strEq(o2.?.toCss(buf[0..]), "clip"));
    expect(Overflow.parse("auto auto auto") == null);

    expect(TextOverflow.parse("ellipsis") == TextOverflow.ellipsis);
    expect(Position.parse("-webkit-sticky").?.isSticky());
    expect(Position.parse("absolute") == Position.absolute);
    expect(ContainerType.parse("inline-size") == ContainerType.inline_size);
});

test("css_layout_ctrl: ContainFlags normalization and ZIndex", () => {
    let buf: [64]u8 = [0; 64];

    const c1 = ContainFlags.parse("paint style layout");
    expect(c1 != null);
    expect(strEq(c1.?.toCss(buf[0..]), "content"));

    const c2 = ContainFlags.parse("size layout style paint");
    expect(c2 != null);
    expect(strEq(c2.?.toCss(buf[0..]), "strict"));

    const c3 = ContainFlags.parse("inline-size layout");
    expect(c3 != null);
    expect(strEq(c3.?.toCss(buf[0..]), "inline-size layout"));

    // Mutually exclusive size + inline-size or duplicate rejected
    expect(ContainFlags.parse("size inline-size") == null);
    expect(ContainFlags.parse("layout layout") == null);

    const z1 = ZIndex.parse("-10");
    expect(z1 != null && !z1.?.is_auto && z1.?.value == -10);
    expect(strEq(z1.?.toCss(buf[0..]), "-10"));

    const z2 = ZIndex.parse("auto");
    expect(z2 != null && z2.?.is_auto);
    expect(strEq(z2.?.toCss(buf[0..]), "auto"));
    expect(ZIndex.parse("1.5") == null);
});
