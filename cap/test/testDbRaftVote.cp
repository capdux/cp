// Origin: Pure-CP Stdlib Test Suite (lib/db/raftVote.cp) | Created: 2026-11-23
// Production Deep Verification Suite for Sans-IO Raft Election, Pre-Vote & Quorum Commit FSM:
//   1. 3-node & 5-node full Pre-Vote -> Formal Election -> Leader transition & duplicate vote dedup
//   2. Pre-Vote partition isolation: isolated node cannot inflate its own term or healthy cluster term
//   3. Raft Section 5.4.1 log up-to-date check rejecting stale-term and shorter-log candidates
//   4. Leader Lease anti-disruption guard rejecting spurious RequestVote while leader heartbeat is fresh
//   5. Higher-term automatic step-down (`handleHeartbeat` / `handleVoteResponse` / `stepDown`)
//   6. Leader `computeQuorumCommitIndex` / `advanceCommitIndex` monotonic quorum commit & `cap.db` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    RAFT_MAX_PEERS,
    RAFT_NO_NODE,
    RaftRole,
    RaftVoteRejectReason,
    RaftVoteRequest,
    RaftHeartbeat,
    isLogUpToDate,
    formatRaftRole,
    RaftElectionNode,
} from "cap:db";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("RaftVote: 3-node & 5-node Pre-Vote -> Formal Candidate -> Leader election") {
    expect_true(RaftElectionNode.init(RAFT_NO_NODE, 3, 300, 100, true) == null);
    expect_true(RaftElectionNode.init(1, 0, 300, 100, true) == null);
    expect_true(RaftElectionNode.init(1, RAFT_MAX_PEERS + 1, 300, 100, true) == null);

    // 1. 3-node cluster (quorum = 2)
    let n1 = RaftElectionNode.init(1, 3, 300, 100, true) ?? return;
    let n2 = RaftElectionNode.init(2, 3, 300, 100, true) ?? return;
    expect_eq(n1.quorumSize(), @as(usize, 2));
    expect_true(n1.tick(100, 1, 5) == null); // not timed out yet

    // Timeout at t=300 triggers Pre-Vote (current_term stays 0!)
    const pre_req = n1.tick(300, 1, 5) ?? return;
    expect_true(pre_req.is_pre_vote);
    expect_eq(pre_req.term, @as(u64, 1));
    expect_eq(n1.stats().role, RaftRole.pre_candidate);
    expect_eq(n1.stats().current_term, @as(u64, 0));

    const pre_resp = n2.handleVoteRequest(pre_req, 300, 1, 5);
    expect_true(pre_resp.vote_granted);
    expect_eq(n2.stats().voted_for, RAFT_NO_NODE); // Pre-Vote must NOT persist voted_for

    // n1 receives Pre-Vote quorum -> promotes to formal candidate (term = 1) and emits formal RequestVote
    const formal_req = n1.handleVoteResponse(pre_resp, 310, 1, 5) ?? return;
    expect_true(!formal_req.is_pre_vote);
    expect_eq(formal_req.term, @as(u64, 1));
    expect_eq(n1.stats().role, RaftRole.candidate);
    expect_eq(n1.stats().current_term, @as(u64, 1));

    const formal_resp = n2.handleVoteRequest(formal_req, 310, 1, 5);
    expect_true(formal_resp.vote_granted);
    expect_eq(n2.stats().voted_for, @as(u32, 1));
    * = n1.handleVoteResponse(formal_resp, 320, 1, 5);
    expect_eq(n1.stats().role, RaftRole.leader);
    expect_eq(n1.stats().leader_id, @as(u32, 1));
    expect_eq(n1.stats().elections_won, @as(u64, 1));

    // 2. 5-node cluster (quorum = 3) with duplicate vote deduplication
    let a1 = RaftElectionNode.init(1, 5, 200, 50, false) ?? return;
    let a2 = RaftElectionNode.init(2, 5, 200, 50, false) ?? return;
    let a3 = RaftElectionNode.init(3, 5, 200, 50, false) ?? return;
    expect_eq(a1.quorumSize(), @as(usize, 3));

    const req5 = a1.tick(250, 2, 10) ?? return;
    const r2 = a2.handleVoteRequest(req5, 250, 2, 10);
    const r3 = a3.handleVoteRequest(req5, 250, 2, 10);
    * = a1.handleVoteResponse(r2, 260, 2, 10);
    // Duplicate vote from voter 2 must NOT count as quorum!
    * = a1.handleVoteResponse(r2, 265, 2, 10);
    expect_eq(a1.stats().role, RaftRole.candidate);
    expect_eq(a1.stats().votes_granted, @as(usize, 2));
    // Third distinct vote from voter 3 elects a1 as Leader
    * = a1.handleVoteResponse(r3, 270, 2, 10);
    expect_eq(a1.stats().role, RaftRole.leader);

    expect_true(bytesEq(formatRaftRole(RaftRole.leader), "leader"));
    expect_true(bytesEq(formatRaftRole(RaftRole.pre_candidate), "pre_candidate"));
}

