// Source: cp/cap/test/test_lockfree_queue_production_deep.cp

import { test, expect_eq } from "cap:test";
import { SpscQueue, MpmcQueue, BoundedChannel, ChannelError } from "cap:lockfree_queue";
import cap from "cap";

const std = @import("std");
const builtin = @import("builtin");

const SPSC_COUNT: u64 = 100000;

fn spscProducer(q: *SpscQueue(u64, 1024)) {
    let i: u64 = 0;
    while (i < SPSC_COUNT) {
        if (q.tryPush(i)) {
            i += 1;
        } else {
            std.Thread.yield() catch {};
        }
    }
}

const MPMC_PER_PRODUCER: u64 = 10000;
const MPMC_PRODUCERS: usize = 4;
const MPMC_CONSUMERS: usize = 4;

const MpmcCtx = struct {
    q: *MpmcQueue(u64, 256),
    producers_done: std.atomic.Value(usize),
    total_count: std.atomic.Value(usize),
    total_sum: std.atomic.Value(u64),
};

fn mpmcProducer(ctx: *MpmcCtx) {
    let i: u64 = 1;
    while (i <= MPMC_PER_PRODUCER) {
        if (ctx.q.tryPush(i)) {
            i += 1;
        } else {
            std.Thread.yield() catch {};
        }
    }
    * = ctx.producers_done.fetchAdd(1, .release);
}

fn mpmcConsumer(ctx: *MpmcCtx) {
    let local_count: usize = 0;
    let local_sum: u64 = 0;
    while (true) {
        const item = ctx.q.tryPop();
        if (item != null) {
            local_count += 1;
            local_sum += item.?;
        } else {
            if (ctx.producers_done.load(.acquire) >= MPMC_PRODUCERS) {
                const tail_item = ctx.q.tryPop();
                if (tail_item != null) {
                    local_count += 1;
                    local_sum += tail_item.?;
                } else {
                    break;
                }
            } else {
                std.Thread.yield() catch {};
            }
        }
    }
    * = ctx.total_count.fetchAdd(local_count, .acq_rel);
    * = ctx.total_sum.fetchAdd(local_sum, .acq_rel);
}

const ChanCtx = struct {
    ch: *BoundedChannel(u64),
    total_count: std.atomic.Value(usize),
    total_sum: std.atomic.Value(u64),
};

fn chanSender(ch: *BoundedChannel(u64)) {
    let i: u64 = 1;
    while (i <= 500) {
        const res = ch.send(i);
        if (res == .err) unreachable;
        i += 1;
    }
}

fn chanReceiver(ctx: *ChanCtx) {
    let local_count: usize = 0;
    let local_sum: u64 = 0;
    while (true) {
        const item = ctx.ch.recv();
        if (item == null) {
            break;
        }
        local_count += 1;
        local_sum += item.?;
    }
    * = ctx.total_count.fetchAdd(local_count, .acq_rel);
    * = ctx.total_sum.fetchAdd(local_sum, .acq_rel);
}

fn runTest1(): bool {
    let q = SpscQueue(u64, 1024).init();
    if (!q.isEmpty()) return false;
    if (q.capacity() != 1024) return false;

    const prod = std.Thread.spawn(.{}, spscProducer, .{&q}) catch unreachable;

    let expected: u64 = 0;
    while (expected < SPSC_COUNT) {
        const val = q.tryPop();
        if (val != null) {
            if (val.? != expected) return false;
            expected += 1;
        } else {
            std.Thread.yield() catch {};
        }
    }
    prod.join();
    return expected == SPSC_COUNT and q.isEmpty();
}

fn runTest2(): bool {
    let q = MpmcQueue(u64, 256).init();
    let ctx = MpmcCtx{
        .q = &q,
        .producers_done = std.atomic.Value(usize).init(0),
        .total_count = std.atomic.Value(usize).init(0),
        .total_sum = std.atomic.Value(u64).init(0),
    };

    let producers: [MPMC_PRODUCERS]std.Thread = undefined;
    let consumers: [MPMC_CONSUMERS]std.Thread = undefined;

    let i: usize = 0;
    while (i < MPMC_PRODUCERS) {
        producers[i] = std.Thread.spawn(.{}, mpmcProducer, .{&ctx}) catch unreachable;
        i += 1;
    }
    i = 0;
    while (i < MPMC_CONSUMERS) {
        consumers[i] = std.Thread.spawn(.{}, mpmcConsumer, .{&ctx}) catch unreachable;
        i += 1;
    }

    i = 0;
    while (i < MPMC_PRODUCERS) {
        producers[i].join();
        i += 1;
    }
    i = 0;
    while (i < MPMC_CONSUMERS) {
        consumers[i].join();
        i += 1;
    }

    return ctx.total_count.load(.acquire) == 40000 and
        ctx.total_sum.load(.acquire) == 200020000 and
        q.isEmpty();
}

