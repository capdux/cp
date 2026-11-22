// Origin: Pure-CP Stdlib (AI-Native Zero-Alloc Paired A/B Eval & Statistical Significance Gate) | Created: 2026-11-22
// Pure CP zero-heap-allocation Paired A/B Evaluation & Statistical Significance Release Gate for AI Agents:
//   - Streaming O(1)-memory Welford online mean & unbiased variance for Arm A (baseline) and Arm B (candidate)
//   - Two-sample Welch t-statistic with deterministic zero-variance degeneration handling
//   - Exact Wilson Score 95% confidence interval (`computeWilsonInterval95`) clamped to `[0.0, 1.0]`
//   - Latency & micro-USD cost regression guardrails in basis points (`bps`, 100 bps = 1%)
//   - Strict NaN/Inf rejection on ingestion & deterministic 5-way release verdict (`AbVerdictKind`)

const std = @import("std");

// Exact 97.5th percentile of standard normal distribution for two-sided 95% Wilson CI.
const WILSON_Z_95: f64 = 1.959963984540054;
const SCORE_TIE_EPS: f64 = 1.0e-12;
const ZERO_SE_EPS: f64 = 1.0e-15;
const DEGENERATE_T_MAGNITUDE: f64 = 1_000_000.0;
const BPS_CLAMP_LIMIT: f64 = 1_000_000_000.0;

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn clamp01(x: f64): f64 {
    if (x < 0.0) return 0.0;
    if (x > 1.0) return 1.0;
    return x;
}

fn absF64(x: f64): f64 {
    if (x < 0.0) return -x;
    return x;
}

fn computeDeltaBps(base_mean: f64, cand_mean: f64): i64 {
    if (!isFiniteF64(base_mean) || !isFiniteF64(cand_mean)) {
        return 0;
    }
    if (base_mean <= SCORE_TIE_EPS) {
        if (cand_mean <= SCORE_TIE_EPS) {
            return 0;
        }
        // Baseline is effectively zero while candidate is strictly positive:
        // report a large deterministic regression exceeding any u16 bps threshold.
        return 100_000;
    }
    let bps_f = ((cand_mean - base_mean) / base_mean) * 10000.0;
    if (bps_f > BPS_CLAMP_LIMIT) {
        bps_f = BPS_CLAMP_LIMIT;
    } else if (bps_f < -BPS_CLAMP_LIMIT) {
        bps_f = -BPS_CLAMP_LIMIT;
    }
    if (bps_f >= 0.0) {
        return (bps_f + 0.5) as i64;
    }
    return (bps_f - 0.5) as i64;
}

// ============================================================================
// 1. Public Enums, Configuration & Report Models
// ============================================================================

/// Deterministic release gate decision for a paired A/B AI Agent evaluation.
pub enum AbVerdictKind {
    inconclusive_low_samples,
    ship_candidate_b,
    keep_baseline_a,
    reject_cost_or_latency_regression,
    statistically_tied,
}

/// Statistical significance and latency/cost regression thresholds for `AgentAbEvaluator.evaluate`.
pub struct AbEvalGateConfig {
    min_paired_samples: usize = 20;
    min_score_lift_f64: f64 = 0.02;
    z_threshold_f64: f64 = 1.96;
    max_latency_regression_bps: u16 = 1500;
    max_cost_regression_bps: u16 = 1500;

    /// Returns production-default 95% confidence gate thresholds
    /// (`min_paired_samples = 20`, `min_score_lift_f64 = 0.02`, `z_threshold_f64 = 1.96`,
    /// `max_latency_regression_bps = 1500`, `max_cost_regression_bps = 1500`).
    static fn defaults(): AbEvalGateConfig {
        return AbEvalGateConfig{};
    }

    /// Validates all gate configuration invariants:
    /// - `min_paired_samples >= 2` (required for unbiased sample variance)
    /// - `min_score_lift_f64` is finite and `>= 0.0`
    /// - `z_threshold_f64` is finite and `> 0.0`
    fn isValid(): bool {
        if (_.min_paired_samples < 2) return false;
        if (!isFiniteF64(_.min_score_lift_f64) || _.min_score_lift_f64 < 0.0) return false;
        if (!isFiniteF64(_.z_threshold_f64) || _.z_threshold_f64 <= 0.0) return false;
        return true;
    }
}

/// Aggregated statistics for a single evaluation arm (Arm A baseline or Arm B candidate).
pub struct AbArmStats {
    count: u64 = 0;
    passes: u64 = 0;
    mean_score: f64 = 0.0;
    variance_score: f64 = 0.0;
    wilson_lo_95: f64 = 0.0;
    wilson_hi_95: f64 = 0.0;
    mean_latency_ms: f64 = 0.0;
    mean_cost_micro_usd: f64 = 0.0;
}

