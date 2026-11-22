// Source: cp/cap/test/test_circuit_production_deep.cp
// Deep Production Verification Suite for cap:circuit & cap.circuit
// Covers:
//   1. Closed -> Open -> HalfOpen -> Closed full state machine & HalfOpen failure immediate fallback to Open
//   2. Ring-buffer sliding window eviction of old failures & minimum_calls guard
//   3. BackoffPolicy 4 JitterMode (None/Full/Equal/Decorrelated) bounds, determinism & max_delay_ms cap
//   4. Bulkhead concurrency limit, releasePermit recovery, forceOpen/forceClose/reset & InvalidConfig checks
//   5. `import cap from "cap"; cap.circuit` dynamic mounting & std.testing.allocator zero-leak verification

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    CircuitError,
    CircuitState,
    CircuitConfig,
    CircuitStats,
    CircuitBreaker,
    JitterMode,
    BackoffPolicy,
    Bulkhead,
} from "cap:web";

test("cap:circuit - Closed -> Open -> HalfOpen -> Closed & HalfOpen failure fallback") {
    const alloc = std.heap.page_allocator;
    let cb = CircuitBreaker.init(alloc, CircuitConfig{
        failure_threshold_rate: 0.5,
        minimum_calls: 4,
        window_size: 6,
        open_duration_ms: 1000,
        half_open_max_calls: 2,
        half_open_success_threshold: 2,
    }) catch unreachable;
    defer cb.deinit();

    // 2 successes, 2 failures -> 4 calls >= minimum_calls, rate = 0.5 >= 0.5 -> trips to Open
    expect_eq(cb.allowRequest(100), true);
    cb.recordSuccess(100);
    expect_eq(cb.allowRequest(110), true);
    cb.recordSuccess(110);
    expect_eq(cb.allowRequest(120), true);
    cb.recordFailure(120);
    expect_eq(cb.stats(120).state == CircuitState.Closed, true);

    expect_eq(cb.allowRequest(130), true);
    cb.recordFailure(130);

    const st_open = cb.stats(130);
    expect_eq(st_open.state == CircuitState.Open, true);
    expect_eq(st_open.opened_count, 1);

    // Before open_duration_ms (1000ms), requests are rejected
    expect_eq(cb.allowRequest(500), false);
    expect_eq(cb.allowRequest(1129), false);
    expect_eq(cb.stats(1129).rejected_count, 2);

    // At 1130ms (130 + 1000), advances to HalfOpen; max 2 probes allowed
    expect_eq(cb.allowRequest(1130), true);
    expect_eq(cb.stats(1130).state == CircuitState.HalfOpen, true);
    expect_eq(cb.allowRequest(1131), true);
    // 3rd probe in HalfOpen exceeds half_open_max_calls (2) -> rejected
    expect_eq(cb.allowRequest(1132), false);
    expect_eq(cb.stats(1132).rejected_count, 3);

    // A single failure in HalfOpen immediately falls back to Open
    cb.recordFailure(1140);
    expect_eq(cb.stats(1140).state == CircuitState.Open, true);
    expect_eq(cb.stats(1140).opened_count, 2);
    expect_eq(cb.allowRequest(1500), false);

    // Wait another 1000ms -> 2140ms -> HalfOpen -> 2 consecutive successes -> Closed
    expect_eq(cb.allowRequest(2140), true);
    cb.recordSuccess(2145);
    expect_eq(cb.stats(2145).state == CircuitState.HalfOpen, true);
    expect_eq(cb.allowRequest(2150), true);
    cb.recordSuccess(2155);

    const st_closed = cb.stats(2155);
    expect_eq(st_closed.state == CircuitState.Closed, true);
    expect_eq(st_closed.window_calls, 0);
    expect_eq(st_closed.window_failures, 0);
}

