// Origin: Pure-CP Stdlib (AI Agent Zero-Alloc Temporal Entity Knowledge Graph & Two-Hop Recall Engine) | Created: 2026-11-22
// Pure CP zero-heap-allocation temporal entity relationship graph for GraphRAG and long-horizon
// fact memory evolution:
//   - Half-open temporal validity intervals `[valid_from_ms, valid_to_ms)` (`valid_to_ms == 0` means still valid)
//   - Atomic fact superseding (`supersedeFact`) resolving stale-vs-new memory conflicts while preserving history
//   - Point-in-time historical snapshot queries (`queryFactsAt`)
//   - Zero-alloc 1-hop + 2-hop multi-hop neighborhood expansion (`traverseTwoHop`) with multiplicative
//     path confidence decay, cycle avoidance, and best-path entity deduplication

const std = @import("std");

// ============================================================================
// Internal Validation & Ordering Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isValidEdgeConfidence(c: f32): bool {
    return isFiniteF32(c) && c > 0.0 && c <= 1.0;
}

fn isValidMinConfidence(c: f32): bool {
    return isFiniteF32(c) && c >= 0.0 && c <= 1.0;
}

fn isFactValidAt(edge: TemporalFactEdge, at_ms: i64): bool {
    if (!edge.active) {
        return false;
    }
    if (at_ms < edge.valid_from_ms) {
        return false;
    }
    if (edge.valid_to_ms != 0 && at_ms >= edge.valid_to_ms) {
        return false;
    }
    return true;
}

fn isBetterHopHit(a: GraphHopHit, b: GraphHopHit): bool {
    if (a.path_confidence > b.path_confidence) {
        return true;
    }
    if (a.path_confidence < b.path_confidence) {
        return false;
    }
    if (a.entity_id != b.entity_id) {
        return a.entity_id < b.entity_id;
    }
    if (a.hop_depth != b.hop_depth) {
        return a.hop_depth < b.hop_depth;
    }
    return a.via_edge_id < b.via_edge_id;
}

fn upsertHopHit(out_hits: *[]GraphHopHit, count: *usize, cand: GraphHopHit) {
    const limit: usize = out_hits.len;
    if (limit == 0) {
        return;
    }

    // 1. Deduplicate: if entity_id is already present, keep the higher-confidence / shallower path
    let existing: usize? = null;
    let i: usize = 0;
    while (i < count.*) : (i += 1) {
        if (out_hits[i].entity_id == cand.entity_id) {
            existing = i;
            break;
        }
    }

    if (idx := existing) {
        const cur = out_hits[idx];
        const cand_better = cand.path_confidence > cur.path_confidence ||
            (cand.path_confidence == cur.path_confidence &&
                (cand.hop_depth < cur.hop_depth ||
                    (cand.hop_depth == cur.hop_depth && cand.via_edge_id < cur.via_edge_id)));
        if (!cand_better) {
            return;
        }
        out_hits[idx] = cand;
        // Bubble left to maintain descending path_confidence, ascending entity_id order
        let pos: usize = idx;
        while (pos > 0 && isBetterHopHit(out_hits[pos], out_hits[pos - 1])) : (pos -= 1) {
            const tmp = out_hits[pos - 1];
            out_hits[pos - 1] = out_hits[pos];
            out_hits[pos] = tmp;
        }
        return;
    }

    // 2. Insert new entity into sorted Top-K output buffer
    if (count.* < limit) {
        let pos: usize = count.*;
        while (pos > 0 && isBetterHopHit(cand, out_hits[pos - 1])) : (pos -= 1) {
            out_hits[pos] = out_hits[pos - 1];
        }
        out_hits[pos] = cand;
        count.* += 1;
    } else if (isBetterHopHit(cand, out_hits[limit - 1])) {
        let pos: usize = limit - 1;
        while (pos > 0 && isBetterHopHit(cand, out_hits[pos - 1])) : (pos -= 1) {
            out_hits[pos] = out_hits[pos - 1];
        }
        out_hits[pos] = cand;
    }
}

// ============================================================================
// 1. Temporal Entity Graph Data Models
// ============================================================================

/// Directed temporal fact edge `(src_entity_id, rel_type_id, dst_entity_id)` valid over
/// `[valid_from_ms, valid_to_ms)`. `valid_to_ms == 0` indicates the fact is currently valid.
pub struct TemporalFactEdge {
    edge_id: u32 = 0;
    src_entity_id: u64 = 0;
    rel_type_id: u32 = 0;
    dst_entity_id: u64 = 0;
    confidence: f32 = 0.0;
    valid_from_ms: i64 = 0;
    valid_to_ms: i64 = 0;
    active: bool = false;
}

/// Multi-hop neighborhood hit returned by `TemporalEntityGraph.traverseTwoHop`.
pub struct GraphHopHit {
    entity_id: u64 = 0;
    hop_depth: u8 = 0;
    path_confidence: f32 = 0.0;
    via_edge_id: u32 = 0;
}

