// Origin: Pure-CP Stdlib Test Suite (lib/agent/provider_pool.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Multi-Model LLM Provider RPM/TPM & 429 Failover Pool:
//   1. Priority & cost tie-breaking primary selection ((priority asc, cost asc, slot asc))
//   2. Dual-dimension RPM and TPM exhaustion automatic failover (used_failover = true)
//   3. Upstream 429 Retry-After cooldown isolation & automatic expiration recovery
//   4. reconcile_actual_tokens refund (unlocking TPM), extra charge & underflow clamp guard
//   5. Rate window expiration automatic quota refresh
//   6. All-endpoints-unavailable exact earliest_retry_ms computation across cooldown & window limits
//   7. set_enabled online drain/restore & out-of-range slot guards
//   8. Invalid spec, duplicate ID & full 16-endpoint pool capacity boundary guards

import { test, expect_eq } from "cap:test";
import {
    PROVIDER_POOL_MAX_ENDPOINTS,
    PROVIDER_ID_MAX_LEN,
    ProviderStatus,
    ProviderEndpointSpec,
    ProviderLeaseResult,
    ProviderPoolStats,
    LlmProviderPool,
} from "cap:agent";

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Priority & Cost Tie-Breaking Primary Selection
// ============================================================================
test("provider_pool: priority and cost tie-breaking primary selection", () => {
    expect_eq(PROVIDER_POOL_MAX_ENDPOINTS, 16);
    expect_eq(PROVIDER_ID_MAX_LEN, 48);

    let pool = LlmProviderPool.init();
    const p_backup = ProviderEndpointSpec.create("backup-p2", 2, 100, 100_000, 60_000, 500) ?? return;
    const p_exp = ProviderEndpointSpec.create("primary-expensive", 1, 100, 100_000, 60_000, 4_000) ?? return;
    const p_cheap = ProviderEndpointSpec.create("primary-cheap", 1, 100, 100_000, 60_000, 2_000) ?? return;

    expect_eq(pool.register_endpoint(p_backup), true);
    expect_eq(pool.register_endpoint(p_exp), true);
    expect_eq(pool.register_endpoint(p_cheap), true);

    // Must select priority=1 with lowest cost (primary-cheap at slot 2)
    const lease: ProviderLeaseResult = pool.acquire(1_000, 1_000);
    expect_eq(lease.acquired, true);
    expect_eq(lease.slot_idx, 2);
    expect_eq(eqBytes(lease.idSlice(), "primary-cheap"), true);
    expect_eq(lease.used_failover, false);
    expect_eq(lease.earliest_retry_ms, 0);

    const st: ProviderPoolStats = pool.stats(1_000);
    expect_eq(st.total_endpoints, 3);
    expect_eq(st.available_endpoints, 3);
    expect_eq(st.total_leases, 1);
    expect_eq(st.total_failovers, 0);
});

// ============================================================================
// Test 2: RPM & TPM Exhaustion Automatic Failover
// ============================================================================
test("provider_pool: RPM and TPM exhaustion triggers automatic failover", () => {
    let pool = LlmProviderPool.init();

    // Primary: rpm=2, tpm=10_000
    const p1 = ProviderEndpointSpec.create("ep-primary", 1, 2, 10_000, 60_000, 1_000) ?? return;
    // Secondary: rpm=10, tpm=4_000
    const p2 = ProviderEndpointSpec.create("ep-secondary", 2, 10, 4_000, 60_000, 2_000) ?? return;
    // Tertiary: rpm=10, tpm=50_000
    const p3 = ProviderEndpointSpec.create("ep-tertiary", 3, 10, 50_000, 60_000, 3_000) ?? return;

    expect_eq(pool.register_endpoint(p1), true);
    expect_eq(pool.register_endpoint(p2), true);
    expect_eq(pool.register_endpoint(p3), true);

    // 2 requests exhaust p1's RPM (2/2)
    const l1 = pool.acquire(1_000, 100);
    expect_eq(l1.acquired, true);
    expect_eq(l1.slot_idx, 0);
    expect_eq(l1.used_failover, false);

    const l2 = pool.acquire(1_000, 200);
    expect_eq(l2.acquired, true);
    expect_eq(l2.slot_idx, 0);
    expect_eq(l2.used_failover, false);

    // 3rd request (3_000 tokens): p1 RPM exhausted -> fails over to p2 (slot 1)
    const l3 = pool.acquire(3_000, 300);
    expect_eq(l3.acquired, true);
    expect_eq(l3.slot_idx, 1);
    expect_eq(eqBytes(l3.idSlice(), "ep-secondary"), true);
    expect_eq(l3.used_failover, true);

    // 4th request (2_000 tokens): p1 RPM full, p2 TPM would be 3000+2000 > 4000 -> fails over to p3 (slot 2)
    const l4 = pool.acquire(2_000, 400);
    expect_eq(l4.acquired, true);
    expect_eq(l4.slot_idx, 2);
    expect_eq(eqBytes(l4.idSlice(), "ep-tertiary"), true);
    expect_eq(l4.used_failover, true);

    const st = pool.stats(400);
    expect_eq(st.total_leases, 4);
    expect_eq(st.total_failovers, 2);
});

