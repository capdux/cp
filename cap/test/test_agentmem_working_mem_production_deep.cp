// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/working_mem.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation AI Agent Working Memory & Token-Budget
// Context Window Manager: within-budget lifecycle, pinned prompt protection, FIFO/LRU/LowestScore
// eviction policy differentiation, atomic oldest-slot summary compaction, and defensive guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MemSlotRole,
    EvictionPolicy,
    WorkingMemItem,
    WorkingMemStats,
    WorkingMemory,
} from "cap:agentmem";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff: f32 = if (a >= b) a - b else b - a;
    return diff <= eps;
}

fn testWithinBudgetLifecycleAndStats()! {
    let pool: [6]WorkingMemItem = undefined;
    let pool_slice: []WorkingMemItem = pool[0..];

    let wm = cap.agentmem.WorkingMemory.init(pool_slice, 1000, .Lru);

    const sys_id = wm.push(.System, 150, 1.0, 1, true, 0xAAAA) ?? 0;
    const usr_id = wm.push(.User, 200, 0.7, 2, false, 0xBBBB) ?? 0;
    const ast_id = wm.push(.Assistant, 250, 0.8, 3, false, 0xCCCC) ?? 0;
    const tool_id = wm.push(.Tool, 100, 0.4, 4, false, 0xDDDD) ?? 0;

    expect_eq(sys_id > 0, true);
    expect_eq(usr_id > sys_id, true);
    expect_eq(ast_id > usr_id, true);
    expect_eq(tool_id > ast_id, true);

    expect_eq(wm.contains(sys_id), true);
    expect_eq(wm.contains(tool_id), true);
    expect_eq(wm.contains(999), false);

    // Touch updates last_accessed_turn without mutating created_turn.
    expect_eq(wm.touch(usr_id, 10), true);
    expect_eq(wm.touch(999, 10), false);
    if (u_item := wm.get(usr_id)) {
        expect_eq(u_item.created_turn, 2);
        expect_eq(u_item.last_accessed_turn, 10);
        expect_eq(u_item.tokens, 200);
        expect_eq(u_item.content_hash, 0xBBBB);
        expect_eq(u_item.role == .User, true);
    } else {
        expect_eq(true, false);
    }

    // Pin / unpin / remove lifecycle.
    expect_eq(wm.pin(usr_id), true);
    expect_eq(wm.pin(999), false);
    let st = wm.stats();
    expect_eq(st.active_items, 4);
    expect_eq(st.pinned_items, 2);
    expect_eq(st.used_tokens, 700);
    expect_eq(st.budget_tokens, 1000);
    expect_eq(st.evicted_total, 0);
    expect_eq(st.compacted_total, 0);
    expect_eq(approxEqF32(st.utilization_pct, 70.0, 0.01), true);

    expect_eq(wm.unpin(usr_id), true);
    expect_eq(wm.remove(tool_id), true);
    expect_eq(wm.remove(tool_id), false);
    expect_eq(wm.contains(tool_id), false);

    st = wm.stats();
    expect_eq(st.active_items, 3);
    expect_eq(st.pinned_items, 1);
    expect_eq(st.used_tokens, 600);
    expect_eq(approxEqF32(st.utilization_pct, 60.0, 0.01), true);
}

