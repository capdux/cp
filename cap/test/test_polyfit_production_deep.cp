// Source: cp/cap/test/test_polyfit_production_deep.cp (Production deep tests for cap:polyfit)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    POLYFIT_MAX_DEG,
    LinearFit,
    FitMetrics,
    polyEval,
    polyDerivEval,
    polyDerivCoeffs,
    polyIntegrateCoeffs,
    linearFit,
    polyfit,
    polyfitRidge,
    evalFitMetrics,
    polyRootBisect,
    polyRootNewton,
} from "cap:polyfit";

fn absDiff(a: f64, b: f64): f64 {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

test("1. linearFit exact regression + noisy R^2 / RMSE and evalFitMetrics", () => {
    expect_eq(POLYFIT_MAX_DEG, 15 as usize);

    // Exact line y = 2.5 * x - 4.0
    const xs: [6]f64 = [0.0, 1.0, 2.0, 3.0, 4.0, 5.0];
    const ys: [6]f64 = [-4.0, -1.5, 1.0, 3.5, 6.0, 8.5];

    if (lf := linearFit(xs[0..], ys[0..])) {
        expect_eq(absDiff(lf.slope, 2.5) < 1.0e-12, true);
        expect_eq(absDiff(lf.intercept, -4.0) < 1.0e-12, true);
        expect_eq(absDiff(lf.r_squared, 1.0) < 1.0e-12, true);
        expect_eq(lf.rmse < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }

    // Noisy data -> R^2 in (0.9, 1.0), RMSE > 0.
    const ys_noisy: [6]f64 = [-3.8, -1.7, 1.2, 3.3, 6.1, 8.4];
    if (lf2 := linearFit(xs[0..], ys_noisy[0..])) {
        expect_eq(lf2.r_squared > 0.98 && lf2.r_squared < 1.0, true);
        expect_eq(lf2.rmse > 0.1 && lf2.rmse < 0.3, true);

        const c2: [2]f64 = [lf2.intercept, lf2.slope];
        if (m := evalFitMetrics(xs[0..], ys_noisy[0..], c2[0..])) {
            expect_eq(absDiff(m.r_squared, lf2.r_squared) < 1.0e-12, true);
            expect_eq(absDiff(m.rmse, lf2.rmse) < 1.0e-12, true);
            expect_eq(m.max_abs_err > 0.0, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
});

test("2. polyfit degree 3 & degree 5 exact recovery (including large x-offset [1000, 1010] conditioning)", () => {
    // 1) Degree 5 polynomial on [-2, 2]: P(x) = 1.5 - 2.0x + 0.5x^2 + 3.0x^3 - 0.25x^4 + 0.1x^5
    const true5: [6]f64 = [1.5, -2.0, 0.5, 3.0, -0.25, 0.1];
    let xs5: [12]f64 = [0.0; 12];
    let ys5: [12]f64 = [0.0; 12];
    let i: usize = 0;
    while (i < 12) {
        const x: f64 = -2.0 + (i as f64) * 0.36;
        xs5[i] = x;
        ys5[i] = polyEval(true5[0..], x);
        i += 1;
    }

    let fit5_arr: [6]f64 = [0.0; 6];
    let fit5: []f64 = fit5_arr[0..];
    expect_eq(polyfit(xs5[0..], ys5[0..], 5, *fit5), true);
    let k: usize = 0;
    while (k < 6) {
        expect_eq(absDiff(fit5_arr[k], true5[k]) < 1.0e-10, true);
        k += 1;
    }

    // 2) Degree 3 polynomial with large coordinate shift x in [1000, 1010]!
    // P(x) = 0.002*(x - 1000)^3 - 0.05*(x - 1000)^2 + 1.2*(x - 1000) + 7.0
    // Expanded at x: we verify both exact prediction (< 1e-9) and coefficient accuracy (< 1e-6).
    const true3: [4]f64 = [7.0, 1.2, -0.05, 0.002];
    let xs_big: [11]f64 = [0.0; 11];
    let ys_big: [11]f64 = [0.0; 11];
    i = 0;
    while (i < 11) {
        const x: f64 = 1000.0 + (i as f64);
        xs_big[i] = x;
        ys_big[i] = polyEval(true3[0..], x);
        i += 1;
    }

    let fit3_arr: [4]f64 = [0.0; 4];
    let fit3: []f64 = fit3_arr[0..];
    expect_eq(polyfit(xs_big[0..], ys_big[0..], 3, *fit3), true);
    if (m := evalFitMetrics(xs_big[0..], ys_big[0..], fit3_arr[0..])) {
        expect_eq(m.max_abs_err < 1.0e-9, true);
        expect_eq(absDiff(m.r_squared, 1.0) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }
    expect_eq(absDiff(fit3_arr[3], true3[3]) < 1.0e-9, true);
    expect_eq(absDiff(fit3_arr[2], true3[2]) < 1.0e-6, true);
});

test("3. polyfitRidge L2 regularization shrinks coefficient norm and suppresses noisy high-degree overfitting", () => {
    // 8 points sampled from simple linear trend y = 2x + 1 with small alternating noise, fitted with deg=6.
    const xs: [8]f64 = [-1.0, -0.7, -0.4, -0.1, 0.2, 0.5, 0.8, 1.0];
    const ys: [8]f64 = [-0.85, -0.55, 0.35, 0.65, 1.55, 1.85, 2.75, 2.85];

    let ols_arr: [7]f64 = [0.0; 7];
    let ridge_arr: [7]f64 = [0.0; 7];
    let ols: []f64 = ols_arr[0..];
    let ridge: []f64 = ridge_arr[0..];

    expect_eq(polyfit(xs[0..], ys[0..], 6, *ols), true);
    expect_eq(polyfitRidge(xs[0..], ys[0..], 6, 1.0, *ridge), true);

    // Sum of squared non-intercept coefficients must be strictly smaller under Ridge penalty.
    let norm_ols: f64 = 0.0;
    let norm_ridge: f64 = 0.0;
    let k: usize = 1;
    while (k < 7) {
        norm_ols += ols_arr[k] * ols_arr[k];
        norm_ridge += ridge_arr[k] * ridge_arr[k];
        k += 1;
    }
    expect_eq(norm_ridge < norm_ols, true);
});

test("4. Polynomial calculus (deriv/integrate inverse) and real root solvers (Bisect & Newton: x^3 - 2 = 0)", () => {
    const p: [4]f64 = [3.0, -2.0, 5.0, -1.0]; // 3 - 2x + 5x^2 - x^3
    let d_arr: [4]f64 = [0.0; 4];
    let d_slice: []f64 = d_arr[0..];
    let int_arr: [5]f64 = [0.0; 5];
    let int_slice: []f64 = int_arr[0..];

    if (dlen := polyDerivCoeffs(p[0..], *d_slice)) {
        expect_eq(dlen, 3 as usize);
        expect_eq(absDiff(polyEval(d_arr[0..dlen], 1.75), polyDerivEval(p[0..], 1.75)) < 1.0e-12, true);

        // Integrating P'(x) with c0 = p[0] restores p(x) identically.
        if (ilen := polyIntegrateCoeffs(d_arr[0..dlen], p[0], *int_slice)) {
            expect_eq(ilen, 4 as usize);
            let i: usize = 0;
            while (i < 4) {
                expect_eq(absDiff(int_arr[i], p[i]) < 1.0e-12, true);
                i += 1;
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Solve x^3 - 2 = 0 -> cube root of 2 (~1.2599210498948732) via both Bisection and Newton.
    const cube2: [4]f64 = [-2.0, 0.0, 0.0, 1.0];
    const exact_cbrt2: f64 = std.math.pow(f64, 2.0, 1.0 / 3.0);

    if (r_bi := polyRootBisect(cube2[0..], 1.0, 2.0, 1.0e-13, 100)) {
        expect_eq(absDiff(r_bi, exact_cbrt2) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }
    if (r_nw := polyRootNewton(cube2[0..], 1.5, 1.0e-13, 50)) {
        expect_eq(absDiff(r_nw, exact_cbrt2) < 1.0e-12, true);
    } else {
        expect_eq(true, false);
    }
});

test("5. Malformed input guards: insufficient samples, deg > 15, same-sign bracket, singular degenerate x", () => {
    const xs2: [2]f64 = [1.0, 2.0];
    const ys2: [2]f64 = [3.0, 4.0];
    let c_arr: [20]f64 = [0.0; 20];
    let c: []f64 = c_arr[0..];

    // xs.len <= deg rejected.
    expect_eq(polyfit(xs2[0..], ys2[0..], 2, *c), false);
    // deg > 15 rejected.
    expect_eq(polyfit(xs2[0..], ys2[0..], 16, *c), false);
    // Negative ridge lambda rejected.
    expect_eq(polyfitRidge(xs2[0..], ys2[0..], 1, -0.5, *c), false);

    // Degenerate identical x values rejected for deg >= 1 and linearFit.
    const same_x: [3]f64 = [5.0, 5.0, 5.0];
    const y3: [3]f64 = [1.0, 2.0, 3.0];
    if (_ := linearFit(same_x[0..], y3[0..])) { expect_eq(true, false); }
    expect_eq(polyfit(same_x[0..], y3[0..], 1, *c), false);
    // But deg == 0 succeeds with mean(y3) == 2.0.
    expect_eq(polyfit(same_x[0..], y3[0..], 0, *c), true);
    expect_eq(absDiff(c_arr[0], 2.0) < 1.0e-12, true);

    // Same-sign bracket rejected by bisection; zero derivative at x0=0 rejected by Newton.
    const quad_pos: [3]f64 = [1.0, 0.0, 1.0]; // x^2 + 1 > 0 everywhere
    if (_ := polyRootBisect(quad_pos[0..], -2.0, 2.0, 1.0e-9, 50)) { expect_eq(true, false); }
    if (_ := polyRootNewton(quad_pos[0..], 0.0, 1.0e-9, 50)) { expect_eq(true, false); }
});
