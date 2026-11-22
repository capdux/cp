// Origin: Pure-CP Stdlib Test Suite (AI Agent Reflexion Self-Improvement & Convergence Controller) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/reflection.cp
// Covers:
//   1. Progressive score improvement converging to AcceptBest & post-acceptance idempotence
//   2. Plateau early-stopping (consecutive stalls >= plateau_patience -> AbortNoProgress)
//   3. Repeated identical FailureCategory escalation (EscalateHuman) & streak reset on category change
//   4. Lesson hash deduplication (hasSeenLesson), bestAttempt backtracking on regression & reset() reuse
//   5. Invalid ReflexionConfig / empty slot guards, NaN/out-of-range score clamping & full-slot exhaustion

import { test, expect_eq } from "cap:test";
import {
    FailureCategory,
    ReflectionAction,
    AttemptRecord,
    ReflexionConfig,
    ReflexionStats,
    ReflexionController,
} from "../../lib/agent/index.cp";

fn approxEq(a: f32, b: f32, eps: f32): bool {
    const d = if (a > b) a - b else b - a;
    return d <= eps;
}

fn testProgressiveImprovementToAcceptBest()! {
    let slots_buf: [6]AttemptRecord = [AttemptRecord{}; 6];
    let slots: []AttemptRecord = slots_buf[0..];
    const cfg = ReflexionConfig{
        target_score: 0.90,
        max_attempts: 6,
        plateau_patience: 2,
        min_delta: 0.05,
        repeat_failure_limit: 3,
    };
    let ctrl = ReflexionController.init(slots, cfg) ?? return;

    const a1 = ctrl.recordAttempt(0.40, FailureCategory.SyntaxError, 0xAA01, 400);
    expect_eq(a1 == ReflectionAction.ContinueRefine, true);

    const a2 = ctrl.recordAttempt(0.72, FailureCategory.TestAssertion, 0xAA02, 550);
    expect_eq(a2 == ReflectionAction.ContinueRefine, true);

    const a3 = ctrl.recordAttempt(0.95, FailureCategory.None, 0xAA03, 500);
    expect_eq(a3 == ReflectionAction.AcceptBest, true);

    // Subsequent calls after acceptance remain AcceptBest without mutating history
    const a4 = ctrl.recordAttempt(0.10, FailureCategory.RuntimeError, 0xAA04, 100);
    expect_eq(a4 == ReflectionAction.AcceptBest, true);

    const st: ReflexionStats = ctrl.stats();
    expect_eq(st.attempts, 3);
    expect_eq(st.best_attempt_no, 3);
    expect_eq(approxEq(st.best_score, 0.95, 0.0001), true);
    expect_eq(st.consecutive_stalls, 0);
    expect_eq(st.total_tokens, 1450);

    const best = ctrl.bestAttempt() ?? AttemptRecord{};
    expect_eq(best.attempt_no, 3);
    expect_eq(best.accepted, true);
    expect_eq(best.lesson_hash, 0xAA03);
}

fn testPlateauEarlyStoppingAbortNoProgress()! {
    let slots_buf: [6]AttemptRecord = [AttemptRecord{}; 6];
    let slots: []AttemptRecord = slots_buf[0..];
    const cfg = ReflexionConfig{
        target_score: 0.95,
        max_attempts: 6,
        plateau_patience: 2,
        min_delta: 0.05,
        repeat_failure_limit: 4,
    };
    let ctrl = ReflexionController.init(slots, cfg) ?? return;

    // Attempt 1 sets baseline 0.60 (stalls=0)
    expect_eq(ctrl.recordAttempt(0.60, FailureCategory.TestAssertion, 0xB1, 300) == ReflectionAction.ContinueRefine, true);
    // Attempt 2 improves by only 0.02 (< min_delta=0.05) -> stall 1
    expect_eq(ctrl.recordAttempt(0.62, FailureCategory.FormatViolation, 0xB2, 300) == ReflectionAction.ContinueRefine, true);
    expect_eq(ctrl.stats().consecutive_stalls, 1);
    // Attempt 3 regresses to 0.58 -> stall 2 == plateau_patience -> AbortNoProgress
    expect_eq(ctrl.recordAttempt(0.58, FailureCategory.RuntimeError, 0xB3, 300) == ReflectionAction.AbortNoProgress, true);

    const st = ctrl.stats();
    expect_eq(st.attempts, 3);
    expect_eq(st.consecutive_stalls, 2);
    // Best attempt must still reflect attempt 2 (0.62), not the regressed attempt 3 (0.58)
    expect_eq(st.best_attempt_no, 2);
    expect_eq(approxEq(st.best_score, 0.62, 0.0001), true);
}

fn testRepeatedFailureEscalatesHuman()! {
    let slots_buf: [6]AttemptRecord = [AttemptRecord{}; 6];
    let slots: []AttemptRecord = slots_buf[0..];
    const cfg = ReflexionConfig{
        target_score: 0.95,
        max_attempts: 6,
        plateau_patience: 5,
        min_delta: 0.01,
        repeat_failure_limit: 3,
    };
    let ctrl = ReflexionController.init(slots, cfg) ?? return;

    // Different failure resets streak
    expect_eq(ctrl.recordAttempt(0.20, FailureCategory.Timeout, 0xC1, 100) == ReflectionAction.ContinueRefine, true);
    expect_eq(ctrl.stats().repeated_same_failure, 1);

    // Switch to ToolError -> streak resets to 1, then 2, then 3 -> EscalateHuman
    expect_eq(ctrl.recordAttempt(0.30, FailureCategory.ToolError, 0xC2, 100) == ReflectionAction.ContinueRefine, true);
    expect_eq(ctrl.stats().repeated_same_failure, 1);
    expect_eq(ctrl.recordAttempt(0.40, FailureCategory.ToolError, 0xC3, 100) == ReflectionAction.ContinueRefine, true);
    expect_eq(ctrl.stats().repeated_same_failure, 2);
    expect_eq(ctrl.recordAttempt(0.50, FailureCategory.ToolError, 0xC4, 100) == ReflectionAction.EscalateHuman, true);
    expect_eq(ctrl.stats().repeated_same_failure, 3);
}

