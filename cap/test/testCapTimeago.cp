// cap/test/testCapTimeago.cp
// Comprehensive unit tests for @cap/timeago

import { test, expect_eq } from "cap:test";
import {
    TimeagoLocale,
    TimeagoStyle,
    formatTimeago,
    formatBetween,
    formatTimeagoEn,
    formatTimeagoZh,
    formatCompact,
} from "../../packages/@cap/timeago/src/index.cp";

fn main() {
    test("1. @cap/timeago — English Long past (just now, singular & plural)", () => {
        let out: [64]u8 = undefined;
        expect_eq(formatTimeagoEn(0, out[0..64]), "just now");
        expect_eq(formatTimeagoEn(-9, out[0..64]), "just now");
        expect_eq(formatTimeagoEn(-45, out[0..64]), "45 seconds ago");
        expect_eq(formatTimeagoEn(-60, out[0..64]), "1 minute ago");
        expect_eq(formatTimeagoEn(-300, out[0..64]), "5 minutes ago");
        expect_eq(formatTimeagoEn(-3600, out[0..64]), "1 hour ago");
        expect_eq(formatTimeagoEn(-10800, out[0..64]), "3 hours ago");
        expect_eq(formatTimeagoEn(-86400, out[0..64]), "1 day ago");
        expect_eq(formatTimeagoEn(-518400, out[0..64]), "6 days ago");
        expect_eq(formatTimeagoEn(-604800, out[0..64]), "1 week ago");
        expect_eq(formatTimeagoEn(-1814400, out[0..64]), "3 weeks ago");
        expect_eq(formatTimeagoEn(-2592000, out[0..64]), "1 month ago");
        expect_eq(formatTimeagoEn(-20736000, out[0..64]), "8 months ago");
        expect_eq(formatTimeagoEn(-31536000, out[0..64]), "1 year ago");
        expect_eq(formatTimeagoEn(-63072000, out[0..64]), "2 years ago");
    });

    test("2. @cap/timeago — English Long future (in ...)", () => {
        let out: [64]u8 = undefined;
        expect_eq(formatTimeagoEn(5, out[0..64]), "just now");
        expect_eq(formatTimeagoEn(45, out[0..64]), "in 45 seconds");
        expect_eq(formatTimeagoEn(60, out[0..64]), "in 1 minute");
        expect_eq(formatTimeagoEn(300, out[0..64]), "in 5 minutes");
        expect_eq(formatTimeagoEn(7200, out[0..64]), "in 2 hours");
        expect_eq(formatTimeagoEn(86400, out[0..64]), "in 1 day");
        expect_eq(formatTimeagoEn(31536000, out[0..64]), "in 1 year");
    });

    test("3. @cap/timeago — Chinese Long past & future", () => {
        let out: [64]u8 = undefined;
        expect_eq(formatTimeagoZh(0, out[0..64]), "刚刚");
        expect_eq(formatTimeagoZh(-45, out[0..64]), "45 秒前");
        expect_eq(formatTimeagoZh(45, out[0..64]), "45 秒后");
        expect_eq(formatTimeagoZh(-60, out[0..64]), "1 分钟前");
        expect_eq(formatTimeagoZh(300, out[0..64]), "5 分钟后");
        expect_eq(formatTimeagoZh(-10800, out[0..64]), "3 小时前");
        expect_eq(formatTimeagoZh(518400, out[0..64]), "6 天后");
        expect_eq(formatTimeagoZh(-1814400, out[0..64]), "3 周前");
        expect_eq(formatTimeagoZh(20736000, out[0..64]), "8 个月后");
        expect_eq(formatTimeagoZh(-63072000, out[0..64]), "2 年前");
    });

    test("4. @cap/timeago — Short style (EN & ZH)", () => {
        let out: [64]u8 = undefined;
        expect_eq(formatTimeago(0, TimeagoLocale.En, TimeagoStyle.Short, out[0..64]), "now");
        expect_eq(formatTimeago(-300, TimeagoLocale.En, TimeagoStyle.Short, out[0..64]), "5m ago");
        expect_eq(formatTimeago(7200, TimeagoLocale.En, TimeagoStyle.Short, out[0..64]), "in 2h");
        expect_eq(formatTimeago(-2592000, TimeagoLocale.En, TimeagoStyle.Short, out[0..64]), "1mo ago");

        expect_eq(formatTimeago(0, TimeagoLocale.Zh, TimeagoStyle.Short, out[0..64]), "刚刚");
        expect_eq(formatTimeago(-300, TimeagoLocale.Zh, TimeagoStyle.Short, out[0..64]), "5分前");
        expect_eq(formatTimeago(7200, TimeagoLocale.Zh, TimeagoStyle.Short, out[0..64]), "2时后");
    });

    test("5. @cap/timeago — Ultra-compact CLI/monitoring formatCompact", () => {
        let out: [64]u8 = undefined;
        expect_eq(formatCompact(0, out[0..64]), "now");
        expect_eq(formatCompact(-9, out[0..64]), "now");
        expect_eq(formatCompact(-45, out[0..64]), "-45s");
        expect_eq(formatCompact(45, out[0..64]), "+45s");
        expect_eq(formatCompact(-300, out[0..64]), "-5m");
        expect_eq(formatCompact(7200, out[0..64]), "+2h");
        expect_eq(formatCompact(-259200, out[0..64]), "-3d");
        expect_eq(formatCompact(-31536000, out[0..64]), "-1y");
    });

    test("6. @cap/timeago — formatBetween & unit boundary transitions", () => {
        let out: [64]u8 = undefined;
        const now: i64 = 1700000000;
        expect_eq(
            formatBetween(now - 59, now, TimeagoLocale.En, TimeagoStyle.Long, out[0..64]),
            "59 seconds ago",
        );
        expect_eq(
            formatBetween(now - 60, now, TimeagoLocale.En, TimeagoStyle.Long, out[0..64]),
            "1 minute ago",
        );
        expect_eq(
            formatBetween(now + 3599, now, TimeagoLocale.En, TimeagoStyle.Long, out[0..64]),
            "in 59 minutes",
        );
        expect_eq(
            formatBetween(now + 3600, now, TimeagoLocale.En, TimeagoStyle.Long, out[0..64]),
            "in 1 hour",
        );
        expect_eq(
            formatBetween(now - 86400, now, TimeagoLocale.Zh, TimeagoStyle.Long, out[0..64]),
            "1 天前",
        );
    });
}
