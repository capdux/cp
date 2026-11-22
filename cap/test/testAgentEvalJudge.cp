// Origin: Pure-CP Stdlib Test Suite (lib/agent/evalJudge.cp) | Created: 2026-11-22
// Production Deep Verification Suite for AI Agent Eval Judge & Elo Aggregator:
//   1. Position-bias mitigated pairwise swap matrix & position_bias_bps accuracy
//   2. InconsistentPolicy (treat_as_tie / downweight_tie / discard_sample) & Elo integration
//   3. Multi-dimension weighted RubricScorer normalization, pass & fail_threshold verdicts
//   4. RubricScorer mandatory hard_gate_min_score enforcement & failure diagnostics
//   5. RubricScorer full boundary guards (0 weight, NaN/Inf, OOB scores, duplicates, 16-dim cap)
//   6. ArenaEloTable Logistic expected win probability & zero-sum Elo update math
//   7. Multi-model tournament, deterministic descending leaderboard & undersized buffer guard
//   8. ArenaEloTable boundary guards (invalid K/Elo, unregistered/self match, 32-model cap)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    EVAL_JUDGE_MAX_RUBRIC_DIMS,
    EVAL_JUDGE_MAX_MODELS,
    EVAL_JUDGE_DEFAULT_ELO,
    EVAL_JUDGE_DEFAULT_K,
    RawJudgePick,
    PairwiseOutcome,
    InconsistentPolicy,
    PairwiseVerdict,
    PairwiseSummary,
    resolvePairwiseSwap,
    PairwiseJudgeAggregator,
    RubricDimSpec,
    RubricVerdictKind,
    RubricEvalResult,
    RubricScorer,
    EloModelEntry,
    EloMatchUpdate,
    EloRankEntry,
    ArenaEloTable,
} from "cap:agent";

fn approxEqF64(a: f64, b: f64, eps: f64): bool {
    const diff: f64 = if (a > b) (a - b) else (b - a);
    return diff <= eps;
}

test("evalJudge: position-bias mitigated pairwise swap matrix & position_bias_bps", () => {
    let agg = PairwiseJudgeAggregator.init(InconsistentPolicy.downweight_tie);

    // 1. Consistent A win: (A vs B) -> first, (B vs A) -> second
    const v1 = agg.recordSwap(RawJudgePick.pick_first, RawJudgePick.pick_second);
    expect_eq(v1.outcome, PairwiseOutcome.consistent_a_win);
    expect_eq(approxEqF64(v1.score_a, 1.0, 0.0001), true);
    expect_eq(approxEqF64(v1.score_b, 0.0, 0.0001), true);
    expect_eq(approxEqF64(v1.effective_weight, 1.0, 0.0001), true);

    // 2. Consistent B win: (A vs B) -> second, (B vs A) -> first
    const v2 = agg.recordSwap(RawJudgePick.pick_second, RawJudgePick.pick_first);
    expect_eq(v2.outcome, PairwiseOutcome.consistent_b_win);
    expect_eq(approxEqF64(v2.score_a, 0.0, 0.0001), true);
    expect_eq(approxEqF64(v2.score_b, 1.0, 0.0001), true);

    // 3. Consistent tie
    const v3 = agg.recordSwap(RawJudgePick.pick_tie, RawJudgePick.pick_tie);
    expect_eq(v3.outcome, PairwiseOutcome.consistent_tie);
    expect_eq(approxEqF64(v3.score_a, 0.5, 0.0001), true);
    expect_eq(approxEqF64(v3.effective_weight, 1.0, 0.0001), true);

    // 4. First-slot position bias: judge always picks first regardless of swap
    const v4 = agg.recordSwap(RawJudgePick.pick_first, RawJudgePick.pick_first);
    expect_eq(v4.outcome, PairwiseOutcome.position_bias_inconsistent);
    expect_eq(v4.first_slot_bias, true);
    expect_eq(v4.second_slot_bias, false);
    expect_eq(approxEqF64(v4.score_a, 0.5, 0.0001), true);
    expect_eq(approxEqF64(v4.effective_weight, 0.5, 0.0001), true);

    // 5. Second-slot position bias
    const v5 = agg.recordSwap(RawJudgePick.pick_second, RawJudgePick.pick_second);
    expect_eq(v5.outcome, PairwiseOutcome.position_bias_inconsistent);
    expect_eq(v5.first_slot_bias, false);
    expect_eq(v5.second_slot_bias, true);

    // 2 inconsistent out of 5 total => 4000 bps (40.00%)
    const sum = agg.summary();
    expect_eq(sum.total_pairs, 5 as u64);
    expect_eq(sum.consistent_a_wins, 1 as u64);
    expect_eq(sum.consistent_b_wins, 1 as u64);
    expect_eq(sum.consistent_ties, 1 as u64);
    expect_eq(sum.inconsistent_count, 2 as u64);
    expect_eq(sum.first_slot_bias_count, 1 as u64);
    expect_eq(sum.second_slot_bias_count, 1 as u64);
    expect_eq(sum.position_bias_bps, 4000 as u32);

    agg.reset();
    expect_eq(agg.summary().total_pairs, 0 as u64);
    expect_eq(agg.summary().position_bias_bps, 0 as u32);
});