fn testPinnedProtectionUnderEvictionPressure()! {
    let pool: [4]WorkingMemItem = undefined;
    let pool_slice: []WorkingMemItem = pool[0..];

    let wm = WorkingMemory.init(pool_slice, 500, .FifoOldest);

    const sys_id = wm.push(.System, 200, 1.0, 1, true, 0x1001) ?? 0;
    const msg1 = wm.push(.User, 150, 0.5, 2, false, 0x1002) ?? 0;
    const msg2 = wm.push(.Assistant, 150, 0.5, 3, false, 0x1003) ?? 0;
    expect_eq(wm.stats().used_tokens, 500);

    // Push 200 tokens: must evict msg1 and msg2 (total 300 freed), keeping pinned sys_id intact.
    const msg3 = wm.push(.User, 200, 0.6, 4, false, 0x1004) ?? 0;
    expect_eq(msg3 > 0, true);
    expect_eq(wm.contains(sys_id), true);
    expect_eq(wm.contains(msg1), false);
    expect_eq(wm.contains(msg2), false);
    expect_eq(wm.contains(msg3), true);
    expect_eq(wm.stats().evicted_total, 2);
    expect_eq(wm.stats().used_tokens, 400);

    // Pin msg3 too: now pinned_tokens = 400. Pushing 150 tokens exceeds budget even if non-pinned
    // were evicted -> must atomically reject without disturbing any slot.
    expect_eq(wm.pin(msg3), true);
    expect_eq(wm.push(.Tool, 150, 0.9, 5, false, 0x1005) == null, true);
    expect_eq(wm.contains(sys_id), true);
    expect_eq(wm.contains(msg3), true);
    expect_eq(wm.stats().used_tokens, 400);
    expect_eq(wm.stats().evicted_total, 2);

    // Unpin msg3: now pushing 250 tokens succeeds by evicting msg3 while still keeping sys_id.
    expect_eq(wm.unpin(msg3), true);
    const msg4 = wm.push(.Tool, 250, 0.9, 6, false, 0x1006) ?? 0;
    expect_eq(msg4 > 0, true);
    expect_eq(wm.contains(sys_id), true);
    expect_eq(wm.contains(msg3), false);
    expect_eq(wm.contains(msg4), true);
    expect_eq(wm.stats().used_tokens, 450);
    expect_eq(wm.stats().evicted_total, 3);
}

fn testEvictionPolicyDifferentiationFifoLruLowestScore()! {
    // 1. FIFO_OLDEST: ignores touch() and importance, evicts earliest created_turn (a).
    let pool_fifo: [6]WorkingMemItem = undefined;
    let slice_fifo: []WorkingMemItem = pool_fifo[0..];
    let wm_fifo = WorkingMemory.init(slice_fifo, 300, .FifoOldest);
    const fa = wm_fifo.push(.User, 100, 0.95, 1, false, 0xA1) ?? 0;
    const fb = wm_fifo.push(.User, 100, 0.50, 2, false, 0xA2) ?? 0;
    const fc = wm_fifo.push(.User, 100, 0.05, 3, false, 0xA3) ?? 0;
    * = wm_fifo.touch(fa, 5); // refresh fa, FIFO should ignore this
    const fd = wm_fifo.push(.Assistant, 100, 0.60, 6, false, 0xA4) ?? 0;
    expect_eq(fd > 0, true);
    expect_eq(wm_fifo.contains(fa), false);
    expect_eq(wm_fifo.contains(fb), true);
    expect_eq(wm_fifo.contains(fc), true);
    expect_eq(wm_fifo.contains(fd), true);

    // 2. LRU: respects touch(la, 5), so lb (last_accessed_turn=2) is evicted instead of la.
    let pool_lru: [6]WorkingMemItem = undefined;
    let slice_lru: []WorkingMemItem = pool_lru[0..];
    let wm_lru = WorkingMemory.init(slice_lru, 300, .Lru);
    const la = wm_lru.push(.User, 100, 0.95, 1, false, 0xB1) ?? 0;
    const lb = wm_lru.push(.User, 100, 0.50, 2, false, 0xB2) ?? 0;
    const lc = wm_lru.push(.User, 100, 0.05, 3, false, 0xB3) ?? 0;
    * = wm_lru.touch(la, 5);
    const ld = wm_lru.push(.Assistant, 100, 0.60, 6, false, 0xB4) ?? 0;
    expect_eq(ld > 0, true);
    expect_eq(wm_lru.contains(la), true);
    expect_eq(wm_lru.contains(lb), false);
    expect_eq(wm_lru.contains(lc), true);
    expect_eq(wm_lru.contains(ld), true);

    // 3. LOWEST_SCORE: lc has importance 0.05 (score ~0.274 at turn 6), lower than
    //    lb (importance 0.50, score ~0.544) and la (importance 0.95, score ~0.922),
    //    so lc is evicted even though it was created most recently.
    let pool_sc: [6]WorkingMemItem = undefined;
    let slice_sc: []WorkingMemItem = pool_sc[0..];
    let wm_sc = WorkingMemory.init(slice_sc, 300, .LowestScore);
    const sa = wm_sc.push(.User, 100, 0.95, 1, false, 0xC1) ?? 0;
    const sb = wm_sc.push(.User, 100, 0.50, 2, false, 0xC2) ?? 0;
    const sc = wm_sc.push(.User, 100, 0.05, 3, false, 0xC3) ?? 0;
    * = wm_sc.touch(sa, 5);
    const sd = wm_sc.push(.Assistant, 100, 0.60, 6, false, 0xC4) ?? 0;
    expect_eq(sd > 0, true);
    expect_eq(wm_sc.contains(sa), true);
    expect_eq(wm_sc.contains(sb), true);
    expect_eq(wm_sc.contains(sc), false);
    expect_eq(wm_sc.contains(sd), true);
}

