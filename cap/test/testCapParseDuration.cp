import { test, expect_eq } from "cap:test";
import {
    ParsedDuration,
    parseDuration,
    formatDuration
} from "../../packages/parse_duration/src/index.cp";

fn strEqBuf(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/parse-duration — all units & case-insensitivity (including µs)", () => {
        let d_ns = parseDuration("500ns") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_ns.totalNanos() ?? 0, 500);

        let d_us = parseDuration("250µs") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_us.totalNanos() ?? 0, 250000);

        let d_ms = parseDuration("1500MS") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_ms.secs, 1);
        expect_eq(d_ms.nanos, 500000000);
        expect_eq(d_ms.totalMillis(), 1500);

        let d_s = parseDuration("42Seconds") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_s.totalSecs(), 42);

        let d_m = parseDuration("3min") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_m.totalSecs(), 180);

        let d_h = parseDuration("2hrs") == null;
        expect_eq(d_h, true);
        let d_hr = parseDuration("2hr") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_hr.totalSecs(), 7200);

        let d_d = parseDuration("1day") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_d.totalSecs(), 86400);

        let d_w = parseDuration("2wk") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_w.totalSecs(), 1209600);

        let d_y = parseDuration("1year") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d_y.totalSecs(), 31536000);
    });

    test("2. @cap/parse-duration — compound expressions & separators", () => {
        let d1 = parseDuration("2h 30m 15s") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d1.totalSecs(), 9015);
        expect_eq(d1.nanos, 0);

        let d2 = parseDuration("1d12h") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d2.totalSecs(), 129600);

        let d3 = parseDuration("5m, 10s") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d3.totalSecs(), 310);

        let d4 = parseDuration("1h + 30m") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d4.totalSecs(), 5400);
    });

    test("3. @cap/parse-duration — fractional units with exact nanosecond precision", () => {
        let d1 = parseDuration("1.5h") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d1.totalSecs(), 5400);
        expect_eq(d1.nanos, 0);

        let d2 = parseDuration("0.25s") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d2.totalSecs(), 0);
        expect_eq(d2.totalMillis(), 250);
        expect_eq(d2.nanos, 250000000);

        let d3 = parseDuration("2.5ms") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(d3.totalNanos() ?? 0, 2500000);

        let d4 = parseDuration("0s") ?? ParsedDuration { secs: 1, nanos: 1 };
        expect_eq(d4.isZero(), true);
    });

    test("4. @cap/parse-duration — strict rejection of invalid, negative & overflowing inputs", () => {
        expect_eq(parseDuration("") == null, true);
        expect_eq(parseDuration("   ") == null, true);
        expect_eq(parseDuration("123") == null, true);
        expect_eq(parseDuration("-5s") == null, true);
        expect_eq(parseDuration("10lightyears") == null, true);
        expect_eq(parseDuration("1h +") == null, true);
        expect_eq(parseDuration("5m,") == null, true);
        expect_eq(parseDuration("1.s") == null, true);
        expect_eq(parseDuration("99999999999999999999999s") == null, true);
        expect_eq(parseDuration("1000000000000y") == null, true);
    });

    test("5. @cap/parse-duration — totalNanos overflow safety", () => {
        let big = parseDuration("1000y") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(big.totalSecs(), 31536000000);
        expect_eq(big.totalNanos() == null, true);

        let small = parseDuration("1s 500ns") ?? ParsedDuration { secs: 0, nanos: 0 };
        expect_eq(small.totalNanos() ?? 0, 1000000500);
    });

    test("6. @cap/parse-duration — formatDuration human-readable output", () => {
        let buf: [64]u8 = [
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        ];

        let d1 = ParsedDuration { secs: 5405, nanos: 0 };
        let n1: usize = formatDuration(d1, buf[0..64]);
        expect_eq(strEqBuf(buf[0..64], n1, "1h 30m 5s"), true);

        let d2 = ParsedDuration { secs: 0, nanos: 250000000 };
        let n2: usize = formatDuration(d2, buf[0..64]);
        expect_eq(strEqBuf(buf[0..64], n2, "250ms"), true);

        let d3 = ParsedDuration { secs: 0, nanos: 0 };
        let n3: usize = formatDuration(d3, buf[0..64]);
        expect_eq(strEqBuf(buf[0..64], n3, "0s"), true);
    });
}
