// Reference-hardened against tikv/raft-rs (Apache-2.0)
// Source: native-cp-stdlib (Raft Replicated Log, Conflict Truncation & Snapshot Compaction)
const std = @import("std");

pub err RaftLogError {
    CompactedIndex,
    OutOfBounds,
    ConflictTermMismatch,
    InvalidCommitIndex,
    TruncateCommittedIndex,
    InvalidTermOrder,
    OutOfMemory,
}

pub struct LogEntry {
    pub index: u64,
    pub term: u64,
    pub data: []u8,
}

pub struct LogEntryInput {
    pub term: u64,
    pub data: []const u8,
}

pub struct RaftLog {
    alloc: std.mem.Allocator,
    entries: std.ArrayList(LogEntry),
    snapshot_index: u64,
    snapshot_term: u64,
    commit_index: u64,
    applied_index: u64,

    pub static fn init(alloc: std.mem.Allocator): RaftLog {
        return RaftLog {
            alloc: alloc,
            entries: std.ArrayList(LogEntry).empty,
            snapshot_index: 0,
            snapshot_term: 0,
            commit_index: 0,
            applied_index: 0,
        };
    }

    pub fn deinit(*_) {
        for (entry of _.entries.items) {
            _.alloc.free(entry.data);
        }
        _.entries.deinit(_.alloc);
    }

    pub fn firstIndex(): u64 {
        return _.snapshot_index + 1;
    }

    pub fn lastIndex(): u64 {
        const count: u64 = @intCast(_.entries.items.len);
        return _.snapshot_index + count;
    }

    pub fn lastTerm(): u64 {
        if (_.entries.items.len > 0) {
            const last_pos: usize = _.entries.items.len - 1;
            return _.entries.items[last_pos].term;
        }
        return _.snapshot_term;
    }

    pub fn snapshotIndex(): u64 {
        return _.snapshot_index;
    }

    pub fn snapshotTerm(): u64 {
        return _.snapshot_term;
    }

    pub fn commitIndex(): u64 {
        return _.commit_index;
    }

    pub fn appliedIndex(): u64 {
        return _.applied_index;
    }

    pub fn termAt(index: u64): u64 ! RaftLogError {
        if (index < _.snapshot_index) {
            return RaftLogError.CompactedIndex;
        }
        if (index == _.snapshot_index) {
            return _.snapshot_term;
        }
        if (index > _.lastIndex()) {
            return RaftLogError.OutOfBounds;
        }
        const offset: usize = @intCast(index - _.snapshot_index - 1);
        return _.entries.items[offset].term;
    }

    pub fn entryAt(index: u64): LogEntry ! RaftLogError {
        if (index <= _.snapshot_index) {
            if (index == 0 && _.snapshot_index == 0) {
                return RaftLogError.OutOfBounds;
            }
            return RaftLogError.CompactedIndex;
        }
        if (index > _.lastIndex()) {
            return RaftLogError.OutOfBounds;
        }
        const offset: usize = @intCast(index - _.snapshot_index - 1);
        return _.entries.items[offset];
    }

    pub fn append(*_, term: u64, data: []const u8): u64 ! RaftLogError {
        if (term < _.lastTerm()) {
            return RaftLogError.InvalidTermOrder;
        }
        const next_idx: u64 = _.lastIndex() + 1;
        const cloned = _.alloc.dupe(u8, data) catch return RaftLogError.OutOfMemory;
        errdefer _.alloc.free(cloned);
        _.entries.append(_.alloc, LogEntry {
            index: next_idx,
            term: term,
            data: cloned,
        }) catch return RaftLogError.OutOfMemory;
        return next_idx;
    }

    fn truncateFrom(*_, from_offset: usize) {
        let i: usize = from_offset;
        while (i < _.entries.items.len) {
            _.alloc.free(_.entries.items[i].data);
            i += 1;
        }
        _.entries.shrinkRetainingCapacity(from_offset);
    }

