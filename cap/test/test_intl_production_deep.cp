// Source: cp/cap/test/test_intl_production_deep.cp
// Deep Production Test Suite for cap:intl
// Covers:
//   1. NumberFormatter: US/EU grouped floats, signed ints (incl. minInt), compact K/M/B/T, ordinals (11th/12th/13th/21st), percents
//   2. ByteSize: SI & IEC formatting, case-insensitive parsing ("1.5 GiB", "256MB", "512 k", "1024"), overflow & negative guards
//   3. HumanDuration: compound duration parsing ("1h30m15s", "2d 4h", "500ms") & formatMs round-trip
//   4. RelativeTime: past/future/singular/plural & "just now"
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.intl`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    IntlError,
    NumberFormatOptions,
    NumberFormatter,
    UnitError,
    ByteSize,
    HumanDuration,
    RelativeTime,
} from "cap:text";
import cap from "cap";

fn expectStrEq(alloc: std.mem.Allocator, actual: []u8, expected: string)! {
    defer alloc.free(actual);
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn testNumberFormatter()! {
    const alloc = std.heap.page_allocator;

    // 1. Grouped integers (positive, negative, zero, minInt(i64))
    expectStrEq(alloc, NumberFormatter.formatInt(alloc, 0, ',')?, "0")?;
    expectStrEq(alloc, NumberFormatter.formatInt(alloc, 999, ',')?, "999")?;
    expectStrEq(alloc, NumberFormatter.formatInt(alloc, 1234567, ',')?, "1,234,567")?;
    expectStrEq(alloc, NumberFormatter.formatInt(alloc, -1234567, ',')?, "-1,234,567")?;
    expectStrEq(alloc, NumberFormatter.formatInt(alloc, 1234567, '.')?, "1.234.567")?;
    expectStrEq(alloc, NumberFormatter.formatInt(alloc, std.math.minInt(i64), ',')?, "-9,223,372,036,854,775,808")?;

    // 2. US & European grouped floats, carry rounding, -0.0 normalization, NaN/Inf
    const us_opts = NumberFormatOptions { thousands_sep: ',', decimal_sep: '.', decimals: 2 };
    const eu_opts = NumberFormatOptions { thousands_sep: '.', decimal_sep: ',', decimals: 2 };
    const zero_dec_opts = NumberFormatOptions { thousands_sep: ',', decimal_sep: '.', decimals: 0 };

    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, 1234.56, us_opts)?, "1,234.56")?;
    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, 1234.56, eu_opts)?, "1.234,56")?;
    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, -1234567.8, eu_opts)?, "-1.234.567,80")?;
    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, 99.996, us_opts)?, "100.00")?;
    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, -0.001, us_opts)?, "0.00")?;
    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, 1234.56, zero_dec_opts)?, "1,235")?;
    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, std.math.nan(f64), us_opts)?, "NaN")?;
    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, std.math.inf(f64), us_opts)?, "Infinity")?;
    expectStrEq(alloc, NumberFormatter.formatFloat(alloc, -(std.math.inf(f64)), us_opts)?, "-Infinity")?;

    // 3. Compact notation (K / M / B / T + tier promotion + negative)
    expectStrEq(alloc, NumberFormatter.formatCompact(alloc, 0.0)?, "0")?;
    expectStrEq(alloc, NumberFormatter.formatCompact(alloc, 42.0)?, "42")?;
    expectStrEq(alloc, NumberFormatter.formatCompact(alloc, 1200.0)?, "1.2K")?;
    expectStrEq(alloc, NumberFormatter.formatCompact(alloc, 3400000.0)?, "3.4M")?;
    expectStrEq(alloc, NumberFormatter.formatCompact(alloc, 5600000000.0)?, "5.6B")?;
    expectStrEq(alloc, NumberFormatter.formatCompact(alloc, 7800000000000.0)?, "7.8T")?;
    expectStrEq(alloc, NumberFormatter.formatCompact(alloc, -1234.0)?, "-1.2K")?;
    expectStrEq(alloc, NumberFormatter.formatCompact(alloc, 999990.0)?, "1M")?;

    // 4. English ordinals (1st, 2nd, 3rd, 4th, 11th, 12th, 13th, 21st, 112th)
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 0)?, "0th")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 1)?, "1st")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 2)?, "2nd")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 3)?, "3rd")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 4)?, "4th")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 11)?, "11th")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 12)?, "12th")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 13)?, "13th")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 21)?, "21st")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 22)?, "22nd")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 23)?, "23rd")?;
    expectStrEq(alloc, NumberFormatter.formatOrdinal(alloc, 112)?, "112th")?;

    // 5. Percent formatting
    expectStrEq(alloc, NumberFormatter.formatPercent(alloc, 0.256, 1)?, "25.6%")?;
    expectStrEq(alloc, NumberFormatter.formatPercent(alloc, 0.256, 2)?, "25.60%")?;
    expectStrEq(alloc, NumberFormatter.formatPercent(alloc, 1.0, 0)?, "100%")?;
    expectStrEq(alloc, NumberFormatter.formatPercent(alloc, -0.0525, 2)?, "-5.25%")?;
}

fn testByteSizeSiIecAndGuards()! {
    const alloc = std.heap.page_allocator;

    // 1. SI formatting (base 1000)
    expectStrEq(alloc, ByteSize.formatSi(alloc, 0)?, "0 B")?;
    expectStrEq(alloc, ByteSize.formatSi(alloc, 512)?, "512 B")?;
    expectStrEq(alloc, ByteSize.formatSi(alloc, 1500)?, "1.5 KB")?;
    expectStrEq(alloc, ByteSize.formatSi(alloc, 256000000)?, "256 MB")?;
    expectStrEq(alloc, ByteSize.formatSi(alloc, 1234567890)?, "1.23 GB")?;

    // 2. IEC formatting (base 1024)
    expectStrEq(alloc, ByteSize.formatIec(alloc, 512)?, "512 B")?;
    expectStrEq(alloc, ByteSize.formatIec(alloc, 1024)?, "1 KiB")?;
    expectStrEq(alloc, ByteSize.formatIec(alloc, 1610612736)?, "1.5 GiB")?;
    expectStrEq(alloc, ByteSize.formatIec(alloc, 268435456)?, "256 MiB")?;

    // 3. Bidirectional case-insensitive parsing
    expect_eq(ByteSize.parse("1.5 GiB")?, 1610612736 as u64);
    expect_eq(ByteSize.parse("  1.5 gib  ")?, 1610612736 as u64);
    expect_eq(ByteSize.parse("256MB")?, 256000000 as u64);
    expect_eq(ByteSize.parse("512 k")?, 512000 as u64);
    expect_eq(ByteSize.parse("512 K")?, 512000 as u64);
    expect_eq(ByteSize.parse("1024")?, 1024 as u64);
    expect_eq(ByteSize.parse("256 MiB")?, 268435456 as u64);
    expect_eq(ByteSize.parse("1024 B")?, 1024 as u64);

    // Round-trip formatIec(parse("1.5 GiB")) -> "1.5 GiB"
    const parsed_gib = ByteSize.parse("1.5 GiB")?;
    expectStrEq(alloc, ByteSize.formatIec(alloc, parsed_gib)?, "1.5 GiB")?;

    // 4. Overflow, negative & invalid format guards
    let overflow_hit = false;
    * = ByteSize.parse("16 EiB") catch |e| {
        if (e == UnitError.Overflow) overflow_hit = true;
    };
    expect_eq(overflow_hit, true);

    let overflow_num_hit = false;
    * = ByteSize.parse("18446744073709551616") catch |e| {
        if (e == UnitError.Overflow) overflow_num_hit = true;
    };
    expect_eq(overflow_num_hit, true);

    let neg_hit = false;
    * = ByteSize.parse("-10 MB") catch |e| {
        if (e == UnitError.NegativeValue) neg_hit = true;
    };
    expect_eq(neg_hit, true);

    let invalid_hit = false;
    * = ByteSize.parse("1.5 XB") catch |e| {
        if (e == UnitError.InvalidFormat) invalid_hit = true;
    };
    expect_eq(invalid_hit, true);

    let empty_hit = false;
    * = ByteSize.parse("   ") catch |e| {
        if (e == UnitError.InvalidFormat) empty_hit = true;
    };
    expect_eq(empty_hit, true);
}

fn testHumanDurationRoundTrip()! {
    const alloc = std.heap.page_allocator;

    // 1. Format milliseconds
    expectStrEq(alloc, HumanDuration.formatMs(alloc, 0)?, "0ms")?;
    expectStrEq(alloc, HumanDuration.formatMs(alloc, 450)?, "450ms")?;
    expectStrEq(alloc, HumanDuration.formatMs(alloc, 1500)?, "1s 500ms")?;
    expectStrEq(alloc, HumanDuration.formatMs(alloc, 95415000)?, "1d 2h 30m 15s")?;

    // 2. Parse compound & decimal durations
    expect_eq(HumanDuration.parseMs("1h30m15s")?, 5415000 as u64);
    expect_eq(HumanDuration.parseMs("2d 4h")?, 187200000 as u64);
    expect_eq(HumanDuration.parseMs("500ms")?, 500 as u64);
    expect_eq(HumanDuration.parseMs("1.5h")?, 5400000 as u64);
    expect_eq(HumanDuration.parseMs("1d 2h 30m 15s")?, 95415000 as u64);

    // 3. Round-trip parseMs -> formatMs
    const parsed = HumanDuration.parseMs("1d 2h 30m 15s")?;
    expectStrEq(alloc, HumanDuration.formatMs(alloc, parsed)?, "1d 2h 30m 15s")?;

    const compact_parsed = HumanDuration.parseMs("1h30m15s")?;
    expectStrEq(alloc, HumanDuration.formatMs(alloc, compact_parsed)?, "1h 30m 15s")?;

    // 4. Guards: negative, unknown unit, bare number without unit, overflow
    let neg_hit = false;
    * = HumanDuration.parseMs("-5m") catch |e| {
        if (e == UnitError.NegativeValue) neg_hit = true;
    };
    expect_eq(neg_hit, true);

    let bad_unit_hit = false;
    * = HumanDuration.parseMs("100") catch |e| {
        if (e == UnitError.InvalidFormat) bad_unit_hit = true;
    };
    expect_eq(bad_unit_hit, true);

    let overflow_hit = false;
    * = HumanDuration.parseMs("9999999999999999999d") catch |e| {
        if (e == UnitError.Overflow) overflow_hit = true;
    };
    expect_eq(overflow_hit, true);
}

fn testRelativeTime()! {
    const alloc = std.heap.page_allocator;

    // 1. "just now"
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, 0)?, "just now")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -3)?, "just now")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, 4)?, "just now")?;

    // 2. Past singular & plural
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -15)?, "15 seconds ago")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -60)?, "1 minute ago")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -300)?, "5 minutes ago")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -3600)?, "1 hour ago")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -86400)?, "1 day ago")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -259200)?, "3 days ago")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -2592000)?, "1 month ago")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, -63072000)?, "2 years ago")?;

    // 3. Future singular & plural
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, 30)?, "in 30 seconds")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, 60)?, "in 1 minute")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, 7200)?, "in 2 hours")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, 86400)?, "in 1 day")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, 5184000)?, "in 2 months")?;
    expectStrEq(alloc, RelativeTime.formatDeltaSec(alloc, 31536000)?, "in 1 year")?;

    // 4. Extreme negative value (minInt(i64)) safety
    const extreme = RelativeTime.formatDeltaSec(alloc, std.math.minInt(i64))?;
    defer alloc.free(extreme);
    expect_eq(extreme.len > 0, true);
}

fn testCapIntlDynamicLazyMount()! {
    const alloc = std.heap.page_allocator;

    const grouped = cap.text.NumberFormatter.formatInt(alloc, 9876543, ',')?;
    defer alloc.free(grouped);
    expect_eq(std.mem.eql(u8, grouped, "9,876,543"), true);

    const euro = cap.text.NumberFormatter.formatFloat(alloc, 1234.56, cap.text.NumberFormatOptions {
        thousands_sep: '.',
        decimal_sep: ',',
        decimals: 2,
    })?;
    defer alloc.free(euro);
    expect_eq(std.mem.eql(u8, euro, "1.234,56"), true);

    const bytes = cap.text.ByteSize.parse("2.5 GiB")?;
    expect_eq(bytes, 2684354560 as u64);

    const dur_ms = cap.text.HumanDuration.parseMs("1h30m")?;
    expect_eq(dur_ms, 5400000 as u64);

    const rel = cap.text.RelativeTime.formatDeltaSec(alloc, 7200)?;
    defer alloc.free(rel);
    expect_eq(std.mem.eql(u8, rel, "in 2 hours"), true);
}

fn main() {
    test("1. NumberFormatter: US/EU Grouping, Compact K/M/B/T, Ordinals & Percents", () => {
        testNumberFormatter()?;
    });

    test("2. ByteSize: SI/IEC Formatting, Case-Insensitive Parsing & Overflow Guards", () => {
        testByteSizeSiIecAndGuards()?;
    });

    test("3. HumanDuration: Compound Parsing, FormatMs Round-Trip & Error Guards", () => {
        testHumanDurationRoundTrip()?;
    });

    test("4. RelativeTime: Past/Future, Singular/Plural & Just Now", () => {
        testRelativeTime()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.intl`", () => {
        testCapIntlDynamicLazyMount()?;
    });
}
