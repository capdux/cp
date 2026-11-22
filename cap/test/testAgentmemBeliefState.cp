// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/beliefState.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Agent Belief State Revision & Contradiction Resolver:
//   1. Log-odds Bayesian reinforcement monotonicity, neutral-trust invariance & multi-observation update
//   2. Three-way contradiction arbitration: superseded_old (lineage), retained_old & marked_contested
//   3. Exponential half-life decay math, staleness-driven belief replacement & applyDecay() sweep
//   4. Contested tie-breaking via corroboration + retract() automatic survivor promotion
//   5. Epistemic Shannon entropy, contradiction ratio telemetry & zero-heap conflict audit log
//   6. Subject-level active/history queries, table-full guard, invalid input guards & cap.agentmem facade

const std = @import("std");
import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    BELIEF_MAX_RECORDS,
    BELIEF_MAX_AUDIT_ENTRIES,
    BELIEF_NO_ID,
    beliefLogOddsReinforce,
    beliefHalfLifeDecay,
    beliefEffectiveWeight,
    BeliefStatus,
    BeliefRevisionAction,
    BeliefRevisionConfig,
    BeliefRecord,
    BeliefRevisionResult,
    BeliefConflictAudit,
    BeliefStateStats,
    BeliefStateStore,
} from "cap:agentmem";

fn absF32(x: f32): f32 {
    return if (x < 0.0) (0.0 - x) else x;
}

test("BeliefState: log-odds reinforcement math & corroborating observations") {
    // Positive evidence with high trust monotonically increases confidence without exceeding 1.0
    const r1: f32 = beliefLogOddsReinforce(0.60, 0.80, 0.90);
    expect_true(r1 > 0.60 && r1 < 1.0);
    const r2: f32 = beliefLogOddsReinforce(r1, 0.85, 0.95);
    expect_true(r2 > r1 && r2 < 1.0);

    // Zero trust or neutral evidence (0.5) leaves prior unchanged
    const r_neutral_trust: f32 = beliefLogOddsReinforce(0.70, 0.95, 0.0);
    expect_true(absF32(r_neutral_trust - 0.70) < 0.0001);
    const r_neutral_ev: f32 = beliefLogOddsReinforce(0.70, 0.50, 1.0);
    expect_true(absF32(r_neutral_ev - 0.70) < 0.0001);

    // End-to-end store reinforcement
    let store = BeliefStateStore.init(BeliefRevisionConfig.defaults()) orelse {
        expect_true(false);
        return;
    };
    const ins = store.observe(100, 1, 0xAAAA, 0.65, 0.80, 1000);
    expect_eq(ins.action, BeliefRevisionAction.inserted);
    expect_eq(ins.belief_id, 1 as u32);

    const reinf = store.observe(100, 1, 0xAAAA, 0.85, 0.90, 2000);
    expect_eq(reinf.action, BeliefRevisionAction.reinforced);
    expect_eq(reinf.belief_id, 1 as u32);
    expect_true(reinf.new_confidence > 0.85 && reinf.new_confidence < 1.0);

    const best = store.queryBestBelief(100, 1) orelse {
        expect_true(false);
        return;
    };
    expect_eq(best.observation_count, 2 as u32);
    expect_eq(best.status, BeliefStatus.active);
    expect_eq(store.stats().reinforce_events, 1 as u64);
}

