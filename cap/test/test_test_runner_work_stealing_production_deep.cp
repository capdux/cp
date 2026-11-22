// Production Deep Tests for cap:test_runner Work-Stealing Scheduler (lib/test_runner/work_stealing.cp)
import { test, expect_eq } from "cap:test";
import {
    WORK_STEAL_MAX_WORKERS,
    FileRange,
    WorkStealPool,
} from "cap:test_runner";

test("test_runner work_stealing: FileRange init, popFront, popBack and contains", fn() {
    let r = FileRange.init(10, 14);
    expect_eq(r.len(), @as(u32, 4));
    expect_eq(r.isEmpty(), false);
    expect_eq(r.contains(9), false);
    expect_eq(r.contains(10), true);
    expect_eq(r.contains(13), true);
    expect_eq(r.contains(14), false);

    expect_eq(r.popFront() ?? @as(u32, 999), @as(u32, 10));
    expect_eq(r.popBack() ?? @as(u32, 999), @as(u32, 13));
    expect_eq(r.len(), @as(u32, 2));
    expect_eq(r.popFront() ?? @as(u32, 999), @as(u32, 11));
    expect_eq(r.popFront() ?? @as(u32, 999), @as(u32, 12));
    expect_eq(r.isEmpty(), true);
    expect_eq(r.popFront() == null, true);
    expect_eq(r.popBack() == null, true);

    // Inverted range lo > hi normalizes to empty
    const inv = FileRange.init(20, 5);
    expect_eq(inv.isEmpty(), true);
    expect_eq(inv.len(), @as(u32, 0));
});

test("test_runner work_stealing: stealBackHalf even, odd, single-element and empty", fn() {
    // Even len=6: [0, 6) -> owner [0, 3), thief [3, 6)
    let even = FileRange.init(0, 6);
    const s_even = even.stealBackHalf() ?? unreachable;
    expect_eq(even.lo, @as(u32, 0));
    expect_eq(even.hi, @as(u32, 3));
    expect_eq(s_even.lo, @as(u32, 3));
    expect_eq(s_even.hi, @as(u32, 6));

    // Odd len=5: [10, 15) -> mid=12 -> owner [10, 12) (2), thief [12, 15) (3)
    let odd = FileRange.init(10, 15);
    const s_odd = odd.stealBackHalf() ?? unreachable;
    expect_eq(odd.len(), @as(u32, 2));
    expect_eq(s_odd.len(), @as(u32, 3));
    expect_eq(s_odd.lo, @as(u32, 12));

    // Single element len=1: [7, 8) -> single file goes to thief, owner becomes empty
    let single = FileRange.init(7, 8);
    const s_single = single.stealBackHalf() ?? unreachable;
    expect_eq(single.isEmpty(), true);
    expect_eq(s_single.lo, @as(u32, 7));
    expect_eq(s_single.hi, @as(u32, 8));

    // Empty range returns null
    expect_eq(single.stealBackHalf() == null, true);
});

test("test_runner work_stealing: WorkStealPool partition remainder, small files and clamps", fn() {
    // 10 files across 4 workers -> [0,3), [3,6), [6,8), [8,10)
    const p1 = WorkStealPool.partition(10, 4);
    expect_eq(p1.worker_count, @as(usize, 4));
    expect_eq(p1.totalRemaining(), @as(u32, 10));
    expect_eq(p1.rangeOf(0).len(), @as(u32, 3));
    expect_eq(p1.rangeOf(1).len(), @as(u32, 3));
    expect_eq(p1.rangeOf(2).len(), @as(u32, 2));
    expect_eq(p1.rangeOf(3).len(), @as(u32, 2));

    // Fewer files than workers: 2 files across 4 workers -> [0,1), [1,2), empty, empty
    const p2 = WorkStealPool.partition(2, 4);
    expect_eq(p2.rangeOf(0).len(), @as(u32, 1));
    expect_eq(p2.rangeOf(1).len(), @as(u32, 1));
    expect_eq(p2.rangeOf(2).isEmpty(), true);
    expect_eq(p2.rangeOf(3).isEmpty(), true);

    // Zero workers -> empty pool; > MAX_WORKERS clamps to WORK_STEAL_MAX_WORKERS
    const p_zero = WorkStealPool.partition(10, 0);
    expect_eq(p_zero.worker_count, @as(usize, 0));
    expect_eq(p_zero.totalRemaining(), @as(u32, 0));

    const p_clamped = WorkStealPool.partition(100, 999);
    expect_eq(p_clamped.worker_count, WORK_STEAL_MAX_WORKERS);
    expect_eq(p_clamped.totalRemaining(), @as(u32, 100));
});

test("test_runner work_stealing: end-to-end imbalanced worker stealing visits every file exactly once", fn() {
    // 17 files across 4 workers; worker 0 drains its own range and steals until all 17 complete
    let pool = WorkStealPool.partition(17, 4);
    let seen: [17]u8 = [0; 17];
    let executed: u32 = 0;

    // Fast worker 0 executes 10 tasks (stealing from workers 1..3), slow worker 2 executes 2, then worker 0 finishes rest
    let step: u32 = 0;
    while (step < 10) {
        const idx = pool.nextFor(0) ?? unreachable;
        seen[@as(usize, idx)] += 1;
        executed += 1;
        step += 1;
    }
    if (idx_a := pool.nextFor(2)) {
        seen[@as(usize, idx_a)] += 1;
        executed += 1;
    }
    if (idx_b := pool.nextFor(2)) {
        seen[@as(usize, idx_b)] += 1;
        executed += 1;
    }
    while (true) {
        if (idx := pool.nextFor(0)) {
            seen[@as(usize, idx)] += 1;
            executed += 1;
        } else {
            break;
        }
    }

    expect_eq(executed, @as(u32, 17));
    expect_eq(pool.totalRemaining(), @as(u32, 0));
    let i: usize = 0;
    while (i < 17) {
        expect_eq(seen[i], @as(u8, 1));
        i += 1;
    }
});

test("test_runner work_stealing: out-of-bounds worker id and exhausted pool guards", fn() {
    let pool = WorkStealPool.partition(3, 2);
    expect_eq(pool.rangeOf(99).isEmpty(), true);
    expect_eq(pool.stealFromBusiest(99) == null, true);
    expect_eq(pool.nextFor(99) == null, true);

    // Exhaust pool and verify subsequent steals/pops return null
    * = pool.nextFor(0);
    * = pool.nextFor(0);
    * = pool.nextFor(0);
    expect_eq(pool.nextFor(0) == null, true);
    expect_eq(pool.nextFor(1) == null, true);
    expect_eq(pool.stealFromBusiest(0) == null, true);
});
