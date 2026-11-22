// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Vamana/NSW Navigable Graph ANN using @l2sq) | Created: 2026-11-22
// Zero-heap-allocation Vamana / NSW navigable small-world approximate nearest neighbor (ANN)
// graph index backed by caller-owned flat vector and fixed-degree adjacency pools,
// powered directly by CP's native 8-lane SIMD `@l2sq` distance intrinsic.

const std = @import("std");

/// Maximum out-degree supported per node in the fixed-size adjacency list.
pub const NAV_MAX_DEGREE: usize = 16;

/// Sentinel value representing an empty / invalid graph node identifier.
pub const NAV_INVALID_NODE: u32 = 0xFFFFFFFF;

/// Maximum number of nodes supported by the stack-allocated visited bitset (256 * 64).
const NAV_VISITED_WORDS: usize = 256;
const NAV_MAX_NODES: usize = NAV_VISITED_WORDS * 64;

/// Maximum internal beam width for zero-allocation stack candidate storage.
const NAV_MAX_BEAM: usize = 128;

/// Single approximate nearest neighbor search hit returned by `NavGraphIndex.search`.
pub struct NavGraphHit {
    node_id: u32 = NAV_INVALID_NODE;
    dist_sq: f32 = 0.0;
}

/// Fixed-size adjacency list slot for a single graph node.
pub struct NavNodeAdj {
    neighbors: [16]u32 = [NAV_INVALID_NODE; 16];
    degree: u8 = 0;
    occupied: bool = false;
}

// ============================================================================
// Internal Validation, Ordering & Bitset Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isFiniteSliceF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) {
            return false;
        }
    }
    return true;
}

fn clampBeam(ef: usize): usize {
    if (ef == 0) return 1;
    if (ef > NAV_MAX_BEAM) return NAV_MAX_BEAM;
    return ef;
}

fn hitBetter(a_dist: f32, a_id: u32, b_dist: f32, b_id: u32): bool {
    if (a_dist < b_dist) return true;
    if (a_dist > b_dist) return false;
    return a_id < b_id;
}

fn isVisited(visited: []const u64, node_id: u32): bool {
    const idx: usize = node_id as usize;
    const word: usize = idx >> 6;
    const bit: u6 = @intCast(idx & 63);
    return (visited[word] & ((1 as u64) << bit)) != 0;
}

fn markVisited(visited: *[]u64, node_id: u32) {
    const idx: usize = node_id as usize;
    const word: usize = idx >> 6;
    const bit: u6 = @intCast(idx & 63);
    visited[word] = visited[word] | ((1 as u64) << bit);
}

/// Inserts `(node_id, dist_sq)` into a sorted ascending candidate beam of capacity `eff_ef`.
/// Maintains parallel `expanded` flags so the greedy beam search tracks unexpanded nodes.
fn insertBeamCandidate(
    beam: *[]NavGraphHit,
    expanded: *[]bool,
    beam_len: *usize,
    eff_ef: usize,
    node_id: u32,
    dist_sq: f32,
) {
    const cur_len: usize = beam_len.*;
    if (cur_len == eff_ef) {
        const last = beam[cur_len - 1];
        if (!hitBetter(dist_sq, node_id, last.dist_sq, last.node_id)) {
            return;
        }
    }

    // Locate insertion position in ascending `(dist_sq, node_id)` order.
    let pos: usize = 0;
    while (pos < cur_len) : (pos += 1) {
        if (hitBetter(dist_sq, node_id, beam[pos].dist_sq, beam[pos].node_id)) {
            break;
        }
    }

    const new_len: usize = if (cur_len < eff_ef) cur_len + 1 else eff_ef;
    let shift: usize = new_len - 1;
    while (shift > pos) : (shift -= 1) {
        beam[shift] = beam[shift - 1];
        expanded[shift] = expanded[shift - 1];
    }

    beam[pos] = NavGraphHit{ node_id: node_id, dist_sq: dist_sq };
    expanded[pos] = false;
    beam_len.* = new_len;
}

// ============================================================================
// NavGraphIndex (Zero-Alloc Vamana / NSW Single-Layer Navigable Graph)
// ============================================================================

/// Zero-heap-allocation Vamana / NSW navigable graph ANN index.
/// All vector storage and node adjacency slots are owned by the caller.
pub struct NavGraphIndex {
    vectors_buf: []f32;
    adj_buf: []NavNodeAdj;
    dim: usize = 0;
    max_degree: usize = NAV_MAX_DEGREE;
    node_count: usize = 0;
    entry_point: u32 = NAV_INVALID_NODE;

