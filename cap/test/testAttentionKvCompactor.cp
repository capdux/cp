// Origin: Pure-CP Stdlib Test Suite (lib/attention/kvCompactor.cp) | Created: 2026-11-22
// Production Deep Verification Suite for StreamingLLM + H2O KV-Cache Compactor:
//   1. Three-zone classification (sink + heavy_hitter + recent_window) & in-place compaction order preservation
//   2. Per-decode-step hardware @fma EMA importance accumulation & dynamic heavy-hitter shift
//   3. Deterministic tie-breaking (attn_score_ema -> last_step -> pos_id)
//   4. Undersized out_evicted_pos truncation guard & idempotent compaction when within budget
//   5. Invalid budget/decay config guards & atomic weight-length mismatch rejection
//   6. NaN / +/-Inf / negative attention weight sanitization in both appendToken & updateAttentionStep
//   7. Full-pool (256 slots) auto-compaction, auto_compact_on_full=false rejection, & all-sink protection
//   8. Empty-pool compaction guard, 600-token multi-round streaming stress, & reset() full recovery

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KV_COMPACTOR_MAX_SLOTS,
    KvSlotRole,
    KvCompactorSlot,
    KvCompactorConfig,
    KvCompactReport,
    KvCompactorStats,
    KvCacheCompactor,
} from "cap:attention";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff: f32 = if (a > b) (a - b) else (b - a);
    return diff <= eps;
}

test("kvCompactor: three-zone classification & in-place compaction preserves chronological order", () => {
    const cfg = KvCompactorConfig {
        sink_budget: 2,
        heavy_budget: 2,
        recent_budget: 2,
        ema_decay: 0.9,
        auto_compact_on_full: true,
    };
    let comp = KvCacheCompactor.init(cfg) ?? unreachable;

    // Append 10 tokens (pos_id = 100..109)
    let i: u64 = 0;
    while (i < 10) : (i += 1) {
        expect_eq(comp.appendToken(100 + i, 0.0), true);
    }
    expect_eq(comp.len(), 10 as usize);
    expect_eq(comp.needsCompaction(), true);

    // Give strong attention to middle tokens pos 103 and pos 106; low attention to sinks (0, 1)
    // Sinks must still be pinned regardless of low attention score.
    const w: [10]f32 = [0.001, 0.001, 0.05, 0.92, 0.03, 0.08, 0.84, 0.02, 0.01, 0.01];
    expect_eq(comp.updateAttentionStep(w[0..]), true);

    const pre_stats = comp.stats();
    expect_eq(pre_stats.sink_count, 2 as usize);
    expect_eq(pre_stats.heavy_count, 2 as usize);
    expect_eq(pre_stats.recent_count, 2 as usize);
    expect_eq(pre_stats.evictable_count, 4 as usize);

    let evicted: [8]u64 = [0; 8];
    let evicted_s: []u64 = evicted[0..];
    const rep = comp.compactToBudget(*evicted_s);

    expect_eq(rep.before_len, 10 as usize);
    expect_eq(rep.after_len, 6 as usize);
    expect_eq(rep.evicted_count, 4 as usize);
    expect_eq(rep.written_evicted_pos, 4 as usize);
    expect_eq(rep.sink_kept, 2 as usize);
    expect_eq(rep.heavy_kept, 2 as usize);
    expect_eq(rep.recent_kept, 2 as usize);
    expect_eq(comp.len(), 6 as usize);
    expect_eq(comp.needsCompaction(), false);

    // Retained logical positions in strictly ascending chronological order:
    // sinks: 100, 101 | heavy: 103, 106 | recent: 108, 109
    const active = comp.activeSlots();
    expect_eq(active.len, 6 as usize);
    expect_eq(active[0].pos_id, 100 as u64);
    expect_eq(active[0].role, KvSlotRole.sink);
    expect_eq(active[1].pos_id, 101 as u64);
    expect_eq(active[1].role, KvSlotRole.sink);
    expect_eq(active[2].pos_id, 103 as u64);
    expect_eq(active[2].role, KvSlotRole.heavy_hitter);
    expect_eq(active[3].pos_id, 106 as u64);
    expect_eq(active[3].role, KvSlotRole.heavy_hitter);
    expect_eq(active[4].pos_id, 108 as u64);
    expect_eq(active[4].role, KvSlotRole.recent_window);
    expect_eq(active[5].pos_id, 109 as u64);
    expect_eq(active[5].role, KvSlotRole.recent_window);

    // Evicted positions: 102, 104, 105, 107
    expect_eq(evicted[0], 102 as u64);
    expect_eq(evicted[1], 104 as u64);
    expect_eq(evicted[2], 105 as u64);
    expect_eq(evicted[3], 107 as u64);

    // Verify findByPosId after compaction
    expect_eq(comp.findByPosId(106) ?? 999, 3 as usize);
    expect_eq(comp.findByPosId(104) == null, true);
});

