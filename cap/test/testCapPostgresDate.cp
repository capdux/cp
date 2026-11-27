import { test, expect_eq } from "cap:test";
import {
    PgDateKind,
    parsePgDate,
    toUnixMs,
} from "../../packages/postgres-date/src/index.cp";

fn main() {
    test("1. @cap/postgres-date — parses UTC epoch and converts to Unix ms = 0", () => {
        const dt = parsePgDate("1970-01-01 00:00:00+00");
        expect_eq(dt.isValid(), true);
        expect_eq(dt.kind == .Finite, true);
        expect_eq(toUnixMs(dt) ?? -1, 0 as i64);
    });

    test("2. @cap/postgres-date — parses microseconds and timezone offset +08:00", () => {
        const dt = parsePgDate("2026-11-27 08:30:45.123456+08:00");
        expect_eq(dt.isValid(), true);
        expect_eq(dt.year, 2026 as i32);
        expect_eq(dt.month, 11 as u8);
        expect_eq(dt.day, 27 as u8);
        expect_eq(dt.microsecond, 123456 as u32);
        expect_eq(dt.tz_offset_sec, 28800 as i32);
    });

    test("3. @cap/postgres-date — handles DATE-only and ISO 'T'...'Z' formats", () => {
        const d1 = parsePgDate("2024-02-29"); // leap year
        expect_eq(d1.isValid(), true);
        expect_eq(d1.day, 29 as u8);

        const d2 = parsePgDate("2024-02-29T12:00:00.5Z");
        expect_eq(d2.isValid(), true);
        expect_eq(d2.microsecond, 500000 as u32);
    });

    test("4. @cap/postgres-date — handles infinity, -infinity, and BC dates", () => {
        expect_eq(parsePgDate("infinity").kind == .PosInfinity, true);
        expect_eq(parsePgDate("-infinity").kind == .NegInfinity, true);
        expect_eq(toUnixMs(parsePgDate("infinity")) == null, true);

        const bc = parsePgDate("0044-03-15 00:00:00 BC");
        expect_eq(bc.isValid(), true);
        expect_eq(bc.is_bc, true);
        expect_eq(bc.year, 44 as i32);
    });

    test("5. @cap/postgres-date — timezone offset adjusts UTC Unix ms accurately", () => {
        const utc = parsePgDate("2025-01-01 00:00:00Z");
        const plus2 = parsePgDate("2025-01-01 02:00:00+02:00");
        const minus5 = parsePgDate("2024-12-31 19:00:00-05:00");
        expect_eq(toUnixMs(utc) ?? -1, toUnixMs(plus2) ?? -2);
        expect_eq(toUnixMs(utc) ?? -1, toUnixMs(minus5) ?? -2);
    });

    test("6. @cap/postgres-date — rejects invalid leap day and malformed inputs", () => {
        expect_eq(parsePgDate("2025-02-29").isValid(), false);
        expect_eq(parsePgDate("2025-13-01").isValid(), false);
        expect_eq(parsePgDate("not-a-date").isValid(), false);
        expect_eq(parsePgDate("").isValid(), false);
    });
}
