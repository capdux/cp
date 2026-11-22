// Source: cp/cap/test/test_ratelimit_production_deep.cp
// Deep Production Verification Suite for cap:ratelimit & cap.ratelimit
// Covers:
//   1. TokenBucket burst consumption, exact retry_after_ms, smooth refill & clock-rollback clamping
//   2. SlidingWindowCounter dual-window weighted smoothing preventing boundary double-burst spikes
//   3. GcraLimiter O(1) single-tat_ms burst tolerance & steady emission interval enforcement
//   4. KeyedRateLimiter multi-tenant isolation, evictIdle TTL cleanup & RateLimitError validation
//   5. `import cap from "cap"; cap.ratelimit` dynamic mounting & std.testing.allocator zero-leak check

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RateLimitError,
    RateDecision,
    TokenBucket,
    SlidingWindowCounter,
    GcraLimiter,
    KeyedRateLimiter,
} from "cap:web";

test("cap:ratelimit - TokenBucket burst, exact retry_after_ms, smooth refill & rollback clamp") {
    let tb = TokenBucket.init(10, 5.0, 1000) catch unreachable;

    // Consume full burst: 6 then 4
    const d1 = tb.tryAcquire(6, 1000) catch unreachable;
    expect_eq(d1.allowed, true);
    expect_eq(d1.remaining, 4);
    expect_eq(d1.retry_after_ms, 0);

    const d2 = tb.tryAcquire(4, 1000) catch unreachable;
    expect_eq(d2.allowed, true);
    expect_eq(d2.remaining, 0);
    expect_eq(d2.reset_after_ms, 2000);

    // Bucket empty: requesting 2 tokens at 5 tok/s requires exactly 400ms
    const d3 = tb.tryAcquire(2, 1000) catch unreachable;
    expect_eq(d3.allowed, false);
    expect_eq(d3.remaining, 0);
    expect_eq(d3.retry_after_ms, 400);

    // Advance 200ms -> 1.0 token refilled; requesting 2 still needs 200ms
    const p1 = tb.peek(1200);
    expect_eq(p1.allowed, true);
    expect_eq(p1.remaining, 1);

    const d4 = tb.tryAcquire(2, 1200) catch unreachable;
    expect_eq(d4.allowed, false);
    expect_eq(d4.remaining, 1);
    expect_eq(d4.retry_after_ms, 200);

    // Clock rollback to 900ms must clamp safely and never deduct tokens
    const p_rollback = tb.peek(900);
    expect_eq(p_rollback.remaining, 1);

    // Advance to 1400ms (+200ms from 1200ms) -> 2.0 tokens available -> allowed
    const d5 = tb.tryAcquire(2, 1400) catch unreachable;
    expect_eq(d5.allowed, true);
    expect_eq(d5.remaining, 0);

    // Reset restores full capacity
    tb.reset(2000);
    expect_eq(tb.peek(2000).remaining, 10);
}

test("cap:ratelimit - SlidingWindowCounter boundary smoothing prevents double-burst spike") {
    let sw = SlidingWindowCounter.init(10, 1000, 0) catch unreachable;

    // Fill 8 requests in window [0, 1000)
    const d1 = sw.tryAcquire(8, 900) catch unreachable;
    expect_eq(d1.allowed, true);
    expect_eq(d1.remaining, 2);

    // Cross into next window at 1200ms (20% elapsed -> prev_count weight = 0.8 -> weighted = 6.4)
    // Fixed window would allow 10 more (total 18 in 300ms); sliding window only has 3.6 headroom!
    const d_spike = sw.tryAcquire(5, 1200) catch unreachable;
    expect_eq(d_spike.allowed, false);
    expect_eq(d_spike.remaining, 3);
    expect_eq(d_spike.retry_after_ms > 0, true);

    // Acquiring 3 fits within 6.4 + 3 = 9.4 <= 10.0
    const d_ok = sw.tryAcquire(3, 1200) catch unreachable;
    expect_eq(d_ok.allowed, true);
    expect_eq(d_ok.remaining, 0);

    // At 1800ms (80% elapsed -> prev weight = 0.2 -> 8 * 0.2 + 3 = 4.6), 5 more fits!
    const d_later = sw.tryAcquire(5, 1800) catch unreachable;
    expect_eq(d_later.allowed, true);

    sw.reset(2000);
    expect_eq(sw.currentCount(2000) == 0.0, true);
}