test("kvCompactor: @fma EMA score accumulation & heavy-hitter turnover across decode steps", () => {
    const cfg = KvCompactorConfig {
        sink_budget: 1,
        heavy_budget: 1,
        recent_budget: 1,
        ema_decay: 0.5,
        auto_compact_on_full: true,
    };
    let comp = KvCacheCompactor.init(cfg) ?? unreachable;

    // 4 tokens: sink(0), mid(1), mid(2), recent(3)
    expect_eq(comp.appendToken(0, 0.2), true);
    expect_eq(comp.appendToken(1, 0.8), true);
    expect_eq(comp.appendToken(2, 0.1), true);
    expect_eq(comp.appendToken(3, 0.0), true);

    // Step 1: pos 1 decays (0.5*0.8 + 0.0 = 0.4), pos 2 surges (0.5*0.1 + 0.5 = 0.55)
    const w1: [4]f32 = [0.1, 0.0, 0.5, 0.2];
    expect_eq(comp.updateAttentionStep(w1[0..]), true);

    const s1 = comp.slotAt(1) ?? unreachable;
    const s2 = comp.slotAt(2) ?? unreachable;
    expect_eq(approxEqF32(s1.attn_score_ema, 0.4, 0.0001), true);
    expect_eq(approxEqF32(s2.attn_score_ema, 0.55, 0.0001), true);
    expect_eq(s2.role, KvSlotRole.heavy_hitter);
    expect_eq(s1.role, KvSlotRole.evictable);

    // Step 2: pos 1 gets 0.6 -> 0.5*0.4 + 0.6 = 0.8; pos 2 gets 0.0 -> 0.5*0.55 + 0.0 = 0.275
    const w2: [4]f32 = [0.0, 0.6, 0.0, 0.1];
    expect_eq(comp.updateAttentionStep(w2[0..]), true);

    const s1_b = comp.slotAt(1) ?? unreachable;
    const s2_b = comp.slotAt(2) ?? unreachable;
    expect_eq(approxEqF32(s1_b.attn_score_ema, 0.8, 0.0001), true);
    expect_eq(approxEqF32(s2_b.attn_score_ema, 0.275, 0.0001), true);
    expect_eq(s1_b.role, KvSlotRole.heavy_hitter);
    expect_eq(s2_b.role, KvSlotRole.evictable);

    // Verify last_step: pos 1 was updated at step 6 (4 appends + 2 updates), pos 2 stayed at step 5
    expect_eq(s1_b.last_step, 6 as u64);
    expect_eq(s2_b.last_step, 5 as u64);

    let ev: [2]u64 = [0; 2];
    let ev_s: []u64 = ev[0..];
    const rep = comp.compactToBudget(*ev_s);
    expect_eq(rep.evicted_count, 1 as usize);
    expect_eq(ev[0], 2 as u64);
    expect_eq(comp.findByPosId(1) != null, true);
});

test("kvCompactor: deterministic tie-breaking by last_step then pos_id", () => {
    const cfg = KvCompactorConfig {
        sink_budget: 1,
        heavy_budget: 1,
        recent_budget: 1,
        ema_decay: 1.0,
        auto_compact_on_full: true,
    };
    let comp = KvCacheCompactor.init(cfg) ?? unreachable;

    // 5 tokens: sink(10), middle(20, 30, 40), recent(50)
    expect_eq(comp.appendToken(10, 0.0), true);
    expect_eq(comp.appendToken(20, 0.5), true); // last_step = 2
    expect_eq(comp.appendToken(30, 0.5), true); // last_step = 3
    expect_eq(comp.appendToken(40, 0.5), true); // last_step = 4
    expect_eq(comp.appendToken(50, 0.0), true);

    // All three middle tokens have equal score 0.5; pos 40 has newest last_step=4 -> wins heavy_hitter
    const s40 = comp.slotAt(3) ?? unreachable;
    expect_eq(s40.role, KvSlotRole.heavy_hitter);

    // Now update pos 20 and pos 30 simultaneously in the same step so they share both score=1.0 and last_step=6.
    // Pos 30 has larger pos_id (30 > 20) -> must win deterministically.
    const w: [5]f32 = [0.0, 0.5, 0.5, 0.0, 0.0];
    expect_eq(comp.updateAttentionStep(w[0..]), true);

    const s20 = comp.slotAt(1) ?? unreachable;
    const s30 = comp.slotAt(2) ?? unreachable;
    expect_eq(approxEqF32(s20.attn_score_ema, 1.0, 0.0001), true);
    expect_eq(approxEqF32(s30.attn_score_ema, 1.0, 0.0001), true);
    expect_eq(s20.last_step, s30.last_step);
    expect_eq(s30.role, KvSlotRole.heavy_hitter);
    expect_eq(s20.role, KvSlotRole.evictable);
});

