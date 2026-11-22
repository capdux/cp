// Origin: Pure-CP Stdlib (Zero-Alloc A2A Multi-Agent Weighted Quorum & Confidence Consensus) | Created: 2026-11-22
// Pure CP zero-heap-allocation multi-agent quorum voting, early-stop verdict engine, and robust numeric aggregator:
//   - Fixed inline 64-vote ballot pool (`QUORUM_MAX_VOTES = 64`) with per-agent duplicate guard (`reject | revote`)
//   - Supports 4 policies: `simple_majority` (>50%), `supermajority_two_thirds` (>=2/3),
//     `weighted_threshold_bps` (custom basis points), and `ranked_borda` (weighted Borda count)
//   - Real-time early-stop evaluation (`evaluateQuorum`: `pending | reached | unreachable_early_stop | tie`)
//     short-circuiting remaining LLM inference calls once outcome is mathematically locked or unreachable
//   - Robust numeric consensus (`aggregateNumericEstimates`: median, MAD dispersion & outlier-trimmed mean)

const std = @import("std");

// ============================================================================
// 1. Constants & Public Enums
// ============================================================================

/// Maximum ballots stored inline per proposal in `A2aQuorumSession`.
pub const QUORUM_MAX_VOTES: usize = 64;

/// Maximum distinct option IDs tallied inline during evaluation.
pub const QUORUM_MAX_OPTIONS: usize = 16;

/// Maximum numeric agent estimates aggregated inline by `aggregateNumericEstimates`.
pub const QUORUM_MAX_ESTIMATES: usize = 64;

/// Sentinel option identifier when no winner has been decided.
pub const QUORUM_NO_OPTION: u32 = 0xFFFFFFFF;

/// Quorum decision rule applied by `A2aQuorumSession`.
pub enum QuorumPolicy {
    simple_majority,
    supermajority_two_thirds,
    weighted_threshold_bps,
    ranked_borda,
}

/// Handling mode when the same `agent_id` submits another ballot.
pub enum QuorumDuplicatePolicy {
    reject_duplicate,
    allow_revote,
}

/// Real-time consensus outcome returned by `A2aQuorumSession.evaluateQuorum`.
pub enum QuorumVerdict {
    pending,
    reached,
    unreachable_early_stop,
    tie,
}

/// Immediate submission status returned by `A2aQuorumSession.castVote`.
pub enum QuorumVoteStatus {
    accepted,
    updated_revote,
    duplicate_rejected,
    invalid_vote,
    votes_full,
}

// ============================================================================
// 2. Public Data Models
// ============================================================================

/// Configuration for an `A2aQuorumSession`.
pub struct QuorumConfig {
    proposal_id: u64 = 0;
    policy: QuorumPolicy = QuorumPolicy.simple_majority;
    duplicate_policy: QuorumDuplicatePolicy = QuorumDuplicatePolicy.reject_duplicate;
    threshold_bps: u16 = 5001;
    use_confidence_weighting: bool = false;

    /// Returns `true` iff threshold and policy parameters are valid.
    fn isValid(): bool {
        if (_.policy == QuorumPolicy.weighted_threshold_bps) {
            if (_.threshold_bps == 0 || _.threshold_bps > 10000) {
                return false;
            }
        }
        return true;
    }
}

/// Single agent ballot submitted to `A2aQuorumSession.castVote`.
pub struct QuorumVote {
    agent_id: u64 = 0;
    option_id: u32 = 0;
    weight_bps: u32 = 10000;
    confidence_bps: u16 = 10000;
    round_num: u16 = 1;
    abstained: bool = false;
    borda_rank: u8 = 0;
    borda_num_options: u8 = 0;
}

/// Real-time quorum evaluation report returned by `A2aQuorumSession.evaluateQuorum`.
pub struct QuorumEvaluation {
    verdict: QuorumVerdict = QuorumVerdict.pending;
    winning_option_id: u32 = QUORUM_NO_OPTION;
    winning_score: u64 = 0;
    runner_up_score: u64 = 0;
    cast_weight: u64 = 0;
    abstained_weight: u64 = 0;
    remaining_weight: u64 = 0;
    total_votes: usize = 0;
    avg_confidence_bps: u16 = 0;
}

/// Single agent numeric estimate input for `aggregateNumericEstimates`.
pub struct NumericEstimate {
    agent_id: u64 = 0;
    value: f64 = 0.0;
    confidence_bps: u16 = 10000;
}

