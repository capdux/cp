// Origin: Pure-CP Stdlib Test Suite (lib/agent/plan_executor.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Hierarchical Plan-Execute-Replan Task Tree Engine:
//   1. Hierarchical task tree execution, leaf-only dispatch & automatic bottom-up parent roll-up
//   2. Automatic transient failure retry (`attempts < max_attempts`) & result_note tracking
//   3. Permanent failure cascade (`Skipped` on multi-hop downstream dependents) + independent branch isolation
//   4. Online replanning (`replanAppendStep` + `rebindDependency`) recovering a skipped downstream pipeline
//   5. Deterministic ascending `step_id` tie-breaking & ancestor prerequisite gating
//   6. Invalid init, missing parent/dep, duplicate ID, full buffer, cycle rebind & invalid state transition guards

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    PLAN_NO_PARENT,
    PlanStepState,
    PlanStepNode,
    PlanProgress,
    PlanExecutor,
} from "cap:agent";

// ============================================================================
// Test 1: Hierarchical Task Tree Execution & Bottom-Up Parent Roll-Up
// ============================================================================

fn testHierarchicalExecutionAndRollup()! {
    let buf: [8]PlanStepNode = [PlanStepNode{}; 8];
    let exec = cap.agent.PlanExecutor.init(buf[0..]) ?? return error.InitFailed;

    // Phase 1 (id=1): children 2 -> 3
    // Phase 2 (id=4, depends_on=1): children 5 -> 6
    expect_eq(exec.addStep(1, PLAN_NO_PARENT, PLAN_NO_PARENT, "Phase 1: Build", 1), true);
    expect_eq(exec.addStep(2, 1, PLAN_NO_PARENT, "Compile core", 1), true);
    expect_eq(exec.addStep(3, 1, 2, "Run unit tests", 1), true);
    expect_eq(exec.addStep(4, PLAN_NO_PARENT, 1, "Phase 2: Release", 1), true);
    expect_eq(exec.addStep(5, 4, PLAN_NO_PARENT, "Package binary", 1), true);
    expect_eq(exec.addStep(6, 4, 5, "Publish artifacts", 1), true);

    // Only leaf 2 should be ready (parent 1 is container, leaf 3 waits on 2, Phase 2 waits on 1)
    expect_eq(exec.nextReadyStep() ?? 0, 2);
    expect_eq(exec.startStep(1), false); // Non-leaf parent cannot be started directly
    expect_eq(exec.startStep(5), false); // Blocked by parent 4's dependency on 1

    // Execute 2 -> unlocks 3
    expect_eq(exec.startStep(2), true);
    expect_eq(exec.completeStep(2, true, "compiled ok"), true);
    expect_eq(exec.nextReadyStep() ?? 0, 3);

    // Execute 3 -> Phase 1 (id=1) auto-rolls up to Succeeded -> unlocks leaf 5 under Phase 2!
    expect_eq(exec.startStep(3), true);
    expect_eq(exec.completeStep(3, true, "tests green"), true);

    const p1 = exec.getStep(1) ?? return error.MissingStep;
    expect_eq(p1.state == PlanStepState.Succeeded, true);
    expect_eq(exec.nextReadyStep() ?? 0, 5);

    // Execute 5 -> unlocks 6 -> Phase 2 (id=4) auto-rolls up to Succeeded
    expect_eq(exec.startStep(5), true);
    expect_eq(exec.completeStep(5, true, "packaged"), true);
    expect_eq(exec.nextReadyStep() ?? 0, 6);

    expect_eq(exec.startStep(6), true);
    expect_eq(exec.completeStep(6, true, "published"), true);

    expect_eq(exec.nextReadyStep() == null, true);

    const prog = exec.progress();
    expect_eq(prog.total_steps, 6);
    expect_eq(prog.succeeded_steps, 6);
    expect_eq(prog.failed_steps, 0);
    expect_eq(prog.skipped_steps, 0);
    expect_eq(prog.remaining_steps, 0);
    expect_eq(prog.isDone(), true);
    expect_eq(prog.isAllSucceeded(), true);
    expect_eq(prog.completion_ratio > 0.999, true);
}

