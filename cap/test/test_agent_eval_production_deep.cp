// Source: cp/cap/test/test_agent_eval_production_deep.cp (Production deep tests for cap:agent eval)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    passAtK,
    exactMatchNormalized,
    TextF1Result,
    tokenOverlapF1,
    TrajectoryScore,
    scoreToolTrajectory,
    EvalSummary,
    summarizeEvalRun,
} from "cap:agent";

fn approxEq(a: f64, b: f64, eps: f64): bool {
    const diff = if (a >= b) (a - b) else (b - a);
    return diff <= eps;
}

test("1. passAtK exact combinatorial values, large n=200 k=10 zero-overflow, and boundary guards", () => {
    // Hand-calculated: n=5, c=2, k=2 -> 1 - C(3,2)/C(5,2) = 1 - 3/10 = 0.7
    expect_eq(approxEq(passAtK(5, 2, 2) ?? -1.0, 0.7, 1e-12), true);
    // n=10, c=1, k=1 -> 0.1
    expect_eq(approxEq(passAtK(10, 1, 1) ?? -1.0, 0.1, 1e-12), true);
    // n=10, c=3, k=3 -> 1 - C(7,3)/C(10,3) = 1 - 35/120 = 85/120
    expect_eq(approxEq(passAtK(10, 3, 3) ?? -1.0, 85.0 / 120.0, 1e-12), true);

    // Exact shortcuts: c == 0 -> 0.0; n - c < k -> 1.0
    expect_eq(passAtK(10, 0, 5) ?? -1.0, 0.0);
    expect_eq(passAtK(10, 8, 3) ?? -1.0, 1.0); // n - c = 2 < 3
    expect_eq(passAtK(10, 10, 1) ?? -1.0, 1.0);

    // Large n=200, c=20, k=10 (C(200, 10) ~ 2.24e16 exceeds u64 factorial, must not overflow)
    // Product_{i=0..9} (180 - i)/(200 - i) ~ 0.3401465660394 -> 1 - prod ~ 0.6598534339606
    const p_large = passAtK(200, 20, 10) ?? -1.0;
    expect_eq(approxEq(p_large, 0.6598534339606, 1e-9), true);

    // Invalid parameters rejected
    expect_eq(passAtK(10, 2, 0) == null, true);
    expect_eq(passAtK(5, 2, 6) == null, true);
    expect_eq(passAtK(5, 6, 2) == null, true);
    expect_eq(passAtK(0, 0, 0) == null, true);
});

test("2. exactMatchNormalized and tokenOverlapF1 with multiset duplicate handling", () => {
    expect_eq(exactMatchNormalized("Hello,   WORLD!!", "  hello world "), true);
    expect_eq(exactMatchNormalized("foo-bar_baz", "FOO BAR BAZ"), true);
    expect_eq(exactMatchNormalized("   ...,,, ", ""), true);
    expect_eq(exactMatchNormalized("hello world", "hello worlds"), false);
    expect_eq(exactMatchNormalized("hello world", "hello"), false);

    let ph: [16]u64 = std.mem.zeroes([16]u64);
    let gh: [16]u64 = std.mem.zeroes([16]u64);
    let ph_s: []u64 = ph[0..];
    let gh_s: []u64 = gh[0..];

    // Multiset overlap: pred=["cat","cat","dog"] (3), gold=["cat","dog","dog","bird"] (4)
    // Intersection: 1*"cat" + 1*"dog" = 2 -> P = 2/3, R = 2/4 = 0.5, F1 = 2*(2/3)*(1/2)/(2/3+1/2) = 4/7
    const r1 = tokenOverlapF1("Cat, cat! DOG.", "cat dog dog bird", &ph_s, &gh_s) ?? unreachable;
    expect_eq(approxEq(r1.precision, 2.0 / 3.0, 1e-12), true);
    expect_eq(approxEq(r1.recall, 0.5, 1e-12), true);
    expect_eq(approxEq(r1.f1, 4.0 / 7.0, 1e-12), true);

    // Both empty -> 1.0; one empty -> 0.0
    const r_both_empty = tokenOverlapF1("   ", "---", &ph_s, &gh_s) ?? unreachable;
    expect_eq(r_both_empty.f1, 1.0);
    const r_one_empty = tokenOverlapF1("answer", "", &ph_s, &gh_s) ?? unreachable;
    expect_eq(r_one_empty.f1, 0.0);

    // Insufficient scratch buffer -> null
    let tiny: [2]u64 = std.mem.zeroes([2]u64);
    let tiny_s: []u64 = tiny[0..];
    expect_eq(tokenOverlapF1("one two three", "one two", &tiny_s, &gh_s) == null, true);
    expect_eq(tokenOverlapF1("one two", "one two three", &ph_s, &tiny_s) == null, true);
});

