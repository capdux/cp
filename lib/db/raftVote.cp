// Origin: Pure-CP Stdlib (Zero-Alloc Sans-IO Raft Election, Pre-Vote & Quorum Commit FSM) | Created: 2026-11-23
// Pure CP zero-heap-allocation Sans-IO Raft Leader Election, Pre-Vote & Quorum Commit State Machine:
//   - Strict Raft Section 5.4.1 log up-to-date comparison (`isLogUpToDate`)
//   - Pre-Vote protocol preventing partitioned/isolated nodes from inflating cluster terms
//   - Leader Lease anti-disruption guard rejecting spurious RequestVote RPCs while leader is healthy
//   - Deterministic quorum commit index computation (`computeQuorumCommitIndex`) via zero-heap
//     in-stack sorting of `(local_last_index, peer match_indices)`
//   - Complements `lib/db/raftLog.cp` with zero C or I/O dependencies

// ============================================================================
// 1. Constants, Enums & Message Value Objects
// ============================================================================

/// Maximum cluster size supported inline by `RaftElectionNode`.
pub const RAFT_MAX_PEERS: usize = 15;

/// Sentinel node ID indicating no leader or no vote cast.
pub const RAFT_NO_NODE: u32 = 0;

/// Current consensus role of a `RaftElectionNode`.
pub enum RaftRole {
    follower,
    pre_candidate,
    candidate,
    leader,
}

/// Reason why a vote or pre-vote request was rejected.
pub enum RaftVoteRejectReason {
    none,
    stale_term,
    already_voted,
    log_not_up_to_date,
    leader_lease_active,
}

/// Sans-IO RequestVote / Pre-Vote RPC message.
pub struct RaftVoteRequest {
    term: u64 = 0;
    candidate_id: u32 = 0;
    last_log_index: u64 = 0;
    last_log_term: u64 = 0;
    is_pre_vote: bool = false;
}

/// Sans-IO RequestVote / Pre-Vote RPC reply message.
pub struct RaftVoteResponse {
    term: u64 = 0;
    voter_id: u32 = 0;
    vote_granted: bool = false;
    is_pre_vote: bool = false;
    reject_reason: RaftVoteRejectReason = RaftVoteRejectReason.none;
}

/// Sans-IO AppendEntries heartbeat message emitted by the active leader.
pub struct RaftHeartbeat {
    term: u64 = 0;
    leader_id: u32 = 0;
    leader_commit: u64 = 0;
}

/// Per-peer replication progress tracked by a leader.
pub struct RaftPeerProgress {
    active: bool = false;
    peer_id: u32 = 0;
    match_index: u64 = 0;
    next_index: u64 = 1;
    last_ack_ms: u64 = 0;
}

/// Zero-heap telemetry snapshot of a `RaftElectionNode`.
pub struct RaftElectionStats {
    node_id: u32 = 0;
    role: RaftRole = RaftRole.follower;
    current_term: u64 = 0;
    voted_for: u32 = RAFT_NO_NODE;
    leader_id: u32 = RAFT_NO_NODE;
    cluster_size: usize = 0;
    quorum_size: usize = 0;
    commit_index: u64 = 0;
    votes_granted: usize = 0;
    pre_elections_started: u64 = 0;
    elections_started: u64 = 0;
    elections_won: u64 = 0;
    step_downs: u64 = 0;
    lease_rejections: u64 = 0;
}

// ============================================================================
// 2. Pure Consensus Helpers
// ============================================================================

/// Raft Section 5.4.1 log up-to-date comparison:
/// Returns true iff candidate's log `(cand_last_term, cand_last_index)` is at least as up-to-date
/// as voter's log `(local_last_term, local_last_index)`.
pub fn isLogUpToDate(
    cand_last_term: u64,
    cand_last_index: u64,
    local_last_term: u64,
    local_last_index: u64,
): bool {
    if (cand_last_term != local_last_term) {
        return cand_last_term > local_last_term;
    }
    return cand_last_index >= local_last_index;
}

