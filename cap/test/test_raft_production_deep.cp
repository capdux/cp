// Source: cp/cap/test/test_raft_production_deep.cp (Production deep tests for cap:raft)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    RaftNode,
    RaftConfig,
    RaftLog,
    RaftAction,
    Role,
    RaftError,
    RequestVoteReq,
    AppendEntriesReq,
    LogEntry,
    deinitActions,
} from "cap:raft";

test("raft 3-node election and higher term step-down") {
    const alloc = std.testing.allocator;
    const p1: [2]u64 = [ 2, 3 ];
    const p2: [2]u64 = [ 1, 3 ];
    const p3: [2]u64 = [ 1, 2 ];

    let n1 = try RaftNode.init(alloc, RaftConfig{
        .id = 1,
        .peers = &p1,
        .election_timeout_min = 3,
        .election_timeout_max = 3,
        .heartbeat_interval = 1,
        .seed = 11,
    });
    defer n1.deinit();

    let n2 = try RaftNode.init(alloc, RaftConfig{
        .id = 2,
        .peers = &p2,
        .election_timeout_min = 10,
        .election_timeout_max = 10,
        .seed = 22,
    });
    defer n2.deinit();

    let n3 = try RaftNode.init(alloc, RaftConfig{
        .id = 3,
        .peers = &p3,
        .election_timeout_min = 10,
        .election_timeout_max = 10,
        .seed = 33,
    });
    defer n3.deinit();

    // Advance n1 until election triggers
    let tick_actions: []RaftAction = try alloc.alloc(RaftAction, 0);
    let step: u32 = 0;
    while (step < 3) {
        deinitActions(alloc, tick_actions);
        tick_actions = try n1.tick();
        step += 1;
    }
    defer deinitActions(alloc, tick_actions);

    try expect_eq(n1.role == Role.Candidate, true);
    try expect_eq(n1.current_term, @as(u64, 1));

    // Deliver RequestVote to n2 and n3
    for (act of tick_actions) {
        switch (act) {
            .SendRequestVote => |sv| {
                if (sv.target_id == 2) {
                    let vr = try n2.handleRequestVote(sv.req);
                    defer vr.deinit(alloc);
                    try expect_eq(vr.resp.vote_granted, true);
                    const resp_acts = try n1.handleRequestVoteResp(vr.resp);
                    deinitActions(alloc, resp_acts);
                } else if (sv.target_id == 3) {
                    let vr = try n3.handleRequestVote(sv.req);
                    defer vr.deinit(alloc);
                    try expect_eq(vr.resp.vote_granted, true);
                    const resp_acts = try n1.handleRequestVoteResp(vr.resp);
                    deinitActions(alloc, resp_acts);
                }
            },
            else => {},
        }
    }

    try expect_eq(n1.role == Role.Leader, true);

    // Higher term RequestVote forces n1 to step down to Follower and clear voted_for
    let higher_vr = try n1.handleRequestVote(RequestVoteReq{
        .term = 5,
        .candidate_id = 2,
        .last_log_index = 0,
        .last_log_term = 0,
    });
    defer higher_vr.deinit(alloc);

    try expect_eq(n1.role == Role.Follower, true);
    try expect_eq(n1.current_term, @as(u64, 5));
    try expect_eq(higher_vr.resp.vote_granted, true);
    try expect_eq(n1.voted_for.?, @as(u64, 2));
}

