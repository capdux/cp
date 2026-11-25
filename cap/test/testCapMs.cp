// Comprehensive Unit Test Suite for @cap/ms
// Covers: all time constants, all unit aliases (case-insensitive), decimals, negative values,
// compound nanosecond durations, short/long/humantime formatting, round-trip, overflow & buffer guards.
import {
    MsError,
    NS_PER_US,
    NS_PER_MS,
    NS_PER_SEC,
    NS_PER_MIN,
    NS_PER_HOUR,
    NS_PER_DAY,
    NS_PER_WEEK,
    MS_PER_SEC,
    MS_PER_MIN,
    MS_PER_HOUR,
    MS_PER_DAY,
    MS_PER_WEEK,
    MS_PER_YEAR,
    parseMs,
    parseDurationNs,
    formatShort,
    formatLong,
    formatDurationNs,
} from "cap:time/human";

fn assert(cond: bool) {
    if (!cond) {
        @panic("@cap/ms test assertion failed");
    }
}

fn sliceEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn assertParseMsErr(input: []const u8, expected: MsError) {
    let matched = false;
    * = parseMs(input) catch e => {
        assert(e == expected);
        matched = true;
    };
    assert(matched);
}

fn assertParseDurationNsErr(input: []const u8, expected: MsError) {
    let matched = false;
    * = parseDurationNs(input) catch e => {
        assert(e == expected);
        matched = true;
    };
    assert(matched);
}

fn testConstants() {
    assert(NS_PER_US == 1000);
    assert(NS_PER_MS == 1000000);
    assert(NS_PER_SEC == 1000000000);
    assert(NS_PER_MIN == 60000000000);
    assert(NS_PER_HOUR == 3600000000000);
    assert(NS_PER_DAY == 86400000000000);
    assert(NS_PER_WEEK == 604800000000000);

    assert(MS_PER_SEC == 1000);
    assert(MS_PER_MIN == 60000);
    assert(MS_PER_HOUR == 3600000);
    assert(MS_PER_DAY == 86400000);
    assert(MS_PER_WEEK == 604800000);
    assert(MS_PER_YEAR == 31557600000);
}

fn testParseMsAllUnitsAndAliases() {
    // Bare numbers (milliseconds)
    assert((parseMs("0") catch unreachable) == 0);
    assert((parseMs("100") catch unreachable) == 100);
    assert((parseMs("  250  ") catch unreachable) == 250);
    assert((parseMs("+42") catch unreachable) == 42);
    assert((parseMs("-100") catch unreachable) == -100);
    assert((parseMs("-0") catch unreachable) == 0);

    // Milliseconds aliases (case-insensitive)
    assert((parseMs("53ms") catch unreachable) == 53);
    assert((parseMs("53 MS") catch unreachable) == 53);
    assert((parseMs("53msec") catch unreachable) == 53);
    assert((parseMs("53msecs") catch unreachable) == 53);
    assert((parseMs("1 millisecond") catch unreachable) == 1);
    assert((parseMs("17 milliseconds") catch unreachable) == 17);
    assert((parseMs("17 MiLLiSeCoNdS") catch unreachable) == 17);

    // Seconds aliases
    assert((parseMs("1s") catch unreachable) == MS_PER_SEC);
    assert((parseMs("2sec") catch unreachable) == 2 * MS_PER_SEC);
    assert((parseMs("3 secs") catch unreachable) == 3 * MS_PER_SEC);
    assert((parseMs("1 second") catch unreachable) == MS_PER_SEC);
    assert((parseMs("5 SECONDS") catch unreachable) == 5 * MS_PER_SEC);

    // Minutes aliases
    assert((parseMs("1m") catch unreachable) == MS_PER_MIN);
    assert((parseMs("2min") catch unreachable) == 2 * MS_PER_MIN);
    assert((parseMs("3 mins") catch unreachable) == 3 * MS_PER_MIN);
    assert((parseMs("1 minute") catch unreachable) == MS_PER_MIN);
    assert((parseMs("4 Minutes") catch unreachable) == 4 * MS_PER_MIN);

    // Hours aliases
    assert((parseMs("1h") catch unreachable) == MS_PER_HOUR);
    assert((parseMs("2hr") catch unreachable) == 2 * MS_PER_HOUR);
    assert((parseMs("3 hrs") catch unreachable) == 3 * MS_PER_HOUR);
    assert((parseMs("1 hour") catch unreachable) == MS_PER_HOUR);
    assert((parseMs("10 HOURS") catch unreachable) == 10 * MS_PER_HOUR);

    // Days aliases
    assert((parseMs("1d") catch unreachable) == MS_PER_DAY);
    assert((parseMs("2 day") catch unreachable) == 2 * MS_PER_DAY);
    assert((parseMs("3 DAYS") catch unreachable) == 3 * MS_PER_DAY);

    // Weeks aliases
    assert((parseMs("1w") catch unreachable) == MS_PER_WEEK);
    assert((parseMs("2 week") catch unreachable) == 2 * MS_PER_WEEK);
    assert((parseMs("3 Weeks") catch unreachable) == 3 * MS_PER_WEEK);

    // Years aliases
    assert((parseMs("1y") catch unreachable) == MS_PER_YEAR);
    assert((parseMs("2yr") catch unreachable) == 2 * MS_PER_YEAR);
    assert((parseMs("3 yrs") catch unreachable) == 3 * MS_PER_YEAR);
    assert((parseMs("1 year") catch unreachable) == MS_PER_YEAR);
    assert((parseMs("2 YEARS") catch unreachable) == 2 * MS_PER_YEAR);
}

