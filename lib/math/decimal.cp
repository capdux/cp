// Source: cp/lib/decimal/decimal.cp (128-bit Fixed-Point Decimal Arithmetic & Rounding)
// Pure CP 128-bit fixed-point decimal arithmetic engine (`0..28` decimal places)
// eliminating IEEE 754 floating-point drift for financial settlement and billing.

const std = @import("std");

/// Maximum supported fractional scale (28 decimal digits).
pub const MAX_SCALE: u8 = 28;

const I128_MAX_U128: u128 = ((1 as u128) << 127) - 1;
const I128_MIN_ABS_U128: u128 = (1 as u128) << 127;
const U64_MASK: u128 = ((1 as u128) << 64) - 1;

/// Error set for 128-bit decimal parsing, scaling, and arithmetic operations.
pub err DecimalError {
    InvalidFormat,
    Overflow,
    DivisionByZero,
    InvalidScale,
    OutOfMemory,
}

/// Rounding strategy applied by `Decimal.round`, `Decimal.div`, and `Money.mulFactor`.
pub enum RoundingMode {
    /// Round half away from zero (`2.5 -> 3`, `-2.5 -> -3`).
    HalfUp,
    /// Banker's rounding: round half to the nearest even digit (`2.5 -> 2`, `3.5 -> 4`).
    HalfEven,
    /// Round toward negative infinity.
    Floor,
    /// Round toward positive infinity.
    Ceil,
    /// Round toward zero (truncate fractional remainder).
    Trunc,
}

fn absU128(v: i128): u128 {
    if (v >= 0) return v as u128;
    const raw: u128 = @bitCast(v);
    return 0 -% raw;
}

fn u128ToI128(mag: u128, is_neg: bool): i128 ! DecimalError {
    if (mag == 0) return 0;
    if (!is_neg) {
        if (mag > I128_MAX_U128) return DecimalError.Overflow;
        return mag as i128;
    }
    if (mag > I128_MIN_ABS_U128) return DecimalError.Overflow;
    if (mag == I128_MIN_ABS_U128) return std.math.minInt(i128);
    return -(mag as i128);
}

fn pow10U128(exp: u8): u128 {
    let res: u128 = 1;
    let i: u8 = 0;
    while (i < exp) : (i += 1) {
        res *= 10;
    }
    return res;
}

struct U384DivRem {
    q: U384 = U384.zero();
    r: U384 = U384.zero();
}

struct U384 {
    limbs: [6]u64 = [0; 6];

    static fn zero(): U384 {
        return U384 { limbs: [0; 6] };
    }

    static fn fromU128(v: u128): U384 {
        let out = U384.zero();
        out.limbs[0] = (v & U64_MASK) as u64;
        out.limbs[1] = (v >> 64) as u64;
        return out;
    }

    fn isZero(): bool {
        for (i of 0..6) {
            if (_.limbs[i] != 0) return false;
        }
        return true;
    }

    fn fitsU128(): bool {
        return _.limbs[2] == 0 && _.limbs[3] == 0 && _.limbs[4] == 0 && _.limbs[5] == 0;
    }

    fn toU128(): u128 ! DecimalError {
        if (!_.fitsU128()) return DecimalError.Overflow;
        return (_.limbs[0] as u128) | ((_.limbs[1] as u128) << 64);
    }

    fn cmp(other: U384): i8 {
        let i: usize = 6;
        while (i > 0) {
            i -= 1;
            if (_.limbs[i] < other.limbs[i]) return -1;
            if (_.limbs[i] > other.limbs[i]) return 1;
        }
        return 0;
    }

    fn add(other: U384): U384 ! DecimalError {
        let out = U384.zero();
        let carry: u128 = 0;
        for (i of 0..6) {
            const sum: u128 = (_.limbs[i] as u128) + (other.limbs[i] as u128) + carry;
            out.limbs[i] = (sum & U64_MASK) as u64;
            carry = sum >> 64;
        }
        if (carry != 0) return DecimalError.Overflow;
        return out;
    }

