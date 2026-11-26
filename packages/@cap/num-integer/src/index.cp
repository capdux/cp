// @cap/num-integer — Zero-allocation integer number theory & exact division engine for CP
// Clean-room MIT implementation inspired by Rust `num-integer` (crates.io #145, 888M+ downloads).
//
// Features:
// - Exact floor, ceil, and Euclidean division/remainder (`divFloor`, `modFloor`, `divCeil`, `divEuclid`, `remEuclid`).
// - GCD, overflow-safe LCM, Extended Euclidean algorithm (`extendedGcd`), and modular inverse (`modInverse`).
// - Overflow-free integer square root (`isqrtU64`), cube root (`icbrtU64`), binomial coefficients (`binomialU64`),
//   deterministic 6k±1 primality test (`isPrimeU64`), and overflow-free floor average (`averageFloor`).
// - Pure integer arithmetic with zero heap allocation and zero floating-point inaccuracy.

pub struct ExtendedGcd {
    pub gcd: i64,
    pub x: i64,
    pub y: i64,
}

fn absU64(v: i64) -> u64 {
    if (v < 0) {
        return ((0 - (v + 1)) as u64) + 1;
    }
    return v as u64;
}

fn divTruncI64(a: i64, b: i64) -> i64 {
    const qa: u64 = absU64(a) / absU64(b);
    const neg = (a < 0) != (b < 0);
    if (neg && qa > 0) {
        return 0 - (qa as i64);
    }
    return qa as i64;
}

fn remTruncI64(a: i64, b: i64) -> i64 {
    const ra: u64 = absU64(a) % absU64(b);
    if (a < 0 && ra > 0) {
        return 0 - (ra as i64);
    }
    return ra as i64;
}

// ============================================================================
// 1. Exact Floor, Ceil & Euclidean Division / Remainder
// ============================================================================

/// Floored integer division (rounds toward negative infinity).
/// Example: `divFloor(-7, 3) == -3`.
pub fn divFloor(a: i64, b: i64) -> i64 {
    const q = divTruncI64(a, b);
    const r = remTruncI64(a, b);
    if (r != 0 && ((r < 0) != (b < 0))) {
        return q - 1;
    }
    return q;
}

/// Floored modulo (same sign as divisor `b`).
/// Satisfies `divFloor(a, b) * b + modFloor(a, b) == a`.
pub fn modFloor(a: i64, b: i64) -> i64 {
    const r = remTruncI64(a, b);
    if (r != 0 && ((r < 0) != (b < 0))) {
        return r + b;
    }
    return r;
}

/// Ceiling integer division (rounds toward positive infinity).
/// Example: `divCeil(7, 3) == 3`, `divCeil(-7, 3) == -2`.
pub fn divCeil(a: i64, b: i64) -> i64 {
    const q = divTruncI64(a, b);
    const r = remTruncI64(a, b);
    if (r != 0 && ((r > 0) == (b > 0))) {
        return q + 1;
    }
    return q;
}

/// Euclidean integer division paired with non-negative `remEuclid`.
pub fn divEuclid(a: i64, b: i64) -> i64 {
    const q = divTruncI64(a, b);
    const r = remTruncI64(a, b);
    if (r < 0) {
        if (b > 0) {
            return q - 1;
        }
        return q + 1;
    }
    return q;
}

/// Euclidean remainder guaranteed to lie in `[0, |b|)`.
pub fn remEuclid(a: i64, b: i64) -> i64 {
    const r = remTruncI64(a, b);
    if (r < 0) {
        if (b > 0) {
            return r + b;
        }
        return r - b;
    }
    return r;
}

// ============================================================================
// 2. GCD, LCM, Extended Euclidean Algorithm & Modular Inverse
// ============================================================================

/// Greatest common divisor for unsigned 64-bit integers.
pub fn gcdU64(a: u64, b: u64) -> u64 {
    let x = a;
    let y = b;
    while (y != 0) {
        const rem = x % y;
        x = y;
        y = rem;
    }
    return x;
}

/// Least common multiple for unsigned 64-bit integers (divides before multiplying to reduce overflow).
pub fn lcmU64(a: u64, b: u64) -> u64 {
    if (a == 0 || b == 0) {
        return 0;
    }
    return (a / gcdU64(a, b)) * b;
}

/// Non-negative greatest common divisor for signed 64-bit integers.
pub fn gcdI64(a: i64, b: i64) -> i64 {
    return gcdU64(absU64(a), absU64(b)) as i64;
}

