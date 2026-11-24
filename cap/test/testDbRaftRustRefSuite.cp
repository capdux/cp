// Reference-hardened test suite against tikv/raft-rs (Apache-2.0) & Raft consensus specification
// Covers Election Safety, Pre-Vote anti-disruption, Figure-8 commit guard, Log Matching conflict
// truncation & committed-log protection, 4-lane @Vector(4, u64) SIMD 0-diff parity, and 1,600-step
// deterministic PRNG 5-node Raft cluster simulation fuzzing.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    RAFT_NO_NODE,
    RaftRole,
    RaftVoteRejectReason,
    RaftVoteRequest,
    RaftHeartbeat,
    simdRaftCountAtLeastU64,
    scalarRaftCountAtLeastU64,
    simdRaftMaxU64,
    scalarRaftMaxU64,
    hasQuorum,
    isLogUpToDate,
    RaftElectionNode,
} from "../../lib/db/raftVote.cp";
import {
    RaftLogError,
    LogEntryInput,
    RaftLog,
} from "../../lib/db/raftLog.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

fn checkElectionPreVoteAndFigure8(): u32 {
    // 1. Overflow-safe quorum & Section 5.4.1 log up-to-date order
    if (hasQuorum(0, 0) || hasQuorum(1, 2) || !hasQuorum(2, 3) || hasQuorum(2, 4) || !hasQuorum(3, 5)) return 1;
    if (!isLogUpToDate(2, 1, 1, 100)) return 2; // higher term wins even with shorter index
    if (isLogUpToDate(1, 100, 2, 1)) return 3;
    if (!isLogUpToDate(2, 10, 2, 10) || isLogUpToDate(2, 9, 2, 10)) return 4;

    // 2. Pre-Vote does NOT increment current_term on isolated node
    let n1 = RaftElectionNode.init(1, 3, 300, 100, true) ?? return 5;
    let n2 = RaftElectionNode.init(2, 3, 300, 100, true) ?? return 6;
    let n3 = RaftElectionNode.init(3, 3, 300, 100, true) ?? return 7;

    const pre_req = n1.tick(400, 1, 5) ?? return 8;
    if (!pre_req.is_pre_vote || pre_req.term != 1 || n1.stats().current_term != 0) return 9;

    // Voter rejects stale log pre-vote without mutating its term or voted_for
    const pre_rej = n2.handleVoteRequest(pre_req, 400, 2, 1);
    if (pre_rej.vote_granted || pre_rej.reject_reason != RaftVoteRejectReason.log_not_up_to_date) return 10;
    if (n2.stats().current_term != 0 || n2.stats().voted_for != RAFT_NO_NODE) return 11;

    // Valid pre-vote -> promotes n1 to candidate(term=1) -> formal vote -> n1 becomes leader
    const pre_ok = n3.handleVoteRequest(pre_req, 400, 1, 5);
    if (!pre_ok.vote_granted) return 12;
    const vote_req = n1.handleVoteResponse(pre_ok, 410, 1, 5) ?? return 13;
    if (vote_req.is_pre_vote || vote_req.term != 1 || n1.stats().role != RaftRole.candidate) return 14;

    // Same-term double-vote prevention on n3
    const vote_ok = n3.handleVoteRequest(vote_req, 410, 1, 5);
    if (!vote_ok.vote_granted || n3.stats().voted_for != 1) return 15;
    const dup_req = RaftVoteRequest{
        term: 1,
        candidate_id: 2,
        last_log_index: 10,
        last_log_term: 1,
        is_pre_vote: false,
    };
    const dup_resp = n3.handleVoteRequest(dup_req, 420, 1, 5);
    if (dup_resp.vote_granted || dup_resp.reject_reason != RaftVoteRejectReason.already_voted) return 16;

    // Higher term forces step-down & clears voted_for
    * = n1.handleVoteResponse(vote_ok, 420, 1, 5);
    if (n1.stats().role != RaftRole.leader) return 17;

    // 3. Figure 8 safety: leader (term=1) replicates index 10 whose entry term is 0 (old term) -> MUST NOT commit
    * = n1.recordPeerAck(2, 10, 430);
    * = n1.recordPeerAck(3, 10, 430);
    const c_old = n1.advanceCommitIndexForTerm(10, 0);
    if (c_old != 0) return 18;
    // Once entry at quorum belongs to current_term (1), commit_index advances to 10
    const c_cur = n1.advanceCommitIndexForTerm(10, 1);
    if (c_cur != 10) return 19;

    // Higher term heartbeat forces leader to step down to follower and clear voted_for
    const hb_hi = RaftHeartbeat{ term: 5, leader_id: 2, leader_commit: 8 };
    if (!n1.handleHeartbeatBounded(hb_hi, 500, 10)) return 20;
    if (n1.stats().role != RaftRole.follower || n1.stats().current_term != 5 || n1.stats().voted_for != RAFT_NO_NODE) return 21;
    // commit_index stays monotonic (10, never regresses to 8)
    if (n1.stats().commit_index != 10) return 22;

    return 0;
}

