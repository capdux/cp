// Source: cp/cap/test/test_spline_production_deep.cp (Production deep tests for cap:spline)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CubicSegment,
    CubicSpline,
    buildNaturalCubicSpline,
    buildClampedCubicSpline,
    buildPchipSpline,
    catmullRom1d,
    catmullRom1dDeriv,
} from "cap:spline";

fn absDiff(a: f64, b: f64): f64 {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

fn poly3(x: f64): f64 {
    return x * x * x - 2.0 * x * x + 3.0 * x - 1.0;
}

fn poly3Deriv(x: f64): f64 {
    return 3.0 * x * x - 4.0 * x + 3.0;
}

test("1. Natural Cubic Spline: exact node interpolation, C0/C1/C2 interior continuity, zero endpoint S'', and integrate()", () => {
    const xs: [5]f64 = [0.0, 1.0, 2.5, 4.0, 5.0];
    const ys: [5]f64 = [0.0, 1.5, 0.5, 2.0, 1.0];
    let seg_arr: [4]CubicSegment = std.mem.zeroes([4]CubicSegment);
    let scr_arr: [15]f64 = [0.0; 15];
    let segs: []CubicSegment = seg_arr[0..];
    let scratch: []f64 = scr_arr[0..];

    if (sp := buildNaturalCubicSpline(xs[0..], ys[0..], *segs, *scratch)) {
        // Exact interpolation at every knot.
        let i: usize = 0;
        while (i < 5) {
            expect_eq(absDiff(sp.eval(xs[i]), ys[i]) < 1.0e-12, true);
            i += 1;
        }

        // Natural boundary conditions: S''(x_0) == 0 and S''(x_{n-1}) == 0.
        expect_eq(absDiff(sp.secondDerivative(xs[0]), 0.0) < 1.0e-12, true);
        expect_eq(absDiff(sp.secondDerivative(xs[4]), 0.0) < 1.0e-12, true);

        // C0, C1, C2 continuity across all interior knots (comparing left segment end vs right segment start).
        let k: usize = 0;
        while (k + 1 < 4) {
            const xk: f64 = xs[k + 1];
            expect_eq(absDiff(sp.segments[k].eval(xk), sp.segments[k + 1].eval(xk)) < 1.0e-12, true);
            expect_eq(absDiff(sp.segments[k].derivative(xk), sp.segments[k + 1].derivative(xk)) < 1.0e-12, true);
            expect_eq(absDiff(sp.segments[k].secondDerivative(xk), sp.segments[k + 1].secondDerivative(xk)) < 1.0e-12, true);
            k += 1;
        }

        // Verify analytic integrate() matches fine trapezoidal numerical integration.
        const exact_int: f64 = sp.integrate();
        const steps: usize = 10000;
        const h: f64 = (xs[4] - xs[0]) / (steps as f64);
        let num_int: f64 = 0.5 * (sp.eval(xs[0]) + sp.eval(xs[4]));
        let s: usize = 1;
        while (s < steps) {
            num_int += sp.eval(xs[0] + (s as f64) * h);
            s += 1;
        }
        num_int *= h;
        expect_eq(absDiff(exact_int, num_int) < 1.0e-6, true);
    } else {
        expect_eq(true, false);
    }
});

test("2. Clamped Cubic Spline reproduces exact cubic polynomial y = x^3 - 2x^2 + 3x - 1 (< 1e-11 across domain)", () => {
    const xs: [5]f64 = [-1.0, 0.0, 1.0, 2.5, 4.0];
    let ys: [5]f64 = [0.0; 5];
    let i: usize = 0;
    while (i < 5) {
        ys[i] = poly3(xs[i]);
        i += 1;
    }

    let seg_arr: [4]CubicSegment = std.mem.zeroes([4]CubicSegment);
    let scr_arr: [15]f64 = [0.0; 15];
    let segs: []CubicSegment = seg_arr[0..];
    let scratch: []f64 = scr_arr[0..];

    const dy0: f64 = poly3Deriv(xs[0]);
    const dy_end: f64 = poly3Deriv(xs[4]);

    if (sp := buildClampedCubicSpline(xs[0..], ys[0..], dy0, dy_end, *segs, *scratch)) {
        // Sample 50 points across [-1.0, 4.0]: must reproduce both value and derivative to machine precision.
        let p: usize = 0;
        while (p <= 50) {
            const x: f64 = -1.0 + (p as f64) * 0.1;
            expect_eq(absDiff(sp.eval(x), poly3(x)) < 1.0e-11, true);
            expect_eq(absDiff(sp.derivative(x), poly3Deriv(x)) < 1.0e-11, true);
            p += 1;
        }

        // Exact integral of (x^3 - 2x^2 + 3x - 1) from -1 to 4:
        // F(x) = x^4/4 - 2x^3/3 + 3x^2/2 - x
        // F(4) = 64 - 128/3 + 24 - 4 = 84 - 128/3
        // F(-1) = 1/4 + 2/3 + 3/2 + 1 = 41/12
        const expected_int: f64 = (84.0 - 128.0 / 3.0) - (41.0 / 12.0);
        expect_eq(absDiff(sp.integrate(), expected_int) < 1.0e-11, true);
    } else {
        expect_eq(true, false);
    }
});

test("3. Fritsch-Carlson PCHIP strictly preserves monotonicity on step data [0, 0, 0, 1, 1, 1] with zero overshoot", () => {
    const xs: [6]f64 = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0];
    const ys: [6]f64 = [0.0, 0.0, 0.0, 1.0, 1.0, 1.0];
    let seg_arr: [5]CubicSegment = std.mem.zeroes([5]CubicSegment);
    let segs: []CubicSegment = seg_arr[0..];

    if (sp := buildPchipSpline(xs[0..], ys[0..], *segs)) {
        let prev_val: f64 = sp.eval(0.0);
        let step: usize = 0;
        while (step <= 500) {
            const x: f64 = (step as f64) * 0.01;
            const y: f64 = sp.eval(x);
            // Strictly within [0, 1] — zero overshoot or undershoot!
            expect_eq(y >= -1.0e-15 && y <= 1.0 + 1.0e-15, true);
            // Strictly non-decreasing across entire domain.
            expect_eq(y + 1.0e-15 >= prev_val, true);
            prev_val = y;
            step += 1;
        }
        // Flat plateaus must remain identically 0.0 and 1.0.
        expect_eq(absDiff(sp.eval(1.5), 0.0) < 1.0e-15, true);
        expect_eq(absDiff(sp.eval(3.5), 1.0) < 1.0e-15, true);
    } else {
        expect_eq(true, false);
    }
});

