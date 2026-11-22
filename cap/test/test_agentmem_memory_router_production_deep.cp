// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/memory_router.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Multi-Tier Memory Fusion Router:
//   1. Intent preset switching: recent_context favors working/episodic vs factual_lookup favors graph/semantic
//   2. Multi-store consensus RRF boost overtakes single-store rank-0 & sets exact source_mask
//   3. Pinned memory locking under tight token budget
//   4. Hard token budget cap & greedy skip of oversized candidates to fit smaller items
//   5. Deterministic tie-break by memory_id ascending
//   6. Same-store duplicate candidate protection (keeps best rank without double-counting)
//   7. Empty input, invalid weights (zero/neg/NaN/bad rrf_k), undersized out_picks & >64 cap guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MEM_ROUTER_MAX_CANDIDATES,
    MemorySourceStore,
    MemoryQueryIntent,
    MemoryTierWeights,
    MemoryRecallCandidate,
    FusedMemoryPick,
    MemoryRouteSummary,
    fuseAndPackMemoryRecalls,
} from "cap:agentmem";

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

// ============================================================================
// Test 1: Intent Weight Presets (recent_context vs factual_lookup)
// ============================================================================

fn testIntentPresetsSwitchRanking()! {
    const cands: [4]MemoryRecallCandidate = [
        MemoryRecallCandidate{ memory_id: 1, source: MemorySourceStore.working,      rank_in_source: 0, token_cost: 50, pinned: false },
        MemoryRecallCandidate{ memory_id: 2, source: MemorySourceStore.episodic,     rank_in_source: 0, token_cost: 50, pinned: false },
        MemoryRecallCandidate{ memory_id: 3, source: MemorySourceStore.semantic,     rank_in_source: 0, token_cost: 50, pinned: false },
        MemoryRecallCandidate{ memory_id: 4, source: MemorySourceStore.entity_graph, rank_in_source: 0, token_cost: 50, pinned: false },
    ];

    let picks: [4]FusedMemoryPick = [FusedMemoryPick{}; 4];
    let picks_s: []FusedMemoryPick = picks[0..];

    // recent_context: working (2.0) > episodic (1.5) > semantic (0.6) > entity_graph (0.4)
    const sum_recent: MemoryRouteSummary = cap.agentmem.fuseAndPackMemoryRecalls(
        cands[0..],
        MemoryTierWeights.forIntent(MemoryQueryIntent.recent_context),
        500,
        picks_s,
    );
    expect_eq(sum_recent.selected_count, 4);
    expect_eq(picks[0].memory_id, 1);
    expect_eq(picks[1].memory_id, 2);
    expect_eq(picks[2].memory_id, 3);
    expect_eq(picks[3].memory_id, 4);

    // factual_lookup: entity_graph (2.0) > semantic (1.6) > episodic (0.6) > working (0.5)
    const sum_fact = fuseAndPackMemoryRecalls(
        cands[0..],
        MemoryTierWeights.forIntent(MemoryQueryIntent.factual_lookup),
        500,
        picks_s,
    );
    expect_eq(sum_fact.selected_count, 4);
    expect_eq(picks[0].memory_id, 4);
    expect_eq(picks[1].memory_id, 3);
    expect_eq(picks[2].memory_id, 2);
    expect_eq(picks[3].memory_id, 1);
}

// ============================================================================
// Test 2: Multi-Store Consensus RRF Boost & Exact source_mask Bitset
// ============================================================================

fn testMultiStoreConsensusBoostAndMask()! {
    // Under equal weights (1.0 each, rrf_k=60):
    //   mem 100 (working rank 0 only): 1 / 61 ≈ 0.01639
    //   mem 200 (semantic rank 2 + entity_graph rank 1): 1/63 + 1/62 ≈ 0.03200 -> overtakes mem 100!
    const cands: [3]MemoryRecallCandidate = [
        MemoryRecallCandidate{ memory_id: 100, source: MemorySourceStore.working,      rank_in_source: 0, token_cost: 60, pinned: false },
        MemoryRecallCandidate{ memory_id: 200, source: MemorySourceStore.semantic,     rank_in_source: 2, token_cost: 70, pinned: false },
        MemoryRecallCandidate{ memory_id: 200, source: MemorySourceStore.entity_graph, rank_in_source: 1, token_cost: 70, pinned: false },
    ];
    let picks: [3]FusedMemoryPick = [FusedMemoryPick{}; 3];
    let picks_s: []FusedMemoryPick = picks[0..];

    const summary = fuseAndPackMemoryRecalls(
        cands[0..],
        MemoryTierWeights.forIntent(MemoryQueryIntent.custom_weights),
        200,
        picks_s,
    );
    expect_eq(summary.input_candidates, 3);
    expect_eq(summary.unique_memories, 2);
    expect_eq(summary.selected_count, 2);
    expect_eq(summary.total_tokens, 130);
    expect_eq(summary.multi_store_consensus_count, 1);

    expect_eq(picks[0].memory_id, 200);
    expect_eq(picks[0].source_mask, 12); // bit2 (semantic=4) | bit3 (entity_graph=8) == 12
    expect_eq(picks[0].fused_score_f32 > picks[1].fused_score_f32, true);
    expect_eq(picks[1].memory_id, 100);
    expect_eq(picks[1].source_mask, 1); // bit0 (working=1)
}