/// Non-negative least common multiple for signed 64-bit integers.
pub fn lcmI64(a: i64, b: i64) -> i64 {
    return lcmU64(absU64(a), absU64(b)) as i64;
}

/// Extended Euclidean algorithm returning `{ gcd, x, y }` with `gcd >= 0` and `a*x + b*y == gcd`.
pub fn extendedGcd(a: i64, b: i64) -> ExtendedGcd {
    let old_r: i64 = a;
    let r: i64 = b;
    let old_s: i64 = 1;
    let s: i64 = 0;
    let old_t: i64 = 0;
    let t: i64 = 1;

    while (r != 0) {
        const q = divTruncI64(old_r, r);
        const next_r = old_r - (q * r);
        old_r = r;
        r = next_r;

        const next_s = old_s - (q * s);
        old_s = s;
        s = next_s;

        const next_t = old_t - (q * t);
        old_t = t;
        t = next_t;
    }

    if (old_r < 0) {
        old_r = 0 - old_r;
        old_s = 0 - old_s;
        old_t = 0 - old_t;
    }

    return ExtendedGcd{ gcd: old_r, x: old_s, y: old_t };
}

/// Computes the modular multiplicative inverse of `a` modulo `m` in `[0, m)`.
/// Returns `null` if `m <= 0` or `gcd(a, m) != 1`.
pub fn modInverse(a: i64, m: i64) -> i64? {
    if (m <= 0) {
        return null;
    }
    if (m == 1) {
        return 0;
    }
    const eg = extendedGcd(a, m);
    if (eg.gcd != 1) {
        return null;
    }
    return remEuclid(eg.x, m);
}

// ============================================================================
// 3. Integer Roots, Binomial Coefficients, Primality & Overflow-Free Average
// ============================================================================

/// Exact integer square root `floor(sqrt(n))` across the full `u64` domain (zero overflow).
pub fn isqrtU64(n: u64) -> u64 {
    let bit: u64 = (1 as u64) << 62;
    while (bit > n) {
        bit = bit >> 2;
    }
    let res: u64 = 0;
    let rem: u64 = n;
    while (bit != 0) {
        if (rem >= res + bit) {
            rem -= res + bit;
            res = (res >> 1) + bit;
        } else {
            res = res >> 1;
        }
        bit = bit >> 2;
    }
    return res;
}

/// Exact integer cube root `floor(cbrt(n))` across the full `u64` domain (zero overflow).
pub fn icbrtU64(n: u64) -> u64 {
    if (n <= 1) {
        return n;
    }
    let lo: u64 = 1;
    // floor(cbrt(u64::MAX)) == 2642245
    let hi: u64 = 2642245;
    if (n < hi) {
        hi = n;
    }
    let ans: u64 = 1;
    while (lo <= hi) {
        const mid: u64 = lo + ((hi - lo) >> 1);
        const cube: u64 = mid * mid * mid;
        if (cube == n) {
            return mid;
        }
        if (cube < n) {
            ans = mid;
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return ans;
}

/// Computes binomial coefficient `C(n, k)` using GCD reduction at each step to prevent intermediate overflow.
pub fn binomialU64(n: u64, k: u64) -> u64 {
    if (k > n) {
        return 0;
    }
    if (k == 0 || k == n) {
        return 1;
    }
    const kk: u64 = if (k > n - k) { n - k } else { k };
    let res: u64 = 1;
    let i: u64 = 1;
    while (i <= kk) {
        const num: u64 = n - kk + i;
        const g1: u64 = gcdU64(res, i);
        const d1: u64 = i / g1;
        res = (res / g1) * (num / d1);
        i += 1;
    }
    return res;
}

/// Deterministic 6k±1 primality test for unsigned 64-bit integers.
pub fn isPrimeU64(n: u64) -> bool {
    if (n < 2) {
        return false;
    }
    if (n == 2 || n == 3) {
        return true;
    }
    if ((n % 2) == 0 || (n % 3) == 0) {
        return false;
    }
    const limit = isqrtU64(n);
    let i: u64 = 5;
    while (i <= limit) {
        if ((n % i) == 0 || (n % (i + 2)) == 0) {
            return false;
        }
        i += 6;
    }
    return true;
}

/// Overflow-free floored average `floor((a + b) / 2)` for signed 64-bit integers.
pub fn averageFloor(a: i64, b: i64) -> i64 {
    return (a & b) + ((a ^ b) >> 1);
}
