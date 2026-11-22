// Origin: Pure-CP Stdlib Test Suite (lib/agent/durable_journal.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Durable Execution Journal:
//   1. Live execution of 3 steps & result persistence + double-complete guard
//   2. Crash recovery via begin_replay(): 3 cached replays + seamless transition to execute_live on step 4
//   3. Unfinished pending step resumes in-place as execute_live without duplicating journal entries
//   4. Non-determinism guard: changed step kind during replay triggers divergence_detected
//   5. Non-determinism guard: changed input_hash during replay triggers divergence_detected
//   6. Failed steps replay deterministically with cached_outcome == failed without re-execution
//   7. Oversized result (>128B) rejection, OOB get_step & 128-step journal_full boundary guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    DURABLE_JOURNAL_MAX_STEPS,
    DURABLE_RESULT_MAX_LEN,
    DurableStepKind,
    DurableStepOutcome,
    DurableReplayAction,
    DurableStepEntry,
    DurableStepDecision,
    DurableJournalStats,
    DurableExecutionJournal,
} from "cap:agent";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// ============================================================================
// Test 1: Live 3-Step Execution & Persistence + Double-Complete Guard
// ============================================================================

fn testLiveExecutionAndPersistence()! {
    let j = cap.agent.DurableExecutionJournal.init(1001);

    const d0: DurableStepDecision = j.next_step(DurableStepKind.llm_completion, 0x1111, 1000);
    expect_eq(d0.action == DurableReplayAction.execute_live, true);
    expect_eq(d0.step_seq, 0);
    expect_eq(j.complete_step(0, true, "plan:call_search"), true);
    // Completing an already-completed step must fail
    expect_eq(j.complete_step(0, true, "overwrite"), false);

    const d1 = j.next_step(DurableStepKind.tool_call, 0x2222, 1050);
    expect_eq(d1.action == DurableReplayAction.execute_live, true);
    expect_eq(d1.step_seq, 1);
    expect_eq(j.complete_step(1, true, "{\"hits\":5}"), true);

    const d2 = j.next_step(DurableStepKind.human_approval, 0x3333, 1100);
    expect_eq(d2.action == DurableReplayAction.execute_live, true);
    expect_eq(d2.step_seq, 2);
    expect_eq(j.complete_step(2, true, "approved_by_ops"), true);

    const e1: DurableStepEntry = j.get_step(1) ?? return error.MissingStep;
    expect_eq(e1.outcome == DurableStepOutcome.succeeded, true);
    expect_eq(sliceEq(e1.resultSlice(), "{\"hits\":5}"), true);

    const st: DurableJournalStats = j.stats();
    expect_eq(st.recorded_steps, 3);
    expect_eq(st.live_executions, 3);
    expect_eq(st.replayed_hits, 0);
    expect_eq(st.is_replaying, false);
}

// ============================================================================
// Test 2: Crash Recovery Replay (3 Cached Hits -> Seamless Step 4 Live)
// ============================================================================