test("kvCompactor: undersized out_evicted_pos truncation & within-budget idempotency", () => {
    const cfg = KvCompactorConfig {
        sink_budget: 1,
        heavy_budget: 1,
        recent_budget: 1,
        ema_decay: 0.9,
        auto_compact_on_full: true,
    };
    let comp = KvCacheCompactor.init(cfg) ?? unreachable;

    let i: u64 = 0;
    while (i < 8) : (i += 1) {
        expect_eq(comp.appendToken(i, 0.1), true);
    }
    // 8 active, budget 3 -> 5 evicted, but output buffer only holds 2
    let small_buf: [2]u64 = [999, 999];
    let small_s: []u64 = small_buf[0..];
    const rep1 = comp.compactToBudget(*small_s);
    expect_eq(rep1.before_len, 8 as usize);
    expect_eq(rep1.after_len, 3 as usize);
    expect_eq(rep1.evicted_count, 5 as usize);
    expect_eq(rep1.written_evicted_pos, 2 as usize);
    expect_eq(comp.stats().total_compactions, 1 as u64);

    // Second compaction when already within budget must be a no-op
    const rep2 = comp.compactToBudget(*small_s);
    expect_eq(rep2.before_len, 3 as usize);
    expect_eq(rep2.after_len, 3 as usize);
    expect_eq(rep2.evicted_count, 0 as usize);
    expect_eq(rep2.written_evicted_pos, 0 as usize);
    expect_eq(comp.stats().total_compactions, 1 as u64);
});

test("kvCompactor: invalid budget/decay config & atomic weight-length mismatch guards", () => {
    // Budget sum == 0
    const zero_cfg = KvCompactorConfig {
        sink_budget: 0,
        heavy_budget: 0,
        recent_budget: 0,
        ema_decay: 0.9,
        auto_compact_on_full: true,
    };
    expect_eq(KvCacheCompactor.init(zero_cfg) == null, true);

    // Budget sum > KV_COMPACTOR_MAX_SLOTS
    const over_cfg = KvCompactorConfig {
        sink_budget: 100,
        heavy_budget: 100,
        recent_budget: 100,
        ema_decay: 0.9,
        auto_compact_on_full: true,
    };
    expect_eq(KvCacheCompactor.init(over_cfg) == null, true);

    // Invalid ema_decay: 0.0, negative, > 1.0, NaN, Inf
    const bad_decays: [5]f32 = [0.0, -0.5, 1.01, std.math.nan(f32), std.math.inf(f32)];
    let k: usize = 0;
    while (k < 5) : (k += 1) {
        const bad_cfg = KvCompactorConfig {
            sink_budget: 2,
            heavy_budget: 2,
            recent_budget: 2,
            ema_decay: bad_decays[k],
            auto_compact_on_full: true,
        };
        expect_eq(KvCacheCompactor.init(bad_cfg) == null, true);
    }

    // Valid compactor: weight length mismatch must return false atomically
    let comp = KvCacheCompactor.init(KvCompactorConfig.defaults()) ?? unreachable;
    expect_eq(comp.appendToken(1, 0.25), true);
    expect_eq(comp.appendToken(2, 0.50), true);
    const step_before: u64 = comp.stats().current_step;

    const short_w: [1]f32 = [0.9];
    const long_w: [3]f32 = [0.9, 0.9, 0.9];
    expect_eq(comp.updateAttentionStep(short_w[0..]), false);
    expect_eq(comp.updateAttentionStep(long_w[0..]), false);

    // State completely untouched
    expect_eq(comp.stats().current_step, step_before);
    expect_eq(approxEqF32((comp.slotAt(0) ?? unreachable).attn_score_ema, 0.25, 0.0001), true);
    expect_eq(approxEqF32((comp.slotAt(1) ?? unreachable).attn_score_ema, 0.50, 0.0001), true);
    expect_eq(comp.slotAt(2) == null, true);
});