test("evalJudge: InconsistentPolicy modes & ArenaEloTable verdict integration", () => {
    const v_tie = resolvePairwiseSwap(RawJudgePick.pick_first, RawJudgePick.pick_first, InconsistentPolicy.treat_as_tie);
    const v_down = resolvePairwiseSwap(RawJudgePick.pick_first, RawJudgePick.pick_first, InconsistentPolicy.downweight_tie);
    const v_drop = resolvePairwiseSwap(RawJudgePick.pick_first, RawJudgePick.pick_first, InconsistentPolicy.discard_sample);

    expect_eq(approxEqF64(v_tie.effective_weight, 1.0, 0.0001), true);
    expect_eq(approxEqF64(v_down.effective_weight, 0.5, 0.0001), true);
    expect_eq(approxEqF64(v_drop.effective_weight, 0.0, 0.0001), true);

    // Give model 1 higher rating first so a tie produces non-zero delta
    let arena = ArenaEloTable.init(32.0, 1500.0) ?? unreachable;
    expect_eq(arena.registerModel(1), true);
    expect_eq(arena.registerModel(2), true);
    const win_v = resolvePairwiseSwap(RawJudgePick.pick_first, RawJudgePick.pick_second, InconsistentPolicy.discard_sample);
    const u0 = arena.recordPairwiseVerdict(1, 2, win_v);
    expect_eq(u0.applied, true);

    // Discard sample must NOT alter ratings or match counts
    const before_m1 = arena.getModel(1) ?? unreachable;
    const u_drop = arena.recordPairwiseVerdict(1, 2, v_drop);
    expect_eq(u_drop.applied, false);
    expect_eq((arena.getModel(1) ?? unreachable).matches, before_m1.matches);
    expect_eq(approxEqF64((arena.getModel(1) ?? unreachable).elo, before_m1.elo, 0.00001), true);

    // Downweighted tie applies half-K adjustment
    const u_down = arena.recordPairwiseVerdict(1, 2, v_down);
    expect_eq(u_down.applied, true);
    expect_eq(u_down.delta_a < 0.0, true); // Higher-rated model 1 loses small Elo on a tie
    expect_eq(approxEqF64(u_down.delta_a + u_down.delta_b, 0.0, 0.00001), true);
});

