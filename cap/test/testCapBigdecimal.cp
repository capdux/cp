import { test, expect_eq } from "cap:test";
import {
    BigDecimal,
    parseDecimal,
    formatDecimal
} from "../../packages/bigdecimal/src/index.cp";

fn bdecTestBufEq(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn bdecTestBuf64(): [64]u8 {
    let b: [64]u8 = [
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    ];
    return b;
}

fn main() {
    test("1. @cap/bigdecimal — constructors, normalize & exact 0.1 + 0.2 == 0.3", () => {
        let z: BigDecimal = BigDecimal.zero();
        expect_eq(z.digits, 0);
        expect_eq(z.scale, 0);

        let a: BigDecimal = parseDecimal("0.1") ?? BigDecimal.zero();
        let b: BigDecimal = parseDecimal("0.2") ?? BigDecimal.zero();
        let c: BigDecimal = parseDecimal("0.3") ?? BigDecimal.zero();
        let sum: BigDecimal = a.add(b);
        expect_eq(sum.eq(c), true);

        let trailing: BigDecimal = BigDecimal.new(123400, 4).normalize();
        expect_eq(trailing.digits, 1234);
        expect_eq(trailing.scale, 2);
        expect_eq(BigDecimal.new(0, 5).normalize().scale, 0);
    });

    test("2. @cap/bigdecimal — parseDecimal (signs, whitespace, scientific) & invalid rejection", () => {
        let p1 = parseDecimal("  -42.750 ");
        expect_eq(p1 == null, false);
        let v1: BigDecimal = p1 ?? BigDecimal.zero();
        expect_eq(v1.eq(BigDecimal.new(-4275, 2)), true);

        let p2 = parseDecimal("+1.25e3");
        expect_eq(p2 == null, false);
        let v2: BigDecimal = p2 ?? BigDecimal.zero();
        expect_eq(v2.eq(BigDecimal.fromInt(1250)), true);

        let p3 = parseDecimal("5E-4");
        expect_eq(p3 == null, false);
        let v3: BigDecimal = p3 ?? BigDecimal.zero();
        expect_eq(v3.eq(BigDecimal.new(5, 4)), true);

        expect_eq(parseDecimal("") == null, true);
        expect_eq(parseDecimal("   ") == null, true);
        expect_eq(parseDecimal(".") == null, true);
        expect_eq(parseDecimal("12.3.4") == null, true);
        expect_eq(parseDecimal("12e") == null, true);
        expect_eq(parseDecimal("12abc") == null, true);
    });

    test("3. @cap/bigdecimal — formatDecimal for integers, fractions, negative & negative scale", () => {
        let buf: [64]u8 = bdecTestBuf64();

        let n1: usize = formatDecimal(BigDecimal.new(12345, 2), buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n1, "123.45"), true);

        let n2: usize = formatDecimal(BigDecimal.new(-5, 3), buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n2, "-0.005"), true);

        let n3: usize = formatDecimal(BigDecimal.new(42, -2), buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n3, "4200"), true);

        let n4: usize = formatDecimal(BigDecimal.zero(), buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n4, "0"), true);
    });

    test("4. @cap/bigdecimal — exact sub, mul, neg, abs & cmp ordering", () => {
        let price: BigDecimal = parseDecimal("19.99") ?? BigDecimal.zero();
        let qty: BigDecimal = BigDecimal.fromInt(3);
        let total: BigDecimal = price.mul(qty);
        expect_eq(total.eq(BigDecimal.new(5997, 2)), true);

        let discount: BigDecimal = parseDecimal("100.00") ?? BigDecimal.zero();
        let diff: BigDecimal = total.sub(discount);
        expect_eq(diff.eq(BigDecimal.new(-4003, 2)), true);
        expect_eq(diff.abs().eq(BigDecimal.new(4003, 2)), true);
        expect_eq(diff.neg().eq(BigDecimal.new(4003, 2)), true);

        expect_eq(BigDecimal.new(-5, 1).cmp(BigDecimal.new(-2, 1)), -1);
        expect_eq(BigDecimal.new(100, 2).cmp(BigDecimal.fromInt(1)), 0);
        expect_eq(BigDecimal.new(101, 2).cmp(BigDecimal.fromInt(1)), 1);
    });

    test("5. @cap/bigdecimal — divWithScale exact long division & divide-by-zero null", () => {
        let buf: [64]u8 = bdecTestBuf64();
        let one: BigDecimal = BigDecimal.fromInt(1);
        let three: BigDecimal = BigDecimal.fromInt(3);

        let q1 = one.divWithScale(three, 6);
        expect_eq(q1 == null, false);
        let n1: usize = formatDecimal(q1 ?? BigDecimal.zero(), buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n1, "0.333333"), true);

        let two: BigDecimal = BigDecimal.fromInt(2);
        let q2 = two.divWithScale(three, 4);
        expect_eq(q2 == null, false);
        let n2: usize = formatDecimal(q2 ?? BigDecimal.zero(), buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n2, "0.6667"), true);

        let neg_num: BigDecimal = parseDecimal("-10.5") ?? BigDecimal.zero();
        let q3 = neg_num.divWithScale(BigDecimal.fromInt(4), 3);
        expect_eq(q3 == null, false);
        let n3: usize = formatDecimal(q3 ?? BigDecimal.zero(), buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n3, "-2.625"), true);

        expect_eq(one.divWithScale(BigDecimal.zero(), 2) == null, true);
    });

    test("6. @cap/bigdecimal — half-away-from-zero financial rounding", () => {
        let buf: [64]u8 = bdecTestBuf64();

        let r1: BigDecimal = BigDecimal.new(12345, 3).round(2);
        let n1: usize = formatDecimal(r1, buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n1, "12.35"), true);

        let r2: BigDecimal = BigDecimal.new(-12345, 3).round(2);
        let n2: usize = formatDecimal(r2, buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n2, "-12.35"), true);

        let r3: BigDecimal = BigDecimal.new(12344, 3).round(2);
        let n3: usize = formatDecimal(r3, buf[0..64]);
        expect_eq(bdecTestBufEq(buf[0..64], n3, "12.34"), true);

        let r4: BigDecimal = BigDecimal.new(9999, 2).round(0);
        expect_eq(r4.eq(BigDecimal.fromInt(100)), true);
    });
}
