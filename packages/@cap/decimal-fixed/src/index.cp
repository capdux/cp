// @cap/decimal-fixed — Zero-heap high-precision financial decimal arithmetic in pure CP
// MIT License — Clean-room implementation (value = mantissa * 10^(-scale), scale in 0..12)

pub const MAX_SCALE: u8 = 12;

const POW10: [13]u64 = [
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000,
    1000000000,
    10000000000,
    100000000000,
    1000000000000,
];

fn absToU64(v: i64): u64 {
    return if (v < 0) (-v) as u64 else v as u64;
}

fn clampScale(s: u8): u8 {
    return if (s > MAX_SCALE) MAX_SCALE else s;
}

fn pow10F64(s: u8): f64 {
    return POW10[clampScale(s) as usize] as f64;
}

pub struct ParseDecResult {
    pub value: Decimal,
    pub valid: bool,
}

/// Fixed-point decimal number representing `mantissa * 10^(-scale)` with `scale` in `[0, 12]`.
pub struct Decimal {
    pub mantissa: i64,
    pub scale: u8,

    /// Construct a Decimal with given mantissa and scale (clamped to MAX_SCALE = 12).
    pub static fn new(mantissa: i64, scale: u8): Decimal {
        return Decimal {
            mantissa: mantissa,
            scale: clampScale(scale),
        };
    }

    /// Construct a Decimal from an integer (scale = 0).
    pub static fn fromInt(v: i64): Decimal {
        return Decimal { mantissa: v, scale: 0 };
    }

    /// Construct zero (0, scale 0).
    pub static fn zero(): Decimal {
        return Decimal { mantissa: 0, scale: 0 };
    }

    /// Construct one (1, scale 0).
    pub static fn one(): Decimal {
        return Decimal { mantissa: 1, scale: 0 };
    }

    /// Parse a decimal string such as `"-123.45"`, `"+0.100"`, `"42"`, `".5"`.
    pub static fn parse(s: string): ParseDecResult {
        if (s.len == 0) {
            return ParseDecResult { value: Decimal.zero(), valid: false };
        }
        let idx: usize = 0;
        let is_neg: bool = false;
        if (s[0] == 45) { // '-'
            is_neg = true;
            idx = 1;
        } else if (s[0] == 43) { // '+'
            idx = 1;
        }
        if (idx >= s.len) {
            return ParseDecResult { value: Decimal.zero(), valid: false };
        }

        let mag: u64 = 0;
        let seen_dot: bool = false;
        let seen_digit: bool = false;
        let frac_digits: u8 = 0;

        while (idx < s.len) {
            const ch: u8 = s[idx];
            if (ch == 46) { // '.'
                if (seen_dot) {
                    return ParseDecResult { value: Decimal.zero(), valid: false };
                }
                seen_dot = true;
            } else if (ch >= 48 && ch <= 57) {
                const digit: u64 = (ch - 48) as u64;
                seen_digit = true;
                if (seen_dot) {
                    if (frac_digits >= MAX_SCALE) {
                        if (digit != 0) {
                            return ParseDecResult { value: Decimal.zero(), valid: false };
                        }
                    } else {
                        if (mag > 922337203685477580) {
                            return ParseDecResult { value: Decimal.zero(), valid: false };
                        }
                        mag = mag * 10 + digit;
                        frac_digits += 1;
                    }
                } else {
                    if (mag > 922337203685477580) {
                        return ParseDecResult { value: Decimal.zero(), valid: false };
                    }
                    mag = mag * 10 + digit;
                }
            } else {
                return ParseDecResult { value: Decimal.zero(), valid: false };
            }
            idx += 1;
        }

        if (!seen_digit) {
            return ParseDecResult { value: Decimal.zero(), valid: false };
        }
        const m: i64 = if (is_neg && mag != 0) -(mag as i64) else (mag as i64);
        return ParseDecResult {
            value: Decimal { mantissa: m, scale: frac_digits },
            valid: true,
        };
    }

    /// Strip trailing fractional zeros (e.g. `1.2500` -> `1.25`, `0.00` -> `0`).
    pub fn normalize(): Decimal {
        if (_.mantissa == 0) {
            return Decimal.zero();
        }
        let mag: u64 = absToU64(_.mantissa);
        let sc: u8 = clampScale(_.scale);
        while (sc > 0 && (mag % 10) == 0) {
            mag = mag / 10;
            sc -= 1;
        }
        const m: i64 = if (_.mantissa < 0) -(mag as i64) else (mag as i64);
        return Decimal { mantissa: m, scale: sc };
    }