fn testAtomicCompactionIntoSummary()! {
    let pool: [6]WorkingMemItem = undefined;
    let pool_slice: []WorkingMemItem = pool[0..];

    let wm = WorkingMemory.init(pool_slice, 1000, .Lru);

    const sys_id = wm.push(.System, 200, 1.0, 1, true, 0x0001) ?? 0;
    const m1 = wm.push(.User, 200, 0.4, 2, false, 0x0002) ?? 0;
    const m2 = wm.push(.Assistant, 250, 0.85, 3, false, 0x0003) ?? 0;
    const m3 = wm.push(.Tool, 150, 0.5, 4, false, 0x0004) ?? 0;
    expect_eq(wm.stats().used_tokens, 800);

    // Invalid count == 0 -> null, zero side effects.
    expect_eq(wm.compactOldestIntoSummary(0, 50, 5, 0x9000) == null, true);

    // Summary too large (800 - 450 + 700 = 1050 > 1000) -> atomic rejection, m1/m2 untouched.
    expect_eq(wm.compactOldestIntoSummary(2, 700, 5, 0x9001) == null, true);
    expect_eq(wm.contains(m1), true);
    expect_eq(wm.contains(m2), true);
    expect_eq(wm.stats().used_tokens, 800);
    expect_eq(wm.stats().compacted_total, 0);

    // Valid compaction of oldest 2 non-pinned entries (m1=200, m2=250 -> 120 summary tokens).
    // Pinned sys_id (turn 1) must be skipped and remain untouched.
    const sum_id = wm.compactOldestIntoSummary(2, 120, 5, 0x9002) ?? 0;
    expect_eq(sum_id > 0, true);
    expect_eq(wm.contains(sys_id), true);
    expect_eq(wm.contains(m1), false);
    expect_eq(wm.contains(m2), false);
    expect_eq(wm.contains(m3), true);
    expect_eq(wm.contains(sum_id), true);

    if (sum_item := wm.get(sum_id)) {
        expect_eq(sum_item.role == .Summary, true);
        expect_eq(sum_item.tokens, 120);
        expect_eq(sum_item.created_turn, 5);
        expect_eq(sum_item.content_hash, 0x9002);
        expect_eq(sum_item.pinned, false);
        expect_eq(approxEqF32(sum_item.importance, 0.85, 0.001), true);
    } else {
        expect_eq(true, false);
    }

    const st = wm.stats();
    expect_eq(st.active_items, 3);
    expect_eq(st.pinned_items, 1);
    expect_eq(st.used_tokens, 470); // 200 (sys) + 150 (m3) + 120 (summary)
    expect_eq(st.compacted_total, 2);
    expect_eq(st.evicted_total, 0);
}