test("raft multi-node log replication commit and ApplyEntry") {
    const alloc = std.testing.allocator;
    const p1: [2]u64 = [ 2, 3 ];
    const p2: [2]u64 = [ 1, 3 ];
    const p3: [2]u64 = [ 1, 2 ];

    let n1 = try RaftNode.init(alloc, RaftConfig{ .id = 1, .peers = &p1, .election_timeout_min = 2, .election_timeout_max = 2 });
    defer n1.deinit();
    let n2 = try RaftNode.init(alloc, RaftConfig{ .id = 2, .peers = &p2, .election_timeout_min = 20, .election_timeout_max = 20 });
    defer n2.deinit();
    let n3 = try RaftNode.init(alloc, RaftConfig{ .id = 3, .peers = &p3, .election_timeout_min = 20, .election_timeout_max = 20 });
    defer n3.deinit();

    // Elect n1
    deinitActions(alloc, try n1.tick());
    const elect_acts = try n1.tick();
    defer deinitActions(alloc, elect_acts);
    for (act of elect_acts) {
        switch (act) {
            .SendRequestVote => |sv| {
                if (sv.target_id == 2) {
                    let vr = try n2.handleRequestVote(sv.req);
                    defer vr.deinit(alloc);
                    deinitActions(alloc, try n1.handleRequestVoteResp(vr.resp));
                }
            },
            else => {},
        }
    }
    try expect_eq(n1.role == Role.Leader, true);

    // Propose two commands on Leader
    const prop1 = try n1.propose("set x=10");
    defer deinitActions(alloc, prop1);
    const prop2 = try n1.propose("set y=20");
    defer deinitActions(alloc, prop2);

    let applied_on_leader: usize = 0;
    for (act of prop2) {
        switch (act) {
            .SendAppendEntries => |sa| {
                if (sa.target_id == 2) {
                    let ar = try n2.handleAppendEntries(sa.asReq());
                    defer ar.deinit(alloc);
                    try expect_eq(ar.resp.success, true);
                    const resp_acts = try n1.handleAppendEntriesResp(ar.resp);
                    defer deinitActions(alloc, resp_acts);
                    for (ra of resp_acts) {
                        switch (ra) {
                            .ApplyEntry => applied_on_leader += 1,
                            else => {},
                        }
                    }
                }
            },
            else => {},
        }
    }

    try expect_eq(n1.commit_index, @as(u64, 2));
    try expect_eq(n1.last_applied, @as(u64, 2));
    try expect_eq(applied_on_leader, @as(usize, 2));

    // Heartbeat propagates leader_commit=2 to n2 and triggers ApplyEntry on follower
    const hb_acts = try n1.propose("set z=30");
    defer deinitActions(alloc, hb_acts);
    let applied_on_follower: usize = 0;
    for (act of hb_acts) {
        switch (act) {
            .SendAppendEntries => |sa| {
                if (sa.target_id == 2) {
                    let ar = try n2.handleAppendEntries(sa.asReq());
                    defer ar.deinit(alloc);
                    for (fa of ar.actions) {
                        switch (fa) {
                            .ApplyEntry => applied_on_follower += 1,
                            else => {},
                        }
                    }
                }
            },
            else => {},
        }
    }
    try expect_eq(n2.commit_index, @as(u64, 2));
    try expect_eq(applied_on_follower, @as(usize, 2));
}

