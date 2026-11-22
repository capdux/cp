// Origin: Pure-CP Stdlib (Zero-Alloc Agent Reflexion Verbal Reinforcement Lesson Bank) | Created: 2026-11-23
// Pure CP zero-heap-allocation Reflexion self-reflection & verbal reinforcement lesson bank:
//   - Fixed 64-slot lesson bank storing domain bitmask, error kind, action hash, short verbal lesson,
//     Bayesian posterior efficiency `(helped + 1) / (helped + failed + 2)`, timestamps & lifecycle status
//   - Multi-signal Top-K lesson recall combining `@popCount` domain overlap, exact error-kind & action-hash
//     matches, Bayesian efficiency, and half-life recency decay
//   - Closed-loop reinforcement feedback with automatic low-efficiency deprecation and smart capacity eviction
//   - Zero-alloc LLM Prompt injection block formatter & operational telemetry snapshot

const std = @import("std");

// ============================================================================
// 1. Constants, Enums & Data Models
// ============================================================================

/// Maximum number of verbal reinforcement lessons stored inline (zero dynamic heap).
pub const REFLEXION_MAX_LESSONS: usize = 64;

/// Maximum byte length of a single verbal reflection lesson text.
pub const REFLEXION_TEXT_MAX_LEN: usize = 160;

/// Sentinel invalid lesson ID.
pub const REFLEXION_NO_ID: u32 = 0;

/// Basis-points scale (10000 bps == 100.00%).
pub const REFLEXION_BPS_SCALE: u32 = 10000;

/// Failure category associated with a Reflexion lesson.
pub enum ReflexionErrorKind {
    tool_failure,
    schema_mismatch,
    timeout,
    permission_denied,
    hallucination,
    logic_error,
    other,
}

/// Lifecycle state of a stored lesson.
pub enum ReflexionStatus {
    active,
    deprecated,
    retired,
}

/// Tuning parameters for multi-signal recall scoring and automatic deprecation.
pub struct ReflexionConfig {
    half_life_ms: u64 = 86_400_000,
    deprecate_min_trials: u32 = 3,
    deprecate_threshold_bps: u32 = 3500,
    domain_weight: f32 = 0.30,
    error_match_weight: f32 = 0.25,
    action_match_weight: f32 = 0.20,
    posterior_weight: f32 = 0.15,
    recency_weight: f32 = 0.10,

    pub fn defaults(): ReflexionConfig {
        return ReflexionConfig{};
    }

    pub fn isValid(): bool {
        if (half_life_ms == 0 || deprecate_min_trials == 0 || deprecate_threshold_bps > REFLEXION_BPS_SCALE) {
            return false;
        }
        if (domain_weight < 0.0 || error_match_weight < 0.0 || action_match_weight < 0.0 ||
            posterior_weight < 0.0 || recency_weight < 0.0)
        {
            return false;
        }
        const total = domain_weight + error_match_weight + action_match_weight + posterior_weight + recency_weight;
        return !std.math.isNan(total) && !std.math.isInf(total) && total > 0.0;
    }
}

/// Single inline verbal reinforcement lesson record.
pub struct ReflexionLesson {
    occupied: bool = false;
    lesson_id: u32 = 0;
    domain_mask: u64 = 0;
    error_kind: ReflexionErrorKind = ReflexionErrorKind.other;
    action_hash: u64 = 0;
    text_buf: [REFLEXION_TEXT_MAX_LEN]u8 = [0; REFLEXION_TEXT_MAX_LEN];
    text_len: usize = 0;
    helped_count: u32 = 0;
    failed_count: u32 = 0;
    consecutive_failures: u32 = 0;
    created_ms: u64 = 0;
    last_used_ms: u64 = 0;
    status: ReflexionStatus = ReflexionStatus.active;

    pub fn textSlice(): []const u8 {
        return text_buf[0..text_len];
    }

    pub fn totalTrials(): u32 {
        return helped_count + failed_count;
    }

