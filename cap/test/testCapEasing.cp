// cap/test/testCapEasing.cp
// Comprehensive unit tests for @cap/easing

import { test, expect_eq } from "cap:test";
import {
    linear,
    quadIn,
    quadOut,
    quadInOut,
    cubicIn,
    cubicOut,
    cubicInOut,
    quartIn,
    quartOut,
    quartInOut,
    quintIn,
    quintOut,
    quintInOut,
    sineIn,
    sineOut,
    sineInOut,
    circIn,
    circOut,
    circInOut,
    backIn,
    backOut,
    backInOut,
    bounceIn,
    bounceOut,
    bounceInOut,
    cubicBezier,
} from "../../packages/ezing/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff: f64 = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/easing — Exact endpoints f(0)==0, f(1)==1 & out-of-range clamping", () => {
        expect_eq(linear(-0.5), 0.0);
        expect_eq(linear(1.5), 1.0);
        expect_eq(quadIn(0.0), 0.0);
        expect_eq(quadIn(1.0), 1.0);
        expect_eq(quadOut(0.0), 0.0);
        expect_eq(quadOut(1.0), 1.0);
        expect_eq(cubicIn(0.0), 0.0);
        expect_eq(cubicIn(1.0), 1.0);
        expect_eq(quartOut(0.0), 0.0);
        expect_eq(quartOut(1.0), 1.0);
        expect_eq(quintInOut(0.0), 0.0);
        expect_eq(quintInOut(1.0), 1.0);
        expect_eq(sineIn(0.0), 0.0);
        expect_eq(sineIn(1.0), 1.0);
        expect_eq(sineOut(0.0), 0.0);
        expect_eq(sineOut(1.0), 1.0);
        expect_eq(sineInOut(0.0), 0.0);
        expect_eq(sineInOut(1.0), 1.0);
        expect_eq(circIn(0.0), 0.0);
        expect_eq(circIn(1.0), 1.0);
        expect_eq(circOut(0.0), 0.0);
        expect_eq(circOut(1.0), 1.0);
        expect_eq(backIn(0.0), 0.0);
        expect_eq(backIn(1.0), 1.0);
        expect_eq(backOut(0.0), 0.0);
        expect_eq(backOut(1.0), 1.0);
        expect_eq(bounceIn(0.0), 0.0);
        expect_eq(bounceIn(1.0), 1.0);
        expect_eq(bounceOut(0.0), 0.0);
        expect_eq(bounceOut(1.0), 1.0);
    });

    test("2. @cap/easing — Polynomial quad/cubic/quart/quint symmetry at t=0.5", () => {
        expect_eq(approxEq(quadInOut(0.5), 0.5, 0.000000001), true);
        expect_eq(approxEq(cubicInOut(0.5), 0.5, 0.000000001), true);
        expect_eq(approxEq(quartInOut(0.5), 0.5, 0.000000001), true);
        expect_eq(approxEq(quintInOut(0.5), 0.5, 0.000000001), true);
        expect_eq(approxEq(quadIn(0.5), 0.25, 0.000000001), true);
        expect_eq(approxEq(cubicIn(0.5), 0.125, 0.000000001), true);
        expect_eq(approxEq(quartIn(0.5), 0.0625, 0.000000001), true);
        expect_eq(approxEq(quintIn(0.5), 0.03125, 0.000000001), true);
        expect_eq(approxEq(quadIn(0.25) + quadOut(0.75), 1.0, 0.000000001), true);
    });

    test("3. @cap/easing — Sine & Circular precision and monotonicity", () => {
        expect_eq(approxEq(sineInOut(0.5), 0.5, 0.000000001), true);
        expect_eq(approxEq(circInOut(0.5), 0.5, 0.000000001), true);
        let prevS: f64 = 0.0;
        let prevC: f64 = 0.0;
        let i: usize = 1;
        while (i <= 10) {
            const t: f64 = (i as f64) * 0.1;
            const curS: f64 = sineInOut(t);
            const curC: f64 = circInOut(t);
            expect_eq(curS > prevS, true);
            expect_eq(curC > prevC, true);
            prevS = curS;
            prevC = curC;
            i += 1;
        }
    });

    test("4. @cap/easing — BackIn / BackOut / BackInOut overshoot behavior", () => {
        expect_eq(backIn(0.3) < 0.0, true);
        expect_eq(backOut(0.7) > 1.0, true);
        expect_eq(backInOut(0.15) < 0.0, true);
        expect_eq(backInOut(0.85) > 1.0, true);
        expect_eq(approxEq(backInOut(0.5), 0.5, 0.000000001), true);
    });

    test("5. @cap/easing — BounceOut / BounceIn / BounceInOut characteristics", () => {
        expect_eq(bounceOut(0.5) > 0.7, true);
        expect_eq(bounceOut(0.9) > 0.9, true);
        expect_eq(approxEq(bounceIn(0.3) + bounceOut(0.7), 1.0, 0.000000001), true);
        expect_eq(approxEq(bounceInOut(0.5), 0.5, 0.000000001), true);
    });

    test("6. @cap/easing — CSS cubicBezier exact evaluation (linear & CSS ease)", () => {
        // Linear cubic-bezier(0, 0, 1, 1)
        expect_eq(approxEq(cubicBezier(0.0, 0.0, 1.0, 1.0, 0.35), 0.35, 0.000000001), true);
        expect_eq(cubicBezier(0.25, 0.1, 0.25, 1.0, 0.0), 0.0);
        expect_eq(cubicBezier(0.25, 0.1, 0.25, 1.0, 1.0), 1.0);

        // Standard CSS `ease` = cubic-bezier(0.25, 0.1, 0.25, 1.0): at x=0.5, y ≈ 0.8024
        const easeMid: f64 = cubicBezier(0.25, 0.1, 0.25, 1.0, 0.5);
        expect_eq(approxEq(easeMid, 0.8024, 0.001), true);

        // Symmetric ease-in-out = cubic-bezier(0.42, 0, 0.58, 1.0): at x=0.5, y == 0.5
        const eioMid: f64 = cubicBezier(0.42, 0.0, 0.58, 1.0, 0.5);
        expect_eq(approxEq(eioMid, 0.5, 0.000001), true);
    });
}
