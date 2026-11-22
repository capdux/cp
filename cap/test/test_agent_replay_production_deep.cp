// Origin: Pure-CP Stdlib Test Suite (lib/agent/replay.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Deterministic Offline Replay & Trajectory Diff Debugger:
//   1. Cassette recording & StrictOrder deterministic sequential playback + reset()
//   2. StrictOrder drift guard: arg_mismatches vs misses without advancing cursor
//   3. KeyedLoose out-of-order playback, repeated hits & argument drift detection
//   4. Step-by-step trajectory diff: exact match, multi-class divergences & fidelity_score
//   5. Cassette capacity full guard, frameAt bounds & clear() reuse
//   6. Empty cassette/trajectory edge cases & out_diffs truncation boundary protection

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    computeReplayArgsHash,
    ReplayEntryKind,
    ReplayMatchMode,
    ReplayFrame,
    ReplayCassette,
    ReplayStats,
    ReplayPlayer,
    ReplayDiffKind,
    ReplayStepDiff,
    ReplayDiffSummary,
    diffReplayTrajectories,
} from "cap:agent";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const d = if (a >= b) (a - b) else (b - a);
    return d <= eps;
}

// ============================================================================
// Test 1: Recording & StrictOrder Sequential Replay
// ============================================================================

fn testStrictOrderRecordingAndReplay()! {
    let storage: [4]ReplayFrame = [ReplayFrame{}; 4];
    let storage_s: []ReplayFrame = storage[0..];
    let cassette = cap.agent.ReplayCassette.init(storage_s);

    expect_eq(cassette.capacity(), 4);
    expect_eq(cassette.count(), 0);

    expect_eq(cassette.recordRawArgs(
        ReplayEntryKind.LlmCall, "claude-sonnet", "{\"prompt\":\"plan\"}", "call_tool:read_file", 180, true,
    ), true);
    expect_eq(cassette.recordRawArgs(
        ReplayEntryKind.ToolCall, "read_file", "{\"path\":\"lib/agent/replay.cp\"}", "fn replay()", 12, true,
    ), true);
    expect_eq(cassette.recordRawArgs(
        ReplayEntryKind.LlmCall, "claude-sonnet", "{\"obs\":\"fn replay()\"}", "final answer", 140, true,
    ), true);
    expect_eq(cassette.count(), 3);

    let player = ReplayPlayer.init(cassette.frames(), ReplayMatchMode.StrictOrder);
    expect_eq(player.remaining(), 3);
    expect_eq(player.isExhausted(), false);

    const f0 = player.replayRawArgs(ReplayEntryKind.LlmCall, "claude-sonnet", "{\"prompt\":\"plan\"}") ?? return error.ReplayMiss;
    expect_eq(f0.seq, 0);
    expect_eq(strEq(f0.response, "call_tool:read_file"), true);
    expect_eq(f0.latency_ms, 180);

    const f1 = player.replayRawArgs(ReplayEntryKind.ToolCall, "read_file", "{\"path\":\"lib/agent/replay.cp\"}") ?? return error.ReplayMiss;
    expect_eq(f1.seq, 1);
    expect_eq(strEq(f1.response, "fn replay()"), true);

    const f2 = player.replayRawArgs(ReplayEntryKind.LlmCall, "claude-sonnet", "{\"obs\":\"fn replay()\"}") ?? return error.ReplayMiss;
    expect_eq(f2.seq, 2);
    expect_eq(strEq(f2.response, "final answer"), true);

    expect_eq(player.isExhausted(), true);
    expect_eq(player.remaining(), 0);
    const st: ReplayStats = player.stats();
    expect_eq(st.hits, 3);
    expect_eq(st.misses, 0);
    expect_eq(st.arg_mismatches, 0);
    expect_eq(st.cursor, 3);
    expect_eq(st.total_frames, 3);

    // Extra call after exhaustion counts as miss
    expect_eq(player.replayRawArgs(ReplayEntryKind.LlmCall, "claude-sonnet", "{}") == null, true);
    expect_eq(player.stats().misses, 1);

    // Reset rewinds cleanly
    player.reset();
    expect_eq(player.remaining(), 3);
    expect_eq(player.stats().hits, 0);
    expect_eq(player.stats().misses, 0);
    expect_eq(player.replayRawArgs(ReplayEntryKind.LlmCall, "claude-sonnet", "{\"prompt\":\"plan\"}") != null, true);
}

// ============================================================================
// Test 2: StrictOrder Drift Interception (Args Mismatch vs Name/Kind Miss)
// ============================================================================

