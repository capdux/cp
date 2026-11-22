// Origin: Pure-CP Stdlib Test Suite (lib/a2a/delegation.cp) | Created: 2026-11-22
// Production Deep Verification Suite for A2A Task Delegation & Fencing Token Manager:
//   1. Normal delegation, deadline-capped lease renewal, completion & terminal slot reuse
//   2. Lease timeout failover to backup agent, monotonic fence rotation & stale zombie write rejection
//   3. Hard deadline escalation (`Escalated`) and no-backup / post-failover timeout (`TimedOut`)
//   4. Explicit fail & cancel lifecycle transitions with post-cancel fence invalidation
//   5. Duplicate task_id, full slot table, invalid init & parameter validation guards
//   6. Clock boundary protection (exact expiry, backwards clock skew & i64 lease saturation)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DelegationState,
    DelegationRecord,
    DelegationStats,
    A2aDelegationManager,
} from "cap:a2a";

// ============================================================================
// Test 1: Normal Delegation, Lease Renewal, Completion & Slot Reuse
// ============================================================================

fn testNormalDelegationRenewCompleteAndReuse()! {
    let slots: [4]DelegationRecord = [DelegationRecord{}; 4];
    let slots_s: []DelegationRecord = slots[0..];

    let mgr = cap.a2a.A2aDelegationManager.init(slots_s) ?? return error.InitFailed;
    expect_eq(mgr.activeCount(), 0);

    // Delegate task 101: owner=10, backup=20, lease=1000ms, deadline=3500ms, now=1000ms
    const tok1 = mgr.delegate(101, 10, 20, 1000, 3500, 1000) ?? return error.DelegateFailed;
    expect_eq(tok1, 1);
    expect_eq(mgr.activeCount(), 1);
    expect_eq(mgr.verifyFence(101, tok1), true);
    expect_eq(mgr.verifyFence(101, tok1 + 1), false);

    // Renew lease at t=1800ms by 1200ms -> lease_expires_ms = 3000ms
    expect_eq(mgr.renewLease(101, tok1, 1200, 1800), true);
    const rec_a = mgr.get(101) ?? return error.MissingRecord;
    expect_eq(rec_a.lease_expires_ms, 3000);
    expect_eq(rec_a.attempt, 1);

    // Renew again at t=2800ms by 2000ms -> capped at deadline_ms (3500ms)
    expect_eq(mgr.renewLease(101, tok1, 2000, 2800), true);
    const rec_b = mgr.get(101) ?? return error.MissingRecord;
    expect_eq(rec_b.lease_expires_ms, 3500);

    // Complete before deadline
    expect_eq(mgr.complete(101, tok1, "artifact-ready", 3200), true);
    expect_eq(mgr.activeCount(), 0);
    expect_eq(mgr.verifyFence(101, tok1), false);

    const rec_done = mgr.get(101) ?? return error.MissingRecord;
    expect_eq(rec_done.state == DelegationState.Completed, true);
    expect_eq(std.mem.eql(u8, rec_done.result_summary, "artifact-ready"), true);

    const st: DelegationStats = mgr.stats();
    expect_eq(st.total, 1);
    expect_eq(st.completed, 1);
    expect_eq(st.active, 0);
    expect_eq(st.last_fence_token, 1);

    // Terminal slot can be released and task_id reused with a fresh higher fence_token
    expect_eq(mgr.releaseTerminal(101), true);
    expect_eq(mgr.get(101) == null, true);
    const tok2 = mgr.delegate(101, 10, 20, 1000, 6000, 4000) ?? return error.ReDelegateFailed;
    expect_eq(tok2 > tok1, true);
}

// ============================================================================
// Test 2: Lease Timeout Failover & Stale Fencing Token Write-Back Rejection
// ============================================================================

