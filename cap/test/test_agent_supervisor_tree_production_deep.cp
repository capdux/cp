// Origin: Pure-CP Stdlib Test Suite (lib/agent/supervisor_tree.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc OTP Agent Supervisor Tree:
//   1. Restart policy matrix (permanent / transient / temporary x all exit reasons)
//   2. one_for_one isolated single-child restart
//   3. one_for_all deterministic start_order group restart (skipping temporary/terminated)
//   4. rest_for_one cascading downstream restart preserving upstream children
//   5. Sliding-window restart intensity circuit breaker (escalate_shutdown) & window slide-out recovery
//   6. Capped exponential backoff progression & healthy-window consecutive crash reset
//   7. Boundary guards (invalid config, dup/zero ID, unknown child, full 32 slots, post-escalation)

import { test, expect_eq } from "cap:test";
import {
    SUPERVISOR_MAX_CHILDREN,
    SUPERVISOR_MAX_RESTART_HISTORY,
    AgentSupervisorStrategy,
    AgentRestartPolicy,
    AgentExitReason,
    AgentChildState,
    SupervisorActionKind,
    AgentSupervisorConfig,
    SupervisedChildSlot,
    SupervisorDecision,
    SupervisorStats,
    AgentSupervisorTree,
} from "cap:agent";

// ============================================================================
// Test 1: Restart Policy Matrix (permanent / transient / temporary)
// ============================================================================
test("supervisor_tree: permanent, transient and temporary restart policy matrix", () => {
    const cfg = AgentSupervisorConfig{
        strategy: AgentSupervisorStrategy.one_for_one,
        max_restarts: 10,
        intensity_window_ms: 10_000,
        base_backoff_ms: 50,
        max_backoff_ms: 1000,
    };
    const sup_opt = AgentSupervisorTree.init(cfg);
    expect_eq(sup_opt != null, true);
    let sup = sup_opt ?? return;

    expect_eq(sup.register_child(1, AgentRestartPolicy.permanent, 1), true);
    expect_eq(sup.register_child(2, AgentRestartPolicy.transient, 2), true);
    expect_eq(sup.register_child(3, AgentRestartPolicy.transient, 3), true);
    expect_eq(sup.register_child(4, AgentRestartPolicy.temporary, 4), true);

    expect_eq(sup.mark_started(1, 100), true);
    expect_eq(sup.mark_started(2, 100), true);
    expect_eq(sup.mark_started(3, 100), true);
    expect_eq(sup.mark_started(4, 100), true);

    // permanent restarts even on normal exit
    const d_perm = sup.report_exit(1, AgentExitReason.normal, 200);
    expect_eq(d_perm.action == SupervisorActionKind.restart_children, true);
    expect_eq(d_perm.restart_count, 1);
    expect_eq(d_perm.restart_child_ids[0], 1);

    // transient does NOT restart on normal or shutdown
    const d_tr_ok = sup.report_exit(2, AgentExitReason.normal, 210);
    expect_eq(d_tr_ok.action == SupervisorActionKind.none, true);
    const c2 = sup.get_child(2) ?? return;
    expect_eq(c2.state == AgentChildState.terminated, true);

    // transient DOES restart on crash_error / timeout / budget_exceeded
    const d_tr_err = sup.report_exit(3, AgentExitReason.budget_exceeded, 220);
    expect_eq(d_tr_err.action == SupervisorActionKind.restart_children, true);
    expect_eq(d_tr_err.restart_child_ids[0], 3);

    // temporary NEVER restarts even on crash_error
    const d_temp = sup.report_exit(4, AgentExitReason.crash_error, 230);
    expect_eq(d_temp.action == SupervisorActionKind.none, true);
    const c4 = sup.get_child(4) ?? return;
    expect_eq(c4.state == AgentChildState.terminated, true);
});

