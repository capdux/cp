// Source: cp/lib/fx/money.cp (Fixed-Point Multi-Currency Money Arithmetic & ISO 4217 Registry)
// Pure CP zero-float fixed-point multi-currency Money arithmetic, ISO 4217 minor-unit
// registry, 256-bit intermediate rounding, and Hamilton largest-remainder allocation.

const std = @import("std");

const I128_MAX_U128: u128 = ((1 as u128) << 127) - 1;
const I128_MIN_ABS_U128: u128 = (1 as u128) << 127;
const U64_MASK: u128 = ((1 as u128) << 64) - 1;
const MAX_MINOR_UNITS: u8 = 28;
const MAX_CUSTOM_CURRENCIES: usize = 64;

/// Error set for multi-currency money operations, parsing, allocation, and FX conversion.
pub err FxError {
    CurrencyMismatch,
    UnknownCurrency,
    InvalidAmountFormat,
    Overflow,
    DivisionByZero,
    RateNotFound,
    InvalidRate,
    InvalidAllocationWeights,
    OutOfMemory,
}

/// Rounding modes for fixed-point scaling and cross-currency conversion.
pub enum RoundingMode {
    HalfUp,
    HalfEven,
    Floor,
    Ceil,
    Trunc,
}

let custom_count: usize = 0;
let custom_codes: [64][3]u8 = undefined;
let custom_units: [64]u8 = undefined;

