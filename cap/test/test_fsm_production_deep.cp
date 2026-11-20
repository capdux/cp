// Source: cp/cap/test/test_fsm_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:fsm & cap.fsm

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    FsmError,
    TransitionRule,
    TransitionRecord,
    StateMachine,
    CircuitState,
    CircuitBreakerConfig,
    CircuitStats,
    CircuitBreaker,
    DagError,
    TaskStatus,
    WorkflowDag,
} from "cap:fsm";

fn testStateMachineAuditAndMermaid(): !void {
    const alloc = std.heap.page_allocator;
    let sm = StateMachine.init(alloc, "idle");
    defer sm.deinit();

    expect_eq(sm.currentState(), "idle");
    expect_eq(sm.canTrigger("plan"), false);

    sm.addTransition("idle", "plan", "planning")?;
    sm.addTransition("planning", "execute", "running")?;
    sm.addTransition("running", "fail", "failed")?;
    sm.addTransition("failed", "retry", "running")?;
    sm.addTransition("running", "finish", "completed")?;

    // Duplicate (from_state, event) rule must be rejected
    const dup_res = sm.addTransition("idle", "plan", "other");
    const dup_tag = switch (dup_res) {
        FsmError.DuplicateTransition => "dup",
        else => "other",
        ok => "ok",
    };
    expect_eq(dup_tag, "dup");

    expect_eq(sm.canTrigger("plan"), true);
    expect_eq(sm.canTrigger("finish"), false);

    // Trigger valid sequence including retry loop
    expect_eq(sm.trigger("plan")?, "planning");
    expect_eq(sm.trigger("execute")?, "running");
    expect_eq(sm.trigger("fail")?, "failed");
    expect_eq(sm.trigger("retry")?, "running");
    expect_eq(sm.trigger("finish")?, "completed");
    expect_eq(sm.currentState(), "completed");

    // Invalid transition from terminal state must return FsmError.InvalidTransition
    const bad_res = sm.trigger("plan");
    const bad_tag = switch (bad_res) {
        FsmError.InvalidTransition => "invalid",
        else => "other",
        ok => "ok",
    };
    expect_eq(bad_tag, "invalid");
    expect_eq(sm.currentState(), "completed");

    // Verify full audit history
    const hist = sm.history();
    expect_eq(hist.len, 5 as usize);
    expect_eq(hist[0].step, 0 as usize);
    expect_eq(hist[0].from_state, "idle");
    expect_eq(hist[0].event, "plan");
    expect_eq(hist[0].to_state, "planning");
    expect_eq(hist[2].from_state, "running");
    expect_eq(hist[2].event, "fail");
    expect_eq(hist[2].to_state, "failed");
    expect_eq(hist[4].from_state, "running");
    expect_eq(hist[4].event, "finish");
    expect_eq(hist[4].to_state, "completed");

    // Export Mermaid stateDiagram-v2
    const mermaid = sm.toMermaid(alloc)?;
    defer alloc.free(mermaid);
    const expected_mermaid =
        "stateDiagram-v2\n" ++
        "    [*] --> idle\n" ++
        "    idle --> planning : plan\n" ++
        "    planning --> running : execute\n" ++
        "    running --> failed : fail\n" ++
        "    failed --> running : retry\n" ++
        "    running --> completed : finish\n";
    expect_eq(mermaid, expected_mermaid);

    // Reset restores initial state & clears history while preserving rules
    sm.reset();
    expect_eq(sm.currentState(), "idle");
    expect_eq(sm.history().len, 0 as usize);
    expect_eq(sm.canTrigger("plan"), true);
    expect_eq(sm.trigger("plan")?, "planning");
}

