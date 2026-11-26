import { test, expect_eq } from "cap:test";
import { parseMs, formatMs, MS_DAY, MS_HOUR, MS_MINUTE, MS_SECOND } from "../../packages/@cap/ms/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ms — Parses standard units and bare numbers", () => {
        expect_eq(parseMs("100").ms, 100);
        expect_eq(parseMs("2 days").ms, 2 * MS_DAY);
        expect_eq(parseMs("1d").ms, MS_DAY);
        expect_eq(parseMs("10h").ms, 10 * MS_HOUR);
        expect_eq(parseMs("2.5 hrs").ms, 9000000);
        expect_eq(parseMs("1m").ms, MS_MINUTE);
        expect_eq(parseMs("5s").ms, 5 * MS_SECOND);
    });

    test("2. @cap/ms — Parses negative and decimal values", () => {
        expect_eq(parseMs("-3 days").ms, -3 * MS_DAY);
        expect_eq(parseMs("-1h").ms, -MS_HOUR);
        expect_eq(parseMs(".5ms").ok, true);
        expect_eq(parseMs("1.5s").ms, 1500);
    });

    test("3. @cap/ms — Case-insensitive long unit names", () => {
        expect_eq(parseMs("1 SECOND").ms, MS_SECOND);
        expect_eq(parseMs("2 Minutes").ms, 2 * MS_MINUTE);
        expect_eq(parseMs("1 Week").ms, 7 * MS_DAY);
    });

    test("4. @cap/ms — Short formatting mode", () => {
        let buf: [32]u8 = undefined;
        expect_eq(eqBytes(buf[0..32], formatMs(60000, false, buf[0..32]), "1m"), true);
        expect_eq(eqBytes(buf[0..32], formatMs(2 * MS_MINUTE, false, buf[0..32]), "2m"), true);
        expect_eq(eqBytes(buf[0..32], formatMs(-3 * MS_MINUTE, false, buf[0..32]), "-3m"), true);
        expect_eq(eqBytes(buf[0..32], formatMs(500, false, buf[0..32]), "500ms"), true);
    });

    test("5. @cap/ms — Long formatting mode with singular/plural", () => {
        let buf: [32]u8 = undefined;
        expect_eq(eqBytes(buf[0..32], formatMs(MS_MINUTE, true, buf[0..32]), "1 minute"), true);
        expect_eq(eqBytes(buf[0..32], formatMs(2 * MS_MINUTE, true, buf[0..32]), "2 minutes"), true);
        expect_eq(eqBytes(buf[0..32], formatMs(MS_DAY, true, buf[0..32]), "1 day"), true);
        expect_eq(eqBytes(buf[0..32], formatMs(-2 * MS_HOUR, true, buf[0..32]), "-2 hours"), true);
    });

    test("6. @cap/ms — Rejects invalid inputs cleanly", () => {
        expect_eq(parseMs("").ok, false);
        expect_eq(parseMs("   ").ok, false);
        expect_eq(parseMs("foo").ok, false);
        expect_eq(parseMs("10 lightyears").ok, false);
    });
}
