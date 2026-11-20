// Source: cp/lib/raft/node.cp (Sans-IO Raft consensus state machine)
const std = @import("std");
import {
    Role,
    RaftError,
    LogEntry,
    RequestVoteReq,
    RequestVoteResp,
    AppendEntriesReq,
    AppendEntriesResp,
    PersistState,
    ApplyEntry,
    SendRequestVoteAction,
    SendAppendEntriesAction,
    RaftAction,
    VoteResult,
    AppendResult,
} from "./types.cp";
import { RaftLog } from "./log.cp";

pub struct RaftConfig {
    pub id: u64,
    pub peers: []const u64,
    pub election_timeout_min: u32 = 10,
    pub election_timeout_max: u32 = 20,
    pub heartbeat_interval: u32 = 3,
    pub seed: u64 = 1,
}

pub struct RaftNode {
    pub allocator: std.mem.Allocator,
    pub id: u64,
    pub peers: []u64,
    pub role: Role,
    pub current_term: u64,
    pub voted_for: u64?,
    pub log: RaftLog,
    pub commit_index: u64,
    pub last_applied: u64,
    pub election_elapsed: u32,
    pub heartbeat_elapsed: u32,
    pub election_timeout: u32,
    pub election_timeout_min: u32,
    pub election_timeout_max: u32,
    pub heartbeat_interval: u32,
    pub rng_state: u64,
    pub votes_received: std.ArrayList(u64),
    pub next_index: std.ArrayList(u64),
    pub match_index: std.ArrayList(u64),

    pub fn init(allocator: std.mem.Allocator, config: RaftConfig): !RaftNode {
        const peers_copy = try allocator.dupe(u64, config.peers);
        errdefer allocator.free(peers_copy);

        let next_idx = std.ArrayList(u64).empty;
        errdefer next_idx.deinit(allocator);
        let match_idx = std.ArrayList(u64).empty;
        errdefer match_idx.deinit(allocator);

        let pi: usize = 0;
        while (pi < peers_copy.len) {
            try next_idx.append(allocator, 1);
            try match_idx.append(allocator, 0);
            pi += 1;
        }

        let node = RaftNode{
            .allocator = allocator,
            .id = config.id,
            .peers = peers_copy,
            .role = Role.Follower,
            .current_term = 0,
            .voted_for = null,
            .log = RaftLog.init(allocator),
            .commit_index = 0,
            .last_applied = 0,
            .election_elapsed = 0,
            .heartbeat_elapsed = 0,
            .election_timeout = config.election_timeout_min,
            .election_timeout_min = config.election_timeout_min,
            .election_timeout_max = config.election_timeout_max,
            .heartbeat_interval = config.heartbeat_interval,
            .rng_state = if (config.seed == 0) 0x9e3779b97f4a7c15 else config.seed,
            .votes_received = std.ArrayList(u64).empty,
            .next_index = next_idx,
            .match_index = match_idx,
        };
        node.resetElectionTimeout();
        return node;
    }

    pub fn deinit(*_) {
        _.allocator.free(_.peers);
        _.log.deinit();
        _.votes_received.deinit(_.allocator);
        _.next_index.deinit(_.allocator);
        _.match_index.deinit(_.allocator);
    }

    pub fn quorum(): usize {
        return (_.peers.len + 1) / 2 + 1;
    }

    fn resetElectionTimeout(*_) {
        _.election_elapsed = 0;
        const min_t = _.election_timeout_min;
        const max_t = if (_.election_timeout_max > min_t) _.election_timeout_max else min_t;
        const span = max_t - min_t + 1;
        let x = _.rng_state;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        _.rng_state = x;
        const r: u32 = @intCast((x *% 0x2545F4914F6CDD1D) % @as(u64, span));
        _.election_timeout = min_t + r;
    }

