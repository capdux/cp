// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/episode_index.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Hybrid Temporal-Entity-Vector Episodic Index:
//   1. Tri-modal hybrid retrieval: @dot cosine + @popCount entity overlap + recency decay + breakdown verification
//   2. Entity bitset tri-filtering (required_entities, any_entities, excluded_entities)
//   3. Temporal window bounds [min_ts_ms, max_ts_ms], invalid range (min > max) & session isolation
//   4. Top-K truncation & deterministic tie-breaking (score desc, timestamp desc, id asc)
//   5. Chronological session timeline replay (`scanSessionTimeline`) with out-of-order inserts
//   6. Watermark pruning (`pruneBeforeTimestamp`) with `pinned` protection & dynamic unpin
//   7. Full pool capacity guard (`EP_INDEX_MAX_SLOTS = 128`) & slot reclamation after prune
//   8. Boundary guards: invalid dim, empty index, zero/NaN vector, duplicate ID & invalid weights

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    EP_INDEX_MAX_SLOTS,
    EP_INDEX_MAX_DIM,
    EpisodeRecord,
    EpisodeQueryFilter,
    EpisodeHybridWeights,
    EpisodeHybridHit,
    EpisodeIndexStats,
    HybridEpisodeIndex,
} from "cap:agentmem";

fn absF32(x: f32): f32 {
    return if (x < 0.0) -x else x;
}

// ============================================================================
// Test 1: Tri-Modal Hybrid Retrieval & Per-Modality Score Breakdown
// ============================================================================

fn testTriModalHybridScoringBreakdown()! {
    let idx = cap.agentmem.HybridEpisodeIndex.init(4) ?? return;

    const ENT_USER: u64 = 1 << 0;
    const ENT_TOOL: u64 = 1 << 1;
    const ENT_DB: u64 = 1 << 2;
    const ENT_NET: u64 = 1 << 3;

    // Episode 1: exact vector match [1,0,0,0], recent (t=10000), 3/3 entity overlap, high importance 0.9
    const v1: [4]f32 = [2.0, 0.0, 0.0, 0.0]; // auto L2-normalized to [1,0,0,0]
    expect_eq(idx.insertEpisode(1, 10_000, 100, ENT_USER | ENT_TOOL | ENT_DB, 0.9, false, v1[0..]), true);

    // Episode 2: exact vector match, exactly 1 half-life older (t=6400 vs now=10000, hl=3600 -> recency=0.5),
    //            2/3 entity overlap, importance 0.6
    const v2: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    expect_eq(idx.insertEpisode(2, 6_400, 100, ENT_USER | ENT_DB, 0.6, false, v2[0..]), true);

    // Episode 3: orthogonal vector [0,1,0,0], very recent, 1/3 entity overlap, importance 0.2
    const v3: [4]f32 = [0.0, 3.0, 0.0, 0.0];
    expect_eq(idx.insertEpisode(3, 10_000, 100, ENT_USER | ENT_NET, 0.2, false, v3[0..]), true);

    const q: [4]f32 = [5.0, 0.0, 0.0, 0.0];
    const filter = EpisodeQueryFilter{
        session_id: 0,
        min_ts_ms: 0,
        max_ts_ms: 20_000,
        required_entities: 0,
        any_entities: 0,
        excluded_entities: 0,
        target_entities: ENT_USER | ENT_TOOL | ENT_DB,
    };
    const weights = EpisodeHybridWeights{
        vector_weight: 0.50,
        recency_weight: 0.20,
        entity_weight: 0.20,
        importance_weight: 0.10,
        recency_half_life_ms: 3_600,
    };

    let hits: [4]EpisodeHybridHit = [EpisodeHybridHit{}; 4];
    let hits_s: []EpisodeHybridHit = hits[0..];
    const n = idx.queryHybrid(q[0..], filter, weights, 10_000, *hits_s);
    expect_eq(n, 3);

    // Strict descending order: ep 1 > ep 2 > ep 3
    expect_eq(hits[0].episode_id, 1);
    expect_eq(hits[1].episode_id, 2);
    expect_eq(hits[2].episode_id, 3);

    // Verify exact breakdown for ep 1: 0.5*1.0 + 0.2*1.0 + 0.2*1.0 + 0.1*0.9 = 0.99
    expect_eq(absF32(hits[0].vector_score - 1.0) < 1.0e-5, true);
    expect_eq(absF32(hits[0].recency_score - 1.0) < 1.0e-5, true);
    expect_eq(absF32(hits[0].entity_score - 1.0) < 1.0e-5, true);
    expect_eq(absF32(hits[0].importance_score - 0.9) < 1.0e-5, true);
    expect_eq(absF32(hits[0].total_score - 0.99) < 1.0e-4, true);

    // Verify exact breakdown for ep 2: recency == 0.5, entity == 2/3
    expect_eq(absF32(hits[1].vector_score - 1.0) < 1.0e-5, true);
    expect_eq(absF32(hits[1].recency_score - 0.5) < 1.0e-4, true);
    expect_eq(absF32(hits[1].entity_score - (2.0 / 3.0)) < 1.0e-4, true);

    // Verify ep 3 orthogonal vector_score == 0.0, entity == 1/3
    expect_eq(absF32(hits[2].vector_score) < 1.0e-5, true);
    expect_eq(absF32(hits[2].entity_score - (1.0 / 3.0)) < 1.0e-4, true);
}

