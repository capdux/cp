// Origin: Pure-CP Stdlib (Zero-Alloc AI Agent Eval Judge: Position-Bias Pairwise Mitigation, Rubric Scorer & Arena Elo Table) | Created: 2026-11-22
// Pure CP zero-heap-allocation automated evaluation judge & aggregator for AI Agents and LLMs:
//   1. Position-Bias Mitigated Pairwise Judge (`resolvePairwiseSwap` & `PairwiseJudgeAggregator`):
//      Combines forward (A vs B) and reversed (B vs A) verdicts to isolate position bias,
//      computes `position_bias_bps`, and applies configurable inconsistency policies.
//   2. Multi-Dimension Weighted Rubric Scorer (`RubricScorer`):
//      Fixed-capacity weighted criteria (`weight_bps`) with mandatory `hard_gate_min_score`
//      guardrails and hardware `@fma` score accumulation.
//   3. Multi-Model Arena Elo Leaderboard (`ArenaEloTable`):
//      Standard Logistic expected win-rate Elo updates with configurable K-factor, weighted
//      verdict ingestion, and deterministic descending leaderboard ranking.

const std = @import("std");

// ============================================================================
// Constants & Internal Helpers
// ============================================================================

pub const EVAL_JUDGE_MAX_RUBRIC_DIMS: usize = 16;
pub const EVAL_JUDGE_MAX_MODELS: usize = 32;
pub const EVAL_JUDGE_DEFAULT_ELO: f64 = 1500.0;
pub const EVAL_JUDGE_DEFAULT_K: f64 = 32.0;

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

// ============================================================================
// 1. Position-Bias Mitigated Pairwise Judge
// ============================================================================

/// Raw judge choice for a single ordered presentation `(first_slot, second_slot)`.
pub enum RawJudgePick {
    pick_first,
    pick_second,
    pick_tie,
}

/// Consolidated outcome after evaluating both `(A vs B)` and `(B vs A)`.
pub enum PairwiseOutcome {
    consistent_a_win,
    consistent_b_win,
    consistent_tie,
    position_bias_inconsistent,
}

/// Policy for handling position-bias inconsistent pairs when feeding downstream Elo updates.
pub enum InconsistentPolicy {
    treat_as_tie,
    downweight_tie,
    discard_sample,
}

/// Resolved verdict from a swapped `(A vs B)` + `(B vs A)` judge pair.
pub struct PairwiseVerdict {
    outcome: PairwiseOutcome = PairwiseOutcome.consistent_tie;
    score_a: f64 = 0.5;
    score_b: f64 = 0.5;
    effective_weight: f64 = 1.0;
    first_slot_bias: bool = false;
    second_slot_bias: bool = false;
}

/// Aggregate pairwise evaluation statistics including position-bias rate in basis points.
pub struct PairwiseSummary {
    total_pairs: u64 = 0;
    consistent_a_wins: u64 = 0;
    consistent_b_wins: u64 = 0;
    consistent_ties: u64 = 0;
    inconsistent_count: u64 = 0;
    first_slot_bias_count: u64 = 0;
    second_slot_bias_count: u64 = 0;
    position_bias_bps: u32 = 0;
}

