// Origin: Pure-CP Stdlib Test Suite (lib/agent/tool_governor.cp) | Created: 2026-11-22
// Production Deep Verification Suite for AI Agent Tool Execution Governor:
//   1. Concurrency semaphore enforcement (`max_concurrency`) & lease release recovery
//   2. Short-window duplicate argument suppression (`dedupe_window_ms`) & expiry recovery
//   3. Sliding-window rate limiting (`max_calls_per_window`) & window slide recovery
//   4. Window micro-USD cost cap (`max_cost_micro_usd_per_window`) & actual-cost reconciliation on release
//   5. Circuit breaker full lifecycle: closed -> open -> cooldown -> half_open single probe -> re-trip -> closed
//   6. Double-release guard, invalid lease guard & monotonic clock-rollback protection
//   7. Invalid policy guards, duplicate tool registration, full 32-tool table & 64-entry dedupe ring wrap
//   8. Full `ToolGovernorStats` accounting invariant across all decision branches

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TOOL_GOV_MAX_TOOLS,
    TOOL_GOV_MAX_WINDOW_EVENTS,
    TOOL_GOV_MAX_DEDUPE_ENTRIES,
    TOOL_GOV_MAX_ACTIVE_LEASES,
    TOOL_GOV_INVALID_LEASE,
    ToolBreakerState,
    ToolGovDecisionKind,
    ToolReleaseStatus,
    ToolGovPolicy,
    ToolLeaseHandle,
    ToolAcquireDecision,
    ToolGovToolSnapshot,
    ToolGovernorStats,
    AgentToolGovernor,
} from "cap:agent";

// ============================================================================
// Test 1: Concurrency Semaphore Enforcement & Release Recovery
// ============================================================================

