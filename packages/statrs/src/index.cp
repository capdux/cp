// @cap/statrs — Zero-heap descriptive statistics, probability distributions & linear regression.
// Clean-room MIT implementation modeled on crates.io `statrs` v0.17.

/// Result of ordinary least-squares simple linear regression `y = slope * x + intercept`.
pub struct LinearFit {
    pub slope: f64,
    pub intercept: f64,
    pub r_squared: f64,
    pub valid: bool,
}

fn statAbs(x: f64): f64 {
    return if (x < 0.0) { -x } else { x };
}

fn statSqrt(x: f64): f64 {
    if (x <= 0.0) { return 0.0; }
    let guess: f64 = if (x > 1.0) { x * 0.5 } else { 1.0 };
    let i: u32 = 0;
    while (i < 32) {
        let next: f64 = 0.5 * (guess + x / guess);
        if (statAbs(next - guess) <= 1.0e-15 * next) {
            return next;
        }
        guess = next;
        i = i + 1;
    }
    return guess;
}

fn statExp(x: f64): f64 {
    if (x > 709.0) { return 1.0e308; }
    if (x < -745.0) { return 0.0; }

    let ln2: f64 = 0.6931471805599453;
    let inv_ln2: f64 = 1.4426950408889634;
    let kf: f64 = x * inv_ln2;
    let k: i32 = if (kf >= 0.0) { (kf + 0.5) as i32 } else { (kf - 0.5) as i32 };
    let r: f64 = x - (k as f64) * ln2;

    // 16-term Taylor series for exp(r) where |r| <= ln(2)/2
    let sum: f64 = 1.0;
    let term: f64 = 1.0;
    let i: u32 = 1;
    while (i <= 16) {
        term = term * r / (i as f64);
        sum = sum + term;
        i = i + 1;
    }

    if (k > 0) {
        let steps: i32 = 0;
        while (steps < k) {
            sum = sum * 2.0;
            steps = steps + 1;
        }
    } else if (k < 0) {
        let steps: i32 = 0;
        let neg_k: i32 = -k;
        while (steps < neg_k) {
            sum = sum * 0.5;
            steps = steps + 1;
        }
    }
    return sum;
}

fn statGcdU64(a: u64, b: u64): u64 {
    let x: u64 = a;
    let y: u64 = b;
    while (y != 0) {
        let t: u64 = x % y;
        x = y;
        y = t;
    }
    return x;
}

/// Arithmetic mean of `xs`, or `null` if empty.
pub fn mean(xs: []f64): f64? {
    if (xs.len == 0) { return null; }
    let sum: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        sum = sum + xs[i];
        i = i + 1;
    }
    return sum / (xs.len as f64);
}

/// Population variance (dividing by `n`), or `null` if empty.
pub fn popVariance(xs: []f64): f64? {
    let maybe_m = mean(xs);
    if (maybe_m == null) { return null; }
    let m: f64 = maybe_m ?? 0.0;
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        let d: f64 = xs[i] - m;
        acc = acc + d * d;
        i = i + 1;
    }
    return acc / (xs.len as f64);
}

/// Sample variance (Bessel-corrected, dividing by `n - 1`), or `null` if `xs.len < 2`.
pub fn variance(xs: []f64): f64? {
    if (xs.len < 2) { return null; }
    let m: f64 = mean(xs) ?? 0.0;
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        let d: f64 = xs[i] - m;
        acc = acc + d * d;
        i = i + 1;
    }
    return acc / ((xs.len - 1) as f64);
}

/// Sample standard deviation (`sqrt(variance(xs))`), or `null` if `xs.len < 2`.
pub fn stdDev(xs: []f64): f64? {
    let maybe_v = variance(xs);
    if (maybe_v == null) { return null; }
    let v: f64 = maybe_v ?? 0.0;
    return statSqrt(v);
}

/// Sample covariance between `xs` and `ys`, or `null` if lengths differ or `< 2`.
pub fn covariance(xs: []f64, ys: []f64): f64? {
    if (xs.len != ys.len || xs.len < 2) { return null; }
    let mx: f64 = mean(xs) ?? 0.0;
    let my: f64 = mean(ys) ?? 0.0;
    let acc: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        acc = acc + (xs[i] - mx) * (ys[i] - my);
        i = i + 1;
    }
    return acc / ((xs.len - 1) as f64);
}

/// Pearson correlation coefficient in `[-1.0, 1.0]`, or `null` if lengths differ, `< 2`, or zero variance.
pub fn pearsonCorr(xs: []f64, ys: []f64): f64? {
    if (xs.len != ys.len || xs.len < 2) { return null; }
    let mx: f64 = mean(xs) ?? 0.0;
    let my: f64 = mean(ys) ?? 0.0;
    let sxx: f64 = 0.0;
    let syy: f64 = 0.0;
    let sxy: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        let dx: f64 = xs[i] - mx;
        let dy: f64 = ys[i] - my;
        sxx = sxx + dx * dx;
        syy = syy + dy * dy;
        sxy = sxy + dx * dy;
        i = i + 1;
    }
    if (sxx <= 0.0 || syy <= 0.0) { return null; }
    return sxy / statSqrt(sxx * syy);
}

