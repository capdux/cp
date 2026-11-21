// Source: cp/cap/test/test_rfc3339_time_production_deep.cp (Production deep tests for cap:rfc3339_time)

import { test, expect_eq } from "cap:test";
import {
    isLeapYear,
    daysInMonth,
    Rfc3339Time,
} from "cap:time";

test("rfc3339_time: isLeapYear and daysInMonth calendar rules") {
    expect_eq(isLeapYear(2000), true);
    expect_eq(isLeapYear(2024), true);
    expect_eq(isLeapYear(1900), false);
    expect_eq(isLeapYear(2026), false);

    expect_eq(daysInMonth(2024, 2), 29);
    expect_eq(daysInMonth(2025, 2), 28);
    expect_eq(daysInMonth(2026, 4), 30);
    expect_eq(daysInMonth(2026, 12), 31);
    expect_eq(daysInMonth(2026, 0), 0);
    expect_eq(daysInMonth(2026, 13), 0);
}

test("rfc3339_time: parse with T/t/space, fractional seconds and timezone offsets") {
    const t1 = Rfc3339Time.parse("2026-11-22T01:55:00Z").?;
    expect_eq(t1.year, 2026);
    expect_eq(t1.month, 11);
    expect_eq(t1.day, 22);
    expect_eq(t1.hour, 1);
    expect_eq(t1.minute, 55);
    expect_eq(t1.second, 0);
    expect_eq(t1.nanos, 0);
    expect_eq(t1.offset_minutes, 0);

    // 小写 t + 1 位小数秒自动右补零至纳秒 + 东八区偏移
    const t2 = Rfc3339Time.parse("2024-02-29t12:30:45.5+08:00").?;
    expect_eq(t2.year, 2024);
    expect_eq(t2.month, 2);
    expect_eq(t2.day, 29);
    expect_eq(t2.nanos, 500000000);
    expect_eq(t2.offset_minutes, 480);

    // 空格分隔 + 9 位纳秒 + 西五区偏移
    const t3 = Rfc3339Time.parse("2026-01-01 00:00:00.123456789-05:30").?;
    expect_eq(t3.nanos, 123456789);
    expect_eq(t3.offset_minutes, -330);
}

test("rfc3339_time: toUnixSec and fromUnixSecUtc round-trip including pre-1970 negative timestamps") {
    // Unix Epoch = 0
    const epoch = Rfc3339Time.parse("1970-01-01T00:00:00Z").?;
    expect_eq(epoch.toUnixSec(), 0);

    // 1969-12-31T23:59:59Z = -1
    const pre1 = Rfc3339Time.fromUnixSecUtc(-1, 0).?;
    expect_eq(pre1.year, 1969);
    expect_eq(pre1.month, 12);
    expect_eq(pre1.day, 31);
    expect_eq(pre1.hour, 23);
    expect_eq(pre1.minute, 59);
    expect_eq(pre1.second, 59);
    expect_eq(pre1.toUnixSec(), -1);

    // 1960-02-29T12:00:00Z（1970 前闰年日期往返）
    const leap_pre = Rfc3339Time.parse("1960-02-29T12:00:00Z").?;
    const neg_sec = leap_pre.toUnixSec();
    expect_eq(neg_sec < 0, true);
    const back_pre = Rfc3339Time.fromUnixSecUtc(neg_sec, 0).?;
    expect_eq(back_pre.year, 1960);
    expect_eq(back_pre.month, 2);
    expect_eq(back_pre.day, 29);
    expect_eq(back_pre.hour, 12);

    // 带时区偏移的时间戳换算为同一 UTC Unix 秒
    const utc_t = Rfc3339Time.parse("2026-11-21T17:55:00Z").?;
    const cst_t = Rfc3339Time.parse("2026-11-22T01:55:00+08:00").?;
    expect_eq(utc_t.toUnixSec(), cst_t.toUnixSec());

    const back_utc = Rfc3339Time.fromUnixSecUtc(cst_t.toUnixSec(), 250000000).?;
    expect_eq(back_utc.year, 2026);
    expect_eq(back_utc.month, 11);
    expect_eq(back_utc.day, 21);
    expect_eq(back_utc.hour, 17);
    expect_eq(back_utc.minute, 55);
}

test("rfc3339_time: format canonical serialization and parse round-trip") {
    let buf: [64]u8 = [0; 64];

    const t1 = Rfc3339Time.parse("2026-11-22T01:55:00z").?;
    expect_eq(std.mem.eql(u8, t1.format(buf[0..]).?, "2026-11-22T01:55:00Z"), true);

    const t2 = Rfc3339Time.parse("2026-11-22 01:55:00.250000+08:00").?;
    const f2 = t2.format(buf[0..]).?;
    expect_eq(std.mem.eql(u8, f2, "2026-11-22T01:55:00.25+08:00"), true);

    const t3 = Rfc3339Time.parse("2026-11-22T01:55:00.000000001-05:00").?;
    const f3 = t3.format(buf[0..]).?;
    expect_eq(std.mem.eql(u8, f3, "2026-11-22T01:55:00.000000001-05:00"), true);
    expect_eq(Rfc3339Time.parse(f3).?.nanos, 1);
}

test("rfc3339_time: strict validation rejects invalid leap dates, ranges and small buffers") {
    // 非闰年 2 月 29 日、非法月份/日期/时分秒/时区、缺失时区、空小数点
    expect_eq(Rfc3339Time.parse("2025-02-29T00:00:00Z") == null, true);
    expect_eq(Rfc3339Time.parse("1900-02-29T00:00:00Z") == null, true);
    expect_eq(Rfc3339Time.parse("2026-13-01T00:00:00Z") == null, true);
    expect_eq(Rfc3339Time.parse("2026-04-31T00:00:00Z") == null, true);
    expect_eq(Rfc3339Time.parse("2026-01-01T24:00:00Z") == null, true);
    expect_eq(Rfc3339Time.parse("2026-01-01T00:60:00Z") == null, true);
    expect_eq(Rfc3339Time.parse("2026-01-01T00:00:60Z") == null, true);
    expect_eq(Rfc3339Time.parse("2026-01-01T00:00:00") == null, true);
    expect_eq(Rfc3339Time.parse("2026-01-01T00:00:00.Z") == null, true);
    expect_eq(Rfc3339Time.parse("2026-01-01T00:00:00+24:00") == null, true);
    expect_eq(Rfc3339Time.parse("2026-01-01T00:00:00Zextra") == null, true);
    expect_eq(Rfc3339Time.fromUnixSecUtc(0, 1000000000) == null, true);

    let tiny: [10]u8 = [0; 10];
    const valid = Rfc3339Time.parse("2026-11-22T01:55:00Z").?;
    expect_eq(valid.format(tiny[0..]) == null, true);
}