test("raft conflict truncation recovery and 5.4.2 current-term commit rule") {
    const alloc = std.testing.allocator;
    const p1: [2]u64 = [ 2, 3 ];
    const p2: [2]u64 = [ 1, 3 ];

    let leader = try RaftNode.init(alloc, RaftConfig{ .id = 1, .peers = &p1 });
    defer leader.deinit();
    let follower = try RaftNode.init(alloc, RaftConfig{ .id = 2, .peers = &p2 });
    defer follower.deinit();

    // Simulate old term 1 entry on both, plus divergent term 2 entries on follower
    * = try leader.log.append(1, "old_1");
    * = try follower.log.append(1, "old_1");
    * = try follower.log.append(2, "stale_2a");
    * = try follower.log.append(2, "stale_2b");

    // Leader becomes leader in term 3
    leader.current_term = 3;
    leader.role = Role.Leader;
    leader.next_index.items[0] = 2; // optimistic next_index for peer 2
    leader.match_index.items[0] = 0;

    // Leader replicates entry at index 1? Actually index 2 in term 3 would fail if prev=1?
    // Let's set next_index = 4 so prev_log_index = 3 fails first and triggers conflict rollback!
    leader.next_index.items[0] = 4;
    const empty_entries = try alloc.alloc(LogEntry, 0);
    defer alloc.free(empty_entries);
    const stale_req = AppendEntriesReq{
        .term = 3,
        .leader_id = 1,
        .prev_log_index = 3,
        .prev_log_term = 3,
        .entries = empty_entries,
        .leader_commit = 0,
    };
    let fail_res = try follower.handleAppendEntries(stale_req);
    defer fail_res.deinit(alloc);
    try expect_eq(fail_res.resp.success, false);
    try expect_eq(fail_res.resp.conflict_term, @as(u64, 2));
    try expect_eq(fail_res.resp.conflict_index, @as(u64, 2));

    // Leader processes conflict response -> rolls back next_index to 2 and sends AppendEntries
    const retry_acts = try leader.handleAppendEntriesResp(fail_res.resp);
    defer deinitActions(alloc, retry_acts);
    try expect_eq(leader.next_index.items[0], @as(u64, 2));

    // Raft §5.4.2 check: even if old term 1 entry is replicated on majority,
    // leader in term 3 MUST NOT advance commit_index until a term 3 entry is committed.
    leader.match_index.items[0] = 1;
    const dummy_ack = try leader.handleAppendEntriesResp(fail_res.resp);
    deinitActions(alloc, dummy_ack);
    try expect_eq(leader.commit_index, @as(u64, 0));

    // Now propose a term 3 entry and replicate to follower -> truncates stale term 2 and commits both!
    const prop_acts = try leader.propose("term3_cmd");
    defer deinitActions(alloc, prop_acts);
    for (act of prop_acts) {
        switch (act) {
            .SendAppendEntries => |sa| {
                if (sa.target_id == 2) {
                    let ok_res = try follower.handleAppendEntries(sa.asReq());
                    defer ok_res.deinit(alloc);
                    try expect_eq(ok_res.resp.success, true);
                    try expect_eq(follower.log.lastIndex(), @as(u64, 2));
                    try expect_eq(follower.log.termAt(2).?, @as(u64, 3));
                    deinitActions(alloc, try leader.handleAppendEntriesResp(ok_res.resp));
                }
            },
            else => {},
        }
    }
    try expect_eq(leader.commit_index, @as(u64, 2));
}

test("raft log compactTo snapshot baseline and boundaries") {
    const alloc = std.testing.allocator;
    let log = RaftLog.init(alloc);
    defer log.deinit();

    * = try log.append(1, "cmd1");
    * = try log.append(1, "cmd2");
    * = try log.append(2, "cmd3");
    * = try log.append(2, "cmd4");

    try expect_eq(log.lastIndex(), @as(u64, 4));
    try expect_eq(log.lastTerm(), @as(u64, 2));

    // Compact up to index 2 (term 1)
    try log.compactTo(2, 1);
    try expect_eq(log.snapshot_index, @as(u64, 2));
    try expect_eq(log.snapshot_term, @as(u64, 1));
    try expect_eq(log.lastIndex(), @as(u64, 4));
    try expect_eq(log.termAt(1) == null, true);
    try expect_eq(log.matchTerm(2, 1), true);
    try expect_eq(log.matchTerm(3, 2), true);
    try expect_eq(log.entryAt(2) == null, true);
    try expect_eq(log.entryAt(3).?.term, @as(u64, 2));

    // Compacting beyond lastIndex returns InvalidIndex
    let err_hit = false;
    log.compactTo(10, 2) catch |e| {
        if (e == RaftError.InvalidIndex) {
            err_hit = true;
        }
    };
    try expect_eq(err_hit, true);

    // Compact all remaining entries
    try log.compactTo(4, 2);
    try expect_eq(log.lastIndex(), @as(u64, 4));
    try expect_eq(log.lastTerm(), @as(u64, 2));
    try expect_eq(log.matchTerm(4, 2), true);
}

test("cap.raft dynamic mount and zero-leak verification") {
    const alloc = std.testing.allocator;
    let log = cap.raft.RaftLog.init(alloc);
    defer log.deinit();

    const idx = try log.append(7, "mounted_cmd");
    try expect_eq(idx, @as(u64, 1));
    try expect_eq(log.matchTerm(1, 7), true);

    const sliced = try log.sliceFrom(alloc, 1);
    defer {
        for (e of sliced) {
            alloc.free(e.command);
        }
        alloc.free(sliced);
    }
    try expect_eq(sliced.len, @as(usize, 1));
    try expect_eq(sliced[0].term, @as(u64, 7));
}