// ============================================================================
// Test 2: Automatic Transient Failure Retry (`attempts < max_attempts`)
// ============================================================================

fn testAutomaticTransientRetry()! {
    let buf: [4]PlanStepNode = [PlanStepNode{}; 4];
    let exec = PlanExecutor.init(buf[0..]) ?? return error.InitFailed;

    expect_eq(exec.addStep(10, PLAN_NO_PARENT, PLAN_NO_PARENT, "Call flaky API", 3), true);
    expect_eq(exec.addStep(20, PLAN_NO_PARENT, 10, "Parse API response", 1), true);

    // Attempt 1 fails -> automatically returns to Ready
    expect_eq(exec.startStep(10), true);
    expect_eq(exec.completeStep(10, false, "HTTP 503 #1"), true);
    const s1 = exec.getStep(10) ?? return error.MissingStep;
    expect_eq(s1.attempts, 1);
    expect_eq(s1.state == PlanStepState.Ready, true);
    expect_eq(exec.nextReadyStep() ?? 0, 10);

    // Attempt 2 fails -> still < max_attempts (3), returns to Ready
    expect_eq(exec.startStep(10), true);
    expect_eq(exec.completeStep(10, false, "HTTP 503 #2"), true);
    const s2 = exec.getStep(10) ?? return error.MissingStep;
    expect_eq(s2.attempts, 2);
    expect_eq(s2.state == PlanStepState.Ready, true);

    // Attempt 3 succeeds -> unlocks step 20!
    expect_eq(exec.startStep(10), true);
    expect_eq(exec.completeStep(10, true, "HTTP 200 OK"), true);
    const s3 = exec.getStep(10) ?? return error.MissingStep;
    expect_eq(s3.attempts, 3);
    expect_eq(s3.state == PlanStepState.Succeeded, true);
    expect_eq(exec.nextReadyStep() ?? 0, 20);

    expect_eq(exec.startStep(20), true);
    expect_eq(exec.completeStep(20, true, "parsed"), true);
    expect_eq(exec.progress().isAllSucceeded(), true);
}

// ============================================================================
// Test 3: Permanent Failure Cascade (`Skipped`) & Independent Branch Isolation
// ============================================================================

fn testPermanentFailureCascadeAndIsolation()! {
    let buf: [6]PlanStepNode = [PlanStepNode{}; 6];
    let exec = PlanExecutor.init(buf[0..]) ?? return error.InitFailed;

    // Chain: 1 -> 2 -> 3 -> 4; plus independent step 5
    expect_eq(exec.addStep(1, PLAN_NO_PARENT, PLAN_NO_PARENT, "Init", 1), true);
    expect_eq(exec.addStep(2, PLAN_NO_PARENT, 1, "Risky migrate", 1), true);
    expect_eq(exec.addStep(3, PLAN_NO_PARENT, 2, "Verify schema", 1), true);
    expect_eq(exec.addStep(4, PLAN_NO_PARENT, 3, "Switch traffic", 1), true);
    expect_eq(exec.addStep(5, PLAN_NO_PARENT, PLAN_NO_PARENT, "Emit audit log", 1), true);

    // Step 1 succeeds
    expect_eq(exec.nextReadyStep() ?? 0, 1);
    expect_eq(exec.startStep(1), true);
    expect_eq(exec.completeStep(1, true, "ok"), true);

    // Step 2 fails permanently (attempts=1 == max_attempts=1) -> cascades Skipped to 3 and 4!
    expect_eq(exec.nextReadyStep() ?? 0, 2);
    expect_eq(exec.startStep(2), true);
    expect_eq(exec.completeStep(2, false, "migration checksum error"), true);

    const n2 = exec.getStep(2) ?? return error.MissingStep;
    const n3 = exec.getStep(3) ?? return error.MissingStep;
    const n4 = exec.getStep(4) ?? return error.MissingStep;
    expect_eq(n2.state == PlanStepState.Failed, true);
    expect_eq(n3.state == PlanStepState.Skipped, true);
    expect_eq(n4.state == PlanStepState.Skipped, true);

    // Independent step 5 is unaffected and runs to completion
    expect_eq(exec.nextReadyStep() ?? 0, 5);
    expect_eq(exec.startStep(5), true);
    expect_eq(exec.completeStep(5, true, "audit logged"), true);

    const prog = exec.progress();
    expect_eq(prog.total_steps, 5);
    expect_eq(prog.succeeded_steps, 2);
    expect_eq(prog.failed_steps, 1);
    expect_eq(prog.skipped_steps, 2);
    expect_eq(prog.remaining_steps, 0);
    expect_eq(prog.isDone(), true);
    expect_eq(prog.isAllSucceeded(), false);
    expect_eq(prog.completion_ratio > 0.999, true);
}

