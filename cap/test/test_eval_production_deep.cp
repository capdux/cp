// Source: cp/cap/test/test_eval_production_deep.cp
// Deep Production Test Suite for cap:eval
// Covers:
//   1. EvalScorer: exactMatch (6 modes), containsAll, forbiddenNone, tokenF1 (SQuAD) & jsonFieldMatch (with cap:json)
//   2. EvalScorer.passAtK: unbiased estimation boundaries (c=0, c=n, n-c<k, n=100/c=1/k=10 exact 0.1) & error guards
//   3. EvalReport: weighted score, exact P95 across 100 latencies, token/cost totals, toMarkdown() & toJson()
//   4. EvalReport.compareBaseline: CI regression gating for score drop, P95 latency spike & acceptable variance
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.eval`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { JSON } from "cap:json";
import {
    EvalError,
    MatchMode,
    EvalScorer,
    EvalCase,
    EvalSummary,
    RegressionDiff,
    EvalReport,
} from "cap:eval";
import cap from "cap";

fn approxEq(a: f64, b: f64, tol: f64): bool {
    const diff = if (a > b) a - b else b - a;
    return diff <= tol;
}

fn testEvalScorerMetrics()! {
    const alloc = std.heap.page_allocator;

    // 1. exactMatch across all 6 MatchMode variants
    expect_eq(EvalScorer.exactMatch("hello", "hello", MatchMode.Exact), 1.0);
    expect_eq(EvalScorer.exactMatch("hello", "hello ", MatchMode.Exact), 0.0);
    expect_eq(EvalScorer.exactMatch("  hello\n", "\thello  ", MatchMode.ExactTrimmed), 1.0);
    expect_eq(EvalScorer.exactMatch("Hello World", "hello world", MatchMode.CaseInsensitive), 1.0);
    expect_eq(EvalScorer.exactMatch("Hello", "hello!", MatchMode.CaseInsensitive), 0.0);
    expect_eq(EvalScorer.exactMatch("success", "operation success!", MatchMode.Contains), 1.0);
    expect_eq(EvalScorer.exactMatch("HTTP/1.1", "HTTP/1.1 200 OK", MatchMode.Prefix), 1.0);
    expect_eq(EvalScorer.exactMatch("200 OK", "HTTP/1.1 200 OK", MatchMode.Suffix), 1.0);

    // 2. containsAll
    const kw3: [3]string = [ "alpha", "beta", "gamma" ];
    expect_eq(EvalScorer.containsAll("alpha beta gamma delta", &kw3), 1.0);
    expect_eq(approxEq(EvalScorer.containsAll("alpha and gamma", &kw3), 2.0 / 3.0, 0.000000001), true);
    expect_eq(EvalScorer.containsAll("none", &kw3), 0.0);
    const empty_kw: []const string = [];
    expect_eq(EvalScorer.containsAll("anything", empty_kw), 1.0);

    // 3. forbiddenNone
    const banned: [2]string = [ "PASSWORD", "sk-secret" ];
    expect_eq(EvalScorer.forbiddenNone("Safe response output", &banned), 1.0);
    expect_eq(EvalScorer.forbiddenNone("Leaked sk-secret token", &banned), 0.0);

    // 4. tokenF1 (SQuAD)
    const f1_exact = EvalScorer.tokenF1(alloc, "Hello, World!", "hello world") catch unreachable;
    expect_eq(approxEq(f1_exact, 1.0, 0.000000001), true);

    const f1_half = EvalScorer.tokenF1(alloc, "the quick brown fox", "the fast brown dog") catch unreachable;
    expect_eq(approxEq(f1_half, 0.5, 0.000000001), true);

    const f1_asym = EvalScorer.tokenF1(alloc, "a b c", "a b") catch unreachable;
    expect_eq(approxEq(f1_asym, 0.8, 0.000000001), true);

    const f1_empty = EvalScorer.tokenF1(alloc, "   ", "...") catch unreachable;
    expect_eq(approxEq(f1_empty, 1.0, 0.000000001), true);

    const f1_zero = EvalScorer.tokenF1(alloc, "alpha", "beta") catch unreachable;
    expect_eq(approxEq(f1_zero, 0.0, 0.000000001), true);

    // 5. jsonFieldMatch (with cap:json)
    const exp_json = "{\"user\":{\"id\":42,\"role\":\"admin\"},\"items\":[{\"name\":\"a\"},{\"name\":\"b\"}]}";
    const act_json_ok = "{\"user\":{\"role\":\"admin\",\"id\":42},\"items\":[{\"name\":\"x\"},{\"name\":\"b\"}]}";
    const act_json_bad = "{\"user\":{\"role\":\"guest\",\"id\":42}}";

    const jm_role = EvalScorer.jsonFieldMatch(alloc, exp_json, act_json_ok, "user.role") catch unreachable;
    expect_eq(jm_role, 1.0);

    const jm_ptr = EvalScorer.jsonFieldMatch(alloc, exp_json, act_json_ok, "/items/1/name") catch unreachable;
    expect_eq(jm_ptr, 1.0);

    const jm_diff = EvalScorer.jsonFieldMatch(alloc, exp_json, act_json_bad, "user.role") catch unreachable;
    expect_eq(jm_diff, 0.0);

    const jm_missing = EvalScorer.jsonFieldMatch(alloc, exp_json, act_json_bad, "items.1.name") catch unreachable;
    expect_eq(jm_missing, 0.0);

    let invalid_caught = false;
    switch (EvalScorer.jsonFieldMatch(alloc, "{bad json", act_json_ok, "user.role")) {
        err(e) => {
            expect_eq(e == EvalError.InvalidInput, true);
            invalid_caught = true;
        },
        ok => {
            * = ok;
            expect_eq(false, true);
        },
    }
    expect_eq(invalid_caught, true);
}

fn testPassAtKUnbiased()! {
    expect_eq(approxEq(EvalScorer.passAtK(10, 0, 5) catch unreachable, 0.0, 0.000000000001), true);
    expect_eq(approxEq(EvalScorer.passAtK(10, 10, 5) catch unreachable, 1.0, 0.000000000001), true);
    expect_eq(approxEq(EvalScorer.passAtK(10, 7, 5) catch unreachable, 1.0, 0.000000000001), true);

    // Codex paper classic check: n=100, c=1, k=10 -> 1 - C(99,10)/C(100,10) = 1 - 90/100 = 0.1
    const p_100 = EvalScorer.passAtK(100, 1, 10) catch unreachable;
    expect_eq(approxEq(p_100, 0.1, 0.000000001), true);

    // Large n without factorial overflow
    const p_large = EvalScorer.passAtK(1000, 50, 20) catch unreachable;
    expect_eq(p_large > 0.6 && p_large < 0.7, true);

    // Invalid parameter guards
    let k_gt_n = false;
    switch (EvalScorer.passAtK(5, 2, 6)) {
        err(e) => {
            expect_eq(e == EvalError.InvalidInput, true);
            k_gt_n = true;
        },
        ok => {
            * = ok;
            expect_eq(false, true);
        },
    }
    expect_eq(k_gt_n, true);

    let k_zero = false;
    switch (EvalScorer.passAtK(5, 2, 0)) {
        err(e) => {
            expect_eq(e == EvalError.InvalidInput, true);
            k_zero = true;
        },
        ok => {
            * = ok;
            expect_eq(false, true);
        },
    }
    expect_eq(k_zero, true);

    let c_gt_n = false;
    switch (EvalScorer.passAtK(5, 6, 1)) {
        err(e) => {
            expect_eq(e == EvalError.InvalidInput, true);
            c_gt_n = true;
        },
        ok => {
            * = ok;
            expect_eq(false, true);
        },
    }
    expect_eq(c_gt_n, true);
}

fn testEvalReportAggregationAndExport()! {
    const alloc = std.heap.page_allocator;

    let report = EvalReport.init(alloc, "agent-qa-v1") catch unreachable;
    defer report.deinit();

    // Empty dataset guard
    let empty_err = false;
    switch (report.summarize()) {
        err(e) => {
            expect_eq(e == EvalError.EmptyDataset, true);
            empty_err = true;
        },
        ok => {
            * = ok;
            expect_eq(false, true);
        },
    }
    expect_eq(empty_err, true);

    // Record 100 cases in reverse latency order (100.0 down to 1.0)
    // First 80 cases (lat 100..21) passed with score 1.0, weight 2.0
    // Last 20 cases (lat 20..1) failed with score 0.5, weight 1.0
    let i: usize = 0;
    while (i < 100) : (i += 1) {
        const lat = (100 - i) as f64;
        const passed = i < 80;
        const score: f64 = if (passed) 1.0 else 0.5;
        const weight: f64 = if (passed) 2.0 else 1.0;
        report.recordCase(EvalCase{
            id: "case",
            category: "reasoning",
            score: score,
            passed: passed,
            latency_ms: lat,
            tokens_in: 100,
            tokens_out: 50,
            cost_usd: 0.001,
            weight: weight,
        }) catch unreachable;
    }

    const s = report.summarize() catch unreachable;
    expect_eq(s.total_cases, 100 as usize);
    expect_eq(s.passed_cases, 80 as usize);
    expect_eq(approxEq(s.pass_rate, 0.8, 0.000000001), true);
    // Weighted score = (80*2*1.0 + 20*1*0.5) / (80*2 + 20*1) = 170 / 180
    expect_eq(approxEq(s.weighted_score, 170.0 / 180.0, 0.000000001), true);
    expect_eq(approxEq(s.mean_latency_ms, 50.5, 0.000000001), true);
    expect_eq(approxEq(s.p95_latency_ms, 95.0, 0.000000001), true);
    expect_eq(s.total_tokens, 15000 as u64);
    expect_eq(approxEq(s.total_cost_usd, 0.1, 0.000000001), true);

    const md = report.toMarkdown() catch unreachable;
    defer alloc.free(md);
    expect_eq(std.mem.indexOf(u8, md, "# Eval Report: agent-qa-v1") != null, true);
    expect_eq(std.mem.indexOf(u8, md, "| P95 Latency (ms) | 95.00 |") != null, true);

    const js = report.toJson() catch unreachable;
    defer alloc.free(js);
    const parsed = JSON.parse(alloc, js) catch unreachable;
    expect_eq(std.mem.eql(u8, parsed.get("suite_name").?.asString().?, "agent-qa-v1"), true);
    expect_eq(parsed.get("summary").?.get("total_cases").?.asNumber().? == 100.0, true);
    expect_eq(approxEq(parsed.get("summary").?.get("p95_latency_ms").?.asNumber().?, 95.0, 0.000001), true);
    expect_eq(parsed.get("cases").?.len(), 100 as usize);
}

fn testRegressionGate()! {
    const alloc = std.heap.page_allocator;

    let report = EvalReport.init(alloc, "ci-gate") catch unreachable;
    defer report.deinit();

    report.recordCase(EvalCase{
        id: "c1",
        category: "code",
        score: 0.85,
        passed: true,
        latency_ms: 120.0,
        tokens_in: 50,
        tokens_out: 50,
        cost_usd: 0.002,
        weight: 1.0,
    }) catch unreachable;

    // 1. Acceptable fluctuation -> pass
    const base_ok = EvalSummary{
        total_cases: 1,
        passed_cases: 1,
        pass_rate: 1.0,
        weighted_score: 0.87,
        mean_latency_ms: 110.0,
        p95_latency_ms: 110.0,
        total_tokens: 100,
        total_cost_usd: 0.002,
    };
    const diff_ok = report.compareBaseline(base_ok, 0.05, 20.0) catch unreachable;
    expect_eq(diff_ok.regressed, false);
    expect_eq(std.mem.eql(u8, diff_ok.reason, "ok"), true);

    // 2. Score drop > max_score_drop -> regressed
    const base_high_score = EvalSummary{
        total_cases: 1,
        passed_cases: 1,
        pass_rate: 1.0,
        weighted_score: 0.95,
        mean_latency_ms: 120.0,
        p95_latency_ms: 120.0,
        total_tokens: 100,
        total_cost_usd: 0.002,
    };
    const diff_score = report.compareBaseline(base_high_score, 0.05, 20.0) catch unreachable;
    expect_eq(diff_score.regressed, true);
    expect_eq(std.mem.eql(u8, diff_score.reason, "weighted_score_drop"), true);

    // 3. P95 latency spike > max_p95_increase_ms -> regressed
    const base_fast = EvalSummary{
        total_cases: 1,
        passed_cases: 1,
        pass_rate: 1.0,
        weighted_score: 0.85,
        mean_latency_ms: 50.0,
        p95_latency_ms: 50.0,
        total_tokens: 100,
        total_cost_usd: 0.002,
    };
    const diff_lat = report.compareBaseline(base_fast, 0.05, 20.0) catch unreachable;
    expect_eq(diff_lat.regressed, true);
    expect_eq(std.mem.eql(u8, diff_lat.reason, "p95_latency_increase"), true);

    // 4. Negative threshold rejected
    let bad_thresh = false;
    switch (report.compareBaseline(base_ok, -0.1, 10.0)) {
        err(e) => {
            expect_eq(e == EvalError.InvalidThreshold, true);
            bad_thresh = true;
        },
        ok => {
            * = ok;
            expect_eq(false, true);
        },
    }
    expect_eq(bad_thresh, true);
}

fn testDynamicCapEvalMounting()! {
    const alloc = std.heap.page_allocator;

    expect_eq(cap.eval.EvalScorer.exactMatch("cp", " cp ", MatchMode.ExactTrimmed), 1.0);
    const p10 = cap.eval.EvalScorer.passAtK(100, 1, 10) catch unreachable;
    expect_eq(approxEq(p10, 0.1, 0.000000001), true);

    let r = cap.eval.EvalReport.init(alloc, "dyn-mount") catch unreachable;
    defer r.deinit();
    r.recordCase(EvalCase{
        id: "d1",
        category: "smoke",
        score: 1.0,
        passed: true,
        latency_ms: 12.5,
        tokens_in: 10,
        tokens_out: 20,
        cost_usd: 0.0001,
        weight: 1.0,
    }) catch unreachable;
    const s = r.summarize() catch unreachable;
    expect_eq(s.total_cases, 1 as usize);
    expect_eq(approxEq(s.weighted_score, 1.0, 0.000000001), true);
}

test("cap:eval EvalScorer exactMatch, containsAll, forbiddenNone, tokenF1 & jsonFieldMatch") {
    testEvalScorerMetrics() catch unreachable;
}

test("cap:eval EvalScorer.passAtK unbiased estimation & boundary guards") {
    testPassAtKUnbiased() catch unreachable;
}

test("cap:eval EvalReport weighted summary, P95 latency & Markdown/JSON export") {
    testEvalReportAggregationAndExport() catch unreachable;
}

test("cap:eval EvalReport.compareBaseline CI regression gate") {
    testRegressionGate() catch unreachable;
}

test("cap:eval dynamic mounting via cap.eval") {
    testDynamicCapEvalMounting() catch unreachable;
}
