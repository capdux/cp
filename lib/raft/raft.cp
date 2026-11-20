// Source: cp/lib/raft/raft.cp (Deterministic Event-Driven Raft Consensus State Machine)
// Pure CP zero-network-coupling deterministic Raft consensus core supporting
// Follower/Candidate/Leader state transitions, RequestVote (with log up-to-date check),
// AppendEntries (with consistency check & conflict truncation), Figure 8 safe
// current-term quorum commit advancement, and committed log extraction.

const std = @import("std");

/// Error set for Raft state machine operations.
pub err RaftError {
    NotLeader,
    EmptyPeers,
    DuplicateNode,
    InvalidIndex,
    OutOfMemory,
}

/// Role of a node in the Raft consensus cluster.
pub enum RaftRole {
    Follower,
    Candidate,
    Leader,
}

/// A single replicated state machine command entry in the Raft log.
/// Raft log indices are 1-based; index 0 is the empty sentinel.
pub struct LogEntry {
    term: u64 = 0,
    index: u64 = 0,
    command: string = "",
}

/// RequestVote RPC arguments (Raft Section 5.2 & 5.4.1).
pub struct RequestVoteReq {
    term: u64 = 0,
    candidate_id: u64 = 0,
    last_log_index: u64 = 0,
    last_log_term: u64 = 0,
}

/// RequestVote RPC response.
pub struct RequestVoteResp {
    from_node: u64 = 0,
    term: u64 = 0,
    vote_granted: bool = false,
}

/// AppendEntries RPC arguments (used for both heartbeats and log replication).
pub struct AppendEntriesReq {
    term: u64 = 0,
    leader_id: u64 = 0,
    prev_log_index: u64 = 0,
    prev_log_term: u64 = 0,
    entries: []const LogEntry = [],
    leader_commit: u64 = 0,
}

/// AppendEntries RPC response.
pub struct AppendEntriesResp {
    from_node: u64 = 0,
    term: u64 = 0,
    success: bool = false,
    match_index: u64 = 0,
    conflict_index: u64 = 0,
}

/// Tagged union payload for outbound Raft messages.
pub enum RaftPayload {
    RequestVote: RequestVoteReq,
    RequestVoteResp: RequestVoteResp,
    AppendEntries: AppendEntriesReq,
    AppendEntriesResp: AppendEntriesResp,
}

/// Outbound message queued by `RaftNode` for external transport or cluster simulation.
pub struct RaftOutboundMsg {
    to: u64 = 0,
    from: u64 = 0,
    payload: RaftPayload = undefined,
}

/// Deterministic, event-driven Raft consensus node state machine.
pub struct RaftNode {
    alloc: std.mem.Allocator = undefined,
    node_id: u64 = 0,
    peers: []u64 = [],
    election_timeout_ticks: u64 = 10,
    heartbeat_ticks: u64 = 3,

    // Persistent Raft state
    current_term: u64 = 0,
    voted_for: u64? = null,
    logs: std.ArrayList(LogEntry) = .empty,

    // Volatile Raft state
    role: RaftRole = .Follower,
    leader_id: u64? = null,
    commit_index: u64 = 0,
    last_applied: u64 = 0,
    election_elapsed: u64 = 0,
    heartbeat_elapsed: u64 = 0,

    // Election tracking (per-peer vote bitmap)
    votes_received: []bool = [],

    // Leader volatile state (indexed parallel to `peers`)
    next_index: []u64 = [],
    match_index: []u64 = [],

    // Outbound message queue
    outbound: std.ArrayList(RaftOutboundMsg) = .empty,

