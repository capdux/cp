// Source: cp/lib/decimal/money.cp (ISO 4217 Currency-Safe Money & Penny-Exact Allocation)
// Pure CP currency-safe monetary arithmetic and Hamilton largest-remainder
// penny-exact allocation engine ensuring zero-cent loss across splits.

const std = @import("std");
import { Decimal, DecimalError, RoundingMode, MAX_SCALE } from "./decimal.cp";

const I128_MAX_U128: u128 = ((1 as u128) << 127) - 1;
const I128_MIN_ABS_U128: u128 = (1 as u128) << 127;
const U64_MASK: u128 = ((1 as u128) << 64) - 1;

/// Error set for ISO 4217 currency validation and safe monetary allocation.
pub err MoneyError {
    CurrencyMismatch,
    InvalidCurrency,
    InvalidShares,
}

fn validateCurrencyCode(code: []const u8): [3]u8 ! MoneyError {
    if (code.len != 3) return MoneyError.InvalidCurrency;
    let out: [3]u8 = undefined;
    for (i of 0..3) {
        const c = code[i];
        if (c < 'A' || c > 'Z') return MoneyError.InvalidCurrency;
        out[i] = c;
    }
    return out;
}

fn eqlCurrency(a: [3]u8, b: [3]u8): bool {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

fn absU128Money(v: i128): u128 {
    if (v >= 0) return v as u128;
    const raw: u128 = @bitCast(v);
    return 0 -% raw;
}

fn u128ToI128Money(mag: u128, is_neg: bool): i128 ! DecimalError {
    if (mag == 0) return 0;
    if (!is_neg) {
        if (mag > I128_MAX_U128) return DecimalError.Overflow;
        return mag as i128;
    }
    if (mag > I128_MIN_ABS_U128) return DecimalError.Overflow;
    if (mag == I128_MIN_ABS_U128) return std.math.minInt(i128);
    return -(mag as i128);
}

struct ShareDivRem {
    q: u128 = 0;
    rem: u128 = 0;
}

// Exact (a * b) / den and remainder for u128 * u128 / u128 using 256-bit intermediate product.
fn mulDivRemU128(a: u128, b: u128, den: u128): ShareDivRem ! DecimalError {
    if (a == 0 || b == 0) return ShareDivRem { q: 0, rem: 0 };

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
        return ShareDivRem {
            q: lo / den,
            rem: lo % den,
        };
    }

    // Binary long division over 256-bit (hi, lo) by den:
    if (hi >= den) return DecimalError.Overflow;

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

    return ShareDivRem { q: q, rem: r };
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

/// Currency-tagged monetary value with ISO 4217 code and minor-unit precision.
pub struct Money {
    currency: [3]u8 = ['U', 'S', 'D'];
    amount: Decimal = Decimal.zero();
    minor_units: u8 = 2;

    /// Constructs a currency-safe `Money` value validating the 3-letter uppercase ISO 4217 code.
    static fn init(currency: []const u8, amount: Decimal, minor_units: u8): Money! {
        const code = validateCurrencyCode(currency)?;
        if (minor_units > MAX_SCALE) return DecimalError.InvalidScale;
        return Money {
            currency: code,
            amount: amount,
            minor_units: minor_units,
        };
    }

    /// Returns `true` if currency, minor units, and numerical amount all match.
    fn eql(other: Money): bool {
        return eqlCurrency(_.currency, other.currency) &&
            _.minor_units == other.minor_units &&
            _.amount.eql(other.amount);
    }

    /// Currency-safe addition; fails with `MoneyError.CurrencyMismatch` on differing currencies.
    fn add(other: Money): Money! {
        if (!eqlCurrency(_.currency, other.currency) || _.minor_units != other.minor_units) {
            return MoneyError.CurrencyMismatch;
        }
        const sum = _.amount.add(other.amount)?;
        return Money {
            currency: _.currency,
            amount: sum,
            minor_units: _.minor_units,
        };
    }

    /// Currency-safe subtraction; fails with `MoneyError.CurrencyMismatch` on differing currencies.
    fn sub(other: Money): Money! {
        if (!eqlCurrency(_.currency, other.currency) || _.minor_units != other.minor_units) {
            return MoneyError.CurrencyMismatch;
        }
        const diff = _.amount.sub(other.amount)?;
        return Money {
            currency: _.currency,
            amount: diff,
            minor_units: _.minor_units,
        };
    }

    /// Multiplies the monetary amount by `factor` and rounds to `minor_units` using `mode`.
    fn mulFactor(factor: Decimal, mode: RoundingMode): Money ! DecimalError {
        const prod = _.amount.mul(factor)?;
        const rounded = prod.round(_.minor_units, mode)?;
        return Money {
            currency: _.currency,
            amount: rounded,
            minor_units: _.minor_units,
        };
    }

    /// Converts the monetary amount into exact integer minor units (e.g. cents).
    fn toMinorUnits(): i128 ! DecimalError {
        const norm = _.amount.normalize();
        if (norm.scale > _.minor_units) return DecimalError.InvalidScale;
        const rescaled = norm.rescale(_.minor_units, .HalfEven)?;
        return rescaled.mantissa;
    }

    /// Penny-exact even split across `n` shares (e.g., `$100.00 / 3 -> [$33.34, $33.33, $33.33]`).
    fn allocateEven(alloc: std.mem.Allocator, n: usize): ([]Money)! {
        if (n == 0) return MoneyError.InvalidShares;
        const total_minor = _.toMinorUnits()?;

        let out = alloc.alloc(Money, n) catch return DecimalError.OutOfMemory;
        const n_i128 = n as i128;
        const q = @divTrunc(total_minor, n_i128);
        const r = @rem(total_minor, n_i128);

        for (i of 0..n) {
            let share_minor = q;
            const idx_i128 = i as i128;
            if (r > 0 && idx_i128 < r) {
                share_minor += 1;
            } else if (r < 0 && idx_i128 < -r) {
                share_minor -= 1;
            }
            out[i] = Money {
                currency: _.currency,
                amount: Decimal { mantissa: share_minor, scale: _.minor_units },
                minor_units: _.minor_units,
            };
        }
        return out;
    }

    /// Penny-exact weighted split using Hamilton's largest-remainder method.
    fn allocateRatios(alloc: std.mem.Allocator, ratios: []const u64): ([]Money)! {
        if (ratios.len == 0) return MoneyError.InvalidShares;

        let total_ratio: u128 = 0;
        for (r of ratios) {
            total_ratio += r as u128;
        }
        if (total_ratio == 0) return MoneyError.InvalidShares;

        const total_minor = _.toMinorUnits()?;
        const is_neg = total_minor < 0;
        const abs_total = absU128Money(total_minor);

        let out = alloc.alloc(Money, ratios.len) catch return DecimalError.OutOfMemory;
        errdefer alloc.free(out);

        let base_shares = alloc.alloc(u128, ratios.len) catch return DecimalError.OutOfMemory;
        defer alloc.free(base_shares);

        let rems = alloc.alloc(ShareRem, ratios.len) catch return DecimalError.OutOfMemory;
        defer alloc.free(rems);

        let allocated_sum: u128 = 0;
        for (i of 0..ratios.len) {
            const dr = mulDivRemU128(abs_total, ratios[i] as u128, total_ratio)?;
            base_shares[i] = dr.q;
            allocated_sum += dr.q;
            rems[i] = ShareRem { idx: i as usize, rem: dr.rem };
        }

        const leftover = abs_total - allocated_sum;
        if (leftover > 0) {
            std.sort.block(ShareRem, rems, 0 as u8, shareRemLessThan);
            let k: usize = 0;
            while ((k as u128) < leftover && k < rems.len) : (k += 1) {
                base_shares[rems[k].idx] += 1;
            }
        }

        for (i of 0..ratios.len) {
            const signed_minor = u128ToI128Money(base_shares[i], is_neg)?;
            out[i] = Money {
                currency: _.currency,
                amount: Decimal { mantissa: signed_minor, scale: _.minor_units },
                minor_units: _.minor_units,
            };
        }

        return out;
    }

    /// Formats money as `"USD 100.00"` into an owned string allocated from `alloc`.
    fn format(alloc: std.mem.Allocator): ([]u8) ! DecimalError {
        let amt_buf: [80]u8 = undefined;
        const amt_str = _.amount.formatBuf(&amt_buf)?;
        let out = alloc.alloc(u8, 4 + amt_str.len) catch return DecimalError.OutOfMemory;
        out[0] = _.currency[0];
        out[1] = _.currency[1];
        out[2] = _.currency[2];
        out[3] = ' ';
        @memcpy(out[4..], amt_str);
        return out;
    }
}
