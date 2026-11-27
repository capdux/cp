import { test, expect_eq } from "cap:test";
import {
    IntPredicate,
    StrPredicate,
    FloatPredicate,
} from "../../packages/predicates/src/index.cp";

fn main() {
    test("1. @cap/predicates — IntPredicate basic comparisons & Always/Never", () => {
        const p_eq = IntPredicate.eq(42);
        const p_ne = IntPredicate.ne(42);
        const p_lt = IntPredicate.lt(10);
        const p_le = IntPredicate.le(10);
        const p_gt = IntPredicate.gt(10);
        const p_ge = IntPredicate.ge(10);
        const p_rng = IntPredicate.inRange(-5, 5);
        const p_all = IntPredicate.always();
        const p_none = IntPredicate.never();

        expect_eq(p_eq.eval(42), true);
        expect_eq(p_eq.eval(41), false);
        expect_eq(p_ne.eval(41), true);
        expect_eq(p_lt.eval(9), true);
        expect_eq(p_lt.eval(10), false);
        expect_eq(p_le.eval(10), true);
        expect_eq(p_gt.eval(11), true);
        expect_eq(p_ge.eval(10), true);
        expect_eq(p_rng.eval(-5), true);
        expect_eq(p_rng.eval(5), true);
        expect_eq(p_rng.eval(6), false);
        expect_eq(p_all.eval(999), true);
        expect_eq(p_none.eval(0), false);
    });

    test("2. @cap/predicates — IntPredicate boolean combinators (andPred / orPred / notPred)", () => {
        // (x >= 10 && x <= 20) || x == 99
        const p_ge10 = IntPredicate.ge(10);
        const p_le20 = IntPredicate.le(20);
        const p_eq99 = IntPredicate.eq(99);
        const band = p_ge10.andPred(p_le20);
        const combined = band.orPred(p_eq99);
        const negated = combined.notPred();

        expect_eq(combined.eval(15), true);
        expect_eq(combined.eval(99), true);
        expect_eq(combined.eval(25), false);
        expect_eq(negated.eval(25), true);
        expect_eq(negated.eval(15), false);
    });

    test("3. @cap/predicates — StrPredicate Eq, Contains, HasPrefix, HasSuffix, IsEmpty", () => {
        const p_eq = StrPredicate.eq("hello");
        const p_con = StrPredicate.contains("world");
        const p_pre = StrPredicate.hasPrefix("cap:");
        const p_suf = StrPredicate.hasSuffix(".cp");
        const p_emp = StrPredicate.isEmpty();

        expect_eq(p_eq.eval("hello"), true);
        expect_eq(p_eq.eval("Hello"), false);
        expect_eq(p_con.eval("hello world!"), true);
        expect_eq(p_con.eval("hello"), false);
        expect_eq(p_pre.eval("cap:test"), true);
        expect_eq(p_pre.eval("std:test"), false);
        expect_eq(p_suf.eval("index.cp"), true);
        expect_eq(p_suf.eval("index.rs"), false);
        expect_eq(p_emp.eval(""), true);
        expect_eq(p_emp.eval("a"), false);
    });

    test("4. @cap/predicates — StrPredicate LenEq, LenBetween, EqIgnoreCase & combinators", () => {
        const p_len = StrPredicate.lenEq(4);
        const p_bet = StrPredicate.lenBetween(2, 5);
        const p_ic = StrPredicate.eqIgnoreCase("Hello");

        expect_eq(p_len.eval("abcd"), true);
        expect_eq(p_len.eval("abc"), false);
        expect_eq(p_bet.eval("ab"), true);
        expect_eq(p_bet.eval("abcdef"), false);
        expect_eq(p_ic.eval("hElLo"), true);
        expect_eq(p_ic.eval("world"), false);

        // starts with "pkg-" AND ends with ".cp" AND NOT empty
        const p1 = StrPredicate.hasPrefix("pkg-");
        const p2 = StrPredicate.hasSuffix(".cp");
        const p3 = StrPredicate.isEmpty();
        const p12 = p1.andPred(p2);
        const p3n = p3.notPred();
        const rule = p12.andPred(p3n);
        expect_eq(rule.eval("pkg-foo.cp"), true);
        expect_eq(rule.eval("pkg-foo.rs"), false);
        expect_eq(rule.eval(""), false);
    });

    test("5. @cap/predicates — FloatPredicate ApproxEq, InRange & IsFinite", () => {
        const p_approx = FloatPredicate.approxEq(3.14159, 0.001);
        const p_rng = FloatPredicate.inRange(-1.0, 1.0);
        const p_fin = FloatPredicate.isFinite();

        expect_eq(p_approx.eval(3.1416), true);
        expect_eq(p_approx.eval(3.15), false);
        expect_eq(p_rng.eval(0.5), true);
        expect_eq(p_rng.eval(1.5), false);
        expect_eq(p_fin.eval(42.0), true);

        // Positive infinity (0x7FF0000000000000) & NaN (0x7FF8000000000000)
        const inf_bits: u64 = (0x7FF00000 as u64) << 32;
        const nan_bits: u64 = (0x7FF80000 as u64) << 32;
        const pos_inf = @as(f64, @bitCast(inf_bits));
        const quiet_nan = @as(f64, @bitCast(nan_bits));

        expect_eq(p_fin.eval(pos_inf), false);
        expect_eq(p_fin.eval(quiet_nan), false);
        expect_eq(p_approx.eval(quiet_nan), false);
        expect_eq(p_rng.eval(pos_inf), false);
    });

    test("6. @cap/predicates — FloatPredicate boolean composition", () => {
        const p_lo = FloatPredicate.inRange(0.0, 1.0);
        const p_hi = FloatPredicate.approxEq(10.0, 0.5);
        const p_either = p_lo.orPred(p_hi);
        const p_neither = p_either.notPred();

        expect_eq(p_either.eval(0.25), true);
        expect_eq(p_either.eval(9.75), true);
        expect_eq(p_either.eval(5.0), false);
        expect_eq(p_neither.eval(5.0), true);
    });
}