    /// Initializes a `RaftNode` in `Follower` state with term 0.
    /// Rejects empty `peer_ids` (`EmptyPeers`) and duplicate node IDs or self in `peer_ids` (`DuplicateNode`).
    static fn init(
        alloc: std.mem.Allocator,
        node_id: u64,
        peer_ids: []const u64,
        election_timeout_ticks: u64,
        heartbeat_ticks: u64,
    ): RaftNode ! RaftError {
        if (peer_ids.len == 0) return RaftError.EmptyPeers;

        for (i of 0..peer_ids.len) {
            if (peer_ids[i] == node_id) return RaftError.DuplicateNode;
            for (j of (i + 1)..peer_ids.len) {
                if (peer_ids[i] == peer_ids[j]) return RaftError.DuplicateNode;
            }
        }

        const peers_copy = alloc.dupe(u64, peer_ids) catch return RaftError.OutOfMemory;
        errdefer alloc.free(peers_copy);

        let votes_buf = alloc.alloc(bool, peer_ids.len) catch return RaftError.OutOfMemory;
        errdefer alloc.free(votes_buf);
        for (i of 0..peer_ids.len) {
            votes_buf[i] = false;
        }

        let next_buf = alloc.alloc(u64, peer_ids.len) catch return RaftError.OutOfMemory;
        errdefer alloc.free(next_buf);
        for (i of 0..peer_ids.len) {
            next_buf[i] = 1;
        }

        let match_buf = alloc.alloc(u64, peer_ids.len) catch return RaftError.OutOfMemory;
        for (i of 0..peer_ids.len) {
            match_buf[i] = 0;
        }

        const eff_election: u64 = if (election_timeout_ticks == 0) 1 else election_timeout_ticks;
        const eff_heartbeat: u64 = if (heartbeat_ticks == 0) 1 else heartbeat_ticks;

        return RaftNode{
            alloc: alloc,
            node_id: node_id,
            peers: peers_copy,
            election_timeout_ticks: eff_election,
            heartbeat_ticks: eff_heartbeat,
            current_term: 0,
            voted_for: null,
            logs: .empty,
            role: .Follower,
            leader_id: null,
            commit_index: 0,
            last_applied: 0,
            election_elapsed: 0,
            heartbeat_elapsed: 0,
            votes_received: votes_buf,
            next_index: next_buf,
            match_index: match_buf,
            outbound: .empty,
        };
    }

    /// Releases all memory owned by the `RaftNode`, including log entries and queued outbound messages.
    fn deinit(*_) {
        for (entry of _.logs.items) {
            if (entry.command.len > 0) {
                _.alloc.free(entry.command);
            }
        }
        _.logs.deinit(_.alloc);

        for (msg of _.outbound.items) {
            freePayloadEntries(_.alloc, msg.payload);
        }
        _.outbound.deinit(_.alloc);

        if (_.peers.len > 0) _.alloc.free(_.peers);
        if (_.votes_received.len > 0) _.alloc.free(_.votes_received);
        if (_.next_index.len > 0) _.alloc.free(_.next_index);
        if (_.match_index.len > 0) _.alloc.free(_.match_index);
    }

    /// Frees a slice of `RaftOutboundMsg` returned by `drainOutbound`.
    static fn freeOutbound(alloc: std.mem.Allocator, msgs: []RaftOutboundMsg) {
        for (msg of msgs) {
            freePayloadEntries(alloc, msg.payload);
        }
        if (msgs.len > 0) {
            alloc.free(msgs);
        }
    }

    /// Returns the total cluster size (self + peers).
    fn clusterSize(): usize {
        return 1 + _.peers.len;
    }

    /// Returns the quorum majority count `(N / 2) + 1`.
    fn quorumSize(): usize {
        return (_.clusterSize() / 2) + 1;
    }

    /// Returns the 1-based index of the last log entry, or 0 if the log is empty.
    fn lastLogIndex(): u64 {
        return _.logs.items.len as u64;
    }

    /// Returns the term of the last log entry, or 0 if the log is empty.
    fn lastLogTerm(): u64 {
        if (_.logs.items.len == 0) return 0;
        return _.logs.items[_.logs.items.len - 1].term;
    }

    /// Returns the term of the log entry at 1-based `index`, or 0 if `index == 0` or out of bounds.
    fn termAt(index: u64): u64 {
        if (index == 0) return 0;
        const pos = usize(index - 1);
        if (pos >= _.logs.items.len) return 0;
        return _.logs.items[pos].term;
    }

    /// Returns the log entry at 1-based `index` if present.
    fn entryAt(index: u64): LogEntry? {
        if (index == 0) return null;
        const pos = usize(index - 1);
        if (pos >= _.logs.items.len) return null;
        return _.logs.items[pos];
    }

    /// Advances the logical clock by 1 tick.
    /// - Follower / Candidate: triggers election on `election_timeout_ticks` and queues `RequestVoteReq`.
    /// - Leader: triggers heartbeat / log replication on `heartbeat_ticks` and queues `AppendEntriesReq`.
    fn tick(*_): void ! RaftError {
        if (_.role == .Leader) {
            _.heartbeat_elapsed += 1;
            if (_.heartbeat_elapsed >= _.heartbeat_ticks) {
                _.heartbeat_elapsed = 0;
                _.broadcastAppendEntries()?;
            }
        } else {
            _.election_elapsed += 1;
            if (_.election_elapsed >= _.election_timeout_ticks) {
                _.startElection()?;
            }
        }
    }

