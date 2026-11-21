// Source: cp/cap/test/test_css_gradient_production_deep.cp (Production deep tests for cap:css_gradient)

import { test, expect_eq } from "cap:test";
import {
    CssGradient as Gradient,
    CssGradientKind as GradientKind,
    DirectionSide,
    LineDirection,
    RadialShape,
    RadialExtent,
} from "cap:css";

test("Linear and repeating-linear gradients, directions and default omission") {
    let buf: [128]u8 = [0; 128];

    // 默认方向 to bottom 与 180deg 在 toCss 中自动省略
    const g1 = Gradient.parse("linear-gradient(to bottom, #fff, #000)").?;
    expect_eq(g1.kind == .linear, true);
    expect_eq(g1.isRepeating(), false);
    expect_eq(g1.stopCount(), @as(usize, 2));
    expect_eq(g1.linear.direction.isDefault(), true);
    expect_eq(std.mem.eql(u8, g1.toCss(buf[0..128]), "linear-gradient(#fff, #000)"), true);

    const g180 = Gradient.parse("linear-gradient(180deg, red, blue)").?;
    expect_eq(g180.linear.direction.isDefault(), true);
    expect_eq(std.mem.eql(u8, g180.toCss(buf[0..128]), "linear-gradient(red, blue)"), true);

    // 任意顺序角方向 "to left top" -> 规范输出 "to top left"
    const g_corner = Gradient.parse("repeating-linear-gradient(to left top, red 0px, blue 20px)").?;
    expect_eq(g_corner.kind == .repeating_linear, true);
    expect_eq(g_corner.isRepeating(), true);
    expect_eq(g_corner.linear.direction.side == .to_top_left, true);
    expect_eq(
        std.mem.eql(u8, g_corner.toCss(buf[0..128]), "repeating-linear-gradient(to top left, red 0px, blue 20px)"),
        true,
    );

    // turn 角度换算：0.25turn -> 90deg
    const g_turn = Gradient.parse("linear-gradient(0.25turn, cyan, magenta)").?;
    expect_eq(std.mem.eql(u8, g_turn.toCss(buf[0..128]), "linear-gradient(90deg, cyan, magenta)"), true);
}

test("Double-position color stops and nested rgb()/rgba() color functions") {
    let buf: [160]u8 = [0; 160];
    const g = Gradient.parse("linear-gradient(45deg, red 10% 30%, rgba(0, 128, 255, 0.5) 50%, green 80% 100%)").?;
    expect_eq(g.stopCount(), @as(usize, 3));

    const stops = g.linear.stopSlice();
    expect_eq(stops[0].color, "red");
    expect_eq(stops[0].pos1, "10%");
    expect_eq(stops[0].pos2, "30%");
    expect_eq(stops[0].isDoublePosition(), true);

    expect_eq(stops[1].color, "rgba(0, 128, 255, 0.5)");
    expect_eq(stops[1].pos1, "50%");
    expect_eq(stops[1].isDoublePosition(), false);

    expect_eq(stops[2].isDoublePosition(), true);
    expect_eq(
        std.mem.eql(u8, g.toCss(buf[0..160]), "linear-gradient(45deg, red 10% 30%, rgba(0, 128, 255, 0.5) 50%, green 80% 100%)"),
        true,
    );
}

test("Radial and repeating-radial gradients with shape, extent and position") {
    let buf: [128]u8 = [0; 128];

    const r1 = Gradient.parse("radial-gradient(circle closest-side at 20% 80%, white, black)").?;
    expect_eq(r1.kind == .radial, true);
    expect_eq(r1.radial.shape == .circle, true);
    expect_eq(r1.radial.extent == .closest_side, true);
    expect_eq(r1.radial.position, "20% 80%");
    expect_eq(
        std.mem.eql(u8, r1.toCss(buf[0..128]), "radial-gradient(circle closest-side at 20% 80%, white, black)"),
        true,
    );

    // 默认 ellipse farthest-corner at center 在 toCss 中自动省略
    const r2 = Gradient.parse("repeating-radial-gradient(ellipse farthest-corner at center, red 0%, blue 10%)").?;
    expect_eq(r2.isRepeating(), true);
    expect_eq(
        std.mem.eql(u8, r2.toCss(buf[0..128]), "repeating-radial-gradient(red 0%, blue 10%)"),
        true,
    );
}

test("Conic and repeating-conic gradients with from <angle> and at <position>") {
    let buf: [128]u8 = [0; 128];

    // from 45deg at center -> 省略默认 at center
    const c1 = Gradient.parse("conic-gradient(from 45deg at center, red, yellow, green)").?;
    expect_eq(c1.kind == .conic, true);
    expect_eq(c1.conic.has_from_angle, true);
    expect_eq(@abs(c1.conic.from_angle_deg - 45.0) <= 0.0001, true);
    expect_eq(c1.conic.position, "center");
    expect_eq(
        std.mem.eql(u8, c1.toCss(buf[0..128]), "conic-gradient(from 45deg, red, yellow, green)"),
        true,
    );

    // 非默认位置与 repeating-conic
    const c2 = Gradient.parse("repeating-conic-gradient(from 90deg at 25% 75%, #f00 0deg 30deg, #00f 30deg 60deg)").?;
    expect_eq(c2.isRepeating(), true);
    expect_eq(
        std.mem.eql(u8, c2.toCss(buf[0..128]), "repeating-conic-gradient(from 90deg at 25% 75%, #f00 0deg 30deg, #00f 30deg 60deg)"),
        true,
    );

    // 默认 from 0deg 省略
    const c3 = Gradient.parse("conic-gradient(from 0deg, black, white)").?;
    expect_eq(std.mem.eql(u8, c3.toCss(buf[0..128]), "conic-gradient(black, white)"), true);
}

test("LineDirection, RadialShape and RadialExtent standalone helpers") {
    expect_eq(LineDirection.parse("to top").?.toDegrees(), @as(f64, 0.0));
    expect_eq(LineDirection.parse("to right").?.toDegrees(), @as(f64, 90.0));
    expect_eq(LineDirection.parse("to bottom").?.toDegrees(), @as(f64, 180.0));
    expect_eq(LineDirection.parse("to left").?.toDegrees(), @as(f64, 270.0));
    expect_eq(LineDirection.parse("to bottom right").?.toDegrees(), @as(f64, 135.0));
    expect_eq(LineDirection.parse("to top bottom") == null, true);

    expect_eq(RadialShape.fromStr("circle").?.asStr(), "circle");
    expect_eq(RadialShape.fromStr("square") == null, true);
    expect_eq(RadialExtent.fromStr("closest-corner").?.asStr(), "closest-corner");
    expect_eq(RadialExtent.fromStr("unknown") == null, true);
}

test("Malformed gradient inputs and small output buffer boundary guards") {
    expect_eq(Gradient.parse("") == null, true);
    expect_eq(Gradient.parse("unknown-gradient(red, blue)") == null, true);
    expect_eq(Gradient.parse("linear-gradient(red, blue") == null, true);
    expect_eq(Gradient.parse("linear-gradient(red)") == null, true);
    expect_eq(Gradient.parse("linear-gradient(to right, red)") == null, true);
    expect_eq(Gradient.parse("linear-gradient(, red, blue)") == null, true);
    expect_eq(Gradient.parse("linear-gradient(rgba(1,2,3, red)") == null, true);

    // 序列化缓冲不足安全返回空切片不越界
    const g = Gradient.parse("linear-gradient(red, blue)").?;
    let tiny: [5]u8 = [0; 5];
    expect_eq(g.toCss(tiny[0..5]).len, @as(usize, 0));
}
