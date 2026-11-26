// packages/@cap/num-integer/src/index.cp
// Zero-allocation integer number theory & exact integer division library.
// Clean-room MIT pure CP implementation inspired by Rust `num-integer` v0.1.

/// Result of the extended Euclidean algorithm satisfying `a * x + b * y == gcd` with `gcd >= 0`.
pub struct ExtGcdResult {
    pub gcd: i64,
    pub x: i64,
    pub y: i64,
}

/// Overflow-safe absolute value of signed 64-bit integer as `u64` (handles `i64::MIN`).
fn nintAbsU64(v: i64): u64 {
    if (v >= 0) {
        return v as u64;
    }
    return ((-(v + 1)) as u64) + 1;
}

/// Signed truncating division & remainder computed strictly via `u64` division.
fn nintDivTruncRem(a: i64, b: i64, q_out: *i64, r_out: *i64) {
    if (b == 0) {
        q_out.* = 0;
        r_out.* = 0;
        return;
    }
    const ua: u64 = nintAbsU64(a);
    const ub: u64 = nintAbsU64(b);
    const uq: u64 = ua / ub;
    const ur: u64 = ua % ub;

    const neg_q: bool = (a < 0) != (b < 0);
    if (neg_q && uq != 0) {
        q_out.* = -(uq as i64);
    } else {
        q_out.* = uq as i64;
    }

    if (a < 0 && ur != 0) {
        r_out.* = -(ur as i64);
    } else {
        r_out.* = ur as i64;
    }
}

/// Greatest common divisor of two unsigned 64-bit integers (Euclidean algorithm).
pub fn gcdU64(a: u64, b: u64): u64 {
    let x: u64 = a;
    let y: u64 = b;
    while (y != 0) {
        const t: u64 = x % y;
        x = y;
        y = t;
    }
    return x;
}

/// Least common multiple of two unsigned 64-bit integers, dividing before multiplying to avoid overflow.
pub fn lcmU64(a: u64, b: u64): u64 {
    if (a == 0 || b == 0) {
        return 0;
    }
    const g: u64 = gcdU64(a, b);
    return (a / g) * b;
}

/// Extended Euclidean algorithm returning `{ gcd, x, y }` such that `a * x + b * y == gcd` and `gcd >= 0`.
pub fn extendedGcd(a: i64, b: i64): ExtGcdResult {
    let old_r: i64 = a;
    let r: i64 = b;
    let old_s: i64 = 1;
    let s: i64 = 0;
    let old_t: i64 = 0;
    let t: i64 = 1;

    while (r != 0) {
        let q: i64 = 0;
        let rem: i64 = 0;
        nintDivTruncRem(old_r, r, &q, &rem);

        old_r = r;
        r = rem;

        const next_s: i64 = old_s - q * s;
        old_s = s;
        s = next_s;

        const next_t: i64 = old_t - q * t;
        old_t = t;
        t = next_t;
    }

    if (old_r < 0) {
        old_r = -old_r;
        old_s = -old_s;
        old_t = -old_t;
    }

    return ExtGcdResult {
        gcd: old_r,
        x: old_s,
        y: old_t,
    };
}

/// Mathematical floored integer division (rounds toward negative infinity).
/// Returns 0 if `b == 0`.
pub fn divFloor(a: i64, b: i64): i64 {
    if (b == 0) {
        return 0;
    }
    let q: i64 = 0;
    let r: i64 = 0;
    nintDivTruncRem(a, b, &q, &r);
    if (r != 0 && ((r < 0) != (b < 0))) {
        q -= 1;
    }
    return q;
}

/// Mathematical floored modulo matching `divFloor` (`a == divFloor(a, b) * b + modFloor(a, b)`).
/// Result has the same sign as `b` (or is 0). Returns 0 if `b == 0`.
pub fn modFloor(a: i64, b: i64): i64 {
    if (b == 0) {
        return 0;
    }
    let q: i64 = 0;
    let r: i64 = 0;
    nintDivTruncRem(a, b, &q, &r);
    if (r != 0 && ((r < 0) != (b < 0))) {
        r += b;
    }
    return r;
}

/// Mathematical ceiling integer division (rounds toward positive infinity).
/// Returns 0 if `b == 0`.
pub fn divCeil(a: i64, b: i64): i64 {
    if (b == 0) {
        return 0;
    }
    let q: i64 = 0;
    let r: i64 = 0;
    nintDivTruncRem(a, b, &q, &r);
    if (r != 0 && ((r > 0) == (b > 0))) {
        q += 1;
    }
    return q;
}

/// Exact integer square root `floor(sqrt(n))` for all `u64` without overflow.
pub fn isqrtU64(n: u64): u64 {
    let rem: u64 = n;
    let res: u64 = 0;
    let bit: u64 = (1 as u64) << (62 as u6);

    while (bit > rem) {
        bit = bit >> (2 as u6);
    }
    while (bit != 0) {
        if (rem >= res + bit) {
            rem -= res + bit;
            res = (res >> (1 as u6)) + bit;
        } else {
            res = res >> (1 as u6);
        }
        bit = bit >> (2 as u6);
    }
    return res;
}

/// Exact integer cube root `floor(cbrt(n))` for all `u64` without overflow.
pub fn icbrtU64(n: u64): u64 {
    if (n == 0) {
        return 0;
    }
    // floor(cbrt(u64::MAX)) == 2642245; 2642245^3 fits cleanly inside u64.
    let lo: u64 = 1;
    let hi: u64 = 2642245;
    if (n < hi) {
        hi = n;
    }
    let ans: u64 = 1;
    while (lo <= hi) {
        const mid: u64 = lo + ((hi - lo) >> (1 as u6));
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

/// Exact binomial coefficient `C(n, k)` using step-wise GCD reduction to prevent intermediate overflow.
pub fn binomialU64(n: u64, k: u64): u64 {
    if (k > n) {
        return 0;
    }
    let kk: u64 = k;
    if (kk > n - kk) {
        kk = n - kk;
    }
    let res: u64 = 1;
    let i: u64 = 1;
    while (i <= kk) {
        const num: u64 = n - kk + i;
        const g1: u64 = gcdU64(res, i);
        const res_div: u64 = res / g1;
        const den_div: u64 = i / g1;
        const num_div: u64 = num / den_div;
        res = res_div * num_div;
        i += 1;
    }
    return res;
}

/// Returns true if `n` is even.
pub fn isEven(n: i64): bool {
    return (n & 1) == 0;
}

/// Returns true if `n` is odd.
pub fn isOdd(n: i64): bool {
    return (n & 1) != 0;
}

/// Returns the smallest multiple of `step` that is `>= n`. Returns `n` if `step == 0`.
pub fn nextMultipleOfU64(n: u64, step: u64): u64 {
    if (step == 0) {
        return n;
    }
    const rem: u64 = n % step;
    if (rem == 0) {
        return n;
    }
    return n + (step - rem);
}

/// Returns the largest multiple of `step` that is `<= n`. Returns `n` if `step == 0`.
pub fn prevMultipleOfU64(n: u64, step: u64): u64 {
    if (step == 0) {
        return n;
    }
    return n - (n % step);
}