/// Robust aggregation summary returned by `aggregateNumericEstimates`.
pub struct NumericAggregateReport {
    valid: bool = false;
    total_count: usize = 0;
    kept_count: usize = 0;
    outlier_count: usize = 0;
    median: f64 = 0.0;
    trimmed_mean: f64 = 0.0;
    mad: f64 = 0.0;
    consensus_tight: bool = false;
}

// ============================================================================
// 3. Internal Tally & Threshold Helpers
// ============================================================================

struct OptionTally {
    option_id: u32 = 0;
    score: u64 = 0;
}

fn meetsWeightThreshold(policy: QuorumPolicy, threshold_bps: u16, score: u64, denom: u64): bool {
    if (denom == 0) return false;
    if (policy == QuorumPolicy.simple_majority) {
        return score * 2 > denom;
    }
    if (policy == QuorumPolicy.supermajority_two_thirds) {
        return score * 3 >= denom * 2;
    }
    if (policy == QuorumPolicy.weighted_threshold_bps) {
        return score * 10000 >= denom * (threshold_bps as u64);
    }
    return false;
}

fn isFiniteF64(x: f64): bool {
    return !std.math.isNan(x) && !std.math.isInf(x);
}

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn sortF64InPlace(arr: *[]f64) {
    let i: usize = 1;
    while (i < arr.len) : (i += 1) {
        const key: f64 = arr[i];
        let j: usize = i;
        while (j > 0 && arr[j - 1] > key) : (j -= 1) {
            arr[j] = arr[j - 1];
        }
        arr[j] = key;
    }
}

fn medianOfSorted(arr: []const f64): f64 {
    const n: usize = arr.len;
    if (n == 0) return 0.0;
    if (n % 2 == 1) {
        return arr[n / 2];
    }
    return (arr[(n / 2) - 1] + arr[n / 2]) * 0.5;
}

// ============================================================================
// 4. Zero-Alloc Multi-Agent Weighted Quorum Session
// ============================================================================

/// Zero-heap-allocation multi-agent ballot pool and early-stop quorum evaluator.
pub struct A2aQuorumSession {
    cfg: QuorumConfig = QuorumConfig{};
    votes: [QUORUM_MAX_VOTES]QuorumVote = [QuorumVote{}; QUORUM_MAX_VOTES];
    vote_count: usize = 0;

    /// Initializes a new quorum session if `cfg.isValid()`, else returns `null`.
    static fn init(cfg: QuorumConfig): A2aQuorumSession? {
        if (!cfg.isValid()) {
            return null;
        }
        return A2aQuorumSession{
            cfg: cfg,
            votes: [QuorumVote{}; QUORUM_MAX_VOTES],
            vote_count: 0,
        };
    }

    /// Resets all collected ballots in place for a new voting round.
    fn reset(*_) {
        _.vote_count = 0;
    }

    /// Returns the number of ballots currently stored.
    fn voteCount(): usize {
        return _.vote_count;
    }

    /// Submits a ballot from `vote.agent_id`, enforcing validity, duplicate policy, and capacity.
    fn castVote(*_, vote: QuorumVote): QuorumVoteStatus {
        if (vote.weight_bps == 0 || vote.confidence_bps > 10000) {
            return QuorumVoteStatus.invalid_vote;
        }
        if (_.cfg.policy == QuorumPolicy.ranked_borda && !vote.abstained) {
            if (vote.borda_num_options == 0 || vote.borda_rank >= vote.borda_num_options) {
                return QuorumVoteStatus.invalid_vote;
            }
        }

        // Check for existing vote from the same agent_id.
        let i: usize = 0;
        while (i < _.vote_count) : (i += 1) {
            if (_.votes[i].agent_id == vote.agent_id) {
                if (_.cfg.duplicate_policy == QuorumDuplicatePolicy.reject_duplicate) {
                    return QuorumVoteStatus.duplicate_rejected;
                }
                _.votes[i] = vote;
                return QuorumVoteStatus.updated_revote;
            }
        }

        if (_.vote_count >= QUORUM_MAX_VOTES) {
            return QuorumVoteStatus.votes_full;
        }

        _.votes[_.vote_count] = vote;
        _.vote_count += 1;
        return QuorumVoteStatus.accepted;
    }

