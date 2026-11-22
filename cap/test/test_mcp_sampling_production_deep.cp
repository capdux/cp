// Origin: Pure-CP Stdlib Test Suite (MCP Sampling Model Preference Router, Recursion Guard & Progress Tracker) | Created: 2026-11-22
// Deep Production Verification Suite for lib/mcp/sampling.cp
// Covers:
//   1. 3D ModelPreferences weighted scoring, tie-breaking, unavailable & min_tokens filtering
//   2. SamplingDepthGuard nested recursion depth cap, cumulative tree token budget & overflow/underflow safety
//   3. McpProgressTracker strict monotonic advancement, regression/duplicate/overflow rejection & dynamic total
//   4. Explicit/auto completion, percentage calculation, single-slot removal & bulk purgeCompleted recycling
//   5. Invalid weights (negative, >1, NaN, Inf), invalid candidate scores, empty slices & zero-slot tracker safety

import { test, expect_eq } from "cap:test";
import {
    isValidUnitWeight,
    ModelPreferences,
    ModelCandidate,
    scoreSamplingCandidate,
    selectSamplingModel,
    SamplingDepthGuard,
    ProgressEntry,
    ProgressUpdateStatus,
    McpProgressTracker,
} from "../../lib/mcp/index.cp";

fn testSamplingModelPreferenceSelectionAndFiltering()! {
    const candidates: [4]ModelCandidate = [
        // id=101: ultra-cheap & fast mini model, modest intelligence, 8k context
        ModelCandidate{
            model_id: 101,
            cost_score: 0.96,
            speed_score: 0.92,
            intelligence_score: 0.55,
            max_tokens: 8192,
            available: true,
        },
        // id=202: balanced mid-tier model, 32k context
        ModelCandidate{
            model_id: 202,
            cost_score: 0.70,
            speed_score: 0.75,
            intelligence_score: 0.82,
            max_tokens: 32768,
            available: true,
        },
        // id=303: frontier reasoning model, expensive/slower, 128k context
        ModelCandidate{
            model_id: 303,
            cost_score: 0.20,
            speed_score: 0.35,
            intelligence_score: 0.99,
            max_tokens: 131072,
            available: true,
        },
        // id=404: perfect score but unavailable
        ModelCandidate{
            model_id: 404,
            cost_score: 1.0,
            speed_score: 1.0,
            intelligence_score: 1.0,
            max_tokens: 262144,
            available: false,
        },
    ];

    // 1. Cost-focused preference picks id=101 (unavailable id=404 must be filtered out)
    const cost_prefs = ModelPreferences{
        cost_priority: 0.95,
        speed_priority: 0.20,
        intelligence_priority: 0.05,
    };
    expect_eq(selectSamplingModel(cost_prefs, 4096, candidates[0..]), 101);

    // 2. Speed-focused preference picks id=101 when min_tokens fits 8192
    const speed_prefs = ModelPreferences{
        cost_priority: 0.0,
        speed_priority: 1.0,
        intelligence_priority: 0.0,
    };
    expect_eq(selectSamplingModel(speed_prefs, 8192, candidates[0..]), 101);

    // 3. Same speed preference with min_tokens=16384 filters out id=101 -> picks id=202
    expect_eq(selectSamplingModel(speed_prefs, 16384, candidates[0..]), 202);

    // 4. Intelligence-focused preference picks id=303
    const smart_prefs = ModelPreferences{
        cost_priority: 0.05,
        speed_priority: 0.10,
        intelligence_priority: 0.95,
    };
    expect_eq(selectSamplingModel(smart_prefs, 4096, candidates[0..]), 303);

    // 5. All-zero preference falls back to unweighted 3D average:
    //    101 avg = 0.81, 202 avg = 0.7567, 303 avg = 0.5133 -> picks 101
    const zero_prefs = ModelPreferences{
        cost_priority: 0.0,
        speed_priority: 0.0,
        intelligence_priority: 0.0,
    };
    expect_eq(selectSamplingModel(zero_prefs, 1024, candidates[0..]), 101);

    // 6. min_tokens exceeding every available model returns null
    expect_eq(selectSamplingModel(ModelPreferences.balanced(), 200000, candidates[0..]) == null, true);

    // 7. Deterministic tie-breaking preserves earlier candidate in slice order
    const tied: [2]ModelCandidate = [
        ModelCandidate{
            model_id: 11,
            cost_score: 0.8,
            speed_score: 0.8,
            intelligence_score: 0.8,
            max_tokens: 4096,
            available: true,
        },
        ModelCandidate{
            model_id: 22,
            cost_score: 0.8,
            speed_score: 0.8,
            intelligence_score: 0.8,
            max_tokens: 8192,
            available: true,
        },
    ];
    expect_eq(selectSamplingModel(ModelPreferences.balanced(), 1024, tied[0..]), 11);
}