    /// Proposes a new command on a Leader node, appending it to the local log and
    /// queuing `AppendEntriesReq` replication messages to all peers.
    /// Returns the assigned 1-based log index, or `RaftError.NotLeader` if not Leader.
    fn propose(*_, command: string): u64 ! RaftError {
        if (_.role != .Leader) return RaftError.NotLeader;

        const new_index = _.lastLogIndex() + 1;
        const owned_cmd = if (command.len > 0)
            (_.alloc.dupe(u8, command) catch return RaftError.OutOfMemory)
        else
            "";
        errdefer if (owned_cmd.len > 0) _.alloc.free(owned_cmd);

        _.logs.append(_.alloc, LogEntry{
            term: _.current_term,
            index: new_index,
            command: owned_cmd,
        }) catch return RaftError.OutOfMemory;

        _.heartbeat_elapsed = 0;
        _.broadcastAppendEntries()?;
        return new_index;
    }

    /// Handles an incoming `RequestVoteReq`, enforcing term monotonicity, `voted_for`
    /// idempotency, and Raft Section 5.4.1 log up-to-date safety.
    fn handleRequestVote(*_, req: RequestVoteReq): RequestVoteResp {
        if (req.term < _.current_term) {
            return RequestVoteResp{
                from_node: _.node_id,
                term: _.current_term,
                vote_granted: false,
            };
        }

        if (req.term > _.current_term) {
            _.stepDown(req.term);
        }

        let can_vote = true;
        if (vf := _.voted_for) {
            can_vote = (vf == req.candidate_id);
        }
        const my_last_term = _.lastLogTerm();
        const my_last_index = _.lastLogIndex();
        const log_ok = (req.last_log_term > my_last_term) ||
            (req.last_log_term == my_last_term && req.last_log_index >= my_last_index);

        if (can_vote && log_ok) {
            _.voted_for = req.candidate_id;
            _.election_elapsed = 0;
            return RequestVoteResp{
                from_node: _.node_id,
                term: _.current_term,
                vote_granted: true,
            };
        }

        return RequestVoteResp{
            from_node: _.node_id,
            term: _.current_term,
            vote_granted: false,
        };
    }

    /// Handles an incoming `RequestVoteResp`.
    /// Steps down on higher term; promotes to `Leader` upon reaching quorum in the current term.
    fn handleRequestVoteResp(*_, resp: RequestVoteResp): void ! RaftError {
        if (resp.term > _.current_term) {
            _.stepDown(resp.term);
        } else if (_.role == .Candidate && resp.term == _.current_term && resp.vote_granted) {
            if (peer_idx := _.findPeerIndex(resp.from_node)) {
                _.votes_received[peer_idx] = true;
            }

            let vote_count: usize = 1; // Self vote
            for (granted of _.votes_received) {
                if (granted) vote_count += 1;
            }

            if (vote_count >= _.quorumSize()) {
                _.becomeLeader()?;
            }
        }
    }

    /// Handles an incoming `AppendEntriesReq` (heartbeat or log replication).
    /// Validates term, checks `prev_log_index`/`prev_log_term` consistency, truncates
    /// conflicting log suffixes, appends new entries, and advances `commit_index`.
    fn handleAppendEntries(*_, req: AppendEntriesReq): AppendEntriesResp ! RaftError {
        if (req.term < _.current_term) {
            return AppendEntriesResp{
                from_node: _.node_id,
                term: _.current_term,
                success: false,
                match_index: 0,
                conflict_index: 0,
            };
        }

        if (req.term > _.current_term) {
            _.stepDown(req.term);
        } else if (_.role == .Candidate) {
            _.role = .Follower;
        }

        _.election_elapsed = 0;
        _.leader_id = req.leader_id;

        // Consistency check on prev_log_index / prev_log_term
        if (req.prev_log_index > 0) {
            const my_last = _.lastLogIndex();
            if (req.prev_log_index > my_last) {
                return AppendEntriesResp{
                    from_node: _.node_id,
                    term: _.current_term,
                    success: false,
                    match_index: 0,
                    conflict_index: my_last + 1,
                };
            }

            const prev_term = _.termAt(req.prev_log_index);
            if (prev_term != req.prev_log_term) {
                // Find the first index in the conflicting term for fast rollback
                let c_idx = req.prev_log_index;
                while (c_idx > 1 && _.termAt(c_idx - 1) == prev_term) {
                    c_idx -= 1;
                }
                return AppendEntriesResp{
                    from_node: _.node_id,
                    term: _.current_term,
                    success: false,
                    match_index: 0,
                    conflict_index: c_idx,
                };
            }
        }

        // Append new entries & truncate any conflicting suffix
        let last_new_index: u64 = req.prev_log_index;
        for (entry of req.entries) {
            last_new_index = entry.index;
            if (entry.index <= _.lastLogIndex()) {
                if (_.termAt(entry.index) != entry.term) {
                    _.truncateFrom(entry.index);
                    _.appendClonedEntry(entry)?;
                }
            } else {
                _.appendClonedEntry(entry)?;
            }
        }

        // Advance commit_index = min(leader_commit, last_new_index)
        if (req.leader_commit > _.commit_index) {
            const target = if (req.leader_commit < last_new_index)
                req.leader_commit
            else
                last_new_index;
            if (target > _.commit_index) {
                _.commit_index = target;
            }
        }

        return AppendEntriesResp{
            from_node: _.node_id,
            term: _.current_term,
            success: true,
            match_index: last_new_index,
            conflict_index: 0,
        };
    }