    /// Evaluates real-time quorum status against `total_eligible_weight`, supporting mathematical
    /// early-stop (`reached` or `unreachable_early_stop`) before all agents finish voting.
    fn evaluateQuorum(total_eligible_weight: u64): QuorumEvaluation {
        let eval = QuorumEvaluation{
            verdict: QuorumVerdict.pending,
            winning_option_id: QUORUM_NO_OPTION,
            total_votes: _.vote_count,
        };

        let tallies: [QUORUM_MAX_OPTIONS]OptionTally = [OptionTally{}; QUORUM_MAX_OPTIONS];
        let option_count: usize = 0;
        let cast_weight: u64 = 0;
        let abstained_weight: u64 = 0;
        let conf_sum: u64 = 0;
        let non_abstain_cnt: u64 = 0;
        let max_borda_mult: u64 = 1;

        let i: usize = 0;
        while (i < _.vote_count) : (i += 1) {
            const v = _.votes[i];
            const raw_w: u64 = v.weight_bps as u64;
            if (v.abstained) {
                abstained_weight += raw_w;
                continue;
            }

            cast_weight += raw_w;
            conf_sum += v.confidence_bps as u64;
            non_abstain_cnt += 1;

            const eff_w: u64 = if (_.cfg.use_confidence_weighting)
                ((raw_w * (v.confidence_bps as u64)) / 10000)
            else
                raw_w;

            let pts: u64 = eff_w;
            if (_.cfg.policy == QuorumPolicy.ranked_borda) {
                const mult: u64 = (v.borda_num_options - v.borda_rank) as u64;
                if ((v.borda_num_options as u64) > max_borda_mult) {
                    max_borda_mult = v.borda_num_options as u64;
                }
                pts = eff_w * mult;
            }

            // Accumulate into option tally table.
            let found: usize? = null;
            let k: usize = 0;
            while (k < option_count) : (k += 1) {
                if (tallies[k].option_id == v.option_id) {
                    found = k;
                    break;
                }
            }
            if (idx := found) {
                tallies[idx].score += pts;
            } else if (option_count < QUORUM_MAX_OPTIONS) {
                tallies[option_count] = OptionTally{
                    option_id: v.option_id,
                    score: pts,
                };
                option_count += 1;
            }
        }

        eval.cast_weight = cast_weight;
        eval.abstained_weight = abstained_weight;
        const accounted: u64 = cast_weight + abstained_weight;
        const remaining: u64 = if (total_eligible_weight > accounted) (total_eligible_weight - accounted) else 0;
        eval.remaining_weight = remaining;
        if (non_abstain_cnt > 0) {
            eval.avg_confidence_bps = (conf_sum / non_abstain_cnt) as u16;
        }

        // Find leader & runner-up scores.
        let leader_score: u64 = 0;
        let runner_up_score: u64 = 0;
        let leader_opt: u32 = QUORUM_NO_OPTION;
        let leader_count: usize = 0;

        let t: usize = 0;
        while (t < option_count) : (t += 1) {
            const sc = tallies[t].score;
            if (sc > leader_score) {
                runner_up_score = leader_score;
                leader_score = sc;
                leader_opt = tallies[t].option_id;
                leader_count = 1;
            } else if (sc == leader_score && sc > 0) {
                runner_up_score = sc;
                leader_count += 1;
            } else if (sc > runner_up_score) {
                runner_up_score = sc;
            }
        }

        eval.winning_score = leader_score;
        eval.runner_up_score = runner_up_score;

        // Handle ranked Borda count policy.
        if (_.cfg.policy == QuorumPolicy.ranked_borda) {
            if (remaining == 0) {
                if (leader_count == 0) {
                    eval.verdict = QuorumVerdict.unreachable_early_stop;
                } else if (leader_count > 1) {
                    eval.verdict = QuorumVerdict.tie;
                } else {
                    eval.verdict = QuorumVerdict.reached;
                    eval.winning_option_id = leader_opt;
                }
                return eval;
            }
            if (leader_count == 1 && leader_score > runner_up_score + remaining * max_borda_mult) {
                eval.verdict = QuorumVerdict.reached;
                eval.winning_option_id = leader_opt;
                return eval;
            }
            eval.verdict = QuorumVerdict.pending;
            return eval;
        }

        // Threshold-based policies: simple_majority, supermajority_two_thirds, weighted_threshold_bps.
        const denom: u64 = if (total_eligible_weight > 0) total_eligible_weight else accounted;
        if (denom == 0) {
            eval.verdict = QuorumVerdict.pending;
            return eval;
        }

        // 1. Early-stop WIN: unique leader already crossed threshold.
        if (leader_count == 1 && meetsWeightThreshold(_.cfg.policy, _.cfg.threshold_bps, leader_score, denom)) {
            eval.verdict = QuorumVerdict.reached;
            eval.winning_option_id = leader_opt;
            return eval;
        }

        // 2. Early-stop UNREACHABLE: even if all remaining weight voted for the current leader, threshold cannot be met.
        if (!meetsWeightThreshold(_.cfg.policy, _.cfg.threshold_bps, leader_score + remaining, denom)) {
            eval.verdict = QuorumVerdict.unreachable_early_stop;
            return eval;
        }

        // 3. All ballots cast without crossing threshold.
        if (remaining == 0) {
            if (leader_count > 1) {
                eval.verdict = QuorumVerdict.tie;
            } else {
                eval.verdict = QuorumVerdict.unreachable_early_stop;
            }
            return eval;
        }

        eval.verdict = QuorumVerdict.pending;
        return eval;
    }
}

