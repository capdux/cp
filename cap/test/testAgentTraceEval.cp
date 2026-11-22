// Origin: Pure-CP Stdlib Test Suite (lib/agent/traceEval.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Agent Trajectory Contract Assertion & CI Suite Verifier:
//   1. exact_sequence: strict 1-to-1 match, length mismatch, wrong order & failed-step rejection
//   2. ordered_subsequence: exploratory steps allowed between milestones; out-of-order rejected
//   3. unordered_subset (with duplicate expectation multiplicity) & any_of matching modes
//   4. Forbidden tool blacklist priority (forbidden_tool_invoked) & strict args_hash verification
//   5. Hard regression budgets (max_steps, max_total_latency_ms, max_total_tokens) & zero-budget unlimited
//   6. Batch assertion runner (evaluateTraceBatch) & CI suite accumulator (pass_rate_bps, meetsMinPassRate)
//   7. Boundary guards (empty expected, empty trace, 32-item caps, 128-step overflow) & cap.agent facade

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    TRACE_EVAL_MAX_EXPECTED,
    TRACE_EVAL_MAX_FORBIDDEN,
    traceToolHash,
    TraceMatchMode,
    TraceFailReason,
    TraceToolStep,
    TraceContract,
    TraceAssertionReport,
    evaluateTraceContract,
    evaluateTraceBatch,
    TraceSuiteAccumulator,
} from "cap:agent";

// ============================================================================
// Test 1: exact_sequence Mode
// ============================================================================
test("traceEval: exact_sequence enforces strict 1-to-1 tool order, length and success", () => {
    const h_search = traceToolHash("search");
    const h_read = traceToolHash("read_file");
    const h_edit = traceToolHash("edit_file");

    let contract = TraceContract.init(TraceMatchMode.exact_sequence);
    expect_true(contract.addExpected(h_search));
    expect_true(contract.addExpected(h_read));
    expect_true(contract.addExpected(h_edit));

    const exact_steps: [3]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_search, latency_ms: 50, tokens_used: 120 },
        TraceToolStep{ tool_id_hash: h_read, latency_ms: 30, tokens_used: 80 },
        TraceToolStep{ tool_id_hash: h_edit, latency_ms: 40, tokens_used: 150 },
    ];
    const ok_rep = evaluateTraceContract(contract, exact_steps[0..]);
    expect_true(ok_rep.passed);
    expect_eq(ok_rep.fail_reason, TraceFailReason.none);
    expect_eq(ok_rep.matched_expected, 3 as usize);
    expect_eq(ok_rep.total_latency_ms, 120 as u64);
    expect_eq(ok_rep.total_tokens, 350 as u64);

    // Extra trailing step fails exact_sequence
    const extra_steps: [4]TraceToolStep = [
        exact_steps[0], exact_steps[1], exact_steps[2],
        TraceToolStep{ tool_id_hash: h_read, latency_ms: 10, tokens_used: 20 },
    ];
    const extra_rep = evaluateTraceContract(contract, extra_steps[0..]);
    expect_eq(extra_rep.passed, false);
    expect_eq(extra_rep.fail_reason, TraceFailReason.sequence_mismatch);
    expect_eq(extra_rep.offending_step_idx, 3 as usize);

    // Failed step (succeeded=false) rejected when require_success=true
    let failed_steps = exact_steps;
    failed_steps[1].succeeded = false;
    const fail_rep = evaluateTraceContract(contract, failed_steps[0..]);
    expect_eq(fail_rep.passed, false);
    expect_eq(fail_rep.fail_reason, TraceFailReason.sequence_mismatch);
    expect_eq(fail_rep.offending_step_idx, 1 as usize);
    expect_eq(fail_rep.failed_steps, 1 as usize);
});

