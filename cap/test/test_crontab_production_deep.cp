// Source: cp/cap/test/test_crontab_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { CronError, DateTime, CronExpr } from "cap:crontab";

test "crontab: step, range, list, and named months/weekdays" {
    const cron = try CronExpr.parse("*/15 9-17 * NOV MON-FRI");
    // 2026-11-23 is Monday
    const mon = DateTime{ .year = 2026, .month = 11, .day = 23, .hour = 9, .minute = 15 };
    try expect_eq(mon.weekday(), 1);
    try expect_eq(cron.matches(mon), true);

    // 2026-11-21 is Saturday
    const sat = DateTime{ .year = 2026, .month = 11, .day = 21, .hour = 9, .minute = 15 };
    try expect_eq(sat.weekday(), 6);
    try expect_eq(cron.matches(sat), false);
}

test "crontab: nextAfter across day/month/leap-year boundaries" {
    const cron = try CronExpr.parse("30 0 29 2 *");
    const start = DateTime{ .year = 2026, .month = 11, .day = 21, .hour = 12, .minute = 0 };
    const next = cron.nextAfter(start) orelse return error.TestFailed;
    try expect_eq(next.year, 2028);
    try expect_eq(next.month, 2);
    try expect_eq(next.day, 29);
    try expect_eq(next.hour, 0);
    try expect_eq(next.minute, 30);
}

test "crontab: macros @daily, @hourly, @weekly and POSIX OR semantics" {
    const daily = try CronExpr.parse("@daily");
    try expect_eq(daily.matches(DateTime{ .year = 2026, .month = 1, .day = 1, .hour = 0, .minute = 0 }), true);
    try expect_eq(daily.matches(DateTime{ .year = 2026, .month = 1, .day = 1, .hour = 0, .minute = 1 }), false);

    // 1st of month OR Friday
    const or_cron = try CronExpr.parse("0 12 1 * FRI");
    // 2026-11-01 is Sunday, 1st of month
    try expect_eq(or_cron.matches(DateTime{ .year = 2026, .month = 11, .day = 1, .hour = 12, .minute = 0 }), true);
    // 2026-11-20 is Friday
    try expect_eq(or_cron.matches(DateTime{ .year = 2026, .month = 11, .day = 20, .hour = 12, .minute = 0 }), true);
}

test "crontab: fromUnixSeconds and error guards" {
    const dt = DateTime.fromUnixSeconds(0);
    try expect_eq(dt.year, 1970);
    try expect_eq(dt.month, 1);
    try expect_eq(dt.day, 1);

    try expect_eq(CronExpr.parse("* * * *") == CronError.InvalidFieldCount, true);
    try expect_eq(CronExpr.parse("60 * * * *") == CronError.OutOfRange, true);
    try expect_eq(CronExpr.parse("*/0 * * * *") == CronError.InvalidStep, true);
    try expect_eq(CronExpr.parse("5-2 * * * *") == CronError.InvalidRange, true);
}

test "crontab: cap.crontab dynamic mount" {
    const cron = try cap.crontab.CronExpr.parse("@hourly");
    const next = cron.nextAfter(cap.crontab.DateTime{ .year = 2026, .month = 5, .day = 10, .hour = 14, .minute = 45 }) orelse return error.TestFailed;
    try expect_eq(next.hour, 15);
    try expect_eq(next.minute, 0);
}
