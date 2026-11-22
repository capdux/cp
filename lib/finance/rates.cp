// Source: cp/lib/fx/rates.cp (FX Rate Registry & 1-Hop Triangulation Engine)
// Pure CP fixed-point FX rate book supporting direct lookup, automatic inverse rates,
// 1-hop pivot triangulation, and minor-unit-aware cross-currency conversion.

const std = @import("std");
import {
    FxError,
    RoundingMode,
    Currency,
    Money,
    pow10U128,
    mulDivUnsignedRational,
} from "./money.cp";

pub const RATE_SCALE_E9: u128 = 1_000_000_000;
const RATE_SCALE_E18: u128 = 1_000_000_000_000_000_000;
const MAX_RATE_E9_FLOAT: f64 = 1.0e28;

fn eqlCcy(a: [3]u8, b: [3]u8): bool {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

fn gcdU128(a_in: u128, b_in: u128): u128 {
    let a = a_in;
    let b = b_in;
    while (b != 0) {
        const t = a % b;
        a = b;
        b = t;
    }
    if (a == 0) return 1;
    return a;
}

struct RationalRate {
    num: u128 = 1;
    den: u128 = 1;
}

fn reduceRational(num_in: u128, den_in: u128): RationalRate {
    const g = gcdU128(num_in, den_in);
    return RationalRate {
        num: num_in / g,
        den: den_in / g,
    };
}

fn mulU128Checked(a: u128, b: u128): u128 ! FxError {
    if (a == 0 || b == 0) return 0;
    if (a > std.math.maxInt(u128) / b) return FxError.Overflow;
    return a * b;
}

/// Fixed-point foreign exchange rate (`1 base = (rate_e9 / 1e9) quote`).
pub struct FxRate {
    base: [3]u8 = ['U', 'S', 'D'];
    quote: [3]u8 = ['U', 'S', 'D'];
    rate_e9: u128 = RATE_SCALE_E9;

    /// Constructs an `FxRate` from a validated `1e-9` scaled integer rate.
    pub static fn fromE9(base: []const u8, quote: []const u8, rate_e9: u128): FxRate ! FxError {
        const b3 = Currency.parseCode(base)?;
        const q3 = Currency.parseCode(quote)?;
        if (rate_e9 == 0) return FxError.InvalidRate;
        return FxRate {
            base: b3,
            quote: q3,
            rate_e9: rate_e9,
        };
    }

    /// Constructs an `FxRate` from a positive finite `f64` rate, rejecting NaN, Inf, and `<= 0`.
    pub static fn fromFloat(base: []const u8, quote: []const u8, rate: f64): FxRate ! FxError {
        const b3 = Currency.parseCode(base)?;
        const q3 = Currency.parseCode(quote)?;
        if (!std.math.isFinite(rate) || rate <= 0.0) return FxError.InvalidRate;
        const scaled = @round(rate * (RATE_SCALE_E9 as f64));
        if (!std.math.isFinite(scaled) || scaled <= 0.0 || scaled > MAX_RATE_E9_FLOAT) {
            return FxError.InvalidRate;
        }
        const rate_e9: u128 = @intFromFloat(scaled);
        if (rate_e9 == 0) return FxError.InvalidRate;
        return FxRate {
            base: b3,
            quote: q3,
            rate_e9: rate_e9,
        };
    }

    /// Returns the rate as an `f64` approximation for display/inspection.
    pub fn toFloat(): f64 {
        return (_.rate_e9 as f64) / (RATE_SCALE_E9 as f64);
    }
}

/// Foreign exchange rate book supporting direct, inverse, and 1-hop pivot triangulation.
pub struct FxBook {
    alloc: std.mem.Allocator;
    rates: std.ArrayListUnmanaged(FxRate) = .empty;

    /// Creates an empty `FxBook` backed by `allocator`.
    pub static fn init(allocator: std.mem.Allocator): FxBook {
        return FxBook {
            alloc: allocator,
            rates: .empty,
        };
    }

    /// Releases all memory owned by the `FxBook`.
    pub fn deinit(*_) {
        _.rates.deinit(_.alloc);
    }

    /// Registers or updates a direct exchange rate `base -> quote` from a positive finite `f64`.
    pub fn setRate(*_, base: []const u8, quote: []const u8, rate: f64): void ! FxError {
        const entry = FxRate.fromFloat(base, quote, rate)?;
        _.upsertRate(entry)?;
    }

    /// Registers or updates a direct exchange rate `base -> quote` using exact `1e-9` integer scaling.
    pub fn setRateE9(*_, base: []const u8, quote: []const u8, rate_e9: u128): void ! FxError {
        const entry = FxRate.fromE9(base, quote, rate_e9)?;
        _.upsertRate(entry)?;
    }

    fn upsertRate(*_, entry: FxRate): void ! FxError {
        if (eqlCcy(entry.base, entry.quote)) {
            if (entry.rate_e9 != RATE_SCALE_E9) return FxError.InvalidRate;
            return;
        }
        for (*item of _.rates.items) {
            if (eqlCcy(item.base, entry.base) && eqlCcy(item.quote, entry.quote)) {
                item.rate_e9 = entry.rate_e9;
                return;
            }
        }
        _.rates.append(_.alloc, entry) catch return FxError.OutOfMemory;
    }

    fn findDirectOrInverse(base: [3]u8, quote: [3]u8): RationalRate? {
        for (item of _.rates.items) {
            if (eqlCcy(item.base, base) && eqlCcy(item.quote, quote)) {
                return reduceRational(item.rate_e9, RATE_SCALE_E9);
            }
        }
        for (item of _.rates.items) {
            if (eqlCcy(item.base, quote) && eqlCcy(item.quote, base)) {
                return reduceRational(RATE_SCALE_E9, item.rate_e9);
            }
        }
        return null;
    }

    fn findTriangulated(base: [3]u8, quote: [3]u8): RationalRate ! FxError {
        // Prefer standard global hub currencies first when multiple 1-hop paths exist.
        const preferred_hubs: [2][3]u8 = [
            ['U', 'S', 'D'],
            ['E', 'U', 'R'],
        ];
        for (hub of preferred_hubs) {
            if (!eqlCcy(hub, base) && !eqlCcy(hub, quote)) {
                if (r1 := _.findDirectOrInverse(base, hub)) {
                    if (r2 := _.findDirectOrInverse(hub, quote)) {
                        const num = mulU128Checked(r1.num, r2.num)?;
                        const den = mulU128Checked(r1.den, r2.den)?;
                        return reduceRational(num, den);
                    }
                }
            }
        }

        // Fall back to any registered currency as a 1-hop pivot.
        for (item of _.rates.items) {
            const candidates: [2][3]u8 = [item.base, item.quote];
            for (pivot of candidates) {
                if (!eqlCcy(pivot, base) && !eqlCcy(pivot, quote)) {
                    if (r1 := _.findDirectOrInverse(base, pivot)) {
                        if (r2 := _.findDirectOrInverse(pivot, quote)) {
                            const num = mulU128Checked(r1.num, r2.num)?;
                            const den = mulU128Checked(r1.den, r2.den)?;
                            return reduceRational(num, den);
                        }
                    }
                }
            }
        }

        return FxError.RateNotFound;
    }

    fn resolveRational(base: [3]u8, quote: [3]u8): RationalRate ! FxError {
        if (eqlCcy(base, quote)) return RationalRate { num: 1, den: 1 };
        if (r := _.findDirectOrInverse(base, quote)) {
            return r;
        }
        return _.findTriangulated(base, quote);
    }

    /// Resolves `base -> quote` via direct lookup, automatic inverse, or 1-hop pivot triangulation.
    pub fn getRate(base: []const u8, quote: []const u8): FxRate ! FxError {
        const b3 = Currency.parseCode(base)?;
        const q3 = Currency.parseCode(quote)?;
        if (eqlCcy(b3, q3)) {
            return FxRate {
                base: b3,
                quote: q3,
                rate_e9: RATE_SCALE_E9,
            };
        }

        const rat = _.resolveRational(b3, q3)?;
        // Convert exact rational (num / den) to nearest 1e-9 fixed-point integer.
        if (rat.num > std.math.maxInt(u128) / RATE_SCALE_E9) return FxError.Overflow;
        const scaled_num = rat.num * RATE_SCALE_E9;
        const rate_e9 = (scaled_num + (rat.den >> 1)) / rat.den;
        if (rate_e9 == 0) return FxError.InvalidRate;
        return FxRate {
            base: b3,
            quote: q3,
            rate_e9: rate_e9,
        };
    }

    /// Converts `money` into `target_code` with exact minor-unit scaling and `rounding`.
    pub fn convert(money: Money, target_code: []const u8, rounding: RoundingMode): Money ! FxError {
        * = Currency.minorUnits(&money.currency)?;
        const dst_c3 = Currency.parseCode(target_code)?;
        if (eqlCcy(money.currency, dst_c3)) return money;

        const rat = _.resolveRational(money.currency, dst_c3)?;
        const src_mu = Currency.minorUnits(&money.currency)?;
        const dst_mu = Currency.minorUnits(target_code)?;

        let num = rat.num;
        let den = rat.den;
        if (dst_mu > src_mu) {
            const factor = pow10U128(dst_mu - src_mu)?;
            const g = gcdU128(factor, den);
            num = mulU128Checked(num, factor / g)?;
            den /= g;
        } else if (src_mu > dst_mu) {
            const factor = pow10U128(src_mu - dst_mu)?;
            const g = gcdU128(factor, num);
            den = mulU128Checked(den, factor / g)?;
            num /= g;
        }

        const reduced = reduceRational(num, den);
        const converted_minor = mulDivUnsignedRational(
            money.amount_minor,
            reduced.num,
            reduced.den,
            rounding,
        )?;
        return Money {
            amount_minor: converted_minor,
            currency: dst_c3,
        };
    }
}
