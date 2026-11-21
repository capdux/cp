import { test, expect } from "cap:test";
import {
    PredefinedCounterStyle,
    ListStylePosition,
    MarkerSide,
    ListStyleTypeKind,
    ListStyleType,
    ListStyle,
} from "cap:css";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("css_list_style: ListStyle shorthand and none disambiguation", () => {
    let buf: [128]u8 = [0; 128];

    const l1 = ListStyle.parse("square inside url(\"bullet.svg\")");
    expect(l1 != null);
    expect(l1.?.position == ListStylePosition.inside);
    expect(l1.?.list_type.predefined == PredefinedCounterStyle.square);
    expect(strEq(l1.?.toCss(buf[0..]), "inside url(\"bullet.svg\") square"));

    // Double `none` sets both image and type to none
    const l2 = ListStyle.parse("none none");
    expect(l2 != null && l2.?.list_type.kind == ListStyleTypeKind.none);
    expect(strEq(l2.?.toCss(buf[0..]), "none"));

    // Single `none` with explicit type sets image=none
    const l3 = ListStyle.parse("lower-roman none");
    expect(l3 != null && l3.?.list_type.predefined == PredefinedCounterStyle.lower_roman);
    expect(strEq(l3.?.toCss(buf[0..]), "lower-roman"));

    // Custom string marker
    const l4 = ListStyle.parse("'-> ' inside");
    expect(l4 != null && l4.?.list_type.kind == ListStyleTypeKind.string_marker);
    expect(strEq(l4.?.toCss(buf[0..]), "inside \"-> \""));

    // All default -> "disc"
    const l5 = ListStyle.parse("outside disc");
    expect(l5 != null);
    expect(strEq(l5.?.toCss(buf[0..]), "disc"));

    // Invalid: triple none, duplicate position
    expect(ListStyle.parse("none none none") == null);
    expect(ListStyle.parse("inside outside") == null);
});

test("css_list_style: PredefinedCounterStyle, MarkerSide, custom counter", () => {
    expect(PredefinedCounterStyle.parse("cjk-decimal") == PredefinedCounterStyle.cjk_decimal);
    expect(MarkerSide.parse("match-parent") == MarkerSide.match_parent);

    const custom = ListStyleType.parse("my-counter");
    expect(custom != null && custom.?.kind == ListStyleTypeKind.custom_counter);
    expect(strEq(custom.?.text, "my-counter"));
});