    /// Laplace-smoothed Bayesian posterior effectiveness `(helped + 1) / (helped + failed + 2)`.
    pub fn posteriorEfficiency(): f32 {
        const num: f32 = (helped_count + 1) as f32;
        const den: f32 = (helped_count + failed_count + 2) as f32;
        return num / den;
    }

    /// Bayesian posterior effectiveness expressed in basis points (`0..=10000`).
    pub fn posteriorBps(): u32 {
        const num: u64 = ((helped_count as u64) + 1) * (REFLEXION_BPS_SCALE as u64);
        const den: u64 = (helped_count as u64) + (failed_count as u64) + 2;
        const bps: u64 = num / den;
        if (bps > (REFLEXION_BPS_SCALE as u64)) return REFLEXION_BPS_SCALE;
        return bps as u32;
    }
}

/// Ranked recall hit returned by `ReflexionBank.recallLessons`.
pub struct ReflexionRecallHit {
    lesson_id: u32 = 0;
    slot_index: usize = 0;
    score: f32 = 0.0;
    posterior_bps: u32 = 0;
    domain_overlap_bits: u32 = 0;
    exact_error_match: bool = false;
    exact_action_match: bool = false;
    text_buf: [REFLEXION_TEXT_MAX_LEN]u8 = [0; REFLEXION_TEXT_MAX_LEN];
    text_len: usize = 0;

    pub fn textSlice(): []const u8 {
        return text_buf[0..text_len];
    }
}

/// Operational telemetry snapshot for `ReflexionBank`.
pub struct ReflexionBankStats {
    total_slots_used: usize = 0;
    capacity: usize = REFLEXION_MAX_LESSONS;
    active_count: usize = 0;
    deprecated_count: usize = 0;
    retired_count: usize = 0;
    total_recorded: u64 = 0;
    total_recalls: u64 = 0;
    total_evictions: u64 = 0;
    total_auto_deprecations: u64 = 0;
    total_helped_feedback: u64 = 0;
    total_failed_feedback: u64 = 0;
}

// ============================================================================
// 2. Internal Helpers
// ============================================================================

fn isValidLessonText(s: []const u8): bool {
    if (s.len == 0 || s.len > REFLEXION_TEXT_MAX_LEN) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == 0x00) return false;
    }
    return true;
}

fn computeRecencyDecay(last_used_ms: u64, now_ms: u64, half_life_ms: u64): f32 {
    if (now_ms <= last_used_ms || half_life_ms == 0) return 1.0;
    const elapsed_ms: u64 = now_ms - last_used_ms;
    const ratio: f64 = (elapsed_ms as f64) / (half_life_ms as f64);
    const decay: f64 = std.math.exp(-0.6931471805599453 * ratio);
    if (decay < 0.0) return 0.0;
    if (decay > 1.0) return 1.0;
    return decay as f32;
}

fn appendRaw(out: *[]u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[pos.* + i] = s[i];
    }
    pos.* = pos.* + s.len;
    return true;
}

fn appendU64(out: *[]u8, pos: *usize, val_in: u64): bool {
    if (val_in == 0) return appendRaw(out, pos, "0");
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v: u64 = val_in;
    while (v > 0) {
        tmp[n] = (0x30 as u8) + ((v % 10) as u8);
        v = v / 10;
        n += 1;
    }
    if (pos.* + n > out.len) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* = pos.* + n;
    return true;
}

fn statusEvictPriority(st: ReflexionStatus): u8 {
    // Higher number => evict earlier (retired -> deprecated -> active)
    if (st == ReflexionStatus.retired) return 2;
    if (st == ReflexionStatus.deprecated) return 1;
    return 0;
}

// ============================================================================
// 3. Core Engine (`ReflexionBank`)
// ============================================================================

/// Zero-heap verbal reinforcement lesson bank for self-reflecting AI Agents.
pub struct ReflexionBank {
    slots: [REFLEXION_MAX_LESSONS]ReflexionLesson = [ReflexionLesson{}; REFLEXION_MAX_LESSONS];
    cfg: ReflexionConfig = ReflexionConfig{},
    next_lesson_id: u32 = 1,
    total_recorded: u64 = 0,
    total_recalls: u64 = 0,
    total_evictions: u64 = 0,
    total_auto_deprecations: u64 = 0,
    total_helped_feedback: u64 = 0,
    total_failed_feedback: u64 = 0;