    fn sub(other: U384): U384 {
        let out = U384.zero();
        let borrow: u128 = 0;
        const base: u128 = (1 as u128) << 64;
        for (i of 0..6) {
            const a: u128 = _.limbs[i] as u128;
            const b: u128 = (other.limbs[i] as u128) + borrow;
            if (a >= b) {
                out.limbs[i] = (a - b) as u64;
                borrow = 0;
            } else {
                out.limbs[i] = ((a + base) - b) as u64;
                borrow = 1;
            }
        }
        return out;
    }

    fn mul(other: U384): U384 ! DecimalError {
        let out = U384.zero();
        for (i of 0..6) {
            if (_.limbs[i] == 0) continue;
            let carry: u128 = 0;
            for (j of 0..6) {
                const prod: u128 = (_.limbs[i] as u128) * (other.limbs[j] as u128) + carry;
                const k = i + j;
                if (k >= 6) {
                    if (prod != 0) return DecimalError.Overflow;
                } else {
                    const total: u128 = (out.limbs[k] as u128) + (prod & U64_MASK);
                    out.limbs[k] = (total & U64_MASK) as u64;
                    carry = (prod >> 64) + (total >> 64);
                }
            }
            if (carry != 0) return DecimalError.Overflow;
        }
        return out;
    }

    fn mulPow10(exp: u16): U384 ! DecimalError {
        let cur = _;
        let rem = exp;
        while (rem > 0) {
            const step: u8 = if (rem > 38) 38 else (rem as u8);
            cur = cur.mul(U384.fromU128(pow10U128(step)))?;
            rem -= step as u16;
        }
        return cur;
    }

    fn getBit(bit: usize): bool {
        const limb_idx = bit >> 6;
        const bit_off: u6 = @truncate(bit);
        return ((_.limbs[limb_idx] >> bit_off) & 1) != 0;
    }

    fn setBit(*_, bit: usize) {
        const limb_idx = bit >> 6;
        const bit_off: u6 = @truncate(bit);
        _.limbs[limb_idx] |= (1 as u64) << bit_off;
    }

    fn shl1WithBit(low_bit: bool): U384 {
        let out = U384.zero();
        let carry: u64 = if (low_bit) 1 else 0;
        for (i of 0..6) {
            const next_carry = _.limbs[i] >> 63;
            out.limbs[i] = (_.limbs[i] << 1) | carry;
            carry = next_carry;
        }
        return out;
    }

    fn topBit(): usize? {
        let i: usize = 6;
        while (i > 0) {
            i -= 1;
            const limb = _.limbs[i];
            if (limb != 0) {
                const lz = @clz(limb) as usize;
                return i * 64 + (63 - lz);
            }
        }
        return null;
    }

    fn divRem(den: U384): U384DivRem {
        if (_.fitsU128() && den.fitsU128()) {
            const a: u128 = (_.limbs[0] as u128) | ((_.limbs[1] as u128) << 64);
            const b: u128 = (den.limbs[0] as u128) | ((den.limbs[1] as u128) << 64);
            return U384DivRem {
                q: U384.fromU128(a / b),
                r: U384.fromU128(a % b),
            };
        }

        const c = _.cmp(den);
        if (c < 0) {
            return U384DivRem { q: U384.zero(), r: _ };
        }
        if (c == 0) {
            return U384DivRem { q: U384.fromU128(1), r: U384.zero() };
        }

        const top = _.topBit() ?? return U384DivRem { q: U384.zero(), r: U384.zero() };
        let q = U384.zero();
        let r = U384.zero();
        let bit_idx = top + 1;
        while (bit_idx > 0) {
            bit_idx -= 1;
            r = r.shl1WithBit(_.getBit(bit_idx));
            if (r.cmp(den) >= 0) {
                r = r.sub(den);
                q.setBit(bit_idx);
            }
        }
        return U384DivRem { q: q, r: r };
    }
}

