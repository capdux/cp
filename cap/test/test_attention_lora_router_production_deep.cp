// Origin: Pure-CP Stdlib Test Suite (lib/attention/lora_router.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Multi-LoRA Router & SGMV Batch Planner:
//   1. Adapter residency, cache hit/miss accounting, scale_f32 & refcount underflow guard
//   2. Slot-cap & memory-budget LRU cold adapter eviction (single & multi-eviction)
//   3. Pinned & in-flight (active_refs > 0) eviction immunity + atomic oom_blocked pre-flight
//   4. unpin_adapter unlocking & subsequent LRU reclamation
//   5. planLoraSgmvBatch stable multi-tenant grouping with isolated base-only (id == 0) group
//   6. applyLoraDeltaSimdF32 accuracy vs naive reference matmul (cross-SIMD-width 12->4->8)
//   7. Comprehensive boundary guards (invalid config, rank > max_rank, oversized adapter, bad dims)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    LORA_ROUTER_MAX_ADAPTERS,
    LORA_MAX_BATCH_GROUPS,
    LoraRouterConfig,
    LoraAdapterSlot,
    LoraAcquireOutcome,
    LoraBatchGroup,
    LoraRouterStats,
    MultiLoraRouter,
    planLoraSgmvBatch,
    applyLoraDeltaSimdF32,
} from "cap:attention";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: Residency, Cache Hit/Miss, Scale Calculation & Refcount Guard
// ============================================================================
test("lora_router: cache hit/miss, scale_f32 & refcount underflow guard", () => {
    const cfg = LoraRouterConfig{
        max_loaded_adapters: 4,
        memory_budget_bytes: 1024,
        max_rank: 32,
    };
    const router_opt = MultiLoraRouter.init(cfg);
    expect_eq(router_opt != null, true);
    let router = router_opt ?? return;

    // First acquire -> cache miss, active_refs = 1, scale = 32.0 / 16 = 2.0
    const o1 = router.acquire_adapter(101, 16, 32.0, 256, 10, false);
    expect_eq(o1.ok, true);
    expect_eq(o1.cache_hit, false);
    expect_eq(o1.evicted_count, 0);
    expect_eq(o1.oom_blocked, false);

    // Second acquire of same adapter -> cache hit, same slot, active_refs = 2
    const o2 = router.acquire_adapter(101, 16, 32.0, 256, 20, false);
    expect_eq(o2.ok, true);
    expect_eq(o2.cache_hit, true);
    expect_eq(o2.slot_idx, o1.slot_idx);

    const slot = router.get_slot(o1.slot_idx) ?? return;
    expect_eq(slot.active_refs, 2);
    expect_eq(slot.last_used_step, 20);
    expect_eq(approxEqF32(slot.scale_f32, 2.0, 0.0001), true);

    const st = router.stats();
    expect_eq(st.loaded_count, 1);
    expect_eq(st.memory_used_bytes, 256);
    expect_eq(st.cache_hits, 1);
    expect_eq(st.cache_misses, 1);

    // Release twice succeeds; third release blocked (underflow guard)
    expect_eq(router.release_adapter(101), true);
    expect_eq(router.release_adapter(101), true);
    expect_eq(router.release_adapter(101), false);
    expect_eq(router.release_adapter(999), false);
});

// ============================================================================
// Test 2: Slot-Cap & Memory-Budget LRU Eviction
// ============================================================================
test("lora_router: LRU eviction on slot exhaustion and multi-eviction on memory pressure", () => {
    const cfg = LoraRouterConfig{
        max_loaded_adapters: 3,
        memory_budget_bytes: 300,
        max_rank: 32,
    };
    let router = MultiLoraRouter.init(cfg) ?? return;

    // Load 3 adapters of 100 bytes each at steps 1, 2, 3, then release all
    expect_eq(router.acquire_adapter(1, 8, 16.0, 100, 1, false).ok, true);
    expect_eq(router.acquire_adapter(2, 8, 16.0, 100, 2, false).ok, true);
    expect_eq(router.acquire_adapter(3, 8, 16.0, 100, 3, false).ok, true);
    // Touch adapter 1 at step 5 so LRU order becomes: 2 (step 2), 3 (step 3), 1 (step 5)
    expect_eq(router.acquire_adapter(1, 8, 16.0, 100, 5, false).cache_hit, true);
    expect_eq(router.release_adapter(1), true);
    expect_eq(router.release_adapter(1), true);
    expect_eq(router.release_adapter(2), true);
    expect_eq(router.release_adapter(3), true);

    // Acquire adapter 4 needing 200 bytes -> must evict TWO coldest adapters (2 and 3)
    const o4 = router.acquire_adapter(4, 16, 32.0, 200, 6, false);
    expect_eq(o4.ok, true);
    expect_eq(o4.cache_hit, false);
    expect_eq(o4.evicted_count, 2);

    const st = router.stats();
    expect_eq(st.loaded_count, 2); // adapters 1 (100B) and 4 (200B) remain
    expect_eq(st.memory_used_bytes, 300);
    expect_eq(st.evictions, 2);

    // Adapter 1 is still a cache hit; adapter 2 was evicted
    expect_eq(router.acquire_adapter(1, 8, 16.0, 100, 7, false).cache_hit, true);
});

