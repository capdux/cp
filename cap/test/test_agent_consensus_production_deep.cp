// Origin: Pure-CP Stdlib Test Suite (lib/agent/consensus.cp) | Created: 2026-11-22
// Production Deep Verification Suite for zero-allocation multi-agent Self-Consistency,
// confidence-weighted consensus, Shannon disagreement entropy, quorum & tie arbitration,
// weighted/equal-weight Borda count ranking, and defensive parameter/NaN/buffer guards.

import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    AgentVote,
    ConsensusStatus,
    ConsensusResult,
    resolveConsensus,
    BordaResult,
    resolveBordaRanking,
} from "cap:agent";

fn approxEqual(a: f64, b: f64, eps: f64): bool {
    const d = a - b;
    return d >= -eps && d <= eps;
}

fn testFiveWayMajorityAndUnanimousZeroEntropy()! {
    let h_arr: [8]u64 = undefined;
    let w_arr: [8]f64 = undefined;
    let idx_arr: [8]usize = undefined;
    let h_s: []u64 = h_arr[0..];
    let w_s: []f64 = w_arr[0..];
    let idx_s: []usize = idx_arr[0..];

    // 1. 5-way 3:2 majority vote (equal weights, no confidence scaling)
    const votes_3_2: [5]AgentVote = [
        AgentVote{ answer_hash: 101, answer_index: 0, confidence: 0.9, agent_weight: 1.0 },
        AgentVote{ answer_hash: 202, answer_index: 1, confidence: 0.8, agent_weight: 1.0 },
        AgentVote{ answer_hash: 101, answer_index: 2, confidence: 0.7, agent_weight: 1.0 },
        AgentVote{ answer_hash: 202, answer_index: 3, confidence: 0.6, agent_weight: 1.0 },
        AgentVote{ answer_hash: 101, answer_index: 4, confidence: 0.95, agent_weight: 1.0 },
    ];

    if (res := resolveConsensus(votes_3_2[0..], 0.5, false, h_s, w_s, idx_s)) {
        expect_eq(res.status == ConsensusStatus.Reached, true);
        expect_eq(res.winner_hash, 101);
        expect_eq(res.winner_index, 0);
        expect_eq(approxEqual(res.winner_weight, 3.0, 1.0e-12), true);
        expect_eq(approxEqual(res.total_weight, 5.0, 1.0e-12), true);
        expect_eq(approxEqual(res.agreement_ratio, 0.6, 1.0e-12), true);
        expect_eq(res.unique_answers, 2);
        // Binary entropy H(0.6, 0.4) = -(0.6*log2(0.6) + 0.4*log2(0.4)) ~= 0.9709505944546686
        expect_eq(approxEqual(res.disagreement_entropy, 0.9709505944546686, 1.0e-9), true);
    } else {
        expect_eq(true, false);
    }

    // 2. Unanimous 5:0 consensus -> disagreement_entropy == 0.0 exactly
    const votes_unanimous: [5]AgentVote = [
        AgentVote{ answer_hash: 777, answer_index: 10, confidence: 0.9, agent_weight: 1.0 },
        AgentVote{ answer_hash: 777, answer_index: 11, confidence: 0.8, agent_weight: 1.0 },
        AgentVote{ answer_hash: 777, answer_index: 12, confidence: 0.7, agent_weight: 1.0 },
        AgentVote{ answer_hash: 777, answer_index: 13, confidence: 0.95, agent_weight: 1.0 },
        AgentVote{ answer_hash: 777, answer_index: 14, confidence: 0.85, agent_weight: 1.0 },
    ];
    if (res2 := resolveConsensus(votes_unanimous[0..], 0.8, true, h_s, w_s, idx_s)) {
        expect_eq(res2.status == ConsensusStatus.Reached, true);
        expect_eq(res2.winner_hash, 777);
        expect_eq(res2.winner_index, 10);
        expect_eq(res2.unique_answers, 1);
        expect_eq(approxEqual(res2.agreement_ratio, 1.0, 1.0e-12), true);
        expect_eq(res2.disagreement_entropy, 0.0);
    } else {
        expect_eq(true, false);
    }
}

