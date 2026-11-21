// Source: cp/cap/test/test_css_units_production_deep.cp (Production deep tests for cap:css_units)

import { test, expect_eq } from "cap:test";
import {
    Angle,
    AngleUnit,
    Resolution,
    ResolutionUnit,
} from "cap:css_units";

fn approxEq(a: f64, b: f64): bool {
    return @abs(a - b) <= 0.0001;
}

test("Angle 4-unit parsing and cross-unit conversions") {
    const a_deg = Angle.parse("180deg") catch unreachable;
    const a_rad = Angle.parse("3.141592653589793rad") catch unreachable;
    const a_grad = Angle.parse("200GRAD") catch unreachable;
    const a_turn = Angle.parse("  0.5Turn  ") catch unreachable;

    expect_eq(a_deg.unit == .deg, true);
    expect_eq(a_rad.unit == .rad, true);
    expect_eq(a_grad.unit == .grad, true);
    expect_eq(a_turn.unit == .turn, true);

    expect_eq(a_deg.eql(a_rad), true);
    expect_eq(a_deg.eql(a_grad), true);
    expect_eq(a_deg.eql(a_turn), true);

    expect_eq(approxEq(a_turn.toDegrees(), 180.0), true);
    expect_eq(approxEq(a_turn.toRadians(), 3.141592653589793), true);
    expect_eq(approxEq(a_turn.toGradians(), 200.0), true);
    expect_eq(approxEq(a_deg.toTurns(), 0.5), true);
    expect_eq(approxEq(Angle.deg(90.0).toGradians(), 100.0), true);
}

test("Angle parseWithUnitlessZero and normalizeDegrees [0, 360)") {
    // 严格 parse 拒绝无单位 0，parseWithUnitlessZero 接受 0 / -0 / 0.0
    const strict_zero = switch (Angle.parse("0")) {
        err.MissingUnit => true,
        else => false,
    };
    expect_eq(strict_zero, true);

    const z1 = Angle.parseWithUnitlessZero("0") catch unreachable;
    const z2 = Angle.parseWithUnitlessZero("-0.0") catch unreachable;
    expect_eq(z1.isZero(), true);
    expect_eq(z2.isZero(), true);
    expect_eq(z1.unit == .deg, true);

    // 非零无单位在两种模式下均返回 MissingUnit
    const non_zero_unitless = switch (Angle.parseWithUnitlessZero("45")) {
        err.MissingUnit => true,
        else => false,
    };
    expect_eq(non_zero_unitless, true);

    // normalizeDegrees 归一化到 [0, 360)
    expect_eq(approxEq(Angle.deg(-90.0).normalizeDegrees(), 270.0), true);
    expect_eq(approxEq(Angle.deg(-360.0).normalizeDegrees(), 0.0), true);
    expect_eq(approxEq(Angle.deg(450.0).normalizeDegrees(), 90.0), true);
    expect_eq(approxEq(Angle.deg(720.0).normalizeDegrees(), 0.0), true);
    expect_eq(approxEq(Angle.turn(-0.25).normalizeDegrees(), 270.0), true);
}

test("Angle arithmetic operations (same-unit preservation and cross-unit promotion)") {
    const t_sum = Angle.turn(0.25).add(Angle.turn(0.5));
    expect_eq(t_sum.unit == .turn, true);
    expect_eq(approxEq(t_sum.value, 0.75), true);

    const cross_sum = Angle.deg(90.0).add(Angle.turn(0.25));
    expect_eq(cross_sum.unit == .deg, true);
    expect_eq(approxEq(cross_sum.toDegrees(), 180.0), true);

    const diff = Angle.grad(200.0).sub(Angle.deg(90.0));
    expect_eq(diff.unit == .deg, true);
    expect_eq(approxEq(diff.toDegrees(), 90.0), true);

    const scaled = Angle.deg(45.0).scale(2.0);
    expect_eq(scaled.unit == .deg, true);
    expect_eq(approxEq(scaled.value, 90.0), true);
}

test("Resolution 4 aliases (dpi/dpcm/dppx/x) parsing and cross-unit conversion") {
    const r_x = Resolution.parse("2x") catch unreachable;
    const r_dppx = Resolution.parse("2DPPX") catch unreachable;
    const r_dpi = Resolution.parse("192dpi") catch unreachable;
    const r_dpcm = Resolution.parse("75.590551dpcm") catch unreachable;

    expect_eq(r_x.unit == .dppx, true);
    expect_eq(approxEq(r_x.value, 2.0), true);
    expect_eq(r_x.eql(r_dppx), true);
    expect_eq(r_x.eql(r_dpi), true);
    expect_eq(r_x.eql(r_dpcm), true);

    expect_eq(approxEq(r_x.toDpi(), 192.0), true);
    expect_eq(approxEq(r_dpi.toDppx(), 2.0), true);
    expect_eq(approxEq(Resolution.dpi(254.0).toDpcm(), 100.0), true);
    expect_eq(approxEq(Resolution.dpcm(100.0).toDpi(), 254.0), true);
}

test("Canonical toCss serialization and BufferTooSmall guard") {
    let buf: [32]u8 = [0; 32];

    expect_eq(Angle.deg(90.0).toCss(buf[0..32]) catch unreachable, "90deg");
    expect_eq(Angle.turn(0.5).toCss(buf[0..32]) catch unreachable, "0.5turn");
    expect_eq(Angle.deg(-45.25).toCss(buf[0..32]) catch unreachable, "-45.25deg");
    expect_eq(Angle.deg(-0.0).toCss(buf[0..32]) catch unreachable, "0deg");
    expect_eq(Angle.deg(0.0).toCssWithUnitlessZero(buf[0..32]) catch unreachable, "0");

    expect_eq(Resolution.dppx(2.0).toCss(buf[0..32]) catch unreachable, "2dppx");
    expect_eq(Resolution.dppx(2.0).toCssShort(buf[0..32]) catch unreachable, "2x");
    expect_eq(Resolution.dpi(96.0).toCss(buf[0..32]) catch unreachable, "96dpi");

    let tiny: [3]u8 = [0; 3];
    const small_err = switch (Angle.deg(90.0).toCss(tiny[0..3])) {
        err.BufferTooSmall => true,
        else => false,
    };
    expect_eq(small_err, true);
}

test("Malformed input and negative resolution boundary guards") {
    expect_eq(switch (Angle.parse("")) { err.EmptyInput => true, else => false }, true);
    expect_eq(switch (Angle.parse("   ")) { err.EmptyInput => true, else => false }, true);
    expect_eq(switch (Angle.parse("deg")) { err.InvalidNumber => true, else => false }, true);
    expect_eq(switch (Angle.parse(".deg")) { err.InvalidNumber => true, else => false }, true);
    expect_eq(switch (Angle.parse("180")) { err.MissingUnit => true, else => false }, true);
    expect_eq(switch (Angle.parse("180px")) { err.UnknownUnit => true, else => false }, true);

    expect_eq(switch (Resolution.parse("-2x")) { err.NegativeResolution => true, else => false }, true);
    expect_eq(switch (Resolution.parse("-96dpi")) { err.NegativeResolution => true, else => false }, true);
    expect_eq(switch (Resolution.parse("300")) { err.MissingUnit => true, else => false }, true);
    expect_eq(switch (Resolution.parse("300ppi")) { err.UnknownUnit => true, else => false }, true);
}