fn roundDivU384(num: U384, den: U384, is_neg: bool, mode: RoundingMode): i128 ! DecimalError {
    const qr = num.divRem(den);
    let q = qr.q;
    if (!qr.r.isZero()) {
        const den_minus_r = den.sub(qr.r);
        const cmp_half = qr.r.cmp(den_minus_r);
        let inc = false;
        switch (mode) {
            .Trunc => { inc = false; },
            .Floor => { inc = is_neg; },
            .Ceil => { inc = !is_neg; },
            .HalfUp => { inc = cmp_half >= 0; },
            .HalfEven => {
                if (cmp_half > 0) {
                    inc = true;
                } else if (cmp_half == 0) {
                    inc = (q.limbs[0] & 1) == 1;
                }
            },
        }
        if (inc) {
            q = q.add(U384.fromU128(1))?;
        }
    }
    const q_u128 = q.toU128()?;
    return u128ToI128(q_u128, is_neg);
}

fn addSignedDec(a: Decimal, b: Decimal, negate_b: bool): Decimal ! DecimalError {
    const b_eff_neg = if (negate_b) (b.mantissa > 0) else (b.mantissa < 0);
    const b_eff_pos = if (negate_b) (b.mantissa < 0) else (b.mantissa > 0);
    const b_is_neg = if (b.mantissa == 0) false else b_eff_neg;

    if (a.mantissa == 0 && b.mantissa == 0) {
        const s = if (a.scale >= b.scale) a.scale else b.scale;
        return Decimal { mantissa: 0, scale: s };
    }

    const target_scale: u8 = if (a.scale >= b.scale) a.scale else b.scale;
    const shift_a = (target_scale - a.scale) as u16;
    const shift_b = (target_scale - b.scale) as u16;

    const a_mag = U384.fromU128(absU128(a.mantissa)).mulPow10(shift_a)?;
    const b_mag = U384.fromU128(absU128(b.mantissa)).mulPow10(shift_b)?;

    const a_is_neg = a.mantissa < 0;
    const b_sign_neg = if (b.mantissa == 0) false else (b_is_neg && !b_eff_pos);

    let res_mag = U384.zero();
    let res_neg = false;

    if (a_is_neg == b_sign_neg) {
        res_mag = a_mag.add(b_mag)?;
        res_neg = a_is_neg;
    } else {
        const c = a_mag.cmp(b_mag);
        if (c == 0) {
            return Decimal { mantissa: 0, scale: target_scale };
        } else if (c > 0) {
            res_mag = a_mag.sub(b_mag);
            res_neg = a_is_neg;
        } else {
            res_mag = b_mag.sub(a_mag);
            res_neg = b_sign_neg;
        }
    }

    const res_u128 = res_mag.toU128()?;
    const res_i128 = u128ToI128(res_u128, res_neg)?;
    return Decimal { mantissa: res_i128, scale: target_scale };
}

/// 128-bit fixed-point decimal value (`value = mantissa * 10^(-scale)`, `scale` in `0..=28`).
pub struct Decimal {
    mantissa: i128 = 0;
    scale: u8 = 0;

    /// Returns `0`.
    static fn zero(): Decimal {
        return Decimal { mantissa: 0, scale: 0 };
    }

    /// Returns `1`.
    static fn one(): Decimal {
        return Decimal { mantissa: 1, scale: 0 };
    }

    /// Constructs a `Decimal` from a 64-bit signed integer (`scale = 0`).
    static fn fromInt(v: i64): Decimal {
        return Decimal { mantissa: v as i128, scale: 0 };
    }

    /// Constructs a `Decimal` from a 128-bit signed integer (`scale = 0`).
    static fn fromI128(v: i128): Decimal {
        return Decimal { mantissa: v, scale: 0 };
    }

    /// Constructs a `Decimal` from a raw `mantissa` and fractional `scale` (`0..=28`).
    static fn fromScaled(mantissa: i128, scale: u8): Decimal ! DecimalError {
        if (scale > MAX_SCALE) return DecimalError.InvalidScale;
        return Decimal { mantissa: mantissa, scale: scale };
    }