fn checkLogMatchingAndSnapshotGuards(alloc: std.mem.Allocator): u32 {
    let log = RaftLog.init(alloc);
    defer log.deinit();

    const idx1 = log.append(1, "cmd1") catch return 1;
    const idx2 = log.append(1, "cmd2") catch return 2;
    const idx3 = log.append(2, "cmd3") catch return 3;
    if (idx1 != 1 || idx2 != 2 || idx3 != 3 || !log.verifyInvariants()) return 4;

    // Reject non-monotonic term append
    let hit_term = false;
    * = log.append(1, "bad") catch |e| {
        if (e == RaftLogError.InvalidTermOrder) hit_term = true;
    };
    if (!hit_term) return 5;

    // Commit up to index 2
    log.commitTo(2) catch return 6;

    // Commit monotonicity & out-of-bounds guards
    let hit_inv_c = false;
    log.commitTo(1) catch |e| {
        if (e == RaftLogError.InvalidCommitIndex) hit_inv_c = true;
    };
    if (!hit_inv_c) return 7;
    let hit_oob = false;
    log.commitTo(99) catch |e| {
        if (e == RaftLogError.OutOfBounds) hit_oob = true;
    };
    if (!hit_oob) return 8;

    // Strictly forbid truncating committed entry (index 2 <= commit_index 2)
    const conflict_committed: [1]LogEntryInput = [LogEntryInput{ term: 3, data: "override" }];
    let hit_trunc = false;
    * = log.appendIfMatch(1, 1, conflict_committed[0..]) catch |e| {
        if (e == RaftLogError.TruncateCommittedIndex) hit_trunc = true;
    };
    if (!hit_trunc) return 9;

    // Valid conflict truncation on uncommitted suffix (index 3 > commit_index 2)
    const replace_uncommitted: [2]LogEntryInput = [
        LogEntryInput{ term: 3, data: "new3" },
        LogEntryInput{ term: 3, data: "new4" },
    ];
    const ok_rep = log.appendIfMatch(2, 1, replace_uncommitted[0..]) catch return 10;
    if (!ok_rep || log.lastIndex() != 4 || (log.termAt(3) catch 0) != 3 || !log.verifyInvariants()) return 11;

    // Snapshot compaction & stale installSnapshot guard
    log.commitTo(4) catch return 12;
    log.compactTo(3) catch return 13;
    if (log.snapshotIndex() != 3 || log.snapshotTerm() != 3 || !log.verifyInvariants()) return 14;

    let hit_compacted = false;
    * = log.termAt(2) catch |e| {
        if (e == RaftLogError.CompactedIndex) hit_compacted = true;
    };
    if (!hit_compacted) return 15;

    let hit_stale_snap = false;
    log.installSnapshot(4, 3) catch |e| {
        if (e == RaftLogError.CompactedIndex) hit_stale_snap = true;
    };
    if (!hit_stale_snap) return 16;

    log.installSnapshot(10, 5) catch return 17;
    if (log.snapshotIndex() != 10 || log.commitIndex() != 10 || log.lastIndex() != 10 || !log.verifyInvariants()) return 18;

    return 0;
}

fn checkSimdDifferential(): u32 {
    let prng = XorShift64.init(0x5241_4654_5349_4d44);
    let vals: [132]u64 = [0; 132];
    let len: usize = 0;
    while (len <= 131) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            vals[i] = prng.next() % 1000;
        }
        const max_u64: u64 = std.math.maxInt(u64);
        if (len > 0) {
            vals[len - 1] = max_u64;
        }
        const targets: [4]u64 = [0, 500, 999, max_u64];
        for (t of targets) {
            if (simdRaftCountAtLeastU64(vals[0..len], t) != scalarRaftCountAtLeastU64(vals[0..len], t)) return 1;
        }
        if (simdRaftMaxU64(vals[0..len]) != scalarRaftMaxU64(vals[0..len])) return 2;
    }
    return 0;
}

