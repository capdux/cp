// cap/test/testCapStatsd.cp
// Comprehensive unit tests for @cap/statsd (StatsD & DogStatsD parity)

import { test, expect_eq } from "cap:test";
import {
    STATSD_COUNTER,
    STATSD_TIMING,
    STATSD_GAUGE,
    STATSD_SET,
    StatsdMetric,
    formatMetric,
    parseMetric,
} from "../../packages/hot-shots/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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
    test("1. @cap/statsd — Counter, Timing, Gauge & Set formatting", () => {
        let buf: [128]u8 = undefined;

        const n1: i64 = formatMetric("req.cnt", 5, STATSD_COUNTER, 1000, "", buf[0..128]);
        expect_eq(sliceEq(buf[0..(n1 as usize)], "req.cnt:5|c"), true);

        const n2: i64 = formatMetric("req.lat", 42, STATSD_TIMING, 1000, "", buf[0..128]);
        expect_eq(sliceEq(buf[0..(n2 as usize)], "req.lat:42|ms"), true);

        const n3: i64 = formatMetric("mem.used", 1024, STATSD_GAUGE, 1000, "", buf[0..128]);
        expect_eq(sliceEq(buf[0..(n3 as usize)], "mem.used:1024|g"), true);

        const n4: i64 = formatMetric("users.uniq", 999, STATSD_SET, 1000, "", buf[0..128]);
        expect_eq(sliceEq(buf[0..(n4 as usize)], "users.uniq:999|s"), true);
    });

    test("2. @cap/statsd — sample rate |@0.25 & DogStatsD tags round-trip", () => {
        let buf: [128]u8 = undefined;
        const n: i64 = formatMetric("http.req", 1, STATSD_COUNTER, 250, "env:prod,region:us", buf[0..128]);
        expect_eq(n > 0, true);
        const pkt: []const u8 = buf[0..(n as usize)];
        expect_eq(sliceEq(pkt, "http.req:1|c|@0.25|#env:prod,region:us"), true);

        const m: StatsdMetric = parseMetric(pkt);
        expect_eq(m.valid, true);
        expect_eq(sliceEq(pkt[m.name_start..(m.name_start + m.name_len)], "http.req"), true);
        expect_eq(m.value, 1 as i64);
        expect_eq(m.kind, STATSD_COUNTER);
        expect_eq(m.sample_rate_milli, 250 as u16);
        expect_eq(sliceEq(pkt[m.tags_start..(m.tags_start + m.tags_len)], "env:prod,region:us"), true);
    });

    test("3. @cap/statsd — negative Gauge delta round-trip", () => {
        let buf: [64]u8 = undefined;
        const n: i64 = formatMetric("queue.depth", -15, STATSD_GAUGE, 1000, "", buf[0..64]);
        expect_eq(sliceEq(buf[0..(n as usize)], "queue.depth:-15|g"), true);

        const m: StatsdMetric = parseMetric(buf[0..(n as usize)]);
        expect_eq(m.valid, true);
        expect_eq(m.kind, STATSD_GAUGE);
        expect_eq(m.value, -15 as i64);
        expect_eq(m.sample_rate_milli, 1000 as u16);
    });

    test("4. @cap/statsd — full sample rate 1000 omits |@ segment", () => {
        let buf: [64]u8 = undefined;
        const n: i64 = formatMetric("db.qps", 10, STATSD_COUNTER, 1000, "svc:auth", buf[0..64]);
        expect_eq(sliceEq(buf[0..(n as usize)], "db.qps:10|c|#svc:auth"), true);

        const m: StatsdMetric = parseMetric(buf[0..(n as usize)]);
        expect_eq(m.valid, true);
        expect_eq(m.sample_rate_milli, 1000 as u16);
        expect_eq(sliceEq(buf[m.tags_start..(m.tags_start + m.tags_len)], "svc:auth"), true);
    });

    test("5. @cap/statsd — rejects invalid metric names & malformed packets", () => {
        let buf: [64]u8 = undefined;
        expect_eq(formatMetric("", 1, STATSD_COUNTER, 1000, "", buf[0..64]), -1 as i64);
        expect_eq(formatMetric("bad:name", 1, STATSD_COUNTER, 1000, "", buf[0..64]), -1 as i64);
        expect_eq(formatMetric("bad|name", 1, STATSD_COUNTER, 1000, "", buf[0..64]), -1 as i64);
        expect_eq(formatMetric("ok", 1, STATSD_COUNTER, 0, "", buf[0..64]), -1 as i64);
        expect_eq(formatMetric("ok", 1, 99, 1000, "", buf[0..64]), -1 as i64);

        expect_eq(parseMetric("").valid, false);
        expect_eq(parseMetric("nocolon|c").valid, false);
        expect_eq(parseMetric("k:abc|c").valid, false);
        expect_eq(parseMetric("k:1|unknown").valid, false);
        expect_eq(parseMetric("k:1|c|@2.0").valid, false);
    });

    test("6. @cap/statsd — tiny buffer overflow returns -1", () => {
        let tiny: [6]u8 = undefined;
        expect_eq(formatMetric("http.requests", 100, STATSD_COUNTER, 500, "env:prod", tiny[0..6]), -1 as i64);
    });
}
