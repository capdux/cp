// Source: cp/lib/raft/log.cp (Replicated Raft log with snapshot compaction baseline)
const std = @import("std");
import { LogEntry, RaftError } from "./types.cp";

pub struct RaftLog {
    pub allocator: std.mem.Allocator,
    pub snapshot_index: u64,
    pub snapshot_term: u64,
    pub entries: std.ArrayList(LogEntry),

    pub fn init(allocator: std.mem.Allocator): RaftLog {
        return RaftLog{
            .allocator = allocator,
            .snapshot_index = 0,
            .snapshot_term = 0,
            .entries = std.ArrayList(LogEntry).empty,
        };
    }

    pub fn deinit(*_) {
        for (entry of _.entries.items) {
            _.allocator.free(entry.command);
        }
        _.entries.deinit(_.allocator);
    }

    pub fn lastIndex(): u64 {
        if (_.entries.items.len == 0) {
            return _.snapshot_index;
        }
        return _.snapshot_index + @as(u64, @intCast(_.entries.items.len));
    }

    pub fn lastTerm(): u64 {
        if (_.entries.items.len == 0) {
            return _.snapshot_term;
        }
        return _.entries.items[_.entries.items.len - 1].term;
    }

    pub fn termAt(index: u64): u64? {
        if (index < _.snapshot_index) {
            return null;
        }
        if (index == _.snapshot_index) {
            return _.snapshot_term;
        }
        const offset: usize = @intCast(index - _.snapshot_index - 1);
        if (offset >= _.entries.items.len) {
            return null;
        }
        return _.entries.items[offset].term;
    }

    pub fn matchTerm(index: u64, term: u64): bool {
        if (t := _.termAt(index)) {
            return t == term;
        }
        return false;
    }

    pub fn entryAt(index: u64): LogEntry? {
        if (index <= _.snapshot_index) {
            return null;
        }
        const offset: usize = @intCast(index - _.snapshot_index - 1);
        if (offset >= _.entries.items.len) {
            return null;
        }
        return _.entries.items[offset];
    }

    pub fn append(*_, term: u64, command: []const u8): !u64 {
        const next_idx = _.lastIndex() + 1;
        const cmd_copy = try _.allocator.dupe(u8, command);
        errdefer _.allocator.free(cmd_copy);
        try _.entries.append(_.allocator, LogEntry{
            .term = term,
            .index = next_idx,
            .command = cmd_copy,
        });
        return next_idx;
    }

    pub fn truncateFrom(*_, from_index: u64) {
        if (from_index <= _.snapshot_index) {
            for (entry of _.entries.items) {
                _.allocator.free(entry.command);
            }
            _.entries.clearRetainingCapacity();
            return;
        }
        const keep_len: usize = @intCast(from_index - _.snapshot_index - 1);
        if (keep_len >= _.entries.items.len) {
            return;
        }
        for (entry of _.entries.items[keep_len..]) {
            _.allocator.free(entry.command);
        }
        _.entries.shrinkRetainingCapacity(keep_len);
    }

    pub fn compactTo(*_, snap_index: u64, snap_term: u64): !void {
        if (snap_index <= _.snapshot_index) {
            return;
        }
        if (snap_index > _.lastIndex()) {
            return RaftError.InvalidIndex;
        }
        const remove_count: usize = @intCast(snap_index - _.snapshot_index);
        for (entry of _.entries.items[0..remove_count]) {
            _.allocator.free(entry.command);
        }
        const remaining = _.entries.items.len - remove_count;
        if (remaining > 0) {
            std.mem.copyForwards(LogEntry, _.entries.items[0..remaining], _.entries.items[remove_count..]);
        }
        _.entries.shrinkRetainingCapacity(remaining);
        _.snapshot_index = snap_index;
        _.snapshot_term = snap_term;
    }

    pub fn sliceFrom(allocator: std.mem.Allocator, from_index: u64): ![]LogEntry {
        if (from_index <= _.snapshot_index or from_index > _.lastIndex()) {
            return try allocator.alloc(LogEntry, 0);
        }
        const start_offset: usize = @intCast(from_index - _.snapshot_index - 1);
        const count = _.entries.items.len - start_offset;
        let out = try allocator.alloc(LogEntry, count);
        let built: usize = 0;
        errdefer {
            for (e of out[0..built]) {
                allocator.free(e.command);
            }
            allocator.free(out);
        }
        for (entry of _.entries.items[start_offset..]) {
            out[built] = LogEntry{
                .term = entry.term,
                .index = entry.index,
                .command = try allocator.dupe(u8, entry.command),
            };
            built += 1;
        }
        return out;
    }
}
