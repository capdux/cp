// Origin: Pure-CP Stdlib Test Suite (lib/agent/handoff.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Multi-Agent Collaboration:
//   1. HandoffRouter normal multi-hop trajectory, summary audit & reset
//   2. HandoffRouter cycle protection (A->B->A), self-handoff, max_hops & history capacity guards
//   3. AgentBlackboard optimistic CAS (putIfVersion), writer audit & slot reuse
//   4. AgentBlackboard prefix filtering, incremental delta sync (listSinceSeq) & boundary guards
//   5. FanOutAggregator early quorum readiness & score-ranked result collection
//   6. FanOutAggregator quorum failure detection, invalid init & capacity/NaN guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    HandoffStatus,
    HandoffRecord,
    HandoffRouter,
    BlackboardWriteStatus,
    BlackboardEntry,
    AgentBlackboard,
    FanOutTaskState,
    FanOutStatus,
    FanOutSlot,
    FanOutSummary,
    FanOutAggregator,
} from "cap:agent";

// ============================================================================
// Test 1: HandoffRouter Normal Multi-Hop Trajectory & Reset
// ============================================================================

fn testHandoffNormalTrajectory()! {
    let trace_buf: [4]HandoffRecord = [HandoffRecord{}; 4];
    let trace_s: []HandoffRecord = trace_buf[0..];

    let router = cap.agent.HandoffRouter.init(trace_s, 10, 4, true) ?? return error.InitFailed;
    expect_eq(router.currentAgent(), 10);
    expect_eq(router.hopCount(), 0);
    expect_eq(router.remainingHops(), 4);
    expect_eq(router.hasVisited(10), true);
    expect_eq(router.hasVisited(20), false);

    expect_eq(router.handoff(20, "route to coder", 1000) == HandoffStatus.Accepted, true);
    expect_eq(router.handoff(30, "route to reviewer", 2000) == HandoffStatus.Accepted, true);
    expect_eq(router.handoff(40, "route to deployer", 3000) == HandoffStatus.Accepted, true);

    expect_eq(router.currentAgent(), 40);
    expect_eq(router.hopCount(), 3);
    expect_eq(router.remainingHops(), 1);
    expect_eq(router.hasVisited(30), true);

    const hist = router.history();
    expect_eq(hist.len, 3);
    expect_eq(hist[0].hop_index, 1);
    expect_eq(hist[0].from_agent, 10);
    expect_eq(hist[0].to_agent, 20);
    expect_eq(std.mem.eql(u8, hist[0].summary, "route to coder"), true);

    const rec2 = router.recordAt(2) ?? return error.MissingRecord;
    expect_eq(rec2.hop_index, 3);
    expect_eq(rec2.from_agent, 30);
    expect_eq(rec2.to_agent, 40);
    expect_eq(rec2.timestamp_ms, 3000);
    expect_eq(router.recordAt(3) == null, true);

    // Reset clears history and starts fresh chain.
    router.reset(99);
    expect_eq(router.currentAgent(), 99);
    expect_eq(router.hopCount(), 0);
    expect_eq(router.hasVisited(10), false);
    expect_eq(router.hasVisited(99), true);
}

// ============================================================================
// Test 2: HandoffRouter Cycle Guard, Self-Handoff, Max Hops & Capacity Limits
// ============================================================================

