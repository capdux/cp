import { test, expect_eq } from "cap:test";
import {
    PI, clamp, clamp01, approxEq, lerp, lerpClamped, inverseLerp,
    remap, remapClamped, smoothstep, smootherstep, step,
    wrap, pingPong, degToRad, radToDeg,
    easeInQuad, easeOutQuad, easeInOutQuad, easeInCubic, easeOutCubic, easeInOutCubic
} from "../../packages/lerp/src/index.cp";

fn main() {
    test("lerp & inverseLerp", () => {
        expect_eq(approxEq(lerp(10.0, 20.0, 0.5), 15.0, 0.000000001), true);
        expect_eq(approxEq(lerp(10.0, 20.0, 1.5), 25.0, 0.000000001), true);
        expect_eq(approxEq(lerpClamped(10.0, 20.0, 1.5), 20.0, 0.000000001), true);
        expect_eq(approxEq(inverseLerp(10.0, 20.0, 15.0), 0.5, 0.000000001), true);
        expect_eq(approxEq(inverseLerp(5.0, 5.0, 5.0), 0.0, 0.000000001), true);
    });

    test("clamp & remap", () => {
        expect_eq(approxEq(clamp(-5.0, 0.0, 10.0), 0.0, 0.000000001), true);
        expect_eq(approxEq(clamp(15.0, 0.0, 10.0), 10.0, 0.000000001), true);
        expect_eq(approxEq(clamp01(0.42), 0.42, 0.000000001), true);
        expect_eq(approxEq(remap(0.0, 100.0, -1.0, 1.0, 50.0), 0.0, 0.000000001), true);
        expect_eq(approxEq(remapClamped(0.0, 100.0, 0.0, 10.0, 200.0), 10.0, 0.000000001), true);
    });

    test("smoothstep, smootherstep & step", () => {
        expect_eq(approxEq(smoothstep(0.0, 1.0, 0.0), 0.0, 0.000000001), true);
        expect_eq(approxEq(smoothstep(0.0, 1.0, 0.5), 0.5, 0.000000001), true);
        expect_eq(approxEq(smoothstep(0.0, 1.0, 1.0), 1.0, 0.000000001), true);
        expect_eq(approxEq(smootherstep(0.0, 1.0, 0.5), 0.5, 0.000000001), true);
        expect_eq(approxEq(step(0.5, 0.49), 0.0, 0.000000001), true);
        expect_eq(approxEq(step(0.5, 0.5), 1.0, 0.000000001), true);
    });

    test("wrap & pingPong", () => {
        expect_eq(approxEq(wrap(370.0, 0.0, 360.0), 10.0, 0.000000001), true);
        expect_eq(approxEq(wrap(-10.0, 0.0, 360.0), 350.0, 0.000000001), true);
        expect_eq(approxEq(pingPong(0.5, 1.0), 0.5, 0.000000001), true);
        expect_eq(approxEq(pingPong(1.5, 1.0), 0.5, 0.000000001), true);
        expect_eq(approxEq(pingPong(2.0, 1.0), 0.0, 0.000000001), true);
    });

    test("degToRad & radToDeg", () => {
        expect_eq(approxEq(degToRad(180.0), PI, 0.000000001), true);
        expect_eq(approxEq(radToDeg(PI / 2.0), 90.0, 0.000000001), true);
    });

    test("quadratic & cubic easing curves", () => {
        expect_eq(approxEq(easeInQuad(0.5), 0.25, 0.000000001), true);
        expect_eq(approxEq(easeOutQuad(0.5), 0.75, 0.000000001), true);
        expect_eq(approxEq(easeInOutQuad(0.5), 0.5, 0.000000001), true);
        expect_eq(approxEq(easeInCubic(0.5), 0.125, 0.000000001), true);
        expect_eq(approxEq(easeOutCubic(0.5), 0.875, 0.000000001), true);
        expect_eq(approxEq(easeInOutCubic(0.5), 0.5, 0.000000001), true);
    });
}
