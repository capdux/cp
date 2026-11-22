// Origin: Pure-CP Stdlib Test Suite (lib/agent/ab_evaluator.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc Paired A/B Eval & Statistical Significance Gate:
//   1. Insufficient sample counts (< min_paired_samples) -> inconclusive_low_samples + reset() reuse
//   2. Statistically significant lift within latency/cost guardrails -> ship_candidate_b
//   3. Statistically significant regression in candidate score -> keep_baseline_a
//   4. Significant score lift rejected by latency or cost regression (> bps cap) -> reject_cost_or_latency_regression
//   5. No statistically significant difference (noisy tie & exact zero-variance tie) -> statistically_tied
//   6. Exact Wilson Score 95% CI verification at 0%, 50%, 100%, 0/0, and passes > total bounds
//   7. Exact Welford variance & Welch t-stat numerical verification + NaN/Inf & invalid config guards

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AbVerdictKind,
    AbEvalGateConfig,
    AbArmStats,
    AbComparisonReport,
    computeWilsonInterval95,
    AgentAbEvaluator,
} from "cap:agent";

fn approxEq(actual: f64, expected: f64, tol: f64): bool {
    const diff: f64 = if (actual >= expected) (actual - expected) else (expected - actual);
    return diff <= tol;
}

// ============================================================================
// Test 1: Low Samples -> inconclusive_low_samples & reset() Lifecycle
// ============================================================================

fn testLowSamplesAndReset()! {
    let ev = cap.agent.AgentAbEvaluator.init();
    const cfg = AbEvalGateConfig.defaults();
    expect_eq(cfg.isValid(), true);
    expect_eq(cfg.min_paired_samples, 20);
    expect_eq(cfg.max_latency_regression_bps, 1500);
    expect_eq(cfg.max_cost_regression_bps, 1500);

    // 0 samples
    const r0 = ev.evaluate(cfg);
    expect_eq(r0.paired_samples, 0);
    expect_eq(r0.verdict == AbVerdictKind.inconclusive_low_samples, true);
    expect_eq(approxEq(r0.arm_a.wilson_lo_95, 0.0, 1.0e-12), true);
    expect_eq(approxEq(r0.arm_a.wilson_hi_95, 0.0, 1.0e-12), true);

    // 19 samples (just 1 below default threshold 20)
    let i: u64 = 0;
    while (i < 19) : (i += 1) {
        const ok = ev.record_pair(0.60, false, 500, 1000, 0.95, true, 500, 1000);
        expect_eq(ok, true);
    }
    const r19 = ev.evaluate(cfg);
    expect_eq(r19.paired_samples, 19);
    expect_eq(r19.b_wins, 19);
    expect_eq(r19.a_wins, 0);
    expect_eq(r19.ties, 0);
    expect_eq(r19.verdict == AbVerdictKind.inconclusive_low_samples, true);

    // reset() clears all accumulators cleanly
    ev.reset();
    const r_after_reset = ev.evaluate(cfg);
    expect_eq(r_after_reset.paired_samples, 0);
    expect_eq(r_after_reset.b_wins, 0);
    expect_eq(r_after_reset.arm_a.count, 0);
    expect_eq(r_after_reset.arm_b.passes, 0);
    expect_eq(r_after_reset.verdict == AbVerdictKind.inconclusive_low_samples, true);
}

// ============================================================================
// Test 2: Significant Lift Within Latency & Cost Guardrails -> ship_candidate_b
// ============================================================================

fn testShipCandidateB()! {
    let ev = AgentAbEvaluator.init();
    const cfg = AbEvalGateConfig.defaults();

    // 24 paired samples: Arm A mean ~0.715, Arm B mean ~0.865 (+0.15 lift),
    // latency +10% (1000 bps <= 1500), cost +12% (1200 bps <= 1500).
    let i: u64 = 0;
    while (i < 24) : (i += 1) {
        const offset: f64 = ((i % 4) as f64) * 0.01;
        const ok = ev.record_pair(
            0.70 + offset, i % 3 != 0, 500, 1000,
            0.85 + offset, true,       550, 1120,
        );
        expect_eq(ok, true);
    }

    const r: AbComparisonReport = ev.evaluate(cfg);
    expect_eq(r.verdict == AbVerdictKind.ship_candidate_b, true);
    expect_eq(r.paired_samples, 24);
    expect_eq(r.b_wins, 24);
    expect_eq(r.a_wins, 0);
    expect_eq(r.ties, 0);
    expect_eq(approxEq(r.score_delta_mean, 0.15, 1.0e-9), true);
    expect_eq(r.welch_t_stat > 10.0, true);
    expect_eq(r.latency_delta_bps, 1000);
    expect_eq(r.cost_delta_bps, 1200);
    expect_eq(r.arm_a.passes, 16);
    expect_eq(r.arm_b.passes, 24);
    expect_eq(r.arm_b.wilson_hi_95 <= 1.0 && r.arm_b.wilson_lo_95 > 0.85, true);
}