fn testHandoffCycleAndBudgetGuards()! {
    // Invalid init guards
    let dummy_buf: [2]HandoffRecord = [HandoffRecord{}; 2];
    let dummy_s: []HandoffRecord = dummy_buf[0..];
    let empty_s: []HandoffRecord = dummy_buf[0..0];
    expect_eq(HandoffRouter.init(dummy_s, 1, 0, true) == null, true);
    expect_eq(HandoffRouter.init(empty_s, 1, 3, true) == null, true);

    // Cycle protection (A -> B -> A rejected, self-handoff rejected)
    let trace_buf: [4]HandoffRecord = [HandoffRecord{}; 4];
    let trace_s: []HandoffRecord = trace_buf[0..];
    let router = HandoffRouter.init(trace_s, 1, 3, true) ?? return error.InitFailed;

    expect_eq(router.handoff(1, "self", 100) == HandoffStatus.SelfHandoff, true);
    expect_eq(router.handoff(2, "to B", 200) == HandoffStatus.Accepted, true);
    expect_eq(router.handoff(1, "back to A", 300) == HandoffStatus.CycleDetected, true);
    expect_eq(router.currentAgent(), 2);
    expect_eq(router.hopCount(), 1);

    expect_eq(router.handoff(3, "to C", 400) == HandoffStatus.Accepted, true);
    expect_eq(router.handoff(4, "to D", 500) == HandoffStatus.Accepted, true);
    // Max hops (3) reached
    expect_eq(router.handoff(5, "to E", 600) == HandoffStatus.MaxHopsExceeded, true);
    expect_eq(router.hopCount(), 3);
    expect_eq(router.remainingHops(), 0);

    // HistoryFull when max_hops > records.len, and forbid_cycles == false permits A -> B -> A
    let small_buf: [2]HandoffRecord = [HandoffRecord{}; 2];
    let small_s: []HandoffRecord = small_buf[0..];
    let cyclic = HandoffRouter.init(small_s, 1, 5, false) ?? return error.InitFailed;
    expect_eq(cyclic.handoff(2, "hop1", 10) == HandoffStatus.Accepted, true);
    expect_eq(cyclic.handoff(1, "return", 20) == HandoffStatus.Accepted, true);
    expect_eq(cyclic.handoff(3, "overflow", 30) == HandoffStatus.HistoryFull, true);
    expect_eq(cyclic.hopCount(), 2);
}

// ============================================================================
// Test 3: AgentBlackboard Optimistic CAS (`putIfVersion`) & Audit Trail
// ============================================================================

fn testBlackboardCasAndAudit()! {
    let slots_buf: [4]BlackboardEntry = [BlackboardEntry{}; 4];
    let slots_s: []BlackboardEntry = slots_buf[0..];
    let board = AgentBlackboard.init(slots_s);

    expect_eq(board.count(), 0);
    expect_eq(board.capacity(), 4);
    expect_eq(board.currentSeq(), 0);

    // Absent key with expected_version != 0 must fail with VersionConflict.
    expect_eq(
        board.putIfVersion("task.plan", "v1", 1, 101, 1000) == BlackboardWriteStatus.VersionConflict,
        true,
    );
    expect_eq(board.count(), 0);

    // Insert-if-absent with expected_version == 0 succeeds -> version 1.
    expect_eq(
        board.putIfVersion("task.plan", "draft", 0, 101, 1000) == BlackboardWriteStatus.Ok,
        true,
    );
    const e1 = board.get("task.plan") ?? return error.MissingEntry;
    expect_eq(e1.version, 1);
    expect_eq(e1.writer_agent_id, 101);
    expect_eq(std.mem.eql(u8, e1.value, "draft"), true);

    // Agent 102 updates with matching version 1 -> version 2.
    expect_eq(
        board.putIfVersion("task.plan", "reviewed", 1, 102, 2000) == BlackboardWriteStatus.Ok,
        true,
    );

    // Concurrent Agent 103 attempts stale CAS with version 1 -> VersionConflict, no mutation.
    expect_eq(
        board.putIfVersion("task.plan", "stale_overwrite", 1, 103, 2100) == BlackboardWriteStatus.VersionConflict,
        true,
    );
    const e2 = board.get("task.plan") ?? return error.MissingEntry;
    expect_eq(e2.version, 2);
    expect_eq(e2.writer_agent_id, 102);
    expect_eq(std.mem.eql(u8, e2.value, "reviewed"), true);

    // Unconditional put increments version to 3 and updates writer audit.
    expect_eq(board.put("task.plan", "final", 104, 3000) == BlackboardWriteStatus.Ok, true);
    const e3 = board.get("task.plan") ?? return error.MissingEntry;
    expect_eq(e3.version, 3);
    expect_eq(e3.writer_agent_id, 104);

    // Remove frees slot for reuse.
    expect_eq(board.remove("task.plan"), true);
    expect_eq(board.remove("task.plan"), false);
    expect_eq(board.count(), 0);
    expect_eq(board.get("task.plan") == null, true);
}

