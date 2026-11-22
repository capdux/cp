// Origin: Pure-CP Stdlib Test Suite (lib/net/p2c_balancer.cp) | Created: 2026-11-22
// Production Deep Verification Suite for P2C + Peak-EWMA Adaptive Load Balancer:
//   1. Backend registration, duplicate-ID guard, zero-weight guard, 64-slot capacity & removal reuse
//   2. Peak-EWMA low-latency backend preference & spike reaction
//   3. High inflight concurrency avoidance & weight-skewed load scoring
//   4. Consecutive-error passive outlier ejection & max_ejection_percent anti-cascade guard
//   5. Cooldown expiration -> half_open probing -> healthy recovery & failed probe re-ejection
//   6. Panic-Mode fallback routing when 100% of backends are ejected
//   7. Invalid config, out-of-bounds outcome guards & P2cBalancerStats consistency

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_P2C_BACKENDS,
    BackendHealthState,
    P2cBalancerConfig,
    P2cPickDecision,
    P2cBalancerStats,
    P2cLoadBalancer,
} from "cap:net";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: Registration, Deduplication, Capacity (64) & Removal Slot Reuse
// ============================================================================

fn testRegistrationDedupCapacityAndRemove()! {
    let lb = cap.net.P2cLoadBalancer.init(P2cBalancerConfig{}) ?? return error.InitFailed;

    // Empty pool pick returns found=false
    expect_eq(lb.pick_backend(1000, 1).found, false);

    // Reject empty ID and zero weight
    expect_eq(lb.register_backend("", 1) == null, true);
    expect_eq(lb.register_backend("node-0", 0) == null, true);

    const s0 = lb.register_backend("node-00", 2) ?? return error.RegFailed;
    expect_eq(s0, 0);
    // Duplicate ID rejected
    expect_eq(lb.register_backend("node-00", 1) == null, true);

    // Fill remaining 63 slots up to MAX_P2C_BACKENDS (64)
    let id_buf: [16]u8 = [0; 16];
    let i: usize = 1;
    while (i < MAX_P2C_BACKENDS) : (i += 1) {
        id_buf[0] = 110; // 'n'
        id_buf[1] = (48 + (i / 10)) as u8;
        id_buf[2] = (48 + (i % 10)) as u8;
        expect_eq(lb.register_backend(id_buf[0..3], 1) != null, true);
    }
    // 65th backend rejected
    expect_eq(lb.register_backend("overflow-65", 1) == null, true);

    // Removing slot 0 frees it for reuse
    expect_eq(lb.remove_backend(0), true);
    expect_eq(lb.remove_backend(0), false);
    const reused = lb.register_backend("replacement", 3) ?? return error.ReuseFailed;
    expect_eq(reused, 0);
}

// ============================================================================
// Test 2: Peak-EWMA Low-Latency Preference & Spike Reaction
// ============================================================================

fn testPeakEwmaLatencyPreference()! {
    let lb = P2cLoadBalancer.init(P2cBalancerConfig{
        ewma_alpha_f32: 0.5,
        max_consecutive_errors: 5,
        ejection_cooldown_ms: 5000,
        max_ejection_percent: 50,
        inflight_penalty_us: 1000.0,
    }) ?? return error.InitFailed;

    const fast = lb.register_backend("fast-node", 1) ?? return error.RegFailed;
    const slow = lb.register_backend("slow-node", 1) ?? return error.RegFailed;

    // Seed latency samples: fast = 2000us (2ms), slow = 80000us (80ms)
    expect_eq(lb.report_outcome(fast, true, 2000, 1000), true);
    expect_eq(lb.report_outcome(slow, true, 80000, 1000), true);

    // With 2 nodes, P2C always compares both -> fast must win every single time when inflight == 0
    let fast_wins: usize = 0;
    let i: usize = 0;
    while (i < 50) : (i += 1) {
        const pick: P2cPickDecision = lb.pick_backend(2000, 100 + (i as u64));
        expect_eq(pick.found, true);
        expect_eq(pick.used_fallback_panic_mode, false);
        if (pick.slot_idx == fast) {
            fast_wins += 1;
        }
        * = lb.report_outcome(pick.slot_idx, true, 2000, 2002);
    }
    expect_eq(fast_wins, 50);

    // Peak-EWMA spike reaction: if fast spikes to 120000us, its ewma jumps immediately to 120000us
    * = lb.report_outcome(fast, true, 120000, 3000);
    expect_eq(approxEqF32(lb.slots[fast].ewma_latency_us, 120000.0, 0.1), true);
    // Now slow (80000us) has lower load than spiked fast (120000us)
    const after_spike = lb.pick_backend(3010, 999);
    expect_eq(after_spike.slot_idx, slow);
    * = lb.report_outcome(slow, true, 40000, 3050); // EWMA decays: 0.5*40000 + 0.5*80000 = 60000
    expect_eq(approxEqF32(lb.slots[slow].ewma_latency_us, 60000.0, 0.1), true);
}

