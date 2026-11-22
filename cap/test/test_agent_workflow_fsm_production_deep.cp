// Origin: Pure-CP Stdlib Test Suite (AI Agent Deterministic Workflow FSM & Transition Audit Log) | Created: 2026-11-22
// Deep Production Verification Suite for lib/agent/workflow_fsm.cp
// Covers:
//   1. Multi-branch priority routing to terminal state & pre-start/post-terminal InvalidState guards
//   2. Per-node max_visits cycle guard circuit breaking & fallback routing
//   3. TransitionRecord audit log fidelity & rollbackLastStep restoration (including visit_count)
//   4. Static validateReachability (valid graph, unreachable nodes/terminals, dangling edges, missing start)
//   5. HumanGate pause detection, rejected event blocking & human approval/denial resumption

import { test, expect_eq } from "cap:test";
import {
    WorkflowNodeKind,
    WorkflowNode,
    WorkflowEdge,
    TransitionRecord,
    WorkflowStepResult,
    ReachabilityReport,
    AgentWorkflowFsm,
} from "../../lib/agent/index.cp";

const EV_BEGIN: u16 = 1;
const EV_NEED_TOOL: u16 = 2;
const EV_TOOL_DONE: u16 = 3;
const EV_FINISH: u16 = 4;
const EV_ABORT: u16 = 5;
const EV_APPROVED: u16 = 6;
const EV_REJECTED: u16 = 7;

fn testMultiBranchPriorityAndTerminal()! {
    let nodes_buf: [5]WorkflowNode = [
        WorkflowNode{ id: 1, kind: WorkflowNodeKind.Start, max_visits: 1 },
        WorkflowNode{ id: 2, kind: WorkflowNodeKind.LlmStep, max_visits: 5 },
        WorkflowNode{ id: 3, kind: WorkflowNodeKind.ToolStep, max_visits: 5 },
        WorkflowNode{ id: 4, kind: WorkflowNodeKind.TerminalOk, max_visits: 1 },
        WorkflowNode{ id: 5, kind: WorkflowNodeKind.TerminalFail, max_visits: 1 },
    ];
    // Duplicate (2, EV_FINISH) edges with different priorities -> priority 10 (to 4) must win over priority 1 (to 5)
    const edges: [5]WorkflowEdge = [
        WorkflowEdge{ from_id: 1, to_id: 2, event_code: EV_BEGIN, priority: 0 },
        WorkflowEdge{ from_id: 2, to_id: 3, event_code: EV_NEED_TOOL, priority: 0 },
        WorkflowEdge{ from_id: 3, to_id: 2, event_code: EV_TOOL_DONE, priority: 0 },
        WorkflowEdge{ from_id: 2, to_id: 5, event_code: EV_FINISH, priority: 1 },
        WorkflowEdge{ from_id: 2, to_id: 4, event_code: EV_FINISH, priority: 10 },
    ];
    let hist_buf: [16]TransitionRecord = [TransitionRecord{}; 16];
    let nodes: []WorkflowNode = nodes_buf[0..];
    let hist: []TransitionRecord = hist_buf[0..];
    let fsm = AgentWorkflowFsm.init(nodes, edges[0..], hist);

    // Pre-start guards
    expect_eq(fsm.currentNodeId() == null, true);
    expect_eq(fsm.trigger(EV_BEGIN, 100) == WorkflowStepResult.InvalidState, true);

    expect_eq(fsm.start(), true);
    expect_eq(fsm.currentNodeId(), 1);
    expect_eq(fsm.isTerminal(), false);

    // Unmatched event returns NoMatchingEdge without moving
    expect_eq(fsm.trigger(EV_FINISH, 105) == WorkflowStepResult.NoMatchingEdge, true);
    expect_eq(fsm.currentNodeId(), 1);

    // Start(1) -> LlmStep(2) -> ToolStep(3) -> LlmStep(2) -> TerminalOk(4)
    expect_eq(fsm.trigger(EV_BEGIN, 110) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.trigger(EV_NEED_TOOL, 120) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.trigger(EV_TOOL_DONE, 130) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.trigger(EV_FINISH, 140) == WorkflowStepResult.TerminalReached, true);

    expect_eq(fsm.currentNodeId(), 4);
    expect_eq(fsm.isTerminal(), true);
    expect_eq(fsm.history().len, 4);

    // Post-terminal trigger rejected
    expect_eq(fsm.trigger(EV_BEGIN, 150) == WorkflowStepResult.InvalidState, true);
}

