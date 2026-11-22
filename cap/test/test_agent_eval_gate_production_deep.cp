// Origin: Pure-CP Stdlib Test Suite (lib/agent/eval_gate.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Automated Agent CI Eval Gate:
//   1. All-passing benchmark with exact unsorted P50/P95/max latency & cost/tool aggregation
//   2. Individual SLA violation verdicts: PassRateBelowMin, LatencySlaViolated, CostSlaViolated, ToolErrorRateExceeded
//   3. Baseline regression detection (BaselineRegression vs within-tolerance Passed)
//   4. Zero-alloc Markdown CI summary renderer & strict buffer-overflow protection
//   5. Empty input, undersized scratch & invalid/NaN threshold guards
//   6. Zero-tool-call edge cases & saturating cost overflow protection

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    EvalGateThresholds,
    EvalCaseResult,
    EvalGateVerdict,
    EvalGateReport,
    evaluateAgentGate,
    compareWithBaselineGate,
    renderEvalGateSummary,
} from "cap:agent";

// ============================================================================
// Test 1: All-Passing Suite & Exact Unsorted P50 / P95 / Max Latency
// ============================================================================

fn testPassingSuiteAndExactP95()! {
    // 20 unsorted cases with latencies 100..2000 (step 100):
    // Sorted: 100, 200, ..., 1000 (p50, idx 9), ..., 1900 (p95, idx 18), 2000 (max, idx 19).
    let cases: [20]EvalCaseResult = [EvalCaseResult{}; 20];
    let i: usize = 0;
    while (i < 20) : (i += 1) {
        // Scramble order using (i * 7 + 3) % 20
        const rank: u64 = (((i * 7 + 3) % 20) + 1) as u64;
        cases[i] = EvalCaseResult{
            case_id: (i + 1) as u32,
            passed: i < 19, // 19/20 = 95%
            latency_ms: rank * 100,
            cost_micro_usd: 1000,
            tool_calls: 2,
            tool_errors: if (i == 0) 1 else 0, // 1/40 = 2.5%
        };
    }

    let scratch: [20]u64 = [0; 20];
    let scratch_s: []u64 = scratch[0..];
    const thresholds = EvalGateThresholds{
        min_pass_rate: 0.90,
        max_p95_latency_ms: 1900,
        max_avg_cost_micro_usd: 1000,
        max_tool_error_rate: 0.05,
        max_pass_rate_drop: 0.02,
    };

    const report: EvalGateReport = cap.agent.evaluateAgentGate(cases[0..], thresholds, scratch_s) ?? return error.EvalFailed;
    expect_eq(report.isPassed(), true);
    expect_eq(report.total_cases, 20);
    expect_eq(report.passed_cases, 19);
    expect_eq(report.failed_cases, 1);
    expect_eq(report.p50_latency_ms, 1000);
    expect_eq(report.p95_latency_ms, 1900);
    expect_eq(report.max_latency_ms, 2000);
    expect_eq(report.avg_cost_micro_usd, 1000);
    expect_eq(report.total_cost_micro_usd, 20000);
    expect_eq(report.total_tool_calls, 40);
    expect_eq(report.total_tool_errors, 1);
    expect_eq(report.has_baseline, false);
}

// ============================================================================
// Test 2: Individual SLA Violation Verdicts
// ============================================================================

