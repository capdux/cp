// Origin: Pure-CP Stdlib (Zero-Alloc Special Math Functions: Gamma, Digamma, Incomplete Gamma/Beta & Erf/Erfc) | Created: 2026-11-22

import std from "std";

const PI: f64 = 3.14159265358979323846;
const LN_2PI_HALF: f64 = 0.91893853320467274178; // 0.5 * ln(2 * pi)
const LN_PI: f64 = 1.14472988584940017414;
const LN2: f64 = 0.69314718055994530942;
const FPMIN: f64 = 1.0e-300;
const CF_EPS: f64 = 1.0e-15;
const MAX_ITER: usize = 300;

// Lanczos g = 7, 9-term double-precision coefficients
const LANCZOS_G: f64 = 7.0;
const LANCZOS_P: [9]f64 = [
    0.99999999999980993227684700473478,
    676.520368121885098567009190444019,
    -1259.13921672240287047156078755283,
    771.3234287776530788486528258894,
    -176.61502916214059906584551354,
    12.507343278686904814458936853,
    -0.13857109526572011689554707,
    0.000009984369578019570859563,
    0.000000150563273514931155834,
];

fn isFiniteF64(x: f64): bool {
    return (x == x) && ((x - x) == 0.0);
}

fn isNanF64(x: f64): bool {
    return x != x;
}

fn nanF64(): f64 {
    const z: f64 = 0.0;
    return z / z;
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) (0.0 - x) else x;
}

fn isNonPositiveInt(x: f64): bool {
    if (x > 0.0 || !isFiniteF64(x)) {
        return false;
    }
    return std.math.floor(x) == x;
}

/// High-precision pure-CP natural logarithm for `x > 0` (~1e-16 accuracy).
fn lnF64(x: f64): f64 {
    let m: f64 = x;
    let k: i32 = 0;
    while (m >= 1.5) {
        m *= 0.5;
        k += 1;
    }
    while (m < 0.75) {
        m *= 2.0;
        k -= 1;
    }
    const t: f64 = (m - 1.0) / (m + 1.0);
    const t2: f64 = t * t;
    let term: f64 = t;
    let sum: f64 = 0.0;
    let denom: f64 = 1.0;
    let iter: usize = 0;
    while (iter < 12) {
        sum += term / denom;
        term *= t2;
        denom += 2.0;
        iter += 1;
    }
    return 2.0 * sum + (k as f64) * LN2;
}

fn lgammaPos(x: f64): f64 {
    const z: f64 = x - 1.0;
    let ag: f64 = LANCZOS_P[0];
    let k: usize = 1;
    while (k < 9) {
        ag += LANCZOS_P[k] / (z + (k as f64));
        k += 1;
    }
    const t: f64 = z + LANCZOS_G + 0.5;
    return LN_2PI_HALF + (z + 0.5) * lnF64(t) - t + lnF64(ag);
}

/// Computes `ln(|Gamma(x)|)` via 9-term Lanczos (`g = 7`) approximation and Euler reflection.
/// Returns `+inf` at non-positive integer poles and `NaN` for `NaN`.
pub fn lgamma(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return std.math.inf(f64);
    }
    if (isNonPositiveInt(x)) {
        return std.math.inf(f64);
    }
    if (x < 0.5) {
        const s: f64 = absF64(std.math.sin(PI * x));
        if (s == 0.0) {
            return std.math.inf(f64);
        }
        return LN_PI - lnF64(s) - lgammaPos(1.0 - x);
    }
    return lgammaPos(x);
}

/// Computes the Gamma function `Gamma(x)`.
/// Exact for small positive integers (`Gamma(n) = (n - 1)!`), uses Lanczos + reflection elsewhere.
/// Returns `NaN` at non-positive integer poles (`0, -1, -2, ...`) and for `NaN`.
pub fn tgamma(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return if (x > 0.0) std.math.inf(f64) else nanF64();
    }
    if (isNonPositiveInt(x)) {
        return nanF64();
    }
    // Exact integer factorials for 1..21
    if (x >= 1.0 && x <= 21.0 && std.math.floor(x) == x) {
        const n: usize = x as usize;
        let prod: f64 = 1.0;
        let i: usize = 2;
        while (i < n) {
            prod *= (i as f64);
            i += 1;
        }
        return prod;
    }
    if (x < 0.5) {
        const s: f64 = std.math.sin(PI * x);
        if (s == 0.0) {
            return nanF64();
        }
        return PI / (s * std.math.exp(lgammaPos(1.0 - x)));
    }
    return std.math.exp(lgammaPos(x));
}

