// Origin: Pure-CP Stdlib Test Suite (lib/a2a/consensus_quorum.cp) | Created: 2026-11-22
// Production Deep Verification Suite for Zero-Alloc A2A Weighted Quorum & Confidence Consensus:
//   1. simple_majority & supermajority_two_thirds real-time early-stop (reached before all votes cast)
//   2. unreachable_early_stop when remaining uncast weight cannot mathematically meet threshold
//   3. weighted_threshold_bps + confidence weighting & exact tie verdict
//   4. ranked_borda weighted ranking & irreversible-lead early stop
//   5. Duplicate vote policies (reject_duplicate vs allow_revote) & reset() reuse
//   6. aggregateNumericEstimates (median, MAD, 3*MAD outlier rejection & trimmed_mean)
//   7. Boundary guards (zero weight, >10000 confidence, bad threshold, all abstained, 64-vote full)
//   8. cap.a2a namespace access for both quorum session & numeric aggregation

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    QUORUM_MAX_VOTES,
    QUORUM_NO_OPTION,
    QuorumPolicy,
    QuorumDuplicatePolicy,
    QuorumVerdict,
    QuorumVoteStatus,
    QuorumConfig,
    QuorumVote,
    NumericEstimate,
    A2aQuorumSession,
    aggregateNumericEstimates,
} from "cap:a2a";

// ============================================================================
// Test 1: simple_majority & supermajority_two_thirds Early-Stop Reached
// ============================================================================

fn testSimpleAndSupermajorityEarlyStop()! {
    // 1. Simple majority: 5 agents x 10000 = 50000 total eligible; 3 votes (30000 > 25000) -> early stop!
    let s1 = A2aQuorumSession.init(QuorumConfig{
        proposal_id: 1,
        policy: QuorumPolicy.simple_majority,
    }) ?? return error.InitS1;

    expect_eq(s1.castVote(QuorumVote{ agent_id: 1, option_id: 42, weight_bps: 10000, confidence_bps: 9000 }) == QuorumVoteStatus.accepted, true);
    expect_eq(s1.castVote(QuorumVote{ agent_id: 2, option_id: 42, weight_bps: 10000, confidence_bps: 8000 }) == QuorumVoteStatus.accepted, true);
    expect_eq(s1.evaluateQuorum(50000).verdict == QuorumVerdict.pending, true);

    expect_eq(s1.castVote(QuorumVote{ agent_id: 3, option_id: 42, weight_bps: 10000, confidence_bps: 10000 }) == QuorumVoteStatus.accepted, true);
    const ev1 = s1.evaluateQuorum(50000);
    expect_eq(ev1.verdict == QuorumVerdict.reached, true);
    expect_eq(ev1.winning_option_id, 42);
    expect_eq(ev1.remaining_weight, 20000); // 2 agents saved from unnecessary LLM inference!
    expect_eq(ev1.avg_confidence_bps, 9000);

    // 2. Supermajority 2/3: 3 agents x 10000 = 30000; 2 votes (20000 * 3 >= 30000 * 2) -> reached!
    let s2 = A2aQuorumSession.init(QuorumConfig{
        proposal_id: 2,
        policy: QuorumPolicy.supermajority_two_thirds,
    }) ?? return error.InitS2;
    * = s2.castVote(QuorumVote{ agent_id: 10, option_id: 9, weight_bps: 10000 });
    * = s2.castVote(QuorumVote{ agent_id: 11, option_id: 9, weight_bps: 10000 });
    const ev2 = s2.evaluateQuorum(30000);
    expect_eq(ev2.verdict == QuorumVerdict.reached, true);
    expect_eq(ev2.winning_option_id, 9);
}

// ============================================================================
// Test 2: Mathematically Unreachable Early-Stop (Saves Remaining Calls)
// ============================================================================

