// @cap/bigdecimal — Zero-heap exact base-10 financial arithmetic.
// Clean-room MIT implementation modeled on crates.io `bigdecimal` v0.4.

fn bdecIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn bdecIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn bdecAbsU64(v: i64): u64 {
    if (v < 0) {
        return (-(v + 1)) as u64 + 1;
    }
    return v as u64;
}

fn bdecSignedFromU64(mag: u64, neg: bool): i64 {
    if (mag == 0) { return 0; }
    if (neg) {
        return -((mag as i64));
    }
    return mag as i64;
}

fn bdecPow10U64(exp: u32): u64 {
    let res: u64 = 1;
    let i: u32 = 0;
    while (i < exp) {
        res = res * 10;
        i = i + 1;
    }
    return res;
}

fn bdecDigitCount(u: u64): i32 {
    if (u == 0) { return 1; }
    let v: u64 = u;
    let n: i32 = 0;
    while (v > 0) {
        v = v / 10;
        n = n + 1;
    }
    return n;
}

fn bdecWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) { return pos; }
    out[pos] = b;
    return pos + 1;
}

/// Exact base-10 decimal number representing `digits * 10^(-scale)` with up to 18 significant digits.
pub struct BigDecimal {
    pub digits: i64,
    pub scale: i32,

    /// Returns decimal zero (`0`).
    pub static fn zero(): BigDecimal {
        return BigDecimal { digits: 0, scale: 0 };
    }

    /// Constructs a `BigDecimal` from a 64-bit integer.
    pub static fn fromInt(v: i64): BigDecimal {
        return BigDecimal { digits: v, scale: 0 };
    }

    /// Constructs a `BigDecimal` from unscaled `digits` and base-10 `scale`.
    pub static fn new(digits: i64, scale: i32): BigDecimal {
        return BigDecimal { digits: digits, scale: scale };
    }

    /// Strips trailing base-10 zeros from `digits` and adjusts `scale` accordingly.
    pub fn normalize(): BigDecimal {
        if (_.digits == 0) {
            return BigDecimal { digits: 0, scale: 0 };
        }
        let is_neg: bool = _.digits < 0;
        let mag: u64 = bdecAbsU64(_.digits);
        let sc: i32 = _.scale;
        while (mag > 0 && (mag % 10) == 0) {
            mag = mag / 10;
            sc = sc - 1;
        }
        return BigDecimal {
            digits: bdecSignedFromU64(mag, is_neg),
            scale: sc,
        };
    }

    /// Returns the negated decimal value.
    pub fn neg(): BigDecimal {
        if (_.digits == 0) {
            return BigDecimal { digits: 0, scale: _.scale };
        }
        return BigDecimal { digits: -_.digits, scale: _.scale };
    }

    /// Returns the absolute decimal value.
    pub fn abs(): BigDecimal {
        if (_.digits < 0) {
            return BigDecimal { digits: -_.digits, scale: _.scale };
        }
        return BigDecimal { digits: _.digits, scale: _.scale };
    }

    /// Rounds to `new_scale` decimal places using half-away-from-zero financial rounding.
    pub fn round(new_scale: i32): BigDecimal {
        if (new_scale >= _.scale) {
            return BigDecimal { digits: _.digits, scale: _.scale };
        }
        if (_.digits == 0) {
            return BigDecimal { digits: 0, scale: new_scale };
        }
        let drop: i32 = _.scale - new_scale;
        if (drop > 18) {
            return BigDecimal { digits: 0, scale: new_scale };
        }
        let is_neg: bool = _.digits < 0;
        let mag: u64 = bdecAbsU64(_.digits);
        let p10: u64 = bdecPow10U64(drop as u32);
        let q: u64 = mag / p10;
        let rem: u64 = mag % p10;
        let half: u64 = p10 / 2;
        if (rem >= half) {
            q = q + 1;
        }
        return BigDecimal {
            digits: bdecSignedFromU64(q, is_neg),
            scale: new_scale,
        };
    }