test("3. scoreToolTrajectory covers exact, reordered, extra, missing, and empty tool sequences", () => {
    let dp_prev: [16]usize = std.mem.zeroes([16]usize);
    let dp_curr: [16]usize = std.mem.zeroes([16]usize);
    let prev_s: []usize = dp_prev[0..];
    let curr_s: []usize = dp_curr[0..];

    const exp: [3]u64 = [10, 20, 30];

    // 1. Exact match
    const act_exact: [3]u64 = [10, 20, 30];
    const s_exact = scoreToolTrajectory(act_exact[0..], exp[0..], &prev_s, &curr_s) ?? unreachable;
    expect_eq(s_exact.exact_order_match, true);
    expect_eq(s_exact.order_lcs_ratio, 1.0);
    expect_eq(s_exact.set_precision, 1.0);
    expect_eq(s_exact.set_recall, 1.0);
    expect_eq(s_exact.set_f1, 1.0);

    // 2. Reordered: [10, 30, 20] vs [10, 20, 30] -> LCS = 2/3, set F1 = 1.0
    const act_reord: [3]u64 = [10, 30, 20];
    const s_reord = scoreToolTrajectory(act_reord[0..], exp[0..], &prev_s, &curr_s) ?? unreachable;
    expect_eq(s_reord.exact_order_match, false);
    expect_eq(approxEq(s_reord.order_lcs_ratio, 2.0 / 3.0, 1e-12), true);
    expect_eq(s_reord.set_precision, 1.0);
    expect_eq(s_reord.set_recall, 1.0);
    expect_eq(s_reord.set_f1, 1.0);

    // 3. Extra redundant tool + repeated tool: [10, 99, 20, 20, 30] vs [10, 20, 30]
    // LCS = 3 / max(5, 3) = 0.6; unique actual = {10,99,20,30} (4), intersection = 3 -> P=0.75, R=1.0, F1=6/7
    const act_extra: [5]u64 = [10, 99, 20, 20, 30];
    const s_extra = scoreToolTrajectory(act_extra[0..], exp[0..], &prev_s, &curr_s) ?? unreachable;
    expect_eq(s_extra.exact_order_match, false);
    expect_eq(approxEq(s_extra.order_lcs_ratio, 0.6, 1e-12), true);
    expect_eq(approxEq(s_extra.set_precision, 0.75, 1e-12), true);
    expect_eq(approxEq(s_extra.set_recall, 1.0, 1e-12), true);
    expect_eq(approxEq(s_extra.set_f1, 6.0 / 7.0, 1e-12), true);

    // 4. Missing tool: [10, 30] vs [10, 20, 30] -> LCS = 2/3, P=1.0, R=2/3, F1=0.8
    const act_miss: [2]u64 = [10, 30];
    const s_miss = scoreToolTrajectory(act_miss[0..], exp[0..], &prev_s, &curr_s) ?? unreachable;
    expect_eq(s_miss.exact_order_match, false);
    expect_eq(approxEq(s_miss.order_lcs_ratio, 2.0 / 3.0, 1e-12), true);
    expect_eq(approxEq(s_miss.set_precision, 1.0, 1e-12), true);
    expect_eq(approxEq(s_miss.set_recall, 2.0 / 3.0, 1e-12), true);
    expect_eq(approxEq(s_miss.set_f1, 0.8, 1e-12), true);

    // 5. Empty trajectories & insufficient DP scratch
    const empty_u64: [0]u64 = [];
    const s_both_empty = scoreToolTrajectory(empty_u64[0..], empty_u64[0..], &prev_s, &curr_s) ?? unreachable;
    expect_eq(s_both_empty.exact_order_match, true);
    expect_eq(s_both_empty.order_lcs_ratio, 1.0);
    const s_one_empty = scoreToolTrajectory(empty_u64[0..], exp[0..], &prev_s, &curr_s) ?? unreachable;
    expect_eq(s_one_empty.exact_order_match, false);
    expect_eq(s_one_empty.set_f1, 0.0);

    let tiny_prev: [3]usize = std.mem.zeroes([3]usize);
    let tiny_curr: [3]usize = std.mem.zeroes([3]usize);
    let tp_s: []usize = tiny_prev[0..];
    let tc_s: []usize = tiny_curr[0..];
    expect_eq(scoreToolTrajectory(act_exact[0..], exp[0..], &tp_s, &tc_s) == null, true);
});