// ============================================================================
// Test 2: ordered_subsequence Mode
// ============================================================================
test("traceEval: ordered_subsequence allows exploratory steps but enforces milestone order", () => {
    const h_search = traceToolHash("search");
    const h_lint = traceToolHash("lint");
    const h_edit = traceToolHash("edit");
    const h_test = traceToolHash("run_test");

    let contract = TraceContract.init(TraceMatchMode.ordered_subsequence);
    expect_true(contract.addExpected(h_search));
    expect_true(contract.addExpected(h_edit));
    expect_true(contract.addExpected(h_test));

    const valid_trace: [5]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_lint },
        TraceToolStep{ tool_id_hash: h_search },
        TraceToolStep{ tool_id_hash: h_lint },
        TraceToolStep{ tool_id_hash: h_edit },
        TraceToolStep{ tool_id_hash: h_test },
    ];
    const ok_rep = evaluateTraceContract(contract, valid_trace[0..]);
    expect_true(ok_rep.passed);
    expect_eq(ok_rep.matched_expected, 3 as usize);

    // Out-of-order: edit before search -> fails missing_expected_tool for edit after search
    const bad_order: [3]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_edit },
        TraceToolStep{ tool_id_hash: h_search },
        TraceToolStep{ tool_id_hash: h_test },
    ];
    const bad_rep = evaluateTraceContract(contract, bad_order[0..]);
    expect_eq(bad_rep.passed, false);
    expect_eq(bad_rep.fail_reason, TraceFailReason.missing_expected_tool);
    expect_eq(bad_rep.matched_expected, 1 as usize);
    expect_eq(bad_rep.offending_tool_hash, h_edit);
});

// ============================================================================
// Test 3: unordered_subset & any_of Modes
// ============================================================================
test("traceEval: unordered_subset handles duplicate multiplicity & any_of matches first target", () => {
    const h_a = traceToolHash("tool_a");
    const h_b = traceToolHash("tool_b");
    const h_c = traceToolHash("tool_c");

    // Require tool_a twice and tool_b once in any order
    let sub = TraceContract.init(TraceMatchMode.unordered_subset);
    expect_true(sub.addExpected(h_a));
    expect_true(sub.addExpected(h_b));
    expect_true(sub.addExpected(h_a));

    const only_one_a: [2]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_b },
        TraceToolStep{ tool_id_hash: h_a },
    ];
    const r_short = evaluateTraceContract(sub, only_one_a[0..]);
    expect_eq(r_short.passed, false);
    expect_eq(r_short.fail_reason, TraceFailReason.missing_expected_tool);
    expect_eq(r_short.matched_expected, 2 as usize);

    const two_a: [3]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_b },
        TraceToolStep{ tool_id_hash: h_a },
        TraceToolStep{ tool_id_hash: h_a },
    ];
    expect_true(evaluateTraceContract(sub, two_a[0..]).passed);

    // any_of mode
    let any_c = TraceContract.init(TraceMatchMode.any_of);
    expect_true(any_c.addExpected(h_a));
    expect_true(any_c.addExpected(h_b));

    const only_c: [1]TraceToolStep = [TraceToolStep{ tool_id_hash: h_c }];
    expect_eq(evaluateTraceContract(any_c, only_c[0..]).fail_reason, TraceFailReason.missing_expected_tool);

    const hit_b: [2]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_c },
        TraceToolStep{ tool_id_hash: h_b },
    ];
    const r_any = evaluateTraceContract(any_c, hit_b[0..]);
    expect_true(r_any.passed);
    expect_eq(r_any.matched_expected, 1 as usize);
});

