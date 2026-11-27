import { test, expect_eq } from "cap:test";
import {
    easeLinear,
    easeQuadIn,
    easeQuadOut,
    easeQuadInOut,
    easeCubicIn,
    easeCubicOut,
    easeCubicInOut,
    easePolyIn,
    easePolyOut,
    easePolyInOut,
    easePolyInExp,
    easeSinIn,
    easeSinOut,
    easeSinInOut,
    easeExpIn,
    easeExpOut,
    easeExpInOut,
    easeCircleIn,
    easeCircleOut,
    easeCircleInOut,
    easeBounceIn,
    easeBounceOut,
    easeBounceInOut,
    easeBackIn,
    easeBackOut,
    easeBackInOut,
    easeBackInOvershoot,
    easeElasticIn,
    easeElasticOut,
    easeElasticInOut,
    easeElasticOutParams,
} from "../../packages/d3-ease/src/index.cp";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = a - b;
    if (diff < 0.0) {
        return (0.0 - diff) <= eps;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/d3-ease — Linear, Quad & Cubic exact values", () => {
        expect_eq(approxEq(easeLinear(0.35), 0.35, 1.0e-12), true);
        expect_eq(approxEq(easeQuadIn(0.5), 0.25, 1.0e-12), true);
        expect_eq(approxEq(easeQuadOut(0.5), 0.75, 1.0e-12), true);
        expect_eq(approxEq(easeQuadInOut(0.25), 0.125, 1.0e-12), true);
        expect_eq(approxEq(easeQuadInOut(0.75), 0.875, 1.0e-12), true);
        expect_eq(approxEq(easeCubicIn(0.5), 0.125, 1.0e-12), true);
        expect_eq(approxEq(easeCubicOut(0.5), 0.875, 1.0e-12), true);
        expect_eq(approxEq(easeCubicInOut(0.5), 0.5, 1.0e-12), true);
    });

    test("2. @cap/d3-ease — Poly default (e=3) & custom exponent", () => {
        expect_eq(approxEq(easePolyIn(0.5), easeCubicIn(0.5), 1.0e-12), true);
        expect_eq(approxEq(easePolyOut(0.5), easeCubicOut(0.5), 1.0e-12), true);
        expect_eq(approxEq(easePolyInOut(0.25), easeCubicInOut(0.25), 1.0e-12), true);
        expect_eq(approxEq(easePolyInExp(0.5, 4.0), 0.0625, 1.0e-12), true);
    });

    test("3. @cap/d3-ease — Sin, Exp & Circle boundary and midpoint checks", () => {
        expect_eq(approxEq(easeSinIn(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeSinIn(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeSinOut(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeSinInOut(0.5), 0.5, 1.0e-12), true);

        expect_eq(approxEq(easeExpIn(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeExpIn(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeExpOut(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeExpOut(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeExpInOut(0.5), 0.5, 1.0e-12), true);

        expect_eq(approxEq(easeCircleIn(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeCircleOut(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeCircleInOut(0.5), 0.5, 1.0e-12), true);
    });

    test("4. @cap/d3-ease — Bounce piecewise curves & symmetry", () => {
        expect_eq(approxEq(easeBounceOut(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeBounceOut(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeBounceIn(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeBounceIn(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeBounceInOut(0.5), 0.5, 1.0e-12), true);
        expect_eq(approxEq(easeBounceIn(0.3), 1.0 - easeBounceOut(0.7), 1.0e-12), true);
    });

    test("5. @cap/d3-ease — Back overshoot & custom overshoot", () => {
        expect_eq(approxEq(easeBackIn(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeBackIn(1.0), 1.0, 1.0e-12), true);
        expect_eq(easeBackIn(0.3) < 0.0, true);
        expect_eq(easeBackOut(0.7) > 1.0, true);
        expect_eq(approxEq(easeBackInOut(0.5), 0.5, 1.0e-12), true);
        expect_eq(easeBackInOvershoot(0.3, 3.0) < easeBackIn(0.3), true);
    });

    test("6. @cap/d3-ease — Elastic endpoints, oscillation & custom params", () => {
        expect_eq(approxEq(easeElasticIn(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeElasticIn(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeElasticOut(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeElasticOut(1.0), 1.0, 1.0e-12), true);
        expect_eq(approxEq(easeElasticInOut(0.0), 0.0, 1.0e-12), true);
        expect_eq(approxEq(easeElasticInOut(0.5), 0.5, 1.0e-12), true);
        expect_eq(approxEq(easeElasticInOut(1.0), 1.0, 1.0e-12), true);
        // ElasticOut overshoots above 1.0 early in the curve
        expect_eq(easeElasticOut(0.2) > 1.0, true);
        expect_eq(approxEq(easeElasticOutParams(1.0, 1.5, 0.4), 1.0, 1.0e-12), true);
    });
}
