// Source: cp/cap/test/test_cron_production_deep.cp
// Deep Production Verification Suite for cap:cron & cap.cron
// Covers:
//   1. 5/6-field parsing, @daily/@hourly/@yearly/@monthly/@weekly aliases, JAN-DEC & SUN-SAT & 7=SUN
//   2. nextAfter / prevBefore / nextN across month, year, and leap-year 2024-02-29 boundaries
//   3. Vixie Cron dom/dow OR semantics (both non-wildcard) vs AND semantics (one wildcard)
//   4. Invalid field count, out-of-range values, step=0, empty expr, and unreachable date (0 0 31 2 *) NoMatchingTime
//   5. `import cap from "cap"; cap.cron` dynamic mounting & std.testing.allocator zero-leak verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CronError,
    CronSchedule,
    decomposeUtc,
    toEpochSec,
} from "cap:cron";

test("1. 5/6-field parsing, @aliases, JAN-DEC & SUN-SAT & 7=SUN") {
    // 5-field: seconds fixed to 0
    const s5 = CronSchedule.parse("15 10 * jan,MAR sun,7") catch unreachable;
    expect_eq(s5.has_seconds, false);
    expect_eq(s5.seconds, 1);
    expect_eq(s5.matches(toEpochSec(2024, 1, 7, 10, 15, 0)), true);   // 2024-01-07 is Sunday
    expect_eq(s5.matches(toEpochSec(2024, 1, 7, 10, 15, 1)), false);  // sec=1 fails for 5-field
    expect_eq(s5.matches(toEpochSec(2024, 3, 3, 10, 15, 0)), true);   // 2024-03-03 is Sunday

    // 6-field with step and range
    const s6 = CronSchedule.parse("*/20 5-10/2 8 * * MON-FRI") catch unreachable;
    expect_eq(s6.has_seconds, true);
    expect_eq(s6.matches(toEpochSec(2024, 1, 1, 8, 5, 0)), true);     // Mon, 08:05:00
    expect_eq(s6.matches(toEpochSec(2024, 1, 1, 8, 7, 40)), true);    // Mon, 08:07:40
    expect_eq(s6.matches(toEpochSec(2024, 1, 1, 8, 6, 20)), false);   // minute 6 not in 5-10/2
    expect_eq(s6.matches(toEpochSec(2024, 1, 7, 8, 5, 0)), false);    // Sunday not in MON-FRI

    // Aliases
    const daily = CronSchedule.parse("@daily") catch unreachable;
    expect_eq(daily.matches(toEpochSec(2024, 6, 15, 0, 0, 0)), true);
    expect_eq(daily.matches(toEpochSec(2024, 6, 15, 0, 1, 0)), false);

    const hourly = CronSchedule.parse("@hourly") catch unreachable;
    expect_eq(hourly.matches(toEpochSec(2024, 6, 15, 14, 0, 0)), true);
    expect_eq(hourly.matches(toEpochSec(2024, 6, 15, 14, 30, 0)), false);

    const yearly = CronSchedule.parse("@yearly") catch unreachable;
    expect_eq(yearly.matches(toEpochSec(2025, 1, 1, 0, 0, 0)), true);
    expect_eq(yearly.matches(toEpochSec(2025, 1, 2, 0, 0, 0)), false);

    const monthly = CronSchedule.parse("@monthly") catch unreachable;
    expect_eq(monthly.matches(toEpochSec(2025, 4, 1, 0, 0, 0)), true);

    const weekly = CronSchedule.parse("@weekly") catch unreachable;
    expect_eq(weekly.matches(toEpochSec(2024, 1, 7, 0, 0, 0)), true);
}

test("2. nextAfter / prevBefore / nextN across month, year, and leap-year 2024-02-29") {
    // Leap year 2024-02-29 trigger
    const leap_sched = CronSchedule.parse("0 12 29 2 *") catch unreachable;
    const before_leap = toEpochSec(2024, 2, 28, 12, 0, 0);
    const hit_2024 = leap_sched.nextAfter(before_leap) catch unreachable;
    expect_eq(hit_2024, toEpochSec(2024, 2, 29, 12, 0, 0));

    // Next Feb 29 after 2024-02-29 12:00:00 is 2028-02-29 12:00:00
    const hit_2028 = leap_sched.nextAfter(hit_2024) catch unreachable;
    expect_eq(hit_2028, toEpochSec(2028, 2, 29, 12, 0, 0));

    // prevBefore from 2028-02-29 12:00:00 returns 2024-02-29 12:00:00
    const back_2024 = leap_sched.prevBefore(hit_2028) catch unreachable;
    expect_eq(back_2024, hit_2024);

    // Cross-year boundary: 2024-12-31 23:59:30 -> 2025-01-01 00:00:00
    const min_sched = CronSchedule.parse("0 0 * * *") catch unreachable;
    const nye = toEpochSec(2024, 12, 31, 23, 59, 30);
    const new_year = min_sched.nextAfter(nye) catch unreachable;
    expect_eq(new_year, toEpochSec(2025, 1, 1, 0, 0, 0));
    expect_eq(min_sched.prevBefore(new_year) catch unreachable, toEpochSec(2024, 12, 31, 0, 0, 0));

    // 6-field nextN across minute boundary
    const sec_sched = CronSchedule.parse("0,30 59 23 31 12 *") catch unreachable;
    const list = sec_sched.nextN(std.heap.page_allocator, toEpochSec(2024, 12, 31, 23, 58, 0), 3) catch unreachable;
    defer std.heap.page_allocator.free(list);
    expect_eq(list.len, 3);
    expect_eq(list[0], toEpochSec(2024, 12, 31, 23, 59, 0));
    expect_eq(list[1], toEpochSec(2024, 12, 31, 23, 59, 30));
    expect_eq(list[2], toEpochSec(2025, 12, 31, 23, 59, 0));

    const dt = decomposeUtc(hit_2024);
    expect_eq(dt.year, 2024);
    expect_eq(dt.month, 2);
    expect_eq(dt.day, 29);
    expect_eq(dt.dow, 4);
}

