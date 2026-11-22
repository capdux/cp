// Origin: Pure-CP Stdlib Test Suite (lib/agent/turn_runner.cp) | Created: 2026-11-22
// Production Deep Verification Suite for End-to-End Agent Turn Runner:
//   1. Normal multi-turn tool-call convergence, audit trace verification & zero-alloc reset
//   2. Input/output token & micro-USD cost budget circuit breaking + saturating overflow protection
//   3. Consecutive tool error circuit breaker, streak reset on success & max_tool_calls guard
//   4. Max turns enforcement & stopped-state idempotency
//   5. Step buffer full boundary protection across beginTurn / recordLlmResponse / recordToolCall / finish
//   6. Invalid init guards, backwards clock skew protection & explicit Cancelled / LlmError finish

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    TurnRunnerConfig,
    TurnStopReason,
    TurnStepKind,
    TurnStepEntry,
    TurnOutcome,
    AgentTurnRunner,
} from "cap:agent";

// ============================================================================
// Test 1: Multi-Turn Tool Call Convergence, Audit Trace & Zero-Alloc Reset
// ============================================================================

fn testMultiTurnConvergenceAndReset()! {
    let steps_buf: [24]TurnStepEntry = [TurnStepEntry{}; 24];
    let steps_s: []TurnStepEntry = steps_buf[0..];

    let runner = cap.agent.AgentTurnRunner.init(steps_s, TurnRunnerConfig{
        max_turns: 5,
        max_tool_calls: 10,
        max_input_tokens: 10000,
        max_output_tokens: 4000,
        max_cost_micro_usd: 50000,
        max_consecutive_errors: 2,
    }) ?? return error.InitFailed;

    expect_eq(runner.currentTurn(), 0);
    expect_eq(runner.isStopped(), false);
    expect_eq(runner.stopReason() == null, true);

    // Turn 1: LLM requests two tools; first succeeds, second succeeds.
    expect_eq(runner.beginTurn(1000) == null, true);
    expect_eq(runner.currentTurn(), 1);
    expect_eq(runner.recordLlmResponse(500, 120, 1500, true, "need search & read", 1100) == null, true);
    expect_eq(runner.recordToolCall("search", true, "found 2 files", 1250) == null, true);
    expect_eq(runner.recordToolCall("read_file", true, "read 80 lines", 1400) == null, true);

    // Turn 2: LLM requests a tool that fails once, then recovers on retry.
    expect_eq(runner.beginTurn(1500) == null, true);
    expect_eq(runner.currentTurn(), 2);
    expect_eq(runner.recordLlmResponse(900, 180, 2400, true, "run test", 1650) == null, true);
    expect_eq(runner.recordToolCall("run_test", false, "transient timeout", 1800) == null, true);
    expect_eq(runner.consecutiveErrors(), 1);
    expect_eq(runner.recordToolCall("run_test", true, "all tests green", 1950) == null, true);
    expect_eq(runner.consecutiveErrors(), 0);

    // Turn 3: LLM converges with final answer (has_tool_call = false).
    expect_eq(runner.beginTurn(2000) == null, true);
    const stop_opt = runner.recordLlmResponse(1100, 300, 3100, false, "final answer ready", 2200);
    expect_eq(stop_opt != null, true);
    const stop_reason = stop_opt ?? return error.ExpectedStop;
    expect_eq(stop_reason == TurnStopReason.Completed, true);
    expect_eq(runner.isStopped(), true);

    const outcome: TurnOutcome = runner.finish(stop_reason, 2250);
    expect_eq(outcome.isSuccess(), true);
    expect_eq(outcome.isBudgetExceeded(), false);
    expect_eq(outcome.turns_executed, 3);
    expect_eq(outcome.tool_calls, 4);
    expect_eq(outcome.tool_errors, 1);
    expect_eq(outcome.consecutive_errors, 0);
    expect_eq(outcome.total_input_tokens, 2500);
    expect_eq(outcome.total_output_tokens, 600);
    expect_eq(outcome.total_tokens, 3100);
    expect_eq(outcome.total_cost_micro_usd, 7000);
    expect_eq(outcome.started_at_ms, 1000);
    expect_eq(outcome.ended_at_ms, 2250);
    expect_eq(outcome.elapsed_ms, 1250);
    expect_eq(outcome.steps_dropped, 0);

    // 3 TurnStarted + 3 LlmResponse + 4 ToolCall + 1 Stopped = 11 steps
    expect_eq(outcome.steps.len, 11);
    expect_eq(outcome.steps[0].seq, 1);
    expect_eq(outcome.steps[0].kind == TurnStepKind.TurnStarted, true);
    expect_eq(outcome.steps[2].kind == TurnStepKind.ToolSucceeded, true);
    expect_eq(std.mem.eql(u8, outcome.steps[2].tool_name, "search"), true);
    expect_eq(outcome.steps[6].kind == TurnStepKind.ToolFailed, true);
    expect_eq(outcome.steps[6].succeeded, false);
    expect_eq(outcome.steps[9].kind == TurnStepKind.LlmResponse, true);
    expect_eq(outcome.steps[9].has_tool_call, false);
    expect_eq(outcome.steps[10].kind == TurnStepKind.Stopped, true);
    expect_eq(outcome.steps[10].seq, 11);

    // Reset reuses buffer cleanly with zero allocation.
    runner.reset();
    expect_eq(runner.currentTurn(), 0);
    expect_eq(runner.toolCallCount(), 0);
    expect_eq(runner.totalTokens(), 0);
    expect_eq(runner.totalCostMicroUsd(), 0);
    expect_eq(runner.stepCount(), 0);
    expect_eq(runner.isStopped(), false);

    expect_eq(runner.beginTurn(5000) == null, true);
    const stop2 = runner.recordLlmResponse(100, 50, 200, false, "quick reply", 5050) ?? return error.ExpectedStop;
    expect_eq(stop2 == TurnStopReason.Completed, true);
    const out2 = runner.finish(stop2, 5100);
    expect_eq(out2.turns_executed, 1);
    expect_eq(out2.elapsed_ms, 100);
    expect_eq(out2.steps.len, 3);
}

