// Source: cp/cap/test/test_threading_production_deep.cp (cap:threading deep multi-threaded tests)
import { test, expect_eq } from "cap:test";
import {
    ResetEvent,
    WaitGroup,
    CountDownLatch,
    Semaphore,
    SpinLock,
    OnceFlag,
    Barrier,
} from "cap:threading";
import cap from "cap";

const std = @import("std");

struct WgContext {
    wg: *WaitGroup,
    slot: *usize,
    value: usize,
}

fn wgWorker(ctx: WgContext) {
    defer ctx.wg.done();
    ctx.slot.* = ctx.value;
}

test("WaitGroup multi-thread fan-out and underflow guard") {
    let wg = WaitGroup.init();
    wg.done();
    expect_eq(wg.count(), @as(usize, 0));

    let results: [4]usize = [0, 0, 0, 0];
    wg.add(4);
    expect_eq(wg.count(), @as(usize, 4));

    const t0 = std.Thread.spawn(.{}, wgWorker, .{WgContext{ .wg = &wg, .slot = &results[0], .value = 10 }}) catch unreachable;
    const t1 = std.Thread.spawn(.{}, wgWorker, .{WgContext{ .wg = &wg, .slot = &results[1], .value = 20 }}) catch unreachable;
    const t2 = std.Thread.spawn(.{}, wgWorker, .{WgContext{ .wg = &wg, .slot = &results[2], .value = 30 }}) catch unreachable;
    const t3 = std.Thread.spawn(.{}, wgWorker, .{WgContext{ .wg = &wg, .slot = &results[3], .value = 40 }}) catch unreachable;

    wg.wait();
    t0.join();
    t1.join();
    t2.join();
    t3.join();

    expect_eq(wg.count(), @as(usize, 0));
    expect_eq(results[0] + results[1] + results[2] + results[3], @as(usize, 100));
}

struct SemContext {
    sem: *Semaphore,
    active: *std.atomic.Value(usize),
    max_active: *std.atomic.Value(usize),
    completed: *std.atomic.Value(usize),
}

fn semWorker(ctx: SemContext) {
    let iter: usize = 0;
    while (iter < 10) {
        ctx.sem.acquire();
        const cur = ctx.active.fetchAdd(1, .acq_rel) + 1;

        let prev_max = ctx.max_active.load(.acquire);
        while (cur > prev_max) {
            if (actual := ctx.max_active.cmpxchgWeak(prev_max, cur, .acq_rel, .monotonic)) {
                prev_max = actual;
            } else {
                break;
            }
        }

        std.Thread.yield() catch {};
        * = ctx.active.fetchSub(1, .acq_rel);
        ctx.sem.post();
        * = ctx.completed.fetchAdd(1, .acq_rel);
        iter += 1;
    }
}

test("Semaphore concurrency limit 4 threads 2 permits") {
    let sem = Semaphore.init(2);
    expect_eq(sem.availablePermits(), @as(usize, 2));
    expect_eq(sem.tryAcquire(), true);
    expect_eq(sem.availablePermits(), @as(usize, 1));
    sem.post();
    expect_eq(sem.availablePermits(), @as(usize, 2));

    let active = std.atomic.Value(usize).init(0);
    let max_active = std.atomic.Value(usize).init(0);
    let completed = std.atomic.Value(usize).init(0);
    const ctx = SemContext{
        .sem = &sem,
        .active = &active,
        .max_active = &max_active,
        .completed = &completed,
    };

    const t0 = std.Thread.spawn(.{}, semWorker, .{ctx}) catch unreachable;
    const t1 = std.Thread.spawn(.{}, semWorker, .{ctx}) catch unreachable;
    const t2 = std.Thread.spawn(.{}, semWorker, .{ctx}) catch unreachable;
    const t3 = std.Thread.spawn(.{}, semWorker, .{ctx}) catch unreachable;

    t0.join();
    t1.join();
    t2.join();
    t3.join();

    expect_eq(completed.load(.acquire), @as(usize, 40));
    expect_eq(active.load(.acquire), @as(usize, 0));
    expect_eq(max_active.load(.acquire) <= 2, true);
    expect_eq(sem.availablePermits(), @as(usize, 2));
}

struct EventLatchCtx {
    event: *ResetEvent,
    latch: *CountDownLatch,
    observed: *std.atomic.Value(usize),
}

fn eventWaiter(ctx: EventLatchCtx) {
    ctx.event.wait();
    * = ctx.observed.fetchAdd(1, .acq_rel);
    ctx.latch.countDown();
}