    /// Replicates `new_entries` after `(prev_index, prev_term)` per Raft Log Matching property.
    /// Rejects non-monotonic terms and strictly forbids truncating any entry `<= commit_index`.
    pub fn appendIfMatch(*_, prev_index: u64, prev_term: u64, new_entries: []const LogEntryInput): bool ! RaftLogError {
        if (prev_index < _.snapshot_index) {
            return false;
        }
        if (prev_index > _.lastIndex()) {
            return false;
        }
        const local_term = _.termAt(prev_index) catch return false;
        if (local_term != prev_term) {
            return false;
        }

        // Pre-validate monotonic term ordering across incoming entries
        let expected_min_term = prev_term;
        for (item of new_entries) {
            if (item.term < expected_min_term) {
                return RaftLogError.InvalidTermOrder;
            }
            expected_min_term = item.term;
        }

        let i: usize = 0;
        while (i < new_entries.len) {
            const idx_offset: u64 = @intCast(i);
            const target_idx: u64 = prev_index + 1 + idx_offset;
            const item = new_entries[i];

            if (target_idx <= _.lastIndex()) {
                const offset: usize = @intCast(target_idx - _.snapshot_index - 1);
                if (_.entries.items[offset].term != item.term) {
                    if (target_idx <= _.commit_index) {
                        return RaftLogError.TruncateCommittedIndex;
                    }
                    _.truncateFrom(offset);
                    * = try _.append(item.term, item.data);
                }
            } else {
                * = try _.append(item.term, item.data);
            }
            i += 1;
        }

        return true;
    }

    pub fn commitTo(*_, index: u64): void ! RaftLogError {
        if (index < _.commit_index) {
            return RaftLogError.InvalidCommitIndex;
        }
        if (index > _.lastIndex()) {
            return RaftLogError.OutOfBounds;
        }
        _.commit_index = index;
    }

    pub fn appliedTo(*_, index: u64): void ! RaftLogError {
        if (index < _.applied_index || index > _.commit_index) {
            return RaftLogError.InvalidCommitIndex;
        }
        _.applied_index = index;
    }

    pub fn compactTo(*_, index: u64): void ! RaftLogError {
        if (index <= _.snapshot_index) {
            return RaftLogError.CompactedIndex;
        }
        if (index > _.lastIndex()) {
            return RaftLogError.OutOfBounds;
        }
        if (index > _.commit_index) {
            return RaftLogError.InvalidCommitIndex;
        }

        const cut_count: usize = @intCast(index - _.snapshot_index);
        const last_cut_pos: usize = cut_count - 1;
        const new_snap_term: u64 = _.entries.items[last_cut_pos].term;

        let i: usize = 0;
        while (i < cut_count) {
            _.alloc.free(_.entries.items[i].data);
            i += 1;
        }

        const rem_len: usize = _.entries.items.len - cut_count;
        let j: usize = 0;
        while (j < rem_len) {
            const src_pos: usize = j + cut_count;
            _.entries.items[j] = _.entries.items[src_pos];
            j += 1;
        }
        _.entries.shrinkRetainingCapacity(rem_len);

        _.snapshot_index = index;
        _.snapshot_term = new_snap_term;
        if (_.applied_index < index) {
            _.applied_index = index;
        }
    }

    /// Installs a remote snapshot at `(snap_index, snap_term)`, rejecting stale snapshots
    /// `<= commit_index` or term regressions.
    pub fn installSnapshot(*_, snap_index: u64, snap_term: u64): void ! RaftLogError {
        if (snap_index <= _.commit_index || snap_index <= _.snapshot_index) {
            return RaftLogError.CompactedIndex;
        }
        if (snap_term < _.snapshot_term) {
            return RaftLogError.InvalidTermOrder;
        }
        _.truncateFrom(0);
        _.snapshot_index = snap_index;
        _.snapshot_term = snap_term;
        _.commit_index = snap_index;
        _.applied_index = snap_index;
    }

    /// Verifies core Raft log structural invariants: contiguous indices, monotonic terms,
    /// and `snapshot_index <= applied_index <= commit_index <= lastIndex()`.
    pub fn verifyInvariants(): bool {
        if (_.applied_index < _.snapshot_index || _.applied_index > _.commit_index) return false;
        if (_.commit_index < _.snapshot_index || _.commit_index > _.lastIndex()) return false;
        let prev_term = _.snapshot_term;
        let i: usize = 0;
        while (i < _.entries.items.len) : (i += 1) {
            const expected_idx = _.snapshot_index + 1 + @as(u64, @intCast(i));
            const e = _.entries.items[i];
            if (e.index != expected_idx) return false;
            if (e.term < prev_term) return false;
            prev_term = e.term;
        }
        return true;
    }

    pub fn isUpToDate(candidate_last_term: u64, candidate_last_index: u64): bool {
        const my_term = _.lastTerm();
        if (candidate_last_term != my_term) {
            return candidate_last_term > my_term;
        }
        return candidate_last_index >= _.lastIndex();
    }
}
