import { test, expect_eq } from "cap:test";
import { Decimal } from "../../packages/@cap/decimal-fixed/src/index.cp";

fn strEq(buf: []const u8, expected: string): bool {
    if (buf.len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < buf.len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/decimal-fixed — Classic 0.1 + 0.2 == 0.3 exact equality", () => {
        const a: Decimal = Decimal.parse("0.1").value;
        const b: Decimal = Decimal.parse("0.2").value;
        const c: Decimal = Decimal.parse("0.3").value;
        const sum: Decimal = a.add(b);
        expect_eq(sum.eq(c), true);
        expect_eq(sum.mantissa, 3);
        expect_eq(sum.scale, 1);

        const f: f64 = sum.toF64();
        const diff: f64 = if (f > 0.3) f - 0.3 else 0.3 - f;
        expect_eq(diff < 0.000000001, true);
    });

    test("2. @cap/decimal-fixed — parse() & format() round-trip + invalid rejection", () => {
        let buf: [32]u8 = undefined;

        const r1 = Decimal.parse("-123.45");
        expect_eq(r1.valid, true);
        const v1: Decimal = r1.value;
        const n1: usize = v1.format(&buf);
        expect_eq(strEq(buf[0..n1], "-123.45"), true);

        const r2 = Decimal.parse("+0.0050");
        expect_eq(r2.valid, true);
        const v2: Decimal = r2.value;
        const n2: usize = v2.format(&buf);
        expect_eq(strEq(buf[0..n2], "0.0050"), true);

        const r3 = Decimal.parse("42");
        expect_eq(r3.valid, true);
        const v3: Decimal = r3.value;
        const n3: usize = v3.format(&buf);
        expect_eq(strEq(buf[0..n3], "42"), true);

        expect_eq(Decimal.parse("").valid, false);
        expect_eq(Decimal.parse("1.2.3").valid, false);
        expect_eq(Decimal.parse("12a3").valid, false);
        expect_eq(Decimal.parse("-").valid, false);
    });

    test("3. @cap/decimal-fixed — Mixed-scale add/sub alignment & normalize()", () => {
        const a: Decimal = Decimal.parse("10.5").value;
        const b: Decimal = Decimal.parse("2.250").value;
        const sum: Decimal = a.add(b);
        expect_eq(sum.eq(Decimal.parse("12.75").value), true);

        const norm: Decimal = sum.normalize();
        expect_eq(norm.mantissa, 1275);
        expect_eq(norm.scale, 2);

        const raw_diff: Decimal = a.sub(b);
        const diff: Decimal = raw_diff.normalize();
        expect_eq(diff.eq(Decimal.parse("8.25").value), true);

        const z_raw: Decimal = Decimal.new(0, 4);
        const z: Decimal = z_raw.normalize();
        expect_eq(z.mantissa, 0);
        expect_eq(z.scale, 0);
    });

    test("4. @cap/decimal-fixed — Exact mul/div & divide-by-zero safety", () => {
        const price: Decimal = Decimal.parse("19.99").value;
        const qty: Decimal = Decimal.fromInt(3);
        const total: Decimal = price.mul(qty);
        expect_eq(total.eq(Decimal.parse("59.97").value), true);

        const one: Decimal = Decimal.one();
        const three: Decimal = Decimal.fromInt(3);
        const third: Decimal = one.div(three, 4);
        expect_eq(third.mantissa, 3333);
        expect_eq(third.scale, 4);

        const d75: Decimal = Decimal.parse("7.5").value;
        const d25: Decimal = Decimal.parse("2.5").value;
        const exact_div: Decimal = d75.div(d25, 2);
        expect_eq(exact_div.eq(Decimal.fromInt(3)), true);

        // Divide by zero safely returns zero
        const z_div: Decimal = price.div(Decimal.zero(), 2);
        expect_eq(z_div.eq(Decimal.zero()), true);
    });

    test("5. @cap/decimal-fixed — roundHalfEven() banker's rounding (2.5->2, 3.5->4)", () => {
        const p25: Decimal = Decimal.parse("2.5").value;
        const p35: Decimal = Decimal.parse("3.5").value;
        const n25: Decimal = Decimal.parse("-2.5").value;
        const n35: Decimal = Decimal.parse("-3.5").value;
        const p251: Decimal = Decimal.parse("2.51").value;
        const p249: Decimal = Decimal.parse("2.49").value;

        expect_eq(p25.roundHalfEven(0).mantissa, 2);
        expect_eq(p35.roundHalfEven(0).mantissa, 4);
        expect_eq(n25.roundHalfEven(0).mantissa, -2);
        expect_eq(n35.roundHalfEven(0).mantissa, -4);

        // Non-tie rounding
        expect_eq(p251.roundHalfEven(0).mantissa, 3);
        expect_eq(p249.roundHalfEven(0).mantissa, 2);

        // Standard rescale half-away-from-zero: 2.5 -> 3
        expect_eq(p25.rescale(0).mantissa, 3);
    });

    test("6. @cap/decimal-fixed — Comparison, negation & absolute value", () => {
        const neg_v: Decimal = Decimal.parse("-15.75").value;
        const pos_v: Decimal = Decimal.parse("15.750").value;
        const eq_v: Decimal = Decimal.parse("15.75").value;
        const neg_half: Decimal = Decimal.parse("-0.5").value;
        const neg_quarter: Decimal = Decimal.parse("-0.25").value;

        const negated: Decimal = neg_v.neg();
        const absed: Decimal = neg_v.abs();
        expect_eq(negated.eq(pos_v), true);
        expect_eq(absed.eq(pos_v), true);
        expect_eq(neg_v.cmp(pos_v), -1);
        expect_eq(pos_v.cmp(neg_v), 1);
        expect_eq(pos_v.cmp(eq_v), 0);
        expect_eq(neg_half.cmp(neg_quarter), -1);
    });
}