    /// Exact comparison returning `-1` if less, `0` if equal, `1` if greater.
    pub fn cmp(other: BigDecimal): i32 {
        let a: BigDecimal = _.normalize();
        let b: BigDecimal = other.normalize();

        if (a.digits == 0 && b.digits == 0) { return 0; }
        if (a.digits < 0 && b.digits >= 0) { return -1; }
        if (a.digits > 0 && b.digits <= 0) { return 1; }
        if (a.digits == 0) {
            return if (b.digits > 0) { -1 } else { 1 };
        }

        // Same non-zero sign
        let both_neg: bool = a.digits < 0;
        let ma: u64 = bdecAbsU64(a.digits);
        let mb: u64 = bdecAbsU64(b.digits);

        // Compare order of magnitude: digit_count - scale
        let ord_a: i32 = bdecDigitCount(ma) - a.scale;
        let ord_b: i32 = bdecDigitCount(mb) - b.scale;
        if (ord_a != ord_b) {
            let mag_cmp: i32 = if (ord_a > ord_b) { 1 } else { -1 };
            return if (both_neg) { -mag_cmp } else { mag_cmp };
        }

        // Same order of magnitude: align scales safely
        let sa: u64 = ma;
        let sb: u64 = mb;
        if (a.scale < b.scale) {
            let diff: u32 = (b.scale - a.scale) as u32;
            sa = sa * bdecPow10U64(diff);
        } else if (b.scale < a.scale) {
            let diff: u32 = (a.scale - b.scale) as u32;
            sb = sb * bdecPow10U64(diff);
        }

        let mag_cmp: i32 = 0;
        if (sa > sb) { mag_cmp = 1; }
        else if (sa < sb) { mag_cmp = -1; }
        return if (both_neg) { -mag_cmp } else { mag_cmp };
    }

    /// Exact mathematical equality (e.g. `1.20 == 1.2`).
    pub fn eq(other: BigDecimal): bool {
        return _.cmp(other) == 0;
    }

    /// Exact addition.
    pub fn add(other: BigDecimal): BigDecimal {
        let a: BigDecimal = _.normalize();
        let b: BigDecimal = other.normalize();
        if (a.digits == 0) { return b; }
        if (b.digits == 0) { return a; }

        let target_scale: i32 = if (a.scale > b.scale) { a.scale } else { b.scale };
        let da: i64 = a.digits;
        let db: i64 = b.digits;
        if (a.scale < target_scale) {
            let mul_a: i64 = bdecPow10U64((target_scale - a.scale) as u32) as i64;
            da = da * mul_a;
        }
        if (b.scale < target_scale) {
            let mul_b: i64 = bdecPow10U64((target_scale - b.scale) as u32) as i64;
            db = db * mul_b;
        }
        let sum: i64 = da + db;
        if (sum == 0) {
            return BigDecimal { digits: 0, scale: 0 };
        }
        return BigDecimal { digits: sum, scale: target_scale };
    }

    /// Exact subtraction.
    pub fn sub(other: BigDecimal): BigDecimal {
        return _.add(other.neg());
    }

    /// Exact multiplication.
    pub fn mul(other: BigDecimal): BigDecimal {
        let a: BigDecimal = _.normalize();
        let b: BigDecimal = other.normalize();
        if (a.digits == 0 || b.digits == 0) {
            return BigDecimal { digits: 0, scale: 0 };
        }
        return BigDecimal {
            digits: a.digits * b.digits,
            scale: a.scale + b.scale,
        };
    }

    /// Exact division rounded (half-away-from-zero) to `target_scale` decimal places.
    /// Returns `null` when dividing by zero.
    pub fn divWithScale(other: BigDecimal, target_scale: i32): BigDecimal? {
        if (other.digits == 0) { return null; }
        if (_.digits == 0) {
            return BigDecimal { digits: 0, scale: target_scale };
        }

        let a: BigDecimal = _.normalize();
        let b: BigDecimal = other.normalize();
        let is_neg: bool = (a.digits < 0) != (b.digits < 0);
        let num: u64 = bdecAbsU64(a.digits);
        let den: u64 = bdecAbsU64(b.digits);
        let shift: i32 = target_scale + b.scale - a.scale;

        if (shift >= 0) {
            let q: u64 = num / den;
            let rem: u64 = num % den;
            let s: i32 = 0;
            while (s < shift) {
                let d_cur: u64 = den;
                let r_cur: u64 = rem;
                if (r_cur > 1844674407370955161) {
                    r_cur = r_cur / 10;
                    d_cur = d_cur / 10;
                }
                let scaled_rem: u64 = r_cur * 10;
                let digit: u64 = scaled_rem / d_cur;
                rem = scaled_rem % d_cur;
                q = q * 10 + digit;
                s = s + 1;
            }
            // Half-away-from-zero rounding on final remainder
            let doubled: u64 = if (rem > 9223372036854775807) { den } else { rem * 2 };
            if (doubled >= den) {
                q = q + 1;
            }
            return BigDecimal {
                digits: bdecSignedFromU64(q, is_neg),
                scale: target_scale,
            };
        }

        // shift < 0: divide by den * 10^(-shift)
        let q0: u64 = num / den;
        let raw: BigDecimal = BigDecimal {
            digits: bdecSignedFromU64(q0, is_neg),
            scale: target_scale - shift,
        };
        return raw.round(target_scale);
    }
}