fn testHighConfidenceExpertOutweighsLowConfidenceMajority()! {
    let h_arr: [4]u64 = undefined;
    let w_arr: [4]f64 = undefined;
    let idx_arr: [4]usize = undefined;
    let h_s: []u64 = h_arr[0..];
    let w_s: []f64 = w_arr[0..];
    let idx_s: []usize = idx_arr[0..];

    // 3 low-confidence generalists vote for hash 10 (total: 3 * 1.0 * 0.2 = 0.6)
    // 1 high-confidence domain expert votes for hash 20 (total: 2.0 * 0.95 = 1.9)
    // With confidence weighting: expert wins with 1.9 / 2.5 = 0.76 >= 0.6!
    const votes: [4]AgentVote = [
        AgentVote{ answer_hash: 10, answer_index: 0, confidence: 0.2, agent_weight: 1.0 },
        AgentVote{ answer_hash: 10, answer_index: 1, confidence: 0.2, agent_weight: 1.0 },
        AgentVote{ answer_hash: 10, answer_index: 2, confidence: 0.2, agent_weight: 1.0 },
        AgentVote{ answer_hash: 20, answer_index: 3, confidence: 0.95, agent_weight: 2.0 },
    ];

    if (res := resolveConsensus(votes[0..], 0.6, true, h_s, w_s, idx_s)) {
        expect_eq(res.status == ConsensusStatus.Reached, true);
        expect_eq(res.winner_hash, 20);
        expect_eq(res.winner_index, 3);
        expect_eq(approxEqual(res.winner_weight, 1.9, 1.0e-12), true);
        expect_eq(approxEqual(res.total_weight, 2.5, 1.0e-12), true);
        expect_eq(approxEqual(res.agreement_ratio, 0.76, 1.0e-12), true);
    } else {
        expect_eq(true, false);
    }
}

fn testQuorumFailureAndTieDetection()! {
    let h_arr: [6]u64 = undefined;
    let w_arr: [6]f64 = undefined;
    let idx_arr: [6]usize = undefined;
    let h_s: []u64 = h_arr[0..];
    let w_s: []f64 = w_arr[0..];
    let idx_s: []usize = idx_arr[0..];

    // 1. 3:2 vote has agreement_ratio = 0.6 < quorum_ratio = 0.7 -> NoQuorum
    const votes_no_quorum: [5]AgentVote = [
        AgentVote{ answer_hash: 1, answer_index: 0, confidence: 1.0, agent_weight: 1.0 },
        AgentVote{ answer_hash: 1, answer_index: 1, confidence: 1.0, agent_weight: 1.0 },
        AgentVote{ answer_hash: 1, answer_index: 2, confidence: 1.0, agent_weight: 1.0 },
        AgentVote{ answer_hash: 2, answer_index: 3, confidence: 1.0, agent_weight: 1.0 },
        AgentVote{ answer_hash: 2, answer_index: 4, confidence: 1.0, agent_weight: 1.0 },
    ];
    if (res := resolveConsensus(votes_no_quorum[0..], 0.7, false, h_s, w_s, idx_s)) {
        expect_eq(res.status == ConsensusStatus.NoQuorum, true);
        expect_eq(res.winner_hash, 1);
        expect_eq(approxEqual(res.agreement_ratio, 0.6, 1.0e-12), true);
    } else {
        expect_eq(true, false);
    }

    // 2. Exact 2:2 tie -> TieUnresolved with entropy == 1.0 bit
    const votes_tie: [4]AgentVote = [
        AgentVote{ answer_hash: 11, answer_index: 0, confidence: 0.8, agent_weight: 1.0 },
        AgentVote{ answer_hash: 22, answer_index: 1, confidence: 0.8, agent_weight: 1.0 },
        AgentVote{ answer_hash: 11, answer_index: 2, confidence: 0.8, agent_weight: 1.0 },
        AgentVote{ answer_hash: 22, answer_index: 3, confidence: 0.8, agent_weight: 1.0 },
    ];
    if (res_t := resolveConsensus(votes_tie[0..], 0.4, true, h_s, w_s, idx_s)) {
        expect_eq(res_t.status == ConsensusStatus.TieUnresolved, true);
        expect_eq(approxEqual(res_t.agreement_ratio, 0.5, 1.0e-12), true);
        expect_eq(approxEqual(res_t.disagreement_entropy, 1.0, 1.0e-12), true);
    } else {
        expect_eq(true, false);
    }
}