fn testStrictOrderDriftInterception()! {
    let storage: [2]ReplayFrame = [ReplayFrame{}; 2];
    let storage_s: []ReplayFrame = storage[0..];
    let cassette = ReplayCassette.init(storage_s);

    * = cassette.recordRawArgs(ReplayEntryKind.ToolCall, "search_docs", "{\"q\":\"actor\"}", "hit1", 10, true);
    * = cassette.recordRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "{\"ctx\":\"hit1\"}", "done", 90, true);

    let player = ReplayPlayer.init(cassette.frames(), ReplayMatchMode.StrictOrder);

    // 2a. Same kind & name, drifted args -> arg_mismatches += 1, cursor stays at 0
    expect_eq(player.replayRawArgs(ReplayEntryKind.ToolCall, "search_docs", "{\"q\":\"beam\"}") == null, true);
    expect_eq(player.stats().arg_mismatches, 1);
    expect_eq(player.stats().misses, 0);
    expect_eq(player.stats().cursor, 0);

    // 2b. Wrong name -> misses += 1, cursor stays at 0
    expect_eq(player.replayRawArgs(ReplayEntryKind.ToolCall, "read_file", "{\"q\":\"actor\"}") == null, true);
    expect_eq(player.stats().misses, 1);
    expect_eq(player.stats().cursor, 0);

    // 2c. Wrong kind -> misses += 1, cursor stays at 0
    expect_eq(player.replayRawArgs(ReplayEntryKind.LlmCall, "search_docs", "{\"q\":\"actor\"}") == null, true);
    expect_eq(player.stats().misses, 2);
    expect_eq(player.stats().cursor, 0);

    // 2d. Exact call still succeeds because cursor was preserved
    const hit = player.replayRawArgs(ReplayEntryKind.ToolCall, "search_docs", "{\"q\":\"actor\"}") ?? return error.ReplayMiss;
    expect_eq(strEq(hit.response, "hit1"), true);
    expect_eq(player.stats().hits, 1);
    expect_eq(player.stats().cursor, 1);
}

// ============================================================================
// Test 3: KeyedLoose Out-of-Order Playback & Repeated Lookups
// ============================================================================

fn testKeyedLooseOutOfOrderReplay()! {
    let storage: [3]ReplayFrame = [ReplayFrame{}; 3];
    let storage_s: []ReplayFrame = storage[0..];
    let cassette = ReplayCassette.init(storage_s);

    * = cassette.recordRawArgs(ReplayEntryKind.ToolCall, "fetch_a", "{\"id\":1}", "resp_a", 5, true);
    * = cassette.recordRawArgs(ReplayEntryKind.ToolCall, "fetch_b", "{\"id\":2}", "resp_b", 8, true);
    * = cassette.recordRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "{\"sum\":3}", "final_ok", 100, true);

    let player = ReplayPlayer.init(cassette.frames(), ReplayMatchMode.KeyedLoose);

    // Replay in reversed order (e.g. parallel fan-out completed out of order)
    const fb = player.replayRawArgs(ReplayEntryKind.ToolCall, "fetch_b", "{\"id\":2}") ?? return error.ReplayMiss;
    expect_eq(strEq(fb.response, "resp_b"), true);

    const fa = player.replayRawArgs(ReplayEntryKind.ToolCall, "fetch_a", "{\"id\":1}") ?? return error.ReplayMiss;
    expect_eq(strEq(fa.response, "resp_a"), true);

    // Repeated lookup wraps around cleanly
    const fb2 = player.replayRawArgs(ReplayEntryKind.ToolCall, "fetch_b", "{\"id\":2}") ?? return error.ReplayMiss;
    expect_eq(strEq(fb2.response, "resp_b"), true);

    // Same name with drifted args -> arg_mismatches += 1
    expect_eq(player.replayRawArgs(ReplayEntryKind.ToolCall, "fetch_a", "{\"id\":999}") == null, true);
    expect_eq(player.stats().arg_mismatches, 1);

    // Completely unknown tool -> misses += 1
    expect_eq(player.replayRawArgs(ReplayEntryKind.ToolCall, "unknown_tool", "{}") == null, true);
    expect_eq(player.stats().misses, 1);
    expect_eq(player.stats().hits, 3);
}

// ============================================================================
// Test 4: Trajectory Step-by-Step Diff & Fidelity Score
// ============================================================================

