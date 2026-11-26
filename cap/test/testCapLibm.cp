// cap/test/testCapLibm.cp
// Comprehensive unit tests for @cap/libm

import { test, expect_eq } from "cap:test";
import {
    PI, TAU, FRAC_PI_2, FRAC_PI_4, E, LN_2, LN_10,
    fabs, floor, ceil, trunc, round, fmod, copysign, fmin, fmax,
    sqrt, cbrt, hypot, exp, ln, log2, log10, pow,
    sin, cos, tan, asin, acos, atan, atan2,
    sinh, cosh, tanh
} from "../../packages/@cap/libm/src/index.cp";

fn approx(a: f64, b: f64, eps: f64): bool {
    return fabs(a - b) <= eps;
}

fn main() {
    test("1. @cap/libm — floor, ceil, trunc, round, fmod, copysign, fmin & fmax (with negatives)", () => {
        expect_eq(floor(2.7) == 2.0, true);
        expect_eq(floor(-2.3) == -3.0, true);
        expect_eq(floor(-2.0) == -2.0, true);

        expect_eq(ceil(2.3) == 3.0, true);
        expect_eq(ceil(-2.7) == -2.0, true);
        expect_eq(ceil(2.0) == 2.0, true);

        expect_eq(trunc(2.9) == 2.0, true);
        expect_eq(trunc(-2.9) == -2.0, true);

        expect_eq(round(2.5) == 3.0, true);
        expect_eq(round(2.4) == 2.0, true);
        expect_eq(round(-2.5) == -3.0, true);
        expect_eq(round(-2.4) == -2.0, true);

        expect_eq(approx(fmod(5.5, 2.0), 1.5, 0.000001), true);
        expect_eq(approx(fmod(-5.5, 2.0), -1.5, 0.000001), true);
        expect_eq(approx(fmod(5.5, -2.0), 1.5, 0.000001), true);

        expect_eq(copysign(3.5, -1.0) == -3.5, true);
        expect_eq(copysign(-3.5, 1.0) == 3.5, true);
        expect_eq(fmin(-4.0, 2.0) == -4.0, true);
        expect_eq(fmax(-4.0, 2.0) == 2.0, true);
    });

    test("2. @cap/libm — sqrt, cbrt & hypot(3, 4) == 5", () => {
        expect_eq(approx(sqrt(0.0), 0.0, 0.000001), true);
        expect_eq(approx(sqrt(2.0), 1.4142135623730951, 0.000001), true);
        expect_eq(approx(sqrt(144.0), 12.0, 0.000001), true);
        expect_eq(approx(sqrt(0.0001), 0.01, 0.000001), true);

        expect_eq(approx(cbrt(27.0), 3.0, 0.000001), true);
        expect_eq(approx(cbrt(-64.0), -4.0, 0.000001), true);
        expect_eq(approx(cbrt(0.125), 0.5, 0.000001), true);

        expect_eq(approx(hypot(3.0, 4.0), 5.0, 0.000001), true);
        expect_eq(approx(hypot(-5.0, 12.0), 13.0, 0.000001), true);
        expect_eq(approx(hypot(0.0, 0.0), 0.0, 0.000001), true);
    });

    test("3. @cap/libm — exp, ln, log2, log10 & pow(2, 10) == 1024 round-trips", () => {
        expect_eq(approx(exp(0.0), 1.0, 0.000001), true);
        expect_eq(approx(exp(1.0), E, 0.000001), true);
        expect_eq(approx(exp(LN_2), 2.0, 0.000001), true);
        expect_eq(approx(exp(-1.0), 1.0 / E, 0.000001), true);

        expect_eq(approx(ln(1.0), 0.0, 0.000001), true);
        expect_eq(approx(ln(E), 1.0, 0.000001), true);
        expect_eq(approx(ln(10.0), LN_10, 0.000001), true);
        expect_eq(approx(ln(exp(3.75)), 3.75, 0.000001), true);
        expect_eq(approx(exp(ln(42.5)), 42.5, 0.000001), true);

        expect_eq(approx(log2(1024.0), 10.0, 0.000001), true);
        expect_eq(approx(log2(0.25), -2.0, 0.000001), true);
        expect_eq(approx(log10(1000.0), 3.0, 0.000001), true);
        expect_eq(approx(log10(0.01), -2.0, 0.000001), true);

        expect_eq(pow(2.0, 10.0) == 1024.0, true);
        expect_eq(pow(-2.0, 3.0) == -8.0, true);
        expect_eq(approx(pow(2.0, -3.0), 0.125, 0.000001), true);
        expect_eq(approx(pow(9.0, 0.5), 3.0, 0.000001), true);
        expect_eq(approx(pow(8.0, 1.0 / 3.0), 2.0, 0.000001), true);
    });

    test("4. @cap/libm — sin, cos & tan at 0, PI/6, PI/4, PI/2, PI & large angle reduction", () => {
        expect_eq(approx(sin(0.0), 0.0, 0.000001), true);
        expect_eq(approx(cos(0.0), 1.0, 0.000001), true);
        expect_eq(approx(tan(0.0), 0.0, 0.000001), true);

        expect_eq(approx(sin(PI / 6.0), 0.5, 0.000001), true);
        expect_eq(approx(cos(PI / 6.0), sqrt(3.0) * 0.5, 0.000001), true);

        expect_eq(approx(sin(FRAC_PI_4), sqrt(2.0) * 0.5, 0.000001), true);
        expect_eq(approx(cos(FRAC_PI_4), sqrt(2.0) * 0.5, 0.000001), true);
        expect_eq(approx(tan(FRAC_PI_4), 1.0, 0.000001), true);

        expect_eq(approx(sin(FRAC_PI_2), 1.0, 0.000001), true);
        expect_eq(approx(cos(FRAC_PI_2), 0.0, 0.000001), true);

        expect_eq(approx(sin(PI), 0.0, 0.000001), true);
        expect_eq(approx(cos(PI), -1.0, 0.000001), true);

        // Large angle reduction: 100 * TAU + PI/6
        let big: f64 = 100.0 * TAU + PI / 6.0;
        expect_eq(approx(sin(big), 0.5, 0.000001), true);
        expect_eq(approx(cos(big), sqrt(3.0) * 0.5, 0.000001), true);
        expect_eq(approx(sin(-100.0 * TAU - FRAC_PI_4), -sqrt(2.0) * 0.5, 0.000001), true);
    });

    test("5. @cap/libm — asin, acos, atan & atan2 four-quadrant accuracy", () => {
        expect_eq(approx(asin(0.0), 0.0, 0.000001), true);
        expect_eq(approx(asin(0.5), PI / 6.0, 0.000001), true);
        expect_eq(approx(asin(1.0), FRAC_PI_2, 0.000001), true);
        expect_eq(approx(asin(-1.0), -FRAC_PI_2, 0.000001), true);

        expect_eq(approx(acos(1.0), 0.0, 0.000001), true);
        expect_eq(approx(acos(0.5), PI / 3.0, 0.000001), true);
        expect_eq(approx(acos(0.0), FRAC_PI_2, 0.000001), true);
        expect_eq(approx(acos(-1.0), PI, 0.000001), true);

        expect_eq(approx(atan(0.0), 0.0, 0.000001), true);
        expect_eq(approx(atan(1.0), FRAC_PI_4, 0.000001), true);
        expect_eq(approx(atan(-1.0), -FRAC_PI_4, 0.000001), true);
        expect_eq(approx(atan(sqrt(3.0)), PI / 3.0, 0.000001), true);

        // Four quadrants for atan2
        expect_eq(approx(atan2(1.0, 1.0), FRAC_PI_4, 0.000001), true);
        expect_eq(approx(atan2(1.0, -1.0), 3.0 * FRAC_PI_4, 0.000001), true);
        expect_eq(approx(atan2(-1.0, -1.0), -3.0 * FRAC_PI_4, 0.000001), true);
        expect_eq(approx(atan2(-1.0, 1.0), -FRAC_PI_4, 0.000001), true);
        expect_eq(approx(atan2(1.0, 0.0), FRAC_PI_2, 0.000001), true);
        expect_eq(approx(atan2(-1.0, 0.0), -FRAC_PI_2, 0.000001), true);
    });

    test("6. @cap/libm — sinh, cosh & tanh hyperbolic identities (cosh^2 - sinh^2 == 1)", () => {
        expect_eq(approx(sinh(0.0), 0.0, 0.000001), true);
        expect_eq(approx(cosh(0.0), 1.0, 0.000001), true);
        expect_eq(approx(tanh(0.0), 0.0, 0.000001), true);

        let xs: [5]f64 = [-2.5, -0.5, 0.00001, 1.0, 3.0];
        let i: usize = 0;
        while (i < 5) {
            let v = xs[i];
            let s = sinh(v);
            let c = cosh(v);
            let t = tanh(v);
            expect_eq(approx(c * c - s * s, 1.0, 0.000001), true);
            expect_eq(approx(t, s / c, 0.000001), true);
            i = i + 1;
        }

        expect_eq(approx(tanh(25.0), 1.0, 0.000001), true);
        expect_eq(approx(tanh(-25.0), -1.0, 0.000001), true);
    });
}
