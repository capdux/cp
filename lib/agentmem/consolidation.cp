// Origin: Pure-CP Stdlib (AI Agent Memory Consolidation, Dedup & Retention Pruning Engine) | Created: 2026-11-22
// Zero-heap-allocation sleep-phase memory consolidation engine for AI Agents:
// 1. In-place duplicate merging by content_hash (accumulating recalls, max strength/timestamp, inheriting pinned)
// 2. Exponential half-life retention decay (2^(-age / half_life_ms), clock-skew safe)
// 3. Graduated tier promotion (ShortTerm -> Episodic -> Semantic)
// 4. Low-strength forgetting pruning and max-age archiving (with strict pinned protection)

const std = @import("std");

const LN2: f64 = 0.693147180559945309417232121458176568;

/// Lifecycle consolidation tier for an AI Agent memory record.
pub enum MemConsolidationTier {
    ShortTerm,
    Episodic,
    Semantic,
    Archived,
}

/// Fixed-size in-place consolidatable memory record (zero heap allocations).
pub struct ConsolidatableRecord {
    id: u32 = 0;
    content_hash: u64 = 0;
    topic_hash: u64 = 0;
    tier: MemConsolidationTier = MemConsolidationTier.ShortTerm;
    strength: f32 = 0.0;
    recall_count: u32 = 0;
    last_recall_ms: u64 = 0;
    pinned: bool = false;
    alive: bool = false;
}

/// Policy parameters governing sleep-phase memory consolidation, promotion, archiving, and pruning.
pub struct ConsolidationPolicy {
    prune_strength_threshold: f32 = 0.15;
    promote_min_recalls: u32 = 3;
    promote_min_strength: f32 = 0.60;
    half_life_ms: u64 = 86400000;
    max_age_archive_ms: u64 = 604800000;

    /// Returns a sensible production default policy (24h half-life, 7d archive age).
    static fn default(): ConsolidationPolicy {
        return ConsolidationPolicy{
            prune_strength_threshold: 0.15,
            promote_min_recalls: 3,
            promote_min_strength: 0.60,
            half_life_ms: 86400000,
            max_age_archive_ms: 604800000,
        };
    }

    /// Validates that thresholds are finite within `[0.0, 1.0]`, `half_life_ms > 0`, and `promote_min_recalls > 0`.
    fn isValid(): bool {
        const prune_ok = (_.prune_strength_threshold == _.prune_strength_threshold)
            && (_.prune_strength_threshold >= 0.0)
            && (_.prune_strength_threshold <= 1.0);
        const prom_ok = (_.promote_min_strength == _.promote_min_strength)
            && (_.promote_min_strength >= 0.0)
            && (_.promote_min_strength <= 1.0);
        return prune_ok && prom_ok && (_.half_life_ms > 0) && (_.promote_min_recalls > 0);
    }
}

/// Summary telemetry produced by a single `consolidateMemoriesInPlace` pass.
pub struct ConsolidationReport {
    merged_duplicates: u32 = 0;
    promoted_count: u32 = 0;
    archived_count: u32 = 0;
    pruned_count: u32 = 0;
    surviving_count: u32 = 0;
}

