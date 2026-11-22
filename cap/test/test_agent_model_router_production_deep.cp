// Source: cp/cap/test/test_agent_model_router_production_deep.cp (Production deep tests for cap:agent model_router)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ModelRoutePolicy,
    ModelEndpoint,
    ModelRouter,
} from "cap:agent";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

test("1. PriorityOrder, LowestCost, and LowestLatency select deterministically", () => {
    let pool: [4]ModelEndpoint = std.mem.zeroes([4]ModelEndpoint);
    let pool_s: []ModelEndpoint = pool[0..];
    let router = ModelRouter.init(&pool_s);

    const flagship = router.addEndpoint(10, "flagship", 100, 5000000, 0, 0) ?? unreachable;
    const cheap = router.addEndpoint(20, "cheap-mini", 50, 150000, 0, 0) ?? unreachable;
    const fast = router.addEndpoint(30, "fast-flash", 70, 800000, 0, 0) ?? unreachable;

    // Seed EWMA latencies: flagship=600ms, cheap=400ms, fast=120ms
    expect_eq(router.recordSuccess(flagship, 600.0, 0), true);
    expect_eq(router.recordSuccess(cheap, 400.0, 0), true);
    expect_eq(router.recordSuccess(fast, 120.0, 0), true);

    expect_eq(router.availableCount(1000, 1000), 3);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 1000, 1000) ?? 99, flagship);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.LowestCost, 1000, 1000) ?? 99, cheap);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.LowestLatency, 1000, 1000) ?? 99, fast);
});

test("2. RPM and TPM dual rate limits failover to backup and auto-reset across 60s window", () => {
    let pool: [4]ModelEndpoint = std.mem.zeroes([4]ModelEndpoint);
    let pool_s: []ModelEndpoint = pool[0..];
    let router = ModelRouter.init(&pool_s);

    // Primary: rpm=2, tpm=5000; Backup: rpm=10, tpm=50000
    const primary = router.addEndpoint(1, "primary", 100, 3000000, 2, 5000) ?? unreachable;
    const backup = router.addEndpoint(2, "backup", 50, 1000000, 10, 50000) ?? unreachable;

    // Req 1 on primary: est=2000 -> window_reqs=1, window_tokens=2000
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 2000, 10000) ?? 99, primary);
    // Actual tokens used was 2500 (+500 delta) -> window_tokens=2500
    expect_eq(router.recordSuccess(primary, 200.0, 500), true);
    expect_eq((router.endpointAt(primary) ?? unreachable).window_tokens, 2500);

    // Req 2 with est=3000 exceeds primary remaining TPM (2500 + 3000 = 5500 > 5000) -> fails over to backup!
    expect_eq(router.availableCount(3000, 10500), 1);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 3000, 10500) ?? 99, backup);

    // Req 3 with est=2000 fits primary TPM (2500 + 2000 = 4500 <= 5000) & hits RPM=2
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 2000, 11000) ?? 99, primary);
    // Actual tokens was 1500 (-500 delta) -> window_tokens=4000, window_reqs=2 (RPM full!)
    expect_eq(router.recordSuccess(primary, 210.0, -500), true);
    expect_eq((router.endpointAt(primary) ?? unreachable).window_tokens, 4000);

    // Req 4 with est=100 fails over to backup because primary RPM=2 is exhausted
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 100, 20000) ?? 99, backup);

    // After 60s window expires (10000 + 60000 = 70000), primary automatically recovers!
    expect_eq(router.availableCount(4500, 70000), 2);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 4500, 70000) ?? 99, primary);
    expect_eq((router.endpointAt(primary) ?? unreachable).window_reqs, 1);
    expect_eq((router.endpointAt(primary) ?? unreachable).window_tokens, 4500);
});