fn testTrajectoryDiffAndFidelity()! {
    let base_buf: [5]ReplayFrame = [ReplayFrame{}; 5];
    let cand_buf: [6]ReplayFrame = [ReplayFrame{}; 6];
    let base_s: []ReplayFrame = base_buf[0..];
    let cand_s: []ReplayFrame = cand_buf[0..];
    let base = ReplayCassette.init(base_s);
    let cand = ReplayCassette.init(cand_s);

    // Step 0: exact match
    * = base.recordRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "p0", "r0", 50, true);
    * = cand.recordRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "p0", "r0", 55, true);

    // Step 1: ArgsMismatch (same kind & name, different args)
    * = base.recordRawArgs(ReplayEntryKind.ToolCall, "search", "{\"q\":\"a\"}", "r1", 10, true);
    * = cand.recordRawArgs(ReplayEntryKind.ToolCall, "search", "{\"q\":\"b\"}", "r1", 10, true);

    // Step 2: NameMismatch (same kind, different tool name)
    * = base.recordRawArgs(ReplayEntryKind.ToolCall, "read_file", "{\"p\":\"x\"}", "r2", 10, true);
    * = cand.recordRawArgs(ReplayEntryKind.ToolCall, "grep_file", "{\"p\":\"x\"}", "r2", 10, true);

    // Step 3: KindMismatch (ToolCall vs LlmCall)
    * = base.recordRawArgs(ReplayEntryKind.ToolCall, "write_file", "{\"p\":\"x\"}", "r3", 10, true);
    * = cand.recordRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "{\"p\":\"x\"}", "r3", 80, true);

    // Step 4: exact match
    * = base.recordRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "p4", "r4", 60, true);
    * = cand.recordRawArgs(ReplayEntryKind.LlmCall, "gpt-4o", "p4", "r4", 60, true);

    // Step 5: ExtraStep in candidate
    * = cand.recordRawArgs(ReplayEntryKind.ToolCall, "cleanup", "{}", "ok", 5, true);

    let diffs: [8]ReplayStepDiff = [ReplayStepDiff{}; 8];
    let diffs_s: []ReplayStepDiff = diffs[0..];

    // 4a. Baseline vs itself -> exact match, fidelity = 1.0
    const exact: ReplayDiffSummary = diffReplayTrajectories(base.frames(), base.frames(), diffs_s);
    expect_eq(exact.isExactMatch(), true);
    expect_eq(exact.has_divergence, false);
    expect_eq(exact.matched_steps, 5);
    expect_eq(exact.diffs_written, 0);
    expect_eq(approxEq(exact.fidelity_score, 1.0, 0.000000001), true);

    // 4b. Baseline (5) vs Candidate (6): 2 matched / 6 total = 1/3
    const sum: ReplayDiffSummary = diffReplayTrajectories(base.frames(), cand.frames(), diffs_s);
    expect_eq(sum.isExactMatch(), false);
    expect_eq(sum.has_divergence, true);
    expect_eq(sum.first_divergence_step, 1);
    expect_eq(sum.compared_steps, 6);
    expect_eq(sum.matched_steps, 2);
    expect_eq(sum.mismatch_steps, 3);
    expect_eq(sum.missing_steps, 0);
    expect_eq(sum.extra_steps, 1);
    expect_eq(sum.diffs_written, 4);
    expect_eq(sum.diffs_truncated, false);
    expect_eq(approxEq(sum.fidelity_score, 2.0 / 6.0, 0.000000001), true);

    expect_eq(diffs[0].step_index, 1);
    expect_eq(diffs[0].kind == ReplayDiffKind.ArgsMismatch, true);
    expect_eq(diffs[0].baseline_args_hash == computeReplayArgsHash("{\"q\":\"a\"}"), true);
    expect_eq(diffs[0].candidate_args_hash == computeReplayArgsHash("{\"q\":\"b\"}"), true);

    expect_eq(diffs[1].step_index, 2);
    expect_eq(diffs[1].kind == ReplayDiffKind.NameMismatch, true);
    expect_eq(strEq(diffs[1].baseline_name, "read_file"), true);
    expect_eq(strEq(diffs[1].candidate_name, "grep_file"), true);

    expect_eq(diffs[2].step_index, 3);
    expect_eq(diffs[2].kind == ReplayDiffKind.KindMismatch, true);

    expect_eq(diffs[3].step_index, 5);
    expect_eq(diffs[3].kind == ReplayDiffKind.ExtraStep, true);
    expect_eq(strEq(diffs[3].candidate_name, "cleanup"), true);

    // 4c. Reverse comparison (Candidate as baseline vs shorter Base) -> MissingStep at index 5
    const rev: ReplayDiffSummary = diffReplayTrajectories(cand.frames(), base.frames(), diffs_s);
    expect_eq(rev.missing_steps, 1);
    expect_eq(rev.extra_steps, 0);
    expect_eq(diffs[3].kind == ReplayDiffKind.MissingStep, true);
}

// ============================================================================
// Test 5: Cassette Capacity Guard, Bounds & Clear Reuse
// ============================================================================

