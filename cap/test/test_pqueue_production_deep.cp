// Source: cp/cap/test/test_pqueue_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:pqueue & cap.pqueue

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PQueueError,
    PriorityItem,
    TopKEntry,
    IndexedMinHeap,
    BoundedTopK,
    StreamingMedian,
} from "cap:time";

test("1. IndexedMinHeap out-of-order push, peek/pop strict ordering, negative priorities and multi-round stress") {
    const alloc = std.testing.allocator;
    let pq = IndexedMinHeap<i64>.init(alloc);
    defer pq.deinit();

    expect_eq(pq.isEmpty(), true);
    expect_eq(pq.len(), 0);
    expect_eq(pq.peek() == null, true);
    expect_eq(pq.pop() == null, true);

    // Push 256 items in scrambled order with negative and positive priorities
    let step: usize = 0;
    while (step < 256) : (step += 1) {
        const scrambled = ((step * 173 + 41) % 256) as i64;
        const prio: i64 = scrambled - 128;
        const h = pq.push(prio, scrambled * 10)?;
        expect_eq(pq.containsHandle(h), true);
    }
    expect_eq(pq.len(), 256);
    expect_eq(pq.isEmpty(), false);

    // Peek must match minimum (-128)
    const top = pq.peek().?;
    expect_eq(top.priority, -128);
    expect_eq(top.value, 0);

    // Pop all 256 items -> strictly non-decreasing priorities from -128 to 127
    let expected_prio: i64 = -128;
    while (item := pq.pop()) {
        expect_eq(item.priority, expected_prio);
        expect_eq(item.value, (expected_prio + 128) * 10);
        expect_eq(pq.containsHandle(item.handle), false);
        expected_prio += 1;
    }
    expect_eq(expected_prio, 128);
    expect_eq(pq.isEmpty(), true);
    expect_eq(pq.len(), 0);

    // Second round: mixed push/pop + clear
    const h1 = pq.push(50, 1)?;
    const h2 = pq.push(10, 2)?;
    const h3 = pq.push(30, 3)?;
    expect_eq(pq.pop().?.value, 2);
    expect_eq(pq.containsHandle(h2), false);
    expect_eq(pq.containsHandle(h1), true);
    expect_eq(pq.containsHandle(h3), true);

    pq.clear();
    expect_eq(pq.isEmpty(), true);
    expect_eq(pq.containsHandle(h1), false);
    expect_eq(pq.containsHandle(h3), false);
}

test("2. IndexedMinHeap updatePriority sift-up/down, removeByHandle and InvalidHandle ABA protection") {
    const alloc = std.testing.allocator;
    let pq = IndexedMinHeap<i32>.init(alloc);
    defer pq.deinit();

    const h1 = pq.push(100, 1)?;
    const h2 = pq.push(200, 2)?;
    const h3 = pq.push(300, 3)?;
    const h4 = pq.push(400, 4)?;
    const h5 = pq.push(500, 5)?;

    // Sift-up: h5 (500 -> 50) becomes new minimum
    pq.updatePriority(h5, 50)?;
    expect_eq(pq.peek().?.handle, h5);
    expect_eq(pq.peek().?.priority, 50);

    // Sift-down: h5 (50 -> 600) sinks to bottom, h1 (100) becomes minimum
    pq.updatePriority(h5, 600)?;
    expect_eq(pq.peek().?.handle, h1);
    expect_eq(pq.peek().?.priority, 100);

    // Remove middle element h3 (300) in place
    const removed = pq.removeByHandle(h3)?;
    expect_eq(removed.handle, h3);
    expect_eq(removed.priority, 300);
    expect_eq(removed.value, 3);
    expect_eq(pq.containsHandle(h3), false);
    expect_eq(pq.len(), 4);

    // Removing or updating already-removed h3 must fail with InvalidHandle
    let err_rem = false;
    * = pq.removeByHandle(h3) catch |e| {
        if (e == PQueueError.InvalidHandle) err_rem = true;
    };
    expect_eq(err_rem, true);

    let err_upd = false;
    pq.updatePriority(h3, 10) catch |e| {
        if (e == PQueueError.InvalidHandle) err_upd = true;
    };
    expect_eq(err_upd, true);

    // Pop h1, then push a new item (reuses h1's slot with bumped generation)
    const popped_h1 = pq.pop().?;
    expect_eq(popped_h1.handle, h1);
    const h6 = pq.push(150, 6)?;
    expect_eq(pq.containsHandle(h6), true);

    // Stale handle h1 must STILL fail with InvalidHandle despite slot reuse!
    let err_stale = false;
    pq.updatePriority(h1, 1) catch |e| {
        if (e == PQueueError.InvalidHandle) err_stale = true;
    };
    expect_eq(err_stale, true);

    // Bogus handle must fail with InvalidHandle
    let err_bogus = false;
    * = pq.removeByHandle(0xDEADBEEF) catch |e| {
        if (e == PQueueError.InvalidHandle) err_bogus = true;
    };
    expect_eq(err_bogus, true);

    // Remaining order: h6(150), h2(200), h4(400), h5(600)
    expect_eq(pq.pop().?.value, 6);
    expect_eq(pq.pop().?.value, 2);
    expect_eq(pq.pop().?.value, 4);
    expect_eq(pq.pop().?.value, 5);
    expect_eq(pq.isEmpty(), true);
}

