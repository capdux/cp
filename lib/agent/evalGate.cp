// Origin: Pure-CP Stdlib (AI Agent Automated CI Evaluation Gate & Baseline Regression Comparator) | Created: 2026-11-22
// Pure CP zero-heap-allocation evaluation gate for production AI Agent CI pipelines:
//   - Single-pass aggregation of pass rate, average micro-USD cost, tool error rate & exact P50/P95/max latency
//   - Multi-dimensional SLA verdict (`PassRateBelowMin`, `LatencySlaViolated`, `CostSlaViolated`, `ToolErrorRateExceeded`)
//   - Baseline regression detection (`compareWithBaselineGate` -> `BaselineRegression`)
//   - Zero-allocation Markdown/text CI summary renderer (`renderEvalGateSummary`) with strict buffer bounds

const std = @import("std");

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn satAddU64(a: u64, b: u64): u64 {
    if (a > 0xFFFFFFFFFFFFFFFF - b) {
        return 0xFFFFFFFFFFFFFFFF;
    }
    return a + b;
}

fn siftDownU64(arr: *[]u64, start: usize, end: usize) {
    let root: usize = start;
    while (root * 2 + 1 < end) {
        const left = root * 2 + 1;
        let swap_idx = root;
        if (arr[swap_idx] < arr[left]) {
            swap_idx = left;
        }
        if (left + 1 < end && arr[swap_idx] < arr[left + 1]) {
            swap_idx = left + 1;
        }
        if (swap_idx == root) return;
        const tmp = arr[root];
        arr[root] = arr[swap_idx];
        arr[swap_idx] = tmp;
        root = swap_idx;
    }
}

fn heapSortU64(arr: *[]u64, len: usize) {
    if (len < 2) return;
    let start: usize = len / 2;
    while (start > 0) {
        start -= 1;
        siftDownU64(arr, start, len);
    }
    let end: usize = len;
    while (end > 1) {
        end -= 1;
        const tmp = arr[end];
        arr[end] = arr[0];
        arr[0] = tmp;
        siftDownU64(arr, 0, end);
    }
}

fn nearestRankIndex(len: usize, pct: usize): usize {
    if (len <= 1) return 0;
    const rank = (pct * len + 99) / 100;
    if (rank == 0) return 0;
    const idx = rank - 1;
    if (idx >= len) return len - 1;
    return idx;
}

/// CI gate SLA thresholds for evaluating an AI Agent benchmark suite.
/// `max_p95_latency_ms == 0` and `max_avg_cost_micro_usd == 0` disable those respective checks.
pub struct EvalGateThresholds {
    min_pass_rate: f64 = 0.90;
    max_p95_latency_ms: u64 = 0;
    max_avg_cost_micro_usd: u64 = 0;
    max_tool_error_rate: f64 = 1.0;
    max_pass_rate_drop: f64 = 0.02;
}

/// Single benchmark case execution result fed into `evaluateAgentGate`.
pub struct EvalCaseResult {
    case_id: u32 = 0;
    passed: bool = false;
    latency_ms: u64 = 0;
    cost_micro_usd: u64 = 0;
    tool_calls: u32 = 0;
    tool_errors: u32 = 0;
}

/// Deterministic CI gate decision returned inside `EvalGateReport`.
pub enum EvalGateVerdict {
    Passed,
    PassRateBelowMin,
    LatencySlaViolated,
    CostSlaViolated,
    ToolErrorRateExceeded,
    BaselineRegression,
}

/// Complete aggregated CI gate report produced by `evaluateAgentGate` or `compareWithBaselineGate`.
pub struct EvalGateReport {
    verdict: EvalGateVerdict = EvalGateVerdict.Passed;
    total_cases: usize = 0;
    passed_cases: usize = 0;
    failed_cases: usize = 0;
    pass_rate: f64 = 0.0;
    baseline_pass_rate: f64 = 0.0;
    pass_rate_delta: f64 = 0.0;
    has_baseline: bool = false;
    p50_latency_ms: u64 = 0;
    p95_latency_ms: u64 = 0;
    max_latency_ms: u64 = 0;
    total_cost_micro_usd: u64 = 0;
    avg_cost_micro_usd: u64 = 0;
    total_tool_calls: u64 = 0;
    total_tool_errors: u64 = 0;
    tool_error_rate: f64 = 0.0;

