// Origin: Pure-CP Stdlib Test Suite (lib/agentmem/entity_graph.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Temporal Entity Knowledge Graph & Two-Hop Recall:
//   1. assertFact, atomic supersedeFact & point-in-time historical snapshot queries
//   2. queryFactsAt relation filtering, confidence ordering & manual invalidateEdge
//   3. traverseTwoHop 1-hop + 2-hop multiplicative confidence decay & temporal isolation
//   4. Cycle avoidance (A->B->A), multi-path deduplication (keeping best confidence) & tie-breaking
//   5. min_confidence filtering, EntityGraphStats & reset lifecycle
//   6. Full capacity atomic guard (supersedeFact does not corrupt old facts when full) & invalid input guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TemporalFactEdge,
    GraphHopHit,
    EntityGraphStats,
    TemporalEntityGraph,
} from "cap:agentmem";

fn approxEqF32(a: f32, b: f32, eps: f32): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= eps;
}

// ============================================================================
// Test 1: assertFact, supersedeFact & Historical Time-Travel Snapshots
// ============================================================================

fn testSupersedeFactAndHistoricalSnapshots()! {
    let buf: [16]TemporalFactEdge = [TemporalFactEdge{}; 16];
    let buf_s: []TemporalFactEdge = buf[0..];
    let g = cap.agentmem.TemporalEntityGraph.init(buf_s) ?? return error.InitFailed;

    const alice: u64 = 100;
    const rel_lives_in: u32 = 1;
    const tokyo: u64 = 201;
    const sydney: u64 = 202;

    // t=1000: Alice lives_in Tokyo (confidence 0.90)
    const e1 = g.assertFact(alice, rel_lives_in, tokyo, 0.90, 1000) ?? return error.AssertFailed;
    expect_eq(e1, 1);

    // t=2000: Alice moves to Sydney -> supersedes Tokyo fact atomically
    const e2 = g.supersedeFact(alice, rel_lives_in, sydney, 0.98, 2000) ?? return error.SupersedeFailed;
    expect_eq(e2, 2);

    let out: [4]TemporalFactEdge = [TemporalFactEdge{}; 4];
    let out_s: []TemporalFactEdge = out[0..];

    // Before t=1000: no facts valid
    expect_eq(g.queryFactsAt(alice, rel_lives_in, 500, out_s), 0);

    // At t=1500: historical snapshot must return Tokyo only
    const n_hist = g.queryFactsAt(alice, rel_lives_in, 1500, out_s);
    expect_eq(n_hist, 1);
    expect_eq(out[0].edge_id, e1);
    expect_eq(out[0].dst_entity_id, tokyo);
    expect_eq(out[0].valid_to_ms, 2000);

    // At t=2000 (exact boundary) & t=3000: must return Sydney only
    const n_now = g.queryFactsAt(alice, rel_lives_in, 2000, out_s);
    expect_eq(n_now, 1);
    expect_eq(out[0].edge_id, e2);
    expect_eq(out[0].dst_entity_id, sydney);
    expect_eq(out[0].valid_to_ms, 0);

    const st: EntityGraphStats = g.stats();
    expect_eq(st.total_edges, 2);
    expect_eq(st.current_valid_edges, 1);
    expect_eq(st.superseded_edges, 1);
    expect_eq(st.total_assertions, 2);
    expect_eq(st.total_supersedes, 1);
}

// ============================================================================
// Test 2: queryFactsAt Filtering, Ordering & Manual invalidateEdge
// ============================================================================

