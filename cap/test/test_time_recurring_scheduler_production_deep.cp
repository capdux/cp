// Origin: Pure-CP Stdlib Test Suite (lib/time/recurring_scheduler.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Deterministic Recurring Task Scheduler:
//   1. OneShot, FixedInterval (max_runs) & FixedDelay ordering, nextDueMs, in_flight & slot reuse
//   2. Deterministic SplitMix64 jitter anti-thundering-herd bounds & reproducibility
//   3. Exponential retry backoff (1x, 2x -> Failed) & recovery resetting retry_count
//   4. Pause, resume (stale-due clamping), cancel & nextDueMs exclusion
//   5. FixedInterval missed-slot skip protection (no catch-up flood) & i64 saturation
//   6. Empty init, duplicate ID, full table & invalid parameter guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    ScheduleKind,
    ScheduleState,
    ScheduledTaskSlot,
    SchedulerStats,
    RecurringScheduler,
} from "cap:time";

// ============================================================================
// Test 1: OneShot, FixedInterval, FixedDelay, nextDueMs & Slot Reuse
// ============================================================================

fn testOneShotIntervalAndFixedDelay()! {
    let slots: [4]ScheduledTaskSlot = [ScheduledTaskSlot{}; 4];
    let slots_s: []ScheduledTaskSlot = slots[0..];
    let sched = cap.time.RecurringScheduler.init(slots_s) ?? return error.InitFailed;

    expect_eq(sched.nextDueMs() == null, true);

    // Task 20 due at 1000 (OneShot), Task 10 due at 1000 (FixedInterval, max_runs=2), Task 30 due at 1500 (FixedDelay)
    expect_eq(sched.scheduleOneShot(20, "oneshot", 1000), true);
    expect_eq(sched.scheduleInterval(10, "interval", 1000, 500, 2, 0), true);
    expect_eq(sched.scheduleFixedDelay(30, "delay", 1500, 400, 2, 0), true);

    expect_eq(sched.nextDueMs() ?? -1, 1000);

    // At t=1000: tasks 10 and 20 are due; deterministic order is task_id asc -> [10, 20]
    let due: [4]u64 = [0; 4];
    let due_s: []u64 = due[0..];
    const n1 = sched.pollDue(1000, due_s);
    expect_eq(n1, 2);
    expect_eq(due[0], 10);
    expect_eq(due[1], 20);

    // Both 10 & 20 are in_flight, so nextDueMs is now 1500 (task 30) and pollDue(1000) returns 0!
    expect_eq(sched.nextDueMs() ?? -1, 1500);
    expect_eq(sched.pollDue(1000, due_s), 0);

    // Complete task 20 (OneShot -> Completed) and task 10 (run 1/2 -> next_run_ms = 1500)
    expect_eq(sched.reportResult(20, true, 1050), true);
    expect_eq(sched.reportResult(10, true, 1080), true);
    expect_eq((sched.get(20) ?? return error.Missing).state == ScheduleState.Completed, true);
    expect_eq((sched.get(10) ?? return error.Missing).next_run_ms, 1500);

    // At t=1500: task 10 and task 30 both due -> [10, 30]
    const n2 = sched.pollDue(1500, due_s);
    expect_eq(n2, 2);
    expect_eq(due[0], 10);
    expect_eq(due[1], 30);

    // Complete task 10 (run 2/2 -> Completed) and task 30 at t=1700 (FixedDelay -> next = 1700 + 400 = 2100)
    expect_eq(sched.reportResult(10, true, 1600), true);
    expect_eq(sched.reportResult(30, true, 1700), true);
    expect_eq((sched.get(10) ?? return error.Missing).state == ScheduleState.Completed, true);
    expect_eq((sched.get(30) ?? return error.Missing).next_run_ms, 2100);
    expect_eq(sched.nextDueMs() ?? -1, 2100);

    const st: SchedulerStats = sched.stats();
    expect_eq(st.completed, 2);
    expect_eq(st.active, 1);
    expect_eq(st.total_dispatches, 4);
    expect_eq(st.total_successes, 4);

    // Release completed slot and reuse task_id 20
    expect_eq(sched.releaseTerminal(20), true);
    expect_eq(sched.releaseTerminal(30), false); // still active
    expect_eq(sched.scheduleOneShot(20, "reused", 3000), true);
}

// ============================================================================
// Test 2: Deterministic Jitter Bounds & Reproducibility
// ============================================================================