    /// Initializes a `ReflexionBank` with default scoring and deprecation thresholds.
    pub fn init(): ReflexionBank {
        return ReflexionBank{};
    }

    /// Initializes a `ReflexionBank` with custom `config`, or `null` if `config` is invalid.
    pub fn initWithConfig(config: ReflexionConfig): ReflexionBank? {
        if (!config.isValid()) return null;
        return ReflexionBank{
            cfg: config,
        };
    }

    fn findSlotById(lesson_id: u32): usize? {
        if (lesson_id == REFLEXION_NO_ID) return null;
        let i: usize = 0;
        while (i < REFLEXION_MAX_LESSONS) : (i += 1) {
            if (slots[i].occupied && slots[i].lesson_id == lesson_id) return i;
        }
        return null;
    }

    fn findFreeOrEvictSlot(*_): usize {
        // 1. Prefer any unoccupied slot
        let i: usize = 0;
        while (i < REFLEXION_MAX_LESSONS) : (i += 1) {
            if (!slots[i].occupied) return i;
        }

        // 2. Evict lowest-value lesson: retired > deprecated > active; then lowest posteriorBps;
        //    then oldest last_used_ms; then lowest lesson_id.
        let victim: usize = 0;
        let j: usize = 1;
        while (j < REFLEXION_MAX_LESSONS) : (j += 1) {
            const cur = slots[j];
            const best = slots[victim];

            const cur_prio = statusEvictPriority(cur.status);
            const best_prio = statusEvictPriority(best.status);
            if (cur_prio > best_prio) {
                victim = j;
                continue;
            }
            if (cur_prio < best_prio) continue;

            const cur_bps = cur.posteriorBps();
            const best_bps = best.posteriorBps();
            if (cur_bps < best_bps) {
                victim = j;
                continue;
            }
            if (cur_bps > best_bps) continue;

            if (cur.last_used_ms < best.last_used_ms ||
                (cur.last_used_ms == best.last_used_ms && cur.lesson_id < best.lesson_id))
            {
                victim = j;
            }
        }

        total_evictions += 1;
        return victim;
    }

    /// Records a new self-reflection lesson. When all 64 slots are occupied, automatically evicts
    /// the most obsolete or lowest-efficiency lesson. Returns the assigned `lesson_id`, or `null`
    /// if `text` is empty, contains NUL, or exceeds `REFLEXION_TEXT_MAX_LEN`.
    pub fn recordLesson(
        *_,
        domain_mask: u64,
        error_kind: ReflexionErrorKind,
        action_hash: u64,
        text: []const u8,
        now_ms: u64,
    ): u32? {
        if (!isValidLessonText(text)) return null;

        const slot = findFreeOrEvictSlot();
        const assigned_id = next_lesson_id;
        next_lesson_id += 1;
        if (next_lesson_id == 0) next_lesson_id = 1;

        let entry = ReflexionLesson{};
        entry.occupied = true;
        entry.lesson_id = assigned_id;
        entry.domain_mask = domain_mask;
        entry.error_kind = error_kind;
        entry.action_hash = action_hash;
        entry.text_len = text.len;
        let k: usize = 0;
        while (k < text.len) : (k += 1) {
            entry.text_buf[k] = text[k];
        }
        entry.created_ms = now_ms;
        entry.last_used_ms = now_ms;
        entry.status = ReflexionStatus.active;

        slots[slot] = entry;
        total_recorded += 1;
        return assigned_id;
    }