fn testUnreachableEarlyStop()! {
    // 2/3 supermajority out of 30000 requires >= 20000.
    // Agent 1 abstains (10000) and Agent 2 votes option 2 (5000): remaining = 15000.
    // Max possible score for any option = 5000 + 15000 = 20000 -> still reachable (pending).
    // Once Agent 3 votes option 3 (1000), remaining = 14000 -> max possible = 5000 + 14000 = 19000 < 20000 -> unreachable_early_stop!
    let s = A2aQuorumSession.init(QuorumConfig{
        proposal_id: 3,
        policy: QuorumPolicy.supermajority_two_thirds,
    }) ?? return error.InitS3;

    * = s.castVote(QuorumVote{ agent_id: 1, option_id: 0, weight_bps: 10000, abstained: true });
    * = s.castVote(QuorumVote{ agent_id: 2, option_id: 2, weight_bps: 5000 });
    expect_eq(s.evaluateQuorum(30000).verdict == QuorumVerdict.pending, true);

    * = s.castVote(QuorumVote{ agent_id: 3, option_id: 3, weight_bps: 1000 });
    const ev = s.evaluateQuorum(30000);
    expect_eq(ev.verdict == QuorumVerdict.unreachable_early_stop, true);
    expect_eq(ev.winning_option_id, QUORUM_NO_OPTION);
    expect_eq(ev.remaining_weight, 14000);
    expect_eq(ev.abstained_weight, 10000);
}

// ============================================================================
// Test 3: Custom Weighted Threshold + Confidence Weighting & Exact Tie
// ============================================================================

fn testWeightedThresholdConfidenceAndTie()! {
    // Custom 75% threshold with confidence weighting
    let s = A2aQuorumSession.init(QuorumConfig{
        proposal_id: 4,
        policy: QuorumPolicy.weighted_threshold_bps,
        threshold_bps: 7500,
        use_confidence_weighting: true,
    }) ?? return error.InitS4;

    // Raw weight 10000 * confidence 8000 / 10000 = 8000 effective score (>= 75% of 10000)
    * = s.castVote(QuorumVote{ agent_id: 1, option_id: 5, weight_bps: 10000, confidence_bps: 8000 });
    const ev = s.evaluateQuorum(10000);
    expect_eq(ev.verdict == QuorumVerdict.reached, true);
    expect_eq(ev.winning_score, 8000);

    // Exact 50-50 tie when all eligible weight is cast under 50% threshold
    let s_tie = A2aQuorumSession.init(QuorumConfig{
        proposal_id: 5,
        policy: QuorumPolicy.weighted_threshold_bps,
        threshold_bps: 5000,
    }) ?? return error.InitTie;
    * = s_tie.castVote(QuorumVote{ agent_id: 1, option_id: 1, weight_bps: 10000 });
    * = s_tie.castVote(QuorumVote{ agent_id: 2, option_id: 2, weight_bps: 10000 });
    const ev_tie = s_tie.evaluateQuorum(20000);
    expect_eq(ev_tie.verdict == QuorumVerdict.tie, true);
    expect_eq(ev_tie.winning_option_id, QUORUM_NO_OPTION);
}

// ============================================================================
// Test 4: Ranked Borda Count & Irreversible-Lead Early Stop
// ============================================================================

fn testRankedBordaAndEarlyStop()! {
    let s = A2aQuorumSession.init(QuorumConfig{
        proposal_id: 6,
        policy: QuorumPolicy.ranked_borda,
    }) ?? return error.InitBorda;

    // 3 options (rank 0 -> x3, rank 1 -> x2, rank 2 -> x1), total eligible weight = 30000
    // Agent 1 (w=15000): option 100 rank 0 -> 45000 pts
    // Agent 2 (w=10000): option 200 rank 2 -> 10000 pts
    // Remaining = 5000, max gain = 5000 * 3 = 15000 -> 10000 + 15000 = 25000 < 45000 => irreversible early stop!
    * = s.castVote(QuorumVote{ agent_id: 1, option_id: 100, weight_bps: 15000, borda_rank: 0, borda_num_options: 3 });
    * = s.castVote(QuorumVote{ agent_id: 2, option_id: 200, weight_bps: 10000, borda_rank: 2, borda_num_options: 3 });
    const ev = s.evaluateQuorum(30000);
    expect_eq(ev.verdict == QuorumVerdict.reached, true);
    expect_eq(ev.winning_option_id, 100);
    expect_eq(ev.winning_score, 45000);
    expect_eq(ev.runner_up_score, 10000);
}