// ============================================================================
// Test 2: one_for_one Isolated Restart
// ============================================================================
test("supervisor_tree: one_for_one restarts only the failing child", () => {
    const cfg = AgentSupervisorConfig{
        strategy: AgentSupervisorStrategy.one_for_one,
        max_restarts: 5,
        intensity_window_ms: 5000,
        base_backoff_ms: 100,
        max_backoff_ms: 2000,
    };
    let sup = AgentSupervisorTree.init(cfg) ?? return;

    expect_eq(sup.register_child(10, AgentRestartPolicy.permanent, 1), true);
    expect_eq(sup.register_child(20, AgentRestartPolicy.permanent, 2), true);
    expect_eq(sup.register_child(30, AgentRestartPolicy.permanent, 3), true);
    expect_eq(sup.mark_started(10, 1000), true);
    expect_eq(sup.mark_started(20, 1000), true);
    expect_eq(sup.mark_started(30, 1000), true);

    const decision = sup.report_exit(20, AgentExitReason.timeout, 1200);
    expect_eq(decision.action == SupervisorActionKind.restart_children, true);
    expect_eq(decision.restart_count, 1);
    expect_eq(decision.restart_child_ids[0], 20);
    expect_eq(decision.backoff_ms, 100);

    const st = sup.stats();
    expect_eq(st.running_children, 2);
    expect_eq(st.backoff_children, 1);
    expect_eq(st.total_restarts, 1);
});

// ============================================================================
// Test 3: one_for_all Deterministic Ordered Group Restart
// ============================================================================
test("supervisor_tree: one_for_all restarts all non-temporary active children in start_order", () => {
    const cfg = AgentSupervisorConfig{
        strategy: AgentSupervisorStrategy.one_for_all,
        max_restarts: 5,
        intensity_window_ms: 5000,
        base_backoff_ms: 100,
        max_backoff_ms: 1000,
    };
    let sup = AgentSupervisorTree.init(cfg) ?? return;

    // Register out of start_order
    expect_eq(sup.register_child(300, AgentRestartPolicy.permanent, 30), true);
    expect_eq(sup.register_child(100, AgentRestartPolicy.permanent, 10), true);
    expect_eq(sup.register_child(400, AgentRestartPolicy.temporary, 25), true); // excluded from sibling restart
    expect_eq(sup.register_child(200, AgentRestartPolicy.transient, 20), true);

    expect_eq(sup.mark_started(100, 1000), true);
    expect_eq(sup.mark_started(200, 1000), true);
    expect_eq(sup.mark_started(300, 1000), true);
    expect_eq(sup.mark_started(400, 1000), true);

    const decision = sup.report_exit(200, AgentExitReason.crash_error, 1500);
    expect_eq(decision.action == SupervisorActionKind.restart_children, true);
    expect_eq(decision.restart_count, 3);
    // Strictly ordered by start_order ascending: 100 (10), 200 (20), 300 (30)
    expect_eq(decision.restart_child_ids[0], 100);
    expect_eq(decision.restart_child_ids[1], 200);
    expect_eq(decision.restart_child_ids[2], 300);

    // Temporary child 400 remains running untouched
    const c400 = sup.get_child(400) ?? return;
    expect_eq(c400.state == AgentChildState.running, true);
});

// ============================================================================
// Test 4: rest_for_one Cascading Downstream Restart
// ============================================================================
test("supervisor_tree: rest_for_one cascades only to target and higher start_order siblings", () => {
    const cfg = AgentSupervisorConfig{
        strategy: AgentSupervisorStrategy.rest_for_one,
        max_restarts: 5,
        intensity_window_ms: 5000,
        base_backoff_ms: 50,
        max_backoff_ms: 1000,
    };
    let sup = AgentSupervisorTree.init(cfg) ?? return;

    expect_eq(sup.register_child(1, AgentRestartPolicy.permanent, 1), true);
    expect_eq(sup.register_child(4, AgentRestartPolicy.permanent, 4), true);
    expect_eq(sup.register_child(2, AgentRestartPolicy.permanent, 2), true);
    expect_eq(sup.register_child(3, AgentRestartPolicy.transient, 3), true);

    expect_eq(sup.mark_started(1, 500), true);
    expect_eq(sup.mark_started(2, 500), true);
    expect_eq(sup.mark_started(3, 500), true);
    expect_eq(sup.mark_started(4, 500), true);

    // Child 2 (order 2) fails -> restarts [2, 3, 4], child 1 (order 1) stays running
    const decision = sup.report_exit(2, AgentExitReason.crash_error, 700);
    expect_eq(decision.action == SupervisorActionKind.restart_children, true);
    expect_eq(decision.restart_count, 3);
    expect_eq(decision.restart_child_ids[0], 2);
    expect_eq(decision.restart_child_ids[1], 3);
    expect_eq(decision.restart_child_ids[2], 4);

    const c1 = sup.get_child(1) ?? return;
    expect_eq(c1.state == AgentChildState.running, true);
    expect_eq(sup.stats().running_children, 1);
    expect_eq(sup.stats().backoff_children, 3);
});

