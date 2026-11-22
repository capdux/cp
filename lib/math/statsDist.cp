// Origin: Pure-CP Stdlib (Unbounded-Slice Probability Distributions, Hypothesis Testing & OLS/Ridge Regression Suite) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice statistical computing engine:
//   - High-precision special functions: 9-term Lanczos `logGamma` (< 1e-13), `regularizedIncompleteGammaP/Q`
//     (series + Modified Lentz CF), `regularizedIncompleteBeta` (symmetry + Modified Lentz CF),
//     `erfApprox` / `erfcApprox` (< 1e-14), and Acklam + Halley `normalInvCdf` (< 1e-13)
//   - 6 continuous/discrete probability distributions (PDF/PMF, CDF, PPF):
//     Normal, Student-t (Newton-Halley PPF), Chi-Square (Wilson-Hilferty + Newton PPF), F, Beta, Poisson
//   - Arbitrary-length slice descriptive statistics & hypothesis tests:
//     `describeSample`, `oneSampleTTest`, `welchTwoSampleTTest` (Welch-Satterthwaite df, 95% CI, Cohen's d),
//     `chiSquareGoodnessOfFit`, and `pearsonCorrelationTest`
//   - Simple OLS (`fitSimpleOls`) & Multivariate OLS / Ridge Regression (`fitRidgeRegression`, up to 16 features
//     over arbitrary sample count `N` via partial-pivoting Gauss-Jordan normal equations)

const std = @import("std");

// ============================================================================
// 1. Constants, Error Set & Internal Numerical Utilities
// ============================================================================

const PI_F64: f64 = 3.14159265358979323846;
const SQRT_2PI_F64: f64 = 2.50662827463100050241;
const LN_SQRT_2PI_F64: f64 = 0.91893853320467274178;
const SQRT_2_F64: f64 = 1.41421356237309504880;
const FPMIN_F64: f64 = 1.0e-300;
const SERIES_EPS_F64: f64 = 1.0e-14;
const MAX_ITER: usize = 300;

/// Maximum number of explanatory features supported by `fitRidgeRegression` (excluding intercept).
pub const STATS_MAX_FEATURES: usize = 16;

const MAX_AUG_DIM: usize = STATS_MAX_FEATURES + 1;

