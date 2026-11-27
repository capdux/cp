// cap/test/testCapSpline.cp
// Comprehensive unit tests for @cap/spline (Natural Cubic Spline, Bezier, Catmull-Rom)

import { test, expect_eq } from "cap:test";
import {
    Vec2,
    CubicSpline,
    lerp,
    smoothstep,
    bezierQuad,
    bezierCubic,
    catmullRom,
} from "../../packages/splines/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff: f64 = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/spline — cubic spline passes exactly through all control knots", () => {
        const xs: [5]f64 = [0.0, 1.0, 2.5, 4.0, 6.0];
        const ys: [5]f64 = [1.0, 3.0, 0.5, 4.0, 2.0];
        const sp: CubicSpline = CubicSpline.build(xs[0..5], ys[0..5]);

        expect_eq(sp.isValid(), true);
        expect_eq(sp.knotCount(), 5 as usize);

        let i: usize = 0;
        while (i < 5) {
            expect_eq(approxEq(sp.eval(xs[i]), ys[i], 0.000000001), true);
            i += 1;
        }
    });

    test("2. @cap/spline — linear data reproduces exact values and constant derivative", () => {
        // y = 2.5 * x - 1.0
        const xs: [4]f64 = [-2.0, 0.0, 3.0, 10.0];
        const ys: [4]f64 = [-6.0, -1.0, 6.5, 24.0];
        const sp: CubicSpline = CubicSpline.build(xs[0..4], ys[0..4]);

        expect_eq(sp.isValid(), true);
        expect_eq(approxEq(sp.eval(1.5), 2.75, 0.000000001), true);
        expect_eq(approxEq(sp.eval(7.0), 16.5, 0.000000001), true);
        expect_eq(approxEq(sp.derivative(-1.0), 2.5, 0.000000001), true);
        expect_eq(approxEq(sp.derivative(5.0), 2.5, 0.000000001), true);
    });

    test("3. @cap/spline — smooth sine curve interpolation accuracy", () => {
        // Sample sin(x) at 9 knots on [0, pi]
        const xs: [9]f64 = [
            0.0,
            0.39269908169872414,
            0.7853981633974483,
            1.1780972450961724,
            1.5707963267948966,
            1.9634954084936207,
            2.356194490192345,
            2.748893571891069,
            3.141592653589793,
        ];
        const ys: [9]f64 = [
            0.0,
            0.3826834323650898,
            0.7071067811865476,
            0.9238795325112867,
            1.0,
            0.9238795325112867,
            0.7071067811865476,
            0.3826834323650898,
            0.0,
        ];
        const sp: CubicSpline = CubicSpline.build(xs[0..9], ys[0..9]);
        expect_eq(sp.isValid(), true);

        // At pi/6 (0.5235987755982988), sin(pi/6) = 0.5, cos(pi/6) = 0.8660254
        const pi6: f64 = 0.5235987755982988;
        expect_eq(approxEq(sp.eval(pi6), 0.5, 0.001), true);
        expect_eq(approxEq(sp.derivative(pi6), 0.8660254, 0.005), true);

        // Peak at pi/2 has derivative ~ 0
        expect_eq(approxEq(sp.derivative(1.5707963267948966), 0.0, 0.001), true);
    });

    test("4. @cap/spline — rejects non-monotonic xs, mismatched lengths & < 2 knots", () => {
        const xs_bad: [3]f64 = [0.0, 2.0, 2.0];
        const ys_bad: [3]f64 = [1.0, 2.0, 3.0];
        const sp_dup: CubicSpline = CubicSpline.build(xs_bad[0..3], ys_bad[0..3]);
        expect_eq(sp_dup.isValid(), false);

        const xs_desc: [3]f64 = [0.0, 3.0, 1.0];
        const sp_desc: CubicSpline = CubicSpline.build(xs_desc[0..3], ys_bad[0..3]);
        expect_eq(sp_desc.isValid(), false);

        const xs_one: [1]f64 = [0.0];
        const ys_one: [1]f64 = [1.0];
        const sp_one: CubicSpline = CubicSpline.build(xs_one[0..1], ys_one[0..1]);
        expect_eq(sp_one.isValid(), false);

        const sp_mismatch: CubicSpline = CubicSpline.build(xs_bad[0..2], ys_bad[0..3]);
        expect_eq(sp_mismatch.isValid(), false);
    });

    test("5. @cap/spline — quadratic & cubic Bezier endpoints and midpoints", () => {
        const p0 = Vec2 { x: 0.0, y: 0.0 };
        const p1 = Vec2 { x: 1.0, y: 2.0 };
        const p2 = Vec2 { x: 2.0, y: 0.0 };
        const q0: Vec2 = bezierQuad(p0, p1, p2, 0.0);
        const q5: Vec2 = bezierQuad(p0, p1, p2, 0.5);
        const q1: Vec2 = bezierQuad(p0, p1, p2, 1.0);

        expect_eq(approxEq(q0.x, 0.0, 0.000000001) && approxEq(q0.y, 0.0, 0.000000001), true);
        // At t=0.5: 0.25*p0 + 0.5*p1 + 0.25*p2 = (1.0, 1.0)
        expect_eq(approxEq(q5.x, 1.0, 0.000000001) && approxEq(q5.y, 1.0, 0.000000001), true);
        expect_eq(approxEq(q1.x, 2.0, 0.000000001) && approxEq(q1.y, 0.0, 0.000000001), true);

        const c0 = Vec2 { x: 0.0, y: 0.0 };
        const c1 = Vec2 { x: 0.0, y: 4.0 };
        const c2 = Vec2 { x: 4.0, y: 4.0 };
        const c3 = Vec2 { x: 4.0, y: 0.0 };
        const b5: Vec2 = bezierCubic(c0, c1, c2, c3, 0.5);
        // At t=0.5: (1/8)*c0 + (3/8)*c1 + (3/8)*c2 + (1/8)*c3 = (2.0, 3.0)
        expect_eq(approxEq(b5.x, 2.0, 0.000000001) && approxEq(b5.y, 3.0, 0.000000001), true);
    });

    test("6. @cap/spline — catmullRom, smoothstep & lerp boundary checks", () => {
        expect_eq(approxEq(lerp(10.0, 20.0, 0.0), 10.0, 0.000000001), true);
        expect_eq(approxEq(lerp(10.0, 20.0, 0.25), 12.5, 0.000000001), true);
        expect_eq(approxEq(lerp(10.0, 20.0, 1.0), 20.0, 0.000000001), true);

        expect_eq(approxEq(smoothstep(0.0, 10.0, -5.0), 0.0, 0.000000001), true);
        expect_eq(approxEq(smoothstep(0.0, 10.0, 5.0), 0.5, 0.000000001), true);
        expect_eq(approxEq(smoothstep(0.0, 10.0, 15.0), 1.0, 0.000000001), true);

        // Catmull-Rom passes through p1 at t=0 and p2 at t=1; linear sequence stays linear
        expect_eq(approxEq(catmullRom(0.0, 1.0, 2.0, 3.0, 0.0), 1.0, 0.000000001), true);
        expect_eq(approxEq(catmullRom(0.0, 1.0, 2.0, 3.0, 0.5), 1.5, 0.000000001), true);
        expect_eq(approxEq(catmullRom(0.0, 1.0, 2.0, 3.0, 1.0), 2.0, 0.000000001), true);
    });
}
