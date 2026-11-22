// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Agent Trajectory Contract Assertion & CI Suite Verifier) | Created: 2026-11-22
// 100% pure CP zero-heap-allocation AI Agent tool-call trajectory contract verifier and CI regression gate:
//   - Compact `TraceToolStep` events (`tool_id_hash`, `args_hash`, `succeeded`, `latency_ms`, `tokens_used`).
//   - 4 trajectory matching modes: `exact_sequence`, `ordered_subsequence`, `unordered_subset`, `any_of`.
//   - Forbidden tool blacklist enforcement (`forbidden_tool_invoked`), optional strict `args_hash` checks,
//     and hard budgets for max steps, total latency (ms), and total tokens.
//   - Batch assertion runner (`evaluateTraceBatch`) and multi-case CI gate accumulator (`TraceSuiteAccumulator`
//     with `pass_rate_bps` & `meetsMinPassRate`).
//   - Full boundary guards: empty expected sets, empty actual traces, zero-budget unlimited semantics,
//     and step-limit overflow protection.

// ============================================================================
// Constants & Hash Utility
// ============================================================================

pub const TRACE_EVAL_MAX_EXPECTED: usize = 32;
pub const TRACE_EVAL_MAX_FORBIDDEN: usize = 32;
pub const TRACE_EVAL_MAX_STEPS: usize = 128;
pub const TRACE_EVAL_BPS_DENOM: u32 = 10_000;

/// Deterministic 64-bit FNV-1a hash helper for tool names and canonicalized argument payloads.
pub fn traceToolHash(bytes: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        h = (h ^ (bytes[i] as u64)) *% 0x100000001b3;
    }
    return h;
}

// ============================================================================
// 1. Enums & Data Models
// ============================================================================

/// Trajectory matching mode for a `TraceContract`.
pub enum TraceMatchMode {
    exact_sequence,
    ordered_subsequence,
    unordered_subset,
    any_of,
}

/// Primary failure reason reported by `evaluateTraceContract`.
pub enum TraceFailReason {
    none,
    forbidden_tool_invoked,
    step_budget_exceeded,
    latency_budget_exceeded,
    token_budget_exceeded,
    sequence_mismatch,
    missing_expected_tool,
    args_hash_mismatch,
    step_limit_overflow,
}

/// Single observed tool invocation step in an Agent execution trajectory.
pub struct TraceToolStep {
    tool_id_hash: u64 = 0;
    args_hash: u64 = 0;
    succeeded: bool = true;
    latency_ms: u32 = 0;
    tokens_used: u32 = 0;
}

/// Single expected tool milestone within a `TraceContract`.
pub struct TraceExpectedTool {
    tool_id_hash: u64 = 0;
    require_args_hash: bool = false;
    expected_args_hash: u64 = 0;
    require_success: bool = true;
}

/// Declarative zero-alloc assertion contract for an Agent tool-call trajectory.
pub struct TraceContract {
    mode: TraceMatchMode = TraceMatchMode.ordered_subsequence;
    expected: [32]TraceExpectedTool = [TraceExpectedTool{}; 32];
    expected_count: usize = 0;
    forbidden_tools: [32]u64 = [0; 32];
    forbidden_count: usize = 0;
    max_steps: usize = 0;
    max_total_latency_ms: u64 = 0;
    max_total_tokens: u64 = 0;

    static fn init(mode: TraceMatchMode): TraceContract {
        return TraceContract{
            mode: mode,
        };
    }

    /// Appends an expected tool (requiring `succeeded == true`, any `args_hash`).
    /// Returns `false` if the 32-item expected table is full.
    fn addExpected(*_, tool_id_hash: u64): bool {
        if (_.expected_count >= TRACE_EVAL_MAX_EXPECTED) {
            return false;
        }
        _.expected[_.expected_count] = TraceExpectedTool{
            tool_id_hash: tool_id_hash,
            require_args_hash: false,
            expected_args_hash: 0,
            require_success: true,
        };
        _.expected_count += 1;
        return true;
    }

