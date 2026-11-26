import { test, expect_eq } from "cap:test";
import {
    parsePgInterval,
    toIsoDuration,
    toPostgresString,
    totalSecondsApprox,
} from "../../packages/@cap/postgres-interval/src/index.cp";

fn main() {
    test("1. @cap/postgres-interval — Full PostgreSQL interval with fractional seconds", () => {
        const iv = parsePgInterval("1 year 2 mons 3 days 04:05:06.789");
        expect_eq(iv.valid, true);
        expect_eq(iv.years, 1);
        expect_eq(iv.months, 2);
        expect_eq(iv.days, 3);
        expect_eq(iv.hours, 4);
        expect_eq(iv.minutes, 5);
        expect_eq(iv.seconds, 6);
        expect_eq(iv.milliseconds, 789);

        let buf: [128]u8 = undefined;
        const n_pg = toPostgresString(iv, buf[0..128]);
        expect_eq(std.mem.eql(u8, buf[0..n_pg], "1 year 2 mons 3 days 04:05:06.789"), true);

        const n_iso = toIsoDuration(iv, buf[0..128]);
        expect_eq(std.mem.eql(u8, buf[0..n_iso], "P1Y2M3DT4H5M6.789S"), true);
    });

    test("2. @cap/postgres-interval — Negative days, negative time & verbose ago", () => {
        const iv = parsePgInterval("-2 days -01:30:00");
        expect_eq(iv.valid, true);
        expect_eq(iv.days, -2);
        expect_eq(iv.hours, -1);
        expect_eq(iv.minutes, -30);
        expect_eq(iv.seconds, 0);

        let buf: [128]u8 = undefined;
        const n_pg = toPostgresString(iv, buf[0..128]);
        expect_eq(std.mem.eql(u8, buf[0..n_pg], "-2 days -01:30:00"), true);

        const n_iso = toIsoDuration(iv, buf[0..128]);
        expect_eq(std.mem.eql(u8, buf[0..n_iso], "P-2DT-1H-30M"), true);

        const ago = parsePgInterval("@ 3 years 6 months ago");
        expect_eq(ago.valid, true);
        expect_eq(ago.years, -3);
        expect_eq(ago.months, -6);
    });

    test("3. @cap/postgres-interval — ISO-8601 duration parsing & round-trip", () => {
        const iv = parsePgInterval("P1Y2M3DT4H5M6S");
        expect_eq(iv.valid, true);
        expect_eq(iv.years, 1);
        expect_eq(iv.months, 2);
        expect_eq(iv.days, 3);
        expect_eq(iv.hours, 4);
        expect_eq(iv.minutes, 5);
        expect_eq(iv.seconds, 6);
        expect_eq(iv.milliseconds, 0);

        let buf: [128]u8 = undefined;
        const n_iso = toIsoDuration(iv, buf[0..128]);
        expect_eq(std.mem.eql(u8, buf[0..n_iso], "P1Y2M3DT4H5M6S"), true);

        const w = parsePgInterval("P2W");
        expect_eq(w.valid, true);
        expect_eq(w.days, 14);
    });

    test("4. @cap/postgres-interval — Time-only & zero intervals", () => {
        const t = parsePgInterval("00:00:42");
        expect_eq(t.valid, true);
        expect_eq(t.hours, 0);
        expect_eq(t.minutes, 0);
        expect_eq(t.seconds, 42);
        expect_eq(totalSecondsApprox(t), 42 as i64);

        const z = parsePgInterval("00:00:00");
        expect_eq(z.valid, true);
        let buf: [64]u8 = undefined;
        const n_iso = toIsoDuration(z, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n_iso], "PT0S"), true);
        const n_pg = toPostgresString(z, buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n_pg], "00:00:00"), true);
    });

    test("5. @cap/postgres-interval — totalSecondsApprox & plurals", () => {
        const iv = parsePgInterval("3 years 6 months");
        expect_eq(iv.valid, true);
        expect_eq(iv.years, 3);
        expect_eq(iv.months, 6);
        const expected: i64 = 3 * 31536000 + 6 * 2592000;
        expect_eq(totalSecondsApprox(iv), expected);

        const iv2 = parsePgInterval("1 day 2 hours 3 mins 4 secs");
        expect_eq(iv2.valid, true);
        expect_eq(totalSecondsApprox(iv2), (86400 + 7200 + 180 + 4) as i64);
    });

    test("6. @cap/postgres-interval — Invalid inputs & tiny buffer guard", () => {
        expect_eq(parsePgInterval("").valid, false);
        expect_eq(parsePgInterval("   ").valid, false);
        expect_eq(parsePgInterval("not an interval").valid, false);
        expect_eq(parsePgInterval("P").valid, false);
        expect_eq(parsePgInterval("10 lightyears").valid, false);

        const iv = parsePgInterval("1 year 2 mons");
        let tiny: [4]u8 = undefined;
        expect_eq(toIsoDuration(iv, tiny[0..4]), 0 as usize);
        expect_eq(toPostgresString(iv, tiny[0..4]), 0 as usize);
    });
}
