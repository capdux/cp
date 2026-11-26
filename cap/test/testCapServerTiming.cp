import { test, expect_eq } from "cap:test";
import {
    TimingMetric,
    parseHeader,
    findMetric,
    formatMetric,
    formatHeader,
    totalDurationMs,
} from "../../packages/@cap/server-timing/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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

fn approxEq(a: f64, b: f64, eps: f64) -> bool {
    let diff = a - b;
    if (diff < 0.0) {
        diff = -diff;
    }
    return diff <= eps;
}

fn main() {
    test("1. @cap/server-timing — Bare & single metric parsing", () => {
        let out: [4]TimingMetric = undefined;
        const n = parseHeader("miss, db;dur=53.2", out[0..4]);
        expect_eq(n, 2 as usize);

        expect_eq(testEq(out[0].name, "miss"), true);
        expect_eq(out[0].has_dur, false);
        expect_eq(testEq(out[0].desc, ""), true);

        expect_eq(testEq(out[1].name, "db"), true);
        expect_eq(out[1].has_dur, true);
        expect_eq(approxEq(out[1].dur_ms, 53.2, 0.0001), true);
    });

    test("2. @cap/server-timing — Multi-metric with quoted commas & semicolons in desc", () => {
        const hdr = "cache;desc=\"Redis, L1; hit\", db;dur=12.75;desc=\"PostgreSQL Query\", app;dur=4";
        let out: [4]TimingMetric = undefined;
        const n = parseHeader(hdr, out[0..4]);
        expect_eq(n, 3 as usize);

        expect_eq(testEq(out[0].name, "cache"), true);
        expect_eq(testEq(out[0].desc, "Redis, L1; hit"), true);

        expect_eq(testEq(out[1].name, "db"), true);
        expect_eq(approxEq(out[1].dur_ms, 12.75, 0.0001), true);
        expect_eq(testEq(out[1].desc, "PostgreSQL Query"), true);

        expect_eq(testEq(out[2].name, "app"), true);
        expect_eq(approxEq(out[2].dur_ms, 4.0, 0.0001), true);
    });

    test("3. @cap/server-timing — findMetric exact lookup", () => {
        const hdr = "miss, db;dur=53.2;desc=\"PostgreSQL\", app;dur=12.4";
        let m = TimingMetric { name: "", has_dur: false, dur_ms: 0.0, desc: "" };

        expect_eq(findMetric(hdr, "db", &m), true);
        expect_eq(approxEq(m.dur_ms, 53.2, 0.0001), true);
        expect_eq(testEq(m.desc, "PostgreSQL"), true);

        expect_eq(findMetric(hdr, "app", &m), true);
        expect_eq(approxEq(m.dur_ms, 12.4, 0.0001), true);

        expect_eq(findMetric(hdr, "nonexistent", &m), false);
    });

    test("4. @cap/server-timing — formatMetric & formatHeader roundtrip", () => {
        const metrics: [3]TimingMetric = [
            TimingMetric { name: "miss", has_dur: false, dur_ms: 0.0, desc: "" },
            TimingMetric { name: "db", has_dur: true, dur_ms: 53.2, desc: "SQL \"main\"" },
            TimingMetric { name: "app", has_dur: true, dur_ms: 12.0, desc: "worker" },
        ];
        let buf: [256]u8 = undefined;
        const n = formatHeader(metrics[0..3], buf[0..256]);
        const expected = "miss, db;dur=53.2;desc=\"SQL \\\"main\\\"\", app;dur=12;desc=worker";
        expect_eq(n, expected.len);
        expect_eq(testEq(buf[0..n], expected), true);

        // Roundtrip back through parseHeader
        let reparsed: [4]TimingMetric = undefined;
        const cnt = parseHeader(buf[0..n], reparsed[0..4]);
        expect_eq(cnt, 3 as usize);
        expect_eq(approxEq(reparsed[1].dur_ms, 53.2, 0.0001), true);
        expect_eq(testEq(reparsed[2].desc, "worker"), true);
    });

    test("5. @cap/server-timing — totalDurationMs aggregation", () => {
        const hdr = "miss, db;dur=53.2, cache;dur=0.5, app;dur=12.3";
        const total = totalDurationMs(hdr);
        expect_eq(approxEq(total, 66.0, 0.0001), true);
        expect_eq(approxEq(totalDurationMs("miss, hit"), 0.0, 0.0001), true);
    });

    test("6. @cap/server-timing — Malformed input & undersized buffer protection", () => {
        let out: [4]TimingMetric = undefined;
        expect_eq(parseHeader("", out[0..4]), 0 as usize);
        expect_eq(parseHeader(" , , ;dur=10 ", out[0..4]), 0 as usize);

        // Invalid dur ignored while metric name still parsed
        const n = parseHeader("db;dur=notanumber", out[0..4]);
        expect_eq(n, 1 as usize);
        expect_eq(out[0].has_dur, false);

        let tiny: [5]u8 = undefined;
        const m = TimingMetric { name: "database", has_dur: true, dur_ms: 10.5, desc: "Primary DB" };
        expect_eq(formatMetric(m, tiny[0..5]), 0 as usize);
        const arr: [1]TimingMetric = [m];
        expect_eq(formatHeader(arr[0..1], tiny[0..5]), 0 as usize);
    });
}
