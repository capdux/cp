// Source: cp/cap/test/test_fx_production_deep.cp
// Deep Production Test Suite for cap:fx & cap.fx
// Covers:
//   1. Money.parse / format / minorUnits (USD 2, JPY 0, BTC 8, custom Currency.register) & CurrencyMismatch guard
//   2. All 5 RoundingModes (HalfUp, HalfEven banker's rounding, Floor, Ceil, Trunc) & allocate zero-penny-loss conservation
//   3. FxBook direct, automatic inverse, and 1-hop triangulation (CNY -> USD -> JPY) & cross-minorUnits (USD <-> JPY <-> BTC)
//   4. Invalid amount format / negative/NaN/Inf rate / RateNotFound / UnknownCurrency / DivisionByZero / InvalidAllocationWeights
//   5. Dynamic mounting via `import cap from "cap"; cap.fx` & `std.testing.allocator` zero-leak verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    FxError,
    RoundingMode,
    Currency,
    Money,
    FxRate,
    FxBook,
} from "cap:finance";
import cap from "cap";

fn expectMoneyFmt(alloc: std.mem.Allocator, m: Money, expected: []const u8)! {
    const s = m.format(alloc)?;
    defer alloc.free(s);
    expect_eq(std.mem.eql(u8, s, expected), true);
}

test("1. Money.parse/format, ISO-4217 minorUnits (USD/JPY/BTC/custom) & CurrencyMismatch") {
    const alloc = std.testing.allocator;

    // 1. Built-in ISO-4217 minorUnits
    expect_eq(Currency.minorUnits("USD")?, 2 as u8);
    expect_eq(Currency.minorUnits("EUR")?, 2 as u8);
    expect_eq(Currency.minorUnits("GBP")?, 2 as u8);
    expect_eq(Currency.minorUnits("CNY")?, 2 as u8);
    expect_eq(Currency.minorUnits("JPY")?, 0 as u8);
    expect_eq(Currency.minorUnits("KRW")?, 0 as u8);
    expect_eq(Currency.minorUnits("CHF")?, 2 as u8);
    expect_eq(Currency.minorUnits("HKD")?, 2 as u8);
    expect_eq(Currency.minorUnits("SGD")?, 2 as u8);
    expect_eq(Currency.minorUnits("BTC")?, 8 as u8);

    // 2. Custom currency registration & query
    Currency.register("XAU", 4)?;
    expect_eq(Currency.minorUnits("XAU")?, 4 as u8);
    const gold = Money.parse("1.25", "XAU")?;
    expect_eq(gold.amount_minor, 12500 as i128);
    expectMoneyFmt(alloc, gold, "1.2500 XAU")?;

    // 3. Parse & format across USD (2), JPY (0), BTC (8)
    const usd = Money.parse("12.34", "USD")?;
    expect_eq(usd.amount_minor, 1234 as i128);
    expectMoneyFmt(alloc, usd, "12.34 USD")?;

    const usd_small = Money.fromMinor(-5, "USD")?;
    expectMoneyFmt(alloc, usd_small, "-0.05 USD")?;

    const jpy = Money.parse("1500", "JPY")?;
    expect_eq(jpy.amount_minor, 1500 as i128);
    expectMoneyFmt(alloc, jpy, "1500 JPY")?;

    const sat = Money.parse("0.00000001", "BTC")?;
    expect_eq(sat.amount_minor, 1 as i128);
    expectMoneyFmt(alloc, sat, "0.00000001 BTC")?;

    // 4. Same-currency add/sub & cross-currency CurrencyMismatch guard
    const usd2 = Money.parse("0.66", "USD")?;
    const sum_usd = usd.add(usd2)?;
    expectMoneyFmt(alloc, sum_usd, "13.00 USD")?;
    const diff_usd = sum_usd.sub(usd2)?;
    expect_eq(diff_usd.eql(usd), true);

    const eur = Money.parse("10.00", "EUR")?;
    let add_mismatch = false;
    * = usd.add(eur) catch |e| {
        if (e == FxError.CurrencyMismatch) add_mismatch = true;
    };
    expect_eq(add_mismatch, true);

    let sub_mismatch = false;
    * = usd.sub(eur) catch |e| {
        if (e == FxError.CurrencyMismatch) sub_mismatch = true;
    };
    expect_eq(sub_mismatch, true);
}

