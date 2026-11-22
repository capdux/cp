// Source: cp/cap/test/test_quadrature_production_deep.cp (Production deep tests for cap:quadrature)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    Gk15Estimate,
    QuadResult,
    gaussLegendre5,
    gaussLegendre10,
    compositeSimpson,
    gaussKronrod15,
    adaptiveGk15,
    romberg,
} from "cap:math";

const PI: f64 = 3.14159265358979323846;

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn powInt(x: f64, p: usize): f64 {
    let r: f64 = 1.0;
    let i: usize = 0;
    while (i < p) {
        r *= x;
        i += 1;
    }
    return r;
}

fn polyDeg9(x: f64): f64 {
    // 10*x^9 + 7*x^6 - 4*x^3 + 3  -> antiderivative on [0, 1] is 1 + 1 - 1 + 3 = 4.0
    return 10.0 * powInt(x, 9) + 7.0 * powInt(x, 6) - 4.0 * powInt(x, 3) + 3.0;
}

fn polyDeg18(x: f64): f64 {
    // 19*x^18 + 11*x^10 + 1  -> antiderivative on [-1, 1] is 2 + 2 + 2 = 6.0
    return 19.0 * powInt(x, 18) + 11.0 * powInt(x, 10) + 1.0;
}

fn fSin(x: f64): f64 {
    return std.math.sin(x);
}

fn fFourOver1PlusX2(x: f64): f64 {
    return 4.0 / (1.0 + x * x);
}

fn fExp(x: f64): f64 {
    return std.math.exp(x);
}

fn fInvSqrt(x: f64): f64 {
    return 1.0 / std.math.sqrt(x);
}

fn fSharpPeak(x: f64): f64 {
    const d: f64 = x - 3.0;
    return std.math.exp(-100.0 * d * d);
}

fn fDampedCos(x: f64): f64 {
    return std.math.cos(5.0 * x) * std.math.exp(-0.5 * x);
}

fn fReturnsNan(x: f64): f64 {
    const z: f64 = x - x;
    return z / z;
}

test("1. Gauss-Legendre 5 (deg 9) & 10 (deg 18) achieve machine precision < 1e-14", () => {
    const gl5: f64 = gaussLegendre5(polyDeg9, 0.0, 1.0);
    expect_eq(absF64(gl5 - 4.0) < 1.0e-14, true);

    const gl10: f64 = gaussLegendre10(polyDeg18, -1.0, 1.0);
    expect_eq(absF64(gl10 - 6.0) < 1.0e-14, true);

    // Reversed bounds negate the integral
    const gl5_rev: f64 = gaussLegendre5(polyDeg9, 1.0, 0.0);
    expect_eq(absF64(gl5_rev + 4.0) < 1.0e-14, true);
});

test("2. Classic analytical integrals: sin(x), 4/(1+x^2), exp(x) across GL10, Simpson, GK15 & Romberg", () => {
    // integral_0^pi sin(x) dx = 2
    expect_eq(absF64(gaussLegendre10(fSin, 0.0, PI) - 2.0) < 1.0e-14, true);
    const simp_sin: f64 = compositeSimpson(fSin, 0.0, PI, 100) ?? unreachable;
    expect_eq(absF64(simp_sin - 2.0) < 1.0e-8, true);
    const gk_sin: Gk15Estimate = gaussKronrod15(fSin, 0.0, PI);
    expect_eq(absF64(gk_sin.value - 2.0) < 1.0e-13, true);
    expect_eq(gk_sin.abs_error < 1.0e-10, true);

    // integral_0^1 4/(1+x^2) dx = pi
    expect_eq(absF64(gaussLegendre10(fFourOver1PlusX2, 0.0, 1.0) - PI) < 1.0e-12, true);
    const simp_pi: f64 = compositeSimpson(fFourOver1PlusX2, 0.0, 1.0, 64) ?? unreachable;
    expect_eq(absF64(simp_pi - PI) < 1.0e-9, true);

    // integral_0^1 exp(x) dx = e - 1
    const exact_e: f64 = std.math.exp(1.0) - 1.0;
    expect_eq(absF64(gaussLegendre10(fExp, 0.0, 1.0) - exact_e) < 1.0e-14, true);

    let r_prev_b: [16]f64 = std.mem.zeroes([16]f64);
    let r_curr_b: [16]f64 = std.mem.zeroes([16]f64);
    let r_prev: []f64 = r_prev_b[0..];
    let r_curr: []f64 = r_curr_b[0..];

    const romb_pi: QuadResult = romberg(fFourOver1PlusX2, 0.0, 1.0, 1.0e-12, 14, *r_prev, *r_curr) ?? unreachable;
    expect_eq(romb_pi.converged, true);
    expect_eq(absF64(romb_pi.value - PI) < 1.0e-12, true);
});

