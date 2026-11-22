// Origin: Pure-CP Stdlib Test Suite (lib/net/circuitPool.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Envoy-Style Upstream Outlier Ejection & Circuit-Breaker Pool:
//   1. Consecutive 5xx/timeout ejection, escalating cooldown (base * count capped at max), single-flight half-open probe & recovery
//   2. Envoy max_ejection_percent_bps (50%) protection prevents cascading total-cluster blackhole
//   3. 8-bucket sliding-window error_rate_bps ejection, bucket expiry & 4xx client-error immunity
//   4. Weighted healthy endpoint selection proportionality & dynamic weight updates
//   5. Panic-mode fallback routing when all endpoints are ejected
//   6. Monotonic clock rollback guard & manual forceRecover
//   7. Full boundary guards (invalid config, duplicate/unknown IDs, zero weight, 64-slot capacity) & cap.net facade

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    CIRCUIT_POOL_MAX_ENDPOINTS,
    CircuitEndpointState,
    CircuitOutcomeKind,
    CircuitPickMode,
    CircuitPoolConfig,
    CircuitBreakerPool,
} from "cap:net";

// ============================================================================
// Test 1: Consecutive Failures, Escalating Cooldown, Single-Flight Probe & Recovery
// ============================================================================
test("circuitPool: consecutive ejection, escalating cooldown, single-flight half-open probe & recovery", () => {
    const cfg = CircuitPoolConfig{
        consecutive_5xx_threshold: 3,
        error_rate_threshold_bps: 0, // isolate consecutive-failure trigger
        window_ms: 8_000,
        base_ejection_ms: 10_000,
        max_ejection_ms: 25_000,
        max_ejection_percent_bps: 5_000,
        half_open_success_threshold: 2,
    };
    let pool = CircuitBreakerPool.init(cfg) ?? return;
    expect_true(pool.addEndpoint(101, 10));
    expect_true(pool.addEndpoint(102, 10));

    // 3 consecutive failures on endpoint 101 -> ejected for 10_000ms (1 * base)
    expect_true(pool.recordOutcome(101, CircuitOutcomeKind.server_error_5xx, 1_000));
    expect_true(pool.recordOutcome(101, CircuitOutcomeKind.timeout, 1_100));
    expect_true(pool.recordOutcome(101, CircuitOutcomeKind.connect_failure, 1_200));

    if (v1 := pool.getEndpoint(101)) {
        expect_eq(v1.state, CircuitEndpointState.ejected_open);
        expect_eq(v1.ejection_count, 1 as u32);
        expect_eq(v1.eject_until_ms, 11_200 as u64);
    } else {
        expect_true(false);
    }

    // Before expiry, picks must skip 101 and always route to healthy 102
    let s: u64 = 0;
    while (s < 20) : (s += 1) {
        const pick = pool.pickHealthyEndpoint(s, 5_000);
        expect_eq(pick.mode, CircuitPickMode.normal_healthy);
        expect_eq(pick.endpoint_id, 102 as u64);
    }

    // At t = 11_200, 101 expires into half_open_probe; first pick admits single-flight probe
    const probe1 = pool.pickHealthyEndpoint(1, 11_200);
    expect_eq(probe1.mode, CircuitPickMode.half_open_probe);
    expect_eq(probe1.endpoint_id, 101 as u64);

    // Second concurrent pick while probe is in flight must route to healthy 102
    const concurrent = pool.pickHealthyEndpoint(2, 11_250);
    expect_eq(concurrent.mode, CircuitPickMode.normal_healthy);
    expect_eq(concurrent.endpoint_id, 102 as u64);

    // Probe fails -> re-ejected with escalated cooldown 2 * 10_000 = 20_000ms (until 31_300)
    expect_true(pool.recordOutcome(101, CircuitOutcomeKind.server_error_5xx, 11_300));
    if (v2 := pool.getEndpoint(101)) {
        expect_eq(v2.state, CircuitEndpointState.ejected_open);
        expect_eq(v2.ejection_count, 2 as u32);
        expect_eq(v2.eject_until_ms, 31_300 as u64);
    } else {
        expect_true(false);
    }

    // Expire and fail probe again -> 3 * 10_000 = 30_000 capped at max_ejection_ms = 25_000 (until 56_300)
    const probe2 = pool.pickHealthyEndpoint(3, 31_300);
    expect_eq(probe2.mode, CircuitPickMode.half_open_probe);
    expect_true(pool.recordOutcome(101, CircuitOutcomeKind.timeout, 31_300));
    if (v3 := pool.getEndpoint(101)) {
        expect_eq(v3.ejection_count, 3 as u32);
        expect_eq(v3.eject_until_ms, 56_300 as u64);
    } else {
        expect_true(false);
    }

    // Expire at 56_300 and succeed twice (half_open_success_threshold = 2) -> back to healthy
    const probe3a = pool.pickHealthyEndpoint(4, 56_300);
    expect_eq(probe3a.mode, CircuitPickMode.half_open_probe);
    expect_true(pool.recordOutcome(101, CircuitOutcomeKind.success, 56_400));
    if (vmid := pool.getEndpoint(101)) {
        expect_eq(vmid.state, CircuitEndpointState.half_open_probe);
        expect_eq(vmid.probe_in_flight, false);
    } else {
        expect_true(false);
    }

    const probe3b = pool.pickHealthyEndpoint(5, 56_500);
    expect_eq(probe3b.mode, CircuitPickMode.half_open_probe);
    expect_true(pool.recordOutcome(101, CircuitOutcomeKind.success, 56_600));
    if (vrec := pool.getEndpoint(101)) {
        expect_eq(vrec.state, CircuitEndpointState.healthy);
        expect_eq(vrec.consecutive_failures, 0 as u32);
        expect_eq(vrec.window_errors, 0 as u32);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 2: Envoy Max Ejection Percent Protection (50% Cap)
// ============================================================================
test("circuitPool: Envoy max_ejection_percent_bps caps ejected endpoints at 50%", () => {
    const cfg = CircuitPoolConfig{
        consecutive_5xx_threshold: 2,
        error_rate_threshold_bps: 0,
        window_ms: 8_000,
        base_ejection_ms: 30_000,
        max_ejection_ms: 60_000,
        max_ejection_percent_bps: 5_000, // 50% of 4 = max 2 ejected
    };
    let pool = CircuitBreakerPool.init(cfg) ?? return;
    expect_true(pool.addEndpoint(1, 10));
    expect_true(pool.addEndpoint(2, 10));
    expect_true(pool.addEndpoint(3, 10));
    expect_true(pool.addEndpoint(4, 10));
    expect_eq(pool.maxEjectableCount(), 2 as usize);

    // Eject nodes 1 and 2
    let ep: u64 = 1;
    while (ep <= 4) : (ep += 1) {
        expect_true(pool.recordOutcome(ep, CircuitOutcomeKind.server_error_5xx, 1_000));
        expect_true(pool.recordOutcome(ep, CircuitOutcomeKind.server_error_5xx, 1_100));
    }

    const st = pool.stats();
    expect_eq(st.total_endpoints, 4 as usize);
    expect_eq(st.ejected_count, 2 as usize);
    expect_eq(st.healthy_count, 2 as usize);
    expect_eq(st.total_ejections, 2 as u64);
    expect_eq(st.ejection_suppressed_by_cap, 2 as u64);

    // Normal routing continues using nodes 3 and 4 without panic
    const pick = pool.pickHealthyEndpoint(42, 2_000);
    expect_eq(pick.mode, CircuitPickMode.normal_healthy);
    expect_true(pick.endpoint_id == 3 || pick.endpoint_id == 4);
});

// ============================================================================
// Test 3: Sliding-Window Error Rate (error_rate_bps) & 4xx Immunity
// ============================================================================
test("circuitPool: sliding-window error_rate_bps ejection, bucket expiry & 4xx immunity", () => {
    const cfg = CircuitPoolConfig{
        consecutive_5xx_threshold: 10, // high so only error rate triggers
        error_rate_threshold_bps: 5_000, // 50%
        min_request_volume: 6,
        window_ms: 8_000, // 1000ms per bucket
        base_ejection_ms: 20_000,
        max_ejection_ms: 60_000,
        max_ejection_percent_bps: 5_000,
    };
    let pool = CircuitBreakerPool.init(cfg) ?? return;
    expect_true(pool.addEndpoint(10, 1));
    expect_true(pool.addEndpoint(20, 1));

    // Endpoint 20 receives 10 client 4xx errors -> must stay healthy with 0 error_rate_bps
    let i: u64 = 0;
    while (i < 10) : (i += 1) {
        expect_true(pool.recordOutcome(20, CircuitOutcomeKind.client_error_4xx, 1_000 + i * 10));
    }
    if (v20 := pool.getEndpoint(20)) {
        expect_eq(v20.state, CircuitEndpointState.healthy);
        expect_eq(v20.consecutive_failures, 0 as u32);
        expect_eq(v20.error_rate_bps, 0 as u32);
        expect_eq(v20.window_total, 10 as u32);
    } else {
        expect_true(false);
    }

    // Endpoint 10 gets alternating success / 5xx (3 ok + 3 err = 50% at volume 6)
    expect_true(pool.recordOutcome(10, CircuitOutcomeKind.success, 1_000));
    expect_true(pool.recordOutcome(10, CircuitOutcomeKind.server_error_5xx, 1_100));
    expect_true(pool.recordOutcome(10, CircuitOutcomeKind.success, 1_200));
    expect_true(pool.recordOutcome(10, CircuitOutcomeKind.server_error_5xx, 1_300));
    expect_true(pool.recordOutcome(10, CircuitOutcomeKind.success, 1_400));
    // Before 6th request, still healthy (volume 5 < min_request_volume 6)
    if (v10_pre := pool.getEndpoint(10)) {
        expect_eq(v10_pre.state, CircuitEndpointState.healthy);
    } else {
        expect_true(false);
    }

    // 6th request is 5xx -> 3/6 = 5000 bps >= 5000 -> ejected!
    expect_true(pool.recordOutcome(10, CircuitOutcomeKind.server_error_5xx, 1_500));
    if (v10_post := pool.getEndpoint(10)) {
        expect_eq(v10_post.state, CircuitEndpointState.ejected_open);
        expect_eq(v10_post.error_rate_bps, 5_000 as u32);
    } else {
        expect_true(false);
    }

    // Verify sliding window expiration on endpoint 20 after jumping > 8000ms
    expect_true(pool.recordOutcome(20, CircuitOutcomeKind.success, 20_000));
    if (v20_exp := pool.getEndpoint(20)) {
        expect_eq(v20_exp.window_total, 1 as u32);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 4: Weighted Healthy Endpoint Selection Distribution
// ============================================================================
test("circuitPool: weighted healthy selection honors endpoint weights & dynamic updates", () => {
    let pool = CircuitBreakerPool.init(CircuitPoolConfig.defaults()) ?? return;
    expect_true(pool.addEndpoint(1, 10));
    expect_true(pool.addEndpoint(2, 30));
    expect_true(pool.addEndpoint(3, 60));

    let c1: usize = 0;
    let c2: usize = 0;
    let c3: usize = 0;
    let seed: u64 = 0;
    while (seed < 1000) : (seed += 1) {
        const res = pool.pickHealthyEndpoint(seed, 1_000);
        expect_eq(res.mode, CircuitPickMode.normal_healthy);
        if (res.endpoint_id == 1) {
            c1 += 1;
        } else if (res.endpoint_id == 2) {
            c2 += 1;
        } else if (res.endpoint_id == 3) {
            c3 += 1;
        }
    }
    expect_eq(c1 + c2 + c3, 1000 as usize);
    // Expected ~100, ~300, ~600 with wide statistical tolerance
    expect_true(c1 >= 50 && c1 <= 160);
    expect_true(c2 >= 230 && c2 <= 370);
    expect_true(c3 >= 520 && c3 <= 680);

    // Update weight of endpoint 1 to dominate and remove endpoint 3
    expect_true(pool.updateWeight(1, 90));
    expect_true(pool.removeEndpoint(3));
    expect_eq(pool.endpointCount(), 2 as usize);

    let c1_after: usize = 0;
    seed = 0;
    while (seed < 200) : (seed += 1) {
        const res = pool.pickHealthyEndpoint(seed + 5000, 1_000);
        expect_true(res.endpoint_id == 1 || res.endpoint_id == 2);
        if (res.endpoint_id == 1) {
            c1_after += 1;
        }
    }
    expect_true(c1_after > 120); // 90/(90+30) = 75% -> ~150
});

// ============================================================================
// Test 5: Panic-Mode Fallback Routing When All Endpoints Ejected
// ============================================================================
test("circuitPool: panic-mode fallback routes across all endpoints when 100% ejected", () => {
    const cfg = CircuitPoolConfig{
        consecutive_5xx_threshold: 2,
        error_rate_threshold_bps: 0,
        window_ms: 8_000,
        base_ejection_ms: 30_000,
        max_ejection_ms: 60_000,
        max_ejection_percent_bps: 10_000, // 100% allowed to test panic-mode fallback
    };
    let pool = CircuitBreakerPool.init(cfg) ?? return;
    expect_true(pool.addEndpoint(701, 5));
    expect_true(pool.addEndpoint(702, 5));

    // Eject both endpoints
    expect_true(pool.recordOutcome(701, CircuitOutcomeKind.server_error_5xx, 1_000));
    expect_true(pool.recordOutcome(701, CircuitOutcomeKind.server_error_5xx, 1_100));
    expect_true(pool.recordOutcome(702, CircuitOutcomeKind.timeout, 1_000));
    expect_true(pool.recordOutcome(702, CircuitOutcomeKind.timeout, 1_100));

    const st_before = pool.stats();
    expect_eq(st_before.healthy_count, 0 as usize);
    expect_eq(st_before.ejected_count, 2 as usize);

    // Picking before expiry must trigger panic_fallback instead of failing
    let saw_701: bool = false;
    let saw_702: bool = false;
    let s: u64 = 0;
    while (s < 32) : (s += 1) {
        const pick = pool.pickHealthyEndpoint(s, 5_000);
        expect_eq(pick.mode, CircuitPickMode.panic_fallback);
        expect_eq(pick.state, CircuitEndpointState.ejected_open);
        if (pick.endpoint_id == 701) {
            saw_701 = true;
        }
        if (pick.endpoint_id == 702) {
            saw_702 = true;
        }
    }
    expect_true(saw_701 && saw_702);
    expect_eq(pool.stats().panic_picks, 32 as u64);
});

// ============================================================================
// Test 6: Monotonic Clock Rollback Guard & Manual forceRecover
// ============================================================================
test("circuitPool: monotonic clock rollback guard & manual forceRecover", () => {
    const cfg = CircuitPoolConfig{
        consecutive_5xx_threshold: 2,
        error_rate_threshold_bps: 0,
        window_ms: 8_000,
        base_ejection_ms: 10_000,
        max_ejection_ms: 30_000,
        max_ejection_percent_bps: 10_000,
    };
    let pool = CircuitBreakerPool.init(cfg) ?? return;
    expect_true(pool.addEndpoint(1, 10));
    expect_true(pool.addEndpoint(2, 10));

    // Eject node 1 at t=50_000 -> eject_until_ms = 60_000
    expect_true(pool.recordOutcome(1, CircuitOutcomeKind.server_error_5xx, 50_000));
    expect_true(pool.recordOutcome(1, CircuitOutcomeKind.server_error_5xx, 50_000));

    // Clock rollback to 10_000 must NOT corrupt state or prematurely expire
    expect_eq(pool.advanceClock(10_000), 0 as usize);
    if (v := pool.getEndpoint(1)) {
        expect_eq(v.state, CircuitEndpointState.ejected_open);
        expect_eq(v.eject_until_ms, 60_000 as u64);
    } else {
        expect_true(false);
    }

    // Manual forceRecover restores node 1 immediately
    expect_true(pool.forceRecover(1));
    if (v2 := pool.getEndpoint(1)) {
        expect_eq(v2.state, CircuitEndpointState.healthy);
        expect_eq(v2.eject_until_ms, 0 as u64);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 7: Full Boundary Guards, 64-Slot Capacity & cap.net Dynamic Facade
// ============================================================================
test("circuitPool: invalid config, duplicate/unknown guards, 64-slot limit & cap.net facade", () => {
    // Invalid configs must return null
    expect_eq(CircuitBreakerPool.init(CircuitPoolConfig{ consecutive_5xx_threshold: 0 }) == null, true);
    expect_eq(CircuitBreakerPool.init(CircuitPoolConfig{ error_rate_threshold_bps: 10_001 }) == null, true);
    expect_eq(CircuitBreakerPool.init(CircuitPoolConfig{ max_ejection_percent_bps: 10_001 }) == null, true);
    expect_eq(CircuitBreakerPool.init(CircuitPoolConfig{ window_ms: 4 }) == null, true);
    expect_eq(CircuitBreakerPool.init(CircuitPoolConfig{ base_ejection_ms: 0 }) == null, true);
    expect_eq(CircuitBreakerPool.init(CircuitPoolConfig{ base_ejection_ms: 50_000, max_ejection_ms: 10_000 }) == null, true);
    expect_eq(CircuitBreakerPool.init(CircuitPoolConfig{ half_open_success_threshold: 0 }) == null, true);

    // Construct via dynamic cap.net facade
    let pool = cap.net.CircuitBreakerPool.init(CircuitPoolConfig.defaults()) ?? return;

    // Empty pool pick returns empty_pool
    expect_eq(pool.pickHealthyEndpoint(1, 1_000).mode, CircuitPickMode.empty_pool);

    // Zero weight rejected
    expect_eq(pool.addEndpoint(1, 0), false);

    // Unknown endpoint guards
    expect_eq(pool.recordOutcome(999, CircuitOutcomeKind.success, 1_000), false);
    expect_eq(pool.removeEndpoint(999), false);
    expect_eq(pool.updateWeight(999, 5), false);
    expect_eq(pool.forceRecover(999), false);
    expect_eq(pool.getEndpoint(999) == null, true);

    // Fill all 64 slots
    let i: u64 = 0;
    while (i < (CIRCUIT_POOL_MAX_ENDPOINTS as u64)) : (i += 1) {
        expect_true(pool.addEndpoint(1000 + i, 1));
    }
    expect_eq(pool.endpointCount(), CIRCUIT_POOL_MAX_ENDPOINTS);

    // Duplicate ID and 65th slot must be rejected
    expect_eq(pool.addEndpoint(1000, 5), false);
    expect_eq(pool.addEndpoint(9999, 1), false);
    expect_eq(pool.updateWeight(1000, 0), false);

    // Removing one frees a slot for reuse
    expect_true(pool.removeEndpoint(1000));
    expect_eq(pool.endpointCount(), CIRCUIT_POOL_MAX_ENDPOINTS - 1);
    expect_true(pool.addEndpoint(9999, 2));
    expect_eq(pool.endpointCount(), CIRCUIT_POOL_MAX_ENDPOINTS);
});
