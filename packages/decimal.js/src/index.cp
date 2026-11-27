// @cap/decimal — Zero-allocation 128-bit exact decimal & financial arithmetic engine for CP
// Clean-room MIT implementation inspired by npm `decimal.js`, `bignumber.js`, and Rust `rust_decimal`.

const MAX_SCALE: u8 = 28;

pub enum RoundingMode {
    HalfEven, // Banker's rounding (IEEE 754 / financial standard)
    HalfUp,   // Round half away from zero
    HalfDown, // Round half toward zero
    Floor,    // Toward -infinity
    Ceil,     // Toward +infinity
    Trunc,    // Toward zero
}

fn pow10(exp: u8) -> i128 {
    let r: i128 = 1;
    let i: u8 = 0;
    while (i < exp) {
        r = r * 10;
        i += 1;
    }
    return r;
}

fn absI128(v: i128) -> i128 {
    if (v < 0) {
        return -v;
    }
    return v;
}

fn divRoundI128(num: i128, den: i128, mode: RoundingMode) -> i128 {
    // Assumes den > 0
    const neg = num < 0;
    const anum = absI128(num);
    const q = @divTrunc(anum, den);
    const rem = @rem(anum, den);
    if (rem == 0) {
        if (neg) {
            return -q;
        }
        return q;
    }

    let inc = false;
    const twice_rem = rem * 2;
    switch (mode) {
        RoundingMode.Trunc => {
            inc = false;
        },
        RoundingMode.Floor => {
            inc = neg;
        },
        RoundingMode.Ceil => {
            inc = !neg;
        },
        RoundingMode.HalfUp => {
            inc = twice_rem >= den;
        },
        RoundingMode.HalfDown => {
            inc = twice_rem > den;
        },
        RoundingMode.HalfEven => {
            if (twice_rem > den) {
                inc = true;
            } else if (twice_rem == den) {
                inc = @rem(q, 2) != 0;
            } else {
                inc = false;
            }
        },
    }

    let res = q;
    if (inc) {
        res = q + 1;
    }
    if (neg) {
        return -res;
    }
    return res;
}

pub struct Decimal {
    pub mantissa: i128,
    pub scale: u8,

    pub static fn zero() -> Decimal {
        return Decimal{ mantissa: 0, scale: 0 };
    }

    pub static fn one() -> Decimal {
        return Decimal{ mantissa: 1, scale: 0 };
    }

    pub static fn fromInt(v: i64) -> Decimal {
        return Decimal{ mantissa: v as i128, scale: 0 };
    }

    pub static fn fromParts(mantissa: i128, scale: u8) -> Decimal {
        let sc = scale;
        let m = mantissa;
        if (sc > MAX_SCALE) {
            const diff = sc - MAX_SCALE;
            m = divRoundI128(m, pow10(diff), RoundingMode.HalfEven);
            sc = MAX_SCALE;
        }
        return Decimal{ mantissa: m, scale: sc };
    }

    pub static fn parse(raw: string) -> Decimal? {
        // Trim leading/trailing ASCII whitespace
        let start: usize = 0;
        let end = raw.len;
        while (start < end && (raw[start] == ' ' || raw[start] == '\t' || raw[start] == '\r' || raw[start] == '\n')) {
            start += 1;
        }
        while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t' || raw[end - 1] == '\r' || raw[end - 1] == '\n')) {
            end -= 1;
        }
        if (start >= end) {
            return null;
        }

        let i = start;
        let is_neg = false;
        if (raw[i] == '-') {
            is_neg = true;
            i += 1;
        } else if (raw[i] == '+') {
            i += 1;
        }
        if (i >= end) {
            return null;
        }

        let mant: i128 = 0;
        let digits: usize = 0;
        let frac_digits: i32 = 0;
        let seen_dot = false;
        let exp_val: i32 = 0;