fn runTest3(): bool {
    let gpa = std.heap.DebugAllocator(.{}).init;
    const alloc = if (builtin.is_test) std.testing.allocator else gpa.allocator();

    {
        let ch = BoundedChannel(u64).init(alloc, 8).ok;
        defer ch.deinit();

        let ctx = ChanCtx{
            .ch = &ch,
            .total_count = std.atomic.Value(usize).init(0),
            .total_sum = std.atomic.Value(u64).init(0),
        };

        const r1 = std.Thread.spawn(.{}, chanReceiver, .{&ctx}) catch unreachable;
        const r2 = std.Thread.spawn(.{}, chanReceiver, .{&ctx}) catch unreachable;
        const s1 = std.Thread.spawn(.{}, chanSender, .{&ch}) catch unreachable;
        const s2 = std.Thread.spawn(.{}, chanSender, .{&ch}) catch unreachable;

        s1.join();
        s2.join();
        ch.close();
        r1.join();
        r2.join();

        if (ctx.total_count.load(.acquire) != 1000) return false;
        if (ctx.total_sum.load(.acquire) != 250500) return false;
        if (!ch.isClosed()) return false;
    }

    if (!builtin.is_test) {
        if (gpa.deinit() == .leak) return false;
    }
    return true;
}

fn runTest4(): bool {
    let spsc = SpscQueue(u32, 4).init();
    if (!spsc.tryPush(10)) return false;
    if (!spsc.tryPush(20)) return false;
    if (!spsc.tryPush(30)) return false;
    if (!spsc.tryPush(40)) return false;
    if (!spsc.isFull()) return false;
    if (spsc.len() != 4) return false;
    if (spsc.tryPush(50)) return false;
    if (spsc.tryPop().? != 10) return false;
    if (spsc.tryPop().? != 20) return false;
    if (spsc.tryPop().? != 30) return false;
    if (spsc.tryPop().? != 40) return false;
    if (spsc.tryPop() != null) return false;
    if (!spsc.isEmpty()) return false;

    let mpmc = MpmcQueue(u32, 4).init();
    if (!mpmc.tryPush(1)) return false;
    if (!mpmc.tryPush(2)) return false;
    if (!mpmc.tryPush(3)) return false;
    if (!mpmc.tryPush(4)) return false;
    if (!mpmc.isFull()) return false;
    if (mpmc.tryPush(5)) return false;

    let gpa = std.heap.DebugAllocator(.{}).init;
    const alloc = if (builtin.is_test) std.testing.allocator else gpa.allocator();

    {
        let ch = BoundedChannel(u32).init(alloc, 2).ok;
        defer ch.deinit();

        if (ch.trySend(100) == .err) return false;
        if (ch.trySend(200) == .err) return false;

        let got_full = false;
        const full_res = ch.trySend(300);
        if (full_res == .err and full_res.err == ChannelError.QueueFull) {
            got_full = true;
        }
        if (!got_full) return false;

        ch.close();
        let got_closed_try = false;
        const closed_try_res = ch.trySend(400);
        if (closed_try_res == .err and closed_try_res.err == ChannelError.ChannelClosed) {
            got_closed_try = true;
        }
        if (!got_closed_try) return false;

        let got_closed_send = false;
        const closed_send_res = ch.send(500);
        if (closed_send_res == .err and closed_send_res.err == ChannelError.ChannelClosed) {
            got_closed_send = true;
        }
        if (!got_closed_send) return false;

        if (ch.recv().? != 100) return false;
        if (ch.recv().? != 200) return false;
        if (ch.recv() != null) return false;
    }

    if (!builtin.is_test) {
        if (gpa.deinit() == .leak) return false;
    }
    return true;
}

fn runTest5(): bool {
    let spsc = cap.lockfree_queue.SpscQueue(u64, 8).init();
    if (!spsc.tryPush(7)) return false;
    if (spsc.tryPop().? != 7) return false;

    let mpmc = cap.lockfree_queue.MpmcQueue(u64, 8).init();
    if (!mpmc.tryPush(9)) return false;
    if (mpmc.tryPop().? != 9) return false;

    let gpa = std.heap.DebugAllocator(.{}).init;
    const alloc = if (builtin.is_test) std.testing.allocator else gpa.allocator();

    {
        let ch = cap.lockfree_queue.BoundedChannel(u64).init(alloc, 16).ok;
        defer ch.deinit();
        if (ch.trySend(123) == .err) return false;
        if (ch.len() != 1) return false;
        if (ch.tryRecv().? != 123) return false;
        if (ch.tryRecv() != null) return false;
    }

    if (!builtin.is_test) {
        if (gpa.deinit() == .leak) return false;
    }
    return true;
}

fn main() {
    test("1. SpscQueue 100k ordered wait-free transfer", () => {
        expect_eq(runTest1(), true);
    });
    test("2. MpmcQueue 4P4C high-contention exact conservation", () => {
        expect_eq(runTest2(), true);
    });
    test("3. BoundedChannel multi-thread blocking send/recv and close drain", () => {
        expect_eq(runTest3(), true);
    });
    test("4. Full queue false/QueueFull and closed channel ChannelClosed boundaries", () => {
        expect_eq(runTest4(), true);
    });
    test("5. cap.lockfree_queue dynamic mount and zero-leak allocator verification", () => {
        expect_eq(runTest5(), true);
    });
}