// ============================================================================
// Test 4: Forbidden Tool Blacklist & Strict args_hash Enforcement
// ============================================================================
test("traceEval: forbidden tool blacklist takes priority & args_hash mismatch diagnosed", () => {
    const h_sql = traceToolHash("sql_query");
    const h_rm = traceToolHash("rm_rf");
    const good_args = traceToolHash("{\"limit\":10}");
    const bad_args = traceToolHash("{\"limit\":9999}");

    let contract = TraceContract.init(TraceMatchMode.ordered_subsequence);
    expect_true(contract.addExpectedWithArgs(h_sql, good_args, true));
    expect_true(contract.addForbidden(h_rm));

    // Forbidden tool invoked even when expected tool succeeded -> forbidden_tool_invoked
    const forbidden_trace: [2]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_sql, args_hash: good_args },
        TraceToolStep{ tool_id_hash: h_rm, args_hash: 0 },
    ];
    const rf = evaluateTraceContract(contract, forbidden_trace[0..]);
    expect_eq(rf.passed, false);
    expect_eq(rf.fail_reason, TraceFailReason.forbidden_tool_invoked);
    expect_eq(rf.offending_step_idx, 1 as usize);
    expect_eq(rf.offending_tool_hash, h_rm);

    // Expected tool called with wrong args_hash -> args_hash_mismatch
    const wrong_args_trace: [1]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_sql, args_hash: bad_args },
    ];
    const ra = evaluateTraceContract(contract, wrong_args_trace[0..]);
    expect_eq(ra.passed, false);
    expect_eq(ra.fail_reason, TraceFailReason.args_hash_mismatch);
    expect_eq(ra.offending_step_idx, 0 as usize);
    expect_eq(ra.offending_tool_hash, h_sql);

    // Matching args_hash passes
    const right_args_trace: [1]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_sql, args_hash: good_args },
    ];
    expect_true(evaluateTraceContract(contract, right_args_trace[0..]).passed);
});

// ============================================================================
// Test 5: Hard Regression Budgets (Steps, Latency, Tokens) & Zero-Budget Unlimited
// ============================================================================
test("traceEval: hard budgets catch step, latency and token regressions while 0 means unlimited", () => {
    const h_ok = traceToolHash("fetch");
    const steps: [3]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_ok, latency_ms: 100, tokens_used: 400 },
        TraceToolStep{ tool_id_hash: h_ok, latency_ms: 150, tokens_used: 500 },
        TraceToolStep{ tool_id_hash: h_ok, latency_ms: 200, tokens_used: 600 },
    ]; // total: 3 steps, 450ms, 1500 tokens

    let c = TraceContract.init(TraceMatchMode.any_of);
    expect_true(c.addExpected(h_ok));

    // Zero budgets = unlimited -> passes
    c.setBudgets(0, 0, 0);
    expect_true(evaluateTraceContract(c, steps[0..]).passed);

    // Step budget exceeded (max_steps = 2 < 3)
    c.setBudgets(2, 1000, 5000);
    expect_eq(evaluateTraceContract(c, steps[0..]).fail_reason, TraceFailReason.step_budget_exceeded);

    // Latency budget exceeded (max_total_latency_ms = 400 < 450)
    c.setBudgets(5, 400, 5000);
    expect_eq(evaluateTraceContract(c, steps[0..]).fail_reason, TraceFailReason.latency_budget_exceeded);

    // Token budget exceeded (max_total_tokens = 1400 < 1500)
    c.setBudgets(5, 500, 1400);
    expect_eq(evaluateTraceContract(c, steps[0..]).fail_reason, TraceFailReason.token_budget_exceeded);

    // Exact boundary (3, 450, 1500) passes
    c.setBudgets(3, 450, 1500);
    expect_true(evaluateTraceContract(c, steps[0..]).passed);
});

