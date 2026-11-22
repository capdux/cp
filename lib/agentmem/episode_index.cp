// Origin: Pure-CP Stdlib (Hybrid Temporal-Entity-Vector Episodic Memory Index for Long-Horizon AI Agents) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation tri-modal episodic memory index:
//   - Fixed-capacity episodic slots (`episode_id`, `timestamp_ms`, `session_id`, `entity_mask: u64`, `importance`, L2-normalized vector `dim <= 32`)
//   - Tri-modal hybrid retrieval (`queryHybrid`): temporal window + entity bitset filtering (`required`/`any`/`excluded`) +
//     hardware `@popCount` entity overlap + native `@dot` cosine similarity + recency decay + `@fma` weighted scoring
//   - Chronological session timeline replay (`scanSessionTimeline`) & pinned-safe watermark pruning (`pruneBeforeTimestamp`)

const std = @import("std");

// ============================================================================
// Constants
// ============================================================================

/// Fixed maximum number of active episodic memory slots inside `HybridEpisodeIndex`.
pub const EP_INDEX_MAX_SLOTS: usize = 128;

/// Maximum supported vector dimensionality (`1..=32`).
pub const EP_INDEX_MAX_DIM: usize = 32;

const LN2_F64: f64 = 0.6931471805599453;

// ============================================================================
// Internal Numeric & Bitset Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clamp01F32(x: f32): f32 {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

fn clampCosineF32(x: f32): f32 {
    if (x < -1.0) return -1.0;
    if (x > 1.0) return 1.0;
    return x;
}

fn normalizeIntoBuf(src: []const f32, dst: *[EP_INDEX_MAX_DIM]f32): bool {
    if (src.len == 0 || src.len > EP_INDEX_MAX_DIM) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        if (!isFiniteF32(src[i])) return false;
    }
    const nsq: f32 = @dot(src, src);
    if (!isFiniteF32(nsq) || nsq <= 1.0e-12) return false;
    const inv_norm: f32 = (1.0 as f32) / @sqrt(nsq);

    let d: usize = 0;
    while (d < src.len) : (d += 1) {
        dst[d] = src[d] * inv_norm;
    }
    while (d < EP_INDEX_MAX_DIM) : (d += 1) {
        dst[d] = 0.0;
    }
    return true;
}

fn computeRecencyScore(ts_ms: u64, now_ms: u64, half_life_ms: u64): f32 {
    if (now_ms <= ts_ms) return 1.0;
    const age_f: f64 = (now_ms - ts_ms) as f64;
    const hl_f: f64 = half_life_ms as f64;
    const exponent: f64 = -LN2_F64 * (age_f / hl_f);
    if (exponent < -80.0) return 0.0;
    return std.math.exp(exponent) as f32;
}

fn computeEntityOverlapScore(ep_mask: u64, target_mask: u64): f32 {
    if (target_mask == 0) return 0.0;
    const denom: usize = @popCount(target_mask) as usize;
    if (denom == 0) return 0.0;
    const overlap: usize = @popCount(ep_mask & target_mask) as usize;
    return (overlap as f32) / (denom as f32);
}

/// Deterministic strict total order for Top-K hits:
///   1) Higher `total_score` first
///   2) Newer `timestamp_ms` first
///   3) Smaller `episode_id` first
fn hitRanksAhead(a: EpisodeHybridHit, b: EpisodeHybridHit): bool {
    if (a.total_score > b.total_score) return true;
    if (a.total_score < b.total_score) return false;
    if (a.timestamp_ms > b.timestamp_ms) return true;
    if (a.timestamp_ms < b.timestamp_ms) return false;
    return a.episode_id < b.episode_id;
}

// ============================================================================
// 1. Public Data Models
// ============================================================================

/// Stored episodic memory record inside `HybridEpisodeIndex`.
pub struct EpisodeRecord {
    active: bool = false;
    episode_id: u64 = 0;
    timestamp_ms: u64 = 0;
    session_id: u64 = 0;
    entity_mask: u64 = 0;
    importance_f32: f32 = 0.0;
    pinned: bool = false;
    vec: [EP_INDEX_MAX_DIM]f32 = [0.0; EP_INDEX_MAX_DIM];
}