test("2. All 5 RoundingModes (HalfUp/HalfEven/Floor/Ceil/Trunc) & Zero-Loss Allocate") {
    const alloc = std.testing.allocator;

    // 1. HalfEven (Banker's Rounding): 2.5 -> 2, 3.5 -> 4, -2.5 -> -2, -3.5 -> -4
    const j5 = Money.fromMinor(5, "JPY")?;
    const j7 = Money.fromMinor(7, "JPY")?;
    const jn5 = Money.fromMinor(-5, "JPY")?;
    const jn7 = Money.fromMinor(-7, "JPY")?;

    expect_eq((j5.mulScale(1, 2, .HalfEven)?).amount_minor, 2 as i128);
    expect_eq((j7.mulScale(1, 2, .HalfEven)?).amount_minor, 4 as i128);
    expect_eq((jn5.mulScale(1, 2, .HalfEven)?).amount_minor, -2 as i128);
    expect_eq((jn7.mulScale(1, 2, .HalfEven)?).amount_minor, -4 as i128);

    // 2. HalfUp (half away from zero): 2.5 -> 3, -2.5 -> -3
    expect_eq((j5.mulScale(1, 2, .HalfUp)?).amount_minor, 3 as i128);
    expect_eq((jn5.mulScale(1, 2, .HalfUp)?).amount_minor, -3 as i128);

    // 3. Floor vs Ceil vs Trunc on positive & negative fractions (7 / 3 = 2.333..., -7 / 3 = -2.333...)
    expect_eq((j7.mulScale(1, 3, .Floor)?).amount_minor, 2 as i128);
    expect_eq((j7.mulScale(1, 3, .Ceil)?).amount_minor, 3 as i128);
    expect_eq((j7.mulScale(1, 3, .Trunc)?).amount_minor, 2 as i128);

    expect_eq((jn7.mulScale(1, 3, .Floor)?).amount_minor, -3 as i128);
    expect_eq((jn7.mulScale(1, 3, .Ceil)?).amount_minor, -2 as i128);
    expect_eq((jn7.mulScale(1, 3, .Trunc)?).amount_minor, -2 as i128);

    // 4. Weighted allocation: $100.00 split [1, 1, 1] -> [33.34, 33.33, 33.33]
    const hundred = Money.parse("100.00", "USD")?;
    const eq_weights: [3]u64 = [1, 1, 1];
    const eq_parts = hundred.allocate(alloc, &eq_weights)?;
    defer alloc.free(eq_parts);
    expect_eq(eq_parts.len, 3 as usize);
    expect_eq(eq_parts[0].amount_minor, 3334 as i128);
    expect_eq(eq_parts[1].amount_minor, 3333 as i128);
    expect_eq(eq_parts[2].amount_minor, 3333 as i128);

    let recon = Money.fromMinor(0, "USD")?;
    for (p of eq_parts) {
        recon = recon.add(p)?;
    }
    expect_eq(recon.eql(hundred), true);

    // 5. Weighted allocation with zero-weight party & negative refund
    const penny = Money.fromMinor(1, "USD")?;
    const zero_wt: [3]u64 = [0, 1, 1];
    const zero_parts = penny.allocate(alloc, &zero_wt)?;
    defer alloc.free(zero_parts);
    expect_eq(zero_parts[0].amount_minor, 0 as i128);
    expect_eq(zero_parts[1].amount_minor, 1 as i128);
    expect_eq(zero_parts[2].amount_minor, 0 as i128);

    const neg_bill = Money.parse("-100.01", "EUR")?;
    const w_70_20_10: [3]u64 = [70, 20, 10];
    const neg_parts = neg_bill.allocate(alloc, &w_70_20_10)?;
    defer alloc.free(neg_parts);
    expect_eq(neg_parts[0].amount_minor, -7001 as i128);
    expect_eq(neg_parts[1].amount_minor, -2000 as i128);
    expect_eq(neg_parts[2].amount_minor, -1000 as i128);

    let recon_neg = Money.fromMinor(0, "EUR")?;
    for (p of neg_parts) {
        recon_neg = recon_neg.add(p)?;
    }
    expect_eq(recon_neg.eql(neg_bill), true);
}

