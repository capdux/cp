// packages/@cap/num-rational/src/index.cp
// Zero-allocation exact 64-bit rational number library (`Ratio64`).
// Clean-room MIT pure CP implementation inspired by Rust `num-rational` v0.4; reuses @cap/num-integer.

import { gcdU64, divFloor, divCeil } from "../../num-integer/src/index.cp";

fn nratAbsU64(v: i64): u64 {
    if (v >= 0) {
        return v as u64;
    }
    return ((-(v + 1)) as u64) + 1;
}

/// Normalize `(numer, denom)` with `denom != 0` into canonical form: `denom > 0` and `gcd(|numer|, denom) == 1`.
fn nratReduce(numer: i64, denom: i64): Ratio64 {
    if (numer == 0) {
        return Ratio64 { numer: 0, denom: 1 };
    }
    const neg: bool = (numer < 0) != (denom < 0);
    const un: u64 = nratAbsU64(numer);
    const ud: u64 = nratAbsU64(denom);
    const g: u64 = gcdU64(un, ud);
    const rn: u64 = un / g;
    const rd: u64 = ud / g;
    const final_n: i64 = if (neg) { -(rn as i64) } else { rn as i64 };
    return Ratio64 {
        numer: final_n,
        denom: rd as i64,
    };
}

/// Exact 64-bit rational number in canonical reduced form (`denom > 0`, `gcd(|numer|, denom) == 1`).
pub struct Ratio64 {
    pub numer: i64,
    pub denom: i64,

    /// Construct a reduced rational `numer / denom`. Returns `null` if `denom == 0`.
    pub static fn new(numer: i64, denom: i64): Ratio64? {
        if (denom == 0) {
            return null;
        }
        return nratReduce(numer, denom);
    }

    /// Construct an integer rational `n / 1`.
    pub static fn fromInt(n: i64): Ratio64 {
        return Ratio64 { numer: n, denom: 1 };
    }

    /// Return additive identity `0 / 1`.
    pub static fn zero(): Ratio64 {
        return Ratio64 { numer: 0, denom: 1 };
    }

    /// Return multiplicative identity `1 / 1`.
    pub static fn one(): Ratio64 {
        return Ratio64 { numer: 1, denom: 1 };
    }

    /// Return true if this rational is an exact integer (`denom == 1`).
    pub fn isInteger(): bool {
        return _.denom == 1;
    }

    /// Return true if equal to 0.
    pub fn isZero(): bool {
        return _.numer == 0;
    }

    /// Return true if strictly greater than 0.
    pub fn isPositive(): bool {
        return _.numer > 0;
    }

    /// Return true if strictly less than 0.
    pub fn isNegative(): bool {
        return _.numer < 0;
    }

    /// Exact addition with denominator GCD pre-reduction to minimize overflow risk.
    pub fn add(other: Ratio64): Ratio64 {
        const g: u64 = gcdU64(_.denom as u64, other.denom as u64);
        const d1: u64 = (_.denom as u64) / g;
        const d2: u64 = (other.denom as u64) / g;
        const num: i64 = _.numer * (d2 as i64) + other.numer * (d1 as i64);
        const den: i64 = (d1 * (other.denom as u64)) as i64;
        return nratReduce(num, den);
    }

    /// Exact subtraction with denominator GCD pre-reduction.
    pub fn sub(other: Ratio64): Ratio64 {
        const g: u64 = gcdU64(_.denom as u64, other.denom as u64);
        const d1: u64 = (_.denom as u64) / g;
        const d2: u64 = (other.denom as u64) / g;
        const num: i64 = _.numer * (d2 as i64) - other.numer * (d1 as i64);
        const den: i64 = (d1 * (other.denom as u64)) as i64;
        return nratReduce(num, den);
    }

    /// Exact multiplication with cross-cancellation to prevent intermediate overflow.
    pub fn mul(other: Ratio64): Ratio64 {
        if (_.numer == 0 || other.numer == 0) {
            return Ratio64.zero();
        }
        const is_neg: bool = (_.numer < 0) != (other.numer < 0);
        const n1: u64 = nratAbsU64(_.numer);
        const d1: u64 = _.denom as u64;
        const n2: u64 = nratAbsU64(other.numer);
        const d2: u64 = other.denom as u64;

        const g1: u64 = gcdU64(n1, d2);
        const g2: u64 = gcdU64(n2, d1);
        const rn: u64 = (n1 / g1) * (n2 / g2);
        const rd: u64 = (d1 / g2) * (d2 / g1);
        const final_n: i64 = if (is_neg) { -(rn as i64) } else { rn as i64 };
        return Ratio64 { numer: final_n, denom: rd as i64 };
    }

    /// Return multiplicative inverse `denom / numer`, or `null` if `numer == 0`.
    pub fn recip(): Ratio64? {
        if (_.numer == 0) {
            return null;
        }
        if (_.numer < 0) {
            return Ratio64 { numer: -_.denom, denom: -_.numer };
        }
        return Ratio64 { numer: _.denom, denom: _.numer };
    }

    /// Exact division `_ / other`, returning `null` if `other` is zero.
    pub fn div(other: Ratio64): Ratio64? {
        if (inv := other.recip()) {
            return _.mul(inv);
        }
        return null;
    }

    /// Arithmetic negation `-_`.
    pub fn neg(): Ratio64 {
        return Ratio64 { numer: -_.numer, denom: _.denom };
    }

    /// Absolute value `|_|`.
    pub fn abs(): Ratio64 {
        if (_.numer < 0) {
            return Ratio64 { numer: -_.numer, denom: _.denom };
        }
        return Ratio64 { numer: _.numer, denom: _.denom };
    }

    /// Three-way comparison returning `-1` (`_ < other`), `0` (`_ == other`), or `1` (`_ > other`).
    pub fn cmp(other: Ratio64): i32 {
        if (_.numer == other.numer && _.denom == other.denom) {
            return 0;
        }
        const g: u64 = gcdU64(_.denom as u64, other.denom as u64);
        const d1: u64 = (_.denom as u64) / g;
        const d2: u64 = (other.denom as u64) / g;
        const lhs: i64 = _.numer * (d2 as i64);
        const rhs: i64 = other.numer * (d1 as i64);
        if (lhs < rhs) {
            return -1;
        }
        if (lhs > rhs) {
            return 1;
        }
        return 0;
    }

    /// Exact equality check (O(1) thanks to canonical reduced representation).
    pub fn eq(other: Ratio64): bool {
        return _.numer == other.numer && _.denom == other.denom;
    }

    /// Greatest integer `<= _`.
    pub fn floor(): i64 {
        return divFloor(_.numer, _.denom);
    }

    /// Smallest integer `>= _`.
    pub fn ceil(): i64 {
        return divCeil(_.numer, _.denom);
    }

    /// Convert to 64-bit IEEE-754 floating point approximation.
    pub fn toF64(): f64 {
        return (_.numer as f64) / (_.denom as f64);
    }
}
