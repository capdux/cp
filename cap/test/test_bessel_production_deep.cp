// Source: cp/cap/test/test_bessel_production_deep.cp (Production deep tests for cap:bessel)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    j0,
    j1,
    jn,
    y0,
    y1,
    yn,
    besselI0,
    besselI1,
    i0e,
    i1e,
    k0,
    k1,
    k0e,
    k1e,
} from "cap:bessel";

const PI: f64 = 3.14159265358979323846;

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn isNanF64(x: f64): bool {
    return x != x;
}

test("1. J0/J1/Y0/Y1 reference points, J0 zeros & parity symmetries", () => {
    expect_eq(j0(0.0), 1.0);
    expect_eq(j1(0.0), 0.0);

    // Classic zeros of J0
    expect_eq(absF64(j0(2.4048255576957727)) < 1.0e-14, true);
    expect_eq(absF64(j0(5.5200781102863106)) < 1.0e-14, true);

    // Reference values at x = 1.0
    expect_eq(absF64(j0(1.0) - 0.76519768655796655) < 1.0e-14, true);
    expect_eq(absF64(j1(1.0) - 0.44005058574493352) < 1.0e-14, true);
    expect_eq(absF64(y0(1.0) - 0.08825696421567696) < 1.0e-14, true);
    expect_eq(absF64(y1(1.0) + 0.78121282130028872) < 1.0e-14, true);

    // Even/odd parity
    expect_eq(absF64(j0(-3.7) - j0(3.7)) < 1.0e-15, true);
    expect_eq(absF64(j1(-3.7) + j1(3.7)) < 1.0e-15, true);
    expect_eq(absF64(jn(-3, 2.5) + jn(3, 2.5)) < 1.0e-15, true);
    expect_eq(absF64(jn(-4, 2.5) - jn(4, 2.5)) < 1.0e-15, true);
    expect_eq(absF64(jn(3, -2.5) + jn(3, 2.5)) < 1.0e-15, true);
});

test("2. Cylindrical Wronskian J1(x)*Y0(x) - J0(x)*Y1(x) == 2/(pi*x) across full domain", () => {
    const xs: [8]f64 = [0.1, 1.0, 5.0, 12.0, 17.9, 18.1, 30.0, 100.0];
    let i: usize = 0;
    while (i < 8) {
        const x: f64 = xs[i];
        const w: f64 = j1(x) * y0(x) - j0(x) * y1(x);
        const exact: f64 = 2.0 / (PI * x);
        expect_eq(absF64(w - exact) <= 1.0e-12 * exact, true);
        i += 1;
    }
});

test("3. Modified Wronskian I0*K1 + I1*K0 == 1/x & scaled i0e*k1e + i1e*k0e == 1/x up to x = 1000", () => {
    // Unscaled Wronskian across series and asymptotic crossover
    const xs: [6]f64 = [0.2, 1.5, 8.0, 15.9, 16.1, 25.0];
    let i: usize = 0;
    while (i < 6) {
        const x: f64 = xs[i];
        const w: f64 = besselI0(x) * k1(x) + besselI1(x) * k0(x);
        const exact: f64 = 1.0 / x;
        expect_eq(absF64(w - exact) <= 1.0e-12 * exact, true);
        i += 1;
    }

    // Exponentially scaled Wronskian at large x (100, 500, 1000) without overflow/underflow
    const big_xs: [4]f64 = [24.5, 100.0, 500.0, 1000.0];
    let j: usize = 0;
    while (j < 4) {
        const x: f64 = big_xs[j];
        const ws: f64 = i0e(x) * k1e(x) + i1e(x) * k0e(x);
        const exact: f64 = 1.0 / x;
        expect_eq(absF64(ws - exact) <= 1.0e-12 * exact, true);
        j += 1;
    }
});

