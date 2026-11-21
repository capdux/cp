import { test, expect } from "cap:test";
import { TimeUnit, CssTime } from "cap:css_time";

test("css_time: parse s/ms (case-insensitive) and unit conversions", () => {
    const t1 = CssTime.parse("250ms").?;
    expect(t1.unit == .milliseconds).toBeTrue();
    expect(t1.toMs()).toBe(250.0);
    expect(t1.toSeconds()).toBe(0.25);

    const t2 = CssTime.parse("  1.5S ").?;
    expect(t2.unit == .seconds).toBeTrue();
    expect(t2.toMs()).toBe(1500.0);

    const t3 = CssTime.parse("-50MS").?;
    expect(t3.toMs()).toBe(-50.0);

    expect(CssTime.parse("10px") == null).toBeTrue();
    expect(CssTime.parse("s") == null).toBeTrue();
    expect(CssTime.parse("") == null).toBeTrue();
});

test("css_time: arithmetic, cross-unit equality, and shortest-form CSS output", () => {
    const a = CssTime.seconds(0.5);
    const b = CssTime.milliseconds(500.0);
    expect(a.eql(b)).toBeTrue();
    expect(a.cmp(b)).toBe(0);

    const sum = a.add(CssTime.milliseconds(250.0));
    expect(sum.unit == .seconds).toBeTrue();
    expect(sum.toMs()).toBe(750.0);

    const scaled = CssTime.milliseconds(20.0).mul(2.5);
    expect(scaled.toMs()).toBe(50.0);

    let buf: [32]u8 = undefined;
    expect(CssTime.milliseconds(0.0).toCss(buf[0..])).toBe("0s");
    expect(CssTime.seconds(0.05).toCss(buf[0..])).toBe("50ms");
    expect(CssTime.milliseconds(500.0).toCss(buf[0..])).toBe("0.5s");
});