// ============================================================================
// Test 3: Pinned & In-Flight Immunity + Atomic oom_blocked Pre-Flight
// ============================================================================
test("lora_router: pinned & in-flight adapters are immune and oom_blocked is atomic", () => {
    const cfg = LoraRouterConfig{
        max_loaded_adapters: 2,
        memory_budget_bytes: 200,
        max_rank: 32,
    };
    let router = MultiLoraRouter.init(cfg) ?? return;

    // Adapter 10: pinned (100B), released to active_refs == 0
    expect_eq(router.acquire_adapter(10, 8, 8.0, 100, 1, true).ok, true);
    expect_eq(router.release_adapter(10), true);
    // Adapter 20: unpinned (100B), still in-flight (active_refs == 1)
    expect_eq(router.acquire_adapter(20, 8, 8.0, 100, 2, false).ok, true);

    // Adapter 30 (100B): neither 10 (pinned) nor 20 (in-flight) can be evicted -> oom_blocked!
    const o30 = router.acquire_adapter(30, 8, 8.0, 100, 3, false);
    expect_eq(o30.ok, false);
    expect_eq(o30.oom_blocked, true);
    expect_eq(o30.evicted_count, 0);
    expect_eq(router.stats().loaded_count, 2);

    // Release 20 so 100B becomes cold, but request 150B (> 100B freeable) -> atomic oom_blocked!
    expect_eq(router.release_adapter(20), true);
    const o_big = router.acquire_adapter(40, 8, 8.0, 150, 4, false);
    expect_eq(o_big.ok, false);
    expect_eq(o_big.oom_blocked, true);
    expect_eq(o_big.evicted_count, 0);
    // Atomic guarantee: adapter 20 was NOT needlessly evicted!
    expect_eq(router.stats().loaded_count, 2);
    expect_eq(router.acquire_adapter(20, 8, 8.0, 100, 5, false).cache_hit, true);
});

// ============================================================================
// Test 4: unpin_adapter Unlocks Eviction Eligibility
// ============================================================================
test("lora_router: unpin_adapter allows subsequent LRU reclamation", () => {
    const cfg = LoraRouterConfig{
        max_loaded_adapters: 1,
        memory_budget_bytes: 128,
        max_rank: 16,
    };
    let router = MultiLoraRouter.init(cfg) ?? return;

    expect_eq(router.acquire_adapter(10, 8, 16.0, 64, 1, true).ok, true);
    expect_eq(router.release_adapter(10), true);
    expect_eq(router.stats().pinned_count, 1);

    // Blocked while pinned
    expect_eq(router.acquire_adapter(20, 8, 16.0, 64, 2, false).oom_blocked, true);

    // Unpin adapter 10
    expect_eq(router.unpin_adapter(10), true);
    expect_eq(router.unpin_adapter(10), false); // already unpinned
    expect_eq(router.stats().pinned_count, 0);

    // Now adapter 20 evicts 10 and loads cleanly
    const o20 = router.acquire_adapter(20, 8, 16.0, 64, 3, false);
    expect_eq(o20.ok, true);
    expect_eq(o20.evicted_count, 1);
});

