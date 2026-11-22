import {
    CSS_INT_MIN,
    CSS_INT_MAX,
    CssNumberError,
    CssNumber,
    CssInteger,
} from "cap:css";
import { expect, test } from "cap:test";

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("css number: CssNumber.parse decimal, leading dot and scientific notation", fn () {
    const n1 = CssNumber.parse("  +42.5  ") catch CssNumber.fromF64(0.0);
    expect(n1.approxEql(CssNumber.fromF64(42.5), 1e-9)).toBe(true);
    expect(n1.sign()).toBe(1.0);
    expect(n1.isNegative()).toBe(false);

    const n2 = CssNumber.parse("-.25") catch CssNumber.fromF64(0.0);
    expect(n2.approxEql(CssNumber.fromF64(-0.25), 1e-9)).toBe(true);
    expect(n2.sign()).toBe(-1.0);
    expect(n2.abs().approxEql(CssNumber.fromF64(0.25), 1e-9)).toBe(true);

    const n3 = CssNumber.parse("1.5e2") catch CssNumber.fromF64(0.0);
    expect(n3.approxEql(CssNumber.fromF64(150.0), 1e-9)).toBe(true);

    const n4 = CssNumber.parse("25E-2") catch CssNumber.fromF64(0.0);
    expect(n4.approxEql(CssNumber.fromF64(0.25), 1e-9)).toBe(true);

    const n5 = CssNumber.parse("-0.0") catch CssNumber.fromF64(1.0);
    expect(n5.isZero()).toBe(true);
    expect(n5.sign()).toBe(0.0);
});

test("css number: CssNumber.toCss canonical minimal formatting", fn () {
    let buf: [32]u8 = [0; 32];

    const l0 = CssNumber.fromF64(-0.0).toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..l0], "0")).toBe(true);

    // 0 < |x| < 1 omits leading zero
    const l1 = CssNumber.fromF64(0.5).toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..l1], ".5")).toBe(true);

    const l2 = CssNumber.fromF64(-0.25).toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..l2], "-.25")).toBe(true);

    // Strips trailing zeros and whole-number decimal point
    const l3 = CssNumber.fromF64(42.0).toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..l3], "42")).toBe(true);

    const l4 = CssNumber.fromF64(-12.5).toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..l4], "-12.5")).toBe(true);

    // Clamping helper
    const clamped = CssNumber.fromF64(1.8).clamp(0.0, 1.0);
    const l5 = clamped.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..l5], "1")).toBe(true);
});

test("css number: CssInteger parse, extreme i32 bounds and toCss", fn () {
    let buf: [32]u8 = [0; 32];

    const i1 = CssInteger.parse("+1024") catch CssInteger.fromI32(0);
    expect(i1.value).toBe(1024);
    expect(i1.sign()).toBe(1);
    const n1 = i1.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n1], "1024")).toBe(true);

    const i_min = CssInteger.parse("-2147483648") catch CssInteger.fromI32(0);
    expect(i_min.value).toBe(CSS_INT_MIN);
    expect(i_min.sign()).toBe(-1);
    const n_min = i_min.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n_min], "-2147483648")).toBe(true);

    const i_max = CssInteger.parse("2147483647") catch CssInteger.fromI32(0);
    expect(i_max.value).toBe(CSS_INT_MAX);
    const n_max = i_max.toCss(buf[0..]) catch 0;
    expect(sliceEql(buf[0..n_max], "2147483647")).toBe(true);

    const i_zero = CssInteger.parse("-0") catch CssInteger.fromI32(9);
    expect(i_zero.value).toBe(0);
    expect(i_zero.sign()).toBe(0);
    expect(i_zero.clamp(-10, 10).eql(CssInteger.fromI32(0))).toBe(true);
});

test("css number: rejects malformed <number> syntax and empty input", fn () {
    let got_empty = false;
    * = CssNumber.parse("  ") catch |e| {
        got_empty = e == CssNumberError.EmptyInput;
    };
    expect(got_empty).toBe(true);

    // Trailing dot without digits is invalid per CSS Syntax L3
    let got_dot = false;
    * = CssNumber.parse("12.") catch |e| {
        got_dot = e == CssNumberError.InvalidSyntax;
    };
    expect(got_dot).toBe(true);

    // Trailing unit is not a bare <number>
    let got_unit = false;
    * = CssNumber.parse("12px") catch |e| {
        got_unit = e == CssNumberError.InvalidSyntax;
    };
    expect(got_unit).toBe(true);

    // Missing exponent digits
    let got_exp = false;
    * = CssNumber.parse("1e+") catch |e| {
        got_exp = e == CssNumberError.InvalidSyntax;
    };
    expect(got_exp).toBe(true);
});

test("css number: rejects <integer> decimals, exponents and i32 overflow", fn () {
    let got_dec = false;
    * = CssInteger.parse("1.0") catch |e| {
        got_dec = e == CssNumberError.InvalidSyntax;
    };
    expect(got_dec).toBe(true);

    let got_exp = false;
    * = CssInteger.parse("1e3") catch |e| {
        got_exp = e == CssNumberError.InvalidSyntax;
    };
    expect(got_exp).toBe(true);

    let got_pos_ovf = false;
    * = CssInteger.parse("2147483648") catch |e| {
        got_pos_ovf = e == CssNumberError.Overflow;
    };
    expect(got_pos_ovf).toBe(true);

    let got_neg_ovf = false;
    * = CssInteger.parse("-2147483649") catch |e| {
        got_neg_ovf = e == CssNumberError.Overflow;
    };
    expect(got_neg_ovf).toBe(true);
});

test("css number: buffer too small guards on toCss", fn () {
    let tiny: [2]u8 = [0; 2];

    let num_small = false;
    * = CssNumber.fromF64(-0.25).toCss(tiny[0..]) catch |e| {
        num_small = e == CssNumberError.BufferTooSmall;
    };
    expect(num_small).toBe(true);

    let int_small = false;
    * = CssInteger.fromI32(-1234).toCss(tiny[0..]) catch |e| {
        int_small = e == CssNumberError.BufferTooSmall;
    };
    expect(int_small).toBe(true);
});
