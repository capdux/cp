// Origin: Pure-CP Stdlib Test Suite (lib/vector/nav_graph.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation Vamana/NSW Navigable Graph ANN index:
// exact self-retrieval (dist_sq == 0), 64D clustered Recall@5 >= 0.90 vs brute-force ground truth,
// bidirectional edge symmetry & max_degree cap guard, ef_search recall expansion, and boundary guards.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    NAV_MAX_DEGREE,
    NAV_INVALID_NODE,
    NavGraphHit,
    NavNodeAdj,
    NavGraphIndex,
} from "cap:vector";

// Deterministic SplitMix64 PRNG for reproducible high-dimensional cluster generation.
fn nextRandU64(state: *u64): u64 {
    const c_add: u64 = ((0x9e3779b9 as u64) << 32) | (0x7f4a7c15 as u64);
    const c_m1: u64 = ((0xbf58476d as u64) << 32) | (0x1ce4e5b9 as u64);
    const c_m2: u64 = ((0x94d049bb as u64) << 32) | (0x133111eb as u64);
    state.* = state.* +% c_add;
    let z: u64 = state.*;
    z = (z ^ (z >> 30)) *% c_m1;
    z = (z ^ (z >> 27)) *% c_m2;
    return z ^ (z >> 31);
}

fn nextRandUnitF32(state: *u64): f32 {
    const r: u64 = nextRandU64(state) >> 40; // 24-bit mantissa
    const u: f32 = (r as f32) / 16777216.0;  // [0.0, 1.0)
    return (u * 2.0) - 1.0;                  // [-1.0, 1.0)
}

fn bruteForceTopK(
    query: []const f32,
    corpus: []const f32,
    n_nodes: usize,
    dim: usize,
    k: usize,
    out_ids: *[]u32,
    out_dists: *[]f32,
) {
    let count: usize = 0;
    let i: usize = 0;
    while (i < n_nodes) : (i += 1) {
        const vec: []const f32 = corpus[i * dim .. (i + 1) * dim];
        let d: f32 = @l2sq(query, vec);
        if (d < 0.0) d = 0.0;
        const id: u32 = i as u32;

        if (count == k) {
            if (d > out_dists[count - 1] || (d == out_dists[count - 1] && id >= out_ids[count - 1])) {
                continue;
            }
        }

        let pos: usize = 0;
        while (pos < count) : (pos += 1) {
            if (d < out_dists[pos] || (d == out_dists[pos] && id < out_ids[pos])) {
                break;
            }
        }
        const new_count: usize = if (count < k) count + 1 else k;
        let s: usize = new_count - 1;
        while (s > pos) : (s -= 1) {
            out_dists[s] = out_dists[s - 1];
            out_ids[s] = out_ids[s - 1];
        }
        out_dists[pos] = d;
        out_ids[pos] = id;
        count = new_count;
    }
}

fn countHitsOverlap(hits: []const NavGraphHit, truth_ids: []const u32, k: usize): usize {
    let matched: usize = 0;
    let i: usize = 0;
    while (i < k) : (i += 1) {
        let j: usize = 0;
        while (j < k) : (j += 1) {
            if (hits[i].node_id == truth_ids[j]) {
                matched += 1;
                break;
            }
        }
    }
    return matched;
}