fn testIndividualSlaViolations()! {
    let scratch: [4]u64 = [0; 4];
    let scratch_s: []u64 = scratch[0..];

    // 2a. PassRateBelowMin (2/4 = 50% < 80%)
    const low_pass: [4]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 100, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 110, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 },
        EvalCaseResult{ case_id: 3, passed: false, latency_ms: 120, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 },
        EvalCaseResult{ case_id: 4, passed: false, latency_ms: 130, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 },
    ];
    const r_pass = evaluateAgentGate(low_pass[0..], EvalGateThresholds{ min_pass_rate: 0.80 }, scratch_s) ?? return error.EvalFailed;
    expect_eq(r_pass.verdict == EvalGateVerdict.PassRateBelowMin, true);
    expect_eq(r_pass.isPassed(), false);

    // 2b. LatencySlaViolated (p95 = 900ms > 500ms)
    const slow_cases: [4]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 100, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 150, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 },
        EvalCaseResult{ case_id: 3, passed: true, latency_ms: 200, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 },
        EvalCaseResult{ case_id: 4, passed: true, latency_ms: 900, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 },
    ];
    const r_lat = evaluateAgentGate(slow_cases[0..], EvalGateThresholds{
        min_pass_rate: 0.90,
        max_p95_latency_ms: 500,
    }, scratch_s) ?? return error.EvalFailed;
    expect_eq(r_lat.verdict == EvalGateVerdict.LatencySlaViolated, true);

    // 2c. CostSlaViolated (avg = 2500 > 1500)
    const pricey_cases: [2]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 100, cost_micro_usd: 2000, tool_calls: 1, tool_errors: 0 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 120, cost_micro_usd: 3000, tool_calls: 1, tool_errors: 0 },
    ];
    const r_cost = evaluateAgentGate(pricey_cases[0..], EvalGateThresholds{
        min_pass_rate: 0.90,
        max_avg_cost_micro_usd: 1500,
    }, scratch_s) ?? return error.EvalFailed;
    expect_eq(r_cost.verdict == EvalGateVerdict.CostSlaViolated, true);

    // 2d. ToolErrorRateExceeded (2/4 = 50% > 20%)
    const flaky_tools: [2]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 100, cost_micro_usd: 500, tool_calls: 2, tool_errors: 1 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 120, cost_micro_usd: 500, tool_calls: 2, tool_errors: 1 },
    ];
    const r_tool = evaluateAgentGate(flaky_tools[0..], EvalGateThresholds{
        min_pass_rate: 0.90,
        max_tool_error_rate: 0.20,
    }, scratch_s) ?? return error.EvalFailed;
    expect_eq(r_tool.verdict == EvalGateVerdict.ToolErrorRateExceeded, true);
}

// ============================================================================
// Test 3: Baseline Regression Detection
// ============================================================================

fn testBaselineRegressionDetection()! {
    let baseline: [10]EvalCaseResult = [EvalCaseResult{}; 10];
    let candidate: [10]EvalCaseResult = [EvalCaseResult{}; 10];
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        baseline[i] = EvalCaseResult{ case_id: (i + 1) as u32, passed: true, latency_ms: 100, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 };
        // Candidate passes 9/10 (90%): meets min_pass_rate=0.85, but drops 10% vs baseline 100%.
        candidate[i] = EvalCaseResult{ case_id: (i + 1) as u32, passed: i < 9, latency_ms: 100, cost_micro_usd: 500, tool_calls: 1, tool_errors: 0 };
    }

    let scratch: [10]u64 = [0; 10];
    let scratch_s: []u64 = scratch[0..];

    // Strict drop tolerance (2%) -> BaselineRegression
    const strict_res = compareWithBaselineGate(candidate[0..], baseline[0..], EvalGateThresholds{
        min_pass_rate: 0.85,
        max_pass_rate_drop: 0.02,
    }, scratch_s) ?? return error.CompareFailed;
    expect_eq(strict_res.verdict == EvalGateVerdict.BaselineRegression, true);
    expect_eq(strict_res.has_baseline, true);
    expect_eq(strict_res.isPassed(), false);

    // Relaxed drop tolerance (10%) -> Passed
    const relaxed_res = compareWithBaselineGate(candidate[0..], baseline[0..], EvalGateThresholds{
        min_pass_rate: 0.85,
        max_pass_rate_drop: 0.10,
    }, scratch_s) ?? return error.CompareFailed;
    expect_eq(relaxed_res.verdict == EvalGateVerdict.Passed, true);
    expect_eq(relaxed_res.isPassed(), true);
}

// ============================================================================
// Test 4: Markdown CI Summary Rendering & Buffer Overflow Protection
// ============================================================================

fn testMarkdownSummaryAndBufferBounds()! {
    const baseline: [2]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 100, cost_micro_usd: 800, tool_calls: 2, tool_errors: 0 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 200, cost_micro_usd: 1200, tool_calls: 2, tool_errors: 0 },
    ];
    const candidate: [2]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 110, cost_micro_usd: 900, tool_calls: 2, tool_errors: 0 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 210, cost_micro_usd: 1100, tool_calls: 2, tool_errors: 0 },
    ];
    let scratch: [4]u64 = [0; 4];
    let scratch_s: []u64 = scratch[0..];
    const report = compareWithBaselineGate(candidate[0..], baseline[0..], EvalGateThresholds{
        min_pass_rate: 0.90,
        max_p95_latency_ms: 500,
        max_avg_cost_micro_usd: 2000,
        max_tool_error_rate: 0.10,
        max_pass_rate_drop: 0.02,
    }, scratch_s) ?? return error.CompareFailed;

    // Render into ample buffer
    let out_buf: [512]u8 = [0; 512];
    let out_s: []u8 = out_buf[0..];
    const written = renderEvalGateSummary(report, out_s) ?? return error.RenderFailed;
    const text = out_s[0..written];
    expect_eq(std.mem.indexOf(u8, text, "### Agent Eval Gate: PASSED") != null, true);
    expect_eq(std.mem.indexOf(u8, text, "- Cases: 2/2 (100.00%)") != null, true);
    expect_eq(std.mem.indexOf(u8, text, "- Baseline: 100.00% (delta +0.00%)") != null, true);
    expect_eq(std.mem.indexOf(u8, text, "p95=210ms") != null, true);
    expect_eq(std.mem.indexOf(u8, text, "avg=1000 micro_usd") != null, true);

    // Too-small buffer and zero-length buffer must return null without overflow
    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..];
    let empty_out: []u8 = tiny[0..0];
    expect_eq(renderEvalGateSummary(report, tiny_s) == null, true);
    expect_eq(renderEvalGateSummary(report, empty_out) == null, true);
}

