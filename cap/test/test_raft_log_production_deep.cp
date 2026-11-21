// Source: native-cp-stdlib (Raft Replicated Log & Snapshot Compaction)
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { RaftLog, RaftLogError, LogEntryInput } from "cap:raft_log";
import cap from "cap";

test "raft_log: 1-based append and termAt/entryAt queries" {
    let log = RaftLog.init(std.testing.allocator);
    defer log.deinit();

    expect_eq(log.firstIndex(), @as(u64, 1));
    expect_eq(log.lastIndex(), @as(u64, 0));
    expect_eq(log.lastTerm(), @as(u64, 0));
    expect_eq(try log.termAt(0), @as(u64, 0));

    const i1 = try log.append(1, "cmd-1");
    const i2 = try log.append(2, "cmd-2");
    expect_eq(i1, @as(u64, 1));
    expect_eq(i2, @as(u64, 2));
    expect_eq(log.lastIndex(), @as(u64, 2));
    expect_eq(log.lastTerm(), @as(u64, 2));
    expect_eq(try log.termAt(1), @as(u64, 1));
    expect_eq(try log.termAt(2), @as(u64, 2));

    const e1 = try log.entryAt(1);
    expect_eq(e1.index, @as(u64, 1));
    expect_eq(e1.term, @as(u64, 1));
    expect_eq(std.mem.eql(u8, e1.data, "cmd-1"), true);

    const oob = log.termAt(3);
    expect_eq(oob == RaftLogError.OutOfBounds, true);
}

test "raft_log: appendIfMatch consistency check and conflict truncation" {
    let log = RaftLog.init(std.testing.allocator);
    defer log.deinit();

    * = try log.append(1, "a1");
    * = try log.append(1, "a2");
    * = try log.append(1, "stale-3");

    const mismatch_entries = [
        LogEntryInput { term: 2, data: "b3" },
    ];
    const ok_bad_term = try log.appendIfMatch(2, 99, &mismatch_entries);
    expect_eq(ok_bad_term, false);

    const ok_bad_idx = try log.appendIfMatch(10, 1, &mismatch_entries);
    expect_eq(ok_bad_idx, false);

    const override_entries = [
        LogEntryInput { term: 2, data: "new-3" },
        LogEntryInput { term: 3, data: "new-4" },
    ];
    const ok_good = try log.appendIfMatch(2, 1, &override_entries);
    expect_eq(ok_good, true);
    expect_eq(log.lastIndex(), @as(u64, 4));
    expect_eq(log.lastTerm(), @as(u64, 3));
    expect_eq(try log.termAt(3), @as(u64, 2));

    const e3 = try log.entryAt(3);
    expect_eq(std.mem.eql(u8, e3.data, "new-3"), true);
    const e4 = try log.entryAt(4);
    expect_eq(std.mem.eql(u8, e4.data, "new-4"), true);
}

test "raft_log: commitTo and appliedTo monotonic advance and rollback rejection" {
    let log = RaftLog.init(std.testing.allocator);
    defer log.deinit();

    * = try log.append(1, "k1");
    * = try log.append(1, "k2");

    try log.commitTo(2);
    expect_eq(log.commitIndex(), @as(u64, 2));

    const rollback_commit = log.commitTo(1);
    expect_eq(rollback_commit == RaftLogError.InvalidCommitIndex, true);

    const oob_commit = log.commitTo(5);
    expect_eq(oob_commit == RaftLogError.OutOfBounds, true);

    const over_apply = log.appliedTo(3);
    expect_eq(over_apply == RaftLogError.InvalidCommitIndex, true);

    try log.appliedTo(1);
    expect_eq(log.appliedIndex(), @as(u64, 1));

    const rollback_apply = log.appliedTo(0);
    expect_eq(rollback_apply == RaftLogError.InvalidCommitIndex, true);
}

test "raft_log: compactTo prefix truncation and snapshot metadata retention" {
    let log = RaftLog.init(std.testing.allocator);
    defer log.deinit();

    * = try log.append(1, "e1");
    * = try log.append(2, "e2");
    * = try log.append(2, "e3");
    * = try log.append(3, "e4");

    try log.commitTo(3);
    try log.compactTo(2);

    expect_eq(log.snapshotIndex(), @as(u64, 2));
    expect_eq(log.snapshotTerm(), @as(u64, 2));
    expect_eq(log.firstIndex(), @as(u64, 3));
    expect_eq(log.lastIndex(), @as(u64, 4));
    expect_eq(log.lastTerm(), @as(u64, 3));
    expect_eq(log.appliedIndex(), @as(u64, 2));

    expect_eq(try log.termAt(2), @as(u64, 2));
    const compacted_term = log.termAt(1);
    expect_eq(compacted_term == RaftLogError.CompactedIndex, true);
    const compacted_entry = log.entryAt(2);
    expect_eq(compacted_entry == RaftLogError.CompactedIndex, true);

    const e3 = try log.entryAt(3);
    expect_eq(e3.index, @as(u64, 3));
    expect_eq(std.mem.eql(u8, e3.data, "e3"), true);

    const uncommit_compact = log.compactTo(4);
    expect_eq(uncommit_compact == RaftLogError.InvalidCommitIndex, true);
}

test "raft_log: cap.raft_log dynamic mount and isUpToDate comparison" {
    let log = cap.raft_log.RaftLog.init(std.testing.allocator);
    defer log.deinit();

    * = try log.append(2, "x1");
    * = try log.append(2, "x2");

    expect_eq(log.isUpToDate(3, 1), true);
    expect_eq(log.isUpToDate(1, 100), false);
    expect_eq(log.isUpToDate(2, 2), true);
    expect_eq(log.isUpToDate(2, 3), true);
    expect_eq(log.isUpToDate(2, 1), false);
}
