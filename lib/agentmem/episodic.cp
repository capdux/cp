// Origin: Pure-CP Stdlib (AI Agent Episodic Memory: Park 2023 Recency-Importance-Relevance & Ebbinghaus Decay) | Created: 2026-11-22
// Pure CP zero-allocation episodic memory scoring, Ebbinghaus retention decay,
// spaced-repetition reinforcement, deterministic Top-K ranking, and pinned-safe eviction.

const std = @import("std");

const MAX_STABILITY_HOURS: f64 = 10000.0;

/// Weight configuration for Park et al. (Generative Agents, 2023) 3-factor episodic recall
/// combined with Ebbinghaus hourly decay rate.
pub struct MemoryWeights {
    recency: f64 = 1.0,
    importance: f64 = 1.0,
    relevance: f64 = 1.0,
    /// Hourly exponential decay rate (lambda >= 0.0). Effective stability S = strength / decay_rate.
    decay_rate: f64 = 0.05,

    pub static fn default(): MemoryWeights {
        return MemoryWeights{
            recency: 1.0,
            importance: 1.0,
            relevance: 1.0,
            decay_rate: 0.05,
        };
    }
}

/// Lightweight fixed-layout metadata for a single episodic memory slot (zero heap ownership).
pub struct EpisodeMeta {
    id: u32 = 0,
    created_at_sec: i64 = 0,
    last_accessed_sec: i64 = 0,
    access_count: u32 = 0,
    importance: f64 = 0.5,
    strength: f64 = 1.0,
    pinned: bool = false,
}

/// Scored episodic retrieval candidate with full 3-factor score breakdown.
pub struct ScoredEpisode {
    id: u32 = 0,
    total_score: f64 = 0.0,
    recency_score: f64 = 0.0,
    importance_score: f64 = 0.0,
    relevance_score: f64 = 0.0,
}

// ============================================================================
// Internal Validation & Math Helpers
// ============================================================================

