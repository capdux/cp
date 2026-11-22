// Origin: Pure-CP Stdlib Test Suite (AI Agent Memory Consolidation, Dedup & Retention Pruning Engine) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agentmem/consolidation.cp
// Covers:
//   1. Duplicate memory merging by content_hash, recall accumulation, max strength/timestamp & pinned inheritance
//   2. Exponential half-life decay accuracy, clock-skew safety & low-strength pruning
//   3. Multi-cycle ShortTerm -> Episodic -> Semantic graduated promotion path & Semantic prune immunity
//   4. Max-age archiving to Archived tier & strict pinned protection against pruning and archiving
//   5. Invalid policy parameters (NaN/Inf/out-of-range/zero half-life), zero side-effects & empty slice safety

import { test, expect_eq } from "cap:test";
import {
    MemConsolidationTier,
    ConsolidatableRecord,
    ConsolidationPolicy,
    ConsolidationReport,
    halfLifeDecayFactor,
    consolidateMemoriesInPlace,
    EpisodeMemoryTier,
    ConsolidationConfig,
    MemoryEpisodeMeta,
    ConsolidationDecisionKind,
    ConsolidationAction,
    ConsolidationSummary,
    computeRetentionScore,
    cosineSimEpisodeF32,
    planMemoryConsolidation,
} from "../../lib/agentmem/index.cp";

