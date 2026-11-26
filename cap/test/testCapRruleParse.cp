// cap/test/testCapRruleParse.cp
// Comprehensive unit tests for @cap/rrule-parse (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    FREQ_YEARLY,
    FREQ_MONTHLY,
    FREQ_WEEKLY,
    FREQ_DAILY,
    FREQ_HOURLY,
    DAY_SU,
    DAY_MO,
    DAY_WE,
    DAY_FR,
    parseRRule,
    formatRRule,
    matchesWeekday,
    isValidRRule,
} from "../../packages/@cap/rrule-parse/src/index.cp";

fn main() {
    test("1. @cap/rrule-parse — parse full RFC 5545 RRULE with RRULE: prefix & case-insensitivity", () => {
        const r = parseRRule("rrule:freq=weekly;interval=2;count=10;byday=mo,we,fr;bymonth=1,6,12;wkst=su");
        expect_eq(r.valid, true);
        expect_eq(r.freq, FREQ_WEEKLY);
        expect_eq(r.interval, 2 as u32);
        expect_eq(r.has_count, true);
        expect_eq(r.count, 10 as u32);
        expect_eq(r.byday_mask, DAY_MO | DAY_WE | DAY_FR);
        // Months 1 (bit 0), 6 (bit 5), 12 (bit 11) => 1 + 32 + 2048 = 2081
        expect_eq(r.bymonth_mask, 2081 as u16);
        expect_eq(r.wkst, DAY_SU);
    });

    test("2. @cap/rrule-parse — UNTIL date & UTC datetime parsing across frequencies", () => {
        const r1 = parseRRule("FREQ=DAILY;UNTIL=20261231T235959Z");
        expect_eq(r1.valid, true);
        expect_eq(r1.freq, FREQ_DAILY);
        expect_eq(r1.interval, 1 as u32);
        expect_eq(r1.has_count, false);
        expect_eq(r1.until, "20261231T235959Z");

        const r2 = parseRRule("FREQ=YEARLY;UNTIL=20300101");
        expect_eq(r2.valid, true);
        expect_eq(r2.freq, FREQ_YEARLY);
        expect_eq(r2.until, "20300101");

        const r3 = parseRRule("FREQ=MONTHLY;BYDAY=-1FR");
        expect_eq(r3.valid, true);
        expect_eq(r3.freq, FREQ_MONTHLY);
        expect_eq(r3.byday_mask, DAY_FR);
    });

    test("3. @cap/rrule-parse — formatRRule canonical serialization & round-trip", () => {
        const r = parseRRule("RRULE:freq=weekly;interval=2;byday=fr,mo,we;bymonth=12,1,6;count=8;wkst=su");
        let buf: [256]u8 = undefined;
        const n = formatRRule(r, buf[0..256]);
        expect_eq(
            buf[0..n],
            "FREQ=WEEKLY;INTERVAL=2;COUNT=8;BYDAY=MO,WE,FR;BYMONTH=1,6,12;WKST=SU"
        );

        // Default interval=1 and wkst=MO omitted in canonical output
        const r2 = parseRRule("FREQ=HOURLY;INTERVAL=1;WKST=MO");
        const n2 = formatRRule(r2, buf[0..256]);
        expect_eq(buf[0..n2], "FREQ=HOURLY");
        expect_eq(r2.freq, FREQ_HOURLY);
    });

    test("4. @cap/rrule-parse — matchesWeekday (0=Sun..6=Sat) with and without BYDAY", () => {
        const r = parseRRule("FREQ=WEEKLY;BYDAY=MO,WE,FR");
        expect_eq(matchesWeekday(r, 0), false); // Sun
        expect_eq(matchesWeekday(r, 1), true);  // Mon
        expect_eq(matchesWeekday(r, 2), false); // Tue
        expect_eq(matchesWeekday(r, 3), true);  // Wed
        expect_eq(matchesWeekday(r, 4), false); // Thu
        expect_eq(matchesWeekday(r, 5), true);  // Fri
        expect_eq(matchesWeekday(r, 6), false); // Sat
        expect_eq(matchesWeekday(r, 7), false); // out of range

        // No BYDAY restriction -> all weekdays match
        const daily = parseRRule("FREQ=DAILY");
        expect_eq(matchesWeekday(daily, 0), true);
        expect_eq(matchesWeekday(daily, 6), true);
    });

    test("5. @cap/rrule-parse — strict RFC 5545 rejection (missing FREQ, COUNT+UNTIL conflict, invalid values)", () => {
        expect_eq(isValidRRule(""), false);
        expect_eq(isValidRRule("RRULE:"), false);
        expect_eq(isValidRRule("INTERVAL=2;COUNT=5"), false); // missing FREQ
        expect_eq(isValidRRule("FREQ=DAILY;COUNT=5;UNTIL=20261231"), false); // COUNT + UNTIL conflict
        expect_eq(isValidRRule("FREQ=DECADE"), false); // invalid FREQ
        expect_eq(isValidRRule("FREQ=DAILY;INTERVAL=0"), false); // interval must be >= 1
        expect_eq(isValidRRule("FREQ=DAILY;COUNT=0"), false); // count must be >= 1
        expect_eq(isValidRRule("FREQ=WEEKLY;BYDAY=XX"), false); // invalid weekday
        expect_eq(isValidRRule("FREQ=YEARLY;BYMONTH=13"), false); // invalid month
        expect_eq(isValidRRule("FREQ=DAILY;UNTIL=20261301"), false); // invalid month in UNTIL
    });

    test("6. @cap/rrule-parse — invalid rule formatting returns 0 & tight buffer safety", () => {
        const bad = parseRRule("INVALID=1");
        expect_eq(bad.valid, false);
        expect_eq(matchesWeekday(bad, 1), false);

        let buf: [64]u8 = undefined;
        expect_eq(formatRRule(bad, buf[0..64]), 0 as usize);

        const good = parseRRule("FREQ=WEEKLY;INTERVAL=3;BYDAY=MO,WE,FR");
        let tiny: [10]u8 = undefined;
        const n = formatRRule(good, tiny[0..10]);
        expect_eq(n, 10 as usize);
        expect_eq(tiny[0..n], "FREQ=WEEKL");
    });
}
