// Origin: Pure-CP Stdlib Test Suite (lib/attention/kv_eviction.cp) | Created: 2026-11-22
// Production Deep Verification Suite for StreamingLLM Attention Sink + H2O KV-Cache Eviction:
//   1. StreamingLLM Attention Sink permanent pinning & sliding window FIFO eviction
//   2. H2O Heavy-Hitter Oracle: high-score retention & lowest-score / LRU tie-break eviction
//   3. Hybrid three-zone (Sink + Heavy + Recent): recent-to-heavy promotion & strict quota conservation
//   4. Exponential decay (@fma) score updates & hot-token migration over decode steps
//   5. Ascending RoPE active position collection (collect_active_positions) & lookup_by_token_pos
//   6. Long-sequence compression_ratio_f32, KvEvictionStats & reset lifecycle
//   7. Invalid config, all-sink non-evictable guard, NaN/negative score & undersized step_scores guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_KV_EVICTION_SLOTS,
    KvEvictionPolicyKind,
    KvTokenRole,
    KvEvictionConfig,
    KvEvictionResult,
    KvEvictionStats,
    KvEvictionManager,
} from "cap:attention";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: StreamingLLM Attention Sink Permanent Pinning & Sliding Window
// ============================================================================

fn testStreamingSinkAndSlidingWindow()! {
    const cfg = KvEvictionConfig{
        sink_budget: 2,
        heavy_budget: 0,
        recent_budget: 3,
        score_decay_f32: 0.95,
        policy: KvEvictionPolicyKind.streaming_sink_window,
    };
    expect_eq(cfg.totalCapacity(), 5);
    expect_eq(cfg.isValid(), true);

    let mgr = cap.attention.KvEvictionManager.init(cfg) ?? return error.InitFailed;

    // Admit 5 tokens filling 2 sinks (pos 0, 1) + 3 recent (pos 2, 3, 4)
    let p: u64 = 0;
    while (p < 5) : (p += 1) {
        const res: KvEvictionResult = mgr.admit_token(p, (100 + p) as u32, 0.5);
        expect_eq(res.admitted, true);
        expect_eq(res.evicted, false);
    }

    // Admit pos 5 -> must evict oldest non-sink (pos 2, block 102), NEVER pos 0 or 1!
    const r5 = mgr.admit_token(5, 105, 0.5);
    expect_eq(r5.admitted, true);
    expect_eq(r5.evicted, true);
    expect_eq(r5.evicted_token_pos, 2);
    expect_eq(r5.evicted_kv_block_idx, 102);

    // Admit pos 6 -> must evict pos 3
    const r6 = mgr.admit_token(6, 106, 0.5);
    expect_eq(r6.evicted, true);
    expect_eq(r6.evicted_token_pos, 3);

    // Sinks 0 and 1 must still be resident and pinned as attention_sink
    const s0 = mgr.lookup_by_token_pos(0) ?? return error.MissingSink0;
    const s1 = mgr.lookup_by_token_pos(1) ?? return error.MissingSink1;
    expect_eq(mgr.slots[s0].role == KvTokenRole.attention_sink, true);
    expect_eq(mgr.slots[s1].role == KvTokenRole.attention_sink, true);
    expect_eq(mgr.lookup_by_token_pos(2) == null, true);
    expect_eq(mgr.lookup_by_token_pos(3) == null, true);
}

// ============================================================================
// Test 2: H2O Heavy-Hitter High-Score Retention & LRU Tie-Breaking
// ============================================================================