    /// Returns `true` iff the CI gate verdict is `EvalGateVerdict.Passed`.
    fn isPassed(): bool {
        return _.verdict == EvalGateVerdict.Passed;
    }
}

fn thresholdsValid(t: EvalGateThresholds): bool {
    if (!isFiniteF64(t.min_pass_rate) || t.min_pass_rate < 0.0 || t.min_pass_rate > 1.0) {
        return false;
    }
    if (!isFiniteF64(t.max_tool_error_rate) || t.max_tool_error_rate < 0.0 || t.max_tool_error_rate > 1.0) {
        return false;
    }
    if (!isFiniteF64(t.max_pass_rate_drop) || t.max_pass_rate_drop < 0.0 || t.max_pass_rate_drop > 1.0) {
        return false;
    }
    return true;
}

/// Evaluates `cases` against `thresholds` using caller-owned `latency_scratch` (must have `len >= cases.len`).
/// Returns `null` if `cases.len == 0`, `latency_scratch.len < cases.len`, or `thresholds` has invalid/NaN bounds.
pub fn evaluateAgentGate(
    cases: []const EvalCaseResult,
    thresholds: EvalGateThresholds,
    latency_scratch: *[]u64,
): EvalGateReport? {
    if (cases.len == 0 || latency_scratch.len < cases.len) return null;
    if (!thresholdsValid(thresholds)) return null;

    let passed_cases: usize = 0;
    let total_cost: u64 = 0;
    let total_tool_calls: u64 = 0;
    let total_tool_errors: u64 = 0;

    let i: usize = 0;
    while (i < cases.len) : (i += 1) {
        if (cases[i].passed) {
            passed_cases += 1;
        }
        latency_scratch[i] = cases[i].latency_ms;
        total_cost = satAddU64(total_cost, cases[i].cost_micro_usd);
        total_tool_calls = satAddU64(total_tool_calls, cases[i].tool_calls as u64);
        total_tool_errors = satAddU64(total_tool_errors, cases[i].tool_errors as u64);
    }

    heapSortU64(latency_scratch, cases.len);

    const p50 = latency_scratch[nearestRankIndex(cases.len, 50)];
    const p95 = latency_scratch[nearestRankIndex(cases.len, 95)];
    const max_lat = latency_scratch[cases.len - 1];

    const pass_rate: f64 = (passed_cases as f64) / (cases.len as f64);
    const avg_cost: u64 = total_cost / (cases.len as u64);
    let tool_err_rate: f64 = 0.0;
    if (total_tool_calls > 0) {
        const clamped_errs = if (total_tool_errors > total_tool_calls)
            total_tool_calls
        else
            total_tool_errors;
        tool_err_rate = (clamped_errs as f64) / (total_tool_calls as f64);
    } else if (total_tool_errors > 0) {
        tool_err_rate = 1.0;
    }

    const eps: f64 = 0.000000000001;
    let verdict: EvalGateVerdict = EvalGateVerdict.Passed;
    if (pass_rate + eps < thresholds.min_pass_rate) {
        verdict = EvalGateVerdict.PassRateBelowMin;
    } else if (thresholds.max_p95_latency_ms > 0 && p95 > thresholds.max_p95_latency_ms) {
        verdict = EvalGateVerdict.LatencySlaViolated;
    } else if (thresholds.max_avg_cost_micro_usd > 0 && avg_cost > thresholds.max_avg_cost_micro_usd) {
        verdict = EvalGateVerdict.CostSlaViolated;
    } else if (tool_err_rate - eps > thresholds.max_tool_error_rate) {
        verdict = EvalGateVerdict.ToolErrorRateExceeded;
    }

    return EvalGateReport{
        verdict: verdict,
        total_cases: cases.len,
        passed_cases: passed_cases,
        failed_cases: cases.len - passed_cases,
        pass_rate: pass_rate,
        baseline_pass_rate: 0.0,
        pass_rate_delta: 0.0,
        has_baseline: false,
        p50_latency_ms: p50,
        p95_latency_ms: p95,
        max_latency_ms: max_lat,
        total_cost_micro_usd: total_cost,
        avg_cost_micro_usd: avg_cost,
        total_tool_calls: total_tool_calls,
        total_tool_errors: total_tool_errors,
        tool_error_rate: tool_err_rate,
    };
}