// ============================================================================
// Test 4: Dynamic Replanning (`replanAppendStep` + `rebindDependency`)
// ============================================================================

fn testDynamicReplanRecovery()! {
    let buf: [8]PlanStepNode = [PlanStepNode{}; 8];
    let exec = PlanExecutor.init(buf[0..]) ?? return error.InitFailed;

    expect_eq(exec.addStep(1, PLAN_NO_PARENT, PLAN_NO_PARENT, "Fetch primary mirror", 1), true);
    expect_eq(exec.addStep(2, PLAN_NO_PARENT, 1, "Unpack archive", 1), true);
    expect_eq(exec.addStep(3, PLAN_NO_PARENT, 2, "Run benchmark", 1), true);

    // Step 1 fails permanently -> steps 2 & 3 cascade to Skipped
    expect_eq(exec.startStep(1), true);
    expect_eq(exec.completeStep(1, false, "mirror 404"), true);
    expect_eq((exec.getStep(2) ?? return error.MissingStep).state == PlanStepState.Skipped, true);
    expect_eq((exec.getStep(3) ?? return error.MissingStep).state == PlanStepState.Skipped, true);
    expect_eq(exec.nextReadyStep() == null, true);

    // Agent replans: inject fallback fetch step 10 and rebind step 2 -> depends_on 10!
    expect_eq(exec.replanAppendStep(10, PLAN_NO_PARENT, PLAN_NO_PARENT, "Fetch backup mirror", 2), true);
    expect_eq(exec.rebindDependency(2, 10), true);
    expect_eq(exec.rebindDependency(3, 2), true);

    // Step 10 is immediately ready; step 2 & 3 are Pending awaiting 10
    expect_eq(exec.nextReadyStep() ?? 0, 10);
    expect_eq((exec.getStep(2) ?? return error.MissingStep).state == PlanStepState.Pending, true);

    // Execute recovery path: 10 -> 2 -> 3
    expect_eq(exec.startStep(10), true);
    expect_eq(exec.completeStep(10, true, "backup fetched"), true);

    expect_eq(exec.nextReadyStep() ?? 0, 2);
    expect_eq(exec.startStep(2), true);
    expect_eq(exec.completeStep(2, true, "unpacked"), true);

    expect_eq(exec.nextReadyStep() ?? 0, 3);
    expect_eq(exec.startStep(3), true);
    expect_eq(exec.completeStep(3, true, "bench passed"), true);

    const prog = exec.progress();
    expect_eq(prog.total_steps, 4);
    expect_eq(prog.succeeded_steps, 3);
    expect_eq(prog.failed_steps, 1);
    expect_eq(prog.skipped_steps, 0);
    expect_eq(prog.replan_count, 3);
    expect_eq(prog.isDone(), true);
}

// ============================================================================
// Test 5: Deterministic Ascending `step_id` Selection
// ============================================================================