fn testCrashReplayAndSeamlessLiveHandoff()! {
    let j = DurableExecutionJournal.init(1002);

    // Run 3 steps live before simulated crash
    const s0 = j.next_step(DurableStepKind.llm_completion, 0xAAA1, 100);
    expect_eq(j.complete_step(s0.step_seq, true, "resp_0"), true);
    const s1 = j.next_step(DurableStepKind.tool_call, 0xBBB2, 200);
    expect_eq(j.complete_step(s1.step_seq, true, "resp_1"), true);
    const s2 = j.next_step(DurableStepKind.timer_sleep, 0xCCC3, 300);
    expect_eq(j.complete_step(s2.step_seq, true, "woke_up"), true);

    // Simulated crash & process restart -> replay from step 0
    j.begin_replay();
    expect_eq(j.stats().is_replaying, true);
    expect_eq(j.stats().replay_cursor, 0);

    const r0 = j.next_step(DurableStepKind.llm_completion, 0xAAA1, 1000);
    expect_eq(r0.action == DurableReplayAction.replay_cached, true);
    expect_eq(r0.step_seq, 0);
    expect_eq(r0.cached_outcome == DurableStepOutcome.succeeded, true);

    const r1 = j.next_step(DurableStepKind.tool_call, 0xBBB2, 1001);
    expect_eq(r1.action == DurableReplayAction.replay_cached, true);
    expect_eq(r1.step_seq, 1);
    const e1 = j.get_step(r1.step_seq) ?? return error.Missing;
    expect_eq(sliceEq(e1.resultSlice(), "resp_1"), true);

    const r2 = j.next_step(DurableStepKind.timer_sleep, 0xCCC3, 1002);
    expect_eq(r2.action == DurableReplayAction.replay_cached, true);
    expect_eq(r2.step_seq, 2);

    // Step 4 (index 3) is past journal tip -> seamlessly transitions to execute_live!
    const s3 = j.next_step(DurableStepKind.subagent_spawn, 0xDDD4, 1005);
    expect_eq(s3.action == DurableReplayAction.execute_live, true);
    expect_eq(s3.step_seq, 3);
    expect_eq(j.complete_step(3, true, "subagent_done"), true);

    const st = j.stats();
    expect_eq(st.recorded_steps, 4);
    expect_eq(st.replayed_hits, 3);
    expect_eq(st.live_executions, 4);
    expect_eq(st.is_replaying, false);
}

// ============================================================================
// Test 3: Unfinished Pending Step Resumes In-Place After Crash
// ============================================================================

fn testPendingStepResumesInPlaceOnCrash()! {
    let j = DurableExecutionJournal.init(1003);

    const s0 = j.next_step(DurableStepKind.llm_completion, 0x01, 100);
    expect_eq(j.complete_step(s0.step_seq, true, "ok_0"), true);
    // Step 1 starts, then worker crashes BEFORE complete_step(1, ...) is called!
    const s1 = j.next_step(DurableStepKind.tool_call, 0x02, 200);
    expect_eq(s1.step_seq, 1);

    // Restart & replay
    j.begin_replay();
    const r0 = j.next_step(DurableStepKind.llm_completion, 0x01, 900);
    expect_eq(r0.action == DurableReplayAction.replay_cached, true);

    // Reaching pending step 1 -> returns execute_live with SAME step_seq = 1!
    const r1 = j.next_step(DurableStepKind.tool_call, 0x02, 950);
    expect_eq(r1.action == DurableReplayAction.execute_live, true);
    expect_eq(r1.step_seq, 1);
    expect_eq(r1.cached_outcome == DurableStepOutcome.pending, true);
    expect_eq(j.stats().recorded_steps, 2); // no duplicate step appended
    expect_eq(j.stats().is_replaying, false);

    // Now complete step 1 cleanly
    expect_eq(j.complete_step(1, true, "resumed_ok"), true);
    const e1 = j.get_step(1) ?? return error.Missing;
    expect_eq(sliceEq(e1.resultSlice(), "resumed_ok"), true);
}

// ============================================================================
// Test 4: Divergence Detection on Changed Step Kind
// ============================================================================

fn testDivergenceOnChangedKind()! {
    let j = DurableExecutionJournal.init(1004);
    const s0 = j.next_step(DurableStepKind.tool_call, 0x5555, 100);
    expect_eq(j.complete_step(s0.step_seq, true, "done"), true);

    j.begin_replay();
    // Code changed after crash: now calls llm_completion instead of tool_call!
    const div = j.next_step(DurableStepKind.llm_completion, 0x5555, 200);
    expect_eq(div.action == DurableReplayAction.divergence_detected, true);
    expect_eq(div.step_seq, 0);
    expect_eq(div.expected_kind == DurableStepKind.tool_call, true);
    expect_eq(div.expected_input_hash, 0x5555);
    expect_eq(j.stats().divergences_detected, 1);
    expect_eq(j.stats().replay_cursor, 0); // cursor stays locked on divergence
}

// ============================================================================
// Test 5: Divergence Detection on Changed input_hash
// ============================================================================

