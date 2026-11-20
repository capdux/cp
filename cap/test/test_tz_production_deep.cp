// Source: cp/cap/test/test_tz_production_deep.cp
// Deep production test suite for pure CP `cap:tz` & `cap.tz` dynamic mount.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TzError,
    DstRule,
    LocalDateTime,
    Timezone,
    isLeapYear,
    daysInMonth,
    daysFromCivil,
} from "cap:tz";
import cap from "cap";

fn expectStrEq(actual: []const u8, expected: []const u8): ! {
    expect_eq(std.mem.eql(u8, actual, expected), true);
}

fn expectLookupErr(name: []const u8, expected_err: []const u8): ! {
    let tz = Timezone.lookup(name) catch e => {
        expectStrEq(@errorName(e), expected_err)?;
        return;
    };
    * = tz;
    expect_eq(true, false);
}

fn expectPosixErr(rule: []const u8, expected_err: []const u8): ! {
    let tz = Timezone.parsePosix(rule) catch e => {
        expectStrEq(@errorName(e), expected_err)?;
        return;
    };
    * = tz;
    expect_eq(true, false);
}

fn expectOffsetErr(off_str: []const u8, expected_err: []const u8): ! {
    let off = Timezone.parseOffset(off_str) catch e => {
        expectStrEq(@errorName(e), expected_err)?;
        return;
    };
    * = off;
    expect_eq(true, false);
}

fn expectToUnixErr(tz: Timezone, y: i32, m: u8, d: u8, hh: u8, mm: u8, ss: u8): ! {
    let ts = tz.toUnix(y, m, d, hh, mm, ss) catch e => {
        expectStrEq(@errorName(e), "InvalidDateTime")?;
        return;
    };
    * = ts;
    expect_eq(true, false);
}