test("4. summarizeEvalRun exact P50/P95/P99 latency percentiles, pass_threshold fallback, and cost sum", () => {
    // 100 cases with latencies 1.0..100.0 stored in reverse order to exercise heapSortF64
    let scores: [100]f64 = std.mem.zeroes([100]f64);
    let lats: [100]f64 = std.mem.zeroes([100]f64);
    let costs: [100]u64 = std.mem.zeroes([100]u64);
    let scratch: [100]f64 = std.mem.zeroes([100]f64);

    let i: usize = 0;
    while (i < 100) {
        scores[i] = if (i < 75) 1.0 else 0.2;
        lats[i] = (100 - i) as f64;
        costs[i] = 1000;
        i += 1;
    }

    const empty_passed: [0]u8 = [];
    let scratch_s: []f64 = scratch[0..];
    const sum = summarizeEvalRun(
        scores[0..],
        empty_passed[0..],
        lats[0..],
        costs[0..],
        0.5,
        &scratch_s,
    ) ?? unreachable;

    expect_eq(sum.total_cases, 100);
    expect_eq(sum.passed_cases, 75);
    expect_eq(approxEq(sum.pass_rate, 0.75, 1e-12), true);
    expect_eq(approxEq(sum.mean_score, 0.8, 1e-12), true);
    expect_eq(approxEq(sum.p50_ms, 50.5, 1e-9), true);
    expect_eq(approxEq(sum.p95_ms, 95.05, 1e-9), true);
    expect_eq(approxEq(sum.p99_ms, 99.01, 1e-9), true);
    expect_eq(sum.total_cost_micro_usd, 100000);

    // Explicit passed slice override
    const sc5: [5]f64 = [0.9, 0.8, 0.7, 0.6, 0.5];
    const pass5: [5]u8 = [1, 0, 1, 0, 1];
    const lat5: [5]f64 = [50.0, 10.0, 40.0, 20.0, 30.0];
    const empty_costs: [0]u64 = [];
    let sc5_buf: [5]f64 = std.mem.zeroes([5]f64);
    let sc5_s: []f64 = sc5_buf[0..];
    const sum5 = summarizeEvalRun(sc5[0..], pass5[0..], lat5[0..], empty_costs[0..], 0.95, &sc5_s) ?? unreachable;
    expect_eq(sum5.passed_cases, 3);
    expect_eq(approxEq(sum5.p50_ms, 30.0, 1e-12), true);
    expect_eq(approxEq(sum5.p95_ms, 48.0, 1e-12), true);
    expect_eq(approxEq(sum5.p99_ms, 49.6, 1e-12), true);
    expect_eq(sum5.total_cost_micro_usd, 0);
});

test("5. summarizeEvalRun defends against empty slices, mismatches, insufficient scratch, negative latency, and NaN", () => {
    const empty_f: [0]f64 = [];
    const empty_u8: [0]u8 = [];
    const empty_u64: [0]u64 = [];
    let scratch_arr: [4]f64 = std.mem.zeroes([4]f64);
    let scratch_s: []f64 = scratch_arr[0..];

    // Empty scores -> null
    expect_eq(summarizeEvalRun(empty_f[0..], empty_u8[0..], empty_f[0..], empty_u64[0..], 0.5, &scratch_s) == null, true);

    const sc3: [3]f64 = [1.0, 0.5, 0.0];
    const lat3: [3]f64 = [10.0, 20.0, 30.0];
    const lat2: [2]f64 = [10.0, 20.0];
    const pass2: [2]u8 = [1, 0];
    const cost2: [2]u64 = [100, 200];

    // Length mismatches -> null
    expect_eq(summarizeEvalRun(sc3[0..], empty_u8[0..], lat2[0..], empty_u64[0..], 0.5, &scratch_s) == null, true);
    expect_eq(summarizeEvalRun(sc3[0..], pass2[0..], lat3[0..], empty_u64[0..], 0.5, &scratch_s) == null, true);
    expect_eq(summarizeEvalRun(sc3[0..], empty_u8[0..], lat3[0..], cost2[0..], 0.5, &scratch_s) == null, true);

    // Insufficient lat_scratch -> null
    let tiny_scratch: [2]f64 = std.mem.zeroes([2]f64);
    let tiny_s: []f64 = tiny_scratch[0..];
    expect_eq(summarizeEvalRun(sc3[0..], empty_u8[0..], lat3[0..], empty_u64[0..], 0.5, &tiny_s) == null, true);

    // Negative latency -> null
    const neg_lat: [3]f64 = [10.0, -1.0, 30.0];
    expect_eq(summarizeEvalRun(sc3[0..], empty_u8[0..], neg_lat[0..], empty_u64[0..], 0.5, &scratch_s) == null, true);

    // NaN in score, latency, or pass_threshold -> null
    const nan_val: f64 = std.math.nan(f64);
    const nan_sc: [3]f64 = [1.0, nan_val, 0.5];
    const nan_lat: [3]f64 = [10.0, nan_val, 30.0];
    expect_eq(summarizeEvalRun(nan_sc[0..], empty_u8[0..], lat3[0..], empty_u64[0..], 0.5, &scratch_s) == null, true);
    expect_eq(summarizeEvalRun(sc3[0..], empty_u8[0..], nan_lat[0..], empty_u64[0..], 0.5, &scratch_s) == null, true);
    expect_eq(summarizeEvalRun(sc3[0..], empty_u8[0..], lat3[0..], empty_u64[0..], nan_val, &scratch_s) == null, true);
});