// ============================================================================
// Test 2: Token & Cost Budget Circuit Breaking + Overflow Saturation
// ============================================================================

fn testTokenAndCostBudgetGuards()! {
    let steps_buf: [16]TurnStepEntry = [TurnStepEntry{}; 16];
    let steps_s: []TurnStepEntry = steps_buf[0..];

    // 2a. Input token budget exceeded during recordLlmResponse
    let r1 = AgentTurnRunner.init(steps_s, TurnRunnerConfig{
        max_turns: 4,
        max_tool_calls: 0,
        max_input_tokens: 1000,
        max_output_tokens: 0,
        max_cost_micro_usd: 0,
        max_consecutive_errors: 0,
    }) ?? return error.InitFailed;

    expect_eq(r1.beginTurn(100) == null, true);
    const in_stop = r1.recordLlmResponse(1001, 50, 100, true, "too large", 150) ?? return error.ExpectedBudgetStop;
    expect_eq(in_stop == TurnStopReason.InputTokenBudgetExceeded, true);
    const out_in = r1.finish(in_stop, 200);
    expect_eq(out_in.isSuccess(), false);
    expect_eq(out_in.isBudgetExceeded(), true);

    // 2b. Exact-boundary input token usage: allowed to finish if !has_tool_call,
    //     blocked at next beginTurn if has_tool_call.
    r1.reset();
    expect_eq(r1.beginTurn(100) == null, true);
    expect_eq(r1.recordLlmResponse(1000, 50, 100, true, "at exact cap", 120) == null, true);
    const next_stop = r1.beginTurn(150) ?? return error.ExpectedPreTurnStop;
    expect_eq(next_stop == TurnStopReason.InputTokenBudgetExceeded, true);

    // 2c. Output token budget exceeded
    let r2 = AgentTurnRunner.init(steps_s, TurnRunnerConfig{
        max_turns: 4,
        max_tool_calls: 0,
        max_input_tokens: 0,
        max_output_tokens: 300,
        max_cost_micro_usd: 0,
        max_consecutive_errors: 0,
    }) ?? return error.InitFailed;
    expect_eq(r2.beginTurn(100) == null, true);
    const out_stop = r2.recordLlmResponse(200, 301, 100, false, "verbose", 150) ?? return error.ExpectedOutStop;
    expect_eq(out_stop == TurnStopReason.OutputTokenBudgetExceeded, true);

    // 2d. Cost micro-USD budget exceeded + saturating overflow check
    let r3 = AgentTurnRunner.init(steps_s, TurnRunnerConfig{
        max_turns: 4,
        max_tool_calls: 0,
        max_input_tokens: 0,
        max_output_tokens: 0,
        max_cost_micro_usd: 5000,
        max_consecutive_errors: 0,
    }) ?? return error.InitFailed;
    expect_eq(r3.beginTurn(100) == null, true);
    expect_eq(r3.recordLlmResponse(100, 50, 4000, true, "step1", 120) == null, true);
    expect_eq(r3.beginTurn(140) == null, true);
    const cost_stop = r3.recordLlmResponse(100, 50, 0xFFFFFFFFFFFFFFFF, true, "huge", 160) ?? return error.ExpectedCostStop;
    expect_eq(cost_stop == TurnStopReason.CostBudgetExceeded, true);
    const out_cost = r3.finish(cost_stop, 200);
    expect_eq(out_cost.total_cost_micro_usd, 0xFFFFFFFFFFFFFFFF);
    expect_eq(out_cost.isBudgetExceeded(), true);
}