fn approxEq(a: f32, b: f32, eps: f32): bool {
    const diff: f32 = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

fn testDuplicateMergingAndPinnedInheritance()! {
    let recs_buf: [5]ConsolidatableRecord = [
        // Duplicate group A (hash 0x1001): earliest, lower strength, not pinned, ShortTerm
        ConsolidatableRecord{
            id: 1,
            content_hash: 0x1001,
            topic_hash: 10,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.40,
            recall_count: 1,
            last_recall_ms: 5000,
            pinned: false,
            alive: true,
        },
        // Duplicate group A: higher strength, newer timestamp, pinned=true, Episodic
        ConsolidatableRecord{
            id: 2,
            content_hash: 0x1001,
            topic_hash: 10,
            tier: MemConsolidationTier.Episodic,
            strength: 0.85,
            recall_count: 2,
            last_recall_ms: 10000,
            pinned: true,
            alive: true,
        },
        // Duplicate group A: third copy adds recall_count
        ConsolidatableRecord{
            id: 3,
            content_hash: 0x1001,
            topic_hash: 10,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.50,
            recall_count: 3,
            last_recall_ms: 7000,
            pinned: false,
            alive: true,
        },
        // Two records with content_hash == 0 must NOT be merged together
        ConsolidatableRecord{
            id: 4,
            content_hash: 0,
            topic_hash: 20,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.70,
            recall_count: 1,
            last_recall_ms: 10000,
            pinned: false,
            alive: true,
        },
        ConsolidatableRecord{
            id: 5,
            content_hash: 0,
            topic_hash: 21,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.70,
            recall_count: 1,
            last_recall_ms: 10000,
            pinned: false,
            alive: true,
        },
    ];

    const policy = ConsolidationPolicy{
        prune_strength_threshold: 0.10,
        promote_min_recalls: 5,
        promote_min_strength: 0.80,
        half_life_ms: 10000,
        max_age_archive_ms: 100000,
    };

    const rep = consolidateMemoriesInPlace(recs_buf[0..], 10000, policy) ?? ConsolidationReport{};
    expect_eq(rep.merged_duplicates, 2);
    expect_eq(rep.promoted_count, 1); // Group A merged to Episodic with recall=6, strength=0.85 -> promoted to Semantic
    expect_eq(rep.pruned_count, 0);
    expect_eq(rep.archived_count, 0);
    expect_eq(rep.surviving_count, 3);

    // Verify surviving merged record id=1 inherited all fields properly
    expect_eq(recs_buf[0].alive, true);
    expect_eq(recs_buf[0].recall_count, 6);
    expect_eq(recs_buf[0].last_recall_ms, 10000);
    expect_eq(approxEq(recs_buf[0].strength, 0.85, 0.001), true);
    expect_eq(recs_buf[0].pinned, true);
    expect_eq(recs_buf[0].tier, MemConsolidationTier.Semantic);

    // Merged duplicates are dead
    expect_eq(recs_buf[1].alive, false);
    expect_eq(recs_buf[2].alive, false);

    // Unhashed records (content_hash == 0) remain separate and alive
    expect_eq(recs_buf[3].alive, true);
    expect_eq(recs_buf[4].alive, true);
}

fn testHalfLifeDecayAndThresholdPruning()! {
    // Direct halfLifeDecayFactor verification
    expect_eq(approxEq(halfLifeDecayFactor(0, 1000), 1.0, 0.0001), true);
    expect_eq(approxEq(halfLifeDecayFactor(1000, 1000), 0.5, 0.001), true);
    expect_eq(approxEq(halfLifeDecayFactor(2000, 1000), 0.25, 0.001), true);
    expect_eq(approxEq(halfLifeDecayFactor(3000, 1000), 0.125, 0.001), true);

    let recs_buf: [3]ConsolidatableRecord = [
        // 1 half-life old: 0.80 * 0.5 = 0.40 (>= 0.20 -> survives)
        ConsolidatableRecord{
            id: 10,
            content_hash: 0xA1,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.80,
            recall_count: 1,
            last_recall_ms: 9000,
            pinned: false,
            alive: true,
        },
        // 3 half-lives old: 0.80 * 0.125 = 0.10 (< 0.20 -> pruned)
        ConsolidatableRecord{
            id: 11,
            content_hash: 0xA2,
            tier: MemConsolidationTier.Episodic,
            strength: 0.80,
            recall_count: 1,
            last_recall_ms: 7000,
            pinned: false,
            alive: true,
        },
        // Clock skew (last_recall_ms > now_ms): age clamps to 0 -> no decay
        ConsolidatableRecord{
            id: 12,
            content_hash: 0xA3,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.50,
            recall_count: 1,
            last_recall_ms: 15000,
            pinned: false,
            alive: true,
        },
    ];

    const policy = ConsolidationPolicy{
        prune_strength_threshold: 0.20,
        promote_min_recalls: 5,
        promote_min_strength: 0.80,
        half_life_ms: 1000,
        max_age_archive_ms: 0,
    };

    const rep = consolidateMemoriesInPlace(recs_buf[0..], 10000, policy) ?? ConsolidationReport{};
    expect_eq(rep.pruned_count, 1);
    expect_eq(rep.surviving_count, 2);

    expect_eq(recs_buf[0].alive, true);
    expect_eq(approxEq(recs_buf[0].strength, 0.40, 0.005), true);

    expect_eq(recs_buf[1].alive, false);
    expect_eq(approxEq(recs_buf[1].strength, 0.10, 0.005), true);

    expect_eq(recs_buf[2].alive, true);
    expect_eq(approxEq(recs_buf[2].strength, 0.50, 0.001), true);
}

fn testGraduatedPromotionPathShortTermToEpisodicToSemantic()! {
    let recs_buf: [2]ConsolidatableRecord = [
        // High recall + high strength ShortTerm record
        ConsolidatableRecord{
            id: 100,
            content_hash: 0xC1,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.90,
            recall_count: 4,
            last_recall_ms: 10000,
            pinned: false,
            alive: true,
        },
        // Low recall ShortTerm record (should NOT promote)
        ConsolidatableRecord{
            id: 101,
            content_hash: 0xC2,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.90,
            recall_count: 1,
            last_recall_ms: 10000,
            pinned: false,
            alive: true,
        },
    ];

    const policy = ConsolidationPolicy{
        prune_strength_threshold: 0.15,
        promote_min_recalls: 3,
        promote_min_strength: 0.65,
        half_life_ms: 10000,
        max_age_archive_ms: 0,
    };

    // Cycle 1 at t=10000: ShortTerm -> Episodic
    const rep1 = consolidateMemoriesInPlace(recs_buf[0..], 10000, policy) ?? ConsolidationReport{};
    expect_eq(rep1.promoted_count, 1);
    expect_eq(recs_buf[0].tier, MemConsolidationTier.Episodic);
    expect_eq(recs_buf[1].tier, MemConsolidationTier.ShortTerm);

    // Cycle 2 at t=11000: strength decays slightly (~0.84 >= 0.65) -> Episodic -> Semantic
    const rep2 = consolidateMemoriesInPlace(recs_buf[0..], 11000, policy) ?? ConsolidationReport{};
    expect_eq(rep2.promoted_count, 1);
    expect_eq(recs_buf[0].tier, MemConsolidationTier.Semantic);
    expect_eq(recs_buf[1].tier, MemConsolidationTier.ShortTerm);

    // Cycle 3 at t=60000 (5 half-lives later): Semantic record is immune to low-strength pruning,
    // while ShortTerm id=101 is pruned.
    const rep3 = consolidateMemoriesInPlace(recs_buf[0..], 60000, policy) ?? ConsolidationReport{};
    expect_eq(rep3.promoted_count, 0);
    expect_eq(rep3.pruned_count, 1);
    expect_eq(rep3.surviving_count, 1);
    expect_eq(recs_buf[0].alive, true);
    expect_eq(recs_buf[0].tier, MemConsolidationTier.Semantic);
    expect_eq(recs_buf[1].alive, false);
}

fn testMaxAgeArchivingAndStrictPinnedProtection()! {
    let recs_buf: [3]ConsolidatableRecord = [
        // Aged Semantic record (age=50000 >= max_age=30000): immune to prune, archived to Archived
        ConsolidatableRecord{
            id: 201,
            content_hash: 0xD1,
            tier: MemConsolidationTier.Semantic,
            strength: 0.80,
            recall_count: 10,
            last_recall_ms: 10000,
            pinned: false,
            alive: true,
        },
        // Aged Episodic record with long half-life so strength remains above prune threshold -> Archived
        ConsolidatableRecord{
            id: 202,
            content_hash: 0xD2,
            tier: MemConsolidationTier.Episodic,
            strength: 0.95,
            recall_count: 1,
            last_recall_ms: 20000,
            pinned: false,
            alive: true,
        },
        // Pinned ShortTerm record with near-zero strength and extreme age -> NEVER pruned or archived
        ConsolidatableRecord{
            id: 203,
            content_hash: 0xD3,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.01,
            recall_count: 1,
            last_recall_ms: 1000,
            pinned: true,
            alive: true,
        },
    ];

    const policy = ConsolidationPolicy{
        prune_strength_threshold: 0.20,
        promote_min_recalls: 5,
        promote_min_strength: 0.80,
        half_life_ms: 100000,
        max_age_archive_ms: 30000,
    };

    const rep = consolidateMemoriesInPlace(recs_buf[0..], 60000, policy) ?? ConsolidationReport{};
    expect_eq(rep.pruned_count, 0);
    expect_eq(rep.archived_count, 2);
    expect_eq(rep.surviving_count, 3);

    expect_eq(recs_buf[0].alive, true);
    expect_eq(recs_buf[0].tier, MemConsolidationTier.Archived);

    expect_eq(recs_buf[1].alive, true);
    expect_eq(recs_buf[1].tier, MemConsolidationTier.Archived);

    // Pinned record survives untouched in ShortTerm tier despite strength < threshold and age > max_age
    expect_eq(recs_buf[2].alive, true);
    expect_eq(recs_buf[2].tier, MemConsolidationTier.ShortTerm);
    expect_eq(recs_buf[2].pinned, true);
}

fn testInvalidPoliciesAndEmptySliceGuards()! {
    const zero_f: f32 = 0.0;
    const one_f: f32 = 1.0;
    const nan_f: f32 = zero_f / zero_f;
    const pos_inf: f32 = one_f / zero_f;

    let recs_buf: [1]ConsolidatableRecord = [
        ConsolidatableRecord{
            id: 1,
            content_hash: 0xE1,
            tier: MemConsolidationTier.ShortTerm,
            strength: 0.50,
            recall_count: 2,
            last_recall_ms: 1000,
            pinned: false,
            alive: true,
        },
    ];

    // 1. half_life_ms == 0 rejected with zero side effects
    let bad_hl = ConsolidationPolicy.default();
    bad_hl.half_life_ms = 0;
    expect_eq(bad_hl.isValid(), false);
    expect_eq(consolidateMemoriesInPlace(recs_buf[0..], 90000, bad_hl) == null, true);

    // 2. promote_min_recalls == 0 rejected
    let bad_rec = ConsolidationPolicy.default();
    bad_rec.promote_min_recalls = 0;
    expect_eq(consolidateMemoriesInPlace(recs_buf[0..], 90000, bad_rec) == null, true);

    // 3. NaN / Inf / negative / >1 thresholds rejected
    let bad_nan = ConsolidationPolicy.default();
    bad_nan.prune_strength_threshold = nan_f;
    expect_eq(consolidateMemoriesInPlace(recs_buf[0..], 90000, bad_nan) == null, true);

    let bad_inf = ConsolidationPolicy.default();
    bad_inf.promote_min_strength = pos_inf;
    expect_eq(consolidateMemoriesInPlace(recs_buf[0..], 90000, bad_inf) == null, true);

    let bad_neg = ConsolidationPolicy.default();
    bad_neg.prune_strength_threshold = -0.1;
    expect_eq(consolidateMemoriesInPlace(recs_buf[0..], 90000, bad_neg) == null, true);

    let bad_over = ConsolidationPolicy.default();
    bad_over.promote_min_strength = 1.5;
    expect_eq(consolidateMemoriesInPlace(recs_buf[0..], 90000, bad_over) == null, true);

    // Record must remain completely untouched after rejected calls
    expect_eq(recs_buf[0].alive, true);
    expect_eq(approxEq(recs_buf[0].strength, 0.50, 0.0001), true);

    // 4. Empty slice returns zeroed report cleanly
    let empty_buf: [0]ConsolidatableRecord = [];
    let empty_slice: []ConsolidatableRecord = empty_buf[0..];
    const empty_rep = consolidateMemoriesInPlace(empty_slice, 10000, ConsolidationPolicy.default()) ?? ConsolidationReport{ surviving_count: 99 };
    expect_eq(empty_rep.merged_duplicates, 0);
    expect_eq(empty_rep.promoted_count, 0);
    expect_eq(empty_rep.archived_count, 0);
    expect_eq(empty_rep.pruned_count, 0);
    expect_eq(empty_rep.surviving_count, 0);
}

test("agentmem consolidation: duplicate merging by content_hash and pinned inheritance", testDuplicateMergingAndPinnedInheritance);
test("agentmem consolidation: exponential half-life decay, clock-skew safety and threshold pruning", testHalfLifeDecayAndThresholdPruning);
test("agentmem consolidation: ShortTerm -> Episodic -> Semantic graduated promotion and prune immunity", testGraduatedPromotionPathShortTermToEpisodicToSemantic);
test("agentmem consolidation: max-age archiving and strict pinned protection", testMaxAgeArchivingAndStrictPinnedProtection);
test("agentmem consolidation: invalid policies, zero side-effects and empty slice guards", testInvalidPoliciesAndEmptySliceGuards);

// ============================================================================
// Part 2 Tests: SIMD Episodic Consolidation Planner (Wave 5 Round 9)
// ============================================================================

fn testRetentionCurveAndAccessFrequencyBoost()! {
    const recent_hi = MemoryEpisodeMeta{
        episode_id: 1, last_access_ms: 10000, access_count: 10, importance_f32: 0.8,
        tier: EpisodeMemoryTier.episodic,
    };
    const recent_lo = MemoryEpisodeMeta{
        episode_id: 2, last_access_ms: 10000, access_count: 1, importance_f32: 0.8,
        tier: EpisodeMemoryTier.episodic,
    };
    const old_ep = MemoryEpisodeMeta{
        episode_id: 3, last_access_ms: 0, access_count: 1, importance_f32: 0.8,
        tier: EpisodeMemoryTier.episodic,
    };

    const s_hi = computeRetentionScore(recent_hi, 10000, 2000);
    const s_lo = computeRetentionScore(recent_lo, 10000, 2000);
    const s_old = computeRetentionScore(old_ep, 10000, 2000); // 5 half-lives -> 1/32 decay

    expect_eq(s_hi > s_lo, true);
    expect_eq(s_lo > s_old, true);
    expect_eq(s_old < 0.05, true);
    expect_eq(computeRetentionScore(recent_hi, 10000, 0), 0.0);
}

fn testSimdCosineDedupAndRepresentativeSelection()! {
    const va: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const vb: [4]f32 = [0.99, 0.05, 0.0, 0.0];
    const vc: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(cosineSimEpisodeF32(va[0..], vb[0..]) > 0.99, true);
    expect_eq(approxEq(cosineSimEpisodeF32(va[0..], vc[0..]), 0.0, 0.0001), true);
    expect_eq(cosineSimEpisodeF32(va[0..2], vb[0..]) == 0.0, true); // mismatched len

    // Episode 0 (lower retention) and Episode 1 (higher retention) are near-duplicates ->
    // Episode 0 must merge into Episode 1 (higher retention representative)!
    const eps: [3]MemoryEpisodeMeta = [
        MemoryEpisodeMeta{ episode_id: 10, last_access_ms: 8000, access_count: 1, importance_f32: 0.4, tier: EpisodeMemoryTier.episodic },
        MemoryEpisodeMeta{ episode_id: 11, last_access_ms: 10000, access_count: 5, importance_f32: 0.9, tier: EpisodeMemoryTier.episodic },
        MemoryEpisodeMeta{ episode_id: 12, last_access_ms: 10000, access_count: 1, importance_f32: 0.7, tier: EpisodeMemoryTier.episodic },
    ];
    const embs: [12]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.99, 0.05, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
    ];
    const cfg = ConsolidationConfig{
        decay_half_life_ms: 5000,
        merge_cosine_threshold: 0.95,
        promote_access_count: 4,
        promote_importance_min: 0.80,
        evict_retention_threshold: 0.15,
        max_retained_entries: 10,
    };
    let actions: [3]ConsolidationAction = [ConsolidationAction{}; 3];
    let actions_s: []ConsolidationAction = actions[0..];
    const sum: ConsolidationSummary = planMemoryConsolidation(eps[0..], embs[0..], 4, 10000, cfg, actions_s);

    expect_eq(sum.total_input, 3);
    expect_eq(sum.merged_count, 1);
    expect_eq(sum.promoted_count, 1);
    expect_eq(sum.kept_count, 1);
    expect_eq(actions[0].kind == ConsolidationDecisionKind.merged_into, true);
    expect_eq(actions[0].target_idx, 1);
    expect_eq(actions[1].kind == ConsolidationDecisionKind.promote_semantic, true);
    expect_eq(actions[2].kind == ConsolidationDecisionKind.keep, true);
}