    /// Appends an expected tool requiring a specific `args_hash` fingerprint.
    /// Returns `false` if the 32-item expected table is full.
    fn addExpectedWithArgs(
        *_,
        tool_id_hash: u64,
        expected_args_hash: u64,
        require_success: bool,
    ): bool {
        if (_.expected_count >= TRACE_EVAL_MAX_EXPECTED) {
            return false;
        }
        _.expected[_.expected_count] = TraceExpectedTool{
            tool_id_hash: tool_id_hash,
            require_args_hash: true,
            expected_args_hash: expected_args_hash,
            require_success: require_success,
        };
        _.expected_count += 1;
        return true;
    }

    /// Adds a forbidden tool hash to the blacklist. Returns `false` if the 32-item blacklist is full.
    fn addForbidden(*_, tool_id_hash: u64): bool {
        if (_.forbidden_count >= TRACE_EVAL_MAX_FORBIDDEN) {
            return false;
        }
        _.forbidden_tools[_.forbidden_count] = tool_id_hash;
        _.forbidden_count += 1;
        return true;
    }

    /// Configures hard regression budgets (`0` disables the corresponding budget check).
    fn setBudgets(*_, max_steps: usize, max_total_latency_ms: u64, max_total_tokens: u64) {
        _.max_steps = max_steps;
        _.max_total_latency_ms = max_total_latency_ms;
        _.max_total_tokens = max_total_tokens;
    }
}

/// Detailed pass/fail assertion report for a single trajectory evaluation.
pub struct TraceAssertionReport {
    passed: bool = false;
    fail_reason: TraceFailReason = TraceFailReason.none;
    total_steps: usize = 0;
    succeeded_steps: usize = 0;
    failed_steps: usize = 0;
    matched_expected: usize = 0;
    expected_total: usize = 0;
    total_latency_ms: u64 = 0;
    total_tokens: u64 = 0;
    offending_step_idx: usize = 0;
    offending_tool_hash: u64 = 0;
}

/// Aggregate CI suite summary produced by `TraceSuiteAccumulator`.
pub struct TraceSuiteSummary {
    total_cases: usize = 0;
    passed_cases: usize = 0;
    failed_cases: usize = 0;
    pass_rate_bps: u32 = 0;
    forbidden_failures: usize = 0;
    budget_failures: usize = 0;
    contract_failures: usize = 0;
    total_latency_ms: u64 = 0;
    total_tokens: u64 = 0;
}

// ============================================================================
// Internal Matching Helpers
// ============================================================================

fn isForbiddenTool(contract: TraceContract, tool_hash: u64): bool {
    let i: usize = 0;
    while (i < contract.forbidden_count) : (i += 1) {
        if (contract.forbidden_tools[i] == tool_hash) {
            return true;
        }
    }
    return false;
}

fn stepMatchesExpected(step: TraceToolStep, exp: TraceExpectedTool): bool {
    if (step.tool_id_hash != exp.tool_id_hash) {
        return false;
    }
    if (exp.require_success && !step.succeeded) {
        return false;
    }
    if (exp.require_args_hash && step.args_hash != exp.expected_args_hash) {
        return false;
    }
    return true;
}

fn stepMatchesToolOnly(step: TraceToolStep, exp: TraceExpectedTool): bool {
    if (step.tool_id_hash != exp.tool_id_hash) {
        return false;
    }
    if (exp.require_success && !step.succeeded) {
        return false;
    }
    return true;
}

// ============================================================================
// 2. Core Contract Assertion Evaluator & Batch Runner
// ============================================================================

