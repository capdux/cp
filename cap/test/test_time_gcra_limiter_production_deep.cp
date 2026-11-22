// Origin: Pure-CP Stdlib Test Suite (lib/time/gcra_limiter.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Multi-Tenant GCRA Smooth Burst Rate Limiter:
//   1. Exact burst consumption, remaining_burst/reset_after_ms & exact retry_after_ms admission
//   2. Weighted cost > 1 consumption, infeasible cost > max_cost_per_call & zero-cost probe
//   3. Read-only peek (zero side effects) & resetTenant / resetAll lifecycle
//   4. Multi-tenant isolation & oldest-TAT eviction under full capacity
//   5. Clock rollback safety (no rate-limit bypass, no permanent tenant lockout)
//   6. Invalid GcraConfig (zero/negative/overflow) & empty slot buffer guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    GcraConfig,
    GcraDecision,
    GcraTenantSlot,
    GcraStats,
    GcraLimiter,
} from "cap:time";

// ============================================================================
// Test 1: Exact Burst Consumption & Exact retry_after_ms Admission
// ============================================================================

fn testExactBurstAndRetryAfterAdmission()! {
    // 100ms per unit (10 req/s), burst_capacity = 3, max_cost = 3
    const cfg = cap.time.GcraConfig.init(100, 3, 3) ?? return error.BadCfg;
    expect_eq(cfg.burstWindowMs(), 300);

    let slots: [8]GcraTenantSlot = [GcraTenantSlot{}; 8];
    let slots_s: []GcraTenantSlot = slots[0..];
    let limiter = GcraLimiter.init(slots_s, cfg) ?? return error.InitFailed;

    const k: u64 = 1001;
    // Burst 1, 2, 3 at t=1000 all allowed
    const d1: GcraDecision = limiter.checkAndConsume(k, 1, 1000);
    expect_eq(d1.allowed, true);
    expect_eq(d1.remaining_burst, 2);
    expect_eq(d1.retry_after_ms, 0);
    expect_eq(d1.reset_after_ms, 100);
    expect_eq(d1.tat_ms, 1100);

    const d2 = limiter.checkAndConsume(k, 1, 1000);
    expect_eq(d2.allowed, true);
    expect_eq(d2.remaining_burst, 1);
    expect_eq(d2.reset_after_ms, 200);

    const d3 = limiter.checkAndConsume(k, 1, 1000);
    expect_eq(d3.allowed, true);
    expect_eq(d3.remaining_burst, 0);
    expect_eq(d3.reset_after_ms, 300);
    expect_eq(d3.tat_ms, 1300);

    // 4th request at t=1000 rejected with exact retry_after_ms == 100
    const d4 = limiter.checkAndConsume(k, 1, 1000);
    expect_eq(d4.allowed, false);
    expect_eq(d4.remaining_burst, 0);
    expect_eq(d4.retry_after_ms, 100);
    expect_eq(d4.reset_after_ms, 300);
    expect_eq(d4.tat_ms, 1300);

    // 1ms before retry_after_ms (t=1099): still rejected with retry_after_ms == 1
    const d5 = limiter.checkAndConsume(k, 1, 1099);
    expect_eq(d5.allowed, false);
    expect_eq(d5.retry_after_ms, 1);

    // Exactly at t = 1000 + d4.retry_after_ms (1100): admitted immediately!
    const d6 = limiter.checkAndConsume(k, 1, 1100);
    expect_eq(d6.allowed, true);
    expect_eq(d6.remaining_burst, 0);
    expect_eq(d6.retry_after_ms, 0);
    expect_eq(d6.reset_after_ms, 300);
    expect_eq(d6.tat_ms, 1400);

    // After full reset window (t=1400), bucket is completely refilled (remaining=2 after consuming 1)
    const d7 = limiter.checkAndConsume(k, 1, 1400);
    expect_eq(d7.allowed, true);
    expect_eq(d7.remaining_burst, 2);
    expect_eq(d7.reset_after_ms, 100);
}

// ============================================================================
// Test 2: Weighted Cost > 1, Infeasible Cost & Zero-Cost Probe
// ============================================================================

