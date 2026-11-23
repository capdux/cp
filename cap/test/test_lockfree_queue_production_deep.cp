// Source: cp/cap/test/test_lockfree_queue_production_deep.cp

import { test, expect_eq } from "cap:test";
import {
    SpscQueue,
    MpmcQueue,
    BoundedChannel,
    ChannelError,
    CACHE_LINE_LENGTH,
    QUEUE_PADDING_LENGTH,
    UnboundedQueue,
} from "cap:ds";
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
    let spsc = cap.ds.SpscQueue(u64, 8).init();
    if (!spsc.tryPush(7)) return false;
    if (spsc.tryPop().? != 7) return false;

    let mpmc = cap.ds.MpmcQueue(u64, 8).init();
    if (!mpmc.tryPush(9)) return false;
    if (mpmc.tryPop().? != 9) return false;

    let gpa = std.heap.DebugAllocator(.{}).init;
    const alloc = if (builtin.is_test) std.testing.allocator else gpa.allocator();

    {
        let ch = cap.ds.BoundedChannel(u64).init(alloc, 16).ok;
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

struct IntNode {
    value: u32 = 0,
    next: (*IntNode!)? = null,
}

struct JobNode {
    worker_id: u32 = 0,
    seq: u32 = 0,
    link: (*JobNode!)? = null,
}

const IntQueue = UnboundedQueue(IntNode, .next);
const JobQueue = UnboundedQueue(JobNode, .link);

struct UqProducerCtx {
    queue: *JobQueue!,
    nodes: []JobNode,
    worker_id: u32,
}

fn uqProducerWorker(ctx: UqProducerCtx) {
    let nodes = ctx.nodes;
    let i: usize = 0;
    while (i < nodes.len) {
        nodes[i].worker_id = ctx.worker_id;
        nodes[i].seq = i as u32;
        nodes[i].link = null;
        ctx.queue.push(&nodes[i]);
        i += 1;
    }
}

fn runTest6(): bool {
    if (CACHE_LINE_LENGTH < usize(32) || QUEUE_PADDING_LENGTH < usize(64)) return false;
    let q = IntQueue.init();
    if (!q.isEmpty()) return false;
    if (q.pop() != null) return false;
    let empty_batch = q.popBatch();
    if (!empty_batch.isEmpty() || empty_batch.len() != usize(0)) return false;
    let it = empty_batch.iterator();
    if (it.next() != null) return false;
    return true;
}

fn runTest7(): bool {
    let q = IntQueue.init();
    let nodes: [16]IntNode = std.mem.zeroes([16]IntNode);
    let i: usize = 0;
    while (i < 16) {
        nodes[i].value = (i + 1) as u32;
        q.push(&nodes[i]);
        i += 1;
    }
    if (q.isEmpty()) return false;
    let j: usize = 0;
    while (j < 16) {
        if (popped := q.pop()) {
            if (popped.value != (j + 1) as u32) return false;
        } else {
            return false;
        }
        j += 1;
    }
    return q.isEmpty() && q.pop() == null;
}

fn runTest8(): bool {
    let q = IntQueue.init();
    let nodes: [10]IntNode = std.mem.zeroes([10]IntNode);
    let i: usize = 0;
    while (i < 10) {
        nodes[i].value = (i * 10) as u32;
        i += 1;
    }
    nodes[0].next = &nodes[1];
    nodes[1].next = &nodes[2];
    nodes[2].next = &nodes[3];
    nodes[3].next = &nodes[4];
    nodes[4].next = null;
    nodes[5].next = &nodes[6];
    nodes[6].next = &nodes[7];
    nodes[7].next = &nodes[8];
    nodes[8].next = &nodes[9];
    nodes[9].next = null;

    q.pushBatch(&nodes[0], &nodes[4]);
    q.pushBatch(&nodes[5], &nodes[9]);
    const batch = q.popBatch();
    if (!q.isEmpty() || batch.len() != usize(10)) return false;

    let it = batch.iterator();
    let idx: usize = 0;
    while (node := it.next()) {
        if (node.value != (idx * 10) as u32) return false;
        idx += 1;
    }
    return idx == usize(10) && it.next() == null;
}

fn runTest9(): bool {
    let q = JobQueue.init();
    let jobs: [4]JobNode = std.mem.zeroes([4]JobNode);
    let round: u32 = 0;
    while (round < 5) {
        let i: usize = 0;
        while (i < 4) {
            jobs[i].worker_id = round;
            jobs[i].seq = i as u32;
            q.push(&jobs[i]);
            i += 1;
        }
        if (first := q.pop()) {
            if (first.worker_id != round || first.seq != u32(0)) return false;
        } else {
            return false;
        }
        const rest = q.popBatch();
        if (rest.len() != usize(3)) return false;
        let it = rest.iterator();
        let expected_seq: u32 = 1;
        while (job := it.next()) {
            if (job.worker_id != round || job.seq != expected_seq) return false;
            expected_seq += 1;
        }
        if (expected_seq != u32(4) || !q.isEmpty()) return false;
        round += 1;
    }
    return true;
}

fn runTest10(): bool {
    let gpa = std.heap.DebugAllocator(.{}).init;
    const alloc = if (builtin.is_test) std.testing.allocator else gpa.allocator();
    const per_worker: usize = 250;
    const num_workers: usize = 4;
    const total_nodes: usize = per_worker * num_workers;

    {
        let pool = alloc.alloc(JobNode, total_nodes) catch return false;
        defer alloc.free(pool);

        let q = JobQueue.init();
        let threads: [4]std.Thread = undefined;
        let w: usize = 0;
        while (w < num_workers) {
            const start = w * per_worker;
            const ctx = UqProducerCtx{
                queue: &q,
                nodes: pool[start .. start + per_worker],
                worker_id: w as u32,
            };
            threads[w] = std.Thread.spawn(.{}, uqProducerWorker, .{ctx}) catch return false;
            w += 1;
        }

        let consumed: usize = 0;
        let seq_sum: u64 = 0;
        while (consumed < total_nodes) {
            const batch = q.popBatch();
            let it = batch.iterator();
            while (node := it.next()) {
                consumed += 1;
                seq_sum += node.seq as u64;
            }
            if (consumed < total_nodes) {
                std.atomic.spinLoopHint();
            }
        }

        let t: usize = 0;
        while (t < num_workers) {
            threads[t].join();
            t += 1;
        }
        if (consumed != total_nodes || !q.isEmpty() || seq_sum != u64(124500)) return false;
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
    test("6. UnboundedQueue empty invariants, cache-line padding & null pop", () => {
        expect_eq(runTest6(), true);
    });
    test("7. UnboundedQueue single-threaded strict FIFO push/pop ordering", () => {
        expect_eq(runTest7(), true);
    });
    test("8. UnboundedQueue pushBatch & popBatch with Batch.Iterator traversal", () => {
        expect_eq(runTest8(), true);
    });
    test("9. UnboundedQueue custom node multi-round reuse", () => {
        expect_eq(runTest9(), true);
    });
    test("10. UnboundedQueue 4-producer MPSC 1000-node concurrent stress test", () => {
        expect_eq(runTest10(), true);
    });
}