// ============================================================================
// Test 3: High Inflight Concurrency Avoidance & Weight Skew
// ============================================================================

fn testInflightAvoidanceAndWeightSkew()! {
    let lb = P2cLoadBalancer.init(P2cBalancerConfig{
        ewma_alpha_f32: 0.25,
        max_consecutive_errors: 3,
        ejection_cooldown_ms: 5000,
        max_ejection_percent: 50,
        inflight_penalty_us: 10000.0,
    }) ?? return error.InitFailed;

    const heavy = lb.register_backend("heavy-w4", 4) ?? return error.RegFailed;
    const light = lb.register_backend("light-w1", 1) ?? return error.RegFailed;

    // Equal latency 4000us: heavy load = 4000/4 = 1000, light load = 4000/1 = 4000 -> heavy picked first
    * = lb.report_outcome(heavy, true, 4000, 1000);
    * = lb.report_outcome(light, true, 4000, 1000);

    const p1 = lb.pick_backend(1100, 1);
    expect_eq(p1.slot_idx, heavy);
    expect_eq(lb.slots[heavy].inflight, 1);

    // While heavy has inflight=1: heavy load = 4000*2/4 + 10000 = 12000 > light load 4000 -> light picked!
    const p2 = lb.pick_backend(1100, 2);
    expect_eq(p2.slot_idx, light);
    expect_eq(lb.slots[light].inflight, 1);
}

// ============================================================================
// Test 4: Consecutive Errors Ejection & max_ejection_percent Anti-Cascade Guard
// ============================================================================

fn testOutlierEjectionAndAntiCascadeGuard()! {
    let lb = P2cLoadBalancer.init(P2cBalancerConfig{
        ewma_alpha_f32: 0.25,
        max_consecutive_errors: 2,
        ejection_cooldown_ms: 10000,
        max_ejection_percent: 50, // max 50% of 4 nodes = 2 ejected!
        inflight_penalty_us: 1000.0,
    }) ?? return error.InitFailed;

    const n0 = lb.register_backend("n0", 1) ?? return error.RegFailed;
    const n1 = lb.register_backend("n1", 1) ?? return error.RegFailed;
    const n2 = lb.register_backend("n2", 1) ?? return error.RegFailed;
    const n3 = lb.register_backend("n3", 1) ?? return error.RegFailed;
    * = n3;

    // Eject n0 (2 consecutive errors)
    * = lb.report_outcome(n0, false, 5000, 1000);
    expect_eq(lb.slots[n0].state == BackendHealthState.healthy, true);
    * = lb.report_outcome(n0, false, 5000, 1010);
    expect_eq(lb.slots[n0].state == BackendHealthState.ejected, true);

    // Eject n1 (2 consecutive errors -> 2/4 = 50% ejected)
    * = lb.report_outcome(n1, false, 5000, 1020);
    * = lb.report_outcome(n1, false, 5000, 1030);
    expect_eq(lb.slots[n1].state == BackendHealthState.ejected, true);

    // n2 fails 3 times -> MUST NOT be ejected because 2/4 already hit max_ejection_percent = 50%!
    * = lb.report_outcome(n2, false, 5000, 1040);
    * = lb.report_outcome(n2, false, 5000, 1050);
    * = lb.report_outcome(n2, false, 5000, 1060);
    expect_eq(lb.slots[n2].state == BackendHealthState.healthy, true);

    const st = lb.stats();
    expect_eq(st.ejected_count, 2);
    expect_eq(st.healthy_count, 2);
    expect_eq(st.total_ejections, 2);
}

// ============================================================================
// Test 5: Cooldown -> Half-Open Probing -> Recovery & Re-Ejection
// ============================================================================

fn testHalfOpenRecoveryAndReEjection()! {
    let lb = P2cLoadBalancer.init(P2cBalancerConfig{
        ewma_alpha_f32: 0.25,
        max_consecutive_errors: 2,
        ejection_cooldown_ms: 5000,
        max_ejection_percent: 50,
        inflight_penalty_us: 1000.0,
    }) ?? return error.InitFailed;

    const n0 = lb.register_backend("primary", 1) ?? return error.RegFailed;
    const n1 = lb.register_backend("backup", 1) ?? return error.RegFailed;
    * = n1;

    // Eject n0 at t=1000 (cooldown expires at t=6000)
    * = lb.report_outcome(n0, false, 5000, 1000);
    * = lb.report_outcome(n0, false, 5000, 1000);
    expect_eq(lb.slots[n0].state == BackendHealthState.ejected, true);
    expect_eq(lb.slots[n0].ejected_until_ms, 6000);

    // At t=5999: still ejected, pick_backend only sees n1
    const p_before = lb.pick_backend(5999, 1);
    expect_eq(p_before.slot_idx, n1);
    expect_eq(lb.slots[n0].state == BackendHealthState.ejected, true);
    * = lb.report_outcome(n1, true, 1000, 5999);

    // At t=6000: cooldown expired -> pick_backend transitions n0 to half_open
    * = lb.pick_backend(6000, 2);
    expect_eq(lb.slots[n0].state == BackendHealthState.half_open, true);

    // Failed half_open probe immediately re-ejects n0 until t=11000!
    * = lb.report_outcome(n0, false, 5000, 6000);
    expect_eq(lb.slots[n0].state == BackendHealthState.ejected, true);
    expect_eq(lb.slots[n0].ejected_until_ms, 11000);

    // At t=11000: transitions to half_open again; successful probe restores healthy!
    * = lb.pick_backend(11000, 3);
    expect_eq(lb.slots[n0].state == BackendHealthState.half_open, true);
    * = lb.report_outcome(n0, true, 1000, 11005);
    expect_eq(lb.slots[n0].state == BackendHealthState.healthy, true);
    expect_eq(lb.slots[n0].consecutive_errors, 0);
}