fn testCycleGuardVisitLimitExceeded()! {
    let nodes_buf: [4]WorkflowNode = [
        WorkflowNode{ id: 1, kind: WorkflowNodeKind.Start, max_visits: 1 },
        WorkflowNode{ id: 2, kind: WorkflowNodeKind.LlmStep, max_visits: 0 }, // unlimited
        WorkflowNode{ id: 3, kind: WorkflowNodeKind.ToolStep, max_visits: 2 }, // max 2 visits
        WorkflowNode{ id: 4, kind: WorkflowNodeKind.TerminalFail, max_visits: 1 },
    ];
    const edges: [4]WorkflowEdge = [
        WorkflowEdge{ from_id: 1, to_id: 2, event_code: EV_BEGIN, priority: 0 },
        WorkflowEdge{ from_id: 2, to_id: 3, event_code: EV_NEED_TOOL, priority: 0 },
        WorkflowEdge{ from_id: 3, to_id: 2, event_code: EV_TOOL_DONE, priority: 0 },
        WorkflowEdge{ from_id: 2, to_id: 4, event_code: EV_ABORT, priority: 0 },
    ];
    let hist_buf: [16]TransitionRecord = [TransitionRecord{}; 16];
    let nodes: []WorkflowNode = nodes_buf[0..];
    let hist: []TransitionRecord = hist_buf[0..];
    let fsm = AgentWorkflowFsm.init(nodes, edges[0..], hist);

    expect_eq(fsm.start(), true);
    expect_eq(fsm.trigger(EV_BEGIN, 10) == WorkflowStepResult.Transitioned, true);

    // Visit 1 & 2 to ToolStep(3) succeed
    expect_eq(fsm.trigger(EV_NEED_TOOL, 20) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.trigger(EV_TOOL_DONE, 30) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.trigger(EV_NEED_TOOL, 40) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.trigger(EV_TOOL_DONE, 50) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.history().len, 5);

    // 3rd attempt to enter ToolStep(3) triggers VisitLimitExceeded without mutating state/history
    expect_eq(fsm.trigger(EV_NEED_TOOL, 60) == WorkflowStepResult.VisitLimitExceeded, true);
    expect_eq(fsm.currentNodeId(), 2);
    expect_eq(fsm.history().len, 5);

    // Graceful abort transition to TerminalFail(4) still works
    expect_eq(fsm.trigger(EV_ABORT, 70) == WorkflowStepResult.TerminalReached, true);
    expect_eq(fsm.currentNodeId(), 4);
}