// ============================================================================
// Test 5: planLoraSgmvBatch Stable Multi-Tenant Grouping
// ============================================================================
test("lora_router: planLoraSgmvBatch stably groups multi-tenant & base-only requests", () => {
    // Mixed batch: adapter 101 at [0, 3, 6], base-only 0 at [1, 4], adapter 202 at [2, 5]
    const reqs: [7]u64 = [101, 0, 202, 101, 0, 202, 101];
    let groups: [4]LoraBatchGroup = [LoraBatchGroup{}; 4];
    let sorted: [7]usize = [0; 7];
    let groups_s: []LoraBatchGroup = groups[0..];
    let sorted_s: []usize = sorted[0..];

    const ng = planLoraSgmvBatch(reqs[0..], groups_s, sorted_s);
    expect_eq(ng, 3);

    // Group 0: adapter 101 -> offset 0, count 3 -> indices [0, 3, 6]
    expect_eq(groups[0].adapter_id, 101);
    expect_eq(groups[0].start_offset, 0);
    expect_eq(groups[0].req_count, 3);
    expect_eq(sorted[0], 0);
    expect_eq(sorted[1], 3);
    expect_eq(sorted[2], 6);

    // Group 1: base-only 0 -> offset 3, count 2 -> indices [1, 4]
    expect_eq(groups[1].adapter_id, 0);
    expect_eq(groups[1].start_offset, 3);
    expect_eq(groups[1].req_count, 2);
    expect_eq(sorted[3], 1);
    expect_eq(sorted[4], 4);

    // Group 2: adapter 202 -> offset 5, count 2 -> indices [2, 5]
    expect_eq(groups[2].adapter_id, 202);
    expect_eq(groups[2].start_offset, 5);
    expect_eq(groups[2].req_count, 2);
    expect_eq(sorted[5], 2);
    expect_eq(sorted[6], 5);

    // Insufficient group buffer / sorted buffer / empty input return 0
    let tiny_g: [2]LoraBatchGroup = [LoraBatchGroup{}; 2];
    let tiny_g_s: []LoraBatchGroup = tiny_g[0..];
    expect_eq(planLoraSgmvBatch(reqs[0..], tiny_g_s, sorted_s), 0);
    expect_eq(planLoraSgmvBatch(reqs[0..0], groups_s, sorted_s), 0);
    expect_eq(planLoraSgmvBatch(reqs[0..], groups_s, sorted[0..3]), 0);
});

// ============================================================================
// Test 6: applyLoraDeltaSimdF32 Accuracy vs Naive Reference Matmul
// ============================================================================
test("lora_router: applyLoraDeltaSimdF32 matches naive matrix multiply across SIMD tail", () => {
    // in_dim = 12 (8 SIMD lanes + 4 tail), rank = 4, out_dim = 8
    const in_dim: usize = 12;
    const rank: usize = 4;
    const out_dim: usize = 8;
    const scale: f32 = 1.5;

    let x: [12]f32 = [0.0; 12];
    let a_mat: [48]f32 = [0.0; 48];
    let b_mat: [32]f32 = [0.0; 32];
    let y_simd: [8]f32 = [0.0; 8];
    let y_ref: [8]f32 = [0.0; 8];

    let i: usize = 0;
    while (i < in_dim) : (i += 1) {
        x[i] = ((i as f32) - 5.0) * 0.25;
    }
    let r: usize = 0;
    while (r < rank) : (r += 1) {
        let j: usize = 0;
        while (j < in_dim) : (j += 1) {
            a_mat[r * in_dim + j] = (((r * 7 + j * 3) % 11) as f32 - 5.0) * 0.1;
        }
    }
    let o: usize = 0;
    while (o < out_dim) : (o += 1) {
        y_simd[o] = (o as f32) * 0.5 - 1.0;
        y_ref[o] = y_simd[o];
        let rr: usize = 0;
        while (rr < rank) : (rr += 1) {
            b_mat[o * rank + rr] = (((o * 5 + rr * 2) % 9) as f32 - 4.0) * 0.2;
        }
    }

    // Naive reference: y_ref += scale * B * (A * x)
    let mid_ref: [4]f32 = [0.0; 4];
    r = 0;
    while (r < rank) : (r += 1) {
        let acc: f32 = 0.0;
        let j: usize = 0;
        while (j < in_dim) : (j += 1) {
            acc += a_mat[r * in_dim + j] * x[j];
        }
        mid_ref[r] = acc;
    }
    o = 0;
    while (o < out_dim) : (o += 1) {
        let acc: f32 = 0.0;
        let rr: usize = 0;
        while (rr < rank) : (rr += 1) {
            acc += b_mat[o * rank + rr] * mid_ref[rr];
        }
        y_ref[o] += scale * acc;
    }

    let scratch: [4]f32 = [0.0; 4];
    let scratch_s: []f32 = scratch[0..];
    let y_s: []f32 = y_simd[0..];
    expect_eq(applyLoraDeltaSimdF32(x[0..], a_mat[0..], b_mat[0..], in_dim, rank, out_dim, scale, scratch_s, y_s), true);

    o = 0;
    while (o < out_dim) : (o += 1) {
        expect_eq(approxEqF32(y_simd[o], y_ref[o], 0.0001), true);
    }
});