test("cap:ratelimit - GcraLimiter O(1) single-tat_ms burst tolerance & emission interval") {
    // T = 100ms, burst_tolerance = 2 -> max burst = 3 cells
    let gcra = GcraLimiter.init(100, 2) catch unreachable;

    const d1 = gcra.tryAcquire(1, 1000) catch unreachable;
    expect_eq(d1.allowed, true);
    expect_eq(d1.remaining, 2);
    expect_eq(gcra.tatMs(), 1100);

    const d2 = gcra.tryAcquire(1, 1000) catch unreachable;
    expect_eq(d2.allowed, true);
    expect_eq(d2.remaining, 1);
    expect_eq(gcra.tatMs(), 1200);

    const d3 = gcra.tryAcquire(1, 1000) catch unreachable;
    expect_eq(d3.allowed, true);
    expect_eq(d3.remaining, 0);
    expect_eq(gcra.tatMs(), 1300);

    // 4th immediate cell rejected; retry_after_ms = 100ms
    const d4 = gcra.tryAcquire(1, 1000) catch unreachable;
    expect_eq(d4.allowed, false);
    expect_eq(d4.remaining, 0);
    expect_eq(d4.retry_after_ms, 100);

    // At 1100ms (+100ms), 1 cell is admitted and tat advances to 1400ms
    const d5 = gcra.tryAcquire(1, 1100) catch unreachable;
    expect_eq(d5.allowed, true);
    expect_eq(d5.remaining, 0);
    expect_eq(gcra.tatMs(), 1400);

    gcra.reset();
    expect_eq(gcra.tatMs(), 0);
}

test("cap:ratelimit - KeyedRateLimiter isolation, evictIdle & RateLimitError validation") {
    const alloc = std.testing.allocator;
    let keyed = KeyedRateLimiter.init(alloc, 4, 2.0) catch unreachable;
    defer keyed.deinit();

    // Tenant A exhausts its quota; Tenant B remains unaffected
    const a1 = keyed.tryAcquireFor("tenant-a", 4, 100) catch unreachable;
    expect_eq(a1.allowed, true);
    expect_eq(a1.remaining, 0);

    const a2 = keyed.tryAcquireFor("tenant-a", 1, 100) catch unreachable;
    expect_eq(a2.allowed, false);

    const b1 = keyed.tryAcquireFor("tenant-b", 2, 500) catch unreachable;
    expect_eq(b1.allowed, true);
    expect_eq(b1.remaining, 2);
    expect_eq(keyed.keyCount(), 2);

    // At now=1200 with idle_ttl=1000: tenant-a idle=1100 (evicted), tenant-b idle=700 (kept)
    const evicted = keyed.evictIdle(1200, 1000);
    expect_eq(evicted, 1);
    expect_eq(keyed.keyCount(), 1);

    // Invalid configuration & cost error checks
    let err_burst = false;
    * = TokenBucket.init(0, 1.0, 0) catch |e| {
        expect_eq(e == RateLimitError.InvalidBurst, true);
        err_burst = true;
    };
    expect_eq(err_burst, true);

    let err_rate = false;
    * = GcraLimiter.init(0, 1) catch |e| {
        expect_eq(e == RateLimitError.InvalidRate, true);
        err_rate = true;
    };
    expect_eq(err_rate, true);

    let err_win = false;
    * = SlidingWindowCounter.init(10, 0, 0) catch |e| {
        expect_eq(e == RateLimitError.InvalidWindow, true);
        err_win = true;
    };
    expect_eq(err_win, true);

    let err_cost = false;
    * = keyed.tryAcquireFor("tenant-b", 0, 1200) catch |e| {
        expect_eq(e == RateLimitError.InvalidCost, true);
        err_cost = true;
    };
    expect_eq(err_cost, true);
}

test("cap.ratelimit - dynamic mounting & std.testing.allocator zero-leak verification") {
    const alloc = std.testing.allocator;

    let tb = cap.web.TokenBucket.initWithTokens(5, 10.0, 2, 1000) catch unreachable;
    const d1 = tb.tryAcquire(2, 1000) catch unreachable;
    expect_eq(d1.allowed, true);
    expect_eq(d1.remaining, 0);

    let sw = cap.web.SlidingWindowCounter.init(4, 1000, 0) catch unreachable;
    expect_eq((sw.tryAcquire(4, 100) catch unreachable).allowed, true);

    let gcra = cap.web.GcraLimiter.init(50, 1) catch unreachable;
    expect_eq((gcra.tryAcquire(2, 500) catch unreachable).allowed, true);

    let keyed = cap.web.KeyedRateLimiter.init(alloc, 3, 1.0) catch unreachable;
    defer keyed.deinit();
    expect_eq((keyed.tryAcquireFor("k1", 1, 100) catch unreachable).allowed, true);
    expect_eq((keyed.tryAcquireFor("k2", 1, 200) catch unreachable).allowed, true);
    expect_eq(keyed.evictIdle(1500, 1350), 1);
    expect_eq(keyed.keyCount(), 1);
}
