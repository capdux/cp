// Origin: Pure-CP Stdlib Test Suite (lib/db/conn_pool.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Database Connection Pool State Machine:
//   1. Create, bindHandle, LIFO warm reuse & Exhausted backpressure at max_conns
//   2. Idle timeout & max lifetime auto-eviction during acquire
//   3. Unhealthy release consecutive error threshold & reset-on-healthy behavior
//   4. Max-lifetime retirement on release
//   5. Batch evictExpired respecting min_idle floor vs hard max_lifetime
//   6. Stats accuracy & invalid config / undersized slots / double-release defensive guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DbPoolConfig,
    DbConnState,
    DbConnSlot,
    DbAcquireKind,
    DbAcquireOutcome,
    DbPoolStats,
    DbConnPool,
} from "cap:db";

// ============================================================================
// Test 1: Create, bindHandle, LIFO Reuse & Backpressure Exhausted
// ============================================================================

fn testCreateReuseAndExhausted()! {
    let slots: [4]DbConnSlot = [DbConnSlot{}; 4];
    const cfg = cap.db.DbPoolConfig.init(2, 0, 60000, 600000, 1) ?? return error.InitFailed;
    let pool = cap.db.DbConnPool.init(slots[0..], cfg) ?? return error.InitFailed;

    // Acquire 2 new connections -> fills max_conns = 2
    const a1 = pool.acquire(1000);
    expect_eq(a1.kind == DbAcquireKind.Created, true);
    expect_eq(a1.conn_id, 1);
    expect_eq(pool.bindHandle(a1.slot_index, 1001), true);

    const a2 = pool.acquire(1010);
    expect_eq(a2.kind == DbAcquireKind.Created, true);
    expect_eq(a2.conn_id, 2);
    expect_eq(pool.bindHandle(a2.slot_index, 1002), true);

    // 3rd acquire hits backpressure -> Exhausted
    const a3 = pool.acquire(1020);
    expect_eq(a3.kind == DbAcquireKind.Exhausted, true);

    // Release a1 at t=1100, then a2 at t=1200 -> LIFO should reuse a2 (warmest) first!
    expect_eq(pool.release(a1.slot_index, true, 1100), true);
    expect_eq(pool.release(a2.slot_index, true, 1200), true);

    const r1 = pool.acquire(1300);
    expect_eq(r1.kind == DbAcquireKind.Reused, true);
    expect_eq(r1.conn_id, 2);
    expect_eq(r1.handle_tag, 1002);

    const st = pool.stats();
    expect_eq(st.active, 1);
    expect_eq(st.idle, 1);
    expect_eq(st.total_acquired, 3);
    expect_eq(st.total_created, 2);
    expect_eq(st.total_reused, 1);
    expect_eq(st.total_exhausted, 1);
}

// ============================================================================
// Test 2: Idle Timeout & Max Lifetime Auto-Eviction During Acquire
// ============================================================================

fn testIdleAndLifetimeEvictionOnAcquire()! {
    let slots: [2]DbConnSlot = [DbConnSlot{}; 2];
    const cfg = DbPoolConfig.init(2, 0, 5000, 20000, 1) ?? return error.InitFailed;
    let pool = DbConnPool.init(slots[0..], cfg) ?? return error.InitFailed;

    const a1 = pool.acquire(1000);
    expect_eq(pool.release(a1.slot_index, true, 2000), true);

    // Acquire at t = 8000 (idle for 6000ms >= 5000ms) -> old conn evicted, fresh Created returned!
    const a2 = pool.acquire(8000);
    expect_eq(a2.kind == DbAcquireKind.Created, true);
    expect_eq(a2.conn_id, 2);
    expect_eq(pool.stats().total_evicted, 1);
}

// ============================================================================
// Test 3: Unhealthy Release Threshold & Reset-on-Healthy
// ============================================================================

fn testUnhealthyReleaseAndConsecutiveErrors()! {
    let slots: [2]DbConnSlot = [DbConnSlot{}; 2];
    // Tolerates 1 transient error, drops on 2nd consecutive error
    const cfg = DbPoolConfig.init(2, 0, 60000, 600000, 2) ?? return error.InitFailed;
    let pool = DbConnPool.init(slots[0..], cfg) ?? return error.InitFailed;

    const a1 = pool.acquire(1000);
    // 1st unhealthy release -> consecutive_errors = 1 (< 2), returns to Idle
    expect_eq(pool.release(a1.slot_index, false, 1100), true);
    expect_eq(pool.stats().idle, 1);
    expect_eq(pool.stats().total_unhealthy_drops, 0);

    // Reuse and release healthy -> resets consecutive_errors to 0
    const a2 = pool.acquire(1200);
    expect_eq(a2.kind == DbAcquireKind.Reused, true);
    expect_eq(pool.release(a2.slot_index, true, 1300), true);
    const slot_ok = pool.slotAt(a2.slot_index) ?? return error.MissingSlot;
    expect_eq(slot_ok.consecutive_errors, 0);

    // Two consecutive unhealthy releases -> dropped on 2nd!
    const a3 = pool.acquire(1400);
    expect_eq(pool.release(a3.slot_index, false, 1500), true);
    const a4 = pool.acquire(1600);
    expect_eq(pool.release(a4.slot_index, false, 1700), true);

    expect_eq(pool.stats().idle, 0);
    expect_eq(pool.stats().active, 0);
    expect_eq(pool.stats().total_unhealthy_drops, 1);
}