fn testSamplingDepthGuardAndTokenBudget()! {
    let guard = SamplingDepthGuard.init(3, 10000);
    expect_eq(guard.current_depth, 0);
    expect_eq(guard.spent_tokens, 0);
    expect_eq(guard.remainingTokens(), 10000);
    expect_eq(guard.remainingDepth(), 3);

    // Enter 3 nested frames cleanly
    expect_eq(guard.tryEnter(2500), true);
    expect_eq(guard.current_depth, 1);
    expect_eq(guard.remainingTokens(), 7500);

    expect_eq(guard.tryEnter(3000), true);
    expect_eq(guard.current_depth, 2);
    expect_eq(guard.remainingTokens(), 4500);

    expect_eq(guard.tryEnter(1500), true);
    expect_eq(guard.current_depth, 3);
    expect_eq(guard.remainingDepth(), 0);
    expect_eq(guard.remainingTokens(), 3000);

    // 4th nested call must be hard-blocked by max_depth without mutating budget
    expect_eq(guard.tryEnter(100), false);
    expect_eq(guard.current_depth, 3);
    expect_eq(guard.remainingTokens(), 3000);

    // Exit one level -> depth drops to 2, cumulative tree tokens remain 7000
    guard.exit();
    expect_eq(guard.current_depth, 2);
    expect_eq(guard.remainingTokens(), 3000);

    // Requesting more than remainingTokens() (including u32 max overflow probe) must fail
    expect_eq(guard.tryEnter(3001), false);
    expect_eq(guard.tryEnter(4294967295), false);
    expect_eq(guard.current_depth, 2);
    expect_eq(guard.remainingTokens(), 3000);

    // Exact remaining budget succeeds
    expect_eq(guard.tryEnter(3000), true);
    expect_eq(guard.remainingTokens(), 0);
    expect_eq(guard.spent_tokens, 10000);

    // Refund unused tokens & verify underflow clamping
    guard.refundTokens(1200);
    expect_eq(guard.remainingTokens(), 1200);
    guard.refundTokens(999999);
    expect_eq(guard.spent_tokens, 0);
    expect_eq(guard.remainingTokens(), 10000);

    // Unwind all frames + extra exit() calls must saturate safely at 0
    guard.exit();
    guard.exit();
    guard.exit();
    guard.exit();
    expect_eq(guard.current_depth, 0);

    // Reset clears both depth and spent tokens
    expect_eq(guard.tryEnter(4000), true);
    guard.reset();
    expect_eq(guard.current_depth, 0);
    expect_eq(guard.spent_tokens, 0);
    expect_eq(guard.remainingTokens(), 10000);
}

fn testProgressTrackerMonotonicAdvancementAndRejections()! {
    let slots_buf: [4]ProgressEntry = [ProgressEntry{}; 4];
    let slots: []ProgressEntry = slots_buf[0..];
    let tracker = McpProgressTracker.init(slots);

    expect_eq(tracker.capacity(), 4);
    expect_eq(tracker.activeCount(), 0);

    // Register token 1001 with total=200
    expect_eq(tracker.register(1001, 200), true);
    expect_eq(tracker.activeCount(), 1);
    expect_eq(tracker.percent(1001), 0.0);

    // Monotonic advancement: 0 -> 50 -> 150
    expect_eq(tracker.update(1001, 50, null), ProgressUpdateStatus.Updated);
    expect_eq(tracker.percent(1001), 25.0);

    expect_eq(tracker.update(1001, 150, null), ProgressUpdateStatus.Updated);
    expect_eq(tracker.percent(1001), 75.0);

    // Reject regression (100 < 150) and duplicate (150 == 150) without mutating progress
    expect_eq(tracker.update(1001, 100, null), ProgressUpdateStatus.NonMonotonic);
    expect_eq(tracker.update(1001, 150, null), ProgressUpdateStatus.NonMonotonic);
    const snap1 = tracker.get(1001) ?? ProgressEntry{};
    expect_eq(snap1.progress, 150);
    expect_eq(snap1.completed, false);

    // Reject value exceeding total (201 > 200)
    expect_eq(tracker.update(1001, 201, null), ProgressUpdateStatus.ExceedsTotal);
    expect_eq((tracker.get(1001) ?? ProgressEntry{}).progress, 150);

    // Dynamically expand total to 300 while advancing to 240 -> 80%
    expect_eq(tracker.update(1001, 240, 300), ProgressUpdateStatus.Updated);
    expect_eq(tracker.percent(1001), 80.0);

    // Reaching total=300 automatically marks entry Completed
    expect_eq(tracker.update(1001, 300, null), ProgressUpdateStatus.Completed);
    expect_eq(tracker.percent(1001), 100.0);
    expect_eq((tracker.get(1001) ?? ProgressEntry{}).completed, true);

    // Further updates on completed token are rejected
    expect_eq(tracker.update(1001, 301, 400), ProgressUpdateStatus.AlreadyCompleted);

    // Updating unknown token returns NotFound
    expect_eq(tracker.update(9999, 10, 100), ProgressUpdateStatus.NotFound);
}