    /// Handles an incoming `AppendEntriesResp` from `from_peer`.
    /// Updates `match_index`/`next_index`, retries on log mismatch, and advances
    /// `commit_index` only for entries replicated to a majority in `current_term` (Figure 8).
    fn handleAppendEntriesResp(*_, from_peer: u64, resp: AppendEntriesResp): void ! RaftError {
        if (resp.term > _.current_term) {
            _.stepDown(resp.term);
        } else if (_.role == .Leader && resp.term == _.current_term) {
            if (peer_idx := _.findPeerIndex(from_peer)) {
                if (resp.success) {
                    if (resp.match_index > _.match_index[peer_idx]) {
                        _.match_index[peer_idx] = resp.match_index;
                    }
                    const next_idx = _.match_index[peer_idx] + 1;
                    if (next_idx > _.next_index[peer_idx]) {
                        _.next_index[peer_idx] = next_idx;
                    }
                    _.advanceCommitIndex();
                } else {
                    if (resp.conflict_index > 0 && resp.conflict_index < _.next_index[peer_idx]) {
                        _.next_index[peer_idx] = resp.conflict_index;
                    } else if (_.next_index[peer_idx] > 1) {
                        _.next_index[peer_idx] -= 1;
                    }
                    _.sendAppendEntriesToPeer(peer_idx)?;
                }
            }
        }
    }

    /// Extracts all newly committed entries (`last_applied < index <= commit_index`)
    /// and advances `last_applied`. Returned `LogEntry.command` slices borrow from the
    /// node's committed log; caller frees the returned slice with `alloc.free(...)`.
    fn drainApplied(*_, alloc: std.mem.Allocator): ([]LogEntry) ! RaftError {
        if (_.last_applied >= _.commit_index) {
            return alloc.alloc(LogEntry, 0) catch return RaftError.OutOfMemory;
        }

        const count = usize(_.commit_index - _.last_applied);
        let out = alloc.alloc(LogEntry, count) catch return RaftError.OutOfMemory;
        for (i of 0..count) {
            const pos = usize(_.last_applied) + usize(i);
            out[i] = _.logs.items[pos];
        }
        _.last_applied = _.commit_index;
        return out;
    }

    /// Drains all queued outbound messages into a newly allocated slice.
    /// Caller owns the returned slice and should free it via `RaftNode.freeOutbound(alloc, msgs)`.
    fn drainOutbound(*_, alloc: std.mem.Allocator): ([]RaftOutboundMsg) ! RaftError {
        const count = _.outbound.items.len;
        let out = alloc.alloc(RaftOutboundMsg, count) catch return RaftError.OutOfMemory;
        for (i of 0..count) {
            out[i] = _.outbound.items[i];
        }
        _.outbound.clearRetainingCapacity();
        return out;
    }

    // --- Internal helpers ---

    priv fn findPeerIndex(peer_id: u64): usize? {
        for (i of 0.._.peers.len) {
            if (_.peers[i] == peer_id) return usize(i);
        }
        return null;
    }

    priv fn stepDown(*_, new_term: u64) {
        _.current_term = new_term;
        _.role = .Follower;
        _.voted_for = null;
        _.leader_id = null;
        _.election_elapsed = 0;
        _.heartbeat_elapsed = 0;
        for (i of 0.._.votes_received.len) {
            _.votes_received[i] = false;
        }
    }

    priv fn startElection(*_): void ! RaftError {
        _.role = .Candidate;
        _.current_term += 1;
        _.voted_for = _.node_id;
        _.leader_id = null;
        _.election_elapsed = 0;
        for (i of 0.._.votes_received.len) {
            _.votes_received[i] = false;
        }

        const req = RequestVoteReq{
            term: _.current_term,
            candidate_id: _.node_id,
            last_log_index: _.lastLogIndex(),
            last_log_term: _.lastLogTerm(),
        };

        for (peer_id of _.peers) {
            _.outbound.append(_.alloc, RaftOutboundMsg{
                to: peer_id,
                from: _.node_id,
                payload: .RequestVote(req),
            }) catch return RaftError.OutOfMemory;
        }
    }