pub fn main() {
    test("1. Howard Hinnant O(1) Calendar, Leap Year (2024-02-29 / 2100 Non-Leap), 1970- & 2038+ Roundtrip", () => {
        let utc = Timezone.utc();

        expect_eq(isLeapYear(2024), true);
        expect_eq(isLeapYear(2000), true);
        expect_eq(isLeapYear(2100), false);
        expect_eq(daysInMonth(2024, 2), 29);
        expect_eq(daysInMonth(2100, 2), 28);

        // 2024-02-29T12:34:56Z roundtrip
        let ts_leap = utc.toUnix(2024, 2, 29, 12, 34, 56) catch unreachable;
        let dt_leap = utc.toLocal(ts_leap) catch unreachable;
        expect_eq(dt_leap.year, 2024);
        expect_eq(dt_leap.month, 2);
        expect_eq(dt_leap.day, 29);
        expect_eq(dt_leap.hour, 12);
        expect_eq(dt_leap.minute, 34);
        expect_eq(dt_leap.second, 56);
        expect_eq(dt_leap.dayOfWeek(), 4); // Thursday

        // 2100-02-28 + 1 day -> 2100-03-01 (century non-leap)
        let ts_2100_feb28 = utc.toUnix(2100, 2, 28, 23, 59, 59) catch unreachable;
        let dt_2100_mar1 = utc.toLocal(ts_2100_feb28 + 1) catch unreachable;
        expect_eq(dt_2100_mar1.year, 2100);
        expect_eq(dt_2100_mar1.month, 3);
        expect_eq(dt_2100_mar1.day, 1);
        expect_eq(dt_2100_mar1.hour, 0);

        // Beyond Y2038 (2045-08-19T15:20:10Z)
        let ts_2045 = utc.toUnix(2045, 8, 19, 15, 20, 10) catch unreachable;
        expect_eq(ts_2045 > 2147483647, true);
        let dt_2045 = utc.toLocal(ts_2045) catch unreachable;
        expect_eq(dt_2045.year, 2045);
        expect_eq(dt_2045.month, 8);
        expect_eq(dt_2045.day, 19);
        expect_eq(dt_2045.hour, 15);
        expect_eq(dt_2045.minute, 20);
        expect_eq(dt_2045.second, 10);

        // Pre-1970 negative timestamp (1969-12-31T23:59:59Z == -1)
        let dt_pre = utc.toLocal(-1) catch unreachable;
        expect_eq(dt_pre.year, 1969);
        expect_eq(dt_pre.month, 12);
        expect_eq(dt_pre.day, 31);
        expect_eq(dt_pre.hour, 23);
        expect_eq(dt_pre.minute, 59);
        expect_eq(dt_pre.second, 59);
        expect_eq(utc.toUnix(1969, 12, 31, 23, 59, 59) catch unreachable, -1);
    });

    test("2. Northern Hemisphere DST (America/New_York EST -05:00 vs EDT -04:00 Exact Transitions)", () => {
        let ny = Timezone.lookup("America/New_York") catch unreachable;
        let utc = Timezone.utc();

        // 2026-03-08 is 2nd Sunday of March: 02:00 EST == 07:00:00 UTC
        let spring_utc = utc.toUnix(2026, 3, 8, 7, 0, 0) catch unreachable;

        let before_spring = ny.toLocal(spring_utc - 1) catch unreachable;
        expect_eq(before_spring.is_dst, false);
        expect_eq(before_spring.offset_sec, -18000);
        expectStrEq(before_spring.abbr, "EST") catch unreachable;
        expect_eq(before_spring.hour, 1);
        expect_eq(before_spring.minute, 59);
        expect_eq(before_spring.second, 59);

        let at_spring = ny.toLocal(spring_utc) catch unreachable;
        expect_eq(at_spring.is_dst, true);
        expect_eq(at_spring.offset_sec, -14400);
        expectStrEq(at_spring.abbr, "EDT") catch unreachable;
        expect_eq(at_spring.hour, 3);
        expect_eq(at_spring.minute, 0);
        expect_eq(at_spring.second, 0);

        // 2026-11-01 is 1st Sunday of November: 02:00 EDT == 06:00:00 UTC
        let fall_utc = utc.toUnix(2026, 11, 1, 6, 0, 0) catch unreachable;

        let before_fall = ny.toLocal(fall_utc - 1) catch unreachable;
        expect_eq(before_fall.is_dst, true);
        expect_eq(before_fall.offset_sec, -14400);
        expectStrEq(before_fall.abbr, "EDT") catch unreachable;
        expect_eq(before_fall.hour, 1);
        expect_eq(before_fall.minute, 59);

        let at_fall = ny.toLocal(fall_utc) catch unreachable;
        expect_eq(at_fall.is_dst, false);
        expect_eq(at_fall.offset_sec, -18000);
        expectStrEq(at_fall.abbr, "EST") catch unreachable;
        expect_eq(at_fall.hour, 1);
        expect_eq(at_fall.minute, 0);
    });

    test("3. Southern Hemisphere Cross-Year DST (Australia/Sydney +11:00/+10:00) & Asia/Shanghai (+08:00)", () => {
        let sydney = Timezone.lookup("Australia/Sydney") catch unreachable;
        let shanghai = Timezone.lookup("Asia/Shanghai") catch unreachable;
        let utc = Timezone.utc();

        // January in Sydney is Summer (AEDT, +11:00)
        let jan_utc = utc.toUnix(2026, 1, 15, 0, 0, 0) catch unreachable;
        expect_eq(sydney.isDstAt(jan_utc), true);
        expect_eq(sydney.offsetAt(jan_utc), 39600);
        let syd_jan = sydney.toLocal(jan_utc) catch unreachable;
        expectStrEq(syd_jan.abbr, "AEDT") catch unreachable;
        expect_eq(syd_jan.hour, 11);

        // July in Sydney is Winter (AEST, +10:00)
        let jul_utc = utc.toUnix(2026, 7, 15, 0, 0, 0) catch unreachable;
        expect_eq(sydney.isDstAt(jul_utc), false);
        expect_eq(sydney.offsetAt(jul_utc), 36000);
        let syd_jul = sydney.toLocal(jul_utc) catch unreachable;
        expectStrEq(syd_jul.abbr, "AEST") catch unreachable;
        expect_eq(syd_jul.hour, 10);

        // Asia/Shanghai has no DST year-round (+08:00)
        expect_eq(shanghai.isDstAt(jan_utc), false);
        expect_eq(shanghai.isDstAt(jul_utc), false);
        expect_eq(shanghai.offsetAt(jan_utc), 28800);
        let sha_jul = shanghai.toLocal(jul_utc) catch unreachable;
        expect_eq(sha_jul.hour, 8);
        expectStrEq(sha_jul.abbr, "CST") catch unreachable;
    });

    test("4. parseOffset, parsePosix, formatRfc3339 & Error Guards (Zero Memory Leak)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // parseOffset variants
        expect_eq(Timezone.parseOffset("Z") catch unreachable, 0);
        expect_eq(Timezone.parseOffset("UTC+08:00") catch unreachable, 28800);
        expect_eq(Timezone.parseOffset("+05:30") catch unreachable, 19800);
        expect_eq(Timezone.parseOffset("-05:00") catch unreachable, -18000);
        expect_eq(Timezone.parseOffset("-0430") catch unreachable, -16200);

        // parsePosix custom rule (CET-1CEST,M3.5.0/2,M10.5.0/3)
        let berlin = Timezone.parsePosix("CET-1CEST,M3.5.0/2,M10.5.0/3") catch unreachable;
        expect_eq(berlin.std_offset_sec, 3600);
        expect_eq(berlin.dst_offset_sec, 7200);
        expect_eq(berlin.has_dst, true);

        // formatRfc3339
        let ny = Timezone.lookup("America/New_York") catch unreachable;
        let ts_summer = ny.toUnix(2026, 7, 15, 10, 30, 0) catch unreachable;
        let rfc_ny = ny.formatRfc3339(alloc, ts_summer) catch unreachable;
        defer alloc.free(rfc_ny);
        expectStrEq(rfc_ny, "2026-07-15T10:30:00-04:00") catch unreachable;

        let sh = Timezone.lookup("Asia/Shanghai") catch unreachable;
        let rfc_sh = sh.formatRfc3339(alloc, ts_summer) catch unreachable;
        defer alloc.free(rfc_sh);
        expectStrEq(rfc_sh, "2026-07-15T22:30:00+08:00") catch unreachable;

        let utc = Timezone.utc();
        let rfc_utc = utc.formatRfc3339(alloc, ts_summer) catch unreachable;
        defer alloc.free(rfc_utc);
        expectStrEq(rfc_utc, "2026-07-15T14:30:00Z") catch unreachable;

        // Error interceptions
        expectLookupErr("Mars/Olympus_Mons", "UnknownTimezone") catch unreachable;
        expectPosixErr("", "InvalidTzString") catch unreachable;
        expectPosixErr("EST5EDT,M13.1.0,M11.1.0", "InvalidTzString") catch unreachable;
        expectPosixErr("EST5EDT", "InvalidTzString") catch unreachable;
        expectOffsetErr("UTC+25:00", "InvalidOffset") catch unreachable;
        expectOffsetErr("invalid", "InvalidOffset") catch unreachable;
        expectToUnixErr(utc, 2100, 2, 29, 0, 0, 0) catch unreachable;
        expectToUnixErr(utc, 2026, 4, 31, 12, 0, 0) catch unreachable;
    });

    test("5. cap.tz Dynamic Mount Verification", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let tokyo: Timezone = cap.tz.Timezone.lookup("Asia/Tokyo") catch unreachable;
        expect_eq(tokyo.std_offset_sec, 32400);

        let ts = tokyo.toUnix(2026, 5, 1, 9, 0, 0) catch unreachable;
        let rfc = tokyo.formatRfc3339(alloc, ts) catch unreachable;
        defer alloc.free(rfc);
        expectStrEq(rfc, "2026-05-01T09:00:00+09:00") catch unreachable;

        let fixed: Timezone = cap.tz.Timezone.fixedOffset("IST", 19800);
        let dt = fixed.toLocal(ts) catch unreachable;
        expect_eq(dt.hour, 5);
        expect_eq(dt.minute, 30);
    });
}