fn testProgressCompletionAndSlotRecycling()! {
    let slots_buf: [3]ProgressEntry = [ProgressEntry{}; 3];
    let slots: []ProgressEntry = slots_buf[0..];
    let tracker = McpProgressTracker.init(slots);

    // Fill all 3 slots: 10 (indeterminate total=0), 20 (total=10), 30 (total=50)
    expect_eq(tracker.register(10, 0), true);
    expect_eq(tracker.register(20, 10), true);
    expect_eq(tracker.register(30, 50), true);
    expect_eq(tracker.activeCount(), 3);

    // Pool is full -> 4th token rejected
    expect_eq(tracker.register(40, 100), false);

    // Indeterminate token 10 advances monotonically; percent stays 0.0 until explicit complete()
    expect_eq(tracker.update(10, 7, null), ProgressUpdateStatus.Updated);
    expect_eq(tracker.percent(10), 0.0);
    expect_eq(tracker.complete(10), true);
    expect_eq(tracker.percent(10), 100.0);

    // Token 20 auto-completes at 10/10; token 30 stays in progress at 25/50
    expect_eq(tracker.update(20, 10, null), ProgressUpdateStatus.Completed);
    expect_eq(tracker.update(30, 25, null), ProgressUpdateStatus.Updated);

    // Bulk purgeCompleted reclaims tokens 10 and 20, keeping token 30 intact
    const reclaimed = tracker.purgeCompleted();
    expect_eq(reclaimed, 2);
    expect_eq(tracker.activeCount(), 1);
    expect_eq(tracker.get(10) == null, true);
    expect_eq(tracker.get(20) == null, true);
    expect_eq((tracker.get(30) ?? ProgressEntry{}).progress, 25);

    // Reclaimed slots can be reused immediately
    expect_eq(tracker.register(40, 100), true);
    expect_eq(tracker.register(50, 100), true);
    expect_eq(tracker.activeCount(), 3);

    // Single remove frees slot and rejects duplicate remove
    expect_eq(tracker.remove(30), true);
    expect_eq(tracker.remove(30), false);
    expect_eq(tracker.activeCount(), 2);
    expect_eq(tracker.register(60, 10), true);
    expect_eq(tracker.activeCount(), 3);
}

