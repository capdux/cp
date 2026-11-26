import { test, expect_eq } from "cap:test";
import {
    Rational,
    gcd64,
    new,
    fromInt,
    zero,
    one,
    add,
    sub,
    mul,
    div,
    neg,
    abs,
    recip,
    eq,
    cmp,
    toF64,
    floorInt,
    ceilInt,
    isInteger,
} from "../../packages/@cap/rational/src/index.cp";

fn main() {
    test("1. @cap/rational — Auto-reduction & negative denominator normalization", () => {
        expect_eq(gcd64(48, 18), 6);
        expect_eq(gcd64(-48, 18), 6);
        expect_eq(gcd64(0, 7), 7);

        const r1 = new(6, 8);
        expect_eq(r1.numer, 3);
        expect_eq(r1.denom, 4);

        const r2 = new(4, -6);
        expect_eq(r2.numer, -2);
        expect_eq(r2.denom, 3);

        const r3 = new(-10, -15);
        expect_eq(r3.numer, 2);
        expect_eq(r3.denom, 3);

        const r4 = new(0, -99);
        expect_eq(r4.numer, 0);
        expect_eq(r4.denom, 1);
    });

    test("2. @cap/rational — Exact add, sub, mul & div arithmetic", () => {
        // 1/3 + 1/6 == 1/2
        const sum = add(new(1, 3), new(1, 6));
        expect_eq(eq(sum, new(1, 2)), true);
        expect_eq(sum.numer, 1);
        expect_eq(sum.denom, 2);

        // 1/2 - 5/6 == -1/3
        const diff = sub(new(1, 2), new(5, 6));
        expect_eq(eq(diff, new(-1, 3)), true);

        // (2/3) * (9/4) == 3/2
        const prod = mul(new(2, 3), new(9, 4));
        expect_eq(eq(prod, new(3, 2)), true);

        // (3/5) / (9/10) == 2/3
        const quot = div(new(3, 5), new(9, 10));
        expect_eq(eq(quot, new(2, 3)), true);

        // 1/10 + 2/10 == 3/10 (exact, no float drift)
        expect_eq(eq(add(new(1, 10), new(2, 10)), new(3, 10)), true);
    });

    test("3. @cap/rational — Comparison & sorting", () => {
        expect_eq(cmp(new(1, 3), new(1, 2)), -1);
        expect_eq(cmp(new(2, 4), new(1, 2)), 0);
        expect_eq(cmp(new(3, 4), new(2, 3)), 1);
        expect_eq(cmp(new(-1, 2), new(-1, 3)), -1);

        // Insertion sort 5 fractions in ascending order
        let arr: [5]Rational = [
            new(3, 4),
            new(-1, 2),
            new(2, 3),
            zero(),
            one(),
        ];
        let i: usize = 1;
        while (i < 5) {
            const key = arr[i];
            let j: usize = i;
            while (j > 0 && cmp(arr[j - 1], key) > 0) {
                arr[j] = arr[j - 1];
                j -= 1;
            }
            arr[j] = key;
            i += 1;
        }
        expect_eq(eq(arr[0], new(-1, 2)), true);
        expect_eq(eq(arr[1], zero()), true);
        expect_eq(eq(arr[2], new(2, 3)), true);
        expect_eq(eq(arr[3], new(3, 4)), true);
        expect_eq(eq(arr[4], one()), true);
    });

    test("4. @cap/rational — recip, abs & neg invariants", () => {
        const q = new(-3, 5);
        expect_eq(eq(neg(q), new(3, 5)), true);
        expect_eq(eq(abs(q), new(3, 5)), true);
        expect_eq(eq(abs(new(3, 5)), new(3, 5)), true);

        const inv = recip(q);
        expect_eq(inv.numer, -5);
        expect_eq(inv.denom, 3);
        expect_eq(eq(mul(q, inv), one()), true);
    });

    test("5. @cap/rational — toF64, floorInt, ceilInt & isInteger", () => {
        const half = new(1, 2);
        const f: f64 = toF64(half);
        const diff: f64 = if (f > 0.5) f - 0.5 else 0.5 - f;
        expect_eq(diff < 0.000000001, true);

        expect_eq(floorInt(new(7, 2)), 3);
        expect_eq(ceilInt(new(7, 2)), 4);
        expect_eq(floorInt(new(-7, 2)), -4);
        expect_eq(ceilInt(new(-7, 2)), -3);
        expect_eq(floorInt(fromInt(-3)), -3);
        expect_eq(ceilInt(fromInt(-3)), -3);

        expect_eq(isInteger(new(6, 3)), true);
        expect_eq(isInteger(fromInt(0)), true);
        expect_eq(isInteger(new(7, 3)), false);
    });

    test("6. @cap/rational — Zero denominator & divide-by-zero safety", () => {
        const z_den = new(5, 0);
        expect_eq(z_den.numer, 0);
        expect_eq(z_den.denom, 1);

        const z_div = div(new(3, 4), zero());
        expect_eq(eq(z_div, zero()), true);

        const z_rec = recip(zero());
        expect_eq(eq(z_rec, zero()), true);

        const raw_bad = Rational { numer: 5, denom: 0 };
        expect_eq(toF64(raw_bad), 0.0);
        expect_eq(floorInt(raw_bad), 0);
        expect_eq(ceilInt(raw_bad), 0);
        expect_eq(isInteger(raw_bad), false);
    });
}