/// Evaluates `candidate_cases` against `thresholds` and checks for pass-rate regression
/// relative to `baseline_cases` (`baseline_pass_rate - candidate_pass_rate > max_pass_rate_drop`).
/// Returns `null` if either slice is empty, scratch buffer is too small, or thresholds are invalid.
pub fn compareWithBaselineGate(
    candidate_cases: []const EvalCaseResult,
    baseline_cases: []const EvalCaseResult,
    thresholds: EvalGateThresholds,
    latency_scratch: *[]u64,
): EvalGateReport? {
    if (baseline_cases.len == 0) return null;
    let report = evaluateAgentGate(candidate_cases, thresholds, latency_scratch) ?? return null;

    let base_passed: usize = 0;
    let i: usize = 0;
    while (i < baseline_cases.len) : (i += 1) {
        if (baseline_cases[i].passed) {
            base_passed += 1;
        }
    }

    const base_rate: f64 = (base_passed as f64) / (baseline_cases.len as f64);
    report.has_baseline = true;
    report.baseline_pass_rate = base_rate;
    report.pass_rate_delta = report.pass_rate - base_rate;

    if (report.verdict == EvalGateVerdict.Passed) {
        const drop = base_rate - report.pass_rate;
        if (drop > thresholds.max_pass_rate_drop + 0.000000000001) {
            report.verdict = EvalGateVerdict.BaselineRegression;
        }
    }
    return report;
}

// ============================================================================
// Zero-Heap-Allocation CI Markdown Summary Renderer
// ============================================================================

fn appendBytes(out: *[]u8, pos: *usize, src: []const u8): bool {
    if (pos.* > out.len || src.len > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos.* + i] = src[i];
    }
    pos.* += src.len;
    return true;
}

fn appendU64(out: *[]u8, pos: *usize, v: u64): bool {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let cur: u64 = v;
    if (cur == 0) {
        tmp[0] = 48; // '0'
        n = 1;
    } else {
        while (cur > 0) {
            tmp[n] = (48 + (cur % 10)) as u8;
            cur /= 10;
            n += 1;
        }
    }
    if (pos.* > out.len || n > out.len - pos.*) return false;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos.* + i] = tmp[n - 1 - i];
    }
    pos.* += n;
    return true;
}

fn appendPct2(out: *[]u8, pos: *usize, ratio: f64): bool {
    let clamped = ratio;
    if (!isFiniteF64(clamped) || clamped < 0.0) clamped = 0.0;
    if (clamped > 1.0) clamped = 1.0;
    const bp: u64 = ((clamped * 10000.0) + 0.5) as u64;
    const whole = bp / 100;
    const frac = bp % 100;
    if (!appendU64(out, pos, whole)) return false;
    if (!appendBytes(out, pos, ".")) return false;
    if (frac < 10) {
        if (!appendBytes(out, pos, "0")) return false;
    }
    if (!appendU64(out, pos, frac)) return false;
    return appendBytes(out, pos, "%");
}