// ============================================================================
// Test 5: Sliding-Window Restart Intensity Breaker & Window Slide-Out Recovery
// ============================================================================
test("supervisor_tree: sliding-window intensity breach escalates & expired history recovers", () => {
    // Part A: 4 crashes within 5000ms window with max_restarts = 3 -> escalate_shutdown
    const cfg = AgentSupervisorConfig{
        strategy: AgentSupervisorStrategy.one_for_one,
        max_restarts: 3,
        intensity_window_ms: 5000,
        base_backoff_ms: 50,
        max_backoff_ms: 500,
    };
    let sup = AgentSupervisorTree.init(cfg) ?? return;
    expect_eq(sup.register_child(1, AgentRestartPolicy.permanent, 1), true);
    expect_eq(sup.register_child(2, AgentRestartPolicy.permanent, 2), true);
    expect_eq(sup.mark_started(1, 1000), true);
    expect_eq(sup.mark_started(2, 1000), true);

    expect_eq(sup.report_exit(1, AgentExitReason.crash_error, 1100).action == SupervisorActionKind.restart_children, true);
    expect_eq(sup.mark_started(1, 1200), true);
    expect_eq(sup.report_exit(1, AgentExitReason.crash_error, 2000).action == SupervisorActionKind.restart_children, true);
    expect_eq(sup.mark_started(1, 2100), true);
    expect_eq(sup.report_exit(1, AgentExitReason.crash_error, 3000).action == SupervisorActionKind.restart_children, true);
    expect_eq(sup.mark_started(1, 3100), true);

    // 4th crash at t=4000 is within [0, 4000] (window 5000ms) -> intensity exceeded!
    const esc = sup.report_exit(1, AgentExitReason.crash_error, 4000);
    expect_eq(esc.action == SupervisorActionKind.escalate_shutdown, true);
    expect_eq(esc.intensity_exceeded, true);
    expect_eq(sup.stats().escalated, true);
    expect_eq(sup.stats().terminated_children, 2);

    // Part B: Window slide-out recovery when 4th crash occurs after earliest expires
    let sup2 = AgentSupervisorTree.init(cfg) ?? return;
    expect_eq(sup2.register_child(1, AgentRestartPolicy.permanent, 1), true);
    expect_eq(sup2.mark_started(1, 1000), true);
    expect_eq(sup2.report_exit(1, AgentExitReason.crash_error, 1000).action == SupervisorActionKind.restart_children, true);
    expect_eq(sup2.mark_started(1, 1100), true);
    expect_eq(sup2.report_exit(1, AgentExitReason.crash_error, 2000).action == SupervisorActionKind.restart_children, true);
    expect_eq(sup2.mark_started(1, 2100), true);
    expect_eq(sup2.report_exit(1, AgentExitReason.crash_error, 3000).action == SupervisorActionKind.restart_children, true);
    expect_eq(sup2.mark_started(1, 3100), true);

    // At t=7000, window_start = 2000 -> crash at t=1000 slid out, only 2 remain -> 3rd allowed!
    const ok4 = sup2.report_exit(1, AgentExitReason.crash_error, 7000);
    expect_eq(ok4.action == SupervisorActionKind.restart_children, true);
    expect_eq(ok4.intensity_exceeded, false);
    expect_eq(sup2.stats().escalated, false);
});