        while (i < end) {
            const c = raw[i];
            if (c >= '0' && c <= '9') {
                const d: i128 = (c - '0') as i128;
                if (digits < 36) {
                    mant = mant * 10 + d;
                    digits += 1;
                    if (seen_dot) {
                        frac_digits += 1;
                    }
                } else if (!seen_dot) {
                    // Too many integer digits for 128-bit decimal
                    return null;
                }
                i += 1;
            } else if (c == '_' && digits > 0) {
                i += 1;
            } else if (c == '.' && !seen_dot) {
                seen_dot = true;
                i += 1;
            } else if (c == 'e' || c == 'E') {
                i += 1;
                if (i >= end) {
                    return null;
                }
                let exp_neg = false;
                if (raw[i] == '-') {
                    exp_neg = true;
                    i += 1;
                } else if (raw[i] == '+') {
                    i += 1;
                }
                if (i >= end) {
                    return null;
                }
                let exp_digits: usize = 0;
                while (i < end) {
                    const ec = raw[i];
                    if (ec < '0' || ec > '9') {
                        return null;
                    }
                    exp_val = exp_val * 10 + ((ec - '0') as i32);
                    if (exp_val > 100) {
                        return null;
                    }
                    exp_digits += 1;
                    i += 1;
                }
                if (exp_digits == 0) {
                    return null;
                }
                if (exp_neg) {
                    exp_val = -exp_val;
                }
                break;
            } else {
                return null;
            }
        }

        if (digits == 0) {
            return null;
        }

        let total_scale: i32 = frac_digits - exp_val;
        if (total_scale < 0) {
            const shift = (-total_scale) as u8;
            if (shift > 28) {
                return null;
            }
            mant = mant * pow10(shift);
            total_scale = 0;
        } else if (total_scale > (MAX_SCALE as i32)) {
            const drop = (total_scale - (MAX_SCALE as i32)) as u8;
            if (drop > 28) {
                mant = 0;
            } else {
                mant = divRoundI128(mant, pow10(drop), RoundingMode.HalfEven);
            }
            total_scale = MAX_SCALE as i32;
        }