fn testOversizedRejectionAndDefensiveGuards()! {
    let pool: [4]WorkingMemItem = undefined;
    let pool_slice: []WorkingMemItem = pool[0..];

    let wm = WorkingMemory.init(pool_slice, 500, .Lru);
    const m1 = wm.push(.User, 200, 0.6, 1, false, 0x5555) ?? 0;
    expect_eq(m1 > 0, true);

    // Single item larger than total token_budget -> atomic rejection, m1 preserved.
    expect_eq(wm.push(.User, 501, 0.9, 2, false, 0x5556) == null, true);
    expect_eq(wm.contains(m1), true);
    expect_eq(wm.stats().used_tokens, 200);
    expect_eq(wm.stats().evicted_total, 0);

    // Invalid importance (NaN, Inf, <0, >1) -> rejected.
    expect_eq(wm.push(.User, 50, std.math.nan(f32), 2, false, 0x5557) == null, true);
    expect_eq(wm.push(.User, 50, std.math.inf(f32), 2, false, 0x5558) == null, true);
    expect_eq(wm.push(.User, 50, -0.1, 2, false, 0x5559) == null, true);
    expect_eq(wm.push(.User, 50, 1.01, 2, false, 0x555A) == null, true);
    expect_eq(wm.stats().active_items, 1);

    // Empty slot pool -> all pushes and compactions fail safely.
    let empty_buf: [0]WorkingMemItem = undefined;
    let empty_slice: []WorkingMemItem = empty_buf[0..];
    let wm_empty = WorkingMemory.init(empty_slice, 500, .Lru);
    expect_eq(wm_empty.push(.User, 10, 0.5, 1, false, 0x1) == null, true);
    expect_eq(wm_empty.compactOldestIntoSummary(1, 5, 2, 0x2) == null, true);
    expect_eq(wm_empty.stats().active_items, 0);

    // Zero token budget -> utilization_pct == 0.0 and non-zero items rejected.
    let zbuf: [2]WorkingMemItem = undefined;
    let zslice: []WorkingMemItem = zbuf[0..];
    let wm_zero = WorkingMemory.init(zslice, 0, .FifoOldest);
    expect_eq(wm_zero.push(.User, 1, 0.5, 1, false, 0x1) == null, true);
    expect_eq(wm_zero.stats().utilization_pct, 0.0);
}

fn testSlotCapacityEvictionAndStatsAccounting()! {
    // Tight slot capacity (3 slots) with ample token budget (10000):
    // verifies slot exhaustion still triggers policy-based eviction and accurate stats.
    let pool: [3]WorkingMemItem = undefined;
    let pool_slice: []WorkingMemItem = pool[0..];

    let wm = WorkingMemory.init(pool_slice, 10000, .FifoOldest);
    const sys_id = wm.push(.System, 100, 1.0, 1, true, 0x7000) ?? 0;

    let turn: u32 = 2;
    while (turn <= 7) : (turn += 1) {
        const id = wm.push(.User, 100, 0.5, turn, false, 0x7000 + (turn as u64)) ?? 0;
        expect_eq(id > 0, true);
    }

    const st = wm.stats();
    expect_eq(wm.contains(sys_id), true);
    expect_eq(st.active_items, 3);
    expect_eq(st.pinned_items, 1);
    expect_eq(st.used_tokens, 300);
    expect_eq(st.evicted_total, 4); // 6 user pushes into 2 non-pinned slots -> 4 evictions
    expect_eq(approxEqF32(st.utilization_pct, 3.0, 0.01), true);
}

fn main() {
    test("1. agentmem/working_mem: within-budget lifecycle, touch, pin, remove & stats", () => {
        testWithinBudgetLifecycleAndStats()?;
    });

    test("2. agentmem/working_mem: pinned prompt protection under eviction pressure", () => {
        testPinnedProtectionUnderEvictionPressure()?;
    });

    test("3. agentmem/working_mem: FIFO vs LRU vs LowestScore eviction differentiation", () => {
        testEvictionPolicyDifferentiationFifoLruLowestScore()?;
    });

    test("4. agentmem/working_mem: atomic oldest-slot compaction into Summary", () => {
        testAtomicCompactionIntoSummary()?;
    });

    test("5. agentmem/working_mem: oversized rejection, NaN/Inf & empty/zero-budget guards", () => {
        testOversizedRejectionAndDefensiveGuards()?;
    });

    test("6. agentmem/working_mem: slot-capacity eviction & telemetry accounting", () => {
        testSlotCapacityEvictionAndStatsAccounting()?;
    });
}