fn testH2oHeavyHitterRetentionAndLruTieBreak()! {
    const cfg = KvEvictionConfig{
        sink_budget: 0,
        heavy_budget: 4,
        recent_budget: 0,
        score_decay_f32: 1.0,
        policy: KvEvictionPolicyKind.h2o_heavy_hitter,
    };
    let mgr = KvEvictionManager.init(cfg) ?? return error.InitFailed;

    // Admit 4 tokens: pos 10 (score 9.0), pos 11 (score 1.0), pos 12 (score 1.0), pos 13 (score 5.0)
    * = mgr.admit_token(10, 10, 9.0);
    * = mgr.admit_token(11, 11, 1.0); // earliest access among tied score 1.0
    * = mgr.admit_token(12, 12, 1.0);
    * = mgr.admit_token(13, 13, 5.0);

    // Admit pos 14 (score 3.0) -> tie between pos 11 & pos 12 (both 1.0) -> LRU evicts pos 11!
    const r14 = mgr.admit_token(14, 14, 3.0);
    expect_eq(r14.admitted, true);
    expect_eq(r14.evicted, true);
    expect_eq(r14.evicted_token_pos, 11);

    // Admit pos 15 (score 4.0) -> lowest score is now pos 12 (1.0) -> evicts pos 12!
    const r15 = mgr.admit_token(15, 15, 4.0);
    expect_eq(r15.evicted_token_pos, 12);

    // High-score heavy hitters pos 10 (9.0) and pos 13 (5.0) remain resident
    expect_eq(mgr.lookup_by_token_pos(10) != null, true);
    expect_eq(mgr.lookup_by_token_pos(13) != null, true);
}

// ============================================================================
// Test 3: Hybrid Three-Zone Promotion & Strict Quota Conservation
// ============================================================================

fn testHybridThreeZonePromotionAndQuotaConservation()! {
    const cfg = KvEvictionConfig{
        sink_budget: 2,
        heavy_budget: 2,
        recent_budget: 2,
        score_decay_f32: 0.95,
        policy: KvEvictionPolicyKind.hybrid_sink_h2o_window,
    };
    let mgr = KvEvictionManager.init(cfg) ?? return error.InitFailed;

    // pos 0, 1 -> attention_sink
    * = mgr.admit_token(0, 0, 0.2);
    * = mgr.admit_token(1, 1, 0.2);
    // pos 2 (low 0.1), pos 3 (low 0.2) -> fill recent_window
    * = mgr.admit_token(2, 2, 0.1);
    * = mgr.admit_token(3, 3, 0.2);
    // pos 4 (high 8.0), pos 5 (low 0.3) -> push pos 2, 3 into heavy_hitter while filling capacity=6
    * = mgr.admit_token(4, 4, 8.0);
    * = mgr.admit_token(5, 5, 0.3);

    const st_full = mgr.stats();
    expect_eq(st_full.sink_count, 2);
    expect_eq(st_full.heavy_count, 2);
    expect_eq(st_full.recent_count, 2);

    // Admit pos 6 (score 0.4):
    // pos 4 (8.0) slides out of recent_window, beats weakest heavy_hitter pos 2 (0.1) ->
    // pos 4 is PROMOTED to heavy_hitter, pos 2 is EVICTED, pos 6 enters recent_window!
    const r6 = mgr.admit_token(6, 6, 0.4);
    expect_eq(r6.admitted, true);
    expect_eq(r6.evicted, true);
    expect_eq(r6.evicted_token_pos, 2);

    const slot4 = mgr.lookup_by_token_pos(4) ?? return error.MissingPromoted4;
    expect_eq(mgr.slots[slot4].role == KvTokenRole.heavy_hitter, true);

    // Three-zone budgets strictly conserved!
    const st_after = mgr.stats();
    expect_eq(st_after.sink_count, 2);
    expect_eq(st_after.heavy_count, 2);
    expect_eq(st_after.recent_count, 2);
}

// ============================================================================
// Test 4: Exponential Decay (@fma) & Hot-Token Migration
// ============================================================================