test("3. adaptiveGk15 handles endpoint singularity 1/sqrt(x) & narrow Gaussian spike exp(-100*(x-3)^2)", () => {
    let sa_b: [128]f64 = std.mem.zeroes([128]f64);
    let sb_b: [128]f64 = std.mem.zeroes([128]f64);
    let sa: []f64 = sa_b[0..];
    let sb: []f64 = sb_b[0..];

    // Weak endpoint singularity: integral_0^1 1/sqrt(x) dx = 2.0
    const sing: QuadResult = adaptiveGk15(fInvSqrt, 0.0, 1.0, 1.0e-8, 1.0e-8, 256, *sa, *sb) ?? unreachable;
    expect_eq(sing.converged, true);
    expect_eq(absF64(sing.value - 2.0) < 1.0e-7, true);
    expect_eq(sing.evals >= 15, true);

    // Sharp Gaussian peak on [0, 10]: integral ~ sqrt(pi) / 10 = 0.1772453850905516
    const exact_peak: f64 = std.math.sqrt(PI) / 10.0;
    const peak: QuadResult = adaptiveGk15(fSharpPeak, 0.0, 10.0, 1.0e-10, 1.0e-10, 256, *sa, *sb) ?? unreachable;
    expect_eq(peak.converged, true);
    expect_eq(absF64(peak.value - exact_peak) < 1.0e-9, true);
});

test("4. Reversed bounds (a > b) sign-flip & zero-width interval (a == b) across methods", () => {
    let sa_b: [64]f64 = std.mem.zeroes([64]f64);
    let sb_b: [64]f64 = std.mem.zeroes([64]f64);
    let sa: []f64 = sa_b[0..];
    let sb: []f64 = sb_b[0..];

    const rev_gk: QuadResult = adaptiveGk15(fSin, PI, 0.0, 1.0e-12, 1.0e-12, 64, *sa, *sb) ?? unreachable;
    expect_eq(rev_gk.converged, true);
    expect_eq(absF64(rev_gk.value + 2.0) < 1.0e-11, true);

    const zero_gk: QuadResult = adaptiveGk15(fSin, 1.5, 1.5, 1.0e-10, 1.0e-10, 16, *sa, *sb) ?? unreachable;
    expect_eq(zero_gk.converged, true);
    expect_eq(zero_gk.value == 0.0 && zero_gk.evals == 0, true);

    let r_prev_b: [12]f64 = std.mem.zeroes([12]f64);
    let r_curr_b: [12]f64 = std.mem.zeroes([12]f64);
    let r_prev: []f64 = r_prev_b[0..];
    let r_curr: []f64 = r_curr_b[0..];

    const rev_romb: QuadResult = romberg(fSin, PI, 0.0, 1.0e-12, 12, *r_prev, *r_curr) ?? unreachable;
    expect_eq(rev_romb.converged, true);
    expect_eq(absF64(rev_romb.value + 2.0) < 1.0e-11, true);

    const rev_simp: f64 = compositeSimpson(fSin, PI, 0.0, 64) ?? unreachable;
    expect_eq(absF64(rev_simp + 2.0) < 1.0e-7, true);
});

