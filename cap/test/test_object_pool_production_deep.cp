// Source: cp/cap/test/test_object_pool_production_deep.cp (Production deep tests for cap:object_pool)

import { test, expect_eq } from "cap:test";
import { ObjectPool, PoolNode } from "cap:object_pool";

struct ResettableWorkItem {
    id: u32 = 0,
    dirty: bool = false,
    reset_calls: u32 = 0,

    fn reset(*_) {
        _.id = 0;
        _.dirty = false;
        _.reset_calls += 1;
    }
}

test("acquire/recycle reuse hit rate and physical address reuse") {
    const alloc = std.testing.allocator;
    let pool = ObjectPool<u64>.init(alloc, 8);

    expect_eq(pool.isEmpty(), true);
    expect_eq(pool.hitRatePermille(), @as(u32, 0));

    const first = pool.acquire() catch unreachable;
    let first_addr: usize = 0;
    unsafe {
        first_addr = first as usize;
        first.data = 12345;
    }
    pool.recycle(first);

    // 再循环借出并归还 19 次，全部应命中同一物理地址
    let i: usize = 0;
    while (i < 19) {
        const node = pool.acquire() catch unreachable;
        let cur_addr: usize = 0;
        unsafe { cur_addr = node as usize; }
        expect_eq(cur_addr, first_addr);
        pool.recycle(node);
        i += 1;
    }

    expect_eq(pool.allocated(), @as(usize, 1));
    expect_eq(pool.hits(), @as(usize, 19));
    expect_eq(pool.hitRatePermille(), @as(u32, 950));
    expect_eq(pool.count(), @as(usize, 1));
}

test("max_count cap overflow frees excess nodes immediately without caching") {
    const alloc = std.testing.allocator;
    let pool = ObjectPool<u32>.init(alloc, 3);

    let nodes: [5](*PoolNode<u32>!)? = [null; 5];
    let i: usize = 0;
    while (i < 5) {
        nodes[i] = pool.acquireWith((i as u32) + 1) catch unreachable;
        i += 1;
    }
    expect_eq(pool.allocated(), @as(usize, 5));
    expect_eq(pool.count(), @as(usize, 0));

    // 归还 5 个节点：前 3 个入池，后 2 个超出 max_count 直接销毁释放
    let j: usize = 0;
    while (j < 5) {
        if (n := nodes[j]) {
            pool.recycle(n);
        }
        j += 1;
    }

    expect_eq(pool.count(), @as(usize, 3));
    expect_eq(pool.isFull(), true);
    expect_eq(pool.freed(), @as(usize, 2));
}

test("Unbounded mode (max_count == 0) caches all recycled nodes and cleans up via ~init()") {
    const alloc = std.testing.allocator;
    let pool = ObjectPool<u64>.initUnbounded(alloc);

    let nodes: [32](*PoolNode<u64>!)? = [null; 32];
    let i: usize = 0;
    while (i < 32) {
        nodes[i] = pool.acquireWith(i as u64) catch unreachable;
        i += 1;
    }
    expect_eq(pool.allocated(), @as(usize, 32));

    let j: usize = 0;
    while (j < 32) {
        if (n := nodes[j]) {
            pool.recycle(n);
        }
        j += 1;
    }

    expect_eq(pool.isFull(), false);
    expect_eq(pool.count(), @as(usize, 32));
    expect_eq(pool.freed(), @as(usize, 0));
    // 离开作用域自动触发 ~init() 释放全部 32 个节点，std.testing.allocator 验证零泄漏
}

test("Custom struct auto-reset hook on reuse") {
    const alloc = std.testing.allocator;
    let pool = ObjectPool<ResettableWorkItem>.init(alloc, 4);

    const n1 = pool.acquire() catch unreachable;
    unsafe {
        expect_eq(n1.data.reset_calls, @as(u32, 0));
        n1.data.id = 999;
        n1.data.dirty = true;
    }
    pool.recycle(n1);

    // 复用借出时自动触发 reset()
    const n2 = pool.acquire() catch unreachable;
    unsafe {
        expect_eq(n2.data.id, @as(u32, 0));
        expect_eq(n2.data.dirty, false);
        expect_eq(n2.data.reset_calls, @as(u32, 1));
    }
    pool.recycle(n2);

    const n3 = pool.acquireIfCached().?;
    unsafe {
        expect_eq(n3.data.reset_calls, @as(u32, 2));
    }
    pool.recycle(n3);
}

test("acquireIfCached, acquireWith and explicit destroy behavior") {
    const alloc = std.testing.allocator;
    let pool = ObjectPool<i32>.init(alloc, 4);

    // 空池 acquireIfCached 返回 null 且不触发分配
    expect_eq(pool.acquireIfCached() == null, true);
    expect_eq(pool.allocated(), @as(usize, 0));

    const n1 = pool.acquireWith(777) catch unreachable;
    unsafe { expect_eq(n1.data, @as(i32, 777)); }
    pool.recycle(n1);

    if (cached := pool.acquireIfCached()) {
        unsafe { expect_eq(cached.data, @as(i32, 777)); }
        pool.destroy(cached);
    } else {
        expect_eq(true, false);
    }

    expect_eq(pool.isEmpty(), true);
    expect_eq(pool.freed(), @as(usize, 1));
}

test("Idempotent clear and ~init() zero-leak lifecycle") {
    const alloc = std.testing.allocator;
    let pool = ObjectPool<u64>.init(alloc, 10);

    let round: usize = 0;
    while (round < 3) {
        let batch: [6](*PoolNode<u64>!)? = [null; 6];
        let i: usize = 0;
        while (i < 6) {
            batch[i] = pool.acquireWith((round * 10 + i) as u64) catch unreachable;
            i += 1;
        }
        let j: usize = 0;
        while (j < 6) {
            if (n := batch[j]) {
                pool.recycle(n);
            }
            j += 1;
        }
        expect_eq(pool.count(), @as(usize, 6));
        pool.clear();
        expect_eq(pool.count(), @as(usize, 0));
        // 幂等二次 clear
        pool.clear();
        round += 1;
    }

    expect_eq(pool.allocated(), pool.freed());
}
