// cap/test/testCapCronExpr.cp
// Comprehensive unit tests for @cap/cron-expr (5-field Unix Cron)

import { test, expect_eq } from "cap:test";
import {
    CronSchedule,
    parseCron,
    matches,
    isValidCron,
    countMatchingMinutesPerDay,
} from "../../packages/@cap/cron-expr/src/index.cp";

fn main() {
    test("1. @cap/cron-expr — wildcard '* * * * *' & workday '*/15 9-17 * * 1-5'", () => {
        const every: CronSchedule = parseCron("* * * * *");
        expect_eq(every.valid, true);
        expect_eq(matches(every, 0, 0, 1, 1, 0), true);
        expect_eq(matches(every, 59, 23, 31, 12, 6), true);

        const work: CronSchedule = parseCron("*/15 9-17 * * 1-5");
        expect_eq(work.valid, true);
        expect_eq(matches(work, 0, 9, 10, 6, 1), true);
        expect_eq(matches(work, 45, 17, 10, 6, 5), true);
        expect_eq(matches(work, 10, 9, 10, 6, 1), false); // minute 10 not multiple of 15
        expect_eq(matches(work, 0, 18, 10, 6, 1), false); // hour 18 outside 9-17
        expect_eq(matches(work, 0, 12, 10, 6, 0), false); // Sunday excluded
    });

    test("2. @cap/cron-expr — comma list & stepped range '0,30 8-18/2 * * *'", () => {
        const s: CronSchedule = parseCron("0,30 8-18/2 * * *");
        expect_eq(s.valid, true);
        expect_eq(matches(s, 0, 8, 1, 1, 1), true);
        expect_eq(matches(s, 30, 10, 1, 1, 1), true);
        expect_eq(matches(s, 30, 18, 1, 1, 1), true);
        expect_eq(matches(s, 0, 9, 1, 1, 1), false);
        expect_eq(matches(s, 15, 8, 1, 1, 1), false);
    });

    test("3. @cap/cron-expr — Sunday dow=7 normalizes to 0", () => {
        const s1: CronSchedule = parseCron("0 12 * * 7");
        expect_eq(s1.valid, true);
        expect_eq(matches(s1, 0, 12, 1, 1, 0), true);
        expect_eq(matches(s1, 0, 12, 1, 1, 7), true);
        expect_eq(matches(s1, 0, 12, 1, 1, 1), false);

        const s2: CronSchedule = parseCron("0 12 * * 5-7");
        expect_eq(s2.valid, true);
        expect_eq(matches(s2, 0, 12, 1, 1, 5), true);
        expect_eq(matches(s2, 0, 12, 1, 1, 6), true);
        expect_eq(matches(s2, 0, 12, 1, 1, 0), true);
    });

    test("4. @cap/cron-expr — standard macros @daily, @hourly, @weekly, @monthly, @yearly", () => {
        const daily: CronSchedule = parseCron("@daily");
        expect_eq(daily.valid, true);
        expect_eq(matches(daily, 0, 0, 15, 7, 3), true);
        expect_eq(matches(daily, 1, 0, 15, 7, 3), false);

        const hourly: CronSchedule = parseCron("@hourly");
        expect_eq(hourly.valid, true);
        expect_eq(matches(hourly, 0, 14, 15, 7, 3), true);
        expect_eq(matches(hourly, 5, 14, 15, 7, 3), false);

        const weekly: CronSchedule = parseCron("@weekly");
        expect_eq(weekly.valid, true);
        expect_eq(matches(weekly, 0, 0, 15, 7, 0), true);
        expect_eq(matches(weekly, 0, 0, 15, 7, 2), false);

        const yearly: CronSchedule = parseCron("@yearly");
        expect_eq(yearly.valid, true);
        expect_eq(matches(yearly, 0, 0, 1, 1, 4), true);
        expect_eq(matches(yearly, 0, 0, 2, 1, 4), false);
    });

    test("5. @cap/cron-expr — countMatchingMinutesPerDay popcount accuracy", () => {
        expect_eq(countMatchingMinutesPerDay(parseCron("* * * * *")), 1440 as u32);
        expect_eq(countMatchingMinutesPerDay(parseCron("@daily")), 1 as u32);
        expect_eq(countMatchingMinutesPerDay(parseCron("@hourly")), 24 as u32);
        // 4 minutes/hour (0,15,30,45) * 9 hours (9..17) = 36
        expect_eq(countMatchingMinutesPerDay(parseCron("*/15 9-17 * * 1-5")), 36 as u32);
        // 2 minutes/hour (0,30) * 6 hours (8,10,12,14,16,18) = 12
        expect_eq(countMatchingMinutesPerDay(parseCron("0,30 8-18/2 * * *")), 12 as u32);
    });

    test("6. @cap/cron-expr — rejects wrong field count, out-of-range values & step=0", () => {
        expect_eq(isValidCron(""), false);
        expect_eq(isValidCron("* * * *"), false);
        expect_eq(isValidCron("* * * * * *"), false);
        expect_eq(isValidCron("60 * * * *"), false);
        expect_eq(isValidCron("* 24 * * *"), false);
        expect_eq(isValidCron("* * 0 * *"), false);
        expect_eq(isValidCron("* * 32 * *"), false);
        expect_eq(isValidCron("* * * 13 *"), false);
        expect_eq(isValidCron("* * * * 8"), false);
        expect_eq(isValidCron("*/0 * * * *"), false);
        expect_eq(isValidCron("10-5 * * * *"), false);
        expect_eq(isValidCron("@unknown"), false);

        // Out-of-bounds match args return false
        const all: CronSchedule = parseCron("* * * * *");
        expect_eq(matches(all, 60, 0, 1, 1, 0), false);
        expect_eq(matches(all, 0, 24, 1, 1, 0), false);
        expect_eq(matches(all, 0, 0, 0, 1, 0), false);
        expect_eq(matches(all, 0, 0, 1, 13, 0), false);
    });
}
