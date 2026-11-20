// Source: cp/cap/test/test_queue_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:queue & cap.queue

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    QueueError,
    JobPriority,
    JobState,
    RetryPolicy,
    Job,
    QueueStats,
    JobQueue,
} from "cap:queue";

fn testPriorityOrderingAndFifo(): ! {
    const alloc = std.heap.page_allocator;
    let q = JobQueue.init(alloc, RetryPolicy.default(), 5000)?;
    defer q.deinit();

    // Ownership-safe string duplication: mutate caller stack buffers immediately after enqueue
    let topic_buf: [16]u8 = undefined;
    let payload_buf: [16]u8 = undefined;
    @memcpy(topic_buf[0..5], "alpha");
    @memcpy(payload_buf[0..5], "pay_1");
    const id_low_1 = q.enqueue(topic_buf[0..5], payload_buf[0..5], .Low, 100)?;
    @memcpy(topic_buf[0..5], "XXXXX");
    @memcpy(payload_buf[0..5], "YYYYY");

    const id_norm_1 = q.enqueue("beta", "norm_1", .Normal, 100)?;
    const id_high_1 = q.enqueue("gamma", "high_1", .High, 100)?;
    const id_crit_1 = q.enqueue("delta", "crit_1", .Critical, 100)?;
    const id_norm_2 = q.enqueue("beta", "norm_2", .Normal, 100)?;
    const id_crit_2 = q.enqueue("delta", "crit_2", .Critical, 100)?;
    const id_high_2 = q.enqueue("gamma", "high_2", .High, 100)?;
    const id_low_2 = q.enqueue("alpha", "low_2", .Low, 100)?;

    const st0 = q.stats();
    expect_eq(st0.pending, 8 as usize);
    expect_eq(st0.total_enqueued, 8 as u64);

    // Expected order: Critical(1,2) -> High(1,2) -> Normal(1,2) -> Low(1,2)
    const j1 = q.dequeue(105).?;
    expect_eq(j1.id, id_crit_1);
    expect_eq(j1.state == .Running, true);
    expect_eq(j1.lease_until_ms, 5105 as i64);
    q.ack(j1.id)?;

    const j2 = q.dequeue(106).?;
    expect_eq(j2.id, id_crit_2);
    q.ack(j2.id)?;

    const j3 = q.dequeue(107).?;
    expect_eq(j3.id, id_high_1);
    q.ack(j3.id)?;

    const j4 = q.dequeue(108).?;
    expect_eq(j4.id, id_high_2);
    q.ack(j4.id)?;

    const j5 = q.dequeue(109).?;
    expect_eq(j5.id, id_norm_1);
    q.ack(j5.id)?;

    const j6 = q.dequeue(110).?;
    expect_eq(j6.id, id_norm_2);
    q.ack(j6.id)?;

    const j7 = q.dequeue(111).?;
    expect_eq(j7.id, id_low_1);
    // Verify owned copies survived caller buffer overwrite
    expect_eq(std.mem.eql(u8, j7.topic, "alpha"), true);
    expect_eq(std.mem.eql(u8, j7.payload, "pay_1"), true);
    q.ack(j7.id)?;

    const j8 = q.dequeue(112).?;
    expect_eq(j8.id, id_low_2);
    q.ack(j8.id)?;

    expect_eq(q.dequeue(113) == null, true);

    const st1 = q.stats();
    expect_eq(st1.pending, 0 as usize);
    expect_eq(st1.running, 0 as usize);
    expect_eq(st1.completed, 8 as usize);

    // Invalid state & unknown id errors
    let got_not_found = false;
    q.ack(99999) catch |e| {
        if (e == @as(anyerror, QueueError.JobNotFound)) got_not_found = true;
    };
    expect_eq(got_not_found, true);

    let got_invalid_state = false;
    q.ack(id_low_1) catch |e| {
        if (e == @as(anyerror, QueueError.InvalidState)) got_invalid_state = true;
    };
    expect_eq(got_invalid_state, true);
}

