// Origin: Pure-CP Stdlib (Zero-Alloc Predicate-Aware Filtered HNSW & Adaptive Selectivity Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation ACORN / Filtered-DiskANN style predicate-aware navigable graph ANN:
//   - Fixed inline node metadata & bounded bidirectional adjacency pool (`FHNSW_MAX_NODES = 256`, `M <= 16`)
//     backed by caller-owned flat `f32` embedding buffer (0 heap allocation)
//   - Powered by native CP `@dot`, `@l2sq`, and `@popCount` primitives
//   - Single-stage predicate-aware graph traversal: traverses non-matching & tombstoned nodes to preserve
//     graph connectivity while admitting only predicate-matching live nodes into the Top-K result set
//   - Adaptive selectivity routing via `@popCount`: automatically falls back to exact masked `@dot` scan
//     when `selectivity_bps < exact_fallback_bps` to guarantee 100% recall under rare filters

const std = @import("std");

// ============================================================================
// 1. Constants & Public Enums
// ============================================================================

/// Maximum nodes supported inline by `FilteredHnswIndex`.
pub const FHNSW_MAX_NODES: usize = 256;

/// Maximum bidirectional out-degree per graph node (`M <= 16`).
pub const FHNSW_MAX_DEGREE: usize = 16;

/// Number of `u64` words in the 256-bit candidate & visited bitsets.
const FHNSW_BITSET_WORDS: usize = 4;

/// Maximum beam width used for stack-allocated greedy search queues.
const FHNSW_MAX_BEAM: usize = 64;

/// Sentinel slot index representing an empty adjacency entry.
pub const FHNSW_INVALID_SLOT: u16 = 0xFFFF;

/// Wildcard `tenant_id` matching any tenant in `FilteredPredicate`.
pub const FHNSW_ANY_TENANT: u32 = 0;

/// Execution path selected by `FilteredHnswIndex.search`.
pub enum FilteredSearchPath {
    graph_filtered,
    exact_fallback,
    empty_result,
}

/// Status code returned by `FilteredHnswIndex.insert`.
pub enum FilteredGraphStatus {
    ok,
    invalid_dimension,
    invalid_vector,
    index_full,
    duplicate_doc_id,
    node_not_found,
}

// ============================================================================
// 2. Public Data Models
// ============================================================================

/// Configuration for `FilteredHnswIndex`.
pub struct FilteredGraphConfig {
    dim: usize = 0;
    max_nodes: usize = FHNSW_MAX_NODES;
    max_degree: usize = 12;
    ef_construction: usize = 32;
    exact_fallback_bps: u16 = 1500;
}

/// Metadata filter predicate evaluated against each node's `(tag_mask, tenant_id)`.
pub struct FilteredPredicate {
    required_tags: u64 = 0;
    forbidden_tags: u64 = 0;
    tenant_id: u32 = FHNSW_ANY_TENANT;

    /// Returns `true` iff a live node with `(tag_mask, node_tenant)` satisfies all constraints.
    fn matches(tag_mask: u64, node_tenant: u32): bool {
        if (_.tenant_id != FHNSW_ANY_TENANT && node_tenant != _.tenant_id) {
            return false;
        }
        if ((tag_mask & _.required_tags) != _.required_tags) {
            return false;
        }
        if ((tag_mask & _.forbidden_tags) != 0) {
            return false;
        }
        return true;
    }
}

/// Inline metadata and bounded adjacency list for one graph node.
pub struct FilteredGraphNode {
    occupied: bool = false;
    deleted: bool = false;
    doc_id: u64 = 0;
    tenant_id: u32 = 0;
    tag_mask: u64 = 0;
    neighbors: [FHNSW_MAX_DEGREE]u16 = [FHNSW_INVALID_SLOT; FHNSW_MAX_DEGREE];
    degree: u8 = 0;
}

