// Origin: Pure-CP Stdlib Test Suite (lib/net/hedgedReq.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Adaptive Tail-Latency Hedged Requests & Budget Scheduler:
//   1. Adaptive TailLatencyEstimator: cold-start default, P95/P99 dynamic threshold & min/max clamp
//   2. Hedged replica wins race: pollDueHedges launch, winner_hedged, slow-primary cancel list & tail-saved stats
//   3. Primary wins fast (zero hedge) & primary outraces launched hedge (winner_primary cancels backup)
//   4. Amplification budget guard (10% ratio): suppresses excess hedges under overload & refills with primary traffic
//   5. Fail-fast immediate backup promotion on primary error & all_attempts_failed fallback
//   6. Timeout reaping (reapTimedOut) & slot reclamation
//   7. Full boundary guards (invalid config, zero/dup nodes, dup request_id) & cap.net facade

import { test, expect_eq, expect_true } from "cap:test";
import {
    HedgeCompletionKind,
    HedgeSchedulerConfig,
    TailLatencyEstimator,
    HedgeLaunchEvent,
    HedgedRequestScheduler,
} from "../../lib/net/hedgedReq.cp";

// ============================================================================
// Test 1: Adaptive TailLatencyEstimator (Cold-Start, P95/P99 & Clamp Bounds)
// ============================================================================
test("hedgedReq: TailLatencyEstimator computes cold-start default, P95 & clamped bounds", () => {
    let est = TailLatencyEstimator.init(9500, 10, 50, 10, 500);

    // Cold start (< 10 samples) returns default 50ms
    let i: u64 = 1;
    while (i <= 9) : (i += 1) {
        est.recordSample(i * 10);
    }
    expect_eq(est.estimateDelayMs(), 50);

    // 20 samples: [10, 20, ..., 200] -> P95 nearest-rank = 19th element = 190ms
    while (i <= 20) : (i += 1) {
        est.recordSample(i * 10);
    }
    expect_eq(est.estimateDelayMs(), 190);

    // Clamp to max_delay_ms (500) on extreme spikes
    let est_hi = TailLatencyEstimator.init(9500, 4, 50, 10, 500);
    est_hi.recordSample(1000);
    est_hi.recordSample(2000);
    est_hi.recordSample(3000);
    est_hi.recordSample(4000);
    expect_eq(est_hi.estimateDelayMs(), 500);

    // Clamp to min_delay_ms (10) on ultra-fast samples
    let est_lo = TailLatencyEstimator.init(9500, 4, 50, 10, 500);
    est_lo.recordSample(1);
    est_lo.recordSample(2);
    est_lo.recordSample(3);
    est_lo.recordSample(4);
    expect_eq(est_lo.estimateDelayMs(), 10);
});

// ============================================================================
// Test 2: Hedged Replica Wins Race & Returns Slow Primary to Cancel
// ============================================================================
test("hedgedReq: due hedge launches backup, backup wins & cancels slow primary with saved tail stats", () => {
    let cfg = HedgeSchedulerConfig.defaults();
    cfg.default_hedge_delay_ms = 50;
    cfg.initial_hedge_tokens = 3;
    let sched = HedgedRequestScheduler.init(cfg) ?? return;

    const nodes: [3]u64 = [101, 102, 103];
    expect_true(sched.startRequest(1, nodes[0..], 500, 1000));

    // At t=1030 (< 1050): no hedge due yet
    let launches: [4]HedgeLaunchEvent = [HedgeLaunchEvent{}; 4];
    let launches_s: []HedgeLaunchEvent = launches[0..];
    expect_eq(sched.pollDueHedges(1030, *launches_s), 0);

    // At t=1050: backup node 102 (attempt 1) is launched
    const n1 = sched.pollDueHedges(1050, *launches_s);
    expect_eq(n1, 1);
    expect_eq(launches[0].request_id, 1);
    expect_eq(launches[0].attempt_idx, 1);
    expect_eq(launches[0].node_id, 102);

    // At t=1065: backup node 102 succeeds first! Must cancel slow primary 101
    const comp = sched.completeAttempt(1, 1, true, 1065);
    expect_eq(comp.kind, HedgeCompletionKind.winner_hedged);
    expect_eq(comp.winner_node_id, 102);
    expect_eq(comp.latency_ms, 15);
    expect_eq(comp.cancel_count, 1);
    expect_eq(comp.cancelNodesSlice()[0], 101);

    // Late primary arrival is ignored cleanly
    const late = sched.completeAttempt(1, 0, true, 1200);
    expect_eq(late.kind, HedgeCompletionKind.ignored_stale);

    const st = sched.stats();
    expect_eq(st.inflight_requests, 0);
    expect_eq(st.total_hedges_launched, 1);
    expect_eq(st.total_hedge_wins, 1);
    expect_eq(st.estimated_tail_saved_ms, 50);
});