    /// Parses decimal strings including optional sign, fractional point, and scientific notation (`1.5e-4`).
    static fn parse(input: []const u8): Decimal ! DecimalError {
        if (input.len == 0) return DecimalError.InvalidFormat;

        let idx: usize = 0;
        let is_neg = false;
        if (input[idx] == '-') {
            is_neg = true;
            idx += 1;
        } else if (input[idx] == '+') {
            idx += 1;
        }
        if (idx >= input.len) return DecimalError.InvalidFormat;

        let mag: u128 = 0;
        let saw_digit = false;
        let saw_dot = false;
        let frac_digits: i32 = 0;
        let nonzero_frac_digits: i32 = 0;
        let trailing_frac_zeros: i32 = 0;
        let exp_val: i32 = 0;

        while (idx < input.len) {
            const ch = input[idx];
            if (ch >= '0' && ch <= '9') {
                saw_digit = true;
                const d = (ch - '0') as u128;
                if (mag != 0 || d != 0) {
                    if (mag > (I128_MIN_ABS_U128 / 10)) {
                        // Still reject if trailing zeros cannot save it; check overflow cleanly.
                        if (saw_dot && d == 0) {
                            trailing_frac_zeros += 1;
                            idx += 1;
                            continue;
                        }
                        return DecimalError.Overflow;
                    }
                    const next_mag = mag * 10 + d;
                    if (next_mag > I128_MIN_ABS_U128) return DecimalError.Overflow;
                    mag = next_mag;
                }
                if (saw_dot) {
                    frac_digits += 1;
                    if (d == 0) {
                        trailing_frac_zeros += 1;
                    } else {
                        trailing_frac_zeros = 0;
                        nonzero_frac_digits = frac_digits;
                    }
                }
                idx += 1;
            } else if (ch == '.') {
                if (saw_dot) return DecimalError.InvalidFormat;
                saw_dot = true;
                idx += 1;
            } else if (ch == 'e' || ch == 'E') {
                if (!saw_digit) return DecimalError.InvalidFormat;
                idx += 1;
                if (idx >= input.len) return DecimalError.InvalidFormat;

                let exp_neg = false;
                if (input[idx] == '-') {
                    exp_neg = true;
                    idx += 1;
                } else if (input[idx] == '+') {
                    idx += 1;
                }
                if (idx >= input.len) return DecimalError.InvalidFormat;

                let exp_abs: i32 = 0;
                let saw_exp_digit = false;
                while (idx < input.len) : (idx += 1) {
                    const ec = input[idx];
                    if (ec < '0' || ec > '9') return DecimalError.InvalidFormat;
                    saw_exp_digit = true;
                    if (exp_abs < 100000) {
                        exp_abs = exp_abs * 10 + ((ec - '0') as i32);
                    }
                }
                if (!saw_exp_digit) return DecimalError.InvalidFormat;
                exp_val = if (exp_neg) -exp_abs else exp_abs;
                break;
            } else {
                return DecimalError.InvalidFormat;
            }
        }

        if (!saw_digit) return DecimalError.InvalidFormat;

        // Effective digits in `mag` omitted trailing zeros if we skipped past u128 capacity;
        // adjust `frac_digits` by how many zeros were skipped when mag overflowed guard.
        const stored_frac = frac_digits - trailing_frac_zeros;
        let final_scale: i32 = frac_digits - exp_val;

        if (mag == 0) {
            if (final_scale < 0) final_scale = 0;
            if (final_scale > (MAX_SCALE as i32)) final_scale = MAX_SCALE as i32;
            return Decimal { mantissa: 0, scale: final_scale as u8 };
        }

        // `mag` currently holds `digits * 10^(trailing_frac_zeros_actually_multiplied)`.
        // If `trailing_frac_zeros` was only skipped on the overflow branch, trim `mag` cleanly:
        let active_frac = frac_digits;
        // Normalize excess trailing zeros when `mag` accumulated all digits up to `nonzero_frac_digits`:
        if (active_frac > stored_frac && mag % 10 == 0) {
            // Normal path already multiplied trailing zeros into `mag`.
        }

        // First handle negative final_scale (positive exponent shifting left):
        if (final_scale < 0) {
            const up: i32 = -final_scale;
            if (up > 38) return DecimalError.Overflow;
            const factor = pow10U128(up as u8);
            const limit = if (is_neg) (I128_MIN_ABS_U128 / factor) else (I128_MAX_U128 / factor);
            if (mag > limit) return DecimalError.Overflow;
            mag *= factor;
            final_scale = 0;
        } else if (final_scale > (MAX_SCALE as i32)) {
            // Strip exact trailing zeros from `mag` to see if the number fits in `<= MAX_SCALE`.
            while (final_scale > (MAX_SCALE as i32) && (mag % 10) == 0) {
                mag /= 10;
                final_scale -= 1;
            }
            if (final_scale > (MAX_SCALE as i32)) {
                return DecimalError.InvalidScale;
            }
        }

        // Ensure `nonzero_frac_digits` wasn't violated when final_scale was valid
        * = nonzero_frac_digits;
        const m_i128 = u128ToI128(mag, is_neg)?;
        return Decimal { mantissa: m_i128, scale: final_scale as u8 };
    }

