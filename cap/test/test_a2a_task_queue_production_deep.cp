// Origin: Pure-CP Stdlib Test Suite (lib/a2a/task_queue.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation A2A task lifecycle state machine
// and lease priority queue: idempotency-key deduplication, priority + earliest-deadline
// scheduling, lease heartbeat renewal & expired-lease reclamation, max-attempts DeadLetter
// escalation, InputRequired/resumeInput flow, and legal/illegal state transition enforcement.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    A2aTaskState,
    canTransition,
    A2aTaskItem,
    A2aQueueStats,
    A2aTaskQueue,
} from "cap:a2a";

fn testIdempotencyDeduplicationAndPoolLimit()! {
    let slots_arr: [2]A2aTaskItem = undefined;
    let slots: []A2aTaskItem = slots_arr[0..];
    let q = A2aTaskQueue.init(slots);

    // 1. Enqueue with idempotency key 0x1234
    if (t1 := q.enqueue(100, 0x1234, 5, 1000, 5000, 3)) {
        expect_eq(t1, 100);
    } else {
        expect_eq(true, false);
    }

    // 2. Duplicate idempotency key with different proposed task_id returns original task_id (100)
    if (t_dup := q.enqueue(999, 0x1234, 99, 1010, 2000, 3)) {
        expect_eq(t_dup, 100);
    } else {
        expect_eq(true, false);
    }
    expect_eq(q.stats().total_tasks, 1);

    // 3. Duplicate task_id with different idempotency key rejected
    expect_eq(q.enqueue(100, 0x5678, 5, 1020, 5000, 3) == null, true);

    // 4. Fill 2nd slot, 3rd enqueue fails (pool full)
    expect_eq(q.enqueue(101, 0, 1, 1030, 5000, 3) != null, true);
    expect_eq(q.enqueue(102, 0, 1, 1040, 5000, 3) == null, true);
}

fn testPriorityAndEarliestDeadlineScheduling()! {
    let slots_arr: [4]A2aTaskItem = undefined;
    let slots: []A2aTaskItem = slots_arr[0..];
    let q = A2aTaskQueue.init(slots);

    // Enqueue out of order:
    //  id 1: priority 10, deadline 5000
    //  id 2: priority 20, deadline 9000 (highest priority -> 1st)
    //  id 3: priority 10, deadline 3000 (same priority 10, earlier DDL -> 2nd)
    //  id 4: priority 10, deadline 0    (no DDL -> after DDL tasks -> 4th)
    * = q.enqueue(1, 0, 10, 100, 5000, 3);
    * = q.enqueue(2, 0, 20, 100, 9000, 3);
    * = q.enqueue(3, 0, 10, 100, 3000, 3);
    * = q.enqueue(4, 0, 10, 100, 0, 3);

    expect_eq(q.claimNext(1, 200, 1000) ?? 0, 2);
    expect_eq(q.claimNext(2, 200, 1000) ?? 0, 3);
    expect_eq(q.claimNext(3, 200, 1000) ?? 0, 1);
    expect_eq(q.claimNext(4, 200, 1000) ?? 0, 4);
    expect_eq(q.claimNext(5, 200, 1000) == null, true);
}

fn testLeaseHeartbeatRenewalAndExpiredReclamation()! {
    let slots_arr: [4]A2aTaskItem = undefined;
    let slots: []A2aTaskItem = slots_arr[0..];
    let q = A2aTaskQueue.init(slots);

    * = q.enqueue(10, 0, 5, 1000, 0, 3);
    * = q.enqueue(11, 0, 5, 1000, 0, 3);

    // Worker 1 claims task 10 (lease until 2000), Worker 2 claims task 11 (lease until 2000)
    expect_eq(q.claimNext(1, 1000, 1000) ?? 0, 10);
    expect_eq(q.claimNext(2, 1000, 1000) ?? 0, 11);

    // Wrong worker cannot heartbeat task 10
    expect_eq(q.heartbeat(10, 2, 1500, 1000), false);
    // Worker 1 renews lease on task 10 until 1500 + 1500 = 3000
    expect_eq(q.heartbeat(10, 1, 1500, 1500), true);

    // At t=2500: task 11 lease (2000) expired -> reclaimed to Submitted; task 10 still Working
    const reclaimed = q.reclaimExpiredLeases(2500);
    expect_eq(reclaimed, 1);

    if (t10 := q.findTask(10)) {
        expect_eq(t10.state == A2aTaskState.Working, true);
        expect_eq(t10.worker_id, 1);
    } else {
        expect_eq(true, false);
    }
    if (t11 := q.findTask(11)) {
        expect_eq(t11.state == A2aTaskState.Submitted, true);
        expect_eq(t11.worker_id, 0);
        expect_eq(t11.attempts, 1);
    } else {
        expect_eq(true, false);
    }
}