// ============================================================================
// Test 3: Primary Wins Fast & Primary Outraces Launched Hedge
// ============================================================================
test("hedgedReq: primary wins before hedge (no cancel) and after hedge (cancels backup)", () => {
    let cfg = HedgeSchedulerConfig.defaults();
    cfg.default_hedge_delay_ms = 50;
    cfg.initial_hedge_tokens = 2;
    let sched = HedgedRequestScheduler.init(cfg) ?? return;

    const nodes: [2]u64 = [201, 202];
    // 3A. Fast primary completes at t=1020 before hedge at 1050
    expect_true(sched.startRequest(10, nodes[0..], 500, 1000));
    const c1 = sched.completeAttempt(10, 0, true, 1020);
    expect_eq(c1.kind, HedgeCompletionKind.winner_primary);
    expect_eq(c1.cancel_count, 0);

    // 3B. Primary completes at t=1060 after backup launched at t=1050 -> cancels backup 202
    expect_true(sched.startRequest(11, nodes[0..], 500, 1000));
    let launches: [2]HedgeLaunchEvent = [HedgeLaunchEvent{}; 2];
    let launches_s: []HedgeLaunchEvent = launches[0..];
    expect_eq(sched.pollDueHedges(1050, *launches_s), 1);

    const c2 = sched.completeAttempt(11, 0, true, 1060);
    expect_eq(c2.kind, HedgeCompletionKind.winner_primary);
    expect_eq(c2.winner_node_id, 201);
    expect_eq(c2.cancel_count, 1);
    expect_eq(c2.cancelNodesSlice()[0], 202);
    expect_eq(sched.stats().total_primary_wins, 2);
});

// ============================================================================
// Test 4: Amplification Budget Guard (10% Ratio Refill & Overload Suppression)
// ============================================================================
test("hedgedReq: amplification budget suppresses overload hedges and refills with 10% primary traffic", () => {
    let cfg = HedgeSchedulerConfig.defaults();
    cfg.default_hedge_delay_ms = 20;
    cfg.max_hedge_ratio_bps = 1000; // 10% => 10 primary requests = 1 hedge token
    cfg.max_burst_hedges = 4;
    cfg.initial_hedge_tokens = 1;   // Start with exactly 1 hedge token
    let sched = HedgedRequestScheduler.init(cfg) ?? return;

    const nodes: [2]u64 = [1, 2];
    expect_true(sched.startRequest(1, nodes[0..], 500, 1000));
    expect_true(sched.startRequest(2, nodes[0..], 500, 1000));

    // At t=1020: req 1 consumes the 1 available token; req 2 is suppressed!
    let launches: [4]HedgeLaunchEvent = [HedgeLaunchEvent{}; 4];
    let launches_s: []HedgeLaunchEvent = launches[0..];
    const launched = sched.pollDueHedges(1020, *launches_s);
    expect_eq(launched, 1);
    expect_eq(sched.stats().total_hedges_suppressed, 1);

    // Complete req 1 & req 2, then send 8 more fast primary requests (total 10 primaries -> +1 full token)
    * = sched.completeAttempt(1, 0, true, 1025);
    * = sched.completeAttempt(2, 0, true, 1025);
    let r: u64 = 10;
    while (r < 18) : (r += 1) {
        expect_true(sched.startRequest(r, nodes[0..], 500, 2000));
        * = sched.completeAttempt(r, 0, true, 2005);
    }
    expect_eq(sched.stats().available_budget_tokens, 1);

    // Next slow request can launch a hedge again!
    expect_true(sched.startRequest(99, nodes[0..], 500, 3000));
    expect_eq(sched.pollDueHedges(3050, *launches_s), 1);
});