fn testParseMsDecimalsAndNegatives() {
    assert((parseMs("2.5 hrs") catch unreachable) == 9000000);
    assert((parseMs("1.5h") catch unreachable) == 5400000);
    assert((parseMs(".5s") catch unreachable) == 500);
    assert((parseMs("-.5s") catch unreachable) == -500);
    assert((parseMs("+.5s") catch unreachable) == 500);
    assert((parseMs("-3 days") catch unreachable) == -3 * MS_PER_DAY);
    assert((parseMs("-1.5h") catch unreachable) == -5400000);
    assert((parseMs("-10.5h") catch unreachable) == -37800000);
    assert((parseMs("1.5d") catch unreachable) == 129600000);
    assert((parseMs("0.5ms") catch unreachable) == 1);
    assert((parseMs("1.234s") catch unreachable) == 1234);
    assert((parseMs("1.2345s") catch unreachable) == 1235);
    assert((parseMs("-1.2345s") catch unreachable) == -1235);
    assert((parseMs("100.") catch unreachable) == 100);
}

fn testParseDurationNsCompoundAndUnits() {
    assert((parseDurationNs("0s") catch unreachable) == 0);
    assert((parseDurationNs("500ns") catch unreachable) == 500);
    assert((parseDurationNs("500 nsecs") catch unreachable) == 500);
    assert((parseDurationNs("1 nanosecond") catch unreachable) == 1);
    assert((parseDurationNs("10 nanoseconds") catch unreachable) == 10);

    assert((parseDurationNs("100us") catch unreachable) == 100 * NS_PER_US);
    assert((parseDurationNs("100 usec") catch unreachable) == 100 * NS_PER_US);
    assert((parseDurationNs("100 microseconds") catch unreachable) == 100 * NS_PER_US);

    // UTF-8 micro sign µs
    assert((parseDurationNs("42µs") catch unreachable) == 42 * NS_PER_US);

    assert((parseDurationNs("500ms") catch unreachable) == 500 * NS_PER_MS);
    assert((parseDurationNs("15s") catch unreachable) == 15 * NS_PER_SEC);
    assert((parseDurationNs("30m") catch unreachable) == 30 * NS_PER_MIN);
    assert((parseDurationNs("1h") catch unreachable) == NS_PER_HOUR);
    assert((parseDurationNs("2d") catch unreachable) == 2 * NS_PER_DAY);
    assert((parseDurationNs("1w") catch unreachable) == NS_PER_WEEK);
    assert((parseDurationNs("1y") catch unreachable) == u64(MS_PER_YEAR) * NS_PER_MS);

    // Multi-segment compound durations
    assert(
        (parseDurationNs("1h 30m 15s 500ms") catch unreachable) ==
            NS_PER_HOUR + 30 * NS_PER_MIN + 15 * NS_PER_SEC + 500 * NS_PER_MS
    );
    assert(
        (parseDurationNs("2d 4h") catch unreachable) ==
            2 * NS_PER_DAY + 4 * NS_PER_HOUR
    );
    // Compact (no spaces) & mixed case
    assert(
        (parseDurationNs("  1H30M15S250MS100US50NS  ") catch unreachable) ==
            NS_PER_HOUR + 30 * NS_PER_MIN + 15 * NS_PER_SEC + 250 * NS_PER_MS + 100 * NS_PER_US + 50
    );
    // Repeated units accumulate
    assert((parseDurationNs("1s 2s 3s") catch unreachable) == 6 * NS_PER_SEC);
}

