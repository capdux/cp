// Origin: Pure-CP Stdlib Test Suite (lib/net/traffic_shaper.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Traffic Shaper, WDRR Scheduler & AIMD Limiter:
//   1. RFC 2698 trTCM Green/Yellow/Red marking, fractional ms refill, clock-skew immunity & reset
//   2. trTCM invalid config guards & large-delta saturation
//   3. WeightedDrrScheduler exact 3:1 weighted fairness across competing tenant flows
//   4. WeightedDrrScheduler jumbo-vs-small anti-starvation & O(N) huge-packet fast-forward
//   5. AimdLimiter additive increase, multiplicative decrease, clamping & in-flight admission control
//   6. WDRR & AIMD invalid init, duplicate/full/unknown flow & zero-byte guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TrafficColor,
    TrTcmConfig,
    TrTcmPolicer,
    DrrFlowSlot,
    WeightedDrrScheduler,
    AimdConfig,
    AimdLimiter,
} from "cap:net";

// ============================================================================
// Test 1: RFC 2698 trTCM Three-Color Marking, Fractional Refill & Clock Skew
// ============================================================================

fn testTrTcmPolicerAndRefill()! {
    // CIR=1000 B/s, CBS=1000 B; PIR=2000 B/s, PBS=2000 B
    let policer = cap.net.TrTcmPolicer.init(TrTcmConfig{
        cir_bytes_per_sec: 1000,
        cbs_bytes: 1000,
        pir_bytes_per_sec: 2000,
        pbs_bytes: 2000,
    }, 1000) ?? return error.InitFailed;

    expect_eq(policer.committedTokens(), 1000);
    expect_eq(policer.peakTokens(), 2000);

    // 1000B fits in both C and P -> Green (tc=0, tp=1000)
    expect_eq(policer.police(1000, 1000) == TrafficColor.Green, true);
    expect_eq(policer.committedTokens(), 0);
    expect_eq(policer.peakTokens(), 1000);

    // 800B exceeds C (0) but fits in P (1000) -> Yellow (tc=0, tp=200)
    expect_eq(policer.police(800, 1000) == TrafficColor.Yellow, true);
    expect_eq(policer.committedTokens(), 0);
    expect_eq(policer.peakTokens(), 200);

    // 500B exceeds P (200) -> Red (buckets unchanged)
    expect_eq(policer.police(500, 1000) == TrafficColor.Red, true);
    expect_eq(policer.peakTokens(), 200);

    // Backwards clock skew (t=500 < 1000) must not alter buckets
    policer.refill(500);
    expect_eq(policer.committedTokens(), 0);
    expect_eq(policer.peakTokens(), 200);

    // Advance 500ms in 5x 100ms ticks -> C refills +500B (to 500), P refills +1000B (to 1200)
    let tick: i64 = 1100;
    while (tick <= 1500) : (tick += 100) {
        policer.refill(tick);
    }
    expect_eq(policer.committedTokens(), 500);
    expect_eq(policer.peakTokens(), 1200);

    // 500B now conforms to both C and P -> Green
    expect_eq(policer.police(500, 1500) == TrafficColor.Green, true);
    expect_eq(policer.greenCount(), 2);
    expect_eq(policer.yellowCount(), 1);
    expect_eq(policer.redCount(), 1);

    // Reset restores full burst capacities and zeroes counters
    policer.reset(2000);
    expect_eq(policer.committedTokens(), 1000);
    expect_eq(policer.peakTokens(), 2000);
    expect_eq(policer.greenCount(), 0);
}

// ============================================================================
// Test 2: trTCM Invalid Config Guards & Overflow Saturation
// ============================================================================

fn testTrTcmInvalidConfigsAndSaturation()! {
    // cir == 0
    expect_eq(TrTcmPolicer.init(TrTcmConfig{ cir_bytes_per_sec: 0, cbs_bytes: 100, pir_bytes_per_sec: 200, pbs_bytes: 200 }, 0) == null, true);
    // cbs == 0
    expect_eq(TrTcmPolicer.init(TrTcmConfig{ cir_bytes_per_sec: 100, cbs_bytes: 0, pir_bytes_per_sec: 200, pbs_bytes: 200 }, 0) == null, true);
    // pir < cir
    expect_eq(TrTcmPolicer.init(TrTcmConfig{ cir_bytes_per_sec: 500, cbs_bytes: 100, pir_bytes_per_sec: 400, pbs_bytes: 200 }, 0) == null, true);
    // pbs < cbs
    expect_eq(TrTcmPolicer.init(TrTcmConfig{ cir_bytes_per_sec: 100, cbs_bytes: 500, pir_bytes_per_sec: 200, pbs_bytes: 400 }, 0) == null, true);

    // Huge time jump saturates cleanly to burst cap
    let p = TrTcmPolicer.init(TrTcmConfig{
        cir_bytes_per_sec: 1000000,
        cbs_bytes: 4096,
        pir_bytes_per_sec: 2000000,
        pbs_bytes: 8192,
    }, 0) ?? return error.InitFailed;
    * = p.police(4096, 0);
    p.refill(9223372036854775807);
    expect_eq(p.committedTokens(), 4096);
    expect_eq(p.peakTokens(), 8192);
}