fn testSemanticPromotionAndDecayEviction()! {
    const eps: [3]MemoryEpisodeMeta = [
        // Qualifying episodic -> promote_semantic
        MemoryEpisodeMeta{ episode_id: 1, last_access_ms: 10000, access_count: 5, importance_f32: 0.85, tier: EpisodeMemoryTier.episodic },
        // Old low-retention working memory -> evict_decay
        MemoryEpisodeMeta{ episode_id: 2, last_access_ms: 0, access_count: 1, importance_f32: 0.2, tier: EpisodeMemoryTier.working },
        // Already semantic_long_term -> immune to decay eviction even if old
        MemoryEpisodeMeta{ episode_id: 3, last_access_ms: 0, access_count: 1, importance_f32: 0.5, tier: EpisodeMemoryTier.semantic_long_term },
    ];
    const embs: [12]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
    ];
    const cfg = ConsolidationConfig{
        decay_half_life_ms: 1000,
        merge_cosine_threshold: 0.95,
        promote_access_count: 3,
        promote_importance_min: 0.70,
        evict_retention_threshold: 0.15,
        max_retained_entries: 10,
    };
    let actions: [3]ConsolidationAction = [ConsolidationAction{}; 3];
    let actions_s: []ConsolidationAction = actions[0..];
    const sum = planMemoryConsolidation(eps[0..], embs[0..], 4, 10000, cfg, actions_s);

    expect_eq(sum.promoted_count, 1);
    expect_eq(sum.evicted_decay_count, 1);
    expect_eq(sum.kept_count, 1);
    expect_eq(actions[0].kind == ConsolidationDecisionKind.promote_semantic, true);
    expect_eq(actions[1].kind == ConsolidationDecisionKind.evict_decay, true);
    expect_eq(actions[2].kind == ConsolidationDecisionKind.keep, true);
}