// ============================================================================
// Test 3: Pinned Memory Locking Under Tight Token Budget
// ============================================================================

fn testPinnedMemoryLocking()! {
    const cands: [3]MemoryRecallCandidate = [
        MemoryRecallCandidate{ memory_id: 10, source: MemorySourceStore.working,  rank_in_source: 0,  token_cost: 100, pinned: false },
        MemoryRecallCandidate{ memory_id: 20, source: MemorySourceStore.semantic, rank_in_source: 50, token_cost: 80,  pinned: true },
        MemoryRecallCandidate{ memory_id: 30, source: MemorySourceStore.episodic, rank_in_source: 1,  token_cost: 40,  pinned: false },
    ];
    // Budget = 120 -> pinned mem 20 (80) locked first, mem 10 (100) skipped, mem 30 (40) fits!
    let picks: [3]FusedMemoryPick = [FusedMemoryPick{}; 3];
    let picks_s: []FusedMemoryPick = picks[0..];

    const summary = fuseAndPackMemoryRecalls(
        cands[0..],
        MemoryTierWeights.forIntent(MemoryQueryIntent.custom_weights),
        120,
        picks_s,
    );
    expect_eq(summary.selected_count, 2);
    expect_eq(summary.total_tokens, 120);
    expect_eq(picks[0].memory_id, 20);
    expect_eq(picks[0].pinned, true);
    expect_eq(picks[1].memory_id, 30);
    expect_eq(picks[1].pinned, false);
}

// ============================================================================
// Test 4: Hard Token Budget Cap & Greedy Skip of Oversized Candidates
// ============================================================================

fn testGreedyTokenBudgetPacking()! {
    const cands: [4]MemoryRecallCandidate = [
        MemoryRecallCandidate{ memory_id: 1, source: MemorySourceStore.working, rank_in_source: 0, token_cost: 60,  pinned: false },
        MemoryRecallCandidate{ memory_id: 2, source: MemorySourceStore.working, rank_in_source: 1, token_cost: 200, pinned: false }, // oversized
        MemoryRecallCandidate{ memory_id: 3, source: MemorySourceStore.working, rank_in_source: 2, token_cost: 35,  pinned: false },
        MemoryRecallCandidate{ memory_id: 4, source: MemorySourceStore.working, rank_in_source: 3, token_cost: 20,  pinned: false }, // exceeds remaining 5
    ];
    let picks: [4]FusedMemoryPick = [FusedMemoryPick{}; 4];
    let picks_s: []FusedMemoryPick = picks[0..];

    const summary = fuseAndPackMemoryRecalls(
        cands[0..],
        MemoryTierWeights.forIntent(MemoryQueryIntent.custom_weights),
        100,
        picks_s,
    );
    expect_eq(summary.selected_count, 2);
    expect_eq(summary.total_tokens, 95);
    expect_eq(summary.total_tokens <= 100, true);
    expect_eq(picks[0].memory_id, 1);
    expect_eq(picks[1].memory_id, 3);
}

// ============================================================================
// Test 5: Deterministic Tie-Break by memory_id Ascending
// ============================================================================

fn testDeterministicTieBreakByMemoryId()! {
    // Three memories with identical tier weight & rank -> exact same fused score
    const cands: [3]MemoryRecallCandidate = [
        MemoryRecallCandidate{ memory_id: 300, source: MemorySourceStore.working,  rank_in_source: 0, token_cost: 10, pinned: false },
        MemoryRecallCandidate{ memory_id: 100, source: MemorySourceStore.episodic, rank_in_source: 0, token_cost: 10, pinned: false },
        MemoryRecallCandidate{ memory_id: 200, source: MemorySourceStore.semantic, rank_in_source: 0, token_cost: 10, pinned: false },
    ];
    let picks: [3]FusedMemoryPick = [FusedMemoryPick{}; 3];
    let picks_s: []FusedMemoryPick = picks[0..];

    const summary = fuseAndPackMemoryRecalls(
        cands[0..],
        MemoryTierWeights.forIntent(MemoryQueryIntent.custom_weights),
        100,
        picks_s,
    );
    expect_eq(summary.selected_count, 3);
    expect_eq(picks[0].memory_id, 100);
    expect_eq(picks[1].memory_id, 200);
    expect_eq(picks[2].memory_id, 300);
}

// ============================================================================
// Test 6: Same-Store Duplicate Candidate Protection (No Double-Counting)
// ============================================================================