    priv fn becomeLeader(*_): void ! RaftError {
        _.role = .Leader;
        _.leader_id = _.node_id;
        _.heartbeat_elapsed = 0;

        const next_idx = _.lastLogIndex() + 1;
        for (i of 0.._.peers.len) {
            _.next_index[i] = next_idx;
            _.match_index[i] = 0;
        }

        _.broadcastAppendEntries()?;
    }

    priv fn broadcastAppendEntries(*_): void ! RaftError {
        for (i of 0.._.peers.len) {
            _.sendAppendEntriesToPeer(usize(i))?;
        }
    }

    priv fn sendAppendEntriesToPeer(*_, peer_idx: usize): void ! RaftError {
        const peer_id = _.peers[peer_idx];
        const next_idx = _.next_index[peer_idx];
        const prev_idx = if (next_idx > 1) next_idx - 1 else 0;
        const prev_term = _.termAt(prev_idx);

        const last_idx = _.lastLogIndex();
        const send_count: usize = if (last_idx >= next_idx) usize(last_idx - next_idx + 1) else 0;

        let entries_buf = _.alloc.alloc(LogEntry, send_count) catch return RaftError.OutOfMemory;
        let built: usize = 0;
        errdefer {
            for (k of 0..built) {
                if (entries_buf[k].command.len > 0) _.alloc.free(entries_buf[k].command);
            }
            _.alloc.free(entries_buf);
        }

        for (k of 0..send_count) {
            const src = _.logs.items[usize(prev_idx) + usize(k)];
            const cmd_copy = if (src.command.len > 0)
                (_.alloc.dupe(u8, src.command) catch return RaftError.OutOfMemory)
            else
                "";
            entries_buf[k] = LogEntry{
                term: src.term,
                index: src.index,
                command: cmd_copy,
            };
            built += 1;
        }

        const req = AppendEntriesReq{
            term: _.current_term,
            leader_id: _.node_id,
            prev_log_index: prev_idx,
            prev_log_term: prev_term,
            entries: entries_buf,
            leader_commit: _.commit_index,
        };

        _.outbound.append(_.alloc, RaftOutboundMsg{
            to: peer_id,
            from: _.node_id,
            payload: .AppendEntries(req),
        }) catch return RaftError.OutOfMemory;
    }

    priv fn truncateFrom(*_, start_index: u64) {
        if (start_index == 0) return;
        const start_pos = usize(start_index - 1);
        if (start_pos >= _.logs.items.len) return;

        let i: usize = start_pos;
        while (i < _.logs.items.len) : (i += 1) {
            if (_.logs.items[i].command.len > 0) {
                _.alloc.free(_.logs.items[i].command);
            }
        }
        _.logs.shrinkRetainingCapacity(start_pos);
    }

    priv fn appendClonedEntry(*_, entry: LogEntry): void ! RaftError {
        const cmd_copy = if (entry.command.len > 0)
            (_.alloc.dupe(u8, entry.command) catch return RaftError.OutOfMemory)
        else
            "";
        errdefer if (cmd_copy.len > 0) _.alloc.free(cmd_copy);

        _.logs.append(_.alloc, LogEntry{
            term: entry.term,
            index: entry.index,
            command: cmd_copy,
        }) catch return RaftError.OutOfMemory;
    }

    priv fn advanceCommitIndex(*_) {
        const last_idx = _.lastLogIndex();
        if (last_idx <= _.commit_index) return;

        let n = last_idx;
        while (n > _.commit_index) {
            // Figure 8 safety: only commit entries from current_term via majority count.
            if (_.termAt(n) == _.current_term) {
                let replicas: usize = 1; // Leader itself
                for (m of _.match_index) {
                    if (m >= n) replicas += 1;
                }
                if (replicas >= _.quorumSize()) {
                    _.commit_index = n;
                    break;
                }
            }
            n -= 1;
        }
    }
}

fn freePayloadEntries(alloc: std.mem.Allocator, payload: RaftPayload) {
    switch (payload) {
        .AppendEntries req => {
            for (e of req.entries) {
                if (e.command.len > 0) {
                    alloc.free(e.command);
                }
            }
            if (req.entries.len > 0) {
                alloc.free(req.entries);
            }
        },
        else => {},
    }
}