    /// Rescale to `new_scale` (clamped to MAX_SCALE), using half-away-from-zero rounding when shrinking.
    pub fn rescale(new_scale: u8): Decimal {
        const target: u8 = clampScale(new_scale);
        const cur: u8 = clampScale(_.scale);
        if (target == cur) {
            return Decimal { mantissa: _.mantissa, scale: cur };
        }
        if (target > cur) {
            const factor: i64 = POW10[(target - cur) as usize] as i64;
            return Decimal { mantissa: _.mantissa * factor, scale: target };
        }
        const diff: usize = (cur - target) as usize;
        const factor: u64 = POW10[diff];
        const half: u64 = factor / 2;
        const mag: u64 = absToU64(_.mantissa);
        let q: u64 = mag / factor;
        const rem: u64 = mag % factor;
        if (rem >= half) {
            q += 1;
        }
        const m: i64 = if (_.mantissa < 0 && q != 0) -(q as i64) else (q as i64);
        return Decimal { mantissa: m, scale: target };
    }

    /// Banker's rounding (round half to even) to `dp` decimal places.
    pub fn roundHalfEven(dp: u8): Decimal {
        const target: u8 = clampScale(dp);
        const cur: u8 = clampScale(_.scale);
        if (target >= cur) {
            return _.rescale(target);
        }
        const diff: usize = (cur - target) as usize;
        const factor: u64 = POW10[diff];
        const half: u64 = factor / 2;
        const mag: u64 = absToU64(_.mantissa);
        let q: u64 = mag / factor;
        const rem: u64 = mag % factor;
        if (rem > half || (rem == half && (q & 1) == 1)) {
            q += 1;
        }
        const m: i64 = if (_.mantissa < 0 && q != 0) -(q as i64) else (q as i64);
        return Decimal { mantissa: m, scale: target };
    }

    /// Negate this decimal.
    pub fn neg(): Decimal {
        return Decimal { mantissa: -_.mantissa, scale: clampScale(_.scale) };
    }

    /// Absolute value of this decimal.
    pub fn abs(): Decimal {
        return Decimal {
            mantissa: if (_.mantissa < 0) -_.mantissa else _.mantissa,
            scale: clampScale(_.scale),
        };
    }

    /// Exact addition aligning scales automatically.
    pub fn add(other: Decimal): Decimal {
        const sa: u8 = clampScale(_.scale);
        const sb: u8 = clampScale(other.scale);
        const target: u8 = if (sa > sb) sa else sb;
        const ma: i64 = _.mantissa * (POW10[(target - sa) as usize] as i64);
        const mb: i64 = other.mantissa * (POW10[(target - sb) as usize] as i64);
        return Decimal { mantissa: ma + mb, scale: target };
    }

    /// Exact subtraction aligning scales automatically.
    pub fn sub(other: Decimal): Decimal {
        const sa: u8 = clampScale(_.scale);
        const sb: u8 = clampScale(other.scale);
        const target: u8 = if (sa > sb) sa else sb;
        const ma: i64 = _.mantissa * (POW10[(target - sa) as usize] as i64);
        const mb: i64 = other.mantissa * (POW10[(target - sb) as usize] as i64);
        return Decimal { mantissa: ma - mb, scale: target };
    }

    /// Exact multiplication (normalizes operands first; rounds to MAX_SCALE if combined scale > 12).
    pub fn mul(other: Decimal): Decimal {
        const a: Decimal = _.normalize();
        const b: Decimal = other.normalize();
        if (a.mantissa == 0 || b.mantissa == 0) {
            return Decimal.zero();
        }
        const raw_scale: u8 = a.scale + b.scale;
        const prod: i64 = a.mantissa * b.mantissa;
        if (raw_scale <= MAX_SCALE) {
            return Decimal { mantissa: prod, scale: raw_scale };
        }
        const drop: usize = (raw_scale - MAX_SCALE) as usize;
        const factor: u64 = POW10[drop];
        const half: u64 = factor / 2;
        const mag: u64 = absToU64(prod);
        let q: u64 = mag / factor;
        if ((mag % factor) >= half) {
            q += 1;
        }
        const m: i64 = if (prod < 0 && q != 0) -(q as i64) else (q as i64);
        return Decimal { mantissa: m, scale: MAX_SCALE };
    }