// ============================================================================
// Test 3: 429 Retry-After Cooldown Isolation & Expiration Recovery
// ============================================================================
test("provider_pool: 429 Retry-After cooldown failover & automatic recovery", () => {
    let pool = LlmProviderPool.init();
    const p1 = ProviderEndpointSpec.create("openai-main", 1, 100, 100_000, 60_000, 1_000) ?? return;
    const p2 = ProviderEndpointSpec.create("openai-fallback", 2, 100, 100_000, 60_000, 2_000) ?? return;
    expect_eq(pool.register_endpoint(p1), true);
    expect_eq(pool.register_endpoint(p2), true);

    // Upstream returns 429 Retry-After: 5000ms at t=1000 -> cooldown until t=6000
    expect_eq(pool.report_rate_limit_429(0, 5_000, 1_000), true);
    expect_eq(pool.endpoint_status(0, 1_500) == ProviderStatus.cooldown_429, true);
    expect_eq(pool.stats(1_500).cooldown_endpoints, 1);
    expect_eq(pool.stats(1_500).available_endpoints, 1);

    // At t=5999, still in cooldown -> fails over to fallback
    const l1 = pool.acquire(1_000, 5_999);
    expect_eq(l1.acquired, true);
    expect_eq(l1.slot_idx, 1);
    expect_eq(l1.used_failover, true);

    // At t=6000, cooldown expired -> primary automatically recovers
    const l2 = pool.acquire(1_000, 6_000);
    expect_eq(l2.acquired, true);
    expect_eq(l2.slot_idx, 0);
    expect_eq(l2.used_failover, false);
    expect_eq(pool.stats(6_000).cooldown_endpoints, 0);
});

// ============================================================================
// Test 4: reconcile_actual_tokens Refund, Surcharge & Underflow Protection
// ============================================================================
test("provider_pool: reconcile_actual_tokens refund, surcharge & underflow clamp", () => {
    let pool = LlmProviderPool.init();
    const p1 = ProviderEndpointSpec.create("primary-tpm", 1, 10, 5_000, 60_000, 1_000) ?? return;
    expect_eq(pool.register_endpoint(p1), true);

    // Reserve 4_500 / 5_000 tokens
    const l1 = pool.acquire(4_500, 1_000);
    expect_eq(l1.acquired, true);

    // Another 2_000-token request is currently rejected (4500+2000 > 5000)
    expect_eq(pool.acquire(2_000, 1_100).acquired, false);

    // Actual response only used 1_500 tokens -> refund 3_000 (window_tokens drops to 1_500)
    expect_eq(pool.reconcile_actual_tokens(0, 4_500, 1_500), true);

    // Now the 2_000-token request succeeds (1500+2000 = 3500 <= 5000)!
    const l2 = pool.acquire(2_000, 1_200);
    expect_eq(l2.acquired, true);

    // Surcharge: actual was 3_500 instead of 2_000 -> +1_500 => window_tokens = 5_000 (full!)
    expect_eq(pool.reconcile_actual_tokens(0, 2_000, 3_500), true);
    expect_eq(pool.acquire(1, 1_300).acquired, false);

    // Massive refund larger than window_tokens must clamp safely to 0 without underflow
    expect_eq(pool.reconcile_actual_tokens(0, 100_000, 0), true);
    expect_eq(pool.acquire(5_000, 1_400).acquired, true);

    // Out-of-bounds slot rejected
    expect_eq(pool.reconcile_actual_tokens(9, 100, 50), false);
});

// ============================================================================
// Test 5: Rate Window Expiration Automatic Quota Refresh
// ============================================================================
test("provider_pool: expired window automatically refreshes RPM and TPM quotas", () => {
    let pool = LlmProviderPool.init();
    const p1 = ProviderEndpointSpec.create("short-win", 1, 1, 2_000, 1_000, 1_000) ?? return;
    expect_eq(pool.register_endpoint(p1), true);

    // Exhaust window [1000, 2000)
    expect_eq(pool.acquire(2_000, 1_000).acquired, true);
    expect_eq(pool.endpoint_status(0, 1_500) == ProviderStatus.rate_limited, true);
    expect_eq(pool.acquire(100, 1_999).acquired, false);

    // At t=2000 (elapsed 1000 >= window_ms), window refreshes cleanly
    expect_eq(pool.endpoint_status(0, 2_000) == ProviderStatus.available, true);
    const l2 = pool.acquire(2_000, 2_000);
    expect_eq(l2.acquired, true);
    expect_eq(l2.slot_idx, 0);
});

