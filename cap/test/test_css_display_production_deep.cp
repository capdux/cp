import { test, expect } from "cap:test";
import {
    Visibility,
    DisplayKeyword,
    DisplayOutside,
    DisplayInside,
    DisplayPair,
    Display,
} from "cap:css_display";

test("css_display: Visibility and DisplayKeyword parse/toCss", () => {
    expect(Visibility.parse("HIDDEN").? == .hidden).toBeTrue();
    expect(Visibility.collapse.toCss()).toBe("collapse");
    expect(Visibility.parse("bogus") == null).toBeTrue();

    const d = Display.parse("none").?;
    expect(d.isNone()).toBeTrue();
    let buf: [64]u8 = undefined;
    expect(d.toCssBuf(buf[0..])).toBe("none");

    const tc = DisplayKeyword.parse("table-cell").?;
    expect(tc.isTableInternal()).toBeTrue();
    expect(tc.isRubyInternal()).toBeFalse();
    expect(DisplayKeyword.ruby_text.isRubyInternal()).toBeTrue();
});

test("css_display: legacy shorthands and multi-keyword normalization", () => {
    let buf: [64]u8 = undefined;

    const ib = Display.parse("inline flow-root").?;
    expect(ib.pair.outside == .inline_box).toBeTrue();
    expect(ib.pair.inside == .flow_root).toBeTrue();
    expect(ib.toCssBuf(buf[0..])).toBe("inline-block");

    const iflex = Display.parse("inline-flex").?;
    expect(iflex.pair.outside == .inline_box).toBeTrue();
    expect(iflex.pair.inside.isFlex()).toBeTrue();
    expect(iflex.toCssBuf(buf[0..])).toBe("inline-flex");

    const bflex = Display.parse("block flex").?;
    expect(bflex.toCssBuf(buf[0..])).toBe("flex");

    const ruby = Display.parse("ruby").?;
    expect(ruby.pair.outside == .inline_box).toBeTrue();
    expect(ruby.toCssBuf(buf[0..])).toBe("ruby");

    const li = Display.parse("inline list-item").?;
    expect(li.pair.is_list_item).toBeTrue();
    expect(li.toCssBuf(buf[0..])).toBe("inline list-item");
});

test("css_display: invalid combinations rejected", () => {
    // list-item is only valid with flow or flow-root
    expect(Display.parse("flex list-item") == null).toBeTrue();
    expect(Display.parse("grid list-item") == null).toBeTrue();
    expect(Display.parse("block inline") == null).toBeTrue();
    expect(Display.parse("") == null).toBeTrue();
});
