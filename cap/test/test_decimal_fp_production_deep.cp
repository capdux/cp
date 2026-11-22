// Source: native-cp-stdlib (Exact Fixed-Point Financial Decimal)
const std = @import("std");
import cap from "cap";
import { Decimal, DecimalError, RoundingMode, MAX_SCALE } from "../../lib/math/decimal_fp_.cp";
import { test, expect_eq } from "cap:test";

test "decimal_fp classic 0.1 + 0.2 == 0.3 and exact sub mul" {
    const a = Decimal.parse("0.1")?;
    const b = Decimal.parse("0.2")?;
    const sum = a.add(b)?;
    const expected = Decimal.parse("0.3")?;
    expect_eq(sum.equals(expected), true);

    const alloc = std.testing.allocator;
    const formatted = sum.formatAlloc(alloc)?;
    defer alloc.free(formatted);
    expect_eq(std.mem.eql(u8, formatted, "0.3"), true);

    const diff = expected.sub(a)?;
    expect_eq(diff.equals(b), true);

    const prod = a.mul(b)?;
    const expected_prod = Decimal.parse("0.02")?;
    expect_eq(prod.equals(expected_prod), true);
}

test "decimal_fp scale alignment add sub mul divRound" {
    const alloc = std.testing.allocator;

    const x = Decimal.parse("12.34")?;
    const y = Decimal.parse("0.0056")?;
    const sum = x.add(y)?;
    const sum_str = sum.formatAlloc(alloc)?;
    defer alloc.free(sum_str);
    expect_eq(std.mem.eql(u8, sum_str, "12.3456"), true);

    const diff = x.sub(y)?;
    const diff_str = diff.formatAlloc(alloc)?;
    defer alloc.free(diff_str);
    expect_eq(std.mem.eql(u8, diff_str, "12.3344"), true);

    const m = Decimal.parse("100.5")?;
    const n = Decimal.parse("0.25")?;
    const prod = m.mul(n)?;
    const prod_str = prod.formatAlloc(alloc)?;
    defer alloc.free(prod_str);
    expect_eq(std.mem.eql(u8, prod_str, "25.125"), true);

    const ten = Decimal.fromInt(10);
    const three = Decimal.fromInt(3);
    const q = ten.divRound(three, 4, RoundingMode.HalfUp)?;
    const q_str = q.formatAlloc(alloc)?;
    defer alloc.free(q_str);
    expect_eq(std.mem.eql(u8, q_str, "3.3333"), true);
}

test "decimal_fp four rounding modes and banker rounding" {
    const alloc = std.testing.allocator;

    const p_even = Decimal.parse("2.5")?;
    const p_odd = Decimal.parse("3.5")?;
    const n_even = Decimal.parse("-2.5")?;
    const n_odd = Decimal.parse("-3.5")?;

    const hu_pos = p_even.rescale(0, RoundingMode.HalfUp)?;
    expect_eq(hu_pos.mantissa, 3);
    const hu_neg = n_even.rescale(0, RoundingMode.HalfUp)?;
    expect_eq(hu_neg.mantissa, -3);

    const he_p1 = p_even.rescale(0, RoundingMode.HalfEven)?;
    expect_eq(he_p1.mantissa, 2);
    const he_p2 = p_odd.rescale(0, RoundingMode.HalfEven)?;
    expect_eq(he_p2.mantissa, 4);
    const he_n1 = n_even.rescale(0, RoundingMode.HalfEven)?;
    expect_eq(he_n1.mantissa, -2);
    const he_n2 = n_odd.rescale(0, RoundingMode.HalfEven)?;
    expect_eq(he_n2.mantissa, -4);

    const fl_pos = Decimal.parse("2.9")?.rescale(0, RoundingMode.Floor)?;
    expect_eq(fl_pos.mantissa, 2);
    const fl_neg = Decimal.parse("-2.1")?.rescale(0, RoundingMode.Floor)?;
    expect_eq(fl_neg.mantissa, -3);

    const ce_pos = Decimal.parse("2.1")?.rescale(0, RoundingMode.Ceil)?;
    expect_eq(ce_pos.mantissa, 3);
    const ce_neg = Decimal.parse("-2.9")?.rescale(0, RoundingMode.Ceil)?;
    expect_eq(ce_neg.mantissa, -2);

    const up_scaled = Decimal.parse("1.2")?.rescale(3, RoundingMode.HalfEven)?;
    const up_str = up_scaled.formatAlloc(alloc)?;
    defer alloc.free(up_str);
    expect_eq(std.mem.eql(u8, up_str, "1.200"), true);
}

test "decimal_fp parse edges and error interception" {
    const alloc = std.testing.allocator;

    const lz = Decimal.parse("-00042.050")?;
    const lz_str = lz.formatAlloc(alloc)?;
    defer alloc.free(lz_str);
    expect_eq(std.mem.eql(u8, lz_str, "-42.050"), true);

    const abs_lz = lz.abs();
    expect_eq(abs_lz.compare(Decimal.parse("42.05")?), 0);
    expect_eq(abs_lz.neg().equals(lz), true);

    const zero_dot = Decimal.parse(".5")?;
    expect_eq(zero_dot.equals(Decimal.parse("0.5")?), true);

    const bad_empty = Decimal.parse("");
    expect_eq(std.meta.isError(bad_empty), true);
    const bad_alpha = Decimal.parse("12.3a");
    expect_eq(std.meta.isError(bad_alpha), true);
    const bad_dots = Decimal.parse("1.2.3");
    expect_eq(std.meta.isError(bad_dots), true);

    const too_many_frac = Decimal.parse("0.1234567890123456789");
    expect_eq(std.meta.isError(too_many_frac), true);

    const bad_parts = Decimal.fromParts(1, MAX_SCALE + 1);
    expect_eq(std.meta.isError(bad_parts), true);

    const div_zero = Decimal.fromInt(1).divRound(Decimal.fromInt(0), 2, RoundingMode.HalfUp);
    expect_eq(std.meta.isError(div_zero), true);

    const max_d = Decimal.fromParts(std.math.maxInt(i128), 0)?;
    const ovf = max_d.add(Decimal.fromInt(1));
    expect_eq(std.meta.isError(ovf), true);
}

test "decimal_fp dynamic cap mount and zero leak verification" {
    const alloc = std.testing.allocator;

    const a = Decimal.parse("19.995")?;
    const b = Decimal.fromParts(2000, 2)?;
    const rounded = a.rescale(2, RoundingMode.HalfEven)?;
    expect_eq(rounded.equals(b), true);

    const s1 = rounded.formatAlloc(alloc)?;
    defer alloc.free(s1);
    expect_eq(std.mem.eql(u8, s1, "20.00"), true);

    const z = Decimal.fromParts(0, 3)?;
    const z_str = z.formatAlloc(alloc)?;
    defer alloc.free(z_str);
    expect_eq(std.mem.eql(u8, z_str, "0.000"), true);
}