test("BeliefState: three-way contradiction arbitration (supersede / retain / contest)") {
    let cfg = BeliefRevisionConfig.defaults();
    cfg.contest_margin = 0.12;
    let store = BeliefStateStore.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // 1. Weak incumbent superseded by strong incoming evidence -> superseded_old + lineage
    const b1 = store.observe(1, 10, 0x1111, 0.55, 0.40, 1000);
    expect_eq(b1.action, BeliefRevisionAction.inserted);

    const b2 = store.observe(1, 10, 0x2222, 0.92, 0.95, 1100);
    expect_eq(b2.action, BeliefRevisionAction.superseded_old);
    expect_eq(b2.conflict_with_id, b1.belief_id);

    const old_rec = store.getBelief(b1.belief_id) orelse { expect_true(false); return; };
    expect_eq(old_rec.status, BeliefStatus.superseded);
    expect_eq(old_rec.superseded_by, b2.belief_id);

    const active_now = store.queryBestBelief(1, 10) orelse { expect_true(false); return; };
    expect_eq(active_now.belief_id, b2.belief_id);
    expect_eq(active_now.object_hash, 0x2222 as u64);

    // 2. Weak incoming rumor cannot displace strong incumbent -> retained_old
    const b3 = store.observe(1, 10, 0x3333, 0.40, 0.25, 1200);
    expect_eq(b3.action, BeliefRevisionAction.retained_old);
    expect_eq(b3.belief_id, b2.belief_id); // incumbent remains winner
    const still_active = store.queryBestBelief(1, 10) orelse { expect_true(false); return; };
    expect_eq(still_active.belief_id, b2.belief_id);
    expect_eq(still_active.status, BeliefStatus.active);

    // 3. Closely matched rival -> marked_contested (both candidates flagged contested)
    const c1 = store.observe(2, 20, 0xAAAA, 0.80, 0.80, 2000);
    expect_eq(c1.action, BeliefRevisionAction.inserted);
    const c2 = store.observe(2, 20, 0xBBBB, 0.78, 0.82, 2100);
    expect_eq(c2.action, BeliefRevisionAction.marked_contested);

    let contested_buf: [4]BeliefRecord = [BeliefRecord{}; 4];
    let contested_s: []BeliefRecord = contested_buf[0..];
    const n_c: usize = store.queryContested(2, 20, *contested_s);
    expect_eq(n_c, 2 as usize);
    expect_eq(contested_buf[0].status, BeliefStatus.contested);
    expect_eq(contested_buf[1].status, BeliefStatus.contested);
}

test("BeliefState: half-life time decay enables fresh evidence to replace stale beliefs") {
    // Exact half-life math: after 1 half-life confidence halves; after huge time floors at min_confidence
    expect_true(absF32(beliefHalfLifeDecay(0.80, 1000, 1000, 0.05) - 0.40) < 0.0001);
    expect_true(absF32(beliefHalfLifeDecay(0.80, 2000, 1000, 0.05) - 0.20) < 0.0001);
    expect_true(absF32(beliefHalfLifeDecay(0.80, 100000, 1000, 0.05) - 0.05) < 0.00001);

    let cfg = BeliefRevisionConfig.defaults();
    cfg.half_life_ms = 1000;
    cfg.contest_margin = 0.10;
    cfg.min_confidence = 0.05;

    let store = BeliefStateStore.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // t=0: Very strong initial belief (0.95, trust=0.95)
    const old_b = store.observe(7, 1, 0x01, 0.95, 0.95, 0);
    expect_eq(old_b.action, BeliefRevisionAction.inserted);

    // t=3000 (3 half-lives -> decayed conf ~= 0.95 / 8 = 0.11875):
    // Moderate fresh evidence (0.75, trust=0.80) now decisively supersedes stale belief!
    const fresh_b = store.observe(7, 1, 0x02, 0.75, 0.80, 3000);
    expect_eq(fresh_b.action, BeliefRevisionAction.superseded_old);
    expect_true(fresh_b.incumbent_effective_weight < 0.15);
    expect_true(fresh_b.incoming_effective_weight > 0.60);

    // applyDecay sweep at t=4000 (1 half-life after t=3000) halves fresh_b's confidence
    const swept: usize = store.applyDecay(4000);
    expect_eq(swept, 1 as usize);
    const after_decay = store.queryBestBelief(7, 1) orelse { expect_true(false); return; };
    expect_true(absF32(after_decay.confidence - 0.375) < 0.001);
}

