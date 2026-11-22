// Source: cp/cap/test/test_pool_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:pool & cap.pool

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PoolError,
    BorrowOrder,
    PoolConfig,
    PooledHandle,
    PoolStats,
    ResourcePool,
} from "cap:time";

struct MockConn {
    id: u64;
    healthy: bool;
}

struct MockPoolCtx {
    next_id: u64 = 1;
    created: u64 = 0;
    destroyed: u64 = 0;
    reject_id: u64 = 0;
}

fn mockCreate(ctx_ptr: *anyopaque, alloc: std.mem.Allocator): *anyopaque ! PoolError {
    unsafe {
        let ctx = ctx_ptr as *MockPoolCtx!;
        let conn = alloc.create(MockConn) catch return PoolError.CreateFailed;
        conn.id = ctx.next_id;
        conn.healthy = true;
        ctx.next_id += 1;
        ctx.created += 1;
        return conn as *anyopaque!;
    }
}

fn mockDestroy(ctx_ptr: *anyopaque, res_ptr: *anyopaque, alloc: std.mem.Allocator) {
    unsafe {
        let ctx = ctx_ptr as *MockPoolCtx!;
        let conn = res_ptr as *MockConn!;
        alloc.destroy(conn);
        ctx.destroyed += 1;
    }
}

fn mockValidate(ctx_ptr: *anyopaque, res_ptr: *anyopaque): bool {
    unsafe {
        let ctx = ctx_ptr as *MockPoolCtx!;
        let conn = res_ptr as *MockConn!;
        if (ctx.reject_id != 0 && conn.id == ctx.reject_id) return false;
        return conn.healthy;
    }
}

fn getConnId(handle: PooledHandle): u64 {
    unsafe {
        const conn = handle.ptr as *MockConn!;
        return conn.id;
    }
}

fn setConnHealthy(handle: PooledHandle, healthy: bool) {
    unsafe {
        let conn = handle.ptr as *MockConn!;
        conn.healthy = healthy;
    }
}

fn testWarmupLifoFifoExhaustionAndRecovery(): ! {
    const alloc = std.heap.page_allocator;

    // 1. LIFO pool: warmup 2 (id=1, id=2), max_size=3
    let ctx_lifo = MockPoolCtx{};
    unsafe {
        let pool = ResourcePool.init(
            alloc,
            PoolConfig{
                min_idle: 2,
                max_size: 3,
                idle_timeout_ms: 0,
                max_lifetime_ms: 0,
                borrow_order: BorrowOrder.Lifo,
            },
            *ctx_lifo as *anyopaque!,
            mockCreate,
            mockDestroy,
            mockValidate,
        )?;
        defer pool.deinit();

        pool.warmup(100)?;
        let st = pool.stats();
        expect_eq(st.total_created, 2 as u64);
        expect_eq(st.idle_count, 2 as usize);
        expect_eq(st.active_count, 0 as usize);

        // LIFO pops most recently pushed (id=2) first, then id=1, then creates id=3
        const h1 = pool.acquire(110)?;
        expect_eq(getConnId(h1), 2 as u64);
        expect_eq(h1.borrow_count, 1 as u64);

        const h2 = pool.acquire(111)?;
        expect_eq(getConnId(h2), 1 as u64);

        const h3 = pool.acquire(112)?;
        expect_eq(getConnId(h3), 3 as u64);
        expect_eq(pool.stats().active_count, 3 as usize);
        expect_eq(pool.stats().idle_count, 0 as usize);

        // 4th acquire must fail with PoolExhausted
        let got_exhausted = false;
        * = pool.acquire(113) catch |e| {
            if (std.mem.eql(u8, @errorName(e), "PoolExhausted")) { got_exhausted = true; }
        };
        expect_eq(got_exhausted, true);
        expect_eq(pool.stats().exhausted_rejects, 1 as u64);

        // Checkin h1 (id=2), then h3 (id=3) -> LIFO should hand out id=3 next
        pool.checkin(h1, 120)?;
        pool.checkin(h3, 121)?;
        expect_eq(pool.stats().active_count, 1 as usize);
        expect_eq(pool.stats().idle_count, 2 as usize);

        const h_again = pool.acquire(130)?;
        expect_eq(getConnId(h_again), 3 as u64);
        expect_eq(h_again.borrow_count, 2 as u64);

        pool.checkin(h_again, 131)?;
        pool.checkin(h2, 132)?;
    }
    expect_eq(ctx_lifo.created, 3 as u64);
    expect_eq(ctx_lifo.destroyed, 3 as u64);

    // 2. FIFO pool: warmup 2 (id=1, id=2) -> acquire hands out id=1 first, then id=2
    let ctx_fifo = MockPoolCtx{};
    unsafe {
        let fpool = ResourcePool.init(
            alloc,
            PoolConfig{
                min_idle: 2,
                max_size: 2,
                idle_timeout_ms: 0,
                max_lifetime_ms: 0,
                borrow_order: BorrowOrder.Fifo,
            },
            *ctx_fifo as *anyopaque!,
            mockCreate,
            mockDestroy,
            null,
        )?;
        defer fpool.deinit();

        fpool.warmup(200)?;
        const a = fpool.acquire(210)?;
        const b = fpool.acquire(211)?;
        expect_eq(getConnId(a), 1 as u64);
        expect_eq(getConnId(b), 2 as u64);

        fpool.checkin(a, 220)?;
        fpool.checkin(b, 221)?;

        const c = fpool.acquire(230)?;
        expect_eq(getConnId(c), 1 as u64);
        fpool.checkin(c, 231)?;
    }
    expect_eq(ctx_fifo.created, 2 as u64);
    expect_eq(ctx_fifo.destroyed, 2 as u64);
}