// ============================================================================
// Test 3: Candidate Score Regresses Significantly -> keep_baseline_a
// ============================================================================

fn testKeepBaselineAOnScoreDrop()! {
    let ev = AgentAbEvaluator.init();
    const cfg = AbEvalGateConfig.defaults();

    // Candidate B is faster and cheaper, but scores significantly worse (-0.18).
    let i: u64 = 0;
    while (i < 24) : (i += 1) {
        const offset: f64 = ((i % 4) as f64) * 0.01;
        * = ev.record_pair(
            0.88 + offset, true,       600, 1500,
            0.70 + offset, i % 2 == 0, 400, 900,
        );
    }

    const r = ev.evaluate(cfg);
    expect_eq(r.verdict == AbVerdictKind.keep_baseline_a, true);
    expect_eq(r.a_wins, 24);
    expect_eq(r.b_wins, 0);
    expect_eq(approxEq(r.score_delta_mean, -0.18, 1.0e-9), true);
    expect_eq(r.welch_t_stat < -10.0, true);
    expect_eq(r.latency_delta_bps < 0, true);
    expect_eq(r.cost_delta_bps < 0, true);
}

// ============================================================================
// Test 4: Latency or Cost Regression Rejects Shipping Candidate B
// ============================================================================

fn testRejectCostOrLatencyRegression()! {
    const cfg = AbEvalGateConfig.defaults();

    // 4a. Significant +0.20 score lift, cost flat, but latency +20% (2000 bps > 1500 bps)
    let ev_lat = AgentAbEvaluator.init();
    let i: u64 = 0;
    while (i < 20) : (i += 1) {
        const jitter: f64 = ((i % 2) as f64) * 0.02;
        * = ev_lat.record_pair(
            0.70 + jitter, true, 500, 1000,
            0.90 + jitter, true, 600, 1000,
        );
    }
    const r_lat = ev_lat.evaluate(cfg);
    expect_eq(r_lat.verdict == AbVerdictKind.reject_cost_or_latency_regression, true);
    expect_eq(r_lat.latency_delta_bps, 2000);
    expect_eq(r_lat.cost_delta_bps, 0);
    expect_eq(r_lat.welch_t_stat > cfg.z_threshold_f64, true);

    // 4b. Significant +0.20 score lift, latency flat, but cost +25% (2500 bps > 1500 bps)
    let ev_cost = AgentAbEvaluator.init();
    i = 0;
    while (i < 20) : (i += 1) {
        const jitter: f64 = ((i % 2) as f64) * 0.02;
        * = ev_cost.record_pair(
            0.70 + jitter, true, 500, 1000,
            0.90 + jitter, true, 500, 1250,
        );
    }
    const r_cost = ev_cost.evaluate(cfg);
    expect_eq(r_cost.verdict == AbVerdictKind.reject_cost_or_latency_regression, true);
    expect_eq(r_cost.latency_delta_bps, 0);
    expect_eq(r_cost.cost_delta_bps, 2500);

    // 4c. Baseline cost == 0, candidate cost > 0 -> large positive bps -> reject
    let ev_zero_base = AgentAbEvaluator.init();
    i = 0;
    while (i < 20) : (i += 1) {
        * = ev_zero_base.record_pair(0.70, true, 100, 0, 0.90, true, 100, 50);
    }
    const r_zb = ev_zero_base.evaluate(cfg);
    expect_eq(r_zb.verdict == AbVerdictKind.reject_cost_or_latency_regression, true);
    expect_eq(r_zb.cost_delta_bps > (cfg.max_cost_regression_bps as i64), true);
}

// ============================================================================
// Test 5: Statistically Tied (Noisy Near-Equal & Zero-Variance Exact Tie)
// ============================================================================