fn appendSignedPct2(out: *[]u8, pos: *usize, delta: f64): bool {
    if (!isFiniteF64(delta)) {
        return appendBytes(out, pos, "+0.00%");
    }
    if (delta < 0.0) {
        if (!appendBytes(out, pos, "-")) return false;
        return appendPct2(out, pos, -delta);
    }
    if (!appendBytes(out, pos, "+")) return false;
    return appendPct2(out, pos, delta);
}

fn verdictLabel(v: EvalGateVerdict): []const u8 {
    match (v) {
        EvalGateVerdict.Passed => return "PASSED",
        EvalGateVerdict.PassRateBelowMin => return "FAILED (PassRateBelowMin)",
        EvalGateVerdict.LatencySlaViolated => return "FAILED (LatencySlaViolated)",
        EvalGateVerdict.CostSlaViolated => return "FAILED (CostSlaViolated)",
        EvalGateVerdict.ToolErrorRateExceeded => return "FAILED (ToolErrorRateExceeded)",
        EvalGateVerdict.BaselineRegression => return "FAILED (BaselineRegression)",
    }
}

/// Renders a concise CI-friendly Markdown summary of `report` into caller-owned `out_buf`.
/// Returns the number of bytes written, or `null` if `out_buf` is too small.
pub fn renderEvalGateSummary(report: EvalGateReport, out_buf: *[]u8): usize? {
    if (out_buf.len == 0) return null;
    let pos: usize = 0;

    if (!appendBytes(out_buf, &pos, "### Agent Eval Gate: ")) return null;
    if (!appendBytes(out_buf, &pos, verdictLabel(report.verdict))) return null;
    if (!appendBytes(out_buf, &pos, "\n- Cases: ")) return null;
    if (!appendU64(out_buf, &pos, report.passed_cases as u64)) return null;
    if (!appendBytes(out_buf, &pos, "/")) return null;
    if (!appendU64(out_buf, &pos, report.total_cases as u64)) return null;
    if (!appendBytes(out_buf, &pos, " (")) return null;
    if (!appendPct2(out_buf, &pos, report.pass_rate)) return null;
    if (!appendBytes(out_buf, &pos, ")\n")) return null;

    if (report.has_baseline) {
        if (!appendBytes(out_buf, &pos, "- Baseline: ")) return null;
        if (!appendPct2(out_buf, &pos, report.baseline_pass_rate)) return null;
        if (!appendBytes(out_buf, &pos, " (delta ")) return null;
        if (!appendSignedPct2(out_buf, &pos, report.pass_rate_delta)) return null;
        if (!appendBytes(out_buf, &pos, ")\n")) return null;
    }

    if (!appendBytes(out_buf, &pos, "- Latency: p50=")) return null;
    if (!appendU64(out_buf, &pos, report.p50_latency_ms)) return null;
    if (!appendBytes(out_buf, &pos, "ms, p95=")) return null;
    if (!appendU64(out_buf, &pos, report.p95_latency_ms)) return null;
    if (!appendBytes(out_buf, &pos, "ms, max=")) return null;
    if (!appendU64(out_buf, &pos, report.max_latency_ms)) return null;
    if (!appendBytes(out_buf, &pos, "ms\n- Cost: avg=")) return null;
    if (!appendU64(out_buf, &pos, report.avg_cost_micro_usd)) return null;
    if (!appendBytes(out_buf, &pos, " micro_usd, total=")) return null;
    if (!appendU64(out_buf, &pos, report.total_cost_micro_usd)) return null;
    if (!appendBytes(out_buf, &pos, " micro_usd\n- Tool Errors: ")) return null;
    if (!appendU64(out_buf, &pos, report.total_tool_errors)) return null;
    if (!appendBytes(out_buf, &pos, "/")) return null;
    if (!appendU64(out_buf, &pos, report.total_tool_calls)) return null;
    if (!appendBytes(out_buf, &pos, " (")) return null;
    if (!appendPct2(out_buf, &pos, report.tool_error_rate)) return null;
    if (!appendBytes(out_buf, &pos, ")\n")) return null;

    return pos;
}
