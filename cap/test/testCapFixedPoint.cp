// cap/test/testCapFixedPoint.cp
// Comprehensive unit tests for @cap/fixed-point (Q32.32 deterministic fixed-point)

import { test, expect_eq } from "cap:test";
import { Fixed32 } from "../../packages/fixed/src/index.cp";

fn approxF64(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/fixed-point — constructors & negative floor/ceil/round/toInt boundaries", () => {
        const pos = Fixed32.fromRatio(3, 2); // +1.5
        expect_eq(pos.toInt(), 1);
        expect_eq(pos.floor(), 1);
        expect_eq(pos.ceil(), 2);
        expect_eq(pos.round(), 2);

        const neg = Fixed32.fromRatio(-3, 2); // -1.5
        expect_eq(neg.toInt(), -1);
        expect_eq(neg.floor(), -2);
        expect_eq(neg.ceil(), -1);
        expect_eq(neg.round(), -2);

        const exact_neg = Fixed32.fromInt(-2);
        expect_eq(exact_neg.floor(), -2);
        expect_eq(exact_neg.ceil(), -2);
        expect_eq(exact_neg.round(), -2);

        const from_f = Fixed32.fromF64(2.25);
        expect_eq(approxF64(from_f.toF64(), 2.25, 0.000001), true);
        expect_eq(Fixed32.fromRaw(from_f.rawValue()).cmp(from_f), 0);
    });

    test("2. @cap/fixed-point — add, sub, neg, abs & large-value split mul without i64 overflow", () => {
        const a = Fixed32.fromRatio(5, 2); // 2.5
        const b = Fixed32.fromRatio(-7, 4); // -1.75
        expect_eq(approxF64(a.add(b).toF64(), 0.75, 0.000001), true);
        expect_eq(approxF64(a.sub(b).toF64(), 4.25, 0.000001), true);
        expect_eq(approxF64(b.abs().toF64(), 1.75, 0.000001), true);
        expect_eq(approxF64(a.neg().toF64(), -2.5, 0.000001), true);

        // Large multiplication: 10000.5 * 10000.5 = 100010000.25 (would overflow naive i64 * i64!)
        const big = Fixed32.fromRatio(20001, 2);
        const sq = big.mul(big);
        expect_eq(approxF64(sq.toF64(), 100010000.25, 0.0001), true);
    });

    test("3. @cap/fixed-point — exact Q32.32 division & division-by-zero protection", () => {
        const ten = Fixed32.fromInt(10);
        const four = Fixed32.fromInt(4);
        const neg_three = Fixed32.fromInt(-3);

        expect_eq(approxF64(ten.div(four).toF64(), 2.5, 0.000001), true);
        expect_eq(approxF64(Fixed32.fromInt(1).div(neg_three).toF64(), -0.333333333, 0.000001), true);

        // Division by zero safely returns 0
        const zero = Fixed32.fromInt(0);
        expect_eq(ten.div(zero).rawValue(), 0 as i64);
        expect_eq(Fixed32.fromRatio(5, 0).rawValue(), 0 as i64);
    });

    test("4. @cap/fixed-point — deterministic Newton sqrt(2), sqrt(9), sqrt(0.25) & negative guard", () => {
        const s9: Fixed32 = Fixed32.fromInt(9).sqrt();
        expect_eq(s9.toInt(), 3);
        expect_eq(approxF64(s9.toF64(), 3.0, 0.000001), true);

        const s2: Fixed32 = Fixed32.fromInt(2).sqrt();
        expect_eq(approxF64(s2.toF64(), 1.414213562, 0.000001), true);

        const s_quarter: Fixed32 = Fixed32.fromRatio(1, 4).sqrt();
        expect_eq(approxF64(s_quarter.toF64(), 0.5, 0.000001), true);

        expect_eq(Fixed32.fromInt(0).sqrt().rawValue(), 0 as i64);
        expect_eq(Fixed32.fromInt(-4).sqrt().rawValue(), 0 as i64);
    });

    test("5. @cap/fixed-point — lerp & clamp & cmp", () => {
        const lo = Fixed32.fromInt(10);
        const hi = Fixed32.fromInt(20);
        const t = Fixed32.fromRatio(1, 4); // 0.25

        const mid = lo.lerp(hi, t); // 10 + 10*0.25 = 12.5
        expect_eq(approxF64(mid.toF64(), 12.5, 0.000001), true);

        expect_eq(Fixed32.fromInt(5).clamp(lo, hi).cmp(lo), 0);
        expect_eq(Fixed32.fromInt(15).clamp(lo, hi).toInt(), 15);
        expect_eq(Fixed32.fromInt(25).clamp(lo, hi).cmp(hi), 0);
        expect_eq(lo.cmp(hi), -1);
        expect_eq(hi.cmp(lo), 1);
    });

    test("6. @cap/fixed-point — deterministic multi-step physics kinematic formula", () => {
        // x(t) = x0 + v0 * t + 0.5 * a * t^2 with x0=10, v0=3.5, a=-9.8, t=2.0
        // exact = 10 + 7.0 - 19.6 = -2.6
        const x0 = Fixed32.fromInt(10);
        const v0 = Fixed32.fromRatio(7, 2);
        const acc = Fixed32.fromRatio(-98, 10);
        const t = Fixed32.fromInt(2);
        const half = Fixed32.fromRatio(1, 2);

        const pos1: Fixed32 = x0.add(v0.mul(t)).add(half.mul(acc).mul(t.mul(t)));
        const pos2: Fixed32 = x0.add(v0.mul(t)).add(half.mul(acc).mul(t.mul(t)));
        // Bit-exact lockstep determinism
        expect_eq(pos1.rawValue(), pos2.rawValue());
        expect_eq(approxF64(pos1.toF64(), -2.6, 0.00001), true);
    });
}