fn testAttentionScoreDecayAndHotTokenMigration()! {
    const cfg = KvEvictionConfig{
        sink_budget: 0,
        heavy_budget: 3,
        recent_budget: 0,
        score_decay_f32: 0.5, // fast 50% decay per step to test migration cleanly
        policy: KvEvictionPolicyKind.h2o_heavy_hitter,
    };
    let mgr = KvEvictionManager.init(cfg) ?? return error.InitFailed;

    const r0 = mgr.admit_token(0, 0, 4.0); // slot 0 starts high (4.0)
    const r1 = mgr.admit_token(1, 1, 1.0); // slot 1 starts low (1.0)
    const r2 = mgr.admit_token(2, 2, 2.0); // slot 2 medium (2.0)

    // Two decode steps where slot 1 receives fresh attention (+3.0 each step) while slot 0 receives 0.0
    // Step 1: slot0 = 4.0*0.5 + 0 = 2.0, slot1 = 1.0*0.5 + 3.0 = 3.5, slot2 = 2.0*0.5 + 1.0 = 2.0
    // Step 2: slot0 = 2.0*0.5 + 0 = 1.0, slot1 = 3.5*0.5 + 3.0 = 4.75, slot2 = 2.0*0.5 + 1.0 = 2.0
    const step_s: [3]f32 = [0.0, 3.0, 1.0];
    expect_eq(mgr.update_attention_scores(step_s[0..]), true);
    expect_eq(mgr.update_attention_scores(step_s[0..]), true);

    expect_eq(approxEqF32(mgr.slots[r0.assigned_slot].cum_attn_score, 1.0, 0.001), true);
    expect_eq(approxEqF32(mgr.slots[r1.assigned_slot].cum_attn_score, 4.75, 0.001), true);
    expect_eq(approxEqF32(mgr.slots[r2.assigned_slot].cum_attn_score, 2.0, 0.001), true);

    // Now admit pos 3: decayed slot 0 (1.0) is now the weakest and gets evicted instead of slot 1!
    const r3 = mgr.admit_token(3, 3, 2.5);
    expect_eq(r3.evicted, true);
    expect_eq(r3.evicted_token_pos, 0);
    expect_eq(mgr.lookup_by_token_pos(1) != null, true);
}

// ============================================================================
// Test 5: Ascending RoPE Active Position Collection
// ============================================================================

fn testCollectActivePositionsAscending()! {
    const cfg = KvEvictionConfig{
        sink_budget: 2,
        heavy_budget: 2,
        recent_budget: 2,
        score_decay_f32: 0.95,
        policy: KvEvictionPolicyKind.hybrid_sink_h2o_window,
    };
    let mgr = KvEvictionManager.init(cfg) ?? return error.InitFailed;

    let pos: u64 = 0;
    while (pos < 12) : (pos += 1) {
        const sc: f32 = if (pos == 4 || pos == 7) 10.0 else 0.2;
        * = mgr.admit_token(pos, pos as u32, sc);
    }

    let out_pos: [6]u64 = [0; 6];
    let out_s: []u64 = out_pos[0..];
    const n = mgr.collect_active_positions(out_s);
    expect_eq(n, 6);

    // Must be strictly ascending and contain sinks (0, 1), heavy hitters (4, 7), recent (10, 11)
    expect_eq(out_pos[0], 0);
    expect_eq(out_pos[1], 1);
    expect_eq(out_pos[2], 4);
    expect_eq(out_pos[3], 7);
    expect_eq(out_pos[4], 10);
    expect_eq(out_pos[5], 11);
}

// ============================================================================
// Test 6: Compression Ratio, Stats & Reset Lifecycle
// ============================================================================

fn testCompressionRatioStatsAndReset()! {
    const cfg = KvEvictionConfig{
        sink_budget: 2,
        heavy_budget: 4,
        recent_budget: 4,
        score_decay_f32: 0.95,
        policy: KvEvictionPolicyKind.hybrid_sink_h2o_window,
    };
    let mgr = KvEvictionManager.init(cfg) ?? return error.InitFailed;

    // Stream 50 tokens through a 10-slot KV cache -> exact 5.0x compression ratio
    let pos: u64 = 0;
    while (pos < 50) : (pos += 1) {
        const r = mgr.admit_token(pos, pos as u32, 0.5);
        expect_eq(r.admitted, true);
    }

    const st: KvEvictionStats = mgr.stats();
    expect_eq(st.total_admitted, 50);
    expect_eq(st.total_evicted, 40);
    expect_eq(st.sink_count, 2);
    expect_eq(st.heavy_count, 4);
    expect_eq(st.recent_count, 4);
    expect_eq(approxEqF32(st.compression_ratio_f32, 5.0, 0.001), true);

    mgr.reset();
    const st0 = mgr.stats();
    expect_eq(st0.total_admitted, 0);
    expect_eq(st0.total_evicted, 0);
    expect_eq(st0.sink_count, 0);
    expect_eq(approxEqF32(st0.compression_ratio_f32, 1.0, 0.001), true);
}