    /// Initializes a navigable graph index over caller-provided flat vector storage
    /// (`vectors_buf.len >= adj_buf.len * dim`) and adjacency slots (`adj_buf`).
    ///
    /// Rejects `dim == 0`, `max_degree == 0 || max_degree > NAV_MAX_DEGREE`,
    /// `adj_buf.len > 16384`, or insufficient/overflowing `vectors_buf`.
    static fn init(
        vectors_buf: *[]f32,
        adj_buf: *[]NavNodeAdj,
        dim: usize,
        max_degree: usize,
    ): NavGraphIndex? {
        if (dim == 0) return null;
        if (max_degree == 0 || max_degree > NAV_MAX_DEGREE) return null;
        if (adj_buf.len > NAV_MAX_NODES) return null;
        if (adj_buf.len > 0 && adj_buf.len > std.math.maxInt(usize) / dim) return null;
        if (vectors_buf.len < adj_buf.len * dim) return null;

        let i: usize = 0;
        while (i < adj_buf.len) : (i += 1) {
            adj_buf[i] = NavNodeAdj{};
        }

        return NavGraphIndex{
            vectors_buf: vectors_buf,
            adj_buf: adj_buf,
            dim: dim,
            max_degree: max_degree,
            node_count: 0,
            entry_point: NAV_INVALID_NODE,
        };
    }

    /// Returns the flat `f32` vector slice for `node_id`.
    fn vectorOf(node_id: u32): []const f32 {
        const base: usize = (node_id as usize) * _.dim;
        return _.vectors_buf[base .. base + _.dim];
    }

    /// Computes squared Euclidean distance from `query` to `node_id` via native 8-lane SIMD `@l2sq`.
    fn distSqToNode(query: []const f32, node_id: u32): f32 {
        const d: f32 = @l2sq(query, _.vectorOf(node_id));
        if (d < 0.0) return 0.0;
        return d;
    }

    /// Executes Vamana/NSW greedy beam search starting from `_.entry_point` with beam width `eff_ef`.
    /// Writes up to `eff_ef` closest visited nodes sorted ascending by `(dist_sq, node_id)` into `out_beam`.
    fn greedyBeamSearch(
        query: []const f32,
        eff_ef: usize,
        out_beam: *[]NavGraphHit,
    ): usize {
        if (_.node_count == 0 || _.entry_point == NAV_INVALID_NODE) {
            return 0;
        }

        let visited_buf: [NAV_VISITED_WORDS]u64 = undefined;
        const words_needed: usize = (_.node_count + 63) >> 6;
        let w: usize = 0;
        while (w < words_needed) : (w += 1) {
            visited_buf[w] = 0;
        }
        let visited_s: []u64 = visited_buf[0..words_needed];

        let expanded_buf: [NAV_MAX_BEAM]bool = [false; NAV_MAX_BEAM];
        let expanded_s: []bool = expanded_buf[0..eff_ef];
        let beam_len: usize = 0;

        const ep: u32 = _.entry_point;
        markVisited(visited_s, ep);
        const ep_dist: f32 = _.distSqToNode(query, ep);
        insertBeamCandidate(out_beam, expanded_s, &beam_len, eff_ef, ep, ep_dist);

        while (true) {
            // Find the closest unexpanded candidate in the active beam.
            let next_pos: usize = eff_ef;
            let i: usize = 0;
            while (i < beam_len) : (i += 1) {
                if (!expanded_s[i]) {
                    next_pos = i;
                    break;
                }
            }
            if (next_pos == eff_ef) {
                break;
            }

            expanded_s[next_pos] = true;
            const cur_id: u32 = out_beam[next_pos].node_id;
            const cur_adj = _.adj_buf[cur_id as usize];
            const deg: usize = cur_adj.degree as usize;

            let n_i: usize = 0;
            while (n_i < deg) : (n_i += 1) {
                const nb_id: u32 = cur_adj.neighbors[n_i];
                if (nb_id == NAV_INVALID_NODE || (nb_id as usize) >= _.node_count) {
                    continue;
                }
                if (isVisited(visited_buf[0..words_needed], nb_id)) {
                    continue;
                }
                markVisited(visited_s, nb_id);

                const nb_dist: f32 = _.distSqToNode(query, nb_id);
                insertBeamCandidate(out_beam, expanded_s, &beam_len, eff_ef, nb_id, nb_dist);
            }
        }

        return beam_len;
    }