// ============================================================================
// Test 2: Entity Bitset Tri-Filtering (required / any / excluded)
// ============================================================================

fn testEntityBitsetTriFiltering()! {
    let idx = HybridEpisodeIndex.init(2) ?? return;
    const v: [2]f32 = [1.0, 0.0];

    const A: u64 = 1 << 0;
    const B: u64 = 1 << 1;
    const C: u64 = 1 << 2;
    const D: u64 = 1 << 3;

    * = idx.insertEpisode(1, 100, 1, A | B, 0.5, false, v[0..]);       // passes required=A, any=B|C, not D
    * = idx.insertEpisode(2, 100, 1, A | C, 0.5, false, v[0..]);       // passes
    * = idx.insertEpisode(3, 100, 1, A | B | D, 0.5, false, v[0..]);   // excluded by D
    * = idx.insertEpisode(4, 100, 1, B | C, 0.5, false, v[0..]);       // missing required A
    * = idx.insertEpisode(5, 100, 1, A, 0.5, false, v[0..]);           // missing any(B|C)

    const filter = EpisodeQueryFilter{
        session_id: 0,
        min_ts_ms: 0,
        max_ts_ms: 1000,
        required_entities: A,
        any_entities: B | C,
        excluded_entities: D,
        target_entities: 0, // falls back to required | any = A|B|C
    };

    let hits: [8]EpisodeHybridHit = [EpisodeHybridHit{}; 8];
    let hits_s: []EpisodeHybridHit = hits[0..];
    const n = idx.queryHybrid(v[0..], filter, EpisodeHybridWeights.defaults(), 100, *hits_s);
    expect_eq(n, 2);
    expect_eq(hits[0].episode_id, 1);
    expect_eq(hits[1].episode_id, 2);
    // Both have 2/3 overlap with fallback target A|B|C
    expect_eq(absF32(hits[0].entity_score - (2.0 / 3.0)) < 1.0e-4, true);
}

// ============================================================================
// Test 3: Temporal Window Bounds, Invalid Range & Session Isolation
// ============================================================================

fn testTemporalWindowAndSessionIsolation()! {
    let idx = HybridEpisodeIndex.init(2) ?? return;
    const v: [2]f32 = [1.0, 0.0];

    * = idx.insertEpisode(10, 1000, 1, 1, 0.5, false, v[0..]);
    * = idx.insertEpisode(20, 2000, 1, 1, 0.5, false, v[0..]);
    * = idx.insertEpisode(30, 3000, 1, 1, 0.5, false, v[0..]);
    * = idx.insertEpisode(40, 2000, 2, 1, 0.5, false, v[0..]); // different session

    let hits: [8]EpisodeHybridHit = [EpisodeHybridHit{}; 8];
    let hits_s: []EpisodeHybridHit = hits[0..];

    // Closed interval [1000, 2000] on session 1 -> hits 20 (newer) and 10
    const f_win = EpisodeQueryFilter{
        session_id: 1,
        min_ts_ms: 1000,
        max_ts_ms: 2000,
        required_entities: 0,
        any_entities: 0,
        excluded_entities: 0,
        target_entities: 0,
    };
    const n1 = idx.queryHybrid(v[0..], f_win, EpisodeHybridWeights.defaults(), 3000, *hits_s);
    expect_eq(n1, 2);
    expect_eq(hits[0].episode_id, 20);
    expect_eq(hits[1].episode_id, 10);

    // Invalid window min_ts > max_ts -> safely returns 0
    const f_bad = EpisodeQueryFilter{
        session_id: 0,
        min_ts_ms: 5000,
        max_ts_ms: 1000,
        required_entities: 0,
        any_entities: 0,
        excluded_entities: 0,
        target_entities: 0,
    };
    expect_eq(idx.queryHybrid(v[0..], f_bad, EpisodeHybridWeights.defaults(), 5000, *hits_s), 0);
}

// ============================================================================
// Test 4: Top-K Truncation & Deterministic Tie-Breaking
// ============================================================================

