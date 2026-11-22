// cap/test/testEventOneShotTask.cp
// Production-grade deep tests for cap:event oneShotTask.cp (Bun event_loop/ManagedTask pool)

import { test, assert, assertEq } from "cap:test";
import {
    OneShotTaskError,
    OneShotTaskState,
    OneShotTaskHandle,
    OneShotTaskSlot,
    OneShotRunOutcome,
    OneShotTaskPool,
} from "cap:event";

fn addCallback(ctx: u64, payload: u64): i32 {
    return @as(i32, @intCast(ctx + payload));
}

fn errorCodeCallback(_: u64, _: u64): i32 {
    return -42;
}

test("OneShotTaskPool: schedule & run auto-reclaims slot (one-shot semantics)", () => {
    let slots: [8]OneShotTaskSlot = [OneShotTaskSlot{}; 8];
    let pool = OneShotTaskPool.init(slots[0..]);
    assertEq(pool.capacity(), @as(usize, 8));
    assertEq(pool.activeCount(), @as(usize, 0));

    const h = pool.schedule(100, 23, addCallback) catch OneShotTaskHandle{};
    assert(pool.isAlive(h));
    assertEq(pool.activeCount(), @as(usize, 1));
    assertEq(pool.pendingCount(), @as(usize, 1));
    assertEq(pool.stateOf(h) catch OneShotTaskState.Empty, OneShotTaskState.Pending);

    const out = pool.run(h) catch OneShotRunOutcome{};
    assert(out.executed);
    assert(!out.was_cancelled);
    assertEq(out.result_code, @as(i32, 123));
    assertEq(out.ctx_id, @as(u64, 100));

    // Slot is immediately reclaimed after run
    assert(!pool.isAlive(h));
    assertEq(pool.activeCount(), @as(usize, 0));
    assertEq(pool.pendingCount(), @as(usize, 0));
    assertEq(pool.completedTotal(), @as(usize, 1));

    // Second run with same handle must fail with StaleHandle
    let got_stale = false;
    * = pool.run(h) catch |e| {
        if (e == OneShotTaskError.StaleHandle) {
            got_stale = true;
        }
        OneShotRunOutcome{}
    };
    assert(got_stale);
});

test("OneShotTaskPool: cancel replaces callback with noop & is idempotent", () => {
    let slots: [4]OneShotTaskSlot = [OneShotTaskSlot{}; 4];
    let pool = OneShotTaskPool.init(slots[0..]);

    const h = pool.schedule(7, 99, errorCodeCallback) catch OneShotTaskHandle{};
    assert(pool.cancel(h) catch false);
    // Second cancel is idempotent no-op returning false
    assert(!(pool.cancel(h) catch true));
    assertEq(pool.stateOf(h) catch OneShotTaskState.Empty, OneShotTaskState.Cancelled);
    assertEq(pool.pendingCount(), @as(usize, 0));
    assertEq(pool.activeCount(), @as(usize, 1));
    assertEq(pool.cancelledTotal(), @as(usize, 1));

    // Running a cancelled task does not invoke original callback and reclaims slot
    const out = pool.run(h) catch OneShotRunOutcome{};
    assert(!out.executed);
    assert(out.was_cancelled);
    assertEq(out.result_code, @as(i32, 0));
    assertEq(pool.activeCount(), @as(usize, 0));
    assertEq(pool.completedTotal(), @as(usize, 0));
});

test("OneShotTaskPool: ABA generation guard after slot reuse", () => {
    let slots: [1]OneShotTaskSlot = [OneShotTaskSlot{}; 1];
    let pool = OneShotTaskPool.init(slots[0..]);

    const h1 = pool.schedule(1, 2, addCallback) catch OneShotTaskHandle{};
    * = pool.run(h1) catch OneShotRunOutcome{};

    // Reuse the only slot (index 0)
    const h2 = pool.schedule(10, 20, addCallback) catch OneShotTaskHandle{};
    assertEq(h2.index, h1.index);
    assert(h2.generation != h1.generation);
    assert(!h1.eql(h2));
    assert(!pool.isAlive(h1));
    assert(pool.isAlive(h2));

    // Old handle cannot cancel or run the new occupant
    let got_stale = false;
    * = pool.cancel(h1) catch |e| {
        if (e == OneShotTaskError.StaleHandle) {
            got_stale = true;
        }
        false
    };
    assert(got_stale);

    const out = pool.run(h2) catch OneShotRunOutcome{};
    assert(out.executed);
    assertEq(out.result_code, @as(i32, 30));
});

