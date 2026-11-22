import { test, expect } from "cap:test";
import {
    MYSQL_REQ_QUEUE_CAPACITY,
    MySqlRequestQueueError,
    MySqlRequestState,
    MySqlRequestQueue,
} from "cap:mysql";

test("MySqlRequestQueue auto-pipelining dispatches pipelinable queries and blocks non-pipelinable tail", () => {
    let q = MySqlRequestQueue.init();
    expect(q.isEmpty()).toBe(true);
    expect(q.canExecuteQuery(true)).toBe(true);
    expect(q.canPipeline(true)).toBe(true);

    * = q.enqueue(101, true)?;
    * = q.enqueue(102, true)?;
    * = q.enqueue(103, false)?;
    expect(q.len()).toBe(3);

    // Dispatch first pipelinable query
    expect(q.nextDispatchableId(true) ?? 0).toBe(101);
    * = q.startRun(101, true)?;
    expect(q.pipelined_requests).toBe(1);
    expect(q.nonpipelinable_requests).toBe(0);

    // Second pipelinable query is immediately dispatchable in parallel
    expect(q.nextDispatchableId(true) ?? 0).toBe(102);
    * = q.startRun(102, true)?;
    expect(q.pipelined_requests).toBe(2);

    // Third query is non-pipelinable -> blocked while pipelined_requests > 0
    expect(q.nextDispatchableId(true) == null).toBe(true);

    // Finish 101 and 102 in FIFO order
    if (done1 := q.finishCurrent()) {
        expect(done1.id).toBe(101);
        expect(done1.state == MySqlRequestState.Completed).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    expect(q.pipelined_requests).toBe(1);
    expect(q.nextDispatchableId(true) == null).toBe(true);

    if (done2 := q.finishCurrent()) {
        expect(done2.id).toBe(102);
    } else {
        expect(false).toBe(true);
    }
    expect(q.pipelined_requests).toBe(0);

    // Now non-pipelinable 103 is unblocked
    expect(q.nextDispatchableId(true) ?? 0).toBe(103);
    * = q.startRun(103, true)?;
    expect(q.nonpipelinable_requests).toBe(1);
    * = q.finishCurrent();
    expect(q.isEmpty()).toBe(true);
    expect(q.nonpipelinable_requests).toBe(0);
});

test("MySqlRequestQueue prepare barrier blocks subsequent dispatch until markAsPrepared", () => {
    let q = MySqlRequestQueue.init();
    * = q.enqueue(201, true)?;
    * = q.enqueue(202, true)?;

    expect(q.canPrepareQuery(true)).toBe(true);
    * = q.startPrepare(201, true)?;
    expect(q.waiting_to_prepare).toBe(true);
    expect(q.is_ready_for_query).toBe(false);
    expect(q.canPrepareQuery(true)).toBe(false);
    expect(q.canPipeline(true)).toBe(false);
    expect(q.nextDispatchableId(true) == null).toBe(true);

    expect(q.markAsPrepared()).toBe(true);
    expect(q.waiting_to_prepare).toBe(false);
    if (cur := q.current()) {
        expect(cur.state == MySqlRequestState.Prepared).toBe(true);
    } else {
        expect(false).toBe(true);
    }

    // Now 201 can transition from Prepared -> Running
    expect(q.nextDispatchableId(true) ?? 0).toBe(201);
    * = q.startRun(201, true)?;
    expect(q.pipelined_requests).toBe(1);
    * = q.finishCurrent();
    expect(q.len()).toBe(1);
});

test("MySqlRequestQueue disabled auto-pipelining mode serializes all requests", () => {
    let q = MySqlRequestQueue.initWithPipelining(false);
    expect(q.canPipeline(true)).toBe(false);

    * = q.enqueue(301, true)?;
    * = q.enqueue(302, true)?;
    * = q.startRun(301, true)?;
    expect(q.nonpipelinable_requests).toBe(1);
    expect(q.pipelined_requests).toBe(0);

    // 302 cannot be dispatched while 301 is running
    expect(q.nextDispatchableId(true) == null).toBe(true);

    * = q.finishCurrent();
    q.markAsReadyForQuery();
    expect(q.canExecuteQuery(true)).toBe(true);
    expect(q.nextDispatchableId(true) ?? 0).toBe(302);
});

test("MySqlRequestQueue failCurrent, cancelAll disconnect cleanup, and ring-buffer wraparound", () => {
    let q = MySqlRequestQueue.init();
    * = q.enqueue(401, false)?;
    * = q.enqueue(402, true)?;
    * = q.enqueue(403, true)?;

    * = q.startRun(401, true)?;
    if (failed := q.failCurrent()) {
        expect(failed.id).toBe(401);
        expect(failed.state == MySqlRequestState.Failed).toBe(true);
    } else {
        expect(false).toBe(true);
    }
    expect(q.nonpipelinable_requests).toBe(0);
    expect(q.len()).toBe(2);

    // Bulk disconnect cancellation cancels remaining 2 entries and resets state
    const cancelled = q.cancelAll();
    expect(cancelled).toBe(2);
    expect(q.isEmpty()).toBe(true);
    expect(q.pipelined_requests).toBe(0);
    expect(q.nonpipelinable_requests).toBe(0);
    expect(q.waiting_to_prepare).toBe(false);

    // Ring buffer wraparound across 100 > MYSQL_REQ_QUEUE_CAPACITY (64) cycles
    let id: u64 = 1;
    while (id <= 100) {
        * = q.enqueue(id, true)?;
        * = q.startRun(id, true)?;
        if (done := q.finishCurrent()) {
            expect(done.id).toBe(id);
        } else {
            expect(false).toBe(true);
        }
        id += 1;
    }
    expect(q.isEmpty()).toBe(true);
});

test("MySqlRequestQueue enforces QueueFull, RequestNotFound, ConnectionNotWritable, and InvalidStateTransition", () => {
    let q = MySqlRequestQueue.init();
    expect(q.capacity()).toBe(MYSQL_REQ_QUEUE_CAPACITY);

    let i: usize = 0;
    while (i < MYSQL_REQ_QUEUE_CAPACITY) {
        * = q.enqueue(@as(u64, @intCast(i + 1)), true)?;
        i += 1;
    }
    expect(q.isFull()).toBe(true);

    let got_full = false;
    * = q.enqueue(999, true) catch |e| {
        if (e == MySqlRequestQueueError.QueueFull) {
            got_full = true;
        }
        0
    };
    expect(got_full).toBe(true);

    let got_not_writable = false;
    * = q.startRun(1, false) catch |e| {
        if (e == MySqlRequestQueueError.ConnectionNotWritable) {
            got_not_writable = true;
        }
        false
    };
    expect(got_not_writable).toBe(true);

    let got_not_found = false;
    * = q.startRun(99999, true) catch |e| {
        if (e == MySqlRequestQueueError.RequestNotFound) {
            got_not_found = true;
        }
        false
    };
    expect(got_not_found).toBe(true);

    // Duplicate startRun on an already-Running entry -> InvalidStateTransition
    * = q.startRun(1, true)?;
    let got_invalid = false;
    * = q.startRun(1, true) catch |e| {
        if (e == MySqlRequestQueueError.InvalidStateTransition) {
            got_invalid = true;
        }
        false
    };
    expect(got_invalid).toBe(true);
});
