// Origin: Pure-CP Stdlib Test Suite (lib/sys/gen_slab.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc ABA-Safe Generational Slab Handle Pool:
//   1. O(1) alloc/get/set/is_alive lifecycle & GenHandle toU64/fromU64/eql round-trip
//   2. LIFO slot reuse after free & strict ABA old-generation handle interception
//   3. Double-free interception & exact stale_handle_rejects accounting
//   4. clear() bulk invalidation of all historical handles & full-capacity reuse
//   5. high_water_mark tracking & full-capacity null rejection
//   6. Invalid capacity (0, >256) & forged out-of-bounds / generation-0 handle guards
//   7. Multi-round alloc-free churn conservation stress test

import { test, expect_eq } from "cap:test";
import {
    GEN_SLAB_MAX_SLOTS,
    GEN_SLAB_NO_FREE,
    GenHandle,
    GenSlabStats,
    GenSlabPool,
} from "cap:sys";

// ============================================================================
// Test 1: O(1) Alloc/Get/Set & GenHandle toU64/fromU64/eql Round-Trip
// ============================================================================
test("gen_slab: alloc, get, set & GenHandle 64-bit encoding round-trip", () => {
    expect_eq(GEN_SLAB_MAX_SLOTS, 256);
    expect_eq(GEN_SLAB_NO_FREE, 0xFFFF);

    let pool = GenSlabPool.init(16) ?? return;
    const h0: GenHandle = pool.alloc(1001) ?? return;
    const h1: GenHandle = pool.alloc(2002) ?? return;

    expect_eq(pool.is_alive(h0), true);
    expect_eq(pool.is_alive(h1), true);
    expect_eq(h0.eql(h1), false);
    expect_eq(pool.get(h0) ?? 0, 1001);
    expect_eq(pool.get(h1) ?? 0, 2002);

    // Lossless 64-bit compact handle round-trip
    const raw0: u64 = h0.toU64();
    const dec0: GenHandle = GenHandle.fromU64(raw0);
    expect_eq(dec0.eql(h0), true);
    expect_eq(pool.set(dec0, 9999), true);
    expect_eq(pool.get(h0) ?? 0, 9999);

    // Extreme u16 index & u32 generation round-trip
    const extreme = GenHandle{ index: 0xABCD, generation: 0xDEAD_BEEF };
    expect_eq(GenHandle.fromU64(extreme.toU64()).eql(extreme), true);

    const st: GenSlabStats = pool.stats();
    expect_eq(st.capacity, 16);
    expect_eq(st.live_count, 2);
    expect_eq(st.high_water_mark, 2);
    expect_eq(st.total_allocs, 2);
    expect_eq(st.total_frees, 0);
});

// ============================================================================
// Test 2: Slot Reuse & Strict ABA Old-Generation Interception
// ============================================================================
test("gen_slab: LIFO slot reuse after free & strict ABA interception", () => {
    let pool = GenSlabPool.init(8) ?? return;

    const h_a = pool.alloc(0xAA01) ?? return;
    const h_b = pool.alloc(0xBB02) ?? return;

    // Free h_a and verify returned payload
    const freed_val = pool.free(h_a) ?? 0;
    expect_eq(freed_val, 0xAA01);
    expect_eq(pool.is_alive(h_a), false);
    expect_eq(pool.get(h_a) == null, true);

    // Next alloc reuses h_a's exact slot index with generation incremented!
    const h_a2 = pool.alloc(0xAA99) ?? return;
    expect_eq(h_a2.index, h_a.index);
    expect_eq(h_a2.generation, h_a.generation + 1);
    expect_eq(h_a2.eql(h_a), false);

    // Old handle h_a must be strictly rejected without touching h_a2's payload
    expect_eq(pool.is_alive(h_a), false);
    expect_eq(pool.get(h_a) == null, true);
    expect_eq(pool.set(h_a, 0xBAD0), false);
    expect_eq(pool.free(h_a) == null, true);

    // New handle h_a2 and sibling h_b remain completely intact
    expect_eq(pool.get(h_a2) ?? 0, 0xAA99);
    expect_eq(pool.get(h_b) ?? 0, 0xBB02);
    expect_eq(pool.stats().live_count, 2);
    expect_eq(pool.stats().stale_handle_rejects, 2); // set + free on stale h_a
});

// ============================================================================
// Test 3: Double-Free Guard & Exact stale_handle_rejects Accounting
// ============================================================================
test("gen_slab: double-free guard & stale_handle_rejects accounting", () => {
    let pool = GenSlabPool.init(4) ?? return;

    const h = pool.alloc(4242) ?? return;
    expect_eq(pool.free(h) ?? 0, 4242);

    // Double-free & triple-free rejected cleanly
    expect_eq(pool.free(h) == null, true);
    expect_eq(pool.free(h) == null, true);
    expect_eq(pool.set(h, 777), false);

    const st = pool.stats();
    expect_eq(st.live_count, 0);
    expect_eq(st.total_allocs, 1);
    expect_eq(st.total_frees, 1);
    expect_eq(st.stale_handle_rejects, 3);
});