fn testInvalidWeightsZeroSlotsAndEdgeGuards()! {
    const zero_f: f32 = 0.0;
    const one_f: f32 = 1.0;
    const neg_one_f: f32 = -1.0;
    const nan_f: f32 = zero_f / zero_f;
    const pos_inf: f32 = one_f / zero_f;
    const neg_inf: f32 = neg_one_f / zero_f;

    // 1. Unit weight validator rejects NaN, +/-Inf, negative, >1.0
    expect_eq(isValidUnitWeight(0.0), true);
    expect_eq(isValidUnitWeight(0.5), true);
    expect_eq(isValidUnitWeight(1.0), true);
    expect_eq(isValidUnitWeight(-0.01), false);
    expect_eq(isValidUnitWeight(1.01), false);
    expect_eq(isValidUnitWeight(nan_f), false);
    expect_eq(isValidUnitWeight(pos_inf), false);
    expect_eq(isValidUnitWeight(neg_inf), false);

    const valid_cands: [2]ModelCandidate = [
        // Candidate 0 has a NaN score and must be skipped safely
        ModelCandidate{
            model_id: 1,
            cost_score: nan_f,
            speed_score: 0.9,
            intelligence_score: 0.9,
            max_tokens: 16384,
            available: true,
        },
        // Candidate 1 is valid
        ModelCandidate{
            model_id: 2,
            cost_score: 0.6,
            speed_score: 0.7,
            intelligence_score: 0.8,
            max_tokens: 16384,
            available: true,
        },
    ];

    // 2. Invalid preferences (negative / >1 / NaN / Inf) return null
    const neg_prefs = ModelPreferences{ cost_priority: -0.2, speed_priority: 0.5, intelligence_priority: 0.5 };
    const over_prefs = ModelPreferences{ cost_priority: 0.5, speed_priority: 1.5, intelligence_priority: 0.5 };
    const nan_prefs = ModelPreferences{ cost_priority: 0.5, speed_priority: nan_f, intelligence_priority: 0.5 };
    const inf_prefs = ModelPreferences{ cost_priority: pos_inf, speed_priority: 0.5, intelligence_priority: 0.5 };

    expect_eq(selectSamplingModel(neg_prefs, 1024, valid_cands[0..]) == null, true);
    expect_eq(selectSamplingModel(over_prefs, 1024, valid_cands[0..]) == null, true);
    expect_eq(selectSamplingModel(nan_prefs, 1024, valid_cands[0..]) == null, true);
    expect_eq(selectSamplingModel(inf_prefs, 1024, valid_cands[0..]) == null, true);
    expect_eq(scoreSamplingCandidate(nan_prefs, valid_cands[1]) == null, true);

    // 3. clamped() sanitizes invalid preferences and skips NaN candidate -> picks id=2
    const sanitized = over_prefs.clamped();
    expect_eq(sanitized.isValid(), true);
    expect_eq(selectSamplingModel(sanitized, 1024, valid_cands[0..]), 2);

    // 4. Empty candidates slice returns null
    const empty_cands: [0]ModelCandidate = [];
    expect_eq(selectSamplingModel(ModelPreferences.balanced(), 0, empty_cands[0..]) == null, true);

    // 5. Zero-depth guard blocks all sampling entries
    let zero_guard = SamplingDepthGuard.init(0, 10000);
    expect_eq(zero_guard.tryEnter(1), false);
    expect_eq(zero_guard.tryEnter(0), false);
    expect_eq(zero_guard.remainingDepth(), 0);

    // 6. Zero-slot progress tracker handles all operations safely without out-of-bounds
    let empty_slots_buf: [0]ProgressEntry = [];
    let empty_slots: []ProgressEntry = empty_slots_buf[0..];
    let zero_tracker = McpProgressTracker.init(empty_slots);
    expect_eq(zero_tracker.capacity(), 0);
    expect_eq(zero_tracker.activeCount(), 0);
    expect_eq(zero_tracker.register(1, 100), false);
    expect_eq(zero_tracker.update(1, 10, 100), ProgressUpdateStatus.NotFound);
    expect_eq(zero_tracker.complete(1), false);
    expect_eq(zero_tracker.remove(1), false);
    expect_eq(zero_tracker.percent(1) == null, true);
    expect_eq(zero_tracker.purgeCompleted(), 0);

    // 7. Token ID 0 and duplicate registration are rejected on normal tracker
    let one_buf: [2]ProgressEntry = [ProgressEntry{}; 2];
    let one_slots: []ProgressEntry = one_buf[0..];
    let normal_tracker = McpProgressTracker.init(one_slots);
    expect_eq(normal_tracker.register(0, 100), false);
    expect_eq(normal_tracker.register(77, 100), true);
    expect_eq(normal_tracker.register(77, 200), false);
}

test("mcp sampling: 3D model preference scoring, tie-break & availability/token filtering", testSamplingModelPreferenceSelectionAndFiltering);
test("mcp sampling: nested recursion depth guard, tree token budget & overflow safety", testSamplingDepthGuardAndTokenBudget);
test("mcp sampling: progress tracker monotonic advancement & regression/overflow rejection", testProgressTrackerMonotonicAdvancementAndRejections);
test("mcp sampling: progress completion, percentage & slot recycling", testProgressCompletionAndSlotRecycling);
test("mcp sampling: invalid weights (neg/>1/NaN/Inf), zero-slot tracker & edge guards", testInvalidWeightsZeroSlotsAndEdgeGuards);
