// Origin: Pure-CP Stdlib (Voyager/Reflexion Procedural Skill Memory Bank with @dot/@fma/@popCount) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation procedural skill memory bank for autonomous AI Agents:
//   - Fixed-capacity 64-slot skill registry with L2-normalized intent embeddings (dim <= 32)
//   - Tool-capability bitmask filtering ((skill.tool_mask & ~available_tools) == 0) & @popCount overlap
//   - Laplace-smoothed Bayesian reliability estimation & @dot/@fma composite Top-K retrieval
//   - Closed-loop execution feedback (success/failure/latency) & pinned-safe unreliable skill pruning

const std = @import("std");

/// Maximum fixed number of procedural skills stored in `SkillBank`.
pub const SKILL_BANK_MAX_SLOTS: usize = 64;

/// Maximum supported intent embedding dimension for zero-heap stack storage.
pub const SKILL_BANK_MAX_DIM: usize = 32;

/// Maximum basis points for reliability thresholds (`10000` = 100.00%).
pub const SKILL_BANK_MAX_BPS: u32 = 10000;

// ============================================================================
// Internal Numeric Helpers
// ============================================================================

fn isFiniteF32(x: f32): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clampF32(x: f32, lo: f32, hi: f32): f32 {
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

/// Validates and L2-normalizes `src[0..dim]` into `dst[0..dim]` using native SIMD `@dot`.
/// Returns `false` if `src.len < dim`, any element is NaN/Inf, or the vector has near-zero norm.
fn normalizeIntentVec(src: []const f32, dim: usize, dst: *[SKILL_BANK_MAX_DIM]f32): bool {
    if (dim == 0 || dim > SKILL_BANK_MAX_DIM || src.len < dim) {
        return false;
    }
    let i: usize = 0;
    while (i < dim) : (i += 1) {
        if (!isFiniteF32(src[i])) {
            return false;
        }
    }
    const norm_sq: f32 = @dot(src[0..dim], src[0..dim]);
    if (!isFiniteF32(norm_sq) || norm_sq <= 1.0e-12) {
        return false;
    }
    const inv_norm: f32 = (1.0 / std.math.sqrt(norm_sq as f64)) as f32;
    if (!isFiniteF32(inv_norm)) {
        return false;
    }
    let j: usize = 0;
    while (j < dim) : (j += 1) {
        dst[j] = src[j] * inv_norm;
    }
    while (j < SKILL_BANK_MAX_DIM) : (j += 1) {
        dst[j] = 0.0;
    }
    return true;
}

// ============================================================================
// Data Models (100% Zero-Heap Value Semantics)
// ============================================================================

/// Procedural skill entry with L2-normalized trigger intent embedding, required tool bitmask,
/// Bayesian reliability counters, running average latency, and pin protection.
pub struct SkillRecord {
    skill_id: u64 = 0;
    vec: [SKILL_BANK_MAX_DIM]f32 = [0.0; SKILL_BANK_MAX_DIM];
    tool_mask: u64 = 0;
    success_count: u32 = 0;
    failure_count: u32 = 0;
    avg_latency_ms: f32 = 0.0;
    last_used_ms: u64 = 0;
    pinned: bool = false;
    occupied: bool = false;

    static fn empty(): SkillRecord {
        return SkillRecord{
            skill_id: 0,
            vec: [0.0; SKILL_BANK_MAX_DIM],
            tool_mask: 0,
            success_count: 0,
            failure_count: 0,
            avg_latency_ms: 0.0,
            last_used_ms: 0,
            pinned: false,
            occupied: false,
        };
    }

    /// Returns total recorded execution attempts (`success_count + failure_count`).
    fn totalTrials(): u64 {
        return (_.success_count as u64) + (_.failure_count as u64);
    }

    /// Laplace-smoothed Bayesian reliability estimate: `(success + 1) / (success + failure + 2)`.
    /// New skills with 0 trials start at neutral `0.5`.
    fn reliability(): f32 {
        const num: f64 = (_.success_count as f64) + 1.0;
        const den: f64 = (_.success_count as f64) + (_.failure_count as f64) + 2.0;
        return (num / den) as f32;
    }

    /// Returns Laplace-smoothed Bayesian reliability scaled to basis points `[0, 10000]`.
    fn reliabilityBps(): u32 {
        const r: f64 = (((_.success_count as f64) + 1.0) / ((_.success_count as f64) + (_.failure_count as f64) + 2.0)) * 10000.0;
        if (r <= 0.0) return 0;
        if (r >= 10000.0) return SKILL_BANK_MAX_BPS;
        return (r + 0.5) as u32;
    }

    /// Returns `true` iff every required tool bit in `tool_mask` is present in `available_tools_mask`.
    fn isExecutableWith(available_tools_mask: u64): bool {
        return (_.tool_mask & (~available_tools_mask)) == 0;
    }
}

/// Scoring weights and threshold for `SkillBank.retrieveSkills`.
pub struct SkillRetrievalConfig {
    reliability_weight: f32 = 1.0;
    tool_overlap_weight: f32 = 0.15;
    min_cosine: f32 = -1.0;

    static fn defaults(): SkillRetrievalConfig {
        return SkillRetrievalConfig{
            reliability_weight: 1.0,
            tool_overlap_weight: 0.15,
            min_cosine: -1.0,
        };
    }

    fn isValid(): bool {
        if (!isFiniteF32(_.reliability_weight) || _.reliability_weight < 0.0) return false;
        if (!isFiniteF32(_.tool_overlap_weight) || _.tool_overlap_weight < 0.0) return false;
        if (!isFiniteF32(_.min_cosine) || _.min_cosine < -1.0 || _.min_cosine > 1.0) return false;
        return true;
    }
}

/// Ranked procedural skill recall result with full score breakdown.
pub struct SkillRecallHit {
    skill_id: u64 = 0;
    total_score: f32 = 0.0;
    cosine_sim: f32 = 0.0;
    reliability: f32 = 0.0;
    tool_overlap: f32 = 0.0;
    tool_mask: u64 = 0;
    avg_latency_ms: f32 = 0.0;
}

/// Telemetry summary for `SkillBank`.
pub struct SkillBankStats {
    active_skills: usize = 0;
    pinned_skills: usize = 0;
    capacity: usize = SKILL_BANK_MAX_SLOTS;
    dim: usize = 0;
    total_registered: u64 = 0;
    total_executions: u64 = 0;
    total_successes: u64 = 0;
    total_failures: u64 = 0;
    total_pruned: u64 = 0;
}

// ============================================================================
// Core Procedural Skill Memory Bank Engine
// ============================================================================

/// Zero-heap-allocation procedural skill memory bank for AI Agents (Voyager / Reflexion).
pub struct SkillBank {
    dim: usize = 0;
    slots: [SKILL_BANK_MAX_SLOTS]SkillRecord = [SkillRecord.empty(); SKILL_BANK_MAX_SLOTS];
    total_registered: u64 = 0;
    total_executions: u64 = 0;
    total_successes: u64 = 0;
    total_failures: u64 = 0;
    total_pruned: u64 = 0;

    /// Initializes a `SkillBank` for intent embeddings of dimension `dim` (`1 <= dim <= 32`).
    /// Returns `null` if `dim == 0` or `dim > SKILL_BANK_MAX_DIM`.
    static fn init(dim: usize): SkillBank? {
        if (dim == 0 || dim > SKILL_BANK_MAX_DIM) {
            return null;
        }
        return SkillBank{
            dim: dim,
            slots: [SkillRecord.empty(); SKILL_BANK_MAX_SLOTS],
            total_registered: 0,
            total_executions: 0,
            total_successes: 0,
            total_failures: 0,
            total_pruned: 0,
        };
    }

    /// Registers a new procedural skill with an L2-normalized trigger intent vector and required tool bitmask.
    /// Rejects `skill_id == 0`, duplicate `skill_id`, invalid/zero/NaN vector, or full bank.
    fn registerSkill(
        *_,
        skill_id: u64,
        intent_vec: []const f32,
        tool_mask: u64,
        pinned: bool,
        now_ms: u64,
    ): bool {
        if (skill_id == 0 || intent_vec.len != _.dim) {
            return false;
        }

        let norm_vec: [SKILL_BANK_MAX_DIM]f32 = [0.0; SKILL_BANK_MAX_DIM];
        if (!normalizeIntentVec(intent_vec, _.dim, *norm_vec)) {
            return false;
        }

        let free_idx: usize = SKILL_BANK_MAX_SLOTS;
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied) {
                if (_.slots[i].skill_id == skill_id) {
                    return false;
                }
            } else if (free_idx == SKILL_BANK_MAX_SLOTS) {
                free_idx = i;
            }
        }

        if (free_idx == SKILL_BANK_MAX_SLOTS) {
            return false;
        }

        _.slots[free_idx] = SkillRecord{
            skill_id: skill_id,
            vec: norm_vec,
            tool_mask: tool_mask,
            success_count: 0,
            failure_count: 0,
            avg_latency_ms: 0.0,
            last_used_ms: now_ms,
            pinned: pinned,
            occupied: true,
        };
        _.total_registered += 1;
        return true;
    }

    /// Retrieves Top-K executable skills for `query_vec` and `available_tools_mask`, writing
    /// results sorted by `(total_score desc, skill_id asc)` into `out_hits`.
    /// - Filters strictly to skills where `(skill.tool_mask & ~available_tools_mask) == 0`.
    /// - Scores via native SIMD `@dot`, Laplace Bayesian reliability, `@popCount` tool overlap, and `@fma`.
    /// - Returns `0` on invalid query vector, invalid config, or empty `out_hits`.
    fn retrieveSkills(
        query_vec: []const f32,
        available_tools_mask: u64,
        cfg: SkillRetrievalConfig,
        out_hits: *[]SkillRecallHit,
    ): usize {
        if (out_hits.len == 0 || !cfg.isValid() || query_vec.len != _.dim) {
            return 0;
        }

        let q_norm: [SKILL_BANK_MAX_DIM]f32 = [0.0; SKILL_BANK_MAX_DIM];
        if (!normalizeIntentVec(query_vec, _.dim, *q_norm)) {
            return 0;
        }

        const q_slice: []const f32 = q_norm[0.._.dim];
        const avail_bits: u32 = @popCount(available_tools_mask) as u32;

        let count: usize = 0;
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            const s = _.slots[i];
            if (!s.occupied) continue;
            if (!s.isExecutableWith(available_tools_mask)) continue;

            const raw_cos: f32 = @dot(q_slice, s.vec[0.._.dim]);
            if (!isFiniteF32(raw_cos)) continue;
            const cos_sim: f32 = clampF32(raw_cos, -1.0, 1.0);
            if (cos_sim < cfg.min_cosine) continue;

            const rel: f32 = s.reliability();
            let tool_overlap: f32 = 0.0;
            if (avail_bits > 0 && s.tool_mask != 0) {
                const matched_bits: u32 = @popCount(s.tool_mask & available_tools_mask) as u32;
                tool_overlap = (matched_bits as f32) / (avail_bits as f32);
            }

            // Positive similarity scales with Bayesian reliability; negative similarity stays negative
            const rel_term: f32 = if (cos_sim >= 0.0) (cos_sim * rel * cfg.reliability_weight) else cos_sim;
            const total_score: f32 = @fma(tool_overlap, cfg.tool_overlap_weight, rel_term);
            if (!isFiniteF32(total_score)) continue;

            const hit = SkillRecallHit{
                skill_id: s.skill_id,
                total_score: total_score,
                cosine_sim: cos_sim,
                reliability: rel,
                tool_overlap: tool_overlap,
                tool_mask: s.tool_mask,
                avg_latency_ms: s.avg_latency_ms,
            };

            // Insertion-maintain Top-K in out_hits sorted by (total_score desc, skill_id asc)
            let insert_pos: usize = count;
            if (count < out_hits.len) {
                count += 1;
            } else {
                const last = out_hits[out_hits.len - 1];
                if (hit.total_score < last.total_score ||
                    (hit.total_score == last.total_score && hit.skill_id >= last.skill_id))
                {
                    continue;
                }
                insert_pos = out_hits.len - 1;
            }

            while (insert_pos > 0) {
                const prev = out_hits[insert_pos - 1];
                if (hit.total_score > prev.total_score ||
                    (hit.total_score == prev.total_score && hit.skill_id < prev.skill_id))
                {
                    out_hits[insert_pos] = prev;
                    insert_pos -= 1;
                } else {
                    break;
                }
            }
            out_hits[insert_pos] = hit;
        }

        return count;
    }

    /// Records a closed-loop execution outcome for `skill_id`, updating reliability counters,
    /// running average latency (via `@fma`), and monotonic `last_used_ms`.
    /// Returns `false` if `skill_id == 0`, skill does not exist, or `latency_ms` is negative/NaN/Inf.
    fn recordExecution(*_, skill_id: u64, succeeded: bool, latency_ms: f32, now_ms: u64): bool {
        if (skill_id == 0 || !isFiniteF32(latency_ms) || latency_ms < 0.0) {
            return false;
        }
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].skill_id == skill_id) {
                if (succeeded) {
                    if (_.slots[i].success_count < std.math.maxInt(u32)) {
                        _.slots[i].success_count += 1;
                    }
                    _.total_successes += 1;
                } else {
                    if (_.slots[i].failure_count < std.math.maxInt(u32)) {
                        _.slots[i].failure_count += 1;
                    }
                    _.total_failures += 1;
                }
                const trials: u64 = _.slots[i].totalTrials();
                if (trials <= 1) {
                    _.slots[i].avg_latency_ms = latency_ms;
                } else {
                    const inv_n: f32 = (1.0 / (trials as f64)) as f32;
                    _.slots[i].avg_latency_ms = @fma(latency_ms - _.slots[i].avg_latency_ms, inv_n, _.slots[i].avg_latency_ms);
                }
                if (now_ms > _.slots[i].last_used_ms) {
                    _.slots[i].last_used_ms = now_ms;
                }
                _.total_executions += 1;
                return true;
            }
        }
        return false;
    }

    /// Prunes unreliable non-pinned skills that have accumulated at least `min_trials` executions
    /// and whose Bayesian reliability in basis points is strictly below `min_reliability_bps`.
    /// Always protects `pinned == true` core skills. Returns the number of pruned skills.
    fn pruneUnreliable(*_, min_trials: u32, min_reliability_bps: u32): usize {
        if (min_reliability_bps > SKILL_BANK_MAX_BPS) {
            return 0;
        }
        let pruned_now: usize = 0;
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            const s = _.slots[i];
            if (!s.occupied || s.pinned) continue;
            if (s.totalTrials() >= (min_trials as u64) && s.reliabilityBps() < min_reliability_bps) {
                _.slots[i] = SkillRecord.empty();
                pruned_now += 1;
                _.total_pruned += 1;
            }
        }
        return pruned_now;
    }

    /// Updates pin protection for an existing skill.
    fn setPinned(*_, skill_id: u64, pinned: bool): bool {
        if (skill_id == 0) return false;
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].skill_id == skill_id) {
                _.slots[i].pinned = pinned;
                return true;
            }
        }
        return false;
    }

    /// Explicitly removes a skill by `skill_id`.
    fn removeSkill(*_, skill_id: u64): bool {
        if (skill_id == 0) return false;
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].skill_id == skill_id) {
                _.slots[i] = SkillRecord.empty();
                return true;
            }
        }
        return false;
    }

    /// Returns a read-only snapshot of `skill_id`, or `null` if not found.
    fn getSkill(skill_id: u64): SkillRecord? {
        if (skill_id == 0) return null;
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied && _.slots[i].skill_id == skill_id) {
                return _.slots[i];
            }
        }
        return null;
    }

    /// Returns the number of active registered skills.
    fn len(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied) {
                count += 1;
            }
        }
        return count;
    }

    /// Clears all skill slots while retaining configured embedding dimension.
    fn clear(*_) {
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            _.slots[i] = SkillRecord.empty();
        }
    }

    /// Returns a telemetry summary of the skill bank.
    fn stats(): SkillBankStats {
        let active: usize = 0;
        let pinned_cnt: usize = 0;
        let i: usize = 0;
        while (i < SKILL_BANK_MAX_SLOTS) : (i += 1) {
            if (_.slots[i].occupied) {
                active += 1;
                if (_.slots[i].pinned) {
                    pinned_cnt += 1;
                }
            }
        }
        return SkillBankStats{
            active_skills: active,
            pinned_skills: pinned_cnt,
            capacity: SKILL_BANK_MAX_SLOTS,
            dim: _.dim,
            total_registered: _.total_registered,
            total_executions: _.total_executions,
            total_successes: _.total_successes,
            total_failures: _.total_failures,
            total_pruned: _.total_pruned,
        };
    }
}