test("RaftVote: Pre-Vote prevents partitioned node from inflating term") {
    let isolated = RaftElectionNode.init(3, 3, 200, 50, true) ?? return;

    // Isolated node times out 3 times without getting quorum Pre-Vote replies
    * = isolated.tick(200, 1, 5);
    * = isolated.tick(500, 1, 5);
    * = isolated.tick(800, 1, 5);

    // Term stays 0 (no runaway term inflation!) and voted_for stays 0
    expect_eq(isolated.stats().current_term, @as(u64, 0));
    expect_eq(isolated.stats().voted_for, RAFT_NO_NODE);
    expect_eq(isolated.stats().pre_elections_started, @as(u64, 3));
    expect_eq(isolated.stats().elections_started, @as(u64, 0));
}

test("RaftVote: Section 5.4.1 log up-to-date check rejects outdated candidates") {
    // Pure comparison tests
    expect_true(isLogUpToDate(3, 1, 2, 100));   // higher term wins despite shorter log
    expect_true(!isLogUpToDate(2, 100, 3, 1));  // lower term loses despite longer log
    expect_true(isLogUpToDate(2, 10, 2, 10));   // equal term & index ok
    expect_true(!isLogUpToDate(2, 9, 2, 10));   // equal term, shorter log rejected

    let voter = RaftElectionNode.init(2, 3, 300, 100, false) ?? return;
    // Voter has (term=3, index=20); candidate has (term=2, index=50) -> rejected
    const bad_req = RaftVoteRequest{
        term: 4,
        candidate_id: 1,
        last_log_term: 2,
        last_log_index: 50,
        is_pre_vote: false,
    };
    const resp = voter.handleVoteRequest(bad_req, 500, 3, 20);
    expect_true(!resp.vote_granted);
    expect_eq(resp.reject_reason, RaftVoteRejectReason.log_not_up_to_date);
    expect_eq(voter.stats().voted_for, RAFT_NO_NODE);

    // Up-to-date candidate (term=3, index=20) succeeds and locks voted_for=3
    const good_req = RaftVoteRequest{
        term: 4,
        candidate_id: 3,
        last_log_term: 3,
        last_log_index: 20,
        is_pre_vote: false,
    };
    expect_true(voter.handleVoteRequest(good_req, 510, 3, 20).vote_granted);

    // Competing candidate in same term 4 rejected with already_voted
    const dup_req = RaftVoteRequest{
        term: 4,
        candidate_id: 1,
        last_log_term: 3,
        last_log_index: 25,
        is_pre_vote: false,
    };
    const dup_resp = voter.handleVoteRequest(dup_req, 520, 3, 20);
    expect_true(!dup_resp.vote_granted);
    expect_eq(dup_resp.reject_reason, RaftVoteRejectReason.already_voted);
}

