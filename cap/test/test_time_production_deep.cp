// Source: cp/cap/test/test_time_production_deep.cp
// cap/test/test_time_production_deep.cp
// Deep Production Test Suite for cap:time
// Covers:
//   1. CivilDateTime Howard Hinnant epoch conversions (1970 before/after, leap years, timezones)
//   2. Iso8601 strict RFC 3339 parsing, formatting, fractional seconds & invalid rejection
//   3. IsoDuration parsing, exact/calendar milliseconds & formatting
//   4. POSIX 5-field CronExpr parser, aliases, matches & cross-leap-year nextAfter
//   5. TokenBucket, SlidingWindowLimiter & ExponentialBackoff deterministic behavior
//   6. Duration arithmetic & WallClock Linux realtime/monotonic sanity

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TimeUnit,
    Duration,
    WallClock,
    CivilDateTime,
    Iso8601,
    IsoDuration,
    CronExpr,
    TokenBucket,
    SlidingWindowLimiter,
    ExponentialBackoff,
} from "cap:time";

fn main() {
    test("1. CivilDateTime Howard Hinnant Epoch & Leap Year Conversions", () => {
        // Epoch origin 1970-01-01T00:00:00Z
        const epoch = CivilDateTime.utc(1970, 1, 1, 0, 0, 0);
        expect_eq(epoch.toUnixSeconds(), 0);
        expect_eq(epoch.dayOfWeek() as i64, 4); // Thursday
        const epoch_rt = CivilDateTime.fromUnixSeconds(0);
        expect_eq(epoch_rt.year as i64, 1970);
        expect_eq(epoch_rt.month as i64, 1);
        expect_eq(epoch_rt.day as i64, 1);

        // Pre-1970 negative timestamps
        const pre_1s = CivilDateTime.fromUnixSeconds(-1);
        expect_eq(pre_1s.year as i64, 1969);
        expect_eq(pre_1s.month as i64, 12);
        expect_eq(pre_1s.day as i64, 31);
        expect_eq(pre_1s.hour as i64, 23);
        expect_eq(pre_1s.minute as i64, 59);
        expect_eq(pre_1s.second as i64, 59);
        expect_eq(pre_1s.toUnixSeconds(), -1);
        expect_eq(pre_1s.dayOfWeek() as i64, 3); // Wednesday

        // Negative milliseconds (-1 ms -> 1969-12-31T23:59:59.999Z)
        const pre_1ms = CivilDateTime.fromUnixMillis(-1);
        expect_eq(pre_1ms.year as i64, 1969);
        expect_eq(pre_1ms.second as i64, 59);
        expect_eq(pre_1ms.nanosecond as i64, 999000000);
        expect_eq(pre_1ms.toUnixMillis(), -1);

        // Pre-1970 leap year (1968) and non-leap century (1900)
        expect_eq(CivilDateTime.isLeapYear(1968), true);
        expect_eq(CivilDateTime.isLeapYear(1900), false);
        expect_eq(CivilDateTime.isLeapYear(2000), true);
        expect_eq(CivilDateTime.isLeapYear(2100), false);
        expect_eq(CivilDateTime.isLeapYear(2024), true);

        const leap_1968 = CivilDateTime.utc(1968, 2, 29, 12, 30, 45);
        const rt_1968 = CivilDateTime.fromUnixSeconds(leap_1968.toUnixSeconds());
        expect_eq(rt_1968.year as i64, 1968);
        expect_eq(rt_1968.month as i64, 2);
        expect_eq(rt_1968.day as i64, 29);
        expect_eq(rt_1968.hour as i64, 12);

        const feb28_1900 = CivilDateTime.utc(1900, 2, 28, 23, 59, 59);
        const mar01_1900 = CivilDateTime.fromUnixSeconds(feb28_1900.toUnixSeconds() + 1);
        expect_eq(mar01_1900.year as i64, 1900);
        expect_eq(mar01_1900.month as i64, 3);
        expect_eq(mar01_1900.day as i64, 1);

        // Century leap 2000-02-29 & non-leap 2100-02-28
        const leap_2000 = CivilDateTime.utc(2000, 2, 29, 0, 0, 0);
        expect_eq(CivilDateTime.fromUnixSeconds(leap_2000.toUnixSeconds()).day as i64, 29);
        const feb28_2100 = CivilDateTime.utc(2100, 2, 28, 23, 59, 59);
        const next_2100 = CivilDateTime.fromUnixSeconds(feb28_2100.toUnixSeconds() + 1);
        expect_eq(next_2100.month as i64, 3);
        expect_eq(next_2100.day as i64, 1);

        // 2024-02-29 Thursday check & timezone offset conversions
        const leap_2024 = CivilDateTime.utc(2024, 2, 29, 0, 0, 0);
        expect_eq(leap_2024.dayOfWeek() as i64, 4);

        const local_p8 = CivilDateTime.init(2026, 11, 20, 22, 30, 0, 0, 480);
        const utc_conv = local_p8.toUtc();
        expect_eq(utc_conv.hour as i64, 14);
        expect_eq(utc_conv.minute as i64, 30);
        const ny_conv = local_p8.withOffset(-300);
        expect_eq(ny_conv.hour as i64, 9);
        expect_eq(ny_conv.tz_offset_minutes as i64, -300);
    });

    test("2. Iso8601 Strict RFC 3339 Parse & Format Round-Trip", () => {
        let buf: [64]u8 = [0; 64];

        const dt_z = Iso8601.parseDateTime("2024-02-29T23:59:59.123Z").?;
        expect_eq(dt_z.year as i64, 2024);
        expect_eq(dt_z.month as i64, 2);
        expect_eq(dt_z.day as i64, 29);
        expect_eq(dt_z.nanosecond as i64, 123000000);
        expect_eq(dt_z.tz_offset_minutes as i64, 0);
        const fmt_z = Iso8601.formatRfc3339(dt_z, &buf) catch unreachable;
        expect_eq(std.mem.eql(u8, fmt_z, "2024-02-29T23:59:59.123Z"), true);

        const dt_off = Iso8601.parseDateTime("2026-07-04T15:30:45.5-04:00").?;
        expect_eq(dt_off.tz_offset_minutes as i64, -240);
        expect_eq(dt_off.nanosecond as i64, 500000000);
        const fmt_off = dt_off.formatRfc3339(&buf) catch unreachable;
        expect_eq(std.mem.eql(u8, fmt_off, "2026-07-04T15:30:45.5-04:00"), true);

        const dt_p930 = Iso8601.parseDateTime("2026-01-15t08:05:09.005+09:30").?;
        expect_eq(dt_p930.tz_offset_minutes as i64, 570);
        expect_eq(dt_p930.nanosecond as i64, 5000000);
        const fmt_p930 = Iso8601.formatRfc3339(dt_p930, &buf) catch unreachable;
        expect_eq(std.mem.eql(u8, fmt_p930, "2026-01-15T08:05:09.005+09:30"), true);

        const dt_plain = Iso8601.parseDateTime("1970-01-01T00:00:00Z").?;
        expect_eq(dt_plain.toUnixSeconds(), 0);
        const fmt_plain = Iso8601.formatRfc3339(dt_plain, &buf) catch unreachable;
        expect_eq(std.mem.eql(u8, fmt_plain, "1970-01-01T00:00:00Z"), true);

        // Date-only parser
        expect_eq(Iso8601.parseDate("2024-02-29") != null, true);
        expect_eq(Iso8601.parseDate("2025-02-29") == null, true);

        // Strict rejections
        expect_eq(Iso8601.parseDateTime("2023-02-29T00:00:00Z") == null, true);
        expect_eq(Iso8601.parseDateTime("2100-02-29T00:00:00Z") == null, true);
        expect_eq(Iso8601.parseDateTime("2026-13-01T00:00:00Z") == null, true);
        expect_eq(Iso8601.parseDateTime("2026-01-01T24:00:00Z") == null, true);
        expect_eq(Iso8601.parseDateTime("2026-01-01T00:00:00") == null, true);
        expect_eq(Iso8601.parseDateTime("2026-01-01T00:00:00.Z") == null, true);
        expect_eq(Iso8601.parseDateTime("2026-01-01X00:00:00Z") == null, true);
        expect_eq(Iso8601.parseDateTime("2026-01-01T00:00:00+24:00") == null, true);
        expect_eq(Iso8601.parseDateTime("invalid") == null, true);
    });

    test("3. IsoDuration Parse, Milliseconds Conversion & Formatting", () => {
        let buf: [64]u8 = [0; 64];

        const d1 = IsoDuration.parse("P1DT2H30M15S").?;
        expect_eq(d1.days, 1);
        expect_eq(d1.hours, 2);
        expect_eq(d1.minutes, 30);
        expect_eq(d1.seconds, 15);
        expect_eq(d1.toMilliseconds(), 95415000);
        expect_eq(d1.toMillisecondsExact().?, 95415000);
        const f1 = d1.format(&buf) catch unreachable;
        expect_eq(std.mem.eql(u8, f1, "P1DT2H30M15S"), true);

        const d2 = IsoDuration.parse("P2W").?;
        expect_eq(d2.toMilliseconds(), 14 * 86400 * 1000);
        const f2 = d2.format(&buf) catch unreachable;
        expect_eq(std.mem.eql(u8, f2, "P2W"), true);

        const d3 = IsoDuration.parse("-PT1.5S").?;
        expect_eq(d3.negative, true);
        expect_eq(d3.toMilliseconds(), -1500);
        const f3 = d3.format(&buf) catch unreachable;
        expect_eq(std.mem.eql(u8, f3, "-PT1.5S"), true);

        const d4 = IsoDuration.parse("P1Y2M3DT4H").?;
        expect_eq(d4.toMillisecondsExact() == null, true);
        expect_eq(d4.toMilliseconds(), (365 + 60 + 3) * 86400000 + 4 * 3600000);
        const f4 = d4.format(&buf) catch unreachable;
        expect_eq(std.mem.eql(u8, f4, "P1Y2M3DT4H"), true);

        // Invalid durations
        expect_eq(IsoDuration.parse("P") == null, true);
        expect_eq(IsoDuration.parse("PT") == null, true);
        expect_eq(IsoDuration.parse("P1DT") == null, true);
        expect_eq(IsoDuration.parse("1D") == null, true);
        expect_eq(IsoDuration.parse("PT1.5H") == null, true);
        expect_eq(IsoDuration.parse("P1D2Y") == null, true);
    });

    test("4. POSIX 5-Field Cron Parser, Aliases, Matches & Cross-Leap nextAfter", () => {
        // Weekday business hours every 15 minutes
        const biz = CronExpr.parse("*/15 9-17 * * MON-FRI").?;
        expect_eq(biz.matches(CivilDateTime.utc(2026, 11, 20, 9, 0, 0)), true);   // Friday 09:00
        expect_eq(biz.matches(CivilDateTime.utc(2026, 11, 20, 17, 45, 0)), true); // Friday 17:45
        expect_eq(biz.matches(CivilDateTime.utc(2026, 11, 20, 8, 45, 0)), false); // Before 9
        expect_eq(biz.matches(CivilDateTime.utc(2026, 11, 20, 18, 0, 0)), false); // After 17
        expect_eq(biz.matches(CivilDateTime.utc(2026, 11, 21, 10, 0, 0)), false); // Saturday

        // Month abbreviations + comma list + step range
        const quarterly = CronExpr.parse("5-25/10 12 1,15 JAN,JUN,DEC *").?;
        expect_eq(quarterly.matches(CivilDateTime.utc(2026, 1, 1, 12, 5, 0)), true);
        expect_eq(quarterly.matches(CivilDateTime.utc(2026, 6, 15, 12, 25, 0)), true);
        expect_eq(quarterly.matches(CivilDateTime.utc(2026, 12, 15, 12, 15, 0)), true);
        expect_eq(quarterly.matches(CivilDateTime.utc(2026, 2, 1, 12, 5, 0)), false);
        expect_eq(quarterly.matches(CivilDateTime.utc(2026, 1, 1, 12, 35, 0)), false);

        // POSIX DOM + DOW union semantics when both are restricted
        const dom_or_dow = CronExpr.parse("0 0 15 * FRI").?;
        expect_eq(dom_or_dow.matches(CivilDateTime.utc(2026, 11, 15, 0, 0, 0)), true); // 15th (Sunday)
        expect_eq(dom_or_dow.matches(CivilDateTime.utc(2026, 11, 20, 0, 0, 0)), true); // Friday (20th)
        expect_eq(dom_or_dow.matches(CivilDateTime.utc(2026, 11, 19, 0, 0, 0)), false);

        // Sunday as 7 and aliases
        const sun7 = CronExpr.parse("0 0 * * 7").?;
        expect_eq(sun7.matches(CivilDateTime.utc(2026, 11, 22, 0, 0, 0)), true); // Sunday
        const weekly = CronExpr.parse("@weekly").?;
        expect_eq(weekly.matches(CivilDateTime.utc(2026, 11, 22, 0, 0, 0)), true);
        const yearly = CronExpr.parse("@yearly").?;
        expect_eq(yearly.matches(CivilDateTime.utc(2027, 1, 1, 0, 0, 0)), true);
        const monthly = CronExpr.parse("@monthly").?;
        expect_eq(monthly.matches(CivilDateTime.utc(2026, 12, 1, 0, 0, 0)), true);
        const daily = CronExpr.parse("@daily").?;
        expect_eq(daily.matches(CivilDateTime.utc(2026, 11, 20, 0, 0, 0)), true);
        const hourly = CronExpr.parse("@hourly").?;
        expect_eq(hourly.matches(CivilDateTime.utc(2026, 11, 20, 14, 0, 0)), true);

        // Cross-leap-year nextAfter: 2024-02-29 -> 2028-02-29
        const leap_cron = CronExpr.parse("0 0 29 2 *").?;
        const t_2024: i64 = CivilDateTime.utc(2024, 2, 29, 0, 0, 0).toUnixSeconds();
        const next_2028 = leap_cron.nextAfter(t_2024).?;
        expect_eq(next_2028, CivilDateTime.utc(2028, 2, 29, 0, 0, 0).toUnixSeconds());

        // Cross-century non-leap 2100: 2096-02-29 -> 2104-02-29
        const t_2096: i64 = CivilDateTime.utc(2096, 2, 29, 0, 0, 0).toUnixSeconds();
        const next_2104 = leap_cron.nextAfter(t_2096).?;
        expect_eq(next_2104, CivilDateTime.utc(2104, 2, 29, 0, 0, 0).toUnixSeconds());

        // Cross-month minute rollover: 2026-01-31T23:55:00Z -> 2026-02-01T00:00:00Z
        const midnight_cron = CronExpr.parse("0 0 * * *").?;
        const jan31: i64 = CivilDateTime.utc(2026, 1, 31, 23, 55, 0).toUnixSeconds();
        expect_eq(midnight_cron.nextAfter(jan31).?, CivilDateTime.utc(2026, 2, 1, 0, 0, 0).toUnixSeconds());

        // Impossible schedule (Feb 31) returns null cleanly
        const feb31 = CronExpr.parse("0 0 31 2 *").?;
        expect_eq(feb31.nextAfter(jan31) == null, true);

        // Invalid cron expressions
        expect_eq(CronExpr.parse("") == null, true);
        expect_eq(CronExpr.parse("* * *") == null, true);
        expect_eq(CronExpr.parse("60 * * * *") == null, true);
        expect_eq(CronExpr.parse("0 0 0 * *") == null, true);
        expect_eq(CronExpr.parse("* * * NOPE *") == null, true);
        expect_eq(CronExpr.parse("*/0 * * * *") == null, true);
    });

    test("5. TokenBucket, SlidingWindowLimiter & ExponentialBackoff", () => {
        // TokenBucket: capacity=10, refill=5 tokens/sec
        let tb = TokenBucket.init(10.0, 5.0, 1000);
        expect_eq(tb.tryAcquire(6.0, 1000), true);
        expect_eq(tb.tryAcquire(5.0, 1000), false);
        expect_eq(tb.retryAfterMs(5.0, 1000), 200);
        expect_eq(tb.tryAcquire(5.0, 1199), false);
        expect_eq(tb.retryAfterMs(5.0, 1199), 1);
        expect_eq(tb.tryAcquire(5.0, 1200), true);
        expect_eq(tb.retryAfterMs(1.0, 1200), 200);
        // Idle refill caps at capacity
        expect_eq(tb.available(100000) == 10.0, true);
        // Impossible acquisition > capacity
        expect_eq(tb.tryAcquire(11.0, 100000), false);
        expect_eq(tb.retryAfterMs(11.0, 100000), -1);

        // SlidingWindowLimiter: limit=4 per 1000ms
        let sw = SlidingWindowLimiter.init(4, 1000, 0);
        expect_eq(sw.tryAcquire(0), true);
        expect_eq(sw.tryAcquire(100), true);
        expect_eq(sw.tryAcquire(200), true);
        expect_eq(sw.tryAcquire(300), true);
        expect_eq(sw.remaining(300) as i64, 0);
        expect_eq(sw.tryAcquire(400), false);
        // At exact boundary t=1000, prev_count=4 with weight=1.0 still blocks boundary burst
        expect_eq(sw.tryAcquire(1000), false);
        // At t=1500, prev_count=4 * 0.5 = 2.0 weighted -> 2 slots available
        expect_eq(sw.remaining(1500) as i64, 2);
        expect_eq(sw.tryAcquire(1500), true);
        expect_eq(sw.tryAcquire(1500), true);
        expect_eq(sw.tryAcquire(1500), false);
        // After >=2 windows, completely reset
        expect_eq(sw.remaining(4000) as i64, 4);
        expect_eq(sw.tryAcquire(4000), true);

        // ExponentialBackoff: deterministic full-jitter & equal-jitter
        let b1 = ExponentialBackoff.init(100, 1000, 2.0);
        let b2 = ExponentialBackoff.init(100, 1000, 2.0);
        expect_eq(b1.rawDelayMs() as i64, 100);
        const seed_a: u64 = 0x5AFEBABE12345678;
        const d0_a = b1.nextDelayMs(seed_a);
        const d0_b = b2.nextDelayMs(seed_a);
        expect_eq(d0_a == d0_b, true);
        expect_eq(d0_a <= 100, true);
        expect_eq(b1.rawDelayMs() as i64, 200);

        const d1 = b1.nextDelayMs(seed_a);
        expect_eq(d1 <= 200, true);
        expect_eq(b1.rawDelayMs() as i64, 400);
        const d2 = b1.nextDelayMs(seed_a);
        expect_eq(d2 <= 400, true);
        const d3 = b1.nextDelayMs(seed_a);
        expect_eq(d3 <= 800, true);
        // Capped at max_ms = 1000
        expect_eq(b1.rawDelayMs() as i64, 1000);
        const d4 = b1.nextEqualJitterMs(0x1234);
        expect_eq(d4 >= 500 && d4 <= 1000, true);

        b1.reset();
        expect_eq(b1.attempt as i64, 0);
        expect_eq(b1.nextDelayMs(seed_a) == d0_a, true);
    });

    test("6. Duration Arithmetic & WallClock OS Clock Integration", () => {
        const d_day = Duration.fromDays(1);
        const d_hr = Duration.fromHours(2);
        const sum = d_day.add(d_hr);
        expect_eq(sum.toSeconds(), 86400 + 7200);
        expect_eq(d_hr.sub(d_day).isNegative(), true);
        expect_eq(d_hr.sub(d_day).abs().toMillis(), 22 * 3600 * 1000);

        const now_sec = WallClock.nowUnixSec();
        expect_eq(now_sec > 1700000000, true);
        const now_dt = CivilDateTime.fromUnixMillis(WallClock.nowUnixMs());
        expect_eq(now_dt.isValid(), true);

        const m1 = WallClock.monotonicNs();
        const m2 = WallClock.monotonicNs();
        expect_eq(m2 >= m1, true);
        expect_eq(TimeUnit.Seconds as i64, 3);
    });
}