/// Single Top-K result hit returned by `FilteredHnswIndex.search`.
pub struct FilteredSearchHit {
    doc_id: u64 = 0;
    slot_idx: u16 = FHNSW_INVALID_SLOT;
    tenant_id: u32 = 0;
    tag_mask: u64 = 0;
    similarity: f32 = 0.0;
    l2_sq_dist: f32 = 0.0;
}

/// Diagnostic execution report returned by `FilteredHnswIndex.search`.
pub struct FilteredSearchReport {
    path: FilteredSearchPath = FilteredSearchPath.empty_result;
    hit_count: usize = 0;
    live_nodes: usize = 0;
    matching_candidates: usize = 0;
    selectivity_bps: u16 = 0;
    visited_nodes: usize = 0;
    distance_evals: usize = 0;
}

/// Index-wide telemetry snapshot returned by `FilteredHnswIndex.stats`.
pub struct FilteredGraphStats {
    total_slots: usize = 0;
    live_nodes: usize = 0;
    deleted_nodes: usize = 0;
    total_edges: usize = 0;
    capacity: usize = 0;
    dim: usize = 0;
}

// ============================================================================
// 3. Internal Math, Bitset & Candidate Queue Helpers
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

fn normalizeInto(src: []const f32, dst: *[]f32, dst_offset: usize): bool {
    const norm_sq: f32 = @dot(src, src);
    if (!isFiniteF32(norm_sq)) {
        return false;
    }
    if (norm_sq <= 1.0e-12) {
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            dst[dst_offset + i] = 0.0;
        }
        return true;
    }
    const inv_norm: f32 = 1.0 / std.math.sqrt(norm_sq);
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_offset + i] = src[i] * inv_norm;
    }
    return true;
}

fn bitGet(words: [FHNSW_BITSET_WORDS]u64, idx: usize): bool {
    const w: usize = idx >> 6;
    const b: u6 = (idx & 63) as u6;
    return (words[w] & ((1 as u64) << b)) != 0;
}

fn bitSet(words: *[FHNSW_BITSET_WORDS]u64, idx: usize) {
    const w: usize = idx >> 6;
    const b: u6 = (idx & 63) as u6;
    words[w] = words[w] | ((1 as u64) << b);
}

fn hitBetter(a_sim: f32, a_id: u64, b_sim: f32, b_id: u64): bool {
    if (a_sim > b_sim) return true;
    if (a_sim < b_sim) return false;
    return a_id < b_id;
}

fn insertSortedHit(
    hits: *[]FilteredSearchHit,
    hit_len: *usize,
    max_k: usize,
    cand: FilteredSearchHit,
) {
    const cur_len: usize = hit_len.*;
    // Find insertion position in descending similarity order.
    let pos: usize = cur_len;
    while (pos > 0) {
        const prev = hits[pos - 1];
        if (hitBetter(cand.similarity, cand.doc_id, prev.similarity, prev.doc_id)) {
            pos -= 1;
        } else {
            break;
        }
    }
    if (pos >= max_k) {
        return;
    }
    const new_len: usize = if (cur_len < max_k) (cur_len + 1) else max_k;
    let shift: usize = new_len - 1;
    while (shift > pos) : (shift -= 1) {
        hits[shift] = hits[shift - 1];
    }
    hits[pos] = cand;
    hit_len.* = new_len;
}

struct BeamEntry {
    slot: u16 = FHNSW_INVALID_SLOT;
    sim: f32 = -2.0;
    expanded: bool = false;
}

fn insertBeamEntry(
    beam: *[FHNSW_MAX_BEAM]BeamEntry,
    beam_len: *usize,
    eff_ef: usize,
    slot: u16,
    sim: f32,
) {
    const cur_len: usize = beam_len.*;
    let pos: usize = cur_len;
    while (pos > 0) {
        if (sim > beam[pos - 1].sim || (sim == beam[pos - 1].sim && slot < beam[pos - 1].slot)) {
            pos -= 1;
        } else {
            break;
        }
    }
    if (pos >= eff_ef) {
        return;
    }
    const new_len: usize = if (cur_len < eff_ef) (cur_len + 1) else eff_ef;
    let shift: usize = new_len - 1;
    while (shift > pos) : (shift -= 1) {
        beam[shift] = beam[shift - 1];
    }
    beam[pos] = BeamEntry{
        slot: slot,
        sim: sim,
        expanded: false,
    };
    beam_len.* = new_len;
}

