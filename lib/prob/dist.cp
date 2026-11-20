// Source: cp/lib/prob/dist.cp (Probability Distributions, Deterministic RNG & Hypothesis Testing)
// Pure CP zero-C-dependency probability distributions (Normal, Exponential, Binomial, Poisson),
// SplitMix64 + xoshiro256** deterministic RNG, and two-sample Z-test hypothesis testing engine.

const std = @import("std");

/// Error set returned by probability distribution, sampling, and hypothesis testing operations.
pub err ProbError {
    InvalidParameter,
    InvalidProbability,
    EmptyWeights,
    SampleSizeTooSmall,
    OutOfMemory,
}

fn isFiniteF64(x: f64): bool {
    return std.math.isFinite(x);
}

fn rotl64(x: u64, k: u6): u64 {
    return std.math.rotl(u64, x, k);
}

const SM_GAMMA: u64 = ((0x9e3779b9 as u64) << 32) | 0x7f4a7c15;
const SM_MUL1: u64 = ((0xbf58476d as u64) << 32) | 0x1ce4e5b9;
const SM_MUL2: u64 = ((0x94d049bb as u64) << 32) | 0x133111eb;

fn splitmix64(x: u64): u64 {
    let z: u64 = x;
    z = (z ^ (z >> 30)) *% SM_MUL1;
    z = (z ^ (z >> 27)) *% SM_MUL2;
    return z ^ (z >> 31);
}

/// High-precision pure-CP error function erf(x) (error < 1e-14).
fn erfApprox(x: f64): f64 {
    if (x == 0.0) return 0.0;
    const sign: f64 = if (x < 0.0) -1.0 else 1.0;
    const ax: f64 = @abs(x);

    if (ax >= 6.0) return sign;

    // For |x| < 3.5, power series converges rapidly to machine precision
    if (ax < 3.5) {
        const two_over_sqrt_pi: f64 = 1.12837916709551257390;
        const x2: f64 = ax * ax;
        let term: f64 = ax;
        let sum: f64 = ax;
        let n: i32 = 1;
        while (n < 60) : (n += 1) {
            const nf = n as f64;
            term *= -x2 / nf;
            const add_val = term / (2.0 * nf + 1.0);
            sum += add_val;
            if (@abs(add_val) <= @abs(sum) * 1.0e-16) break;
        }
        let res = sign * two_over_sqrt_pi * sum;
        if (res > 1.0) res = 1.0;
        if (res < -1.0) res = -1.0;
        return res;
    }

    // For 3.5 <= |x| < 6.0, use continued fraction for erfc(ax)
    const inv_sqrt_pi: f64 = 0.56418958354775628695;
    const tiny: f64 = 1.0e-300;
    let b: f64 = ax + 0.5;
    let c: f64 = 1.0 / tiny;
    let d: f64 = 1.0 / b;
    let h: f64 = d;
    let i: i32 = 1;
    while (i <= 40) : (i += 1) {
        const fi = i as f64;
        const a = -fi * (fi - 0.5);
        b += 2.0;
        d = a * d + b;
        if (@abs(d) < tiny) d = tiny;
        c = b + a / c;
        if (@abs(c) < tiny) c = tiny;
        d = 1.0 / d;
        const del = d * c;
        h *= del;
        if (@abs(del - 1.0) < 1.0e-15) break;
    }
    const erfc_val = @exp(-ax * ax) * inv_sqrt_pi * h;
    let res = sign * (1.0 - erfc_val);
    if (res > 1.0) res = 1.0;
    if (res < -1.0) res = -1.0;
    return res;
}

fn normCdfStd(z: f64): f64 {
    return 0.5 * (1.0 + erfApprox(z / 1.41421356237309504880));
}