    /// Returns `true` if the value is numerically zero.
    fn isZero(): bool {
        return _.mantissa == 0;
    }

    /// Returns `true` if the value is strictly less than zero.
    fn isNegative(): bool {
        return _.mantissa < 0;
    }

    /// Returns `true` if the value is strictly greater than zero.
    fn isPositive(): bool {
        return _.mantissa > 0;
    }

    /// Returns `-this`, returning `Overflow` if `mantissa == i128.min`.
    fn neg(): Decimal ! DecimalError {
        if (_.mantissa == std.math.minInt(i128)) return DecimalError.Overflow;
        const m = _.mantissa;
        return Decimal { mantissa: -m, scale: _.scale };
    }

    /// Returns `|this|`, returning `Overflow` if `mantissa == i128.min`.
    fn abs(): Decimal ! DecimalError {
        if (_.mantissa < 0) return _.neg();
        return _;
    }

    /// Removes trailing fractional zeros (`1.200 -> 1.2`, `0.00 -> 0`).
    fn normalize(): Decimal {
        if (_.mantissa == 0) {
            return Decimal { mantissa: 0, scale: 0 };
        }
        let m = _.mantissa;
        let s = _.scale;
        while (s > 0 && @rem(m, 10) == 0) {
            m = @divTrunc(m, 10);
            s -= 1;
        }
        return Decimal { mantissa: m, scale: s };
    }

    /// Rescales to `target_scale` by padding trailing zeros or rounding if `target_scale < scale`.
    fn rescale(target_scale: u8, mode: RoundingMode = .HalfEven): Decimal ! DecimalError {
        if (target_scale > MAX_SCALE) return DecimalError.InvalidScale;
        if (target_scale == _.scale) return _;
        if (target_scale < _.scale) return _.round(target_scale, mode);
        if (_.mantissa == 0) return Decimal { mantissa: 0, scale: target_scale };

        const diff = target_scale - _.scale;
        const factor = pow10U128(diff);
        const mag = absU128(_.mantissa);
        const is_neg = _.mantissa < 0;
        const limit = if (is_neg) (I128_MIN_ABS_U128 / factor) else (I128_MAX_U128 / factor);
        if (mag > limit) return DecimalError.Overflow;
        const scaled_mag = mag * factor;
        const m = u128ToI128(scaled_mag, is_neg)?;
        return Decimal { mantissa: m, scale: target_scale };
    }