fn testCircuitBreakerFullLifecycle(): !void {
    let cb = CircuitBreaker.init(CircuitBreakerConfig{
        failure_threshold: 3,
        success_threshold_half_open: 2,
        open_timeout_ms: 5000,
        half_open_max_probes: 1,
    });

    // 1. Closed phase: failures below threshold reset on success
    expect_eq(cb.state(100), CircuitState.Closed);
    expect_eq(cb.allowRequest(100), true);
    cb.recordFailure(110);
    cb.recordFailure(120);
    expect_eq(cb.state(120), CircuitState.Closed);
    cb.recordSuccess(130); // resets consecutive failures
    expect_eq(cb.stats().consecutive_failures, 0 as u32);

    // 2. Trip to Open after 3 consecutive failures
    expect_eq(cb.allowRequest(200), true);
    cb.recordFailure(210);
    cb.recordFailure(220);
    expect_eq(cb.state(220), CircuitState.Closed);
    cb.recordFailure(230); // 3rd consecutive failure -> Open at 230ms
    expect_eq(cb.state(230), CircuitState.Open);
    expect_eq(cb.stats().open_count, 1 as u64);

    // 3. Reject requests while within cooldown window (< 5000ms)
    expect_eq(cb.allowRequest(1000), false);
    expect_eq(cb.allowRequest(5229), false);
    expect_eq(cb.state(5229), CircuitState.Open);

    // 4. Cooldown expires at 230 + 5000 = 5230 -> HalfOpen, probe slot limit enforced
    expect_eq(cb.allowRequest(5230), true);
    expect_eq(cb.currentState(), CircuitState.HalfOpen);
    expect_eq(cb.allowRequest(5231), false); // max 1 probe in flight

    // 5. Probe fails -> immediately re-opens at 5250ms
    cb.recordFailure(5250);
    expect_eq(cb.currentState(), CircuitState.Open);
    expect_eq(cb.stats().open_count, 2 as u64);
    expect_eq(cb.allowRequest(10249), false);

    // 6. Second HalfOpen cycle: 2 consecutive probe successes recover to Closed
    expect_eq(cb.allowRequest(10250), true);
    expect_eq(cb.currentState(), CircuitState.HalfOpen);
    cb.recordSuccess(10260);
    expect_eq(cb.currentState(), CircuitState.HalfOpen); // 1/2 successes
    expect_eq(cb.allowRequest(10270), true);
    cb.recordSuccess(10280); // 2/2 successes -> Closed!
    expect_eq(cb.currentState(), CircuitState.Closed);

    // 7. forceOpen & reset
    cb.forceOpen(11000);
    expect_eq(cb.currentState(), CircuitState.Open);
    expect_eq(cb.allowRequest(11100), false);
    cb.reset();
    expect_eq(cb.currentState(), CircuitState.Closed);
    expect_eq(cb.allowRequest(11200), true);

    const st = cb.stats();
    expect_eq(st.open_count, 3 as u64);
    expect_eq(st.total_requests, 11 as u64);
    expect_eq(st.total_allowed, 6 as u64);
    expect_eq(st.total_rejected, 5 as u64);
    expect_eq(st.total_failures, 6 as u64);
    expect_eq(st.total_successes, 3 as u64);
}