fn testAuditTrailAndRollbackLastStep()! {
    let nodes_buf: [3]WorkflowNode = [
        WorkflowNode{ id: 10, kind: WorkflowNodeKind.Start, max_visits: 1 },
        WorkflowNode{ id: 20, kind: WorkflowNodeKind.LlmStep, max_visits: 1 },
        WorkflowNode{ id: 30, kind: WorkflowNodeKind.TerminalOk, max_visits: 1 },
    ];
    const edges: [2]WorkflowEdge = [
        WorkflowEdge{ from_id: 10, to_id: 20, event_code: EV_BEGIN, priority: 0 },
        WorkflowEdge{ from_id: 20, to_id: 30, event_code: EV_FINISH, priority: 0 },
    ];
    let hist_buf: [8]TransitionRecord = [TransitionRecord{}; 8];
    let nodes: []WorkflowNode = nodes_buf[0..];
    let hist: []TransitionRecord = hist_buf[0..];
    let fsm = AgentWorkflowFsm.init(nodes, edges[0..], hist);

    expect_eq(fsm.start(), true);
    expect_eq(fsm.rollbackLastStep(), false); // empty history

    expect_eq(fsm.trigger(EV_BEGIN, 1000) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.trigger(EV_FINISH, 2000) == WorkflowStepResult.TerminalReached, true);

    const h = fsm.history();
    expect_eq(h.len, 2);
    expect_eq(h[0].step_seq, 1);
    expect_eq(h[0].from_id, 10);
    expect_eq(h[0].to_id, 20);
    expect_eq(h[0].timestamp_ms, 1000);
    expect_eq(h[1].step_seq, 2);
    expect_eq(h[1].from_id, 20);
    expect_eq(h[1].to_id, 30);
    expect_eq(h[1].timestamp_ms, 2000);

    // Rollback from TerminalOk(30) back to LlmStep(20): visit_count of 30 decremented -> can re-enter 30!
    expect_eq(fsm.rollbackLastStep(), true);
    expect_eq(fsm.currentNodeId(), 20);
    expect_eq(fsm.isTerminal(), false);
    expect_eq(fsm.history().len, 1);

    // Re-entering TerminalOk(30) succeeds because visit_count was cleanly decremented
    expect_eq(fsm.trigger(EV_FINISH, 3000) == WorkflowStepResult.TerminalReached, true);
    expect_eq(fsm.currentNodeId(), 30);

    // Rollback twice all the way to Start(10)
    expect_eq(fsm.rollbackLastStep(), true);
    expect_eq(fsm.rollbackLastStep(), true);
    expect_eq(fsm.currentNodeId(), 10);
    expect_eq(fsm.history().len, 0);
    expect_eq(fsm.rollbackLastStep(), false);
}

fn testReachabilityAndInvalidEdgeValidation()! {
    let hist_buf: [4]TransitionRecord = [TransitionRecord{}; 4];
    let hist: []TransitionRecord = hist_buf[0..];

    // 1. Valid connected graph with 2 reachable terminals
    let ok_nodes_buf: [4]WorkflowNode = [
        WorkflowNode{ id: 1, kind: WorkflowNodeKind.Start },
        WorkflowNode{ id: 2, kind: WorkflowNodeKind.Decision },
        WorkflowNode{ id: 3, kind: WorkflowNodeKind.TerminalOk },
        WorkflowNode{ id: 4, kind: WorkflowNodeKind.TerminalFail },
    ];
    const ok_edges: [3]WorkflowEdge = [
        WorkflowEdge{ from_id: 1, to_id: 2, event_code: EV_BEGIN },
        WorkflowEdge{ from_id: 2, to_id: 3, event_code: EV_FINISH },
        WorkflowEdge{ from_id: 2, to_id: 4, event_code: EV_ABORT },
    ];
    let ok_nodes: []WorkflowNode = ok_nodes_buf[0..];
    let ok_fsm = AgentWorkflowFsm.init(ok_nodes, ok_edges[0..], hist);
    const ok_rep = ok_fsm.validateReachability();
    expect_eq(ok_rep.has_start, true);
    expect_eq(ok_rep.has_invalid_edge, false);
    expect_eq(ok_rep.reachable_nodes, 4);
    expect_eq(ok_rep.unreachable_nodes, 0);
    expect_eq(ok_rep.reachable_terminals, 2);
    expect_eq(ok_rep.isValid(), true);

    // 2. Graph with unreachable orphan terminal + dangling edge
    let bad_nodes_buf: [3]WorkflowNode = [
        WorkflowNode{ id: 1, kind: WorkflowNodeKind.Start },
        WorkflowNode{ id: 2, kind: WorkflowNodeKind.LlmStep },
        WorkflowNode{ id: 3, kind: WorkflowNodeKind.TerminalOk },
    ];
    const bad_edges: [2]WorkflowEdge = [
        WorkflowEdge{ from_id: 1, to_id: 2, event_code: EV_BEGIN },
        WorkflowEdge{ from_id: 2, to_id: 99, event_code: EV_FINISH }, // dangling target 99
    ];
    let bad_nodes: []WorkflowNode = bad_nodes_buf[0..];
    let bad_fsm = AgentWorkflowFsm.init(bad_nodes, bad_edges[0..], hist);
    const bad_rep = bad_fsm.validateReachability();
    expect_eq(bad_rep.isValid(), false);
    expect_eq(bad_rep.has_invalid_edge, true);
    expect_eq(bad_rep.unreachable_nodes, 1);
    expect_eq(bad_rep.reachable_terminals, 0);

    // 3. Graph without Start node
    let no_start_buf: [1]WorkflowNode = [
        WorkflowNode{ id: 1, kind: WorkflowNodeKind.LlmStep },
    ];
    let no_start: []WorkflowNode = no_start_buf[0..];
    let ns_fsm = AgentWorkflowFsm.init(no_start, ok_edges[0..0], hist);
    expect_eq(ns_fsm.validateReachability().has_start, false);
    expect_eq(ns_fsm.start(), false);
}

