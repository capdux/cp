// Reference Only: Functional overlap with lib/math/decimal.cp | Kept for comparison/analysis
// Source: native-cp-stdlib (Exact Fixed-Point Financial Decimal)
const std = @import("std");

pub const MAX_SCALE: u8 = 18;

pub const DecimalError = error{
    InvalidFormat,
    ScaleOutOfRange,
    Overflow,
    DivisionByZero,
};

pub const RoundingMode = enum {
    HalfUp,
    HalfEven,
    Floor,
    Ceil,
};

fn absU128(v: i128): u128 {
    if (v >= 0) {
        return @intCast(v);
    }
    if (v == std.math.minInt(i128)) {
        return @as(u128, 1) << 127;
    }
    return @intCast(-v);
}

fn checkedAddI128(a: i128, b: i128): DecimalError!i128 {
    if (b > 0 and a > std.math.maxInt(i128) - b) {
        return DecimalError.Overflow;
    }
    if (b < 0 and a < std.math.minInt(i128) - b) {
        return DecimalError.Overflow;
    }
    return a + b;
}

fn checkedSubI128(a: i128, b: i128): DecimalError!i128 {
    if (b < 0 and a > std.math.maxInt(i128) + b) {
        return DecimalError.Overflow;
    }
    if (b > 0 and a < std.math.minInt(i128) + b) {
        return DecimalError.Overflow;
    }
    return a - b;
}

fn checkedMulI128(a: i128, b: i128): DecimalError!i128 {
    if (a == 0 or b == 0) {
        return 0;
    }
    const is_neg = (a < 0) != (b < 0);
    const ua = absU128(a);
    const ub = absU128(b);
    const limit: u128 = if (is_neg) (@as(u128, 1) << 127) else ((@as(u128, 1) << 127) - 1);
    if (ua > limit / ub) {
        return DecimalError.Overflow;
    }
    const prod = ua * ub;
    if (is_neg) {
        if (prod == (@as(u128, 1) << 127)) {
            return std.math.minInt(i128);
        }
        return -@as(i128, @intCast(prod));
    }
    return @as(i128, @intCast(prod));
}

fn pow10I128(exp: u8): DecimalError!i128 {
    if (exp > 38) {
        return DecimalError.Overflow;
    }
    let res: i128 = 1;
    let i: u8 = 0;
    while (i < exp) {
        if (res > @divTrunc(std.math.maxInt(i128), 10)) {
            return DecimalError.Overflow;
        }
        res = res * 10;
        i += 1;
    }
    return res;
}

fn pow10U256(exp: u8): u256 {
    let res: u256 = 1;
    let i: u8 = 0;
    while (i < exp) {
        res = res * 10;
        i += 1;
    }
    return res;
}

fn pow10I256(exp: u8): i256 {
    let res: i256 = 1;
    let i: u8 = 0;
    while (i < exp) {
        res = res * 10;
        i += 1;
    }
    return res;
}

fn applyRoundingU256(is_neg: bool, q_in: u256, rem: u256, den: u256, mode: RoundingMode): DecimalError!i128 {
    let q = q_in;
    if (rem > 0) {
        let inc = false;
        const twice_rem = rem * 2;
        if (mode == RoundingMode.HalfUp) {
            if (twice_rem >= den) {
                inc = true;
            }
        } else if (mode == RoundingMode.HalfEven) {
            if (twice_rem > den) {
                inc = true;
            } else if (twice_rem == den and (q % 2) == 1) {
                inc = true;
            }
        } else if (mode == RoundingMode.Floor) {
            if (is_neg) {
                inc = true;
            }
        } else if (mode == RoundingMode.Ceil) {
            if (!is_neg) {
                inc = true;
            }
        }
        if (inc) {
            q += 1;
        }
    }

    if (q == 0) {
        return 0;
    }

    const max_pos: u256 = (@as(u256, 1) << 127) - 1;
    const max_neg: u256 = @as(u256, 1) << 127;

    if (is_neg) {
        if (q > max_neg) {
            return DecimalError.Overflow;
        }
        if (q == max_neg) {
            return std.math.minInt(i128);
        }
        return -@as(i128, @intCast(q));
    }

    if (q > max_pos) {
        return DecimalError.Overflow;
    }
    return @as(i128, @intCast(q));
}

pub struct Decimal {
    mantissa: i128,
    scale: u8,

    pub static fn fromInt(v: i64): Decimal {
        return Decimal{
            .mantissa = @as(i128, v),
            .scale = 0,
        };
    }

    pub static fn fromParts(mantissa: i128, scale: u8): DecimalError!Decimal {
        if (scale > MAX_SCALE) {
            return DecimalError.ScaleOutOfRange;
        }
        return Decimal{
            .mantissa = mantissa,
            .scale = scale,
        };
    }