/// Format a `RaftRole` variant as a canonical lowercase ASCII label.
pub fn formatRaftRole(role: RaftRole): []const u8 {
    if (role == RaftRole.follower) return "follower";
    if (role == RaftRole.pre_candidate) return "pre_candidate";
    if (role == RaftRole.candidate) return "candidate";
    return "leader";
}

// ============================================================================
// 3. RaftElectionNode — Sans-IO Raft Election, Pre-Vote & Quorum Commit FSM
// ============================================================================

/// Zero-heap-allocation Sans-IO Raft election and quorum commit state machine.
pub struct RaftElectionNode {
    node_id: u32 = 0;
    cluster_size: usize = 1;
    election_timeout_ms: u64 = 300;
    heartbeat_interval_ms: u64 = 100;
    enable_pre_vote: bool = true;

    role: RaftRole = RaftRole.follower;
    current_term: u64 = 0;
    voted_for: u32 = RAFT_NO_NODE;
    leader_id: u32 = RAFT_NO_NODE;
    commit_index: u64 = 0;
    last_heard_ms: u64 = 0;
    has_heard_leader: bool = false;

    voters: [RAFT_MAX_PEERS]u32 = [0; RAFT_MAX_PEERS];
    vote_count: usize = 0;

    peers: [RAFT_MAX_PEERS]RaftPeerProgress = [RaftPeerProgress{}; RAFT_MAX_PEERS];
    peer_count: usize = 0;

    pre_elections_started: u64 = 0;
    elections_started: u64 = 0;
    elections_won: u64 = 0;
    step_downs: u64 = 0;
    lease_rejections: u64 = 0;

    /// Initialize a new `RaftElectionNode` in `follower` state.
    /// Returns `null` if `node_id == 0`, `cluster_size` is outside `1..=RAFT_MAX_PEERS`,
    /// or timeout/heartbeat intervals are zero.
    static fn init(
        node_id: u32,
        cluster_size: usize,
        election_timeout_ms: u64,
        heartbeat_interval_ms: u64,
        enable_pre_vote: bool,
    ): RaftElectionNode? {
        if (node_id == RAFT_NO_NODE) return null;
        if (cluster_size == 0 || cluster_size > RAFT_MAX_PEERS) return null;
        if (election_timeout_ms == 0 || heartbeat_interval_ms == 0) return null;

        let n = RaftElectionNode{};
        n.node_id = node_id;
        n.cluster_size = cluster_size;
        n.election_timeout_ms = election_timeout_ms;
        n.heartbeat_interval_ms = heartbeat_interval_ms;
        n.enable_pre_vote = enable_pre_vote;
        return n;
    }

    /// Return strict majority quorum size `(cluster_size / 2) + 1`.
    fn quorumSize(): usize {
        return (cluster_size / 2) + 1;
    }

    /// Return true if a healthy leader lease is currently active at `now_ms`.
    fn isLeaderLeaseActive(now_ms: u64): bool {
        if (role == RaftRole.leader) return true;
        if (!has_heard_leader || leader_id == RAFT_NO_NODE) return false;
        if (now_ms < last_heard_ms) return true;
        return (now_ms - last_heard_ms) < election_timeout_ms;
    }

    fn resetVotes(*_) {
        let i: usize = 0;
        while (i < RAFT_MAX_PEERS) : (i += 1) {
            voters[i] = 0;
        }
        vote_count = 0;
    }

    fn recordVote(*_, voter_id: u32): bool {
        if (voter_id == RAFT_NO_NODE) return false;
        let i: usize = 0;
        while (i < vote_count) : (i += 1) {
            if (voters[i] == voter_id) return false; // duplicate vote ignored
        }
        if (vote_count >= RAFT_MAX_PEERS) return false;
        voters[vote_count] = voter_id;
        vote_count += 1;
        return true;
    }

