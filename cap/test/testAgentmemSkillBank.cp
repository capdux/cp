// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/skillBank.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Procedural Skill Memory Bank (Voyager / Reflexion):
//   1. Skill registration L2 normalization, @dot cosine recall & Laplace Bayesian initial reliability (0.5)
//   2. Closed-loop execution feedback (recordExecution): Bayesian reliability re-ranking & @fma running latency
//   3. Tool capability bitmask executability filter & @popCount tool-overlap scoring bonus
//   4. Unreliable skill pruning (pruneUnreliable) with min_trials gate & pinned core skill protection
//   5. Top-K truncation, deterministic tie-breaking (skill_id asc) & min_cosine threshold filtering
//   6. Full 64-slot capacity stress, slot reclamation after prune/remove, setPinned & clear
//   7. Full boundary guards (invalid dim, zero/NaN vec, dup ID, bad latency, bad bps) & cap.agentmem facade

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SKILL_BANK_MAX_SLOTS,
    SKILL_BANK_MAX_DIM,
    SKILL_BANK_MAX_BPS,
    SkillRetrievalConfig,
    SkillRecallHit,
    SkillBank,
} from "cap:agentmem";

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

// ============================================================================
// Test 1: Registration L2 Normalization, @dot Recall & Initial Reliability 0.5
// ============================================================================
test("skillBank: L2-normalized @dot recall and neutral 0.5 Laplace reliability for new skills", () => {
    let bank = SkillBank.init(4) ?? return;

    // Unnormalized vectors (scaled by 5.0 and 10.0) must normalize cleanly
    const v_a: [4]f32 = [5.0, 0.0, 0.0, 0.0];
    const v_b: [4]f32 = [0.0, 10.0, 0.0, 0.0];
    expect_true(bank.registerSkill(10, v_a[0..], 0b0001, false, 1000));
    expect_true(bank.registerSkill(20, v_b[0..], 0b0001, false, 1000));
    expect_eq(bank.len(), 2);

    if (rec := bank.getSkill(10)) {
        expect_true(absF32(rec.reliability() - 0.5) < 1.0e-5);
        expect_eq(rec.reliabilityBps(), 5000);
        expect_eq(rec.totalTrials(), 0);
    } else {
        expect_true(false);
    }

    const q: [4]f32 = [0.9, 0.1, 0.0, 0.0];
    let hits: [4]SkillRecallHit = [SkillRecallHit{}; 4];
    let hits_s: []SkillRecallHit = hits[0..];
    const n = bank.retrieveSkills(q[0..], 0b0001, SkillRetrievalConfig.defaults(), *hits_s);
    expect_eq(n, 2);
    expect_eq(hits[0].skill_id, 10);
    expect_eq(hits[1].skill_id, 20);
    expect_true(hits[0].cosine_sim > 0.99);
    expect_true(hits[0].total_score > hits[1].total_score);
});

// ============================================================================
// Test 2: Closed-Loop Execution Feedback & Bayesian Reliability Re-Ranking
// ============================================================================
test("skillBank: recordExecution updates Bayesian reliability, avg_latency_ms and re-ranks skills", () => {
    let bank = SkillBank.init(4) ?? return;
    const same_intent: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    expect_true(bank.registerSkill(1, same_intent[0..], 0, false, 100));
    expect_true(bank.registerSkill(2, same_intent[0..], 0, false, 100));

    // Skill 1: 1 success, 4 failures -> reliability = (1+1)/(1+4+2) = 2/7 (~0.2857)
    expect_true(bank.recordExecution(1, true, 100.0, 200));
    expect_true(bank.recordExecution(1, false, 200.0, 300));
    expect_true(bank.recordExecution(1, false, 300.0, 400));
    expect_true(bank.recordExecution(1, false, 200.0, 500));
    expect_true(bank.recordExecution(1, false, 200.0, 600));

    // Skill 2: 4 successes, 0 failures -> reliability = (4+1)/(4+0+2) = 5/6 (~0.8333)
    expect_true(bank.recordExecution(2, true, 40.0, 250));
    expect_true(bank.recordExecution(2, true, 60.0, 350));
    expect_true(bank.recordExecution(2, true, 50.0, 450));
    expect_true(bank.recordExecution(2, true, 50.0, 550));

    if (s1 := bank.getSkill(1)) {
        expect_eq(s1.success_count, 1);
        expect_eq(s1.failure_count, 4);
        expect_true(absF32(s1.reliability() - (2.0 / 7.0)) < 1.0e-5);
        expect_true(absF32(s1.avg_latency_ms - 200.0) < 1.0e-4);
        expect_eq(s1.last_used_ms, 600);
    } else {
        expect_true(false);
    }

    if (s2 := bank.getSkill(2)) {
        expect_true(absF32(s2.reliability() - (5.0 / 6.0)) < 1.0e-5);
        expect_true(absF32(s2.avg_latency_ms - 50.0) < 1.0e-4);
    } else {
        expect_true(false);
    }

    // Skill 2 must outrank Skill 1 despite identical intent vectors
    let hits: [4]SkillRecallHit = [SkillRecallHit{}; 4];
    let hits_s: []SkillRecallHit = hits[0..];
    const n = bank.retrieveSkills(same_intent[0..], 0, SkillRetrievalConfig.defaults(), *hits_s);
    expect_eq(n, 2);
    expect_eq(hits[0].skill_id, 2);
    expect_eq(hits[1].skill_id, 1);
    expect_true(hits[0].total_score > hits[1].total_score);

    const st = bank.stats();
    expect_eq(st.total_executions, 9);
    expect_eq(st.total_successes, 5);
    expect_eq(st.total_failures, 4);
});