    pub static fn parse(s: []const u8): DecimalError!Decimal {
        if (s.len == 0) {
            return DecimalError.InvalidFormat;
        }

        let idx: usize = 0;
        let is_neg = false;
        if (s[0] == '-') {
            is_neg = true;
            idx = 1;
        } else if (s[0] == '+') {
            idx = 1;
        }

        if (idx >= s.len) {
            return DecimalError.InvalidFormat;
        }

        let mag: u128 = 0;
        let seen_dot = false;
        let seen_digit = false;
        let frac_count: u16 = 0;
        const limit: u128 = if (is_neg) (@as(u128, 1) << 127) else ((@as(u128, 1) << 127) - 1);

        while (idx < s.len) {
            const ch = s[idx];
            if (ch == '.') {
                if (seen_dot) {
                    return DecimalError.InvalidFormat;
                }
                seen_dot = true;
            } else if (ch >= '0' and ch <= '9') {
                seen_digit = true;
                const digit: u128 = @intCast(ch - '0');
                if (mag > (limit - digit) / 10) {
                    return DecimalError.Overflow;
                }
                mag = mag * 10 + digit;
                if (seen_dot) {
                    frac_count += 1;
                    if (frac_count > @as(u16, MAX_SCALE)) {
                        return DecimalError.ScaleOutOfRange;
                    }
                }
            } else {
                return DecimalError.InvalidFormat;
            }
            idx += 1;
        }

        if (!seen_digit) {
            return DecimalError.InvalidFormat;
        }

        let final_mant: i128 = 0;
        if (mag == 0) {
            final_mant = 0;
        } else if (is_neg) {
            if (mag == (@as(u128, 1) << 127)) {
                final_mant = std.math.minInt(i128);
            } else {
                final_mant = -@as(i128, @intCast(mag));
            }
        } else {
            final_mant = @as(i128, @intCast(mag));
        }

        return Decimal{
            .mantissa = final_mant,
            .scale = @intCast(frac_count),
        };
    }

    pub fn formatAlloc(_: Decimal, alloc: std.mem.Allocator): ![]u8 {
        let out = std.ArrayList(u8).empty;
        errdefer out.deinit(alloc);

        if (_.mantissa == 0) {
            out.append(alloc, '0')?;
            if (_.scale > 0) {
                out.append(alloc, '.')?;
                let z: u8 = 0;
                while (z < _.scale) {
                    out.append(alloc, '0')?;
                    z += 1;
                }
            }
            return out.toOwnedSlice(alloc);
        }

        const is_neg = _.mantissa < 0;
        if (is_neg) {
            out.append(alloc, '-')?;
        }

        let mag = absU128(_.mantissa);
        let rev_buf: [48]u8 = undefined;
        let rev_len: usize = 0;
        while (mag > 0) {
            const d: u8 = @intCast(mag % 10);
            rev_buf[rev_len] = d + '0';
            rev_len += 1;
            mag = mag / 10;
        }

        const s_len: usize = @intCast(_.scale);
        if (s_len == 0) {
            let i: usize = 0;
            while (i < rev_len) {
                const ridx = rev_len - 1 - i;
                out.append(alloc, rev_buf[ridx])?;
                i += 1;
            }
        } else if (rev_len <= s_len) {
            out.append(alloc, '0')?;
            out.append(alloc, '.')?;
            let z: usize = 0;
            const zeros_needed = s_len - rev_len;
            while (z < zeros_needed) {
                out.append(alloc, '0')?;
                z += 1;
            }
            let i: usize = 0;
            while (i < rev_len) {
                const ridx = rev_len - 1 - i;
                out.append(alloc, rev_buf[ridx])?;
                i += 1;
            }
        } else {
            const int_len = rev_len - s_len;
            let i: usize = 0;
            while (i < int_len) {
                const ridx = rev_len - 1 - i;
                out.append(alloc, rev_buf[ridx])?;
                i += 1;
            }
            out.append(alloc, '.')?;
            while (i < rev_len) {
                const ridx = rev_len - 1 - i;
                out.append(alloc, rev_buf[ridx])?;
                i += 1;
            }
        }

        return out.toOwnedSlice(alloc);
    }

    pub fn add(_: Decimal, other: Decimal): DecimalError!Decimal {
        const target_scale: u8 = if (_.scale >= other.scale) _.scale else other.scale;
        let a_mant = _.mantissa;
        let b_mant = other.mantissa;

        if (_.scale < target_scale) {
            const factor = pow10I128(target_scale - _.scale)?;
            a_mant = checkedMulI128(a_mant, factor)?;
        }
        if (other.scale < target_scale) {
            const factor = pow10I128(target_scale - other.scale)?;
            b_mant = checkedMulI128(b_mant, factor)?;
        }

        const sum = checkedAddI128(a_mant, b_mant)?;
        return Decimal{
            .mantissa = sum,
            .scale = target_scale,
        };
    }

    pub fn sub(_: Decimal, other: Decimal): DecimalError!Decimal {
        const target_scale: u8 = if (_.scale >= other.scale) _.scale else other.scale;
        let a_mant = _.mantissa;
        let b_mant = other.mantissa;

        if (_.scale < target_scale) {
            const factor = pow10I128(target_scale - _.scale)?;
            a_mant = checkedMulI128(a_mant, factor)?;
        }
        if (other.scale < target_scale) {
            const factor = pow10I128(target_scale - other.scale)?;
            b_mant = checkedMulI128(b_mant, factor)?;
        }

        const diff = checkedSubI128(a_mant, b_mant)?;
        return Decimal{
            .mantissa = diff,
            .scale = target_scale,
        };
    }