        if (is_neg && mant != 0) {
            mant = -mant;
        }
        return Decimal{ mantissa: mant, scale: total_scale as u8 };
    }

    pub fn isZero() -> bool {
        return _.mantissa == 0;
    }

    pub fn isNegative() -> bool {
        return _.mantissa < 0;
    }

    pub fn isPositive() -> bool {
        return _.mantissa > 0;
    }

    pub fn neg() -> Decimal {
        return Decimal{ mantissa: -_.mantissa, scale: _.scale };
    }

    pub fn abs() -> Decimal {
        return Decimal{ mantissa: absI128(_.mantissa), scale: _.scale };
    }

    pub fn normalize() -> Decimal {
        if (_.mantissa == 0) {
            return Decimal{ mantissa: 0, scale: 0 };
        }
        let m = _.mantissa;
        let sc = _.scale;
        while (sc > 0 && @rem(m, 10) == 0) {
            m = @divTrunc(m, 10);
            sc -= 1;
        }
        return Decimal{ mantissa: m, scale: sc };
    }

    pub fn round(dp: u8, mode: RoundingMode) -> Decimal {
        if (dp >= _.scale) {
            return Decimal{ mantissa: _.mantissa, scale: _.scale };
        }
        const diff = _.scale - dp;
        const m = divRoundI128(_.mantissa, pow10(diff), mode);
        return Decimal{ mantissa: m, scale: dp };
    }

    pub fn scaleTo(new_scale: u8, mode: RoundingMode) -> Decimal {
        let target = new_scale;
        if (target > MAX_SCALE) {
            target = MAX_SCALE;
        }
        if (target == _.scale) {
            return Decimal{ mantissa: _.mantissa, scale: _.scale };
        }
        if (target > _.scale) {
            const diff = target - _.scale;
            return Decimal{ mantissa: _.mantissa * pow10(diff), scale: target };
        }
        return _.round(target, mode);
    }

    pub fn cmp(other: Decimal) -> i8 {
        let a = _.mantissa;
        let b = other.mantissa;
        if (_.scale < other.scale) {
            a = a * pow10(other.scale - _.scale);
        } else if (_.scale > other.scale) {
            b = b * pow10(_.scale - other.scale);
        }
        if (a < b) {
            return -1;
        }
        if (a > b) {
            return 1;
        }
        return 0;
    }

    pub fn eq(other: Decimal) -> bool {
        return _.cmp(other) == 0;
    }

    pub fn lt(other: Decimal) -> bool {
        return _.cmp(other) < 0;
    }

    pub fn lte(other: Decimal) -> bool {
        return _.cmp(other) <= 0;
    }

    pub fn gt(other: Decimal) -> bool {
        return _.cmp(other) > 0;
    }

    pub fn gte(other: Decimal) -> bool {
        return _.cmp(other) >= 0;
    }

    pub fn min(other: Decimal) -> Decimal {
        if (_.lte(other)) {
            return Decimal{ mantissa: _.mantissa, scale: _.scale };
        }
        return other;
    }

    pub fn max(other: Decimal) -> Decimal {
        if (_.gte(other)) {
            return Decimal{ mantissa: _.mantissa, scale: _.scale };
        }
        return other;
    }

    pub fn add(other: Decimal) -> Decimal {
        if (_.scale == other.scale) {
            return Decimal{ mantissa: _.mantissa + other.mantissa, scale: _.scale };
        }
        if (_.scale < other.scale) {
            const m = _.mantissa * pow10(other.scale - _.scale) + other.mantissa;
            return Decimal{ mantissa: m, scale: other.scale };
        }
        const m = _.mantissa + other.mantissa * pow10(_.scale - other.scale);
        return Decimal{ mantissa: m, scale: _.scale };
    }

    pub fn sub(other: Decimal) -> Decimal {
        return _.add(other.neg());
    }

    pub fn mul(other: Decimal) -> Decimal {
        let m = _.mantissa * other.mantissa;
        let sc: u16 = (_.scale as u16) + (other.scale as u16);
        if (sc > (MAX_SCALE as u16)) {
            const drop = (sc - (MAX_SCALE as u16)) as u8;
            m = divRoundI128(m, pow10(drop), RoundingMode.HalfEven);
            sc = MAX_SCALE as u16;
        }
        return Decimal{ mantissa: m, scale: sc as u8 };
    }

    pub fn div(other: Decimal, dp: u8, mode: RoundingMode) -> Decimal? {
        if (other.mantissa == 0) {
            return null;
        }
        let target_dp = dp;
        if (target_dp > MAX_SCALE) {
            target_dp = MAX_SCALE;
        }
        // Result value = (a / 10^sa) / (b / 10^sb) * 10^target_dp
        //              = (a * 10^(target_dp + sb - sa)) / b
        let num = _.mantissa;
        let den = other.mantissa;
        if (den < 0) {
            num = -num;
            den = -den;
        }
        const shift: i32 = (target_dp as i32) + (other.scale as i32) - (_.scale as i32);
        if (shift > 0) {
            num = num * pow10(shift as u8);
        } else if (shift < 0) {
            den = den * pow10((-shift) as u8);
        }
        const q = divRoundI128(num, den, mode);
        return Decimal{ mantissa: q, scale: target_dp };
    }

    pub fn format(out: []u8) -> string {
        let pos: usize = 0;
        let val = _.mantissa;
        if (val < 0) {
            if (pos < out.len) {
                out[pos] = '-';
                pos += 1;
            }
            val = -val;
        }

        // Convert val into reversed ASCII digits
        let tmp: [48]u8 = undefined;
        let tlen: usize = 0;
        if (val == 0) {
            tmp[0] = '0';
            tlen = 1;
        } else {
            while (val > 0 && tlen < 48) {
                const digit: u8 = (@rem(val, 10) as u8) + '0';
                tmp[tlen] = digit;
                tlen += 1;
                val = @divTrunc(val, 10);
            }
        }

        const sc = _.scale as usize;
        if (sc == 0) {
            let k = tlen;
            while (k > 0 && pos < out.len) {
                k -= 1;
                out[pos] = tmp[k];
                pos += 1;
            }
            return out[0..pos];
        }

        if (tlen <= sc) {
            if (pos < out.len) {
                out[pos] = '0';
                pos += 1;
            }
            if (pos < out.len) {
                out[pos] = '.';
                pos += 1;
            }
            let z: usize = 0;
            while (z < sc - tlen && pos < out.len) {
                out[pos] = '0';
                pos += 1;
                z += 1;
            }
            let k = tlen;
            while (k > 0 && pos < out.len) {
                k -= 1;
                out[pos] = tmp[k];
                pos += 1;
            }
            return out[0..pos];
        }

        const int_digits = tlen - sc;
        let idx: usize = 0;
        while (idx < int_digits && pos < out.len) {
            out[pos] = tmp[tlen - 1 - idx];
            pos += 1;
            idx += 1;
        }
        if (pos < out.len) {
            out[pos] = '.';
            pos += 1;
        }
        while (idx < tlen && pos < out.len) {
            out[pos] = tmp[tlen - 1 - idx];
            pos += 1;
            idx += 1;
        }
        return out[0..pos];
    }

    pub fn toFixed(dp: u8, mode: RoundingMode, out: []u8) -> string {
        const scaled = _.scaleTo(dp, mode);
        return scaled.format(out);
    }
}

export {
    RoundingMode,
    Decimal,
};