// ============================================================================
// 5. Robust Multi-Agent Numeric Estimate Aggregator (Trimmed Mean + MAD)
// ============================================================================

/// Aggregates multi-agent numeric estimates using median, MAD dispersion, symmetric trimming
/// (`trim_pct` in `0..=40`), and 3*MAD outlier rejection with zero dynamic heap allocation.
pub fn aggregateNumericEstimates(
    estimates: []const NumericEstimate,
    trim_pct: u8,
    max_allowed_mad: f64,
): NumericAggregateReport {
    let rep = NumericAggregateReport{};
    if (estimates.len == 0 || trim_pct > 40 || !isFiniteF64(max_allowed_mad) || max_allowed_mad < 0.0) {
        return rep;
    }

    let vals: [QUORUM_MAX_ESTIMATES]f64 = [0.0; QUORUM_MAX_ESTIMATES];
    let n: usize = 0;
    let i: usize = 0;
    while (i < estimates.len && n < QUORUM_MAX_ESTIMATES) : (i += 1) {
        if (estimates[i].confidence_bps > 10000 || !isFiniteF64(estimates[i].value)) {
            continue;
        }
        vals[n] = estimates[i].value;
        n += 1;
    }

    if (n == 0) {
        return rep;
    }

    let vals_s: []f64 = vals[0..n];
    sortF64InPlace(*vals_s);
    const med: f64 = medianOfSorted(vals[0..n]);

    // Compute MAD = median(|x_i - median|).
    let devs: [QUORUM_MAX_ESTIMATES]f64 = [0.0; QUORUM_MAX_ESTIMATES];
    let d_i: usize = 0;
    while (d_i < n) : (d_i += 1) {
        devs[d_i] = absF64(vals[d_i] - med);
    }
    let devs_s: []f64 = devs[0..n];
    sortF64InPlace(*devs_s);
    const mad: f64 = medianOfSorted(devs[0..n]);

    // Symmetric trim bounds + 3*MAD outlier filter.
    let trim_k: usize = (n * (trim_pct as usize)) / 100;
    if (trim_k * 2 >= n) {
        trim_k = 0;
    }
    const lo_idx: usize = trim_k;
    const hi_idx: usize = n - trim_k;

    let sum: f64 = 0.0;
    let kept: usize = 0;
    let j: usize = lo_idx;
    while (j < hi_idx) : (j += 1) {
        if (n >= 4 && mad > 0.0 && absF64(vals[j] - med) > 3.0 * mad) {
            continue;
        }
        sum += vals[j];
        kept += 1;
    }

    // Fallback to median if aggressive MAD filtering dropped every point.
    const tmean: f64 = if (kept > 0) (sum / (kept as f64)) else med;

    rep.valid = true;
    rep.total_count = n;
    rep.kept_count = if (kept > 0) kept else 1;
    rep.outlier_count = if (n >= rep.kept_count) (n - rep.kept_count) else 0;
    rep.median = med;
    rep.trimmed_mean = tmean;
    rep.mad = mad;
    rep.consensus_tight = (mad <= max_allowed_mad);
    return rep;
}