test("kvCompactor: NaN / +/-Inf / negative weight sanitization", () => {
    const cfg = KvCompactorConfig {
        sink_budget: 1,
        heavy_budget: 2,
        recent_budget: 1,
        ema_decay: 0.8,
        auto_compact_on_full: true,
    };
    let comp = KvCacheCompactor.init(cfg) ?? unreachable;

    const nan_v: f32 = std.math.nan(f32);
    const pos_inf: f32 = std.math.inf(f32);
    const neg_inf: f32 = -std.math.inf(f32);

    // Malformed initial weights sanitized to 0.0
    expect_eq(comp.appendToken(0, nan_v), true);
    expect_eq(comp.appendToken(1, pos_inf), true);
    expect_eq(comp.appendToken(2, neg_inf), true);
    expect_eq(comp.appendToken(3, -3.5), true);
    expect_eq(comp.appendToken(4, 0.5), true);

    expect_eq(approxEqF32((comp.slotAt(0) ?? unreachable).attn_score_ema, 0.0, 0.0001), true);
    expect_eq(approxEqF32((comp.slotAt(1) ?? unreachable).attn_score_ema, 0.0, 0.0001), true);
    expect_eq(approxEqF32((comp.slotAt(2) ?? unreachable).attn_score_ema, 0.0, 0.0001), true);
    expect_eq(approxEqF32((comp.slotAt(3) ?? unreachable).attn_score_ema, 0.0, 0.0001), true);
    expect_eq(approxEqF32((comp.slotAt(4) ?? unreachable).attn_score_ema, 0.5, 0.0001), true);

    const prev_step_0: u64 = (comp.slotAt(0) ?? unreachable).last_step;
    const dirty_w: [5]f32 = [nan_v, -1.0, pos_inf, neg_inf, 0.25];
    expect_eq(comp.updateAttentionStep(dirty_w[0..]), true);

    // Sanitized zero weights must not poison EMA or bump last_step
    expect_eq(approxEqF32((comp.slotAt(0) ?? unreachable).attn_score_ema, 0.0, 0.0001), true);
    expect_eq((comp.slotAt(0) ?? unreachable).last_step, prev_step_0);
    expect_eq(approxEqF32((comp.slotAt(1) ?? unreachable).attn_score_ema, 0.0, 0.0001), true);
    expect_eq(approxEqF32((comp.slotAt(2) ?? unreachable).attn_score_ema, 0.0, 0.0001), true);
    expect_eq(approxEqF32((comp.slotAt(3) ?? unreachable).attn_score_ema, 0.0, 0.0001), true);
    // Slot 4: 0.8 * 0.5 + 0.25 = 0.65
    expect_eq(approxEqF32((comp.slotAt(4) ?? unreachable).attn_score_ema, 0.65, 0.0001), true);
});

test("kvCompactor: full-pool auto-eviction, strict rejection, & all-sink protection", () => {
    // Case A: auto_compact_on_full = true with budget 16 -> filling 256 then appending triggers compaction
    const auto_cfg = KvCompactorConfig {
        sink_budget: 4,
        heavy_budget: 6,
        recent_budget: 6,
        ema_decay: 0.9,
        auto_compact_on_full: true,
    };
    let comp_auto = KvCacheCompactor.init(auto_cfg) ?? unreachable;
    let p: u64 = 0;
    while (p < KV_COMPACTOR_MAX_SLOTS as u64) : (p += 1) {
        expect_eq(comp_auto.appendToken(p, 0.01), true);
    }
    expect_eq(comp_auto.len(), KV_COMPACTOR_MAX_SLOTS);
    // 257th token triggers automatic compaction to 16, then appends -> len == 17
    expect_eq(comp_auto.appendToken(9999, 0.5), true);
    expect_eq(comp_auto.len(), 17 as usize);
    expect_eq(comp_auto.stats().total_compactions, 1 as u64);
    expect_eq(comp_auto.stats().total_evicted, 240 as u64);
    // Sinks 0..3 must remain intact
    expect_eq((comp_auto.slotAt(0) ?? unreachable).pos_id, 0 as u64);
    expect_eq((comp_auto.slotAt(3) ?? unreachable).pos_id, 3 as u64);

    // Case B: auto_compact_on_full = false -> rejects 257th token
    const strict_cfg = KvCompactorConfig {
        sink_budget: 4,
        heavy_budget: 6,
        recent_budget: 6,
        ema_decay: 0.9,
        auto_compact_on_full: false,
    };
    let comp_strict = KvCacheCompactor.init(strict_cfg) ?? unreachable;
    p = 0;
    while (p < KV_COMPACTOR_MAX_SLOTS as u64) : (p += 1) {
        expect_eq(comp_strict.appendToken(p, 0.01), true);
    }
    expect_eq(comp_strict.appendToken(9999, 0.5), false);
    expect_eq(comp_strict.len(), KV_COMPACTOR_MAX_SLOTS);

    // Case C: all-sink budget == KV_COMPACTOR_MAX_SLOTS -> sinks are never evicted
    const all_sink_cfg = KvCompactorConfig {
        sink_budget: KV_COMPACTOR_MAX_SLOTS,
        heavy_budget: 0,
        recent_budget: 0,
        ema_decay: 0.9,
        auto_compact_on_full: true,
    };
    let comp_sink = KvCacheCompactor.init(all_sink_cfg) ?? unreachable;
    p = 0;
    while (p < KV_COMPACTOR_MAX_SLOTS as u64) : (p += 1) {
        expect_eq(comp_sink.appendToken(p, 0.01), true);
    }
    expect_eq(comp_sink.appendToken(9999, 1.0), false);
    expect_eq(comp_sink.len(), KV_COMPACTOR_MAX_SLOTS);
});