fn testPinnedProtectionAgainstDecayAndBudget()! {
    const eps: [3]MemoryEpisodeMeta = [
        // Very old pinned memory: must survive BOTH decay and tight budget (max_retained_entries = 1)
        MemoryEpisodeMeta{ episode_id: 1, last_access_ms: 0, access_count: 1, importance_f32: 0.1, tier: EpisodeMemoryTier.working, pinned: true },
        // Second pinned memory: also must NEVER be evicted even if budget is 1
        MemoryEpisodeMeta{ episode_id: 2, last_access_ms: 0, access_count: 1, importance_f32: 0.1, tier: EpisodeMemoryTier.episodic, pinned: true },
        // Fresh unpinned memory: evicted by budget because 2 pinned already exceed budget=1
        MemoryEpisodeMeta{ episode_id: 3, last_access_ms: 10000, access_count: 2, importance_f32: 0.9, tier: EpisodeMemoryTier.working, pinned: false },
    ];
    const embs: [12]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
    ];
    const cfg = ConsolidationConfig{
        decay_half_life_ms: 1000,
        merge_cosine_threshold: 0.95,
        promote_access_count: 5,
        promote_importance_min: 0.9,
        evict_retention_threshold: 0.5,
        max_retained_entries: 1,
    };
    let actions: [3]ConsolidationAction = [ConsolidationAction{}; 3];
    let actions_s: []ConsolidationAction = actions[0..];
    const sum = planMemoryConsolidation(eps[0..], embs[0..], 4, 10000, cfg, actions_s);

    expect_eq(sum.kept_count, 2);
    expect_eq(sum.evicted_budget_count, 1);
    expect_eq(sum.evicted_decay_count, 0);
    expect_eq(actions[0].kind == ConsolidationDecisionKind.keep, true);
    expect_eq(actions[1].kind == ConsolidationDecisionKind.keep, true);
    expect_eq(actions[2].kind == ConsolidationDecisionKind.evict_budget, true);
}