// ============================================================================
// Test 6: Capped Exponential Backoff & Healthy-Window Reset
// ============================================================================
test("supervisor_tree: exponential backoff progression, cap clamping & healthy reset", () => {
    const cfg = AgentSupervisorConfig{
        strategy: AgentSupervisorStrategy.one_for_one,
        max_restarts: 20,
        intensity_window_ms: 5000,
        base_backoff_ms: 100,
        max_backoff_ms: 500,
    };
    let sup = AgentSupervisorTree.init(cfg) ?? return;
    expect_eq(sup.register_child(7, AgentRestartPolicy.permanent, 1), true);
    expect_eq(sup.mark_started(7, 1000), true);

    // Crash 1 -> 100ms
    const d1 = sup.report_exit(7, AgentExitReason.crash_error, 1100);
    expect_eq(d1.backoff_ms, 100);
    expect_eq(sup.mark_started(7, 1200), true);

    // Crash 2 -> 200ms
    const d2 = sup.report_exit(7, AgentExitReason.crash_error, 1300);
    expect_eq(d2.backoff_ms, 200);
    expect_eq(sup.mark_started(7, 1500), true);

    // Crash 3 -> 400ms
    const d3 = sup.report_exit(7, AgentExitReason.crash_error, 1600);
    expect_eq(d3.backoff_ms, 400);
    expect_eq(sup.mark_started(7, 2000), true);

    // Crash 4 -> 800ms clamped to max_backoff_ms = 500ms
    const d4 = sup.report_exit(7, AgentExitReason.crash_error, 2100);
    expect_eq(d4.backoff_ms, 500);
    expect_eq(sup.mark_started(7, 2600), true);

    // Runs healthy for >= 5000ms (2600 -> 9000) -> consecutive_crashes resets -> next backoff = 100ms
    const d5 = sup.report_exit(7, AgentExitReason.crash_error, 9000);
    expect_eq(d5.backoff_ms, 100);
});

// ============================================================================
// Test 7: Boundary Guards (Invalid Config, Dup/Zero ID, Unknown Child, Full 32 Slots)
// ============================================================================
test("supervisor_tree: boundary guards for invalid config, dup ID & full 32 slots", () => {
    expect_eq(SUPERVISOR_MAX_CHILDREN, 32);
    expect_eq(SUPERVISOR_MAX_RESTART_HISTORY, 64);

    // Invalid configs
    const bad1 = AgentSupervisorConfig{ intensity_window_ms: 0, base_backoff_ms: 100, max_backoff_ms: 1000 };
    expect_eq(bad1.isValid(), false);
    expect_eq(AgentSupervisorTree.init(bad1) == null, true);

    const bad2 = AgentSupervisorConfig{ intensity_window_ms: 1000, base_backoff_ms: 500, max_backoff_ms: 100 };
    expect_eq(bad2.isValid(), false);
    expect_eq(AgentSupervisorTree.init(bad2) == null, true);

    let sup = AgentSupervisorTree.init(AgentSupervisorConfig{}) ?? return;
    // child_id == 0 rejected
    expect_eq(sup.register_child(0, AgentRestartPolicy.permanent, 1), false);
    // Unknown child operations fail safely
    expect_eq(sup.mark_started(999, 100), false);
    expect_eq(sup.report_exit(999, AgentExitReason.crash_error, 100).action == SupervisorActionKind.none, true);

    // Fill all 32 slots
    let id: u64 = 1;
    while (id <= 32) : (id += 1) {
        expect_eq(sup.register_child(id, AgentRestartPolicy.permanent, id as u16), true);
    }
    // Duplicate ID & 33rd child rejected
    expect_eq(sup.register_child(1, AgentRestartPolicy.permanent, 1), false);
    expect_eq(sup.register_child(33, AgentRestartPolicy.permanent, 33), false);
    expect_eq(sup.stats().total_children, 32);
});