// ============================================================================
// Test 4: Max-Lifetime Retirement on Release
// ============================================================================

fn testMaxLifetimeRetireOnRelease()! {
    let slots: [2]DbConnSlot = [DbConnSlot{}; 2];
    const cfg = DbPoolConfig.init(2, 0, 60000, 10000, 1) ?? return error.InitFailed;
    let pool = DbConnPool.init(slots[0..], cfg) ?? return error.InitFailed;

    const a1 = pool.acquire(1000);
    // Released healthy at t = 12000 (age 11000ms >= max_lifetime 10000ms) -> retired instead of Idle!
    expect_eq(pool.release(a1.slot_index, true, 12000), true);
    expect_eq(pool.stats().idle, 0);
    expect_eq(pool.stats().active, 0);
    expect_eq(pool.stats().total_evicted, 1);
}

// ============================================================================
// Test 5: Batch evictExpired Respecting min_idle vs Hard max_lifetime
// ============================================================================

fn testBatchEvictExpiredAndMinIdle()! {
    let slots: [3]DbConnSlot = [DbConnSlot{}; 3];
    // max_conns=3, min_idle=1, idle_timeout=5000ms, max_lifetime=20000ms
    const cfg = DbPoolConfig.init(3, 1, 5000, 20000, 1) ?? return error.InitFailed;
    let pool = DbConnPool.init(slots[0..], cfg) ?? return error.InitFailed;

    const c1 = pool.acquire(1000);
    const c2 = pool.acquire(1000);
    const c3 = pool.acquire(1000);
    expect_eq(pool.release(c1.slot_index, true, 2000), true);
    expect_eq(pool.release(c2.slot_index, true, 2000), true);
    expect_eq(pool.release(c3.slot_index, true, 2000), true);
    expect_eq(pool.stats().idle, 3);

    // At t = 8000 (idle 6000ms >= 5000ms, age 7000ms < 20000ms):
    // Evicts 2 idle connections, preserving min_idle = 1!
    const ev1 = pool.evictExpired(8000);
    expect_eq(ev1, 2);
    expect_eq(pool.stats().idle, 1);

    // At t = 25000 (age 24000ms >= max_lifetime 20000ms):
    // Hard lifetime expiry evicts even the last min_idle connection!
    const ev2 = pool.evictExpired(25000);
    expect_eq(ev2, 1);
    expect_eq(pool.stats().idle, 0);
    expect_eq(pool.stats().total_evicted, 3);
}

// ============================================================================
// Test 6: Defensive Guards (Invalid Config, Undersized Slots, Double Release)
// ============================================================================

fn testDefensiveGuards()! {
    let slots: [2]DbConnSlot = [DbConnSlot{}; 2];

    // Invalid configs
    expect_eq(DbPoolConfig.init(0, 0, 1000, 1000, 1) == null, true);
    expect_eq(DbPoolConfig.init(2, 3, 1000, 1000, 1) == null, true);
    expect_eq(DbPoolConfig.init(2, 0, -1, 1000, 1) == null, true);
    expect_eq(DbPoolConfig.init(2, 0, 1000, -1, 1) == null, true);

    // Undersized slots slice
    const cfg4 = DbPoolConfig.init(4, 0, 1000, 1000, 1) ?? return error.InitFailed;
    expect_eq(DbConnPool.init(slots[0..], cfg4) == null, true);

    // Double release, out-of-bounds release, bindHandle on non-active slot
    const cfg2 = DbPoolConfig.init(2, 0, 1000, 10000, 1) ?? return error.InitFailed;
    let pool = DbConnPool.init(slots[0..], cfg2) ?? return error.InitFailed;

    expect_eq(pool.release(0, true, 100), false);
    expect_eq(pool.release(99, true, 100), false);
    expect_eq(pool.bindHandle(0, 777), false);
    expect_eq(pool.slotAt(99) == null, true);

    const a = pool.acquire(100);
    expect_eq(pool.release(a.slot_index, true, 200), true);
    expect_eq(pool.release(a.slot_index, true, 250), false); // double release rejected
}

fn main() {
    test("1. db.conn_pool: create, bindHandle, LIFO reuse & Exhausted backpressure", () => {
        testCreateReuseAndExhausted()?;
    });
    test("2. db.conn_pool: idle timeout & max lifetime auto-eviction on acquire", () => {
        testIdleAndLifetimeEvictionOnAcquire()?;
    });
    test("3. db.conn_pool: unhealthy release consecutive error threshold & reset", () => {
        testUnhealthyReleaseAndConsecutiveErrors()?;
    });
    test("4. db.conn_pool: max-lifetime retirement on release", () => {
        testMaxLifetimeRetireOnRelease()?;
    });
    test("5. db.conn_pool: batch evictExpired with min_idle vs hard max_lifetime", () => {
        testBatchEvictExpiredAndMinIdle()?;
    });
    test("6. db.conn_pool: defensive guards (config, slots, double release)", () => {
        testDefensiveGuards()?;
    });
}