fn testDagWavesAndCycleDetection(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Multi-layer parallel AI workflow DAG
    let dag = WorkflowDag.init(alloc);
    defer dag.deinit();

    dag.addNode("fetch_docs")?;
    dag.addNode("fetch_schema")?;
    dag.addNode("embed_docs")?;
    dag.addNode("parse_schema")?;
    dag.addNode("plan")?;
    dag.addNode("codegen")?;
    dag.addNode("test_gen")?;
    dag.addNode("verify")?;

    dag.addEdge("fetch_docs", "embed_docs")?;
    dag.addEdge("fetch_schema", "parse_schema")?;
    dag.addEdge("embed_docs", "plan")?;
    dag.addEdge("parse_schema", "plan")?;
    dag.addEdge("plan", "codegen")?;
    dag.addEdge("plan", "test_gen")?;
    dag.addEdge("codegen", "verify")?;
    dag.addEdge("test_gen", "verify")?;

    expect_eq(dag.nodeCount(), 8 as usize);
    expect_eq(dag.edgeCount(), 8 as usize);
    expect_eq(dag.hasCycle(), false);

    const waves = dag.executionWaves(alloc)?;
    defer WorkflowDag.freeWaves(alloc, waves);

    expect_eq(waves.len, 5 as usize);
    expect_eq(waves[0].len, 2 as usize);
    expect_eq(waves[0][0], "fetch_docs");
    expect_eq(waves[0][1], "fetch_schema");

    expect_eq(waves[1].len, 2 as usize);
    expect_eq(waves[1][0], "embed_docs");
    expect_eq(waves[1][1], "parse_schema");

    expect_eq(waves[2].len, 1 as usize);
    expect_eq(waves[2][0], "plan");

    expect_eq(waves[3].len, 2 as usize);
    expect_eq(waves[3][0], "codegen");
    expect_eq(waves[3][1], "test_gen");

    expect_eq(waves[4].len, 1 as usize);
    expect_eq(waves[4][0], "verify");

    // 2. 3-node cycle detection
    let cyclic = WorkflowDag.init(alloc);
    defer cyclic.deinit();
    cyclic.addNode("a")?;
    cyclic.addNode("b")?;
    cyclic.addNode("c")?;
    cyclic.addEdge("a", "b")?;
    cyclic.addEdge("b", "c")?;
    cyclic.addEdge("c", "a")?;

    expect_eq(cyclic.hasCycle(), true);
    const cyc_res = cyclic.executionWaves(alloc);
    const cyc_tag = switch (cyc_res) {
        DagError.CycleDetected => "cycle",
        else => "other",
        ok => "ok",
    };
    expect_eq(cyc_tag, "cycle");

    // 3. Self-loop cycle detection
    let self_loop = WorkflowDag.init(alloc);
    defer self_loop.deinit();
    self_loop.addNode("x")?;
    self_loop.addEdge("x", "x")?;
    expect_eq(self_loop.hasCycle(), true);
}

fn testDagDynamicSchedulingAndFailureCascade(): !void {
    const alloc = std.heap.page_allocator;
    let dag = WorkflowDag.init(alloc);
    defer dag.deinit();

    dag.addNode("init")?;
    dag.addNode("fetch_a")?;
    dag.addNode("fetch_b")?;
    dag.addNode("merge")?;
    dag.addNode("deploy")?;
    dag.addNode("telemetry")?;

    dag.addEdge("init", "fetch_a")?;
    dag.addEdge("init", "fetch_b")?;
    dag.addEdge("init", "telemetry")?;
    dag.addEdge("fetch_a", "merge")?;
    dag.addEdge("fetch_b", "merge")?;
    dag.addEdge("merge", "deploy")?;

    expect_eq(dag.isComplete(), false);

    // Wave 0 ready: only "init"
    const r0 = dag.readyNodes(alloc)?;
    defer alloc.free(r0);
    expect_eq(r0.len, 1 as usize);
    expect_eq(r0[0], "init");

    dag.markRunning("init")?;
    expect_eq(dag.nodeStatus("init").?, TaskStatus.Running);
    dag.markSucceeded("init")?;
    expect_eq(dag.nodeStatus("init").?, TaskStatus.Succeeded);

    // Now fetch_a, fetch_b, telemetry are ready
    const r1 = dag.readyNodes(alloc)?;
    defer alloc.free(r1);
    expect_eq(r1.len, 3 as usize);
    expect_eq(r1[0], "fetch_a");
    expect_eq(r1[1], "fetch_b");
    expect_eq(r1[2], "telemetry");

    // fetch_a fails -> merge and deploy must cascade to Skipped!
    dag.markRunning("fetch_a")?;
    dag.markFailed("fetch_a")?;
    expect_eq(dag.nodeStatus("fetch_a").?, TaskStatus.Failed);
    expect_eq(dag.nodeStatus("merge").?, TaskStatus.Skipped);
    expect_eq(dag.nodeStatus("deploy").?, TaskStatus.Skipped);
    expect_eq(dag.nodeStatus("fetch_b").?, TaskStatus.Pending);
    expect_eq(dag.nodeStatus("telemetry").?, TaskStatus.Pending);
    expect_eq(dag.isComplete(), false);

    // Skipped node cannot transition to Running
    const skip_res = dag.markRunning("merge");
    const skip_tag = switch (skip_res) {
        DagError.InvalidStateTransition => "invalid_state",
        else => "other",
        ok => "ok",
    };
    expect_eq(skip_tag, "invalid_state");

    // Independent branches complete cleanly
    dag.markRunning("fetch_b")?;
    dag.markSucceeded("fetch_b")?;
    dag.markRunning("telemetry")?;
    dag.markSucceeded("telemetry")?;

    const r2 = dag.readyNodes(alloc)?;
    defer alloc.free(r2);
    expect_eq(r2.len, 0 as usize);
    expect_eq(dag.isComplete(), true);

    // Reset restores all nodes to Pending
    dag.reset();
    expect_eq(dag.isComplete(), false);
    expect_eq(dag.nodeStatus("deploy").?, TaskStatus.Pending);
}

