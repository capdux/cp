// Origin: Pure-CP Stdlib Test Suite (lib/vector/filtered_hnsw.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Predicate-Aware Filtered HNSW:
//   1. Bounded bidirectional graph construction & high-selectivity single-stage graph_filtered search
//   2. Non-matching bridge traversal preserving connectivity across multi-hop filtered search
//   3. Low-selectivity adaptive exact_fallback via @popCount (100% recall & minimal distance_evals)
//   4. Zero-match predicate short-circuit (0 distance_evals) & empty index safety
//   5. Live metadata updateNodeTags hot-reindexing
//   6. Tombstone markDeleted (excluded from hits, preserves bridge connectivity, accurate stats)
//   7. Boundary & error guards (bad init, invalid_dimension, NaN vector, duplicate_doc_id, index_full)
//   8. cap.vector namespace access & multi-tenant multi-tag exact top-K verification

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FHNSW_ANY_TENANT,
    FilteredSearchPath,
    FilteredGraphStatus,
    FilteredGraphConfig,
    FilteredPredicate,
    FilteredSearchHit,
    FilteredHnswIndex,
} from "cap:vector";

// ============================================================================
// Test 1: High-Selectivity Single-Stage graph_filtered Search
// ============================================================================

fn testHighSelectivityGraphFilteredSearch()! {
    let vec_buf: [256]f32 = [0.0; 256]; // 64 x 4D
    let vec_s: []f32 = vec_buf[0..];
    const cfg = FilteredGraphConfig{
        dim: 4,
        max_nodes: 32,
        max_degree: 6,
        ef_construction: 16,
        exact_fallback_bps: 1500,
    };
    let idx = FilteredHnswIndex.init(cfg, *vec_s) ?? return error.InitFailed;

    const TAG_PUBLIC: u64 = 1 << 0;
    const TAG_TECH: u64 = 1 << 1;
    const TAG_DEPRECATED: u64 = 1 << 2;

    // Insert 10 vectors across 2 tenants with varied tag masks
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        const fi: f32 = i as f32;
        const v: [4]f32 = [1.0 - fi * 0.08, fi * 0.09, 0.1, 0.0];
        let tags: u64 = TAG_PUBLIC | TAG_TECH;
        if (i == 0) {
            tags |= TAG_DEPRECATED; // closest vector is forbidden!
        }
        const tenant: u32 = if (i % 2 == 0) 1 else 2;
        expect_eq(idx.insert(100 + (i as u64), v[0..], tags, tenant) == FilteredGraphStatus.ok, true);
    }

    const q: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const pred = FilteredPredicate{
        required_tags: TAG_PUBLIC | TAG_TECH,
        forbidden_tags: TAG_DEPRECATED,
        tenant_id: 1, // even indices except 0 (deprecated) -> 2, 4, 6, 8 => 4/10 = 4000 bps >= 1500
    };

    let hits: [4]FilteredSearchHit = [FilteredSearchHit{}; 4];
    let hits_s: []FilteredSearchHit = hits[0..];
    const rep = idx.search(q[0..], pred, 3, 16, *hits_s);

    expect_eq(rep.path == FilteredSearchPath.graph_filtered, true);
    expect_eq(rep.matching_candidates, 4);
    expect_eq(rep.selectivity_bps, 4000);
    expect_eq(rep.hit_count, 3);
    // Closest matching tenant=1 non-deprecated nodes should be doc 102, 104, 106 in order
    expect_eq(hits[0].doc_id, 102);
    expect_eq(hits[1].doc_id, 104);
    expect_eq(hits[2].doc_id, 106);
    expect_eq(hits[0].similarity >= hits[1].similarity, true);
    expect_eq(hits[1].similarity >= hits[2].similarity, true);
}

// ============================================================================
// Test 2: Non-Matching Bridge Traversal Connectivity
// ============================================================================