test("evalJudge: multi-dimension weighted RubricScorer pass & fail_threshold", () => {
    let scorer = RubricScorer.init(0.75) ?? unreachable;

    // 3 dimensions with different raw scales: [0..5], [0..100], [0..1]
    expect_eq(scorer.addDimension(RubricDimSpec {
        dim_id: 101,
        weight_bps: 5000,
        min_score: 0.0,
        max_score: 5.0,
        has_hard_gate: false,
        hard_gate_min_score: 0.0,
    }), true);
    expect_eq(scorer.addDimension(RubricDimSpec {
        dim_id: 102,
        weight_bps: 3000,
        min_score: 0.0,
        max_score: 100.0,
        has_hard_gate: false,
        hard_gate_min_score: 0.0,
    }), true);
    expect_eq(scorer.addDimension(RubricDimSpec {
        dim_id: 103,
        weight_bps: 2000,
        min_score: 0.0,
        max_score: 1.0,
        has_hard_gate: false,
        hard_gate_min_score: 0.0,
    }), true);

    expect_eq(scorer.dimCount(), 3 as usize);
    expect_eq(scorer.totalWeightBps(), 10000 as u64);

    // [4.0/5=0.8 (50%), 80/100=0.8 (30%), 0.5/1=0.5 (20%)] => 0.40 + 0.24 + 0.10 = 0.74 < 0.75 => fail_threshold
    const s_low: [3]f64 = [4.0, 80.0, 0.5];
    const r_low = scorer.evaluate(s_low[0..]);
    expect_eq(r_low.verdict, RubricVerdictKind.fail_threshold);
    expect_eq(r_low.passed_all_hard_gates, true);
    expect_eq(approxEqF64(r_low.weighted_score, 0.74, 0.0001), true);

    // [4.5/5=0.9 (50%), 80/100=0.8 (30%), 0.5/1=0.5 (20%)] => 0.45 + 0.24 + 0.10 = 0.79 >= 0.75 => pass
    const s_hi: [3]f64 = [4.5, 80.0, 0.5];
    const r_hi = scorer.evaluate(s_hi[0..]);
    expect_eq(r_hi.verdict, RubricVerdictKind.pass);
    expect_eq(r_hi.passed_all_hard_gates, true);
    expect_eq(approxEqF64(r_hi.weighted_score, 0.79, 0.0001), true);
});

test("evalJudge: RubricScorer mandatory hard_gate_min_score blocks high-score unsafe candidate", () => {
    let scorer = RubricScorer.init(0.60) ?? unreachable;

    // Correctness (80% weight, no hard gate)
    expect_eq(scorer.addDimension(RubricDimSpec {
        dim_id: 1,
        weight_bps: 8000,
        min_score: 0.0,
        max_score: 10.0,
        has_hard_gate: false,
        hard_gate_min_score: 0.0,
    }), true);
    // Safety (10% weight, hard gate >= 8.0)
    expect_eq(scorer.addDimension(RubricDimSpec {
        dim_id: 2,
        weight_bps: 1000,
        min_score: 0.0,
        max_score: 10.0,
        has_hard_gate: true,
        hard_gate_min_score: 8.0,
    }), true);
    // Format compliance (10% weight, hard gate >= 1.0)
    expect_eq(scorer.addDimension(RubricDimSpec {
        dim_id: 3,
        weight_bps: 1000,
        min_score: 0.0,
        max_score: 1.0,
        has_hard_gate: true,
        hard_gate_min_score: 1.0,
    }), true);

    // Candidate gets 10/10 correctness (weighted 0.85 > 0.60) but fails safety (5.0 < 8.0) and format (0.0 < 1.0)
    const scores: [3]f64 = [10.0, 5.0, 0.0];
    const res = scorer.evaluate(scores[0..]);
    expect_eq(res.verdict, RubricVerdictKind.fail_hard_gate);
    expect_eq(res.passed_all_hard_gates, false);
    expect_eq(res.failed_hard_gate_count, 2 as usize);
    expect_eq(res.first_failed_dim_id, 2 as u32);
    expect_eq(approxEqF64(res.weighted_score, 0.85, 0.0001), true);

    // Candidate passes both hard gates exactly on boundary
    const pass_scores: [3]f64 = [7.0, 8.0, 1.0];
    const res_ok = scorer.evaluate(pass_scores[0..]);
    expect_eq(res_ok.verdict, RubricVerdictKind.pass);
    expect_eq(res_ok.passed_all_hard_gates, true);
    expect_eq(res_ok.failed_hard_gate_count, 0 as usize);
});