    /// Immediately step down to `follower` at `new_term` (clearing `voted_for` if term advanced).
    fn stepDown(*_, new_term: u64) {
        if (new_term > current_term) {
            current_term = new_term;
            voted_for = RAFT_NO_NODE;
        }
        if (role != RaftRole.follower) {
            step_downs += 1;
        }
        role = RaftRole.follower;
        leader_id = RAFT_NO_NODE;
        has_heard_leader = false;
        resetVotes();
    }

    fn becomeLeader(*_, now_ms: u64, last_log_index: u64) {
        role = RaftRole.leader;
        leader_id = node_id;
        last_heard_ms = now_ms;
        has_heard_leader = true;
        elections_won += 1;
        let i: usize = 0;
        while (i < peer_count) : (i += 1) {
            if (peers[i].active) {
                peers[i].match_index = 0;
                peers[i].next_index = last_log_index + 1;
            }
        }
    }

    /// Advance election timer at `now_ms`.
    /// When `election_timeout_ms` elapses without leader contact, initiates a Pre-Vote
    /// (if `enable_pre_vote`) or a formal RequestVote and returns the outbound RPC request.
    fn tick(*_, now_ms: u64, last_log_term: u64, last_log_index: u64): RaftVoteRequest? {
        if (role == RaftRole.leader) return null;
        if (now_ms < last_heard_ms || (now_ms - last_heard_ms) < election_timeout_ms) {
            return null;
        }

        // Single-node cluster elects itself immediately.
        if (cluster_size == 1) {
            current_term += 1;
            voted_for = node_id;
            elections_started += 1;
            resetVotes();
            * = recordVote(node_id);
            becomeLeader(now_ms, last_log_index);
            return null;
        }

        last_heard_ms = now_ms;
        has_heard_leader = false;
        leader_id = RAFT_NO_NODE;
        resetVotes();
        * = recordVote(node_id);

        if (enable_pre_vote) {
            role = RaftRole.pre_candidate;
            pre_elections_started += 1;
            // Pre-Vote probes `current_term + 1` WITHOUT incrementing `current_term` or `voted_for`.
            return RaftVoteRequest{
                term: current_term + 1,
                candidate_id: node_id,
                last_log_index: last_log_index,
                last_log_term: last_log_term,
                is_pre_vote: true,
            };
        }

        role = RaftRole.candidate;
        current_term += 1;
        voted_for = node_id;
        elections_started += 1;
        return RaftVoteRequest{
            term: current_term,
            candidate_id: node_id,
            last_log_index: last_log_index,
            last_log_term: last_log_term,
            is_pre_vote: false,
        };
    }