// ============================================================================
// Test 5: Duplicate Vote Policies (reject_duplicate vs allow_revote) & reset()
// ============================================================================

fn testDuplicatePoliciesAndReset()! {
    let s_rej = A2aQuorumSession.init(QuorumConfig{
        policy: QuorumPolicy.simple_majority,
        duplicate_policy: QuorumDuplicatePolicy.reject_duplicate,
    }) ?? return error.InitRej;
    expect_eq(s_rej.castVote(QuorumVote{ agent_id: 1, option_id: 1, weight_bps: 10000 }) == QuorumVoteStatus.accepted, true);
    expect_eq(s_rej.castVote(QuorumVote{ agent_id: 1, option_id: 2, weight_bps: 10000 }) == QuorumVoteStatus.duplicate_rejected, true);
    expect_eq(s_rej.voteCount(), 1);

    let s_rev = A2aQuorumSession.init(QuorumConfig{
        policy: QuorumPolicy.simple_majority,
        duplicate_policy: QuorumDuplicatePolicy.allow_revote,
    }) ?? return error.InitRev;
    expect_eq(s_rev.castVote(QuorumVote{ agent_id: 1, option_id: 1, weight_bps: 10000 }) == QuorumVoteStatus.accepted, true);
    expect_eq(s_rev.castVote(QuorumVote{ agent_id: 1, option_id: 2, weight_bps: 10000 }) == QuorumVoteStatus.updated_revote, true);
    expect_eq(s_rev.voteCount(), 1);
    const ev = s_rev.evaluateQuorum(10000);
    expect_eq(ev.verdict == QuorumVerdict.reached, true);
    expect_eq(ev.winning_option_id, 2);

    s_rev.reset();
    expect_eq(s_rev.voteCount(), 0);
}

// ============================================================================
// Test 6: Robust Numeric Estimate Aggregation (Median, MAD & Outlier Trimming)
// ============================================================================

fn testNumericEstimateOutlierTrimming()! {
    // 6 agents cluster around 100.0, 1 rogue agent outputs 9999.0!
    let ests: [7]NumericEstimate = [
        NumericEstimate{ agent_id: 1, value: 98.0 },
        NumericEstimate{ agent_id: 2, value: 100.0 },
        NumericEstimate{ agent_id: 3, value: 101.0 },
        NumericEstimate{ agent_id: 4, value: 99.0 },
        NumericEstimate{ agent_id: 5, value: 102.0 },
        NumericEstimate{ agent_id: 6, value: 100.0 },
        NumericEstimate{ agent_id: 7, value: 9999.0 },
    ];
    const rep = aggregateNumericEstimates(ests[0..], 15, 2.0);
    expect_eq(rep.valid, true);
    expect_eq(rep.total_count, 7);
    expect_eq(rep.median, 100.0);
    expect_eq(rep.mad <= 1.5, true);
    expect_eq(rep.consensus_tight, true);
    // Extreme outlier 9999.0 must be trimmed so trimmed_mean stays right around 100.0!
    expect_eq(rep.trimmed_mean >= 99.0 && rep.trimmed_mean <= 101.5, true);
    expect_eq(rep.outlier_count >= 2, true);
}

// ============================================================================
// Test 7: Boundary Guards (Bad Threshold, Zero Weight, >10000 Conf, Full Pool)
// ============================================================================