fn isFinite(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn isValidWeights(w: MemoryWeights): bool {
    if (!isFinite(w.recency) || !isFinite(w.importance) || !isFinite(w.relevance) || !isFinite(w.decay_rate)) {
        return false;
    }
    if (w.recency < 0.0 || w.importance < 0.0 || w.relevance < 0.0 || w.decay_rate < 0.0) {
        return false;
    }
    return (w.recency + w.importance + w.relevance) > 0.0;
}

fn isValidUnitScore(x: f64): bool {
    return isFinite(x) && x >= 0.0 && x <= 1.0;
}

fn elapsedHours(last_accessed_sec: i64, now_sec: i64): f64 {
    if (now_sec <= last_accessed_sec) {
        return 0.0;
    }
    const diff_sec: i64 = now_sec - last_accessed_sec;
    return (diff_sec as f64) / 3600.0;
}

fn computeRetention(ep: EpisodeMeta, now_sec: i64, decay_rate: f64): f64 {
    if (decay_rate == 0.0) {
        return 1.0;
    }
    const dt_hours: f64 = elapsedHours(ep.last_accessed_sec, now_sec);
    const stability_hours: f64 = ep.strength / decay_rate;
    return ebbinghausRetention(dt_hours, stability_hours);
}

fn isBetterScored(a: ScoredEpisode, b: ScoredEpisode): bool {
    if (a.total_score > b.total_score) {
        return true;
    }
    if (a.total_score < b.total_score) {
        return false;
    }
    return a.id < b.id;
}

// ============================================================================
// 1. Ebbinghaus Forgetting Curve & Spaced-Repetition Reinforcement
// ============================================================================

/// Computes Ebbinghaus memory retention $R = e^{-t / S}$ in `[0.0, 1.0]`.
/// Guards against `NaN`, non-positive stability, and negative elapsed time.
pub fn ebbinghausRetention(elapsed_hours: f64, stability_hours: f64): f64 {
    if (std.math.isNan(elapsed_hours) || std.math.isNan(stability_hours)) {
        return 0.0;
    }
    if (elapsed_hours <= 0.0) {
        return 1.0;
    }
    if (std.math.isInf(elapsed_hours)) {
        return 0.0;
    }
    if (stability_hours <= 0.0) {
        return 0.0;
    }
    if (std.math.isInf(stability_hours)) {
        return 1.0;
    }
    const ratio: f64 = elapsed_hours / stability_hours;
    const r: f64 = std.math.exp(-ratio);
    if (r < 0.0) {
        return 0.0;
    }
    if (r > 1.0) {
        return 1.0;
    }
    return r;
}

/// Reinforces memory stability upon successful recall using diminishing spaced-repetition gain.
/// Strictly increases positive stability while capping at `MAX_STABILITY_HOURS`.
pub fn reinforceStability(stability: f64, access_count: u32): f64 {
    let base: f64 = stability;
    if (!isFinite(base) || base <= 0.0) {
        base = 1.0;
    }
    const gain: f64 = 1.0 + 0.35 / (1.0 + 0.1 * (access_count as f64));
    const next: f64 = base * gain;
    if (next > MAX_STABILITY_HOURS) {
        return MAX_STABILITY_HOURS;
    }
    return next;
}

// ============================================================================
// 2. Park 2023 3-Factor Episodic Scoring
// ============================================================================

/// Scores a single episodic memory using Park et al. (2023) normalized 3-factor weights:
/// recency (Ebbinghaus retention modulated by `strength / decay_rate`), importance, and relevance.
/// Returns `null` if weights, `importance`, `strength`, or `relevance_01` are invalid.
pub fn scoreEpisode(ep: EpisodeMeta, now_sec: i64, relevance_01: f64, w: MemoryWeights): ScoredEpisode? {
    if (!isValidWeights(w)) {
        return null;
    }
    if (!isValidUnitScore(ep.importance) || !isValidUnitScore(relevance_01)) {
        return null;
    }
    if (!isFinite(ep.strength) || ep.strength <= 0.0) {
        return null;
    }

    const recency_score: f64 = computeRetention(ep, now_sec, w.decay_rate);
    const importance_score: f64 = ep.importance;
    const relevance_score: f64 = relevance_01;
    const weight_sum: f64 = w.recency + w.importance + w.relevance;
    const total_score: f64 = (w.recency * recency_score + w.importance * importance_score + w.relevance * relevance_score) / weight_sum;

    return ScoredEpisode{
        id: ep.id,
        total_score: total_score,
        recency_score: recency_score,
        importance_score: importance_score,
        relevance_score: relevance_score,
    };
}

// ============================================================================
// 3. Fixed-Pool Top-K Ranking, Eviction Selection & Recall Touch
// ============================================================================

/// Ranks episodic memories in descending `total_score` order (deterministic tie-break by ascending `id`)
/// into caller-provided `out_scored` with zero heap allocation.
/// Returns the number of entries written (`min(top_k, episodes.len)`), or `null` on invalid inputs
/// (mismatched slice lengths, invalid weights/scores, or insufficient `out_scored` capacity).
pub fn rankTopEpisodes(
    episodes: []const EpisodeMeta,
    relevances: []const f64,
    now_sec: i64,
    w: MemoryWeights,
    top_k: usize,
    out_scored: *[]ScoredEpisode,
): usize? {
    if (episodes.len != relevances.len || !isValidWeights(w)) {
        return null;
    }

    const k: usize = if (top_k < episodes.len) top_k else episodes.len;
    if (k == 0) {
        return 0;
    }
    if (out_scored.len < k) {
        return null;
    }

    let count: usize = 0;
    let i: usize = 0;
    while (i < episodes.len) : (i += 1) {
        const cand_opt = scoreEpisode(episodes[i], now_sec, relevances[i], w);
        let cand = ScoredEpisode{};
        if (sc := cand_opt) {
            cand = sc;
        } else {
            return null;
        }

        if (count < k) {
            out_scored[count] = cand;
            let pos: usize = count;
            while (pos > 0 && isBetterScored(out_scored[pos], out_scored[pos - 1])) {
                const tmp = out_scored[pos - 1];
                out_scored[pos - 1] = out_scored[pos];
                out_scored[pos] = tmp;
                pos -= 1;
            }
            count += 1;
        } else if (isBetterScored(cand, out_scored[k - 1])) {
            out_scored[k - 1] = cand;
            let pos: usize = k - 1;
            while (pos > 0 && isBetterScored(out_scored[pos], out_scored[pos - 1])) {
                const tmp = out_scored[pos - 1];
                out_scored[pos - 1] = out_scored[pos];
                out_scored[pos] = tmp;
                pos -= 1;
            }
        }
    }

    return count;
}

/// Selects the slot index of the best eviction candidate among non-pinned episodes,
/// minimizing `retention * importance`. Ties break by older `last_accessed_sec`, then ascending `id`.
/// Returns `null` if `episodes` is empty, all episodes are `pinned`, or inputs are invalid.
pub fn selectEvictionVictim(episodes: []const EpisodeMeta, now_sec: i64, w: MemoryWeights): usize? {
    if (episodes.len == 0 || !isValidWeights(w)) {
        return null;
    }

    let found: bool = false;
    let victim_idx: usize = 0;
    let victim_utility: f64 = 0.0;
    let victim_last_acc: i64 = 0;
    let victim_id: u32 = 0;

    let i: usize = 0;
    while (i < episodes.len) : (i += 1) {
        const ep = episodes[i];
        if (ep.pinned) {
            continue;
        }
        if (!isValidUnitScore(ep.importance) || !isFinite(ep.strength) || ep.strength <= 0.0) {
            return null;
        }
        const retention: f64 = computeRetention(ep, now_sec, w.decay_rate);
        const utility: f64 = retention * ep.importance;

        if (!found) {
            found = true;
            victim_idx = i;
            victim_utility = utility;
            victim_last_acc = ep.last_accessed_sec;
            victim_id = ep.id;
        } else if (utility < victim_utility) {
            victim_idx = i;
            victim_utility = utility;
            victim_last_acc = ep.last_accessed_sec;
            victim_id = ep.id;
        } else if (utility == victim_utility) {
            if (ep.last_accessed_sec < victim_last_acc || (ep.last_accessed_sec == victim_last_acc && ep.id < victim_id)) {
                victim_idx = i;
                victim_utility = utility;
                victim_last_acc = ep.last_accessed_sec;
                victim_id = ep.id;
            }
        }
    }

    if (!found) {
        return null;
    }
    return victim_idx;
}

/// Updates an episode upon recall: refreshes `last_accessed_sec`, reinforces `strength`
/// via Ebbinghaus spaced-repetition gain, and saturating-increments `access_count`.
pub fn touchEpisode(ep: *EpisodeMeta, now_sec: i64) {
    if (now_sec > ep.last_accessed_sec) {
        ep.last_accessed_sec = now_sec;
    }
    ep.strength = reinforceStability(ep.strength, ep.access_count);
    if (ep.access_count < 0xFFFFFFFF) {
        ep.access_count += 1;
    }
}