    /// Process an inbound `RaftVoteRequest` (Pre-Vote or formal RequestVote) and return the reply.
    /// Enforces Leader Lease anti-disruption, term monotonicity, Pre-Vote immutability,
    /// single-vote-per-term persistence, and Section 5.4.1 log up-to-date checks.
    fn handleVoteRequest(
        *_,
        req: RaftVoteRequest,
        now_ms: u64,
        local_last_term: u64,
        local_last_index: u64,
    ): RaftVoteResponse {
        // 1. Leader Lease guard: reject disruptive vote requests while a healthy leader lease holds.
        if (isLeaderLeaseActive(now_ms) && req.candidate_id != leader_id) {
            lease_rejections += 1;
            return RaftVoteResponse{
                term: current_term,
                voter_id: node_id,
                vote_granted: false,
                is_pre_vote: req.is_pre_vote,
                reject_reason: RaftVoteRejectReason.leader_lease_active,
            };
        }

        // 2. Pre-Vote path: NEVER mutate `current_term`, `voted_for`, or `role`.
        if (req.is_pre_vote) {
            if (req.term < current_term) {
                return RaftVoteResponse{
                    term: current_term,
                    voter_id: node_id,
                    vote_granted: false,
                    is_pre_vote: true,
                    reject_reason: RaftVoteRejectReason.stale_term,
                };
            }
            if (!isLogUpToDate(req.last_log_term, req.last_log_index, local_last_term, local_last_index)) {
                return RaftVoteResponse{
                    term: current_term,
                    voter_id: node_id,
                    vote_granted: false,
                    is_pre_vote: true,
                    reject_reason: RaftVoteRejectReason.log_not_up_to_date,
                };
            }
            return RaftVoteResponse{
                term: req.term,
                voter_id: node_id,
                vote_granted: true,
                is_pre_vote: true,
                reject_reason: RaftVoteRejectReason.none,
            };
        }

        // 3. Formal RequestVote path
        if (req.term < current_term) {
            return RaftVoteResponse{
                term: current_term,
                voter_id: node_id,
                vote_granted: false,
                is_pre_vote: false,
                reject_reason: RaftVoteRejectReason.stale_term,
            };
        }

        if (req.term > current_term) {
            stepDown(req.term);
        }

        if (voted_for != RAFT_NO_NODE && voted_for != req.candidate_id) {
            return RaftVoteResponse{
                term: current_term,
                voter_id: node_id,
                vote_granted: false,
                is_pre_vote: false,
                reject_reason: RaftVoteRejectReason.already_voted,
            };
        }

        if (!isLogUpToDate(req.last_log_term, req.last_log_index, local_last_term, local_last_index)) {
            return RaftVoteResponse{
                term: current_term,
                voter_id: node_id,
                vote_granted: false,
                is_pre_vote: false,
                reject_reason: RaftVoteRejectReason.log_not_up_to_date,
            };
        }

        voted_for = req.candidate_id;
        last_heard_ms = now_ms;
        return RaftVoteResponse{
            term: current_term,
            voter_id: node_id,
            vote_granted: true,
            is_pre_vote: false,
            reject_reason: RaftVoteRejectReason.none,
        };
    }

    /// Process an inbound `RaftVoteResponse`.
    /// - When a `pre_candidate` reaches quorum, promotes to formal `candidate` (incrementing term)
    ///   and returns the formal `RaftVoteRequest` to broadcast.
    /// - When a formal `candidate` reaches quorum, transitions directly to `leader`.
    /// - Observing a higher term immediately steps down to `follower`.
    fn handleVoteResponse(
        *_,
        resp: RaftVoteResponse,
        now_ms: u64,
        last_log_term: u64,
        last_log_index: u64,
    ): RaftVoteRequest? {
        if (!resp.is_pre_vote && resp.term > current_term) {
            stepDown(resp.term);
            return null;
        }

        if (resp.is_pre_vote) {
            if (role != RaftRole.pre_candidate) return null;
            if (!resp.vote_granted && resp.term > current_term) {
                stepDown(resp.term);
                return null;
            }
            if (resp.vote_granted && resp.term == current_term + 1) {
                * = recordVote(resp.voter_id);
                if (vote_count >= quorumSize()) {
                    role = RaftRole.candidate;
                    current_term += 1;
                    voted_for = node_id;
                    leader_id = RAFT_NO_NODE;
                    last_heard_ms = now_ms;
                    elections_started += 1;
                    resetVotes();
                    * = recordVote(node_id);
                    return RaftVoteRequest{
                        term: current_term,
                        candidate_id: node_id,
                        last_log_index: last_log_index,
                        last_log_term: last_log_term,
                        is_pre_vote: false,
                    };
                }
            }
            return null;
        }

        // Formal candidate vote tally
        if (role != RaftRole.candidate || resp.term != current_term) return null;
        if (resp.vote_granted) {
            * = recordVote(resp.voter_id);
            if (vote_count >= quorumSize()) {
                becomeLeader(now_ms, last_log_index);
            }
        }
        return null;
    }

