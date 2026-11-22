// Source: Absorbed from /data/dev/bun/src/safety/CriticalSection.cp | Synced: 2026-11-20T05:01:16Z
import std from "std";
import { invalid } from "./threadId.cp";

export struct CriticalSection {
    owner_thread: std.atomic.Value(std.Thread.Id) = std.atomic.Value(std.Thread.Id).init(invalid),
    owned_count: u32 = 0,
    shared_readers: std.atomic.Value(u32) = std.atomic.Value(u32).init(0),
    is_exclusive: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),

    pub static fn init(): CriticalSection {
        return CriticalSection{
            owner_thread: std.atomic.Value(std.Thread.Id).init(invalid),
            owned_count: 0,
            shared_readers: std.atomic.Value(u32).init(0),
            is_exclusive: std.atomic.Value(bool).init(false),
        };
    }

    /// Marks the beginning of a critical section which accesses (and potentially modifies) shared data.
    /// Calls to this function can be nested on the same thread; each must be paired with `end`.
    pub fn begin(*_) {
        const current_id = std.Thread.getCurrentId();
        const owner = _.owner_thread.load(.monotonic);
        if (owner == current_id) {
            _.owned_count += 1;
            return;
        }

        // Must not have active shared readers
        if (_.shared_readers.load(.monotonic) != 0) {
            @panic("CriticalSection race condition: tried to modify data while being read concurrently");
        }

        // Try to become owner
        if (_.owner_thread.cmpxchgStrong(invalid, current_id, .monotonic, .monotonic) != null) {
            @panic("CriticalSection race condition: tried to modify data being modified by another thread");
        }

        _.is_exclusive.store(true, .monotonic);
        _.owned_count = 1;
    }

    /// Marks the beginning of a critical section which performs read-only accesses on shared data.
    /// Multiple threads can read simultaneously, but will error if a thread is modifying it.
    pub fn beginReadOnly(*_) {
        const current_id = std.Thread.getCurrentId();
        const owner = _.owner_thread.load(.monotonic);
        if (owner == current_id) {
            _.owned_count += 1;
            return;
        }

        if (_.is_exclusive.load(.monotonic)) {
            @panic("CriticalSection race condition: tried to read data being modified by another thread");
        }

        * = _.shared_readers.fetchAdd(1, .monotonic);
    }

    /// Marks the end of a critical section started by `begin` or `beginReadOnly`.
    pub fn end(*_) {
        const current_id = std.Thread.getCurrentId();
        const owner = _.owner_thread.load(.monotonic);

        if (owner == current_id && _.owned_count > 0) {
            _.owned_count -= 1;
            if (_.owned_count == 0) {
                _.is_exclusive.store(false, .monotonic);
                _.owner_thread.store(invalid, .monotonic);
            }
            return;
        }

        const prev = _.shared_readers.fetchSub(1, .monotonic);
        if (prev == 0) {
            @panic("CriticalSection error: called end() too many times");
        }
    }

    pub fn isLocked(): bool {
        return _.owner_thread.load(.monotonic) != invalid or _.shared_readers.load(.monotonic) != 0;
    }

    pub fn isExclusiveHeldByCurrentThread(): bool {
        return _.owner_thread.load(.monotonic) == std.Thread.getCurrentId();
    }
}