fn testTopKTruncationAndTieBreaking()! {
    let idx = HybridEpisodeIndex.init(2) ?? return;
    const v: [2]f32 = [1.0, 0.0];

    // Insert 6 identical-score episodes with varying timestamps & IDs
    * = idx.insertEpisode(50, 1000, 1, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(30, 2000, 1, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(10, 2000, 1, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(40, 1000, 1, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(20, 2000, 1, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(60, 3000, 1, 0, 0.5, false, v[0..]);

    // Pure vector weight (all 6 tie on total_score == 1.0)
    const w_pure_vec = EpisodeHybridWeights{
        vector_weight: 1.0,
        recency_weight: 0.0,
        entity_weight: 0.0,
        importance_weight: 0.0,
        recency_half_life_ms: 1000,
    };

    let top3: [3]EpisodeHybridHit = [EpisodeHybridHit{}; 3];
    let top3_s: []EpisodeHybridHit = top3[0..];
    const n = idx.queryHybrid(v[0..], EpisodeQueryFilter.all(), w_pure_vec, 5000, *top3_s);
    expect_eq(n, 3);
    // Tie-break: timestamp desc, then episode_id asc -> 60 (t=3000), 10 (t=2000), 20 (t=2000)
    expect_eq(top3[0].episode_id, 60);
    expect_eq(top3[1].episode_id, 10);
    expect_eq(top3[2].episode_id, 20);
}

// ============================================================================
// Test 5: Chronological Session Timeline Replay (`scanSessionTimeline`)
// ============================================================================

fn testSessionTimelineReplay()! {
    let idx = HybridEpisodeIndex.init(2) ?? return;
    const v: [2]f32 = [0.0, 1.0];

    // Insert out of chronological order across two sessions
    * = idx.insertEpisode(300, 3000, 7, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(100, 1000, 7, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(202, 2000, 7, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(201, 2000, 7, 0, 0.5, false, v[0..]);
    * = idx.insertEpisode(999, 1500, 8, 0, 0.5, false, v[0..]); // other session
    * = idx.insertEpisode(400, 4000, 7, 0, 0.5, false, v[0..]); // outside [1000, 3000]

    let ids: [8]u64 = [0; 8];
    let ids_s: []u64 = ids[0..];
    const count = idx.scanSessionTimeline(7, 1000, 3000, *ids_s);
    expect_eq(count, 4);
    expect_eq(ids[0], 100);
    expect_eq(ids[1], 201);
    expect_eq(ids[2], 202);
    expect_eq(ids[3], 300);

    // Invalid range & session 0 guards
    expect_eq(idx.scanSessionTimeline(7, 5000, 1000, *ids_s), 0);
    expect_eq(idx.scanSessionTimeline(0, 0, 5000, *ids_s), 0);
}

// ============================================================================
// Test 6: Watermark Pruning (`pruneBeforeTimestamp`) & Pinned Protection
// ============================================================================

fn testPruneBeforeTimestampProtectsPinned()! {
    let idx = HybridEpisodeIndex.init(2) ?? return;
    const v: [2]f32 = [1.0, 1.0];

    * = idx.insertEpisode(1, 1000, 1, 0, 0.3, false, v[0..]); // old, unpinned -> prune
    * = idx.insertEpisode(2, 1200, 1, 0, 0.9, true, v[0..]);  // old, pinned -> protect
    * = idx.insertEpisode(3, 5000, 1, 0, 0.5, false, v[0..]); // recent -> keep

    const p1 = idx.pruneBeforeTimestamp(3000);
    expect_eq(p1, 1);
    expect_eq(idx.getEpisode(1) == null, true);
    expect_eq(idx.getEpisode(2) != null, true);
    expect_eq(idx.getEpisode(3) != null, true);
    expect_eq(idx.stats().active_count, 2);
    expect_eq(idx.stats().pinned_count, 1);

    // Unpin episode 2 and prune again -> now reclaimed
    expect_eq(idx.setPinned(2, false), true);
    expect_eq(idx.setPinned(999, true), false);
    expect_eq(idx.stats().pinned_count, 0);
    const p2 = idx.pruneBeforeTimestamp(3000);
    expect_eq(p2, 1);
    expect_eq(idx.getEpisode(2) == null, true);
    expect_eq(idx.stats().total_pruned, 2);
}

// ============================================================================
// Test 7: Full Pool Capacity Guard (`EP_INDEX_MAX_SLOTS = 128`) & Reclamation
// ============================================================================

fn testFullPoolCapacityAndReclamation()! {
    let idx = HybridEpisodeIndex.init(2) ?? return;
    const v: [2]f32 = [1.0, 0.0];

    let i: usize = 0;
    while (i < EP_INDEX_MAX_SLOTS) : (i += 1) {
        expect_eq(idx.insertEpisode((i as u64) + 1, 1000 + (i as u64), 1, 1, 0.5, false, v[0..]), true);
    }
    expect_eq(idx.stats().active_count, EP_INDEX_MAX_SLOTS);

    // 129th insert rejected
    expect_eq(idx.insertEpisode(9999, 5000, 1, 1, 0.5, false, v[0..]), false);

    // Prune oldest 10 entries -> frees 10 slots for fresh inserts
    expect_eq(idx.pruneBeforeTimestamp(1010), 10);
    expect_eq(idx.insertEpisode(9999, 5000, 1, 1, 0.5, false, v[0..]), true);
}

// ============================================================================
// Test 8: Full Boundary Guards (Dim, Empty Index, Zero/NaN Vec, Dup ID, Weights)
// ============================================================================

fn testBoundaryGuardsInvalidInputs()! {
    expect_eq(HybridEpisodeIndex.init(0) == null, true);
    expect_eq(HybridEpisodeIndex.init(EP_INDEX_MAX_DIM + 1) == null, true);

    let idx = HybridEpisodeIndex.init(4) ?? return;
    let hits: [4]EpisodeHybridHit = [EpisodeHybridHit{}; 4];
    let hits_s: []EpisodeHybridHit = hits[0..];
    const q_ok: [4]f32 = [1.0, 0.0, 0.0, 0.0];

    // Empty index returns 0
    expect_eq(idx.queryHybrid(q_ok[0..], EpisodeQueryFilter.all(), EpisodeHybridWeights.defaults(), 100, *hits_s), 0);

    // Zero vector, NaN vector, wrong dim, episode_id == 0 rejected
    const zero_v: [4]f32 = [0.0, 0.0, 0.0, 0.0];
    const nan_v: [4]f32 = [std.math.nan(f32), 1.0, 0.0, 0.0];
    const short_v: [2]f32 = [1.0, 0.0];
    expect_eq(idx.insertEpisode(1, 100, 1, 0, 0.5, false, zero_v[0..]), false);
    expect_eq(idx.insertEpisode(1, 100, 1, 0, 0.5, false, nan_v[0..]), false);
    expect_eq(idx.insertEpisode(1, 100, 1, 0, 0.5, false, short_v[0..]), false);
    expect_eq(idx.insertEpisode(0, 100, 1, 0, 0.5, false, q_ok[0..]), false);
    expect_eq(idx.insertEpisode(1, 100, 1, 0, std.math.nan(f32), false, q_ok[0..]), false);

    // Valid insert + duplicate ID rejected
    expect_eq(idx.insertEpisode(1, 100, 1, 0, 0.5, false, q_ok[0..]), true);
    expect_eq(idx.insertEpisode(1, 200, 1, 0, 0.5, false, q_ok[0..]), false);

    // Zero query vector, wrong dim query, and invalid weights return 0 safely
    expect_eq(idx.queryHybrid(zero_v[0..], EpisodeQueryFilter.all(), EpisodeHybridWeights.defaults(), 100, *hits_s), 0);
    expect_eq(idx.queryHybrid(short_v[0..], EpisodeQueryFilter.all(), EpisodeHybridWeights.defaults(), 100, *hits_s), 0);

    const bad_w = EpisodeHybridWeights{
        vector_weight: -0.5,
        recency_weight: 0.2,
        entity_weight: 0.2,
        importance_weight: 0.1,
        recency_half_life_ms: 1000,
    };
    expect_eq(idx.queryHybrid(q_ok[0..], EpisodeQueryFilter.all(), bad_w, 100, *hits_s), 0);

    const zero_hl = EpisodeHybridWeights{
        vector_weight: 0.5,
        recency_weight: 0.2,
        entity_weight: 0.2,
        importance_weight: 0.1,
        recency_half_life_ms: 0,
    };
    expect_eq(idx.queryHybrid(q_ok[0..], EpisodeQueryFilter.all(), zero_hl, 100, *hits_s), 0);

    expect_eq(EP_INDEX_MAX_SLOTS, 128);
    expect_eq(EP_INDEX_MAX_DIM, 32);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("agentmem.episode_index: tri-modal hybrid scoring breakdown (@dot + @popCount + recency)", testTriModalHybridScoringBreakdown);
test("agentmem.episode_index: entity bitset tri-filtering (required, any, excluded)", testEntityBitsetTriFiltering);
test("agentmem.episode_index: temporal window bounds, invalid range & session isolation", testTemporalWindowAndSessionIsolation);
test("agentmem.episode_index: Top-K truncation & deterministic tie-breaking", testTopKTruncationAndTieBreaking);
test("agentmem.episode_index: chronological session timeline replay", testSessionTimelineReplay);
test("agentmem.episode_index: watermark pruning protects pinned episodes", testPruneBeforeTimestampProtectsPinned);
test("agentmem.episode_index: full 128-slot pool capacity guard & reclamation", testFullPoolCapacityAndReclamation);
test("agentmem.episode_index: invalid dim, empty index, zero/NaN vector & invalid weights guards", testBoundaryGuardsInvalidInputs);
