// Source: cp/cap/test/test_duration_parse_production_deep.cp (Production deep tests for cap:duration_parse)

import { test, expect_eq } from "cap:test";
import {
    NS_PER_US,
    NS_PER_MS,
    NS_PER_SEC,
    NS_PER_MIN,
    NS_PER_HOUR,
    NS_PER_DAY,
    NS_PER_WEEK,
    parseDurationNs,
    parseDurationMs,
    formatDurationNs,
    formatDurationCompact,
} from "cap:time";

test("duration_parse: single units including us/µs, decimals and zero") {
    expect_eq(parseDurationNs("0").?, 0);
    expect_eq(parseDurationNs("42ns").?, 42);
    expect_eq(parseDurationNs("250us").?, 250 * NS_PER_US);
    expect_eq(parseDurationNs("250µs").?, 250 * NS_PER_US);
    expect_eq(parseDurationNs("500ms").?, 500 * NS_PER_MS);
    expect_eq(parseDurationNs("1.5s").?, 1500 * NS_PER_MS);
    expect_eq(parseDurationNs("0.25h").?, 15 * NS_PER_MIN);
    expect_eq(parseDurationNs("1w").?, NS_PER_WEEK);

    expect_eq(parseDurationMs("1.5s").?, 1500);
    expect_eq(parseDurationMs("500us").?, 0);
}

test("duration_parse: compound multi-segment durations with optional whitespace") {
    const t1 = parseDurationNs("1h30m15s").?;
    expect_eq(t1, NS_PER_HOUR + 30 * NS_PER_MIN + 15 * NS_PER_SEC);

    const t2 = parseDurationNs("  2d 4h 500ms  ").?;
    expect_eq(t2, 2 * NS_PER_DAY + 4 * NS_PER_HOUR + 500 * NS_PER_MS);

    const t3 = parseDurationNs("1w2d3h4m5s6ms7us8ns").?;
    const expected = NS_PER_WEEK + 2 * NS_PER_DAY + 3 * NS_PER_HOUR +
        4 * NS_PER_MIN + 5 * NS_PER_SEC + 6 * NS_PER_MS + 7 * NS_PER_US + 8;
    expect_eq(t3, expected);
}

test("duration_parse: formatDurationNs canonical formatting and exact round-trip") {
    let buf: [64]u8 = [0; 64];

    expect_eq(std.mem.eql(u8, formatDurationNs(0, buf[0..]).?, "0s"), true);
    expect_eq(std.mem.eql(u8, formatDurationNs(1500000000, buf[0..]).?, "1s500ms"), true);
    expect_eq(std.mem.eql(u8, formatDurationNs(5400000000000, buf[0..]).?, "1h30m"), true);

    const complex: u64 = (NS_PER_DAY * 2) + (NS_PER_HOUR * 3) + (NS_PER_MS * 12) + 9;
    const formatted = formatDurationNs(complex, buf[0..]).?;
    expect_eq(std.mem.eql(u8, formatted, "2d3h12ms9ns"), true);
    expect_eq(parseDurationNs(formatted).?, complex);
}

test("duration_parse: formatDurationCompact single primary unit output") {
    let buf: [64]u8 = [0; 64];

    expect_eq(std.mem.eql(u8, formatDurationCompact(0, buf[0..]).?, "0s"), true);
    expect_eq(std.mem.eql(u8, formatDurationCompact(750, buf[0..]).?, "750ns"), true);
    expect_eq(std.mem.eql(u8, formatDurationCompact(42 * NS_PER_US, buf[0..]).?, "42us"), true);
    expect_eq(std.mem.eql(u8, formatDurationCompact(250 * NS_PER_MS, buf[0..]).?, "250ms"), true);
    expect_eq(std.mem.eql(u8, formatDurationCompact(1500 * NS_PER_MS, buf[0..]).?, "1.50s"), true);
    expect_eq(std.mem.eql(u8, formatDurationCompact(90 * NS_PER_SEC, buf[0..]).?, "1.50m"), true);
    expect_eq(std.mem.eql(u8, formatDurationCompact(5400 * NS_PER_SEC, buf[0..]).?, "1.50h"), true);
}

test("duration_parse: invalid inputs, negatives, overflow and buffer guards") {
    expect_eq(parseDurationNs("") == null, true);
    expect_eq(parseDurationNs("   ") == null, true);
    expect_eq(parseDurationNs("-5s") == null, true);
    expect_eq(parseDurationNs("1h-30m") == null, true);
    expect_eq(parseDurationNs("100") == null, true);
    expect_eq(parseDurationNs("10years") == null, true);
    expect_eq(parseDurationNs("ms") == null, true);
    expect_eq(parseDurationNs("999999999999w") == null, true);
    expect_eq(parseDurationMs("bad") == null, true);

    let tiny: [2]u8 = [0; 2];
    expect_eq(formatDurationNs(1500000000, tiny[0..]) == null, true);
    expect_eq(formatDurationCompact(1500000000, tiny[0..]) == null, true);
}