// ============================================================================
// 4. Zero-Alloc Predicate-Aware Filtered HNSW Graph Engine
// ============================================================================

/// Zero-heap-allocation predicate-aware navigable graph ANN index with adaptive
/// `@popCount` selectivity estimation and automatic exact SIMD `@dot` fallback.
pub struct FilteredHnswIndex {
    cfg: FilteredGraphConfig = FilteredGraphConfig{};
    vecs: []f32;
    nodes: [FHNSW_MAX_NODES]FilteredGraphNode = [FilteredGraphNode{}; FHNSW_MAX_NODES];
    count: usize = 0;
    live_count: usize = 0;
    entry_slot: u16 = FHNSW_INVALID_SLOT;

    /// Initializes a `FilteredHnswIndex` backed by caller-owned flat `vecs_flat_buf` (`len >= max_nodes * dim`).
    static fn init(cfg: FilteredGraphConfig, vecs_flat_buf: *[]f32): FilteredHnswIndex? {
        if (cfg.dim == 0 || cfg.max_nodes == 0 || cfg.max_nodes > FHNSW_MAX_NODES) {
            return null;
        }
        if (cfg.max_degree == 0 || cfg.max_degree > FHNSW_MAX_DEGREE) {
            return null;
        }
        if (vecs_flat_buf.len < cfg.max_nodes * cfg.dim) {
            return null;
        }
        return FilteredHnswIndex{
            cfg: cfg,
            vecs: vecs_flat_buf,
            nodes: [FilteredGraphNode{}; FHNSW_MAX_NODES],
            count: 0,
            live_count: 0,
            entry_slot: FHNSW_INVALID_SLOT,
        };
    }

    /// Returns a slice view of the normalized vector stored at `slot`.
    fn vecAt(slot: usize): []const f32 {
        const start: usize = slot * _.cfg.dim;
        return _.vecs[start..(start + _.cfg.dim)];
    }

