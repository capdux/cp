// cap/test/testEventExtraCtxQueue.cp
// Production-grade deep tests for cap:event extraCtxQueue.cp (Bun AnyTaskWithExtraContext queue)

import { test, assert, assertEq } from "cap:test";
import {
    ExtraCtxQueueError,
    ExtraCtxTaskItem,
    ExtraCtxTaskQueue,
} from "cap:event";

fn combineTask(task_ctx: u64, extra_ctx: u64): i32 {
    return @as(i32, @intCast(task_ctx * 10 + extra_ctx));
}

fn okZeroTask(_: u64, _: u64): i32 {
    return 0;
}

fn failCodeTask(task_ctx: u64, _: u64): i32 {
    return -@as(i32, @intCast(task_ctx));
}

test("ExtraCtxTaskQueue: FIFO enqueue & dispatch-time extra_ctx injection", () => {
    let buf: [8]ExtraCtxTaskItem = [ExtraCtxTaskItem{}; 8];
    let q = ExtraCtxTaskQueue.init(buf[0..]);
    assert(q.isEmpty());
    assertEq(q.capacity(), @as(usize, 8));
    assertEq(q.remaining(), @as(usize, 8));

    assert(q.enqueue(1, combineTask) catch false);
    assert(q.enqueue(2, combineTask) catch false);
    assert(q.enqueue(3, combineTask) catch false);
    assertEq(q.len(), @as(usize, 3));

    // Inject distinct extra_ctx on each dispatch
    assertEq(q.runNext(5) catch -1, @as(i32, 15));
    assertEq(q.runNext(7) catch -1, @as(i32, 27));
    assertEq(q.runNext(9) catch -1, @as(i32, 39));
    assert(q.isEmpty());
});

test("ExtraCtxTaskQueue: pushFront priority & ring buffer wrap-around", () => {
    let buf: [4]ExtraCtxTaskItem = [ExtraCtxTaskItem{}; 4];
    let q = ExtraCtxTaskQueue.init(buf[0..]);

    // Advance head/tail across wrap-around boundary
    assert(q.enqueue(1, combineTask) catch false);
    assert(q.enqueue(2, combineTask) catch false);
    assertEq(q.runNext(0) catch -1, @as(i32, 10));

    // Now pushFront wraps head backward cleanly
    assert(q.pushFront(9, combineTask) catch false);
    assert(q.enqueue(3, combineTask) catch false);
    assert(q.enqueue(4, combineTask) catch false);
    assert(q.isFull());

    // Expected order: 9, 2, 3, 4
    assertEq(q.runNext(1) catch -1, @as(i32, 91));
    assertEq(q.runNext(1) catch -1, @as(i32, 21));
    assertEq(q.runNext(1) catch -1, @as(i32, 31));
    assertEq(q.runNext(1) catch -1, @as(i32, 41));
    assert(q.isEmpty());
});

test("ExtraCtxTaskQueue: cancelByContext skips cancelled entries cleanly", () => {
    let buf: [6]ExtraCtxTaskItem = [ExtraCtxTaskItem{}; 6];
    let q = ExtraCtxTaskQueue.init(buf[0..]);

    assert(q.enqueue(10, combineTask) catch false);
    assert(q.enqueue(20, combineTask) catch false);
    assert(q.enqueue(10, combineTask) catch false);
    assert(q.enqueue(30, combineTask) catch false);

    assertEq(q.cancelByContext(10), @as(usize, 2));
    // Re-cancelling same context returns 0
    assertEq(q.cancelByContext(10), @as(usize, 0));

    // runNext skips both cancelled ctx=10 items
    assertEq(q.runNext(1) catch -1, @as(i32, 201));
    assertEq(q.runNext(2) catch -1, @as(i32, 302));
    assert(q.isEmpty());
});

test("ExtraCtxTaskQueue: bounded drain & error_count / skipped_cancelled stats", () => {
    let buf: [8]ExtraCtxTaskItem = [ExtraCtxTaskItem{}; 8];
    let q = ExtraCtxTaskQueue.init(buf[0..]);

    assert(q.enqueue(1, okZeroTask) catch false);
    assert(q.enqueue(2, failCodeTask) catch false);
    assert(q.enqueue(3, okZeroTask) catch false);
    assert(q.enqueue(4, failCodeTask) catch false);
    assert(q.enqueue(5, okZeroTask) catch false);
    assertEq(q.cancelByContext(3), @as(usize, 1));

    // Bounded drain of 3 items: [1(ok), 2(fail -2), 3(cancelled)]
    const s1 = q.drain(99, 3);
    assertEq(s1.ran, @as(usize, 2));
    assertEq(s1.skipped_cancelled, @as(usize, 1));
    assertEq(s1.error_count, @as(usize, 1));
    assertEq(s1.last_result, @as(i32, -2));
    assertEq(q.len(), @as(usize, 2));

    // Full drain (max_items == 0): [4(fail -4), 5(ok 0)]
    const s2 = q.drain(99, 0);
    assertEq(s2.ran, @as(usize, 2));
    assertEq(s2.skipped_cancelled, @as(usize, 0));
    assertEq(s2.error_count, @as(usize, 1));
    assertEq(s2.last_result, @as(i32, 0));
    assert(q.isEmpty());
});

test("ExtraCtxTaskQueue: large external slice (300 items, no hardcoded small limit)", () => {
    let buf: [300]ExtraCtxTaskItem = [ExtraCtxTaskItem{}; 300];
    let q = ExtraCtxTaskQueue.init(buf[0..]);

    let i: usize = 0;
    while (i < 300) {
        assert(q.enqueue(@as(u64, i), okZeroTask) catch false);
        i += 1;
    }
    assert(q.isFull());
    const stats = q.drain(1, 0);
    assertEq(stats.ran, @as(usize, 300));
    assertEq(stats.error_count, @as(usize, 0));
    assert(q.isEmpty());
});

test("ExtraCtxTaskQueue: QueueFull / QueueEmpty / clear & zero-cap guards", () => {
    let zero_buf: [0]ExtraCtxTaskItem = [];
    let zq = ExtraCtxTaskQueue.init(zero_buf[0..]);
    let got_zfull = false;
    * = zq.enqueue(1, okZeroTask) catch |e| {
        if (e == ExtraCtxQueueError.QueueFull) {
            got_zfull = true;
        }
        false
    };
    assert(got_zfull);

    let buf: [2]ExtraCtxTaskItem = [ExtraCtxTaskItem{}; 2];
    let q = ExtraCtxTaskQueue.init(buf[0..]);

    let got_empty = false;
    * = q.runNext(0) catch |e| {
        if (e == ExtraCtxQueueError.QueueEmpty) {
            got_empty = true;
        }
        0
    };
    assert(got_empty);

    assert(q.enqueue(1, okZeroTask) catch false);
    assert(q.enqueue(2, okZeroTask) catch false);
    let got_full = false;
    * = q.pushFront(3, okZeroTask) catch |e| {
        if (e == ExtraCtxQueueError.QueueFull) {
            got_full = true;
        }
        false
    };
    assert(got_full);

    q.clear();
    assert(q.isEmpty());
    assertEq(q.remaining(), @as(usize, 2));
});