// ============================================================================
// Test 3: Consecutive Tool Failures Circuit Breaker & Max Tool Calls Guard
// ============================================================================

fn testConsecutiveErrorsAndToolCap()! {
    let steps_buf: [20]TurnStepEntry = [TurnStepEntry{}; 20];
    let steps_s: []TurnStepEntry = steps_buf[0..];

    // 3a. Consecutive errors circuit breaker (streak resets on success)
    let r1 = AgentTurnRunner.init(steps_s, TurnRunnerConfig{
        max_turns: 5,
        max_tool_calls: 20,
        max_input_tokens: 0,
        max_output_tokens: 0,
        max_cost_micro_usd: 0,
        max_consecutive_errors: 2,
    }) ?? return error.InitFailed;

    expect_eq(r1.beginTurn(1000) == null, true);
    expect_eq(r1.recordLlmResponse(100, 50, 100, true, "call tools", 1010) == null, true);
    expect_eq(r1.recordToolCall("http_get", false, "502", 1020) == null, true);
    expect_eq(r1.consecutiveErrors(), 1);
    // Success resets consecutive error streak to 0.
    expect_eq(r1.recordToolCall("http_get", true, "200", 1030) == null, true);
    expect_eq(r1.consecutiveErrors(), 0);
    // Two consecutive failures now trip the circuit breaker.
    expect_eq(r1.recordToolCall("db_query", false, "timeout", 1040) == null, true);
    const err_stop = r1.recordToolCall("db_query", false, "timeout", 1050) ?? return error.ExpectedCircuitBreak;
    expect_eq(err_stop == TurnStopReason.ConsecutiveErrorsExceeded, true);

    const out_err = r1.finish(err_stop, 1060);
    expect_eq(out_err.tool_calls, 4);
    expect_eq(out_err.tool_errors, 3);
    expect_eq(out_err.consecutive_errors, 2);
    expect_eq(out_err.isSuccess(), false);
    expect_eq(out_err.isBudgetExceeded(), false);

    // 3b. Max tool calls reached
    let r2 = AgentTurnRunner.init(steps_s, TurnRunnerConfig{
        max_turns: 5,
        max_tool_calls: 2,
        max_input_tokens: 0,
        max_output_tokens: 0,
        max_cost_micro_usd: 0,
        max_consecutive_errors: 5,
    }) ?? return error.InitFailed;

    expect_eq(r2.beginTurn(2000) == null, true);
    expect_eq(r2.recordLlmResponse(100, 50, 100, true, "tools", 2010) == null, true);
    expect_eq(r2.recordToolCall("t1", true, "ok", 2020) == null, true);
    const cap_stop = r2.recordToolCall("t2", true, "ok", 2030) ?? return error.ExpectedToolCap;
    expect_eq(cap_stop == TurnStopReason.MaxToolCallsReached, true);
    const out_cap = r2.finish(cap_stop, 2040);
    expect_eq(out_cap.tool_calls, 2);
    expect_eq(out_cap.isBudgetExceeded(), true);
}