/// Aggregate operational statistics for `TemporalEntityGraph`.
pub struct EntityGraphStats {
    capacity: usize = 0;
    total_edges: usize = 0;
    current_valid_edges: usize = 0;
    superseded_edges: usize = 0;
    total_assertions: u64 = 0;
    total_supersedes: u64 = 0;
}

// ============================================================================
// 2. Temporal Entity Relationship Graph Engine
// ============================================================================

/// Zero-heap-allocation temporal entity relationship graph backed by a caller-owned
/// `[]TemporalFactEdge` buffer. Supports atomic fact superseding, point-in-time historical
/// queries, and cycle-safe 1-hop + 2-hop confidence-decayed entity expansion.
pub struct TemporalEntityGraph {
    edges: []TemporalFactEdge;
    used: usize = 0;
    next_edge_id: u32 = 1;
    total_assertions: u64 = 0;
    total_supersedes: u64 = 0;

    /// Initializes a zero-alloc temporal graph over caller-owned `edges_buf`.
    /// Returns `null` if `edges_buf.len == 0`.
    static fn init(edges_buf: *[]TemporalFactEdge): TemporalEntityGraph? {
        if (edges_buf.len == 0) {
            return null;
        }
        let i: usize = 0;
        while (i < edges_buf.len) : (i += 1) {
            edges_buf[i] = TemporalFactEdge{};
        }
        return TemporalEntityGraph{
            edges: edges_buf,
            used: 0,
            next_edge_id: 1,
            total_assertions: 0,
            total_supersedes: 0,
        };
    }

    /// Asserts a new currently-valid fact `(src_id, rel_id, dst_id)` starting at `valid_from_ms`.
    /// Requires non-zero IDs, `confidence ∈ (0.0, 1.0]`, `valid_from_ms >= 0`, and free capacity.
    /// Returns the assigned `edge_id`, or `null` on invalid input or full capacity.
    fn assertFact(
        *_,
        src_id: u64,
        rel_id: u32,
        dst_id: u64,
        confidence: f32,
        valid_from_ms: i64,
    ): u32? {
        if (src_id == 0 || rel_id == 0 || dst_id == 0) {
            return null;
        }
        if (!isValidEdgeConfidence(confidence) || valid_from_ms < 0) {
            return null;
        }
        if (_.used >= _.edges.len || _.next_edge_id == 0) {
            return null;
        }

        const eid: u32 = _.next_edge_id;
        _.edges[_.used] = TemporalFactEdge{
            edge_id: eid,
            src_entity_id: src_id,
            rel_type_id: rel_id,
            dst_entity_id: dst_id,
            confidence: confidence,
            valid_from_ms: valid_from_ms,
            valid_to_ms: 0,
            active: true,
        };
        _.used += 1;
        _.next_edge_id = if (eid < std.math.maxInt(u32)) eid + 1 else 0;
        _.total_assertions += 1;
        return eid;
    }

    /// Atomically supersedes all currently valid facts matching `(src_id, rel_id)` whose
    /// `valid_from_ms <= now_ms` by setting their `valid_to_ms = now_ms`, then inserts the new
    /// fact `(src_id, rel_id, new_dst_id)` valid from `now_ms`.
    /// If capacity is full or any argument is invalid, no existing facts are modified and `null` is returned.
    fn supersedeFact(
        *_,
        src_id: u64,
        rel_id: u32,
        new_dst_id: u64,
        confidence: f32,
        now_ms: i64,
    ): u32? {
        if (src_id == 0 || rel_id == 0 || new_dst_id == 0) {
            return null;
        }
        if (!isValidEdgeConfidence(confidence) || now_ms <= 0) {
            return null;
        }
        if (_.used >= _.edges.len || _.next_edge_id == 0) {
            return null;
        }

        let i: usize = 0;
        while (i < _.used) : (i += 1) {
            if (_.edges[i].active &&
                _.edges[i].src_entity_id == src_id &&
                _.edges[i].rel_type_id == rel_id &&
                _.edges[i].valid_to_ms == 0 &&
                _.edges[i].valid_from_ms <= now_ms)
            {
                _.edges[i].valid_to_ms = now_ms;
                _.total_supersedes += 1;
            }
        }

        return _.assertFact(src_id, rel_id, new_dst_id, confidence, now_ms);
    }

    /// Manually invalidates a specific currently-valid edge by setting its `valid_to_ms`.
    /// Returns `false` if not found, already invalidated, or `valid_to_ms <= 0 || valid_to_ms < valid_from_ms`.
    fn invalidateEdge(*_, edge_id: u32, valid_to_ms: i64): bool {
        if (edge_id == 0 || valid_to_ms <= 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.used) : (i += 1) {
            if (_.edges[i].active && _.edges[i].edge_id == edge_id) {
                if (_.edges[i].valid_to_ms != 0 || valid_to_ms < _.edges[i].valid_from_ms) {
                    return false;
                }
                _.edges[i].valid_to_ms = valid_to_ms;
                _.total_supersedes += 1;
                return true;
            }
        }
        return false;
    }