fn hasNeighbor(adj: NavNodeAdj, target: u32): bool {
    let i: usize = 0;
    const deg: usize = adj.degree as usize;
    while (i < deg) : (i += 1) {
        if (adj.neighbors[i] == target) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// Test 1: Insert & Exact Self-Search (dist_sq == 0) + cap.vector export check
// ============================================================================

fn testInsertAndExactSelfSearchZeroDist()! {
    const dim: usize = 8;
    const n: usize = 16;
    let vecs_buf: [128]f32 = [0.0; 128];
    let adj_buf: [16]NavNodeAdj = [NavNodeAdj{}; 16];
    let vecs_s: []f32 = vecs_buf[0..];
    let adj_s: []NavNodeAdj = adj_buf[0..];

    if (idx_init := cap.vector.NavGraphIndex.init(vecs_s, adj_s, dim, 8)) {
        let idx = idx_init;

        let rng: u64 = 0x20261122;
        let inserted_copy: [128]f32 = [0.0; 128];

        let i: usize = 0;
        while (i < n) : (i += 1) {
            let v: [8]f32 = [0.0; 8];
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                const val: f32 = nextRandUnitF32(&rng) * 3.0 + ((i as f32) * 0.5);
                v[d] = val;
                inserted_copy[i * dim + d] = val;
            }
            const assigned = idx.insert(v[0..], 16) ?? NAV_INVALID_NODE;
            expect_eq(assigned, i as u32);
        }

        expect_eq(idx.nodeCount(), n);
        expect_eq(idx.entryPoint(), 0 as u32);
        expect_eq(idx.edgeCount() > 0, true);

        // Every inserted vector must retrieve itself as top-1 with dist_sq == 0.
        let hits_buf: [3]NavGraphHit = [NavGraphHit{}; 3];
        let hits_s: []NavGraphHit = hits_buf[0..];
        let q: usize = 0;
        while (q < n) : (q += 1) {
            const q_vec: []const f32 = inserted_copy[q * dim .. (q + 1) * dim];
            const found = idx.search(q_vec, 3, 16, hits_s) ?? 0;
            expect_eq(found, 3 as usize);
            expect_eq(hits_buf[0].node_id, q as u32);
            expect_eq(hits_buf[0].dist_sq < 0.00001, true);
            expect_eq(hits_buf[0].dist_sq <= hits_buf[1].dist_sq, true);
            expect_eq(hits_buf[1].dist_sq <= hits_buf[2].dist_sq, true);
        }
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 2: 64D Clustered Vectors Recall@5 >= 0.90 vs Brute-Force Ground Truth
// ============================================================================

fn testHighDim64ClusteredRecallAt5()! {
    const dim: usize = 64;
    const n_clusters: usize = 6;
    const per_cluster: usize = 16;
    const n_nodes: usize = 96; // 6 * 16

    let vecs_buf: [6144]f32 = [0.0; 6144]; // 96 * 64
    let adj_buf: [96]NavNodeAdj = [NavNodeAdj{}; 96];
    let vecs_s: []f32 = vecs_buf[0..];
    let adj_s: []NavNodeAdj = adj_buf[0..];

    if (idx_init := NavGraphIndex.init(vecs_s, adj_s, dim, 12)) {
        let idx = idx_init;

        let rng: u64 = 0x4AFEBABE12345678;
        let centers: [384]f32 = [0.0; 384]; // 6 * 64
        let c: usize = 0;
        while (c < n_clusters) : (c += 1) {
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                centers[c * dim + d] = nextRandUnitF32(&rng) * 4.0;
            }
        }

        // Insert 96 vectors interleaved across clusters to form a rich small-world graph.
        let p: usize = 0;
        while (p < per_cluster) : (p += 1) {
            let cl: usize = 0;
            while (cl < n_clusters) : (cl += 1) {
                let v: [64]f32 = [0.0; 64];
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    v[d] = centers[cl * dim + d] + nextRandUnitF32(&rng) * 0.55;
                }
                const id_opt = idx.insert(v[0..], 48);
                expect_eq(id_opt != null, true);
            }
        }
        expect_eq(idx.nodeCount(), n_nodes);

        // Evaluate 18 queries (3 per cluster) against brute-force Top-5 ground truth.
        const k: usize = 5;
        let total_hits: usize = 0;
        const total_queries: usize = 18;

        let hits_buf: [5]NavGraphHit = [NavGraphHit{}; 5];
        let hits_s: []NavGraphHit = hits_buf[0..];
        let truth_ids_buf: [5]u32 = [0; 5];
        let truth_dists_buf: [5]f32 = [0.0; 5];
        let truth_ids_s: []u32 = truth_ids_buf[0..];
        let truth_dists_s: []f32 = truth_dists_buf[0..];

        let qi: usize = 0;
        while (qi < total_queries) : (qi += 1) {
            const cl: usize = qi % n_clusters;
            let query: [64]f32 = [0.0; 64];
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                query[d] = centers[cl * dim + d] + nextRandUnitF32(&rng) * 0.6;
            }

            const n_found = idx.search(query[0..], k, 48, hits_s) ?? 0;
            expect_eq(n_found, k);

            // Verify strictly ascending dist_sq output ordering.
            let r: usize = 1;
            while (r < k) : (r += 1) {
                expect_eq(hits_buf[r - 1].dist_sq <= hits_buf[r].dist_sq, true);
            }

            bruteForceTopK(query[0..], vecs_buf[0..], n_nodes, dim, k, truth_ids_s, truth_dists_s);
            total_hits += countHitsOverlap(hits_buf[0..], truth_ids_buf[0..], k);
        }

        const recall: f64 = (total_hits as f64) / ((total_queries * k) as f64);
        expect_eq(recall >= 0.90, true);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 3: Bidirectional Edge Symmetry & Max-Degree Cap Guard
// ============================================================================

fn testBidirectionalSymmetryAndMaxDegreeGuard()! {
    // Part A: Before pruning triggers (7 nodes, max_degree = 8), every edge is strictly bidirectional.
    {
        const dim: usize = 4;
        let vecs_buf: [28]f32 = [0.0; 28];
        let adj_buf: [7]NavNodeAdj = [NavNodeAdj{}; 7];
        let vecs_s: []f32 = vecs_buf[0..];
        let adj_s: []NavNodeAdj = adj_buf[0..];

        if (idx_init := NavGraphIndex.init(vecs_s, adj_s, dim, 8)) {
            let idx = idx_init;
            let rng: u64 = 0x11223344;
            let i: usize = 0;
            while (i < 7) : (i += 1) {
                let v: [4]f32 = [
                    nextRandUnitF32(&rng),
                    nextRandUnitF32(&rng),
                    nextRandUnitF32(&rng),
                    nextRandUnitF32(&rng),
                ];
                * = idx.insert(v[0..], 16);
            }

            expect_eq(idx.nodeCount(), 7 as usize);
            expect_eq(idx.edgeCount(), 42 as usize); // Complete K_7 undirected graph = 7 * 6 directed edges

            let u: usize = 0;
            while (u < 7) : (u += 1) {
                expect_eq(adj_buf[u].occupied, true);
                expect_eq(adj_buf[u].degree, 6 as u8);
                let e: usize = 0;
                while (e < 6) : (e += 1) {
                    const nb: u32 = adj_buf[u].neighbors[e];
                    expect_eq(nb != (u as u32), true);
                    expect_eq(hasNeighbor(adj_buf[nb as usize], u as u32), true);
                }
            }
        } else {
            expect_eq(true, false);
        }
    }

    // Part B: Heavy pruning (40 nodes in 16D, max_degree = 4): strict cap <= 4, no self-loops, no duplicates.
    {
        const dim: usize = 16;
        const n: usize = 40;
        const max_deg: usize = 4;
        let vecs_buf: [640]f32 = [0.0; 640];
        let adj_buf: [40]NavNodeAdj = [NavNodeAdj{}; 40];
        let vecs_s: []f32 = vecs_buf[0..];
        let adj_s: []NavNodeAdj = adj_buf[0..];

        if (idx_init := NavGraphIndex.init(vecs_s, adj_s, dim, max_deg)) {
            let idx = idx_init;
            let rng: u64 = 0x55667788;
            let i: usize = 0;
            while (i < n) : (i += 1) {
                let v: [16]f32 = [0.0; 16];
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    v[d] = nextRandUnitF32(&rng) * 2.0;
                }
                expect_eq(idx.insert(v[0..], 24) != null, true);
            }

            let sum_deg: usize = 0;
            let u: usize = 0;
            while (u < n) : (u += 1) {
                const adj = adj_buf[u];
                expect_eq(adj.occupied, true);
                const deg: usize = adj.degree as usize;
                expect_eq(deg >= 1, true);
                expect_eq(deg <= max_deg, true);
                sum_deg += deg;

                // Check no self-loops, valid IDs, and no duplicate neighbors.
                let a: usize = 0;
                while (a < deg) : (a += 1) {
                    const nb_a: u32 = adj.neighbors[a];
                    expect_eq(nb_a != (u as u32), true);
                    expect_eq((nb_a as usize) < n, true);
                    let b: usize = a + 1;
                    while (b < deg) : (b += 1) {
                        expect_eq(nb_a != adj.neighbors[b], true);
                    }
                }
            }
            expect_eq(idx.edgeCount(), sum_deg);
            expect_eq(sum_deg <= n * max_deg, true);
        } else {
            expect_eq(true, false);
        }
    }
}

// ============================================================================
// Test 4: ef_search Expansion Improves Recall
// ============================================================================

fn testEfSearchExpansionImprovesRecall()! {
    const dim: usize = 32;
    const n_clusters: usize = 8;
    const per_cluster: usize = 10;
    const n_nodes: usize = 80;

    let vecs_buf: [2560]f32 = [0.0; 2560]; // 80 * 32
    let adj_buf: [80]NavNodeAdj = [NavNodeAdj{}; 80];
    let vecs_s: []f32 = vecs_buf[0..];
    let adj_s: []NavNodeAdj = adj_buf[0..];

    // Moderate degree (6) so narrow beam (ef=1) can stall in local minima while wide beam (ef=40) succeeds.
    if (idx_init := NavGraphIndex.init(vecs_s, adj_s, dim, 6)) {
        let idx = idx_init;

        let rng: u64 = 0x5EADBEEF88776655;
        let centers: [256]f32 = [0.0; 256]; // 8 * 32
        let c: usize = 0;
        while (c < n_clusters) : (c += 1) {
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                centers[c * dim + d] = nextRandUnitF32(&rng) * 5.0;
            }
        }

        // Insert cluster-by-cluster to create realistic multi-hop cluster bridges.
        let cl: usize = 0;
        while (cl < n_clusters) : (cl += 1) {
            let p: usize = 0;
            while (p < per_cluster) : (p += 1) {
                let v: [32]f32 = [0.0; 32];
                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    v[d] = centers[cl * dim + d] + nextRandUnitF32(&rng) * 0.6;
                }
                expect_eq(idx.insert(v[0..], 24) != null, true);
            }
        }

        const total_queries: usize = 24;
        let narrow_hits: usize = 0;
        let wide_hits: usize = 0;

        let hit_buf: [1]NavGraphHit = [NavGraphHit{}; 1];
        let hit_s: []NavGraphHit = hit_buf[0..];
        let truth_id: [1]u32 = [0];
        let truth_dist: [1]f32 = [0.0];
        let truth_id_s: []u32 = truth_id[0..];
        let truth_dist_s: []f32 = truth_dist[0..];

        let qi: usize = 0;
        while (qi < total_queries) : (qi += 1) {
            const target_cl: usize = qi % n_clusters;
            let query: [32]f32 = [0.0; 32];
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                query[d] = centers[target_cl * dim + d] + nextRandUnitF32(&rng) * 0.65;
            }

            bruteForceTopK(query[0..], vecs_buf[0..], n_nodes, dim, 1, truth_id_s, truth_dist_s);

            // Narrow beam: ef_search = 1 (pure greedy hill-climbing)
            const n1 = idx.search(query[0..], 1, 1, hit_s) ?? 0;
            expect_eq(n1, 1 as usize);
            if (hit_buf[0].node_id == truth_id[0]) {
                narrow_hits += 1;
            }

            // Wide beam: ef_search = 40
            const n2 = idx.search(query[0..], 1, 40, hit_s) ?? 0;
            expect_eq(n2, 1 as usize);
            if (hit_buf[0].node_id == truth_id[0]) {
                wide_hits += 1;
            }
        }

        // Wider ef_search must match or beat narrow ef_search and achieve high recall (>= 0.85).
        expect_eq(wide_hits >= narrow_hits, true);
        const wide_recall: f64 = (wide_hits as f64) / (total_queries as f64);
        expect_eq(wide_recall >= 0.85, true);
    } else {
        expect_eq(true, false);
    }
}

