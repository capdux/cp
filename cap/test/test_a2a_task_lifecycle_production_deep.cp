// Origin: Pure-CP Stdlib Test Suite (lib/a2a/task_lifecycle.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc A2A Task Lifecycle State Machine & Artifact Router:
//   1. Full legal lifecycle traversal including auth_required & input_required suspend/resume
//   2. Terminal immutability (rejected_terminal) & illegal transition edge guards (invalid_edge)
//   3. Cooperative request_cancel on active phases & rejection on terminal/unknown tasks
//   4. Monotonic streaming artifact seq validation, byte accumulation & is_last_chunk sealing
//   5. Expired terminal slot reclamation (prune_terminal_before) & slot reuse
//   6. Comprehensive boundary guards (dup ID, full 64 slots, non-working artifact write, bad IDs)
//   7. Coexistence check with existing cap:a2a symbols (task_queue.cp::A2aTaskState)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    A2aTaskState,
    A2A_LIFECYCLE_MAX_TASKS,
    A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK,
    A2A_ARTIFACT_ID_MAX_LEN,
    A2A_ARTIFACT_MIME_MAX_LEN,
    A2aTaskPhase,
    A2aArtifactMeta,
    A2aTaskRecord,
    A2aTransitionOutcome,
    A2aLifecycleStats,
    A2aTaskLifecycleManager,
} from "cap:a2a";

// ============================================================================
// Test 1: Full Legal Lifecycle Traversal (auth_required & input_required)
// ============================================================================
test("a2a_lifecycle: full legal lifecycle with auth_required & input_required suspend/resume", () => {
    let mgr = A2aTaskLifecycleManager.init();
    expect_eq(mgr.submit_task(1001, 55, 1000), true);

    const t0_opt = mgr.get_task(1001);
    expect_eq(t0_opt != null, true);
    const t0 = t0_opt ?? return;
    expect_eq(t0.phase == A2aTaskPhase.submitted, true);
    expect_eq(t0.context_id, 55);
    expect_eq(t0.created_ms, 1000);
    expect_eq(t0.transition_count, 0);

    // submitted -> auth_required -> working -> input_required -> working -> completed
    const o1 = mgr.transition(1001, A2aTaskPhase.auth_required, 1010);
    expect_eq(o1.ok, true);
    expect_eq(o1.prev_phase == A2aTaskPhase.submitted, true);
    expect_eq(o1.next_phase == A2aTaskPhase.auth_required, true);

    const o2 = mgr.transition(1001, A2aTaskPhase.working, 1020);
    expect_eq(o2.ok, true);

    const o3 = mgr.transition(1001, A2aTaskPhase.input_required, 1030);
    expect_eq(o3.ok, true);

    const o4 = mgr.transition(1001, A2aTaskPhase.working, 1040);
    expect_eq(o4.ok, true);

    const o5 = mgr.transition(1001, A2aTaskPhase.completed, 1050);
    expect_eq(o5.ok, true);
    expect_eq(o5.next_phase == A2aTaskPhase.completed, true);

    const tf_opt = mgr.get_task(1001);
    expect_eq(tf_opt != null, true);
    const tf = tf_opt ?? return;
    expect_eq(tf.phase == A2aTaskPhase.completed, true);
    expect_eq(tf.updated_ms, 1050);
    expect_eq(tf.transition_count, 5);

    const st = mgr.stats();
    expect_eq(st.total_tasks, 1);
    expect_eq(st.active_tasks, 0);
    expect_eq(st.completed_tasks, 1);
});