// ============================================================================
// Test 6: Panic-Mode Fallback When All Backends Ejected
// ============================================================================

fn testPanicModeFallbackRouting()! {
    let lb = P2cLoadBalancer.init(P2cBalancerConfig{
        ewma_alpha_f32: 0.25,
        max_consecutive_errors: 1,
        ejection_cooldown_ms: 10000,
        max_ejection_percent: 100, // allow 100% ejection to test Panic Mode
        inflight_penalty_us: 1000.0,
    }) ?? return error.InitFailed;

    const n0 = lb.register_backend("node-a", 1) ?? return error.RegFailed;
    const n1 = lb.register_backend("node-b", 1) ?? return error.RegFailed;

    // Eject both nodes at t=1000
    * = lb.report_outcome(n0, false, 5000, 1000);
    * = lb.report_outcome(n1, false, 5000, 1000);
    expect_eq(lb.stats().ejected_count, 2);
    expect_eq(lb.stats().healthy_count, 0);

    // Picking at t=2000 must still succeed via Panic Mode fallback!
    const panic_pick = lb.pick_backend(2000, 42);
    expect_eq(panic_pick.found, true);
    expect_eq(panic_pick.used_fallback_panic_mode, true);
    expect_eq(lb.stats().panic_mode_picks, 1);
}

// ============================================================================
// Test 7: Invalid Config, Out-of-Bounds Guards & Stats Consistency
// ============================================================================

fn testInvalidConfigAndStatsConsistency()! {
    expect_eq(P2cLoadBalancer.init(P2cBalancerConfig{ ewma_alpha_f32: 0.0 }) == null, true);
    expect_eq(P2cLoadBalancer.init(P2cBalancerConfig{ ewma_alpha_f32: 1.5 }) == null, true);
    expect_eq(P2cLoadBalancer.init(P2cBalancerConfig{ max_consecutive_errors: 0 }) == null, true);
    expect_eq(P2cLoadBalancer.init(P2cBalancerConfig{ ejection_cooldown_ms: 0 }) == null, true);
    expect_eq(P2cLoadBalancer.init(P2cBalancerConfig{ max_ejection_percent: 101 }) == null, true);
    expect_eq(P2cLoadBalancer.init(P2cBalancerConfig{ inflight_penalty_us: -1.0 }) == null, true);

    let lb = P2cLoadBalancer.init(P2cBalancerConfig{}) ?? return error.InitFailed;
    expect_eq(lb.report_outcome(0, true, 100, 1000), false);   // unoccupied slot
    expect_eq(lb.report_outcome(99, true, 100, 1000), false);  // out-of-bounds slot
    expect_eq(lb.remove_backend(99), false);

    const s0 = lb.register_backend("svc-a", 1) ?? return error.RegFailed;
    const pick = lb.pick_backend(1000, 7);
    expect_eq(std.mem.eql(u8, pick.idSlice(), "svc-a"), true);
    * = lb.report_outcome(s0, true, 500, 1001);

    const st: P2cBalancerStats = lb.stats();
    expect_eq(st.active_count, 1);
    expect_eq(st.healthy_count, 1);
    expect_eq(st.ejected_count, 0);
    expect_eq(st.half_open_count, 0);
    expect_eq(st.total_picks, 1);
    expect_eq(st.total_ejections, 0);
    expect_eq(st.panic_mode_picks, 0);
}

test("net p2c_balancer: registration, dedup, 64-slot capacity & removal reuse", testRegistrationDedupCapacityAndRemove);
test("net p2c_balancer: Peak-EWMA low-latency preference & spike reaction", testPeakEwmaLatencyPreference);
test("net p2c_balancer: high inflight concurrency avoidance & weight skew", testInflightAvoidanceAndWeightSkew);
test("net p2c_balancer: consecutive-error ejection & max_ejection_percent anti-cascade", testOutlierEjectionAndAntiCascadeGuard);
test("net p2c_balancer: cooldown half_open recovery & failed probe re-ejection", testHalfOpenRecoveryAndReEjection);
test("net p2c_balancer: Panic-Mode fallback routing when all backends ejected", testPanicModeFallbackRouting);
test("net p2c_balancer: invalid config, out-of-bounds guards & stats consistency", testInvalidConfigAndStatsConsistency);