fn testBordaRankingEqualAndWeighted()! {
    // 3 voters, 3 candidates (0, 1, 2):
    // Voter 0: [0, 1, 2] -> pts: c0=2, c1=1, c2=0
    // Voter 1: [1, 0, 2] -> pts: c1=2, c0=1, c2=0
    // Voter 2: [1, 2, 0] -> pts: c1=2, c2=1, c0=0
    const rankings: [9]usize = [
        0, 1, 2,
        1, 0, 2,
        1, 2, 0,
    ];
    const empty_weights: []const f64 = [];
    let pts_arr: [3]f64 = undefined;
    let pts: []f64 = pts_arr[0..];

    // 1. Equal weight: c0=3, c1=5, c2=1 -> winner = 1
    if (b1 := resolveBordaRanking(rankings[0..], 3, 3, empty_weights, pts)) {
        expect_eq(b1.winner_candidate, 1);
        expect_eq(approxEqual(b1.winner_points, 5.0, 1.0e-12), true);
        expect_eq(b1.is_tie, false);
        expect_eq(approxEqual(pts[0], 3.0, 1.0e-12), true);
        expect_eq(approxEqual(pts[1], 5.0, 1.0e-12), true);
        expect_eq(approxEqual(pts[2], 1.0, 1.0e-12), true);
    } else {
        expect_eq(true, false);
    }

    // 2. Weighted: Voter 0 has weight 5.0, Voters 1 & 2 have weight 1.0
    // c0 = 2*5 + 1*1 + 0*1 = 11.0, c1 = 1*5 + 2*1 + 2*1 = 9.0, c2 = 1.0 -> winner = 0
    const weights: [3]f64 = [5.0, 1.0, 1.0];
    if (b2 := resolveBordaRanking(rankings[0..], 3, 3, weights[0..], pts)) {
        expect_eq(b2.winner_candidate, 0);
        expect_eq(approxEqual(b2.winner_points, 11.0, 1.0e-12), true);
        expect_eq(b2.is_tie, false);
        expect_eq(approxEqual(pts[1], 9.0, 1.0e-12), true);
    } else {
        expect_eq(true, false);
    }

    // 3. Exact tie in Borda ranking
    const tie_rankings: [4]usize = [0, 1, 1, 0];
    let pts2_arr: [2]f64 = undefined;
    let pts2: []f64 = pts2_arr[0..];
    if (b3 := resolveBordaRanking(tie_rankings[0..], 2, 2, empty_weights, pts2)) {
        expect_eq(b3.is_tie, true);
        expect_eq(approxEqual(b3.winner_points, 1.0, 1.0e-12), true);
    } else {
        expect_eq(true, false);
    }
}