test("4. Three-term recurrence for Jn/Yn & downward Miller stability for jn(20, 5.0)", () => {
    // Recurrence J_{n-1}(x) + J_{n+1}(x) == (2*n/x) * J_n(x)
    const test_n: [4]i32 = [2, 5, 10, 15];
    const test_x: [4]f64 = [6.0, 3.0, 8.0, 5.0];
    let i: usize = 0;
    while (i < 4) {
        const n: i32 = test_n[i];
        const x: f64 = test_x[i];
        const lhs: f64 = jn(n - 1, x) + jn(n + 1, x);
        const rhs: f64 = (2.0 * (n as f64) / x) * jn(n, x);
        const scale: f64 = absF64(lhs) + absF64(rhs) + 1.0e-15;
        expect_eq(absF64(lhs - rhs) <= 1.0e-12 * scale, true);
        i += 1;
    }

    // High-order Miller downward recurrence: J_20(5.0) = 2.770330052128947e-11
    const j20_5: f64 = jn(20, 5.0);
    const ref_j20: f64 = 2.770330052128947e-11;
    expect_eq(absF64(j20_5 - ref_j20) / ref_j20 < 1.0e-12, true);

    // Y_n recurrence: Y_{n-1}(x) + Y_{n+1}(x) == (2*n/x) * Y_n(x)
    const yl: f64 = yn(2, 4.0) + yn(4, 4.0);
    const yr: f64 = (6.0 / 4.0) * yn(3, 4.0);
    expect_eq(absF64(yl - yr) < 1.0e-13, true);
});

test("5. Modified Bessel reference values & parity symmetries", () => {
    expect_eq(besselI0(0.0), 1.0);
    expect_eq(besselI1(0.0), 0.0);
    expect_eq(i0e(0.0), 1.0);
    expect_eq(i1e(0.0), 0.0);

    expect_eq(absF64(besselI0(1.0) - 1.2660658777520084) < 1.0e-14, true);
    expect_eq(absF64(besselI1(1.0) - 0.5651591039924850) < 1.0e-14, true);
    expect_eq(absF64(k0(1.0) - 0.4210244382407083) < 1.0e-14, true);
    expect_eq(absF64(k1(1.0) - 0.6019072301972346) < 1.0e-14, true);

    // Parity: besselI0/i0e even, besselI1/i1e odd
    expect_eq(absF64(besselI0(-2.5) - besselI0(2.5)) < 1.0e-15, true);
    expect_eq(absF64(besselI1(-2.5) + besselI1(2.5)) < 1.0e-15, true);
    expect_eq(absF64(i0e(-30.0) - i0e(30.0)) < 1.0e-15, true);
    expect_eq(absF64(i1e(-30.0) + i1e(30.0)) < 1.0e-15, true);
});

test("6. Domain guards: x <= 0 singularities & NaN propagation", () => {
    // Y_n and K_n singularities at x == 0 and NaN for x < 0
    expect_eq(y0(0.0) < -1.0e300, true);
    expect_eq(y1(0.0) < -1.0e300, true);
    expect_eq(yn(2, 0.0) < -1.0e300, true);
    expect_eq(isNanF64(y0(-1.0)), true);
    expect_eq(isNanF64(y1(-1.0)), true);
    expect_eq(isNanF64(yn(3, -1.0)), true);

    expect_eq(k0(0.0) > 1.0e300, true);
    expect_eq(k1(0.0) > 1.0e300, true);
    expect_eq(k0e(0.0) > 1.0e300, true);
    expect_eq(k1e(0.0) > 1.0e300, true);
    expect_eq(isNanF64(k0(-0.5)), true);
    expect_eq(isNanF64(k1(-0.5)), true);
    expect_eq(isNanF64(k0e(-0.5)), true);
    expect_eq(isNanF64(k1e(-0.5)), true);

    // NaN propagation across all functions
    const zero: f64 = 0.0;
    const nan_v: f64 = zero / zero;
    expect_eq(isNanF64(j0(nan_v)), true);
    expect_eq(isNanF64(j1(nan_v)), true);
    expect_eq(isNanF64(jn(2, nan_v)), true);
    expect_eq(isNanF64(y0(nan_v)), true);
    expect_eq(isNanF64(besselI0(nan_v)), true);
    expect_eq(isNanF64(i1e(nan_v)), true);
    expect_eq(isNanF64(k0(nan_v)), true);
    expect_eq(isNanF64(k1e(nan_v)), true);
});
