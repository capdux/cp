// Origin: Pure-CP Stdlib Test Suite (lib/sys/workQueue.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Multi-Priority Aging Work Queue & Backpressure Scheduler:
//   1. Strict multi-priority ordering (3 > 2 > 1 > 0), FIFO tie-breaking & read-only peekNext
//   2. Starvation-free priority aging (effective priority boost + promotion telemetry + disabled aging)
//   3. All 3 full-queue backpressure policies (reject_new, drop_lowest_priority, drop_oldest)
//   4. Exact task cancellation (cancelTask), lazy skip on dequeue & cancelled-slot reclamation
//   5. Deadline expiration skipping on dequeue, batch sweepExpired & already-expired rejection
//   6. Monotonic clock rollback protection (no u64 underflow, monotonic aging & expiry)
//   7. Full boundary guards (invalid config, task_id==0, duplicate ID, priority>3, 128-slot stress) & cap.sys facade

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    WORK_QUEUE_MAX_CAPACITY,
    WORK_QUEUE_MAX_PRIORITY,
    WorkBackpressurePolicy,
    WorkEnqueueStatus,
    WorkQueueConfig,
    WorkQueue,
} from "cap:sys";

// ============================================================================
// Test 1: Priority Ordering, Same-Priority FIFO & Read-Only peekNext
// ============================================================================
test("workQueue: strict priority ordering (3 > 2 > 1 > 0) with FIFO tie-breaking and peekNext", () => {
    const cfg = WorkQueueConfig{
        capacity: 16,
        aging_step_ms: 0, // disable aging to test pure priority + FIFO
        backpressure: WorkBackpressurePolicy.reject_new,
    };
    let q = WorkQueue.init(cfg) ?? return;
    expect_true(q.isEmpty());
    expect_eq(q.capacity(), 16);

    expect_true(q.enqueue(10, 1, 100, 0, 1000).isAccepted());
    expect_true(q.enqueue(11, 3, 100, 0, 1001).isAccepted());
    expect_true(q.enqueue(12, 0, 100, 0, 1002).isAccepted());
    expect_true(q.enqueue(13, 3, 100, 0, 1003).isAccepted());
    expect_true(q.enqueue(14, 2, 100, 0, 1004).isAccepted());
    expect_true(q.enqueue(15, 1, 100, 0, 1005).isAccepted());
    expect_eq(q.len(), 6);

    // peekNext does not mutate queue
    if (p := q.peekNext(100)) {
        expect_eq(p.task.task_id, 11);
        expect_eq(p.effective_priority, 3);
    } else {
        expect_true(false);
    }
    expect_eq(q.len(), 6);

    // Expected dequeue order: 11 (p3), 13 (p3), 14 (p2), 10 (p1), 15 (p1), 12 (p0)
    const expected_ids: [6]u64 = [11, 13, 14, 10, 15, 12];
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        if (item := q.dequeueNext(100)) {
            expect_eq(item.task.task_id, expected_ids[i]);
        } else {
            expect_true(false);
        }
    }
    expect_true(q.isEmpty());
    if (empty_item := q.dequeueNext(100)) {
        * = empty_item;
        expect_true(false);
    }
});

// ============================================================================
// Test 2: Starvation-Free Priority Aging
// ============================================================================
test("workQueue: starvation aging boosts waiting low-priority tasks above fresh high-priority tasks", () => {
    const cfg = WorkQueueConfig{
        capacity: 16,
        aging_step_ms: 500,
        backpressure: WorkBackpressurePolicy.reject_new,
    };
    let q = WorkQueue.init(cfg) ?? return;

    // Enqueue low-priority (0) task at t=1000
    expect_true(q.enqueue(100, 0, 1000, 0, 1).isAccepted());
    // Enqueue high-priority (2) task at t=2400
    expect_true(q.enqueue(200, 2, 2400, 0, 2).isAccepted());

    // At t=2500: task 100 waited 1500ms -> +3 boost -> effective priority = 3 > task 200 (eff 2)
    if (first := q.dequeueNext(2500)) {
        expect_eq(first.task.task_id, 100);
        expect_eq(first.task.priority, 0);
        expect_eq(first.effective_priority, WORK_QUEUE_MAX_PRIORITY);
        expect_eq(first.wait_ms, 1500);
    } else {
        expect_true(false);
    }

    if (second := q.dequeueNext(2500)) {
        expect_eq(second.task.task_id, 200);
        expect_eq(second.effective_priority, 2);
        expect_eq(second.wait_ms, 100);
    } else {
        expect_true(false);
    }

    const st = q.stats();
    expect_eq(st.total_aged_promotions, 1);
    expect_eq(st.total_dequeued, 2);
});