// ============================================================================
// Test 3: Tool Bitmask Executability Filter & @popCount Overlap Bonus
// ============================================================================
test("skillBank: tool_mask strictly filters unexecutable skills & @popCount rewards tool overlap", () => {
    let bank = SkillBank.init(4) ?? return;
    const v: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    // Bit0=Shell, Bit1=WebSearch, Bit2=Python, Bit3=SqlDb
    expect_true(bank.registerSkill(10, v[0..], 0b0000, false, 100)); // pure reasoning (always executable)
    expect_true(bank.registerSkill(11, v[0..], 0b0001, false, 100)); // needs Shell
    expect_true(bank.registerSkill(12, v[0..], 0b0101, false, 100)); // needs Shell + Python
    expect_true(bank.registerSkill(13, v[0..], 0b1001, false, 100)); // needs Shell + SqlDb (unavailable!)

    // Environment has Shell + WebSearch + Python (0b0111), missing SqlDb (bit 3)
    const avail: u64 = 0b0111;
    let hits: [4]SkillRecallHit = [SkillRecallHit{}; 4];
    let hits_s: []SkillRecallHit = hits[0..];
    const n = bank.retrieveSkills(v[0..], avail, SkillRetrievalConfig.defaults(), *hits_s);

    // Skill 13 must be filtered out; remaining 3 ordered by @popCount tool overlap: 12 (2/3) > 11 (1/3) > 10 (0/3)
    expect_eq(n, 3);
    expect_eq(hits[0].skill_id, 12);
    expect_eq(hits[1].skill_id, 11);
    expect_eq(hits[2].skill_id, 10);
    expect_true(hits[0].tool_overlap > hits[1].tool_overlap);
    expect_true(hits[1].tool_overlap > hits[2].tool_overlap);
});

// ============================================================================
// Test 4: Unreliable Skill Pruning & Pinned Core Skill Protection
// ============================================================================
test("skillBank: pruneUnreliable removes failing non-pinned skills while protecting pinned & under-tested", () => {
    let bank = SkillBank.init(4) ?? return;
    const v: [4]f32 = [0.0, 1.0, 0.0, 0.0];

    expect_true(bank.registerSkill(1, v[0..], 0, false, 100)); // failing non-pinned -> PRUNE
    expect_true(bank.registerSkill(2, v[0..], 0, true, 100));  // failing PINNED -> PROTECT
    expect_true(bank.registerSkill(3, v[0..], 0, false, 100)); // under-tested (only 2 trials) -> PROTECT
    expect_true(bank.registerSkill(4, v[0..], 0, false, 100)); // reliable -> KEEP

    // Skill 1: 0/5 -> rel = 1/7 = 1429 bps (< 4000)
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        expect_true(bank.recordExecution(1, false, 10.0, 200));
        expect_true(bank.recordExecution(2, false, 10.0, 200));
        expect_true(bank.recordExecution(4, true, 10.0, 200));
    }
    // Skill 3: 0/2 -> rel = 1/4 = 2500 bps, but trials=2 < min_trials=4
    expect_true(bank.recordExecution(3, false, 10.0, 200));
    expect_true(bank.recordExecution(3, false, 10.0, 200));

    const pruned = bank.pruneUnreliable(4, 4000);
    expect_eq(pruned, 1);
    expect_eq(bank.len(), 3);
    expect_eq(bank.stats().total_pruned, 1);
    expect_eq(bank.stats().pinned_skills, 1);

    // Skill 1 is gone; Skills 2, 3, 4 survive
    if (gone := bank.getSkill(1)) {
        * = gone;
        expect_true(false);
    }
    expect_true(bank.getSkill(2) != null);
    expect_true(bank.getSkill(3) != null);
    expect_true(bank.getSkill(4) != null);
});

