// Source: cp/cap/test/test_decimal_production_deep.cp
// Deep Production Test Suite for cap:decimal
// Covers:
//   1. Exact 0.1 + 0.2 == 0.3, scientific notation parsing (1.5e-4), normalized equality (1.20 == 1.2) & formatting
//   2. High-precision mul/div (28-digit scale & AI token billing), division-by-zero & overflow guards
//   3. All 5 rounding modes (HalfUp, HalfEven banker's rounding 2.5->2 / 3.5->4, Floor, Ceil, Trunc)
//   4. Money ISO 4217 currency isolation, CurrencyMismatch guard, $100.00/3 & weighted penny-exact allocation
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.decimal`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MAX_SCALE,
    DecimalError,
    RoundingMode,
    Decimal,
    MoneyError,
    Money,
} from "cap:decimal";
import cap from "cap";

fn expectDecimalFmt(alloc: std.mem.Allocator, d: Decimal, expected: []const u8)! {
    const s = d.format(alloc)?;
    defer alloc.free(s);
    expect_eq(std.mem.eql(u8, s, expected), true);
}

fn testExactAddSubParseNormalize()! {
    const alloc = std.heap.page_allocator;

    // 1. Classic IEEE 754 killer: 0.1 + 0.2 == 0.3
    const d01 = Decimal.parse("0.1")?;
    const d02 = Decimal.parse("0.2")?;
    const d03 = Decimal.parse("0.3")?;
    const sum = d01.add(d02)?;
    expect_eq(sum.eql(d03), true);
    expect_eq(sum.cmp(d03), 0 as i8);
    expectDecimalFmt(alloc, sum, "0.3")?;

    // 2. Normalized equality: 1.20 == 1.2 == 1.2000e0
    const a = Decimal.parse("1.20")?;
    const b = Decimal.parse("1.2")?;
    expect_eq(a.eql(b), true);
    const a_norm = a.normalize();
    expect_eq(a_norm.mantissa, 12 as i128);
    expect_eq(a_norm.scale, 1 as u8);

    // 3. Scientific notation parsing (positive/negative exponents & signed zero)
    const sci1 = Decimal.parse("1.5e-4")?;
    expect_eq(sci1.eql(Decimal.parse("0.00015")?), true);
    expectDecimalFmt(alloc, sci1, "0.00015")?;

    const sci2 = Decimal.parse("-2.5E3")?;
    expect_eq(sci2.eql(Decimal.fromInt(-2500)), true);
    expectDecimalFmt(alloc, sci2, "-2500")?;

    const sci3 = Decimal.parse("+123.4500e-2")?;
    expect_eq(sci3.eql(Decimal.parse("1.2345")?), true);

    const neg_zero = Decimal.parse("-0.00")?;
    expect_eq(neg_zero.isZero(), true);
    expect_eq(neg_zero.eql(Decimal.zero()), true);

    // 4. Max scale = 28 support & invalid format/scale guards
    const tiny = Decimal.parse("0.0000000000000000000000000001")?;
    expect_eq(tiny.scale, MAX_SCALE);
    expect_eq(tiny.mantissa, 1 as i128);

    const diff = Decimal.one().sub(Decimal.parse("0.9")?)?;
    expect_eq(diff.eql(d01), true);

    const abs_neg = sci2.abs()?;
    expect_eq(abs_neg.eql(Decimal.fromInt(2500)), true);
    const back_neg = abs_neg.neg()?;
    expect_eq(back_neg.eql(sci2), true);

    let bad_fmt_hit = false;
    * = Decimal.parse("12.34.56") catch |e| {
        if (e == DecimalError.InvalidFormat) bad_fmt_hit = true;
    };
    expect_eq(bad_fmt_hit, true);

    let bad_scale_hit = false;
    * = Decimal.fromScaled(1, 29) catch |e| {
        if (e == DecimalError.InvalidScale) bad_scale_hit = true;
    };
    expect_eq(bad_scale_hit, true);
}

fn testHighPrecisionMulDivAndGuards()! {
    const alloc = std.heap.page_allocator;

    // 1. AI Token micro-billing: 1,234,567 tokens * $0.0000015/token = $1.8518505
    const tokens = Decimal.fromInt(1234567);
    const unit_price = Decimal.parse("1.5e-6")?;
    const cost = tokens.mul(unit_price)?;
    expect_eq(cost.eql(Decimal.parse("1.8518505")?), true);
    expectDecimalFmt(alloc, cost.normalize(), "1.8518505")?;

    // 2. High-precision large multiplication beyond 64-bit integer range
    const big1 = Decimal.parse("123456789012345678.90")?;
    const big2 = Decimal.parse("10000000000.5")?;
    const big_prod = big1.mul(big2)?;
    expect_eq(big_prod.eql(Decimal.parse("1234567890185185183506172839.45")?), true);

    // 3. Repeating division: 1 / 3 at scale 18
    const third = Decimal.one().div(Decimal.fromInt(3), 18, .Trunc)?;
    expectDecimalFmt(alloc, third, "0.333333333333333333")?;

    // 4. Negative division & exact cross-scale division
    const num = Decimal.parse("-10.5")?;
    const den = Decimal.parse("0.25")?;
    const q_exact = num.div(den, 2, .HalfEven)?;
    expect_eq(q_exact.eql(Decimal.fromInt(-42)), true);

    // 5. Division by zero guard
    let div_zero_hit = false;
    * = Decimal.one().div(Decimal.zero(), 4, .HalfEven) catch |e| {
        if (e == DecimalError.DivisionByZero) div_zero_hit = true;
    };
    expect_eq(div_zero_hit, true);

    // 6. Overflow guard
    const huge = Decimal.parse("100000000000000000000000000000")?; // 1e29
    let overflow_hit = false;
    * = huge.mul(huge) catch |e| {
        if (e == DecimalError.Overflow) overflow_hit = true;
    };
    expect_eq(overflow_hit, true);
}

fn testAllFiveRoundingModes()! {
    // 1. HalfEven (Banker's Rounding): 2.5 -> 2, 3.5 -> 4, -2.5 -> -2, -3.5 -> -4
    const p25 = Decimal.parse("2.5")?;
    const p35 = Decimal.parse("3.5")?;
    const n25 = Decimal.parse("-2.5")?;
    const n35 = Decimal.parse("-3.5")?;

    const r_p25_he = p25.round(0, .HalfEven)?;
    const r_p35_he = p35.round(0, .HalfEven)?;
    const r_n25_he = n25.round(0, .HalfEven)?;
    const r_n35_he = n35.round(0, .HalfEven)?;
    expect_eq(r_p25_he.eql(Decimal.fromInt(2)), true);
    expect_eq(r_p35_he.eql(Decimal.fromInt(4)), true);
    expect_eq(r_n25_he.eql(Decimal.fromInt(-2)), true);
    expect_eq(r_n35_he.eql(Decimal.fromInt(-4)), true);

    // 2. HalfUp (half away from zero): 2.5 -> 3, 3.5 -> 4, -2.5 -> -3
    const r_p25_hu = p25.round(0, .HalfUp)?;
    const r_p35_hu = p35.round(0, .HalfUp)?;
    const r_n25_hu = n25.round(0, .HalfUp)?;
    expect_eq(r_p25_hu.eql(Decimal.fromInt(3)), true);
    expect_eq(r_p35_hu.eql(Decimal.fromInt(4)), true);
    expect_eq(r_n25_hu.eql(Decimal.fromInt(-3)), true);

    // 3. Floor vs Ceil vs Trunc on positive and negative fractions
    const pos_f = Decimal.parse("1.231")?;
    const neg_f = Decimal.parse("-1.231")?;

    const r_pos_fl = pos_f.round(2, .Floor)?;
    const r_pos_ce = pos_f.round(2, .Ceil)?;
    const r_pos_tr = pos_f.round(2, .Trunc)?;
    expect_eq(r_pos_fl.eql(Decimal.parse("1.23")?), true);
    expect_eq(r_pos_ce.eql(Decimal.parse("1.24")?), true);
    expect_eq(r_pos_tr.eql(Decimal.parse("1.23")?), true);

    const r_neg_fl = neg_f.round(2, .Floor)?;
    const r_neg_ce = neg_f.round(2, .Ceil)?;
    const r_neg_tr = neg_f.round(2, .Trunc)?;
    expect_eq(r_neg_fl.eql(Decimal.parse("-1.24")?), true);
    expect_eq(r_neg_ce.eql(Decimal.parse("-1.23")?), true);
    expect_eq(r_neg_tr.eql(Decimal.parse("-1.23")?), true);

    // 4. Division rounding mode: 5 / 2 = 2.5 -> HalfEven=2, HalfUp=3; 7 / 2 = 3.5 -> HalfEven=4
    const q5_he = Decimal.fromInt(5).div(Decimal.fromInt(2), 0, .HalfEven)?;
    const q5_hu = Decimal.fromInt(5).div(Decimal.fromInt(2), 0, .HalfUp)?;
    const q7_he = Decimal.fromInt(7).div(Decimal.fromInt(2), 0, .HalfEven)?;
    expect_eq(q5_he.eql(Decimal.fromInt(2)), true);
    expect_eq(q5_hu.eql(Decimal.fromInt(3)), true);
    expect_eq(q7_he.eql(Decimal.fromInt(4)), true);
}

fn testMoneySafetyAndPennyExactAllocation()! {
    const alloc = std.heap.page_allocator;

    // 1. ISO 4217 validation & currency-safe arithmetic
    const usd1 = Money.init("USD", Decimal.parse("19.99")?, 2)?;
    const usd2 = Money.init("USD", Decimal.parse("0.01")?, 2)?;
    const usd_sum = usd1.add(usd2)?;
    expect_eq(usd_sum.amount.eql(Decimal.parse("20.00")?), true);

    const usd_str = usd_sum.format(alloc)?;
    defer alloc.free(usd_str);
    expect_eq(std.mem.eql(u8, usd_str, "USD 20.00"), true);

    const cny = Money.init("CNY", Decimal.parse("100.00")?, 2)?;
    let mismatch_hit = false;
    * = usd1.add(cny) catch |e| {
        if (e == @as(anyerror, MoneyError.CurrencyMismatch)) mismatch_hit = true;
    };
    expect_eq(mismatch_hit, true);

    let sub_mismatch_hit = false;
    * = usd1.sub(cny) catch |e| {
        if (e == @as(anyerror, MoneyError.CurrencyMismatch)) sub_mismatch_hit = true;
    };
    expect_eq(sub_mismatch_hit, true);

    let bad_ccy_hit = false;
    * = Money.init("usd", Decimal.one(), 2) catch |e| {
        if (e == @as(anyerror, MoneyError.InvalidCurrency)) bad_ccy_hit = true;
    };
    expect_eq(bad_ccy_hit, true);

    // 2. JPY (minor_units = 0) + mulFactor with tax rate 1.10 (10%)
    const jpy = Money.init("JPY", Decimal.fromInt(125), 0)?;
    const jpy_taxed = jpy.mulFactor(Decimal.parse("1.10")?, .HalfEven)?; // 137.5 -> 138
    expect_eq(jpy_taxed.amount.eql(Decimal.fromInt(138)), true);

    // 3. Penny-exact even allocation: $100.00 / 3 == [$33.34, $33.33, $33.33]
    const hundred = Money.init("USD", Decimal.parse("100.00")?, 2)?;
    const split3 = hundred.allocateEven(alloc, 3)?;
    defer alloc.free(split3);
    expect_eq(split3.len, 3 as usize);
    expect_eq(split3[0].amount.eql(Decimal.parse("33.34")?), true);
    expect_eq(split3[1].amount.eql(Decimal.parse("33.33")?), true);
    expect_eq(split3[2].amount.eql(Decimal.parse("33.33")?), true);

    let recon_even = Money.init("USD", Decimal.zero(), 2)?;
    for (s of split3) {
        recon_even = recon_even.add(s)?;
    }
    expect_eq(recon_even.eql(hundred), true);

    // 4. Penny-exact weighted ratio allocation (Hamilton largest-remainder + zero-weight guard)
    const ratios_a: [3]u64 = [70, 20, 10];
    const bill_100_01 = Money.init("EUR", Decimal.parse("100.01")?, 2)?;
    const split_w = bill_100_01.allocateRatios(alloc, &ratios_a)?;
    defer alloc.free(split_w);
    // 7000.7 (rem 70), 2000.2 (rem 20), 1000.1 (rem 10) -> 7001, 2000, 1000
    expect_eq(split_w[0].amount.eql(Decimal.parse("70.01")?), true);
    expect_eq(split_w[1].amount.eql(Decimal.parse("20.00")?), true);
    expect_eq(split_w[2].amount.eql(Decimal.parse("10.00")?), true);

    // Zero-weight party never receives leftover pennies
    const penny = Money.init("USD", Decimal.parse("0.01")?, 2)?;
    const zero_wt: [3]u64 = [0, 1, 1];
    const split_zero_wt = penny.allocateRatios(alloc, &zero_wt)?;
    defer alloc.free(split_zero_wt);
    expect_eq(split_zero_wt[0].amount.eql(Decimal.zero()), true);
    expect_eq(split_zero_wt[1].amount.eql(Decimal.parse("0.01")?), true);
    expect_eq(split_zero_wt[2].amount.eql(Decimal.zero()), true);

    // Negative refund split (-$100.00 / 3 -> [-$33.34, -$33.33, -$33.33])
    const refund = Money.init("USD", Decimal.parse("-100.00")?, 2)?;
    const refund_shares = refund.allocateEven(alloc, 3)?;
    defer alloc.free(refund_shares);
    let recon_refund = Money.init("USD", Decimal.zero(), 2)?;
    for (s of refund_shares) {
        recon_refund = recon_refund.add(s)?;
    }
    expect_eq(recon_refund.eql(refund), true);

    let bad_shares_even = false;
    * = hundred.allocateEven(alloc, 0) catch |e| {
        if (e == @as(anyerror, MoneyError.InvalidShares)) bad_shares_even = true;
    };
    expect_eq(bad_shares_even, true);

    const zero_ratios: [2]u64 = [0, 0];
    let bad_shares_ratio = false;
    * = hundred.allocateRatios(alloc, &zero_ratios) catch |e| {
        if (e == @as(anyerror, MoneyError.InvalidShares)) bad_shares_ratio = true;
    };
    expect_eq(bad_shares_ratio, true);
}

fn testCapDecimalDynamicLazyMount()! {
    const alloc = std.heap.page_allocator;

    const a = cap.decimal.Decimal.parse("0.1")?;
    const b = cap.decimal.Decimal.parse("0.2")?;
    const c = a.add(b)?;
    expect_eq(c.eql(cap.decimal.Decimal.parse("0.3")?), true);

    const raw_25 = cap.decimal.Decimal.parse("2.5")?;
    const rounded = raw_25.round(0, cap.decimal.RoundingMode.HalfEven)?;
    expect_eq(rounded.eql(cap.decimal.Decimal.fromInt(2)), true);

    const pot = cap.decimal.Money.init("USD", cap.decimal.Decimal.parse("10.00")?, 2)?;
    const parts = pot.allocateEven(alloc, 3)?;
    defer alloc.free(parts);
    expect_eq(parts[0].amount.eql(cap.decimal.Decimal.parse("3.34")?), true);
    expect_eq(parts[1].amount.eql(cap.decimal.Decimal.parse("3.33")?), true);
    expect_eq(parts[2].amount.eql(cap.decimal.Decimal.parse("3.33")?), true);
}

fn main() {
    test("1. Exact 0.1 + 0.2 == 0.3, Scientific Notation, Normalization & Formatting", () => {
        testExactAddSubParseNormalize()?;
    });

    test("2. High-Precision Multiplication/Division, DivisionByZero & Overflow Guards", () => {
        testHighPrecisionMulDivAndGuards()?;
    });

    test("3. All 5 Rounding Modes (HalfUp, HalfEven Banker's, Floor, Ceil, Trunc)", () => {
        testAllFiveRoundingModes()?;
    });

    test("4. ISO 4217 Money Currency Isolation & Penny-Exact Even/Ratio Allocation", () => {
        testMoneySafetyAndPennyExactAllocation()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.decimal`", () => {
        testCapDecimalDynamicLazyMount()?;
    });
}