fn clampUnitStrength(v: f32): f32 {
    if (v != v || v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

fn saturatingAddU32(a: u32, b: u32): u32 {
    const max_u32: u32 = 4294967295;
    if (b > max_u32 - a) return max_u32;
    return a + b;
}

fn tierRank(t: MemConsolidationTier): u8 {
    return switch (t) {
        MemConsolidationTier.Archived => 0,
        MemConsolidationTier.ShortTerm => 1,
        MemConsolidationTier.Episodic => 2,
        MemConsolidationTier.Semantic => 3,
    };
}

/// Computes exponential half-life decay factor `2^(-age_ms / half_life_ms)` in `[0.0, 1.0]`.
pub fn halfLifeDecayFactor(age_ms: u64, half_life_ms: u64): f32 {
    if (half_life_ms == 0) return 0.0;
    if (age_ms == 0) return 1.0;
    const exponent: f64 = -LN2 * ((age_ms as f64) / (half_life_ms as f64));
    if (exponent <= -80.0) return 0.0;
    const factor: f64 = std.math.exp(exponent);
    if (factor <= 0.0) return 0.0;
    if (factor >= 1.0) return 1.0;
    return factor as f32;
}

/// Runs a 4-stage in-place sleep-phase consolidation cycle over `records`:
/// 1. Duplicate merging: alive records sharing a non-zero `content_hash` are merged into the earliest
///    occurrence (saturating sum of `recall_count`, max `strength`, max `last_recall_ms`, OR `pinned`,
///    and highest active `tier`).
/// 2. Half-life decay: decays `strength` by `2^(-age_ms / policy.half_life_ms)` (clock-skew safe).
/// 3. Tier promotion: promotes qualifying (`recall_count >= promote_min_recalls` and
///    `strength >= promote_min_strength`) `ShortTerm -> Episodic` and `Episodic -> Semantic`.
/// 4. Pruning & archiving: prunes non-pinned, non-Semantic records with `strength < prune_strength_threshold`;
///    archives remaining non-pinned, non-Archived records whose age `>= policy.max_age_archive_ms` (when `> 0`).
///    Pinned records are strictly protected from pruning and archiving.
///
/// Returns `null` without mutating `records` if `policy` is invalid.
pub fn consolidateMemoriesInPlace(
    records: *[]ConsolidatableRecord,
    now_ms: u64,
    policy: ConsolidationPolicy,
): ConsolidationReport? {
    if (!policy.isValid()) return null;

    let report = ConsolidationReport{};
    if (records.len == 0) return report;

    // Stage 1: Merge duplicate alive records sharing the same non-zero content_hash
    let i: usize = 0;
    while (i < records.len) : (i += 1) {
        if (!records[i].alive) continue;
        records[i].strength = clampUnitStrength(records[i].strength);
        const h = records[i].content_hash;
        if (h == 0) continue;

        let j: usize = i + 1;
        while (j < records.len) : (j += 1) {
            if (!records[j].alive) continue;
            if (records[j].content_hash != h) continue;

            const dup_strength = clampUnitStrength(records[j].strength);
            records[i].recall_count = saturatingAddU32(records[i].recall_count, records[j].recall_count);
            if (dup_strength > records[i].strength) {
                records[i].strength = dup_strength;
            }
            if (records[j].last_recall_ms > records[i].last_recall_ms) {
                records[i].last_recall_ms = records[j].last_recall_ms;
            }
            if (records[j].pinned) {
                records[i].pinned = true;
            }
            if (tierRank(records[j].tier) > tierRank(records[i].tier)) {
                records[i].tier = records[j].tier;
            }

            records[j].alive = false;
            report.merged_duplicates += 1;
        }
    }

    // Stage 2, 3, 4: Half-life decay -> Promotion -> Pruning / Archiving
    let k: usize = 0;
    while (k < records.len) : (k += 1) {
        if (!records[k].alive) continue;

        const age_ms: u64 = if (now_ms > records[k].last_recall_ms) (now_ms - records[k].last_recall_ms) else 0;

        // Stage 2: Exponential half-life strength decay
        const decay = halfLifeDecayFactor(age_ms, policy.half_life_ms);
        records[k].strength = clampUnitStrength(records[k].strength * decay);

        // Stage 3: Tier promotion (ShortTerm -> Episodic -> Semantic)
        const qualifies_promote = (records[k].recall_count >= policy.promote_min_recalls)
            && (records[k].strength >= policy.promote_min_strength);
        if (qualifies_promote) {
            if (records[k].tier == MemConsolidationTier.ShortTerm) {
                records[k].tier = MemConsolidationTier.Episodic;
                report.promoted_count += 1;
            } else if (records[k].tier == MemConsolidationTier.Episodic) {
                records[k].tier = MemConsolidationTier.Semantic;
                report.promoted_count += 1;
            }
        }

        // Stage 4a: Low-strength forgetting pruning (pinned & Semantic are never pruned)
        if (!records[k].pinned
            && records[k].tier != MemConsolidationTier.Semantic
            && records[k].strength < policy.prune_strength_threshold)
        {
            records[k].alive = false;
            report.pruned_count += 1;
            continue;
        }

        // Stage 4b: Max-age archiving (pinned records remain active in their tier)
        if (!records[k].pinned
            && policy.max_age_archive_ms > 0
            && age_ms >= policy.max_age_archive_ms
            && records[k].tier != MemConsolidationTier.Archived)
        {
            records[k].tier = MemConsolidationTier.Archived;
            report.archived_count += 1;
        }

        report.surviving_count += 1;
    }

    return report;
}

// ============================================================================
// Part 2: Zero-Alloc Episodic Memory Consolidation Planner & SIMD Dedup Engine
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

/// Four-level memory lifecycle tier for episodic consolidation planning.
pub enum EpisodeMemoryTier {
    working,
    episodic,
    semantic_long_term,
    evicted,
}

/// Configuration for zero-alloc episodic memory consolidation, SIMD dedup & budget pruning.
pub struct ConsolidationConfig {
    decay_half_life_ms: u64 = 86400000;
    merge_cosine_threshold: f32 = 0.92;
    promote_access_count: u32 = 3;
    promote_importance_min: f32 = 0.70;
    evict_retention_threshold: f32 = 0.15;
    max_retained_entries: usize = 64;

    /// Validates half-life, cosine threshold `(0, 1]`, unit thresholds `[0, 1]`, and budget `> 0`.
    fn isValid(): bool {
        if (_.decay_half_life_ms == 0 || _.promote_access_count == 0 || _.max_retained_entries == 0) {
            return false;
        }
        if (!isFiniteF32(_.merge_cosine_threshold) || _.merge_cosine_threshold <= 0.0 || _.merge_cosine_threshold > 1.0) {
            return false;
        }
        if (!isFiniteF32(_.promote_importance_min) || _.promote_importance_min < 0.0 || _.promote_importance_min > 1.0) {
            return false;
        }
        if (!isFiniteF32(_.evict_retention_threshold) || _.evict_retention_threshold < 0.0 || _.evict_retention_threshold > 1.0) {
            return false;
        }
        return true;
    }
}

/// Metadata descriptor for an episodic memory entry participating in consolidation planning.
pub struct MemoryEpisodeMeta {
    episode_id: u64 = 0;
    created_ms: u64 = 0;
    last_access_ms: u64 = 0;
    access_count: u32 = 0;
    importance_f32: f32 = 0.0;
    tier: EpisodeMemoryTier = EpisodeMemoryTier.episodic;
    merged_source_count: u32 = 1;
    pinned: bool = false;
}

/// Action assigned to each episode by `planMemoryConsolidation`.
pub enum ConsolidationDecisionKind {
    keep,
    promote_semantic,
    merged_into,
    evict_decay,
    evict_budget,
}

/// Per-episode consolidation action with target representative index and retention score.
pub struct ConsolidationAction {
    episode_idx: usize = 0;
    kind: ConsolidationDecisionKind = ConsolidationDecisionKind.keep;
    target_idx: usize = 0;
    retention_score_f32: f32 = 0.0;
}

/// Aggregate summary returned by `planMemoryConsolidation`.
pub struct ConsolidationSummary {
    total_input: usize = 0;
    kept_count: usize = 0;
    promoted_count: usize = 0;
    merged_count: usize = 0;
    evicted_decay_count: usize = 0;
    evicted_budget_count: usize = 0;
}

/// Computes deterministic retention score in `[0.0, 1.0]` combining exponential half-life decay,
/// logarithmic access-frequency boost, and clamped importance.
pub fn computeRetentionScore(meta: MemoryEpisodeMeta, now_ms: u64, half_life_ms: u64): f32 {
    if (half_life_ms == 0) {
        return 0.0;
    }
    const age_ms: u64 = if (now_ms > meta.last_access_ms) now_ms - meta.last_access_ms else 0;
    const decay: f32 = halfLifeDecayFactor(age_ms, half_life_ms);
    const freq_boost: f64 = 1.0 + 0.25 * std.math.log(f64, std.math.e, 1.0 + (meta.access_count as f64));
    const imp: f32 = clampUnitStrength(meta.importance_f32);
    const base: f32 = 0.4 + 0.6 * imp;
    const raw: f32 = decay * (freq_boost as f32) * base;
    return clampUnitStrength(raw);
}

/// Computes cosine similarity in `[-1.0, 1.0]` between two equal-length `f32` episode embeddings
/// using CP's native 8-lane SIMD `@dot` builtin. Returns `0.0` on empty/mismatched/zero/NaN input.
pub fn cosineSimEpisodeF32(a: []const f32, b: []const f32): f32 {
    if (a.len == 0 || a.len != b.len) {
        return 0.0;
    }
    const na: f32 = @dot(a, a);
    const nb: f32 = @dot(b, b);
    const ab: f32 = @dot(a, b);
    if (!isFiniteF32(na) || !isFiniteF32(nb) || !isFiniteF32(ab)) {
        return 0.0;
    }
    if (na <= 0.000000000001 || nb <= 0.000000000001) {
        return 0.0;
    }
    const denom: f32 = @sqrt(na) * @sqrt(nb);
    if (!isFiniteF32(denom) || denom <= 0.0) {
        return 0.0;
    }
    const cos_val: f32 = ab / denom;
    if (cos_val > 1.0) {
        return 1.0;
    }
    if (cos_val < -1.0) {
        return -1.0;
    }
    return cos_val;
}

/// Zero-heap-allocation episodic memory consolidation planner:
/// 1. Computes retention score for every episode
/// 2. Merges near-duplicate episodes (`cosineSimEpisodeF32 >= merge_cosine_threshold`) into the
///    higher-retention (or pinned) representative via `@dot` SIMD
/// 3. Evicts low-retention non-pinned, non-semantic episodes (`evict_decay`) and promotes
///    qualifying `episodic` memories to `semantic_long_term` (`promote_semantic`)
/// 4. Enforces `max_retained_entries` budget by evicting lowest-retention non-pinned episodes (`evict_budget`)
///
/// Returns zeroed `ConsolidationSummary` if input is empty, invalid, or buffers are undersized.
pub fn planMemoryConsolidation(
    episodes: []const MemoryEpisodeMeta,
    embeddings: []const f32,
    dim: usize,
    now_ms: u64,
    cfg: ConsolidationConfig,
    out_actions: *[]ConsolidationAction,
): ConsolidationSummary {
    const n: usize = episodes.len;
    if (n == 0 || dim == 0 || !cfg.isValid() || out_actions.len < n) {
        return ConsolidationSummary{};
    }
    if (n > std.math.maxInt(usize) / dim) {
        return ConsolidationSummary{};
    }
    if (embeddings.len < n * dim) {
        return ConsolidationSummary{};
    }

    // Pass 1: Initialize actions & compute retention scores
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out_actions[i] = ConsolidationAction{
            episode_idx: i,
            kind: ConsolidationDecisionKind.keep,
            target_idx: i,
            retention_score_f32: computeRetentionScore(episodes[i], now_ms, cfg.decay_half_life_ms),
        };
    }

    // Pass 2: @dot SIMD near-duplicate clustering & merging into best representative
    let idx_i: usize = 0;
    while (idx_i < n) : (idx_i += 1) {
        if (out_actions[idx_i].kind == ConsolidationDecisionKind.merged_into) {
            continue;
        }
        const emb_i = embeddings[idx_i * dim .. (idx_i + 1) * dim];
        let j: usize = idx_i + 1;
        while (j < n) : (j += 1) {
            if (out_actions[j].kind == ConsolidationDecisionKind.merged_into) {
                continue;
            }
            const emb_j = embeddings[j * dim .. (j + 1) * dim];
            const sim: f32 = cosineSimEpisodeF32(emb_i, emb_j);
            if (sim >= cfg.merge_cosine_threshold) {
                const i_pin = episodes[idx_i].pinned;
                const j_pin = episodes[j].pinned;
                const j_better: bool = (!i_pin && j_pin) ||
                    (i_pin == j_pin && out_actions[j].retention_score_f32 > out_actions[idx_i].retention_score_f32);

                if (j_better) {
                    out_actions[idx_i].kind = ConsolidationDecisionKind.merged_into;
                    out_actions[idx_i].target_idx = j;
                    // Redirect any prior episodes that merged into idx_i directly to j
                    let k: usize = 0;
                    while (k < n) : (k += 1) {
                        if (out_actions[k].kind == ConsolidationDecisionKind.merged_into && out_actions[k].target_idx == idx_i) {
                            out_actions[k].target_idx = j;
                        }
                    }
                    break;
                } else {
                    out_actions[j].kind = ConsolidationDecisionKind.merged_into;
                    out_actions[j].target_idx = idx_i;
                }
            }
        }
    }

    // Pass 3: Decay eviction & semantic promotion
    let retained: usize = 0;
    let p: usize = 0;
    while (p < n) : (p += 1) {
        if (out_actions[p].kind == ConsolidationDecisionKind.merged_into) {
            continue;
        }
        const ep = episodes[p];
        if (!ep.pinned &&
            ep.tier != EpisodeMemoryTier.semantic_long_term &&
            out_actions[p].retention_score_f32 < cfg.evict_retention_threshold)
        {
            out_actions[p].kind = ConsolidationDecisionKind.evict_decay;
            continue;
        }
        if (ep.tier == EpisodeMemoryTier.episodic &&
            ep.access_count >= cfg.promote_access_count &&
            ep.importance_f32 >= cfg.promote_importance_min)
        {
            out_actions[p].kind = ConsolidationDecisionKind.promote_semantic;
        }
        retained += 1;
    }

    // Pass 4: Budget pruning (evict lowest-retention non-pinned episodes until retained <= max_retained_entries)
    while (retained > cfg.max_retained_entries) {
        let victim_opt: usize? = null;
        let min_score: f32 = 2.0;
        let c: usize = 0;
        while (c < n) : (c += 1) {
            const knd = out_actions[c].kind;
            if (knd != ConsolidationDecisionKind.keep && knd != ConsolidationDecisionKind.promote_semantic) {
                continue;
            }
            if (episodes[c].pinned) {
                continue;
            }
            if (victim_opt == null || out_actions[c].retention_score_f32 < min_score) {
                victim_opt = c;
                min_score = out_actions[c].retention_score_f32;
            }
        }
        if (v_idx := victim_opt) {
            out_actions[v_idx].kind = ConsolidationDecisionKind.evict_budget;
            retained -= 1;
        } else {
            // All remaining entries are pinned -> never evict pinned memories
            break;
        }
    }

    // Pass 5: Aggregate summary
    let summary = ConsolidationSummary{ total_input: n };
    let s: usize = 0;
    while (s < n) : (s += 1) {
        if (out_actions[s].kind == ConsolidationDecisionKind.keep) {
            summary.kept_count += 1;
        } else if (out_actions[s].kind == ConsolidationDecisionKind.promote_semantic) {
            summary.promoted_count += 1;
        } else if (out_actions[s].kind == ConsolidationDecisionKind.merged_into) {
            summary.merged_count += 1;
        } else if (out_actions[s].kind == ConsolidationDecisionKind.evict_decay) {
            summary.evicted_decay_count += 1;
        } else {
            summary.evicted_budget_count += 1;
        }
    }
    return summary;
}