fn testFailoverAndStaleFenceRejection()! {
    let slots: [4]DelegationRecord = [DelegationRecord{}; 4];
    let slots_s: []DelegationRecord = slots[0..];
    let mgr = A2aDelegationManager.init(slots_s) ?? return error.InitFailed;

    // Owner=10, Backup=20, lease=1000ms, deadline=10000ms, now=1000ms -> expires at 2000ms
    const tok_owner = mgr.delegate(201, 10, 20, 1000, 10000, 1000) ?? return error.DelegateFailed;

    // Sweep before expiry -> no changes
    expect_eq(mgr.sweepTimeouts(1999), 0);

    // Sweep at t=2000ms -> automatic failover to backup 20 with new fence_token
    expect_eq(mgr.sweepTimeouts(2000), 1);

    const rec = mgr.get(201) ?? return error.MissingRecord;
    expect_eq(rec.state == DelegationState.Active, true);
    expect_eq(rec.owner_agent_id, 20);
    expect_eq(rec.attempt, 2);
    expect_eq(rec.failed_over, true);
    expect_eq(rec.fence_token > tok_owner, true);
    expect_eq(rec.lease_expires_ms, 3000);

    // Stale zombie worker 10 tries to renew, fail, and complete with old tok_owner -> all rejected!
    expect_eq(mgr.renewLease(201, tok_owner, 1000, 2100), false);
    expect_eq(mgr.fail(201, tok_owner, "late error", 2150), false);
    expect_eq(mgr.complete(201, tok_owner, "stale result", 2200), false);

    // Backup worker 20 completes with new fence_token -> accepted!
    expect_eq(mgr.complete(201, rec.fence_token, "backup result", 2300), true);

    const done = mgr.get(201) ?? return error.MissingRecord;
    expect_eq(done.state == DelegationState.Completed, true);
    expect_eq(std.mem.eql(u8, done.result_summary, "backup result"), true);

    const st = mgr.stats();
    expect_eq(st.total_failovers, 1);
    expect_eq(st.stale_fence_rejections, 3);
    expect_eq(st.completed, 1);
}

// ============================================================================
// Test 3: Hard Deadline Escalation & No-Backup / Backup-Exhausted Timeout
// ============================================================================

fn testDeadlineEscalationAndTimeout()! {
    let slots: [4]DelegationRecord = [DelegationRecord{}; 4];
    let slots_s: []DelegationRecord = slots[0..];
    let mgr = A2aDelegationManager.init(slots_s) ?? return error.InitFailed;

    // Task 301: has backup, but sweep happens after deadline -> Escalated immediately
    const tok301 = mgr.delegate(301, 10, 20, 1000, 3000, 1000) ?? return error.DelegateFailed;
    // Task 302: no backup (backup_agent_id = 0), lease expires at 2000ms, deadline 5000ms -> TimedOut
    const tok302 = mgr.delegate(302, 11, 0, 1000, 5000, 1000) ?? return error.DelegateFailed;
    // Task 303: has backup, fails over at 2000ms (new lease until 3000ms), then backup also times out at 3000ms -> TimedOut
    const tok303 = mgr.delegate(303, 12, 22, 1000, 6000, 1000) ?? return error.DelegateFailed;
    * = tok302;
    * = tok303;

    // Sweep at t=2000ms: 301 fails over to 20, 302 -> TimedOut, 303 fails over to 22
    expect_eq(mgr.sweepTimeouts(2000), 3);
    expect_eq((mgr.get(302) ?? return error.Missing).state == DelegationState.TimedOut, true);
    expect_eq((mgr.get(303) ?? return error.Missing).owner_agent_id, 22);

    // Sweep at t=3000ms: 301 hits deadline (3000ms) -> Escalated; 303 backup lease expires -> TimedOut
    expect_eq(mgr.sweepTimeouts(3000), 2);
    const r301 = mgr.get(301) ?? return error.Missing;
    const r303 = mgr.get(303) ?? return error.Missing;
    expect_eq(r301.state == DelegationState.Escalated, true);
    expect_eq(r303.state == DelegationState.TimedOut, true);

    // Old token for escalated task is rejected
    expect_eq(mgr.complete(301, tok301, "too late", 3100), false);

    const st = mgr.stats();
    expect_eq(st.escalated, 1);
    expect_eq(st.timed_out, 2);
    expect_eq(st.active, 0);
}

// ============================================================================
// Test 4: Explicit Fail & Cancel Transitions + Post-Cancel Fence Invalidation
// ============================================================================

fn testFailAndCancelLifecycle()! {
    let slots: [4]DelegationRecord = [DelegationRecord{}; 4];
    let slots_s: []DelegationRecord = slots[0..];
    let mgr = A2aDelegationManager.init(slots_s) ?? return error.InitFailed;

    const tok401 = mgr.delegate(401, 10, 20, 2000, 8000, 1000) ?? return error.DelegateFailed;
    const tok402 = mgr.delegate(402, 11, 21, 2000, 8000, 1000) ?? return error.DelegateFailed;

    // Explicit fail with valid fence_token
    expect_eq(mgr.fail(401, tok401, "upstream 503", 1500), true);
    const r401 = mgr.get(401) ?? return error.Missing;
    expect_eq(r401.state == DelegationState.Failed, true);
    expect_eq(std.mem.eql(u8, r401.result_summary, "upstream 503"), true);
    // Cannot release active task, can release failed task
    expect_eq(mgr.releaseTerminal(402), false);
    expect_eq(mgr.releaseTerminal(401), true);

    // Cancel rotates fence_token and blocks late completion
    expect_eq(mgr.cancel(402, 1600), true);
    const r402 = mgr.get(402) ?? return error.Missing;
    expect_eq(r402.state == DelegationState.Cancelled, true);
    expect_eq(r402.fence_token > tok402, true);
    expect_eq(mgr.complete(402, tok402, "late write", 1700), false);
    expect_eq(mgr.cancel(402, 1800), false);
}

