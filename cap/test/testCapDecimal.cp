// cap/test/testCapDecimal.cp
// Comprehensive unit tests for @cap/decimal

import { test, expect_eq } from "cap:test";
import { Decimal, RoundingMode } from "../../packages/@cap/decimal/src/index.cp";

fn mustParse(s: string) -> Decimal {
    return Decimal.parse(s) ?? Decimal.zero();
}

fn main() {
    test("1. @cap/decimal — Exact 0.1 + 0.2 == 0.3 & parsing", () => {
        const a = mustParse("0.1");
        const b = mustParse("0.2");
        const c = mustParse("0.3");
        const sum = a.add(b);
        expect_eq(sum.eq(c), true);

        let buf: [64]u8 = undefined;
        expect_eq(sum.format(buf[0..64]), "0.3");

        const sci1 = mustParse("1.25e-4");
        expect_eq(sci1.format(buf[0..64]), "0.000125");

        const sci2 = mustParse("3.5e3");
        expect_eq(sci2.format(buf[0..64]), "3500");

        const grouped = mustParse("-1_234.50");
        expect_eq(grouped.format(buf[0..64]), "-1234.50");
        expect_eq(grouped.normalize().format(buf[0..64]), "-1234.5");

        expect_eq(Decimal.parse("") == null, true);
        expect_eq(Decimal.parse("12.3.4") == null, true);
    });

    test("2. @cap/decimal — Add, Sub, Mul & Financial Invoice Calculation", () => {
        const price = mustParse("19.99");
        const qty = Decimal.fromInt(3);
        const subtotal = price.mul(qty); // 59.97
        const taxRate = mustParse("0.0825");
        const tax: Decimal = subtotal.mul(taxRate).round(2, RoundingMode.HalfUp); // 4.947525 -> 4.95
        const total = subtotal.add(tax); // 64.92

        let buf: [64]u8 = undefined;
        expect_eq(subtotal.format(buf[0..64]), "59.97");
        expect_eq(tax.format(buf[0..64]), "4.95");
        expect_eq(total.format(buf[0..64]), "64.92");

        const diff: Decimal = mustParse("10.00").sub(mustParse("10.005"));
        expect_eq(diff.format(buf[0..64]), "-0.005");
    });

    test("3. @cap/decimal — Division & Divide-by-Zero Guard", () => {
        const ten = Decimal.fromInt(10);
        const three = Decimal.fromInt(3);
        const q = ten.div(three, 4, RoundingMode.HalfEven) ?? Decimal.zero();
        let buf: [64]u8 = undefined;
        expect_eq(q.format(buf[0..64]), "3.3333");

        const negQ = Decimal.fromInt(-7).div(Decimal.fromInt(2), 0, RoundingMode.Floor) ?? Decimal.zero();
        expect_eq(negQ.format(buf[0..64]), "-4");

        expect_eq(ten.div(Decimal.zero(), 2, RoundingMode.HalfEven) == null, true);
    });

    test("4. @cap/decimal — All 6 Rounding Modes (Banker's HalfEven vs HalfUp)", () => {
        let buf: [64]u8 = undefined;
        const n25 = mustParse("2.5");
        const n35 = mustParse("3.5");
        const neg25 = mustParse("-2.5");

        // Banker's HalfEven: 2.5 -> 2 (even), 3.5 -> 4 (even)
        expect_eq(n25.round(0, RoundingMode.HalfEven).format(buf[0..64]), "2");
        expect_eq(n35.round(0, RoundingMode.HalfEven).format(buf[0..64]), "4");

        // HalfUp: 2.5 -> 3, -2.5 -> -3
        expect_eq(n25.round(0, RoundingMode.HalfUp).format(buf[0..64]), "3");
        expect_eq(neg25.round(0, RoundingMode.HalfUp).format(buf[0..64]), "-3");

        // HalfDown: 2.5 -> 2
        expect_eq(n25.round(0, RoundingMode.HalfDown).format(buf[0..64]), "2");

        // Floor / Ceil / Trunc
        expect_eq(neg25.round(0, RoundingMode.Floor).format(buf[0..64]), "-3");
        expect_eq(neg25.round(0, RoundingMode.Ceil).format(buf[0..64]), "-2");
        expect_eq(neg25.round(0, RoundingMode.Trunc).format(buf[0..64]), "-2");
    });

    test("5. @cap/decimal — Comparison, Scale Normalization & toFixed", () => {
        const a = mustParse("1.50");
        const b = mustParse("1.5");
        const c = mustParse("1.501");
        expect_eq(a.eq(b), true);
        expect_eq(a.lt(c), true);
        expect_eq(c.gt(a), true);
        expect_eq(a.max(c).eq(c), true);
        expect_eq(a.min(c).eq(a), true);

        let buf: [64]u8 = undefined;
        expect_eq(Decimal.fromInt(42).toFixed(2, RoundingMode.HalfEven, buf[0..64]), "42.00");
        expect_eq(mustParse("0.005").toFixed(2, RoundingMode.HalfUp, buf[0..64]), "0.01");
    });
}