test("5. Oscillatory damped cosine integral matches closed-form analytical value < 1e-10", () => {
    // integral_0^(2*pi) cos(5*x)*exp(-0.5*x) dx
    // = [exp(-0.5*x) * (-0.5*cos(5x) + 5*sin(5x)) / (0.25 + 25)]_0^(2*pi)
    // = (1 - exp(-pi)) * 0.5 / 25.25 = (1 - exp(-pi)) / 50.5
    const exact: f64 = (1.0 - std.math.exp(0.0 - PI)) / 50.5;

    let sa_b: [64]f64 = std.mem.zeroes([64]f64);
    let sb_b: [64]f64 = std.mem.zeroes([64]f64);
    let sa: []f64 = sa_b[0..];
    let sb: []f64 = sb_b[0..];

    const res: QuadResult = adaptiveGk15(fDampedCos, 0.0, 2.0 * PI, 1.0e-12, 1.0e-12, 128, *sa, *sb) ?? unreachable;
    expect_eq(res.converged, true);
    expect_eq(absF64(res.value - exact) < 1.0e-10, true);
});

test("6. Defensive guards: NaN/Inf, invalid tolerances, odd/zero panels & short stack buffers", () => {
    const zero: f64 = 0.0;
    const nan_v: f64 = zero / zero;
    const inf_v: f64 = 1.0 / zero;

    // compositeSimpson guards
    expect_eq(compositeSimpson(fSin, 0.0, 1.0, 0) == null, true);
    expect_eq(compositeSimpson(fSin, 0.0, 1.0, 3) == null, true); // odd panels
    expect_eq(compositeSimpson(fSin, nan_v, 1.0, 4) == null, true);
    expect_eq(compositeSimpson(fReturnsNan, 0.0, 1.0, 4) == null, true);

    // adaptiveGk15 guards
    let sa_b: [8]f64 = std.mem.zeroes([8]f64);
    let sb_b: [8]f64 = std.mem.zeroes([8]f64);
    let sa: []f64 = sa_b[0..];
    let sb: []f64 = sb_b[0..];
    let tiny_s: []f64 = sa_b[0..1];

    expect_eq(adaptiveGk15(fSin, nan_v, 1.0, 1.0e-6, 1.0e-6, 16, *sa, *sb) == null, true);
    expect_eq(adaptiveGk15(fSin, 0.0, inf_v, 1.0e-6, 1.0e-6, 16, *sa, *sb) == null, true);
    expect_eq(adaptiveGk15(fSin, 0.0, 1.0, -1.0, 1.0e-6, 16, *sa, *sb) == null, true);
    expect_eq(adaptiveGk15(fSin, 0.0, 1.0, 0.0, 0.0, 16, *sa, *sb) == null, true);
    expect_eq(adaptiveGk15(fSin, 0.0, 1.0, 1.0e-6, 1.0e-6, 0, *sa, *sb) == null, true);
    expect_eq(adaptiveGk15(fSin, 0.0, 1.0, 1.0e-6, 1.0e-6, 16, *tiny_s, *sb) == null, true);
    expect_eq(adaptiveGk15(fReturnsNan, 0.0, 1.0, 1.0e-6, 1.0e-6, 16, *sa, *sb) == null, true);

    // romberg guards
    expect_eq(romberg(fSin, 0.0, 1.0, 0.0, 8, *sa, *sb) == null, true);
    expect_eq(romberg(fSin, 0.0, 1.0, 1.0e-6, 1, *sa, *sb) == null, true);
    expect_eq(romberg(fSin, 0.0, 1.0, 1.0e-6, 10, *sa, *sb) == null, true); // buffers only len 8
    expect_eq(romberg(fReturnsNan, 0.0, 1.0, 1.0e-6, 6, *sa, *sb) == null, true);
});
