// Origin: Pure-CP Stdlib (Zero-Alloc RAPTOR Hierarchical Summary Tree & Multi-Level Retriever) | Created: 2026-11-22
// Pure CP zero-heap-allocation RAPTOR (Recursive Abstractive Processing for Tree-Organized Retrieval)
// hierarchical summary tree and dual-mode multi-level retriever:
//   - Fixed-capacity multi-level tree (`leaf_chunk` -> `cluster_summary` -> `root_abstraction`)
//     using O(1) first-child / next-sibling / parent links with zero recursion (iterative BFS/DFS)
//   - Native `@dot` / `@l2sq` / `@fma` incremental cosine cluster assignment and centroid updates
//   - Dual retrieval modes:
//     * Collapsed Tree: flat multi-level Top-K joint retrieval across all active levels
//     * Tree Traversal: top-down level-by-level beam-pruned routing from roots down to leaves
//   - Level-weighted Linear & Reciprocal Rank Fusion (RRF) scoring with duplicate-hit deduplication
//   - Strict boundary guards for invalid dims, NaN/Inf, zero vectors, out-of-bounds slots, empty queries, and full capacity

const std = @import("std");

/// Maximum total nodes stored in `RaptorTree` (zero dynamic heap allocation).
pub const RAPTOR_MAX_NODES: usize = 256;

/// Maximum hierarchy depth (`level` in `0..RAPTOR_MAX_LEVELS`, where `0` is leaf chunks).
pub const RAPTOR_MAX_LEVELS: usize = 8;

/// Sentinel index indicating no parent, child, sibling, or matched cluster.
pub const RAPTOR_NO_NODE: u16 = 0xFFFF;

const NORM_SQ_EPS: f32 = 1.0e-12;
const SCORE_TIE_EPS: f32 = 1.0e-7;
const STACK_UNIT_DIM: usize = 512;
const BITSET_WORDS: usize = 4; // 4 * 64 = 256 bits >= RAPTOR_MAX_NODES

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn allFiniteF32(xs: []const f32): bool {
    let i: usize = 0;
    while (i < xs.len) : (i += 1) {
        if (!isFiniteF32(xs[i])) return false;
    }
    return true;
}

fn clampCosineF32(x: f32): f32 {
    if (x > 1.0) return 1.0;
    if (x < -1.0) return -1.0;
    return x;
}

fn bitsetTest(bits: [BITSET_WORDS]u64, idx: usize): bool {
    const word_idx: usize = idx >> 6;
    const bit_idx: u6 = (idx & 63) as u6;
    return ((bits[word_idx] >> bit_idx) & 1) != 0;
}

fn bitsetSet(bits: *[BITSET_WORDS]u64, idx: usize) {
    const word_idx: usize = idx >> 6;
    const bit_idx: u6 = (idx & 63) as u6;
    bits[word_idx] = bits[word_idx] | ((1 as u64) << bit_idx);
}

// ============================================================================
// 1. Public Enums, Config, Node & Search Result Models
// ============================================================================

/// Semantic role of a node within the RAPTOR hierarchy.
pub enum RaptorNodeKind {
    leaf_chunk,
    cluster_summary,
    root_abstraction,
}

/// Retrieval strategy over the RAPTOR hierarchy.
pub enum RaptorSearchMode {
    collapsed_tree,
    tree_traversal,
}

/// Cross-level score fusion strategy for ranking multi-level candidates.
pub enum RaptorScoreFusion {
    linear_weighted,
    reciprocal_rank,
}

/// Configuration for `RaptorTree`.
pub struct RaptorTreeConfig {
    dim: usize = 0;
    max_nodes: usize = RAPTOR_MAX_NODES;
    level_weights: [RAPTOR_MAX_LEVELS]f32 = [1.0; RAPTOR_MAX_LEVELS];
    rrf_k: f32 = 60.0;
    cluster_sim_threshold: f32 = 0.5;

    /// Validates dimensions, node capacity, finite non-negative level weights, RRF constant, and cluster threshold.
    fn isValid(): bool {
        if (_.dim == 0) return false;
        if (_.max_nodes == 0 || _.max_nodes > RAPTOR_MAX_NODES) return false;
        if (!isFiniteF32(_.rrf_k) || _.rrf_k <= 0.0) return false;
        if (!isFiniteF32(_.cluster_sim_threshold) || _.cluster_sim_threshold < -1.0 || _.cluster_sim_threshold > 1.0) {
            return false;
        }
        let lvl: usize = 0;
        while (lvl < RAPTOR_MAX_LEVELS) : (lvl += 1) {
            const w: f32 = _.level_weights[lvl];
            if (!isFiniteF32(w) || w < 0.0) return false;
        }
        return true;
    }
}

/// Metadata and topology links for a single node in `RaptorTree`.
pub struct RaptorNode {
    node_id: u32 = 0;
    slot_idx: u16 = 0;
    level: u8 = 0;
    kind: RaptorNodeKind = RaptorNodeKind.leaf_chunk;
    parent_idx: u16 = RAPTOR_NO_NODE;
    first_child_idx: u16 = RAPTOR_NO_NODE;
    next_sibling_idx: u16 = RAPTOR_NO_NODE;
    child_count: u16 = 0;
    token_count: u32 = 0;
    cluster_size: u32 = 1;
    active: bool = false;
}

/// Single ranked hit produced by `RaptorTree` retrieval.
pub struct RaptorSearchHit {
    node_id: u32 = 0;
    slot_idx: u16 = 0;
    level: u8 = 0;
    kind: RaptorNodeKind = RaptorNodeKind.leaf_chunk;
    raw_similarity: f32 = 0.0;
    fused_score: f32 = 0.0;
    l2_sq_dist: f32 = 0.0;
    token_count: u32 = 0;
}

/// Result of incremental cluster lookup or leaf assignment at a target summary level.
pub struct RaptorClusterAssignment {
    leaf_slot: u16 = RAPTOR_NO_NODE;
    target_parent_slot: u16 = RAPTOR_NO_NODE;
    similarity: f32 = 0.0;
    l2_sq_dist: f32 = 0.0;
    matched_existing: bool = false;
}