fn testDelayedJobExactDispatch(): ! {
    const alloc = std.heap.page_allocator;
    let q = JobQueue.init(alloc, RetryPolicy.default(), 3000)?;
    defer q.deinit();

    // Immediate job via delay_ms <= 0
    const immediate_id = q.enqueueDelayed("fast", "now", .Low, 0, 1000)?;
    // Delayed jobs with different due timestamps and priorities
    const d_1500_low = q.enqueueDelayed("d1", "p1", .Low, 500, 1000)?;      // run_at = 1500
    const d_1500_crit = q.enqueueDelayed("d2", "p2", .Critical, 500, 1000)?; // run_at = 1500
    const d_2000_high = q.enqueueDelayed("d3", "p3", .High, 1000, 1000)?;    // run_at = 2000

    const st0 = q.stats();
    expect_eq(st0.pending, 1 as usize);
    expect_eq(st0.delayed, 3 as usize);

    // Immediate job dequeued at 1000
    const first = q.dequeue(1000).?;
    expect_eq(first.id, immediate_id);
    q.ack(first.id)?;

    // At 1499, none of the delayed jobs are due yet
    expect_eq(q.dequeue(1499) == null, true);
    expect_eq(q.stats().delayed, 3 as usize);

    // At exact 1500, both d_1500_low & d_1500_crit promote; Critical dispatches first!
    const due_1 = q.dequeue(1500).?;
    expect_eq(due_1.id, d_1500_crit);
    q.ack(due_1.id)?;

    const due_2 = q.dequeue(1500).?;
    expect_eq(due_2.id, d_1500_low);
    q.ack(due_2.id)?;

    // d_2000_high still waiting until 2000
    expect_eq(q.dequeue(1999) == null, true);
    const due_3 = q.dequeue(2000).?;
    expect_eq(due_3.id, d_2000_high);
    q.ack(due_3.id)?;

    const st1 = q.stats();
    expect_eq(st1.delayed, 0 as usize);
    expect_eq(st1.completed, 4 as usize);
}

fn testExponentialBackoffDlqAndRequeue(): ! {
    const alloc = std.heap.page_allocator;
    const policy = RetryPolicy.init(3, 1000, 3500);

    // Verify exponential backoff + capping + overflow safety
    expect_eq(policy.nextDelayMs(0), 1000 as i64);
    expect_eq(policy.nextDelayMs(1), 2000 as i64);
    expect_eq(policy.nextDelayMs(2), 3500 as i64);
    expect_eq(policy.nextDelayMs(100), 3500 as i64);

    let q = JobQueue.init(alloc, policy, 5000)?;
    defer q.deinit();

    const id = q.enqueue("llm.infer", "{\"prompt\":\"hi\"}", .High, 0)?;

    // Attempt 1 -> nack at t=10 -> Delayed until 10 + 1000 = 1010
    const run1 = q.dequeue(10).?;
    expect_eq(run1.id, id);
    expect_eq(run1.attempts, 0 as u32);
    q.nack(id, "rate limited 429", 10)?;

    let snap = q.getJob(id).?;
    expect_eq(snap.state == .Delayed, true);
    expect_eq(snap.attempts, 1 as u32);
    expect_eq(snap.run_at_ms, 1010 as i64);
    expect_eq(std.mem.eql(u8, snap.last_error, "rate limited 429"), true);
    expect_eq(q.dequeue(1009) == null, true);

    // Attempt 2 -> nack at t=1010 -> Delayed until 1010 + 2000 = 3010
    const run2 = q.dequeue(1010).?;
    expect_eq(run2.id, id);
    expect_eq(run2.attempts, 1 as u32);
    q.nack(id, "upstream 503", 1010)?;

    snap = q.getJob(id).?;
    expect_eq(snap.state == .Delayed, true);
    expect_eq(snap.attempts, 2 as u32);
    expect_eq(snap.run_at_ms, 3010 as i64);
    expect_eq(q.dequeue(3009) == null, true);

    // Attempt 3 (max_attempts = 3) -> nack at t=3010 -> DeadLetter
    const run3 = q.dequeue(3010).?;
    expect_eq(run3.id, id);
    expect_eq(run3.attempts, 2 as u32);
    q.nack(id, "fatal schema error", 3010)?;

    snap = q.getJob(id).?;
    expect_eq(snap.state == .DeadLetter, true);
    expect_eq(snap.attempts, 3 as u32);
    expect_eq(std.mem.eql(u8, snap.last_error, "fatal schema error"), true);
    expect_eq(q.dequeue(99999) == null, true);

    const st_dlq = q.stats();
    expect_eq(st_dlq.dead_letter, 1 as usize);
    expect_eq(st_dlq.total_retried, 2 as u64);

    // Inspect deadLetters(alloc)
    const dls = q.deadLetters(alloc)?;
    defer alloc.free(dls);
    expect_eq(dls.len, 1 as usize);
    expect_eq(dls[0].id, id);

    // Requeue dead letter resets attempts = 0 & clears last_error
    q.requeueDeadLetter(id, 4000)?;
    const snap_requeued = q.getJob(id).?;
    expect_eq(snap_requeued.state == .Pending, true);
    expect_eq(snap_requeued.attempts, 0 as u32);
    expect_eq(snap_requeued.last_error.len, 0 as usize);

    const dls_after = q.deadLetters(alloc)?;
    defer alloc.free(dls_after);
    expect_eq(dls_after.len, 0 as usize);

    const replay = q.dequeue(4000).?;
    expect_eq(replay.id, id);
    q.ack(replay.id)?;
    expect_eq(q.stats().completed, 1 as usize);
    expect_eq(q.stats().dead_letter, 0 as usize);
}