    /// Finds the internal slot index for `doc_id`, or `null` if not found.
    fn findSlotByDocId(doc_id: u64): usize? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.nodes[i].occupied && _.nodes[i].doc_id == doc_id) {
                return i;
            }
        }
        return null;
    }

    /// Adds a directed edge `from -> to`, pruning the weakest neighbor if `from` is at `max_degree`.
    fn linkOneWay(*_, from_slot: usize, to_slot: usize) {
        if (from_slot == to_slot) return;
        const to_u16: u16 = to_slot as u16;
        const deg: usize = _.nodes[from_slot].degree as usize;

        // Deduplicate existing edge
        let i: usize = 0;
        while (i < deg) : (i += 1) {
            if (_.nodes[from_slot].neighbors[i] == to_u16) {
                return;
            }
        }

        if (deg < _.cfg.max_degree) {
            _.nodes[from_slot].neighbors[deg] = to_u16;
            _.nodes[from_slot].degree = (deg + 1) as u8;
            return;
        }

        // At capacity: replace the weakest (lowest similarity) neighbor if `to_slot` is closer.
        const from_vec = _.vecAt(from_slot);
        const new_sim: f32 = @dot(from_vec, _.vecAt(to_slot));
        let worst_idx: usize = 0;
        let worst_sim: f32 = @dot(from_vec, _.vecAt(_.nodes[from_slot].neighbors[0] as usize));

        let k: usize = 1;
        while (k < deg) : (k += 1) {
            const nb_sim: f32 = @dot(from_vec, _.vecAt(_.nodes[from_slot].neighbors[k] as usize));
            if (nb_sim < worst_sim) {
                worst_sim = nb_sim;
                worst_idx = k;
            }
        }
        if (new_sim > worst_sim) {
            _.nodes[from_slot].neighbors[worst_idx] = to_u16;
        }
    }

    /// Inserts a new vector with metadata `(doc_id, tag_mask, tenant_id)` and builds bounded bidirectional edges.
    fn insert(
        *_,
        doc_id: u64,
        embedding: []const f32,
        tag_mask: u64,
        tenant_id: u32,
    ): FilteredGraphStatus {
        if (embedding.len != _.cfg.dim) {
            return FilteredGraphStatus.invalid_dimension;
        }
        if (!isFiniteSliceF32(embedding)) {
            return FilteredGraphStatus.invalid_vector;
        }
        if (_.count >= _.cfg.max_nodes) {
            return FilteredGraphStatus.index_full;
        }
        if (_.findSlotByDocId(doc_id) != null) {
            return FilteredGraphStatus.duplicate_doc_id;
        }

        const slot: usize = _.count;
        let vecs_mut: []f32 = _.vecs;
        if (!normalizeInto(embedding, *vecs_mut, slot * _.cfg.dim)) {
            return FilteredGraphStatus.invalid_vector;
        }

        _.nodes[slot] = FilteredGraphNode{
            occupied: true,
            deleted: false,
            doc_id: doc_id,
            tenant_id: tenant_id,
            tag_mask: tag_mask,
            neighbors: [FHNSW_INVALID_SLOT; FHNSW_MAX_DEGREE],
            degree: 0,
        };
        _.count += 1;
        _.live_count += 1;

        if (slot == 0) {
            _.entry_slot = 0;
            return FilteredGraphStatus.ok;
        }

        // Greedy beam search from `entry_slot` to discover closest neighbors for bidirectional linking.
        const eff_ef: usize = if (_.cfg.ef_construction == 0) 1 else if (_.cfg.ef_construction > FHNSW_MAX_BEAM) FHNSW_MAX_BEAM else _.cfg.ef_construction;
        let visited: [FHNSW_BITSET_WORDS]u64 = [0; FHNSW_BITSET_WORDS];
        let beam: [FHNSW_MAX_BEAM]BeamEntry = [BeamEntry{}; FHNSW_MAX_BEAM];
        let beam_len: usize = 0;

        const new_vec = _.vecAt(slot);
        bitSet(*visited, slot);

        const ep: usize = _.entry_slot as usize;
        bitSet(*visited, ep);
        insertBeamEntry(*beam, *beam_len, eff_ef, _.entry_slot, @dot(new_vec, _.vecAt(ep)));

        while (true) {
            // Pick best unexpanded candidate in beam
            let next_b: usize? = null;
            let b_i: usize = 0;
            while (b_i < beam_len) : (b_i += 1) {
                if (!beam[b_i].expanded) {
                    next_b = b_i;
                    break;
                }
            }
            const cur_b = next_b ?? break;
            beam[cur_b].expanded = true;
            const cur_slot: usize = beam[cur_b].slot as usize;
            const deg: usize = _.nodes[cur_slot].degree as usize;

            let n_i: usize = 0;
            while (n_i < deg) : (n_i += 1) {
                const nb_u16 = _.nodes[cur_slot].neighbors[n_i];
                if (nb_u16 == FHNSW_INVALID_SLOT) continue;
                const nb: usize = nb_u16 as usize;
                if (bitGet(visited, nb)) continue;
                bitSet(*visited, nb);
                const sim: f32 = @dot(new_vec, _.vecAt(nb));
                insertBeamEntry(*beam, *beam_len, eff_ef, nb_u16, sim);
            }
        }

        // Connect up to `max_degree` closest discovered candidates bidirectionally.
        const link_cnt: usize = if (beam_len < _.cfg.max_degree) beam_len else _.cfg.max_degree;
        let c: usize = 0;
        while (c < link_cnt) : (c += 1) {
            const target: usize = beam[c].slot as usize;
            _.linkOneWay(slot, target);
            _.linkOneWay(target, slot);
        }

        return FilteredGraphStatus.ok;
    }

    /// Updates `tag_mask` for an existing live node identified by `doc_id`.
    fn updateNodeTags(*_, doc_id: u64, new_tag_mask: u64): bool {
        const slot = _.findSlotByDocId(doc_id) ?? return false;
        if (_.nodes[slot].deleted) {
            return false;
        }
        _.nodes[slot].tag_mask = new_tag_mask;
        return true;
    }

    /// Logically deletes `doc_id` (tombstoned: excluded from search hits while keeping graph edges navigable).
    fn markDeleted(*_, doc_id: u64): bool {
        const slot = _.findSlotByDocId(doc_id) ?? return false;
        if (_.nodes[slot].deleted) {
            return false;
        }
        _.nodes[slot].deleted = true;
        if (_.live_count > 0) {
            _.live_count -= 1;
        }
        return true;
    }

    /// Returns index-wide node and edge telemetry.
    fn stats(): FilteredGraphStats {
        let edges: usize = 0;
        let deleted: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (!_.nodes[i].occupied) continue;
            if (_.nodes[i].deleted) {
                deleted += 1;
            }
            edges += _.nodes[i].degree as usize;
        }
        return FilteredGraphStats{
            total_slots: _.count,
            live_nodes: _.live_count,
            deleted_nodes: deleted,
            total_edges: edges,
            capacity: _.cfg.max_nodes,
            dim: _.cfg.dim,
        };
    }

    /// Executes predicate-aware Top-K similarity search with `@popCount` selectivity estimation:
    ///   - 0 matching candidates -> `empty_result` fast short-circuit (0 distance evaluations)
    ///   - `selectivity_bps < exact_fallback_bps` -> `exact_fallback` masked SIMD `@dot` scan (100% recall)
    ///   - `selectivity_bps >= exact_fallback_bps` -> `graph_filtered` single-stage traversal
    fn search(
        query: []const f32,
        pred: FilteredPredicate,
        k: usize,
        ef_search: usize,
        out_hits: *[]FilteredSearchHit,
    ): FilteredSearchReport {
        let rep = FilteredSearchReport{
            path: FilteredSearchPath.empty_result,
            live_nodes: _.live_count,
        };
        if (query.len != _.cfg.dim || !isFiniteSliceF32(query) || k == 0 || out_hits.len == 0 || _.live_count == 0) {
            return rep;
        }

        // 1. Build 256-bit predicate match mask and count matching candidates via native `@popCount`.
        let match_bits: [FHNSW_BITSET_WORDS]u64 = [0; FHNSW_BITSET_WORDS];
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const nd = _.nodes[i];
            if (nd.occupied && !nd.deleted && pred.matches(nd.tag_mask, nd.tenant_id)) {
                bitSet(*match_bits, i);
            }
        }

        let matching: usize = 0;
        let w: usize = 0;
        while (w < FHNSW_BITSET_WORDS) : (w += 1) {
            matching += @popCount(match_bits[w]) as usize;
        }
        rep.matching_candidates = matching;
        if (matching == 0) {
            return rep;
        }

        const sel_bps: u16 = (((matching as u64) * 10000) / (_.live_count as u64)) as u16;
        rep.selectivity_bps = sel_bps;

        // Normalize query vector into stack scratch (or handle arbitrary dim directly via dot/norm).
        const q_norm_sq: f32 = @dot(query, query);
        if (!isFiniteF32(q_norm_sq)) {
            return rep;
        }
        const q_inv_norm: f32 = if (q_norm_sq > 1.0e-12) (1.0 / std.math.sqrt(q_norm_sq)) else 0.0;

        const max_k: usize = if (k < out_hits.len) k else out_hits.len;
        let hit_len: usize = 0;

        // 2. Adaptive selectivity routing: exact masked SIMD scan when filter is highly selective.
        if (sel_bps < _.cfg.exact_fallback_bps) {
            rep.path = FilteredSearchPath.exact_fallback;
            let s: usize = 0;
            while (s < _.count) : (s += 1) {
                if (!bitGet(match_bits, s)) continue;
                const v = _.vecAt(s);
                const raw_dot: f32 = @dot(query, v);
                const sim: f32 = raw_dot * q_inv_norm;
                const l2sq: f32 = @l2sq(query, v);
                rep.visited_nodes += 1;
                rep.distance_evals += 1;
                const nd = _.nodes[s];
                insertSortedHit(out_hits, *hit_len, max_k, FilteredSearchHit{
                    doc_id: nd.doc_id,
                    slot_idx: s as u16,
                    tenant_id: nd.tenant_id,
                    tag_mask: nd.tag_mask,
                    similarity: sim,
                    l2_sq_dist: l2sq,
                });
            }
            rep.hit_count = hit_len;
            return rep;
        }

        // 3. Single-stage predicate-aware navigable graph traversal (ACORN / Filtered-DiskANN style):
        //    traverse ALL nodes (even non-matching or tombstoned) for connectivity, admit only `match_bits`.
        rep.path = FilteredSearchPath.graph_filtered;
        let eff_ef: usize = if (ef_search > max_k) ef_search else max_k;
        if (eff_ef > FHNSW_MAX_BEAM) eff_ef = FHNSW_MAX_BEAM;

        let visited: [FHNSW_BITSET_WORDS]u64 = [0; FHNSW_BITSET_WORDS];
        let beam: [FHNSW_MAX_BEAM]BeamEntry = [BeamEntry{}; FHNSW_MAX_BEAM];
        let beam_len: usize = 0;

        const ep: usize = _.entry_slot as usize;
        bitSet(*visited, ep);
        const ep_v = _.vecAt(ep);
        const ep_sim: f32 = @dot(query, ep_v) * q_inv_norm;
        rep.visited_nodes += 1;
        rep.distance_evals += 1;
        insertBeamEntry(*beam, *beam_len, eff_ef, _.entry_slot, ep_sim);

        if (bitGet(match_bits, ep)) {
            const nd = _.nodes[ep];
            insertSortedHit(out_hits, *hit_len, max_k, FilteredSearchHit{
                doc_id: nd.doc_id,
                slot_idx: _.entry_slot,
                tenant_id: nd.tenant_id,
                tag_mask: nd.tag_mask,
                similarity: ep_sim,
                l2_sq_dist: @l2sq(query, ep_v),
            });
        }

        while (true) {
            let next_b: usize? = null;
            let b_i: usize = 0;
            while (b_i < beam_len) : (b_i += 1) {
                if (!beam[b_i].expanded) {
                    next_b = b_i;
                    break;
                }
            }
            const cur_b = next_b ?? break;
            beam[cur_b].expanded = true;

            const cur_slot: usize = beam[cur_b].slot as usize;
            const deg: usize = _.nodes[cur_slot].degree as usize;
            let n_i: usize = 0;
            while (n_i < deg) : (n_i += 1) {
                const nb_u16 = _.nodes[cur_slot].neighbors[n_i];
                if (nb_u16 == FHNSW_INVALID_SLOT) continue;
                const nb: usize = nb_u16 as usize;
                if (bitGet(visited, nb)) continue;
                bitSet(*visited, nb);

                const nb_v = _.vecAt(nb);
                const nb_sim: f32 = @dot(query, nb_v) * q_inv_norm;
                rep.visited_nodes += 1;
                rep.distance_evals += 1;

                // Always add to navigation beam so non-matching nodes remain traversal bridges.
                insertBeamEntry(*beam, *beam_len, eff_ef, nb_u16, nb_sim);

                // Admit into Top-K hits only if the node satisfies the filter predicate and is live.
                if (bitGet(match_bits, nb)) {
                    const nd = _.nodes[nb];
                    insertSortedHit(out_hits, *hit_len, max_k, FilteredSearchHit{
                        doc_id: nd.doc_id,
                        slot_idx: nb_u16,
                        tenant_id: nd.tenant_id,
                        tag_mask: nd.tag_mask,
                        similarity: nb_sim,
                        l2_sq_dist: @l2sq(query, nb_v),
                    });
                }
            }
        }

        rep.hit_count = hit_len;
        return rep;
    }
}