    /// Retrieves up to `top_k` most relevant `active` lessons using multi-signal scoring:
    /// `@popCount` domain Jaccard overlap + exact `error_kind` match + exact `action_hash` match +
    /// Bayesian posterior efficiency + half-life recency decay. Updates `last_used_ms` on returned hits.
    pub fn recallLessons(
        *_,
        domain_mask: u64,
        error_kind: ReflexionErrorKind,
        action_hash: u64,
        now_ms: u64,
        top_k: usize,
        out_hits: *[]ReflexionRecallHit,
    ): usize {
        total_recalls += 1;
        if (top_k == 0 || out_hits.len == 0) return 0;

        let candidates: [REFLEXION_MAX_LESSONS]ReflexionRecallHit = [ReflexionRecallHit{}; REFLEXION_MAX_LESSONS];
        let cand_count: usize = 0;

        let i: usize = 0;
        while (i < REFLEXION_MAX_LESSONS) : (i += 1) {
            if (!slots[i].occupied || slots[i].status != ReflexionStatus.active) continue;
            const entry = slots[i];

            const overlap_bits: u32 = @popCount(domain_mask & entry.domain_mask) as u32;
            const union_bits: u32 = @popCount(domain_mask | entry.domain_mask) as u32;
            let domain_sim: f32 = 0.0;
            if (union_bits > 0) {
                domain_sim = (overlap_bits as f32) / (union_bits as f32);
            } else {
                domain_sim = 0.5;
            }

            const err_match = entry.error_kind == error_kind;
            const act_match = action_hash != 0 && entry.action_hash == action_hash;
            const post_eff = entry.posteriorEfficiency();
            const recency = computeRecencyDecay(entry.last_used_ms, now_ms, cfg.half_life_ms);

            const score: f32 = cfg.domain_weight * domain_sim +
                cfg.error_match_weight * (if (err_match) 1.0 else 0.0) +
                cfg.action_match_weight * (if (act_match) 1.0 else 0.0) +
                cfg.posterior_weight * post_eff +
                cfg.recency_weight * recency;

            let hit = ReflexionRecallHit{
                lesson_id: entry.lesson_id,
                slot_index: i,
                score: score,
                posterior_bps: entry.posteriorBps(),
                domain_overlap_bits: overlap_bits,
                exact_error_match: err_match,
                exact_action_match: act_match,
                text_len: entry.text_len,
            };
            let t: usize = 0;
            while (t < entry.text_len) : (t += 1) {
                hit.text_buf[t] = entry.text_buf[t];
            }

            candidates[cand_count] = hit;
            cand_count += 1;
        }

        // Insertion sort descending by (score, posterior_bps), ascending by lesson_id
        let j: usize = 1;
        while (j < cand_count) : (j += 1) {
            const key = candidates[j];
            let k: usize = j;
            while (k > 0) {
                const prev = candidates[k - 1];
                const swap_needed = (key.score > prev.score) ||
                    (key.score == prev.score && key.posterior_bps > prev.posterior_bps) ||
                    (key.score == prev.score && key.posterior_bps == prev.posterior_bps && key.lesson_id < prev.lesson_id);
                if (!swap_needed) break;
                candidates[k] = prev;
                k -= 1;
            }
            candidates[k] = key;
        }

        let limit: usize = cand_count;
        if (top_k < limit) limit = top_k;
        if (out_hits.len < limit) limit = out_hits.len;

        let w: usize = 0;
        while (w < limit) : (w += 1) {
            out_hits[w] = candidates[w];
            const s_idx = candidates[w].slot_index;
            if (now_ms > slots[s_idx].last_used_ms) {
                slots[s_idx].last_used_ms = now_ms;
            }
        }
        return limit;
    }