// ============================================================================
// Test 2: Terminal Immutability & Illegal Edge Guards
// ============================================================================
test("a2a_lifecycle: terminal immutability and disallowed transition edges", () => {
    expect_eq(A2aTaskPhase.submitted.isTerminal(), false);
    expect_eq(A2aTaskPhase.working.isTerminal(), false);
    expect_eq(A2aTaskPhase.input_required.isTerminal(), false);
    expect_eq(A2aTaskPhase.auth_required.isTerminal(), false);
    expect_eq(A2aTaskPhase.completed.isTerminal(), true);
    expect_eq(A2aTaskPhase.failed.isTerminal(), true);
    expect_eq(A2aTaskPhase.canceled.isTerminal(), true);
    expect_eq(A2aTaskPhase.rejected.isTerminal(), true);

    let mgr = A2aTaskLifecycleManager.init();
    expect_eq(mgr.submit_task(10, 1, 100), true);

    // Illegal: submitted -> completed (skipping working)
    const bad1 = mgr.transition(10, A2aTaskPhase.completed, 110);
    expect_eq(bad1.ok, false);
    expect_eq(bad1.invalid_edge, true);
    expect_eq(bad1.rejected_terminal, false);

    // Illegal: self-loop submitted -> submitted
    const bad2 = mgr.transition(10, A2aTaskPhase.submitted, 115);
    expect_eq(bad2.ok, false);
    expect_eq(bad2.invalid_edge, true);

    // Move to working, then try illegal backward working -> submitted
    expect_eq(mgr.transition(10, A2aTaskPhase.working, 120).ok, true);
    const bad3 = mgr.transition(10, A2aTaskPhase.submitted, 125);
    expect_eq(bad3.ok, false);
    expect_eq(bad3.invalid_edge, true);

    // Move to input_required, illegal input_required -> completed
    expect_eq(mgr.transition(10, A2aTaskPhase.input_required, 130).ok, true);
    const bad4 = mgr.transition(10, A2aTaskPhase.completed, 135);
    expect_eq(bad4.ok, false);
    expect_eq(bad4.invalid_edge, true);

    // Complete cleanly via working
    expect_eq(mgr.transition(10, A2aTaskPhase.working, 140).ok, true);
    expect_eq(mgr.transition(10, A2aTaskPhase.completed, 150).ok, true);

    // Terminal immutability: completed -> working / failed / canceled all rejected_terminal
    const term1 = mgr.transition(10, A2aTaskPhase.working, 160);
    expect_eq(term1.ok, false);
    expect_eq(term1.rejected_terminal, true);
    expect_eq(term1.invalid_edge, false);

    const term2 = mgr.transition(10, A2aTaskPhase.failed, 170);
    expect_eq(term2.ok, false);
    expect_eq(term2.rejected_terminal, true);

    // Rejected terminal path from submitted
    expect_eq(mgr.submit_task(20, 1, 200), true);
    expect_eq(mgr.transition(20, A2aTaskPhase.rejected, 210).ok, true);
    expect_eq(mgr.transition(20, A2aTaskPhase.working, 220).rejected_terminal, true);
    expect_eq(mgr.stats().rejected_tasks, 1);
});

// ============================================================================
// Test 3: Cooperative request_cancel & Terminal Cancel Rejection
// ============================================================================
test("a2a_lifecycle: request_cancel on active tasks & rejection on terminal tasks", () => {
    let mgr = A2aTaskLifecycleManager.init();
    expect_eq(mgr.submit_task(1, 10, 100), true);
    expect_eq(mgr.submit_task(2, 10, 100), true);
    expect_eq(mgr.submit_task(3, 10, 100), true);

    // Task 1: cancel directly from submitted
    expect_eq(mgr.request_cancel(1, 150), true);
    const t1 = mgr.get_task(1) ?? return;
    expect_eq(t1.phase == A2aTaskPhase.canceled, true);
    expect_eq(t1.cancel_requested, true);
    // Re-canceling an already canceled task returns false
    expect_eq(mgr.request_cancel(1, 160), false);

    // Task 2: cancel from working
    expect_eq(mgr.transition(2, A2aTaskPhase.working, 120).ok, true);
    expect_eq(mgr.request_cancel(2, 180), true);
    const t2 = mgr.get_task(2) ?? return;
    expect_eq(t2.phase == A2aTaskPhase.canceled, true);
    expect_eq(t2.cancel_requested, true);

    // Task 3: complete first, then cancel must be rejected
    expect_eq(mgr.transition(3, A2aTaskPhase.working, 120).ok, true);
    expect_eq(mgr.transition(3, A2aTaskPhase.completed, 140).ok, true);
    expect_eq(mgr.request_cancel(3, 200), false);

    // Unknown task cancel returns false
    expect_eq(mgr.request_cancel(9999, 200), false);

    const st = mgr.stats();
    expect_eq(st.canceled_tasks, 2);
    expect_eq(st.completed_tasks, 1);
    expect_eq(st.active_tasks, 0);
});