// ============================================================================
// Test 7: Boundary & Invalid Input Guards
// ============================================================================

fn testBoundaryAndInvalidInputGuards()! {
    // 1. Invalid configs
    expect_eq(KvEvictionManager.init(KvEvictionConfig{
        sink_budget: 0, heavy_budget: 0, recent_budget: 0, score_decay_f32: 0.9,
    }) == null, true); // zero capacity
    expect_eq(KvEvictionManager.init(KvEvictionConfig{
        sink_budget: MAX_KV_EVICTION_SLOTS + 1, heavy_budget: 0, recent_budget: 0, score_decay_f32: 0.9,
    }) == null, true); // exceeds MAX_KV_EVICTION_SLOTS
    expect_eq(KvEvictionManager.init(KvEvictionConfig{
        sink_budget: 2, heavy_budget: 2, recent_budget: 2, score_decay_f32: 0.0,
    }) == null, true); // decay <= 0
    expect_eq(KvEvictionManager.init(KvEvictionConfig{
        sink_budget: 2, heavy_budget: 2, recent_budget: 2, score_decay_f32: 1.5,
    }) == null, true); // decay > 1
    expect_eq(KvEvictionManager.init(KvEvictionConfig{
        sink_budget: 2, heavy_budget: 0, recent_budget: 0, score_decay_f32: 0.9,
        policy: KvEvictionPolicyKind.streaming_sink_window,
    }) == null, true); // streaming_sink_window requires recent_budget > 0

    // 2. All-sink hybrid cache cannot evict once full
    let all_sink = KvEvictionManager.init(KvEvictionConfig{
        sink_budget: 2, heavy_budget: 0, recent_budget: 0, score_decay_f32: 0.9,
        policy: KvEvictionPolicyKind.hybrid_sink_h2o_window,
    }) ?? return error.InitFailed;
    expect_eq(all_sink.admit_token(0, 0, 1.0).admitted, true);
    expect_eq(all_sink.admit_token(1, 1, 1.0).admitted, true);
    expect_eq(all_sink.admit_token(2, 2, 1.0).admitted, false); // all slots pinned as sinks

    // 3. Invalid scores (negative, NaN, Inf) and undersized step_scores slice
    expect_eq(all_sink.admit_token(3, 3, -0.1).admitted, false);
    expect_eq(all_sink.admit_token(3, 3, std.math.nan(f32)).admitted, false);
    expect_eq(all_sink.admit_token(3, 3, std.math.inf(f32)).admitted, false);

    const short_s: [1]f32 = [1.0];
    const neg_s: [2]f32 = [1.0, -0.5];
    expect_eq(all_sink.update_attention_scores(short_s[0..]), false);
    expect_eq(all_sink.update_attention_scores(neg_s[0..]), false);
}

test("attention kv_eviction: StreamingLLM attention sink pinning & sliding window", testStreamingSinkAndSlidingWindow);
test("attention kv_eviction: H2O heavy-hitter retention & LRU tie-breaking", testH2oHeavyHitterRetentionAndLruTieBreak);
test("attention kv_eviction: hybrid three-zone promotion & quota conservation", testHybridThreeZonePromotionAndQuotaConservation);
test("attention kv_eviction: exponential decay (@fma) & hot-token migration", testAttentionScoreDecayAndHotTokenMigration);
test("attention kv_eviction: ascending RoPE active position collection", testCollectActivePositionsAscending);
test("attention kv_eviction: compression_ratio_f32, stats & reset lifecycle", testCompressionRatioStatsAndReset);
test("attention kv_eviction: boundary & invalid input guards", testBoundaryAndInvalidInputGuards);