fn testDefensiveGuardsEmptyNanInvalidAndNamespace()! {
    let zero_f: f64 = 0.0;
    const nan_f: f64 = zero_f / zero_f;

    let h_arr: [4]u64 = undefined;
    let w_arr: [4]f64 = undefined;
    let idx_arr: [4]usize = undefined;
    let h_s: []u64 = h_arr[0..];
    let w_s: []f64 = w_arr[0..];
    let idx_s: []usize = idx_arr[0..];

    // 1. Empty votes -> ConsensusStatus.Empty
    const empty_votes: []const AgentVote = [];
    if (re := resolveConsensus(empty_votes, 0.5, true, h_s, w_s, idx_s)) {
        expect_eq(re.status == ConsensusStatus.Empty, true);
        expect_eq(re.unique_answers, 0);
    } else {
        expect_eq(true, false);
    }

    // 2. Invalid quorum_ratio (>1, <0, NaN) -> null
    expect_eq(resolveConsensus(empty_votes, 1.5, true, h_s, w_s, idx_s) == null, true);
    expect_eq(resolveConsensus(empty_votes, -0.1, true, h_s, w_s, idx_s) == null, true);
    expect_eq(resolveConsensus(empty_votes, nan_f, true, h_s, w_s, idx_s) == null, true);

    // 3. Negative confidence, NaN confidence, zero/negative agent_weight -> null
    const bad_conf: [1]AgentVote = [AgentVote{ answer_hash: 1, answer_index: 0, confidence: -0.2, agent_weight: 1.0 }];
    const nan_conf: [1]AgentVote = [AgentVote{ answer_hash: 1, answer_index: 0, confidence: nan_f, agent_weight: 1.0 }];
    const bad_w: [1]AgentVote = [AgentVote{ answer_hash: 1, answer_index: 0, confidence: 0.8, agent_weight: 0.0 }];
    expect_eq(resolveConsensus(bad_conf[0..], 0.5, true, h_s, w_s, idx_s) == null, true);
    expect_eq(resolveConsensus(nan_conf[0..], 0.5, true, h_s, w_s, idx_s) == null, true);
    expect_eq(resolveConsensus(bad_w[0..], 0.5, true, h_s, w_s, idx_s) == null, true);

    // 4. Insufficient scratch buffer -> null
    const two_distinct: [2]AgentVote = [
        AgentVote{ answer_hash: 1, answer_index: 0, confidence: 0.9, agent_weight: 1.0 },
        AgentVote{ answer_hash: 2, answer_index: 1, confidence: 0.8, agent_weight: 1.0 },
    ];
    let tiny_h_arr: [1]u64 = undefined;
    let tiny_h: []u64 = tiny_h_arr[0..];
    expect_eq(resolveConsensus(two_distinct[0..], 0.5, true, tiny_h, w_s, idx_s) == null, true);

    // 5. Borda duplicate candidate in ballot, out-of-range candidate, NaN weight -> null
    const dup_ballot: [3]usize = [0, 0, 1];
    const oor_ballot: [3]usize = [0, 1, 5];
    const ok_ballot: [3]usize = [0, 1, 2];
    const empty_w: []const f64 = [];
    const nan_w: [1]f64 = [nan_f];
    let pts_arr: [3]f64 = undefined;
    let pts: []f64 = pts_arr[0..];
    expect_eq(resolveBordaRanking(dup_ballot[0..], 1, 3, empty_w, pts) == null, true);
    expect_eq(resolveBordaRanking(oor_ballot[0..], 1, 3, empty_w, pts) == null, true);
    expect_eq(resolveBordaRanking(ok_ballot[0..], 1, 3, nan_w[0..], pts) == null, true);

    // 6. Namespace access via `cap.agent.*`
    if (rn := cap.agent.resolveConsensus(two_distinct[0..], 0.5, true, h_s, w_s, idx_s)) {
        expect_eq(rn.status == cap.agent.ConsensusStatus.Reached, true);
        expect_eq(rn.winner_hash, 1);
    } else {
        expect_eq(true, false);
    }
}

pub fn main()! {
    test("1. agent/consensus: 5-way 3:2 majority Reached and unanimous 0.0 entropy", () => {
        testFiveWayMajorityAndUnanimousZeroEntropy()?;
    });
    test("2. agent/consensus: high-confidence expert outweighs low-confidence majority", () => {
        testHighConfidenceExpertOutweighsLowConfidenceMajority()?;
    });
    test("3. agent/consensus: quorum_ratio NoQuorum and exact TieUnresolved", () => {
        testQuorumFailureAndTieDetection()?;
    });
    test("4. agent/consensus: resolveBordaRanking equal-weight, weighted & tie detection", () => {
        testBordaRankingEqualAndWeighted()?;
    });
    test("5. agent/consensus: defensive guards (empty/NaN/bounds/buffers) & cap.agent", () => {
        testDefensiveGuardsEmptyNanInvalidAndNamespace()?;
    });
}
