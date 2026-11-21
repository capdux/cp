import { test, expect } from "cap:test";
import {
    ColorScheme,
    Resize,
    UserSelect,
    CaretShape,
    Appearance,
    CursorKeyword,
} from "cap:css_ui";

test("css_ui: ColorScheme parse, canonical toCss, and invalid syntax rejection", () => {
    let buf: [32]u8 = undefined;

    const c1 = ColorScheme.parse("normal").?;
    expect(c1.isNormal()).toBeTrue();
    expect(c1.toCss(buf[0..])).toBe("normal");

    const c2 = ColorScheme.parse("dark light").?;
    expect(c2.light && c2.dark && !c2.only).toBeTrue();
    expect(c2.toCss(buf[0..])).toBe("light dark");

    const c3 = ColorScheme.parse("only dark").?;
    expect(c3.dark && c3.only && !c3.light).toBeTrue();
    expect(c3.toCss(buf[0..])).toBe("dark only");

    expect(ColorScheme.parse("only") == null).toBeTrue();
    expect(ColorScheme.parse("light only dark") == null).toBeTrue();
    expect(ColorScheme.parse("normal light") == null).toBeTrue();
});

test("css_ui: Resize, UserSelect, CaretShape, Appearance, and CursorKeyword", () => {
    expect(Resize.parse("inline").? == .inline_axis).toBeTrue();
    expect(Resize.inline_axis.toCss()).toBe("inline");

    expect(UserSelect.parse("NONE").? == .none).toBeTrue();
    expect(CaretShape.parse("underscore").? == .underscore).toBeTrue();

    const ap = Appearance.parse("progress-bar").?;
    expect(ap.isCompatAuto()).toBeTrue();
    expect(ap.toCss()).toBe("progress-bar");
    expect(Appearance.none.isCompatAuto()).toBeFalse();

    const cur = CursorKeyword.parse("nwse-resize").?;
    expect(cur.isResize()).toBeTrue();
    expect(cur.toCss()).toBe("nwse-resize");
    expect(CursorKeyword.pointer.isResize()).toBeFalse();
    expect(CursorKeyword.parse("unknown") == null).toBeTrue();
});