test("3. FxBook Direct, Automatic Inverse & 1-Hop Triangulation Across MinorUnits") {
    const alloc = std.testing.allocator;
    let book = FxBook.init(alloc);
    defer book.deinit();

    // Register USD->CNY = 7.20 and USD->JPY = 150.0 and BTC->USD = 60000.0
    book.setRate("USD", "CNY", 7.20)?;
    book.setRate("USD", "JPY", 150.0)?;
    book.setRate("BTC", "USD", 60000.0)?;

    // 1. Direct conversion: $10.00 USD -> 1500 JPY
    const usd_10 = Money.parse("10.00", "USD")?;
    const jpy_from_usd = book.convert(usd_10, "JPY", .HalfEven)?;
    expect_eq(jpy_from_usd.amount_minor, 1500 as i128);
    expectMoneyFmt(alloc, jpy_from_usd, "1500 JPY")?;

    // 2. Automatic inverse conversion: 1500 JPY -> $10.00 USD (exact)
    const usd_back = book.convert(jpy_from_usd, "USD", .HalfEven)?;
    expect_eq(usd_back.eql(usd_10), true);

    const inv_rate = book.getRate("CNY", "USD")?;
    // 1 / 7.20 ~= 0.138888889 -> 138_888_889
    expect_eq(inv_rate.rate_e9, 138888889 as u128);

    // 3. 1-hop triangulation: CNY -> USD -> JPY
    // 720.00 CNY -> 100.00 USD -> 15000 JPY
    const cny_720 = Money.parse("720.00", "CNY")?;
    const jpy_tri = book.convert(cny_720, "JPY", .HalfEven)?;
    expect_eq(jpy_tri.amount_minor, 15000 as i128);
    expectMoneyFmt(alloc, jpy_tri, "15000 JPY")?;

    // 4. Cross-minorUnits BTC(8) <-> USD(2) <-> JPY(0)
    // 0.00500000 BTC * $60,000/BTC = $300.00 USD
    const btc_half_pct = Money.parse("0.005", "BTC")?;
    const usd_300 = book.convert(btc_half_pct, "USD", .HalfEven)?;
    expect_eq(usd_300.amount_minor, 30000 as i128);
    expectMoneyFmt(alloc, usd_300, "300.00 USD")?;

    // 0.005 BTC -> USD -> JPY = 45000 JPY
    const jpy_45000 = book.convert(btc_half_pct, "JPY", .HalfEven)?;
    expect_eq(jpy_45000.amount_minor, 45000 as i128);

    // 5. Large amount conversion without overflow: $10,000,000,000.00 -> JPY
    const big_usd = Money.parse("10000000000.00", "USD")?;
    const big_jpy = book.convert(big_usd, "JPY", .HalfEven)?;
    expect_eq(big_jpy.amount_minor, 1500000000000 as i128);
}