    /// Compares two decimals mathematically (`-1` if `<`, `0` if `==`, `1` if `>`), ignoring trailing zeros.
    fn cmp(other: Decimal): i8 {
        if (_.mantissa == 0 && other.mantissa == 0) return 0;
        if (_.mantissa < 0 && other.mantissa >= 0) return -1;
        if (_.mantissa > 0 && other.mantissa <= 0) return 1;
        if (_.mantissa == 0 && other.mantissa > 0) return -1;
        if (_.mantissa == 0 && other.mantissa < 0) return 1;

        const a_mag = U384.fromU128(absU128(_.mantissa));
        const b_mag = U384.fromU128(absU128(other.mantissa));

        let c: i8 = 0;
        if (_.scale == other.scale) {
            c = a_mag.cmp(b_mag);
        } else if (_.scale < other.scale) {
            const diff = (other.scale - _.scale) as u16;
            const a_scaled = a_mag.mulPow10(diff) catch unreachable;
            c = a_scaled.cmp(b_mag);
        } else {
            const diff = (_.scale - other.scale) as u16;
            const b_scaled = b_mag.mulPow10(diff) catch unreachable;
            c = a_mag.cmp(b_scaled);
        }

        if (_.mantissa < 0) return -c;
        return c;
    }

    /// Normalized equality (`1.20.eql(1.2) == true`).
    fn eql(other: Decimal): bool {
        return _.cmp(other) == 0;
    }

    /// Exact fixed-point addition.
    fn add(other: Decimal): Decimal ! DecimalError {
        if (_.mantissa == 0) return other;
        if (other.mantissa == 0) return _;
        return addSignedDec(_, other, false) catch |e| {
            if (e == DecimalError.Overflow) {
                return addSignedDec(_.normalize(), other.normalize(), false);
            }
            return e;
        };
    }

    /// Exact fixed-point subtraction.
    fn sub(other: Decimal): Decimal ! DecimalError {
        if (other.mantissa == 0) return _;
        return addSignedDec(_, other, true) catch |e| {
            if (e == DecimalError.Overflow) {
                return addSignedDec(_.normalize(), other.normalize(), true);
            }
            return e;
        };
    }

    /// Exact fixed-point multiplication; rounds to `MAX_SCALE` (`HalfEven`) only if combined scale exceeds 28.
    fn mul(other: Decimal): Decimal ! DecimalError {
        const is_neg = (_.mantissa < 0) != (other.mantissa < 0);
        let total_scale: u16 = (_.scale as u16) + (other.scale as u16);

        if (_.mantissa == 0 || other.mantissa == 0) {
            const s: u8 = if (total_scale <= (MAX_SCALE as u16)) (total_scale as u8) else MAX_SCALE;
            return Decimal { mantissa: 0, scale: s };
        }

        const a_mag = U384.fromU128(absU128(_.mantissa));
        const b_mag = U384.fromU128(absU128(other.mantissa));
        let prod = a_mag.mul(b_mag)?;

        if (total_scale > (MAX_SCALE as u16)) {
            const drop = total_scale - (MAX_SCALE as u16);
            const den = U384.fromU128(1).mulPow10(drop)?;
            const rounded = roundDivU384(prod, den, is_neg, .HalfEven)?;
            return Decimal { mantissa: rounded, scale: MAX_SCALE };
        }

        // If `total_scale <= MAX_SCALE` and `prod` exceeds i128 only due to trailing zeros, trim exact zeros.
        const ten = U384.fromU128(10);
        while (total_scale > 0) {
            const fits_u128 = prod.fitsU128();
            if (fits_u128) {
                const u = prod.toU128() catch unreachable;
                const limit = if (is_neg) I128_MIN_ABS_U128 else I128_MAX_U128;
                if (u <= limit) break;
            }
            const qr = prod.divRem(ten);
            if (!qr.r.isZero()) return DecimalError.Overflow;
            prod = qr.q;
            total_scale -= 1;
        }

        const prod_u128 = prod.toU128()?;
        const m = u128ToI128(prod_u128, is_neg)?;
        return Decimal { mantissa: m, scale: total_scale as u8 };
    }