/// Complete paired A/B comparison report produced by `AgentAbEvaluator.evaluate`.
pub struct AbComparisonReport {
    paired_samples: u64 = 0;
    b_wins: u64 = 0;
    a_wins: u64 = 0;
    ties: u64 = 0;
    score_delta_mean: f64 = 0.0;
    welch_t_stat: f64 = 0.0;
    latency_delta_bps: i64 = 0;
    cost_delta_bps: i64 = 0;
    arm_a: AbArmStats = AbArmStats{};
    arm_b: AbArmStats = AbArmStats{};
    verdict: AbVerdictKind = AbVerdictKind.inconclusive_low_samples;
}

// ============================================================================
// 2. Pure Statistical Helpers
// ============================================================================

/// Computes the exact two-sided 95% Wilson Score confidence interval `[lo, hi]`
/// for `passes` successes out of `total` trials, clamped strictly to `[0.0, 1.0]`.
/// Returns `[0.0, 0.0]` when `total == 0`.
pub fn computeWilsonInterval95(passes: u64, total: u64): [2]f64 {
    if (total == 0) {
        return [0.0, 0.0];
    }
    const clamped_passes: u64 = if (passes > total) total else passes;
    const n: f64 = total as f64;
    const p: f64 = (clamped_passes as f64) / n;
    const z2: f64 = WILSON_Z_95 * WILSON_Z_95;
    const z2_over_n: f64 = z2 / n;
    const denom: f64 = 1.0 + z2_over_n;

    const center: f64 = (p + z2_over_n * 0.5) / denom;
    let rad_inner: f64 = (p * (1.0 - p) + z2_over_n * 0.25) / n;
    if (rad_inner < 0.0) {
        rad_inner = 0.0;
    }
    const margin: f64 = (WILSON_Z_95 * std.math.sqrt(rad_inner)) / denom;

    const lo: f64 = clamp01(center - margin);
    const hi: f64 = clamp01(center + margin);
    return [lo, hi];
}

// ============================================================================
// 3. Streaming Zero-Alloc Paired A/B Evaluator
// ============================================================================

/// O(1)-memory streaming paired A/B evaluator using Welford's online algorithm.
/// Supports millions of paired samples with zero dynamic heap allocation.
pub struct AgentAbEvaluator {
    paired_samples: u64 = 0;
    b_wins: u64 = 0;
    a_wins: u64 = 0;
    ties: u64 = 0;

    passes_a: u64 = 0;
    score_mean_a: f64 = 0.0;
    score_m2_a: f64 = 0.0;
    latency_mean_a: f64 = 0.0;
    cost_mean_a: f64 = 0.0;

    passes_b: u64 = 0;
    score_mean_b: f64 = 0.0;
    score_m2_b: f64 = 0.0;
    latency_mean_b: f64 = 0.0;
    cost_mean_b: f64 = 0.0;

    /// Creates a fresh zero-initialized `AgentAbEvaluator`.
    static fn init(): AgentAbEvaluator {
        return AgentAbEvaluator{};
    }

    /// Resets all accumulated paired statistics back to zero.
    fn reset(*_) {
        _.paired_samples = 0;
        _.b_wins = 0;
        _.a_wins = 0;
        _.ties = 0;

        _.passes_a = 0;
        _.score_mean_a = 0.0;
        _.score_m2_a = 0.0;
        _.latency_mean_a = 0.0;
        _.cost_mean_a = 0.0;

        _.passes_b = 0;
        _.score_mean_b = 0.0;
        _.score_m2_b = 0.0;
        _.latency_mean_b = 0.0;
        _.cost_mean_b = 0.0;
    }

    /// Records a single paired evaluation observation for baseline Arm A and candidate Arm B.
    /// Rejects `NaN` or `Inf` scores without mutating state and returns `false`.
    fn record_pair(
        *_,
        score_a: f64,
        pass_a: bool,
        latency_a_ms: u64,
        cost_a_micro_usd: u64,
        score_b: f64,
        pass_b: bool,
        latency_b_ms: u64,
        cost_b_micro_usd: u64,
    ): bool {
        if (!isFiniteF64(score_a) || !isFiniteF64(score_b)) {
            return false;
        }

        _.paired_samples += 1;
        const n_f: f64 = _.paired_samples as f64;

        // Arm A: Welford online score mean & M2, plus online latency & cost means
        if (pass_a) {
            _.passes_a += 1;
        }
        const delta_a: f64 = score_a - _.score_mean_a;
        _.score_mean_a += delta_a / n_f;
        const delta2_a: f64 = score_a - _.score_mean_a;
        _.score_m2_a += delta_a * delta2_a;

        const lat_a_f: f64 = latency_a_ms as f64;
        _.latency_mean_a += (lat_a_f - _.latency_mean_a) / n_f;
        const cost_a_f: f64 = cost_a_micro_usd as f64;
        _.cost_mean_a += (cost_a_f - _.cost_mean_a) / n_f;

        // Arm B: Welford online score mean & M2, plus online latency & cost means
        if (pass_b) {
            _.passes_b += 1;
        }
        const delta_b: f64 = score_b - _.score_mean_b;
        _.score_mean_b += delta_b / n_f;
        const delta2_b: f64 = score_b - _.score_mean_b;
        _.score_m2_b += delta_b * delta2_b;

        const lat_b_f: f64 = latency_b_ms as f64;
        _.latency_mean_b += (lat_b_f - _.latency_mean_b) / n_f;
        const cost_b_f: f64 = cost_b_micro_usd as f64;
        _.cost_mean_b += (cost_b_f - _.cost_mean_b) / n_f;

        // Head-to-head paired win/loss/tie tally
        if (score_b > score_a + SCORE_TIE_EPS) {
            _.b_wins += 1;
        } else if (score_a > score_b + SCORE_TIE_EPS) {
            _.a_wins += 1;
        } else {
            _.ties += 1;
        }

        return true;
    }

