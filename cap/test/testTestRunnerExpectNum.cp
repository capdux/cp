// cap/test/testTestRunnerExpectNum.cp
// Production-grade deep tests for cap:test_runner expectNum.cp (IEEE-754 numeric & relational matchers)

import { test, assert, assertEq } from "cap:test";
import {
    F64_SIGN_MASK,
    F64_EXP_MASK,
    F64_MAX_SAFE_INT,
    isNaNF64,
    isInfiniteF64,
    isFiniteF64,
    isNegativeZeroF64,
    isPositiveF64,
    isNegativeF64,
    isIntegerF64,
    isSafeIntegerF64,
    isSameValueF64,
    isGreaterThanF64,
    isGreaterThanOrEqualF64,
    isLessThanF64,
    isLessThanOrEqualF64,
    isGreaterThanI64,
    isGreaterThanOrEqualI64,
    isLessThanI64,
    isLessThanOrEqualI64,
    ExpectNumMatcher,
} from "cap:test_runner";

fn makePosInf(): f64 {
    return @as(f64, @bitCast(F64_EXP_MASK));
}

fn makeNegInf(): f64 {
    return @as(f64, @bitCast(F64_SIGN_MASK | F64_EXP_MASK));
}

fn makeNaN(): f64 {
    return @as(f64, @bitCast(F64_EXP_MASK | @as(u64, 1)));
}

fn makeNegZero(): f64 {
    return @as(f64, @bitCast(F64_SIGN_MASK));
}

test("ExpectNum: isFiniteF64 / isInfiniteF64 / isNaNF64 / isNegativeZeroF64", () => {
    const pos_inf = makePosInf();
    const neg_inf = makeNegInf();
    const nan_val = makeNaN();
    const neg_zero = makeNegZero();

    assert(isFiniteF64(0.0));
    assert(isFiniteF64(-0.0));
    assert(isFiniteF64(42.5));
    assert(isFiniteF64(-1e100));
    assert(!isFiniteF64(pos_inf));
    assert(!isFiniteF64(neg_inf));
    assert(!isFiniteF64(nan_val));

    assert(isInfiniteF64(pos_inf));
    assert(isInfiniteF64(neg_inf));
    assert(!isInfiniteF64(nan_val));
    assert(!isInfiniteF64(1.0));

    assert(isNaNF64(nan_val));
    assert(!isNaNF64(pos_inf));
    assert(!isNaNF64(0.0));

    assert(isNegativeZeroF64(neg_zero));
    assert(!isNegativeZeroF64(0.0));
    assert(!isNegativeZeroF64(-1.0));
});

test("ExpectNum: isPositiveF64 & isNegativeF64 strict finite semantics", () => {
    const pos_inf = makePosInf();
    const neg_inf = makeNegInf();
    const nan_val = makeNaN();
    const neg_zero = makeNegZero();

    assert(isPositiveF64(0.0001));
    assert(isPositiveF64(1e300));
    assert(!isPositiveF64(0.0));
    assert(!isPositiveF64(neg_zero));
    assert(!isPositiveF64(-1.0));
    assert(!isPositiveF64(pos_inf));
    assert(!isPositiveF64(nan_val));

    assert(isNegativeF64(-0.0001));
    assert(isNegativeF64(-1e300));
    assert(!isNegativeF64(0.0));
    assert(!isNegativeF64(neg_zero));
    assert(!isNegativeF64(1.0));
    assert(!isNegativeF64(neg_inf));
    assert(!isNegativeF64(nan_val));
});