// ============================================================================
// Test 4: AgentBlackboard Prefix Filtering, Incremental Seq Sync & Boundaries
// ============================================================================

fn testBlackboardPrefixSeqAndBoundaries()! {
    let slots_buf: [3]BlackboardEntry = [BlackboardEntry{}; 3];
    let slots_s: []BlackboardEntry = slots_buf[0..];
    let board = AgentBlackboard.init(slots_s);

    // Empty key rejected
    expect_eq(board.put("", "bad", 1, 100) == BlackboardWriteStatus.InvalidKey, true);
    expect_eq(board.putIfVersion("", "bad", 0, 1, 100) == BlackboardWriteStatus.InvalidKey, true);

    expect_eq(board.put("rag.query", "hybrid", 1, 100) == BlackboardWriteStatus.Ok, true);
    const seq_after_first = board.currentSeq();
    expect_eq(seq_after_first, 1);

    expect_eq(board.put("rag.top1", "doc42", 2, 200) == BlackboardWriteStatus.Ok, true);
    expect_eq(board.put("sys.mode", "fast", 3, 300) == BlackboardWriteStatus.Ok, true);

    // Capacity full (3/3) rejects new key, but allows updating existing key.
    expect_eq(board.put("extra.key", "x", 4, 400) == BlackboardWriteStatus.Full, true);
    expect_eq(board.put("sys.mode", "turbo", 4, 450) == BlackboardWriteStatus.Ok, true);
    expect_eq(board.currentSeq(), 4);

    // Prefix filtering: "rag." should match 2 entries.
    let out_buf: [4]BlackboardEntry = [BlackboardEntry{}; 4];
    let out_s: []BlackboardEntry = out_buf[0..];
    const n_pref = board.listByPrefix("rag.", out_s);
    expect_eq(n_pref, 2);

    // Incremental delta sync: entries updated after seq_after_first (1) -> "rag.top1" (seq 2) and "sys.mode" (seq 4).
    const n_delta = board.listSinceSeq(seq_after_first, out_s);
    expect_eq(n_delta, 2);

    // Entries updated after seq 3 -> only "sys.mode" (seq 4).
    const n_latest = board.listSinceSeq(3, out_s);
    expect_eq(n_latest, 1);
    expect_eq(std.mem.eql(u8, out_buf[0].key, "sys.mode"), true);
    expect_eq(std.mem.eql(u8, out_buf[0].value, "turbo"), true);
}

// ============================================================================
// Test 5: FanOutAggregator Early Quorum Readiness & Score-Ranked Collection
// ============================================================================

