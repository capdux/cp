import { test, expect_eq } from "cap:test";
import {
    IsoDate, IsoTime, IsoDateTime
} from "../../packages/@cap/iso8601/src/index.cp";
import {
    Rfc3339Timestamp, isValidRfc3339, parseRfc3339,
    daysFromCivil, civilFromDays, fromUnixSeconds, formatRfc3339Utc
} from "../../packages/@cap/rfc3339/src/index.cp";

fn main() {
    test("1. @cap/rfc3339 — Unix epoch 1970-01-01T00:00:00Z parses to 0", () => {
        let ts_opt = parseRfc3339("1970-01-01T00:00:00Z");
        expect_eq(ts_opt != null, true);
        let ts = ts_opt ?? Rfc3339Timestamp {
            dt: IsoDateTime {
                date: IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 0 },
                time: IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 0, has_tz: false },
            },
            unix_seconds: -1,
            millisecond: 0,
        };
        expect_eq(ts.unix_seconds, 0);
        expect_eq(ts.millisecond, 0);
    });

    test("2. @cap/rfc3339 — timezone offset normalization (+02:00 vs Z)", () => {
        let a = parseRfc3339("2024-03-01T12:00:00Z");
        let b = parseRfc3339("2024-03-01T14:00:00+02:00");
        let c = parseRfc3339("2024-03-01T07:00:00-05:00");
        let fallback = Rfc3339Timestamp {
            dt: IsoDateTime {
                date: IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 0 },
                time: IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 0, has_tz: false },
            },
            unix_seconds: -1,
            millisecond: 0,
        };
        let sa = (a ?? fallback).unix_seconds;
        let sb = (b ?? fallback).unix_seconds;
        let sc = (c ?? fallback).unix_seconds;
        expect_eq(sa, sb);
        expect_eq(sa, sc);
        expect_eq(sa, 1709294400);
    });

    test("3. @cap/rfc3339 — fractional seconds & space/lowercase separator", () => {
        let ts_opt = parseRfc3339("2026-11-27 06:40:15.125z");
        expect_eq(ts_opt != null, true);
        let ts = ts_opt ?? Rfc3339Timestamp {
            dt: IsoDateTime {
                date: IsoDate { year: 0, month: 0, day: 0, ordinal: 0, week: 0, weekday: 0, kind: 0 },
                time: IsoTime { hour: 0, minute: 0, second: 0, millisecond: 0, tz_offset_minutes: 0, has_tz: false },
            },
            unix_seconds: 0,
            millisecond: 0,
        };
        expect_eq(ts.millisecond, 125);
        expect_eq(ts.dt.date.year, 2026);
    });

    test("4. @cap/rfc3339 — formatRfc3339Utc roundtrip from Unix seconds", () => {
        let buf: [64]u8 = [0; 64];
        let n = formatRfc3339Utc(1709294400, buf[0..64]);
        expect_eq(buf[0..n] as string, "2024-03-01T12:00:00Z");

        let n2 = formatRfc3339Utc(0, buf[0..64]);
        expect_eq(buf[0..n2] as string, "1970-01-01T00:00:00Z");
    });

    test("5. @cap/rfc3339 — pre-epoch negative Unix timestamps & civilFromDays", () => {
        let d = civilFromDays(-1);
        expect_eq(d.year, 1969);
        expect_eq(d.month, 12);
        expect_eq(d.day, 31);
        expect_eq(daysFromCivil(1969, 12, 31), -1);

        let dt = fromUnixSeconds(-1);
        expect_eq(dt.date.year, 1969);
        expect_eq(dt.time.hour, 23);
        expect_eq(dt.time.minute, 59);
        expect_eq(dt.time.second, 59);
    });

    test("6. @cap/rfc3339 — strict RFC 3339 rejects missing TZ, missing colon, or basic ISO", () => {
        expect_eq(isValidRfc3339("2024-03-01T12:00:00"), false);
        expect_eq(isValidRfc3339("2024-03-01T12:00:00+0200"), false);
        expect_eq(isValidRfc3339("20240301T120000Z"), false);
        expect_eq(parseRfc3339("2023-02-29T00:00:00Z") == null, true);
    });
}