/// Temporal, session, and entity-bitset filter for `queryHybrid(...)`.
pub struct EpisodeQueryFilter {
    /// `0` matches all sessions; non-zero restricts results to that `session_id`.
    session_id: u64 = 0;
    /// Inclusive lower timestamp bound (ms).
    min_ts_ms: u64 = 0;
    /// Inclusive upper timestamp bound (ms).
    max_ts_ms: u64 = 0xFFFFFFFFFFFFFFFF;
    /// All bits in `required_entities` must be present in `episode.entity_mask`.
    required_entities: u64 = 0;
    /// If non-zero, at least one bit in `any_entities` must be present in `episode.entity_mask`.
    any_entities: u64 = 0;
    /// If non-zero, no bit in `excluded_entities` may be present in `episode.entity_mask`.
    excluded_entities: u64 = 0;
    /// Target entity bitmask used to compute `@popCount` overlap score.
    /// When `0`, defaults to `required_entities | any_entities`.
    target_entities: u64 = 0;

    /// Unconstrained filter matching all sessions, all timestamps, and all entities.
    static fn all(): EpisodeQueryFilter {
        return EpisodeQueryFilter{};
    }

    /// Returns `true` iff `min_ts_ms <= max_ts_ms`.
    fn isValid(): bool {
        return _.min_ts_ms <= _.max_ts_ms;
    }
}

/// Scoring weights and recency half-life for tri-modal hybrid ranking.
pub struct EpisodeHybridWeights {
    vector_weight: f32 = 0.50;
    recency_weight: f32 = 0.20;
    entity_weight: f32 = 0.15;
    importance_weight: f32 = 0.15;
    recency_half_life_ms: u64 = 3_600_000;

    /// Default balanced weights (0.50 vector / 0.20 recency / 0.15 entity / 0.15 importance, 1h half-life).
    static fn defaults(): EpisodeHybridWeights {
        return EpisodeHybridWeights{};
    }

    /// Validates that all weights are finite non-negative and `recency_half_life_ms > 0`.
    fn isValid(): bool {
        if (!isFiniteF32(_.vector_weight) || _.vector_weight < 0.0) return false;
        if (!isFiniteF32(_.recency_weight) || _.recency_weight < 0.0) return false;
        if (!isFiniteF32(_.entity_weight) || _.entity_weight < 0.0) return false;
        if (!isFiniteF32(_.importance_weight) || _.importance_weight < 0.0) return false;
        if (_.recency_half_life_ms == 0) return false;
        return true;
    }
}

/// Ranked recall hit returned by `queryHybrid(...)` with full per-modality score breakdown.
pub struct EpisodeHybridHit {
    episode_id: u64 = 0;
    timestamp_ms: u64 = 0;
    session_id: u64 = 0;
    entity_mask: u64 = 0;
    total_score: f32 = 0.0;
    vector_score: f32 = 0.0;
    recency_score: f32 = 0.0;
    entity_score: f32 = 0.0;
    importance_score: f32 = 0.0;
}

/// Telemetry counters for `HybridEpisodeIndex`.
pub struct EpisodeIndexStats {
    dim: usize = 0;
    active_count: usize = 0;
    pinned_count: usize = 0;
    total_inserted: u64 = 0;
    total_pruned: u64 = 0;
    total_queries: u64 = 0;
}

// ============================================================================
// 2. HybridEpisodeIndex (Zero-Alloc Tri-Modal Episodic Memory Engine)
// ============================================================================

/// Zero-heap-allocation hybrid temporal + entity-bitset + `@dot` vector episodic memory index.
pub struct HybridEpisodeIndex {
    dim: usize = 0;
    slots: [EP_INDEX_MAX_SLOTS]EpisodeRecord = [EpisodeRecord{}; EP_INDEX_MAX_SLOTS];
    active_count: usize = 0;
    pinned_count: usize = 0;
    total_inserted: u64 = 0;
    total_pruned: u64 = 0;
    total_queries: u64 = 0;

    /// Initializes a fresh index for vectors of dimensionality `dim` (`1..=EP_INDEX_MAX_DIM`).
    /// Returns `null` if `dim == 0` or `dim > EP_INDEX_MAX_DIM`.
    static fn init(dim: usize): HybridEpisodeIndex? {
        if (dim == 0 || dim > EP_INDEX_MAX_DIM) return null;
        return HybridEpisodeIndex{
            dim: dim,
        };
    }

