// Source: cp/cap/test/test_gamma_production_deep.cp (Production deep tests for cap:gamma)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    lgamma,
    tgamma,
    digamma,
    gammaP,
    gammaQ,
    lbeta,
    beta,
    betainc,
    erf,
    erfc,
    logFactorial,
    logBinomial,
} from "cap:gamma";

const PI: f64 = 3.14159265358979323846;
const EULER_GAMMA: f64 = 0.57721566490153286061;

fn absF64(x: f64): f64 {
    return if (x < 0.0) 0.0 - x else x;
}

fn isNanF64(x: f64): bool {
    return x != x;
}

test("1. tgamma & lgamma exact factorials, half-integers & negative reflection", () => {
    // tgamma(n) == (n - 1)! for n = 1..10
    let fact: f64 = 1.0;
    let n: usize = 1;
    while (n <= 10) {
        if (n >= 2) {
            fact *= ((n - 1) as f64);
        }
        expect_eq(absF64(tgamma(n as f64) - fact) < 1.0e-12 * fact, true);
        expect_eq(absF64(std.math.exp(lgamma(n as f64)) - fact) < 1.0e-12 * fact, true);
        n += 1;
    }

    // Half-integers: Gamma(0.5) = sqrt(pi), Gamma(1.5) = 0.5*sqrt(pi), Gamma(-0.5) = -2*sqrt(pi)
    const sqrt_pi: f64 = std.math.sqrt(PI);
    expect_eq(absF64(tgamma(0.5) - sqrt_pi) < 1.0e-14, true);
    expect_eq(absF64(tgamma(1.5) - 0.5 * sqrt_pi) < 1.0e-14, true);
    expect_eq(absF64(tgamma(-0.5) + 2.0 * sqrt_pi) < 1.0e-13, true);
    expect_eq(absF64(tgamma(-1.5) - (4.0 / 3.0) * sqrt_pi) < 1.0e-13, true);
});

test("2. digamma(1) == -gamma_E, digamma(0.5) & recurrence/reflection identities", () => {
    expect_eq(absF64(digamma(1.0) + EULER_GAMMA) < 1.0e-14, true);
    const expected_half: f64 = (0.0 - EULER_GAMMA) - 2.0 * 0.69314718055994530942;
    expect_eq(absF64(digamma(0.5) - expected_half) < 1.0e-13, true);

    // Recurrence identity: psi(x + 1) == psi(x) + 1/x
    const xs: [5]f64 = [0.25, 1.5, 3.7, 12.4, -2.3];
    let i: usize = 0;
    while (i < 5) {
        const x: f64 = xs[i];
        const lhs: f64 = digamma(x + 1.0);
        const rhs: f64 = digamma(x) + 1.0 / x;
        expect_eq(absF64(lhs - rhs) < 1.0e-13, true);
        i += 1;
    }

    // Reflection identity: psi(1 - x) - psi(x) == pi * cos(pi*x) / sin(pi*x)
    const xr: f64 = 0.3;
    const ref_lhs: f64 = digamma(1.0 - xr) - digamma(xr);
    const ref_rhs: f64 = PI * std.math.cos(PI * xr) / std.math.sin(PI * xr);
    expect_eq(absF64(ref_lhs - ref_rhs) < 1.0e-13, true);
});

test("3. Regularized incomplete gamma P(a, x) + Q(a, x) == 1 & chi-square exact CDFs", () => {
    // Test across both series (x < a + 1) and continued-fraction (x >= a + 1) regimes
    const pairs_a: [5]f64 = [0.5, 1.0, 2.0, 5.5, 12.0];
    const pairs_x: [5]f64 = [0.2, 3.0, 1.5, 8.0, 20.0];
    let i: usize = 0;
    while (i < 5) {
        const p: f64 = gammaP(pairs_a[i], pairs_x[i]) ?? unreachable;
        const q: f64 = gammaQ(pairs_a[i], pairs_x[i]) ?? unreachable;
        expect_eq(absF64((p + q) - 1.0) < 1.0e-14, true);
        expect_eq(p >= 0.0 && p <= 1.0 && q >= 0.0 && q <= 1.0, true);
        i += 1;
    }

    // Exact closed-form chi-square CDFs:
    // df = 2 -> a = 1: P(1, x) = 1 - exp(-x)
    const p_df2: f64 = gammaP(1.0, 2.5) ?? unreachable;
    expect_eq(absF64(p_df2 - (1.0 - std.math.exp(-2.5))) < 1.0e-14, true);

    // df = 4 -> a = 2: P(2, x) = 1 - (1 + x) * exp(-x)
    const p_df4_lo: f64 = gammaP(2.0, 1.2) ?? unreachable; // series regime (1.2 < 3)
    const p_df4_hi: f64 = gammaP(2.0, 5.0) ?? unreachable; // CF regime (5.0 >= 3)
    expect_eq(absF64(p_df4_lo - (1.0 - 2.2 * std.math.exp(-1.2))) < 1.0e-13, true);
    expect_eq(absF64(p_df4_hi - (1.0 - 6.0 * std.math.exp(-5.0))) < 1.0e-13, true);

    // Boundary x = 0
    expect_eq(gammaP(3.0, 0.0) ?? unreachable, 0.0);
    expect_eq(gammaQ(3.0, 0.0) ?? unreachable, 1.0);
});