fn testStatisticallyTied()! {
    const cfg = AbEvalGateConfig.defaults();

    // 5a. Alternating small wins/losses with high variance -> |t| << 1.96
    let ev_noisy = AgentAbEvaluator.init();
    let i: u64 = 0;
    while (i < 24) : (i += 1) {
        const a_score: f64 = if (i % 2 == 0) 0.40 else 0.90;
        const b_score: f64 = if (i % 2 == 0) 0.41 else 0.90;
        * = ev_noisy.record_pair(a_score, true, 500, 1000, b_score, true, 505, 1010);
    }
    const r_noisy = ev_noisy.evaluate(cfg);
    expect_eq(r_noisy.verdict == AbVerdictKind.statistically_tied, true);
    expect_eq(r_noisy.b_wins, 12);
    expect_eq(r_noisy.ties, 12);
    expect_eq(r_noisy.a_wins, 0);
    expect_eq(r_noisy.welch_t_stat < cfg.z_threshold_f64 && r_noisy.welch_t_stat > -cfg.z_threshold_f64, true);

    // 5b. Zero-variance exact tie -> welch_t_stat == 0.0, statistically_tied
    let ev_exact = AgentAbEvaluator.init();
    i = 0;
    while (i < 20) : (i += 1) {
        * = ev_exact.record_pair(0.80, true, 200, 500, 0.80, true, 200, 500);
    }
    const r_exact = ev_exact.evaluate(cfg);
    expect_eq(r_exact.verdict == AbVerdictKind.statistically_tied, true);
    expect_eq(r_exact.ties, 20);
    expect_eq(approxEq(r_exact.welch_t_stat, 0.0, 1.0e-12), true);
    expect_eq(approxEq(r_exact.arm_a.variance_score, 0.0, 1.0e-12), true);
}

// ============================================================================
// Test 6: Wilson Score 95% CI Exact Boundary Verification (0%, 50%, 100%)
// ============================================================================

fn testWilsonInterval95Boundaries()! {
    // 0/0 -> [0.0, 0.0]
    const ci_empty = computeWilsonInterval95(0, 0);
    expect_eq(approxEq(ci_empty[0], 0.0, 1.0e-12), true);
    expect_eq(approxEq(ci_empty[1], 0.0, 1.0e-12), true);

    // 0/100 (0%): lo == 0.0, hi ≈ 0.036993
    const ci_0 = computeWilsonInterval95(0, 100);
    expect_eq(approxEq(ci_0[0], 0.0, 1.0e-12), true);
    expect_eq(approxEq(ci_0[1], 0.036993, 1.0e-4), true);

    // 50/100 (50%): symmetric around 0.5, lo ≈ 0.403832, hi ≈ 0.596168
    const ci_50 = computeWilsonInterval95(50, 100);
    expect_eq(approxEq(ci_50[0], 0.403832, 1.0e-4), true);
    expect_eq(approxEq(ci_50[1], 0.596168, 1.0e-4), true);
    expect_eq(approxEq(ci_50[0] + ci_50[1], 1.0, 1.0e-12), true);

    // 100/100 (100%): lo ≈ 0.963007, hi == 1.0
    const ci_100 = computeWilsonInterval95(100, 100);
    expect_eq(approxEq(ci_100[0], 0.963007, 1.0e-4), true);
    expect_eq(approxEq(ci_100[1], 1.0, 1.0e-12), true);

    // Defensive clamp when passes > total (e.g. 120/100 treated as 100/100)
    const ci_over = computeWilsonInterval95(120, 100);
    expect_eq(approxEq(ci_over[0], ci_100[0], 1.0e-12), true);
    expect_eq(approxEq(ci_over[1], 1.0, 1.0e-12), true);
}

// ============================================================================
// Test 7: Exact Welford Variance / Welch t-stat + NaN/Inf & Invalid Config Guards
// ============================================================================