/// Resolves forward `(A vs B)` and reverse `(B vs A)` judge picks into a position-bias
/// mitigated `PairwiseVerdict`.
pub fn resolvePairwiseSwap(
    forward_ab: RawJudgePick,
    reverse_ba: RawJudgePick,
    policy: InconsistentPolicy,
): PairwiseVerdict {
    if (forward_ab == RawJudgePick.pick_first && reverse_ba == RawJudgePick.pick_second) {
        return PairwiseVerdict {
            outcome: PairwiseOutcome.consistent_a_win,
            score_a: 1.0,
            score_b: 0.0,
            effective_weight: 1.0,
            first_slot_bias: false,
            second_slot_bias: false,
        };
    }
    if (forward_ab == RawJudgePick.pick_second && reverse_ba == RawJudgePick.pick_first) {
        return PairwiseVerdict {
            outcome: PairwiseOutcome.consistent_b_win,
            score_a: 0.0,
            score_b: 1.0,
            effective_weight: 1.0,
            first_slot_bias: false,
            second_slot_bias: false,
        };
    }
    if (forward_ab == RawJudgePick.pick_tie && reverse_ba == RawJudgePick.pick_tie) {
        return PairwiseVerdict {
            outcome: PairwiseOutcome.consistent_tie,
            score_a: 0.5,
            score_b: 0.5,
            effective_weight: 1.0,
            first_slot_bias: false,
            second_slot_bias: false,
        };
    }

    const first_bias: bool = forward_ab == RawJudgePick.pick_first && reverse_ba == RawJudgePick.pick_first;
    const second_bias: bool = forward_ab == RawJudgePick.pick_second && reverse_ba == RawJudgePick.pick_second;
    const weight: f64 = if (policy == InconsistentPolicy.treat_as_tie) {
        1.0
    } else if (policy == InconsistentPolicy.downweight_tie) {
        0.5
    } else {
        0.0
    };

    return PairwiseVerdict {
        outcome: PairwiseOutcome.position_bias_inconsistent,
        score_a: 0.5,
        score_b: 0.5,
        effective_weight: weight,
        first_slot_bias: first_bias,
        second_slot_bias: second_bias,
    };
}

/// Stateful aggregator tracking pairwise swap consistency and position-bias rate (`position_bias_bps`).
pub struct PairwiseJudgeAggregator {
    policy: InconsistentPolicy = InconsistentPolicy.downweight_tie;
    total_pairs: u64 = 0;
    consistent_a_wins: u64 = 0;
    consistent_b_wins: u64 = 0;
    consistent_ties: u64 = 0;
    inconsistent_count: u64 = 0;
    first_slot_bias_count: u64 = 0;
    second_slot_bias_count: u64 = 0;

    static fn init(policy: InconsistentPolicy): PairwiseJudgeAggregator {
        return PairwiseJudgeAggregator {
            policy: policy,
        };
    }

    /// Records a forward `(A vs B)` and reverse `(B vs A)` judge pair and returns its resolved verdict.
    fn recordSwap(*_, forward_ab: RawJudgePick, reverse_ba: RawJudgePick): PairwiseVerdict {
        const verdict = resolvePairwiseSwap(forward_ab, reverse_ba, _.policy);
        _.total_pairs += 1;
        if (verdict.outcome == PairwiseOutcome.consistent_a_win) {
            _.consistent_a_wins += 1;
        } else if (verdict.outcome == PairwiseOutcome.consistent_b_win) {
            _.consistent_b_wins += 1;
        } else if (verdict.outcome == PairwiseOutcome.consistent_tie) {
            _.consistent_ties += 1;
        } else {
            _.inconsistent_count += 1;
            if (verdict.first_slot_bias) {
                _.first_slot_bias_count += 1;
            }
            if (verdict.second_slot_bias) {
                _.second_slot_bias_count += 1;
            }
        }
        return verdict;
    }

    /// Returns aggregate summary including `position_bias_bps` (`0..=10000`).
    fn summary(): PairwiseSummary {
        const bias_bps: u32 = if (_.total_pairs == 0) {
            0
        } else {
            ((_.inconsistent_count * 10000) / _.total_pairs) as u32
        };
        return PairwiseSummary {
            total_pairs: _.total_pairs,
            consistent_a_wins: _.consistent_a_wins,
            consistent_b_wins: _.consistent_b_wins,
            consistent_ties: _.consistent_ties,
            inconsistent_count: _.inconsistent_count,
            first_slot_bias_count: _.first_slot_bias_count,
            second_slot_bias_count: _.second_slot_bias_count,
            position_bias_bps: bias_bps,
        };
    }

    /// Resets all counters while keeping the configured inconsistency policy.
    fn reset(*_) {
        _.total_pairs = 0;
        _.consistent_a_wins = 0;
        _.consistent_b_wins = 0;
        _.consistent_ties = 0;
        _.inconsistent_count = 0;
        _.first_slot_bias_count = 0;
        _.second_slot_bias_count = 0;
    }
}

// ============================================================================
// 2. Multi-Dimension Weighted Rubric Scorer with Hard Gates
// ============================================================================

