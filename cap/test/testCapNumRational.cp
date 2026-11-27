import { test, expect_eq } from "cap:test";
import { Ratio64 } from "../../packages/num-rational/src/index.cp";

fn main() {
    test("1. @cap/num-rational — Canonical reduction, sign normalization & zero-denom rejection", () => {
        expect_eq(Ratio64.new(1, 0) == null, true);

        const r1 = Ratio64.new(6, 8);
        expect_eq(r1 != null, true);
        if (v := r1) {
            expect_eq(v.numer, 3 as i64);
            expect_eq(v.denom, 4 as i64);
        }

        const r2 = Ratio64.new(6, -9);
        expect_eq(r2 != null, true);
        if (v := r2) {
            expect_eq(v.numer, -2 as i64);
            expect_eq(v.denom, 3 as i64);
        }

        const r3 = Ratio64.new(-10, -15);
        expect_eq(r3 != null, true);
        if (v := r3) {
            expect_eq(v.numer, 2 as i64);
            expect_eq(v.denom, 3 as i64);
        }

        const r0 = Ratio64.new(0, -42);
        expect_eq(r0 != null, true);
        if (v := r0) {
            expect_eq(v.numer, 0 as i64);
            expect_eq(v.denom, 1 as i64);
            expect_eq(v.isZero(), true);
            expect_eq(v.isInteger(), true);
        }
    });

    test("2. @cap/num-rational — fromInt, zero, one & predicate queries", () => {
        const z = Ratio64.zero();
        const o = Ratio64.one();
        const n = Ratio64.fromInt(-7);

        expect_eq(z.isZero(), true);
        expect_eq(z.isPositive(), false);
        expect_eq(z.isNegative(), false);
        expect_eq(z.isInteger(), true);

        expect_eq(o.isPositive(), true);
        expect_eq(o.isInteger(), true);
        expect_eq(n.isNegative(), true);
        expect_eq(n.isInteger(), true);

        if (half := Ratio64.new(1, 2)) {
            expect_eq(half.isInteger(), false);
            expect_eq(half.isPositive(), true);
        }
    });

    test("3. @cap/num-rational — Exact add, sub, mul, div, recip, neg & abs", () => {
        if (a := Ratio64.new(1, 6)) {
            if (b := Ratio64.new(1, 3)) {
                const sum: Ratio64 = a.add(b);
                expect_eq(sum.numer, 1 as i64);
                expect_eq(sum.denom, 2 as i64);

                const diff: Ratio64 = a.sub(b);
                expect_eq(diff.numer, -1 as i64);
                expect_eq(diff.denom, 6 as i64);
                expect_eq(diff.abs().numer, 1 as i64);
                expect_eq(diff.neg().numer, 1 as i64);

                const prod: Ratio64 = a.mul(b);
                expect_eq(prod.numer, 1 as i64);
                expect_eq(prod.denom, 18 as i64);

                if (quot := a.div(b)) {
                    expect_eq(quot.numer, 1 as i64);
                    expect_eq(quot.denom, 2 as i64);
                }

                if (rec := diff.recip()) {
                    expect_eq(rec.numer, -6 as i64);
                    expect_eq(rec.denom, 1 as i64);
                }
            }
        }
        expect_eq(Ratio64.zero().recip() == null, true);
        expect_eq(Ratio64.one().div(Ratio64.zero()) == null, true);
    });

    test("4. @cap/num-rational — Cross-cancellation in mul prevents overflow on large fractions", () => {
        // (1_000_000_000 / 999_999_937) * (999_999_937 / 1_000_000_000) == 1/1
        if (a := Ratio64.new(1000000000, 999999937)) {
            if (b := Ratio64.new(999999937, 1000000000)) {
                const p: Ratio64 = a.mul(b);
                expect_eq(p.eq(Ratio64.one()), true);
            }
        }
    });

    test("5. @cap/num-rational — cmp & eq ordering across positive and negative fractions", () => {
        if (a := Ratio64.new(2, 3)) {
            if (b := Ratio64.new(3, 4)) {
                if (c := Ratio64.new(4, 6)) {
                    expect_eq(a.eq(c), true);
                    expect_eq(a.cmp(c), 0 as i32);
                    expect_eq(a.cmp(b), -1 as i32);
                    expect_eq(b.cmp(a), 1 as i32);
                    expect_eq(a.neg().cmp(b.neg()), 1 as i32);
                }
            }
        }
    });

    test("6. @cap/num-rational — floor, ceil & toF64 conversion", () => {
        if (pos := Ratio64.new(7, 3)) {
            expect_eq(pos.floor(), 2 as i64);
            expect_eq(pos.ceil(), 3 as i64);
        }
        if (neg := Ratio64.new(-7, 3)) {
            expect_eq(neg.floor(), -3 as i64);
            expect_eq(neg.ceil(), -2 as i64);
        }
        if (exact := Ratio64.new(-6, 3)) {
            expect_eq(exact.floor(), -2 as i64);
            expect_eq(exact.ceil(), -2 as i64);
        }
        if (q := Ratio64.new(3, 4)) {
            expect_eq(q.toF64(), 0.75 as f64);
        }
    });
}