// ============================================================================
// Test 5: Empty Input, Undersized Scratch & Invalid/NaN Threshold Guards
// ============================================================================

fn testInvalidInputsAndNaNGuards()! {
    const sample: [2]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 100, cost_micro_usd: 100 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 200, cost_micro_usd: 200 },
    ];
    const empty_cases: []const EvalCaseResult = sample[0..0];
    let scratch: [4]u64 = [0; 4];
    let scratch_ok: []u64 = scratch[0..];
    let scratch_small: []u64 = scratch[0..1];

    // Empty cases / baseline / undersized scratch -> null
    expect_eq(evaluateAgentGate(empty_cases, EvalGateThresholds{}, scratch_ok) == null, true);
    expect_eq(evaluateAgentGate(sample[0..], EvalGateThresholds{}, scratch_small) == null, true);
    expect_eq(compareWithBaselineGate(sample[0..], empty_cases, EvalGateThresholds{}, scratch_ok) == null, true);

    // Invalid thresholds (out of [0, 1] or NaN) -> null
    expect_eq(evaluateAgentGate(sample[0..], EvalGateThresholds{ min_pass_rate: -0.1 }, scratch_ok) == null, true);
    expect_eq(evaluateAgentGate(sample[0..], EvalGateThresholds{ min_pass_rate: 1.1 }, scratch_ok) == null, true);
    expect_eq(evaluateAgentGate(sample[0..], EvalGateThresholds{ min_pass_rate: std.math.nan(f64) }, scratch_ok) == null, true);
    expect_eq(evaluateAgentGate(sample[0..], EvalGateThresholds{ max_tool_error_rate: 1.5 }, scratch_ok) == null, true);
    expect_eq(evaluateAgentGate(sample[0..], EvalGateThresholds{ max_pass_rate_drop: -0.05 }, scratch_ok) == null, true);
}

// ============================================================================
// Test 6: Zero Tool Calls Edge Cases & Saturating Cost Overflow
// ============================================================================

fn testZeroToolsAndCostSaturation()! {
    let scratch: [2]u64 = [0; 2];
    let scratch_s: []u64 = scratch[0..];

    // Zero tool calls and huge costs saturate cleanly
    const huge_cases: [2]EvalCaseResult = [
        EvalCaseResult{ case_id: 1, passed: true, latency_ms: 50, cost_micro_usd: 0xFFFFFFFFFFFFFFFF, tool_calls: 0, tool_errors: 0 },
        EvalCaseResult{ case_id: 2, passed: true, latency_ms: 70, cost_micro_usd: 1000, tool_calls: 0, tool_errors: 0 },
    ];
    const r = evaluateAgentGate(huge_cases[0..], EvalGateThresholds{ min_pass_rate: 0.90 }, scratch_s) ?? return error.EvalFailed;
    expect_eq(r.isPassed(), true);
    expect_eq(r.total_cost_micro_usd, 0xFFFFFFFFFFFFFFFF);
    expect_eq(r.total_tool_calls, 0);
    expect_eq(r.tool_error_rate == 0.0, true);
}

test("agent eval_gate: all-passing benchmark & exact unsorted P50/P95/max latency", testPassingSuiteAndExactP95);
test("agent eval_gate: individual SLA violation verdicts", testIndividualSlaViolations);
test("agent eval_gate: baseline regression detection vs within-tolerance pass", testBaselineRegressionDetection);
test("agent eval_gate: zero-alloc Markdown CI summary & buffer overflow protection", testMarkdownSummaryAndBufferBounds);
test("agent eval_gate: empty input, undersized scratch & invalid/NaN threshold guards", testInvalidInputsAndNaNGuards);
test("agent eval_gate: zero tool calls & saturating cost overflow protection", testZeroToolsAndCostSaturation);