fn testFanOutEarlyQuorumAndRanking()! {
    let slots_buf: [4]FanOutSlot = [FanOutSlot{}; 4];
    let slots_s: []FanOutSlot = slots_buf[0..];
    let agg = FanOutAggregator.init(slots_s, 2) ?? return error.InitFailed;

    expect_eq(agg.submit(101, 1), true);
    expect_eq(agg.submit(102, 2), true);
    expect_eq(agg.submit(103, 3), true);
    expect_eq(agg.submit(104, 4), true);
    // Duplicate task_id and full slots rejected
    expect_eq(agg.submit(101, 5), false);
    expect_eq(agg.submit(105, 5), false);

    expect_eq(agg.isQuorumReady(), false);
    expect_eq(agg.status() == FanOutStatus.InProgress, true);

    // First task fails, still achievable (3 pending, quorum = 2)
    expect_eq(agg.recordFailure(101, "timeout", 500), true);
    expect_eq(agg.status() == FanOutStatus.InProgress, true);
    // Double terminal transition rejected
    expect_eq(agg.recordSuccess(101, 0.9, "late", 600), false);

    // Two tasks succeed -> quorum = 2 reached early while task 104 is still Pending!
    expect_eq(agg.recordSuccess(102, 0.82, "draft-B", 700), true);
    expect_eq(agg.isQuorumReady(), false);
    expect_eq(agg.recordSuccess(103, 0.96, "draft-C", 800), true);
    expect_eq(agg.isQuorumReady(), true);
    expect_eq(agg.status() == FanOutStatus.QuorumReached, true);

    const sum = agg.summary();
    expect_eq(sum.submitted, 4);
    expect_eq(sum.pending, 1);
    expect_eq(sum.succeeded, 2);
    expect_eq(sum.failed, 1);
    expect_eq(sum.quorum, 2);
    expect_eq(sum.status == FanOutStatus.QuorumReached, true);

    // Collect succeeded results ranked descending by score: 103 (0.96), then 102 (0.82)
    let winners: [2]FanOutSlot = [FanOutSlot{}; 2];
    let winners_s: []FanOutSlot = winners[0..];
    const n_win = agg.collectSucceeded(winners_s);
    expect_eq(n_win, 2);
    expect_eq(winners[0].task_id, 103);
    expect_eq(winners[1].task_id, 102);

    // Third task succeeds with highest score 0.99; collecting into top-1 buffer keeps best (104)
    expect_eq(agg.recordSuccess(104, 0.99, "draft-D", 900), true);
    let top1: [1]FanOutSlot = [FanOutSlot{}; 1];
    let top1_s: []FanOutSlot = top1[0..];
    expect_eq(agg.collectSucceeded(top1_s), 1);
    expect_eq(top1[0].task_id, 104);
}

// ============================================================================
// Test 6: FanOutAggregator Quorum Failure & Defensive Guards
// ============================================================================

fn testFanOutQuorumFailureAndGuards()! {
    let slots_buf: [3]FanOutSlot = [FanOutSlot{}; 3];
    let slots_s: []FanOutSlot = slots_buf[0..];

    // Invalid init guards
    expect_eq(FanOutAggregator.init(slots_s, 0) == null, true);
    expect_eq(FanOutAggregator.init(slots_s, 4) == null, true);

    // Quorum = 2 out of 3 submitted tasks; 2 failures make quorum mathematically impossible.
    let agg = FanOutAggregator.init(slots_s, 2) ?? return error.InitFailed;
    expect_eq(agg.submit(1, 10), true);
    expect_eq(agg.submit(2, 20), true);
    expect_eq(agg.submit(3, 30), true);

    // Non-finite score & unknown task_id rejected
    const nan_f32: f32 = std.math.nan(f32);
    expect_eq(agg.recordSuccess(1, nan_f32, "bad", 100), false);
    expect_eq(agg.recordSuccess(999, 0.5, "unknown", 100), false);
    expect_eq(agg.recordFailure(999, "unknown", 100), false);

    expect_eq(agg.recordFailure(1, "err1", 100), true);
    expect_eq(agg.status() == FanOutStatus.InProgress, true);

    expect_eq(agg.recordFailure(2, "err2", 200), true);
    expect_eq(agg.isQuorumReady(), false);
    expect_eq(agg.status() == FanOutStatus.QuorumFailed, true);

    const sum = agg.summary();
    expect_eq(sum.failed, 2);
    expect_eq(sum.pending, 1);
    expect_eq(sum.status == FanOutStatus.QuorumFailed, true);
}

fn main() {
    test("1. agent.handoff: normal multi-hop trajectory & reset", () => {
        testHandoffNormalTrajectory()?;
    });
    test("2. agent.handoff: cycle guard, self-handoff, max_hops & capacity", () => {
        testHandoffCycleAndBudgetGuards()?;
    });
    test("3. agent.handoff: blackboard optimistic CAS & writer audit", () => {
        testBlackboardCasAndAudit()?;
    });
    test("4. agent.handoff: blackboard prefix filter, delta seq & boundaries", () => {
        testBlackboardPrefixSeqAndBoundaries()?;
    });
    test("5. agent.handoff: fan-out early quorum readiness & score ranking", () => {
        testFanOutEarlyQuorumAndRanking()?;
    });
    test("6. agent.handoff: fan-out quorum failure & defensive guards", () => {
        testFanOutQuorumFailureAndGuards()?;
    });
}