/// Aggregate capacity, depth, and level-distribution telemetry for `RaptorTree`.
pub struct RaptorTreeStats {
    total_nodes: usize = 0;
    capacity: usize = 0;
    dim: usize = 0;
    max_level: u8 = 0;
    level_counts: [RAPTOR_MAX_LEVELS]usize = [0; RAPTOR_MAX_LEVELS];
    leaf_count: usize = 0;
    summary_count: usize = 0;
    root_count: usize = 0;
    total_tokens: u64 = 0;
}

struct RaptorScoredSlot {
    slot_idx: u16 = RAPTOR_NO_NODE;
    node_id: u32 = 0;
    level: u8 = 0;
    raw_sim: f32 = 0.0;
    fused_score: f32 = 0.0;
    l2_sq_dist: f32 = 0.0;
}

fn hitBetter(a: RaptorSearchHit, b: RaptorSearchHit): bool {
    const diff: f32 = a.fused_score - b.fused_score;
    if (diff > SCORE_TIE_EPS) return true;
    if (diff < -SCORE_TIE_EPS) return false;
    const sim_diff: f32 = a.raw_similarity - b.raw_similarity;
    if (sim_diff > SCORE_TIE_EPS) return true;
    if (sim_diff < -SCORE_TIE_EPS) return false;
    return a.node_id < b.node_id;
}

fn scoredBetterByRawSim(a: RaptorScoredSlot, b: RaptorScoredSlot): bool {
    const diff: f32 = a.raw_sim - b.raw_sim;
    if (diff > SCORE_TIE_EPS) return true;
    if (diff < -SCORE_TIE_EPS) return false;
    return a.node_id < b.node_id;
}

fn sortScoredByRawSimDesc(items: *[]RaptorScoredSlot, count: usize) {
    let i: usize = 1;
    while (i < count) : (i += 1) {
        const key = items[i];
        let j: usize = i;
        while (j > 0 && scoredBetterByRawSim(key, items[j - 1])) : (j -= 1) {
            items[j] = items[j - 1];
        }
        items[j] = key;
    }
}

fn insertTopKHit(
    out_hits: *[]RaptorSearchHit,
    count: *usize,
    limit: usize,
    cand: RaptorSearchHit,
) {
    if (limit == 0) return;

    // Deduplicate by node_id: keep only the higher-ranked hit if duplicate node_id appears.
    let existing_idx: usize = limit;
    let scan: usize = 0;
    while (scan < count.*) : (scan += 1) {
        if (out_hits[scan].node_id == cand.node_id || out_hits[scan].slot_idx == cand.slot_idx) {
            existing_idx = scan;
            break;
        }
    }
    if (existing_idx < count.*) {
        if (!hitBetter(cand, out_hits[existing_idx])) {
            return;
        }
        // Remove existing entry before re-inserting in sorted order.
        let shift: usize = existing_idx;
        while (shift + 1 < count.*) : (shift += 1) {
            out_hits[shift] = out_hits[shift + 1];
        }
        count.* -= 1;
    }

    if (count.* < limit) {
        let pos: usize = count.*;
        while (pos > 0 && hitBetter(cand, out_hits[pos - 1])) : (pos -= 1) {
            out_hits[pos] = out_hits[pos - 1];
        }
        out_hits[pos] = cand;
        count.* += 1;
    } else if (hitBetter(cand, out_hits[limit - 1])) {
        let pos: usize = limit - 1;
        while (pos > 0 && hitBetter(cand, out_hits[pos - 1])) : (pos -= 1) {
            out_hits[pos] = out_hits[pos - 1];
        }
        out_hits[pos] = cand;
    }
}

// ============================================================================
// 2. Zero-Alloc RAPTOR Hierarchical Summary Tree & Retriever
// ============================================================================

/// Zero-heap-allocation RAPTOR hierarchical summary tree supporting iterative BFS/DFS traversal,
/// incremental cosine cluster assignment, and Collapsed Tree / Tree Traversal dual-mode retrieval.
pub struct RaptorTree {
    cfg: RaptorTreeConfig = RaptorTreeConfig{},
    vecs_flat: []f32;
    capacity: usize = 0;
    nodes: [RAPTOR_MAX_NODES]RaptorNode = [RaptorNode{}; RAPTOR_MAX_NODES];
    node_count: usize = 0;
    max_level: u8 = 0;

    /// Initializes an empty `RaptorTree` over caller-owned flat embedding storage `vecs_flat_buf`.
    /// Requires `cfg.isValid()` and `vecs_flat_buf.len >= cfg.max_nodes * cfg.dim`.
    static fn init(cfg: RaptorTreeConfig, vecs_flat_buf: *[]f32): RaptorTree? {
        if (!cfg.isValid()) return null;
        const max_cap_from_buf: usize = vecs_flat_buf.len / cfg.dim;
        if (max_cap_from_buf < cfg.max_nodes) return null;

        return RaptorTree{
            cfg: cfg,
            vecs_flat: vecs_flat_buf,
            capacity: cfg.max_nodes,
            nodes: [RaptorNode{}; RAPTOR_MAX_NODES],
            node_count: 0,
            max_level: 0,
        };
    }