test("BeliefState: corroboration breaks contested tie & retract promotes sole survivor") {
    let cfg = BeliefRevisionConfig.defaults();
    cfg.contest_margin = 0.15;
    let store = BeliefStateStore.init(cfg) orelse {
        expect_true(false);
        return;
    };

    // Scenario A: Corroboration breaks contested tie
    const a1 = store.observe(10, 1, 0x10, 0.72, 0.80, 1000);
    const a2 = store.observe(10, 1, 0x20, 0.74, 0.80, 1050);
    expect_eq(a2.action, BeliefRevisionAction.marked_contested);

    // Additional corroborating high-trust evidence for 0x10 resolves the tie automatically!
    const tie_break = store.observe(10, 1, 0x10, 0.92, 0.95, 1100);
    expect_eq(tie_break.action, BeliefRevisionAction.reinforced);

    const winner = store.getBelief(a1.belief_id) orelse { expect_true(false); return; };
    const loser = store.getBelief(a2.belief_id) orelse { expect_true(false); return; };
    expect_eq(winner.status, BeliefStatus.active);
    expect_eq(loser.status, BeliefStatus.superseded);
    expect_eq(loser.superseded_by, a1.belief_id);

    // Scenario B: Retracting one of two contested beliefs auto-promotes survivor to active
    const b1 = store.observe(20, 2, 0xAA, 0.75, 0.80, 2000);
    const b2 = store.observe(20, 2, 0xBB, 0.75, 0.80, 2010);
    expect_eq(b2.action, BeliefRevisionAction.marked_contested);

    expect_true(store.retract(b1.belief_id, 2100));
    expect_eq(store.retract(b1.belief_id, 2200), false); // double retract rejected

    const survivor = store.getBelief(b2.belief_id) orelse { expect_true(false); return; };
    expect_eq(survivor.status, BeliefStatus.active);

    // Retracting survivor leaves no live belief
    expect_true(store.retract(b2.belief_id, 2300));
    expect_true(store.queryBestBelief(20, 2) == null);
}

test("BeliefState: epistemic entropy, contradiction ratio & zero-heap conflict audit log") {
    let store = BeliefStateStore.init(BeliefRevisionConfig.defaults()) orelse {
        expect_true(false);
        return;
    };

    // High-certainty belief -> low epistemic entropy, 0 contradiction ratio
    * = store.observe(1, 1, 0x01, 0.98, 0.95, 1000);
    const st_certain = store.stats();
    expect_eq(st_certain.active_count, 1 as usize);
    expect_eq(st_certain.contested_count, 0 as usize);
    expect_true(st_certain.epistemic_entropy < 0.12);
    expect_true(absF32(st_certain.contradiction_ratio) < 0.000001);

    // Add a maximally uncertain (0.50) contested pair -> entropy rises, contradiction_ratio = 2/3
    * = store.observe(2, 1, 0x0A, 0.50, 0.80, 2000);
    * = store.observe(2, 1, 0x0B, 0.50, 0.80, 2050);
    const st_uncertain = store.stats();
    expect_eq(st_uncertain.active_count, 1 as usize);
    expect_eq(st_uncertain.contested_count, 2 as usize);
    expect_eq(st_uncertain.conflict_events, 1 as u64);
    expect_true(st_uncertain.epistemic_entropy > 0.45);
    expect_true(absF32(st_uncertain.contradiction_ratio - (2.0 / 3.0)) < 0.0001);

    // Trigger another conflict (supersede) and inspect chronological audit log
    * = store.observe(2, 1, 0x0C, 0.96, 0.98, 2100);
    let audits: [8]BeliefConflictAudit = [BeliefConflictAudit{}; 8];
    let audits_s: []BeliefConflictAudit = audits[0..];
    const n_aud: usize = store.recentConflicts(*audits_s);
    expect_eq(n_aud, 2 as usize);
    expect_eq(audits[0].action, BeliefRevisionAction.marked_contested);
    expect_eq(audits[0].timestamp_ms, 2050 as u64);
    expect_eq(audits[1].action, BeliefRevisionAction.superseded_old);
    expect_eq(audits[1].timestamp_ms, 2100 as u64);
    expect_eq(audits[1].incoming_object_hash, 0x0C as u64);
}