fn testFormatShortAndLong() {
    let buf: [64]u8 = [0; 64];

    // formatShort
    assert(sliceEql(formatShort(0, buf[0..]) catch unreachable, "0ms"));
    assert(sliceEql(formatShort(500, buf[0..]) catch unreachable, "500ms"));
    assert(sliceEql(formatShort(-500, buf[0..]) catch unreachable, "-500ms"));
    assert(sliceEql(formatShort(1000, buf[0..]) catch unreachable, "1s"));
    assert(sliceEql(formatShort(10000, buf[0..]) catch unreachable, "10s"));
    assert(sliceEql(formatShort(-10000, buf[0..]) catch unreachable, "-10s"));
    assert(sliceEql(formatShort(60000, buf[0..]) catch unreachable, "1m"));
    assert(sliceEql(formatShort(-180000, buf[0..]) catch unreachable, "-3m"));
    assert(sliceEql(formatShort(36000000, buf[0..]) catch unreachable, "10h"));
    assert(sliceEql(formatShort(2 * MS_PER_DAY, buf[0..]) catch unreachable, "2d"));
    assert(sliceEql(formatShort(-2 * MS_PER_DAY, buf[0..]) catch unreachable, "-2d"));
    // Rounding check: 1.5s -> 2s, 1.4s -> 1s
    assert(sliceEql(formatShort(1500, buf[0..]) catch unreachable, "2s"));
    assert(sliceEql(formatShort(1499, buf[0..]) catch unreachable, "1s"));

    // formatLong (with automatic pluralization)
    assert(sliceEql(formatLong(0, buf[0..]) catch unreachable, "0 ms"));
    assert(sliceEql(formatLong(500, buf[0..]) catch unreachable, "500 ms"));
    assert(sliceEql(formatLong(-500, buf[0..]) catch unreachable, "-500 ms"));
    assert(sliceEql(formatLong(1000, buf[0..]) catch unreachable, "1 second"));
    assert(sliceEql(formatLong(1499, buf[0..]) catch unreachable, "1 second"));
    assert(sliceEql(formatLong(1500, buf[0..]) catch unreachable, "2 seconds"));
    assert(sliceEql(formatLong(-1000, buf[0..]) catch unreachable, "-1 second"));
    assert(sliceEql(formatLong(-1500, buf[0..]) catch unreachable, "-2 seconds"));
    assert(sliceEql(formatLong(MS_PER_MIN, buf[0..]) catch unreachable, "1 minute"));
    assert(sliceEql(formatLong(-3 * MS_PER_MIN, buf[0..]) catch unreachable, "-3 minutes"));
    assert(sliceEql(formatLong(MS_PER_HOUR, buf[0..]) catch unreachable, "1 hour"));
    assert(sliceEql(formatLong(2 * MS_PER_HOUR, buf[0..]) catch unreachable, "2 hours"));
    assert(sliceEql(formatLong(MS_PER_DAY, buf[0..]) catch unreachable, "1 day"));
    assert(sliceEql(formatLong(2 * MS_PER_DAY, buf[0..]) catch unreachable, "2 days"));
    assert(sliceEql(formatLong(-MS_PER_DAY, buf[0..]) catch unreachable, "-1 day"));

    // Extreme i64 min value must not overflow abs()
    const min_i64: i64 = -9223372036854775807 - 1;
    const min_short = formatShort(min_i64, buf[0..]) catch unreachable;
    assert(min_short.len > 2 && min_short[0] == '-' && min_short[min_short.len - 1] == 'd');
    const min_long = formatLong(min_i64, buf[0..]) catch unreachable;
    assert(min_long.len > 5 && min_long[0] == '-');
}

fn testFormatDurationNsAndRoundTrip() {
    let buf: [128]u8 = [0; 128];

    assert(sliceEql(formatDurationNs(0, buf[0..]) catch unreachable, "0s"));
    assert(sliceEql(formatDurationNs(500, buf[0..]) catch unreachable, "500ns"));
    assert(sliceEql(formatDurationNs(100 * NS_PER_US, buf[0..]) catch unreachable, "100us"));
    assert(sliceEql(formatDurationNs(100 * NS_PER_US + 500, buf[0..]) catch unreachable, "100us 500ns"));

    const d1 = NS_PER_HOUR + 30 * NS_PER_MIN + 15 * NS_PER_SEC;
    const s1 = formatDurationNs(d1, buf[0..]) catch unreachable;
    assert(sliceEql(s1, "1h 30m 15s"));
    assert((parseDurationNs(s1) catch unreachable) == d1);

    const d2 = 2 * NS_PER_DAY + 3 * NS_PER_HOUR + 4 * NS_PER_MIN + 5 * NS_PER_SEC + 6 * NS_PER_MS;
    const s2 = formatDurationNs(d2, buf[0..]) catch unreachable;
    assert(sliceEql(s2, "2d 3h 4m 5s 6ms"));
    assert((parseDurationNs(s2) catch unreachable) == d2);

    const d3 = 7 * NS_PER_DAY + 8 * NS_PER_HOUR + 9 * NS_PER_MIN + 10 * NS_PER_SEC + 11 * NS_PER_MS + 12 * NS_PER_US + 13;
    const s3 = formatDurationNs(d3, buf[0..]) catch unreachable;
    assert(sliceEql(s3, "7d 8h 9m 10s 11ms 12us 13ns"));
    assert((parseDurationNs(s3) catch unreachable) == d3);

    // u64::MAX exact round-trip
    const max_i64_u64: u64 = 0x7FFFFFFFFFFFFFFF;
    const max_u64: u64 = max_i64_u64 * 2 + 1;
    const s_max = formatDurationNs(max_u64, buf[0..]) catch unreachable;
    assert((parseDurationNs(s_max) catch unreachable) == max_u64);
}