// ============================================================================
// Test 3: All 3 Full-Queue Backpressure Policies
// ============================================================================
test("workQueue: reject_new, drop_lowest_priority, and drop_oldest backpressure policies", () => {
    // 3A. reject_new
    const cfg_rej = WorkQueueConfig{
        capacity: 2,
        aging_step_ms: 0,
        backpressure: WorkBackpressurePolicy.reject_new,
    };
    let q_rej = WorkQueue.init(cfg_rej) ?? return;
    expect_eq(q_rej.enqueue(1, 0, 10, 0, 1).status, WorkEnqueueStatus.accepted);
    expect_eq(q_rej.enqueue(2, 1, 20, 0, 2).status, WorkEnqueueStatus.accepted);
    expect_true(q_rej.isFull());
    const out_rej = q_rej.enqueue(3, 3, 30, 0, 3);
    expect_eq(out_rej.status, WorkEnqueueStatus.rejected_full);
    expect_eq(out_rej.evicted_task_id, 0);
    expect_eq(q_rej.stats().total_rejected, 1);

    // 3B. drop_lowest_priority
    const cfg_low = WorkQueueConfig{
        capacity: 2,
        aging_step_ms: 0,
        backpressure: WorkBackpressurePolicy.drop_lowest_priority,
    };
    let q_low = WorkQueue.init(cfg_low) ?? return;
    expect_true(q_low.enqueue(10, 1, 100, 0, 1).isAccepted());
    expect_true(q_low.enqueue(20, 2, 110, 0, 2).isAccepted());

    // Incoming lower priority (0 < min 1) must be rejected without evicting higher-priority work
    const out_low_rej = q_low.enqueue(30, 0, 120, 0, 3);
    expect_eq(out_low_rej.status, WorkEnqueueStatus.rejected_full);
    expect_eq(out_low_rej.evicted_task_id, 0);

    // Incoming higher priority (3 >= min 1) evicts task 10
    const out_low_ev = q_low.enqueue(40, 3, 130, 0, 4);
    expect_eq(out_low_ev.status, WorkEnqueueStatus.accepted_after_evict);
    expect_eq(out_low_ev.evicted_task_id, 10);
    expect_eq(q_low.stats().total_evicted, 1);

    // 3C. drop_oldest
    const cfg_old = WorkQueueConfig{
        capacity: 2,
        aging_step_ms: 0,
        backpressure: WorkBackpressurePolicy.drop_oldest,
    };
    let q_old = WorkQueue.init(cfg_old) ?? return;
    expect_true(q_old.enqueue(101, 3, 100, 0, 1).isAccepted());
    expect_true(q_old.enqueue(102, 1, 200, 0, 2).isAccepted());
    // Even lower-priority incoming task evicts oldest (task 101 at t=100) under drop_oldest
    const out_old_ev = q_old.enqueue(103, 0, 300, 0, 3);
    expect_eq(out_old_ev.status, WorkEnqueueStatus.accepted_after_evict);
    expect_eq(out_old_ev.evicted_task_id, 101);
    expect_eq(q_old.len(), 2);
});

// ============================================================================
// Test 4: Exact Cancellation, Dequeue Skipping & Cancelled Slot Reclamation
// ============================================================================
test("workQueue: cancelTask skips cancelled items on dequeue and reclaims slots when full", () => {
    const cfg = WorkQueueConfig{
        capacity: 2,
        aging_step_ms: 0,
        backpressure: WorkBackpressurePolicy.reject_new,
    };
    let q = WorkQueue.init(cfg) ?? return;

    expect_true(q.enqueue(50, 3, 100, 0, 500).isAccepted());
    expect_true(q.enqueue(51, 1, 110, 0, 501).isAccepted());

    // Cancel task 50; second cancel returns false; non-existent returns false
    expect_true(q.cancelTask(50));
    expect_true(!q.cancelTask(50));
    expect_true(!q.cancelTask(999));
    expect_true(!q.cancelTask(0));
    expect_eq(q.len(), 1);

    // Even under reject_new with capacity 2, enqueueing task 52 reclaims cancelled slot 50 directly!
    const out_reuse = q.enqueue(52, 2, 120, 0, 502);
    expect_eq(out_reuse.status, WorkEnqueueStatus.accepted);
    expect_eq(out_reuse.evicted_task_id, 0);
    expect_eq(q.len(), 2);

    // Dequeue returns 52 (p2), then 51 (p1); cancelled task 50 never appears
    if (d1 := q.dequeueNext(130)) {
        expect_eq(d1.task.task_id, 52);
    } else {
        expect_true(false);
    }
    if (d2 := q.dequeueNext(130)) {
        expect_eq(d2.task.task_id, 51);
    } else {
        expect_true(false);
    }
    expect_eq(q.stats().total_cancelled, 1);
});