// ============================================================================
// Test 5: Top-K Truncation, Deterministic Tie-Break & min_cosine Filter
// ============================================================================
test("skillBank: Top-K truncation, skill_id tie-breaking and min_cosine filtering", () => {
    let bank = SkillBank.init(4) ?? return;
    const v_pos: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v_orth: [4]f32 = [0.0, 1.0, 0.0, 0.0];

    // Three identical-score skills registered out of ID order
    expect_true(bank.registerSkill(30, v_pos[0..], 0, false, 100));
    expect_true(bank.registerSkill(10, v_pos[0..], 0, false, 100));
    expect_true(bank.registerSkill(20, v_pos[0..], 0, false, 100));
    expect_true(bank.registerSkill(40, v_orth[0..], 0, false, 100));

    // Top-2 output buffer with min_cosine = 0.5 filters out orthogonal skill 40 and returns [10, 20]
    const cfg = SkillRetrievalConfig{
        reliability_weight: 1.0,
        tool_overlap_weight: 0.15,
        min_cosine: 0.5,
    };
    let top2: [2]SkillRecallHit = [SkillRecallHit{}; 2];
    let top2_s: []SkillRecallHit = top2[0..];
    const n = bank.retrieveSkills(v_pos[0..], 0, cfg, *top2_s);
    expect_eq(n, 2);
    expect_eq(top2[0].skill_id, 10);
    expect_eq(top2[1].skill_id, 20);
});

// ============================================================================
// Test 6: Full 64-Slot Capacity Stress, Slot Reclamation & setPinned/clear
// ============================================================================
test("skillBank: 64-slot full bank rejects overflow and reclaims slots after remove/prune", () => {
    let bank = SkillBank.init(4) ?? return;
    const v: [4]f32 = [0.5, 0.5, 0.5, 0.5];

    let id: u64 = 1;
    while (id <= (SKILL_BANK_MAX_SLOTS as u64)) : (id += 1) {
        expect_true(bank.registerSkill(id, v[0..], 0, false, 1000));
    }
    expect_eq(bank.len(), SKILL_BANK_MAX_SLOTS);
    // 65th skill rejected
    expect_true(!bank.registerSkill(999, v[0..], 0, false, 1000));

    // Remove skill 10 -> slot becomes available for 999
    expect_true(bank.removeSkill(10));
    expect_true(!bank.removeSkill(10));
    expect_true(bank.registerSkill(999, v[0..], 0, false, 1000));
    expect_eq(bank.len(), SKILL_BANK_MAX_SLOTS);

    // setPinned toggle & clear
    expect_true(bank.setPinned(999, true));
    expect_eq(bank.stats().pinned_skills, 1);
    bank.clear();
    expect_eq(bank.len(), 0);
});

// ============================================================================
// Test 7: Full Boundary Guards & cap.agentmem Facade
// ============================================================================
test("skillBank: boundary guards reject invalid dims, zero/NaN vectors, dup IDs & cap.agentmem facade works", () => {
    expect_true(SkillBank.init(0) == null);
    expect_true(SkillBank.init(SKILL_BANK_MAX_DIM + 1) == null);

    let bank = cap.agentmem.SkillBank.init(4) ?? return;
    const good: [4]f32 = [1.0, 2.0, 3.0, 4.0];
    const zero: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    const nan_vec: [4]f32 = [std.math.nan(f32), 1.0, 0.0, 0.0];
    const short_vec: [2]f32 = [1.0, 0.0];

    expect_true(!bank.registerSkill(0, good[0..], 0, false, 100));
    expect_true(!bank.registerSkill(1, zero[0..], 0, false, 100));
    expect_true(!bank.registerSkill(1, nan_vec[0..], 0, false, 100));
    expect_true(!bank.registerSkill(1, short_vec[0..], 0, false, 100));

    expect_true(bank.registerSkill(1, good[0..], 0b0010, false, 100));
    expect_true(!bank.registerSkill(1, good[0..], 0b0010, false, 100)); // duplicate ID

    // Invalid execution feedback
    expect_true(!bank.recordExecution(0, true, 10.0, 200));
    expect_true(!bank.recordExecution(999, true, 10.0, 200));
    expect_true(!bank.recordExecution(1, true, -1.0, 200));
    expect_true(!bank.recordExecution(1, true, std.math.nan(f32), 200));

    // Invalid prune bps (> 10000) returns 0 safely
    expect_eq(bank.pruneUnreliable(0, SKILL_BANK_MAX_BPS + 1), 0);

    // Invalid query vector returns 0 safely
    let hits: [2]SkillRecallHit = [SkillRecallHit{}; 2];
    let hits_s: []SkillRecallHit = hits[0..];
    expect_eq(bank.retrieveSkills(zero[0..], 0b0010, SkillRetrievalConfig.defaults(), *hits_s), 0);
    expect_eq(bank.retrieveSkills(good[0..], 0b0010, SkillRetrievalConfig.defaults(), *hits_s), 1);
});