fn testDivergenceOnChangedInputHash()! {
    let j = DurableExecutionJournal.init(1005);
    const s0 = j.next_step(DurableStepKind.tool_call, 0xCAFE_0001, 100);
    expect_eq(j.complete_step(s0.step_seq, true, "charged"), true);

    j.begin_replay();
    // Same kind, but non-deterministic argument hash changed!
    const div = j.next_step(DurableStepKind.tool_call, 0xCAFE_9999, 200);
    expect_eq(div.action == DurableReplayAction.divergence_detected, true);
    expect_eq(div.expected_input_hash, 0xCAFE_0001);
    expect_eq(j.stats().divergences_detected, 1);
}

// ============================================================================
// Test 6: Failed Steps Replay Deterministically Without Re-Running
// ============================================================================

fn testFailedStepReplaysDeterministically()! {
    let j = DurableExecutionJournal.init(1006);
    const s0 = j.next_step(DurableStepKind.tool_call, 0xDEAD, 100);
    expect_eq(j.complete_step(s0.step_seq, false, "err:403_forbidden"), true);

    j.begin_replay();
    const r0 = j.next_step(DurableStepKind.tool_call, 0xDEAD, 500);
    expect_eq(r0.action == DurableReplayAction.replay_cached, true);
    expect_eq(r0.cached_outcome == DurableStepOutcome.failed, true);

    const e0 = j.get_step(0) ?? return error.Missing;
    expect_eq(e0.outcome == DurableStepOutcome.failed, true);
    expect_eq(sliceEq(e0.resultSlice(), "err:403_forbidden"), true);
}

// ============================================================================
// Test 7: Oversized Result Rejection, OOB get_step & 128-Step journal_full
// ============================================================================

fn testOversizedResultAndJournalFullGuards()! {
    let j = DurableExecutionJournal.init(1007);

    // OOB get_step & complete_step on empty journal
    expect_eq(j.get_step(0) == null, true);
    expect_eq(j.complete_step(0, true, "x"), false);

    // Oversized result (> 128 bytes) rejected while preserving pending state
    const s0 = j.next_step(DurableStepKind.llm_completion, 0, 10);
    const big_res: [DURABLE_RESULT_MAX_LEN + 1]u8 = ['a'; DURABLE_RESULT_MAX_LEN + 1];
    expect_eq(j.complete_step(s0.step_seq, true, big_res[0..]), false);
    expect_eq(j.complete_step(s0.step_seq, true, "ok"), true);

    // Fill remaining 127 steps up to DURABLE_JOURNAL_MAX_STEPS (128)
    let i: usize = 1;
    while (i < DURABLE_JOURNAL_MAX_STEPS) : (i += 1) {
        const step_d = j.next_step(DurableStepKind.tool_call, i as u64, (i as u64) * 10);
        expect_eq(step_d.action == DurableReplayAction.execute_live, true);
        expect_eq(j.complete_step(step_d.step_seq, true, "ok"), true);
    }
    expect_eq(j.stats().recorded_steps, DURABLE_JOURNAL_MAX_STEPS);

    // 129th step returns journal_full safely
    const overflow = j.next_step(DurableStepKind.tool_call, 999, 9999);
    expect_eq(overflow.action == DurableReplayAction.journal_full, true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("agent durable_journal: live 3-step execution, result persistence & double-complete guard", testLiveExecutionAndPersistence);
test("agent durable_journal: crash replay hits cache for 3 steps & seamlessly enters execute_live on step 4", testCrashReplayAndSeamlessLiveHandoff);
test("agent durable_journal: unfinished pending step resumes in-place on crash recovery", testPendingStepResumesInPlaceOnCrash);
test("agent durable_journal: changed step kind during replay triggers divergence_detected", testDivergenceOnChangedKind);
test("agent durable_journal: changed input_hash during replay triggers divergence_detected", testDivergenceOnChangedInputHash);
test("agent durable_journal: failed step replays deterministically with cached_outcome == failed", testFailedStepReplaysDeterministically);
test("agent durable_journal: oversized result rejection, OOB get_step & 128-step journal_full guards", testOversizedResultAndJournalFullGuards);