fn testDeterministicAscendingStepSelection()! {
    let buf: [4]PlanStepNode = [PlanStepNode{}; 4];
    let exec = PlanExecutor.init(buf[0..]) ?? return error.InitFailed;

    // Add independent steps out of order: 30, 10, 20
    expect_eq(exec.addStep(30, PLAN_NO_PARENT, PLAN_NO_PARENT, "Third", 1), true);
    expect_eq(exec.addStep(10, PLAN_NO_PARENT, PLAN_NO_PARENT, "First", 1), true);
    expect_eq(exec.addStep(20, PLAN_NO_PARENT, PLAN_NO_PARENT, "Second", 1), true);

    expect_eq(exec.nextReadyStep() ?? 0, 10);
    expect_eq(exec.startStep(10), true);
    expect_eq(exec.completeStep(10, true, "ok"), true);

    expect_eq(exec.nextReadyStep() ?? 0, 20);
    expect_eq(exec.startStep(20), true);
    expect_eq(exec.completeStep(20, true, "ok"), true);

    expect_eq(exec.nextReadyStep() ?? 0, 30);
    expect_eq(exec.startStep(30), true);
    expect_eq(exec.completeStep(30, true, "ok"), true);

    expect_eq(exec.progress().isAllSucceeded(), true);
}

// ============================================================================
// Test 6: Defensive Boundary & Invalid Input Guards
// ============================================================================

fn testDefensiveGuards()! {
    let buf: [2]PlanStepNode = [PlanStepNode{}; 2];
    let empty_buf: []PlanStepNode = buf[0..0];

    expect_eq(PlanExecutor.init(empty_buf) == null, true);

    let exec = PlanExecutor.init(buf[0..]) ?? return error.InitFailed;
    expect_eq(exec.nextReadyStep() == null, true);
    expect_eq(exec.progress().isDone(), false);
    expect_eq(exec.progress().isAllSucceeded(), false);

    // Invalid sentinel step_id, zero max_attempts, self-parent, self-dep, missing parent/dep
    expect_eq(exec.addStep(PLAN_NO_PARENT, PLAN_NO_PARENT, PLAN_NO_PARENT, "bad", 1), false);
    expect_eq(exec.addStep(1, PLAN_NO_PARENT, PLAN_NO_PARENT, "bad", 0), false);
    expect_eq(exec.addStep(1, 1, PLAN_NO_PARENT, "self-parent", 1), false);
    expect_eq(exec.addStep(1, PLAN_NO_PARENT, 1, "self-dep", 1), false);
    expect_eq(exec.addStep(1, 999, PLAN_NO_PARENT, "missing-parent", 1), false);
    expect_eq(exec.addStep(1, PLAN_NO_PARENT, 999, "missing-dep", 1), false);

    // Valid add + duplicate ID rejected + full buffer rejected
    expect_eq(exec.addStep(1, PLAN_NO_PARENT, PLAN_NO_PARENT, "Step 1", 1), true);
    expect_eq(exec.addStep(1, PLAN_NO_PARENT, PLAN_NO_PARENT, "Dup 1", 1), false);
    expect_eq(exec.addStep(2, PLAN_NO_PARENT, 1, "Step 2", 1), true);
    expect_eq(exec.addStep(3, PLAN_NO_PARENT, PLAN_NO_PARENT, "Overflow", 1), false);

    // Cycle detection on rebindDependency: 2 depends on 1, so rebinding 1 -> 2 must be rejected
    expect_eq(exec.rebindDependency(1, 2), false);

    // Invalid state transitions
    expect_eq(exec.completeStep(1, true, "not running yet"), false);
    expect_eq(exec.startStep(2), false); // Prerequisite 1 not succeeded yet
    expect_eq(exec.startStep(999), false);
    expect_eq(exec.completeStep(999, true, "unknown"), false);
}

// ============================================================================
// Test Runner Registration
// ============================================================================

test("agent/plan_executor: hierarchical tree execution & automatic parent roll-up", testHierarchicalExecutionAndRollup);
test("agent/plan_executor: automatic transient failure retry", testAutomaticTransientRetry);
test("agent/plan_executor: permanent failure cascade & independent branch isolation", testPermanentFailureCascadeAndIsolation);
test("agent/plan_executor: dynamic replanning (replanAppendStep + rebindDependency)", testDynamicReplanRecovery);
test("agent/plan_executor: deterministic ascending step_id selection", testDeterministicAscendingStepSelection);
test("agent/plan_executor: invalid init, duplicates, cycle rebind & full buffer guards", testDefensiveGuards);