test("cap:circuit - sliding window eviction & minimum_calls guard") {
    const alloc = std.heap.page_allocator;
    let cb = CircuitBreaker.init(alloc, CircuitConfig{
        failure_threshold_rate: 0.5,
        minimum_calls: 4,
        window_size: 4,
        open_duration_ms: 1000,
        half_open_max_calls: 1,
        half_open_success_threshold: 1,
    }) catch unreachable;
    defer cb.deinit();

    // 1 failure then 3 successes -> below minimum_calls until 4th call, rate = 1/4 = 0.25 < 0.5
    cb.recordFailure(10);
    expect_eq(cb.stats(10).state == CircuitState.Closed, true);
    cb.recordSuccess(20);
    cb.recordSuccess(30);
    cb.recordSuccess(40);
    expect_eq(cb.stats(40).state == CircuitState.Closed, true);
    expect_eq(cb.stats(40).window_failures, 1);

    // 5th call is a success -> evicts the old failure at slot 0 -> window_failures becomes 0!
    cb.recordSuccess(50);
    expect_eq(cb.stats(50).window_calls, 4);
    expect_eq(cb.stats(50).window_failures, 0);

    // Now 1 failure makes rate = 1/4 = 0.25 (would have been 2/4 = 0.5 without eviction!)
    cb.recordFailure(60);
    expect_eq(cb.stats(60).state == CircuitState.Closed, true);
    expect_eq(cb.stats(60).window_failures, 1);

    // Verify minimum_calls guard on a fresh breaker: 3/3 failures (100%) < minimum_calls (4) -> stays Closed
    let cb2 = CircuitBreaker.init(alloc, CircuitConfig{
        failure_threshold_rate: 0.5,
        minimum_calls: 4,
        window_size: 6,
        open_duration_ms: 500,
        half_open_max_calls: 1,
        half_open_success_threshold: 1,
    }) catch unreachable;
    defer cb2.deinit();

    cb2.recordFailure(1);
    cb2.recordFailure(2);
    cb2.recordFailure(3);
    expect_eq(cb2.stats(3).state == CircuitState.Closed, true);
    cb2.recordFailure(4);
    expect_eq(cb2.stats(4).state == CircuitState.Open, true);
}

test("cap:circuit - BackoffPolicy 4 JitterModes bounds, determinism & max_delay_ms cap") {
    // 1. JitterMode.None: exact exponential growth capped at max_delay_ms
    let p_none = BackoffPolicy.init(100, 500, 2.0, 4, JitterMode.None) catch unreachable;
    expect_eq(p_none.canRetry(0), true);
    expect_eq(p_none.canRetry(3), true);
    expect_eq(p_none.canRetry(4), false);
    expect_eq(p_none.nextDelayMs(0, 0, 99), 100);
    expect_eq(p_none.nextDelayMs(1, 100, 99), 200);
    expect_eq(p_none.nextDelayMs(2, 200, 99), 400);
    expect_eq(p_none.nextDelayMs(3, 400, 99), 500);
    expect_eq(p_none.nextDelayMs(20, 500, 99), 500);

    // 2. JitterMode.Full: [0, capped]
    let p_full = BackoffPolicy.init(100, 1000, 2.0, 5, JitterMode.Full) catch unreachable;
    let attempt: u32 = 0;
    while (attempt < 5) : (attempt += 1) {
        const d1 = p_full.nextDelayMs(attempt, 100, 12345);
        const d2 = p_full.nextDelayMs(attempt, 100, 12345);
        expect_eq(d1, d2);
        expect_eq(d1 >= 0 && d1 <= 1000, true);
    }

    // 3. JitterMode.Equal: [capped/2, capped]
    let p_equal = BackoffPolicy.init(100, 800, 2.0, 5, JitterMode.Equal) catch unreachable;
    const eq0 = p_equal.nextDelayMs(0, 0, 777);
    expect_eq(eq0 >= 50 && eq0 <= 100, true);
    const eq3 = p_equal.nextDelayMs(3, 400, 777);
    expect_eq(eq3 >= 400 && eq3 <= 800, true);

    // 4. JitterMode.Decorrelated: [base_delay_ms, min(max_delay_ms, prev * 3)]
    let p_dec = BackoffPolicy.init(50, 600, 2.0, 5, JitterMode.Decorrelated) catch unreachable;
    let prev: i64 = 50;
    let k: u32 = 0;
    while (k < 6) : (k += 1) {
        const d = p_dec.nextDelayMs(k, prev, 4242);
        expect_eq(d >= 50 && d <= 600, true);
        prev = d;
    }
}