test("OneShotTaskPool: runAllPending bounded batch drain & full drain", () => {
    let slots: [8]OneShotTaskSlot = [OneShotTaskSlot{}; 8];
    let pool = OneShotTaskPool.init(slots[0..]);

    const h0 = pool.schedule(1, 1, addCallback) catch OneShotTaskHandle{};
    const h1 = pool.schedule(2, 2, addCallback) catch OneShotTaskHandle{};
    const h2 = pool.schedule(3, 3, addCallback) catch OneShotTaskHandle{};
    const h3 = pool.schedule(4, 4, addCallback) catch OneShotTaskHandle{};
    * = pool.cancel(h1) catch false;
    * = h0;
    * = h2;
    * = h3;

    // Drain at most 2 slots first
    assertEq(pool.runAllPending(2), @as(usize, 2));
    assertEq(pool.activeCount(), @as(usize, 2));

    // Drain all remaining slots (max_count == 0)
    assertEq(pool.runAllPending(0), @as(usize, 2));
    assertEq(pool.activeCount(), @as(usize, 0));
    assertEq(pool.completedTotal(), @as(usize, 3));
    assertEq(pool.cancelledTotal(), @as(usize, 1));
});

test("OneShotTaskPool: large external slice (256 slots, no hardcoded cap)", () => {
    let slots: [256]OneShotTaskSlot = [OneShotTaskSlot{}; 256];
    let pool = OneShotTaskPool.init(slots[0..]);

    let i: usize = 0;
    while (i < 256) {
        * = pool.schedule(@as(u64, i), 1, addCallback) catch OneShotTaskHandle{};
        i += 1;
    }
    assertEq(pool.activeCount(), @as(usize, 256));
    assertEq(pool.pendingCount(), @as(usize, 256));
    assertEq(pool.runAllPending(0), @as(usize, 256));
    assertEq(pool.activeCount(), @as(usize, 0));
    assertEq(pool.completedTotal(), @as(usize, 256));
});

test("OneShotTaskPool: PoolFull & InvalidHandle & zero-cap slice guards", () => {
    let empty_slots: [0]OneShotTaskSlot = [];
    let empty_pool = OneShotTaskPool.init(empty_slots[0..]);
    let got_empty_full = false;
    * = empty_pool.schedule(1, 1, addCallback) catch |e| {
        if (e == OneShotTaskError.PoolFull) {
            got_empty_full = true;
        }
        OneShotTaskHandle{}
    };
    assert(got_empty_full);

    let slots: [2]OneShotTaskSlot = [OneShotTaskSlot{}; 2];
    let pool = OneShotTaskPool.init(slots[0..]);
    * = pool.schedule(1, 1, addCallback) catch OneShotTaskHandle{};
    * = pool.schedule(2, 2, addCallback) catch OneShotTaskHandle{};

    let got_full = false;
    * = pool.schedule(3, 3, addCallback) catch |e| {
        if (e == OneShotTaskError.PoolFull) {
            got_full = true;
        }
        OneShotTaskHandle{}
    };
    assert(got_full);

    const bad = OneShotTaskHandle{ index: 99, generation: 1 };
    assert(!pool.isAlive(bad));
    let got_invalid = false;
    * = pool.stateOf(bad) catch |e| {
        if (e == OneShotTaskError.InvalidHandle) {
            got_invalid = true;
        }
        OneShotTaskState.Empty
    };
    assert(got_invalid);
});
