// cap/test/test_event_loop.cp
// End-to-end regression test for Unified Event & EventLoop Subsystem (cap:event)

import { test, expect_eq } from "cap:test";
const std = @import("std");

import {
    Timer,
    TimerHeap,
    ConcurrentTask,
    ConcurrentQueue,
    DeferredTaskQueue,
    PollEngine,
    PollEvents,
    EventLoop
} from "cap:event";

class CounterState {
    pub count: i32 = 0;

    pub static fn create(): *CounterState {
        let p = std.heap.page_allocator.create(CounterState) catch unreachable;
        p.count = 0;
        return p;
    }
}

fn timer_callback(t: *Timer) {
    if (p := t.ctx) {
        let state: *CounterState = @ptrCast(@alignCast(p));
        state.count += 1;
    }
}

fn task_callback(ctx: (*anyopaque)?) {
    if (p := ctx) {
        let state: *CounterState = @ptrCast(@alignCast(p));
        state.count += 1;
    }
}

fn deferred_callback(ctx: *anyopaque): bool {
    let state: *CounterState = @ptrCast(@alignCast(ctx));
    state.count += 10;
    return false;
}

fn pipe_poll_callback(_fd: i32, _events: u32, ctx: *!?) {
    if (p := ctx) {
        let state: *CounterState = @ptrCast(@alignCast(p));
        state.count += 100;
    }
}

fn testTimerHeap(): !void {
    let heap = TimerHeap.init();
    expect_eq(heap.count(), 0 as usize);

    const state = CounterState.create();
    const state_ptr: *anyopaque = @ptrCast(state);

    let t1 = Timer{ expires_at_ms: 200, callback: timer_callback, ctx: state_ptr };
    let t2 = Timer{ expires_at_ms: 50, callback: timer_callback, ctx: state_ptr };
    let t3 = Timer{ expires_at_ms: 120, callback: timer_callback, ctx: state_ptr };

    heap.insert(&t1);
    heap.insert(&t2);
    heap.insert(&t3);
    expect_eq(heap.count(), 3 as usize);
    expect_eq(heap.peek().?.expires_at_ms, 50 as i64);

    // Cancel middle timer
    expect_eq(heap.remove(&t3), true);
    expect_eq(heap.count(), 2 as usize);

    // Fire up to 150ms -> only t2 (50ms) fires
    const fired1 = heap.fireExpired(150);
    expect_eq(fired1, 1 as usize);
    expect_eq(state.count, 1);

    // Fire up to 250ms -> t1 (200ms) fires
    const fired2 = heap.fireExpired(250);
    expect_eq(fired2, 1 as usize);
    expect_eq(state.count, 2);
    expect_eq(heap.isEmpty(), true);
}

fn testConcurrentQueue(): !void {
    let q = ConcurrentQueue.init();
    expect_eq(q.len(), 0 as usize);

    const state = CounterState.create();
    const state_ptr: *anyopaque = @ptrCast(state);

    let t1 = ConcurrentTask.init(task_callback, state_ptr, false);
    let t2 = ConcurrentTask.init(task_callback, state_ptr, false);
    q.push(&t1);
    q.push(&t2);
    expect_eq(q.len(), 2 as usize);

    const ran = q.drainAll();
    expect_eq(ran, 2 as usize);
    expect_eq(state.count, 2);
    expect_eq(q.isEmpty(), true);
}

fn testDeferredQueue(): !void {
    let q = DeferredTaskQueue.init();
    const state = CounterState.create();
    const state_ptr: *anyopaque = @ptrCast(state);

    expect_eq(q.postTask(state_ptr, deferred_callback), true);
    expect_eq(q.postTask(state_ptr, deferred_callback), false); // deduplicated
    expect_eq(q.count(), 1 as usize);

    const ran = q.run();
    expect_eq(ran, 1 as usize);
    expect_eq(state.count, 10);
    expect_eq(q.count(), 0 as usize);
}

fn testPollEngine(): !void {
    let pe = PollEngine.init();
    expect_eq(pe.count(), 0 as usize);

    let r_fd: i32 = 0;
    let w_fd: i32 = 0;
    __zig__ {
        var fds: [2]i32 = undefined;
        _ = std.os.linux.pipe(&fds);
        r_fd = fds[0];
        w_fd = fds[1];
    }
    defer * = std.os.linux.close(r_fd);
    defer * = std.os.linux.close(w_fd);

    const state = CounterState.create();
    const state_ptr: *anyopaque = @ptrCast(state);

    const reg_ok = pe.register(r_fd, PollEvents.Readable, pipe_poll_callback, state_ptr);
    expect_eq(reg_ok, true);
    expect_eq(pe.count(), 1 as usize);

    const polled_empty = pe.poll(0);
    expect_eq(polled_empty, 0 as usize);
    expect_eq(state.count, 0);

    const data = "x";
    * = std.os.linux.write(w_fd, data.ptr, data.len);

    const polled_ready = pe.poll(50);
    expect_eq(polled_ready, 1 as usize);
    expect_eq(state.count, 100);

    const unreg = pe.unregister(r_fd);
    expect_eq(unreg, true);
    expect_eq(pe.count(), 0 as usize);
}

fn testEventLoopIntegration(): !void {
    let loop = EventLoop.init();
    expect_eq(loop.hasActiveHandles(), false);

    const state = CounterState.create();
    const state_ptr: *anyopaque = @ptrCast(state);

    * = loop.deferTask(state_ptr, deferred_callback);
    * = loop.setTimeout(10, timer_callback, state_ptr);

    expect_eq(loop.hasActiveHandles(), true);

    * = loop.tick();
    expect_eq(state.count, 10);

    TimerHeap.sleepMs(15);
    * = loop.tick();
    expect_eq(state.count, 11);
    expect_eq(loop.hasActiveHandles(), false);
}

fn main() {
    test("Intrusive Pairing Min-Heap Timer Scheduling", () => {
        testTimerHeap()?;
    });

    test("Spin-Locked Concurrent Task Queue", () => {
        testConcurrentQueue()?;
    });

    test("Deferred Microtask Queue and Deduplication", () => {
        testDeferredQueue()?;
    });

    test("I/O File Descriptor Polling Engine", () => {
        testPollEngine()?;
    });

    test("Unified EventLoop Integration Lifecycle", () => {
        testEventLoopIntegration()?;
    });
}
