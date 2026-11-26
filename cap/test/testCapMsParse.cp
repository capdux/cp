// cap/test/testCapMsParse.cp
// Comprehensive unit tests for @cap/ms-parse

import { test, expect_eq } from "cap:test";
import {
    MS_SECOND,
    MS_MINUTE,
    MS_HOUR,
    MS_DAY,
    MS_WEEK,
    MS_YEAR,
    parseMs,
    formatShort,
    formatLong,
} from "../../packages/@cap/ms-parse/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ms-parse — Parse all units (ms/s/m/h/d/w/y) & long names", () => {
        expect_eq(parseMs("500").ms, 500 as i64);
        expect_eq(parseMs("500ms").ms, 500 as i64);
        expect_eq(parseMs("2 seconds").ms, (2 as i64) * MS_SECOND);
        expect_eq(parseMs("5m").ms, (5 as i64) * MS_MINUTE);
        expect_eq(parseMs("10h").ms, (10 as i64) * MS_HOUR);
        expect_eq(parseMs("2 DAYS").ms, (2 as i64) * MS_DAY);
        expect_eq(parseMs("1w").ms, MS_WEEK);
        expect_eq(parseMs("1 year").ms, MS_YEAR);
    });

    test("2. @cap/ms-parse — Fractional & negative durations ('1.5h', '-3s', '.5s')", () => {
        const r1 = parseMs("1.5h");
        expect_eq(r1.valid, true);
        expect_eq(r1.ms, 5400000 as i64);

        const r2 = parseMs("-3s");
        expect_eq(r2.valid, true);
        expect_eq(r2.ms, -3000 as i64);

        const r3 = parseMs(".5s");
        expect_eq(r3.valid, true);
        expect_eq(r3.ms, 500 as i64);
    });

    test("3. @cap/ms-parse — formatShort positive & negative durations", () => {
        let buf: [32]u8 = undefined;
        const n1 = formatShort(60000 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1], "1m"), true);

        const n2 = formatShort(3600000 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n2], "1h"), true);

        const n3 = formatShort(-5000 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n3], "-5s"), true);

        const n4 = formatShort(250 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n4], "250ms"), true);

        const n5 = formatShort((3 as i64) * MS_DAY, buf[0..32]);
        expect_eq(sliceEq(buf[0..n5], "3d"), true);
    });

    test("4. @cap/ms-parse — formatLong singular vs plural grammar", () => {
        let buf: [32]u8 = undefined;
        const n1 = formatLong(1000 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n1], "1 second"), true);

        const n2 = formatLong(2000 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n2], "2 seconds"), true);

        const n3 = formatLong(60000 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n3], "1 minute"), true);

        const n4 = formatLong(120000 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n4], "2 minutes"), true);

        const n5 = formatLong(-3600000 as i64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n5], "-1 hour"), true);
    });

    test("5. @cap/ms-parse — Reject invalid units, empty strings & overly long input", () => {
        expect_eq(parseMs("").valid, false);
        expect_eq(parseMs("   ").valid, false);
        expect_eq(parseMs("10lightyears").valid, false);
        expect_eq(parseMs("abc").valid, false);
        expect_eq(parseMs("1.s").valid, false);

        let tooLong: [105]u8 = undefined;
        let i: usize = 0;
        while (i < 105) {
            tooLong[i] = '1' as u8;
            i += 1;
        }
        expect_eq(parseMs(tooLong[0..105]).valid, false);
    });

    test("6. @cap/ms-parse — Small output buffer returns 0 safely", () => {
        let tiny: [2]u8 = undefined;
        expect_eq(formatShort(250 as i64, tiny[0..2]), 0 as usize);
        expect_eq(formatLong(60000 as i64, tiny[0..2]), 0 as usize);
    });
}
