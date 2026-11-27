import { test, expect_eq } from "cap:test";
import {
    parseDuration, formatDuration, formatRfc3339Seconds,
    NS_PER_SEC, NS_PER_MS, NS_PER_MIN, NS_PER_HOUR, NS_PER_DAY
} from "../../packages/humantime/src/index.cp";

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
    test("1. @cap/humantime — Parses multi-unit durations accurately", () => {
        let r1 = parseDuration("2h 37min 5s");
        expect_eq(r1.ok, true);
        expect_eq(r1.nanos, 2 * NS_PER_HOUR + 37 * NS_PER_MIN + 5 * NS_PER_SEC);

        let r2 = parseDuration("1day 500ms 20us 10ns");
        expect_eq(r2.ok, true);
        expect_eq(r2.nanos, NS_PER_DAY + 500 * NS_PER_MS + 20000 + 10);
    });

    test("2. @cap/humantime — Supports singular/plural and short/long unit aliases", () => {
        expect_eq(parseDuration("1 second").nanos, NS_PER_SEC);
        expect_eq(parseDuration("2 minutes").nanos, 2 * NS_PER_MIN);
        expect_eq(parseDuration("3 hrs").nanos, 3 * NS_PER_HOUR);
        expect_eq(parseDuration("1 week").nanos, 7 * NS_PER_DAY);
    });

    test("3. @cap/humantime — formatDuration formats zero and compound durations", () => {
        let buf: [64]u8 = undefined;
        let n0 = formatDuration(0, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n0, "0s"), true);

        let d: u64 = (NS_PER_HOUR * 2) + (NS_PER_MIN * 37) + (NS_PER_SEC * 5) + (NS_PER_MS * 120);
        let n1 = formatDuration(d, buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1, "2h 37m 5s 120ms"), true);
    });

    test("4. @cap/humantime — Round-trip formatDuration -> parseDuration", () => {
        let orig: u64 = (NS_PER_DAY * 3) + (NS_PER_HOUR * 4) + (NS_PER_MIN * 15) + (NS_PER_SEC * 9) + (NS_PER_MS * 42);
        let buf: [64]u8 = undefined;
        let n = formatDuration(orig, buf[0..64]);
        let parsed = parseDuration(buf[0..n]);
        expect_eq(parsed.ok, true);
        expect_eq(parsed.nanos, orig);
    });

    test("5. @cap/humantime — formatRfc3339Seconds epoch, leap day, and 2026 date", () => {
        let buf: [32]u8 = undefined;
        let n1 = formatRfc3339Seconds(0, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n1, "1970-01-01T00:00:00Z"), true);

        // 2024-02-29T12:34:56Z = 1709210096
        let n2 = formatRfc3339Seconds(1709210096, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n2, "2024-02-29T12:34:56Z"), true);

        // 2000-01-01T00:00:00Z = 946684800
        let n3 = formatRfc3339Seconds(946684800, buf[0..32]);
        expect_eq(eqBytes(buf[0..32], n3, "2000-01-01T00:00:00Z"), true);
    });

    test("6. @cap/humantime — Rejects invalid duration syntax cleanly", () => {
        expect_eq(parseDuration("").ok, false);
        expect_eq(parseDuration("   ").ok, false);
        expect_eq(parseDuration("42").ok, false);
        expect_eq(parseDuration("10lightyears").ok, false);
        expect_eq(parseDuration("-5s").ok, false);
    });
}
