// Source: cp/lib/crdt/vclock.cp (Lamport Vector Clock & Causal Ordering)
// State-based Lamport Vector Clock with deterministic causal partial ordering
// (Equal, Before, After, Concurrent) and join-semilattice merge semantics.

const std = @import("std");

/// Error set shared across `cap:crdt` conflict-free replicated data types.
pub err CrdtErr {
    OutOfMemory,
    InvalidParameter,
    Overflow,
}

/// Causal partial-order relation between two vector clocks `A.compare(B)`.
pub enum CausalOrder: u8 {
    Equal = 0,
    Before = 1,
    After = 2,
    Concurrent = 3,

    /// Returns `true` when both clocks represent the identical causal state.
    pub fn isEqual(): bool {
        return _ == .Equal;
    }

    /// Returns `true` when `self` causally happened-before `other` (`self < other`).
    pub fn isBefore(): bool {
        return _ == .Before;
    }

    /// Returns `true` when `self` causally happened-after `other` (`self > other`).
    pub fn isAfter(): bool {
        return _ == .After;
    }

    /// Returns `true` when neither clock dominates the other (`self || other`).
    pub fn isConcurrent(): bool {
        return _ == .Concurrent;
    }
}

/// State-based Lamport Vector Clock mapping replica `node_id` -> monotonic `u64` counter.
/// Missing entries are implicitly `0`.
pub struct VectorClock {
    allocator: std.mem.Allocator = undefined,
    clocks: std.StringHashMapUnmanaged(u64) = .empty,

    /// Initializes an empty `VectorClock` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): VectorClock {
        return VectorClock{
            allocator: alloc,
            clocks: .empty,
        };
    }

    /// Releases all owned node identifiers and internal hash map storage.
    pub fn deinit(*_) {
        let iter = _.clocks.iterator();
        while (entry := iter.next()) {
            _.allocator.free(entry.key_ptr.*);
        }
        _.clocks.deinit(_.allocator);
    }

    /// Returns the number of tracked node entries in this clock.
    pub fn len(): usize {
        return _.clocks.count();
    }

    /// Advances the logical clock for `node_id` by 1.
    /// Rejects empty `node_id` with `CrdtErr.InvalidParameter`.
    pub fn increment(*_, node_id: string): ! {
        if (node_id.len == 0) {
            return CrdtErr.InvalidParameter;
        }
        if (ptr := _.clocks.getPtr(node_id)) {
            if (ptr.* == std.math.maxInt(u64)) {
                return CrdtErr.Overflow;
            }
            ptr.* += 1;
            return;
        }
        const owned_id = _.allocator.dupe(u8, node_id) catch return CrdtErr.OutOfMemory;
        errdefer _.allocator.free(owned_id);
        _.clocks.put(_.allocator, owned_id, 1) catch return CrdtErr.OutOfMemory;
    }

    /// Advances the logical clock for `node_id` by `delta`.
    pub fn incrementBy(*_, node_id: string, delta: u64): ! {
        if (node_id.len == 0) {
            return CrdtErr.InvalidParameter;
        }
        if (delta == 0) {
            return;
        }
        if (ptr := _.clocks.getPtr(node_id)) {
            ptr.* = std.math.add(u64, ptr.*, delta) catch return CrdtErr.Overflow;
            return;
        }
        const owned_id = _.allocator.dupe(u8, node_id) catch return CrdtErr.OutOfMemory;
        errdefer _.allocator.free(owned_id);
        _.clocks.put(_.allocator, owned_id, delta) catch return CrdtErr.OutOfMemory;
    }

    /// Returns the logical counter for `node_id` (`0` if never observed).
    pub fn get(node_id: string): u64 {
        if (v := _.clocks.get(node_id)) {
            return v;
        }
        return 0;
    }

    /// Merges `other` into `this` via pointwise maximum (`max(self[k], other[k])`).
    /// Satisfies commutativity, associativity, and idempotence.
    pub fn merge(*_, other: VectorClock): ! {
        let iter = other.clocks.iterator();
        while (entry := iter.next()) {
            const val = entry.value_ptr.*;
            if (val == 0) continue;

            const k = entry.key_ptr.*;
            if (ptr := _.clocks.getPtr(k)) {
                if (val > ptr.*) {
                    ptr.* = val;
                }
            } else {
                const owned_id = _.allocator.dupe(u8, k) catch return CrdtErr.OutOfMemory;
                errdefer _.allocator.free(owned_id);
                _.clocks.put(_.allocator, owned_id, val) catch return CrdtErr.OutOfMemory;
            }
        }
    }

    /// Compares `this` against `other` under the Lamport causal partial order.
    pub fn compare(other: VectorClock): CausalOrder {
        let has_less = false;
        let has_greater = false;

        let iter_self = _.clocks.iterator();
        while (entry := iter_self.next()) {
            const v_self = entry.value_ptr.*;
            const v_other = other.get(entry.key_ptr.*);
            if (v_self < v_other) {
                has_less = true;
            } else if (v_self > v_other) {
                has_greater = true;
            }
            if (has_less && has_greater) {
                return CausalOrder.Concurrent;
            }
        }

        let iter_other = other.clocks.iterator();
        while (entry := iter_other.next()) {
            const k = entry.key_ptr.*;
            if (!_.clocks.contains(k)) {
                if (entry.value_ptr.* > 0) {
                    has_less = true;
                    if (has_greater) {
                        return CausalOrder.Concurrent;
                    }
                }
            }
        }

        if (!has_less && !has_greater) return CausalOrder.Equal;
        if (has_less && !has_greater) return CausalOrder.Before;
        if (!has_less && has_greater) return CausalOrder.After;
        return CausalOrder.Concurrent;
    }

    /// Creates a deep ownership-independent clone of this `VectorClock` using `alloc`.
    pub fn clone(alloc: std.mem.Allocator): !VectorClock {
        let copy = VectorClock.init(alloc);
        errdefer copy.deinit();

        let iter = _.clocks.iterator();
        while (entry := iter.next()) {
            const owned_id = alloc.dupe(u8, entry.key_ptr.*) catch return CrdtErr.OutOfMemory;
            errdefer alloc.free(owned_id);
            copy.clocks.put(alloc, owned_id, entry.value_ptr.*) catch return CrdtErr.OutOfMemory;
        }
        return copy;
    }
}