/// Error set returned by statistical distribution, hypothesis testing, and regression routines.
pub err StatsDistError {
    EmptySample,
    InsufficientSamples,
    InvalidParameter,
    LengthMismatch,
    ZeroVariance,
    SingularMatrix,
    BufferTooSmall,
}

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn clampF64(x: f64, lo: f64, hi: f64): f64 {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

// ============================================================================
// 2. High-Precision Special Functions
// ============================================================================

/// Computes `ln(Gamma(x))` for `x > 0` using the 9-term Lanczos approximation (`g = 7`, accuracy < 1e-13).
pub fn logGamma(x_in: f64): f64 {
    if (!isFinite(x_in) || x_in <= 0.0) {
        return std.math.nan(f64);
    }

    if (x_in < 0.5) {
        // Euler reflection formula: Gamma(x) * Gamma(1 - x) = pi / sin(pi * x)
        const sin_px: f64 = std.math.sin(PI_F64 * x_in);
        if (absF64(sin_px) <= FPMIN_F64) {
            return std.math.inf(f64);
        }
        return std.math.log(f64, std.math.e, PI_F64 / absF64(sin_px)) - logGamma(1.0 - x_in);
    }

    const p: [9]f64 = [
        0.99999999999980993,
        676.5203681218851,
        -1259.1392167224028,
        771.32342877765313,
        -176.61502916214059,
        12.507343278686905,
        -0.13857109526572012,
        9.9843695780195716e-6,
        1.5056327351493116e-7,
    ];

    const z: f64 = x_in - 1.0;
    let acc: f64 = p[0];
    let i: usize = 1;
    while (i < 9) : (i += 1) {
        acc += p[i] / (z + @as(f64, @floatFromInt(i)));
    }
    const t: f64 = z + 7.5;
    return LN_SQRT_2PI_F64 + (z + 0.5) * std.math.log(f64, std.math.e, t) - t + std.math.log(f64, std.math.e, acc);
}

fn gammaSeriesP(a: f64, x: f64): f64 {
    const gln: f64 = logGamma(a);
    if (x <= 0.0) {
        return 0.0;
    }
    let ap: f64 = a;
    let term: f64 = 1.0 / a;
    let sum: f64 = term;
    let n: usize = 1;
    while (n <= MAX_ITER) : (n += 1) {
        ap += 1.0;
        term *= x / ap;
        sum += term;
        if (absF64(term) < absF64(sum) * SERIES_EPS_F64) {
            break;
        }
    }
    const val: f64 = sum * std.math.exp(-x + a * std.math.log(f64, std.math.e, x) - gln);
    return clampF64(val, 0.0, 1.0);
}

fn gammaContinuedFracQ(a: f64, x: f64): f64 {
    const gln: f64 = logGamma(a);
    let b: f64 = x + 1.0 - a;
    let c: f64 = 1.0 / FPMIN_F64;
    let d: f64 = 1.0 / b;
    let h: f64 = d;

    let i: usize = 1;
    while (i <= MAX_ITER) : (i += 1) {
        const fi: f64 = @as(f64, @floatFromInt(i));
        const an: f64 = -fi * (fi - a);
        b += 2.0;
        d = an * d + b;
        if (absF64(d) < FPMIN_F64) d = FPMIN_F64;
        c = b + an / c;
        if (absF64(c) < FPMIN_F64) c = FPMIN_F64;
        d = 1.0 / d;
        const del: f64 = d * c;
        h *= del;
        if (absF64(del - 1.0) < SERIES_EPS_F64) {
            break;
        }
    }
    const val: f64 = std.math.exp(-x + a * std.math.log(f64, std.math.e, x) - gln) * h;
    return clampF64(val, 0.0, 1.0);
}

/// Computes the lower regularized incomplete gamma function `P(a, x)` for `a > 0, x >= 0`.
pub fn regularizedIncompleteGammaP(a: f64, x: f64): f64 {
    if (!isFinite(a) || !isFinite(x) || a <= 0.0 || x < 0.0) {
        return std.math.nan(f64);
    }
    if (x == 0.0) {
        return 0.0;
    }
    if (x < a + 1.0) {
        return gammaSeriesP(a, x);
    }
    return clampF64(1.0 - gammaContinuedFracQ(a, x), 0.0, 1.0);
}

/// Computes the upper regularized incomplete gamma function `Q(a, x) = 1 - P(a, x)` for `a > 0, x >= 0`.
pub fn regularizedIncompleteGammaQ(a: f64, x: f64): f64 {
    if (!isFinite(a) || !isFinite(x) || a <= 0.0 || x < 0.0) {
        return std.math.nan(f64);
    }
    if (x == 0.0) {
        return 1.0;
    }
    if (x < a + 1.0) {
        return clampF64(1.0 - gammaSeriesP(a, x), 0.0, 1.0);
    }
    return gammaContinuedFracQ(a, x);
}

/// Computes the Gauss error function `erf(x)` to near machine precision (~1e-14).
pub fn erfApprox(x: f64): f64 {
    if (!isFinite(x)) {
        if (std.math.isNan(x)) return x;
        return if (x > 0.0) 1.0 else -1.0;
    }
    if (x == 0.0) {
        return 0.0;
    }
    const ax: f64 = absF64(x);
    const mag: f64 = regularizedIncompleteGammaP(0.5, ax * ax);
    return if (x < 0.0) -mag else mag;
}

/// Computes the complementary error function `erfc(x) = 1 - erf(x)` without tail cancellation.
pub fn erfcApprox(x: f64): f64 {
    if (!isFinite(x)) {
        if (std.math.isNan(x)) return x;
        return if (x > 0.0) 0.0 else 2.0;
    }
    if (x == 0.0) {
        return 1.0;
    }
    const ax: f64 = absF64(x);
    const q: f64 = regularizedIncompleteGammaQ(0.5, ax * ax);
    return if (x >= 0.0) q else 2.0 - q;
}

/// Computes the inverse standard normal CDF `Phi^-1(p)` for `0 < p < 1` using Acklam's rational
/// approximation followed by one Halley refinement step (accuracy < 1e-13 across the full domain).
pub fn normalInvCdf(p: f64): f64 {
    if (!isFinite(p) || p <= 0.0 || p >= 1.0) {
        return std.math.nan(f64);
    }
    if (p == 0.5) {
        return 0.0;
    }

    const a1: f64 = -3.969683028665376e+01;
    const a2: f64 = 2.209460984245205e+02;
    const a3: f64 = -2.759285104469687e+02;
    const a4: f64 = 1.383577518672690e+02;
    const a5: f64 = -3.066479806614716e+01;
    const a6: f64 = 2.506628277459239e+00;

    const b1: f64 = -5.447609879822406e+01;
    const b2: f64 = 1.615858368580409e+02;
    const b3: f64 = -1.556989798598866e+02;
    const b4: f64 = 6.680131188771972e+01;
    const b5: f64 = -1.328068155288572e+01;

    const c1: f64 = -7.784894002430293e-03;
    const c2: f64 = -3.223964580411365e-01;
    const c3: f64 = -2.400758277161838e+00;
    const c4: f64 = -2.549732539343734e+00;
    const c5: f64 = 4.374664141464968e+00;
    const c6: f64 = 2.938163982698783e+00;

    const d1: f64 = 7.784695709041462e-03;
    const d2: f64 = 3.224671290700398e-01;
    const d3: f64 = 2.445134137142996e+00;
    const d4: f64 = 3.754408661907416e+00;

    const p_low: f64 = 0.02425;
    const p_high: f64 = 1.0 - p_low;

    let x: f64 = 0.0;
    if (p < p_low) {
        const q: f64 = std.math.sqrt(-2.0 * std.math.log(f64, std.math.e, p));
        x = (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
            ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    } else if (p <= p_high) {
        const q: f64 = p - 0.5;
        const r: f64 = q * q;
        x = (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q /
            (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
    } else {
        const q: f64 = std.math.sqrt(-2.0 * std.math.log(f64, std.math.e, 1.0 - p));
        x = -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
            ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }

    // One Halley refinement step using full-precision erfcApprox.
    const err_p: f64 = 0.5 * erfcApprox(-x / SQRT_2_F64) - p;
    const u: f64 = err_p * SQRT_2PI_F64 * std.math.exp(0.5 * x * x);
    x -= u / (1.0 + 0.5 * x * u);
    return x;
}

fn betaContinuedFrac(a: f64, b: f64, x: f64): f64 {
    const qab: f64 = a + b;
    const qap: f64 = a + 1.0;
    const qam: f64 = a - 1.0;

    let c: f64 = 1.0;
    let d: f64 = 1.0 - qab * x / qap;
    if (absF64(d) < FPMIN_F64) d = FPMIN_F64;
    d = (1.0 as f64) / d;
    let h: f64 = d;

    let m: usize = 1;
    while (m <= MAX_ITER) : (m += 1) {
        const fm: f64 = @as(f64, @floatFromInt(m));
        const m2: f64 = 2.0 * fm;

        // Even step
        let aa: f64 = fm * (b - fm) * x / ((qam + m2) * (a + m2));
        d = (1.0 as f64) + aa * d;
        if (absF64(d) < FPMIN_F64) d = FPMIN_F64;
        c = (1.0 as f64) + aa / c;
        if (absF64(c) < FPMIN_F64) c = FPMIN_F64;
        d = (1.0 as f64) / d;
        h *= d * c;

        // Odd step
        aa = -(a + fm) * (qab + fm) * x / ((a + m2) * (qap + m2));
        d = (1.0 as f64) + aa * d;
        if (absF64(d) < FPMIN_F64) d = FPMIN_F64;
        c = (1.0 as f64) + aa / c;
        if (absF64(c) < FPMIN_F64) c = FPMIN_F64;
        d = (1.0 as f64) / d;
        const del: f64 = d * c;
        h *= del;
        if (absF64(del - 1.0) < SERIES_EPS_F64) {
            break;
        }
    }
    return h;
}

/// Computes the regularized incomplete beta function `I_x(a, b)` for `a > 0, b > 0, 0 <= x <= 1`.
pub fn regularizedIncompleteBeta(a: f64, b: f64, x: f64): f64 {
    if (!isFinite(a) || !isFinite(b) || !isFinite(x) || a <= 0.0 || b <= 0.0 || x < 0.0 || x > 1.0) {
        return std.math.nan(f64);
    }
    if (x == 0.0) return 0.0;
    if (x == 1.0) return 1.0;

    const ln_bt: f64 = logGamma(a + b) - logGamma(a) - logGamma(b) +
        a * std.math.log(f64, std.math.e, x) +
        b * std.math.log(f64, std.math.e, 1.0 - x);
    const bt: f64 = std.math.exp(ln_bt);

    if (x < (a + 1.0) / (a + b + 2.0)) {
        return clampF64(bt * betaContinuedFrac(a, b, x) / a, 0.0, 1.0);
    }
    return clampF64(1.0 - bt * betaContinuedFrac(b, a, 1.0 - x) / b, 0.0, 1.0);
}

// ============================================================================
// 3. Probability Distributions (Normal, Student-t, Chi-Square, F, Beta, Poisson)
// ============================================================================

/// Normal distribution probability density function `N(mu, sigma^2)`.
pub fn normalPdf(x: f64, mu: f64, sigma: f64): f64 ! StatsDistError {
    if (!isFinite(x) || !isFinite(mu) || !isFinite(sigma) || sigma <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    const z: f64 = (x - mu) / sigma;
    return std.math.exp(-0.5 * z * z) / (sigma * SQRT_2PI_F64);
}

/// Normal distribution cumulative distribution function `P(X <= x)`.
pub fn normalCdf(x: f64, mu: f64, sigma: f64): f64 ! StatsDistError {
    if (!isFinite(x) || !isFinite(mu) || !isFinite(sigma) || sigma <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    const z: f64 = (x - mu) / sigma;
    return clampF64(0.5 * erfcApprox(-z / SQRT_2_F64), 0.0, 1.0);
}

/// Normal distribution inverse CDF (quantile function) for `0 < p < 1`.
pub fn normalPpf(p: f64, mu: f64, sigma: f64): f64 ! StatsDistError {
    if (!isFinite(p) || !isFinite(mu) || !isFinite(sigma) || sigma <= 0.0 || p <= 0.0 || p >= 1.0) {
        return StatsDistError.InvalidParameter;
    }
    return mu + sigma * normalInvCdf(p);
}

/// Student's t-distribution PDF with `df > 0` degrees of freedom.
pub fn studentTPdf(t: f64, df: f64): f64 ! StatsDistError {
    if (!isFinite(t) || !isFinite(df) || df <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    const ln_c: f64 = logGamma(0.5 * (df + 1.0)) - logGamma(0.5 * df) - 0.5 * std.math.log(f64, std.math.e, df * PI_F64);
    const ln_body: f64 = -0.5 * (df + 1.0) * std.math.log(f64, std.math.e, 1.0 + (t * t) / df);
    return std.math.exp(ln_c + ln_body);
}

/// Student's t-distribution CDF `P(T <= t)` with `df > 0` degrees of freedom.
pub fn studentTCdf(t: f64, df: f64): f64 ! StatsDistError {
    if (!isFinite(t) || !isFinite(df) || df <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    if (t == 0.0) {
        return 0.5;
    }
    const x: f64 = df / (df + t * t);
    const ib: f64 = regularizedIncompleteBeta(0.5 * df, 0.5, x);
    if (t > 0.0) {
        return clampF64(1.0 - 0.5 * ib, 0.0, 1.0);
    }
    return clampF64(0.5 * ib, 0.0, 1.0);
}

/// Student's t-distribution inverse CDF (quantile function) via safeguarded Newton-Halley iteration.
pub fn studentTPpf(p: f64, df: f64): f64 ! StatsDistError {
    if (!isFinite(p) || !isFinite(df) || df <= 0.0 || p <= 0.0 || p >= 1.0) {
        return StatsDistError.InvalidParameter;
    }
    if (p == 0.5) {
        return 0.0;
    }
    // Exploit symmetry around p = 0.5: solve for q in (0.5, 1.0) then apply sign.
    const sign: f64 = if (p < 0.5) -1.0 else 1.0;
    const q: f64 = if (p < 0.5) 1.0 - p else p;

    // Exact closed form for df == 1 (Cauchy) and df == 2.
    if (absF64(df - 1.0) < 1.0e-14) {
        return sign * std.math.tan(PI_F64 * (q - 0.5));
    }
    if (absF64(df - 2.0) < 1.0e-14) {
        const a: f64 = 2.0 * q - 1.0;
        return sign * a * std.math.sqrt(2.0 / (1.0 - a * a));
    }

    // Cornish-Fisher initial guess for upper quantile.
    const z: f64 = normalInvCdf(q);
    const z2: f64 = z * z;
    let x: f64 = z + (z * (z2 + 1.0)) / (4.0 * df) + (z * (5.0 * z2 * z2 + 16.0 * z2 + 3.0)) / (96.0 * df * df);
    if (x <= 0.0) x = z;

    // Bracket [lo, hi] guaranteeing guaranteed convergence even for small df.
    let lo: f64 = 0.0;
    let hi: f64 = if (x > 1.0) x * 2.0 else 2.0;
    while (studentTCdf(hi, df)? < q) {
        hi *= 2.0;
        if (hi > 1.0e12) break;
    }
    x = clampF64(x, lo + 1.0e-12, hi - 1.0e-12);

    let iter: usize = 0;
    while (iter < 64) : (iter += 1) {
        const cdf_x: f64 = studentTCdf(x, df)?;
        const f: f64 = cdf_x - q;
        if (absF64(f) < 1.0e-13) {
            break;
        }
        if (f < 0.0) {
            lo = x;
        } else {
            hi = x;
        }

        const pdf_x: f64 = studentTPdf(x, df)?;
        let next_x: f64 = 0.5 * (lo + hi);
        if (pdf_x > FPMIN_F64) {
            // Halley's method using f''/f' = -(df + 1) * x / (df + x^2)
            const ratio_f2_f1: f64 = -(df + 1.0) * x / (df + x * x);
            const newton_step: f64 = f / pdf_x;
            const denom: f64 = 1.0 - 0.5 * newton_step * ratio_f2_f1;
            const candidate: f64 = if (absF64(denom) > 0.1) x - newton_step / denom else x - newton_step;
            if (candidate > lo && candidate < hi) {
                next_x = candidate;
            }
        }
        if (absF64(next_x - x) <= 1.0e-12 * (1.0 + absF64(x))) {
            x = next_x;
            break;
        }
        x = next_x;
    }

    return sign * x;
}

/// Chi-square distribution PDF with `df > 0` degrees of freedom.
pub fn chiSquarePdf(x: f64, df: f64): f64 ! StatsDistError {
    if (!isFinite(x) || !isFinite(df) || df <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    if (x < 0.0) return 0.0;
    if (x == 0.0) {
        if (df < 2.0) return std.math.inf(f64);
        if (df == 2.0) return 0.5;
        return 0.0;
    }
    const k2: f64 = 0.5 * df;
    const ln_val: f64 = (k2 - 1.0) * std.math.log(f64, std.math.e, x) - 0.5 * x -
        k2 * std.math.log(f64, std.math.e, 2.0) - logGamma(k2);
    return std.math.exp(ln_val);
}

/// Chi-square distribution CDF `P(X <= x)` with `df > 0` degrees of freedom.
pub fn chiSquareCdf(x: f64, df: f64): f64 ! StatsDistError {
    if (!isFinite(x) || !isFinite(df) || df <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    if (x <= 0.0) return 0.0;
    return regularizedIncompleteGammaP(0.5 * df, 0.5 * x);
}

/// Chi-square distribution inverse CDF (quantile function) via Wilson-Hilferty + safeguarded Newton.
pub fn chiSquarePpf(p: f64, df: f64): f64 ! StatsDistError {
    if (!isFinite(p) || !isFinite(df) || df <= 0.0 || p <= 0.0 || p >= 1.0) {
        return StatsDistError.InvalidParameter;
    }

    // Wilson-Hilferty cube-root initial guess.
    const z: f64 = normalInvCdf(p);
    const term: f64 = 1.0 - 2.0 / (9.0 * df) + z * std.math.sqrt(2.0 / (9.0 * df));
    let x: f64 = if (term > 0.05) df * term * term * term else 0.01 * df;
    if (x <= 1.0e-12) x = 1.0e-6 as f64;

    let lo: f64 = 0.0;
    let hi: f64 = if (x > 1.0) x * 2.0 else 4.0;
    while (chiSquareCdf(hi, df)? < p) {
        hi *= 2.0;
        if (hi > 1.0e12) break;
    }
    x = clampF64(x, lo + 1.0e-12, hi - 1.0e-12);

    let iter: usize = 0;
    while (iter < 64) : (iter += 1) {
        const f: f64 = chiSquareCdf(x, df)? - p;
        if (absF64(f) < 1.0e-13) {
            break;
        }
        if (f < 0.0) {
            lo = x;
        } else {
            hi = x;
        }
        const pdf_x: f64 = chiSquarePdf(x, df)?;
        let next_x: f64 = 0.5 * (lo + hi);
        if (pdf_x > FPMIN_F64) {
            const candidate: f64 = x - f / pdf_x;
            if (candidate > lo && candidate < hi) {
                next_x = candidate;
            }
        }
        if (absF64(next_x - x) <= 1.0e-12 * (1.0 + absF64(x))) {
            x = next_x;
            break;
        }
        x = next_x;
    }
    return x;
}

/// F-distribution CDF `P(F <= x)` with numerator `df1 > 0` and denominator `df2 > 0`.
pub fn fDistCdf(x: f64, df1: f64, df2: f64): f64 ! StatsDistError {
    if (!isFinite(x) || !isFinite(df1) || !isFinite(df2) || df1 <= 0.0 || df2 <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    if (x <= 0.0) return 0.0;
    const z: f64 = (df1 * x) / (df1 * x + df2);
    return regularizedIncompleteBeta(0.5 * df1, 0.5 * df2, z);
}

/// Beta distribution PDF on `[0, 1]` with shape parameters `alpha > 0, beta > 0`.
pub fn betaPdf(x: f64, alpha: f64, beta: f64): f64 ! StatsDistError {
    if (!isFinite(x) || !isFinite(alpha) || !isFinite(beta) || alpha <= 0.0 || beta <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    if (x < 0.0 || x > 1.0) return 0.0;
    if (x == 0.0) {
        if (alpha < 1.0) return std.math.inf(f64);
        if (alpha == 1.0) return beta;
        return 0.0;
    }
    if (x == 1.0) {
        if (beta < 1.0) return std.math.inf(f64);
        if (beta == 1.0) return alpha;
        return 0.0;
    }
    const ln_val: f64 = logGamma(alpha + beta) - logGamma(alpha) - logGamma(beta) +
        (alpha - 1.0) * std.math.log(f64, std.math.e, x) +
        (beta - 1.0) * std.math.log(f64, std.math.e, 1.0 - x);
    return std.math.exp(ln_val);
}

/// Beta distribution CDF `P(X <= x)` on `[0, 1]` with `alpha > 0, beta > 0`.
pub fn betaCdf(x: f64, alpha: f64, beta: f64): f64 ! StatsDistError {
    if (!isFinite(x) || !isFinite(alpha) || !isFinite(beta) || alpha <= 0.0 || beta <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    if (x <= 0.0) return 0.0;
    if (x >= 1.0) return 1.0;
    return regularizedIncompleteBeta(alpha, beta, x);
}

/// Poisson probability mass function `P(X = k)` for rate `lambda > 0`.
pub fn poissonPmf(k: u64, lambda: f64): f64 ! StatsDistError {
    if (!isFinite(lambda) || lambda <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    const fk: f64 = @as(f64, @floatFromInt(k));
    const ln_val: f64 = fk * std.math.log(f64, std.math.e, lambda) - lambda - logGamma(fk + 1.0);
    return std.math.exp(ln_val);
}

/// Poisson cumulative distribution function `P(X <= k) = Q(k + 1, lambda)` for `lambda > 0`.
pub fn poissonCdf(k: u64, lambda: f64): f64 ! StatsDistError {
    if (!isFinite(lambda) || lambda <= 0.0) {
        return StatsDistError.InvalidParameter;
    }
    return regularizedIncompleteGammaQ(@as(f64, @floatFromInt(k + 1)), lambda);
}

// ============================================================================
// 4. Descriptive Statistics & Hypothesis Testing (Unbounded Slices)
// ============================================================================

/// Comprehensive sample descriptive statistics computed with two-pass numerical stability.
pub struct DescriptiveStats {
    count: usize = 0;
    mean: f64 = 0.0;
    variance: f64 = 0.0;
    std_dev: f64 = 0.0;
    skewness: f64 = 0.0;
    kurtosis: f64 = 0.0;
    min: f64 = 0.0;
    max: f64 = 0.0;
    sem: f64 = 0.0;
}

/// Computes descriptive statistics (unbiased sample variance, Fisher-Pearson adjusted skewness &
/// excess kurtosis, min/max, and standard error of the mean) for an arbitrary-length slice.
pub fn describeSample(xs: []const f64): DescriptiveStats ! StatsDistError {
    if (xs.len == 0) {
        return StatsDistError.EmptySample;
    }

    const n: usize = xs.len;
    const nf: f64 = @as(f64, @floatFromInt(n));

    let sum: f64 = 0.0;
    let min_v: f64 = xs[0];
    let max_v: f64 = xs[0];
    let i: usize = 0;
    while (i < n) : (i += 1) {
        const v: f64 = xs[i];
        if (!isFinite(v)) {
            return StatsDistError.InvalidParameter;
        }
        sum += v;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }
    const mean: f64 = sum / nf;

    // Two-pass compensated central moments (m2, m3, m4).
    let comp_sum: f64 = 0.0;
    let m2: f64 = 0.0;
    let m3: f64 = 0.0;
    let m4: f64 = 0.0;
    let j: usize = 0;
    while (j < n) : (j += 1) {
        const d: f64 = xs[j] - mean;
        comp_sum += d;
        const d2: f64 = d * d;
        m2 += d2;
        m3 += d2 * d;
        m4 += d2 * d2;
    }
    // Apply two-pass correction to m2 to eliminate residual rounding error.
    let corrected_m2: f64 = m2 - (comp_sum * comp_sum) / nf;
    if (corrected_m2 < 0.0) corrected_m2 = 0.0;

    const variance: f64 = if (n > 1) corrected_m2 / (nf - 1.0) else 0.0;
    const std_dev: f64 = std.math.sqrt(variance);
    const sem: f64 = if (n > 1) std_dev / std.math.sqrt(nf) else 0.0;

    let skewness: f64 = 0.0;
    if (n >= 3 && std_dev > 0.0) {
        const s3: f64 = std_dev * std_dev * std_dev;
        skewness = (nf / ((nf - 1.0) * (nf - 2.0))) * (m3 / s3);
    }

    let kurtosis: f64 = 0.0;
    if (n >= 4 && variance > 0.0) {
        const s4: f64 = variance * variance;
        const term1: f64 = (nf * (nf + 1.0)) / ((nf - 1.0) * (nf - 2.0) * (nf - 3.0)) * (m4 / s4);
        const term2: f64 = (3.0 * (nf - 1.0) * (nf - 1.0)) / ((nf - 2.0) * (nf - 3.0));
        kurtosis = term1 - term2;
    }

    return DescriptiveStats{
        count: n,
        mean: mean,
        variance: variance,
        std_dev: std_dev,
        skewness: skewness,
        kurtosis: kurtosis,
        min: min_v,
        max: max_v,
        sem: sem,
    };
}

/// Output of one-sample or Welch two-sample Student's t-test.
pub struct TTestResult {
    t_stat: f64 = 0.0;
    df: f64 = 0.0;
    p_value_two_tailed: f64 = 1.0;
    mean_diff: f64 = 0.0;
    ci95_low: f64 = 0.0;
    ci95_high: f64 = 0.0;
    cohens_d: f64 = 0.0;
}

/// Performs a two-tailed one-sample Student's t-test against hypothesized mean `mu0`.
pub fn oneSampleTTest(xs: []const f64, mu0: f64): TTestResult ! StatsDistError {
    if (!isFinite(mu0)) {
        return StatsDistError.InvalidParameter;
    }
    if (xs.len == 0) {
        return StatsDistError.EmptySample;
    }
    if (xs.len < 2) {
        return StatsDistError.InsufficientSamples;
    }

    const st = describeSample(xs)?;
    if (st.std_dev <= 0.0) {
        return StatsDistError.ZeroVariance;
    }

    const df: f64 = @as(f64, @floatFromInt(xs.len - 1));
    const mean_diff: f64 = st.mean - mu0;
    const t_stat: f64 = mean_diff / st.sem;
    const cdf_abs: f64 = studentTCdf(absF64(t_stat), df)?;
    const p_val: f64 = clampF64(2.0 * (1.0 - cdf_abs), 0.0, 1.0);
    const t_crit: f64 = studentTPpf(0.975, df)?;
    const margin: f64 = t_crit * st.sem;

    return TTestResult{
        t_stat: t_stat,
        df: df,
        p_value_two_tailed: p_val,
        mean_diff: mean_diff,
        ci95_low: mean_diff - margin,
        ci95_high: mean_diff + margin,
        cohens_d: mean_diff / st.std_dev,
    };
}

/// Performs a two-tailed Welch's unequal-variance two-sample t-test with Welch-Satterthwaite
/// degrees of freedom, 95% confidence interval for `(mean(xs) - mean(ys))`, and pooled Cohen's d.
pub fn welchTwoSampleTTest(xs: []const f64, ys: []const f64): TTestResult ! StatsDistError {
    if (xs.len == 0 || ys.len == 0) {
        return StatsDistError.EmptySample;
    }
    if (xs.len < 2 || ys.len < 2) {
        return StatsDistError.InsufficientSamples;
    }

    const sx = describeSample(xs)?;
    const sy = describeSample(ys)?;
    const nx: f64 = @as(f64, @floatFromInt(xs.len));
    const ny: f64 = @as(f64, @floatFromInt(ys.len));

    const vx_n: f64 = sx.variance / nx;
    const vy_n: f64 = sy.variance / ny;
    const se2: f64 = vx_n + vy_n;
    if (se2 <= 0.0) {
        return StatsDistError.ZeroVariance;
    }

    const se: f64 = std.math.sqrt(se2);
    const mean_diff: f64 = sx.mean - sy.mean;
    const t_stat: f64 = mean_diff / se;

    const denom_df: f64 = (vx_n * vx_n) / (nx - 1.0) + (vy_n * vy_n) / (ny - 1.0);
    const df: f64 = (se2 * se2) / denom_df;
    const cdf_abs: f64 = studentTCdf(absF64(t_stat), df)?;
    const p_val: f64 = clampF64(2.0 * (1.0 - cdf_abs), 0.0, 1.0);

    const t_crit: f64 = studentTPpf(0.975, df)?;
    const margin: f64 = t_crit * se;

    const pooled_var: f64 = ((nx - 1.0) * sx.variance + (ny - 1.0) * sy.variance) / (nx + ny - 2.0);
    const cohens_d: f64 = if (pooled_var > 0.0) mean_diff / std.math.sqrt(pooled_var) else 0.0;

    return TTestResult{
        t_stat: t_stat,
        df: df,
        p_value_two_tailed: p_val,
        mean_diff: mean_diff,
        ci95_low: mean_diff - margin,
        ci95_high: mean_diff + margin,
        cohens_d: cohens_d,
    };
}

/// Output of Pearson's chi-square goodness-of-fit test.
pub struct ChiSquareTestResult {
    chi2_stat: f64 = 0.0;
    df: f64 = 0.0;
    p_value: f64 = 1.0;
}

/// Performs Pearson's chi-square goodness-of-fit test across arbitrary-length category count slices.
pub fn chiSquareGoodnessOfFit(
    observed: []const f64,
    expected: []const f64,
): ChiSquareTestResult ! StatsDistError {
    if (observed.len != expected.len) {
        return StatsDistError.LengthMismatch;
    }
    if (observed.len == 0) {
        return StatsDistError.EmptySample;
    }
    if (observed.len < 2) {
        return StatsDistError.InsufficientSamples;
    }

    let chi2: f64 = 0.0;
    let i: usize = 0;
    while (i < observed.len) : (i += 1) {
        const o: f64 = observed[i];
        const e: f64 = expected[i];
        if (!isFinite(o) || !isFinite(e) || o < 0.0 || e <= 0.0) {
            return StatsDistError.InvalidParameter;
        }
        const diff: f64 = o - e;
        chi2 += (diff * diff) / e;
    }

    const df: f64 = @as(f64, @floatFromInt(observed.len - 1));
    const p_val: f64 = regularizedIncompleteGammaQ(0.5 * df, 0.5 * chi2);
    return ChiSquareTestResult{
        chi2_stat: chi2,
        df: df,
        p_value: p_val,
    };
}

/// Output of Pearson product-moment correlation hypothesis test.
pub struct CorrelationResult {
    r: f64 = 0.0;
    t_stat: f64 = 0.0;
    p_value: f64 = 1.0;
    n: usize = 0;
}

/// Computes Pearson correlation `r`, Student-t test statistic, and exact two-tailed `p_value`
/// across arbitrary-length paired slices `xs` and `ys`.
pub fn pearsonCorrelationTest(
    xs: []const f64,
    ys: []const f64,
): CorrelationResult ! StatsDistError {
    if (xs.len != ys.len) {
        return StatsDistError.LengthMismatch;
    }
    if (xs.len == 0) {
        return StatsDistError.EmptySample;
    }
    if (xs.len < 3) {
        return StatsDistError.InsufficientSamples;
    }

    const n: usize = xs.len;
    const nf: f64 = @as(f64, @floatFromInt(n));
    let sum_x: f64 = 0.0;
    let sum_y: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (!isFinite(xs[i]) || !isFinite(ys[i])) {
            return StatsDistError.InvalidParameter;
        }
        sum_x += xs[i];
        sum_y += ys[i];
    }
    const mx: f64 = sum_x / nf;
    const my: f64 = sum_y / nf;

    let sxx: f64 = 0.0;
    let syy: f64 = 0.0;
    let sxy: f64 = 0.0;
    let j: usize = 0;
    while (j < n) : (j += 1) {
        const dx: f64 = xs[j] - mx;
        const dy: f64 = ys[j] - my;
        sxx += dx * dx;
        syy += dy * dy;
        sxy += dx * dy;
    }

    if (sxx <= 0.0 || syy <= 0.0) {
        return StatsDistError.ZeroVariance;
    }

    const r: f64 = clampF64(sxy / std.math.sqrt(sxx * syy), -1.0, 1.0);
    const df: f64 = nf - 2.0;

    if (absF64(r) >= 1.0 - 1.0e-15) {
        const inf_t: f64 = if (r >= 0.0) std.math.inf(f64) else -std.math.inf(f64);
        return CorrelationResult{
            r: r,
            t_stat: inf_t,
            p_value: 0.0,
            n: n,
        };
    }

    const t_stat: f64 = r * std.math.sqrt(df / (1.0 - r * r));
    const p_val: f64 = clampF64(2.0 * (1.0 - studentTCdf(absF64(t_stat), df)?), 0.0, 1.0);
    return CorrelationResult{
        r: r,
        t_stat: t_stat,
        p_value: p_val,
        n: n,
    };
}

// ============================================================================
// 5. Simple OLS & Multivariate OLS / Ridge Linear Regression
// ============================================================================

/// Result of univariate Ordinary Least Squares regression `y = intercept + slope * x`.
pub struct SimpleRegressionResult {
    slope: f64 = 0.0;
    intercept: f64 = 0.0;
    r_squared: f64 = 0.0;
    rmse: f64 = 0.0;
    std_err_slope: f64 = 0.0;
    p_value_slope: f64 = 1.0;
}

/// Fits univariate OLS linear regression across arbitrary-length paired slices `xs` and `ys`.
pub fn fitSimpleOls(
    xs: []const f64,
    ys: []const f64,
): SimpleRegressionResult ! StatsDistError {
    if (xs.len != ys.len) {
        return StatsDistError.LengthMismatch;
    }
    if (xs.len == 0) {
        return StatsDistError.EmptySample;
    }
    if (xs.len < 3) {
        return StatsDistError.InsufficientSamples;
    }

    const n: usize = xs.len;
    const nf: f64 = @as(f64, @floatFromInt(n));
    let sum_x: f64 = 0.0;
    let sum_y: f64 = 0.0;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        if (!isFinite(xs[i]) || !isFinite(ys[i])) {
            return StatsDistError.InvalidParameter;
        }
        sum_x += xs[i];
        sum_y += ys[i];
    }
    const mx: f64 = sum_x / nf;
    const my: f64 = sum_y / nf;

    let sxx: f64 = 0.0;
    let syy: f64 = 0.0;
    let sxy: f64 = 0.0;
    let j: usize = 0;
    while (j < n) : (j += 1) {
        const dx: f64 = xs[j] - mx;
        const dy: f64 = ys[j] - my;
        sxx += dx * dx;
        syy += dy * dy;
        sxy += dx * dy;
    }

    if (sxx <= 0.0) {
        return StatsDistError.ZeroVariance;
    }

    const slope: f64 = sxy / sxx;
    const intercept: f64 = my - slope * mx;

    let sse: f64 = 0.0;
    let k: usize = 0;
    while (k < n) : (k += 1) {
        const resid: f64 = ys[k] - (intercept + slope * xs[k]);
        sse += resid * resid;
    }

    const rmse: f64 = std.math.sqrt(sse / nf);
    const r_squared: f64 = if (syy > 0.0) clampF64(1.0 - sse / syy, 0.0, 1.0) else 1.0;

    const df: f64 = nf - 2.0;
    const mse: f64 = sse / df;
    const std_err_slope: f64 = std.math.sqrt(mse / sxx);

    let p_val: f64 = 1.0;
    if (std_err_slope <= 1.0e-15) {
        p_val = if (absF64(slope) > 0.0) 0.0 else 1.0;
    } else {
        const t_stat: f64 = slope / std_err_slope;
        p_val = clampF64(2.0 * (1.0 - studentTCdf(absF64(t_stat), df)?), 0.0, 1.0);
    }

    return SimpleRegressionResult{
        slope: slope,
        intercept: intercept,
        r_squared: r_squared,
        rmse: rmse,
        std_err_slope: std_err_slope,
        p_value_slope: p_val,
    };
}

/// Fitted multivariate linear model `y = intercept + sum_{j=0..n_features-1} coeffs[j] * x[j]`.
pub struct MultiRegressionResult {
    n_features: usize = 0;
    intercept: f64 = 0.0;
    coeffs: [STATS_MAX_FEATURES]f64 = [0.0; STATS_MAX_FEATURES];
    r_squared: f64 = 0.0;
    adj_r_squared: f64 = 0.0;
    rmse: f64 = 0.0;
}

/// Fits multivariate OLS (`ridge_lambda == 0.0`) or L2-regularized Ridge regression
/// `(X^T X + lambda * I_no_intercept) * beta = X^T y` over an arbitrary number of rows `n_samples`
/// and up to `STATS_MAX_FEATURES` (16) features using partial-pivoting Gauss-Jordan elimination.
/// Zero dynamic heap allocation.
pub fn fitRidgeRegression(
    x_rows: []const f64,
    n_samples: usize,
    n_features: usize,
    y: []const f64,
    ridge_lambda: f64,
): MultiRegressionResult ! StatsDistError {
    if (n_features == 0 || n_features > STATS_MAX_FEATURES) {
        return StatsDistError.InvalidParameter;
    }
    if (!isFinite(ridge_lambda) || ridge_lambda < 0.0) {
        return StatsDistError.InvalidParameter;
    }
    if (n_samples == 0 || y.len == 0) {
        return StatsDistError.EmptySample;
    }
    if (y.len != n_samples || x_rows.len < n_samples * n_features) {
        return StatsDistError.LengthMismatch;
    }
    if (n_samples <= n_features + 1) {
        return StatsDistError.InsufficientSamples;
    }

    const dim: usize = n_features + 1; // index 0 = intercept, 1..n_features = features
    let mat: [MAX_AUG_DIM][MAX_AUG_DIM]f64 = [[0.0; MAX_AUG_DIM]; MAX_AUG_DIM];
    let rhs: [MAX_AUG_DIM]f64 = [0.0; MAX_AUG_DIM];

    // 1. Accumulate normal equations A = X^T X and b = X^T y in one streaming pass over N samples.
    let sum_y: f64 = 0.0;
    let r: usize = 0;
    while (r < n_samples) : (r += 1) {
        const y_val: f64 = y[r];
        if (!isFinite(y_val)) {
            return StatsDistError.InvalidParameter;
        }
        sum_y += y_val;

        const row_off: usize = r * n_features;
        let row_vec: [MAX_AUG_DIM]f64 = [0.0; MAX_AUG_DIM];
        row_vec[0] = 1.0;
        let f: usize = 0;
        while (f < n_features) : (f += 1) {
            const xv: f64 = x_rows[row_off + f];
            if (!isFinite(xv)) {
                return StatsDistError.InvalidParameter;
            }
            row_vec[f + 1] = xv;
        }

        let i: usize = 0;
        while (i < dim) : (i += 1) {
            const ri: f64 = row_vec[i];
            rhs[i] += ri * y_val;
            let j: usize = i;
            while (j < dim) : (j += 1) {
                mat[i][j] += ri * row_vec[j];
            }
        }
    }

    // Symmetrize lower triangle and add L2 ridge penalty to non-intercept diagonal entries.
    let i_sym: usize = 0;
    while (i_sym < dim) : (i_sym += 1) {
        let j_sym: usize = 0;
        while (j_sym < i_sym) : (j_sym += 1) {
            mat[i_sym][j_sym] = mat[j_sym][i_sym];
        }
        if (i_sym >= 1) {
            mat[i_sym][i_sym] += ridge_lambda;
        }
    }

    // 2. Partial-pivoting Gauss-Jordan elimination on dim x dim normal matrix.
    let col: usize = 0;
    while (col < dim) : (col += 1) {
        let pivot_row: usize = col;
        let max_abs: f64 = absF64(mat[col][col]);
        let cand: usize = col + 1;
        while (cand < dim) : (cand += 1) {
            const v: f64 = absF64(mat[cand][col]);
            if (v > max_abs) {
                max_abs = v;
                pivot_row = cand;
            }
        }

        if (max_abs <= 1.0e-12) {
            return StatsDistError.SingularMatrix;
        }

        if (pivot_row != col) {
            let k: usize = col;
            while (k < dim) : (k += 1) {
                const tmp: f64 = mat[col][k];
                mat[col][k] = mat[pivot_row][k];
                mat[pivot_row][k] = tmp;
            }
            const tmp_r: f64 = rhs[col];
            rhs[col] = rhs[pivot_row];
            rhs[pivot_row] = tmp_r;
        }

        const inv_piv: f64 = 1.0 / mat[col][col];
        let k2: usize = col;
        while (k2 < dim) : (k2 += 1) {
            mat[col][k2] *= inv_piv;
        }
        rhs[col] *= inv_piv;

        let rr: usize = 0;
        while (rr < dim) : (rr += 1) {
            if (rr == col) continue;
            const factor: f64 = mat[rr][col];
            if (factor == 0.0) continue;
            let k3: usize = col;
            while (k3 < dim) : (k3 += 1) {
                mat[rr][k3] -= factor * mat[col][k3];
            }
            rhs[rr] -= factor * rhs[col];
        }
    }

    let res = MultiRegressionResult{
        n_features: n_features,
        intercept: rhs[0],
    };
    let f2: usize = 0;
    while (f2 < n_features) : (f2 += 1) {
        res.coeffs[f2] = rhs[f2 + 1];
    }

    // 3. Evaluate R^2, adjusted R^2, and RMSE across all N samples.
    const nf: f64 = @as(f64, @floatFromInt(n_samples));
    const pf: f64 = @as(f64, @floatFromInt(n_features));
    const mean_y: f64 = sum_y / nf;

    let sse: f64 = 0.0;
    let sst: f64 = 0.0;
    let s: usize = 0;
    while (s < n_samples) : (s += 1) {
        const row_off: usize = s * n_features;
        let pred: f64 = res.intercept;
        let f3: usize = 0;
        while (f3 < n_features) : (f3 += 1) {
            pred += res.coeffs[f3] * x_rows[row_off + f3];
        }
        const resid: f64 = y[s] - pred;
        const dev: f64 = y[s] - mean_y;
        sse += resid * resid;
        sst += dev * dev;
    }

    res.rmse = std.math.sqrt(sse / nf);
    if (sst > 0.0) {
        res.r_squared = 1.0 - sse / sst;
        res.adj_r_squared = 1.0 - (1.0 - res.r_squared) * ((nf - 1.0) / (nf - pf - 1.0));
    } else {
        res.r_squared = 1.0;
        res.adj_r_squared = 1.0;
    }

    return res;
}

/// Evaluates `res.intercept + dot(res.coeffs[0..n_features], x_row[0..n_features])`.
pub fn predictLinear(
    res: MultiRegressionResult,
    x_row: []const f64,
): f64 ! StatsDistError {
    if (res.n_features == 0 || res.n_features > STATS_MAX_FEATURES) {
        return StatsDistError.InvalidParameter;
    }
    if (x_row.len < res.n_features) {
        return StatsDistError.LengthMismatch;
    }
    let acc: f64 = res.intercept;
    let i: usize = 0;
    while (i < res.n_features) : (i += 1) {
        acc += res.coeffs[i] * x_row[i];
    }
    return acc;
}
