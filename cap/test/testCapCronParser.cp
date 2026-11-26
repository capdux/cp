import { test, expect_eq } from "cap:test";
import {
    CronSchedule,
    parseCron,
    matches,
    nextMinuteOffset,
    isValidCron,
} from "../../packages/@cap/cron-parser/src/index.cp";

fn main() {
    test("1. @cap/cron-parser — Wildcards, steps, ranges & comma lists", () => {
        const s = parseCron("*/15 9-17 * 1,6,12 1-5");
        expect_eq(s.valid, true);
        expect_eq(matches(s, 1, 10, 3, 9, 0), true);
        expect_eq(matches(s, 6, 15, 5, 17, 45), true);
        expect_eq(matches(s, 12, 20, 1, 12, 30), true);

        // Wrong minute, wrong hour, wrong month, weekend
        expect_eq(matches(s, 1, 10, 3, 9, 10), false);
        expect_eq(matches(s, 1, 10, 3, 18, 0), false);
        expect_eq(matches(s, 2, 10, 3, 9, 0), false);
        expect_eq(matches(s, 1, 10, 0, 9, 0), false);
    });

    test("2. @cap/cron-parser — Range with step (a-b/step) & Sunday 7 -> 0 normalization", () => {
        const s = parseCron("10-30/10 8 * * 7");
        expect_eq(s.valid, true);
        expect_eq(matches(s, 3, 12, 0, 8, 10), true);
        expect_eq(matches(s, 3, 12, 7, 8, 20), true);
        expect_eq(matches(s, 3, 12, 0, 8, 30), true);
        expect_eq(matches(s, 3, 12, 0, 8, 40), false);
        expect_eq(matches(s, 3, 12, 1, 8, 10), false);
    });

    test("3. @cap/cron-parser — Preset aliases (@yearly/@annually/@monthly/@weekly/@daily/@midnight/@hourly)", () => {
        const yearly = parseCron("@yearly");
        expect_eq(yearly.valid, true);
        expect_eq(matches(yearly, 1, 1, 3, 0, 0), true);
        expect_eq(matches(yearly, 2, 1, 3, 0, 0), false);

        const annually = parseCron("@annually");
        expect_eq(annually.valid, true);
        expect_eq(annually.month_mask, yearly.month_mask);

        const monthly = parseCron("@monthly");
        expect_eq(monthly.valid, true);
        expect_eq(matches(monthly, 7, 1, 2, 0, 0), true);
        expect_eq(matches(monthly, 7, 2, 2, 0, 0), false);

        const weekly = parseCron("@weekly");
        expect_eq(weekly.valid, true);
        expect_eq(matches(weekly, 5, 14, 0, 0, 0), true);
        expect_eq(matches(weekly, 5, 15, 1, 0, 0), false);

        const daily = parseCron("@daily");
        const midnight = parseCron("@midnight");
        expect_eq(daily.valid, true);
        expect_eq(midnight.valid, true);
        expect_eq(matches(daily, 8, 9, 4, 0, 0), true);
        expect_eq(matches(daily, 8, 9, 4, 1, 0), false);

        const hourly = parseCron("@hourly");
        expect_eq(hourly.valid, true);
        expect_eq(matches(hourly, 8, 9, 4, 15, 0), true);
        expect_eq(matches(hourly, 8, 9, 4, 15, 1), false);
    });

    test("4. @cap/cron-parser — Strict rejection of invalid expressions", () => {
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
        expect_eq(isValidCron("1,,2 * * * *"), false);
    });

    test("5. @cap/cron-parser — nextMinuteOffset within 24 hours including day/month rollover", () => {
        const every15 = parseCron("*/15 * * * *");
        expect_eq(nextMinuteOffset(every15, 4, 10, 2, 10, 7) ?? 0, 8);
        expect_eq(nextMinuteOffset(every15, 4, 10, 2, 10, 15) ?? 0, 15);

        // Midnight rollover across Jan 31 -> Feb 1 (23:55 -> 00:05 = 10 min)
        const feb1 = parseCron("5 0 1 2 *");
        expect_eq(nextMinuteOffset(feb1, 1, 31, 3, 23, 55) ?? 0, 10);

        // Daily at 00:00 from 00:00 -> next trigger is 1440 minutes later
        const daily = parseCron("@daily");
        expect_eq(nextMinuteOffset(daily, 5, 10, 1, 0, 0) ?? 0, 1440);
    });

    test("6. @cap/cron-parser — nextMinuteOffset returns null when not triggered within 24h", () => {
        const dec25 = parseCron("0 9 25 12 *");
        expect_eq(nextMinuteOffset(dec25, 6, 1, 1, 12, 0) == null, true);

        const inv = parseCron("invalid cron");
        expect_eq(nextMinuteOffset(inv, 6, 1, 1, 12, 0) == null, true);
    });
}