    /// Point-in-time historical snapshot query: returns all facts from `src_id` (optionally
    /// filtered by `rel_id_opt`) that were valid at timestamp `at_ms`.
    /// Output is sorted by `confidence` descending (tie-broken by `edge_id` ascending).
    fn queryFactsAt(
        src_id: u64,
        rel_id_opt: u32?,
        at_ms: i64,
        out_edges: *[]TemporalFactEdge,
    ): usize {
        if (src_id == 0 || out_edges.len == 0) {
            return 0;
        }
        const limit: usize = out_edges.len;
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.used) : (i += 1) {
            const e = _.edges[i];
            if (e.src_entity_id != src_id || !isFactValidAt(e, at_ms)) {
                continue;
            }
            if (rel_filter := rel_id_opt) {
                if (e.rel_type_id != rel_filter) {
                    continue;
                }
            }

            if (count < limit) {
                let pos: usize = count;
                while (pos > 0 && (e.confidence > out_edges[pos - 1].confidence ||
                    (e.confidence == out_edges[pos - 1].confidence && e.edge_id < out_edges[pos - 1].edge_id))) : (pos -= 1)
                {
                    out_edges[pos] = out_edges[pos - 1];
                }
                out_edges[pos] = e;
                count += 1;
            } else if (e.confidence > out_edges[limit - 1].confidence ||
                (e.confidence == out_edges[limit - 1].confidence && e.edge_id < out_edges[limit - 1].edge_id))
            {
                let pos: usize = limit - 1;
                while (pos > 0 && (e.confidence > out_edges[pos - 1].confidence ||
                    (e.confidence == out_edges[pos - 1].confidence && e.edge_id < out_edges[pos - 1].edge_id))) : (pos -= 1)
                {
                    out_edges[pos] = out_edges[pos - 1];
                }
                out_edges[pos] = e;
            }
        }
        return count;
    }

    /// Zero-allocation 1-hop + 2-hop entity neighborhood expansion at timestamp `at_ms`.
    /// Path confidence for 2-hop paths is `e1.confidence * e2.confidence`.
    /// Automatically excludes cycles back to `start_entity_id`, filters edges/paths below
    /// `min_confidence`, deduplicates entities by keeping the highest-confidence path, and
    /// outputs sorted by `(path_confidence desc, entity_id asc)`.
    fn traverseTwoHop(
        start_entity_id: u64,
        at_ms: i64,
        min_confidence: f32,
        out_hits: *[]GraphHopHit,
    ): usize {
        if (start_entity_id == 0 || out_hits.len == 0 || !isValidMinConfidence(min_confidence)) {
            return 0;
        }

        let count: usize = 0;
        let i: usize = 0;
        while (i < _.used) : (i += 1) {
            const e1 = _.edges[i];
            if (e1.src_entity_id != start_entity_id || !isFactValidAt(e1, at_ms)) {
                continue;
            }
            if (e1.dst_entity_id == start_entity_id || e1.confidence < min_confidence) {
                continue;
            }

            // Record 1-hop hit
            const hop1 = GraphHopHit{
                entity_id: e1.dst_entity_id,
                hop_depth: 1,
                path_confidence: e1.confidence,
                via_edge_id: e1.edge_id,
            };
            upsertHopHit(out_hits, *count, hop1);

            // Expand 2-hop neighbors from e1.dst_entity_id
            let j: usize = 0;
            while (j < _.used) : (j += 1) {
                const e2 = _.edges[j];
                if (e2.src_entity_id != e1.dst_entity_id || !isFactValidAt(e2, at_ms)) {
                    continue;
                }
                if (e2.dst_entity_id == start_entity_id || e2.dst_entity_id == e1.dst_entity_id) {
                    continue;
                }
                const path_conf: f32 = e1.confidence * e2.confidence;
                if (!isFiniteF32(path_conf) || path_conf < min_confidence) {
                    continue;
                }
                const hop2 = GraphHopHit{
                    entity_id: e2.dst_entity_id,
                    hop_depth: 2,
                    path_confidence: path_conf,
                    via_edge_id: e2.edge_id,
                };
                upsertHopHit(out_hits, *count, hop2);
            }
        }
        return count;
    }

    /// Returns aggregate capacity, valid/superseded edge counts, and operation counters.
    fn stats(): EntityGraphStats {
        let valid_now: usize = 0;
        let superseded: usize = 0;
        let i: usize = 0;
        while (i < _.used) : (i += 1) {
            if (_.edges[i].active) {
                if (_.edges[i].valid_to_ms == 0) {
                    valid_now += 1;
                } else {
                    superseded += 1;
                }
            }
        }
        return EntityGraphStats{
            capacity: _.edges.len,
            total_edges: _.used,
            current_valid_edges: valid_now,
            superseded_edges: superseded,
            total_assertions: _.total_assertions,
            total_supersedes: _.total_supersedes,
        };
    }

    /// Resets the graph to empty state without reallocating.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.edges.len) : (i += 1) {
            _.edges[i] = TemporalFactEdge{};
        }
        _.used = 0;
        _.next_edge_id = 1;
        _.total_assertions = 0;
        _.total_supersedes = 0;
    }
}