fn testBudgetPruningByLowestRetentionScore()! {
    const eps: [4]MemoryEpisodeMeta = [
        MemoryEpisodeMeta{ episode_id: 1, last_access_ms: 10000, access_count: 1, importance_f32: 0.2, tier: EpisodeMemoryTier.working },
        MemoryEpisodeMeta{ episode_id: 2, last_access_ms: 10000, access_count: 8, importance_f32: 0.95, tier: EpisodeMemoryTier.working },
        MemoryEpisodeMeta{ episode_id: 3, last_access_ms: 10000, access_count: 1, importance_f32: 0.5, tier: EpisodeMemoryTier.working },
        MemoryEpisodeMeta{ episode_id: 4, last_access_ms: 10000, access_count: 5, importance_f32: 0.8, tier: EpisodeMemoryTier.working },
    ];
    const embs: [16]f32 = [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    ];
    const cfg = ConsolidationConfig{
        decay_half_life_ms: 10000,
        merge_cosine_threshold: 0.95,
        promote_access_count: 100,
        promote_importance_min: 0.99,
        evict_retention_threshold: 0.05,
        max_retained_entries: 2, // Keep top-2 (indices 1 & 3), evict bottom-2 (indices 0 & 2)
    };
    let actions: [4]ConsolidationAction = [ConsolidationAction{}; 4];
    let actions_s: []ConsolidationAction = actions[0..];
    const sum = planMemoryConsolidation(eps[0..], embs[0..], 4, 10000, cfg, actions_s);

    expect_eq(sum.kept_count, 2);
    expect_eq(sum.evicted_budget_count, 2);
    expect_eq(actions[0].kind == ConsolidationDecisionKind.evict_budget, true);
    expect_eq(actions[1].kind == ConsolidationDecisionKind.keep, true);
    expect_eq(actions[2].kind == ConsolidationDecisionKind.evict_budget, true);
    expect_eq(actions[3].kind == ConsolidationDecisionKind.keep, true);
}