test("evalJudge: RubricScorer full boundary guards (0 weight, NaN/OOB, duplicates, 16-dim cap)", () => {
    // Invalid pass_threshold
    expect_eq(RubricScorer.init(-0.1) == null, true);
    expect_eq(RubricScorer.init(1.1) == null, true);
    expect_eq(RubricScorer.init(std.math.nan(f64)) == null, true);

    let scorer = RubricScorer.init(0.5) ?? unreachable;

    // Evaluating empty scorer -> invalid_input
    const empty_s: [0]f64 = [];
    expect_eq(scorer.evaluate(empty_s[0..]).verdict, RubricVerdictKind.invalid_input);

    // Invalid dimension specs: 0 weight, max <= min, hard gate outside range
    expect_eq(scorer.addDimension(RubricDimSpec { dim_id: 1, weight_bps: 0, min_score: 0.0, max_score: 1.0 }), false);
    expect_eq(scorer.addDimension(RubricDimSpec { dim_id: 1, weight_bps: 100, min_score: 5.0, max_score: 5.0 }), false);
    expect_eq(scorer.addDimension(RubricDimSpec { dim_id: 1, weight_bps: 100, min_score: 0.0, max_score: 5.0, has_hard_gate: true, hard_gate_min_score: 6.0 }), false);

    // Fill to EVAL_JUDGE_MAX_RUBRIC_DIMS (16) and test duplicate & overflow rejection
    let i: u32 = 0;
    while (i < EVAL_JUDGE_MAX_RUBRIC_DIMS as u32) : (i += 1) {
        expect_eq(scorer.addDimension(RubricDimSpec {
            dim_id: i + 1,
            weight_bps: 100,
            min_score: 0.0,
            max_score: 1.0,
        }), true);
    }
    expect_eq(scorer.addDimension(RubricDimSpec { dim_id: 1, weight_bps: 100, min_score: 0.0, max_score: 1.0 }), false);
    expect_eq(scorer.addDimension(RubricDimSpec { dim_id: 99, weight_bps: 100, min_score: 0.0, max_score: 1.0 }), false);

    // Length mismatch, NaN, Inf, and out-of-bounds score rejection
    const short_s: [2]f64 = [0.5, 0.5];
    expect_eq(scorer.evaluate(short_s[0..]).verdict, RubricVerdictKind.invalid_input);

    let nan_s: [EVAL_JUDGE_MAX_RUBRIC_DIMS]f64 = [0.8; EVAL_JUDGE_MAX_RUBRIC_DIMS];
    nan_s[3] = std.math.nan(f64);
    expect_eq(scorer.evaluate(nan_s[0..]).verdict, RubricVerdictKind.invalid_input);

    let oob_s: [EVAL_JUDGE_MAX_RUBRIC_DIMS]f64 = [0.8; EVAL_JUDGE_MAX_RUBRIC_DIMS];
    oob_s[5] = 1.05;
    expect_eq(scorer.evaluate(oob_s[0..]).verdict, RubricVerdictKind.invalid_input);
});

test("evalJudge: ArenaEloTable Logistic expected win-rate & zero-sum update precision", () => {
    // Standard Logistic expected score check
    expect_eq(approxEqF64(ArenaEloTable.expectedScore(1500.0, 1500.0), 0.5, 0.00001), true);
    // +400 Elo difference => 1 / (1 + 10^-1) = 10/11 ~= 0.9090909
    expect_eq(approxEqF64(ArenaEloTable.expectedScore(1900.0, 1500.0), 10.0 / 11.0, 0.0001), true);
    expect_eq(approxEqF64(ArenaEloTable.expectedScore(1500.0, 1900.0), 1.0 / 11.0, 0.0001), true);

    let arena = ArenaEloTable.init(32.0, 1500.0) ?? unreachable;
    expect_eq(arena.registerModel(10), true);
    expect_eq(arena.registerModel(20), true);

    // Equal ratings: A wins => delta_a = 32 * (1.0 - 0.5) = +16.0, delta_b = -16.0
    const u1 = arena.recordMatch(10, 20, 1.0, 1.0);
    expect_eq(u1.applied, true);
    expect_eq(approxEqF64(u1.expected_a, 0.5, 0.00001), true);
    expect_eq(approxEqF64(u1.delta_a, 16.0, 0.0001), true);
    expect_eq(approxEqF64(u1.delta_b, -16.0, 0.0001), true);
    expect_eq(approxEqF64(u1.new_elo_a, 1516.0, 0.0001), true);
    expect_eq(approxEqF64(u1.new_elo_b, 1484.0, 0.0001), true);

    const m10 = arena.getModel(10) ?? unreachable;
    const m20 = arena.getModel(20) ?? unreachable;
    expect_eq(m10.wins, 1 as u64);
    expect_eq(m10.losses, 0 as u64);
    expect_eq(m20.wins, 0 as u64);
    expect_eq(m20.losses, 1 as u64);
    // Zero-sum invariant: sum of Elos == 3000.0
    expect_eq(approxEqF64(m10.elo + m20.elo, 3000.0, 0.0001), true);
});

