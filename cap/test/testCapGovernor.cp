// cap/test/testCapGovernor.cp
// Comprehensive unit tests for @cap/governor (RateLimit policy & structured header engine)

import { test, expect_eq } from "cap:test";
import {
    RateLimitPolicy,
    RateLimitState,
    parseUintHeader,
    parsePolicy,
    formatPolicy,
    parseStructuredRateLimit,
    formatStructuredRateLimit,
    isExhausted,
    computeResetDelaySecs,
} from "../../packages/governor/src/index.cp";

fn govEq(a: []const u8, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/governor — parsePolicy & formatPolicy", () => {
        const p: RateLimitPolicy = parsePolicy("120;w=60;burst=30");
        expect_eq(p.valid, true);
        expect_eq(p.quota, 120 as u64);
        expect_eq(p.window_secs, 60 as u64);
        expect_eq(p.has_burst, true);
        expect_eq(p.burst, 30 as u64);

        let buf: [32]u8 = undefined;
        const n = formatPolicy(120 as u64, 60 as u64, buf[0..32]);
        expect_eq(govEq(buf[0..n], "120;w=60"), true);
    });

    test("2. @cap/governor — parseStructuredRateLimit, isExhausted & reset delay", () => {
        const st: RateLimitState = parseStructuredRateLimit("limit=100, remaining=0, reset=45");
        expect_eq(st.valid, true);
        expect_eq(st.limit, 100 as u64);
        expect_eq(st.remaining, 0 as u64);
        expect_eq(st.reset_secs, 45 as u64);
        expect_eq(isExhausted(st), true);

        let buf: [64]u8 = undefined;
        const n = formatStructuredRateLimit(st, buf[0..64]);
        expect_eq(govEq(buf[0..n], "limit=100, remaining=0, reset=45"), true);

        expect_eq(parseUintHeader(" 42 "), 42 as i64);
        expect_eq(computeResetDelaySecs(1700000100 as u64, 1700000040 as u64), 60 as u64);
    });
}
