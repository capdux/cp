// @cap/rand-distr — Zero-heap probability distribution samplers built on @cap/xoshiro256.
// Clean-room MIT implementation modeled on crates.io `rand_distr` v0.4.

import { Xoshiro256, nextF64 } from "../../rand_xoshiro/src/index.cp";

pub const WEIGHTED_MAX_ITEMS: usize = 16;

fn rdstAbs(x: f64): f64 {
    return if (x < 0.0) { -x } else { x };
}

fn rdstSqrt(x: f64): f64 {
    if (x <= 0.0) { return 0.0; }
    let guess: f64 = if (x > 1.0) { x * 0.5 } else { 1.0 };
    let i: u32 = 0;
    while (i < 32) {
        let next: f64 = 0.5 * (guess + x / guess);
        if (rdstAbs(next - guess) <= 1.0e-15 * next) {
            return next;
        }
        guess = next;
        i = i + 1;
    }
    return guess;
}

fn rdstLn(x: f64): f64 {
    if (x <= 0.0) { return -1.0e308; }
    let m: f64 = x;
    let k: i32 = 0;
    let sqrt2: f64 = 1.4142135623730951;
    let inv_sqrt2: f64 = 0.7071067811865476;
    while (m > sqrt2) {
        m = m * 0.5;
        k = k + 1;
    }
    while (m < inv_sqrt2) {
        m = m * 2.0;
        k = k - 1;
    }
    let t: f64 = (m - 1.0) / (m + 1.0);
    let t2: f64 = t * t;
    let term: f64 = t;
    let sum: f64 = t;
    let denom: f64 = 3.0;
    let i: u32 = 0;
    while (i < 12) {
        term = term * t2;
        sum = sum + term / denom;
        denom = denom + 2.0;
        i = i + 1;
    }
    let ln2: f64 = 0.6931471805599453;
    return 2.0 * sum + (k as f64) * ln2;
}

fn rdstCos(angle: f64): f64 {
    let pi: f64 = 3.141592653589793;
    let two_pi: f64 = 6.283185307179586;
    let half_pi: f64 = 1.5707963267948966;

    let x: f64 = rdstAbs(angle);
    if (x >= two_pi) {
        let turns: u64 = (x / two_pi) as u64;
        x = x - (turns as f64) * two_pi;
    }
    if (x > pi) {
        x = two_pi - x;
    }
    let sign: f64 = 1.0;
    if (x > half_pi) {
        x = pi - x;
        sign = -1.0;
    }

    let x2: f64 = x * x;
    let term: f64 = 1.0;
    let sum: f64 = 1.0;
    let i: u32 = 1;
    while (i <= 10) {
        let d1: f64 = (2 * i - 1) as f64;
        let d2: f64 = (2 * i) as f64;
        term = -term * x2 / (d1 * d2);
        sum = sum + term;
        i = i + 1;
    }
    return sign * sum;
}

fn rdstOpen01(rng: *Xoshiro256): f64 {
    let u: f64 = nextF64(rng);
    if (u <= 0.0) { return 1.0e-16; }
    return u;
}

/// Continuous uniform distribution over `[low, high)`.
pub struct UniformRange {
    pub low: f64,
    pub high: f64,

    /// Constructs a `UniformRange` if `low < high`, otherwise returns `null`.
    pub static fn new(low: f64, high: f64): UniformRange? {
        if (low >= high) { return null; }
        return UniformRange { low: low, high: high };
    }

    /// Samples a value uniformly from `[low, high)`.
    pub fn sample(rng: *Xoshiro256): f64 {
        return _.low + nextF64(rng) * (_.high - _.low);
    }
}

/// Normal (Gaussian) distribution `N(mean, std_dev^2)` sampled via the Box-Muller transform.
pub struct NormalDistr {
    pub mean: f64,
    pub std_dev: f64,

    /// Constructs a `NormalDistr` if `std_dev >= 0.0`, otherwise returns `null`.
    pub static fn new(mean: f64, std_dev: f64): NormalDistr? {
        if (std_dev < 0.0) { return null; }
        return NormalDistr { mean: mean, std_dev: std_dev };
    }

    /// Samples from `N(mean, std_dev^2)` using Box-Muller transform.
    pub fn sample(rng: *Xoshiro256): f64 {
        if (_.std_dev == 0.0) { return _.mean; }
        let r1: f64 = rdstOpen01(rng);
        let r2: f64 = nextF64(rng);
        let radius: f64 = rdstSqrt(-2.0 * rdstLn(r1));
        let theta: f64 = 6.283185307179586 * r2;
        let z0: f64 = radius * rdstCos(theta);
        return _.mean + _.std_dev * z0;
    }
}

/// Exponential distribution `Exp(lambda)` sampled via inverse transform `-ln(1 - u) / lambda`.
pub struct ExpDistr {
    pub lambda: f64,

    /// Constructs an `ExpDistr` if `lambda > 0.0`, otherwise returns `null`.
    pub static fn new(lambda: f64): ExpDistr? {
        if (lambda <= 0.0) { return null; }
        return ExpDistr { lambda: lambda };
    }

    /// Samples a non-negative value from `Exp(lambda)`.
    pub fn sample(rng: *Xoshiro256): f64 {
        let u: f64 = nextF64(rng);
        let one_minus_u: f64 = 1.0 - u;
        if (one_minus_u <= 0.0) { return 0.0; }
        return -rdstLn(one_minus_u) / _.lambda;
    }
}

/// Bernoulli distribution returning `true` with probability `p` in `[0.0, 1.0]`.
pub struct BernoulliDistr {
    pub p: f64,

    /// Constructs a `BernoulliDistr` if `0.0 <= p <= 1.0`, otherwise returns `null`.
    pub static fn new(p: f64): BernoulliDistr? {
        if (p < 0.0 || p > 1.0) { return null; }
        return BernoulliDistr { p: p };
    }

    /// Samples a boolean trial from `Bernoulli(p)`.
    pub fn sample(rng: *Xoshiro256): bool {
        if (_.p <= 0.0) { return false; }
        if (_.p >= 1.0) { return true; }
        return nextF64(rng) < _.p;
    }
}

/// Zero-heap discrete weighted index sampler holding up to 16 cumulative weights.
pub struct WeightedIndex {
    pub cumulative: [16]f64,
    pub count: usize,
    pub total: f64,

    /// Builds a `WeightedIndex` from up to 16 non-negative weights with positive total sum.
    pub static fn fromWeights(weights: []f64): WeightedIndex? {
        if (weights.len == 0 || weights.len > WEIGHTED_MAX_ITEMS) {
            return null;
        }
        let cum: [16]f64 = [
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
            0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0
        ];
        let acc: f64 = 0.0;
        let i: usize = 0;
        while (i < weights.len) {
            let w: f64 = weights[i];
            if (w < 0.0) { return null; }
            acc = acc + w;
            cum[i] = acc;
            i = i + 1;
        }
        if (acc <= 0.0) { return null; }
        return WeightedIndex {
            cumulative: cum,
            count: weights.len,
            total: acc,
        };
    }

    /// Samples an index in `0..count` proportional to its weight.
    pub fn sample(rng: *Xoshiro256): usize {
        let target: f64 = nextF64(rng) * _.total;
        let i: usize = 0;
        while (i + 1 < _.count) {
            if (target < _.cumulative[i]) {
                return i;
            }
            i = i + 1;
        }
        return _.count - 1;
    }
}