// ============================================================================
// Test 6: Batch Runner & CI Suite Accumulator (pass_rate_bps & meetsMinPassRate)
// ============================================================================
test("traceEval: evaluateTraceBatch & TraceSuiteAccumulator compute pass_rate_bps and gate verdicts", () => {
    const h_a = traceToolHash("a");
    const h_bad = traceToolHash("bad");

    let c0 = TraceContract.init(TraceMatchMode.any_of);
    * = c0.addExpected(h_a); // PASS
    let c1 = TraceContract.init(TraceMatchMode.unordered_subset);
    * = c1.addExpected(h_a); // PASS
    let c2 = TraceContract.init(TraceMatchMode.any_of);
    * = c2.addForbidden(h_bad); // FAIL: forbidden
    let c3 = TraceContract.init(TraceMatchMode.any_of);
    * = c3.addExpected(h_a);
    c3.setBudgets(1, 0, 0); // FAIL: step budget (2 > 1)

    const contracts: [4]TraceContract = [c0, c1, c2, c3];
    const trace: [2]TraceToolStep = [
        TraceToolStep{ tool_id_hash: h_a, latency_ms: 50, tokens_used: 100 },
        TraceToolStep{ tool_id_hash: h_bad, latency_ms: 25, tokens_used: 50 },
    ];

    // Undersized output buffer rejected
    let small_out: [2]TraceAssertionReport = [TraceAssertionReport{}; 2];
    let small_s: []TraceAssertionReport = small_out[0..];
    expect_eq(evaluateTraceBatch(contracts[0..], trace[0..], *small_s) == null, true);

    let reports: [4]TraceAssertionReport = [TraceAssertionReport{}; 4];
    let reports_s: []TraceAssertionReport = reports[0..];
    const n = evaluateTraceBatch(contracts[0..], trace[0..], *reports_s) ?? 0;
    expect_eq(n, 4 as usize);

    let suite = TraceSuiteAccumulator.init();
    expect_eq(suite.meetsMinPassRate(0), false); // empty suite never passes gate
    let i: usize = 0;
    while (i < n) : (i += 1) {
        suite.record(reports[i]);
    }

    const sum = suite.summary();
    expect_eq(sum.total_cases, 4 as usize);
    expect_eq(sum.passed_cases, 2 as usize);
    expect_eq(sum.failed_cases, 2 as usize);
    expect_eq(sum.pass_rate_bps, 5_000 as u32);
    expect_eq(sum.forbidden_failures, 1 as usize);
    expect_eq(sum.budget_failures, 1 as usize);
    expect_eq(sum.contract_failures, 0 as usize);
    expect_true(suite.meetsMinPassRate(5_000));
    expect_eq(suite.meetsMinPassRate(7_500), false);
    expect_eq(suite.meetsMinPassRate(10_001), false);
});

// ============================================================================
// Test 7: Boundary Guards & cap.agent Dynamic Facade
// ============================================================================
test("traceEval: empty expected, empty trace, capacity caps, 128-step overflow & cap.agent facade", () => {
    const empty_steps: [0]TraceToolStep = [];
    const one_step: [1]TraceToolStep = [TraceToolStep{ tool_id_hash: traceToolHash("x") }];

    // Empty expected set: exact_sequence passes only on empty trace; ordered_subsequence passes on any
    const empty_exact = cap.agent.TraceContract.init(TraceMatchMode.exact_sequence);
    expect_true(cap.agent.evaluateTraceContract(empty_exact, empty_steps[0..]).passed);
    expect_eq(cap.agent.evaluateTraceContract(empty_exact, one_step[0..]).fail_reason, TraceFailReason.sequence_mismatch);

    const empty_sub = TraceContract.init(TraceMatchMode.ordered_subsequence);
    expect_true(evaluateTraceContract(empty_sub, empty_steps[0..]).passed);
    expect_true(evaluateTraceContract(empty_sub, one_step[0..]).passed);

    // 32-item expected & forbidden caps
    let full_c = TraceContract.init(TraceMatchMode.ordered_subsequence);
    let k: usize = 0;
    while (k < TRACE_EVAL_MAX_EXPECTED) : (k += 1) {
        expect_true(full_c.addExpected((k + 1) as u64));
    }
    expect_eq(full_c.addExpected(999), false);

    k = 0;
    while (k < TRACE_EVAL_MAX_FORBIDDEN) : (k += 1) {
        expect_true(full_c.addForbidden((k + 100) as u64));
    }
    expect_eq(full_c.addForbidden(999), false);

    // 129 steps exceeds TRACE_EVAL_MAX_STEPS (128) -> step_limit_overflow
    const huge_steps: [129]TraceToolStep = [TraceToolStep{}; 129];
    const ov = evaluateTraceContract(empty_sub, huge_steps[0..]);
    expect_eq(ov.passed, false);
    expect_eq(ov.fail_reason, TraceFailReason.step_limit_overflow);
});