test("3. BoundedTopK 1200-item streaming top-5 descending output and InvalidCapacity guard") {
    const alloc = std.testing.allocator;

    // InvalidCapacity when k == 0
    let err_cap = false;
    * = BoundedTopK<usize>.init(alloc, 0) catch |e| {
        if (e == PQueueError.InvalidCapacity) err_cap = true;
    };
    expect_eq(err_cap, true);

    let topk = BoundedTopK<usize>.init(alloc, 5)?;
    defer topk.deinit();

    // Stream 1200 items in pseudo-random order
    let i: usize = 0;
    while (i < 1200) : (i += 1) {
        const val = (i * 499 + 17) % 1200;
        const score = (val as f64) * 1.5;
        topk.push(score, val)?;
    }
    expect_eq(topk.len(), 5);

    const sorted = topk.topSorted(alloc)?;
    defer alloc.free(sorted);
    expect_eq(sorted.len, 5);

    // Top 5 values must be 1199, 1198, 1197, 1196, 1195 in descending order
    expect_eq(sorted[0].value, 1199);
    expect_eq(sorted[1].value, 1198);
    expect_eq(sorted[2].value, 1197);
    expect_eq(sorted[3].value, 1196);
    expect_eq(sorted[4].value, 1195);
    expect_eq(sorted[0].score > sorted[1].score, true);
    expect_eq(sorted[1].score > sorted[2].score, true);
    expect_eq(sorted[2].score > sorted[3].score, true);
    expect_eq(sorted[3].score > sorted[4].score, true);

    // Clear and reuse with fewer than K items
    topk.clear();
    expect_eq(topk.len(), 0);
    topk.push(10.0, 1)?;
    topk.push(30.0, 3)?;
    const partial = topk.topSorted(alloc)?;
    defer alloc.free(partial);
    expect_eq(partial.len, 2);
    expect_eq(partial[0].value, 3);
    expect_eq(partial[1].value, 1);
}

test("4. StreamingMedian odd/even, out-of-order & negative values and EmptyQueue guard") {
    const alloc = std.testing.allocator;
    let sm = StreamingMedian.init(alloc);
    defer sm.deinit();

    expect_eq(sm.count(), 0);
    let err_empty = false;
    * = sm.median() catch |e| {
        if (e == PQueueError.EmptyQueue) err_empty = true;
    };
    expect_eq(err_empty, true);

    // Push: [-10, 20, -30, 40, 0] -> sorted: [-30, -10, 0, 20, 40]
    sm.push(-10.0)?;
    expect_eq(sm.median()?, -10.0);

    sm.push(20.0)?;
    expect_eq(sm.median()?, 5.0); // (-10 + 20) / 2

    sm.push(-30.0)?;
    expect_eq(sm.median()?, -10.0); // [-30, -10, 20]

    sm.push(40.0)?;
    expect_eq(sm.median()?, 5.0); // [-30, -10, 20, 40] -> (-10 + 20) / 2

    sm.push(0.0)?;
    expect_eq(sm.median()?, 0.0); // [-30, -10, 0, 20, 40]
    expect_eq(sm.count(), 5);

    sm.push(10.0)?;
    expect_eq(sm.median()?, 5.0); // [-30, -10, 0, 10, 20, 40] -> (0 + 10) / 2
    expect_eq(sm.count(), 6);

    sm.clear();
    expect_eq(sm.count(), 0);
    let err_after_clear = false;
    * = sm.median() catch |e| {
        if (e == PQueueError.EmptyQueue) err_after_clear = true;
    };
    expect_eq(err_after_clear, true);
}

test("5. Dynamic cap.pqueue namespace mount and std.testing.allocator zero-leak verification") {
    const alloc = std.testing.allocator;

    let pq = cap.time.IndexedMinHeap<i32>.init(alloc);
    defer pq.deinit();
    const h1 = pq.push(20, 200)?;
    const h2 = pq.push(10, 100)?;
    pq.updatePriority(h1, 5)?;
    expect_eq(pq.pop().?.value, 100 * 2); // h1 with prio 5
    expect_eq(pq.pop().?.handle, h2);

    let topk = cap.time.BoundedTopK<i32>.init(alloc, 2)?;
    defer topk.deinit();
    topk.push(1.0, 10)?;
    topk.push(9.0, 90)?;
    topk.push(5.0, 50)?;
    const res = topk.topSorted(alloc)?;
    defer alloc.free(res);
    expect_eq(res.len, 2);
    expect_eq(res[0].value, 90);
    expect_eq(res[1].value, 50);

    let sm = cap.time.StreamingMedian.init(alloc);
    defer sm.deinit();
    sm.push(100.0)?;
    sm.push(200.0)?;
    expect_eq(sm.median()?, 150.0);
}