/// Specification for a single weighted rubric dimension.
pub struct RubricDimSpec {
    dim_id: u32 = 0;
    weight_bps: u32 = 0;
    min_score: f64 = 0.0;
    max_score: f64 = 1.0;
    has_hard_gate: bool = false;
    hard_gate_min_score: f64 = 0.0;

    /// Validates non-zero weight, finite strictly increasing score bounds, and hard-gate range.
    fn isValid(): bool {
        if (_.weight_bps == 0) {
            return false;
        }
        if (!isFiniteF64(_.min_score) || !isFiniteF64(_.max_score) || _.max_score <= _.min_score) {
            return false;
        }
        if (_.has_hard_gate) {
            if (!isFiniteF64(_.hard_gate_min_score)) {
                return false;
            }
            if (_.hard_gate_min_score < _.min_score || _.hard_gate_min_score > _.max_score) {
                return false;
            }
        }
        return true;
    }
}

/// Overall decision produced by `RubricScorer.evaluate`.
pub enum RubricVerdictKind {
    pass,
    fail_threshold,
    fail_hard_gate,
    invalid_input,
}

/// Detailed result of evaluating a candidate response against a multi-dimension rubric.
pub struct RubricEvalResult {
    verdict: RubricVerdictKind = RubricVerdictKind.invalid_input;
    weighted_score: f64 = 0.0;
    total_weight_bps: u64 = 0;
    failed_hard_gate_count: usize = 0;
    first_failed_dim_id: u32 = 0;
    passed_all_hard_gates: bool = false;
}

/// Fixed-capacity weighted multi-dimension rubric scorer (`EVAL_JUDGE_MAX_RUBRIC_DIMS = 16`).
pub struct RubricScorer {
    dims: [EVAL_JUDGE_MAX_RUBRIC_DIMS]RubricDimSpec = [RubricDimSpec {}; EVAL_JUDGE_MAX_RUBRIC_DIMS];
    dim_count: usize = 0;
    total_weight_bps: u64 = 0;
    pass_threshold: f64 = 0.7;

    /// Creates a rubric scorer with normalized `pass_threshold ∈ [0.0, 1.0]`.
    static fn init(pass_threshold: f64): RubricScorer? {
        if (!isFiniteF64(pass_threshold) || pass_threshold < 0.0 || pass_threshold > 1.0) {
            return null;
        }
        return RubricScorer {
            pass_threshold: pass_threshold,
        };
    }

    /// Registers a validated rubric dimension. Rejects invalid specs, duplicate `dim_id`, or full table.
    fn addDimension(*_, spec: RubricDimSpec): bool {
        if (!spec.isValid() || _.dim_count >= EVAL_JUDGE_MAX_RUBRIC_DIMS) {
            return false;
        }
        let i: usize = 0;
        while (i < _.dim_count) : (i += 1) {
            if (_.dims[i].dim_id == spec.dim_id) {
                return false;
            }
        }
        _.dims[_.dim_count] = spec;
        _.dim_count += 1;
        _.total_weight_bps += spec.weight_bps as u64;
        return true;
    }

    fn dimCount(): usize {
        return _.dim_count;
    }

    fn totalWeightBps(): u64 {
        return _.total_weight_bps;
    }

    /// Evaluates per-dimension raw scores (matching registration order) against weights and hard gates.
    /// Any NaN/Inf/out-of-bounds score or length mismatch returns `RubricVerdictKind.invalid_input`.
    fn evaluate(scores: []const f64): RubricEvalResult {
        if (_.dim_count == 0 || _.total_weight_bps == 0 || scores.len != _.dim_count) {
            return RubricEvalResult {};
        }

        let weighted_sum: f64 = 0.0;
        let failed_gates: usize = 0;
        let first_failed_id: u32 = 0;

        let i: usize = 0;
        while (i < _.dim_count) : (i += 1) {
            const spec = _.dims[i];
            const s: f64 = scores[i];
            if (!isFiniteF64(s) || s < spec.min_score || s > spec.max_score) {
                return RubricEvalResult {};
            }

            const norm: f64 = (s - spec.min_score) / (spec.max_score - spec.min_score);
            weighted_sum = @fma(norm, spec.weight_bps as f64, weighted_sum);

            if (spec.has_hard_gate && s < spec.hard_gate_min_score) {
                if (failed_gates == 0) {
                    first_failed_id = spec.dim_id;
                }
                failed_gates += 1;
            }
        }

        const final_score: f64 = weighted_sum / (_.total_weight_bps as f64);
        const gates_ok: bool = failed_gates == 0;
        const verdict: RubricVerdictKind = if (!gates_ok) {
            RubricVerdictKind.fail_hard_gate
        } else if (final_score >= _.pass_threshold) {
            RubricVerdictKind.pass
        } else {
            RubricVerdictKind.fail_threshold
        };

        return RubricEvalResult {
            verdict: verdict,
            weighted_score: final_score,
            total_weight_bps: _.total_weight_bps,
            failed_hard_gate_count: failed_gates,
            first_failed_dim_id: first_failed_id,
            passed_all_hard_gates: gates_ok,
        };
    }
}

