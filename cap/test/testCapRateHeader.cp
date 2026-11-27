// cap/test/testCapRateHeader.cp
// Comprehensive unit tests for @cap/rate-header

import { test, expect_eq } from "cap:test";
import {
    RateLimitPolicy,
    RateLimitState,
    parsePolicy,
    formatPolicy,
    parseUintHeader,
    parseStructuredRateLimit,
    formatStructuredRateLimit,
    isExhausted,
    computeResetDelaySecs,
} from "../../packages/governor/src/index.cp";

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
    test("1. @cap/rate-header — parsePolicy standard, burst & quoted comment", () => {
        const p1 = parsePolicy("100;w=60");
        expect_eq(p1.valid, true);
        expect_eq(p1.quota, 100 as u64);
        expect_eq(p1.window_secs, 60 as u64);
        expect_eq(p1.has_burst, false);

        const p2 = parsePolicy("100;w=60;burst=20");
        expect_eq(p2.valid, true);
        expect_eq(p2.has_burst, true);
        expect_eq(p2.burst, 20 as u64);

        const p3 = parsePolicy("250; w=300; comment=\"sliding; window, v2\"");
        expect_eq(p3.valid, true);
        expect_eq(p3.quota, 250 as u64);
        expect_eq(p3.window_secs, 300 as u64);
    });

    test("2. @cap/rate-header — formatPolicy output & buffer protection", () => {
        let buf: [32]u8 = undefined;
        const n = formatPolicy(100 as u64, 60 as u64, buf[0..32]);
        expect_eq(sliceEq(buf[0..n], "100;w=60"), true);

        let tiny: [4]u8 = undefined;
        expect_eq(formatPolicy(100 as u64, 60 as u64, tiny[0..4]), 0 as usize);
    });

    test("3. @cap/rate-header — parseStructuredRateLimit & formatStructuredRateLimit round-trip", () => {
        const st = parseStructuredRateLimit("limit=100, remaining=42, reset=15");
        expect_eq(st.valid, true);
        expect_eq(st.limit, 100 as u64);
        expect_eq(st.remaining, 42 as u64);
        expect_eq(st.reset_secs, 15 as u64);

        let buf: [64]u8 = undefined;
        const n = formatStructuredRateLimit(st, buf[0..64]);
        expect_eq(sliceEq(buf[0..n], "limit=100, remaining=42, reset=15"), true);

        const st2 = parseStructuredRateLimit(buf[0..n]);
        expect_eq(st2.valid, true);
        expect_eq(st2.limit, 100 as u64);
        expect_eq(st2.remaining, 42 as u64);
        expect_eq(st2.reset_secs, 15 as u64);
    });

    test("4. @cap/rate-header — isExhausted state checks", () => {
        const zeroRem = parseStructuredRateLimit("limit=50, remaining=0, reset=30");
        expect_eq(isExhausted(zeroRem), true);

        const hasRem = parseStructuredRateLimit("limit=50, remaining=1, reset=30");
        expect_eq(isExhausted(hasRem), false);

        const invalid = parseStructuredRateLimit("limit=50, remaining=0");
        expect_eq(invalid.valid, false);
        expect_eq(isExhausted(invalid), false);
    });

    test("5. @cap/rate-header — computeResetDelaySecs relative seconds vs Unix Epoch", () => {
        const now: u64 = 1700000000 as u64;
        expect_eq(computeResetDelaySecs(45 as u64, now), 45 as u64);
        expect_eq(computeResetDelaySecs(1700000120 as u64, now), 120 as u64);
        // Past Unix timestamp clamps to 0.
        expect_eq(computeResetDelaySecs(1699999900 as u64, now), 0 as u64);
    });

    test("6. @cap/rate-header — Invalid, negative, overflow & small buffer rejection", () => {
        expect_eq(parseUintHeader("  42  "), 42 as i64);
        expect_eq(parseUintHeader("0"), 0 as i64);
        expect_eq(parseUintHeader("-5"), -1 as i64);
        expect_eq(parseUintHeader("12a"), -1 as i64);
        expect_eq(parseUintHeader(""), -1 as i64);
        expect_eq(parseUintHeader("99999999999999999999"), -1 as i64);

        expect_eq(parsePolicy("100").valid, false);
        expect_eq(parsePolicy("abc;w=60").valid, false);

        const st = RateLimitState{
            limit: 100 as u64,
            remaining: 50 as u64,
            reset_secs: 10 as u64,
            valid: true,
        };
        let tiny: [10]u8 = undefined;
        expect_eq(formatStructuredRateLimit(st, tiny[0..10]), 0 as usize);
    });
}
