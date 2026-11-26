import { test, expect_eq } from "cap:test";
import {
    RgbColor,
    HslColor,
    interpolateNumber,
    interpolateRound,
    interpolateDiscrete,
    interpolateRgb,
    interpolateHue,
    interpolateHsl,
    interpolateBasis,
    interpolatePiecewise,
} from "../../packages/@cap/d3-interpolate/src/index.cp";

fn approx(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return d >= -eps && d <= eps;
}

fn main() {
    test("1. @cap/d3-interpolate — numeric and rounded integer interpolation", () => {
        expect_eq(approx(interpolateNumber(10.0, 20.0, 0.25), 12.5, 1.0e-9), true);
        expect_eq(interpolateRound(0, 10, 0.26), 3 as i64);
        expect_eq(interpolateRound(-10, 0, 0.26), -7 as i64);
    });

    test("2. @cap/d3-interpolate — discrete step selection", () => {
        const vals: [4]f64 = [10.0, 20.0, 30.0, 40.0];
        expect_eq(interpolateDiscrete(vals[0..4], 0.0), 10.0);
        expect_eq(interpolateDiscrete(vals[0..4], 0.3), 20.0);
        expect_eq(interpolateDiscrete(vals[0..4], 0.8), 40.0);
        expect_eq(interpolateDiscrete(vals[0..4], 1.0), 40.0);
    });

    test("3. @cap/d3-interpolate — RGBA color interpolation", () => {
        const red = RgbColor { r: 255, g: 0, b: 0, a: 1.0 };
        const blue = RgbColor { r: 0, g: 0, b: 255, a: 0.0 };
        const mid = interpolateRgb(red, blue, 0.5);
        expect_eq(mid.r, 128 as u8);
        expect_eq(mid.g, 0 as u8);
        expect_eq(mid.b, 128 as u8);
        expect_eq(approx(mid.a, 0.5, 1.0e-9), true);
    });

    test("4. @cap/d3-interpolate — shortest-arc hue and HSL interpolation", () => {
        // 350 deg -> 10 deg crosses 0 deg at midpoint
        expect_eq(approx(interpolateHue(350.0, 10.0, 0.5), 0.0, 1.0e-9), true);
        const c0 = HslColor { h: 350.0, s: 1.0, l: 0.2, a: 1.0 };
        const c1 = HslColor { h: 10.0, s: 0.5, l: 0.8, a: 1.0 };
        const m = interpolateHsl(c0, c1, 0.5);
        expect_eq(approx(m.h, 0.0, 1.0e-9), true);
        expect_eq(approx(m.s, 0.75, 1.0e-9), true);
        expect_eq(approx(m.l, 0.5, 1.0e-9), true);
    });

    test("5. @cap/d3-interpolate — uniform cubic B-spline preserves endpoints and midpoint", () => {
        const vals: [3]f64 = [0.0, 10.0, 20.0];
        expect_eq(approx(interpolateBasis(vals[0..3], 0.0), 0.0, 1.0e-9), true);
        expect_eq(approx(interpolateBasis(vals[0..3], 0.5), 10.0, 1.0e-9), true);
        expect_eq(approx(interpolateBasis(vals[0..3], 1.0), 20.0, 1.0e-9), true);
    });

    test("6. @cap/d3-interpolate — piecewise linear interpolation across segments", () => {
        const vals: [3]f64 = [0.0, 100.0, 200.0];
        expect_eq(approx(interpolatePiecewise(vals[0..3], 0.25), 50.0, 1.0e-9), true);
        expect_eq(approx(interpolatePiecewise(vals[0..3], 0.75), 150.0, 1.0e-9), true);
    });
}
