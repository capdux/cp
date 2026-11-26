import { test, expect_eq } from "cap:test";
import {
    ParsedMs,
    parseMs,
    parseMsF64,
    toMilliseconds,
} from "../../packages/@cap/parse-ms/src/index.cp";

fn main() {
    test("1. @cap/parse-ms — Classic vector parseMs(1337000001)", () => {
        const p: ParsedMs = parseMs(1337000001);
        expect_eq(p.days, 15);
        expect_eq(p.hours, 11);
        expect_eq(p.minutes, 23);
        expect_eq(p.seconds, 20);
        expect_eq(p.milliseconds, 1);
        expect_eq(p.microseconds, 0);
        expect_eq(p.nanoseconds, 0);
    });

    test("2. @cap/parse-ms — Zero & single millisecond boundaries", () => {
        const z: ParsedMs = parseMs(0);
        expect_eq(z.days, 0);
        expect_eq(z.hours, 0);
        expect_eq(z.minutes, 0);
        expect_eq(z.seconds, 0);
        expect_eq(z.milliseconds, 0);

        const one: ParsedMs = parseMs(1);
        expect_eq(one.days, 0);
        expect_eq(one.seconds, 0);
        expect_eq(one.milliseconds, 1);
    });

    test("3. @cap/parse-ms — Negative durations truncate toward zero (-1500 & -90061001)", () => {
        const n1: ParsedMs = parseMs(-1500);
        expect_eq(n1.seconds, -1);
        expect_eq(n1.milliseconds, -500);
        expect_eq(n1.days, 0);

        // -(1d + 1h + 1m + 1s + 1ms) = -90061001
        const n2: ParsedMs = parseMs(-90061001);
        expect_eq(n2.days, -1);
        expect_eq(n2.hours, -1);
        expect_eq(n2.minutes, -1);
        expect_eq(n2.seconds, -1);
        expect_eq(n2.milliseconds, -1);
    });

    test("4. @cap/parse-ms — Sub-millisecond parseMsF64(1.234567) & negative float", () => {
        const p: ParsedMs = parseMsF64(1.234567);
        expect_eq(p.milliseconds, 1);
        expect_eq(p.microseconds, 234);
        expect_eq(p.nanoseconds, 567);

        const pf: ParsedMs = parseMsF64(1337000001.234);
        expect_eq(pf.days, 15);
        expect_eq(pf.hours, 11);
        expect_eq(pf.minutes, 23);
        expect_eq(pf.seconds, 20);
        expect_eq(pf.milliseconds, 1);
        expect_eq(pf.microseconds, 234);
        expect_eq(pf.nanoseconds, 0);

        const neg_f: ParsedMs = parseMsF64(-2.5);
        expect_eq(neg_f.milliseconds, -2);
        expect_eq(neg_f.microseconds, -500);
        expect_eq(neg_f.nanoseconds, 0);
    });

    test("5. @cap/parse-ms — toMilliseconds exact lossless round-trip", () => {
        const samples: [5]i64 = [0, 999, 1337000001, -1500, -90061001];
        let i: usize = 0;
        while (i < 5) {
            const p: ParsedMs = parseMs(samples[i]);
            expect_eq(toMilliseconds(p), samples[i]);
            i += 1;
        }
    });

    test("6. @cap/parse-ms — Multi-year large day spans without overflow", () => {
        // 100 years * 365 days = 36500 days = 3,153,600,000,000 ms
        const hundred_years_ms: i64 = 36500 * 86400000 + 5 * 3600000 + 42 * 1000 + 7;
        const p: ParsedMs = parseMs(hundred_years_ms);
        expect_eq(p.days, 36500);
        expect_eq(p.hours, 5);
        expect_eq(p.minutes, 0);
        expect_eq(p.seconds, 42);
        expect_eq(p.milliseconds, 7);
        expect_eq(toMilliseconds(p), hundred_years_ms);
    });
}
