// Source: cp/lib/raft/index.cp (Public module exports for cap:raft / cap.raft)
export {
    RaftError,
    Role,
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
    deinitActions,
} from "./types.cp";

export { RaftLog } from "./log.cp";
export { RaftConfig, RaftNode } from "./node.cp";