fn testQueryFactsOrderingAndManualInvalidate()! {
    let buf: [8]TemporalFactEdge = [TemporalFactEdge{}; 8];
    let buf_s: []TemporalFactEdge = buf[0..];
    let g = TemporalEntityGraph.init(buf_s) ?? return error.InitFailed;

    const e1 = g.assertFact(10, 1, 100, 0.60, 100) ?? return error.AssertFailed;
    const e2 = g.assertFact(10, 2, 200, 0.95, 100) ?? return error.AssertFailed;
    const e3 = g.assertFact(10, 1, 300, 0.85, 100) ?? return error.AssertFailed;

    let out: [4]TemporalFactEdge = [TemporalFactEdge{}; 4];
    let out_s: []TemporalFactEdge = out[0..];

    // Query all relations (rel_id_opt = null): sorted by confidence desc -> e2 (0.95), e3 (0.85), e1 (0.60)
    const n_all = g.queryFactsAt(10, null, 200, out_s);
    expect_eq(n_all, 3);
    expect_eq(out[0].edge_id, e2);
    expect_eq(out[1].edge_id, e3);
    expect_eq(out[2].edge_id, e1);

    // Filter by rel_type_id == 1 -> e3 (0.85), e1 (0.60)
    const n_rel1 = g.queryFactsAt(10, 1, 200, out_s);
    expect_eq(n_rel1, 2);
    expect_eq(out[0].edge_id, e3);
    expect_eq(out[1].edge_id, e1);

    // Manually invalidate e3 at t=500
    expect_eq(g.invalidateEdge(e3, 50), false);  // valid_to_ms < valid_from_ms rejected
    expect_eq(g.invalidateEdge(e3, 500), true);
    expect_eq(g.invalidateEdge(e3, 600), false); // already invalidated

    expect_eq(g.queryFactsAt(10, 1, 499, out_s), 2);
    expect_eq(g.queryFactsAt(10, 1, 500, out_s), 1);
    expect_eq(out[0].edge_id, e1);
}

// ============================================================================
// Test 3: Two-Hop Confidence Decay & Temporal Isolation
// ============================================================================

fn testTwoHopConfidenceDecayAndTemporalIsolation()! {
    let buf: [16]TemporalFactEdge = [TemporalFactEdge{}; 16];
    let buf_s: []TemporalFactEdge = buf[0..];
    let g = TemporalEntityGraph.init(buf_s) ?? return error.InitFailed;

    // Graph at t=1000:
    //   1 --(0.90)--> 2 --(0.80)--> 4   (2-hop conf = 0.72)
    //   1 --(0.50)--> 3 --(0.60)--> 5   (2-hop conf = 0.30)
    const e12 = g.assertFact(1, 10, 2, 0.90, 1000) ?? return error.AssertFailed;
    const e24 = g.assertFact(2, 11, 4, 0.80, 1000) ?? return error.AssertFailed;
    const e13 = g.assertFact(1, 10, 3, 0.50, 1000) ?? return error.AssertFailed;
    const e35 = g.assertFact(3, 11, 5, 0.60, 1000) ?? return error.AssertFailed;
    * = e12;
    * = e24;
    * = e13;
    * = e35;

    // Supersede 2 -> 4 with 2 -> 6 at t=2000
    const e26 = g.supersedeFact(2, 11, 6, 0.90, 2000) ?? return error.SupersedeFailed;
    * = e26;

    let hits: [8]GraphHopHit = [GraphHopHit{}; 8];
    let hits_s: []GraphHopHit = hits[0..];

    // Snapshot at t=1500: 2->4 is active, 2->6 is not yet valid
    // Expected descending order: 2 (0.90, hop 1), 4 (0.72, hop 2), 3 (0.50, hop 1), 5 (0.30, hop 2)
    const n1 = g.traverseTwoHop(1, 1500, 0.0, hits_s);
    expect_eq(n1, 4);
    expect_eq(hits[0].entity_id, 2);
    expect_eq(hits[0].hop_depth, 1);
    expect_eq(approxEqF32(hits[0].path_confidence, 0.90, 0.0001), true);

    expect_eq(hits[1].entity_id, 4);
    expect_eq(hits[1].hop_depth, 2);
    expect_eq(approxEqF32(hits[1].path_confidence, 0.72, 0.0001), true);

    expect_eq(hits[2].entity_id, 3);
    expect_eq(hits[2].hop_depth, 1);
    expect_eq(approxEqF32(hits[2].path_confidence, 0.50, 0.0001), true);

    expect_eq(hits[3].entity_id, 5);
    expect_eq(hits[3].hop_depth, 2);
    expect_eq(approxEqF32(hits[3].path_confidence, 0.30, 0.0001), true);

    // Snapshot at t=2500: 2->6 (0.81) replaces 2->4
    const n2 = g.traverseTwoHop(1, 2500, 0.0, hits_s);
    expect_eq(n2, 4);
    expect_eq(hits[0].entity_id, 2);
    expect_eq(hits[1].entity_id, 6);
    expect_eq(hits[1].hop_depth, 2);
    expect_eq(approxEqF32(hits[1].path_confidence, 0.81, 0.0001), true);
}