    pub fn mul(_: Decimal, other: Decimal): DecimalError!Decimal {
        const total_scale: u16 = @as(u16, _.scale) + @as(u16, other.scale);
        if (total_scale <= @as(u16, MAX_SCALE)) {
            const prod = checkedMulI128(_.mantissa, other.mantissa)?;
            return Decimal{
                .mantissa = prod,
                .scale = @intCast(total_scale),
            };
        }

        const drop_scale: u8 = @intCast(total_scale - @as(u16, MAX_SCALE));
        const is_neg = (_.mantissa < 0) != (other.mantissa < 0);
        const prod_u256 = @as(u256, absU128(_.mantissa)) * @as(u256, absU128(other.mantissa));
        const den = pow10U256(drop_scale);
        const q = prod_u256 / den;
        const rem = prod_u256 % den;
        const rounded = applyRoundingU256(is_neg, q, rem, den, RoundingMode.HalfEven)?;
        return Decimal{
            .mantissa = rounded,
            .scale = MAX_SCALE,
        };
    }

    pub fn divRound(_: Decimal, other: Decimal, target_scale: u8, mode: RoundingMode): DecimalError!Decimal {
        if (other.mantissa == 0) {
            return DecimalError.DivisionByZero;
        }
        if (target_scale > MAX_SCALE) {
            return DecimalError.ScaleOutOfRange;
        }
        if (_.mantissa == 0) {
            return Decimal{
                .mantissa = 0,
                .scale = target_scale,
            };
        }

        const is_neg = (_.mantissa < 0) != (other.mantissa < 0);
        const ua: u256 = @as(u256, absU128(_.mantissa));
        const ub: u256 = @as(u256, absU128(other.mantissa));

        const exp_diff: i16 = @as(i16, target_scale) + @as(i16, other.scale) - @as(i16, _.scale);
        let num: u256 = ua;
        let den: u256 = ub;
        if (exp_diff >= 0) {
            const shift: u8 = @intCast(exp_diff);
            num = ua * pow10U256(shift);
        } else {
            const shift: u8 = @intCast(-exp_diff);
            den = ub * pow10U256(shift);
        }

        const q = num / den;
        const rem = num % den;
        const rounded = applyRoundingU256(is_neg, q, rem, den, mode)?;
        return Decimal{
            .mantissa = rounded,
            .scale = target_scale,
        };
    }

    pub fn rescale(_: Decimal, target_scale: u8, mode: RoundingMode): DecimalError!Decimal {
        if (target_scale > MAX_SCALE) {
            return DecimalError.ScaleOutOfRange;
        }
        if (target_scale == _.scale) {
            return _;
        }
        if (target_scale > _.scale) {
            const factor = pow10I128(target_scale - _.scale)?;
            const scaled = checkedMulI128(_.mantissa, factor)?;
            return Decimal{
                .mantissa = scaled,
                .scale = target_scale,
            };
        }

        const drop_scale = _.scale - target_scale;
        const is_neg = _.mantissa < 0;
        const ua: u256 = @as(u256, absU128(_.mantissa));
        const den = pow10U256(drop_scale);
        const q = ua / den;
        const rem = ua % den;
        const rounded = applyRoundingU256(is_neg, q, rem, den, mode)?;
        return Decimal{
            .mantissa = rounded,
            .scale = target_scale,
        };
    }

    pub fn compare(_: Decimal, other: Decimal): i8 {
        if (_.mantissa == 0 and other.mantissa == 0) {
            return 0;
        }
        if (_.mantissa < 0 and other.mantissa >= 0) {
            return -1;
        }
        if (_.mantissa >= 0 and other.mantissa < 0) {
            return 1;
        }

        const max_s: u8 = if (_.scale >= other.scale) _.scale else other.scale;
        let a_val: i256 = @as(i256, _.mantissa);
        let b_val: i256 = @as(i256, other.mantissa);
        if (_.scale < max_s) {
            a_val = a_val * pow10I256(max_s - _.scale);
        }
        if (other.scale < max_s) {
            b_val = b_val * pow10I256(max_s - other.scale);
        }

        if (a_val < b_val) {
            return -1;
        }
        if (a_val > b_val) {
            return 1;
        }
        return 0;
    }

    pub fn equals(_: Decimal, other: Decimal): bool {
        return _.compare(other) == 0;
    }

    pub fn neg(_: Decimal): Decimal {
        return Decimal{
            .mantissa = -_.mantissa,
            .scale = _.scale,
        };
    }

    pub fn abs(_: Decimal): Decimal {
        if (_.mantissa < 0) {
            return Decimal{
                .mantissa = -_.mantissa,
                .scale = _.scale,
            };
        }
        return _;
    }
}