fn testWeightedCostAndZeroCostProbe()! {
    const cfg = GcraConfig.init(50, 5, 4) ?? return error.BadCfg;
    let slots: [4]GcraTenantSlot = [GcraTenantSlot{}; 4];
    let slots_s: []GcraTenantSlot = slots[0..];
    let limiter = GcraLimiter.init(slots_s, cfg) ?? return error.InitFailed;

    const tenant: u64 = 42;
    // Consume cost = 3 at t=2000 -> remaining = 2, tat = 2150
    const d1 = limiter.checkAndConsume(tenant, 3, 2000);
    expect_eq(d1.allowed, true);
    expect_eq(d1.remaining_burst, 2);
    expect_eq(d1.reset_after_ms, 150);

    // Try cost = 3 again at t=2000 -> needs 1 more unit (50ms)
    const d2 = limiter.checkAndConsume(tenant, 3, 2000);
    expect_eq(d2.allowed, false);
    expect_eq(d2.remaining_burst, 2);
    expect_eq(d2.retry_after_ms, 50);

    // Wait 50ms -> t=2050, cost = 3 now succeeds!
    const d3 = limiter.checkAndConsume(tenant, 3, 2050);
    expect_eq(d3.allowed, true);
    expect_eq(d3.remaining_burst, 0);

    // Cost > max_cost_per_call (5 > 4) is rejected even when bucket is full
    const d_over = limiter.checkAndConsume(tenant, 5, 5000);
    expect_eq(d_over.allowed, false);
    expect_eq(d_over.retry_after_ms, 0);

    // Zero-cost probe succeeds without advancing TAT
    const d_zero = limiter.checkAndConsume(tenant, 0, 5000);
    expect_eq(d_zero.allowed, true);
    expect_eq(d_zero.remaining_burst, 5);
    expect_eq(d_zero.reset_after_ms, 0);
}

// ============================================================================
// Test 3: Read-Only Peek & Tenant Reset
// ============================================================================

fn testPeekNoSideEffectsAndResetTenant()! {
    const cfg = GcraConfig.init(100, 2, 2) ?? return error.BadCfg;
    let slots: [4]GcraTenantSlot = [GcraTenantSlot{}; 4];
    let slots_s: []GcraTenantSlot = slots[0..];
    let limiter = GcraLimiter.init(slots_s, cfg) ?? return error.InitFailed;

    // Peek unseen tenant must not allocate a slot or change stats
    const p1 = limiter.peek(777, 1, 1000);
    expect_eq(p1.allowed, true);
    expect_eq(p1.remaining_burst, 1);
    const st0: GcraStats = limiter.stats();
    expect_eq(st0.active_tenants, 0);
    expect_eq(st0.total_allowed, 0);
    expect_eq(st0.total_rejected, 0);

    // Consume full burst (2 units)
    expect_eq(limiter.checkAndConsume(777, 2, 1000).allowed, true);
    // Peek shows rejection with retry_after_ms = 100 without incrementing total_rejected
    const p2 = limiter.peek(777, 1, 1000);
    expect_eq(p2.allowed, false);
    expect_eq(p2.retry_after_ms, 100);
    expect_eq(limiter.stats().total_rejected, 0);

    // Reset tenant restores full burst immediately
    expect_eq(limiter.resetTenant(777), true);
    expect_eq(limiter.resetTenant(777), false); // already removed
    expect_eq(limiter.stats().active_tenants, 0);
    expect_eq(limiter.checkAndConsume(777, 2, 1000).allowed, true);
}

// ============================================================================
// Test 4: Multi-Tenant Isolation & Oldest-TAT Eviction Under Full Capacity
// ============================================================================

fn testMultiTenantIsolationAndOldestTatEviction()! {
    const cfg = GcraConfig.init(100, 2, 2) ?? return error.BadCfg;
    let slots: [3]GcraTenantSlot = [GcraTenantSlot{}; 3];
    let slots_s: []GcraTenantSlot = slots[0..];
    let limiter = GcraLimiter.init(slots_s, cfg) ?? return error.InitFailed;

    // Fill all 3 slots with distinct TATs:
    //   tenant 10 at t=1000 -> tat = 1100 (oldest)
    //   tenant 20 at t=1500 -> tat = 1700 (exhausted burst)
    //   tenant 30 at t=1600 -> tat = 1800 (exhausted burst)
    expect_eq(limiter.checkAndConsume(10, 1, 1000).allowed, true);
    expect_eq(limiter.checkAndConsume(20, 2, 1500).allowed, true);
    expect_eq(limiter.checkAndConsume(30, 2, 1600).allowed, true);
    expect_eq(limiter.stats().active_tenants, 3);
    expect_eq(limiter.stats().total_evictions, 0);

    // Tenant 20 is throttled at t=1600, while tenant 40 (new) arrives at t=1600:
    // Table is full -> must evict tenant 10 (oldest TAT = 1100), keeping tenants 20 & 30 intact!
    expect_eq(limiter.checkAndConsume(20, 1, 1550).allowed, false);
    expect_eq(limiter.checkAndConsume(40, 1, 1600).allowed, true);

    const st = limiter.stats();
    expect_eq(st.active_tenants, 3);
    expect_eq(st.total_evictions, 1);

    // Tenant 20 & 30 must still retain their rate-limit state at t=1600
    expect_eq(limiter.peek(30, 1, 1600).allowed, false);
    expect_eq(limiter.peek(30, 1, 1600).retry_after_ms, 100);
    // Tenant 10 was evicted -> fresh full burst if queried
    expect_eq(limiter.peek(10, 2, 1600).allowed, true);
}