fn testSameStoreDuplicateNoDoubleCounting()! {
    // mem 42 appears twice from the SAME `semantic` store (rank 5 and rank 0)
    // Must only score once using best rank 0: 1.0 / 61.0, NOT (1/66 + 1/61), and NOT count as multi-store consensus!
    const cands: [2]MemoryRecallCandidate = [
        MemoryRecallCandidate{ memory_id: 42, source: MemorySourceStore.semantic, rank_in_source: 5, token_cost: 30, pinned: false },
        MemoryRecallCandidate{ memory_id: 42, source: MemorySourceStore.semantic, rank_in_source: 0, token_cost: 30, pinned: false },
    ];
    let picks: [2]FusedMemoryPick = [FusedMemoryPick{}; 2];
    let picks_s: []FusedMemoryPick = picks[0..];

    const summary = fuseAndPackMemoryRecalls(
        cands[0..],
        MemoryTierWeights.forIntent(MemoryQueryIntent.custom_weights),
        100,
        picks_s,
    );
    expect_eq(summary.unique_memories, 1);
    expect_eq(summary.selected_count, 1);
    expect_eq(summary.multi_store_consensus_count, 0);
    expect_eq(picks[0].source_mask, 4); // only semantic bit
    const expected_score: f32 = 1.0 / 61.0;
    expect_eq(absF32(picks[0].fused_score_f32 - expected_score) < 1.0e-6, true);
}

// ============================================================================
// Test 7: Empty Input, Invalid Weights, Undersized Output & >64 Cap Guards
// ============================================================================

fn testInvalidAndBoundaryGuards()! {
    const nan_f: f32 = std.math.nan(f32);
    const valid_w = MemoryTierWeights.forIntent(MemoryQueryIntent.deep_synthesis);
    expect_eq(valid_w.isValid(), true);

    // Invalid weights
    expect_eq((MemoryTierWeights{ working_w: 0.0, episodic_w: 0.0, semantic_w: 0.0, graph_w: 0.0, rrf_k: 60.0 }).isValid(), false);
    expect_eq((MemoryTierWeights{ working_w: -1.0 }).isValid(), false);
    expect_eq((MemoryTierWeights{ rrf_k: 0.0 }).isValid(), false);
    expect_eq((MemoryTierWeights{ semantic_w: nan_f }).isValid(), false);

    const one_cand: [1]MemoryRecallCandidate = [
        MemoryRecallCandidate{ memory_id: 1, source: MemorySourceStore.working, rank_in_source: 0, token_cost: 10, pinned: false },
    ];
    let picks1: [1]FusedMemoryPick = [FusedMemoryPick{}; 1];
    let picks1_s: []FusedMemoryPick = picks1[0..];

    // Empty input
    const empty_cands: [0]MemoryRecallCandidate = [];
    const s_empty = fuseAndPackMemoryRecalls(empty_cands[0..], valid_w, 100, picks1_s);
    expect_eq(s_empty.selected_count, 0);

    // Invalid weights rejected
    const s_bad_w = fuseAndPackMemoryRecalls(one_cand[0..], MemoryTierWeights{ rrf_k: 0.0 }, 100, picks1_s);
    expect_eq(s_bad_w.selected_count, 0);

    // Zero-length out_picks slice
    let picks0: [0]FusedMemoryPick = [];
    let picks0_s: []FusedMemoryPick = picks0[0..];
    const s_zero_out = fuseAndPackMemoryRecalls(one_cand[0..], valid_w, 100, picks0_s);
    expect_eq(s_zero_out.selected_count, 0);

    // Exceeding MEM_ROUTER_MAX_CANDIDATES (65)
    const too_many: [MEM_ROUTER_MAX_CANDIDATES + 1]MemoryRecallCandidate = [MemoryRecallCandidate{}; MEM_ROUTER_MAX_CANDIDATES + 1];
    const s_many = fuseAndPackMemoryRecalls(too_many[0..], valid_w, 1000, picks1_s);
    expect_eq(s_many.selected_count, 0);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("agentmem memory_router: recent_context vs factual_lookup intent presets switch tier priority", testIntentPresetsSwitchRanking);
test("agentmem memory_router: multi-store consensus RRF boost overtakes single-store rank-0", testMultiStoreConsensusBoostAndMask);
test("agentmem memory_router: pinned memories lock first under tight token budget", testPinnedMemoryLocking);
test("agentmem memory_router: greedy token budget cap skips oversized items to pack smaller ones", testGreedyTokenBudgetPacking);
test("agentmem memory_router: deterministic tie-break orders equal scores by memory_id ascending", testDeterministicTieBreakByMemoryId);
test("agentmem memory_router: same-store duplicate candidates keep best rank without double-counting", testSameStoreDuplicateNoDoubleCounting);
test("agentmem memory_router: empty input, invalid weights, undersized output & >64 cap guards", testInvalidAndBoundaryGuards);