// ============================================================================
// 3. Multi-Model Arena Elo Leaderboard
// ============================================================================

/// Rating and match record for a single candidate Agent/model in `ArenaEloTable`.
pub struct EloModelEntry {
    model_id: u32 = 0;
    elo: f64 = EVAL_JUDGE_DEFAULT_ELO;
    wins: u64 = 0;
    losses: u64 = 0;
    ties: u64 = 0;
    matches: u64 = 0;
    occupied: bool = false;
}

/// Delta report returned after recording a pairwise match in `ArenaEloTable`.
pub struct EloMatchUpdate {
    applied: bool = false;
    expected_a: f64 = 0.5;
    delta_a: f64 = 0.0;
    delta_b: f64 = 0.0;
    new_elo_a: f64 = 0.0;
    new_elo_b: f64 = 0.0;
}

/// Ranked leaderboard row produced by `ArenaEloTable.leaderboard`.
pub struct EloRankEntry {
    rank: usize = 0;
    model_id: u32 = 0;
    elo: f64 = 0.0;
    wins: u64 = 0;
    losses: u64 = 0;
    ties: u64 = 0;
    matches: u64 = 0;
}

fn isHigherRanked(a: EloModelEntry, b: EloModelEntry): bool {
    if (a.elo > b.elo) {
        return true;
    }
    if (a.elo < b.elo) {
        return false;
    }
    if (a.wins > b.wins) {
        return true;
    }
    if (a.wins < b.wins) {
        return false;
    }
    return a.model_id < b.model_id;
}

/// Zero-heap-allocation multi-model Elo rating table (`EVAL_JUDGE_MAX_MODELS = 32`).
pub struct ArenaEloTable {
    models: [EVAL_JUDGE_MAX_MODELS]EloModelEntry = [EloModelEntry {}; EVAL_JUDGE_MAX_MODELS];
    model_count: usize = 0;
    k_factor: f64 = EVAL_JUDGE_DEFAULT_K;
    initial_elo: f64 = EVAL_JUDGE_DEFAULT_ELO;

    /// Initializes an Arena Elo table with validated `k_factor ∈ (0.0, 256.0]` and `initial_elo >= 0.0`.
    static fn init(k_factor: f64, initial_elo: f64): ArenaEloTable? {
        if (!isFiniteF64(k_factor) || k_factor <= 0.0 || k_factor > 256.0) {
            return null;
        }
        if (!isFiniteF64(initial_elo) || initial_elo < 0.0) {
            return null;
        }
        return ArenaEloTable {
            k_factor: k_factor,
            initial_elo: initial_elo,
        };
    }

    /// Returns default Arena Elo table (`k_factor = 32.0, initial_elo = 1500.0`).
    fn defaults(): ArenaEloTable {
        return ArenaEloTable {};
    }