/// Computes the Digamma function `psi(x) = d/dx ln(Gamma(x)) = Gamma'(x) / Gamma(x)`.
/// Uses reflection for `x < 0.5`, recurrence shift to `x >= 10`, and Bernoulli asymptotic series.
/// Returns `NaN` at non-positive integer poles or for `NaN`.
pub fn digamma(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return if (x > 0.0) std.math.inf(f64) else nanF64();
    }
    if (isNonPositiveInt(x)) {
        return nanF64();
    }

    let result: f64 = 0.0;
    let z: f64 = x;

    if (z < 0.5) {
        const s: f64 = std.math.sin(PI * z);
        const c: f64 = std.math.cos(PI * z);
        result -= PI * (c / s);
        z = 1.0 - z;
    }

    while (z < 10.0) {
        result -= 1.0 / z;
        z += 1.0;
    }

    const inv: f64 = 1.0 / z;
    const inv2: f64 = inv * inv;
    // Asymptotic expansion: ln(z) - 1/(2z) - sum_{k=1..7} B_{2k}/(2k * z^{2k})
    const poly: f64 = inv2 * (
        (1.0 / 12.0)
        + inv2 * (
            (0.0 - 1.0 / 120.0)
            + inv2 * (
                (1.0 / 252.0)
                + inv2 * (
                    (0.0 - 1.0 / 240.0)
                    + inv2 * (
                        (1.0 / 132.0)
                        + inv2 * (
                            (0.0 - 691.0 / 32760.0)
                            + inv2 * (1.0 / 12.0)
                        )
                    )
                )
            )
        )
    );
    return result + lnF64(z) - 0.5 * inv - poly;
}

/// Series expansion for lower regularized incomplete gamma `P(a, x)` (fast when `x < a + 1`).
fn gammaPSeries(a: f64, x: f64): f64 {
    if (x == 0.0) {
        return 0.0;
    }
    let ap: f64 = a;
    let term: f64 = 1.0 / a;
    let sum: f64 = term;
    let n: usize = 0;
    while (n < MAX_ITER) {
        ap += 1.0;
        term *= x / ap;
        sum += term;
        if (absF64(term) <= absF64(sum) * CF_EPS) {
            break;
        }
        n += 1;
    }
    const val: f64 = sum * std.math.exp(0.0 - x + a * lnF64(x) - lgamma(a));
    if (val < 0.0) { return 0.0; }
    if (val > 1.0) { return 1.0; }
    return val;
}

/// Modified Lentz continued fraction for upper regularized incomplete gamma `Q(a, x)` (`x >= a + 1`).
fn gammaQContFrac(a: f64, x: f64): f64 {
    let b: f64 = x + 1.0 - a;
    let c: f64 = 1.0 / FPMIN;
    let d: f64 = if (absF64(b) < FPMIN) (1.0 / FPMIN) else (1.0 / b);
    let h: f64 = d;

    let i: usize = 1;
    while (i <= MAX_ITER) {
        const fi: f64 = i as f64;
        const an: f64 = (0.0 - fi) * (fi - a);
        b += 2.0;
        d = an * d + b;
        if (absF64(d) < FPMIN) { d = FPMIN; }
        c = b + an / c;
        if (absF64(c) < FPMIN) { c = FPMIN; }
        d = 1.0 / d;
        const del: f64 = d * c;
        h *= del;
        if (absF64(del - 1.0) <= CF_EPS) {
            break;
        }
        i += 1;
    }
    const val: f64 = std.math.exp(0.0 - x + a * lnF64(x) - lgamma(a)) * h;
    if (val < 0.0) { return 0.0; }
    if (val > 1.0) { return 1.0; }
    return val;
}

/// Lower regularized incomplete gamma function `P(a, x) = gamma(a, x) / Gamma(a)`.
/// Requires `a > 0` and `x >= 0`; returns `null` on invalid inputs.
pub fn gammaP(a: f64, x: f64): f64? {
    if (!isFiniteF64(a) || !isFiniteF64(x) || a <= 0.0 || x < 0.0) {
        return null;
    }
    if (x == 0.0) {
        return 0.0;
    }
    if (x < a + 1.0) {
        return gammaPSeries(a, x);
    }
    return 1.0 - gammaQContFrac(a, x);
}

/// Upper regularized incomplete gamma function `Q(a, x) = Gamma(a, x) / Gamma(a) = 1 - P(a, x)`.
/// Requires `a > 0` and `x >= 0`; returns `null` on invalid inputs.
pub fn gammaQ(a: f64, x: f64): f64? {
    if (!isFiniteF64(a) || !isFiniteF64(x) || a <= 0.0 || x < 0.0) {
        return null;
    }
    if (x == 0.0) {
        return 1.0;
    }
    if (x < a + 1.0) {
        return 1.0 - gammaPSeries(a, x);
    }
    return gammaQContFrac(a, x);
}

/// Log Beta function `ln(B(a, b)) = lgamma(a) + lgamma(b) - lgamma(a + b)`.
/// Requires `a > 0, b > 0`.
pub fn lbeta(a: f64, b: f64): f64? {
    if (!isFiniteF64(a) || !isFiniteF64(b) || a <= 0.0 || b <= 0.0) {
        return null;
    }
    return lgamma(a) + lgamma(b) - lgamma(a + b);
}

