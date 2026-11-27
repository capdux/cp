// cap/test/testCapStrftime.cp
// Comprehensive unit tests for @cap/strftime

import { test, expect_eq } from "cap:test";
import {
    DateTime,
    isLeapYear,
    fromUnixSeconds,
    strftime,
    formatUnix,
    formatIso8601Utc,
} from "../../packages/strftime/src/index.cp";

fn main() {
    test("1. @cap/strftime — Unix epoch and ISO-8601 UTC", () => {
        let buf: [128]u8 = undefined;
        expect_eq(formatIso8601Utc(0, buf[0..128]), "1970-01-01T00:00:00Z");
        expect_eq(formatIso8601Utc(1700000000, buf[0..128]), "2023-11-14T22:13:20Z");
    });

    test("2. @cap/strftime — timezone offsets (+0800 / -0500)", () => {
        let buf: [128]u8 = undefined;
        // 1700000000 UTC = 2023-11-14 22:13:20 UTC => +0800 is 2023-11-15 06:13:20 +0800
        expect_eq(formatUnix("%Y-%m-%d %H:%M:%S %z", 1700000000, 28800, buf[0..128]), "2023-11-15 06:13:20 +0800");
        expect_eq(formatUnix("%Y-%m-%d %H:%M:%S %z", 1700000000, -18000, buf[0..128]), "2023-11-14 17:13:20 -0500");
    });

    test("3. @cap/strftime — weekday, month names, 12-hour clock & AM/PM", () => {
        let buf: [128]u8 = undefined;
        // 1970-01-01 was Thursday
        expect_eq(formatUnix("%a, %d %b %Y %I:%M %p %Z", 0, 0, buf[0..128]), "Thu, 01 Jan 1970 12:00 AM UTC");
        expect_eq(formatUnix("%A, %B %e (%I:%M %P)", 1700000000, 0, buf[0..128]), "Tuesday, November 14 (10:13 pm)");
    });

    test("4. @cap/strftime — leap year & day-of-year (%j)", () => {
        expect_eq(isLeapYear(2000), true);
        expect_eq(isLeapYear(1900), false);
        expect_eq(isLeapYear(2024), true);
        expect_eq(isLeapYear(2025), false);

        // 2024-02-29 00:00:00 UTC = 1709164800 (day 60 of leap year)
        const dt: DateTime = fromUnixSeconds(1709164800, 0);
        expect_eq(dt.year, 2024 as i32);
        expect_eq(dt.month, 2 as u8);
        expect_eq(dt.day, 29 as u8);
        expect_eq(dt.yearday, 60 as u16);

        let buf: [64]u8 = undefined;
        expect_eq(strftime("%F day %j", dt, buf[0..64]), "2024-02-29 day 060");
    });

    test("5. @cap/strftime — negative timestamps before 1970", () => {
        let buf: [64]u8 = undefined;
        // -86400 = 1969-12-31 00:00:00 UTC (Wednesday)
        expect_eq(formatUnix("%F %T %a", -86400, 0, buf[0..64]), "1969-12-31 00:00:00 Wed");
    });

    test("6. @cap/strftime — composite specifiers & literal percent", () => {
        let buf: [64]u8 = undefined;
        expect_eq(formatUnix("%y/%m/%d %R w=%w u=%u 100%%", 0, 0, buf[0..64]), "70/01/01 00:00 w=4 u=4 100%");
    });
}