test("kvCompactor: empty-pool guard, 600-token streaming stress & reset recovery", () => {
    const cfg = KvCompactorConfig {
        sink_budget: 4,
        heavy_budget: 12,
        recent_budget: 16,
        ema_decay: 0.92,
        auto_compact_on_full: true,
    };
    let comp = KvCacheCompactor.init(cfg) ?? unreachable;

    // Empty pool compaction & empty attention update are safe no-ops
    let ev_buf: [64]u64 = [0; 64];
    let ev_s: []u64 = ev_buf[0..];
    const empty_rep = comp.compactToBudget(*ev_s);
    expect_eq(empty_rep.before_len, 0 as usize);
    expect_eq(empty_rep.after_len, 0 as usize);
    expect_eq(empty_rep.evicted_count, 0 as usize);

    const empty_w: [0]f32 = [];
    expect_eq(comp.updateAttentionStep(empty_w[0..]), true);

    // Stream 600 tokens, boosting every 50th token as a long-range heavy hitter
    let weights_buf: [KV_COMPACTOR_MAX_SLOTS]f32 = [0.0; KV_COMPACTOR_MAX_SLOTS];
    let pos: u64 = 0;
    while (pos < 600) : (pos += 1) {
        const init_w: f32 = if (pos % 50 == 0 && pos >= 50) 5.0 else 0.02;
        expect_eq(comp.appendToken(pos, init_w), true);

        // Periodic decode attention update + compaction when 16 tokens over budget
        if (comp.len() >= 48) {
            const cur_n: usize = comp.len();
            let j: usize = 0;
            while (j < cur_n) : (j += 1) {
                const slot = comp.slotAt(j) ?? unreachable;
                weights_buf[j] = if (slot.pos_id % 50 == 0 && slot.pos_id >= 50) 1.5 else 0.01;
            }
            expect_eq(comp.updateAttentionStep(weights_buf[0..cur_n]), true);
            const rep = comp.compactToBudget(*ev_s);
            expect_eq(rep.after_len, 32 as usize);
            expect_eq(rep.sink_kept, 4 as usize);
            expect_eq(rep.heavy_kept, 12 as usize);
            expect_eq(rep.recent_kept, 16 as usize);
        }
    }

    // Final compact to exact budget 32
    * = comp.compactToBudget(*ev_s);
    expect_eq(comp.len(), 32 as usize);

    // Verify invariants: sinks 0..3 permanently retained & active slots strictly sorted by pos_id
    const active = comp.activeSlots();
    expect_eq(active[0].pos_id, 0 as u64);
    expect_eq(active[1].pos_id, 1 as u64);
    expect_eq(active[2].pos_id, 2 as u64);
    expect_eq(active[3].pos_id, 3 as u64);
    let idx: usize = 1;
    while (idx < active.len) : (idx += 1) {
        expect_eq(active[idx].pos_id > active[idx - 1].pos_id, true);
    }

    const st = comp.stats();
    expect_eq(st.total_appended, 600 as u64);
    expect_eq(st.total_evicted, 568 as u64);
    expect_eq(st.total_compactions > 0, true);

    // Reset clears everything cleanly while preserving budget config
    comp.reset();
    expect_eq(comp.len(), 0 as usize);
    expect_eq(comp.budget(), 32 as usize);
    expect_eq(comp.stats().total_appended, 0 as u64);
    expect_eq(comp.stats().total_evicted, 0 as u64);
    expect_eq(comp.stats().total_compactions, 0 as u64);
    expect_eq(comp.appendToken(42, 0.5), true);
    expect_eq(comp.len(), 1 as usize);
});