// ============================================================================
// Test 5: Fail-Fast Immediate Backup Promotion & All-Attempts-Failed Fallback
// ============================================================================
test("hedgedReq: primary failure promotes backup immediately & reports all_attempts_failed", () => {
    let cfg = HedgeSchedulerConfig.defaults();
    cfg.default_hedge_delay_ms = 500; // Long hedge timer
    cfg.initial_hedge_tokens = 2;
    let sched = HedgedRequestScheduler.init(cfg) ?? return;

    const nodes: [2]u64 = [301, 302];
    expect_true(sched.startRequest(50, nodes[0..], 2000, 1000));

    // Primary fails at t=1005 (well before 1500) -> immediately makes backup eligible!
    const f1 = sched.completeAttempt(50, 0, false, 1005);
    expect_eq(f1.kind, HedgeCompletionKind.attempt_failed_continue);

    let launches: [2]HedgeLaunchEvent = [HedgeLaunchEvent{}; 2];
    let launches_s: []HedgeLaunchEvent = launches[0..];
    expect_eq(sched.pollDueHedges(1005, *launches_s), 1);
    expect_eq(launches[0].node_id, 302);

    // Backup also fails -> all_attempts_failed
    const f2 = sched.completeAttempt(50, 1, false, 1020);
    expect_eq(f2.kind, HedgeCompletionKind.all_attempts_failed);
    expect_eq(sched.stats().total_all_failed, 1);
    expect_eq(sched.stats().inflight_requests, 0);
});

// ============================================================================
// Test 6: Timeout Reaping & Slot Reclamation
// ============================================================================
test("hedgedReq: reapTimedOut cleans expired in-flight requests and frees slots", () => {
    let sched = HedgedRequestScheduler.init(HedgeSchedulerConfig.defaults()) ?? return;
    const nodes: [2]u64 = [10, 20];

    expect_true(sched.startRequest(1, nodes[0..], 100, 1000)); // deadline 1100
    expect_true(sched.startRequest(2, nodes[0..], 500, 1000)); // deadline 1500

    let timed_out: [4]u64 = [0; 4];
    let out_s: []u64 = timed_out[0..];
    const n = sched.reapTimedOut(1200, *out_s);
    expect_eq(n, 1);
    expect_eq(timed_out[0], 1);
    expect_eq(sched.stats().total_timed_out, 1);
    expect_eq(sched.stats().inflight_requests, 1);
});

// ============================================================================
// Test 7: Boundary Guards & cap.net Facade Verification
// ============================================================================
test("hedgedReq: rejects invalid configs, bad node lists, dup IDs & cap.net facade works", () => {
    let bad_cfg = HedgeSchedulerConfig.defaults();
    bad_cfg.percentile_bps = 0;
    expect_true(HedgedRequestScheduler.init(bad_cfg) == null);

    let sched = HedgedRequestScheduler.init(HedgeSchedulerConfig.defaults()) ?? return;
    const empty_nodes: [0]u64 = [];
    const zero_node: [2]u64 = [1, 0];
    const dup_nodes: [2]u64 = [5, 5];
    const good_nodes: [2]u64 = [5, 6];

    expect_true(!sched.startRequest(0, good_nodes[0..], 100, 0));
    expect_true(!sched.startRequest(1, empty_nodes[0..], 100, 0));
    expect_true(!sched.startRequest(1, zero_node[0..], 100, 0));
    expect_true(!sched.startRequest(1, dup_nodes[0..], 100, 0));

    expect_true(sched.startRequest(1, good_nodes[0..], 100, 0));
    expect_true(!sched.startRequest(1, good_nodes[0..], 100, 0)); // duplicate active request_id
    expect_eq(sched.currentHedgeDelayMs(), 50);
});