// ============================================================================
// Test 3: WeightedDrrScheduler Exact 3:1 Weighted Fairness
// ============================================================================

fn testWdrrWeightedFairness()! {
    let slots: [4]DrrFlowSlot = [DrrFlowSlot{}; 4];
    let slots_s: []DrrFlowSlot = slots[0..];
    let sched = WeightedDrrScheduler.init(slots_s) ?? return error.InitFailed;

    // Flow 10 has 3x the quantum of Flow 20 (600 vs 200)
    expect_eq(sched.registerFlow(10, 600), true);
    expect_eq(sched.registerFlow(20, 200), true);

    // Enqueue 6x 200B packets on Flow 10, 2x 200B packets on Flow 20
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        expect_eq(sched.enqueue(10, 200), true);
    }
    expect_eq(sched.enqueue(20, 200), true);
    expect_eq(sched.enqueue(20, 200), true);
    expect_eq(sched.activeFlows(), 2);

    // Round 1: Flow 10 dispatches 3 packets (600B), then Flow 20 dispatches 1 packet (200B)
    expect_eq(sched.scheduleNext() ?? 0, 10);
    expect_eq(sched.scheduleNext() ?? 0, 10);
    expect_eq(sched.scheduleNext() ?? 0, 10);
    expect_eq(sched.scheduleNext() ?? 0, 20);

    // Round 2: Flow 10 dispatches remaining 3 packets, then Flow 20 dispatches remaining 1 packet
    expect_eq(sched.scheduleNext() ?? 0, 10);
    expect_eq(sched.scheduleNext() ?? 0, 10);
    expect_eq(sched.scheduleNext() ?? 0, 10);
    expect_eq(sched.scheduleNext() ?? 0, 20);

    // Empty now
    expect_eq(sched.scheduleNext() == null, true);
    expect_eq(sched.activeFlows(), 0);

    const f10 = sched.flowInfo(10) ?? return error.MissingFlow;
    const f20 = sched.flowInfo(20) ?? return error.MissingFlow;
    expect_eq(f10.dispatched_bytes, 1200);
    expect_eq(f10.dispatched_packets, 6);
    expect_eq(f20.dispatched_bytes, 400);
    expect_eq(f20.dispatched_packets, 2);
}

// ============================================================================
// Test 4: WDRR Anti-Starvation (Jumbo vs Small Packets) & Huge Packet Fast-Forward
// ============================================================================

fn testWdrrAntiStarvationAndFastForward()! {
    let slots: [4]DrrFlowSlot = [DrrFlowSlot{}; 4];
    let slots_s: []DrrFlowSlot = slots[0..];
    let sched = WeightedDrrScheduler.init(slots_s) ?? return error.InitFailed;

    // Equal quantum=500 for both flows:
    // Flow 1 has a 1500B jumbo packet; Flow 2 has 6x 250B small packets.
    expect_eq(sched.registerFlow(1, 500), true);
    expect_eq(sched.registerFlow(2, 500), true);
    expect_eq(sched.enqueue(1, 1500), true);
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        expect_eq(sched.enqueue(2, 250), true);
    }

    // Round 1: Flow 1 gets 500 < 1500 (waits); Flow 2 dispatches 2x 250B!
    expect_eq(sched.scheduleNext() ?? 0, 2);
    expect_eq(sched.scheduleNext() ?? 0, 2);
    // Round 2: Flow 1 gets 1000 < 1500 (waits); Flow 2 dispatches 2x 250B!
    expect_eq(sched.scheduleNext() ?? 0, 2);
    expect_eq(sched.scheduleNext() ?? 0, 2);
    // Round 3: Flow 1 reaches 1500 >= 1500 -> dispatches jumbo packet!
    expect_eq(sched.scheduleNext() ?? 0, 1);
    // Remaining 2 small packets on Flow 2 dispatch
    expect_eq(sched.scheduleNext() ?? 0, 2);
    expect_eq(sched.scheduleNext() ?? 0, 2);
    expect_eq(sched.scheduleNext() == null, true);

    // Huge single packet (100,000 bytes on quantum=500) must dispatch in O(N) via fast-forward
    expect_eq(sched.enqueue(1, 100000), true);
    expect_eq(sched.scheduleNext() ?? 0, 1);
    expect_eq(sched.scheduleNext() == null, true);
}

// ============================================================================
// Test 5: AimdLimiter Additive Increase, Multiplicative Decrease & Clamping
// ============================================================================