fn testErrorsOverflowAndBufferGuards() {
    // Empty inputs
    assertParseMsErr("", MsError.EmptyInput);
    assertParseMsErr("   \t\n  ", MsError.EmptyInput);
    assertParseDurationNsErr("", MsError.EmptyInput);
    assertParseDurationNsErr("   ", MsError.EmptyInput);

    // Invalid formats
    assertParseMsErr("-", MsError.InvalidFormat);
    assertParseMsErr("+", MsError.InvalidFormat);
    assertParseMsErr(".", MsError.InvalidFormat);
    assertParseMsErr("-.", MsError.InvalidFormat);
    assertParseMsErr("abc", MsError.InvalidFormat);
    assertParseMsErr("10s 20s", MsError.InvalidFormat);
    assertParseMsErr("10s!", MsError.InvalidFormat);
    assertParseMsErr("1.2.3s", MsError.InvalidFormat);

    assertParseDurationNsErr("-5s", MsError.InvalidFormat);
    assertParseDurationNsErr("100", MsError.InvalidFormat);
    assertParseDurationNsErr("1h 20", MsError.InvalidFormat);
    assertParseDurationNsErr("1.5s", MsError.InvalidFormat);
    assertParseDurationNsErr("s", MsError.InvalidFormat);

    // Unknown units
    assertParseMsErr("10foo", MsError.UnknownUnit);
    assertParseMsErr("5 ns", MsError.UnknownUnit);
    assertParseMsErr("2 fortnights", MsError.UnknownUnit);
    assertParseDurationNsErr("10foo", MsError.UnknownUnit);
    assertParseDurationNsErr("1h 20bar", MsError.UnknownUnit);

    // Overflow checks
    assertParseMsErr("9223372036854775808", MsError.Overflow);
    assertParseMsErr("-9223372036854775809", MsError.Overflow);
    assertParseMsErr("999999999999999999y", MsError.Overflow);
    assertParseMsErr("99999999999999999999999999999", MsError.Overflow);
    // Exact i64 boundaries must succeed
    assert((parseMs("9223372036854775807") catch unreachable) == 9223372036854775807);
    assert((parseMs("-9223372036854775808") catch unreachable) == -9223372036854775807 - 1);

    assertParseDurationNsErr("99999999999999999999ns", MsError.Overflow);
    assertParseDurationNsErr("999999999d", MsError.Overflow);
    assertParseDurationNsErr("18446744073709551615ns 1ns", MsError.Overflow);
    // Exact u64::MAX must succeed
    const max_i64_u64: u64 = 0x7FFFFFFFFFFFFFFF;
    const max_u64: u64 = max_i64_u64 * 2 + 1;
    assert((parseDurationNs("18446744073709551615ns") catch unreachable) == max_u64);

    // BufferTooSmall checks
    let tiny1: [1]u8 = [0; 1];
    let tiny2: [2]u8 = [0; 2];
    let tiny4: [4]u8 = [0; 4];

    let got_small = false;
    * = formatShort(500, tiny2[0..]) catch e => {
        assert(e == MsError.BufferTooSmall);
        got_small = true;
    };
    assert(got_small);

    got_small = false;
    * = formatLong(2 * MS_PER_DAY, tiny4[0..]) catch e => {
        assert(e == MsError.BufferTooSmall);
        got_small = true;
    };
    assert(got_small);

    got_small = false;
    * = formatDurationNs(0, tiny1[0..0]) catch e => {
        assert(e == MsError.BufferTooSmall);
        got_small = true;
    };
    assert(got_small);

    got_small = false;
    * = formatDurationNs(NS_PER_HOUR + NS_PER_MIN, tiny4[0..]) catch e => {
        assert(e == MsError.BufferTooSmall);
        got_small = true;
    };
    assert(got_small);
}

pub fn main() {
    testConstants();
    testParseMsAllUnitsAndAliases();
    testParseMsDecimalsAndNegatives();
    testParseDurationNsCompoundAndUnits();
    testFormatShortAndLong();
    testFormatDurationNsAndRoundTrip();
    testErrorsOverflowAndBufferGuards();
}
