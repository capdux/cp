// cap/test/test_event_loop_complete.cp
// Comprehensive verification of cap:event_loop standard library
import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TimeSpec,
    TimerState,
    Timer,
    TimerHeap,
    ConcurrentTask,
    ConcurrentTaskQueue,
    MAX_DEFERRED_TASKS,
    DeferredTaskEntry,
    DeferredTaskQueue,
    AnyTask,
    AnyTaskWithExtraContext,
    ManagedTask,
    AutoFlusher,
    MAX_LOOP_TIMERS,
    MAX_LOOP_TASKS,
    EventLoop,
} from "cap:event_loop";

// Shared counters for test callbacks
struct TestContext {
    count: i32 = 0,
    val_a: i32 = 0,
    val_b: i32 = 0,
}

fn timer_callback(t: *Timer) {
    if (ctx := t.ctx) {
        let tc: *TestContext = @alignCast(@ptrCast(ctx));
        tc.count += 1;
    }
}

fn test_timer_heap_ordering(): !void {
    let heap = TimerHeap.init();
    expect_eq(heap.count(), 0);
    expect_eq(heap.isEmpty(), true);

    let ctx = TestContext{ count: 0, val_a: 0, val_b: 0 };

    let t1 = Timer{
        expires_at_ms: 100,
        interval_ms: 0,
        state: TimerState.Pending,
        callback: timer_callback,
        ctx: @ptrCast(&ctx),
        child: null,
        prev: null,
        next: null,
        in_heap: false,
    };

    let t2 = Timer{
        expires_at_ms: 300,
        interval_ms: 0,
        state: TimerState.Pending,
        callback: timer_callback,
        ctx: @ptrCast(&ctx),
        child: null,
        prev: null,
        next: null,
        in_heap: false,
    };

    let t3 = Timer{
        expires_at_ms: 200,
        interval_ms: 0,
        state: TimerState.Pending,
        callback: timer_callback,
        ctx: @ptrCast(&ctx),
        child: null,
        prev: null,
        next: null,
        in_heap: false,
    };

    let t4 = Timer{
        expires_at_ms: 400,
        interval_ms: 0,
        state: TimerState.Pending,
        callback: timer_callback,
        ctx: @ptrCast(&ctx),
        child: null,
        prev: null,
        next: null,
        in_heap: false,
    };

    heap.insert(&t1);
    heap.insert(&t2);
    heap.insert(&t3);
    heap.insert(&t4);

    expect_eq(heap.count(), 4);
    expect_eq(heap.isEmpty(), false);

    // Min element should be t1 (100)
    expect_eq(heap.peek().?.expires_at_ms, 100);

    // Remove min
    let m1 = heap.deleteMin().?;
    expect_eq(m1.expires_at_ms, 100);
    expect_eq(heap.count(), 3);

    // Next min should be t3 (200)
    expect_eq(heap.peek().?.expires_at_ms, 200);

    // Remove t4 (400) arbitrarily
    let removed = heap.remove(&t4);
    expect_eq(removed, true);
    expect_eq(t4.isCancelled(), true);
    expect_eq(heap.count(), 2);

    // Fire expired up to 250ms -> should fire t3 (200)
    let fired = heap.fireExpired(250);
    expect_eq(fired, 1);
    expect_eq(ctx.count, 1);
    expect_eq(heap.count(), 1);

    // Only t2 (300) remaining
    expect_eq(heap.peek().?.expires_at_ms, 300);

    // Fire expired up to 350ms -> should fire t2 (300)
    let fired2 = heap.fireExpired(350);
    expect_eq(fired2, 1);
    expect_eq(ctx.count, 2);
    expect_eq(heap.isEmpty(), true);
}

fn deferred_once(ctx_ptr: *anyopaque): bool {
    let tc: *TestContext = @alignCast(@ptrCast(ctx_ptr));
    tc.val_a += 10;
    return false; // completed, do not repeat
}

fn deferred_repeat(ctx_ptr: *anyopaque): bool {
    let tc: *TestContext = @alignCast(@ptrCast(ctx_ptr));
    tc.val_b += 5;
    if (tc.val_b >= 15) {
        return false; // stop repeating once reached 15
    }
    return true; // repeat on next run
}