// ============================================================================
// Test 4: Cycle Avoidance, Multi-Path Deduplication & Tie-Breaking
// ============================================================================

fn testCycleAvoidanceAndMultiPathDedup()! {
    let buf: [16]TemporalFactEdge = [TemporalFactEdge{}; 16];
    let buf_s: []TemporalFactEdge = buf[0..];
    let g = TemporalEntityGraph.init(buf_s) ?? return error.InitFailed;

    // Cycle: 1 -> 2 (0.80), 2 -> 1 (0.95) -- start entity 1 must NEVER appear in hits
    * = g.assertFact(1, 1, 2, 0.80, 100);
    * = g.assertFact(2, 1, 1, 0.95, 100);

    // Multi-path to entity 3:
    //   1 -> 3 directly with 0.50 (hop 1)
    //   1 -> 2 -> 3 with 0.80 * 0.90 = 0.72 (hop 2, higher confidence -> must win & dedup!)
    * = g.assertFact(1, 2, 3, 0.50, 100);
    * = g.assertFact(2, 2, 3, 0.90, 100);

    // Tie-break test: entity 20 and entity 10 both at confidence 0.60 -> entity 10 must come first
    * = g.assertFact(1, 3, 20, 0.60, 100);
    * = g.assertFact(1, 3, 10, 0.60, 100);

    let hits: [8]GraphHopHit = [GraphHopHit{}; 8];
    let hits_s: []GraphHopHit = hits[0..];
    const n = g.traverseTwoHop(1, 200, 0.0, hits_s);

    // Unique entities reachable: 2 (0.80), 3 (0.72 via hop 2), 10 (0.60), 20 (0.60) -> total 4
    expect_eq(n, 4);
    expect_eq(hits[0].entity_id, 2);
    expect_eq(hits[1].entity_id, 3);
    expect_eq(hits[1].hop_depth, 2);
    expect_eq(approxEqF32(hits[1].path_confidence, 0.72, 0.0001), true);
    expect_eq(hits[2].entity_id, 10);
    expect_eq(hits[3].entity_id, 20);
}

// ============================================================================
// Test 5: Low-Confidence Filtering & Reset Lifecycle
// ============================================================================

fn testMinConfidenceFilterAndReset()! {
    let buf: [8]TemporalFactEdge = [TemporalFactEdge{}; 8];
    let buf_s: []TemporalFactEdge = buf[0..];
    let g = TemporalEntityGraph.init(buf_s) ?? return error.InitFailed;

    * = g.assertFact(1, 1, 2, 0.80, 100);
    * = g.assertFact(2, 1, 3, 0.50, 100); // 2-hop = 0.40
    * = g.assertFact(1, 1, 4, 0.30, 100); // 1-hop = 0.30

    let hits: [4]GraphHopHit = [GraphHopHit{}; 4];
    let hits_s: []GraphHopHit = hits[0..];

    // min_confidence = 0.45 keeps only entity 2 (0.80)
    const n = g.traverseTwoHop(1, 200, 0.45, hits_s);
    expect_eq(n, 1);
    expect_eq(hits[0].entity_id, 2);

    // min_confidence = 0.35 keeps entity 2 (0.80) and entity 3 (0.40)
    const n2 = g.traverseTwoHop(1, 200, 0.35, hits_s);
    expect_eq(n2, 2);
    expect_eq(hits[0].entity_id, 2);
    expect_eq(hits[1].entity_id, 3);

    g.reset();
    const st = g.stats();
    expect_eq(st.total_edges, 0);
    expect_eq(st.current_valid_edges, 0);
    expect_eq(g.traverseTwoHop(1, 200, 0.0, hits_s), 0);
}

