// Origin: Pure-CP Stdlib (Zero-Alloc Multi-Tier AI Agent Memory Fusion Router: Working+Episodic+Semantic+Graph RRF) | Created: 2026-11-22
// Pure CP zero-heap-allocation Multi-Tier Memory Recall Fusion & Token-Budget Packer:
//   - Fixed inline stack capacity (`MEM_ROUTER_MAX_CANDIDATES = 64`), 0 dynamic heap allocation
//   - Intent-aware weighted Reciprocal Rank Fusion (`w_s / (rrf_k + rank + 1)`) across 4 stores:
//     Working Memory, Episodic Memory, Semantic Long-Term, and Entity Graph
//   - Cross-store deduplication by `memory_id` with `source_mask` bitset & per-store duplicate-score guard
//   - Multi-store consensus boosting, `pinned` priority locking & greedy token-budget packing

const std = @import("std");

/// Maximum recall candidates processed per `fuseAndPackMemoryRecalls` call (zero dynamic heap).
pub const MEM_ROUTER_MAX_CANDIDATES: usize = 64;

const SCORE_TIE_EPS: f32 = 1.0e-7;

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn sourceIndex(src: MemorySourceStore): usize {
    if (src == MemorySourceStore.working) return 0;
    if (src == MemorySourceStore.episodic) return 1;
    if (src == MemorySourceStore.semantic) return 2;
    return 3;
}

fn sourceBit(src: MemorySourceStore): u8 {
    if (src == MemorySourceStore.working) return 1;
    if (src == MemorySourceStore.episodic) return 2;
    if (src == MemorySourceStore.semantic) return 4;
    return 8;
}

fn weightForSource(w: MemoryTierWeights, src: MemorySourceStore): f32 {
    if (src == MemorySourceStore.working) return w.working_w;
    if (src == MemorySourceStore.episodic) return w.episodic_w;
    if (src == MemorySourceStore.semantic) return w.semantic_w;
    return w.graph_w;
}

// ============================================================================
// 1. Public Enums, Weights & Fusion Models
// ============================================================================

/// Backing memory store that produced a recall candidate.
pub enum MemorySourceStore {
    working,
    episodic,
    semantic,
    entity_graph,
}

/// High-level query intent presets for weighting the 4 memory tiers.
pub enum MemoryQueryIntent {
    recent_context,
    factual_lookup,
    deep_synthesis,
    custom_weights,
}

/// Per-tier RRF weights and smoothing constant `rrf_k`.
pub struct MemoryTierWeights {
    working_w: f32 = 1.0;
    episodic_w: f32 = 1.0;
    semantic_w: f32 = 1.0;
    graph_w: f32 = 1.0;
    rrf_k: f32 = 60.0;

    /// Returns tuned tier weights for a given `MemoryQueryIntent`.
    static fn forIntent(intent: MemoryQueryIntent): MemoryTierWeights {
        if (intent == MemoryQueryIntent.recent_context) {
            return MemoryTierWeights{
                working_w: 2.0,
                episodic_w: 1.5,
                semantic_w: 0.6,
                graph_w: 0.4,
                rrf_k: 60.0,
            };
        }
        if (intent == MemoryQueryIntent.factual_lookup) {
            return MemoryTierWeights{
                working_w: 0.5,
                episodic_w: 0.6,
                semantic_w: 1.6,
                graph_w: 2.0,
                rrf_k: 60.0,
            };
        }
        if (intent == MemoryQueryIntent.deep_synthesis) {
            return MemoryTierWeights{
                working_w: 1.0,
                episodic_w: 1.2,
                semantic_w: 1.5,
                graph_w: 1.5,
                rrf_k: 60.0,
            };
        }
        return MemoryTierWeights{
            working_w: 1.0,
            episodic_w: 1.0,
            semantic_w: 1.0,
            graph_w: 1.0,
            rrf_k: 60.0,
        };
    }

    /// Validates that all weights are finite and `>= 0.0`, at least one weight is `> 0.0`,
    /// and `rrf_k` is finite and `> 0.0`.
    fn isValid(): bool {
        if (!isFiniteF32(_.working_w) || _.working_w < 0.0) return false;
        if (!isFiniteF32(_.episodic_w) || _.episodic_w < 0.0) return false;
        if (!isFiniteF32(_.semantic_w) || _.semantic_w < 0.0) return false;
        if (!isFiniteF32(_.graph_w) || _.graph_w < 0.0) return false;
        if (!isFiniteF32(_.rrf_k) || _.rrf_k <= 0.0) return false;
        const sum: f32 = _.working_w + _.episodic_w + _.semantic_w + _.graph_w;
        return sum > 0.0;
    }
}

/// Single recall candidate emitted by one of the 4 memory stores.
pub struct MemoryRecallCandidate {
    memory_id: u64 = 0;
    source: MemorySourceStore = MemorySourceStore.working;
    /// 0-based rank within `source` (0 = top-ranked in that store).
    rank_in_source: u32 = 0;
    token_cost: u32 = 0;
    pinned: bool = false;
}

/// Selected and deduplicated memory item written to `out_picks`.
pub struct FusedMemoryPick {
    memory_id: u64 = 0;
    fused_score_f32: f32 = 0.0;
    /// Bitmask of stores that recalled this memory: bit0=working, bit1=episodic, bit2=semantic, bit3=entity_graph.
    source_mask: u8 = 0;
    token_cost: u32 = 0;
    pinned: bool = false;
}

/// Aggregated routing and packing telemetry returned by `fuseAndPackMemoryRecalls`.
pub struct MemoryRouteSummary {
    input_candidates: usize = 0;
    unique_memories: usize = 0;
    selected_count: usize = 0;
    total_tokens: u32 = 0;
    /// Number of selected memories recalled independently by >= 2 distinct stores.
    multi_store_consensus_count: usize = 0;
}