// ============================================================================
// Test 4: Streaming Artifact Chunk Monotonic seq & Finalized Sealing
// ============================================================================
test("a2a_lifecycle: streaming artifact monotonic seq, byte totals & finalized sealing", () => {
    let mgr = A2aTaskLifecycleManager.init();
    expect_eq(mgr.submit_task(700, 1, 1000), true);
    expect_eq(mgr.transition(700, A2aTaskPhase.working, 1010).ok, true);

    // Chunk 1 (seq 1, 256B, not last)
    expect_eq(mgr.append_artifact_chunk(700, "code.patch", "text/x-diff", 1, 256, false), true);
    // Out-of-order / duplicate seq rejected
    expect_eq(mgr.append_artifact_chunk(700, "code.patch", "text/x-diff", 1, 128, false), false);
    expect_eq(mgr.append_artifact_chunk(700, "code.patch", "text/x-diff", 0, 128, false), false);

    // Suspend to input_required: artifact streaming still permitted in input_required
    expect_eq(mgr.transition(700, A2aTaskPhase.input_required, 1020).ok, true);
    expect_eq(mgr.append_artifact_chunk(700, "code.patch", "text/x-diff", 2, 512, false), true);

    // Final chunk seals artifact
    expect_eq(mgr.append_artifact_chunk(700, "code.patch", "text/x-diff", 5, 128, true), true);
    // Further chunks after finalized == true rejected even with higher seq
    expect_eq(mgr.append_artifact_chunk(700, "code.patch", "text/x-diff", 6, 64, false), false);

    const art = mgr.get_artifact(700, "code.patch") ?? return;
    expect_eq(art.chunks_received, 3);
    expect_eq(art.total_bytes, 896);
    expect_eq(art.last_seq, 5);
    expect_eq(art.finalized, true);
    expect_eq(std.mem.eql(u8, art.mime_type[0..art.mime_type_len], "text/x-diff"), true);

    // Up to A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK (4) distinct artifacts per task
    expect_eq(mgr.append_artifact_chunk(700, "art2", "text/plain", 1, 10, true), true);
    expect_eq(mgr.append_artifact_chunk(700, "art3", "text/plain", 1, 10, true), true);
    expect_eq(mgr.append_artifact_chunk(700, "art4", "text/plain", 1, 10, true), true);
    // 5th distinct artifact exceeds per-task cap -> rejected
    expect_eq(mgr.append_artifact_chunk(700, "art5", "text/plain", 1, 10, true), false);
    expect_eq(mgr.stats().total_artifacts, 4);
});

// ============================================================================
// Test 5: prune_terminal_before Slot Reclamation & Reuse
// ============================================================================
test("a2a_lifecycle: prune_terminal_before reclaims expired terminal slots only", () => {
    let mgr = A2aTaskLifecycleManager.init();
    expect_eq(mgr.submit_task(1, 1, 100), true);
    expect_eq(mgr.submit_task(2, 1, 100), true);
    expect_eq(mgr.submit_task(3, 1, 100), true);

    // Task 1: completed at t=500 with 1 artifact (expired for cutoff=1000)
    expect_eq(mgr.transition(1, A2aTaskPhase.working, 200).ok, true);
    expect_eq(mgr.append_artifact_chunk(1, "out.json", "application/json", 1, 100, true), true);
    expect_eq(mgr.transition(1, A2aTaskPhase.completed, 500).ok, true);

    // Task 2: failed at t=1200 (newer than cutoff=1000 -> must NOT be pruned)
    expect_eq(mgr.transition(2, A2aTaskPhase.working, 200).ok, true);
    expect_eq(mgr.transition(2, A2aTaskPhase.failed, 1200).ok, true);

    // Task 3: still active working at t=300 (non-terminal -> must NOT be pruned)
    expect_eq(mgr.transition(3, A2aTaskPhase.working, 300).ok, true);

    const pruned = mgr.prune_terminal_before(1000);
    expect_eq(pruned, 1);
    expect_eq(mgr.get_task(1) == null, true);
    expect_eq(mgr.get_task(2) != null, true);
    expect_eq(mgr.get_task(3) != null, true);

    const st = mgr.stats();
    expect_eq(st.total_tasks, 2);
    expect_eq(st.active_tasks, 1);
    expect_eq(st.failed_tasks, 1);
    expect_eq(st.total_artifacts, 0);

    // Reusing pruned task_id 1 succeeds cleanly
    expect_eq(mgr.submit_task(1, 2, 1500), true);
    expect_eq(mgr.stats().total_tasks, 3);
});