    /// Records closed-loop reinforcement feedback (`helped == true` or `false`) for `lesson_id`.
    /// Automatically deprecates lessons whose posterior effectiveness drops below `deprecate_threshold_bps`
    /// after `deprecate_min_trials` (or after `deprecate_min_trials` consecutive failures).
    /// Returns the resulting `ReflexionStatus`, or `null` if `lesson_id` is not found.
    pub fn recordFeedback(*_, lesson_id: u32, helped: bool, now_ms: u64): ReflexionStatus? {
        const slot = findSlotById(lesson_id) ?? return null;
        if (slots[slot].status == ReflexionStatus.retired) {
            return ReflexionStatus.retired;
        }

        if (now_ms > slots[slot].last_used_ms) {
            slots[slot].last_used_ms = now_ms;
        }

        if (helped) {
            slots[slot].helped_count += 1;
            slots[slot].consecutive_failures = 0;
            total_helped_feedback += 1;
            if (slots[slot].status == ReflexionStatus.deprecated &&
                slots[slot].posteriorBps() >= cfg.deprecate_threshold_bps)
            {
                slots[slot].status = ReflexionStatus.active;
            }
        } else {
            slots[slot].failed_count += 1;
            slots[slot].consecutive_failures += 1;
            total_failed_feedback += 1;

            if (slots[slot].status == ReflexionStatus.active) {
                const low_eff = slots[slot].totalTrials() >= cfg.deprecate_min_trials &&
                    slots[slot].posteriorBps() < cfg.deprecate_threshold_bps;
                const streak_fail = slots[slot].consecutive_failures >= cfg.deprecate_min_trials;
                if (low_eff || streak_fail) {
                    slots[slot].status = ReflexionStatus.deprecated;
                    total_auto_deprecations += 1;
                }
            }
        }

        return slots[slot].status;
    }

    /// Permanently retires `lesson_id` so it is excluded from recall and prioritized for eviction.
    pub fn retireLesson(*_, lesson_id: u32): bool {
        const slot = findSlotById(lesson_id) ?? return false;
        slots[slot].status = ReflexionStatus.retired;
        return true;
    }

    /// Returns a read-only copy of `lesson_id`, or `null` if absent.
    pub fn getLesson(lesson_id: u32): ReflexionLesson? {
        const slot = findSlotById(lesson_id) ?? return null;
        return slots[slot];
    }

    /// Returns a point-in-time operational telemetry snapshot.
    pub fn stats(): ReflexionBankStats {
        let used: usize = 0;
        let act: usize = 0;
        let dep: usize = 0;
        let ret: usize = 0;
        let i: usize = 0;
        while (i < REFLEXION_MAX_LESSONS) : (i += 1) {
            if (!slots[i].occupied) continue;
            used += 1;
            if (slots[i].status == ReflexionStatus.active) {
                act += 1;
            } else if (slots[i].status == ReflexionStatus.deprecated) {
                dep += 1;
            } else {
                ret += 1;
            }
        }
        return ReflexionBankStats{
            total_slots_used: used,
            capacity: REFLEXION_MAX_LESSONS,
            active_count: act,
            deprecated_count: dep,
            retired_count: ret,
            total_recorded: total_recorded,
            total_recalls: total_recalls,
            total_evictions: total_evictions,
            total_auto_deprecations: total_auto_deprecations,
            total_helped_feedback: total_helped_feedback,
            total_failed_feedback: total_failed_feedback,
        };
    }
}

// ============================================================================
// 4. Zero-Alloc Prompt Injection Formatter
// ============================================================================

/// Formats recalled Reflexion lessons into a compact, LLM-ready prompt injection block:
///   `[Reflexion Lessons]\n1. (#1, eff=75%) <lesson text>\n...`
/// Returns `0` for an empty slice, total bytes written on success, or `null` if `out_buf` is too small.
pub fn formatLessonsPromptBlock(hits: []const ReflexionRecallHit, out_buf: *[]u8): usize? {
    if (hits.len == 0) return 0;

    let pos: usize = 0;
    if (!appendRaw(out_buf, *pos, "[Reflexion Lessons]\n")) return null;

    let i: usize = 0;
    while (i < hits.len) : (i += 1) {
        const h = hits[i];
        const pct: u64 = (h.posterior_bps / 100) as u64;
        if (!appendU64(out_buf, *pos, (i + 1) as u64)) return null;
        if (!appendRaw(out_buf, *pos, ". (#")) return null;
        if (!appendU64(out_buf, *pos, h.lesson_id as u64)) return null;
        if (!appendRaw(out_buf, *pos, ", eff=")) return null;
        if (!appendU64(out_buf, *pos, pct)) return null;
        if (!appendRaw(out_buf, *pos, "%) ")) return null;
        if (!appendRaw(out_buf, *pos, h.textSlice())) return null;
        if (!appendRaw(out_buf, *pos, "\n")) return null;
    }
    return pos;
}