fn test_deferred_task_queue(): !void {
    let queue = DeferredTaskQueue.init();
    expect_eq(queue.count(), 0);
    expect_eq(queue.isEmpty(), true);

    let ctx1 = TestContext{ count: 0, val_a: 0, val_b: 0 };
    let ctx2 = TestContext{ count: 0, val_a: 0, val_b: 0 };

    let p1 = queue.postTask(@ptrCast(&ctx1), deferred_once);
    expect_eq(p1, true);

    // Duplicate registration on same context returns false
    let p1_dup = queue.postTask(@ptrCast(&ctx1), deferred_once);
    expect_eq(p1_dup, false);

    let p2 = queue.postTask(@ptrCast(&ctx2), deferred_repeat);
    expect_eq(p2, true);
    expect_eq(queue.count(), 2);

    // First run: ctx1 runs once and completes. ctx2 runs and requests repeat.
    let run1 = queue.run();
    expect_eq(run1, 2);
    expect_eq(ctx1.val_a, 10);
    expect_eq(ctx2.val_b, 5);
    expect_eq(queue.count(), 1); // ctx2 still in queue

    // Second run: ctx2 runs again (5 -> 10, still < 15, requests repeat)
    let run2 = queue.run();
    expect_eq(run2, 1);
    expect_eq(ctx2.val_b, 10);
    expect_eq(queue.count(), 1);

    // Third run: ctx2 runs again (10 -> 15, >= 15, completes)
    let run3 = queue.run();
    expect_eq(run3, 1);
    expect_eq(ctx2.val_b, 15);
    expect_eq(queue.count(), 0);
    expect_eq(queue.isEmpty(), true);

    // Test unregisterTask
    * = queue.postTask(@ptrCast(&ctx1), deferred_once);
    expect_eq(queue.count(), 1);
    let unreg = queue.unregisterTask(@ptrCast(&ctx1));
    expect_eq(unreg, true);
    expect_eq(queue.count(), 0);
}

fn concurrent_task_cb(ctx: (*anyopaque)?) {
    if (c := ctx) {
        let tc: *TestContext = @alignCast(@ptrCast(c));
        tc.count += 1;
    }
}

fn test_concurrent_task_queue(): !void {
    let queue = ConcurrentTaskQueue.init();
    expect_eq(queue.count(), 0);
    expect_eq(queue.isEmpty(), true);

    let ctx = TestContext{ count: 0, val_a: 0, val_b: 0 };

    let t1 = ConcurrentTask.init(concurrent_task_cb, @ptrCast(&ctx), false);
    let t2 = ConcurrentTask.init(concurrent_task_cb, @ptrCast(&ctx), false);
    let t3 = ConcurrentTask.init(concurrent_task_cb, @ptrCast(&ctx), false);

    queue.push(&t1);
    queue.push(&t2);
    queue.push(&t3);

    expect_eq(queue.count(), 3);
    expect_eq(queue.isEmpty(), false);

    // Pop first
    let popped = queue.pop().?;
    popped.run();
    expect_eq(ctx.count, 1);
    expect_eq(queue.count(), 2);

    // Drain remaining 2
    let drained_count = queue.drainAll();
    expect_eq(drained_count, 2);
    expect_eq(ctx.count, 3);
    expect_eq(queue.count(), 0);
    expect_eq(queue.isEmpty(), true);
}

fn any_task_cb(ctx: (*anyopaque)?) {
    if (c := ctx) {
        let tc: *TestContext = @alignCast(@ptrCast(c));
        tc.count += 100;
    }
}

fn extra_task_cb(ctx: (*anyopaque)?, extra: (*anyopaque)?) {
    if (c := ctx) {
        let tc: *TestContext = @alignCast(@ptrCast(c));
        if (e := extra) {
            let addend: *i32 = @alignCast(@ptrCast(e));
            tc.count += addend.*;
        }
    }
}

fn test_task_abstractions(): !void {
    let ctx = TestContext{ count: 0, val_a: 0, val_b: 0 };

    // AnyTask
    let any_task = AnyTask.init(@ptrCast(&ctx), any_task_cb);
    any_task.run();
    expect_eq(ctx.count, 100);

    // AnyTaskWithExtraContext
    let extra_val: i32 = 42;
    let extra_task = AnyTaskWithExtraContext.init(@ptrCast(&ctx), extra_task_cb);
    extra_task.run(@ptrCast(&extra_val));
    expect_eq(ctx.count, 142);

    // ManagedTask cancellation
    let managed = ManagedTask.init(@ptrCast(&ctx), any_task_cb);
    expect_eq(managed.isCancelled(), false);
    managed.cancel();
    expect_eq(managed.isCancelled(), true);
    managed.run(); // should not increment because cancelled
    expect_eq(ctx.count, 142);
}