struct AggSlot {
    pick: FusedMemoryPick = FusedMemoryPick{},
    per_source_best: [4]f32 = [0.0, 0.0, 0.0, 0.0],
}

fn aggRanksBefore(a: FusedMemoryPick, b: FusedMemoryPick): bool {
    if (a.pinned != b.pinned) {
        return a.pinned;
    }
    const diff: f32 = a.fused_score_f32 - b.fused_score_f32;
    if (diff > SCORE_TIE_EPS) return true;
    if (diff < -SCORE_TIE_EPS) return false;
    return a.memory_id < b.memory_id;
}

// ============================================================================
// 2. Zero-Alloc Cross-Tier RRF Fusion & Greedy Token-Budget Packer
// ============================================================================

/// Fuses recall candidates from Working, Episodic, Semantic, and Entity Graph stores via
/// weighted Reciprocal Rank Fusion (`w_s / (rrf_k + rank + 1)`), deduplicates by `memory_id`
/// (preventing same-store duplicate double-counting while rewarding multi-store consensus),
/// prioritizes `pinned` memories, and packs deterministically within `token_budget`.
pub fn fuseAndPackMemoryRecalls(
    candidates: []const MemoryRecallCandidate,
    weights: MemoryTierWeights,
    token_budget: u32,
    out_picks: *[]FusedMemoryPick,
): MemoryRouteSummary {
    const n_in: usize = candidates.len;
    if (!weights.isValid() || n_in == 0 || n_in > MEM_ROUTER_MAX_CANDIDATES || out_picks.len == 0) {
        return MemoryRouteSummary{
            input_candidates: n_in,
            unique_memories: 0,
            selected_count: 0,
            total_tokens: 0,
            multi_store_consensus_count: 0,
        };
    }

    // Step 1: Aggregate and deduplicate by memory_id on fixed stack buffer
    let aggs: [MEM_ROUTER_MAX_CANDIDATES]AggSlot = [AggSlot{}; MEM_ROUTER_MAX_CANDIDATES];
    let unique_count: usize = 0;

    let i: usize = 0;
    while (i < n_in) : (i += 1) {
        const c = candidates[i];
        const s_idx: usize = sourceIndex(c.source);
        const s_bit: u8 = sourceBit(c.source);
        const w: f32 = weightForSource(weights, c.source);
        const rrf_contrib: f32 = w / (weights.rrf_k + ((c.rank_in_source as f32) + 1.0));

        let found_idx: usize = unique_count;
        let u: usize = 0;
        while (u < unique_count) : (u += 1) {
            if (aggs[u].pick.memory_id == c.memory_id) {
                found_idx = u;
                break;
            }
        }

        if (found_idx == unique_count) {
            let slot = AggSlot{
                pick: FusedMemoryPick{
                    memory_id: c.memory_id,
                    fused_score_f32: rrf_contrib,
                    source_mask: s_bit,
                    token_cost: c.token_cost,
                    pinned: c.pinned,
                },
                per_source_best: [0.0, 0.0, 0.0, 0.0],
            };
            slot.per_source_best[s_idx] = rrf_contrib;
            aggs[unique_count] = slot;
            unique_count += 1;
        } else {
            if (c.pinned) {
                aggs[found_idx].pick.pinned = true;
            }
            if ((aggs[found_idx].pick.source_mask & s_bit) == 0) {
                // First time seen from this distinct store -> add its RRF contribution
                aggs[found_idx].pick.source_mask = aggs[found_idx].pick.source_mask | s_bit;
                aggs[found_idx].per_source_best[s_idx] = rrf_contrib;
                aggs[found_idx].pick.fused_score_f32 += rrf_contrib;
            } else if (rrf_contrib > aggs[found_idx].per_source_best[s_idx]) {
                // Duplicate from the SAME store: keep only the best rank without double-counting
                aggs[found_idx].pick.fused_score_f32 += rrf_contrib - aggs[found_idx].per_source_best[s_idx];
                aggs[found_idx].per_source_best[s_idx] = rrf_contrib;
            }
        }
    }

    // Step 2: Deterministic insertion sort by (pinned desc, fused_score_f32 desc, memory_id asc)
    let s: usize = 1;
    while (s < unique_count) : (s += 1) {
        const key_slot = aggs[s];
        let j: usize = s;
        while (j > 0 && aggRanksBefore(key_slot.pick, aggs[j - 1].pick)) {
            aggs[j] = aggs[j - 1];
            j -= 1;
        }
        aggs[j] = key_slot;
    }

    // Step 3: Greedy token-budget packing (skips oversized items to fit smaller high-ranked items)
    let selected: usize = 0;
    let total_tokens: u64 = 0;
    let consensus_cnt: usize = 0;
    const budget_u64: u64 = token_budget as u64;

    i = 0;
    while (i < unique_count && selected < out_picks.len) : (i += 1) {
        const pick = aggs[i].pick;
        const cost_u64: u64 = pick.token_cost as u64;
        if (total_tokens + cost_u64 <= budget_u64) {
            out_picks[selected] = pick;
            selected += 1;
            total_tokens += cost_u64;
            if ((pick.source_mask & (pick.source_mask - 1)) != 0) {
                consensus_cnt += 1;
            }
        }
    }

    return MemoryRouteSummary{
        input_candidates: n_in,
        unique_memories: unique_count,
        selected_count: selected,
        total_tokens: total_tokens as u32,
        multi_store_consensus_count: consensus_cnt,
    };
}