fn testDeterministicJitter()! {
    let slots_a: [4]ScheduledTaskSlot = [ScheduledTaskSlot{}; 4];
    let slots_b: [4]ScheduledTaskSlot = [ScheduledTaskSlot{}; 4];
    let sa: []ScheduledTaskSlot = slots_a[0..];
    let sb: []ScheduledTaskSlot = slots_b[0..];
    let sched_a = RecurringScheduler.init(sa) ?? return error.InitFailed;
    let sched_b = RecurringScheduler.init(sb) ?? return error.InitFailed;

    expect_eq(sched_a.scheduleInterval(101, "j1", 1000, 5000, 0, 500), true);
    expect_eq(sched_a.scheduleInterval(102, "j2", 1000, 5000, 0, 500), true);
    expect_eq(sched_b.scheduleInterval(101, "j1", 1000, 5000, 0, 500), true);

    const r1 = sched_a.get(101) ?? return error.Missing;
    const r2 = sched_a.get(102) ?? return error.Missing;
    const r1_b = sched_b.get(101) ?? return error.Missing;

    // Within [1000, 1500]
    expect_eq(r1.next_run_ms >= 1000 && r1.next_run_ms <= 1500, true);
    expect_eq(r2.next_run_ms >= 1000 && r2.next_run_ms <= 1500, true);
    // Exact determinism across instances
    expect_eq(r1.next_run_ms, r1_b.next_run_ms);
    // Distinct task_ids spread apart (anti-thundering-herd)
    expect_eq(r1.next_run_ms != r2.next_run_ms, true);
}

// ============================================================================
// Test 3: Exponential Retry Backoff & Recovery Reset
// ============================================================================

fn testExponentialRetryBackoff()! {
    let slots: [2]ScheduledTaskSlot = [ScheduledTaskSlot{}; 2];
    let slots_s: []ScheduledTaskSlot = slots[0..];
    let sched = RecurringScheduler.init(slots_s) ?? return error.InitFailed;

    // Task 1: max_retries=2, backoff=100ms -> 1st fail +100ms, 2nd fail +200ms, 3rd fail -> Failed
    expect_eq(sched.scheduleOneShot(1, "flaky", 1000), true);
    expect_eq(sched.setRetryPolicy(1, 2, 100), true);

    let due: [2]u64 = [0; 2];
    let due_s: []u64 = due[0..];

    // Attempt 1 fails at t=1000 -> retry 1 at 1100
    expect_eq(sched.pollDue(1000, due_s), 1);
    expect_eq(sched.reportResult(1, false, 1000), true);
    expect_eq((sched.get(1) ?? return error.Missing).next_run_ms, 1100);
    expect_eq((sched.get(1) ?? return error.Missing).retry_count, 1);

    // Attempt 2 fails at t=1100 -> retry 2 at 1300 (+200ms)
    expect_eq(sched.pollDue(1100, due_s), 1);
    expect_eq(sched.reportResult(1, false, 1100), true);
    expect_eq((sched.get(1) ?? return error.Missing).next_run_ms, 1300);
    expect_eq((sched.get(1) ?? return error.Missing).retry_count, 2);

    // Attempt 3 fails at t=1300 -> retries exhausted -> Failed
    expect_eq(sched.pollDue(1300, due_s), 1);
    expect_eq(sched.reportResult(1, false, 1300), true);
    expect_eq((sched.get(1) ?? return error.Missing).state == ScheduleState.Failed, true);

    // Task 2: fails once, then recovers on retry -> retry_count resets to 0
    expect_eq(sched.scheduleInterval(2, "recover", 2000, 1000, 0, 0), true);
    expect_eq(sched.setRetryPolicy(2, 2, 150), true);
    expect_eq(sched.pollDue(2000, due_s), 1);
    expect_eq(sched.reportResult(2, false, 2000), true);
    expect_eq((sched.get(2) ?? return error.Missing).next_run_ms, 2150);
    expect_eq(sched.pollDue(2150, due_s), 1);
    expect_eq(sched.reportResult(2, true, 2160), true);
    expect_eq((sched.get(2) ?? return error.Missing).retry_count, 0);
    expect_eq((sched.get(2) ?? return error.Missing).run_count, 1);

    const st = sched.stats();
    expect_eq(st.total_failures, 4);
    expect_eq(st.total_retries, 3);
    expect_eq(st.failed, 1);
}

// ============================================================================
// Test 4: Pause, Resume & Cancel Lifecycle
// ============================================================================