    /// Resets the tree to empty without reallocating backing buffers.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            _.nodes[i] = RaptorNode{};
        }
        _.node_count = 0;
        _.max_level = 0;
    }

    /// Updates the fusion weight for `level` (`0..RAPTOR_MAX_LEVELS`, finite and >= 0.0).
    fn setLevelWeight(*_, level: u8, weight: f32): bool {
        if ((level as usize) >= RAPTOR_MAX_LEVELS) return false;
        if (!isFiniteF32(weight) || weight < 0.0) return false;
        _.cfg.level_weights[level as usize] = weight;
        return true;
    }

    /// Returns node metadata for `slot_idx`, or `null` if out of bounds or inactive.
    fn getNode(slot_idx: u16): RaptorNode? {
        const idx: usize = slot_idx as usize;
        if (idx >= _.node_count) return null;
        if (!_.nodes[idx].active) return null;
        return _.nodes[idx];
    }

    /// Returns a read-only view of the L2-normalized embedding stored at `slot_idx`.
    fn getEmbedding(slot_idx: u16): ([]const f32)? {
        const idx: usize = slot_idx as usize;
        if (idx >= _.node_count) return null;
        if (!_.nodes[idx].active) return null;
        const dim: usize = _.cfg.dim;
        const base: usize = idx * dim;
        return _.vecs_flat[base .. base + dim];
    }

    /// Returns aggregate tree statistics across all levels.
    fn stats(): RaptorTreeStats {
        let st = RaptorTreeStats{
            total_nodes: _.node_count,
            capacity: _.capacity,
            dim: _.cfg.dim,
            max_level: _.max_level,
        };
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const node = _.nodes[i];
            if (!node.active) continue;
            const lvl: usize = node.level as usize;
            if (lvl < RAPTOR_MAX_LEVELS) {
                st.level_counts[lvl] += 1;
            }
            if (node.kind == RaptorNodeKind.leaf_chunk) {
                st.leaf_count += 1;
            } else if (node.kind == RaptorNodeKind.cluster_summary) {
                st.summary_count += 1;
            } else {
                st.root_count += 1;
            }
            st.total_tokens += node.token_count as u64;
        }
        return st;
    }

    fn hasNodeId(node_id: u32): bool {
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            if (_.nodes[i].active && _.nodes[i].node_id == node_id) return true;
        }
        return false;
    }

    fn writeNormalizedEmbedding(*_, slot_idx: usize, embedding: []const f32): bool {
        const dim: usize = _.cfg.dim;
        if (embedding.len != dim) return false;
        if (!allFiniteF32(embedding)) return false;

        const norm_sq: f32 = @dot(embedding, embedding);
        if (!isFiniteF32(norm_sq) || norm_sq <= NORM_SQ_EPS) return false;
        const inv_norm: f32 = 1.0 / @sqrt(norm_sq);

        const base: usize = slot_idx * dim;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            _.vecs_flat[base + d] = embedding[d] * inv_norm;
        }
        return true;
    }

    fn linkChildUnchecked(*_, parent_idx: usize, child_idx: usize) {
        // Append child to end of parent's sibling list to preserve deterministic insertion order.
        _.nodes[child_idx].parent_idx = parent_idx as u16;
        _.nodes[child_idx].next_sibling_idx = RAPTOR_NO_NODE;
        if (_.nodes[parent_idx].first_child_idx == RAPTOR_NO_NODE) {
            _.nodes[parent_idx].first_child_idx = child_idx as u16;
        } else {
            let cur: u16 = _.nodes[parent_idx].first_child_idx;
            let guard: usize = 0;
            while (_.nodes[cur as usize].next_sibling_idx != RAPTOR_NO_NODE && guard < RAPTOR_MAX_NODES) : (guard += 1) {
                cur = _.nodes[cur as usize].next_sibling_idx;
            }
            _.nodes[cur as usize].next_sibling_idx = child_idx as u16;
        }
        _.nodes[parent_idx].child_count += 1;
    }

    fn detachFromParent(*_, child_idx: usize) {
        const p_slot: u16 = _.nodes[child_idx].parent_idx;
        if (p_slot == RAPTOR_NO_NODE) return;
        const p_idx: usize = p_slot as usize;
        if (p_idx >= _.node_count) {
            _.nodes[child_idx].parent_idx = RAPTOR_NO_NODE;
            _.nodes[child_idx].next_sibling_idx = RAPTOR_NO_NODE;
            return;
        }

        let prev: u16 = RAPTOR_NO_NODE;
        let cur: u16 = _.nodes[p_idx].first_child_idx;
        let guard: usize = 0;
        while (cur != RAPTOR_NO_NODE && guard < RAPTOR_MAX_NODES) : (guard += 1) {
            const next_sib: u16 = _.nodes[cur as usize].next_sibling_idx;
            if ((cur as usize) == child_idx) {
                if (prev == RAPTOR_NO_NODE) {
                    _.nodes[p_idx].first_child_idx = next_sib;
                } else {
                    _.nodes[prev as usize].next_sibling_idx = next_sib;
                }
                if (_.nodes[p_idx].child_count > 0) {
                    _.nodes[p_idx].child_count -= 1;
                }
                break;
            }
            prev = cur;
            cur = next_sib;
        }
        _.nodes[child_idx].parent_idx = RAPTOR_NO_NODE;
        _.nodes[child_idx].next_sibling_idx = RAPTOR_NO_NODE;
    }

    /// Inserts a Level-0 leaf chunk node. Returns its assigned slot index, or `null` on capacity/validation error.
    fn addLeaf(*_, node_id: u32, embedding: []const f32, token_count: u32): u16? {
        if (_.node_count >= _.capacity || _.node_count >= RAPTOR_MAX_NODES) return null;
        if (_.hasNodeId(node_id)) return null;

        const slot: usize = _.node_count;
        if (!_.writeNormalizedEmbedding(slot, embedding)) return null;

        _.nodes[slot] = RaptorNode{
            node_id: node_id,
            slot_idx: slot as u16,
            level: 0,
            kind: RaptorNodeKind.leaf_chunk,
            parent_idx: RAPTOR_NO_NODE,
            first_child_idx: RAPTOR_NO_NODE,
            next_sibling_idx: RAPTOR_NO_NODE,
            child_count: 0,
            token_count: token_count,
            cluster_size: 1,
            active: true,
        };
        _.node_count += 1;
        return slot as u16;
    }

    /// Attaches an existing lower-level node `child_slot` to a higher-level `parent_slot`.
    /// Rejects invalid slots, self-links, `child.level >= parent.level`, or already-attached duplicates.
    fn attachChild(*_, parent_slot: u16, child_slot: u16): bool {
        const p_idx: usize = parent_slot as usize;
        const c_idx: usize = child_slot as usize;
        if (p_idx >= _.node_count || c_idx >= _.node_count) return false;
        if (p_idx == c_idx) return false;
        if (!_.nodes[p_idx].active || !_.nodes[c_idx].active) return false;
        if (_.nodes[c_idx].level >= _.nodes[p_idx].level) return false;
        if (_.nodes[c_idx].parent_idx == parent_slot) return true;

        if (_.nodes[c_idx].parent_idx != RAPTOR_NO_NODE) {
            _.detachFromParent(c_idx);
        }
        _.linkChildUnchecked(p_idx, c_idx);
        return true;
    }

    /// Inserts a summary or root abstraction node (`level >= 1`) with an explicit embedding
    /// and attaches all valid, deduplicated lower-level `child_slots`.
    fn addSummaryNode(
        *_,
        node_id: u32,
        level: u8,
        kind: RaptorNodeKind,
        embedding: []const f32,
        token_count: u32,
        child_slots: []const u16,
    ): u16? {
        if (level == 0 || (level as usize) >= RAPTOR_MAX_LEVELS) return null;
        if (kind == RaptorNodeKind.leaf_chunk) return null;
        if (_.node_count >= _.capacity || _.node_count >= RAPTOR_MAX_NODES) return null;
        if (_.hasNodeId(node_id)) return null;

        // Pre-validate all child_slots before mutating tree state.
        let ci: usize = 0;
        while (ci < child_slots.len) : (ci += 1) {
            const c_idx: usize = child_slots[ci] as usize;
            if (c_idx >= _.node_count || !_.nodes[c_idx].active) return null;
            if (_.nodes[c_idx].level >= level) return null;
        }

        const slot: usize = _.node_count;
        if (!_.writeNormalizedEmbedding(slot, embedding)) return null;

        _.nodes[slot] = RaptorNode{
            node_id: node_id,
            slot_idx: slot as u16,
            level: level,
            kind: kind,
            parent_idx: RAPTOR_NO_NODE,
            first_child_idx: RAPTOR_NO_NODE,
            next_sibling_idx: RAPTOR_NO_NODE,
            child_count: 0,
            token_count: token_count,
            cluster_size: 0,
            active: true,
        };
        _.node_count += 1;
        if (level > _.max_level) {
            _.max_level = level;
        }

        let seen: [BITSET_WORDS]u64 = [0; BITSET_WORDS];
        let total_cluster_sz: u32 = 0;
        let k: usize = 0;
        while (k < child_slots.len) : (k += 1) {
            const c_slot: u16 = child_slots[k];
            const c_idx: usize = c_slot as usize;
            if (bitsetTest(seen, c_idx)) continue;
            bitsetSet(*seen, c_idx);
            * = _.attachChild(slot as u16, c_slot);
            total_cluster_sz += _.nodes[c_idx].cluster_size;
        }
        _.nodes[slot].cluster_size = if (total_cluster_sz > 0) total_cluster_sz else 1;
        return slot as u16;
    }

    /// Synthesizes a summary or root abstraction node directly from the L2-normalized centroid
    /// of `child_slots` (zero dynamic heap allocation; requires at least one valid child).
    fn addSummaryFromChildrenMean(
        *_,
        node_id: u32,
        level: u8,
        kind: RaptorNodeKind,
        token_count: u32,
        child_slots: []const u16,
    ): u16? {
        if (level == 0 || (level as usize) >= RAPTOR_MAX_LEVELS) return null;
        if (kind == RaptorNodeKind.leaf_chunk) return null;
        if (child_slots.len == 0) return null;
        if (_.node_count >= _.capacity || _.node_count >= RAPTOR_MAX_NODES) return null;
        if (_.hasNodeId(node_id)) return null;

        const dim: usize = _.cfg.dim;
        let seen: [BITSET_WORDS]u64 = [0; BITSET_WORDS];
        let unique_children: usize = 0;
        let ci: usize = 0;
        while (ci < child_slots.len) : (ci += 1) {
            const c_idx: usize = child_slots[ci] as usize;
            if (c_idx >= _.node_count || !_.nodes[c_idx].active) return null;
            if (_.nodes[c_idx].level >= level) return null;
            if (!bitsetTest(seen, c_idx)) {
                bitsetSet(*seen, c_idx);
                unique_children += 1;
            }
        }
        if (unique_children == 0) return null;

        const slot: usize = _.node_count;
        const dst_base: usize = slot * dim;
        let d: usize = 0;
        while (d < dim) : (d += 1) {
            _.vecs_flat[dst_base + d] = 0.0;
        }

        let accum_seen: [BITSET_WORDS]u64 = [0; BITSET_WORDS];
        let k: usize = 0;
        while (k < child_slots.len) : (k += 1) {
            const c_idx: usize = child_slots[k] as usize;
            if (bitsetTest(accum_seen, c_idx)) continue;
            bitsetSet(*accum_seen, c_idx);
            const src_base: usize = c_idx * dim;
            let j: usize = 0;
            while (j < dim) : (j += 1) {
                _.vecs_flat[dst_base + j] += _.vecs_flat[src_base + j];
            }
        }

        const dst_slice: []const f32 = _.vecs_flat[dst_base .. dst_base + dim];
        const norm_sq: f32 = @dot(dst_slice, dst_slice);
        if (!isFiniteF32(norm_sq) || norm_sq <= NORM_SQ_EPS) return null;
        const inv_norm: f32 = 1.0 / @sqrt(norm_sq);
        let m: usize = 0;
        while (m < dim) : (m += 1) {
            _.vecs_flat[dst_base + m] *= inv_norm;
        }

        _.nodes[slot] = RaptorNode{
            node_id: node_id,
            slot_idx: slot as u16,
            level: level,
            kind: kind,
            parent_idx: RAPTOR_NO_NODE,
            first_child_idx: RAPTOR_NO_NODE,
            next_sibling_idx: RAPTOR_NO_NODE,
            child_count: 0,
            token_count: token_count,
            cluster_size: 0,
            active: true,
        };
        _.node_count += 1;
        if (level > _.max_level) {
            _.max_level = level;
        }

        let link_seen: [BITSET_WORDS]u64 = [0; BITSET_WORDS];
        let total_cluster_sz: u32 = 0;
        let p: usize = 0;
        while (p < child_slots.len) : (p += 1) {
            const c_slot: u16 = child_slots[p];
            const c_idx: usize = c_slot as usize;
            if (bitsetTest(link_seen, c_idx)) continue;
            bitsetSet(*link_seen, c_idx);
            * = _.attachChild(slot as u16, c_slot);
            total_cluster_sz += _.nodes[c_idx].cluster_size;
        }
        _.nodes[slot].cluster_size = if (total_cluster_sz > 0) total_cluster_sz else 1;
        return slot as u16;
    }

    // ------------------------------------------------------------------------
    // Internal SIMD Cosine + L2-Squared Scoring Kernel (@dot + @l2sq)
    // ------------------------------------------------------------------------

    fn scoreQueryAgainstSlot(
        query: []const f32,
        inv_q_norm: f32,
        has_q_unit: bool,
        q_unit_buf: []const f32,
        slot_idx: usize,
        out_l2_sq: *f32,
    ): f32 {
        const dim: usize = _.cfg.dim;
        const base: usize = slot_idx * dim;
        const node_vec: []const f32 = _.vecs_flat[base .. base + dim];

        if (has_q_unit) {
            const sim: f32 = clampCosineF32(@dot(q_unit_buf, node_vec));
            let d2: f32 = @l2sq(q_unit_buf, node_vec);
            if (!isFiniteF32(d2) || d2 < 0.0) {
                d2 = 0.0;
            }
            out_l2_sq.* = d2;
            return sim;
        } else {
            const sim: f32 = clampCosineF32(@dot(query, node_vec) * inv_q_norm);
            let d2: f32 = 2.0 - 2.0 * sim;
            if (d2 < 0.0) {
                d2 = 0.0;
            }
            out_l2_sq.* = d2;
            return sim;
        }
    }

    // ------------------------------------------------------------------------
    // Incremental Cosine Cluster Assignment (@dot / @l2sq / @fma)
    // ------------------------------------------------------------------------

    /// Finds the highest-similarity summary cluster at `target_level` using native `@dot` and `@l2sq`.
    /// Sets `matched_existing = true` iff the best similarity meets `cfg.cluster_sim_threshold`.
    fn findBestClusterAtLevel(
        embedding: []const f32,
        target_level: u8,
    ): RaptorClusterAssignment? {
        const dim: usize = _.cfg.dim;
        if (embedding.len != dim) return null;
        if (target_level == 0 || (target_level as usize) >= RAPTOR_MAX_LEVELS) return null;
        if (!allFiniteF32(embedding)) return null;

        const norm_sq: f32 = @dot(embedding, embedding);
        if (!isFiniteF32(norm_sq) || norm_sq <= NORM_SQ_EPS) return null;
        const inv_norm: f32 = 1.0 / @sqrt(norm_sq);

        let q_unit: [STACK_UNIT_DIM]f32 = [0.0; STACK_UNIT_DIM];
        const has_q_unit: bool = dim <= STACK_UNIT_DIM;
        if (has_q_unit) {
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                q_unit[d] = embedding[d] * inv_norm;
            }
        }
        const q_unit_slice: []const f32 = if (has_q_unit) q_unit[0..dim] else q_unit[0..0];

        let best_slot: u16 = RAPTOR_NO_NODE;
        let best_sim: f32 = -2.0;
        let best_l2: f32 = 4.0;

        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const node = _.nodes[i];
            if (!node.active || node.level != target_level) continue;
            let d2: f32 = 0.0;
            const sim: f32 = _.scoreQueryAgainstSlot(embedding, inv_norm, has_q_unit, q_unit_slice, i, *d2);
            if (best_slot == RAPTOR_NO_NODE || sim > best_sim + SCORE_TIE_EPS ||
                (sim >= best_sim - SCORE_TIE_EPS && node.node_id < _.nodes[best_slot as usize].node_id)) {
                best_slot = i as u16;
                best_sim = sim;
                best_l2 = d2;
            }
        }

        if (best_slot == RAPTOR_NO_NODE) {
            return RaptorClusterAssignment{
                leaf_slot: RAPTOR_NO_NODE,
                target_parent_slot: RAPTOR_NO_NODE,
                similarity: 0.0,
                l2_sq_dist: 4.0,
                matched_existing: false,
            };
        }

        return RaptorClusterAssignment{
            leaf_slot: RAPTOR_NO_NODE,
            target_parent_slot: best_slot,
            similarity: best_sim,
            l2_sq_dist: best_l2,
            matched_existing: best_sim >= _.cfg.cluster_sim_threshold,
        };
    }

    /// Inserts a new leaf chunk and incrementally assigns it to the best existing summary cluster
    /// at `target_summary_level` if similarity >= `cfg.cluster_sim_threshold`.
    /// When `update_centroid` is `true`, updates the parent summary centroid in-place via `@fma` and re-normalizes.
    fn assignLeafIncremental(
        *_,
        node_id: u32,
        embedding: []const f32,
        token_count: u32,
        target_summary_level: u8,
        update_centroid: bool,
    ): RaptorClusterAssignment? {
        if (target_summary_level == 0 || (target_summary_level as usize) >= RAPTOR_MAX_LEVELS) return null;

        let best = RaptorClusterAssignment{};
        if (found := _.findBestClusterAtLevel(embedding, target_summary_level)) {
            best = found;
        } else {
            return null;
        }

        let leaf_slot: u16 = RAPTOR_NO_NODE;
        if (added := _.addLeaf(node_id, embedding, token_count)) {
            leaf_slot = added;
        } else {
            return null;
        }

        best.leaf_slot = leaf_slot;

        if (best.matched_existing && best.target_parent_slot != RAPTOR_NO_NODE) {
            const p_idx: usize = best.target_parent_slot as usize;
            const c_idx: usize = leaf_slot as usize;
            * = _.attachChild(best.target_parent_slot, leaf_slot);

            const prev_sz: u32 = if (_.nodes[p_idx].cluster_size > 0) _.nodes[p_idx].cluster_size else 1;
            const next_sz: u32 = prev_sz + 1;
            _.nodes[p_idx].cluster_size = next_sz;

            if (update_centroid) {
                const dim: usize = _.cfg.dim;
                const p_base: usize = p_idx * dim;
                const c_base: usize = c_idx * dim;
                const old_w: f32 = (prev_sz as f32) / (next_sz as f32);
                const new_w: f32 = 1.0 / (next_sz as f32);

                let d: usize = 0;
                while (d < dim) : (d += 1) {
                    _.vecs_flat[p_base + d] = @fma(old_w, _.vecs_flat[p_base + d], new_w * _.vecs_flat[c_base + d]);
                }

                const p_slice: []const f32 = _.vecs_flat[p_base .. p_base + dim];
                const nsq: f32 = @dot(p_slice, p_slice);
                if (isFiniteF32(nsq) && nsq > NORM_SQ_EPS) {
                    const inv_n: f32 = 1.0 / @sqrt(nsq);
                    let m: usize = 0;
                    while (m < dim) : (m += 1) {
                        _.vecs_flat[p_base + m] *= inv_n;
                    }
                }
            }
        } else {
            best.target_parent_slot = RAPTOR_NO_NODE;
            best.matched_existing = false;
        }

        return best;
    }

    // ------------------------------------------------------------------------
    // Iterative Zero-Recursion Traversals (BFS / DFS / Children / Ancestors)
    // ------------------------------------------------------------------------

    /// Collects all direct child slots of `parent_slot` in insertion order into `out_slots`.
    fn collectChildren(parent_slot: u16, out_slots: *[]u16): usize {
        const p_idx: usize = parent_slot as usize;
        if (p_idx >= _.node_count || !_.nodes[p_idx].active || out_slots.len == 0) return 0;

        let written: usize = 0;
        let cur: u16 = _.nodes[p_idx].first_child_idx;
        let guard: usize = 0;
        while (cur != RAPTOR_NO_NODE && written < out_slots.len && guard < RAPTOR_MAX_NODES) : (guard += 1) {
            out_slots[written] = cur;
            written += 1;
            cur = _.nodes[cur as usize].next_sibling_idx;
        }
        return written;
    }

    /// Collects ancestor slots walking upward from `slot_idx`'s parent to root into `out_slots`.
    fn collectAncestors(slot_idx: u16, out_slots: *[]u16): usize {
        const idx: usize = slot_idx as usize;
        if (idx >= _.node_count || !_.nodes[idx].active || out_slots.len == 0) return 0;

        let visited: [BITSET_WORDS]u64 = [0; BITSET_WORDS];
        bitsetSet(*visited, idx);

        let written: usize = 0;
        let cur: u16 = _.nodes[idx].parent_idx;
        let guard: usize = 0;
        while (cur != RAPTOR_NO_NODE && written < out_slots.len && guard < RAPTOR_MAX_NODES) : (guard += 1) {
            const c_idx: usize = cur as usize;
            if (c_idx >= _.node_count || !_.nodes[c_idx].active || bitsetTest(visited, c_idx)) break;
            bitsetSet(*visited, c_idx);
            out_slots[written] = cur;
            written += 1;
            cur = _.nodes[c_idx].parent_idx;
        }
        return written;
    }

    /// Pure iterative Breadth-First Search (BFS) starting from `start_slot` (zero recursion).
    fn traverseBfs(start_slot: u16, out_slots: *[]u16): usize {
        const s_idx: usize = start_slot as usize;
        if (s_idx >= _.node_count || !_.nodes[s_idx].active || out_slots.len == 0) return 0;

        let queue: [RAPTOR_MAX_NODES]u16 = [RAPTOR_NO_NODE; RAPTOR_MAX_NODES];
        let visited: [BITSET_WORDS]u64 = [0; BITSET_WORDS];
        let head: usize = 0;
        let tail: usize = 0;

        queue[tail] = start_slot;
        tail += 1;
        bitsetSet(*visited, s_idx);

        let written: usize = 0;
        while (head < tail && written < out_slots.len) {
            const cur: u16 = queue[head];
            head += 1;
            out_slots[written] = cur;
            written += 1;

            let child: u16 = _.nodes[cur as usize].first_child_idx;
            let guard: usize = 0;
            while (child != RAPTOR_NO_NODE && guard < RAPTOR_MAX_NODES) : (guard += 1) {
                const c_idx: usize = child as usize;
                if (c_idx < _.node_count && _.nodes[c_idx].active && !bitsetTest(visited, c_idx)) {
                    if (tail < RAPTOR_MAX_NODES) {
                        bitsetSet(*visited, c_idx);
                        queue[tail] = child;
                        tail += 1;
                    }
                }
                child = _.nodes[c_idx].next_sibling_idx;
            }
        }
        return written;
    }

    /// Pure iterative Depth-First Search (pre-order DFS) starting from `start_slot` (zero recursion).
    fn traverseDfs(start_slot: u16, out_slots: *[]u16): usize {
        const s_idx: usize = start_slot as usize;
        if (s_idx >= _.node_count || !_.nodes[s_idx].active || out_slots.len == 0) return 0;

        let stack: [RAPTOR_MAX_NODES]u16 = [RAPTOR_NO_NODE; RAPTOR_MAX_NODES];
        let child_buf: [RAPTOR_MAX_NODES]u16 = [RAPTOR_NO_NODE; RAPTOR_MAX_NODES];
        let visited: [BITSET_WORDS]u64 = [0; BITSET_WORDS];
        let top: usize = 0;

        stack[top] = start_slot;
        top += 1;
        bitsetSet(*visited, s_idx);

        let written: usize = 0;
        while (top > 0 && written < out_slots.len) {
            top -= 1;
            const cur: u16 = stack[top];
            out_slots[written] = cur;
            written += 1;

            // Gather children then push in reverse so first child is popped first (left-to-right pre-order).
            let c_cnt: usize = 0;
            let child: u16 = _.nodes[cur as usize].first_child_idx;
            let guard: usize = 0;
            while (child != RAPTOR_NO_NODE && c_cnt < RAPTOR_MAX_NODES && guard < RAPTOR_MAX_NODES) : (guard += 1) {
                const c_idx: usize = child as usize;
                if (c_idx < _.node_count && _.nodes[c_idx].active && !bitsetTest(visited, c_idx)) {
                    bitsetSet(*visited, c_idx);
                    child_buf[c_cnt] = child;
                    c_cnt += 1;
                }
                child = _.nodes[c_idx].next_sibling_idx;
            }

            let r: usize = c_cnt;
            while (r > 0) : (r -= 1) {
                if (top < RAPTOR_MAX_NODES) {
                    stack[top] = child_buf[r - 1];
                    top += 1;
                }
            }
        }
        return written;
    }

    // ------------------------------------------------------------------------
    // Dual-Mode Multi-Level Retrieval: Collapsed Tree & Tree Traversal
    // ------------------------------------------------------------------------

    fn finalizeCandidatesToHits(
        candidates: *[]RaptorScoredSlot,
        cand_count: usize,
        fusion: RaptorScoreFusion,
        limit: usize,
        out_hits: *[]RaptorSearchHit,
    ): usize {
        if (cand_count == 0 || limit == 0) return 0;

        if (fusion == RaptorScoreFusion.reciprocal_rank) {
            // Compute per-level rank for each candidate, then fuse via level_weight[lvl] / (rrf_k + rank).
            let lvl: usize = 0;
            while (lvl < RAPTOR_MAX_LEVELS) : (lvl += 1) {
                let lvl_items: [RAPTOR_MAX_NODES]RaptorScoredSlot = [RaptorScoredSlot{}; RAPTOR_MAX_NODES];
                let lvl_orig_idx: [RAPTOR_MAX_NODES]usize = [0; RAPTOR_MAX_NODES];
                let lvl_cnt: usize = 0;

                let i: usize = 0;
                while (i < cand_count) : (i += 1) {
                    if ((candidates[i].level as usize) == lvl) {
                        lvl_items[lvl_cnt] = candidates[i];
                        lvl_orig_idx[lvl_cnt] = i;
                        lvl_cnt += 1;
                    }
                }
                if (lvl_cnt == 0) continue;

                // Sort level items alongside original indices by raw_sim descending.
                let a: usize = 1;
                while (a < lvl_cnt) : (a += 1) {
                    const key_item = lvl_items[a];
                    const key_orig = lvl_orig_idx[a];
                    let b: usize = a;
                    while (b > 0 && scoredBetterByRawSim(key_item, lvl_items[b - 1])) : (b -= 1) {
                        lvl_items[b] = lvl_items[b - 1];
                        lvl_orig_idx[b] = lvl_orig_idx[b - 1];
                    }
                    lvl_items[b] = key_item;
                    lvl_orig_idx[b] = key_orig;
                }

                const w: f32 = _.cfg.level_weights[lvl];
                let rank_idx: usize = 0;
                while (rank_idx < lvl_cnt) : (rank_idx += 1) {
                    const rank_1based: f32 = (rank_idx + 1) as f32;
                    const orig: usize = lvl_orig_idx[rank_idx];
                    candidates[orig].fused_score = w / (_.cfg.rrf_k + rank_1based);
                }
            }
        } else {
            let i: usize = 0;
            while (i < cand_count) : (i += 1) {
                const lvl: usize = candidates[i].level as usize;
                const w: f32 = if (lvl < RAPTOR_MAX_LEVELS) _.cfg.level_weights[lvl] else 1.0;
                candidates[i].fused_score = w * candidates[i].raw_sim;
            }
        }

        let hit_count: usize = 0;
        let k: usize = 0;
        while (k < cand_count) : (k += 1) {
            const c = candidates[k];
            const node = _.nodes[c.slot_idx as usize];
            const hit = RaptorSearchHit{
                node_id: node.node_id,
                slot_idx: c.slot_idx,
                level: node.level,
                kind: node.kind,
                raw_similarity: c.raw_sim,
                fused_score: c.fused_score,
                l2_sq_dist: c.l2_sq_dist,
                token_count: node.token_count,
            };
            insertTopKHit(out_hits, *hit_count, limit, hit);
        }
        return hit_count;
    }

    /// **Collapsed Tree Mode**: scores all active nodes within `[min_level, max_level_filter]`
    /// across the flattened hierarchy and returns the deduplicated Top-K hits.
    fn searchCollapsed(
        query: []const f32,
        top_k: usize,
        fusion: RaptorScoreFusion,
        min_level: u8,
        max_level_filter: u8,
        out_hits: *[]RaptorSearchHit,
    ): usize {
        const dim: usize = _.cfg.dim;
        if (query.len != dim || _.node_count == 0 || top_k == 0 || out_hits.len == 0) return 0;
        if (min_level > max_level_filter) return 0;
        if (!allFiniteF32(query)) return 0;

        const norm_sq: f32 = @dot(query, query);
        if (!isFiniteF32(norm_sq) || norm_sq <= NORM_SQ_EPS) return 0;
        const inv_q_norm: f32 = 1.0 / @sqrt(norm_sq);

        let q_unit: [STACK_UNIT_DIM]f32 = [0.0; STACK_UNIT_DIM];
        const has_q_unit: bool = dim <= STACK_UNIT_DIM;
        if (has_q_unit) {
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                q_unit[d] = query[d] * inv_q_norm;
            }
        }
        const q_unit_slice: []const f32 = if (has_q_unit) q_unit[0..dim] else q_unit[0..0];

        let candidates: [RAPTOR_MAX_NODES]RaptorScoredSlot = [RaptorScoredSlot{}; RAPTOR_MAX_NODES];
        let cand_count: usize = 0;

        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const node = _.nodes[i];
            if (!node.active) continue;
            if (node.level < min_level || node.level > max_level_filter) continue;

            let d2: f32 = 0.0;
            const sim: f32 = _.scoreQueryAgainstSlot(query, inv_q_norm, has_q_unit, q_unit_slice, i, *d2);
            candidates[cand_count] = RaptorScoredSlot{
                slot_idx: i as u16,
                node_id: node.node_id,
                level: node.level,
                raw_sim: sim,
                fused_score: 0.0,
                l2_sq_dist: d2,
            };
            cand_count += 1;
        }

        const limit: usize = if (top_k < out_hits.len) top_k else out_hits.len;
        let cand_slice: []RaptorScoredSlot = candidates[0..cand_count];
        return _.finalizeCandidatesToHits(*cand_slice, cand_count, fusion, limit, out_hits);
    }

    /// **Tree Traversal Mode**: routes top-down from `max_level` to `0`, keeping only the top
    /// `beam_per_level` nodes at each level and expanding their children for the next level.
    /// Collected multi-level candidates are deduplicated and ranked with `fusion`.
    fn searchTreeTraversal(
        query: []const f32,
        beam_per_level: usize,
        top_k: usize,
        fusion: RaptorScoreFusion,
        out_hits: *[]RaptorSearchHit,
    ): usize {
        const dim: usize = _.cfg.dim;
        if (query.len != dim || _.node_count == 0 || beam_per_level == 0 || top_k == 0 || out_hits.len == 0) {
            return 0;
        }
        if (!allFiniteF32(query)) return 0;

        const norm_sq: f32 = @dot(query, query);
        if (!isFiniteF32(norm_sq) || norm_sq <= NORM_SQ_EPS) return 0;
        const inv_q_norm: f32 = 1.0 / @sqrt(norm_sq);

        let q_unit: [STACK_UNIT_DIM]f32 = [0.0; STACK_UNIT_DIM];
        const has_q_unit: bool = dim <= STACK_UNIT_DIM;
        if (has_q_unit) {
            let d: usize = 0;
            while (d < dim) : (d += 1) {
                q_unit[d] = query[d] * inv_q_norm;
            }
        }
        const q_unit_slice: []const f32 = if (has_q_unit) q_unit[0..dim] else q_unit[0..0];

        // Frontier holds slots scheduled for evaluation at the current descent step.
        let frontier: [RAPTOR_MAX_NODES]u16 = [RAPTOR_NO_NODE; RAPTOR_MAX_NODES];
        let frontier_len: usize = 0;

        // Seed frontier with all active root nodes (nodes at max_level, or any top-level parentless node at max_level).
        let i: usize = 0;
        while (i < _.node_count) : (i += 1) {
            const node = _.nodes[i];
            if (node.active && node.level == _.max_level) {
                frontier[frontier_len] = i as u16;
                frontier_len += 1;
            }
        }
        if (frontier_len == 0) return 0;

        let collected: [RAPTOR_MAX_NODES]RaptorScoredSlot = [RaptorScoredSlot{}; RAPTOR_MAX_NODES];
        let collected_count: usize = 0;
        let collected_bits: [BITSET_WORDS]u64 = [0; BITSET_WORDS];

        let step: usize = 0;
        while (frontier_len > 0 && step < RAPTOR_MAX_LEVELS) : (step += 1) {
            let level_scored: [RAPTOR_MAX_NODES]RaptorScoredSlot = [RaptorScoredSlot{}; RAPTOR_MAX_NODES];
            let level_cnt: usize = 0;

            let fi: usize = 0;
            while (fi < frontier_len) : (fi += 1) {
                const slot: u16 = frontier[fi];
                const s_idx: usize = slot as usize;
                if (s_idx >= _.node_count || !_.nodes[s_idx].active) continue;
                const node = _.nodes[s_idx];

                let d2: f32 = 0.0;
                const sim: f32 = _.scoreQueryAgainstSlot(query, inv_q_norm, has_q_unit, q_unit_slice, s_idx, *d2);
                level_scored[level_cnt] = RaptorScoredSlot{
                    slot_idx: slot,
                    node_id: node.node_id,
                    level: node.level,
                    raw_sim: sim,
                    fused_score: 0.0,
                    l2_sq_dist: d2,
                };
                level_cnt += 1;
            }

            if (level_cnt == 0) break;

            let level_slice: []RaptorScoredSlot = level_scored[0..level_cnt];
            sortScoredByRawSimDesc(*level_slice, level_cnt);

            const keep: usize = if (beam_per_level < level_cnt) beam_per_level else level_cnt;
            let next_frontier: [RAPTOR_MAX_NODES]u16 = [RAPTOR_NO_NODE; RAPTOR_MAX_NODES];
            let next_len: usize = 0;
            let next_bits: [BITSET_WORDS]u64 = [0; BITSET_WORDS];

            let b: usize = 0;
            while (b < keep) : (b += 1) {
                const winner = level_scored[b];
                const w_idx: usize = winner.slot_idx as usize;

                if (!bitsetTest(collected_bits, w_idx) && collected_count < RAPTOR_MAX_NODES) {
                    bitsetSet(*collected_bits, w_idx);
                    collected[collected_count] = winner;
                    collected_count += 1;
                }

                // Expand direct children of this surviving beam node.
                let child: u16 = _.nodes[w_idx].first_child_idx;
                let guard: usize = 0;
                while (child != RAPTOR_NO_NODE && guard < RAPTOR_MAX_NODES) : (guard += 1) {
                    const c_idx: usize = child as usize;
                    if (c_idx < _.node_count && _.nodes[c_idx].active &&
                        !bitsetTest(next_bits, c_idx) && !bitsetTest(collected_bits, c_idx)) {
                        if (next_len < RAPTOR_MAX_NODES) {
                            bitsetSet(*next_bits, c_idx);
                            next_frontier[next_len] = child;
                            next_len += 1;
                        }
                    }
                    child = _.nodes[c_idx].next_sibling_idx;
                }
            }

            let nf: usize = 0;
            while (nf < next_len) : (nf += 1) {
                frontier[nf] = next_frontier[nf];
            }
            frontier_len = next_len;
        }

        const limit: usize = if (top_k < out_hits.len) top_k else out_hits.len;
        let col_slice: []RaptorScoredSlot = collected[0..collected_count];
        return _.finalizeCandidatesToHits(*col_slice, collected_count, fusion, limit, out_hits);
    }

    /// Unified retrieval entry point dispatching `collapsed_tree` or `tree_traversal`.
    fn search(
        query: []const f32,
        mode: RaptorSearchMode,
        fusion: RaptorScoreFusion,
        top_k: usize,
        beam_per_level: usize,
        out_hits: *[]RaptorSearchHit,
    ): usize {
        if (mode == RaptorSearchMode.collapsed_tree) {
            return _.searchCollapsed(query, top_k, fusion, 0, _.max_level, out_hits);
        }
        return _.searchTreeTraversal(query, beam_per_level, top_k, fusion, out_hits);
    }
}