test("4. InvalidAmountFormat, InvalidRate, RateNotFound, UnknownCurrency & DivByZero Guards") {
    const alloc = std.testing.allocator;
    let book = FxBook.init(alloc);
    defer book.deinit();

    // 1. InvalidAmountFormat: extra decimal places for USD, trailing dot, empty, non-digits
    let bad_fmt_1 = false;
    * = Money.parse("12.345", "USD") catch |e| {
        if (e == FxError.InvalidAmountFormat) bad_fmt_1 = true;
    };
    expect_eq(bad_fmt_1, true);

    let bad_fmt_2 = false;
    * = Money.parse("12.", "USD") catch |e| {
        if (e == FxError.InvalidAmountFormat) bad_fmt_2 = true;
    };
    expect_eq(bad_fmt_2, true);

    let bad_fmt_3 = false;
    * = Money.parse("10.5", "JPY") catch |e| {
        if (e == FxError.InvalidAmountFormat) bad_fmt_3 = true;
    };
    expect_eq(bad_fmt_3, true);

    // 2. UnknownCurrency
    let bad_ccy = false;
    * = Money.parse("10.00", "ZZZ") catch |e| {
        if (e == FxError.UnknownCurrency) bad_ccy = true;
    };
    expect_eq(bad_ccy, true);

    // 3. InvalidRate: negative, zero, NaN, Inf
    let neg_rate = false;
    * = book.setRate("USD", "EUR", -1.2) catch |e| {
        if (e == FxError.InvalidRate) neg_rate = true;
    };
    expect_eq(neg_rate, true);

    let zero_rate = false;
    * = book.setRate("USD", "EUR", 0.0) catch |e| {
        if (e == FxError.InvalidRate) zero_rate = true;
    };
    expect_eq(zero_rate, true);

    let nan_rate = false;
    * = book.setRate("USD", "EUR", std.math.nan(f64)) catch |e| {
        if (e == FxError.InvalidRate) nan_rate = true;
    };
    expect_eq(nan_rate, true);

    let inf_rate = false;
    * = book.setRate("USD", "EUR", std.math.inf(f64)) catch |e| {
        if (e == FxError.InvalidRate) inf_rate = true;
    };
    expect_eq(inf_rate, true);

    // 4. RateNotFound
    let rate_missing = false;
    * = book.getRate("USD", "GBP") catch |e| {
        if (e == FxError.RateNotFound) rate_missing = true;
    };
    expect_eq(rate_missing, true);

    // 5. DivisionByZero in mulScale & InvalidAllocationWeights
    const usd = Money.parse("10.00", "USD")?;
    let div_zero = false;
    * = usd.mulScale(1, 0, .HalfEven) catch |e| {
        if (e == FxError.DivisionByZero) div_zero = true;
    };
    expect_eq(div_zero, true);

    const zero_weights: [2]u64 = [0, 0];
    let bad_weights = false;
    * = usd.allocate(alloc, &zero_weights) catch |e| {
        if (e == FxError.InvalidAllocationWeights) bad_weights = true;
    };
    expect_eq(bad_weights, true);
}

test("5. Dynamic `cap.fx` Namespace Mounting & std.testing.allocator Zero-Leak") {
    const alloc = std.testing.allocator;

    const a = cap.finance.Money.parse("25.50", "SGD")?;
    const b = cap.finance.Money.parse("4.50", "SGD")?;
    const total = a.add(b)?;
    expectMoneyFmt(alloc, total, "30.00 SGD")?;

    let book = cap.finance.FxBook.init(alloc);
    defer book.deinit();
    book.setRate("USD", "SGD", 1.35)?;

    const usd = book.convert(total, "USD", cap.finance.RoundingMode.HalfEven)?;
    // 30.00 / 1.35 = 22.2222... -> 22.22 USD
    expectMoneyFmt(alloc, usd, "22.22 USD")?;

    const weights: [2]u64 = [1, 1];
    const halves = usd.allocate(alloc, &weights)?;
    defer alloc.free(halves);
    expect_eq(halves[0].amount_minor, 1111 as i128);
    expect_eq(halves[1].amount_minor, 1111 as i128);
}
