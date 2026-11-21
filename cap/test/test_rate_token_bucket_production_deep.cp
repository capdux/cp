// Source: cp/cap/test/test_rate_token_bucket_production_deep.cp
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import { RateLimitError, TokenBucket, GcraLimiter } from "cap:rate_token_bucket";

test "rate_token_bucket: burst consumption, refill, and retryAfterNs" {
    let tb = try TokenBucket.init(10.0, 5.0, 0); // 10 req/s => 1 token per 100ms, burst=5
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        try expect_eq(try tb.allow(1.0, 0), true);
    }
    // 6th immediate request denied
    try expect_eq(try tb.allow(1.0, 0), false);
    const wait = tb.retryAfterNs(1.0, 0);
    try expect_eq(wait, 100_000_000);

    // Advance 100ms -> 1 token refilled
    try expect_eq(try tb.allow(1.0, 100_000_000), true);
    try expect_eq(try tb.allow(1.0, 100_000_000), false);
}

test "rate_token_bucket: capacity cap prevents unbounded accumulation" {
    let tb = try TokenBucket.init(100.0, 10.0, 0);
    // Advance 10 seconds -> capped at 10.0
    const avail = tb.available(10_000_000_000);
    try expect_eq(avail, 10.0);
    // Request exceeding burst capacity is always denied
    try expect_eq(try tb.allow(11.0, 10_000_000_000), false);
}

test "rate_token_bucket: GCRA leaky-bucket burst & sustained pacing" {
    let gcra = try GcraLimiter.init(10.0, 3); // 100ms per request, burst 3
    try expect_eq(gcra.allow(1_000_000_000), true);
    try expect_eq(gcra.allow(1_000_000_000), true);
    try expect_eq(gcra.allow(1_000_000_000), true);
    try expect_eq(gcra.allow(1_000_000_000), false);

    // After 100ms, 1 slot opens
    try expect_eq(gcra.allow(1_100_000_000), true);
    try expect_eq(gcra.allow(1_100_000_000), false);
}

test "rate_token_bucket: error guards for invalid rate, burst, cost" {
    try expect_eq(TokenBucket.init(0.0, 5.0, 0) == RateLimitError.InvalidRate, true);
    try expect_eq(TokenBucket.init(5.0, -1.0, 0) == RateLimitError.InvalidBurst, true);
    try expect_eq(GcraLimiter.init(5.0, 0) == RateLimitError.InvalidBurst, true);

    let tb = try TokenBucket.init(5.0, 5.0, 0);
    try expect_eq(tb.allow(0.0, 0) == RateLimitError.InvalidCost, true);
}

test "rate_token_bucket: cap.rate_token_bucket dynamic mount" {
    let tb = try cap.rate_token_bucket.TokenBucket.init(1.0, 2.0, 0);
    try expect_eq(try tb.allow(2.0, 0), true);
    try expect_eq(try tb.allow(1.0, 0), false);
}