    /// Process an inbound leader heartbeat (`AppendEntries` empty RPC).
    /// Refreshes `last_heard_ms` and `leader_id` for valid terms (`>= current_term`),
    /// stepping down if currently a candidate or behind in term. Rejects stale terms.
    fn handleHeartbeat(*_, hb: RaftHeartbeat, now_ms: u64): bool {
        if (hb.term < current_term) return false;
        if (hb.term > current_term || role != RaftRole.follower) {
            stepDown(hb.term);
        }
        leader_id = hb.leader_id;
        last_heard_ms = now_ms;
        has_heard_leader = true;
        if (hb.leader_commit > commit_index) {
            commit_index = hb.leader_commit;
        }
        return true;
    }

    /// Record a follower's replication acknowledgment (`match_index`) on the leader.
    fn recordPeerAck(*_, peer_id: u32, match_index: u64, now_ms: u64): bool {
        if (peer_id == RAFT_NO_NODE || peer_id == node_id) return false;
        let i: usize = 0;
        while (i < peer_count) : (i += 1) {
            if (peers[i].active && peers[i].peer_id == peer_id) {
                if (match_index > peers[i].match_index) {
                    peers[i].match_index = match_index;
                }
                peers[i].next_index = peers[i].match_index + 1;
                peers[i].last_ack_ms = now_ms;
                return true;
            }
        }
        if (peer_count >= RAFT_MAX_PEERS) return false;
        peers[peer_count] = RaftPeerProgress{
            active: true,
            peer_id: peer_id,
            match_index: match_index,
            next_index: match_index + 1,
            last_ack_ms: now_ms,
        };
        peer_count += 1;
        return true;
    }

    /// Compute the highest log index replicated on a strict majority quorum (`quorumSize()`),
    /// monotonically `>= commit_index` and bounded by `local_last_index`, with zero heap allocation.
    fn computeQuorumCommitIndex(local_last_index: u64): u64 {
        let indices: [RAFT_MAX_PEERS]u64 = [0; RAFT_MAX_PEERS];
        indices[0] = local_last_index;
        let filled: usize = 1;
        let i: usize = 0;
        while (i < peer_count && filled < cluster_size) : (i += 1) {
            if (peers[i].active) {
                const m = if (peers[i].match_index > local_last_index) local_last_index else peers[i].match_index;
                indices[filled] = m;
                filled += 1;
            }
        }

        // Insertion sort descending over `cluster_size` slots (unheard peers remain 0).
        let a: usize = 1;
        while (a < cluster_size) : (a += 1) {
            const key = indices[a];
            let b = a;
            while (b > 0 && indices[b - 1] < key) {
                indices[b] = indices[b - 1];
                b -= 1;
            }
            indices[b] = key;
        }

        const q_idx = quorumSize() - 1;
        const quorum_match = indices[q_idx];
        return if (quorum_match > commit_index) quorum_match else commit_index;
    }

    /// Advance and return the leader's persisted `commit_index` using `computeQuorumCommitIndex`.
    fn advanceCommitIndex(*_, local_last_index: u64): u64 {
        if (role != RaftRole.leader) return commit_index;
        const next_commit = computeQuorumCommitIndex(local_last_index);
        if (next_commit > commit_index) {
            commit_index = next_commit;
        }
        return commit_index;
    }

    /// Return a zero-heap telemetry snapshot.
    fn stats(): RaftElectionStats {
        return RaftElectionStats{
            node_id: node_id,
            role: role,
            current_term: current_term,
            voted_for: voted_for,
            leader_id: leader_id,
            cluster_size: cluster_size,
            quorum_size: quorumSize(),
            commit_index: commit_index,
            votes_granted: vote_count,
            pre_elections_started: pre_elections_started,
            elections_started: elections_started,
            elections_won: elections_won,
            step_downs: step_downs,
            lease_rejections: lease_rejections,
        };
    }
}