fn testZombieLeaseTimeoutRecoveryAndCapacity(): ! {
    const alloc = std.heap.page_allocator;
    let q = JobQueue.initWithCapacity(alloc, RetryPolicy.default(), 2500, 2)?;
    defer q.deinit();

    const j1_id = q.enqueue("worker.task", "job_1", .Normal, 1000)?;
    const j2_id = q.enqueue("worker.task", "job_2", .Low, 1000)?;

    // Capacity bound check (QueueError.QueueFull)
    let hit_full = false;
    * = q.enqueue("worker.task", "job_3", .High, 1000) catch |e| {
        if (e == @as(anyerror, QueueError.QueueFull)) hit_full = true;
    };
    expect_eq(hit_full, true);

    // Worker A acquires j1 at t=1000 -> lease_until_ms = 3500, then stalls/crashes
    const w_a = q.dequeue(1000).?;
    expect_eq(w_a.id, j1_id);
    expect_eq(w_a.lease_until_ms, 3500 as i64);

    // Worker B acquires j2 at t=1000 and acks it at t=1200
    const w_b = q.dequeue(1000).?;
    expect_eq(w_b.id, j2_id);
    q.ack(j2_id)?;

    // Before lease expiry (t=3499), j1 is still Running and NOT reclaimed
    expect_eq(q.dequeue(3499) == null, true);
    expect_eq(q.stats().running, 1 as usize);

    // At exact lease expiry (t=3500), j1 is automatically reclaimed and re-dispatched
    const w_c = q.dequeue(3500).?;
    expect_eq(w_c.id, j1_id);
    expect_eq(w_c.state == .Running, true);
    expect_eq(w_c.lease_until_ms, 6000 as i64);
    expect_eq(q.stats().running, 1 as usize);

    // Worker C completes j1
    q.ack(j1_id)?;
    expect_eq(q.stats().completed, 2 as usize);
    expect_eq(q.stats().running, 0 as usize);

    // Because activeCount dropped below capacity (completed jobs don't occupy active cap), enqueue succeeds
    const j3_id = q.enqueue("worker.task", "job_3_ok", .Critical, 3600)?;
    const w_d = q.dequeue(3600).?;
    expect_eq(w_d.id, j3_id);
    q.ack(j3_id)?;
}

fn testCapQueueAutomaticLazyMount(): ! {
    const alloc = std.heap.page_allocator;

    let hq = cap.queue.JobQueue.init(alloc, cap.queue.RetryPolicy.init(2, 500, 4000), 1000)?;
    defer hq.deinit();

    const prio: cap.queue.JobPriority = .Critical;
    const j_id = hq.enqueue("cap.hub.test", "payload_ok", prio, 100)?;
    const got: cap.queue.Job = hq.dequeue(100).?;
    expect_eq(got.id, j_id);
    expect_eq(got.state == cap.queue.JobState.Running, true);

    hq.nack(j_id, "transient", 100)?;
    const st: cap.queue.QueueStats = hq.stats();
    expect_eq(st.delayed, 1 as usize);
    expect_eq(st.total_retried, 1 as u64);

    const retried = hq.dequeue(600).?;
    expect_eq(retried.id, j_id);
    hq.ack(j_id)?;
    expect_eq(hq.stats().completed, 1 as usize);
}

fn main() {
    test("1. Priority Ordering (Critical > High > Normal > Low), FIFO & Ownership Copy", () => {
        testPriorityOrderingAndFifo()?;
    });
    test("2. Deterministic Delayed Job Dispatch (enqueueDelayed) & Exact Timestamp", () => {
        testDelayedJobExactDispatch()?;
    });
    test("3. Exponential Backoff nack, Dead-Letter Queue (DLQ) & requeueDeadLetter", () => {
        testExponentialBackoffDlqAndRequeue()?;
    });
    test("4. Worker Lease-Timeout Zombie Recovery & Capacity QueueFull Guard", () => {
        testZombieLeaseTimeoutRecoveryAndCapacity()?;
    });
    test("5. import cap from \"cap\"; cap.queue Automatic Dynamic Lazy Mounting", () => {
        testCapQueueAutomaticLazyMount()?;
    });
}