    /// Vamana / HNSW diverse neighbor selection (`RobustPrune` heuristic):
    /// Given `candidates[0..cand_len]` sorted ascending by `(dist_sq, node_id)` relative to `origin_vec`,
    /// selects up to `_.max_degree` neighbors prioritizing angularly diverse (non-occluded) links so
    /// inter-cluster long-range bridges are preserved, then backfills remaining slots with closest candidates.
    fn selectNeighborsRobust(
        candidates: []const NavGraphHit,
        cand_len: usize,
        out_neighbors: *[]u32,
    ): usize {
        const limit: usize = if (cand_len < _.max_degree) cand_len else _.max_degree;
        if (limit == 0) return 0;

        let chosen_flag: [NAV_MAX_BEAM]bool = [false; NAV_MAX_BEAM];
        let sel_count: usize = 0;

        // Phase 1: Keep non-occluded candidates (no already-selected neighbor `s` is closer to `c` than `origin` is).
        let i: usize = 0;
        while (i < cand_len && sel_count < limit) : (i += 1) {
            const c_id: u32 = candidates[i].node_id;
            const c_dist: f32 = candidates[i].dist_sq;
            const c_vec: []const f32 = _.vectorOf(c_id);

            let occluded: bool = false;
            let s: usize = 0;
            while (s < sel_count) : (s += 1) {
                const s_vec: []const f32 = _.vectorOf(out_neighbors[s]);
                let sc_dist: f32 = @l2sq(s_vec, c_vec);
                if (sc_dist < 0.0) sc_dist = 0.0;
                if (sc_dist < c_dist) {
                    occluded = true;
                    break;
                }
            }

            if (!occluded) {
                out_neighbors[sel_count] = c_id;
                sel_count += 1;
                chosen_flag[i] = true;
            }
        }

        // Phase 2: Backfill remaining degree slots with closest discarded candidates.
        if (sel_count < limit) {
            let j: usize = 0;
            while (j < cand_len && sel_count < limit) : (j += 1) {
                if (!chosen_flag[j]) {
                    out_neighbors[sel_count] = candidates[j].node_id;
                    sel_count += 1;
                }
            }
        }

        return sel_count;
    }

    /// Adds a directed edge `nb_id -> new_id`, pruning with Vamana/HNSW diverse selection when
    /// `nb_id` is already at `_.max_degree` so both close local links and long-range bridges survive.
    fn addReverseEdgeWithPrune(*_, nb_id: u32, new_id: u32) {
        const nb_idx: usize = nb_id as usize;
        const deg: usize = _.adj_buf[nb_idx].degree as usize;

        // Deduplicate: do not insert if already present or self-loop.
        if (nb_id == new_id) return;
        let i: usize = 0;
        while (i < deg) : (i += 1) {
            if (_.adj_buf[nb_idx].neighbors[i] == new_id) {
                return;
            }
        }

        if (deg < _.max_degree) {
            _.adj_buf[nb_idx].neighbors[deg] = new_id;
            _.adj_buf[nb_idx].degree = (deg + 1) as u8;
            return;
        }

        // Degree limit reached: collect all `deg + 1` candidates sorted by `@l2sq` to `nb_id`.
        const nb_vec: []const f32 = _.vectorOf(nb_id);
        let cands: [17]NavGraphHit = [NavGraphHit{}; 17];
        let cand_len: usize = 0;

        let j: usize = 0;
        while (j < deg) : (j += 1) {
            const cid: u32 = _.adj_buf[nb_idx].neighbors[j];
            let cdist: f32 = @l2sq(nb_vec, _.vectorOf(cid));
            if (cdist < 0.0) cdist = 0.0;
            cands[cand_len] = NavGraphHit{ node_id: cid, dist_sq: cdist };
            cand_len += 1;
        }

        let new_dist: f32 = @l2sq(nb_vec, _.vectorOf(new_id));
        if (new_dist < 0.0) new_dist = 0.0;
        cands[cand_len] = NavGraphHit{ node_id: new_id, dist_sq: new_dist };
        cand_len += 1;

        // Insertion-sort `cands[0..cand_len]` ascending by `(dist_sq, node_id)`.
        let a: usize = 1;
        while (a < cand_len) : (a += 1) {
            const key = cands[a];
            let b: usize = a;
            while (b > 0 && hitBetter(key.dist_sq, key.node_id, cands[b - 1].dist_sq, cands[b - 1].node_id)) : (b -= 1) {
                cands[b] = cands[b - 1];
            }
            cands[b] = key;
        }

        let pruned_buf: [16]u32 = [NAV_INVALID_NODE; 16];
        let pruned_s: []u32 = pruned_buf[0.._.max_degree];
        const kept: usize = _.selectNeighborsRobust(cands[0..cand_len], cand_len, pruned_s);

        let k: usize = 0;
        while (k < kept) : (k += 1) {
            _.adj_buf[nb_idx].neighbors[k] = pruned_buf[k];
        }
        _.adj_buf[nb_idx].degree = kept as u8;
    }