test("RaftVote: Leader Lease anti-disruption guard rejects spurious RequestVote") {
    let follower = RaftElectionNode.init(2, 3, 300, 100, true) ?? return;

    // Follower receives fresh heartbeat from leader #1 at t=1000 (term 5)
    expect_true(follower.handleHeartbeat(RaftHeartbeat{
        term: 5,
        leader_id: 1,
        leader_commit: 42,
    }, 1000));
    expect_true(follower.isLeaderLeaseActive(1150));

    // Spurious RequestVote from partitioned node #3 with inflated term 99 at t=1150 (< 1000+300)
    const disruptive = RaftVoteRequest{
        term: 99,
        candidate_id: 3,
        last_log_term: 5,
        last_log_index: 50,
        is_pre_vote: false,
    };
    const resp = follower.handleVoteRequest(disruptive, 1150, 5, 42);
    expect_true(!resp.vote_granted);
    expect_eq(resp.reject_reason, RaftVoteRejectReason.leader_lease_active);
    expect_eq(follower.stats().current_term, @as(u64, 5)); // term NOT disrupted!
    expect_eq(follower.stats().lease_rejections, @as(u64, 1));

    // After lease expires at t=1350, follower accepts valid election
    expect_true(!follower.isLeaderLeaseActive(1350));
    const after_lease = follower.handleVoteRequest(disruptive, 1350, 5, 42);
    expect_true(after_lease.vote_granted);
    expect_eq(follower.stats().current_term, @as(u64, 99));
}

test("RaftVote: higher term heartbeat forces leader step-down & rejects stale heartbeat") {
    let leader = RaftElectionNode.init(1, 1, 200, 50, false) ?? return;
    * = leader.tick(200, 1, 10); // single-node self-elects at term 1
    expect_eq(leader.stats().role, RaftRole.leader);
    expect_eq(leader.stats().current_term, @as(u64, 1));

    // Stale heartbeat (term 0) rejected
    expect_true(!leader.handleHeartbeat(RaftHeartbeat{ term: 0, leader_id: 2, leader_commit: 5 }, 250));
    expect_eq(leader.stats().role, RaftRole.leader);

    // Higher-term heartbeat (term 3) forces stepDown -> follower
    expect_true(leader.handleHeartbeat(RaftHeartbeat{ term: 3, leader_id: 2, leader_commit: 15 }, 300));
    expect_eq(leader.stats().role, RaftRole.follower);
    expect_eq(leader.stats().current_term, @as(u64, 3));
    expect_eq(leader.stats().leader_id, @as(u32, 2));
    expect_eq(leader.stats().commit_index, @as(u64, 15));
    expect_eq(leader.stats().step_downs, @as(u64, 1));
}

test("RaftVote: Leader computeQuorumCommitIndex & cap.db facade parity") {
    // 5-node cluster (quorum = 3): leader #1 has local_last_index = 100
    let leader = cap.db.RaftElectionNode.init(1, 5, 200, 50, false) ?? return;
    const req = leader.tick(200, 1, 100) ?? return;
    let f2 = cap.db.RaftElectionNode.init(2, 5, 200, 50, false) ?? return;
    let f3 = cap.db.RaftElectionNode.init(3, 5, 200, 50, false) ?? return;
    * = leader.handleVoteResponse(f2.handleVoteRequest(req, 200, 1, 100), 210, 1, 100);
    * = leader.handleVoteResponse(f3.handleVoteRequest(req, 200, 1, 100), 215, 1, 100);
    expect_eq(leader.stats().role, cap.db.RaftRole.leader);

    // Only leader (100) + peer 2 (80) have logs -> 2/5 < quorum(3) -> commit remains 0
    expect_true(leader.recordPeerAck(2, 80, 220));
    expect_eq(leader.computeQuorumCommitIndex(100), @as(u64, 0));

    // Peer 3 acks 60 -> sorted [100, 80, 60, 0, 0] -> 3rd largest is 60!
    expect_true(leader.recordPeerAck(3, 60, 230));
    expect_eq(leader.advanceCommitIndex(100), @as(u64, 60));

    // Peer 4 acks 95, Peer 5 acks 90 -> sorted [100, 95, 90, 80, 60] -> 3rd largest is 90!
    expect_true(leader.recordPeerAck(4, 95, 240));
    expect_true(leader.recordPeerAck(5, 90, 250));
    expect_eq(leader.advanceCommitIndex(100), @as(u64, 90));
    expect_eq(leader.stats().commit_index, @as(u64, 90));
}