/// Parses a decimal string with optional sign, fractional part, scientific exponent (`e`/`E`),
/// and surrounding whitespace. Returns `null` on invalid syntax or overflow.
pub fn parseDecimal(s: string): BigDecimal? {
    let i: usize = 0;
    while (i < s.len && bdecIsSpace(s[i])) {
        i = i + 1;
    }
    let end: usize = s.len;
    while (end > i && bdecIsSpace(s[end - 1])) {
        end = end - 1;
    }
    if (i >= end) { return null; }

    let neg: bool = false;
    if (s[i] == 45) {
        neg = true;
        i = i + 1;
    } else if (s[i] == 43) {
        i = i + 1;
    }
    if (i >= end) { return null; }

    let mag: u64 = 0;
    let has_digits: bool = false;
    let seen_dot: bool = false;
    let frac_count: i32 = 0;
    let max_mag: u64 = 922337203685477580;

    while (i < end) {
        let c: u8 = s[i];
        if (bdecIsDigit(c)) {
            has_digits = true;
            let d: u64 = (c - 48) as u64;
            if (mag > max_mag || (mag == max_mag && d > 7)) {
                return null;
            }
            mag = mag * 10 + d;
            if (seen_dot) {
                frac_count = frac_count + 1;
            }
            i = i + 1;
        } else if (c == 46) {
            if (seen_dot) { return null; }
            seen_dot = true;
            i = i + 1;
        } else if (c == 101 || c == 69) {
            break;
        } else {
            return null;
        }
    }

    if (!has_digits) { return null; }

    let exp_val: i32 = 0;
    if (i < end && (s[i] == 101 || s[i] == 69)) {
        i = i + 1;
        let exp_neg: bool = false;
        if (i < end && s[i] == 45) {
            exp_neg = true;
            i = i + 1;
        } else if (i < end && s[i] == 43) {
            i = i + 1;
        }
        let has_exp: bool = false;
        let ev: i32 = 0;
        while (i < end && bdecIsDigit(s[i])) {
            has_exp = true;
            if (ev < 100000) {
                ev = ev * 10 + ((s[i] - 48) as i32);
            }
            i = i + 1;
        }
        if (!has_exp || i != end) { return null; }
        exp_val = if (exp_neg) { -ev } else { ev };
    }

    if (i != end) { return null; }

    if (mag == 0) {
        return BigDecimal { digits: 0, scale: 0 };
    }
    return BigDecimal {
        digits: bdecSignedFromU64(mag, neg),
        scale: frac_count - exp_val,
    };
}

/// Formats `val` into decimal text in `out`, returning the number of bytes written.
pub fn formatDecimal(val: BigDecimal, out: []u8): usize {
    let p: usize = 0;
    if (val.digits == 0) {
        p = bdecWriteByte(out, p, 48);
        if (val.scale > 0) {
            p = bdecWriteByte(out, p, 46);
            let z: i32 = 0;
            while (z < val.scale) {
                p = bdecWriteByte(out, p, 48);
                z = z + 1;
            }
        }
        return p;
    }

    let neg: bool = val.digits < 0;
    if (neg) {
        p = bdecWriteByte(out, p, 45);
    }

    let mag: u64 = bdecAbsU64(val.digits);
    let rev: [24]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let n: usize = 0;
    while (mag > 0) {
        rev[n] = (48 + (mag % 10)) as u8;
        mag = mag / 10;
        n = n + 1;
    }

    // Reverse into forward digit buffer
    let dbuf: [24]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let k: usize = 0;
    while (k < n) {
        dbuf[k] = rev[n - 1 - k];
        k = k + 1;
    }

    if (val.scale <= 0) {
        let j: usize = 0;
        while (j < n) {
            p = bdecWriteByte(out, p, dbuf[j]);
            j = j + 1;
        }
        let zeros: i32 = -val.scale;
        let z: i32 = 0;
        while (z < zeros) {
            p = bdecWriteByte(out, p, 48);
            z = z + 1;
        }
        return p;
    }

    let sc: usize = val.scale as usize;
    if (n <= sc) {
        p = bdecWriteByte(out, p, 48);
        p = bdecWriteByte(out, p, 46);
        let lead_zeros: usize = sc - n;
        let z: usize = 0;
        while (z < lead_zeros) {
            p = bdecWriteByte(out, p, 48);
            z = z + 1;
        }
        let j: usize = 0;
        while (j < n) {
            p = bdecWriteByte(out, p, dbuf[j]);
            j = j + 1;
        }
        return p;
    }

    let int_len: usize = n - sc;
    let j: usize = 0;
    while (j < int_len) {
        p = bdecWriteByte(out, p, dbuf[j]);
        j = j + 1;
    }
    p = bdecWriteByte(out, p, 46);
    while (j < n) {
        p = bdecWriteByte(out, p, dbuf[j]);
        j = j + 1;
    }
    return p;
}