fn eqlCode(a: [3]u8, b: [3]u8): bool {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

fn validateRawCode(code: []const u8): [3]u8 ! FxError {
    if (code.len != 3) return FxError.UnknownCurrency;
    let out: [3]u8 = undefined;
    for (i of 0..3) {
        const c = code[i];
        if (c < 'A' || c > 'Z') return FxError.UnknownCurrency;
        out[i] = c;
    }
    return out;
}

fn builtinMinorUnits(code: [3]u8): u8? {
    // 2 minor-unit major fiat currencies
    if (eqlCode(code, ['U', 'S', 'D'])) return 2;
    if (eqlCode(code, ['E', 'U', 'R'])) return 2;
    if (eqlCode(code, ['G', 'B', 'P'])) return 2;
    if (eqlCode(code, ['C', 'N', 'Y'])) return 2;
    if (eqlCode(code, ['C', 'H', 'F'])) return 2;
    if (eqlCode(code, ['H', 'K', 'D'])) return 2;
    if (eqlCode(code, ['S', 'G', 'D'])) return 2;
    if (eqlCode(code, ['A', 'U', 'D'])) return 2;
    if (eqlCode(code, ['C', 'A', 'D'])) return 2;
    if (eqlCode(code, ['N', 'Z', 'D'])) return 2;
    if (eqlCode(code, ['T', 'W', 'D'])) return 2;
    if (eqlCode(code, ['I', 'N', 'R'])) return 2;
    // 0 minor-unit currencies
    if (eqlCode(code, ['J', 'P', 'Y'])) return 0;
    if (eqlCode(code, ['K', 'R', 'W'])) return 0;
    // 3 minor-unit currencies
    if (eqlCode(code, ['K', 'W', 'D'])) return 3;
    if (eqlCode(code, ['B', 'H', 'D'])) return 3;
    if (eqlCode(code, ['O', 'M', 'R'])) return 3;
    // Crypto / high-precision currencies
    if (eqlCode(code, ['B', 'T', 'C'])) return 8;
    if (eqlCode(code, ['E', 'T', 'H'])) return 18;
    return null;
}

/// ISO-4217 currency descriptor and minor-unit registry.
pub struct Currency {
    code: [3]u8 = ['U', 'S', 'D'];
    minor_units: u8 = 2;

    /// Registers or updates a custom 3-letter uppercase currency code and its minor-unit count.
    pub static fn register(code: []const u8, minor_units: u8): void ! FxError {
        const c3 = validateRawCode(code)?;
        if (minor_units > MAX_MINOR_UNITS) return FxError.InvalidAmountFormat;

        let idx: usize = 0;
        while (idx < custom_count) {
            if (eqlCode(custom_codes[idx], c3)) {
                custom_units[idx] = minor_units;
                return;
            }
            idx += 1;
        }
        if (custom_count >= MAX_CUSTOM_CURRENCIES) return FxError.OutOfMemory;
        custom_codes[custom_count] = c3;
        custom_units[custom_count] = minor_units;
        custom_count += 1;
    }

    /// Queries the minor-unit decimal precision for a built-in or custom registered currency code.
    pub static fn minorUnits(code: []const u8): u8 ! FxError {
        const c3 = validateRawCode(code)?;
        let idx: usize = custom_count;
        while (idx > 0) {
            idx -= 1;
            if (eqlCode(custom_codes[idx], c3)) return custom_units[idx];
        }
        if (mu := builtinMinorUnits(c3)) {
            return mu;
        }
        return FxError.UnknownCurrency;
    }

    /// Resolves a `Currency` descriptor for a known currency code.
    pub static fn of(code: []const u8): Currency ! FxError {
        const c3 = validateRawCode(code)?;
        const mu = Currency.minorUnits(code)?;
        return Currency {
            code: c3,
            minor_units: mu,
        };
    }

    /// Validates and normalizes a known 3-letter currency code.
    pub static fn parseCode(code: []const u8): [3]u8 ! FxError {
        const c3 = validateRawCode(code)?;
        * = Currency.minorUnits(code)?;
        return c3;
    }
}

/// Computes `10^exp` as `u128`, returning `FxError.Overflow` if `exp > 38`.
pub fn pow10U128(exp: u8): u128 ! FxError {
    if (exp > 38) return FxError.Overflow;
    let res: u128 = 1;
    let i: u8 = 0;
    while (i < exp) {
        res *= 10;
        i += 1;
    }
    return res;
}

/// Returns the unsigned magnitude of an `i128` without overflowing on `minInt(i128)`.
pub fn absU128(v: i128): u128 {
    if (v >= 0) return v as u128;
    const raw: u128 = @bitCast(v);
    return 0 -% raw;
}

/// Converts an unsigned magnitude and sign back into `i128`, checking bounds.
pub fn u128ToI128(mag: u128, is_neg: bool): i128 ! FxError {
    if (mag == 0) return 0;
    if (!is_neg) {
        if (mag > I128_MAX_U128) return FxError.Overflow;
        return mag as i128;
    }
    if (mag > I128_MIN_ABS_U128) return FxError.Overflow;
    if (mag == I128_MIN_ABS_U128) return std.math.minInt(i128);
    return -(mag as i128);
}

struct DivRemU128 {
    q: u128 = 0;
    rem: u128 = 0;
}

// Exact 256-bit intermediate (a * b) / den and remainder.
fn mulDivRemU128(a: u128, b: u128, den: u128): DivRemU128 ! FxError {
    if (den == 0) return FxError.DivisionByZero;
    if (a == 0 || b == 0) return DivRemU128 { q: 0, rem: 0 };

    const a_lo: u128 = a & U64_MASK;
    const a_hi: u128 = a >> 64;
    const b_lo: u128 = b & U64_MASK;
    const b_hi: u128 = b >> 64;

    const p00 = a_lo * b_lo;
    const p01 = a_lo * b_hi;
    const p10 = a_hi * b_lo;
    const p11 = a_hi * b_hi;

    const mid1 = (p00 >> 64) + (p01 & U64_MASK) + (p10 & U64_MASK);
    const lo: u128 = (p00 & U64_MASK) | ((mid1 & U64_MASK) << 64);
    const hi: u128 = p11 + (p01 >> 64) + (p10 >> 64) + (mid1 >> 64);

    if (hi == 0) {
        return DivRemU128 {
            q: lo / den,
            rem: lo % den,
        };
    }

    if (hi >= den) return FxError.Overflow;

    let q: u128 = 0;
    let r: u128 = hi;
    let bit: usize = 128;
    while (bit > 0) {
        bit -= 1;
        const shift_amt: u7 = @truncate(bit);
        const in_bit: u128 = (lo >> shift_amt) & 1;

        const r_msb = (r >> 127) & 1;
        r = (r << 1) | in_bit;
        if (r_msb != 0 || r >= den) {
            r = r -% den;
            q |= (1 as u128) << shift_amt;
        }
    }

    return DivRemU128 { q: q, rem: r };
}

fn shouldRoundUpMag(q: u128, rem: u128, den: u128, is_neg: bool, mode: RoundingMode): bool {
    if (rem == 0) return false;
    switch (mode) {
        .Trunc => return false,
        .Floor => return is_neg,
        .Ceil => return !is_neg,
        .HalfUp => {
            // Half-away-from-zero: round up iff 2 * rem >= den without overflowing u128
            const half_ceil = (den + 1) >> 1;
            return rem >= half_ceil;
        },
        .HalfEven => {
            const half_floor = den >> 1;
            if ((den & 1) == 0) {
                if (rem > half_floor) return true;
                if (rem < half_floor) return false;
                return (q & 1) == 1;
            }
            return rem > half_floor;
        },
    }
}

/// Computes `round((val * num) / den)` with 256-bit intermediate precision.
pub fn mulDivRounded(val: i128, num: i128, den: i128, mode: RoundingMode): i128 ! FxError {
    if (den == 0) return FxError.DivisionByZero;
    if (val == 0 || num == 0) return 0;

    const is_neg = (val < 0) != (num < 0) != (den < 0);
    const mag_a = absU128(val);
    const mag_num = absU128(num);
    const mag_den = absU128(den);

    const qr = mulDivRemU128(mag_a, mag_num, mag_den)?;
    let q = qr.q;
    if (shouldRoundUpMag(q, qr.rem, mag_den, is_neg, mode)) {
        if (q == std.math.maxInt(u128)) return FxError.Overflow;
        q += 1;
    }
    return u128ToI128(q, is_neg);
}

/// Computes `round((val * num) / den)` for unsigned rational `num / den` with 256-bit precision.
pub fn mulDivUnsignedRational(val: i128, num: u128, den: u128, mode: RoundingMode): i128 ! FxError {
    if (den == 0) return FxError.DivisionByZero;
    if (val == 0 || num == 0) return 0;

    const is_neg = val < 0;
    const mag_a = absU128(val);
    const qr = mulDivRemU128(mag_a, num, den)?;
    let q = qr.q;
    if (shouldRoundUpMag(q, qr.rem, den, is_neg, mode)) {
        if (q == std.math.maxInt(u128)) return FxError.Overflow;
        q += 1;
    }
    return u128ToI128(q, is_neg);
}

struct ShareRem {
    idx: usize = 0;
    rem: u128 = 0;
}

fn shareRemLessThan(_ctx: u8, a: ShareRem, b: ShareRem): bool {
    * = _ctx;
    if (a.rem != b.rem) return a.rem > b.rem;
    return a.idx < b.idx;
}

/// Fixed-point monetary value storing integer minor units and an ISO-4217 currency code.
pub struct Money {
    amount_minor: i128 = 0;
    currency: [3]u8 = ['U', 'S', 'D'];

    /// Constructs a `Money` value from an exact minor-unit integer and currency code.
    pub static fn fromMinor(amount: i128, code: []const u8): Money ! FxError {
        const c3 = Currency.parseCode(code)?;
        return Money {
            amount_minor: amount,
            currency: c3,
        };
    }

    /// Parses a decimal string (e.g. `"12.34"`, `"-0.00000001"`) into exact minor units.
    pub static fn parse(decimal_str: []const u8, code: []const u8): Money ! FxError {
        const c3 = Currency.parseCode(code)?;
        const minor_units = Currency.minorUnits(code)?;
        if (decimal_str.len == 0) return FxError.InvalidAmountFormat;

        let idx: usize = 0;
        let is_neg = false;
        if (decimal_str[0] == '-') {
            is_neg = true;
            idx = 1;
        } else if (decimal_str[0] == '+') {
            idx = 1;
        }

        if (idx >= decimal_str.len) return FxError.InvalidAmountFormat;

        let mag: u128 = 0;
        let int_digits: usize = 0;
        let frac_digits: u8 = 0;
        let seen_dot = false;

        while (idx < decimal_str.len) {
            const ch = decimal_str[idx];
            if (ch == '.') {
                if (seen_dot || int_digits == 0) return FxError.InvalidAmountFormat;
                seen_dot = true;
                // Must have at least one digit after the decimal point
                if (idx + 1 >= decimal_str.len) return FxError.InvalidAmountFormat;
            } else if (ch >= '0' && ch <= '9') {
                const digit: u128 = (ch - '0') as u128;
                if (seen_dot) {
                    if (frac_digits >= minor_units) return FxError.InvalidAmountFormat;
                    frac_digits += 1;
                } else {
                    int_digits += 1;
                }

                if (mag > (std.math.maxInt(u128) - digit) / 10) {
                    return FxError.Overflow;
                }
                mag = mag * 10 + digit;
            } else {
                return FxError.InvalidAmountFormat;
            }
            idx += 1;
        }

        if (int_digits == 0) return FxError.InvalidAmountFormat;

        let pad: u8 = minor_units - frac_digits;
        while (pad > 0) {
            if (mag > std.math.maxInt(u128) / 10) return FxError.Overflow;
            mag *= 10;
            pad -= 1;
        }

        const signed_amt = u128ToI128(mag, is_neg)?;
        return Money {
            amount_minor: signed_amt,
            currency: c3,
        };
    }

    /// Formats the money value as `"<amount> <CCY>"` (e.g. `"12.34 USD"`, `"1000 JPY"`).
    pub fn format(allocator: std.mem.Allocator): ([]u8) ! FxError {
        const minor_units = Currency.minorUnits(&_.currency)?;
        const is_neg = _.amount_minor < 0;
        const mag = absU128(_.amount_minor);

        let digits_buf: [48]u8 = undefined;
        let digit_len: usize = 0;
        if (mag == 0) {
            digits_buf[0] = '0';
            digit_len = 1;
        } else {
            let tmp = mag;
            while (tmp > 0) {
                digits_buf[digit_len] = ('0' as u8) + ((tmp % 10) as u8);
                digit_len += 1;
                tmp /= 10;
            }
        }

        const mu_usize = minor_units as usize;
        let num_len: usize = 0;
        if (mu_usize == 0) {
            num_len = digit_len;
        } else if (digit_len <= mu_usize) {
            // "0." + mu_usize digits
            num_len = (2 as usize) + mu_usize;
        } else {
            // int_digits + "." + mu_usize digits
            num_len = digit_len + (1 as usize);
        }

        let sign_len: usize = 0;
        if (is_neg) sign_len = 1;
        // "<sign><num> <CCY>"
        const total_len = sign_len + num_len + (4 as usize);
        let out = allocator.alloc(u8, total_len) catch return FxError.OutOfMemory;

        let pos: usize = 0;
        if (is_neg) {
            out[pos] = '-';
            pos += 1;
        }

        if (mu_usize == 0) {
            let k = digit_len;
            while (k > 0) {
                k -= 1;
                out[pos] = digits_buf[k];
                pos += 1;
            }
        } else if (digit_len <= mu_usize) {
            out[pos] = '0';
            pos += 1;
            out[pos] = '.';
            pos += 1;
            let zeros = mu_usize - digit_len;
            while (zeros > 0) {
                out[pos] = '0';
                pos += 1;
                zeros -= 1;
            }
            let k = digit_len;
            while (k > 0) {
                k -= 1;
                out[pos] = digits_buf[k];
                pos += 1;
            }
        } else {
            const int_count = digit_len - mu_usize;
            let k = digit_len;
            let written: usize = 0;
            while (k > 0) {
                if (written == int_count) {
                    out[pos] = '.';
                    pos += 1;
                }
                k -= 1;
                out[pos] = digits_buf[k];
                pos += 1;
                written += 1;
            }
        }

        out[pos] = ' ';
        out[pos + 1] = _.currency[0];
        out[pos + 2] = _.currency[1];
        out[pos + 3] = _.currency[2];
        return out;
    }

    /// Returns `true` if both currency and minor-unit amount are equal.
    pub fn eql(other: Money): bool {
        return _.amount_minor == other.amount_minor && eqlCode(_.currency, other.currency);
    }

    /// Returns `true` if the monetary amount is zero.
    pub fn isZero(): bool {
        return _.amount_minor == 0;
    }

    /// Currency-safe addition; returns `FxError.CurrencyMismatch` if currencies differ.
    pub fn add(other: Money): Money ! FxError {
        if (!eqlCode(_.currency, other.currency)) return FxError.CurrencyMismatch;
        const res = @addWithOverflow(_.amount_minor, other.amount_minor);
        if (res[1] != 0) return FxError.Overflow;
        return Money {
            amount_minor: res[0],
            currency: _.currency,
        };
    }

    /// Currency-safe subtraction; returns `FxError.CurrencyMismatch` if currencies differ.
    pub fn sub(other: Money): Money ! FxError {
        if (!eqlCode(_.currency, other.currency)) return FxError.CurrencyMismatch;
        const res = @subWithOverflow(_.amount_minor, other.amount_minor);
        if (res[1] != 0) return FxError.Overflow;
        return Money {
            amount_minor: res[0],
            currency: _.currency,
        };
    }

    /// Scales the monetary amount by `num / den` using 256-bit intermediate precision and `rounding`.
    pub fn mulScale(num: i128, den: i128, rounding: RoundingMode): Money ! FxError {
        const scaled = mulDivRounded(_.amount_minor, num, den, rounding)?;
        return Money {
            amount_minor: scaled,
            currency: _.currency,
        };
    }

    /// Allocates the monetary amount across `weights` with zero minor-unit loss (Hamilton largest-remainder).
    pub fn allocate(allocator: std.mem.Allocator, weights: []const u64): ([]Money) ! FxError {
        if (weights.len == 0) return FxError.InvalidAllocationWeights;

        let total_weight: u128 = 0;
        for (w of weights) {
            total_weight += w as u128;
        }
        if (total_weight == 0) return FxError.InvalidAllocationWeights;

        const n = weights.len;
        const is_neg = _.amount_minor < 0;
        const mag = absU128(_.amount_minor);

        let base_mags = allocator.alloc(u128, n) catch return FxError.OutOfMemory;
        defer allocator.free(base_mags);

        let rems = allocator.alloc(ShareRem, n) catch return FxError.OutOfMemory;
        defer allocator.free(rems);

        let assigned: u128 = 0;
        for (i of 0..n) {
            const w = weights[i] as u128;
            const qr = mulDivRemU128(mag, w, total_weight)?;
            base_mags[i] = qr.q;
            assigned += qr.q;
            rems[i] = ShareRem {
                idx: i,
                rem: qr.rem,
            };
        }

        let leftover = mag - assigned;
        std.sort.block(ShareRem, rems, 0 as u8, shareRemLessThan);

        let k: usize = 0;
        while (leftover > 0 && k < n) {
            const target_idx = rems[k].idx;
            if (weights[target_idx] > 0) {
                base_mags[target_idx] += 1;
                leftover -= 1;
            }
            k += 1;
        }

        let out = allocator.alloc(Money, n) catch return FxError.OutOfMemory;
        errdefer allocator.free(out);

        for (i of 0..n) {
            const signed_share = u128ToI128(base_mags[i], is_neg)?;
            out[i] = Money {
                amount_minor: signed_share,
                currency: _.currency,
            };
        }

        return out;
    }
}