fn testCassetteCapacityAndClear()! {
    let storage: [2]ReplayFrame = [ReplayFrame{}; 2];
    let storage_s: []ReplayFrame = storage[0..];
    let cassette = ReplayCassette.init(storage_s);

    expect_eq(cassette.record(ReplayEntryKind.LlmCall, "m1", 11, "r1", 1, true), true);
    expect_eq(cassette.isFull(), false);
    expect_eq(cassette.record(ReplayEntryKind.ToolCall, "t1", 22, "r2", 2, false), true);
    expect_eq(cassette.isFull(), true);

    // 3rd record rejected cleanly
    expect_eq(cassette.record(ReplayEntryKind.LlmCall, "m2", 33, "r3", 3, true), false);
    expect_eq(cassette.count(), 2);

    // frameAt bounds
    const f1 = cassette.frameAt(1) ?? return error.BoundsError;
    expect_eq(f1.succeeded, false);
    expect_eq(cassette.frameAt(2) == null, true);

    // clear() allows reuse
    cassette.clear();
    expect_eq(cassette.count(), 0);
    expect_eq(cassette.isFull(), false);
    expect_eq(cassette.record(ReplayEntryKind.LlmCall, "m3", 44, "r4", 4, true), true);
    expect_eq(cassette.count(), 1);
}

// ============================================================================
// Test 6: Empty Edge Cases & Diff Output Truncation Protection
// ============================================================================

fn testEmptyAndDiffTruncationGuards()! {
    let empty_frames: [0]ReplayFrame = [];
    let empty_diffs: [0]ReplayStepDiff = [];
    let empty_diffs_s: []ReplayStepDiff = empty_diffs[0..];

    // 6a. Empty cassette playback in both modes
    let p_strict = ReplayPlayer.init(empty_frames[0..], ReplayMatchMode.StrictOrder);
    expect_eq(p_strict.isExhausted(), true);
    expect_eq(p_strict.replay(ReplayEntryKind.LlmCall, "m", 1) == null, true);
    expect_eq(p_strict.stats().misses, 1);

    let p_loose = ReplayPlayer.init(empty_frames[0..], ReplayMatchMode.KeyedLoose);
    expect_eq(p_loose.replay(ReplayEntryKind.LlmCall, "m", 1) == null, true);
    expect_eq(p_loose.stats().misses, 1);

    // 6b. Empty vs empty diff -> exact match, fidelity = 1.0
    const empty_sum = diffReplayTrajectories(empty_frames[0..], empty_frames[0..], empty_diffs_s);
    expect_eq(empty_sum.isExactMatch(), true);
    expect_eq(approxEq(empty_sum.fidelity_score, 1.0, 0.000000001), true);

    // 6c. Small out_diffs buffer (len 1) when 3 divergences exist -> safe truncation
    let b_arr: [3]ReplayFrame = [ReplayFrame{}; 3];
    let b_s: []ReplayFrame = b_arr[0..];
    let bc = ReplayCassette.init(b_s);
    * = bc.recordRawArgs(ReplayEntryKind.LlmCall, "m", "a", "r", 1, true);
    * = bc.recordRawArgs(ReplayEntryKind.ToolCall, "t1", "b", "r", 1, true);
    * = bc.recordRawArgs(ReplayEntryKind.ToolCall, "t2", "c", "r", 1, true);

    let tiny_diffs: [1]ReplayStepDiff = [ReplayStepDiff{}; 1];
    let tiny_s: []ReplayStepDiff = tiny_diffs[0..];
    const trunc_sum = diffReplayTrajectories(bc.frames(), empty_frames[0..], tiny_s);
    expect_eq(trunc_sum.missing_steps, 3);
    expect_eq(trunc_sum.first_divergence_step, 0);
    expect_eq(trunc_sum.diffs_written, 1);
    expect_eq(trunc_sum.diffs_truncated, true);
    expect_eq(approxEq(trunc_sum.fidelity_score, 0.0, 0.000000001), true);
    expect_eq(tiny_diffs[0].kind == ReplayDiffKind.MissingStep, true);
}

test("agent.replay: cassette recording, StrictOrder playback & reset", testStrictOrderRecordingAndReplay);
test("agent.replay: StrictOrder drift interception (arg_mismatches vs misses)", testStrictOrderDriftInterception);
test("agent.replay: KeyedLoose out-of-order & repeated playback", testKeyedLooseOutOfOrderReplay);
test("agent.replay: step-by-step trajectory diff & fidelity score", testTrajectoryDiffAndFidelity);
test("agent.replay: cassette capacity full guard, frameAt bounds & clear reuse", testCassetteCapacityAndClear);
test("agent.replay: empty edge cases & diff output truncation defense", testEmptyAndDiffTruncationGuards);