// ============================================================================
// Test 4: Max Turns Limit & Stopped-State Idempotency
// ============================================================================

fn testMaxTurnsAndStoppedIdempotency()! {
    let steps_buf: [16]TurnStepEntry = [TurnStepEntry{}; 16];
    let steps_s: []TurnStepEntry = steps_buf[0..];

    let runner = AgentTurnRunner.init(steps_s, TurnRunnerConfig{
        max_turns: 2,
        max_tool_calls: 10,
        max_input_tokens: 0,
        max_output_tokens: 0,
        max_cost_micro_usd: 0,
        max_consecutive_errors: 3,
    }) ?? return error.InitFailed;

    // Turn 1
    expect_eq(runner.beginTurn(100) == null, true);
    expect_eq(runner.recordLlmResponse(100, 40, 50, true, "t1", 110) == null, true);
    // Turn 2
    expect_eq(runner.beginTurn(200) == null, true);
    expect_eq(runner.recordLlmResponse(100, 40, 50, true, "t2", 210) == null, true);
    // Turn 3 -> MaxTurnsReached
    const stop = runner.beginTurn(300) ?? return error.ExpectedMaxTurns;
    expect_eq(stop == TurnStopReason.MaxTurnsReached, true);
    expect_eq(runner.currentTurn(), 2);

    // Subsequent calls while stopped must be idempotent and not mutate counters.
    const repeat1 = runner.beginTurn(310) ?? return error.ExpectedStopped;
    expect_eq(repeat1 == TurnStopReason.MaxTurnsReached, true);
    const repeat2 = runner.recordLlmResponse(999, 999, 999, false, "ignored", 320) ?? return error.ExpectedStopped;
    expect_eq(repeat2 == TurnStopReason.MaxTurnsReached, true);
    const repeat3 = runner.recordToolCall("ignored", false, "ignored", 330) ?? return error.ExpectedStopped;
    expect_eq(repeat3 == TurnStopReason.MaxTurnsReached, true);

    const outcome = runner.finish(stop, 350);
    expect_eq(outcome.turns_executed, 2);
    expect_eq(outcome.total_input_tokens, 200);
    expect_eq(outcome.tool_calls, 0);
    expect_eq(outcome.isBudgetExceeded(), true);
}

// ============================================================================
// Test 5: Step Buffer Full Boundary Protection
// ============================================================================