/// Gauss error function `erf(x)` using Abramowitz & Stegun 7.1.26 (max error `< 1.5e-7`).
pub fn erf(x: f64): f64 {
    let sign: f64 = if (x < 0.0) { -1.0 } else { 1.0 };
    let ax: f64 = statAbs(x);
    let t: f64 = 1.0 / (1.0 + 0.3275911 * ax);
    let a1: f64 = 0.254829592;
    let a2: f64 = -0.284496736;
    let a3: f64 = 1.421413741;
    let a4: f64 = -1.453152027;
    let a5: f64 = 1.061405429;
    let poly: f64 = ((((a5 * t + a4) * t + a3) * t + a2) * t + a1) * t;
    let y: f64 = 1.0 - poly * statExp(-ax * ax);
    return sign * y;
}

/// Normal distribution probability density function `N(mu, sigma^2)` at `x`.
pub fn normalPdf(x: f64, mu: f64, sigma: f64): f64 {
    if (sigma <= 0.0) { return 0.0; }
    let z: f64 = (x - mu) / sigma;
    let sqrt_2pi: f64 = 2.5066282746310002;
    return statExp(-0.5 * z * z) / (sigma * sqrt_2pi);
}

/// Normal distribution cumulative distribution function `N(mu, sigma^2)` at `x`.
pub fn normalCdf(x: f64, mu: f64, sigma: f64): f64 {
    if (sigma <= 0.0) { return 0.0; }
    let sqrt_2: f64 = 1.4142135623730951;
    let z: f64 = (x - mu) / (sigma * sqrt_2);
    return 0.5 * (1.0 + erf(z));
}

/// Exponential distribution PDF `rate * exp(-rate * x)` for `x >= 0`, `rate > 0`.
pub fn exponentialPdf(x: f64, rate: f64): f64 {
    if (rate <= 0.0 || x < 0.0) { return 0.0; }
    return rate * statExp(-rate * x);
}

/// Exponential distribution CDF `1 - exp(-rate * x)` for `x >= 0`, `rate > 0`.
pub fn exponentialCdf(x: f64, rate: f64): f64 {
    if (rate <= 0.0 || x < 0.0) { return 0.0; }
    return 1.0 - statExp(-rate * x);
}

/// Poisson probability mass function `P(X = k) = lambda^k * exp(-lambda) / k!`.
pub fn poissonPmf(k: u32, lambda: f64): f64 {
    if (lambda < 0.0) { return 0.0; }
    if (lambda == 0.0) {
        return if (k == 0) { 1.0 } else { 0.0 };
    }
    let p: f64 = statExp(-lambda);
    let i: u32 = 1;
    while (i <= k) {
        p = p * (lambda / (i as f64));
        i = i + 1;
    }
    return p;
}

/// Exact binomial coefficient `C(n, k)` using GCD-reduced multiplication to avoid overflow.
pub fn binomialCoeff(n: u32, k: u32): u64 {
    if (k > n) { return 0; }
    let kk: u32 = if (k > n - k) { n - k } else { k };
    let res: u64 = 1;
    let i: u32 = 1;
    while (i <= kk) {
        let num: u64 = (n - kk + i) as u64;
        let den: u64 = i as u64;
        let g1: u64 = statGcdU64(res, den);
        let r_red: u64 = res / g1;
        let d_red: u64 = den / g1;
        let n_red: u64 = num / d_red;
        res = r_red * n_red;
        i = i + 1;
    }
    return res;
}

/// Ordinary least-squares simple linear regression `y = slope * x + intercept` with `r_squared`.
pub fn linearRegression(xs: []f64, ys: []f64): LinearFit {
    if (xs.len != ys.len || xs.len < 2) {
        return LinearFit { slope: 0.0, intercept: 0.0, r_squared: 0.0, valid: false };
    }
    let mx: f64 = mean(xs) ?? 0.0;
    let my: f64 = mean(ys) ?? 0.0;
    let sxx: f64 = 0.0;
    let syy: f64 = 0.0;
    let sxy: f64 = 0.0;
    let i: usize = 0;
    while (i < xs.len) {
        let dx: f64 = xs[i] - mx;
        let dy: f64 = ys[i] - my;
        sxx = sxx + dx * dx;
        syy = syy + dy * dy;
        sxy = sxy + dx * dy;
        i = i + 1;
    }
    if (sxx <= 0.0) {
        return LinearFit { slope: 0.0, intercept: 0.0, r_squared: 0.0, valid: false };
    }
    let sl: f64 = sxy / sxx;
    let ic: f64 = my - sl * mx;
    let r2: f64 = if (syy <= 0.0) { 1.0 } else { (sxy * sxy) / (sxx * syy) };
    return LinearFit {
        slope: sl,
        intercept: ic,
        r_squared: r2,
        valid: true,
    };
}