    /// Fixed-point division rounded to `target_scale` (`0..=28`) with explicit `RoundingMode`.
    fn div(other: Decimal, target_scale: u8, mode: RoundingMode): Decimal ! DecimalError {
        if (other.mantissa == 0) return DecimalError.DivisionByZero;
        if (target_scale > MAX_SCALE) return DecimalError.InvalidScale;
        if (_.mantissa == 0) return Decimal { mantissa: 0, scale: target_scale };

        const is_neg = (_.mantissa < 0) != (other.mantissa < 0);
        let num = U384.fromU128(absU128(_.mantissa));
        let den = U384.fromU128(absU128(other.mantissa));

        const exp: i16 = (target_scale as i16) + (other.scale as i16) - (_.scale as i16);
        if (exp >= 0) {
            num = num.mulPow10(exp as u16)?;
        } else {
            den = den.mulPow10((-exp) as u16)?;
        }

        const m = roundDivU384(num, den, is_neg, mode)?;
        return Decimal { mantissa: m, scale: target_scale };
    }

    /// Rounds this decimal to `target_scale` fractional digits using `mode`.
    fn round(target_scale: u8, mode: RoundingMode): Decimal ! DecimalError {
        if (target_scale > MAX_SCALE) return DecimalError.InvalidScale;
        if (target_scale >= _.scale) return _;
        if (_.mantissa == 0) return Decimal { mantissa: 0, scale: target_scale };

        const drop = (_.scale - target_scale) as u16;
        const is_neg = _.mantissa < 0;
        const num = U384.fromU128(absU128(_.mantissa));
        const den = U384.fromU128(1).mulPow10(drop)?;
        const m = roundDivU384(num, den, is_neg, mode)?;
        return Decimal { mantissa: m, scale: target_scale };
    }

    /// Formats the decimal into a caller-supplied stack buffer and returns the populated slice.
    fn formatBuf(buf: []u8): ([]const u8) ! DecimalError {
        let rev: [64]u8 = undefined;
        let dlen: usize = 0;

        if (_.mantissa == 0) {
            rev[0] = '0';
            dlen = 1;
        } else {
            let v = absU128(_.mantissa);
            while (v > 0) {
                rev[dlen] = ('0' as u8) + ((v % 10) as u8);
                v /= 10;
                dlen += 1;
            }
        }

        let pos: usize = 0;
        if (_.mantissa < 0) {
            if (pos >= buf.len) return DecimalError.OutOfMemory;
            buf[pos] = '-';
            pos += 1;
        }

        const s = _.scale as usize;
        if (s == 0) {
            if (pos + dlen > buf.len) return DecimalError.OutOfMemory;
            let k = dlen;
            while (k > 0) {
                k -= 1;
                buf[pos] = rev[k];
                pos += 1;
            }
            return buf[0..pos];
        }

        if (dlen <= s) {
            const need = 2 + (s - dlen) + dlen;
            if (pos + need > buf.len) return DecimalError.OutOfMemory;
            buf[pos] = '0';
            buf[pos + 1] = '.';
            pos += 2;
            const zeros = s - dlen;
            for (z of 0..zeros) {
                * = z;
                buf[pos] = '0';
                pos += 1;
            }
            let k = dlen;
            while (k > 0) {
                k -= 1;
                buf[pos] = rev[k];
                pos += 1;
            }
            return buf[0..pos];
        }

        const int_len = dlen - s;
        if (pos + dlen + 1 > buf.len) return DecimalError.OutOfMemory;
        let k = dlen;
        for (idx of 0..int_len) {
            * = idx;
            k -= 1;
            buf[pos] = rev[k];
            pos += 1;
        }
        buf[pos] = '.';
        pos += 1;
        while (k > 0) {
            k -= 1;
            buf[pos] = rev[k];
            pos += 1;
        }
        return buf[0..pos];
    }

    /// Formats the decimal as an owned UTF-8 string allocated from `alloc`.
    fn format(alloc: std.mem.Allocator): ([]u8) ! DecimalError {
        let stack_buf: [80]u8 = undefined;
        const rendered = _.formatBuf(&stack_buf)?;
        return alloc.dupe(u8, rendered) catch return DecimalError.OutOfMemory;
    }
}