fn testStepBufferFullGuards()! {
    // Tight buffer of capacity 2: TurnStarted (1) + LlmResponse (2) fills it completely.
    let small_buf: [2]TurnStepEntry = [TurnStepEntry{}; 2];
    let small_s: []TurnStepEntry = small_buf[0..];

    let r1 = AgentTurnRunner.init(small_s, TurnRunnerConfig{
        max_turns: 5,
        max_tool_calls: 5,
    }) ?? return error.InitFailed;

    expect_eq(r1.beginTurn(100) == null, true);
    expect_eq(r1.recordLlmResponse(50, 20, 10, true, "need tool", 110) == null, true);
    // 3rd step (tool call) cannot fit -> StepBufferFull
    const full_tool = r1.recordToolCall("calc", true, "42", 120) ?? return error.ExpectedBufferFull;
    expect_eq(full_tool == TurnStopReason.StepBufferFull, true);

    // finish() attempts to append Stopped step; must safely drop without overflow.
    const out1 = r1.finish(full_tool, 150);
    expect_eq(out1.stop_reason == TurnStopReason.StepBufferFull, true);
    expect_eq(out1.steps.len, 2);
    expect_eq(out1.steps_dropped, 2); // 1 dropped tool step + 1 dropped Stopped step

    // Buffer of capacity 1: TurnStarted fits, LlmResponse triggers StepBufferFull.
    let tiny_buf: [1]TurnStepEntry = [TurnStepEntry{}; 1];
    let tiny_s: []TurnStepEntry = tiny_buf[0..];
    let r2 = AgentTurnRunner.init(tiny_s, TurnRunnerConfig{ max_turns: 5 }) ?? return error.InitFailed;
    expect_eq(r2.beginTurn(100) == null, true);
    const full_llm = r2.recordLlmResponse(50, 20, 10, false, "answer", 110) ?? return error.ExpectedBufferFull;
    expect_eq(full_llm == TurnStopReason.StepBufferFull, true);
    const out2 = r2.finish(full_llm, 120);
    expect_eq(out2.steps.len, 1);
    expect_eq(out2.steps_dropped, 2);
}

// ============================================================================
// Test 6: Invalid Init Guards, Clock Skew & Explicit Cancelled / LlmError Finish
// ============================================================================

fn testInitGuardsClockSkewAndExplicitFinish()! {
    let buf: [4]TurnStepEntry = [TurnStepEntry{}; 4];
    let s: []TurnStepEntry = buf[0..];
    let empty: []TurnStepEntry = buf[0..0];

    // max_turns == 0 -> null
    expect_eq(AgentTurnRunner.init(s, TurnRunnerConfig{ max_turns: 0 }) == null, true);
    // empty steps_buf -> null
    expect_eq(AgentTurnRunner.init(empty, TurnRunnerConfig{ max_turns: 3 }) == null, true);

    // Backwards clock skew (now_ms < started_at_ms) -> elapsed_ms == 0
    let runner = AgentTurnRunner.init(s, TurnRunnerConfig{ max_turns: 3 }) ?? return error.InitFailed;
    expect_eq(runner.beginTurn(5000) == null, true);
    const out_cancel = runner.finish(TurnStopReason.Cancelled, 4000);
    expect_eq(out_cancel.stop_reason == TurnStopReason.Cancelled, true);
    expect_eq(out_cancel.elapsed_ms, 0);
    expect_eq(out_cancel.isSuccess(), false);
    expect_eq(out_cancel.steps.len, 2);

    // Explicit LlmError finish without any turns executed
    runner.reset();
    const out_err = runner.finish(TurnStopReason.LlmError, 6000);
    expect_eq(out_err.stop_reason == TurnStopReason.LlmError, true);
    expect_eq(out_err.turns_executed, 0);
    expect_eq(out_err.elapsed_ms, 0);
    expect_eq(out_err.steps.len, 1);
    expect_eq(out_err.steps[0].kind == TurnStepKind.Stopped, true);
}

test("agent turn_runner: multi-turn tool convergence, audit trace & reset", testMultiTurnConvergenceAndReset);
test("agent turn_runner: token & micro-USD cost budget guards + overflow saturation", testTokenAndCostBudgetGuards);
test("agent turn_runner: consecutive tool errors circuit breaker & max tool calls", testConsecutiveErrorsAndToolCap);
test("agent turn_runner: max turns limit & stopped-state idempotency", testMaxTurnsAndStoppedIdempotency);
test("agent turn_runner: step buffer full boundary protection", testStepBufferFullGuards);
test("agent turn_runner: invalid init guards, clock skew & explicit finish", testInitGuardsClockSkewAndExplicitFinish);