    fn findSlot(model_id: u32): usize? {
        let i: usize = 0;
        while (i < _.model_count) : (i += 1) {
            if (_.models[i].occupied && _.models[i].model_id == model_id) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new candidate model ID at `initial_elo`.
    /// Returns `false` if already registered or table is full (`EVAL_JUDGE_MAX_MODELS`).
    fn registerModel(*_, model_id: u32): bool {
        if (_.model_count >= EVAL_JUDGE_MAX_MODELS) {
            return false;
        }
        if (_.findSlot(model_id) != null) {
            return false;
        }
        _.models[_.model_count] = EloModelEntry {
            model_id: model_id,
            elo: _.initial_elo,
            occupied: true,
        };
        _.model_count += 1;
        return true;
    }

    fn modelCount(): usize {
        return _.model_count;
    }

    fn getModel(model_id: u32): EloModelEntry? {
        if (idx := _.findSlot(model_id)) {
            return _.models[idx];
        }
        return null;
    }

    /// Computes standard Logistic expected win probability for rating `elo_a` vs `elo_b`.
    static fn expectedScore(elo_a: f64, elo_b: f64): f64 {
        const exp_val: f64 = (elo_b - elo_a) / 400.0;
        return 1.0 / (1.0 + std.math.pow(f64, 10.0, exp_val));
    }

    /// Applies a single weighted match update between `model_a` and `model_b`.
    /// Rejects unregistered models, self-matches, non-finite/out-of-range `score_a ∉ [0, 1]`,
    /// or negative/non-finite `weight`. A zero `weight` safely returns `applied = false`.
    fn recordMatch(*_, model_a: u32, model_b: u32, score_a: f64, weight: f64): EloMatchUpdate {
        if (model_a == model_b) {
            return EloMatchUpdate {};
        }
        if (!isFiniteF64(score_a) || score_a < 0.0 || score_a > 1.0) {
            return EloMatchUpdate {};
        }
        if (!isFiniteF64(weight) || weight <= 0.0) {
            return EloMatchUpdate {};
        }

        const idx_a = _.findSlot(model_a) ?? return EloMatchUpdate {};
        const idx_b = _.findSlot(model_b) ?? return EloMatchUpdate {};

        const ra: f64 = _.models[idx_a].elo;
        const rb: f64 = _.models[idx_b].elo;
        const ea: f64 = ArenaEloTable.expectedScore(ra, rb);
        const eff_k: f64 = _.k_factor * weight;
        const da: f64 = eff_k * (score_a - ea);
        const db: f64 = -da;

        _.models[idx_a].elo = ra + da;
        _.models[idx_b].elo = rb + db;
        _.models[idx_a].matches += 1;
        _.models[idx_b].matches += 1;

        if (score_a > 0.5) {
            _.models[idx_a].wins += 1;
            _.models[idx_b].losses += 1;
        } else if (score_a < 0.5) {
            _.models[idx_a].losses += 1;
            _.models[idx_b].wins += 1;
        } else {
            _.models[idx_a].ties += 1;
            _.models[idx_b].ties += 1;
        }

        return EloMatchUpdate {
            applied: true,
            expected_a: ea,
            delta_a: da,
            delta_b: db,
            new_elo_a: _.models[idx_a].elo,
            new_elo_b: _.models[idx_b].elo,
        };
    }

    /// Convenience helper feeding a position-bias mitigated `PairwiseVerdict` directly into Elo.
    fn recordPairwiseVerdict(*_, model_a: u32, model_b: u32, verdict: PairwiseVerdict): EloMatchUpdate {
        return _.recordMatch(model_a, model_b, verdict.score_a, verdict.effective_weight);
    }

    /// Outputs deterministic descending Elo leaderboard into `out_ranks` (zero heap allocation)
    /// and returns the number of rank entries written.
    fn leaderboard(out_ranks: *[]EloRankEntry): usize {
        const n: usize = _.model_count;
        if (n == 0 || out_ranks.len == 0) {
            return 0;
        }

        let used: [EVAL_JUDGE_MAX_MODELS]bool = [false; EVAL_JUDGE_MAX_MODELS];
        const limit: usize = if (n < out_ranks.len) n else out_ranks.len;

        let written: usize = 0;
        while (written < limit) : (written += 1) {
            let best: usize = n;
            let i: usize = 0;
            while (i < n) : (i += 1) {
                if (used[i]) {
                    continue;
                }
                if (best == n || isHigherRanked(_.models[i], _.models[best])) {
                    best = i;
                }
            }
            if (best == n) {
                break;
            }
            used[best] = true;
            const m = _.models[best];
            out_ranks[written] = EloRankEntry {
                rank: written + 1,
                model_id: m.model_id,
                elo: m.elo,
                wins: m.wins,
                losses: m.losses,
                ties: m.ties,
                matches: m.matches,
            };
        }
        return written;
    }
}
