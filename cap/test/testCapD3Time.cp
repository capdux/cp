import { test, expect_eq } from "cap:test";
import {
    utcFromYmd,
    timeSecondFloor,
    timeSecondCeil,
    timeSecondOffset,
    timeSecondCount,
    timeMinuteFloor,
    timeMinuteOffset,
    timeHourFloor,
    timeHourOffset,
    timeDayFloor,
    timeDayCeil,
    timeDayOffset,
    timeDayCount,
    timeWeekFloor,
    timeWeekOffset,
    timeMonthFloor,
    timeMonthOffset,
    timeYearFloor,
    timeYearOffset,
} from "../../packages/@cap/d3-time/src/index.cp";

fn main() {
    test("1. @cap/d3-time — Second floor, ceil, offset & count (pos and neg)", () => {
        expect_eq(timeSecondFloor(1500), 1000 as i64);
        expect_eq(timeSecondCeil(1000), 1000 as i64);
        expect_eq(timeSecondCeil(1001), 2000 as i64);
        expect_eq(timeSecondOffset(1500, 3), 4500 as i64);
        expect_eq(timeSecondCount(1200, 5800), 4 as i64);
        // Pre-1970 negative timestamp
        expect_eq(timeSecondFloor(-500), -1000 as i64);
        expect_eq(timeSecondCeil(-500), 0 as i64);
    });

    test("2. @cap/d3-time — Minute & Hour floor and offset", () => {
        const base = utcFromYmd(2024, 6, 15) + 3 * 3600000 + 27 * 60000 + 45000;
        expect_eq(timeMinuteFloor(base), utcFromYmd(2024, 6, 15) + 3 * 3600000 + 27 * 60000);
        expect_eq(timeMinuteOffset(base, -27), utcFromYmd(2024, 6, 15) + 3 * 3600000 + 45000);
        expect_eq(timeHourFloor(base), utcFromYmd(2024, 6, 15) + 3 * 3600000);
        expect_eq(timeHourOffset(timeHourFloor(base), 5), utcFromYmd(2024, 6, 15) + 8 * 3600000);
    });

    test("3. @cap/d3-time — Day floor, ceil, offset & count", () => {
        const d1 = utcFromYmd(2024, 2, 28);
        const d2 = utcFromYmd(2024, 3, 1);
        // 2024 is a leap year: Feb 28 -> Feb 29 -> Mar 1 = 2 days
        expect_eq(timeDayCount(d1, d2), 2 as i64);
        expect_eq(timeDayFloor(d1 + 12345), d1);
        expect_eq(timeDayCeil(d1), d1);
        expect_eq(timeDayCeil(d1 + 1), utcFromYmd(2024, 2, 29));
        expect_eq(timeDayOffset(d1, 2), d2);
    });

    test("4. @cap/d3-time — Week floor (Sunday) & offset", () => {
        // 1970-01-01 is Thursday -> Sunday is 1969-12-28
        expect_eq(timeWeekFloor(0), utcFromYmd(1969, 12, 28));
        // 2024-06-15 is Saturday -> Sunday is 2024-06-09
        const sat = utcFromYmd(2024, 6, 15) + 43200000;
        const sun = utcFromYmd(2024, 6, 9);
        expect_eq(timeWeekFloor(sat), sun);
        expect_eq(timeWeekOffset(sun, 2), utcFromYmd(2024, 6, 23));
    });

    test("5. @cap/d3-time — Month floor & offset across leap year and year boundary", () => {
        const mid = utcFromYmd(2024, 1, 31) + 5000;
        expect_eq(timeMonthFloor(mid), utcFromYmd(2024, 1, 1));
        // Jan 31 + 1 month in leap year 2024 -> Feb 29 + 5000ms
        expect_eq(timeMonthOffset(mid, 1), utcFromYmd(2024, 2, 29) + 5000);
        // Jan 1 - 1 month -> Dec 1 of 2023
        expect_eq(timeMonthOffset(utcFromYmd(2024, 1, 1), -1), utcFromYmd(2023, 12, 1));
    });

    test("6. @cap/d3-time — Year floor & offset (including Feb 29 -> non-leap year)", () => {
        const leap = utcFromYmd(2024, 2, 29) + 1000;
        expect_eq(timeYearFloor(leap), utcFromYmd(2024, 1, 1));
        expect_eq(timeYearOffset(leap, 1), utcFromYmd(2025, 2, 28) + 1000);
        expect_eq(timeYearOffset(leap, 4), utcFromYmd(2028, 2, 29) + 1000);
        expect_eq(timeYearFloor(utcFromYmd(1969, 7, 20)), utcFromYmd(1969, 1, 1));
    });
}
