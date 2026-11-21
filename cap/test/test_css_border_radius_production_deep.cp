import { test, expect } from "cap:test";
import { BorderRadius } from "cap:css_border_radius";

test("css_border_radius: 1-4 value expansion and shortest canonical toCss", () => {
    let buf: [128]u8 = undefined;

    const r1 = BorderRadius.parse("8px 8px 8px 8px").?;
    expect(r1.isUniform()).toBeTrue();
    expect(r1.isCircular()).toBeTrue();
    expect(r1.toCss(buf[0..])).toBe("8px");

    const r2 = BorderRadius.parse("10px 20px").?;
    expect(r2.top_left.x.value).toBe(10.0);
    expect(r2.top_right.x.value).toBe(20.0);
    expect(r2.bottom_right.x.value).toBe(10.0);
    expect(r2.bottom_left.x.value).toBe(20.0);
    expect(r2.toCss(buf[0..])).toBe("10px 20px");

    const r3 = BorderRadius.parse("4px 8px 12px 8px").?;
    expect(r3.toCss(buf[0..])).toBe("4px 8px 12px");
});

test("css_border_radius: elliptical '/' syntax and error rejection", () => {
    let buf: [128]u8 = undefined;

    const e = BorderRadius.parse("10px 20px / 5px 15px").?;
    expect(e.isCircular()).toBeFalse();
    expect(e.top_left.x.value).toBe(10.0);
    expect(e.top_left.y.value).toBe(5.0);
    expect(e.toCss(buf[0..])).toBe("10px 20px / 5px 15px");

    // Equal x and y elides '/ ...'
    const eq = BorderRadius.parse("50% / 50%").?;
    expect(eq.isCircular()).toBeTrue();
    expect(eq.toCss(buf[0..])).toBe("50%");

    // Negative radius, double slash, and >4 values rejected
    expect(BorderRadius.parse("-4px") == null).toBeTrue();
    expect(BorderRadius.parse("4px / 4px / 4px") == null).toBeTrue();
    expect(BorderRadius.parse("1px 2px 3px 4px 5px") == null).toBeTrue();
});
