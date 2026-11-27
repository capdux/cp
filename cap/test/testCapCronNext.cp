// cap/test/testCapCronNext.cp
// Comprehensive unit tests for @cap/cron-next

import { test, expect_eq } from "cap:test";
import {
    CronSchedule,
} from "../../packages/cron/src/index.cp";

fn main() {
    test("1. @cap/cron-next — Every minute (* * * * *) aligns to next whole minute", () => {
        const sched = CronSchedule.parse("* * * * *");
        expect_eq(sched.isValid(), true);
        // 2025-01-01T00:00:15Z = 1735689615 -> next minute 1735689660
        expect_eq(sched.nextAfter(1735689615 as i64), 1735689660 as i64);
        // Exact minute boundary strictly advances to +60s
        expect_eq(sched.nextAfter(1735689660 as i64), 1735689720 as i64);
        expect_eq(sched.matchesUtc(1735689660 as i64), true);
    });

    test("2. @cap/cron-next — Weekday business hours (*/15 9-17 * * 1-5) skips weekend", () => {
        const sched = CronSchedule.parse("*/15 9-17 * * 1-5");
        expect_eq(sched.isValid(), true);
        // 2025-01-03 is Friday. 2025-01-03T17:45:00Z = 1735926300 (last slot Friday)
        expect_eq(sched.matchesUtc(1735926300 as i64), true);
        // Next trigger must jump over Saturday (Jan 4) & Sunday (Jan 5) to Monday 2025-01-06T09:00:00Z = 1736154000
        expect_eq(sched.nextAfter(1735926300 as i64), 1736154000 as i64);
    });

    test("3. @cap/cron-next — Leap year Feb 29 (0 0 29 2 *) hits 2028-02-29T00:00:00Z", () => {
        const sched = CronSchedule.parse("0 0 29 2 *");
        expect_eq(sched.isValid(), true);
        // From 2025-01-01T00:00:00Z (1735689600) -> 2028-02-29T00:00:00Z (1835395200)
        expect_eq(sched.nextAfter(1735689600 as i64), 1835395200 as i64);
        expect_eq(sched.matchesUtc(1835395200 as i64), true);

        // Impossible date Feb 31 returns -1
        const impossible = CronSchedule.parse("0 0 31 2 *");
        expect_eq(impossible.isValid(), true);
        expect_eq(impossible.nextAfter(1735689600 as i64), -1 as i64);
    });

    test("4. @cap/cron-next — @daily & @hourly macro aliases", () => {
        const daily = CronSchedule.parse("@daily");
        expect_eq(daily.isValid(), true);
        // 2025-01-01T00:00:00Z (1735689600) -> 2025-01-02T00:00:00Z (1735776000)
        expect_eq(daily.nextAfter(1735689600 as i64), 1735776000 as i64);

        const hourly = CronSchedule.parse("@hourly");
        expect_eq(hourly.isValid(), true);
        expect_eq(hourly.nextAfter(1735689600 as i64), 1735693200 as i64);
    });

    test("5. @cap/cron-next — Multi-list & Sunday 7->0 mapping (0,30 8,20 * * 0,7)", () => {
        const sched = CronSchedule.parse("0,30 8,20 * * 0,7");
        expect_eq(sched.isValid(), true);
        // 2025-01-05 is Sunday: 2025-01-05T08:00:00Z = 1736064000, 08:30:00Z = 1736065800
        expect_eq(sched.matchesUtc(1736064000 as i64), true);
        expect_eq(sched.nextAfter(1736064000 as i64), 1736065800 as i64);
        // 2025-01-06 is Monday -> must NOT match
        expect_eq(sched.matchesUtc(1736150400 as i64), false);
    });

    test("6. @cap/cron-next — Reject invalid field count, out-of-range values & zero step", () => {
        expect_eq(CronSchedule.parse("").isValid(), false);
        expect_eq(CronSchedule.parse("* * * *").isValid(), false);
        expect_eq(CronSchedule.parse("* * * * * *").isValid(), false);
        expect_eq(CronSchedule.parse("60 * * * *").isValid(), false);
        expect_eq(CronSchedule.parse("* 24 * * *").isValid(), false);
        expect_eq(CronSchedule.parse("* * 0 * *").isValid(), false);
        expect_eq(CronSchedule.parse("* * * 13 *").isValid(), false);
        expect_eq(CronSchedule.parse("* * * * 8").isValid(), false);
        expect_eq(CronSchedule.parse("*/0 * * * *").isValid(), false);
        expect_eq(CronSchedule.parse("5-2 * * * *").isValid(), false);
        expect_eq(CronSchedule.parse("bad").nextAfter(1735689600 as i64), -1 as i64);
    });
}
