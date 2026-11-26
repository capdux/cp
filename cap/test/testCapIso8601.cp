import { test, expect_eq } from "cap:test";
import {
    IsoDate, IsoTime, IsoDateTime, IsoDuration,
    isLeapYear, daysInMonth, parseDate, parseTime,
    parseDateTime, parseDuration, formatDateTime
} from "../../packages/@cap/iso8601/src/index.cp";

fn main() {
    test("1. @cap/iso8601 — calendar dates (extended & basic) + leap year validation", () => {
        expect_eq(isLeapYear(2024), true);
        expect_eq(isLeapYear(1900), false);
        expect_eq(isLeapYear(2000), true);
        expect_eq(daysInMonth(2024, 2), 29);

        let d1 = parseDate("2024-02-29") ?? IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 9 };
        expect_eq(d1.year, 2024);
        expect_eq(d1.month, 2);
        expect_eq(d1.day, 29);
        expect_eq(d1.kind, 0);

        let d2 = parseDate("20261127") ?? IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 9 };
        expect_eq(d2.year, 2026);
        expect_eq(d2.month, 11);
        expect_eq(d2.day, 27);

        expect_eq(parseDate("2023-02-29") == null, true);
    });

    test("2. @cap/iso8601 — week dates & ordinal dates", () => {
        let w = parseDate("2026-W48-5") ?? IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 9 };
        expect_eq(w.kind, 1);
        expect_eq(w.year, 2026);
        expect_eq(w.week, 48);
        expect_eq(w.weekday, 5);

        let o = parseDate("2024-366") ?? IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 9 };
        expect_eq(o.kind, 2);
        expect_eq(o.ordinal, 366);
        expect_eq(parseDate("2023-366") == null, true);
    });

    test("3. @cap/iso8601 — time with fractional ms and timezone offsets", () => {
        let t1 = parseTime("14:35:09.25Z") ?? IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 99, has_tz: false };
        expect_eq(t1.hour, 14);
        expect_eq(t1.minute, 35);
        expect_eq(t1.second, 9);
        expect_eq(t1.millisecond, 250);
        expect_eq(t1.has_tz, true);
        expect_eq(t1.tz_offset_minutes, 0);

        let t2 = parseTime("08:00:00+11:00") ?? IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 0, has_tz: false };
        expect_eq(t2.tz_offset_minutes, 660);

        let t3 = parseTime("23:15-0530") ?? IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 0, has_tz: false };
        expect_eq(t3.tz_offset_minutes, -330);
    });

    test("4. @cap/iso8601 — datetime parsing & roundtrip formatting", () => {
        let dt_opt = parseDateTime("2026-11-27T06:40:15+11:00");
        expect_eq(dt_opt != null, true);
        let dt = dt_opt ?? IsoDateTime {
            date: IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 0 },
            time: IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 0, has_tz: false },
        };
        let buf: [64]u8 = [0; 64];
        let n = formatDateTime(dt, buf[0..64]);
        expect_eq(buf[0..n] as string, "2026-11-27T06:40:15+11:00");
    });

    test("5. @cap/iso8601 — ISO-8601 duration parsing", () => {
        let d1 = parseDuration("P1Y2M3DT4H5M6.78S") ?? IsoDuration { years: 0, months: 0, weeks: 0, days: 0, hours: 0, minutes: 0, seconds: 0, milliseconds: 0 };
        expect_eq(d1.years, 1);
        expect_eq(d1.months, 2);
        expect_eq(d1.days, 3);
        expect_eq(d1.hours, 4);
        expect_eq(d1.minutes, 5);
        expect_eq(d1.seconds, 6);
        expect_eq(d1.milliseconds, 780);

        let d2 = parseDuration("P3W") ?? IsoDuration { years: 0, months: 0, weeks: 0, days: 0, hours: 0, minutes: 0, seconds: 0, milliseconds: 0 };
        expect_eq(d2.weeks, 3);
    });

    test("6. @cap/iso8601 — rejects malformed dates, times & durations", () => {
        expect_eq(parseDate("2026-13-01") == null, true);
        expect_eq(parseTime("25:00:00Z") == null, true);
        expect_eq(parseDateTime("not-a-date") == null, true);
        expect_eq(parseDuration("P") == null, true);
        expect_eq(parseDuration("1Y2M") == null, true);
    });
}