fn testBoundaryGuards()! {
    // Invalid weighted_threshold_bps configs
    expect_eq(A2aQuorumSession.init(QuorumConfig{ policy: QuorumPolicy.weighted_threshold_bps, threshold_bps: 0 }) == null, true);
    expect_eq(A2aQuorumSession.init(QuorumConfig{ policy: QuorumPolicy.weighted_threshold_bps, threshold_bps: 10001 }) == null, true);

    let s = A2aQuorumSession.init(QuorumConfig{ policy: QuorumPolicy.simple_majority }) ?? return error.InitB;
    expect_eq(s.castVote(QuorumVote{ agent_id: 1, weight_bps: 0 }) == QuorumVoteStatus.invalid_vote, true);
    expect_eq(s.castVote(QuorumVote{ agent_id: 1, weight_bps: 100, confidence_bps: 10001 }) == QuorumVoteStatus.invalid_vote, true);

    // Fill 64 votes -> 65th rejected with votes_full
    let i: u64 = 1;
    while (i <= (QUORUM_MAX_VOTES as u64)) : (i += 1) {
        expect_eq(s.castVote(QuorumVote{ agent_id: i, option_id: 1, weight_bps: 100, abstained: true }) == QuorumVoteStatus.accepted, true);
    }
    expect_eq(s.castVote(QuorumVote{ agent_id: 999, option_id: 1, weight_bps: 100 }) == QuorumVoteStatus.votes_full, true);

    // All abstained with 0 remaining -> unreachable_early_stop
    expect_eq(s.evaluateQuorum(6400).verdict == QuorumVerdict.unreachable_early_stop, true);
}

// ============================================================================
// Test 8: cap.a2a Namespace Access & Invalid Numeric Inputs
// ============================================================================

fn testCapA2aNamespaceAccess()! {
    let s = cap.a2a.A2aQuorumSession.init(QuorumConfig{
        proposal_id: 88,
        policy: QuorumPolicy.simple_majority,
    }) ?? return error.InitNS;
    * = s.castVote(QuorumVote{ agent_id: 1, option_id: 3, weight_bps: 6000 });
    const ev = s.evaluateQuorum(10000);
    expect_eq(ev.verdict == QuorumVerdict.reached, true);
    expect_eq(ev.winning_option_id, 3);

    const zero_f: f64 = 0.0;
    let bad_ests: [2]NumericEstimate = [
        NumericEstimate{ agent_id: 1, value: zero_f / zero_f },
        NumericEstimate{ agent_id: 2, value: 42.0 },
    ];
    const nrep = cap.a2a.aggregateNumericEstimates(bad_ests[0..], 0, 1.0);
    expect_eq(nrep.valid, true);
    expect_eq(nrep.total_count, 1); // NaN filtered safely
    expect_eq(nrep.median, 42.0);
}

// ============================================================================
// Test Entrypoints
// ============================================================================

test("consensus_quorum: simple_majority & supermajority_two_thirds early-stop", testSimpleAndSupermajorityEarlyStop);
test("consensus_quorum: mathematically unreachable_early_stop", testUnreachableEarlyStop);
test("consensus_quorum: weighted_threshold_bps + confidence weighting & tie", testWeightedThresholdConfidenceAndTie);
test("consensus_quorum: ranked_borda & irreversible-lead early-stop", testRankedBordaAndEarlyStop);
test("consensus_quorum: duplicate policies (reject vs revote) & reset()", testDuplicatePoliciesAndReset);
test("consensus_quorum: aggregateNumericEstimates (median, MAD & outlier trimming)", testNumericEstimateOutlierTrimming);
test("consensus_quorum: boundary guards (threshold, weight, conf, full, all-abstain)", testBoundaryGuards);
test("consensus_quorum: cap.a2a namespace & NaN estimate filtering", testCapA2aNamespaceAccess);
