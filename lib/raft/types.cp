// Source: cp/lib/raft/types.cp (Sans-IO Raft core types, RPC messages, and action model)
const std = @import("std");

pub const RaftError = error{
    NotLeader,
    InvalidIndex,
    CompactedIndex,
    TermMismatch,
    OutOfMemory,
};

pub enum Role {
    Follower,
    Candidate,
    Leader,
}

pub struct LogEntry {
    pub term: u64,
    pub index: u64,
    pub command: []const u8,
}

pub struct RequestVoteReq {
    pub term: u64,
    pub candidate_id: u64,
    pub last_log_index: u64,
    pub last_log_term: u64,
}

pub struct RequestVoteResp {
    pub term: u64,
    pub voter_id: u64,
    pub vote_granted: bool,
}

pub struct AppendEntriesReq {
    pub term: u64,
    pub leader_id: u64,
    pub prev_log_index: u64,
    pub prev_log_term: u64,
    pub entries: []const LogEntry,
    pub leader_commit: u64,
}

pub struct AppendEntriesResp {
    pub term: u64,
    pub follower_id: u64,
    pub success: bool,
    pub match_index: u64,
    pub conflict_index: u64,
    pub conflict_term: u64,
}

pub struct PersistState {
    pub current_term: u64,
    pub voted_for: u64?,
}

pub struct ApplyEntry {
    pub index: u64,
    pub term: u64,
    pub command: []const u8,
}

pub struct SendRequestVoteAction {
    pub target_id: u64,
    pub req: RequestVoteReq,
}

pub struct SendAppendEntriesAction {
    pub target_id: u64,
    pub term: u64,
    pub leader_id: u64,
    pub prev_log_index: u64,
    pub prev_log_term: u64,
    pub entries: []LogEntry,
    pub leader_commit: u64,

    pub fn asReq(): AppendEntriesReq {
        return AppendEntriesReq{
            .term = _.term,
            .leader_id = _.leader_id,
            .prev_log_index = _.prev_log_index,
            .prev_log_term = _.prev_log_term,
            .entries = _.entries,
            .leader_commit = _.leader_commit,
        };
    }
}

pub union(enum) RaftAction {
    SendRequestVote: SendRequestVoteAction,
    SendAppendEntries: SendAppendEntriesAction,
    PersistState: PersistState,
    ApplyEntry: ApplyEntry,

    pub fn deinit(allocator: std.mem.Allocator) {
        switch (_.*) {
            .SendAppendEntries => |act| {
                for (entry of act.entries) {
                    allocator.free(entry.command);
                }
                allocator.free(act.entries);
            },
            .ApplyEntry => |act| {
                allocator.free(act.command);
            },
            else => {},
        }
    }
}

pub struct VoteResult {
    pub resp: RequestVoteResp,
    pub actions: []RaftAction,

    pub fn deinit(allocator: std.mem.Allocator) {
        deinitActions(allocator, _.actions);
    }
}

pub struct AppendResult {
    pub resp: AppendEntriesResp,
    pub actions: []RaftAction,

    pub fn deinit(allocator: std.mem.Allocator) {
        deinitActions(allocator, _.actions);
    }
}

pub fn deinitActions(allocator: std.mem.Allocator, actions: []RaftAction) {
    for (act of actions) {
        act.deinit(allocator);
    }
    allocator.free(actions);
}