/// Evaluates `steps` against `contract` and produces a detailed `TraceAssertionReport`.
/// Failure precedence:
///   1. `step_limit_overflow` (`steps.len > TRACE_EVAL_MAX_STEPS`)
///   2. `forbidden_tool_invoked`
///   3. Hard budgets (`step_budget_exceeded` -> `latency_budget_exceeded` -> `token_budget_exceeded`)
///   4. Trajectory contract matching (`exact_sequence` | `ordered_subsequence` | `unordered_subset` | `any_of`)
pub fn evaluateTraceContract(
    contract: TraceContract,
    steps: []const TraceToolStep,
): TraceAssertionReport {
    let rep = TraceAssertionReport{
        passed: false,
        fail_reason: TraceFailReason.none,
        total_steps: steps.len,
        expected_total: contract.expected_count,
    };

    if (steps.len > TRACE_EVAL_MAX_STEPS) {
        rep.fail_reason = TraceFailReason.step_limit_overflow;
        return rep;
    }

    // 1. Single-pass telemetry aggregation & forbidden-tool scan
    let found_forbidden: bool = false;
    let forb_idx: usize = 0;
    let forb_hash: u64 = 0;

    let i: usize = 0;
    while (i < steps.len) : (i += 1) {
        const st = steps[i];
        if (st.succeeded) {
            rep.succeeded_steps += 1;
        } else {
            rep.failed_steps += 1;
        }
        rep.total_latency_ms += st.latency_ms as u64;
        rep.total_tokens += st.tokens_used as u64;

        if (!found_forbidden && isForbiddenTool(contract, st.tool_id_hash)) {
            found_forbidden = true;
            forb_idx = i;
            forb_hash = st.tool_id_hash;
        }
    }

    if (found_forbidden) {
        rep.fail_reason = TraceFailReason.forbidden_tool_invoked;
        rep.offending_step_idx = forb_idx;
        rep.offending_tool_hash = forb_hash;
        return rep;
    }

    // 2. Hard budget checks (0 = unlimited)
    if (contract.max_steps > 0 && steps.len > contract.max_steps) {
        rep.fail_reason = TraceFailReason.step_budget_exceeded;
        rep.offending_step_idx = contract.max_steps;
        return rep;
    }
    if (contract.max_total_latency_ms > 0 && rep.total_latency_ms > contract.max_total_latency_ms) {
        rep.fail_reason = TraceFailReason.latency_budget_exceeded;
        return rep;
    }
    if (contract.max_total_tokens > 0 && rep.total_tokens > contract.max_total_tokens) {
        rep.fail_reason = TraceFailReason.token_budget_exceeded;
        return rep;
    }

    // 3. Trajectory contract matching
    if (contract.expected_count == 0) {
        if (contract.mode == TraceMatchMode.exact_sequence && steps.len != 0) {
            rep.fail_reason = TraceFailReason.sequence_mismatch;
            rep.offending_step_idx = 0;
            rep.offending_tool_hash = steps[0].tool_id_hash;
            return rep;
        }
        rep.passed = true;
        return rep;
    }

    if (contract.mode == TraceMatchMode.exact_sequence) {
        let min_len = steps.len;
        if (contract.expected_count < min_len) {
            min_len = contract.expected_count;
        }
        let k: usize = 0;
        while (k < min_len) : (k += 1) {
            const exp = contract.expected[k];
            const st = steps[k];
            if (stepMatchesExpected(st, exp)) {
                rep.matched_expected += 1;
            } else {
                rep.offending_step_idx = k;
                rep.offending_tool_hash = st.tool_id_hash;
                if (exp.require_args_hash && stepMatchesToolOnly(st, exp)) {
                    rep.fail_reason = TraceFailReason.args_hash_mismatch;
                } else {
                    rep.fail_reason = TraceFailReason.sequence_mismatch;
                }
                return rep;
            }
        }
        if (steps.len != contract.expected_count) {
            rep.fail_reason = TraceFailReason.sequence_mismatch;
            rep.offending_step_idx = min_len;
            if (min_len < steps.len) {
                rep.offending_tool_hash = steps[min_len].tool_id_hash;
            } else {
                rep.offending_tool_hash = contract.expected[min_len].tool_id_hash;
            }
            return rep;
        }
        rep.passed = true;
        return rep;
    }

    if (contract.mode == TraceMatchMode.ordered_subsequence) {
        let e_idx: usize = 0;
        let saw_args_mismatch: bool = false;
        let mismatch_step: usize = 0;

        let s_idx: usize = 0;
        while (s_idx < steps.len && e_idx < contract.expected_count) : (s_idx += 1) {
            const exp = contract.expected[e_idx];
            if (stepMatchesExpected(steps[s_idx], exp)) {
                e_idx += 1;
                saw_args_mismatch = false;
            } else if (exp.require_args_hash && stepMatchesToolOnly(steps[s_idx], exp)) {
                if (!saw_args_mismatch) {
                    saw_args_mismatch = true;
                    mismatch_step = s_idx;
                }
            }
        }

        rep.matched_expected = e_idx;
        if (e_idx == contract.expected_count) {
            rep.passed = true;
            return rep;
        }
        rep.offending_tool_hash = contract.expected[e_idx].tool_id_hash;
        if (saw_args_mismatch) {
            rep.fail_reason = TraceFailReason.args_hash_mismatch;
            rep.offending_step_idx = mismatch_step;
        } else {
            rep.fail_reason = TraceFailReason.missing_expected_tool;
        }
        return rep;
    }

    if (contract.mode == TraceMatchMode.unordered_subset) {
        let used: [128]bool = [false; 128];
        let e_idx: usize = 0;
        let first_fail_reason = TraceFailReason.none;
        let first_fail_tool: u64 = 0;
        let first_fail_step: usize = 0;

        while (e_idx < contract.expected_count) : (e_idx += 1) {
            const exp = contract.expected[e_idx];
            let matched: bool = false;
            let saw_args_bad: bool = false;
            let bad_step: usize = 0;

            let s_idx: usize = 0;
            while (s_idx < steps.len) : (s_idx += 1) {
                if (used[s_idx]) continue;
                if (stepMatchesExpected(steps[s_idx], exp)) {
                    used[s_idx] = true;
                    matched = true;
                    break;
                } else if (exp.require_args_hash && stepMatchesToolOnly(steps[s_idx], exp)) {
                    if (!saw_args_bad) {
                        saw_args_bad = true;
                        bad_step = s_idx;
                    }
                }
            }

            if (matched) {
                rep.matched_expected += 1;
            } else if (first_fail_reason == TraceFailReason.none) {
                first_fail_tool = exp.tool_id_hash;
                if (saw_args_bad) {
                    first_fail_reason = TraceFailReason.args_hash_mismatch;
                    first_fail_step = bad_step;
                } else {
                    first_fail_reason = TraceFailReason.missing_expected_tool;
                }
            }
        }

        if (rep.matched_expected == contract.expected_count) {
            rep.passed = true;
            return rep;
        }
        rep.fail_reason = first_fail_reason;
        rep.offending_tool_hash = first_fail_tool;
        rep.offending_step_idx = first_fail_step;
        return rep;
    }

    // TraceMatchMode.any_of
    let saw_any_args_bad: bool = false;
    let bad_any_step: usize = 0;
    let bad_any_tool: u64 = 0;

    let s_idx: usize = 0;
    while (s_idx < steps.len) : (s_idx += 1) {
        let e_idx: usize = 0;
        while (e_idx < contract.expected_count) : (e_idx += 1) {
            const exp = contract.expected[e_idx];
            if (stepMatchesExpected(steps[s_idx], exp)) {
                rep.matched_expected = 1;
                rep.passed = true;
                return rep;
            } else if (exp.require_args_hash && stepMatchesToolOnly(steps[s_idx], exp)) {
                if (!saw_any_args_bad) {
                    saw_any_args_bad = true;
                    bad_any_step = s_idx;
                    bad_any_tool = exp.tool_id_hash;
                }
            }
        }
    }

    if (saw_any_args_bad) {
        rep.fail_reason = TraceFailReason.args_hash_mismatch;
        rep.offending_step_idx = bad_any_step;
        rep.offending_tool_hash = bad_any_tool;
    } else {
        rep.fail_reason = TraceFailReason.missing_expected_tool;
        rep.offending_tool_hash = contract.expected[0].tool_id_hash;
    }
    return rep;
}

