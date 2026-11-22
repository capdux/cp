// Origin: Pure-CP Stdlib (AI Agent Reflexion Self-Improvement & Convergence Controller) | Created: 2026-11-22
// Zero-heap-allocation Reflexion self-critique iteration controller with plateau early-stopping,
// repeated-failure human escalation, lesson deduplication, and best-attempt backtracking.

const std = @import("std");

/// Structured failure taxonomy for Reflexion critique and repeated-error detection.
pub enum FailureCategory {
    None,
    SyntaxError,
    RuntimeError,
    TestAssertion,
    ToolError,
    Timeout,
    FormatViolation,
    Hallucination,
}

/// Control decision returned after recording each Reflexion attempt.
pub enum ReflectionAction {
    ContinueRefine,
    AcceptBest,
    EscalateHuman,
    AbortNoProgress,
}

/// Fixed-slot history record for a single Reflexion iteration attempt.
pub struct AttemptRecord {
    attempt_no: u32 = 0;
    score: f32 = 0.0;
    failure: FailureCategory = FailureCategory.None;
    lesson_hash: u64 = 0;
    tokens_used: u32 = 0;
    accepted: bool = false;
}

/// Convergence and safety thresholds governing a Reflexion loop.
pub struct ReflexionConfig {
    target_score: f32 = 0.9;
    max_attempts: u32 = 5;
    plateau_patience: u32 = 2;
    min_delta: f32 = 0.01;
    repeat_failure_limit: u32 = 3;
}

/// Cumulative operational telemetry for a Reflexion controller session.
pub struct ReflexionStats {
    attempts: u32 = 0;
    best_attempt_no: u32 = 0;
    best_score: f32 = 0.0;
    consecutive_stalls: u32 = 0;
    repeated_same_failure: u32 = 0;
    total_tokens: u64 = 0;
}

fn isValidFiniteF32(v: f32): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn clampScore01(v: f32): f32 {
    if (!isValidFiniteF32(v) || v < 0.0) return 0.0;
    if (v > 1.0) return 1.0;
    return v;
}

/// Fixed-capacity, zero-heap-allocation Reflexion self-improvement & convergence controller.
pub struct ReflexionController {
    slots: []AttemptRecord;
    cfg: ReflexionConfig;
    count: usize = 0;
    attempts: u32 = 0;
    has_best: bool = false;
    best_slot: usize = 0;
    best_attempt_no: u32 = 0;
    best_score: f32 = 0.0;
    consecutive_stalls: u32 = 0;
    last_failure: FailureCategory = FailureCategory.None;
    repeated_same_failure: u32 = 0;
    total_tokens: u64 = 0;
    accepted: bool = false;

    /// Initializes a Reflexion controller backed by caller-owned `slots`.
    /// Returns `null` if `slots` is empty or any threshold in `cfg` is invalid.
    static fn init(
        slots: *[]AttemptRecord,
        cfg: ReflexionConfig,
    ): ReflexionController? {
        if (slots.len == 0) return null;
        if (!isValidFiniteF32(cfg.target_score) || cfg.target_score <= 0.0 || cfg.target_score > 1.0) {
            return null;
        }
        if (cfg.max_attempts == 0 || cfg.plateau_patience == 0 || cfg.repeat_failure_limit == 0) {
            return null;
        }
        if (!isValidFiniteF32(cfg.min_delta) || cfg.min_delta < 0.0) {
            return null;
        }

        let i: usize = 0;
        while (i < slots.len) : (i += 1) {
            slots[i] = AttemptRecord{};
        }

        return ReflexionController{
            slots: slots,
            cfg: cfg,
            count: 0,
            attempts: 0,
            has_best: false,
            best_slot: 0,
            best_attempt_no: 0,
            best_score: 0.0,
            consecutive_stalls: 0,
            last_failure: FailureCategory.None,
            repeated_same_failure: 0,
            total_tokens: 0,
            accepted: false,
        };
    }