test("3. 429 retry_after_ms and 5xx exponential backoff cooldown trigger failover and recovery", () => {
    let pool: [2]ModelEndpoint = std.mem.zeroes([2]ModelEndpoint);
    let pool_s: []ModelEndpoint = pool[0..];
    let router = ModelRouter.init(&pool_s);

    const p = router.addEndpoint(1, "primary", 100, 1000, 0, 0) ?? unreachable;
    const b = router.addEndpoint(2, "backup", 50, 2000, 0, 0) ?? unreachable;

    // 429 with explicit Retry-After: 5000ms at t=1000 -> cooldown until 6000
    const until1 = router.recordRateLimitOrFailure(p, 1000, 5000) ?? -1;
    expect_eq(until1, 6000);
    expect_eq(router.availableCount(100, 5999), 1);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 100, 5999) ?? 99, b);

    // At t=6000 cooldown expires -> primary automatically selected again
    expect_eq(router.availableCount(100, 6000), 2);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 100, 6000) ?? 99, p);

    // Success resets consecutive_errors back to 0
    expect_eq(router.recordSuccess(p, 150.0, 0), true);
    expect_eq((router.endpointAt(p) ?? unreachable).consecutive_errors, 0);

    // 5xx exponential backoff (retry_after_ms == 0): 1s, 2s, 4s, 8s, 16s, 32s, capped at 60s
    expect_eq(router.recordRateLimitOrFailure(p, 10000, 0) ?? -1, 11000);
    expect_eq(router.recordRateLimitOrFailure(p, 10000, 0) ?? -1, 12000);
    expect_eq(router.recordRateLimitOrFailure(p, 10000, 0) ?? -1, 14000);
    expect_eq(router.recordRateLimitOrFailure(p, 10000, 0) ?? -1, 18000);
    expect_eq(router.recordRateLimitOrFailure(p, 10000, 0) ?? -1, 26000);
    expect_eq(router.recordRateLimitOrFailure(p, 10000, 0) ?? -1, 42000);
    expect_eq(router.recordRateLimitOrFailure(p, 10000, 0) ?? -1, 70000); // capped at +60000
    expect_eq(router.recordRateLimitOrFailure(p, 10000, 0) ?? -1, 70000); // still capped at +60000
});

test("4. Dynamic EWMA latency updates shift LowestLatency routing decisions", () => {
    let pool: [2]ModelEndpoint = std.mem.zeroes([2]ModelEndpoint);
    let pool_s: []ModelEndpoint = pool[0..];
    let router = ModelRouter.init(&pool_s);

    const a = router.addEndpoint(1, "model-a", 10, 1000, 0, 0) ?? unreachable;
    const b = router.addEndpoint(2, "model-b", 10, 1000, 0, 0) ?? unreachable;

    // Initially A=100ms, B=250ms -> A wins LowestLatency
    expect_eq(router.recordSuccess(a, 100.0, 0), true);
    expect_eq(router.recordSuccess(b, 250.0, 0), true);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.LowestLatency, 100, 1000) ?? 99, a);

    // Model A suffers latency spikes (1000ms x 3):
    // 0.3*1000 + 0.7*100 = 370 -> 0.3*1000 + 0.7*370 = 559 -> 0.3*1000 + 0.7*559 = 691.3
    expect_eq(router.recordSuccess(a, 1000.0, 0), true);
    expect_eq(approxEq((router.endpointAt(a) ?? unreachable).ewma_latency_ms, 370.0, 1e-9), true);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.LowestLatency, 100, 2000) ?? 99, b);
});

test("5. Defensive guards for empty pool, duplicate ID, disabled slot, OOB, negative and NaN latency", () => {
    let pool: [2]ModelEndpoint = std.mem.zeroes([2]ModelEndpoint);
    let pool_s: []ModelEndpoint = pool[0..];
    let router = ModelRouter.init(&pool_s);

    // Empty pool
    expect_eq(router.availableCount(100, 1000), 0);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 100, 1000) == null, true);

    // Reject empty name
    expect_eq(router.addEndpoint(1, "", 10, 100, 0, 0) == null, true);

    // Add 2 endpoints, reject duplicate ID and pool overflow
    const s0 = router.addEndpoint(1, "m1", 10, 100, 0, 0) ?? unreachable;
    expect_eq(router.addEndpoint(1, "m1-dup", 20, 100, 0, 0) == null, true);
    const s1 = router.addEndpoint(2, "m2", 5, 100, 0, 0) ?? unreachable;
    expect_eq(router.addEndpoint(3, "m3-overflow", 5, 100, 0, 0) == null, true);

    // Disabled endpoint skipped
    expect_eq(router.setEnabled(s0, false), true);
    expect_eq(router.availableCount(100, 1000), 1);
    expect_eq(router.selectEndpoint(ModelRoutePolicy.PriorityOrder, 100, 1000) ?? 99, s1);

    // Out-of-bounds, negative latency, and NaN latency rejected
    expect_eq(router.setEnabled(99, false), false);
    expect_eq(router.endpointAt(99) == null, true);
    expect_eq(router.recordRateLimitOrFailure(99, 1000, 0) == null, true);
    expect_eq(router.recordSuccess(99, 100.0, 0), false);
    expect_eq(router.recordSuccess(s1, -5.0, 0), false);
    expect_eq(router.recordSuccess(s1, std.math.nan(f64), 0), false);
});
