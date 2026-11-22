// Production deep regression tests for lib/time/timeLike.cp (cap:time)

import { test, expect_eq } from "cap:test";
import {
    TIMELIKE_NS_PER_S,
    TIMELIKE_NS_PER_MS,
    TIMELIKE_MS_PER_S,
    UTIME_NOW_LINUX,
    UTIME_OMIT_LINUX,
    TimeLikeError,
    TimeLikeSpec,
} from "cap:time";

test("timeLike: sentinels and constants", () => {
    expect_eq(TIMELIKE_NS_PER_S, 1_000_000_000);
    expect_eq(TIMELIKE_NS_PER_MS, 1_000_000);
    expect_eq(TIMELIKE_MS_PER_S, 1_000);
    expect_eq(UTIME_NOW_LINUX, 0x3fffffff);
    expect_eq(UTIME_OMIT_LINUX, 0x3ffffffe);

    const now_s = TimeLikeSpec.nowSentinel();
    expect_eq(now_s.isNow(), true);
    expect_eq(now_s.isOmit(), false);
    expect_eq(now_s.isSpecial(), true);
    expect_eq(now_s.toFloatSeconds() == null, true);
    expect_eq(now_s.toUnixMillis() == null, true);

    const omit_s = TimeLikeSpec.omitSentinel();
    expect_eq(omit_s.isNow(), false);
    expect_eq(omit_s.isOmit(), true);
    expect_eq(omit_s.isSpecial(), true);
    expect_eq(omit_s.toUnixMillis() == null, true);
});

test("timeLike: fromSeconds positive values & negative UTIME_NOW mapping", () => {
    const t0 = TimeLikeSpec.fromSeconds(0.0) catch TimeLikeSpec{};
    expect_eq(t0.sec, 0);
    expect_eq(t0.nsec, 0);
    expect_eq(t0.isSpecial(), false);

    const t1 = TimeLikeSpec.fromSeconds(1700000000.5) catch TimeLikeSpec{};
    expect_eq(t1.sec, 1700000000);
    expect_eq(t1.nsec, 500_000_000);
    expect_eq(t1.toUnixMillis() ?? 0, 1700000000500);

    // Negative seconds map to UTIME_NOW sentinel (Bun time_like.cp parity)
    const t_neg = TimeLikeSpec.fromSeconds(-5.0) catch TimeLikeSpec{};
    expect_eq(t_neg.isNow(), true);
    expect_eq(t_neg.eql(TimeLikeSpec.nowSentinel()), true);
});

test("timeLike: fromMilliseconds and pre-1970 negative floor normalization", () => {
    const pos = TimeLikeSpec.fromMilliseconds(12345.0) catch TimeLikeSpec{};
    expect_eq(pos.sec, 12);
    expect_eq(pos.nsec, 345_000_000);
    expect_eq(pos.toUnixMillis() ?? 0, 12345);

    // -250ms => sec = -1, nsec = 750_000_000 (0 <= nsec < 1e9)
    const neg = TimeLikeSpec.fromMilliseconds(-250.0) catch TimeLikeSpec{};
    expect_eq(neg.sec, -1);
    expect_eq(neg.nsec, 750_000_000);
    expect_eq(neg.toUnixMillis() ?? 0, -250);

    // Exact negative whole second: -2000ms => sec = -2, nsec = 0
    const neg_whole = TimeLikeSpec.fromMilliseconds(-2000.0) catch TimeLikeSpec{};
    expect_eq(neg_whole.sec, -2);
    expect_eq(neg_whole.nsec, 0);
    expect_eq(neg_whole.toUnixMillis() ?? 0, -2000);
});

test("timeLike: fromNumericString parsing & whitespace trimming", () => {
    const s1 = TimeLikeSpec.fromNumericString("  123456789.25  ") catch TimeLikeSpec{};
    expect_eq(s1.sec, 123456789);
    expect_eq(s1.nsec, 250_000_000);

    const s2 = TimeLikeSpec.fromNumericString("+42") catch TimeLikeSpec{};
    expect_eq(s2.sec, 42);
    expect_eq(s2.nsec, 0);

    // Negative numeric string follows Node/Bun utimes -> UTIME_NOW
    const s3 = TimeLikeSpec.fromNumericString("-1.5") catch TimeLikeSpec{};
    expect_eq(s3.isNow(), true);
});

test("timeLike: error guards for NaN, Inf, malformed strings & OutOfRange", () => {
    const zero: f64 = 0.0;
    const nan_v: f64 = zero / zero;
    const inf_v: f64 = 1.0 / zero;

    let nan_err = false;
    * = TimeLikeSpec.fromSeconds(nan_v) catch |e| {
        nan_err = (e == TimeLikeError.NonFinite);
        TimeLikeSpec{}
    };
    expect_eq(nan_err, true);

    let inf_err = false;
    * = TimeLikeSpec.fromMilliseconds(inf_v) catch |e| {
        inf_err = (e == TimeLikeError.NonFinite);
        TimeLikeSpec{}
    };
    expect_eq(inf_err, true);

    let empty_err = false;
    * = TimeLikeSpec.fromNumericString("   ") catch |e| {
        empty_err = (e == TimeLikeError.InvalidNumericString);
        TimeLikeSpec{}
    };
    expect_eq(empty_err, true);

    let bad_str_err = false;
    * = TimeLikeSpec.fromNumericString("12.34.56") catch |e| {
        bad_str_err = (e == TimeLikeError.InvalidNumericString);
        TimeLikeSpec{}
    };
    expect_eq(bad_str_err, true);

    let alpha_err = false;
    * = TimeLikeSpec.fromNumericString("NaN") catch |e| {
        alpha_err = (e == TimeLikeError.InvalidNumericString);
        TimeLikeSpec{}
    };
    expect_eq(alpha_err, true);

    let range_err = false;
    * = TimeLikeSpec.fromSeconds(1.0e20) catch |e| {
        range_err = (e == TimeLikeError.OutOfRange);
        TimeLikeSpec{}
    };
    expect_eq(range_err, true);
});