test("4. Beta & regularized incomplete beta symmetries & closed-form identities", () => {
    // B(2, 3) = 1!*2! / 4! = 2 / 24 = 1/12
    const b23: f64 = beta(2.0, 3.0) ?? unreachable;
    expect_eq(absF64(b23 - (1.0 / 12.0)) < 1.0e-14, true);
    const lb23: f64 = lbeta(2.0, 3.0) ?? unreachable;
    expect_eq(absF64(std.math.exp(lb23) - (1.0 / 12.0)) < 1.0e-14, true);

    // Symmetry: I_x(a, b) + I_{1-x}(b, a) == 1
    const i1: f64 = betainc(0.3, 2.5, 4.2) ?? unreachable;
    const i2: f64 = betainc(0.7, 4.2, 2.5) ?? unreachable;
    expect_eq(absF64((i1 + i2) - 1.0) < 1.0e-13, true);

    // Equal-parameter midpoint: I_{0.5}(a, a) == 0.5
    const mid1: f64 = betainc(0.5, 0.5, 0.5) ?? unreachable;
    const mid2: f64 = betainc(0.5, 5.0, 5.0) ?? unreachable;
    expect_eq(absF64(mid1 - 0.5) < 1.0e-14, true);
    expect_eq(absF64(mid2 - 0.5) < 1.0e-14, true);

    // Closed-form: I_x(a, 1) == x^a and I_x(1, b) == 1 - (1 - x)^b
    const ia1: f64 = betainc(0.4, 3.0, 1.0) ?? unreachable;
    expect_eq(absF64(ia1 - 0.064) < 1.0e-14, true);
    const i1b: f64 = betainc(0.2, 1.0, 4.0) ?? unreachable;
    expect_eq(absF64(i1b - (1.0 - 0.4096)) < 1.0e-14, true);
});

test("5. erf & erfc parity, complementarity & tail accuracy at x = 5.0 without cancellation", () => {
    expect_eq(erf(0.0), 0.0);
    expect_eq(erfc(0.0), 1.0);

    // Known reference: erf(1.0) = 0.84270079294971486934
    expect_eq(absF64(erf(1.0) - 0.84270079294971487) < 1.0e-14, true);
    // Odd symmetry erf(-x) == -erf(x) and erf(x) + erfc(x) == 1
    expect_eq(absF64(erf(-1.5) + erf(1.5)) < 1.0e-15, true);
    expect_eq(absF64((erf(1.5) + erfc(1.5)) - 1.0) < 1.0e-14, true);
    expect_eq(absF64((erf(-1.5) + erfc(-1.5)) - 1.0) < 1.0e-14, true);

    // Far tail at x = 5.0: erfc(5.0) = 1.53745979442803485019e-12
    const ref_erfc5: f64 = 1.53745979442803485e-12;
    const got_erfc5: f64 = erfc(5.0);
    expect_eq(absF64(got_erfc5 - ref_erfc5) / ref_erfc5 < 1.0e-13, true);
});

test("6. Combinatorics & defensive guards: poles, invalid domains & NaN", () => {
    // logFactorial & logBinomial
    expect_eq(logFactorial(0), 0.0);
    expect_eq(logFactorial(1), 0.0);
    const lc10_3: f64 = logBinomial(10, 3) ?? unreachable;
    expect_eq(absF64(std.math.exp(lc10_3) - 120.0) < 1.0e-11, true);
    expect_eq(logBinomial(5, 6) == null, true);

    // Non-positive integer poles
    expect_eq(isNanF64(tgamma(0.0)), true);
    expect_eq(isNanF64(tgamma(-1.0)), true);
    expect_eq(isNanF64(tgamma(-4.0)), true);
    expect_eq(isNanF64(digamma(0.0)), true);
    expect_eq(isNanF64(digamma(-2.0)), true);
    expect_eq(lgamma(0.0) > 1.0e300, true);
    expect_eq(lgamma(-3.0) > 1.0e300, true);

    // NaN propagation
    const zero: f64 = 0.0;
    const nan_v: f64 = zero / zero;
    expect_eq(isNanF64(tgamma(nan_v)), true);
    expect_eq(isNanF64(lgamma(nan_v)), true);
    expect_eq(isNanF64(digamma(nan_v)), true);
    expect_eq(isNanF64(erf(nan_v)), true);
    expect_eq(isNanF64(erfc(nan_v)), true);

    // Domain guards for incomplete gamma/beta
    expect_eq(gammaP(0.0, 1.0) == null, true);
    expect_eq(gammaP(-1.0, 1.0) == null, true);
    expect_eq(gammaQ(1.0, -0.5) == null, true);
    expect_eq(gammaP(nan_v, 1.0) == null, true);
    expect_eq(lbeta(0.0, 2.0) == null, true);
    expect_eq(beta(2.0, -1.0) == null, true);
    expect_eq(betainc(-0.1, 2.0, 2.0) == null, true);
    expect_eq(betainc(1.1, 2.0, 2.0) == null, true);
    expect_eq(betainc(0.5, 0.0, 2.0) == null, true);
    expect_eq(betainc(nan_v, 2.0, 2.0) == null, true);
});