/// Acklam's rational approximation for the inverse standard normal CDF (error < 1.15e-9).
fn normInvCdfStd(p: f64): f64 {
    if (p == 0.0) return 0.0 - std.math.inf(f64);
    if (p == 1.0) return std.math.inf(f64);

    const a1: f64 = -3.969683028665376e+01;
    const a2: f64 =  2.209460984245205e+02;
    const a3: f64 = -2.759285104469687e+02;
    const a4: f64 =  1.383577518672690e+02;
    const a5: f64 = -3.066479806614716e+01;
    const a6: f64 =  2.506628277459239e+00;

    const b1: f64 = -5.447609879822406e+01;
    const b2: f64 =  1.615858368580409e+02;
    const b3: f64 = -1.556989798598866e+02;
    const b4: f64 =  6.680131188771972e+01;
    const b5: f64 = -1.328068155288572e+01;

    const c1: f64 = -7.784894002430293e-03;
    const c2: f64 = -3.223964580411365e-01;
    const c3: f64 = -2.400758277161838e+00;
    const c4: f64 = -2.549732539343734e+00;
    const c5: f64 =  4.374664141464968e+00;
    const c6: f64 =  2.938163982698783e+00;

    const d1: f64 =  7.784695709041462e-03;
    const d2: f64 =  3.224671290700398e-01;
    const d3: f64 =  2.445134137142996e+00;
    const d4: f64 =  3.754408661907416e+00;

    const p_low: f64 = 0.02425;
    const p_high: f64 = 1.0 - p_low;

    if (p < p_low) {
        const q = @sqrt(-2.0 * @log(p));
        return (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
               ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
    }
    if (p <= p_high) {
        const q = p - 0.5;
        const r = q * q;
        return (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q /
               (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1.0);
    }
    const q = @sqrt(-2.0 * @log(1.0 - p));
    return -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) /
            ((((d1 * q + d2) * q + d3) * q + d4) * q + 1.0);
}

/// Lanczos approximation for ln(Gamma(x)) for x > 0.
fn lgammaApprox(x: f64): f64 {
    const coeffs: [9]f64 = [
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
    const z = x - 1.0;
    let sum: f64 = coeffs[0];
    let i: usize = 1;
    while (i < 9) : (i += 1) {
        sum += coeffs[i] / (z + (i as f64));
    }
    const t = z + 7.5;
    const half_ln_2pi: f64 = 0.91893853320467274178;
    return half_ln_2pi + (z + 0.5) * @log(t) - t + @log(sum);
}

/// Deterministic high-quality pseudo-random number generator (SplitMix64 seeding + xoshiro256**).
pub struct Rng {
    s0: u64 = 0;
    s1: u64 = 0;
    s2: u64 = 0;
    s3: u64 = 0;
    has_spare: bool = false;
    spare: f64 = 0.0;

    /// Initializes an `Rng` deterministically from a 64-bit `seed`.
    static fn init(seed: u64): Rng {
        let sm: u64 = seed +% SM_GAMMA;
        let s0 = splitmix64(sm);
        sm = sm +% SM_GAMMA;
        let s1 = splitmix64(sm);
        sm = sm +% SM_GAMMA;
        let s2 = splitmix64(sm);
        sm = sm +% SM_GAMMA;
        let s3 = splitmix64(sm);
        if (s0 == 0 && s1 == 0 && s2 == 0 && s3 == 0) {
            s0 = SM_GAMMA;
        }
        return Rng{
            s0: s0,
            s1: s1,
            s2: s2,
            s3: s3,
            has_spare: false,
            spare: 0.0,
        };
    }

    /// Generates the next 64-bit unsigned integer via xoshiro256**.
    fn nextU64(*_): u64 {
        const result: u64 = rotl64(_.s1 *% 5, 7) *% 9;
        const t: u64 = _.s1 << 17;

        _.s2 ^= _.s0;
        _.s3 ^= _.s1;
        _.s1 ^= _.s2;
        _.s0 ^= _.s3;
        _.s2 ^= t;
        _.s3 = rotl64(_.s3, 45);

        return result;
    }

    /// Generates a uniformly distributed `f64` in `[0, 1)`.
    fn nextF64(*_): f64 {
        const bits = _.nextU64() >> 11;
        return (bits as f64) * (1.0 / 9007199254740992.0);
    }

    /// Samples a continuous uniform `f64` in `[low, high)`.
    fn uniform(*_, low: f64, high: f64): f64 ! ProbError {
        if (!isFiniteF64(low) || !isFiniteF64(high) || low > high) {
            return ProbError.InvalidParameter;
        }
        return low + (high - low) * _.nextF64();
    }

    /// Samples a Bernoulli trial returning `true` with probability `p`.
    fn bernoulli(*_, p: f64): bool ! ProbError {
        if (std.math.isNan(p) || p < 0.0 || p > 1.0) {
            return ProbError.InvalidProbability;
        }
        return _.nextF64() < p;
    }

    /// Samples from `Normal(mean, stddev^2)` using the Box-Muller transform.
    fn normal(*_, mean: f64, stddev: f64): f64 ! ProbError {
        if (!isFiniteF64(mean) || !isFiniteF64(stddev) || stddev <= 0.0) {
            return ProbError.InvalidParameter;
        }
        if (_.has_spare) {
            _.has_spare = false;
            return mean + stddev * _.spare;
        }

        let u_a = _.nextF64();
        if (u_a < 1.0e-300) u_a = 1.0e-300;
        const u_b = _.nextF64();

        const r = @sqrt(-2.0 * @log(u_a));
        const theta = 2.0 * std.math.pi * u_b;
        const z0 = r * @cos(theta);
        const z1 = r * @sin(theta);

        _.spare = z1;
        _.has_spare = true;
        return mean + stddev * z0;
    }

    /// Samples from `Exponential(lambda)` using inverse transform sampling.
    fn exponential(*_, lambda: f64): f64 ! ProbError {
        if (!isFiniteF64(lambda) || lambda <= 0.0) {
            return ProbError.InvalidParameter;
        }
        const u = 1.0 - _.nextF64();
        return -@log(u) / lambda;
    }

    /// Samples from `Poisson(lambda)` using Knuth (lambda < 30) and PTRS (lambda >= 30).
    fn poisson(*_, lambda: f64): u64 ! ProbError {
        if (!isFiniteF64(lambda) || lambda < 0.0) {
            return ProbError.InvalidParameter;
        }
        if (lambda == 0.0) return 0;

        if (lambda < 30.0) {
            const limit = @exp(-lambda);
            let k: u64 = 0;
            let p: f64 = 1.0;
            while (true) {
                k += 1;
                p *= _.nextF64();
                if (p <= limit) break;
            }
            return k - 1;
        }

        // PTRS algorithm (Hormann 1993) for large lambda
        const slam = @sqrt(lambda);
        const loglam = @log(lambda);
        const b = 0.931 + 2.53 * slam;
        const a = -0.059 + 0.02483 * b;
        const inv_alpha = 1.1239 + 1.1328 / (b - 3.4);
        const vr = 0.9277 - 3.6224 / (b - 2.0);

        while (true) {
            const u = _.nextF64() - 0.5;
            const v = _.nextF64();
            const us = 0.5 - @abs(u);
            const k_f: f64 = @floor((2.0 * a / us + b) * u + lambda + 0.43);
            if (k_f < 0.0) continue;
            const k: u64 = @intFromFloat(k_f);

            if (us >= 0.07 && v <= vr) return k;
            if (us < 0.013 && v > us) continue;

            const lhs = @log(v * inv_alpha / (a / (us * us) + b));
            const rhs = -lambda + k_f * loglam - lgammaApprox(k_f + 1.0);
            if (lhs <= rhs) return k;
        }
    }

    /// Selects an index from `0..weights.len` proportional to non-negative `weights`.
    fn weightedChoice(*_, weights: []const f64): usize ! ProbError {
        if (weights.len == 0) return ProbError.EmptyWeights;

        let total: f64 = 0.0;
        let last_positive: usize = 0;
        let i: usize = 0;
        while (i < weights.len) : (i += 1) {
            const w = weights[i];
            if (!isFiniteF64(w) || w < 0.0) return ProbError.InvalidParameter;
            if (w > 0.0) {
                total += w;
                last_positive = i;
            }
        }
        if (total <= 0.0) return ProbError.InvalidParameter;

        const target = _.nextF64() * total;
        let accum: f64 = 0.0;
        i = 0;
        while (i < weights.len) : (i += 1) {
            accum += weights[i];
            if (target < accum) return i;
        }
        return last_positive;
    }

    /// Performs an in-place Fisher-Yates shuffle on `slice`.
    fn shuffle(*_, slice: auto) {
        if (slice.len <= 1) return;
        let i: usize = slice.len - 1;
        while (i > 0) : (i -= 1) {
            const j = (_.nextU64() % ((i + 1) as u64)) as usize;
            const tmp = slice[i];
            slice[i] = slice[j];
            slice[j] = tmp;
        }
    }
}

/// Normal (Gaussian) probability distribution `N(mean, stddev^2)`.
pub struct NormalDist {
    mean: f64 = 0.0;
    stddev: f64 = 1.0;

    /// Constructs a `NormalDist` with given `mean` and positive `stddev`.
    static fn init(mean: f64, stddev: f64): NormalDist ! ProbError {
        if (!isFiniteF64(mean) || !isFiniteF64(stddev) || stddev <= 0.0) {
            return ProbError.InvalidParameter;
        }
        return NormalDist{ mean: mean, stddev: stddev };
    }

    /// Returns the standard normal distribution `N(0, 1)`.
    static fn standard(): NormalDist {
        return NormalDist{ mean: 0.0, stddev: 1.0 };
    }

    /// Probability density function at `x`.
    fn pdf(x: f64): f64 {
        const z = (x - _.mean) / _.stddev;
        const inv_sqrt_2pi: f64 = 0.39894228040143267794;
        return (inv_sqrt_2pi / _.stddev) * @exp(-0.5 * z * z);
    }

    /// Cumulative distribution function `P(X <= x)`.
    fn cdf(x: f64): f64 {
        const z = (x - _.mean) / _.stddev;
        return normCdfStd(z);
    }

    /// Inverse cumulative distribution function (quantile / PPF) at probability `p`.
    fn invCdf(p: f64): f64 ! ProbError {
        if (std.math.isNan(p) || p < 0.0 || p > 1.0) {
            return ProbError.InvalidProbability;
        }
        if (p == 0.0) return 0.0 - std.math.inf(f64);
        if (p == 1.0) return std.math.inf(f64);
        return _.mean + _.stddev * normInvCdfStd(p);
    }
}

/// Exponential probability distribution `Exp(lambda)` with rate `lambda > 0`.
pub struct ExponentialDist {
    lambda: f64 = 1.0;

    /// Constructs an `ExponentialDist` with positive rate `lambda`.
    static fn init(lambda: f64): ExponentialDist ! ProbError {
        if (!isFiniteF64(lambda) || lambda <= 0.0) {
            return ProbError.InvalidParameter;
        }
        return ExponentialDist{ lambda: lambda };
    }

    /// Probability density function at `x`.
    fn pdf(x: f64): f64 {
        if (x < 0.0) return 0.0;
        return _.lambda * @exp(0.0 - (_.lambda * x));
    }

    /// Cumulative distribution function `P(X <= x)`.
    fn cdf(x: f64): f64 {
        if (x <= 0.0) return 0.0;
        return 1.0 - @exp(0.0 - (_.lambda * x));
    }

    /// Inverse cumulative distribution function (quantile / PPF) at probability `p`.
    fn invCdf(p: f64): f64 ! ProbError {
        if (std.math.isNan(p) || p < 0.0 || p > 1.0) {
            return ProbError.InvalidProbability;
        }
        if (p == 0.0) return 0.0;
        if (p == 1.0) return std.math.inf(f64);
        return -@log(1.0 - p) / _.lambda;
    }
}

/// Binomial probability distribution `Binomial(n, p)` using log-gamma to avoid overflow.
pub struct BinomialDist {
    n: u64 = 1;
    p: f64 = 0.5;

    /// Constructs a `BinomialDist` with `n` trials and success probability `p` in `[0, 1]`.
    static fn init(n: u64, p: f64): BinomialDist ! ProbError {
        if (std.math.isNan(p) || p < 0.0 || p > 1.0) {
            return ProbError.InvalidProbability;
        }
        return BinomialDist{ n: n, p: p };
    }

    /// Probability mass function `P(X = k)`.
    fn pmf(k: u64): f64 {
        if (k > _.n) return 0.0;
        if (_.p == 0.0) return if (k == 0) 1.0 else 0.0;
        if (_.p == 1.0) return if (k == _.n) 1.0 else 0.0;

        const nf = _.n as f64;
        const kf = k as f64;
        const log_comb = lgammaApprox(nf + 1.0) - lgammaApprox(kf + 1.0) - lgammaApprox(nf - kf + 1.0);
        const log_prob = log_comb + kf * @log(_.p) + (nf - kf) * @log(1.0 - _.p);
        return @exp(log_prob);
    }

    /// Cumulative distribution function `P(X <= k)`.
    fn cdf(k: u64): f64 {
        if (k >= _.n) return 1.0;
        if (k <= _.n / 2) {
            let sum: f64 = 0.0;
            let i: u64 = 0;
            while (i <= k) : (i += 1) {
                sum += _.pmf(i);
            }
            return if (sum > 1.0) 1.0 else sum;
        }
        let tail: f64 = 0.0;
        let i: u64 = k + 1;
        while (i <= _.n) : (i += 1) {
            tail += _.pmf(i);
        }
        const res = 1.0 - tail;
        return if (res < 0.0) 0.0 else res;
    }
}

/// Poisson probability distribution `Poisson(lambda)` using log-gamma to avoid overflow.
pub struct PoissonDist {
    lambda: f64 = 1.0;

    /// Constructs a `PoissonDist` with positive rate `lambda`.
    static fn init(lambda: f64): PoissonDist ! ProbError {
        if (!isFiniteF64(lambda) || lambda <= 0.0) {
            return ProbError.InvalidParameter;
        }
        return PoissonDist{ lambda: lambda };
    }

    /// Probability mass function `P(X = k)`.
    fn pmf(k: u64): f64 {
        const kf = k as f64;
        const log_prob = kf * @log(_.lambda) - _.lambda - lgammaApprox(kf + 1.0);
        return @exp(log_prob);
    }

    /// Cumulative distribution function `P(X <= k)`.
    fn cdf(k: u64): f64 {
        let sum: f64 = 0.0;
        let i: u64 = 0;
        while (i <= k) : (i += 1) {
            sum += _.pmf(i);
        }
        return if (sum > 1.0) 1.0 else sum;
    }
}

/// Result summary for A/B hypothesis testing and confidence intervals.
pub struct AbTestResult {
    z_score: f64 = 0.0;
    p_value: f64 = 1.0;
    ci_low: f64 = 0.0;
    ci_high: f64 = 0.0;
    relative_lift: f64 = 0.0;
    significant: bool = false;
}

/// Statistical hypothesis testing engine for A/B experiments.
pub static struct HypothesisTest {
    /// Two-proportion two-tailed Z-test comparing conversion rate `conv_b / total_b` against `conv_a / total_a`.
    static fn twoProportionZTest(
        conv_a: u64,
        total_a: u64,
        conv_b: u64,
        total_b: u64,
        alpha: f64,
    ): AbTestResult ! ProbError {
        if (total_a == 0 || total_b == 0) return ProbError.SampleSizeTooSmall;
        if (conv_a > total_a || conv_b > total_b) return ProbError.InvalidParameter;
        if (!isFiniteF64(alpha) || alpha <= 0.0 || alpha >= 1.0) return ProbError.InvalidProbability;

        const na = total_a as f64;
        const nb = total_b as f64;
        const pa = (conv_a as f64) / na;
        const pb = (conv_b as f64) / nb;
        const diff = pb - pa;

        const p_pool = ((conv_a + conv_b) as f64) / (na + nb);
        const se_pool = @sqrt(p_pool * (1.0 - p_pool) * (1.0 / na + 1.0 / nb));
        const z_score = if (se_pool == 0.0) 0.0 else diff / se_pool;
        const p_value = 2.0 * (1.0 - normCdfStd(@abs(z_score)));

        const se_unpooled = @sqrt((pa * (1.0 - pa)) / na + (pb * (1.0 - pb)) / nb);
        const z_crit = normInvCdfStd(1.0 - alpha * 0.5);
        const ci_low = diff - z_crit * se_unpooled;
        const ci_high = diff + z_crit * se_unpooled;
        const relative_lift = if (pa == 0.0) 0.0 else diff / pa;

        return AbTestResult{
            z_score: z_score,
            p_value: p_value,
            ci_low: ci_low,
            ci_high: ci_high,
            relative_lift: relative_lift,
            significant: p_value < alpha,
        };
    }

    /// Two-sample two-tailed Z-test comparing sample mean `mean_b` against `mean_a`.
    static fn twoSampleZTest(
        mean_a: f64,
        std_a: f64,
        n_a: u64,
        mean_b: f64,
        std_b: f64,
        n_b: u64,
        alpha: f64,
    ): AbTestResult ! ProbError {
        if (n_a == 0 || n_b == 0) return ProbError.SampleSizeTooSmall;
        if (!isFiniteF64(mean_a) || !isFiniteF64(mean_b) || !isFiniteF64(std_a) || !isFiniteF64(std_b) || std_a < 0.0 || std_b < 0.0) {
            return ProbError.InvalidParameter;
        }
        if (!isFiniteF64(alpha) || alpha <= 0.0 || alpha >= 1.0) return ProbError.InvalidProbability;

        const na = n_a as f64;
        const nb = n_b as f64;
        const diff = mean_b - mean_a;
        const se = @sqrt((std_a * std_a) / na + (std_b * std_b) / nb);
        const z_score = if (se == 0.0) 0.0 else diff / se;
        const p_value = 2.0 * (1.0 - normCdfStd(@abs(z_score)));

        const z_crit = normInvCdfStd(1.0 - alpha * 0.5);
        const ci_low = diff - z_crit * se;
        const ci_high = diff + z_crit * se;
        const relative_lift = if (mean_a == 0.0) 0.0 else diff / mean_a;

        return AbTestResult{
            z_score: z_score,
            p_value: p_value,
            ci_low: ci_low,
            ci_high: ci_high,
            relative_lift: relative_lift,
            significant: p_value < alpha,
        };
    }
}