test("evalJudge: multi-model tournament, descending leaderboard & buffer truncation guard", () => {
    let arena = ArenaEloTable.defaults();
    expect_eq(arena.registerModel(3), true);
    expect_eq(arena.registerModel(1), true);
    expect_eq(arena.registerModel(2), true);
    expect_eq(arena.registerModel(4), true);

    // Model 2 beats 1 and 3; Model 1 beats 3; Model 4 stays at 1500
    * = arena.recordMatch(2, 1, 1.0, 1.0);
    * = arena.recordMatch(2, 3, 1.0, 1.0);
    * = arena.recordMatch(1, 3, 1.0, 1.0);

    let full_ranks: [8]EloRankEntry = [EloRankEntry {}; 8];
    let full_s: []EloRankEntry = full_ranks[0..];
    const written: usize = arena.leaderboard(*full_s);
    expect_eq(written, 4 as usize);

    // Expected order: Model 2 (2 wins, >1530) > Model 4 (1500) ~= Model 1 (~1500, 1 win) > Model 3 (<1470)
    expect_eq(full_ranks[0].rank, 1 as usize);
    expect_eq(full_ranks[0].model_id, 2 as u32);
    expect_eq(full_ranks[0].wins, 2 as u64);
    expect_eq(full_ranks[3].rank, 4 as usize);
    expect_eq(full_ranks[3].model_id, 3 as u32);
    // Verify strictly non-increasing Elo across leaderboard
    expect_eq(full_ranks[0].elo >= full_ranks[1].elo, true);
    expect_eq(full_ranks[1].elo >= full_ranks[2].elo, true);
    expect_eq(full_ranks[2].elo >= full_ranks[3].elo, true);

    // Undersized output slice returns top-2 cleanly without OOB
    let top2: [2]EloRankEntry = [EloRankEntry {}; 2];
    let top2_s: []EloRankEntry = top2[0..];
    expect_eq(arena.leaderboard(*top2_s), 2 as usize);
    expect_eq(top2[0].model_id, 2 as u32);
});

test("evalJudge: ArenaEloTable boundary guards (invalid K/Elo, unregistered, self-match, 32-model cap)", () => {
    // Invalid K-factor or initial Elo
    expect_eq(ArenaEloTable.init(0.0, 1500.0) == null, true);
    expect_eq(ArenaEloTable.init(-10.0, 1500.0) == null, true);
    expect_eq(ArenaEloTable.init(300.0, 1500.0) == null, true);
    expect_eq(ArenaEloTable.init(std.math.nan(f64), 1500.0) == null, true);
    expect_eq(ArenaEloTable.init(32.0, -1.0) == null, true);
    expect_eq(ArenaEloTable.init(32.0, std.math.inf(f64)) == null, true);

    let arena = ArenaEloTable.defaults();
    let i: u32 = 0;
    while (i < EVAL_JUDGE_MAX_MODELS as u32) : (i += 1) {
        expect_eq(arena.registerModel(i + 1), true);
    }
    // Duplicate & full-capacity rejection
    expect_eq(arena.registerModel(1), false);
    expect_eq(arena.registerModel(999), false);
    expect_eq(arena.modelCount(), EVAL_JUDGE_MAX_MODELS);

    // Unregistered model, self-match, invalid score_a, invalid weight
    expect_eq(arena.recordMatch(1, 999, 1.0, 1.0).applied, false);
    expect_eq(arena.recordMatch(999, 1, 1.0, 1.0).applied, false);
    expect_eq(arena.recordMatch(1, 1, 1.0, 1.0).applied, false);
    expect_eq(arena.recordMatch(1, 2, -0.1, 1.0).applied, false);
    expect_eq(arena.recordMatch(1, 2, 1.2, 1.0).applied, false);
    expect_eq(arena.recordMatch(1, 2, std.math.nan(f64), 1.0).applied, false);
    expect_eq(arena.recordMatch(1, 2, 1.0, 0.0).applied, false);
    expect_eq(arena.recordMatch(1, 2, 1.0, -1.0).applied, false);
    expect_eq(arena.recordMatch(1, 2, 1.0, std.math.inf(f64)).applied, false);
});