// ============================================================================
// Test 7: Boundary Guards (Invalid Config, Bad Rank, Oversized Adapter, Bad Dims)
// ============================================================================
test("lora_router: boundary guards for invalid config, rank & SIMD dims", () => {
    expect_eq(LORA_ROUTER_MAX_ADAPTERS, 32);
    expect_eq(LORA_MAX_BATCH_GROUPS, 32);

    // Invalid configs
    expect_eq(MultiLoraRouter.init(LoraRouterConfig{ max_loaded_adapters: 0, memory_budget_bytes: 100, max_rank: 8 }) == null, true);
    expect_eq(MultiLoraRouter.init(LoraRouterConfig{ max_loaded_adapters: 33, memory_budget_bytes: 100, max_rank: 8 }) == null, true);
    expect_eq(MultiLoraRouter.init(LoraRouterConfig{ max_loaded_adapters: 4, memory_budget_bytes: 0, max_rank: 8 }) == null, true);
    expect_eq(MultiLoraRouter.init(LoraRouterConfig{ max_loaded_adapters: 4, memory_budget_bytes: 100, max_rank: 0 }) == null, true);

    let router = MultiLoraRouter.init(LoraRouterConfig{
        max_loaded_adapters: 4,
        memory_budget_bytes: 256,
        max_rank: 16,
    }) ?? return;

    const nan_f32: f32 = std.math.nan(f32);
    expect_eq(router.acquire_adapter(0, 8, 16.0, 64, 1, false).ok, false);      // adapter_id == 0
    expect_eq(router.acquire_adapter(1, 0, 16.0, 64, 1, false).ok, false);      // rank == 0
    expect_eq(router.acquire_adapter(1, 32, 16.0, 64, 1, false).ok, false);     // rank > max_rank
    expect_eq(router.acquire_adapter(1, 8, 0.0, 64, 1, false).ok, false);       // alpha <= 0
    expect_eq(router.acquire_adapter(1, 8, nan_f32, 64, 1, false).ok, false);   // alpha NaN
    expect_eq(router.acquire_adapter(1, 8, 16.0, 0, 1, false).ok, false);       // mem_bytes == 0
    const o_huge = router.acquire_adapter(1, 8, 16.0, 512, 1, false);           // exceeds budget
    expect_eq(o_huge.ok, false);
    expect_eq(o_huge.oom_blocked, true);

    // applyLoraDeltaSimdF32 dimension & NaN guards
    const x: [4]f32 = [1.0; 4];
    const a: [8]f32 = [1.0; 8];
    const b: [6]f32 = [1.0; 6];
    let scr: [2]f32 = [0.0; 2];
    let y: [3]f32 = [0.0; 3];
    let scr_s: []f32 = scr[0..];
    let y_s: []f32 = y[0..];

    expect_eq(applyLoraDeltaSimdF32(x[0..], a[0..], b[0..], 0, 2, 3, 1.0, scr_s, y_s), false);
    expect_eq(applyLoraDeltaSimdF32(x[0..], a[0..], b[0..], 4, 2, 3, nan_f32, scr_s, y_s), false);
    expect_eq(applyLoraDeltaSimdF32(x[0..2], a[0..], b[0..], 4, 2, 3, 1.0, scr_s, y_s), false);
    expect_eq(applyLoraDeltaSimdF32(x[0..], a[0..], b[0..], 4, 2, 3, 1.0, scr[0..1], y_s), false);
});