    /// Exact long-division to `out_scale` decimal places (clamped to MAX_SCALE).
    /// Safely returns Decimal.zero() on division by zero.
    pub fn div(other: Decimal, out_scale: u8): Decimal {
        if (other.mantissa == 0) {
            return Decimal.zero();
        }
        const a: Decimal = _.normalize();
        const b: Decimal = other.normalize();
        if (a.mantissa == 0) {
            return Decimal { mantissa: 0, scale: clampScale(out_scale) };
        }
        const target: u8 = clampScale(out_scale);
        const neg_res: bool = (a.mantissa < 0) != (b.mantissa < 0);
        const ua: u64 = absToU64(a.mantissa);
        const ub: u64 = absToU64(b.mantissa);
        const shift: i32 = (target as i32) + (b.scale as i32) - (a.scale as i32);

        let q: u64 = 0;
        let rem: u64 = 0;
        let denom: u64 = ub;

        if (shift >= 0) {
            q = ua / ub;
            rem = ua % ub;
            let step: i32 = 0;
            while (step < shift) {
                const scaled_rem: u64 = rem * 10;
                q = q * 10 + (scaled_rem / ub);
                rem = scaled_rem % ub;
                step += 1;
            }
        } else {
            const neg_shift: usize = (-shift) as usize;
            const factor: u64 = if (neg_shift <= 12) POW10[neg_shift] else POW10[12];
            denom = ub * factor;
            q = ua / denom;
            rem = ua % denom;
        }

        if (rem * 2 >= denom) {
            q += 1;
        }
        const m: i64 = if (neg_res && q != 0) -(q as i64) else (q as i64);
        return Decimal { mantissa: m, scale: target };
    }

    /// Three-way comparison returning -1 (self < other), 0 (equal), or 1 (self > other).
    pub fn cmp(other: Decimal): i32 {
        const a: Decimal = _.normalize();
        const b: Decimal = other.normalize();
        const target: u8 = if (a.scale > b.scale) a.scale else b.scale;
        const ma: i64 = a.mantissa * (POW10[(target - a.scale) as usize] as i64);
        const mb: i64 = b.mantissa * (POW10[(target - b.scale) as usize] as i64);
        if (ma < mb) {
            return -1;
        }
        if (ma > mb) {
            return 1;
        }
        return 0;
    }

    /// Exact numeric equality (independent of trailing zeros / scale).
    pub fn eq(other: Decimal): bool {
        return _.cmp(other) == 0;
    }

    /// Convert to f64 floating-point value.
    pub fn toF64(): f64 {
        return (_.mantissa as f64) / pow10F64(_.scale);
    }

    /// Format into `out` as standard decimal text (e.g. `"-123.45"`, `"0.05"`, `"42"`).
    /// Returns bytes written, or 0 if `out` is too small.
    pub fn format(out: []u8): usize {
        const sc: u8 = clampScale(_.scale);
        const is_neg: bool = _.mantissa < 0;
        let mag: u64 = absToU64(_.mantissa);

        let rev: [24]u8 = undefined;
        let digits_len: usize = 0;
        if (mag == 0) {
            rev[0] = 48;
            digits_len = 1;
        } else {
            while (mag > 0) {
                rev[digits_len] = ((mag % 10) as u8) + 48;
                mag = mag / 10;
                digits_len += 1;
            }
        }

        let pos: usize = 0;
        if (is_neg) {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = 45; // '-'
            pos += 1;
        }

        if (sc == 0) {
            if (pos + digits_len > out.len) {
                return 0;
            }
            let k: usize = digits_len;
            while (k > 0) {
                k -= 1;
                out[pos] = rev[k];
                pos += 1;
            }
            return pos;
        }

        const sc_usize: usize = sc as usize;
        if (digits_len <= sc_usize) {
            const needed: usize = pos + 2 + sc_usize;
            if (needed > out.len) {
                return 0;
            }
            out[pos] = 48; // '0'
            pos += 1;
            out[pos] = 46; // '.'
            pos += 1;
            let zeros: usize = sc_usize - digits_len;
            while (zeros > 0) {
                out[pos] = 48;
                pos += 1;
                zeros -= 1;
            }
            let k: usize = digits_len;
            while (k > 0) {
                k -= 1;
                out[pos] = rev[k];
                pos += 1;
            }
            return pos;
        }

        const int_digits: usize = digits_len - sc_usize;
        if (pos + digits_len + 1 > out.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < int_digits) {
            out[pos] = rev[digits_len - 1 - i];
            pos += 1;
            i += 1;
        }
        out[pos] = 46; // '.'
        pos += 1;
        let f: usize = 0;
        while (f < sc_usize) {
            out[pos] = rev[sc_usize - 1 - f];
            pos += 1;
            f += 1;
        }
        return pos;
    }
}