/// Beta function `B(a, b) = Gamma(a) * Gamma(b) / Gamma(a + b)`.
/// Requires `a > 0, b > 0`.
pub fn beta(a: f64, b: f64): f64? {
    const lb: f64 = lbeta(a, b) ?? return null;
    return std.math.exp(lb);
}

fn betacf(x: f64, a: f64, b: f64): f64 {
    const one: f64 = 1.0;
    const qab: f64 = a + b;
    const qap: f64 = a + one;
    const qam: f64 = a - one;
    let c: f64 = one;
    let d: f64 = one - qab * x / qap;
    if (absF64(d) < FPMIN) { d = FPMIN; }
    d = one / d;
    let h: f64 = d;

    let m: usize = 1;
    while (m <= MAX_ITER) {
        const fm: f64 = m as f64;
        const m2: f64 = 2.0 * fm;

        // Even step
        const aa1: f64 = fm * (b - fm) * x / ((qam + m2) * (a + m2));
        d = one + aa1 * d;
        if (absF64(d) < FPMIN) { d = FPMIN; }
        c = one + aa1 / c;
        if (absF64(c) < FPMIN) { c = FPMIN; }
        d = one / d;
        h *= d * c;

        // Odd step
        const aa2: f64 = (0.0 - (a + fm)) * (qab + fm) * x / ((a + m2) * (qap + m2));
        d = one + aa2 * d;
        if (absF64(d) < FPMIN) { d = FPMIN; }
        c = one + aa2 / c;
        if (absF64(c) < FPMIN) { c = FPMIN; }
        d = one / d;
        const del: f64 = d * c;
        h *= del;
        if (absF64(del - one) <= CF_EPS) {
            break;
        }
        m += 1;
    }
    return h;
}

/// Regularized incomplete beta function `I_x(a, b)` for `0 <= x <= 1`, `a > 0`, `b > 0`.
/// Uses symmetry transform `x > (a + 1) / (a + b + 2)` + modified Lentz continued fraction.
pub fn betainc(x: f64, a: f64, b: f64): f64? {
    if (!isFiniteF64(x) || !isFiniteF64(a) || !isFiniteF64(b)) {
        return null;
    }
    if (a <= 0.0 || b <= 0.0 || x < 0.0 || x > 1.0) {
        return null;
    }
    if (x == 0.0) {
        return 0.0;
    }
    if (x == 1.0) {
        return 1.0;
    }
    const lb: f64 = lbeta(a, b) ?? return null;
    const bt: f64 = std.math.exp(a * lnF64(x) + b * lnF64(1.0 - x) - lb);
    let res: f64 = 0.0;
    if (x < (a + 1.0) / (a + b + 2.0)) {
        res = bt * betacf(x, a, b) / a;
    } else {
        res = 1.0 - bt * betacf(1.0 - x, b, a) / b;
    }
    if (res < 0.0) { return 0.0; }
    if (res > 1.0) { return 1.0; }
    return res;
}

/// Error function `erf(x) = (2 / sqrt(pi)) * integral_0^x exp(-t^2) dt`.
pub fn erf(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return if (x > 0.0) 1.0 else -1.0;
    }
    if (x == 0.0) {
        return 0.0;
    }
    const ax: f64 = absF64(x);
    const x2: f64 = ax * ax;
    const mag: f64 = if (x2 < 1.5)
        gammaPSeries(0.5, x2)
    else
        (1.0 - gammaQContFrac(0.5, x2));
    return if (x < 0.0) (0.0 - mag) else mag;
}

/// Complementary error function `erfc(x) = 1 - erf(x)`, accurate for large `x` without cancellation.
pub fn erfc(x: f64): f64 {
    if (isNanF64(x)) {
        return x;
    }
    if (!isFiniteF64(x)) {
        return if (x > 0.0) 0.0 else 2.0;
    }
    if (x == 0.0) {
        return 1.0;
    }
    const ax: f64 = absF64(x);
    const x2: f64 = ax * ax;
    if (x > 0.0) {
        if (ax > 27.5) {
            return 0.0;
        }
        return if (x2 < 1.5) (1.0 - gammaPSeries(0.5, x2)) else gammaQContFrac(0.5, x2);
    }
    const p: f64 = if (x2 < 1.5) gammaPSeries(0.5, x2) else (1.0 - gammaQContFrac(0.5, x2));
    return 1.0 + p;
}

/// Natural logarithm of `n!`: `ln(n!) = lgamma(n + 1)`.
pub fn logFactorial(n: u64): f64 {
    if (n <= 1) {
        return 0.0;
    }
    return lgamma((n as f64) + 1.0);
}

/// Natural logarithm of binomial coefficient `ln(C(n, k))`.
/// Returns `null` if `k > n`.
pub fn logBinomial(n: u64, k: u64): f64? {
    if (k > n) {
        return null;
    }
    if (k == 0 || k == n) {
        return 0.0;
    }
    return logFactorial(n) - logFactorial(k) - logFactorial(n - k);
}