fn checkMultiNodeRaftFuzz(): u32 {
    let prng = XorShift64.init(0x2026_1124_5241_4654);

    // 5-node Raft cluster with Pre-Vote enabled
    let nodes: [5]RaftElectionNode = [RaftElectionNode{}; 5];
    let prev_commits: [5]u64 = [0; 5];
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        nodes[i] = RaftElectionNode.init(@as(u32, @intCast(i + 1)), 5, 200, 50, true) ?? return 1;
    }

    // Track unique leader per term (Election Safety: at most 1 leader per term globally)
    let term_leader: [256]u32 = [0; 256];
    let now_ms: u64 = 1000;

    // 1,600 deterministic PRNG simulation steps with random partitions, drops, elections & heartbeats
    let step: usize = 0;
    while (step < 1600) : (step += 1) {
        now_ms += 10 + (prng.next() % 90);
        const actor = @as(usize, @intCast(prng.next() % 5));
        const action = prng.next() % 4;

        if (action == 0) {
            // Election timeout tick & broadcast vote/pre-vote with random packet drops
            if (req := nodes[actor].tick(now_ms, 1, 10)) {
                let peer: usize = 0;
                while (peer < 5) : (peer += 1) {
                    if (peer == actor) continue;
                    if ((prng.next() % 4) == 0) continue; // 25% drop / partition
                    const resp = nodes[peer].handleVoteRequest(req, now_ms, 1, 10);
                    if ((prng.next() % 4) == 0) continue;
                    if (next_req := nodes[actor].handleVoteResponse(resp, now_ms + 5, 1, 10)) {
                        // Formal RequestVote broadcast after Pre-Vote quorum
                        let p2: usize = 0;
                        while (p2 < 5) : (p2 += 1) {
                            if (p2 == actor) continue;
                            if ((prng.next() % 4) == 0) continue;
                            const r2 = nodes[p2].handleVoteRequest(next_req, now_ms + 10, 1, 10);
                            * = nodes[actor].handleVoteResponse(r2, now_ms + 15, 1, 10);
                        }
                    }
                }
            }
        } else if (action == 1) {
            // If actor is leader, send heartbeat & collect peer acks -> Figure-8 safe commit advance
            const st = nodes[actor].stats();
            if (st.role == RaftRole.leader) {
                const hb = RaftHeartbeat{
                    term: st.current_term,
                    leader_id: st.node_id,
                    leader_commit: st.commit_index,
                };
                let peer: usize = 0;
                while (peer < 5) : (peer += 1) {
                    if (peer == actor) continue;
                    if ((prng.next() % 3) == 0) continue;
                    if (nodes[peer].handleHeartbeatBounded(hb, now_ms, 10)) {
                        * = nodes[actor].recordPeerAck(@as(u32, @intCast(peer + 1)), 10, now_ms);
                    }
                }
                * = nodes[actor].advanceCommitIndexForTerm(10, st.current_term);
            }
        } else if (action == 2) {
            // Out-of-order stale heartbeat injection (must be rejected safely)
            const target = @as(usize, @intCast(prng.next() % 5));
            const stale_hb = RaftHeartbeat{ term: 0, leader_id: 1, leader_commit: 999 };
            * = nodes[target].handleHeartbeatBounded(stale_hb, now_ms, 10);
        } else {
            // Advance clock to allow new elections under partition recovery
            now_ms += 250;
        }

        // Verify Election Safety & Commit Monotonicity after EVERY step
        let ni: usize = 0;
        while (ni < 5) : (ni += 1) {
            const s = nodes[ni].stats();
            if (s.commit_index < prev_commits[ni]) return 2; // commit_index regressed!
            prev_commits[ni] = s.commit_index;

            if (s.role == RaftRole.leader && s.current_term < 256) {
                const t_idx = @as(usize, @intCast(s.current_term));
                if (term_leader[t_idx] == 0) {
                    term_leader[t_idx] = s.node_id;
                } else if (term_leader[t_idx] != s.node_id) {
                    return 3; // Election Safety violated: two different leaders in same term!
                }
            }
        }
    }

    return 0;
}

fn main() {
    test("1. Raft Election Safety, Pre-Vote anti-disruption, Figure-8 commit & quorum guards", () => {
        expect_eq(checkElectionPreVoteAndFigure8(), 0 as u32);
    });

    test("2. RaftLog Log Matching conflict truncation, committed-log & snapshot guards", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        expect_eq(checkLogMatchingAndSnapshotGuards(alloc), 0 as u32);
    });

    test("3. 4-lane @Vector(4, u64) SIMD quorum count & max scan vs scalar 0-diff parity", () => {
        expect_eq(checkSimdDifferential(), 0 as u32);
    });

    test("4. Deterministic PRNG 1,600-step 5-node Raft fuzzing (Election Safety & Commit Monotonicity)", () => {
        expect_eq(checkMultiNodeRaftFuzz(), 0 as u32);
    });
}