fn testNonMatchingBridgeTraversal()! {
    let vec_buf: [128]f32 = [0.0; 128];
    let vec_s: []f32 = vec_buf[0..];
    let idx = FilteredHnswIndex.init(FilteredGraphConfig{
        dim: 4,
        max_nodes: 16,
        max_degree: 4,
        ef_construction: 8,
        exact_fallback_bps: 1000, // 10%
    }, *vec_s) ?? return error.InitFailed;

    // Entry node (doc 1) and intermediate nodes (doc 2, 3) do NOT match target tag 0x10
    const v1: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v2: [4]f32 = [0.7, 0.7, 0.0, 0.0];
    const v3: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    const v4: [4]f32 = [0.0, 0.7, 0.7, 0.0];
    const v5: [4]f32 = [0.0, 0.0, 1.0, 0.0];

    expect_eq(idx.insert(1, v1[0..], 0x01, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(2, v2[0..], 0x01, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(3, v3[0..], 0x01, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(4, v4[0..], 0x10, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(5, v5[0..], 0x10, 1) == FilteredGraphStatus.ok, true);

    // 2/5 = 4000 bps -> takes graph_filtered path, must traverse non-matching entry node 1
    let hits: [2]FilteredSearchHit = [FilteredSearchHit{}; 2];
    let hits_s: []FilteredSearchHit = hits[0..];
    const rep = idx.search(v5[0..], FilteredPredicate{ required_tags: 0x10 }, 2, 8, *hits_s);

    expect_eq(rep.path == FilteredSearchPath.graph_filtered, true);
    expect_eq(rep.hit_count, 2);
    expect_eq(hits[0].doc_id, 5);
    expect_eq(hits[1].doc_id, 4);
}

// ============================================================================
// Test 3: Low-Selectivity Adaptive exact_fallback via @popCount
// ============================================================================

fn testLowSelectivityExactFallback()! {
    let vec_buf: [256]f32 = [0.0; 256]; // 64 x 4D
    let vec_s: []f32 = vec_buf[0..];
    let idx = FilteredHnswIndex.init(FilteredGraphConfig{
        dim: 4,
        max_nodes: 64,
        max_degree: 6,
        ef_construction: 12,
        exact_fallback_bps: 1500, // 15%
    }, *vec_s) ?? return error.InitFailed;

    // Insert 40 nodes; only doc 17 and doc 33 carry rare tag 0x80 (2/40 = 500 bps < 1500 bps)
    let i: usize = 0;
    while (i < 40) : (i += 1) {
        const fi: f32 = i as f32;
        const v: [4]f32 = [1.0, fi * 0.05, 0.2, 0.0];
        const tag: u64 = if (i == 17 || i == 33) 0x80 else 0x01;
        expect_eq(idx.insert((i as u64) + 1, v[0..], tag, 7) == FilteredGraphStatus.ok, true);
    }

    const q: [4]f32 = [1.0, 33.0 * 0.05, 0.2, 0.0];
    let hits: [4]FilteredSearchHit = [FilteredSearchHit{}; 4];
    let hits_s: []FilteredSearchHit = hits[0..];
    const rep = idx.search(q[0..], FilteredPredicate{ required_tags: 0x80, tenant_id: 7 }, 3, 12, *hits_s);

    expect_eq(rep.path == FilteredSearchPath.exact_fallback, true);
    expect_eq(rep.matching_candidates, 2);
    expect_eq(rep.selectivity_bps, 500);
    expect_eq(rep.distance_evals, 2); // Only evaluated the 2 matching nodes!
    expect_eq(rep.hit_count, 2);
    expect_eq(hits[0].doc_id, 34); // index 33 -> doc_id 34
    expect_eq(hits[1].doc_id, 18); // index 17 -> doc_id 18
}

// ============================================================================
// Test 4: Zero-Match Predicate Short-Circuit & Empty Index Safety
// ============================================================================

fn testZeroMatchShortCircuitAndEmptyIndex()! {
    let vec_buf: [64]f32 = [0.0; 64];
    let vec_s: []f32 = vec_buf[0..];
    let idx = FilteredHnswIndex.init(FilteredGraphConfig{ dim: 4, max_nodes: 16, max_degree: 4 }, *vec_s) ?? return error.InitFailed;

    const q: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    let hits: [4]FilteredSearchHit = [FilteredSearchHit{}; 4];
    let hits_s: []FilteredSearchHit = hits[0..];

    // Empty index
    const r0 = idx.search(q[0..], FilteredPredicate{}, 4, 8, *hits_s);
    expect_eq(r0.path == FilteredSearchPath.empty_result, true);
    expect_eq(r0.hit_count, 0);
    expect_eq(r0.distance_evals, 0);

    // Non-empty index, 0 matching candidates
    expect_eq(idx.insert(1, q[0..], 0x01, 1) == FilteredGraphStatus.ok, true);
    const r1 = idx.search(q[0..], FilteredPredicate{ required_tags: 0x100 }, 4, 8, *hits_s);
    expect_eq(r1.path == FilteredSearchPath.empty_result, true);
    expect_eq(r1.matching_candidates, 0);
    expect_eq(r1.distance_evals, 0);
}

// ============================================================================
// Test 5: Dynamic Metadata Tag Updates (updateNodeTags)
// ============================================================================

fn testUpdateNodeTags()! {
    let vec_buf: [64]f32 = [0.0; 64];
    let vec_s: []f32 = vec_buf[0..];
    let idx = FilteredHnswIndex.init(FilteredGraphConfig{ dim: 4, max_nodes: 16, max_degree: 4 }, *vec_s) ?? return error.InitFailed;

    const v: [4]f32 = [0.0, 1.0, 0.0, 0.0];
    expect_eq(idx.insert(500, v[0..], 0x01, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.updateNodeTags(999, 0x02), false); // unknown doc_id

    let hits: [2]FilteredSearchHit = [FilteredSearchHit{}; 2];
    let hits_s: []FilteredSearchHit = hits[0..];
    expect_eq(idx.search(v[0..], FilteredPredicate{ required_tags: 0x04 }, 2, 4, *hits_s).hit_count, 0);

    // Hot-update tag mask to 0x04
    expect_eq(idx.updateNodeTags(500, 0x04), true);
    const rep = idx.search(v[0..], FilteredPredicate{ required_tags: 0x04 }, 2, 4, *hits_s);
    expect_eq(rep.hit_count, 1);
    expect_eq(hits[0].doc_id, 500);
    expect_eq(hits[0].tag_mask, 0x04);
}

// ============================================================================
// Test 6: Tombstone markDeleted & Graph Connectivity Preservation
// ============================================================================

fn testTombstoneDeleteAndStats()! {
    let vec_buf: [64]f32 = [0.0; 64];
    let vec_s: []f32 = vec_buf[0..];
    let idx = FilteredHnswIndex.init(FilteredGraphConfig{ dim: 4, max_nodes: 16, max_degree: 4 }, *vec_s) ?? return error.InitFailed;

    const v0: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    const v1: [4]f32 = [0.9, 0.1, 0.0, 0.0];
    const v2: [4]f32 = [0.8, 0.2, 0.0, 0.0];
    expect_eq(idx.insert(10, v0[0..], 0x01, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(20, v1[0..], 0x01, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(30, v2[0..], 0x01, 1) == FilteredGraphStatus.ok, true);

    // Delete entry node doc 10
    expect_eq(idx.markDeleted(10), true);
    expect_eq(idx.markDeleted(10), false); // double-delete returns false
    expect_eq(idx.updateNodeTags(10, 0xFF), false); // cannot update tombstoned node

    const st = idx.stats();
    expect_eq(st.total_slots, 3);
    expect_eq(st.live_nodes, 2);
    expect_eq(st.deleted_nodes, 1);
    expect_eq(st.total_edges >= 4, true);

    let hits: [3]FilteredSearchHit = [FilteredSearchHit{}; 3];
    let hits_s: []FilteredSearchHit = hits[0..];
    const rep = idx.search(v0[0..], FilteredPredicate{ required_tags: 0x01 }, 3, 8, *hits_s);
    expect_eq(rep.hit_count, 2);
    expect_eq(hits[0].doc_id, 20);
    expect_eq(hits[1].doc_id, 30);
}

// ============================================================================
// Test 7: Boundary & Error Guards
// ============================================================================

fn testBoundaryAndErrorGuards()! {
    let small_buf: [8]f32 = [0.0; 8];
    let small_s: []f32 = small_buf[0..];
    // Invalid init configs
    expect_eq(FilteredHnswIndex.init(FilteredGraphConfig{ dim: 0, max_nodes: 2, max_degree: 2 }, *small_s) == null, true);
    expect_eq(FilteredHnswIndex.init(FilteredGraphConfig{ dim: 4, max_nodes: 4, max_degree: 2 }, *small_s) == null, true); // buf too small
    expect_eq(FilteredHnswIndex.init(FilteredGraphConfig{ dim: 4, max_nodes: 2, max_degree: 17 }, *small_s) == null, true); // M > 16

    let idx = FilteredHnswIndex.init(FilteredGraphConfig{ dim: 4, max_nodes: 2, max_degree: 2 }, *small_s) ?? return error.InitFailed;

    const bad_dim: [3]f32 = [1.0, 0.0, 0.0];
    expect_eq(idx.insert(1, bad_dim[0..], 0, 1) == FilteredGraphStatus.invalid_dimension, true);

    const zero_f: f32 = 0.0;
    const nan_v: [4]f32 = [zero_f / zero_f, 1.0, 0.0, 0.0];
    expect_eq(idx.insert(1, nan_v[0..], 0, 1) == FilteredGraphStatus.invalid_vector, true);

    const ok_v: [4]f32 = [1.0, 0.0, 0.0, 0.0];
    expect_eq(idx.insert(1, ok_v[0..], 0, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(1, ok_v[0..], 0, 1) == FilteredGraphStatus.duplicate_doc_id, true);
    expect_eq(idx.insert(2, ok_v[0..], 0, 1) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(3, ok_v[0..], 0, 1) == FilteredGraphStatus.index_full, true);
}

// ============================================================================
// Test 8: cap.vector Namespace & Wildcard Tenant Multi-Tag Query
// ============================================================================

fn testCapVectorNamespaceAndWildcardTenant()! {
    let vec_buf: [128]f32 = [0.0; 128];
    let vec_s: []f32 = vec_buf[0..];
    let idx = cap.vector.FilteredHnswIndex.init(FilteredGraphConfig{
        dim: 4,
        max_nodes: 16,
        max_degree: 6,
    }, *vec_s) ?? return error.InitFailed;

    const a: [4]f32 = [0.0, 0.0, 1.0, 0.0];
    const b: [4]f32 = [0.0, 0.0, 0.9, 0.4];
    expect_eq(idx.insert(7001, a[0..], 0b101, 10) == FilteredGraphStatus.ok, true);
    expect_eq(idx.insert(7002, b[0..], 0b101, 20) == FilteredGraphStatus.ok, true);

    let hits: [2]FilteredSearchHit = [FilteredSearchHit{}; 2];
    let hits_s: []FilteredSearchHit = hits[0..];
    const rep = idx.search(a[0..], FilteredPredicate{
        required_tags: 0b101,
        tenant_id: FHNSW_ANY_TENANT,
    }, 2, 8, *hits_s);
    expect_eq(rep.hit_count, 2);
    expect_eq(hits[0].doc_id, 7001);
    expect_eq(hits[1].doc_id, 7002);
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("filtered_hnsw: high-selectivity single-stage graph_filtered search", testHighSelectivityGraphFilteredSearch);
test("filtered_hnsw: non-matching bridge traversal preserves connectivity", testNonMatchingBridgeTraversal);
test("filtered_hnsw: low-selectivity adaptive exact_fallback via @popCount", testLowSelectivityExactFallback);
test("filtered_hnsw: zero-match short-circuit & empty index safety", testZeroMatchShortCircuitAndEmptyIndex);
test("filtered_hnsw: dynamic metadata updateNodeTags hot-reindexing", testUpdateNodeTags);
test("filtered_hnsw: tombstone markDeleted & connectivity/stats preservation", testTombstoneDeleteAndStats);
test("filtered_hnsw: boundary & error guards (init, dim, NaN, dup, full)", testBoundaryAndErrorGuards);
test("filtered_hnsw: cap.vector namespace & wildcard tenant query", testCapVectorNamespaceAndWildcardTenant);