/// Evaluates a batch of `contracts` against a single trajectory `steps`, writing reports into
/// `out_reports[0..contracts.len]`. Returns `null` if `out_reports.len < contracts.len`.
pub fn evaluateTraceBatch(
    contracts: []const TraceContract,
    steps: []const TraceToolStep,
    out_reports: *[]TraceAssertionReport,
): usize? {
    if (out_reports.len < contracts.len) {
        return null;
    }
    let i: usize = 0;
    while (i < contracts.len) : (i += 1) {
        out_reports[i] = evaluateTraceContract(contracts[i], steps);
    }
    return contracts.len;
}

// ============================================================================
// 3. Multi-Case CI Suite Accumulator & Pass-Rate Regression Gate
// ============================================================================

/// Zero-heap-allocation multi-case CI regression suite accumulator.
pub struct TraceSuiteAccumulator {
    total_cases: usize = 0;
    passed_cases: usize = 0;
    failed_cases: usize = 0;
    forbidden_failures: usize = 0;
    budget_failures: usize = 0;
    contract_failures: usize = 0;
    total_latency_ms: u64 = 0;
    total_tokens: u64 = 0;

    static fn init(): TraceSuiteAccumulator {
        return TraceSuiteAccumulator{};
    }

    fn reset(*_) {
        _.total_cases = 0;
        _.passed_cases = 0;
        _.failed_cases = 0;
        _.forbidden_failures = 0;
        _.budget_failures = 0;
        _.contract_failures = 0;
        _.total_latency_ms = 0;
        _.total_tokens = 0;
    }