    fn peerSlot(peer_id: u64): usize? {
        let i: usize = 0;
        for (pid of _.peers) {
            if (pid == peer_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn stepDown(*_, new_term: u64, actions: *std.ArrayList(RaftAction)): !void {
        _.current_term = new_term;
        _.role = Role.Follower;
        _.voted_for = null;
        _.votes_received.clearRetainingCapacity();
        _.resetElectionTimeout();
        try actions.append(_.allocator, RaftAction{
            .PersistState = PersistState{
                .current_term = _.current_term,
                .voted_for = _.voted_for,
            },
        });
    }

    fn isLogUpToDate(last_log_index: u64, last_log_term: u64): bool {
        const my_last_term = _.log.lastTerm();
        if (last_log_term != my_last_term) {
            return last_log_term > my_last_term;
        }
        return last_log_index >= _.log.lastIndex();
    }

    fn startElection(*_, actions: *std.ArrayList(RaftAction)): !void {
        _.role = Role.Candidate;
        _.current_term += 1;
        _.voted_for = _.id;
        _.votes_received.clearRetainingCapacity();
        try _.votes_received.append(_.allocator, _.id);
        _.resetElectionTimeout();

        try actions.append(_.allocator, RaftAction{
            .PersistState = PersistState{
                .current_term = _.current_term,
                .voted_for = _.voted_for,
            },
        });

        const req = RequestVoteReq{
            .term = _.current_term,
            .candidate_id = _.id,
            .last_log_index = _.log.lastIndex(),
            .last_log_term = _.log.lastTerm(),
        };

        for (peer_id of _.peers) {
            try actions.append(_.allocator, RaftAction{
                .SendRequestVote = SendRequestVoteAction{
                    .target_id = peer_id,
                    .req = req,
                },
            });
        }

        if (_.votes_received.items.len >= _.quorum()) {
            try _.becomeLeader(actions);
        }
    }

    fn becomeLeader(*_, actions: *std.ArrayList(RaftAction)): !void {
        _.role = Role.Leader;
        _.heartbeat_elapsed = 0;
        const next_idx = _.log.lastIndex() + 1;
        let i: usize = 0;
        while (i < _.peers.len) {
            _.next_index.items[i] = next_idx;
            _.match_index.items[i] = 0;
            i += 1;
        }
        try _.broadcastAppendEntries(actions);
    }

    fn sendAppendToPeer(*_, peer_slot: usize, actions: *std.ArrayList(RaftAction)): !void {
        const peer_id = _.peers[peer_slot];
        const next_idx = _.next_index.items[peer_slot];
        const prev_idx = if (next_idx > 0) next_idx - 1 else 0;
        const prev_term = _.log.termAt(prev_idx) orelse _.log.snapshot_term;
        const entries = try _.log.sliceFrom(_.allocator, next_idx);
        errdefer {
            for (e of entries) {
                _.allocator.free(e.command);
            }
            _.allocator.free(entries);
        }

        try actions.append(_.allocator, RaftAction{
            .SendAppendEntries = SendAppendEntriesAction{
                .target_id = peer_id,
                .term = _.current_term,
                .leader_id = _.id,
                .prev_log_index = prev_idx,
                .prev_log_term = prev_term,
                .entries = entries,
                .leader_commit = _.commit_index,
            },
        });
    }

    fn broadcastAppendEntries(*_, actions: *std.ArrayList(RaftAction)): !void {
        let i: usize = 0;
        while (i < _.peers.len) {
            try _.sendAppendToPeer(i, actions);
            i += 1;
        }
    }

    fn applyCommitted(*_, actions: *std.ArrayList(RaftAction)): !void {
        while (_.last_applied < _.commit_index) {
            const next_apply = _.last_applied + 1;
            if (entry := _.log.entryAt(next_apply)) {
                const cmd_copy = try _.allocator.dupe(u8, entry.command);
                errdefer _.allocator.free(cmd_copy);
                try actions.append(_.allocator, RaftAction{
                    .ApplyEntry = ApplyEntry{
                        .index = entry.index,
                        .term = entry.term,
                        .command = cmd_copy,
                    },
                });
            }
            _.last_applied = next_apply;
        }
    }

    fn advanceCommitIndex(*_, actions: *std.ArrayList(RaftAction)): !void {
        const last_idx = _.log.lastIndex();
        if (last_idx <= _.commit_index) {
            return;
        }
        let n = last_idx;
        while (n > _.commit_index) {
            // Raft §5.4.2: Leader only commits entries from its current term by counting replicas.
            if (t := _.log.termAt(n)) {
                if (t == _.current_term) {
                    let count: usize = 1;
                    for (m of _.match_index.items) {
                        if (m >= n) {
                            count += 1;
                        }
                    }
                    if (count >= _.quorum()) {
                        _.commit_index = n;
                        try _.applyCommitted(actions);
                        break;
                    }
                }
            }
            n -= 1;
        }
    }

    pub fn tick(*_): ![]RaftAction {
        let actions = std.ArrayList(RaftAction).empty;
        errdefer {
            for (a of actions.items) {
                a.deinit(_.allocator);
            }
            actions.deinit(_.allocator);
        }

        switch (_.role) {
            .Leader => {
                _.heartbeat_elapsed += 1;
                if (_.heartbeat_elapsed >= _.heartbeat_interval) {
                    _.heartbeat_elapsed = 0;
                    try _.broadcastAppendEntries(&actions);
                }
            },
            .Follower, .Candidate => {
                _.election_elapsed += 1;
                if (_.election_elapsed >= _.election_timeout) {
                    try _.startElection(&actions);
                }
            },
        }
        return try actions.toOwnedSlice(_.allocator);
    }

    pub fn propose(*_, command: []const u8): ![]RaftAction {
        if (_.role != Role.Leader) {
            return RaftError.NotLeader;
        }
        let actions = std.ArrayList(RaftAction).empty;
        errdefer {
            for (a of actions.items) {
                a.deinit(_.allocator);
            }
            actions.deinit(_.allocator);
        }

        * = try _.log.append(_.current_term, command);
        try _.advanceCommitIndex(&actions);
        try _.broadcastAppendEntries(&actions);
        return try actions.toOwnedSlice(_.allocator);
    }

    pub fn handleRequestVote(*_, req: RequestVoteReq): !VoteResult {
        let actions = std.ArrayList(RaftAction).empty;
        errdefer {
            for (a of actions.items) {
                a.deinit(_.allocator);
            }
            actions.deinit(_.allocator);
        }

        if (req.term > _.current_term) {
            try _.stepDown(req.term, &actions);
        }

        let granted = false;
        if (req.term >= _.current_term) {
            const can_vote = (_.voted_for == null) or (_.voted_for.? == req.candidate_id);
            if (can_vote and _.isLogUpToDate(req.last_log_index, req.last_log_term)) {
                granted = true;
                _.voted_for = req.candidate_id;
                _.resetElectionTimeout();
                try actions.append(_.allocator, RaftAction{
                    .PersistState = PersistState{
                        .current_term = _.current_term,
                        .voted_for = _.voted_for,
                    },
                });
            }
        }

        return VoteResult{
            .resp = RequestVoteResp{
                .term = _.current_term,
                .voter_id = _.id,
                .vote_granted = granted,
            },
            .actions = try actions.toOwnedSlice(_.allocator),
        };
    }

    pub fn handleRequestVoteResp(*_, resp: RequestVoteResp): ![]RaftAction {
        let actions = std.ArrayList(RaftAction).empty;
        errdefer {
            for (a of actions.items) {
                a.deinit(_.allocator);
            }
            actions.deinit(_.allocator);
        }

        if (resp.term > _.current_term) {
            try _.stepDown(resp.term, &actions);
            return try actions.toOwnedSlice(_.allocator);
        }

        if (_.role != Role.Candidate or resp.term != _.current_term) {
            return try actions.toOwnedSlice(_.allocator);
        }

        if (resp.vote_granted) {
            let seen = false;
            for (v of _.votes_received.items) {
                if (v == resp.voter_id) {
                    seen = true;
                    break;
                }
            }
            if (!seen) {
                try _.votes_received.append(_.allocator, resp.voter_id);
            }
            if (_.votes_received.items.len >= _.quorum()) {
                try _.becomeLeader(&actions);
            }
        }

        return try actions.toOwnedSlice(_.allocator);
    }

    pub fn handleAppendEntries(*_, req: AppendEntriesReq): !AppendResult {
        let actions = std.ArrayList(RaftAction).empty;
        errdefer {
            for (a of actions.items) {
                a.deinit(_.allocator);
            }
            actions.deinit(_.allocator);
        }

        if (req.term < _.current_term) {
            return AppendResult{
                .resp = AppendEntriesResp{
                    .term = _.current_term,
                    .follower_id = _.id,
                    .success = false,
                    .match_index = 0,
                    .conflict_index = 0,
                    .conflict_term = 0,
                },
                .actions = try actions.toOwnedSlice(_.allocator),
            };
        }

        if (req.term > _.current_term) {
            try _.stepDown(req.term, &actions);
        } else if (_.role == Role.Candidate) {
            _.role = Role.Follower;
            _.votes_received.clearRetainingCapacity();
        }
        _.resetElectionTimeout();

        if (!_.log.matchTerm(req.prev_log_index, req.prev_log_term)) {
            let conflict_term: u64 = 0;
            let conflict_index: u64 = _.log.lastIndex() + 1;
            if (ct := _.log.termAt(req.prev_log_index)) {
                conflict_term = ct;
                let idx = req.prev_log_index;
                while (idx > _.log.snapshot_index + 1) {
                    if (pt := _.log.termAt(idx - 1)) {
                        if (pt != conflict_term) {
                            break;
                        }
                    } else {
                        break;
                    }
                    idx -= 1;
                }
                conflict_index = idx;
            }
            return AppendResult{
                .resp = AppendEntriesResp{
                    .term = _.current_term,
                    .follower_id = _.id,
                    .success = false,
                    .match_index = 0,
                    .conflict_index = conflict_index,
                    .conflict_term = conflict_term,
                },
                .actions = try actions.toOwnedSlice(_.allocator),
            };
        }

        for (entry of req.entries) {
            if (entry.index <= _.log.snapshot_index) {
                continue;
            }
            if (existing_term := _.log.termAt(entry.index)) {
                if (existing_term != entry.term) {
                    _.log.truncateFrom(entry.index);
                    * = try _.log.append(entry.term, entry.command);
                }
            } else {
                * = try _.log.append(entry.term, entry.command);
            }
        }

        const last_new_index = req.prev_log_index + @as(u64, @intCast(req.entries.len));
        if (req.leader_commit > _.commit_index) {
            const max_commit = if (req.leader_commit < last_new_index) req.leader_commit else last_new_index;
            if (max_commit > _.commit_index) {
                _.commit_index = max_commit;
                try _.applyCommitted(&actions);
            }
        }

        return AppendResult{
            .resp = AppendEntriesResp{
                .term = _.current_term,
                .follower_id = _.id,
                .success = true,
                .match_index = last_new_index,
                .conflict_index = 0,
                .conflict_term = 0,
            },
            .actions = try actions.toOwnedSlice(_.allocator),
        };
    }

    pub fn handleAppendEntriesResp(*_, resp: AppendEntriesResp): ![]RaftAction {
        let actions = std.ArrayList(RaftAction).empty;
        errdefer {
            for (a of actions.items) {
                a.deinit(_.allocator);
            }
            actions.deinit(_.allocator);
        }

        if (resp.term > _.current_term) {
            try _.stepDown(resp.term, &actions);
            return try actions.toOwnedSlice(_.allocator);
        }

        if (_.role != Role.Leader or resp.term != _.current_term) {
            return try actions.toOwnedSlice(_.allocator);
        }

        const slot = _.peerSlot(resp.follower_id) orelse {
            return try actions.toOwnedSlice(_.allocator);
        };

        if (resp.success) {
            if (resp.match_index > _.match_index.items[slot]) {
                _.match_index.items[slot] = resp.match_index;
            }
            const next_val = resp.match_index + 1;
            if (next_val > _.next_index.items[slot]) {
                _.next_index.items[slot] = next_val;
            }
            try _.advanceCommitIndex(&actions);
        } else {
            let new_next: u64 = if (_.next_index.items[slot] > 1) _.next_index.items[slot] - 1 else 1;
            if (resp.conflict_term != 0) {
                let found_idx: u64? = null;
                let idx = _.log.lastIndex();
                while (idx > _.log.snapshot_index) {
                    if (t := _.log.termAt(idx)) {
                        if (t == resp.conflict_term) {
                            found_idx = idx;
                            break;
                        }
                    }
                    idx -= 1;
                }
                if (fi := found_idx) {
                    new_next = fi + 1;
                } else if (resp.conflict_index > 0) {
                    new_next = resp.conflict_index;
                }
            } else if (resp.conflict_index > 0) {
                new_next = resp.conflict_index;
            }
            if (new_next < _.log.snapshot_index + 1) {
                new_next = _.log.snapshot_index + 1;
            }
            _.next_index.items[slot] = new_next;
            try _.sendAppendToPeer(slot, &actions);
        }

        return try actions.toOwnedSlice(_.allocator);
    }
}