fn testAimdLimiterWindowDynamics()! {
    let aimd = AimdLimiter.init(AimdConfig{
        initial_limit: 4,
        min_limit: 2,
        max_limit: 6,
        additive_increase: 1,
        backoff_num: 1,
        backoff_den: 2,
    }) ?? return error.InitFailed;

    expect_eq(aimd.currentLimit(), 4);
    expect_eq(aimd.available(), 4);

    // Fill all 4 in-flight slots; 5th attempt is rejected
    expect_eq(aimd.tryAcquire(), true);
    expect_eq(aimd.tryAcquire(), true);
    expect_eq(aimd.tryAcquire(), true);
    expect_eq(aimd.tryAcquire(), true);
    expect_eq(aimd.tryAcquire(), false);
    expect_eq(aimd.rejectedCount(), 1);
    expect_eq(aimd.inFlight(), 4);
    expect_eq(aimd.available(), 0);

    // Additive increase up to max_limit (6)
    expect_eq(aimd.onSuccess(), 5);
    expect_eq(aimd.onSuccess(), 6);
    expect_eq(aimd.onSuccess(), 6); // clamped at max_limit
    expect_eq(aimd.inFlight(), 1);

    // Release without feedback leaves limit unchanged
    aimd.releaseWithoutFeedback();
    expect_eq(aimd.inFlight(), 0);
    expect_eq(aimd.currentLimit(), 6);

    // Multiplicative decrease (1/2) down to min_limit (2)
    expect_eq(aimd.tryAcquire(), true);
    expect_eq(aimd.onCongestion(), 3); // 6 / 2 = 3
    expect_eq(aimd.tryAcquire(), true);
    expect_eq(aimd.onCongestion(), 2); // floor(3/2)=1 -> clamped at min_limit=2
    expect_eq(aimd.onCongestion(), 2); // stays at min_limit=2

    expect_eq(aimd.successCount(), 3);
    expect_eq(aimd.congestionCount(), 3);

    // Reset restores initial state
    aimd.reset();
    expect_eq(aimd.currentLimit(), 4);
    expect_eq(aimd.inFlight(), 0);
    expect_eq(aimd.rejectedCount(), 0);
}

// ============================================================================
// Test 6: WDRR & AIMD Invalid Init & Boundary Guards
// ============================================================================

fn testWdrrAndAimdBoundaryGuards()! {
    let buf: [2]DrrFlowSlot = [DrrFlowSlot{}; 2];
    let empty: []DrrFlowSlot = buf[0..0];
    let slots_s: []DrrFlowSlot = buf[0..];

    // WDRR empty init -> null
    expect_eq(WeightedDrrScheduler.init(empty) == null, true);

    let sched = WeightedDrrScheduler.init(slots_s) ?? return error.InitFailed;
    expect_eq(sched.registerFlow(0, 100), false);
    expect_eq(sched.registerFlow(1, 0), false);
    expect_eq(sched.registerFlow(1, 100), true);
    expect_eq(sched.registerFlow(1, 200), false); // duplicate
    expect_eq(sched.registerFlow(2, 100), true);
    expect_eq(sched.registerFlow(3, 100), false); // full
    expect_eq(sched.enqueue(99, 100), false);     // unknown flow
    expect_eq(sched.enqueue(1, 0), false);        // zero bytes
    expect_eq(sched.setHeadPacketBytes(1, 100), false); // inactive flow

    // AIMD invalid configs -> null
    expect_eq(AimdLimiter.init(AimdConfig{ initial_limit: 4, min_limit: 0, max_limit: 8, additive_increase: 1, backoff_num: 1, backoff_den: 2 }) == null, true);
    expect_eq(AimdLimiter.init(AimdConfig{ initial_limit: 4, min_limit: 10, max_limit: 8, additive_increase: 1, backoff_num: 1, backoff_den: 2 }) == null, true);
    expect_eq(AimdLimiter.init(AimdConfig{ initial_limit: 2, min_limit: 4, max_limit: 8, additive_increase: 1, backoff_num: 1, backoff_den: 2 }) == null, true);
    expect_eq(AimdLimiter.init(AimdConfig{ initial_limit: 4, min_limit: 1, max_limit: 8, additive_increase: 0, backoff_num: 1, backoff_den: 2 }) == null, true);
    expect_eq(AimdLimiter.init(AimdConfig{ initial_limit: 4, min_limit: 1, max_limit: 8, additive_increase: 1, backoff_num: 2, backoff_den: 2 }) == null, true);
    expect_eq(AimdLimiter.init(AimdConfig{ initial_limit: 4, min_limit: 1, max_limit: 8, additive_increase: 1, backoff_num: 0, backoff_den: 2 }) == null, true);
}

test("net traffic_shaper: RFC 2698 trTCM three-color marking, fractional refill & clock skew", testTrTcmPolicerAndRefill);
test("net traffic_shaper: trTCM invalid config guards & overflow saturation", testTrTcmInvalidConfigsAndSaturation);
test("net traffic_shaper: WeightedDrrScheduler exact 3:1 weighted fairness", testWdrrWeightedFairness);
test("net traffic_shaper: WDRR anti-starvation (jumbo vs small) & O(N) fast-forward", testWdrrAntiStarvationAndFastForward);
test("net traffic_shaper: AimdLimiter additive increase, multiplicative decrease & clamping", testAimdLimiterWindowDynamics);
test("net traffic_shaper: WDRR & AIMD invalid init and boundary guards", testWdrrAndAimdBoundaryGuards);