test("BeliefState: subject queries, capacity limit, malformed guards & cap.agentmem facade") {
    // Invalid configs rejected
    let bad_cfg1 = BeliefRevisionConfig.defaults();
    bad_cfg1.contest_margin = -0.1;
    expect_true(BeliefStateStore.init(bad_cfg1) == null);

    let bad_cfg2 = BeliefRevisionConfig.defaults();
    bad_cfg2.min_confidence = 1.5;
    expect_true(BeliefStateStore.init(bad_cfg2) == null);

    let bad_cfg3 = BeliefRevisionConfig.defaults();
    bad_cfg3.contest_margin = std.math.nan(f32);
    expect_true(BeliefStateStore.init(bad_cfg3) == null);

    let store = BeliefStateStore.init(BeliefRevisionConfig.defaults()) orelse {
        expect_true(false);
        return;
    };

    // Invalid observe inputs rejected cleanly
    expect_eq(store.observe(0, 1, 0x1, 0.8, 0.8, 100).action, BeliefRevisionAction.rejected_invalid);
    expect_eq(store.observe(1, 0, 0x1, 0.8, 0.8, 100).action, BeliefRevisionAction.rejected_invalid);
    expect_eq(store.observe(1, 1, 0x1, 1.2, 0.8, 100).action, BeliefRevisionAction.rejected_invalid);
    expect_eq(store.observe(1, 1, 0x1, 0.8, -0.2, 100).action, BeliefRevisionAction.rejected_invalid);
    expect_eq(store.observe(1, 1, 0x1, std.math.nan(f32), 0.8, 100).action, BeliefRevisionAction.rejected_invalid);
    expect_eq(store.retract(BELIEF_NO_ID, 100), false);
    expect_eq(store.retract(9999, 100), false);

    // Subject queries (active vs include_inactive)
    * = store.observe(42, 1, 0x1, 0.50, 0.40, 100);
    * = store.observe(42, 1, 0x2, 0.95, 0.95, 200); // supersedes 0x1
    * = store.observe(42, 2, 0x3, 0.80, 0.85, 300);

    let buf: [8]BeliefRecord = [BeliefRecord{}; 8];
    let buf_s: []BeliefRecord = buf[0..];
    expect_eq(store.queryBySubject(42, false, *buf_s), 2 as usize);
    expect_eq(store.queryBySubject(42, true, *buf_s), 3 as usize);

    // Clear and fill to BELIEF_MAX_RECORDS -> 129th returns rejected_full
    store.clear();
    expect_eq(store.stats().total_records, 0 as usize);
    let idx: usize = 0;
    while (idx < BELIEF_MAX_RECORDS) : (idx += 1) {
        const res = store.observe((idx + 1) as u64, 1, idx as u64, 0.8, 0.8, 1000);
        expect_eq(res.action, BeliefRevisionAction.inserted);
    }
    const overflow = store.observe(999, 1, 0xFF, 0.8, 0.8, 1000);
    expect_eq(overflow.action, BeliefRevisionAction.rejected_full);

    // Verify cap.agentmem namespace facade parity
    expect_eq(cap.agentmem.BELIEF_MAX_RECORDS, BELIEF_MAX_RECORDS);
    expect_eq(cap.agentmem.BELIEF_MAX_AUDIT_ENTRIES, BELIEF_MAX_AUDIT_ENTRIES);
    const w_fac: f32 = cap.agentmem.beliefEffectiveWeight(0.80, 1.0, 0, 1000, 0.05);
    expect_true(absF32(w_fac - beliefEffectiveWeight(0.80, 1.0, 0, 1000, 0.05)) < 0.000001);
}