fn testWelfordAccuracyAndDefensiveGuards()! {
    let ev = AgentAbEvaluator.init();

    // Hand-verifiable 4-sample dataset:
    //   Arm A scores: [2.0, 4.0, 4.0, 6.0] -> mean = 4.0, M2 = 4+0+0+4 = 8.0, var = 8/3
    //   Arm B scores: [6.0, 8.0, 8.0, 10.0] -> mean = 8.0, M2 = 4+0+0+4 = 8.0, var = 8/3
    //   delta = 4.0, se = sqrt((8/3 + 8/3) / 4) = sqrt(4/3) = 2 / sqrt(3)
    //   Welch t = 4.0 / (2 / sqrt(3)) = 2 * sqrt(3) ≈ 3.4641016151377544
    const a_vals: [4]f64 = [2.0, 4.0, 4.0, 6.0];
    const b_vals: [4]f64 = [6.0, 8.0, 8.0, 10.0];

    let i: usize = 0;
    while (i < 4) : (i += 1) {
        expect_eq(ev.record_pair(a_vals[i], true, 100, 200, b_vals[i], true, 100, 200), true);
    }

    // Attempt to inject NaN, +Inf, -Inf -> must all return false and NOT mutate counts or stats
    const nan_f: f64 = std.math.nan(f64);
    const inf_f: f64 = std.math.inf(f64);
    expect_eq(ev.record_pair(nan_f, true, 100, 200, 8.0, true, 100, 200), false);
    expect_eq(ev.record_pair(4.0, true, 100, 200, nan_f, true, 100, 200), false);
    expect_eq(ev.record_pair(inf_f, true, 100, 200, 8.0, true, 100, 200), false);
    expect_eq(ev.record_pair(4.0, true, 100, 200, -inf_f, true, 100, 200), false);

    const cfg_small = AbEvalGateConfig{
        min_paired_samples: 4,
        min_score_lift_f64: 1.0,
        z_threshold_f64: 1.96,
        max_latency_regression_bps: 1500,
        max_cost_regression_bps: 1500,
    };
    expect_eq(cfg_small.isValid(), true);

    const r = ev.evaluate(cfg_small);
    expect_eq(r.paired_samples, 4);
    expect_eq(approxEq(r.arm_a.mean_score, 4.0, 1.0e-12), true);
    expect_eq(approxEq(r.arm_b.mean_score, 8.0, 1.0e-12), true);
    expect_eq(approxEq(r.arm_a.variance_score, 8.0 / 3.0, 1.0e-12), true);
    expect_eq(approxEq(r.arm_b.variance_score, 8.0 / 3.0, 1.0e-12), true);
    expect_eq(approxEq(r.score_delta_mean, 4.0, 1.0e-12), true);
    expect_eq(approxEq(r.welch_t_stat, 3.4641016151377544, 1.0e-10), true);
    expect_eq(r.verdict == AbVerdictKind.ship_candidate_b, true);

    // Invalid config guards -> isValid() == false & evaluate() returns inconclusive_low_samples
    const bad_min = AbEvalGateConfig{ min_paired_samples: 1 };
    const bad_lift = AbEvalGateConfig{ min_paired_samples: 4, min_score_lift_f64: -0.01 };
    const bad_z = AbEvalGateConfig{ min_paired_samples: 4, z_threshold_f64: 0.0 };
    const bad_nan = AbEvalGateConfig{ min_paired_samples: 4, z_threshold_f64: nan_f };

    expect_eq(bad_min.isValid(), false);
    expect_eq(bad_lift.isValid(), false);
    expect_eq(bad_z.isValid(), false);
    expect_eq(bad_nan.isValid(), false);

    expect_eq(ev.evaluate(bad_min).verdict == AbVerdictKind.inconclusive_low_samples, true);
    expect_eq(ev.evaluate(bad_lift).verdict == AbVerdictKind.inconclusive_low_samples, true);
    expect_eq(ev.evaluate(bad_z).verdict == AbVerdictKind.inconclusive_low_samples, true);
    expect_eq(ev.evaluate(bad_nan).verdict == AbVerdictKind.inconclusive_low_samples, true);
}

// ============================================================================
// Suite Registration
// ============================================================================

test("agent ab_evaluator: low samples returns inconclusive_low_samples & reset clears state", testLowSamplesAndReset);
test("agent ab_evaluator: significant lift within guardrails returns ship_candidate_b", testShipCandidateB);
test("agent ab_evaluator: significant score drop returns keep_baseline_a", testKeepBaselineAOnScoreDrop);
test("agent ab_evaluator: latency or cost regression rejects shipping candidate B", testRejectCostOrLatencyRegression);
test("agent ab_evaluator: noisy tie and zero-variance tie return statistically_tied", testStatisticallyTied);
test("agent ab_evaluator: Wilson 95% CI exact boundaries at 0%, 50%, 100%, and 0/0", testWilsonInterval95Boundaries);
test("agent ab_evaluator: Welford variance, Welch t-stat precision & NaN/Inf/invalid config guards", testWelfordAccuracyAndDefensiveGuards);
