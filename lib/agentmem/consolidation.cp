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