    /// Records a single Reflexion attempt and returns the next control action:
    /// - `AcceptBest` when `score >= target_score` with `FailureCategory.None`;
    /// - `EscalateHuman` when the same non-None failure repeats `repeat_failure_limit` times
    ///   or attempt/slot capacity is exhausted;
    /// - `AbortNoProgress` when consecutive non-improving attempts reach `plateau_patience`;
    /// - `ContinueRefine` otherwise.
    fn recordAttempt(
        *_,
        score: f32,
        failure: FailureCategory,
        lesson_hash: u64,
        tokens_used: u32,
    ): ReflectionAction {
        if (_.accepted) {
            return ReflectionAction.AcceptBest;
        }
        if (_.count >= _.slots.len || _.attempts >= _.cfg.max_attempts) {
            return ReflectionAction.EscalateHuman;
        }

        const safe_score = clampScore01(score);
        _.attempts += 1;
        _.total_tokens += u64(tokens_used);

        // Update best score tracking and plateau stall counter.
        if (!_.has_best) {
            _.has_best = true;
            _.best_score = safe_score;
            _.best_attempt_no = _.attempts;
            _.best_slot = _.count;
            _.consecutive_stalls = 0;
        } else if (safe_score >= _.best_score + _.cfg.min_delta) {
            _.best_score = safe_score;
            _.best_attempt_no = _.attempts;
            _.best_slot = _.count;
            _.consecutive_stalls = 0;
        } else {
            _.consecutive_stalls += 1;
            if (safe_score > _.best_score) {
                _.best_score = safe_score;
                _.best_attempt_no = _.attempts;
                _.best_slot = _.count;
            }
        }

        // Track consecutive repeated failures of the same non-None category.
        if (failure == FailureCategory.None) {
            _.repeated_same_failure = 0;
            _.last_failure = FailureCategory.None;
        } else {
            if (_.attempts > 1 && failure == _.last_failure) {
                _.repeated_same_failure += 1;
            } else {
                _.repeated_same_failure = 1;
            }
            _.last_failure = failure;
        }

        // Evaluate convergence and safety gates in priority order.
        let action = ReflectionAction.ContinueRefine;
        let is_accepted = false;

        if (safe_score >= _.cfg.target_score && failure == FailureCategory.None) {
            action = ReflectionAction.AcceptBest;
            is_accepted = true;
            _.accepted = true;
        } else if (failure != FailureCategory.None && _.repeated_same_failure >= _.cfg.repeat_failure_limit) {
            action = ReflectionAction.EscalateHuman;
        } else if (_.consecutive_stalls >= _.cfg.plateau_patience) {
            action = ReflectionAction.AbortNoProgress;
        } else if (_.attempts >= _.cfg.max_attempts || (_.count + 1) >= _.slots.len) {
            action = ReflectionAction.EscalateHuman;
        }

        _.slots[_.count] = AttemptRecord{
            attempt_no: _.attempts,
            score: safe_score,
            failure: failure,
            lesson_hash: lesson_hash,
            tokens_used: tokens_used,
            accepted: is_accepted,
        };
        _.count += 1;

        return action;
    }

    /// Returns the highest-scoring attempt recorded so far, or `null` if no attempts exist.
    fn bestAttempt(): AttemptRecord? {
        if (!_.has_best || _.best_slot >= _.count) return null;
        return _.slots[_.best_slot];
    }

    /// Returns `true` if a non-zero `lesson_hash` was already recorded in prior attempts.
    fn hasSeenLesson(lesson_hash: u64): bool {
        if (lesson_hash == 0) return false;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.slots[i].lesson_hash == lesson_hash) {
                return true;
            }
        }
        return false;
    }

    /// Returns a read-only telemetry snapshot of the current Reflexion loop.
    fn stats(): ReflexionStats {
        return ReflexionStats{
            attempts: _.attempts,
            best_attempt_no: _.best_attempt_no,
            best_score: _.best_score,
            consecutive_stalls: _.consecutive_stalls,
            repeated_same_failure: _.repeated_same_failure,
            total_tokens: _.total_tokens,
        };
    }

    /// Resets all attempt history and counters while preserving the controller configuration.
    fn reset(*_) {
        let i: usize = 0;
        while (i < _.slots.len) : (i += 1) {
            _.slots[i] = AttemptRecord{};
        }
        _.count = 0;
        _.attempts = 0;
        _.has_best = false;
        _.best_slot = 0;
        _.best_attempt_no = 0;
        _.best_score = 0.0;
        _.consecutive_stalls = 0;
        _.last_failure = FailureCategory.None;
        _.repeated_same_failure = 0;
        _.total_tokens = 0;
        _.accepted = false;
    }
}