test("ExpectNum: isIntegerF64 & isSafeIntegerF64 boundary precision", () => {
    const pos_inf = makePosInf();
    const nan_val = makeNaN();
    const neg_zero = makeNegZero();

    assert(isIntegerF64(0.0));
    assert(isIntegerF64(neg_zero));
    assert(isIntegerF64(1.0));
    assert(isIntegerF64(-42.0));
    assert(isIntegerF64(F64_MAX_SAFE_INT));
    assert(isIntegerF64(-F64_MAX_SAFE_INT));
    assert(isIntegerF64(9007199254740992.0)); // 2^53 is an integer, but not safe

    assert(!isIntegerF64(0.5));
    assert(!isIntegerF64(-0.25));
    assert(!isIntegerF64(3.14159));
    assert(!isIntegerF64(pos_inf));
    assert(!isIntegerF64(nan_val));

    assert(isSafeIntegerF64(0.0));
    assert(isSafeIntegerF64(F64_MAX_SAFE_INT));
    assert(isSafeIntegerF64(-F64_MAX_SAFE_INT));
    assert(!isSafeIntegerF64(9007199254740992.0));
    assert(!isSafeIntegerF64(-9007199254740992.0));
    assert(!isSafeIntegerF64(1.5));
});

test("ExpectNum: isSameValueF64 Object.is semantics (NaN == NaN, +0 != -0)", () => {
    const nan1 = makeNaN();
    const nan2 = @as(f64, @bitCast(F64_EXP_MASK | @as(u64, 0x1234)));
    const neg_zero = makeNegZero();

    assert(isSameValueF64(nan1, nan2));
    assert(!isSameValueF64(0.0, neg_zero));
    assert(isSameValueF64(neg_zero, neg_zero));
    assert(isSameValueF64(0.0, 0.0));
    assert(isSameValueF64(3.5, 3.5));
    assert(!isSameValueF64(3.5, 3.6));
    assert(isSameValueF64(makePosInf(), makePosInf()));
    assert(!isSameValueF64(makePosInf(), makeNegInf()));
});

test("ExpectNum: relational comparisons with NaN guard & i64 helpers", () => {
    const nan_val = makeNaN();

    assert(isGreaterThanF64(5.0, 4.9));
    assert(!isGreaterThanF64(5.0, 5.0));
    assert(!isGreaterThanF64(nan_val, 0.0));
    assert(!isGreaterThanF64(0.0, nan_val));

    assert(isGreaterThanOrEqualF64(5.0, 5.0));
    assert(isGreaterThanOrEqualF64(5.1, 5.0));
    assert(!isGreaterThanOrEqualF64(nan_val, nan_val));

    assert(isLessThanF64(-2.0, -1.5));
    assert(!isLessThanF64(1.0, 1.0));
    assert(!isLessThanF64(nan_val, 1.0));

    assert(isLessThanOrEqualF64(1.0, 1.0));
    assert(isLessThanOrEqualF64(0.9, 1.0));
    assert(!isLessThanOrEqualF64(1.0, nan_val));

    assert(isGreaterThanI64(10, -5));
    assert(isGreaterThanOrEqualI64(10, 10));
    assert(isLessThanI64(-10, 0));
    assert(isLessThanOrEqualI64(-10, -10));
});

test("ExpectNum: ExpectNumMatcher stateful .not negation & counters", () => {
    let m = ExpectNumMatcher.init();

    assert(m.toBeFinite(123.0));
    assert(m.toBeNaN(makeNaN()));
    assert(m.toBePositive(7.5));
    assert(m.toBeNegative(-7.5));
    assert(m.toBeInteger(100.0));
    assert(m.toBeSafeInteger(F64_MAX_SAFE_INT));
    assert(m.toBeSameValue(makeNaN(), makeNaN()));
    assert(m.toBeGreaterThan(10.0, 5.0));
    assert(m.toBeGreaterThanOrEqual(5.0, 5.0));
    assert(m.toBeLessThan(2.0, 3.0));
    assert(m.toBeLessThanOrEqual(3.0, 3.0));
    assert(m.toBeGreaterThanI64(99, 1));
    assert(m.toBeLessThanI64(-99, -1));

    // Single-shot .not negation
    m.negate();
    assert(m.toBeFinite(makePosInf()));
    // Negation automatically resets after one assertion
    assert(!m.toBeFinite(makeNegInf()));

    assertEq(m.assertions, @as(usize, 15));
    assertEq(m.passed, @as(usize, 14));
    assertEq(m.failed, @as(usize, 1));
});