// ============================================================================
// Test 5: Clock Rollback Safety
// ============================================================================

fn testClockRollbackSafety()! {
    const cfg = GcraConfig.init(100, 3, 3) ?? return error.BadCfg;
    let slots: [4]GcraTenantSlot = [GcraTenantSlot{}; 4];
    let slots_s: []GcraTenantSlot = slots[0..];
    let limiter = GcraLimiter.init(slots_s, cfg) ?? return error.InitFailed;

    const k: u64 = 999;
    // Exhaust burst at t=10000 -> tat = 10300
    expect_eq(limiter.checkAndConsume(k, 3, 10000).allowed, true);

    // Simulate severe backward clock jump to t=5000 (5 seconds back):
    // 1. Must NOT bypass rate limit (rejected, not allowed immediately)
    // 2. Must clamp TAT so retry_after_ms == 100ms (NOT 5100ms lockout!)
    const d_back = limiter.checkAndConsume(k, 1, 5000);
    expect_eq(d_back.allowed, false);
    expect_eq(d_back.retry_after_ms, 100);
    expect_eq(d_back.reset_after_ms, 300);

    // Supports key_hash == 0 safely (occupancy tracked explicitly)
    expect_eq(limiter.checkAndConsume(0, 1, 5000).allowed, true);
    expect_eq(limiter.resetTenant(0), true);
}

// ============================================================================
// Test 6: Invalid Config & Buffer Boundary Guards
// ============================================================================

fn testInvalidConfigAndBufferGuards()! {
    expect_eq(GcraConfig.init(0, 10, 1) == null, true);                    // zero interval
    expect_eq(GcraConfig.init(-10, 10, 1) == null, true);                  // negative interval
    expect_eq(GcraConfig.init(100, 0, 1) == null, true);                   // zero burst
    expect_eq(GcraConfig.init(100, 10, 0) == null, true);                  // zero max_cost
    expect_eq(GcraConfig.init(100, 5, 6) == null, true);                   // max_cost > burst
    expect_eq(GcraConfig.init(std.math.maxInt(i64), 2, 1) == null, true);  // overflow guard

    const ok_cfg = GcraConfig.init(100, 5, 5) ?? return error.BadCfg;
    let slots: [2]GcraTenantSlot = [GcraTenantSlot{}; 2];
    let empty_slots: []GcraTenantSlot = slots[0..0];
    let slots_s: []GcraTenantSlot = slots[0..];
    expect_eq(GcraLimiter.init(empty_slots, ok_cfg) == null, true);

    const bad_cfg = GcraConfig{ emission_interval_ms: 0, burst_capacity: 5, max_cost_per_call: 5 };
    expect_eq(GcraLimiter.init(slots_s, bad_cfg) == null, true);

    let limiter = GcraLimiter.init(slots_s, ok_cfg) ?? return error.InitFailed;
    expect_eq(limiter.checkAndConsume(1, 1, 100).allowed, true);
    limiter.resetAll();
    expect_eq(limiter.stats().active_tenants, 0);
    expect_eq(limiter.stats().total_allowed, 0);
}

test("time gcra_limiter: exact burst consumption & retry_after_ms admission", testExactBurstAndRetryAfterAdmission);
test("time gcra_limiter: weighted cost > 1, infeasible cost & zero-cost probe", testWeightedCostAndZeroCostProbe);
test("time gcra_limiter: read-only peek & tenant reset", testPeekNoSideEffectsAndResetTenant);
test("time gcra_limiter: multi-tenant isolation & oldest-TAT eviction", testMultiTenantIsolationAndOldestTatEviction);
test("time gcra_limiter: clock rollback safety & zero key_hash support", testClockRollbackSafety);
test("time gcra_limiter: invalid config & buffer boundary guards", testInvalidConfigAndBufferGuards);