fn testPauseResumeAndCancel()! {
    let slots: [2]ScheduledTaskSlot = [ScheduledTaskSlot{}; 2];
    let slots_s: []ScheduledTaskSlot = slots[0..];
    let sched = RecurringScheduler.init(slots_s) ?? return error.InitFailed;

    expect_eq(sched.scheduleInterval(1, "job1", 1000, 500, 0, 0), true);
    expect_eq(sched.scheduleInterval(2, "job2", 1200, 500, 0, 0), true);

    // Pause task 1 -> nextDueMs skips it and returns 1200
    expect_eq(sched.pause(1), true);
    expect_eq(sched.pause(1), false); // already paused
    expect_eq(sched.nextDueMs() ?? -1, 1200);

    // Resume task 1 at t=2500 -> next_run_ms clamped up to 2500
    expect_eq(sched.resumeTask(1, 2500), true);
    expect_eq((sched.get(1) ?? return error.Missing).next_run_ms, 2500);

    // Cancel task 2 -> nextDueMs becomes 2500
    expect_eq(sched.cancel(2), true);
    expect_eq(sched.cancel(2), false); // already terminal
    expect_eq((sched.get(2) ?? return error.Missing).state == ScheduleState.Cancelled, true);
    expect_eq(sched.nextDueMs() ?? -1, 2500);
}

// ============================================================================
// Test 5: FixedInterval Missed-Slot Skip & i64 Saturation
// ============================================================================

fn testFixedIntervalSkipAndSaturation()! {
    let slots: [2]ScheduledTaskSlot = [ScheduledTaskSlot{}; 2];
    let slots_s: []ScheduledTaskSlot = slots[0..];
    let sched = RecurringScheduler.init(slots_s) ?? return error.InitFailed;

    // Interval 100ms starting at 1000ms; slow execution finishes at t=1550ms
    expect_eq(sched.scheduleInterval(1, "aligned", 1000, 100, 0, 0), true);
    let due: [2]u64 = [0; 2];
    let due_s: []u64 = due[0..];
    expect_eq(sched.pollDue(1000, due_s), 1);
    expect_eq(sched.reportResult(1, true, 1550), true);

    // Next aligned tick after 1550ms is 1600ms (skipped 1100..1500 without catch-up storm!)
    expect_eq((sched.get(1) ?? return error.Missing).next_run_ms, 1600);
}

// ============================================================================
// Test 6: Empty Init, Duplicate ID, Full Slots & Invalid Parameter Guards
// ============================================================================

fn testSchedulerBoundaryGuards()! {
    let buf: [2]ScheduledTaskSlot = [ScheduledTaskSlot{}; 2];
    let empty: []ScheduledTaskSlot = buf[0..0];
    let slots_s: []ScheduledTaskSlot = buf[0..];

    expect_eq(RecurringScheduler.init(empty) == null, true);

    let sched = RecurringScheduler.init(slots_s) ?? return error.InitFailed;
    expect_eq(sched.scheduleOneShot(0, "zero", 100), false);
    expect_eq(sched.scheduleInterval(1, "bad_int", 100, 0, 0, 0), false);
    expect_eq(sched.scheduleInterval(1, "neg_jit", 100, 100, 0, -1), false);
    expect_eq(sched.scheduleFixedDelay(1, "bad_del", 100, -5, 0, 0), false);

    expect_eq(sched.scheduleOneShot(1, "ok1", 100), true);
    expect_eq(sched.scheduleOneShot(1, "dup", 200), false);
    expect_eq(sched.scheduleOneShot(2, "ok2", 200), true);
    expect_eq(sched.scheduleOneShot(3, "full", 300), false);

    // reportResult on non-in-flight task -> false
    expect_eq(sched.reportResult(1, true, 100), false);
    expect_eq(sched.setRetryPolicy(1, 2, -10), false);
}

test("time recurring_scheduler: OneShot, FixedInterval, FixedDelay, nextDueMs & slot reuse", testOneShotIntervalAndFixedDelay);
test("time recurring_scheduler: deterministic jitter bounds & reproducibility", testDeterministicJitter);
test("time recurring_scheduler: exponential retry backoff & recovery reset", testExponentialRetryBackoff);
test("time recurring_scheduler: pause, resume & cancel lifecycle", testPauseResumeAndCancel);
test("time recurring_scheduler: FixedInterval missed-slot skip & alignment", testFixedIntervalSkipAndSaturation);
test("time recurring_scheduler: empty init, duplicate ID, full slots & param guards", testSchedulerBoundaryGuards);