    /// Inserts a new vector into the navigable graph using greedy beam search (`ef_construct`),
    /// diverse Vamana/HNSW neighbor selection, bidirectional edge linking, and degree-capped pruning.
    /// Returns the assigned `node_id`, or `null` if full, dimension mismatched, or non-finite.
    fn insert(*_, vec: []const f32, ef_construct: usize): u32? {
        if (vec.len != _.dim) return null;
        if (_.node_count >= _.adj_buf.len) return null;
        if (!isFiniteSliceF32(vec)) return null;

        const new_idx: usize = _.node_count;
        const new_id: u32 = new_idx as u32;
        const dst_base: usize = new_idx * _.dim;

        // Copy vector into caller-owned flat buffer.
        let d: usize = 0;
        while (d < _.dim) : (d += 1) {
            _.vectors_buf[dst_base + d] = vec[d];
        }

        // First node becomes the entry point.
        if (_.node_count == 0) {
            _.adj_buf[0] = NavNodeAdj{ occupied: true };
            _.node_count = 1;
            _.entry_point = 0;
            return 0;
        }

        // Search existing graph for closest neighbor candidates before publishing `new_id`.
        const want_ef: usize = if (ef_construct > _.max_degree) ef_construct else _.max_degree;
        const eff_ef: usize = clampBeam(want_ef);
        let beam_buf: [NAV_MAX_BEAM]NavGraphHit = [NavGraphHit{}; NAV_MAX_BEAM];
        let beam_s: []NavGraphHit = beam_buf[0..eff_ef];
        const found: usize = _.greedyBeamSearch(vec, eff_ef, beam_s);

        let sel_buf: [16]u32 = [NAV_INVALID_NODE; 16];
        let sel_s: []u32 = sel_buf[0.._.max_degree];
        const link_count: usize = _.selectNeighborsRobust(beam_buf[0..found], found, sel_s);

        let new_adj = NavNodeAdj{ occupied: true, degree: link_count as u8 };
        let i: usize = 0;
        while (i < link_count) : (i += 1) {
            new_adj.neighbors[i] = sel_buf[i];
        }
        _.adj_buf[new_idx] = new_adj;
        _.node_count += 1;

        // Add reverse edges and prune neighbors that exceed `max_degree`.
        let j: usize = 0;
        while (j < link_count) : (j += 1) {
            _.addReverseEdgeWithPrune(new_adj.neighbors[j], new_id);
        }

        return new_id;
    }

    /// Searches for the `k` approximate nearest neighbors of `query` using greedy beam search
    /// with beam width `max(ef_search, k)` and native `@l2sq` SIMD distance evaluation.
    /// Writes up to `min(k, nodeCount())` results sorted ascending by `(dist_sq, node_id)` into `out_hits`.
    /// Returns the number of hits written, or `null` on invalid dimension, non-finite query, or undersized `out_hits`.
    fn search(
        query: []const f32,
        k: usize,
        ef_search: usize,
        out_hits: *[]NavGraphHit,
    ): usize? {
        if (query.len != _.dim) return null;
        if (!isFiniteSliceF32(query)) return null;
        if (out_hits.len < k) return null;
        if (_.node_count == 0 || k == 0) return 0;

        const want_ef: usize = if (ef_search > k) ef_search else k;
        const eff_ef: usize = clampBeam(want_ef);
        let beam_buf: [NAV_MAX_BEAM]NavGraphHit = [NavGraphHit{}; NAV_MAX_BEAM];
        let beam_s: []NavGraphHit = beam_buf[0..eff_ef];
        const found: usize = _.greedyBeamSearch(query, eff_ef, beam_s);

        const out_n: usize = if (found < k) found else k;
        let i: usize = 0;
        while (i < out_n) : (i += 1) {
            out_hits[i] = beam_buf[i];
        }
        return out_n;
    }

    /// Returns the number of inserted nodes in the graph.
    fn nodeCount(): usize {
        return _.node_count;
    }

    /// Returns the total number of directed edges stored across all active nodes.
    fn edgeCount(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            total += _.adj_buf[i].degree as usize;
        }
        return total;
    }

    /// Returns the current graph entry point node ID, or `NAV_INVALID_NODE` if the graph is empty.
    fn entryPoint(): u32 {
        return _.entry_point;
    }
}