    fn findSlotById(episode_id: u64): usize? {
        let i: usize = 0;
        while (i < EP_INDEX_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].active && _.slots[i].episode_id == episode_id) {
                return i;
            }
        }
        return null;
    }

    /// Inserts a new episodic memory record, normalizing `raw_vec` into L2 unit length.
    /// Returns `false` on `episode_id == 0`, duplicate `episode_id`, dimension mismatch,
    /// zero/non-finite vector, non-finite importance, or full slot pool.
    fn insertEpisode(
        *_,
        episode_id: u64,
        timestamp_ms: u64,
        session_id: u64,
        entity_mask: u64,
        importance_f32: f32,
        pinned: bool,
        raw_vec: []const f32,
    ): bool {
        if (episode_id == 0 || raw_vec.len != _.dim) return false;
        if (!isFiniteF32(importance_f32)) return false;
        if (_.active_count >= EP_INDEX_MAX_SLOTS) return false;
        if (_.findSlotById(episode_id) != null) return false;

        let norm_vec: [EP_INDEX_MAX_DIM]f32 = [0.0; EP_INDEX_MAX_DIM];
        if (!normalizeIntoBuf(raw_vec, &norm_vec)) return false;

        let free_slot: usize = EP_INDEX_MAX_SLOTS;
        let i: usize = 0;
        while (i < EP_INDEX_MAX_SLOTS) : (i += 1) {
            if (!_.slots[i].active) {
                free_slot = i;
                break;
            }
        }
        if (free_slot >= EP_INDEX_MAX_SLOTS) return false;

        _.slots[free_slot] = EpisodeRecord{
            active: true,
            episode_id: episode_id,
            timestamp_ms: timestamp_ms,
            session_id: session_id,
            entity_mask: entity_mask,
            importance_f32: clamp01F32(importance_f32),
            pinned: pinned,
            vec: norm_vec,
        };
        _.active_count += 1;
        if (pinned) {
            _.pinned_count += 1;
        }
        _.total_inserted += 1;
        return true;
    }

    /// Updates the `pinned` protection flag for `episode_id`.
    fn setPinned(*_, episode_id: u64, pinned: bool): bool {
        const idx_opt = _.findSlotById(episode_id);
        if (idx_opt == null) return false;
        const idx: usize = idx_opt ?? 0;
        const old_p: bool = _.slots[idx].pinned;
        if (old_p != pinned) {
            _.slots[idx].pinned = pinned;
            if (pinned) {
                _.pinned_count += 1;
            } else if (_.pinned_count > 0) {
                _.pinned_count -= 1;
            }
        }
        return true;
    }

    /// Retrieves a stored `EpisodeRecord` by `episode_id`, or `null` if absent.
    fn getEpisode(episode_id: u64): EpisodeRecord? {
        if (idx := _.findSlotById(episode_id)) {
            return _.slots[idx];
        }
        return null;
    }

    /// Executes tri-modal hybrid retrieval (`temporal window` + `entity bitset` + `@dot` vector)
    /// and writes strictly descending Top-K hits into `out_hits`.
    /// Returns the number of hits written (`0` on empty index, invalid filter/weights, or zero query vector).
    fn queryHybrid(
        *_,
        query_vec: []const f32,
        filter: EpisodeQueryFilter,
        weights: EpisodeHybridWeights,
        now_ms: u64,
        out_hits: *[]EpisodeHybridHit,
    ): usize {
        _.total_queries += 1;
        if (_.active_count == 0 || out_hits.len == 0) return 0;
        if (!filter.isValid() || !weights.isValid()) return 0;
        if (query_vec.len != _.dim) return 0;

        let q_norm: [EP_INDEX_MAX_DIM]f32 = [0.0; EP_INDEX_MAX_DIM];
        if (!normalizeIntoBuf(query_vec, &q_norm)) return 0;

        const target_mask: u64 = if (filter.target_entities != 0)
            filter.target_entities
        else
            (filter.required_entities | filter.any_entities);
        const d: usize = _.dim;

        let written: usize = 0;
        let i: usize = 0;
        while (i < EP_INDEX_MAX_SLOTS) : (i += 1) {
            const ep = _.slots[i];
            if (!ep.active) continue;

            // 1) Temporal & session filters
            if (ep.timestamp_ms < filter.min_ts_ms || ep.timestamp_ms > filter.max_ts_ms) continue;
            if (filter.session_id != 0 && ep.session_id != filter.session_id) continue;

            // 2) Entity bitset filters (required / any / excluded)
            if ((ep.entity_mask & filter.required_entities) != filter.required_entities) continue;
            if (filter.any_entities != 0 && (ep.entity_mask & filter.any_entities) == 0) continue;
            if (filter.excluded_entities != 0 && (ep.entity_mask & filter.excluded_entities) != 0) continue;

            // 3) Tri-modal scoring via native @dot, @popCount, and @fma
            const q_slice: []const f32 = q_norm[0..d];
            const ep_slice: []const f32 = ep.vec[0..d];
            const vec_score: f32 = clampCosineF32(@dot(q_slice, ep_slice));
            const rec_score: f32 = computeRecencyScore(ep.timestamp_ms, now_ms, weights.recency_half_life_ms);
            const ent_score: f32 = computeEntityOverlapScore(ep.entity_mask, target_mask);
            const imp_score: f32 = ep.importance_f32;

            const total: f32 = @fma(
                weights.vector_weight,
                vec_score,
                @fma(
                    weights.recency_weight,
                    rec_score,
                    @fma(weights.entity_weight, ent_score, weights.importance_weight * imp_score),
                ),
            );

            const cand = EpisodeHybridHit{
                episode_id: ep.episode_id,
                timestamp_ms: ep.timestamp_ms,
                session_id: ep.session_id,
                entity_mask: ep.entity_mask,
                total_score: total,
                vector_score: vec_score,
                recency_score: rec_score,
                entity_score: ent_score,
                importance_score: imp_score,
            };

            // Maintain sorted Top-K array in `out_hits[0..written]`
            if (written < out_hits.len) {
                let pos: usize = written;
                while (pos > 0 && hitRanksAhead(cand, out_hits[pos - 1])) : (pos -= 1) {
                    out_hits[pos] = out_hits[pos - 1];
                }
                out_hits[pos] = cand;
                written += 1;
            } else if (hitRanksAhead(cand, out_hits[written - 1])) {
                let pos: usize = written - 1;
                while (pos > 0 && hitRanksAhead(cand, out_hits[pos - 1])) : (pos -= 1) {
                    out_hits[pos] = out_hits[pos - 1];
                }
                out_hits[pos] = cand;
            }
        }

        return written;
    }

    /// Replays all active episode IDs belonging to `session_id` within `[min_ts_ms, max_ts_ms]`,
    /// ordered chronologically ascending by `(timestamp_ms, episode_id)`.
    fn scanSessionTimeline(
        session_id: u64,
        min_ts_ms: u64,
        max_ts_ms: u64,
        out_ids: *[]u64,
    ): usize {
        if (session_id == 0 || min_ts_ms > max_ts_ms || out_ids.len == 0) return 0;

        let ts_buf: [EP_INDEX_MAX_SLOTS]u64 = [0; EP_INDEX_MAX_SLOTS];
        let id_buf: [EP_INDEX_MAX_SLOTS]u64 = [0; EP_INDEX_MAX_SLOTS];
        let matched: usize = 0;

        let i: usize = 0;
        while (i < EP_INDEX_MAX_SLOTS) : (i += 1) {
            const ep = _.slots[i];
            if (!ep.active) continue;
            if (ep.session_id != session_id) continue;
            if (ep.timestamp_ms < min_ts_ms || ep.timestamp_ms > max_ts_ms) continue;

            // Insertion sort ascending by (timestamp_ms, episode_id)
            let pos: usize = matched;
            while (pos > 0 && (ts_buf[pos - 1] > ep.timestamp_ms ||
                (ts_buf[pos - 1] == ep.timestamp_ms && id_buf[pos - 1] > ep.episode_id))) : (pos -= 1)
            {
                ts_buf[pos] = ts_buf[pos - 1];
                id_buf[pos] = id_buf[pos - 1];
            }
            ts_buf[pos] = ep.timestamp_ms;
            id_buf[pos] = ep.episode_id;
            matched += 1;
        }

        const to_copy: usize = if (matched < out_ids.len) matched else out_ids.len;
        let k: usize = 0;
        while (k < to_copy) : (k += 1) {
            out_ids[k] = id_buf[k];
        }
        return to_copy;
    }

    /// Archives and reclaims all unpinned episodes strictly older than `cutoff_ms`.
    /// Episodes with `pinned == true` are strictly protected from pruning.
    fn pruneBeforeTimestamp(*_, cutoff_ms: u64): usize {
        let pruned: usize = 0;
        let i: usize = 0;
        while (i < EP_INDEX_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].active && !_.slots[i].pinned && _.slots[i].timestamp_ms < cutoff_ms) {
                _.slots[i].active = false;
                _.slots[i].episode_id = 0;
                if (_.active_count > 0) {
                    _.active_count -= 1;
                }
                pruned += 1;
            }
        }
        _.total_pruned += pruned as u64;
        return pruned;
    }

    /// Returns index telemetry counters.
    fn stats(): EpisodeIndexStats {
        return EpisodeIndexStats{
            dim: _.dim,
            active_count: _.active_count,
            pinned_count: _.pinned_count,
            total_inserted: _.total_inserted,
            total_pruned: _.total_pruned,
            total_queries: _.total_queries,
        };
    }
}