fn testIdleTimeoutMaxLifetimeAndEvictExpired(): ! {
    const alloc = std.heap.page_allocator;
    let ctx = MockPoolCtx{};

    unsafe {
        let pool = ResourcePool.init(
            alloc,
            PoolConfig{
                min_idle: 1,
                max_size: 4,
                idle_timeout_ms: 500,
                max_lifetime_ms: 2000,
                borrow_order: BorrowOrder.Fifo,
            },
            *ctx as *anyopaque!,
            mockCreate,
            mockDestroy,
            null,
        )?;
        defer pool.deinit();

        // Warmup at t=1000 -> id=1
        pool.warmup(1000)?;
        // Acquire id=1 and id=2 at t=1100, checkin both at t=1200 and t=1400
        const h1 = pool.acquire(1100)?;
        const h2 = pool.acquire(1100)?;
        expect_eq(getConnId(h1), 1 as u64);
        expect_eq(getConnId(h2), 2 as u64);
        pool.checkin(h1, 1200)?; // idle since 1200
        pool.checkin(h2, 1400)?; // idle since 1400

        // At t=1750: h1 idle for 550ms (>=500ms -> evicted), h2 idle for 350ms (kept).
        // min_idle=1 is already satisfied by h2, so no replenishment needed.
        const evicted1 = pool.evictExpired(1750)?;
        expect_eq(evicted1, 1 as usize);
        expect_eq(pool.stats().evicted_idle, 1 as u64);
        expect_eq(pool.stats().idle_count, 1 as usize);

        // At t=2500: h2 idle for 1100ms (>=500ms -> evicted); evictExpired replenishes min_idle=1 (id=3)
        const evicted2 = pool.evictExpired(2500)?;
        expect_eq(evicted2, 1 as usize);
        expect_eq(pool.stats().evicted_idle, 2 as u64);
        expect_eq(pool.stats().idle_count, 1 as usize);
        expect_eq(pool.stats().total_created, 3 as u64);

        // Acquire id=3 (created at 2500), hold until t=4600 (> max_lifetime_ms 2000), checkin must destroy it
        const h3 = pool.acquire(2600)?;
        expect_eq(getConnId(h3), 3 as u64);
        pool.checkin(h3, 4600)?;
        expect_eq(pool.stats().evicted_lifetime, 1 as u64);
        expect_eq(pool.stats().idle_count, 0 as usize);
        expect_eq(pool.stats().total_destroyed, 3 as u64);

        // Next acquire transparently builds id=4
        const h4 = pool.acquire(4650)?;
        expect_eq(getConnId(h4), 4 as u64);
        pool.checkin(h4, 4660)?;
    }
    expect_eq(ctx.created, 4 as u64);
    expect_eq(ctx.destroyed, 4 as u64);
}