    /// Evaluates accumulated paired samples against `cfg` and returns a complete
    /// `AbComparisonReport` with Welch's t-statistic, Wilson 95% CIs, latency/cost
    /// regression basis points, and deterministic `AbVerdictKind`.
    fn evaluate(cfg: AbEvalGateConfig): AbComparisonReport {
        const n: u64 = _.paired_samples;

        let var_a: f64 = 0.0;
        let var_b: f64 = 0.0;
        if (n >= 2) {
            const denom: f64 = (n - 1) as f64;
            var_a = _.score_m2_a / denom;
            var_b = _.score_m2_b / denom;
            if (var_a < 0.0) var_a = 0.0;
            if (var_b < 0.0) var_b = 0.0;
        }

        const ci_a: [2]f64 = computeWilsonInterval95(_.passes_a, n);
        const ci_b: [2]f64 = computeWilsonInterval95(_.passes_b, n);

        const arm_a = AbArmStats{
            count: n,
            passes: _.passes_a,
            mean_score: _.score_mean_a,
            variance_score: var_a,
            wilson_lo_95: ci_a[0],
            wilson_hi_95: ci_a[1],
            mean_latency_ms: _.latency_mean_a,
            mean_cost_micro_usd: _.cost_mean_a,
        };

        const arm_b = AbArmStats{
            count: n,
            passes: _.passes_b,
            mean_score: _.score_mean_b,
            variance_score: var_b,
            wilson_lo_95: ci_b[0],
            wilson_hi_95: ci_b[1],
            mean_latency_ms: _.latency_mean_b,
            mean_cost_micro_usd: _.cost_mean_b,
        };

        const score_delta_mean: f64 = _.score_mean_b - _.score_mean_a;

        let welch_t: f64 = 0.0;
        if (n >= 2) {
            const se_sq: f64 = (var_a + var_b) / (n as f64);
            const se: f64 = if (se_sq > 0.0) std.math.sqrt(se_sq) else 0.0;
            if (se > ZERO_SE_EPS) {
                welch_t = score_delta_mean / se;
            } else if (absF64(score_delta_mean) <= SCORE_TIE_EPS) {
                welch_t = 0.0;
            } else if (score_delta_mean > 0.0) {
                welch_t = DEGENERATE_T_MAGNITUDE;
            } else {
                welch_t = -DEGENERATE_T_MAGNITUDE;
            }
        }

        const latency_delta_bps: i64 = if (n > 0) computeDeltaBps(_.latency_mean_a, _.latency_mean_b) else 0;
        const cost_delta_bps: i64 = if (n > 0) computeDeltaBps(_.cost_mean_a, _.cost_mean_b) else 0;

        // Deterministic release verdict priority:
        //   1. Invalid config or insufficient samples -> inconclusive_low_samples
        //   2. Statistically significant score drop   -> keep_baseline_a
        //   3. Latency or cost regression above cap   -> reject_cost_or_latency_regression
        //   4. Significant score lift meeting min     -> ship_candidate_b
        //   5. Otherwise                              -> statistically_tied
        let verdict: AbVerdictKind = AbVerdictKind.statistically_tied;
        if (!cfg.isValid() || n < (cfg.min_paired_samples as u64)) {
            verdict = AbVerdictKind.inconclusive_low_samples;
        } else if (score_delta_mean < 0.0 && welch_t <= -cfg.z_threshold_f64) {
            verdict = AbVerdictKind.keep_baseline_a;
        } else if (
            latency_delta_bps > (cfg.max_latency_regression_bps as i64) ||
            cost_delta_bps > (cfg.max_cost_regression_bps as i64)
        ) {
            verdict = AbVerdictKind.reject_cost_or_latency_regression;
        } else if (score_delta_mean >= cfg.min_score_lift_f64 && welch_t >= cfg.z_threshold_f64) {
            verdict = AbVerdictKind.ship_candidate_b;
        } else {
            verdict = AbVerdictKind.statistically_tied;
        }

        return AbComparisonReport{
            paired_samples: n,
            b_wins: _.b_wins,
            a_wins: _.a_wins,
            ties: _.ties,
            score_delta_mean: score_delta_mean,
            welch_t_stat: welch_t,
            latency_delta_bps: latency_delta_bps,
            cost_delta_bps: cost_delta_bps,
            arm_a: arm_a,
            arm_b: arm_b,
            verdict: verdict,
        };
    }
}
