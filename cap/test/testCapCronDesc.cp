// cap/test/testCapCronDesc.cp
// Comprehensive unit tests for @cap/cron-desc

import { test, expect_eq } from "cap:test";
import {
    Locale,
    CronDescOptions,
    isValidCron,
    describeCron,
    describeCronWith,
} from "../../packages/@cap/cron-desc/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn main() {
    test("1. @cap/cron-desc — Every minute & step intervals (EN / ZH)", () => {
        let buf: [128]u8 = [0; 128];
        const zh = CronDescOptions.chinese();

        expect_eq(strEq(describeCron("* * * * *", buf[0..128]), "Every minute"), true);
        expect_eq(strEq(describeCronWith("* * * * *", zh, buf[0..128]), "每分钟"), true);

        expect_eq(strEq(describeCron("*/15 * * * *", buf[0..128]), "Every 15 minutes"), true);
        expect_eq(strEq(describeCronWith("*/15 * * * *", zh, buf[0..128]), "每 15 分钟"), true);

        expect_eq(strEq(describeCron("0 */2 * * *", buf[0..128]), "Every 2 hours"), true);
        expect_eq(strEq(describeCronWith("0 */2 * * *", zh, buf[0..128]), "每 2 小时"), true);
    });

    test("2. @cap/cron-desc — Fixed times & weekday ranges (EN / ZH / 12h)", () => {
        let buf: [128]u8 = [0; 128];
        const zh = CronDescOptions.chinese();
        const en12 = CronDescOptions{ locale: Locale.En, use_24h: false };
        const zh12 = CronDescOptions{ locale: Locale.Zh, use_24h: false };

        expect_eq(strEq(describeCron("0 9 * * 1-5", buf[0..128]), "At 09:00, Monday through Friday"), true);
        expect_eq(strEq(describeCronWith("0 9 * * 1-5", zh, buf[0..128]), "在 09:00，周一至周五"), true);

        expect_eq(strEq(describeCron("0 9 * * MON-FRI", buf[0..128]), "At 09:00, Monday through Friday"), true);
        expect_eq(strEq(describeCronWith("30 14 * * 1-5", en12, buf[0..128]), "At 2:30 PM, Monday through Friday"), true);
        expect_eq(strEq(describeCronWith("30 14 * * 1-5", zh12, buf[0..128]), "在 下午 2:30，周一至周五"), true);
    });

    test("3. @cap/cron-desc — Day of month, months & comma lists", () => {
        let buf: [128]u8 = [0; 128];
        const zh = CronDescOptions.chinese();

        expect_eq(strEq(describeCron("30 14 1 * *", buf[0..128]), "At 14:30, on day 1 of the month"), true);
        expect_eq(strEq(describeCronWith("30 14 1 * *", zh, buf[0..128]), "在 14:30，每月第 1 天"), true);

        expect_eq(
            strEq(
                describeCron("0 8 1,15 JAN,JUN *", buf[0..128]),
                "At 08:00, on days 1, 15 of the month, only in January, June"
            ),
            true
        );
        expect_eq(
            strEq(
                describeCronWith("0 8 1,15 JAN,JUN *", zh, buf[0..128]),
                "在 08:00，每月第 1、15 天，仅在 1 月、6 月"
            ),
            true
        );

        expect_eq(
            strEq(
                describeCron("0 12 * * MON,WED,FRI", buf[0..128]),
                "At 12:00, only on Monday, Wednesday, Friday"
            ),
            true
        );
        expect_eq(
            strEq(
                describeCronWith("0 12 * * MON,WED,FRI", zh, buf[0..128]),
                "在 12:00，仅在周一、周三、周五"
            ),
            true
        );
    });

    test("4. @cap/cron-desc — Predefined macros (@daily/@hourly/@weekly/@monthly/@yearly)", () => {
        let buf: [128]u8 = [0; 128];
        const zh = CronDescOptions.chinese();

        expect_eq(strEq(describeCron("@daily", buf[0..128]), "At 00:00, every day"), true);
        expect_eq(strEq(describeCronWith("@daily", zh, buf[0..128]), "每天 00:00"), true);
        expect_eq(strEq(describeCron("@midnight", buf[0..128]), "At 00:00, every day"), true);

        expect_eq(strEq(describeCron("@hourly", buf[0..128]), "Every hour"), true);
        expect_eq(strEq(describeCronWith("@hourly", zh, buf[0..128]), "每小时"), true);

        expect_eq(strEq(describeCron("@weekly", buf[0..128]), "At 00:00, only on Sunday"), true);
        expect_eq(strEq(describeCronWith("@weekly", zh, buf[0..128]), "在 00:00，仅在周日"), true);

        expect_eq(strEq(describeCron("@monthly", buf[0..128]), "At 00:00, on day 1 of the month"), true);
        expect_eq(strEq(describeCronWith("@monthly", zh, buf[0..128]), "在 00:00，每月第 1 天"), true);

        expect_eq(strEq(describeCron("@yearly", buf[0..128]), "At 00:00, on day 1 of the month, only in January"), true);
        expect_eq(strEq(describeCronWith("@annually", zh, buf[0..128]), "在 00:00，每月第 1 天，仅在 1 月"), true);
    });

    test("5. @cap/cron-desc — Named ranges, Sunday=7 & complex minute/hour combinations", () => {
        let buf: [128]u8 = [0; 128];
        const zh = CronDescOptions.chinese();

        // Sunday as 7
        expect_eq(strEq(describeCron("0 10 * * 7", buf[0..128]), "At 10:00, only on Sunday"), true);
        expect_eq(strEq(describeCron("0 10 * * 5-7", buf[0..128]), "At 10:00, Friday through Sunday"), true);

        // Month range
        expect_eq(strEq(describeCron("0 6 1 MAR-MAY *", buf[0..128]), "At 06:00, on day 1 of the month, March through May"), true);
        expect_eq(strEq(describeCronWith("0 6 1 MAR-MAY *", zh, buf[0..128]), "在 06:00，每月第 1 天，3 月至5 月"), true);

        // Step minutes within hour range
        expect_eq(strEq(describeCron("*/10 9-17 * * *", buf[0..128]), "Every 10 minutes, between 09:00 and 17:59"), true);
        expect_eq(strEq(describeCronWith("*/10 9-17 * * *", zh, buf[0..128]), "每 10 分钟，09:00 至 17:59"), true);
    });

    test("6. @cap/cron-desc — Invalid expression rejection", () => {
        let buf: [128]u8 = [0; 128];

        expect_eq(isValidCron(""), false);
        expect_eq(isValidCron("   "), false);
        expect_eq(isValidCron("* * * *"), false);          // only 4 fields
        expect_eq(isValidCron("* * * * * *"), false);      // 6 fields
        expect_eq(isValidCron("60 * * * *"), false);       // minute > 59
        expect_eq(isValidCron("0 24 * * *"), false);       // hour > 23
        expect_eq(isValidCron("0 0 0 * *"), false);        // dom < 1
        expect_eq(isValidCron("0 0 32 * *"), false);       // dom > 31
        expect_eq(isValidCron("0 0 * 13 *"), false);       // month > 12
        expect_eq(isValidCron("0 0 * * 8"), false);        // dow > 7
        expect_eq(isValidCron("*/0 * * * *"), false);      // step == 0
        expect_eq(isValidCron("5-2 * * * *"), false);      // reversed range
        expect_eq(isValidCron("1,,2 * * * *"), false);     // empty list item
        expect_eq(isValidCron("@unknown"), false);         // unknown macro
        expect_eq(isValidCron("0 0 * FOO *"), false);      // invalid month name

        expect_eq(strEq(describeCron("invalid cron", buf[0..128]), ""), true);
        expect_eq(strEq(describeCronWith("60 * * * *", CronDescOptions.chinese(), buf[0..128]), ""), true);
    });
}