// ============================================================================
// Test 5: Empty Graph, Invalid Dimensions, Full Pool & Non-Finite Guards
// ============================================================================

fn testEmptyInvalidDimFullPoolAndNonFiniteGuards()! {
    let vecs_buf: [8]f32 = [0.0; 8];
    let adj_buf: [2]NavNodeAdj = [NavNodeAdj{}; 2];
    let vecs_s: []f32 = vecs_buf[0..];
    let adj_s: []NavNodeAdj = adj_buf[0..];

    // Init validation guards.
    expect_eq(NavGraphIndex.init(vecs_s, adj_s, 0, 4) == null, true);                    // dim == 0
    expect_eq(NavGraphIndex.init(vecs_s, adj_s, 4, 0) == null, true);                    // max_degree == 0
    expect_eq(NavGraphIndex.init(vecs_s, adj_s, 4, NAV_MAX_DEGREE + 1) == null, true);   // max_degree > 16
    expect_eq(NavGraphIndex.init(vecs_buf[0..7], adj_s, 4, 4) == null, true);            // undersized vectors_buf (7 < 2*4)

    if (idx_init := NavGraphIndex.init(vecs_s, adj_s, 4, 4)) {
        let idx = idx_init;
        expect_eq(idx.nodeCount(), 0 as usize);
        expect_eq(idx.edgeCount(), 0 as usize);
        expect_eq(idx.entryPoint(), NAV_INVALID_NODE);

        // Search on empty graph safely returns 0.
        const q_ok: [4]f32 = [1.0, 2.0, 3.0, 4.0];
        let hits_buf: [2]NavGraphHit = [NavGraphHit{}; 2];
        let hits_s: []NavGraphHit = hits_buf[0..];
        expect_eq(idx.search(q_ok[0..], 2, 8, hits_s) ?? 99, 0 as usize);
        expect_eq(idx.search(q_ok[0..], 0, 8, hits_s) ?? 99, 0 as usize);

        // Insert guards: dimension mismatch & non-finite values.
        const bad_dim: [3]f32 = [1.0, 2.0, 3.0];
        const nan_vec: [4]f32 = [1.0, std.math.nan(f32), 0.0, 0.0];
        const inf_vec: [4]f32 = [1.0, std.math.inf(f32), 0.0, 0.0];
        expect_eq(idx.insert(bad_dim[0..], 8) == null, true);
        expect_eq(idx.insert(nan_vec[0..], 8) == null, true);
        expect_eq(idx.insert(inf_vec[0..], 8) == null, true);
        expect_eq(idx.nodeCount(), 0 as usize);

        // Fill capacity (2 slots), then verify 3rd insert returns null.
        const v0: [4]f32 = [1.0, 0.0, 0.0, 0.0];
        const v1: [4]f32 = [0.0, 1.0, 0.0, 0.0];
        const v2: [4]f32 = [0.0, 0.0, 1.0, 0.0];
        expect_eq(idx.insert(v0[0..], 8) ?? NAV_INVALID_NODE, 0 as u32);
        expect_eq(idx.insert(v1[0..], 8) ?? NAV_INVALID_NODE, 1 as u32);
        expect_eq(idx.insert(v2[0..], 8) == null, true);
        expect_eq(idx.nodeCount(), 2 as usize);

        // Search guards: dim mismatch, NaN/Inf query, undersized out_hits.
        expect_eq(idx.search(bad_dim[0..], 1, 8, hits_s) == null, true);
        expect_eq(idx.search(nan_vec[0..], 1, 8, hits_s) == null, true);
        expect_eq(idx.search(inf_vec[0..], 1, 8, hits_s) == null, true);
        expect_eq(idx.search(q_ok[0..], 3, 8, hits_s) == null, true); // out_hits.len (2) < k (3)
    } else {
        expect_eq(true, false);
    }
}

fn main() {
    test("1. vector.nav_graph: insert and exact self-search with dist_sq == 0", () => {
        testInsertAndExactSelfSearchZeroDist()?;
    });
    test("2. vector.nav_graph: 64D clustered vectors Recall@5 >= 0.90 vs brute-force", () => {
        testHighDim64ClusteredRecallAt5()?;
    });
    test("3. vector.nav_graph: bidirectional edge symmetry and max_degree cap guard", () => {
        testBidirectionalSymmetryAndMaxDegreeGuard()?;
    });
    test("4. vector.nav_graph: ef_search expansion improves recall", () => {
        testEfSearchExpansionImprovesRecall()?;
    });
    test("5. vector.nav_graph: empty graph, invalid dim, full pool and non-finite guards", () => {
        testEmptyInvalidDimFullPoolAndNonFiniteGuards()?;
    });
}