test("cap:circuit - Bulkhead concurrency, releasePermit, forceOpen/forceClose & InvalidConfig") {
    let bh = Bulkhead.init(2, 4) catch unreachable;
    expect_eq(bh.maxConcurrent(), 2);
    expect_eq(bh.maxQueue(), 4);
    expect_eq(bh.activeCount(), 0);

    expect_eq(bh.tryAcquire(), true);
    expect_eq(bh.tryAcquire(), true);
    expect_eq(bh.activeCount(), 2);
    expect_eq(bh.tryAcquire(), false);
    expect_eq(bh.rejectedCount(), 1);

    bh.releasePermit();
    expect_eq(bh.activeCount(), 1);
    expect_eq(bh.tryAcquire(), true);
    expect_eq(bh.activeCount(), 2);

    // forceOpen / forceClose / reset
    const alloc = std.heap.page_allocator;
    let cb = CircuitBreaker.init(alloc, CircuitConfig{
        failure_threshold_rate: 0.5,
        minimum_calls: 2,
        window_size: 4,
        open_duration_ms: 1000,
        half_open_max_calls: 1,
        half_open_success_threshold: 1,
    }) catch unreachable;
    defer cb.deinit();

    cb.forceOpen(500);
    expect_eq(cb.stats(500).state == CircuitState.Open, true);
    expect_eq(cb.allowRequest(600), false);
    cb.forceClose();
    expect_eq(cb.stats(600).state == CircuitState.Closed, true);
    expect_eq(cb.allowRequest(600), true);
    cb.reset();
    expect_eq(cb.stats(600).total_calls, 0);
    expect_eq(cb.stats(600).rejected_count, 0);

    // InvalidConfig checks
    let bad_cb_seen = false;
    * = CircuitBreaker.init(alloc, CircuitConfig{
        failure_threshold_rate: 1.5,
        minimum_calls: 2,
        window_size: 4,
        open_duration_ms: 1000,
        half_open_max_calls: 1,
        half_open_success_threshold: 1,
    }) catch |e| {
        expect_eq(e == CircuitError.InvalidConfig, true);
        bad_cb_seen = true;
    };
    expect_eq(bad_cb_seen, true);

    let bad_bo_seen = false;
    * = BackoffPolicy.init(0, 100, 2.0, 3, JitterMode.None) catch |e| {
        expect_eq(e == CircuitError.InvalidConfig, true);
        bad_bo_seen = true;
    };
    expect_eq(bad_bo_seen, true);

    let bad_bh_seen = false;
    * = Bulkhead.init(0, 10) catch |e| {
        expect_eq(e == CircuitError.InvalidConfig, true);
        bad_bh_seen = true;
    };
    expect_eq(bad_bh_seen, true);
}

test("cap.circuit - dynamic mounting & std.testing.allocator zero-leak check") {
    const alloc = std.testing.allocator;
    let cb = cap.web.CircuitBreaker.init(alloc, cap.web.CircuitConfig{
        failure_threshold_rate: 0.5,
        minimum_calls: 2,
        window_size: 4,
        open_duration_ms: 200,
        half_open_max_calls: 1,
        half_open_success_threshold: 1,
    }) catch unreachable;
    defer cb.deinit();

    expect_eq(cb.allowRequest(10), true);
    cb.recordFailure(10);
    cb.recordFailure(20);
    expect_eq(cb.stats(20).state == cap.web.CircuitState.Open, true);
    expect_eq(cb.allowRequest(220), true);
    cb.recordSuccess(225);
    expect_eq(cb.stats(225).state == cap.web.CircuitState.Closed, true);

    let bo = cap.web.BackoffPolicy.init(10, 100, 2.0, 3, cap.web.JitterMode.Equal) catch unreachable;
    const d = bo.nextDelayMs(1, 10, 999);
    expect_eq(d >= 10 && d <= 20, true);

    let bh = cap.web.Bulkhead.init(1, 0) catch unreachable;
    expect_eq(bh.tryAcquire(), true);
    expect_eq(bh.tryAcquire(), false);
    bh.releasePermit();
    expect_eq(bh.tryAcquire(), true);
}