test("4. Catmull-Rom 1D endpoint interpolation (p1 at t=0, p2 at t=1) and analytic vs finite-difference derivative", () => {
    const p0: f64 = -2.0;
    const p1: f64 = 1.5;
    const p2: f64 = 4.25;
    const p3: f64 = 3.0;

    expect_eq(absDiff(catmullRom1d(p0, p1, p2, p3, 0.0), p1) < 1.0e-15, true);
    expect_eq(absDiff(catmullRom1d(p0, p1, p2, p3, 1.0), p2) < 1.0e-15, true);
    expect_eq(absDiff(catmullRom1dDeriv(p0, p1, p2, p3, 0.0), 0.5 * (p2 - p0)) < 1.0e-15, true);
    expect_eq(absDiff(catmullRom1dDeriv(p0, p1, p2, p3, 1.0), 0.5 * (p3 - p1)) < 1.0e-15, true);

    // Check derivative against central finite difference at interior points.
    const eps: f64 = 1.0e-7;
    let i: usize = 1;
    while (i < 10) {
        const t: f64 = (i as f64) * 0.1;
        const analytic: f64 = catmullRom1dDeriv(p0, p1, p2, p3, t);
        const numeric: f64 = (catmullRom1d(p0, p1, p2, p3, t + eps) - catmullRom1d(p0, p1, p2, p3, t - eps)) / (2.0 * eps);
        expect_eq(absDiff(analytic, numeric) < 1.0e-8, true);
        i += 1;
    }
});

test("5. Malformed input guards: non-increasing xs, < 2 knots, length mismatch, and short buffers", () => {
    let seg_arr: [4]CubicSegment = std.mem.zeroes([4]CubicSegment);
    let scr_arr: [15]f64 = [0.0; 15];
    let segs: []CubicSegment = seg_arr[0..];
    let scratch: []f64 = scr_arr[0..];

    // Fewer than 2 points.
    const one_x: [1]f64 = [0.0];
    const one_y: [1]f64 = [1.0];
    if (_ := buildNaturalCubicSpline(one_x[0..], one_y[0..], *segs, *scratch)) { expect_eq(true, false); }
    if (_ := buildPchipSpline(one_x[0..], one_y[0..], *segs)) { expect_eq(true, false); }

    // Duplicate or decreasing x values.
    const dup_x: [3]f64 = [0.0, 1.0, 1.0];
    const dec_x: [3]f64 = [0.0, 2.0, 1.0];
    const y3: [3]f64 = [0.0, 1.0, 2.0];
    if (_ := buildNaturalCubicSpline(dup_x[0..], y3[0..], *segs, *scratch)) { expect_eq(true, false); }
    if (_ := buildClampedCubicSpline(dec_x[0..], y3[0..], 0.0, 0.0, *segs, *scratch)) { expect_eq(true, false); }
    if (_ := buildPchipSpline(dup_x[0..], y3[0..], *segs)) { expect_eq(true, false); }

    // Length mismatch.
    const x2: [2]f64 = [0.0, 1.0];
    if (_ := buildNaturalCubicSpline(x2[0..], y3[0..], *segs, *scratch)) { expect_eq(true, false); }

    // Insufficient segments or scratch buffer.
    const x4: [4]f64 = [0.0, 1.0, 2.0, 3.0];
    const y4: [4]f64 = [0.0, 1.0, 0.0, 1.0];
    let small_seg_arr: [2]CubicSegment = std.mem.zeroes([2]CubicSegment);
    let small_segs: []CubicSegment = small_seg_arr[0..];
    let small_scr_arr: [11]f64 = [0.0; 11];
    let small_scr: []f64 = small_scr_arr[0..];
    if (_ := buildNaturalCubicSpline(x4[0..], y4[0..], *small_segs, *scratch)) { expect_eq(true, false); }
    if (_ := buildNaturalCubicSpline(x4[0..], y4[0..], *segs, *small_scr)) { expect_eq(true, false); }
    if (_ := buildPchipSpline(x4[0..], y4[0..], *small_segs)) { expect_eq(true, false); }
});