fn testPlanConsolidationBoundaryGuards()! {
    const eps: [2]MemoryEpisodeMeta = [MemoryEpisodeMeta{}, MemoryEpisodeMeta{}];
    const embs: [8]f32 = [1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0];
    let actions: [2]ConsolidationAction = [ConsolidationAction{}; 2];
    let actions_s: []ConsolidationAction = actions[0..];
    const ok_cfg = ConsolidationConfig{};

    // Invalid configs
    expect_eq((ConsolidationConfig{ decay_half_life_ms: 0 }).isValid(), false);
    expect_eq((ConsolidationConfig{ merge_cosine_threshold: 0.0 }).isValid(), false);
    expect_eq((ConsolidationConfig{ merge_cosine_threshold: 1.5 }).isValid(), false);
    expect_eq((ConsolidationConfig{ max_retained_entries: 0 }).isValid(), false);

    // Empty input, zero dim, truncated embeddings, undersized out_actions all return zeroed summary
    expect_eq(planMemoryConsolidation(eps[0..0], embs[0..], 4, 1000, ok_cfg, actions_s).total_input, 0);
    expect_eq(planMemoryConsolidation(eps[0..], embs[0..], 0, 1000, ok_cfg, actions_s).total_input, 0);
    expect_eq(planMemoryConsolidation(eps[0..], embs[0..4], 4, 1000, ok_cfg, actions_s).total_input, 0);
    expect_eq(planMemoryConsolidation(eps[0..], embs[0..], 4, 1000, ok_cfg, actions_s[0..1]).total_input, 0);
    expect_eq(planMemoryConsolidation(eps[0..], embs[0..], 4, 1000, ConsolidationConfig{ decay_half_life_ms: 0 }, actions_s).total_input, 0);
}

test("agentmem consolidation: retention decay curve & access frequency boost", testRetentionCurveAndAccessFrequencyBoost);
test("agentmem consolidation: @dot SIMD near-duplicate clustering & representative selection", testSimdCosineDedupAndRepresentativeSelection);
test("agentmem consolidation: semantic promotion & decay eviction immunity", testSemanticPromotionAndDecayEviction);
test("agentmem consolidation: strict pinned protection against decay & budget", testPinnedProtectionAgainstDecayAndBudget);
test("agentmem consolidation: budget pruning by lowest retention score", testBudgetPruningByLowestRetentionScore);
test("agentmem consolidation: planMemoryConsolidation boundary & invalid config guards", testPlanConsolidationBoundaryGuards);
