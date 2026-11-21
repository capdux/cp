import { test, expect } from "cap:test";
import { LineStyle, OutlineStyle, CssBoxShadow as BoxShadow } from "cap:css";

test("css_box_shadow: BoxShadow parse, canonical shortest toCss, and validation", () => {
    let buf: [128]u8 = undefined;

    const s1 = BoxShadow.parse("inset 2px 4px 8px 1px rgba(0, 0, 0, 0.25)").?;
    expect(s1.inset).toBeTrue();
    expect(s1.x_offset.value).toBe(2.0);
    expect(s1.y_offset.value).toBe(4.0);
    expect(s1.blur.value).toBe(8.0);
    expect(s1.spread.value).toBe(1.0);
    expect(s1.color).toBe("rgba(0, 0, 0, 0.25)");
    expect(s1.toCss(buf[0..])).toBe("inset 2px 4px 8px 1px rgba(0, 0, 0, 0.25)");

    // Zero blur/spread and default currentcolor elided
    const s2 = BoxShadow.parse("#000 0 2px 0px 0px").?;
    expect(s2.toCss(buf[0..])).toBe("0 2px #000");

    // Negative blur rejected per CSS Backgrounds spec
    expect(BoxShadow.parse("2px 2px -4px red") == null).toBeTrue();
    expect(BoxShadow.parse("2px") == null).toBeTrue();
    expect(BoxShadow.parse("") == null).toBeTrue();
});

test("css_box_shadow: LineStyle and OutlineStyle", () => {
    expect(LineStyle.parse("dashed").? == .dashed).toBeTrue();
    expect(LineStyle.dashed.isVisible()).toBeTrue();
    expect(LineStyle.none.isVisible()).toBeFalse();
    expect(LineStyle.hidden.isVisible()).toBeFalse();

    expect(OutlineStyle.parse("AUTO").? == .auto).toBeTrue();
    expect(OutlineStyle.groove.toCss()).toBe("groove");
    expect(OutlineStyle.parse("hidden") == null).toBeTrue();
});