fn testCapFsmDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    // 1. cap.fsm.StateMachine
    let sm = cap.fsm.StateMachine.init(alloc, "draft");
    defer sm.deinit();
    sm.addTransition("draft", "submit", "active")?;
    expect_eq(sm.trigger("submit")?, "active");

    // 2. cap.fsm.CircuitBreaker
    let cb = cap.fsm.CircuitBreaker.init(cap.fsm.CircuitBreakerConfig{
        failure_threshold: 1,
        success_threshold_half_open: 1,
        open_timeout_ms: 100,
        half_open_max_probes: 1,
    });
    expect_eq(cb.allowRequest(0), true);
    cb.recordFailure(10);
    expect_eq(cb.state(20), cap.fsm.CircuitState.Open);
    expect_eq(cb.allowRequest(110), true);
    cb.recordSuccess(120);
    expect_eq(cb.currentState(), cap.fsm.CircuitState.Closed);

    // 3. cap.fsm.WorkflowDag
    let dag = cap.fsm.WorkflowDag.init(alloc);
    defer dag.deinit();
    dag.addNode("step1")?;
    dag.addNode("step2")?;
    dag.addEdge("step1", "step2")?;
    const waves = dag.executionWaves(alloc)?;
    defer cap.fsm.WorkflowDag.freeWaves(alloc, waves);
    expect_eq(waves.len, 2 as usize);
    expect_eq(waves[0][0], "step1");
    expect_eq(waves[1][0], "step2");
    dag.markFailed("step1")?;
    expect_eq(dag.nodeStatus("step2").?, cap.fsm.TaskStatus.Skipped);
    expect_eq(dag.isComplete(), true);
}

fn main() {
    test("cap:fsm StateMachine legal/illegal transitions, audit history & Mermaid export", () => {
        testStateMachineAuditAndMermaid()?;
    });

    test("cap:fsm CircuitBreaker Closed->Open->HalfOpen->Closed & probe failure re-open", () => {
        testCircuitBreakerFullLifecycle()?;
    });

    test("cap:fsm WorkflowDag multi-wave parallel executionWaves & cycle detection", () => {
        testDagWavesAndCycleDetection()?;
    });

    test("cap:fsm WorkflowDag dynamic readyNodes & failure-to-Skipped cascade", () => {
        testDagDynamicSchedulingAndFailureCascade()?;
    });

    test("cap.fsm lazy dynamic hub mount via import cap from \"cap\"", () => {
        testCapFsmDynamicMount()?;
    });
}