test("3. Vixie Cron dom/dow OR semantics vs AND semantics") {
    // Both dom=15 and dow=1 (Monday) are non-wildcard -> OR semantics
    const or_sched = CronSchedule.parse("0 0 15 * 1") catch unreachable;
    expect_eq(or_sched.dom_wildcard, false);
    expect_eq(or_sched.dow_wildcard, false);
    // 2024-01-01 is Monday (not 15th) -> matches via dow
    expect_eq(or_sched.matches(toEpochSec(2024, 1, 1, 0, 0, 0)), true);
    // 2024-02-15 is Thursday (15th) -> matches via dom
    expect_eq(or_sched.matches(toEpochSec(2024, 2, 15, 0, 0, 0)), true);
    // 2024-02-14 is Wednesday (14th) -> neither -> false
    expect_eq(or_sched.matches(toEpochSec(2024, 2, 14, 0, 0, 0)), false);

    // Only dom=15, dow=* -> AND semantics (only 15th)
    const dom_only = CronSchedule.parse("0 0 15 * *") catch unreachable;
    expect_eq(dom_only.dom_wildcard, false);
    expect_eq(dom_only.dow_wildcard, true);
    expect_eq(dom_only.matches(toEpochSec(2024, 1, 1, 0, 0, 0)), false);
    expect_eq(dom_only.matches(toEpochSec(2024, 2, 15, 0, 0, 0)), true);

    // Only dow=1, dom=* -> AND semantics (only Monday)
    const dow_only = CronSchedule.parse("0 0 * * 1") catch unreachable;
    expect_eq(dow_only.dom_wildcard, true);
    expect_eq(dow_only.dow_wildcard, false);
    expect_eq(dow_only.matches(toEpochSec(2024, 1, 1, 0, 0, 0)), true);
    expect_eq(dow_only.matches(toEpochSec(2024, 2, 15, 0, 0, 0)), false);
}

test("4. Error guards & unreachable date NoMatchingTime") {
    let empty_err = false;
    * = CronSchedule.parse("   ") catch |e| {
        empty_err = e == CronError.EmptyExpression;
    };
    expect_eq(empty_err, true);

    let count_err = false;
    * = CronSchedule.parse("* * *") catch |e| {
        count_err = e == CronError.InvalidFieldCount;
    };
    expect_eq(count_err, true);

    let count_err7 = false;
    * = CronSchedule.parse("* * * * * * *") catch |e| {
        count_err7 = e == CronError.InvalidFieldCount;
    };
    expect_eq(count_err7, true);

    let step_err = false;
    * = CronSchedule.parse("*/0 * * * *") catch |e| {
        step_err = e == CronError.InvalidStep;
    };
    expect_eq(step_err, true);

    let range_err = false;
    * = CronSchedule.parse("30-10 * * * *") catch |e| {
        range_err = e == CronError.InvalidRange;
    };
    expect_eq(range_err, true);

    let val_err = false;
    * = CronSchedule.parse("60 * * * *") catch |e| {
        val_err = e == CronError.InvalidFieldValue;
    };
    expect_eq(val_err, true);

    // Feb 31 never exists -> NoMatchingTime
    const impossible = CronSchedule.parse("0 0 31 2 *") catch unreachable;
    let no_match = false;
    * = impossible.nextAfter(toEpochSec(2024, 1, 1, 0, 0, 0)) catch |e| {
        no_match = e == CronError.NoMatchingTime;
    };
    expect_eq(no_match, true);

    let no_match_prev = false;
    * = impossible.prevBefore(toEpochSec(2024, 1, 1, 0, 0, 0)) catch |e| {
        no_match_prev = e == CronError.NoMatchingTime;
    };
    expect_eq(no_match_prev, true);
}

test("5. cap.cron dynamic mounting & std.testing.allocator zero-leak check") {
    const alloc = std.testing.allocator;
    const sched = cap.cron.CronSchedule.parse("0 0/30 * * * *") catch unreachable;
    const start = cap.cron.toEpochSec(2024, 5, 1, 12, 0, 0);
    const triggers = sched.nextN(alloc, start, 4) catch unreachable;
    defer alloc.free(triggers);

    expect_eq(triggers.len, 4);
    expect_eq(triggers[0], cap.cron.toEpochSec(2024, 5, 1, 12, 30, 0));
    expect_eq(triggers[1], cap.cron.toEpochSec(2024, 5, 1, 13, 0, 0));
    expect_eq(triggers[2], cap.cron.toEpochSec(2024, 5, 1, 13, 30, 0));
    expect_eq(triggers[3], cap.cron.toEpochSec(2024, 5, 1, 14, 0, 0));
}