// ============================================================================
// Test 6: Full-Capacity Atomic Guard & Invalid Input Guards
// ============================================================================

fn testBoundaryAndInvalidInputGuards()! {
    let buf: [2]TemporalFactEdge = [TemporalFactEdge{}; 2];
    let empty_buf: []TemporalFactEdge = buf[0..0];
    let buf_s: []TemporalFactEdge = buf[0..];
    expect_eq(TemporalEntityGraph.init(empty_buf) == null, true);

    let g = TemporalEntityGraph.init(buf_s) ?? return error.InitFailed;

    // Invalid IDs, confidence, and timestamps
    expect_eq(g.assertFact(0, 1, 2, 0.8, 100) == null, true);
    expect_eq(g.assertFact(1, 0, 2, 0.8, 100) == null, true);
    expect_eq(g.assertFact(1, 1, 0, 0.8, 100) == null, true);
    expect_eq(g.assertFact(1, 1, 2, 0.0, 100) == null, true);
    expect_eq(g.assertFact(1, 1, 2, -0.5, 100) == null, true);
    expect_eq(g.assertFact(1, 1, 2, 1.1, 100) == null, true);
    expect_eq(g.assertFact(1, 1, 2, std.math.nan(f32), 100) == null, true);
    expect_eq(g.assertFact(1, 1, 2, std.math.inf(f32), 100) == null, true);
    expect_eq(g.assertFact(1, 1, 2, 0.8, -1) == null, true);

    // Fill capacity (2 slots)
    expect_eq(g.assertFact(1, 1, 10, 0.9, 100) != null, true);
    expect_eq(g.assertFact(2, 1, 20, 0.9, 100) != null, true);

    // Full capacity: assertFact & supersedeFact must fail WITHOUT invalidating existing fact (1, 1, 10)
    expect_eq(g.assertFact(3, 1, 30, 0.9, 200) == null, true);
    expect_eq(g.supersedeFact(1, 1, 11, 0.95, 200) == null, true);

    // Verify existing fact (1, 1, 10) is still valid at t=300 (atomic rollback guarantee)
    let out: [2]TemporalFactEdge = [TemporalFactEdge{}; 2];
    let out_s: []TemporalFactEdge = out[0..];
    expect_eq(g.queryFactsAt(1, 1, 300, out_s), 1);
    expect_eq(out[0].dst_entity_id, 10);
    expect_eq(out[0].valid_to_ms, 0);

    // Invalid traverseTwoHop min_confidence returns 0
    let hits: [2]GraphHopHit = [GraphHopHit{}; 2];
    let hits_s: []GraphHopHit = hits[0..];
    expect_eq(g.traverseTwoHop(1, 300, std.math.nan(f32), hits_s), 0);
    expect_eq(g.traverseTwoHop(1, 300, -0.1, hits_s), 0);
    expect_eq(g.traverseTwoHop(0, 300, 0.0, hits_s), 0);
}

test("agentmem entity_graph: assertFact, supersedeFact & historical time-travel snapshots", testSupersedeFactAndHistoricalSnapshots);
test("agentmem entity_graph: queryFactsAt relation filter, ordering & manual invalidateEdge", testQueryFactsOrderingAndManualInvalidate);
test("agentmem entity_graph: traverseTwoHop 1-hop + 2-hop decay & temporal isolation", testTwoHopConfidenceDecayAndTemporalIsolation);
test("agentmem entity_graph: cycle avoidance, multi-path dedup & deterministic tie-breaking", testCycleAvoidanceAndMultiPathDedup);
test("agentmem entity_graph: min_confidence filtering & reset lifecycle", testMinConfidenceFilterAndReset);
test("agentmem entity_graph: full-capacity atomic guard & invalid input guards", testBoundaryAndInvalidInputGuards);