// ============================================================================
// Test 5: Duplicate Task ID, Full Slots & Invalid Init/Parameter Guards
// ============================================================================

fn testDuplicateFullAndInvalidGuards()! {
    let buf: [2]DelegationRecord = [DelegationRecord{}; 2];
    let empty: []DelegationRecord = buf[0..0];
    let slots_s: []DelegationRecord = buf[0..];

    // Empty slots init -> null
    expect_eq(A2aDelegationManager.init(empty) == null, true);

    let mgr = A2aDelegationManager.init(slots_s) ?? return error.InitFailed;

    // Invalid delegation arguments -> null
    expect_eq(mgr.delegate(0, 10, 20, 1000, 5000, 1000) == null, true);
    expect_eq(mgr.delegate(501, 0, 20, 1000, 5000, 1000) == null, true);
    expect_eq(mgr.delegate(501, 10, 20, 0, 5000, 1000) == null, true);
    expect_eq(mgr.delegate(501, 10, 20, -100, 5000, 1000) == null, true);
    expect_eq(mgr.delegate(501, 10, 20, 1000, 1000, 1000) == null, true);
    expect_eq(mgr.delegate(501, 10, 20, 1000, 900, 1000) == null, true);

    // Fill 2 slots
    expect_eq(mgr.delegate(501, 10, 20, 1000, 5000, 1000) != null, true);
    // Duplicate task_id -> null
    expect_eq(mgr.delegate(501, 11, 21, 1000, 5000, 1000) == null, true);
    expect_eq(mgr.delegate(502, 11, 21, 1000, 5000, 1000) != null, true);
    // Table full -> null
    expect_eq(mgr.delegate(503, 12, 22, 1000, 5000, 1000) == null, true);
}

// ============================================================================
// Test 6: Clock Boundaries, Backwards Clock Skew & i64 Saturation
// ============================================================================

fn testClockBoundariesAndSaturation()! {
    let slots: [2]DelegationRecord = [DelegationRecord{}; 2];
    let slots_s: []DelegationRecord = slots[0..];
    let mgr = A2aDelegationManager.init(slots_s) ?? return error.InitFailed;

    // Huge lease_ms saturates cleanly and caps to deadline_ms
    const tok601 = mgr.delegate(601, 10, 0, 9223372036854775807, 5000, 1000) ?? return error.DelegateFailed;
    const r601 = mgr.get(601) ?? return error.Missing;
    expect_eq(r601.lease_expires_ms, 5000);

    // Backwards clock skew on renewLease does not shrink lease_expires_ms
    expect_eq(mgr.renewLease(601, tok601, 500, 500), true);
    expect_eq((mgr.get(601) ?? return error.Missing).lease_expires_ms, 5000);

    // Exact expiry boundary (now_ms == deadline_ms) rejects renewLease and complete even before sweep
    expect_eq(mgr.renewLease(601, tok601, 1000, 5000), false);
    expect_eq(mgr.complete(601, tok601, "at exact deadline", 5000), false);
    expect_eq(mgr.renewLease(601, tok601, 0, 2000), false);
}

test("a2a delegation: normal delegate, deadline-capped renew, complete & slot reuse", testNormalDelegationRenewCompleteAndReuse);
test("a2a delegation: lease timeout failover & stale fencing token write rejection", testFailoverAndStaleFenceRejection);
test("a2a delegation: hard deadline escalation & no-backup / backup timeout", testDeadlineEscalationAndTimeout);
test("a2a delegation: explicit fail, cancel & post-cancel fence invalidation", testFailAndCancelLifecycle);
test("a2a delegation: duplicate task_id, full slots & invalid init/arg guards", testDuplicateFullAndInvalidGuards);
test("a2a delegation: clock boundaries, backwards skew & i64 saturation", testClockBoundariesAndSaturation);
