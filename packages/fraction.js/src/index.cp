// @cap/rational — Zero-heap exact rational number (fraction) arithmetic in pure CP
// MIT License — Clean-room implementation maintaining denom > 0 and gcd(|numer|, denom) == 1

pub struct Rational {
    pub numer: i64,
    pub denom: i64,
}

fn absToU64(v: i64): u64 {
    return if (v < 0) (-v) as u64 else v as u64;
}

/// Signed truncated division implemented via unsigned u64 division.
fn divTrunc64(a: i64, b: i64): i64 {
    const neg_res: bool = (a < 0) != (b < 0);
    const ua: u64 = absToU64(a);
    const ub: u64 = absToU64(b);
    const q: i64 = (ua / ub) as i64;
    return if (neg_res) -q else q;
}

/// Greatest common divisor of two 64-bit signed integers (always returns >= 0).
pub fn gcd64(a: i64, b: i64): i64 {
    let x: u64 = absToU64(a);
    let y: u64 = absToU64(b);
    while (y != 0) {
        const rem: u64 = x % y;
        x = y;
        y = rem;
    }
    return x as i64;
}

/// Construct the rational zero (0/1).
pub fn zero(): Rational {
    return Rational { numer: 0, denom: 1 };
}

/// Construct the rational one (1/1).
pub fn one(): Rational {
    return Rational { numer: 1, denom: 1 };
}

/// Construct a rational from an integer (n/1).
pub fn fromInt(n: i64): Rational {
    return Rational { numer: n, denom: 1 };
}

/// Construct a reduced rational n/d with denom > 0 and gcd(|numer|, denom) == 1.
/// Safely returns 0/1 if d == 0.
pub fn new(n: i64, d: i64): Rational {
    if (d == 0 || n == 0) {
        return zero();
    }
    let nn: i64 = n;
    let dd: i64 = d;
    if (dd < 0) {
        nn = -nn;
        dd = -dd;
    }
    const g: i64 = gcd64(nn, dd);
    return Rational {
        numer: divTrunc64(nn, g),
        denom: divTrunc64(dd, g),
    };
}

/// Negate a rational number (-a).
pub fn neg(a: Rational): Rational {
    const r: Rational = new(a.numer, a.denom);
    return Rational { numer: -r.numer, denom: r.denom };
}

/// Absolute value of a rational number (|a|).
pub fn abs(a: Rational): Rational {
    const r: Rational = new(a.numer, a.denom);
    return Rational {
        numer: if (r.numer < 0) -r.numer else r.numer,
        denom: r.denom,
    };
}

/// Reciprocal (1/a); safely returns 0/1 when a == 0.
pub fn recip(a: Rational): Rational {
    if (a.numer == 0 || a.denom == 0) {
        return zero();
    }
    return new(a.denom, a.numer);
}

/// Exact addition (a + b) with intermediate gcd reduction.
pub fn add(a: Rational, b: Rational): Rational {
    const ra: Rational = new(a.numer, a.denom);
    const rb: Rational = new(b.numer, b.denom);
    const g: i64 = gcd64(ra.denom, rb.denom);
    const ad: i64 = divTrunc64(ra.denom, g);
    const bd: i64 = divTrunc64(rb.denom, g);
    return new(ra.numer * bd + rb.numer * ad, ad * rb.denom);
}

/// Exact subtraction (a - b) with intermediate gcd reduction.
pub fn sub(a: Rational, b: Rational): Rational {
    const ra: Rational = new(a.numer, a.denom);
    const rb: Rational = new(b.numer, b.denom);
    const g: i64 = gcd64(ra.denom, rb.denom);
    const ad: i64 = divTrunc64(ra.denom, g);
    const bd: i64 = divTrunc64(rb.denom, g);
    return new(ra.numer * bd - rb.numer * ad, ad * rb.denom);
}

/// Exact multiplication (a * b) with cross-cancellation.
pub fn mul(a: Rational, b: Rational): Rational {
    const ra: Rational = new(a.numer, a.denom);
    const rb: Rational = new(b.numer, b.denom);
    if (ra.numer == 0 || rb.numer == 0) {
        return zero();
    }
    const g1: i64 = gcd64(ra.numer, rb.denom);
    const g2: i64 = gcd64(rb.numer, ra.denom);
    return new(
        divTrunc64(ra.numer, g1) * divTrunc64(rb.numer, g2),
        divTrunc64(ra.denom, g2) * divTrunc64(rb.denom, g1)
    );
}

/// Exact division (a / b); safely returns 0/1 when b == 0.
pub fn div(a: Rational, b: Rational): Rational {
    if (b.numer == 0 || b.denom == 0) {
        return zero();
    }
    return mul(a, recip(b));
}

/// Exact equality test.
pub fn eq(a: Rational, b: Rational): bool {
    const ra: Rational = new(a.numer, a.denom);
    const rb: Rational = new(b.numer, b.denom);
    return ra.numer == rb.numer && ra.denom == rb.denom;
}

/// Three-way comparison returning -1 (a < b), 0 (a == b), or 1 (a > b).
pub fn cmp(a: Rational, b: Rational): i32 {
    const ra: Rational = new(a.numer, a.denom);
    const rb: Rational = new(b.numer, b.denom);
    const g: i64 = gcd64(ra.denom, rb.denom);
    const lhs: i64 = ra.numer * divTrunc64(rb.denom, g);
    const rhs: i64 = rb.numer * divTrunc64(ra.denom, g);
    if (lhs < rhs) {
        return -1;
    }
    if (lhs > rhs) {
        return 1;
    }
    return 0;
}

/// Convert rational to f64 floating-point approximation.
pub fn toF64(a: Rational): f64 {
    if (a.denom == 0) {
        return 0.0;
    }
    return (a.numer as f64) / (a.denom as f64);
}

/// Floor to the greatest integer <= a.
pub fn floorInt(a: Rational): i64 {
    if (a.denom == 0) {
        return 0;
    }
    const r: Rational = new(a.numer, a.denom);
    const q: i64 = divTrunc64(r.numer, r.denom);
    const rem: i64 = r.numer - q * r.denom;
    if (rem < 0) {
        return q - 1;
    }
    return q;
}

/// Ceiling to the least integer >= a.
pub fn ceilInt(a: Rational): i64 {
    if (a.denom == 0) {
        return 0;
    }
    const r: Rational = new(a.numer, a.denom);
    const q: i64 = divTrunc64(r.numer, r.denom);
    const rem: i64 = r.numer - q * r.denom;
    if (rem > 0) {
        return q + 1;
    }
    return q;
}

/// Return true iff the rational represents an exact integer (reduced denom == 1).
pub fn isInteger(a: Rational): bool {
    if (a.denom == 0) {
        return false;
    }
    const r: Rational = new(a.numer, a.denom);
    return r.denom == 1;
}