// ============================================================================
// Test 6: All-Unavailable Exact earliest_retry_ms Calculation
// ============================================================================
test("provider_pool: all endpoints unavailable computes exact earliest_retry_ms", () => {
    let pool = LlmProviderPool.init();
    const p1 = ProviderEndpointSpec.create("ep-cooldown", 1, 10, 10_000, 10_000, 1_000) ?? return;
    const p2 = ProviderEndpointSpec.create("ep-rpm-full", 2, 1, 10_000, 2_000, 2_000) ?? return;
    expect_eq(pool.register_endpoint(p1), true);
    expect_eq(pool.register_endpoint(p2), true);

    // Initialize windows at t=1000; put p1 in 429 cooldown until t=4000; exhaust p2's 1 RPM (window ends t=3000)
    expect_eq(pool.report_rate_limit_429(0, 3_000, 1_000), true); // ready at 4000
    const l_p2 = pool.acquire(500, 1_000);                         // p2 used, window [1000, 3000)
    expect_eq(l_p2.acquired, true);
    expect_eq(l_p2.slot_idx, 1);

    // At t=1500: p1 needs 2500ms (4000-1500), p2 needs 1500ms (3000-1500) -> earliest_retry_ms == 1500!
    const denied = pool.acquire(500, 1_500);
    expect_eq(denied.acquired, false);
    expect_eq(denied.earliest_retry_ms, 1_500);
    expect_eq(pool.stats(1_500).total_rejections, 1);

    // At t=3000 (1500 + 1500), p2 is ready again!
    const recovered = pool.acquire(500, 3_000);
    expect_eq(recovered.acquired, true);
    expect_eq(recovered.slot_idx, 1);
});

// ============================================================================
// Test 7: set_enabled Online Drain & Restore
// ============================================================================
test("provider_pool: set_enabled drains and restores endpoint traffic", () => {
    let pool = LlmProviderPool.init();
    const p1 = ProviderEndpointSpec.create("node-a", 1, 10, 10_000, 60_000, 1_000) ?? return;
    const p2 = ProviderEndpointSpec.create("node-b", 2, 10, 10_000, 60_000, 2_000) ?? return;
    expect_eq(pool.register_endpoint(p1), true);
    expect_eq(pool.register_endpoint(p2), true);

    // Disable node-a -> node-b becomes primary enabled endpoint (used_failover == false)
    expect_eq(pool.set_enabled(0, false), true);
    expect_eq(pool.endpoint_status(0, 1_000) == ProviderStatus.disabled, true);
    const l1 = pool.acquire(100, 1_000);
    expect_eq(l1.acquired, true);
    expect_eq(l1.slot_idx, 1);
    expect_eq(l1.used_failover, false);

    // Disable node-b too -> all disabled -> rejected
    expect_eq(pool.set_enabled(1, false), true);
    expect_eq(pool.acquire(100, 1_100).acquired, false);

    // Re-enable node-a -> serves traffic again
    expect_eq(pool.set_enabled(0, true), true);
    const l2 = pool.acquire(100, 1_200);
    expect_eq(l2.acquired, true);
    expect_eq(l2.slot_idx, 0);

    // Out-of-bounds slot rejected
    expect_eq(pool.set_enabled(5, false), false);
    expect_eq(pool.report_rate_limit_429(5, 1_000, 1_200), false);
});

// ============================================================================
// Test 8: Invalid Spec, Duplicate ID & Full 16-Endpoint Pool Boundary Guards
// ============================================================================
test("provider_pool: invalid spec, duplicate ID & 16-slot capacity boundary guards", () => {
    // Invalid specs rejected by ProviderEndpointSpec.create
    expect_eq(ProviderEndpointSpec.create("", 1, 10, 1000, 1000, 100) == null, true);
    expect_eq(ProviderEndpointSpec.create("a", 1, 0, 1000, 1000, 100) == null, true);
    expect_eq(ProviderEndpointSpec.create("a", 1, 10, 0, 1000, 100) == null, true);
    expect_eq(ProviderEndpointSpec.create("a", 1, 10, 1000, 0, 100) == null, true);

    let pool = LlmProviderPool.init();
    // Invalid manual spec rejected by register_endpoint
    expect_eq(pool.register_endpoint(ProviderEndpointSpec{}), false);

    // Duplicate ID rejected
    const s0 = ProviderEndpointSpec.create("ep-00", 1, 10, 10_000, 60_000, 1_000) ?? return;
    expect_eq(pool.register_endpoint(s0), true);
    expect_eq(pool.register_endpoint(s0), false);

    // Fill remaining 15 slots up to PROVIDER_POOL_MAX_ENDPOINTS (16)
    let i: usize = 1;
    while (i < PROVIDER_POOL_MAX_ENDPOINTS) : (i += 1) {
        let id_arr: [5]u8 = [101, 112, 45, 48, 48]; // "ep-00"
        id_arr[3] = @intCast(48 + (i / 10));
        id_arr[4] = @intCast(48 + (i % 10));
        const spec = ProviderEndpointSpec.create(id_arr[0..], @intCast(i + 1), 10, 10_000, 60_000, 1_000) ?? return;
        expect_eq(pool.register_endpoint(spec), true);
    }
    expect_eq(pool.stats(1_000).total_endpoints, 16);

    // 17th endpoint rejected cleanly
    const extra = ProviderEndpointSpec.create("ep-overflow", 99, 10, 10_000, 60_000, 1_000) ?? return;
    expect_eq(pool.register_endpoint(extra), false);
    expect_eq(pool.stats(1_000).total_endpoints, 16);
});