// ============================================================================
// Test 4: clear() Bulk Invalidation & Full-Capacity Reuse
// ============================================================================
test("gen_slab: clear invalidates all historical handles and restores full capacity", () => {
    let pool = GenSlabPool.init(6) ?? return;

    let old_handles: [6]GenHandle = [GenHandle{}; 6];
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        old_handles[i] = pool.alloc(@intCast(100 + i)) ?? return;
    }
    expect_eq(pool.stats().live_count, 6);
    expect_eq(pool.alloc(999) == null, true); // full

    // Clear invalidates all 6 live slots
    pool.clear();
    expect_eq(pool.stats().live_count, 0);
    expect_eq(pool.stats().total_frees, 6);

    let j: usize = 0;
    while (j < 6) : (j += 1) {
        expect_eq(pool.is_alive(old_handles[j]), false);
        expect_eq(pool.get(old_handles[j]) == null, true);
    }

    // Re-allocate all 6 slots -> every handle gets a strictly newer generation
    let k: usize = 0;
    while (k < 6) : (k += 1) {
        const fresh = pool.alloc(@intCast(500 + k)) ?? return;
        expect_eq(fresh.generation > old_handles[fresh.index as usize].generation, true);
        expect_eq(pool.get(fresh) ?? 0, @intCast(500 + k));
    }
    expect_eq(pool.stats().live_count, 6);
});

// ============================================================================
// Test 5: high_water_mark Tracking & Full-Capacity Null Rejection
// ============================================================================
test("gen_slab: high_water_mark tracking & full-capacity null rejection", () => {
    let pool = GenSlabPool.init(4) ?? return;

    const h0 = pool.alloc(10) ?? return;
    const h1 = pool.alloc(20) ?? return;
    const h2 = pool.alloc(30) ?? return;
    const h3 = pool.alloc(40) ?? return;

    // 5th alloc exceeds capacity -> null
    expect_eq(pool.alloc(50) == null, true);
    expect_eq(pool.stats().live_count, 4);
    expect_eq(pool.stats().high_water_mark, 4);

    // Free 2 slots -> live_count drops to 2, high_water_mark stays 4
    expect_eq(pool.free(h1) ?? 0, 20);
    expect_eq(pool.free(h3) ?? 0, 40);
    expect_eq(pool.stats().live_count, 2);
    expect_eq(pool.stats().high_water_mark, 4);

    // Re-allocate 2 slots -> succeeds, 3rd returns null
    const n1 = pool.alloc(200) ?? return;
    const n2 = pool.alloc(400) ?? return;
    expect_eq(pool.alloc(500) == null, true);
    expect_eq(pool.get(h0) ?? 0, 10);
    expect_eq(pool.get(h2) ?? 0, 30);
    expect_eq(pool.get(n1) ?? 0, 200);
    expect_eq(pool.get(n2) ?? 0, 400);
});

// ============================================================================
// Test 6: Invalid Capacity & Forged Handle Boundary Guards
// ============================================================================
test("gen_slab: invalid capacity & forged/out-of-bounds handle guards", () => {
    expect_eq(GenSlabPool.init(0) == null, true);
    expect_eq(GenSlabPool.init(GEN_SLAB_MAX_SLOTS + 1) == null, true);
    expect_eq(GenSlabPool.init(10_000) == null, true);

    let pool = GenSlabPool.init(4) ?? return;
    const live = pool.alloc(777) ?? return;

    // Forged out-of-bounds index
    const forged_oob = GenHandle{ index: 4, generation: 1 };
    expect_eq(pool.is_alive(forged_oob), false);
    expect_eq(pool.get(forged_oob) == null, true);
    expect_eq(pool.set(forged_oob, 1), false);
    expect_eq(pool.free(forged_oob) == null, true);

    // Forged generation 0
    const forged_gen0 = GenHandle{ index: 0, generation: 0 };
    expect_eq(pool.is_alive(forged_gen0), false);
    expect_eq(pool.get(forged_gen0) == null, true);
    expect_eq(pool.free(forged_gen0) == null, true);

    // Wrong future generation
    const forged_future = GenHandle{ index: 0, generation: 99 };
    expect_eq(pool.is_alive(forged_future), false);
    expect_eq(pool.get(forged_future) == null, true);

    // Real handle unaffected
    expect_eq(pool.get(live) ?? 0, 777);
    expect_eq(pool.stats().stale_handle_rejects, 3);
});

// ============================================================================
// Test 7: Multi-Round Alloc/Free Churn Conservation Stress Test
// ============================================================================
test("gen_slab: multi-round churn conservation & generation progression", () => {
    let pool = GenSlabPool.init(16) ?? return;

    let handles: [16]GenHandle = [GenHandle{}; 16];
    let active: [16]bool = [false; 16];

    let round: usize = 0;
    while (round < 40) : (round += 1) {
        // Fill all inactive positions
        let i: usize = 0;
        while (i < 16) : (i += 1) {
            if (!active[i]) {
                const payload: u64 = ((round as u64) * 1000) + (i as u64);
                handles[i] = pool.alloc(payload) ?? return;
                active[i] = true;
            }
        }
        expect_eq(pool.stats().live_count, 16);

        // Free half of the slots each round (alternating even/odd)
        let j: usize = 0;
        while (j < 16) : (j += 1) {
            if ((j + round) % 2 == 0) {
                const old_h = handles[j];
                const expected: u64 = pool.get(old_h) ?? 0;
                const got: u64 = pool.free(old_h) ?? 0;
                expect_eq(got, expected);
                expect_eq(pool.is_alive(old_h), false);
                active[j] = false;
            }
        }
        expect_eq(pool.stats().live_count, 8);
    }

    const st = pool.stats();
    expect_eq(st.total_allocs - st.total_frees, st.live_count as u64);
    expect_eq(st.high_water_mark, 16);
    expect_eq(st.stale_handle_rejects, 0);
});