fn testHumanGatePauseAndResume()! {
    let nodes_buf: [5]WorkflowNode = [
        WorkflowNode{ id: 1, kind: WorkflowNodeKind.Start },
        WorkflowNode{ id: 2, kind: WorkflowNodeKind.HumanGate },
        WorkflowNode{ id: 3, kind: WorkflowNodeKind.ToolStep },
        WorkflowNode{ id: 4, kind: WorkflowNodeKind.TerminalOk },
        WorkflowNode{ id: 5, kind: WorkflowNodeKind.TerminalFail },
    ];
    const edges: [4]WorkflowEdge = [
        WorkflowEdge{ from_id: 1, to_id: 2, event_code: EV_BEGIN },
        WorkflowEdge{ from_id: 2, to_id: 3, event_code: EV_APPROVED },
        WorkflowEdge{ from_id: 2, to_id: 5, event_code: EV_REJECTED },
        WorkflowEdge{ from_id: 3, to_id: 4, event_code: EV_FINISH },
    ];
    let hist_buf: [8]TransitionRecord = [TransitionRecord{}; 8];
    let nodes: []WorkflowNode = nodes_buf[0..];
    let hist: []TransitionRecord = hist_buf[0..];
    let fsm = AgentWorkflowFsm.init(nodes, edges[0..], hist);

    expect_eq(fsm.start(), true);
    expect_eq(fsm.trigger(EV_BEGIN, 100) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.isWaitingHuman(), true);

    // Non-approval event rejected while paused on HumanGate
    expect_eq(fsm.trigger(EV_FINISH, 110) == WorkflowStepResult.NoMatchingEdge, true);
    expect_eq(fsm.isWaitingHuman(), true);

    // Approval resumes into ToolStep -> TerminalOk
    expect_eq(fsm.trigger(EV_APPROVED, 200) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.isWaitingHuman(), false);
    expect_eq(fsm.currentNodeId(), 3);
    expect_eq(fsm.trigger(EV_FINISH, 300) == WorkflowStepResult.TerminalReached, true);
    expect_eq(fsm.currentNodeId(), 4);

    // Restart and test rejection path -> TerminalFail
    expect_eq(fsm.start(), true);
    expect_eq(fsm.trigger(EV_BEGIN, 400) == WorkflowStepResult.Transitioned, true);
    expect_eq(fsm.trigger(EV_REJECTED, 500) == WorkflowStepResult.TerminalReached, true);
    expect_eq(fsm.currentNodeId(), 5);
}

fn main() {
    test("1. agent workflow_fsm: multi-branch priority routing & terminal guards", () => {
        testMultiBranchPriorityAndTerminal()?;
    });
    test("2. agent workflow_fsm: max_visits cycle guard & fallback routing", () => {
        testCycleGuardVisitLimitExceeded()?;
    });
    test("3. agent workflow_fsm: audit history fidelity & rollbackLastStep", () => {
        testAuditTrailAndRollbackLastStep()?;
    });
    test("4. agent workflow_fsm: static reachability & dangling edge validation", () => {
        testReachabilityAndInvalidEdgeValidation()?;
    });
    test("5. agent workflow_fsm: HumanGate pause & approval/rejection resumption", () => {
        testHumanGatePauseAndResume()?;
    });
}