fn testValidationFailureDiscardAndInvalidHandle(): ! {
    const alloc = std.heap.page_allocator;
    let ctx = MockPoolCtx{};

    unsafe {
        let pool = ResourcePool.init(
            alloc,
            PoolConfig{
                min_idle: 0,
                max_size: 2,
                idle_timeout_ms: 0,
                max_lifetime_ms: 0,
                borrow_order: BorrowOrder.Lifo,
            },
            *ctx as *anyopaque!,
            mockCreate,
            mockDestroy,
            mockValidate,
        )?;
        defer pool.deinit();

        const h1 = pool.acquire(100)?;
        const h2 = pool.acquire(100)?;
        expect_eq(getConnId(h1), 1 as u64);
        expect_eq(getConnId(h2), 2 as u64);

        // Mark h2 unhealthy before returning, then checkin both
        setConnHealthy(h2, false);
        pool.checkin(h1, 110)?;
        pool.checkin(h2, 110)?;

        // LIFO pops h2 first -> validate fails -> destroyed -> pops h1 -> valid!
        const h_ok = pool.acquire(120)?;
        expect_eq(getConnId(h_ok), 1 as u64);
        expect_eq(pool.stats().failed_validations, 1 as u64);
        expect_eq(pool.stats().total_destroyed, 1 as u64);

        // Acquire second slot (creates id=3), then discard it as broken
        const h3 = pool.acquire(130)?;
        expect_eq(getConnId(h3), 3 as u64);
        pool.discard(h3)?;
        expect_eq(pool.stats().total_destroyed, 2 as u64);
        expect_eq(pool.stats().active_count, 1 as usize);

        // Double discard on h3 must return InvalidHandle
        let got_invalid = false;
        pool.discard(h3) catch |e| {
            if (std.mem.eql(u8, @errorName(e), "InvalidHandle")) { got_invalid = true; }
        };
        expect_eq(got_invalid, true);

        // Slot quota was freed by discard, so acquiring again succeeds (creates id=4)
        const h4 = pool.acquire(140)?;
        expect_eq(getConnId(h4), 4 as u64);

        pool.checkin(h_ok, 150)?;
        pool.checkin(h4, 150)?;
    }
    expect_eq(ctx.created, 4 as u64);
    expect_eq(ctx.destroyed, 4 as u64);
}

fn testDeinitZeroLeakAccountingAndClosedGuard(): ! {
    const alloc = std.heap.page_allocator;
    let ctx = MockPoolCtx{};

    unsafe {
        let pool = ResourcePool.init(
            alloc,
            PoolConfig{
                min_idle: 2,
                max_size: 4,
                idle_timeout_ms: 0,
                max_lifetime_ms: 0,
                borrow_order: BorrowOrder.Lifo,
            },
            *ctx as *anyopaque!,
            mockCreate,
            mockDestroy,
            null,
        )?;

        pool.warmup(1000)?;
        const h1 = pool.acquire(1010)?; // active
        const h2 = pool.acquire(1010)?; // active
        const h3 = pool.acquire(1010)?; // active
        pool.checkin(h1, 1020)?;        // back to idle (1 idle, 2 active)
        * = h2;
        * = h3;

        expect_eq(pool.stats().total_created, 3 as u64);
        expect_eq(pool.stats().active_count, 2 as usize);
        expect_eq(pool.stats().idle_count, 1 as usize);

        // deinit must destroy both active and idle resources
        pool.deinit();
        const final_stats = pool.stats();
        expect_eq(final_stats.total_created, 3 as u64);
        expect_eq(final_stats.total_destroyed, 3 as u64);
        expect_eq(final_stats.active_count, 0 as usize);
        expect_eq(final_stats.idle_count, 0 as usize);
        expect_eq(ctx.created, ctx.destroyed);

        // Acquire after deinit must return PoolClosed
        let got_closed = false;
        * = pool.acquire(1030) catch |e| {
            if (std.mem.eql(u8, @errorName(e), "PoolClosed")) { got_closed = true; }
        };
        expect_eq(got_closed, true);
    }
}

fn testCapPoolDynamicLazyMount(): ! {
    const alloc = std.heap.page_allocator;
    let ctx = MockPoolCtx{};

    unsafe {
        let pool = cap.time.ResourcePool.init(
            alloc,
            cap.time.PoolConfig{
                min_idle: 1,
                max_size: 2,
                idle_timeout_ms: 1000,
                max_lifetime_ms: 5000,
                borrow_order: cap.time.BorrowOrder.Fifo,
            },
            *ctx as *anyopaque!,
            mockCreate,
            mockDestroy,
            mockValidate,
        )?;
        defer pool.deinit();

        pool.warmup(500)?;
        const h = pool.acquire(600)?;
        expect_eq(getConnId(h), 1 as u64);
        pool.checkin(h, 700)?;
        expect_eq(pool.stats().idle_count, 1 as usize);
    }
    expect_eq(ctx.created, 1 as u64);
    expect_eq(ctx.destroyed, 1 as u64);
}

fn main() {
    test("1. warmup, LIFO/FIFO BorrowOrder, max_size PoolExhausted & checkin Recovery", () => {
        testWarmupLifoFifoExhaustionAndRecovery()?;
    });
    test("2. idle_timeout_ms & max_lifetime_ms Eviction, Rebuild & evictExpired", () => {
        testIdleTimeoutMaxLifetimeAndEvictExpired()?;
    });
    test("3. validate_fn Auto-Destroy/Replace, discard Quota Release & InvalidHandle", () => {
        testValidationFailureDiscardAndInvalidHandle()?;
    });
    test("4. deinit() Zero-Leak Cleanup of Active+Idle Slots & PoolClosed Guard", () => {
        testDeinitZeroLeakAccountingAndClosedGuard()?;
    });
    test("5. import cap from \"cap\"; cap.pool Dynamic Lazy Mounting", () => {
        testCapPoolDynamicLazyMount()?;
    });
}