    /// Records a single `TraceAssertionReport` into the suite accumulator.
    fn record(*_, report: TraceAssertionReport) {
        _.total_cases += 1;
        _.total_latency_ms += report.total_latency_ms;
        _.total_tokens += report.total_tokens;
        if (report.passed) {
            _.passed_cases += 1;
            return;
        }
        _.failed_cases += 1;
        if (report.fail_reason == TraceFailReason.forbidden_tool_invoked) {
            _.forbidden_failures += 1;
        } else if (report.fail_reason == TraceFailReason.step_budget_exceeded ||
            report.fail_reason == TraceFailReason.latency_budget_exceeded ||
            report.fail_reason == TraceFailReason.token_budget_exceeded ||
            report.fail_reason == TraceFailReason.step_limit_overflow)
        {
            _.budget_failures += 1;
        } else {
            _.contract_failures += 1;
        }
    }

    /// Current suite pass rate in basis points (`0..=10_000`). Returns `0` when empty.
    fn passRateBps(): u32 {
        if (_.total_cases == 0) {
            return 0;
        }
        return (((_.passed_cases as u64) * (TRACE_EVAL_BPS_DENOM as u64)) / (_.total_cases as u64)) as u32;
    }

    /// Returns `true` iff `total_cases > 0`, `min_pass_rate_bps <= 10_000`, and `passRateBps() >= min_pass_rate_bps`.
    fn meetsMinPassRate(min_pass_rate_bps: u32): bool {
        if (_.total_cases == 0 || min_pass_rate_bps > TRACE_EVAL_BPS_DENOM) {
            return false;
        }
        return _.passRateBps() >= min_pass_rate_bps;
    }

    /// Returns a complete `TraceSuiteSummary` snapshot.
    fn summary(): TraceSuiteSummary {
        return TraceSuiteSummary{
            total_cases: _.total_cases,
            passed_cases: _.passed_cases,
            failed_cases: _.failed_cases,
            pass_rate_bps: _.passRateBps(),
            forbidden_failures: _.forbidden_failures,
            budget_failures: _.budget_failures,
            contract_failures: _.contract_failures,
            total_latency_ms: _.total_latency_ms,
            total_tokens: _.total_tokens,
        };
    }
}