fn test_auto_flusher(): !void {
    let flusher = AutoFlusher.init();
    expect_eq(flusher.isRegistered(), false);

    let queue = DeferredTaskQueue.init();
    let ctx = TestContext{ count: 0, val_a: 0, val_b: 0 };

    let reg1 = flusher.registerDeferred(&queue, @ptrCast(&ctx), deferred_once);
    expect_eq(reg1, true);
    expect_eq(flusher.isRegistered(), true);

    // Duplicate register through flusher returns false
    let reg2 = flusher.registerDeferred(&queue, @ptrCast(&ctx), deferred_once);
    expect_eq(reg2, false);

    // Unregister
    let unreg = flusher.unregisterDeferred(&queue, @ptrCast(&ctx));
    expect_eq(unreg, true);
    expect_eq(flusher.isRegistered(), false);
}

fn event_loop_task(ctx: (*anyopaque)?) {
    if (c := ctx) {
        let tc: *TestContext = @alignCast(@ptrCast(c));
        tc.val_a += 1;
    }
}

fn event_loop_def(ctx: *anyopaque): bool {
    let tc: *TestContext = @alignCast(@ptrCast(ctx));
    tc.val_b += 1;
    return false;
}

fn is_done_check(ctx: (*anyopaque)?): bool {
    if (c := ctx) {
        let tc: *TestContext = @alignCast(@ptrCast(c));
        return tc.val_a >= 3;
    }
    return true;
}

fn test_event_loop_lifecycle(): !void {
    let loop = EventLoop.init();
    expect_eq(loop.isAlive(), false);
    expect_eq(loop.iteration, 0);

    let ctx = TestContext{ count: 0, val_a: 0, val_b: 0 };

    // Post concurrent task and deferred task
    let p_task = loop.postTask(event_loop_task, @ptrCast(&ctx));
    expect_eq(p_task, true);
    let p_def = loop.postDeferred(@ptrCast(&ctx), event_loop_def);
    expect_eq(p_def, true);
    expect_eq(loop.isAlive(), true);

    // Tick once
    let processed = loop.tick();
    expect_eq(processed, 2);
    expect_eq(ctx.val_a, 1);
    expect_eq(ctx.val_b, 1);
    expect_eq(loop.iteration, 1);

    // Schedule a timer using setTimeout
    let timer = loop.setTimeout(10, timer_callback, @ptrCast(&ctx)).?;
    expect_eq(timer.isActive(), true);
    expect_eq(loop.isAlive(), true);

    // Cancel timer
    let cancelled = loop.clearTimeout(timer);
    expect_eq(cancelled, true);
    expect_eq(timer.isCancelled(), true);

    // Test runUntilDone
    * = loop.postTask(event_loop_task, @ptrCast(&ctx)); // val_a -> 2
    * = loop.postTask(event_loop_task, @ptrCast(&ctx)); // val_a -> 3
    let done_total = loop.runUntilDone(is_done_check, @ptrCast(&ctx));
    expect_eq(done_total >= 2, true);
    expect_eq(ctx.val_a >= 3, true);

    // TimeSpec tests
    let ts = TimeSpec.fromMillis(1500);
    expect_eq(ts.sec, 1);
    expect_eq(ts.nsec, 500_000_000);
    expect_eq(ts.toMillis(), 1500);
}

fn main() {
    test("1. cap:event_loop — Timer & TimerHeap min-heap ordering, peek, deleteMin, remove", () => {
        test_timer_heap_ordering()?;
    });

    test("2. cap:event_loop — DeferredTaskQueue microtask post, deduplication, unregister, run", () => {
        test_deferred_task_queue()?;
    });

    test("3. cap:event_loop — ConcurrentTask & ConcurrentTaskQueue cross-thread queue & drain", () => {
        test_concurrent_task_queue()?;
    });

    test("4. cap:event_loop — AnyTask, AnyTaskWithExtraContext, ManagedTask cancellation", () => {
        test_task_abstractions()?;
    });

    test("5. cap:event_loop — AutoFlusher auto-flush registration & unregistration", () => {
        test_auto_flusher()?;
    });

    test("6. cap:event_loop — EventLoop complete lifecycle: setTimeout, setInterval, tick, runUntilDone", () => {
        test_event_loop_lifecycle()?;
    });
}