// ============================================================================
// Test 5: Deadline Expiration Skipping & Batch sweepExpired
// ============================================================================
test("workQueue: expired tasks are skipped on dequeue and cleaned by sweepExpired", () => {
    const cfg = WorkQueueConfig{
        capacity: 8,
        aging_step_ms: 0,
        backpressure: WorkBackpressurePolicy.reject_new,
    };
    let q = WorkQueue.init(cfg) ?? return;

    // Enqueue tasks with various deadlines (0 = never expires)
    expect_true(q.enqueue(1, 3, 1000, 1500, 10).isAccepted()); // expires at 1500
    expect_true(q.enqueue(2, 2, 1000, 2000, 20).isAccepted()); // expires at 2000
    expect_true(q.enqueue(3, 1, 1000, 0, 30).isAccepted());    // never expires
    expect_true(q.enqueue(4, 3, 1000, 1800, 40).isAccepted()); // expires at 1800

    // Already-expired deadline at enqueue time is rejected immediately
    const late = q.enqueue(5, 3, 1600, 1500, 50);
    expect_eq(late.status, WorkEnqueueStatus.rejected_already_expired);

    // At t=1600: task 1 (deadline 1500) is expired and skipped; task 4 (p3, deadline 1800) wins
    if (d1 := q.dequeueNext(1600)) {
        expect_eq(d1.task.task_id, 4);
    } else {
        expect_true(false);
    }
    expect_eq(q.stats().total_expired, 1);

    // At t=2500: batch sweepExpired cleans task 2 (deadline 2000), leaves task 3 (deadline 0)
    const swept: usize = q.sweepExpired(2500);
    expect_eq(swept, 1);
    expect_eq(q.len(), 1);
    expect_eq(q.stats().total_expired, 2);

    if (d2 := q.dequeueNext(3000)) {
        expect_eq(d2.task.task_id, 3);
    } else {
        expect_true(false);
    }
});

// ============================================================================
// Test 6: Monotonic Clock Rollback Guard
// ============================================================================
test("workQueue: clock rollback never causes underflow or reverses aging/expiration", () => {
    const cfg = WorkQueueConfig{
        capacity: 8,
        aging_step_ms: 500,
        backpressure: WorkBackpressurePolicy.reject_new,
    };
    let q = WorkQueue.init(cfg) ?? return;

    expect_true(q.enqueue(10, 0, 5000, 6000, 1).isAccepted());
    expect_true(q.enqueue(11, 0, 5000, 0, 2).isAccepted());

    // Advance clock to 6500 (task 10 is now expired; task 11 waited 1500ms -> eff prio 3)
    const swept: usize = q.sweepExpired(6500);
    expect_eq(swept, 1);

    // Now pass a regressed timestamp 2000 (< 6500): high-water mark stays 6500!
    if (item := q.dequeueNext(2000)) {
        expect_eq(item.task.task_id, 11);
        expect_eq(item.effective_priority, 3);
        expect_eq(item.wait_ms, 1500);
    } else {
        expect_true(false);
    }
    expect_eq(q.stats().last_clock_ms, 6500);
});

// ============================================================================
// Test 7: Boundary Guards, Full 128-Slot Capacity & cap.sys Facade
// ============================================================================
test("workQueue: boundary guards, 128-slot stress, clear() and cap.sys facade", () => {
    // Invalid configs rejected
    if (bad0 := WorkQueue.init(WorkQueueConfig{ capacity: 0, aging_step_ms: 100, backpressure: WorkBackpressurePolicy.reject_new })) {
        * = bad0;
        expect_true(false);
    }
    if (bad1 := WorkQueue.init(WorkQueueConfig{ capacity: WORK_QUEUE_MAX_CAPACITY + 1, aging_step_ms: 100, backpressure: WorkBackpressurePolicy.reject_new })) {
        * = bad1;
        expect_true(false);
    }

    // Construct via cap.sys facade with full 128 capacity
    let q = cap.sys.WorkQueue.init(WorkQueueConfig.defaults()) ?? return;

    // Invalid task_id == 0 and priority > 3
    expect_eq(q.enqueue(0, 1, 100, 0, 1).status, WorkEnqueueStatus.rejected_invalid_id);
    expect_eq(q.enqueue(1, 4, 100, 0, 1).status, WorkEnqueueStatus.rejected_invalid_priority);

    // Fill all 128 slots and test duplicate ID rejection
    let i: u64 = 1;
    while (i <= 128) : (i += 1) {
        const prio: u8 = (i % 4) as u8;
        expect_eq(q.enqueue(i, prio, 1000 + i, 0, i * 10).status, WorkEnqueueStatus.accepted);
    }
    expect_true(q.isFull());
    expect_eq(q.len(), 128);
    expect_eq(q.enqueue(42, 2, 2000, 0, 999).status, WorkEnqueueStatus.rejected_duplicate_id);

    // findTask & clear
    if (found := q.findTask(42)) {
        expect_eq(found.payload_id, 420);
    } else {
        expect_true(false);
    }
    q.clear();
    expect_true(q.isEmpty());
    expect_eq(q.len(), 0);
});
