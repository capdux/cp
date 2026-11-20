// Source: cp/cap/test/test_raft_production_deep.cp
// Deep Production Verification Suite for cap:raft & cap.raft
// Covers:
//   1. 3-node in-memory cluster simulator: timeout election, Quorum single Leader, NotLeader rejection
//   2. Normal log replication, majority commitIndex advancement & drainApplied 3-node consistency
//   3. Log up-to-date safety check (RequestVote rejects stale candidate) & Follower conflict truncation
//   4. Network partition split-brain simulation & higher-Term automatic step-down (no dual-leader commit)
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.raft`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RaftError,
    RaftRole,
    LogEntry,
    RequestVoteReq,
    RequestVoteResp,
    AppendEntriesReq,
    AppendEntriesResp,
    RaftOutboundMsg,
    RaftNode,
} from "../../lib/raft/index.cp";

struct Cluster3 {
    alloc: std.mem.Allocator = undefined,
    n1: RaftNode = undefined,
    n2: RaftNode = undefined,
    n3: RaftNode = undefined,
    // Partition flags: if blocked[i][j] is true, messages from node (i+1) to (j+1) are dropped.
    blocked: [3][3]bool = [[false; 3]; 3],

    static fn init(alloc: std.mem.Allocator): Cluster3! {
        const p1: [2]u64 = [2, 3];
        const p2: [2]u64 = [1, 3];
        const p3: [2]u64 = [1, 2];
        // Staggered election timeouts so node 1 wins deterministically first
        let n1 = RaftNode.init(alloc, 1, &p1, 5, 2)?;
        errdefer n1.deinit();
        let n2 = RaftNode.init(alloc, 2, &p2, 9, 2)?;
        errdefer n2.deinit();
        let n3 = RaftNode.init(alloc, 3, &p3, 12, 2)?;
        return Cluster3{
            alloc: alloc,
            n1: n1,
            n2: n2,
            n3: n3,
            blocked: [[false; 3]; 3],
        };
    }

    fn deinit(*_) {
        _.n1.deinit();
        _.n2.deinit();
        _.n3.deinit();
    }

    fn nodePtr(*_, id: u64): *RaftNode {
        if (id == 1) return *_.n1;
        if (id == 2) return *_.n2;
        return *_.n3;
    }

    fn isBlocked(src_id: u64, dst_id: u64): bool {
        return _.blocked[usize(src_id - 1)][usize(dst_id - 1)];
    }

    fn isolateNode(*_, id: u64) {
        const idx = usize(id - 1);
        for (i of 0..3) {
            if (i != idx) {
                _.blocked[idx][i] = true;
                _.blocked[i][idx] = true;
            }
        }
    }

    fn healAll(*_) {
        for (i of 0..3) {
            for (j of 0..3) {
                _.blocked[i][j] = false;
            }
        }
    }

    fn tickAll(*_)! {
        _.n1.tick()?;
        _.n2.tick()?;
        _.n3.tick()?;
    }

    /// Delivers all currently queued outbound messages across the 3 nodes (up to `max_rounds` hops).
    fn pump(*_, max_rounds: usize)! {
        for (r of 0..max_rounds) {
            * = r;
            let delivered = false;
            for (src_id of 1..=3) {
                const msgs = _.nodePtr(src_id as u64).drainOutbound(_.alloc)?;
                defer RaftNode.freeOutbound(_.alloc, msgs);
                if (msgs.len > 0) delivered = true;

                for (msg of msgs) {
                    if (_.isBlocked(msg.from, msg.to)) continue;
                    switch (msg.payload) {
                        .RequestVote req => {
                            const resp = _.nodePtr(msg.to).handleRequestVote(req);
                            if (!_.isBlocked(msg.to, msg.from)) {
                                _.nodePtr(msg.from).handleRequestVoteResp(resp)?;
                            }
                        },
                        .RequestVoteResp resp => {
                            _.nodePtr(msg.to).handleRequestVoteResp(resp)?;
                        },
                        .AppendEntries req => {
                            const resp = _.nodePtr(msg.to).handleAppendEntries(req)?;
                            if (!_.isBlocked(msg.to, msg.from)) {
                                _.nodePtr(msg.from).handleAppendEntriesResp(msg.to, resp)?;
                            }
                        },
                        .AppendEntriesResp resp => {
                            _.nodePtr(msg.to).handleAppendEntriesResp(msg.from, resp)?;
                        },
                    }
                }
            }
            if (!delivered) break;
        }
    }
}

fn testClusterElectionAndNotLeader()! {
    const alloc = std.heap.page_allocator;

    // Validate init error guards: EmptyPeers & DuplicateNode
    const empty_peers: []const u64 = [];
    let got_empty = false;
    * = RaftNode.init(alloc, 1, empty_peers, 5, 2) catch e => {
        if (e == RaftError.EmptyPeers) got_empty = true;
    };
    expect_eq(got_empty, true);

    const dup_self: [2]u64 = [1, 2];
    let got_dup = false;
    * = RaftNode.init(alloc, 1, &dup_self, 5, 2) catch e => {
        if (e == RaftError.DuplicateNode) got_dup = true;
    };
    expect_eq(got_dup, true);

    let cluster = Cluster3.init(alloc)?;
    defer cluster.deinit();

    // Before election, all nodes are Followers and reject propose() with NotLeader
    expect_eq(cluster.n1.role == .Follower, true);
    expect_eq(cluster.n2.role == .Follower, true);
    expect_eq(cluster.n3.role == .Follower, true);

    let got_not_leader = false;
    * = cluster.n1.propose("set x=1") catch e => {
        if (e == RaftError.NotLeader) got_not_leader = true;
    };
    expect_eq(got_not_leader, true);

    // Advance 5 ticks -> node 1 times out, wins Quorum (nodes 2 & 3 vote for 1)
    for (t of 0..5) {
        * = t;
        cluster.tickAll()?;
        cluster.pump(4)?;
    }

    expect_eq(cluster.n1.role == .Leader, true);
    expect_eq(cluster.n2.role == .Follower, true);
    expect_eq(cluster.n3.role == .Follower, true);
    expect_eq(cluster.n1.current_term, 1);
    expect_eq(cluster.n2.leader_id.?, 1);
    expect_eq(cluster.n3.leader_id.?, 1);

    // Non-leader still rejects propose()
    let follower_rejected = false;
    * = cluster.n2.propose("set x=2") catch e => {
        if (e == RaftError.NotLeader) follower_rejected = true;
    };
    expect_eq(follower_rejected, true);
}

fn testLogReplicationCommitAndDrainApplied()! {
    const alloc = std.heap.page_allocator;
    let cluster = Cluster3.init(alloc)?;
    defer cluster.deinit();

    // Elect node 1
    for (t of 0..5) {
        * = t;
        cluster.tickAll()?;
        cluster.pump(4)?;
    }
    expect_eq(cluster.n1.role == .Leader, true);

    // Propose 3 commands on Leader
    const idx1 = cluster.n1.propose("PUT k1=v1")?;
    const idx2 = cluster.n1.propose("PUT k2=v2")?;
    const idx3 = cluster.n1.propose("DEL k1")?;
    expect_eq(idx1, 1);
    expect_eq(idx2, 2);
    expect_eq(idx3, 3);

    // Pump replication + send one heartbeat round so Followers receive updated leader_commit
    cluster.pump(6)?;
    cluster.tickAll()?;
    cluster.tickAll()?;
    cluster.pump(6)?;

    expect_eq(cluster.n1.commit_index, 3);
    expect_eq(cluster.n2.commit_index, 3);
    expect_eq(cluster.n3.commit_index, 3);

    // Drain applied entries on all 3 nodes and verify strong consistency
    const a1 = cluster.n1.drainApplied(alloc)?;
    defer alloc.free(a1);
    const a2 = cluster.n2.drainApplied(alloc)?;
    defer alloc.free(a2);
    const a3 = cluster.n3.drainApplied(alloc)?;
    defer alloc.free(a3);

    expect_eq(a1.len, 3);
    expect_eq(a2.len, 3);
    expect_eq(a3.len, 3);

    for (i of 0..3) {
        expect_eq(a1[i].index, a2[i].index);
        expect_eq(a1[i].index, a3[i].index);
        expect_eq(a1[i].term, a2[i].term);
        expect_eq(std.mem.eql(u8, a1[i].command, a2[i].command), true);
        expect_eq(std.mem.eql(u8, a1[i].command, a3[i].command), true);
    }
    expect_eq(std.mem.eql(u8, a1[0].command, "PUT k1=v1"), true);
    expect_eq(std.mem.eql(u8, a1[2].command, "DEL k1"), true);

    // Second drainApplied returns empty slice
    const empty_a = cluster.n1.drainApplied(alloc)?;
    defer alloc.free(empty_a);
    expect_eq(empty_a.len, 0);
}

fn testLogUpToDateSafetyAndConflictTruncation()! {
    const alloc = std.heap.page_allocator;
    const peers2: [2]u64 = [1, 3];
    let follower = RaftNode.init(alloc, 2, &peers2, 10, 2)?;
    defer follower.deinit();

    // Seed follower with: idx 1 (term 1, "a"), idx 2 (term 1, "b"), idx 3 (term 2, "stale1")
    const seed_entries: [3]LogEntry = [
        LogEntry{ term: 1, index: 1, command: "a" },
        LogEntry{ term: 1, index: 2, command: "b" },
        LogEntry{ term: 2, index: 3, command: "stale1" },
    ];
    const seed_resp = follower.handleAppendEntries(AppendEntriesReq{
        term: 2,
        leader_id: 1,
        prev_log_index: 0,
        prev_log_term: 0,
        entries: &seed_entries,
        leader_commit: 1,
    })?;
    expect_eq(seed_resp.success, true);
    expect_eq(follower.lastLogIndex(), 3);
    expect_eq(follower.lastLogTerm(), 2);

    // 1. RequestVote safety: reject candidate with lower last_log_term even if longer log
    const stale_term_vote = follower.handleRequestVote(RequestVoteReq{
        term: 5,
        candidate_id: 3,
        last_log_index: 10,
        last_log_term: 1,
    });
    expect_eq(stale_term_vote.vote_granted, false);
    // Term still bumped to 5 per Raft rules
    expect_eq(follower.current_term, 5);

    // 2. RequestVote safety: reject candidate with same last_log_term=2 but shorter index=2 < 3
    const stale_idx_vote = follower.handleRequestVote(RequestVoteReq{
        term: 6,
        candidate_id: 3,
        last_log_index: 2,
        last_log_term: 2,
    });
    expect_eq(stale_idx_vote.vote_granted, false);

    // 3. RequestVote safety: grant vote to candidate with up-to-date log (term=2, index=3)
    const ok_vote = follower.handleRequestVote(RequestVoteReq{
        term: 7,
        candidate_id: 1,
        last_log_index: 3,
        last_log_term: 2,
    });
    expect_eq(ok_vote.vote_granted, true);

    // 4. Conflict truncation: new leader in term 7 has idx 2 (term 1), idx 3 (term 7, "new3"), idx 4 (term 7, "new4")
    const overwrite_entries: [2]LogEntry = [
        LogEntry{ term: 7, index: 3, command: "new3" },
        LogEntry{ term: 7, index: 4, command: "new4" },
    ];
    const ae_resp = follower.handleAppendEntries(AppendEntriesReq{
        term: 7,
        leader_id: 1,
        prev_log_index: 2,
        prev_log_term: 1,
        entries: &overwrite_entries,
        leader_commit: 4,
    })?;
    expect_eq(ae_resp.success, true);
    expect_eq(follower.lastLogIndex(), 4);
    expect_eq(follower.termAt(3), 7);
    expect_eq(follower.commit_index, 4);

    const applied = follower.drainApplied(alloc)?;
    defer alloc.free(applied);
    expect_eq(applied.len, 4);
    expect_eq(std.mem.eql(u8, applied[2].command, "new3"), true);
    expect_eq(std.mem.eql(u8, applied[3].command, "new4"), true);
}

fn testPartitionSplitBrainAndStepDown()! {
    const alloc = std.heap.page_allocator;
    let cluster = Cluster3.init(alloc)?;
    defer cluster.deinit();

    // 1. Elect node 1 in term 1
    for (t of 0..5) {
        * = t;
        cluster.tickAll()?;
        cluster.pump(4)?;
    }
    expect_eq(cluster.n1.role == .Leader, true);
    expect_eq(cluster.n1.current_term, 1);

    // 2. Partition node 1 (old Leader) away from {2, 3}
    cluster.isolateNode(1);

    // Old Leader proposes a write while isolated — must NOT commit because it lacks Quorum!
    const stale_idx = cluster.n1.propose("stale_write_on_minority")?;
    expect_eq(stale_idx, 1);

    // Run enough ticks for majority partition {2, 3} to elect node 2 in term 2
    for (t of 0..12) {
        * = t;
        cluster.tickAll()?;
        cluster.pump(4)?;
    }

    // Old leader 1 still cannot commit its minority write
    expect_eq(cluster.n1.commit_index, 0);
    // Majority partition elected node 2 as Leader in term >= 2
    expect_eq(cluster.n2.role == .Leader, true);
    expect_eq(cluster.n2.current_term >= 2, true);

    const maj_term = cluster.n2.current_term;
    const maj_idx = cluster.n2.propose("committed_on_majority")?;
    expect_eq(maj_idx, 1);
    cluster.pump(6)?;
    cluster.tickAll()?;
    cluster.tickAll()?;
    cluster.pump(6)?;
    expect_eq(cluster.n2.commit_index, 1);
    expect_eq(cluster.n3.commit_index, 1);

    // 3. Heal partition: old Leader 1 receives AppendEntries from higher term and steps down to Follower
    cluster.healAll();
    cluster.tickAll()?;
    cluster.tickAll()?;
    cluster.pump(8)?;

    expect_eq(cluster.n1.role == .Follower, true);
    expect_eq(cluster.n1.current_term, maj_term);
    expect_eq(cluster.n1.commit_index, 1);

    const n1_applied = cluster.n1.drainApplied(alloc)?;
    defer alloc.free(n1_applied);
    expect_eq(n1_applied.len, 1);
    expect_eq(std.mem.eql(u8, n1_applied[0].command, "committed_on_majority"), true);
}

fn testCapDynamicRaftMount()! {
    const alloc = std.heap.page_allocator;
    const peers: [2]u64 = [20, 30];
    let node = cap.raft.RaftNode.init(alloc, 10, &peers, 4, 1)?;
    defer node.deinit();

    const role: cap.raft.RaftRole = node.role;
    expect_eq(role == .Follower, true);

    for (t of 0..4) {
        * = t;
        node.tick()?;
    }
    expect_eq(node.role == .Candidate, true);
    expect_eq(node.current_term, 1);

    const out = node.drainOutbound(alloc)?;
    defer cap.raft.RaftNode.freeOutbound(alloc, out);
    expect_eq(out.len, 2);
}

pub fn main()! {
    test("1. Raft 3-Node Election, Quorum & NotLeader Rejection", () => {
        testClusterElectionAndNotLeader() catch unreachable;
    });
    test("2. Raft Log Replication, Majority Commit & drainApplied Consistency", () => {
        testLogReplicationCommitAndDrainApplied() catch unreachable;
    });
    test("3. Raft RequestVote Up-To-Date Check & Conflict Truncation", () => {
        testLogUpToDateSafetyAndConflictTruncation() catch unreachable;
    });
    test("4. Raft Partition Split-Brain Prevention & Higher-Term Step-Down", () => {
        testPartitionSplitBrainAndStepDown() catch unreachable;
    });
    test("5. import cap from \"cap\"; cap.raft Dynamic Lazy Mounting", () => {
        testCapDynamicRaftMount() catch unreachable;
    });
}