test("ResetEvent and CountDownLatch cross-thread wakeup and reset") {
    let event = ResetEvent.init();
    let latch = CountDownLatch.init(3);
    let observed = std.atomic.Value(usize).init(0);
    expect_eq(event.isSet(), false);
    expect_eq(latch.remaining(), @as(usize, 3));

    const ctx = EventLatchCtx{ .event = &event, .latch = &latch, .observed = &observed };
    const t0 = std.Thread.spawn(.{}, eventWaiter, .{ctx}) catch unreachable;
    const t1 = std.Thread.spawn(.{}, eventWaiter, .{ctx}) catch unreachable;
    const t2 = std.Thread.spawn(.{}, eventWaiter, .{ctx}) catch unreachable;

    std.Thread.yield() catch {};
    event.set();
    expect_eq(event.isSet(), true);

    latch.wait();
    t0.join();
    t1.join();
    t2.join();

    expect_eq(observed.load(.acquire), @as(usize, 3));
    expect_eq(latch.remaining(), @as(usize, 0));
    latch.countDown();
    expect_eq(latch.remaining(), @as(usize, 0));

    event.reset();
    expect_eq(event.isSet(), false);

    let latch2 = CountDownLatch.init(1);
    const ctx2 = EventLatchCtx{ .event = &event, .latch = &latch2, .observed = &observed };
    const t3 = std.Thread.spawn(.{}, eventWaiter, .{ctx2}) catch unreachable;
    event.set();
    latch2.wait();
    t3.join();
    expect_eq(observed.load(.acquire), @as(usize, 4));
}

let g_once_counter = std.atomic.Value(usize).init(0);

fn initOnceCallback() {
    * = g_once_counter.fetchAdd(1, .acq_rel);
}

struct SpinOnceCtx {
    spin: *SpinLock,
    once: *OnceFlag,
    sum: *usize,
}

fn spinOnceWorker(ctx: SpinOnceCtx) {
    ctx.once.callOnce(initOnceCallback);
    let i: usize = 0;
    while (i < 500) {
        ctx.spin.lock();
        ctx.sum.* += 1;
        ctx.spin.unlock();
        i += 1;
    }
}

test("SpinLock high-contention accumulation and OnceFlag single init") {
    g_once_counter.store(0, .release);
    let spin = SpinLock.init();
    let once = OnceFlag.init();
    let sum: usize = 0;

    expect_eq(spin.isHeld(), false);
    expect_eq(spin.tryLock(), true);
    expect_eq(spin.isHeld(), true);
    expect_eq(spin.tryLock(), false);
    spin.unlock();
    expect_eq(spin.isHeld(), false);
    expect_eq(once.isDone(), false);

    const ctx = SpinOnceCtx{ .spin = &spin, .once = &once, .sum = &sum };
    const t0 = std.Thread.spawn(.{}, spinOnceWorker, .{ctx}) catch unreachable;
    const t1 = std.Thread.spawn(.{}, spinOnceWorker, .{ctx}) catch unreachable;
    const t2 = std.Thread.spawn(.{}, spinOnceWorker, .{ctx}) catch unreachable;
    const t3 = std.Thread.spawn(.{}, spinOnceWorker, .{ctx}) catch unreachable;

    t0.join();
    t1.join();
    t2.join();
    t3.join();

    expect_eq(once.isDone(), true);
    expect_eq(g_once_counter.load(.acquire), @as(usize, 1));
    expect_eq(sum, @as(usize, 2000));
    expect_eq(spin.isHeld(), false);
}

struct BarrierCtx {
    barrier: *Barrier,
    phase1_count: *std.atomic.Value(usize),
    phase2_count: *std.atomic.Value(usize),
}

fn barrierWorker(ctx: BarrierCtx) {
    * = ctx.phase1_count.fetchAdd(1, .acq_rel);
    ctx.barrier.wait();
    * = ctx.phase2_count.fetchAdd(1, .acq_rel);
    ctx.barrier.wait();
}

test("Barrier multi-round sync and cap.threading mount") {
    let barrier = cap.threading.Barrier.init(3);
    let p1 = std.atomic.Value(usize).init(0);
    let p2 = std.atomic.Value(usize).init(0);
    const ctx = BarrierCtx{ .barrier = &barrier, .phase1_count = &p1, .phase2_count = &p2 };

    const t0 = std.Thread.spawn(.{}, barrierWorker, .{ctx}) catch unreachable;
    const t1 = std.Thread.spawn(.{}, barrierWorker, .{ctx}) catch unreachable;
    const t2 = std.Thread.spawn(.{}, barrierWorker, .{ctx}) catch unreachable;

    t0.join();
    t1.join();
    t2.join();

    expect_eq(p1.load(.acquire), @as(usize, 3));
    expect_eq(p2.load(.acquire), @as(usize, 3));
}
