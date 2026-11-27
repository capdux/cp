import { test, expect_eq } from "cap:test";
import {
    CronExpr6,
    parseCron6,
    matches6,
    nextSecInMinute,
    isValidCron6,
} from "../../packages/cron/src/index.cp";

fn main() {
    test("1. @cap/cron-expr — 6-field second step expression (*/15 * * * * *)", () => {
        const s = parseCron6("*/15 * * * * *");
        expect_eq(s.valid, true);

        expect_eq(matches6(s, 5, 10, 2, 14, 30, 0), true);
        expect_eq(matches6(s, 5, 10, 2, 14, 30, 15), true);
        expect_eq(matches6(s, 5, 10, 2, 14, 30, 30), true);
        expect_eq(matches6(s, 5, 10, 2, 14, 30, 45), true);
        expect_eq(matches6(s, 5, 10, 2, 14, 30, 14), false);
        expect_eq(matches6(s, 5, 10, 2, 14, 30, 59), false);
    });

    test("2. @cap/cron-expr — 6-field lists, ranges & stepped ranges on seconds", () => {
        const s = parseCron6("5,10-20/5,55 0 12 * * 1-5");
        expect_eq(s.valid, true);

        expect_eq(matches6(s, 3, 15, 1, 12, 0, 5), true);
        expect_eq(matches6(s, 3, 15, 1, 12, 0, 10), true);
        expect_eq(matches6(s, 3, 15, 1, 12, 0, 15), true);
        expect_eq(matches6(s, 3, 15, 1, 12, 0, 20), true);
        expect_eq(matches6(s, 3, 15, 1, 12, 0, 55), true);
        expect_eq(matches6(s, 3, 15, 1, 12, 0, 12), false);
        // Weekend (dow=0) should not match 1-5
        expect_eq(matches6(s, 3, 15, 0, 12, 0, 5), false);
    });

    test("3. @cap/cron-expr — 5-field compatibility auto-defaults seconds to 0", () => {
        const s = parseCron6("30 8 * * *");
        expect_eq(s.valid, true);
        expect_eq(s.sec_mask, 1 as u64);

        expect_eq(matches6(s, 1, 1, 1, 8, 30, 0), true);
        expect_eq(matches6(s, 1, 1, 1, 8, 30, 1), false);
        expect_eq(matches6(s, 1, 1, 1, 8, 31, 0), false);
    });

    test("4. @cap/cron-expr — preset aliases (@daily, @hourly) default to second 0", () => {
        const daily = parseCron6("@daily");
        expect_eq(daily.valid, true);
        expect_eq(matches6(daily, 7, 4, 2, 0, 0, 0), true);
        expect_eq(matches6(daily, 7, 4, 2, 0, 0, 1), false);
        expect_eq(matches6(daily, 7, 4, 2, 1, 0, 0), false);

        const hourly = parseCron6("@hourly");
        expect_eq(hourly.valid, true);
        expect_eq(matches6(hourly, 7, 4, 2, 13, 0, 0), true);
        expect_eq(matches6(hourly, 7, 4, 2, 13, 0, 30), false);
    });

    test("5. @cap/cron-expr — nextSecInMinute lookup & boundary handling", () => {
        const s = parseCron6("*/20 * * * * *"); // 0, 20, 40
        expect_eq(nextSecInMinute(s, 0) ?? 255, 0 as u8);
        expect_eq(nextSecInMinute(s, 1) ?? 255, 20 as u8);
        expect_eq(nextSecInMinute(s, 20) ?? 255, 20 as u8);
        expect_eq(nextSecInMinute(s, 21) ?? 255, 40 as u8);
        expect_eq(nextSecInMinute(s, 41) == null, true);
        expect_eq(nextSecInMinute(s, 60) == null, true);
    });

    test("6. @cap/cron-expr — isValidCron6 rejects malformed & out-of-range expressions", () => {
        expect_eq(isValidCron6("0 0 * * * *"), true);
        expect_eq(isValidCron6("  */10 5 * * * *  "), true);
        expect_eq(isValidCron6("@weekly"), true);

        expect_eq(isValidCron6(""), false);
        expect_eq(isValidCron6("60 * * * * *"), false);       // sec > 59
        expect_eq(isValidCron6("*/0 * * * * *"), false);      // step == 0
        expect_eq(isValidCron6("1 2 3 4"), false);            // 4 fields
        expect_eq(isValidCron6("1 2 3 4 5 6 7"), false);      // 7 fields
        expect_eq(isValidCron6("@unknown"), false);
    });
}