fn testLessonDeduplicationBacktrackingAndReset()! {
    let slots_buf: [5]AttemptRecord = [AttemptRecord{}; 5];
    let slots: []AttemptRecord = slots_buf[0..];
    const cfg = ReflexionConfig{
        target_score: 0.95,
        max_attempts: 5,
        plateau_patience: 4,
        min_delta: 0.05,
        repeat_failure_limit: 4,
    };
    let ctrl = ReflexionController.init(slots, cfg) ?? return;

    expect_eq(ctrl.bestAttempt() == null, true);
    expect_eq(ctrl.hasSeenLesson(0xDEAD), false);
    expect_eq(ctrl.hasSeenLesson(0), false);

    * = ctrl.recordAttempt(0.50, FailureCategory.SyntaxError, 0x1111, 200);
    * = ctrl.recordAttempt(0.82, FailureCategory.Hallucination, 0x2222, 250);
    * = ctrl.recordAttempt(0.40, FailureCategory.TestAssertion, 0x3333, 220);

    expect_eq(ctrl.hasSeenLesson(0x1111), true);
    expect_eq(ctrl.hasSeenLesson(0x2222), true);
    expect_eq(ctrl.hasSeenLesson(0x9999), false);
    expect_eq(ctrl.hasSeenLesson(0), false);

    // Backtrack to attempt 2 (highest score 0.82)
    const best = ctrl.bestAttempt() ?? AttemptRecord{};
    expect_eq(best.attempt_no, 2);
    expect_eq(approxEq(best.score, 0.82, 0.0001), true);
    expect_eq(best.failure == FailureCategory.Hallucination, true);

    // Reset clears all history & stats while preserving config
    ctrl.reset();
    expect_eq(ctrl.bestAttempt() == null, true);
    expect_eq(ctrl.hasSeenLesson(0x1111), false);
    expect_eq(ctrl.stats().attempts, 0);
    expect_eq(ctrl.stats().total_tokens, 0);

    // Reuse after reset
    expect_eq(ctrl.recordAttempt(0.96, FailureCategory.None, 0x4444, 150) == ReflectionAction.AcceptBest, true);
    expect_eq(ctrl.stats().attempts, 1);
    expect_eq(ctrl.stats().total_tokens, 150);
}

fn testInvalidConfigFullSlotAndBoundsGuards()! {
    let slots_buf: [2]AttemptRecord = [AttemptRecord{}; 2];
    let slots: []AttemptRecord = slots_buf[0..];
    let empty_buf: [0]AttemptRecord = [];
    let empty_slots: []AttemptRecord = empty_buf[0..];

    // 1. Invalid configs & empty slots rejected by init()
    expect_eq(ReflexionController.init(empty_slots, ReflexionConfig{}) == null, true);
    expect_eq(ReflexionController.init(slots, ReflexionConfig{ target_score: 0.0 }) == null, true);
    expect_eq(ReflexionController.init(slots, ReflexionConfig{ target_score: 1.2 }) == null, true);
    expect_eq(ReflexionController.init(slots, ReflexionConfig{ max_attempts: 0 }) == null, true);
    expect_eq(ReflexionController.init(slots, ReflexionConfig{ plateau_patience: 0 }) == null, true);
    expect_eq(ReflexionController.init(slots, ReflexionConfig{ repeat_failure_limit: 0 }) == null, true);
    expect_eq(ReflexionController.init(slots, ReflexionConfig{ min_delta: -0.1 }) == null, true);

    // 2. Slot exhaustion triggers EscalateHuman even when max_attempts > slots.len
    const cfg = ReflexionConfig{
        target_score: 0.90,
        max_attempts: 10,
        plateau_patience: 5,
        min_delta: 0.05,
        repeat_failure_limit: 5,
    };
    let ctrl = ReflexionController.init(slots, cfg) ?? return;

    // Negative score clamped to 0.0
    expect_eq(ctrl.recordAttempt(-0.5, FailureCategory.SyntaxError, 0x1, 100) == ReflectionAction.ContinueRefine, true);
    expect_eq(approxEq(ctrl.stats().best_score, 0.0, 0.0001), true);

    // Second slot fills buffer -> immediately escalates to human
    expect_eq(ctrl.recordAttempt(0.5, FailureCategory.RuntimeError, 0x2, 100) == ReflectionAction.EscalateHuman, true);
    // Calling again on full buffer safely returns EscalateHuman without OOB write
    expect_eq(ctrl.recordAttempt(0.99, FailureCategory.None, 0x3, 100) == ReflectionAction.EscalateHuman, true);
    expect_eq(ctrl.stats().attempts, 2);
}

test("agent reflection: progressive improvement to AcceptBest and post-accept idempotence", testProgressiveImprovementToAcceptBest);
test("agent reflection: plateau early-stopping triggers AbortNoProgress", testPlateauEarlyStoppingAbortNoProgress);
test("agent reflection: repeated identical failure escalates to human", testRepeatedFailureEscalatesHuman);
test("agent reflection: lesson hash deduplication, bestAttempt backtracking and reset reuse", testLessonDeduplicationBacktrackingAndReset);
test("agent reflection: invalid config, empty/full slot guards and score clamping", testInvalidConfigFullSlotAndBoundsGuards);
