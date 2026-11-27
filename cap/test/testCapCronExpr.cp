// cap/test/testCapCronExpr.cp
// Comprehensive unit tests for @cap/cron (5-field POSIX cron parsing & scheduling)

import { test, expect_eq } from "cap:test";
import { CronSchedule } from "../../packages/cron/src/index.cp";

fn main() {
    test("1. @cap/cron — parse, matchesUtc & nextAfter", () => {
        const s = CronSchedule.parse("*/15 9-17 * * 1-5");
        expect_eq(s.isValid(), true);
        // 2025-01-03T17:45:00Z (Friday)
        expect_eq(s.matchesUtc(1735926300 as i64), true);
        // Next trigger Monday 2025-01-06T09:00:00Z
        expect_eq(s.nextAfter(1735926300 as i64), 1736154000 as i64);
    });

    test("2. @cap/cron — rejects invalid cron expressions", () => {
        expect_eq(CronSchedule.parse("60 * * * *").isValid(), false);
        expect_eq(CronSchedule.parse("* 24 * * *").isValid(), false);
        expect_eq(CronSchedule.parse("* * *").isValid(), false);
        expect_eq(CronSchedule.parse("").isValid(), false);
    });
}