fn testMaxAttemptsDeadLetterEscalation()! {
    let slots_arr: [4]A2aTaskItem = undefined;
    let slots: []A2aTaskItem = slots_arr[0..];
    let q = A2aTaskQueue.init(slots);

    // Task 50 has max_attempts = 2
    * = q.enqueue(50, 0, 10, 100, 0, 2);

    // Attempt 1: claimed at t=100 (lease until 500), expires at t=600 -> requeued to Submitted
    expect_eq(q.claimNext(1, 100, 400) ?? 0, 50);
    expect_eq(q.reclaimExpiredLeases(600), 1);
    if (t1 := q.findTask(50)) {
        expect_eq(t1.state == A2aTaskState.Submitted, true);
        expect_eq(t1.attempts, 1);
    } else {
        expect_eq(true, false);
    }

    // Attempt 2: claimed at t=700 (lease until 1100), expires at t=1200 -> DeadLetter!
    expect_eq(q.claimNext(2, 700, 400) ?? 0, 50);
    expect_eq(q.reclaimExpiredLeases(1200), 1);
    if (t2 := q.findTask(50)) {
        expect_eq(t2.state == A2aTaskState.DeadLetter, true);
        expect_eq(t2.attempts, 2);
    } else {
        expect_eq(true, false);
    }
    expect_eq(q.stats().dead_letter, 1);

    // Task 51: retryable fail also escalates to DeadLetter on final attempt
    * = q.enqueue(51, 0, 10, 100, 0, 1);
    expect_eq(q.claimNext(3, 1300, 400) ?? 0, 51);
    expect_eq(q.fail(51, 3, true), true);
    if (t51 := q.findTask(51)) {
        expect_eq(t51.state == A2aTaskState.DeadLetter, true);
    } else {
        expect_eq(true, false);
    }
}

fn testStateTransitionsInputResumeAndNamespace()! {
    // 1. Direct state transition matrix validation
    expect_eq(canTransition(A2aTaskState.Submitted, A2aTaskState.Working), true);
    expect_eq(canTransition(A2aTaskState.Submitted, A2aTaskState.Completed), false);
    expect_eq(canTransition(A2aTaskState.Working, A2aTaskState.InputRequired), true);
    expect_eq(canTransition(A2aTaskState.InputRequired, A2aTaskState.Submitted), true);
    expect_eq(canTransition(A2aTaskState.Completed, A2aTaskState.Canceled), false);
    expect_eq(canTransition(A2aTaskState.DeadLetter, A2aTaskState.Submitted), false);

    // 2. Full InputRequired -> resumeInput -> claim -> complete flow via cap.a2a namespace
    let slots_arr: [4]cap.a2a.A2aTaskItem = undefined;
    let slots: []cap.a2a.A2aTaskItem = slots_arr[0..];
    let q = cap.a2a.A2aTaskQueue.init(slots);

    * = q.enqueue(70, 0xAAAA, 5, 100, 0, 3);
    * = q.enqueue(71, 0xBBBB, 1, 100, 0, 3);
    expect_eq(q.claimNext(1, 200, 1000) ?? 0, 70);

    // Suspend for human input
    expect_eq(q.requestInput(70, 1), true);
    // Cannot complete while InputRequired
    expect_eq(q.complete(70, 1, 1), false);
    // Resume back to Submitted, re-claim, then complete with 3 artifacts
    expect_eq(q.resumeInput(70), true);
    expect_eq(q.claimNext(2, 300, 1000) ?? 0, 70);
    expect_eq(q.complete(70, 2, 3), true);

    // Terminal Completed task cannot be canceled or failed
    expect_eq(q.cancel(70), false);
    expect_eq(q.fail(70, 2, false), false);

    // Cancel Submitted task 71 succeeds
    expect_eq(q.cancel(71), true);

    const st = q.stats();
    expect_eq(st.total_tasks, 2);
    expect_eq(st.completed, 1);
    expect_eq(st.canceled, 1);
}

pub fn main()! {
    test("1. a2a/task_queue: idempotency-key deduplication and pool limit", () => {
        testIdempotencyDeduplicationAndPoolLimit()?;
    });
    test("2. a2a/task_queue: priority and earliest-deadline claimNext ordering", () => {
        testPriorityAndEarliestDeadlineScheduling()?;
    });
    test("3. a2a/task_queue: lease heartbeat renewal and expired-lease reclamation", () => {
        testLeaseHeartbeatRenewalAndExpiredReclamation()?;
    });
    test("4. a2a/task_queue: max_attempts exhaustion escalates to DeadLetter", () => {
        testMaxAttemptsDeadLetterEscalation()?;
    });
    test("5. a2a/task_queue: legal/illegal state transitions, InputRequired & cap.a2a", () => {
        testStateTransitionsInputResumeAndNamespace()?;
    });
}