// ============================================================================
// Test 6: Boundary Guards (Dup ID, Full 64 Slots, Non-Working Artifact Write, Bad IDs)
// ============================================================================
test("a2a_lifecycle: boundary guards for dup ID, full 64 slots & invalid artifact writes", () => {
    let mgr = A2aTaskLifecycleManager.init();
    expect_eq(A2A_LIFECYCLE_MAX_TASKS, 64);
    expect_eq(A2A_LIFECYCLE_MAX_ARTIFACTS_PER_TASK, 4);

    // task_id == 0 rejected
    expect_eq(mgr.submit_task(0, 1, 100), false);

    // Artifact write rejected in submitted, auth_required, completed, canceled
    expect_eq(mgr.submit_task(1, 1, 100), true);
    expect_eq(mgr.append_artifact_chunk(1, "a", "text/plain", 1, 10, true), false); // submitted
    expect_eq(mgr.transition(1, A2aTaskPhase.auth_required, 110).ok, true);
    expect_eq(mgr.append_artifact_chunk(1, "a", "text/plain", 1, 10, true), false); // auth_required
    expect_eq(mgr.transition(1, A2aTaskPhase.working, 120).ok, true);

    // Empty / oversized artifact_id or mime_type rejected
    let too_long: [65]u8 = [97; 65];
    expect_eq(mgr.append_artifact_chunk(1, "", "text/plain", 1, 10, true), false);
    expect_eq(mgr.append_artifact_chunk(1, "a", "", 1, 10, true), false);
    expect_eq(mgr.append_artifact_chunk(1, too_long[0..], "text/plain", 1, 10, true), false);
    expect_eq(mgr.append_artifact_chunk(1, "a", too_long[0..], 1, 10, true), false);
    expect_eq(A2A_ARTIFACT_ID_MAX_LEN, 64);
    expect_eq(A2A_ARTIFACT_MIME_MAX_LEN, 64);

    // Complete task 1 -> artifact write rejected in terminal state
    expect_eq(mgr.transition(1, A2aTaskPhase.completed, 130).ok, true);
    expect_eq(mgr.append_artifact_chunk(1, "a", "text/plain", 1, 10, true), false);

    // Duplicate task_id 1 rejected while still occupied
    expect_eq(mgr.submit_task(1, 1, 140), false);

    // Fill remaining 63 slots (total 64)
    let id: u64 = 2;
    while (id <= 64) : (id += 1) {
        expect_eq(mgr.submit_task(id, 1, 200), true);
    }
    expect_eq(mgr.stats().total_tasks, 64);
    // 65th task rejected cleanly
    expect_eq(mgr.submit_task(65, 1, 210), false);

    // Prune completed task 1 (updated_ms = 130 < 150), then 65th succeeds
    expect_eq(mgr.prune_terminal_before(150), 1);
    expect_eq(mgr.submit_task(65, 1, 220), true);
    expect_eq(mgr.stats().total_tasks, 64);
});

// ============================================================================
// Test 7: Coexistence with Existing cap:a2a Barrel Symbols
// ============================================================================
test("a2a_lifecycle: zero symbol conflict with existing task_queue.cp A2aTaskState", () => {
    const legacy_state = A2aTaskState.Working;
    const lifecycle_phase = A2aTaskPhase.working;
    expect_eq(legacy_state == A2aTaskState.Working, true);
    expect_eq(lifecycle_phase == A2aTaskPhase.working, true);
});