fn testConcurrencySemaphoreAndRelease()! {
    let gov = cap.agent.AgentToolGovernor.init();
    expect_eq(gov.registerTool(ToolGovPolicy{
        tool_id: 1,
        max_concurrency: 2,
        max_calls_per_window: 10,
        window_ms: 60_000,
        dedupe_window_ms: 0,
        failure_trip_threshold: 3,
        cooldown_ms: 10_000,
        max_cost_micro_usd_per_window: 0,
    }), true);

    const a1: ToolAcquireDecision = gov.acquireToolCall(1, 101, 0, 1000);
    const a2: ToolAcquireDecision = gov.acquireToolCall(1, 102, 0, 1001);
    const a3: ToolAcquireDecision = gov.acquireToolCall(1, 103, 0, 1002);

    expect_eq(a1.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(a1.lease.isValid(), true);
    expect_eq(a2.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(a2.active_concurrency, 2);
    expect_eq(a3.kind == ToolGovDecisionKind.reject_concurrency_limit, true);
    expect_eq(a3.lease.isValid(), false);

    // Release one lease -> concurrency drops to 1 and next acquire succeeds
    expect_eq(gov.releaseToolCall(a1.lease, true, 0, 1050) == ToolReleaseStatus.ok, true);
    const a4 = gov.acquireToolCall(1, 104, 0, 1051);
    expect_eq(a4.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(a4.active_concurrency, 2);

    expect_eq(gov.releaseToolCall(a2.lease, true, 0, 1060) == ToolReleaseStatus.ok, true);
    expect_eq(gov.releaseToolCall(a4.lease, true, 0, 1061) == ToolReleaseStatus.ok, true);
    expect_eq(gov.stats().active_leases, 0);
}

// ============================================================================
// Test 2: Short-Window Duplicate Argument Suppression & Expiry
// ============================================================================

fn testDuplicateArgumentDedupeWindow()! {
    let gov = AgentToolGovernor.init();
    expect_eq(gov.registerTool(ToolGovPolicy{
        tool_id: 2,
        max_concurrency: 4,
        max_calls_per_window: 10,
        window_ms: 60_000,
        dedupe_window_ms: 5_000,
        failure_trip_threshold: 3,
        cooldown_ms: 10_000,
        max_cost_micro_usd_per_window: 0,
    }), true);

    const d1 = gov.acquireToolCall(2, 0xAAAA, 0, 1_000);
    expect_eq(d1.kind == ToolGovDecisionKind.allowed, true);
    * = gov.releaseToolCall(d1.lease, true, 0, 1_100);

    // Same args_hash within 5000ms window -> rejected as duplicate
    const d2 = gov.acquireToolCall(2, 0xAAAA, 0, 3_000);
    expect_eq(d2.kind == ToolGovDecisionKind.reject_duplicate_call, true);

    // Different args_hash -> allowed
    const d3 = gov.acquireToolCall(2, 0xBBBB, 0, 3_100);
    expect_eq(d3.kind == ToolGovDecisionKind.allowed, true);
    * = gov.releaseToolCall(d3.lease, true, 0, 3_200);

    // After dedupe window expires (t = 6_000 >= 1_000 + 5_000) -> 0xAAAA allowed again
    const d4 = gov.acquireToolCall(2, 0xAAAA, 0, 6_000);
    expect_eq(d4.kind == ToolGovDecisionKind.allowed, true);
    * = gov.releaseToolCall(d4.lease, true, 0, 6_100);
}

// ============================================================================
// Test 3: Sliding-Window Rate Limiting & Window Slide Recovery
// ============================================================================

fn testSlidingWindowRateLimitAndRecovery()! {
    let gov = AgentToolGovernor.init();
    expect_eq(gov.registerTool(ToolGovPolicy{
        tool_id: 3,
        max_concurrency: 4,
        max_calls_per_window: 3,
        window_ms: 10_000,
        dedupe_window_ms: 0,
        failure_trip_threshold: 0,
        cooldown_ms: 0,
        max_cost_micro_usd_per_window: 0,
    }), true);

    const r1 = gov.acquireToolCall(3, 1, 0, 1_000);
    * = gov.releaseToolCall(r1.lease, true, 0, 1_050);
    const r2 = gov.acquireToolCall(3, 2, 0, 2_000);
    * = gov.releaseToolCall(r2.lease, true, 0, 2_050);
    const r3 = gov.acquireToolCall(3, 3, 0, 3_000);
    * = gov.releaseToolCall(r3.lease, true, 0, 3_050);

    // 4th call at t=4_000 hits rate limit
    const r4 = gov.acquireToolCall(3, 4, 0, 4_000);
    expect_eq(r4.kind == ToolGovDecisionKind.reject_rate_limit, true);
    expect_eq(r4.window_calls_used, 3);

    // At t=11_000, the t=1_000 call slides out of the 10_000ms window -> allowed!
    const r5 = gov.acquireToolCall(3, 5, 0, 11_000);
    expect_eq(r5.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(r5.window_calls_used, 3);
    * = gov.releaseToolCall(r5.lease, true, 0, 11_050);
}

// ============================================================================
// Test 4: Window Cost Cap & Actual-Cost Reconciliation on Release
// ============================================================================

fn testWindowCostCapAndActualCostReconciliation()! {
    let gov = AgentToolGovernor.init();
    expect_eq(gov.registerTool(ToolGovPolicy{
        tool_id: 4,
        max_concurrency: 4,
        max_calls_per_window: 10,
        window_ms: 60_000,
        dedupe_window_ms: 0,
        failure_trip_threshold: 0,
        cooldown_ms: 0,
        max_cost_micro_usd_per_window: 1_000,
    }), true);

    // Estimated cost 800 admitted
    const c1 = gov.acquireToolCall(4, 1, 800, 1_000);
    expect_eq(c1.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(c1.window_cost_used_micro_usd, 800);

    // Next call est=300 exceeds 1_000 cap -> rejected
    const c2 = gov.acquireToolCall(4, 2, 300, 1_100);
    expect_eq(c2.kind == ToolGovDecisionKind.reject_cost_limit, true);

    // Release c1 with much smaller actual cost = 200 -> reconciles window cost down to 200
    expect_eq(gov.releaseToolCall(c1.lease, true, 200, 1_200) == ToolReleaseStatus.ok, true);
    if (snap := gov.getToolSnapshot(4, 1_200)) {
        expect_eq(snap.window_cost_used_micro_usd, 200);
    } else {
        expect_eq(true, false);
    }

    // Now est=700 fits cleanly within 1_000 (200 + 700 == 900)
    const c3 = gov.acquireToolCall(4, 3, 700, 1_300);
    expect_eq(c3.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(c3.window_cost_used_micro_usd, 900);
    * = gov.releaseToolCall(c3.lease, true, 700, 1_400);
}

// ============================================================================
// Test 5: Circuit Breaker Full Lifecycle (closed -> open -> half_open -> closed)
// ============================================================================

fn testCircuitBreakerFullLifecycle()! {
    let gov = AgentToolGovernor.init();
    expect_eq(gov.registerTool(ToolGovPolicy{
        tool_id: 5,
        max_concurrency: 4,
        max_calls_per_window: 20,
        window_ms: 60_000,
        dedupe_window_ms: 0,
        failure_trip_threshold: 2,
        cooldown_ms: 5_000,
        max_cost_micro_usd_per_window: 0,
    }), true);

    // 2 consecutive failures trip breaker open until t = 1_200 + 5_000 = 6_200
    const f1 = gov.acquireToolCall(5, 1, 0, 1_000);
    * = gov.releaseToolCall(f1.lease, false, 0, 1_100);
    const f2 = gov.acquireToolCall(5, 2, 0, 1_150);
    * = gov.releaseToolCall(f2.lease, false, 0, 1_200);

    // During cooldown: rejected with reject_circuit_open
    const blocked = gov.acquireToolCall(5, 3, 0, 3_000);
    expect_eq(blocked.kind == ToolGovDecisionKind.reject_circuit_open, true);
    expect_eq(blocked.breaker_state == ToolBreakerState.open, true);

    // Cooldown elapsed at t = 6_200: first call transitions to half_open probe
    const probe1 = gov.acquireToolCall(5, 4, 0, 6_200);
    expect_eq(probe1.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(probe1.lease.is_probe, true);
    expect_eq(probe1.breaker_state == ToolBreakerState.half_open, true);

    // While probe1 is in-flight, concurrent call is rejected (only 1 probe allowed)
    const blocked2 = gov.acquireToolCall(5, 5, 0, 6_250);
    expect_eq(blocked2.kind == ToolGovDecisionKind.reject_circuit_open, true);

    // Probe1 fails -> immediately re-trips open until t = 6_300 + 5_000 = 11_300
    * = gov.releaseToolCall(probe1.lease, false, 0, 6_300);
    expect_eq(gov.acquireToolCall(5, 6, 0, 10_000).kind == ToolGovDecisionKind.reject_circuit_open, true);

    // Second cooldown expires -> probe2 succeeds -> breaker recovers to closed!
    const probe2 = gov.acquireToolCall(5, 7, 0, 11_300);
    expect_eq(probe2.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(probe2.lease.is_probe, true);
    * = gov.releaseToolCall(probe2.lease, true, 0, 11_400);

    if (snap := gov.getToolSnapshot(5, 11_500)) {
        expect_eq(snap.breaker_state == ToolBreakerState.closed, true);
        expect_eq(snap.consecutive_failures, 0);
        expect_eq(snap.total_trips, 2);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 6: Double-Release Guard, Invalid Lease Guard & Clock-Rollback Protection
// ============================================================================

fn testDoubleReleaseInvalidLeaseAndClockRollback()! {
    let gov = AgentToolGovernor.init();
    expect_eq(gov.registerTool(ToolGovPolicy.defaults(6)), true);

    const a1 = gov.acquireToolCall(6, 42, 0, 10_000);
    expect_eq(a1.kind == ToolGovDecisionKind.allowed, true);
    expect_eq(gov.releaseToolCall(a1.lease, true, 0, 10_100) == ToolReleaseStatus.ok, true);

    // Double release rejected without corrupting counters
    expect_eq(gov.releaseToolCall(a1.lease, true, 0, 10_200) == ToolReleaseStatus.already_released, true);

    // Invalid & OOB lease handles rejected
    const empty_lease = ToolLeaseHandle{};
    expect_eq(gov.releaseToolCall(empty_lease, true, 0, 10_200) == ToolReleaseStatus.invalid_lease, true);
    const bad_slot = ToolLeaseHandle{ lease_id: 99, tool_id: 6, slot_idx: 999, is_probe: false };
    expect_eq(gov.releaseToolCall(bad_slot, true, 0, 10_200) == ToolReleaseStatus.invalid_lease, true);

    // Clock rollback (now=5_000 < last=10_200) must not bypass dedupe window for args_hash=42
    const dup_rollback = gov.acquireToolCall(6, 42, 0, 5_000);
    expect_eq(dup_rollback.kind == ToolGovDecisionKind.reject_duplicate_call, true);
    expect_eq(gov.stats().duplicate_release_rejects, 3);
}

// ============================================================================
// Test 7: Invalid Policy, Full 32-Tool Table & 64-Entry Dedupe Ring Wrap
// ============================================================================

fn testInvalidPoliciesAndCapacityGuards()! {
    let gov = AgentToolGovernor.init();

    // Invalid policies rejected
    expect_eq(gov.registerTool(ToolGovPolicy{ tool_id: 1, max_concurrency: 0 }), false);
    expect_eq(gov.registerTool(ToolGovPolicy{ tool_id: 1, max_calls_per_window: 0 }), false);
    expect_eq(gov.registerTool(ToolGovPolicy{ tool_id: 1, max_calls_per_window: (TOOL_GOV_MAX_WINDOW_EVENTS as u32) + 1 }), false);
    expect_eq(gov.registerTool(ToolGovPolicy{ tool_id: 1, window_ms: 0 }), false);
    expect_eq(gov.registerTool(ToolGovPolicy{ tool_id: 1, failure_trip_threshold: 2, cooldown_ms: 0 }), false);

    // Register 32 tools -> 33rd and duplicates rejected
    let t: usize = 0;
    while (t < TOOL_GOV_MAX_TOOLS) : (t += 1) {
        expect_eq(gov.registerTool(ToolGovPolicy{
            tool_id: (t as u32) + 1,
            max_concurrency: 4,
            max_calls_per_window: 64,
            window_ms: 60_000,
            dedupe_window_ms: 5_000,
            failure_trip_threshold: 0,
            cooldown_ms: 0,
            max_cost_micro_usd_per_window: 0,
        }), true);
    }
    expect_eq(gov.registerTool(ToolGovPolicy.defaults(1)), false);
    expect_eq(gov.registerTool(ToolGovPolicy.defaults(999)), false);

    // Wrap the 64-entry dedupe ring with 70 distinct calls across two tools
    let k: usize = 0;
    while (k < 70) : (k += 1) {
        const tid: u32 = if (k < 40) 1 else 2;
        const res = gov.acquireToolCall(tid, (k as u64) + 1000, 0, 1_000);
        expect_eq(res.kind == ToolGovDecisionKind.allowed, true);
        * = gov.releaseToolCall(res.lease, true, 0, 1_001);
    }
    // Most recent entry (k=69, tool=2, args=1069) is still deduped
    expect_eq(gov.acquireToolCall(2, 1069, 0, 1_002).kind == ToolGovDecisionKind.reject_duplicate_call, true);

    expect_eq(TOOL_GOV_MAX_TOOLS, 32);
    expect_eq(TOOL_GOV_MAX_WINDOW_EVENTS, 64);
    expect_eq(TOOL_GOV_MAX_DEDUPE_ENTRIES, 64);
    expect_eq(TOOL_GOV_MAX_ACTIVE_LEASES, 64);
    expect_eq(TOOL_GOV_INVALID_LEASE, 0);
}

// ============================================================================
// Test 8: Full `ToolGovernorStats` Accounting Invariant
// ============================================================================

fn testGlobalStatsAccountingInvariant()! {
    let gov = AgentToolGovernor.init();
    expect_eq(gov.registerTool(ToolGovPolicy{
        tool_id: 10,
        max_concurrency: 1,
        max_calls_per_window: 2,
        window_ms: 60_000,
        dedupe_window_ms: 5_000,
        failure_trip_threshold: 1,
        cooldown_ms: 10_000,
        max_cost_micro_usd_per_window: 500,
    }), true);

    * = gov.acquireToolCall(999, 1, 0, 1_000);                       // reject_unknown_tool
    const a1 = gov.acquireToolCall(10, 11, 100, 1_000);              // allowed
    * = gov.acquireToolCall(10, 11, 100, 1_001);                     // reject_duplicate_call
    * = gov.acquireToolCall(10, 12, 100, 1_002);                     // reject_concurrency_limit
    * = gov.releaseToolCall(a1.lease, true, 100, 1_010);
    * = gov.acquireToolCall(10, 13, 500, 1_020);                     // reject_cost_limit (100+500 > 500)
    const a2 = gov.acquireToolCall(10, 14, 100, 1_030);              // allowed (2/2 window calls)
    * = gov.releaseToolCall(a2.lease, true, 100, 1_040);
    * = gov.acquireToolCall(10, 15, 100, 1_050);                     // reject_rate_limit

    const st: ToolGovernorStats = gov.stats();
    expect_eq(st.registered_tools, 1);
    expect_eq(st.total_allowed, 2);
    expect_eq(st.total_releases, 2);
    expect_eq(st.rejected_unknown, 1);
    expect_eq(st.rejected_duplicate, 1);
    expect_eq(st.rejected_concurrency, 1);
    expect_eq(st.rejected_cost_limit, 1);
    expect_eq(st.rejected_rate_limit, 1);
    expect_eq(
        st.total_acquires,
        st.total_allowed + st.rejected_unknown + st.rejected_circuit_open +
            st.rejected_duplicate + st.rejected_concurrency + st.rejected_rate_limit + st.rejected_cost_limit,
    );
}

// ============================================================================
// Suite Registration
// ============================================================================

test("agent.tool_governor: concurrency semaphore enforcement & release recovery", testConcurrencySemaphoreAndRelease);
test("agent.tool_governor: short-window duplicate argument suppression & expiry", testDuplicateArgumentDedupeWindow);
test("agent.tool_governor: sliding-window rate limit & window slide recovery", testSlidingWindowRateLimitAndRecovery);
test("agent.tool_governor: window cost cap & actual-cost reconciliation on release", testWindowCostCapAndActualCostReconciliation);
test("agent.tool_governor: circuit breaker closed -> open -> half_open -> closed lifecycle", testCircuitBreakerFullLifecycle);
test("agent.tool_governor: double-release, invalid lease & clock-rollback guards", testDoubleReleaseInvalidLeaseAndClockRollback);
test("agent.tool_governor: invalid policies, full 32-tool table & 64-entry dedupe ring", testInvalidPoliciesAndCapacityGuards);
test("agent.tool_governor: full ToolGovernorStats accounting invariant", testGlobalStatsAccountingInvariant);
