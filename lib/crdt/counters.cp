// Source: cp/lib/crdt/counters.cp (GCounter & PNCounter State-Based CRDTs)
// Convergent state-based Grow-Only Counter (`GCounter`) and Positive-Negative Counter
// (`PnCounter`) with commutative, associative, and idempotent pointwise-max merge.

const std = @import("std");
import { CrdtErr } from "./vclock.cp";

/// State-based Grow-Only Counter (`GCounter`) CRDT.
/// Tracks per-node monotonic increments and merges via pointwise maximum.
pub struct GCounter {
    allocator: std.mem.Allocator = undefined,
    counts: std.StringHashMapUnmanaged(u64) = .empty,

    /// Initializes an empty `GCounter` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): GCounter {
        return GCounter{
            allocator: alloc,
            counts: .empty,
        };
    }

    /// Releases all owned node identifiers and internal map storage.
    pub fn deinit(*_) {
        let iter = _.counts.iterator();
        while (entry := iter.next()) {
            _.allocator.free(entry.key_ptr.*);
        }
        _.counts.deinit(_.allocator);
    }

    /// Adds `delta` to `node_id`'s local counter slot.
    /// Rejects empty `node_id` with `CrdtErr.InvalidParameter`.
    pub fn increment(*_, node_id: string, delta: u64): ! {
        if (node_id.len == 0) {
            return CrdtErr.InvalidParameter;
        }
        if (delta == 0) {
            return;
        }
        if (ptr := _.counts.getPtr(node_id)) {
            ptr.* = std.math.add(u64, ptr.*, delta) catch return CrdtErr.Overflow;
            return;
        }
        const owned_id = _.allocator.dupe(u8, node_id) catch return CrdtErr.OutOfMemory;
        errdefer _.allocator.free(owned_id);
        _.counts.put(_.allocator, owned_id, delta) catch return CrdtErr.OutOfMemory;
    }

    /// Returns the observed count contributed by `node_id` (`0` if absent).
    pub fn getNode(node_id: string): u64 {
        if (v := _.counts.get(node_id)) {
            return v;
        }
        return 0;
    }

    /// Returns the global counter sum across all observed replicas (saturating on `u64` overflow).
    pub fn value(): u64 {
        let total: u64 = 0;
        let iter = _.counts.iterator();
        while (entry := iter.next()) {
            total = total +| entry.value_ptr.*;
        }
        return total;
    }

    /// Merges `other` into `this` by taking the pointwise maximum for every `node_id`.
    /// Satisfies commutativity, associativity, and idempotence.
    pub fn merge(*_, other: GCounter): ! {
        let iter = other.counts.iterator();
        while (entry := iter.next()) {
            const val = entry.value_ptr.*;
            if (val == 0) continue;

            const k = entry.key_ptr.*;
            if (ptr := _.counts.getPtr(k)) {
                if (val > ptr.*) {
                    ptr.* = val;
                }
            } else {
                const owned_id = _.allocator.dupe(u8, k) catch return CrdtErr.OutOfMemory;
                errdefer _.allocator.free(owned_id);
                _.counts.put(_.allocator, owned_id, val) catch return CrdtErr.OutOfMemory;
            }
        }
    }

    /// Returns `true` if `this` and `other` hold identical per-node counts.
    pub fn eql(other: GCounter): bool {
        let iter_self = _.counts.iterator();
        while (entry := iter_self.next()) {
            if (other.getNode(entry.key_ptr.*) != entry.value_ptr.*) {
                return false;
            }
        }
        let iter_other = other.counts.iterator();
        while (entry := iter_other.next()) {
            if (_.getNode(entry.key_ptr.*) != entry.value_ptr.*) {
                return false;
            }
        }
        return true;
    }

    /// Creates an ownership-independent deep clone of this `GCounter` using `alloc`.
    pub fn clone(alloc: std.mem.Allocator): !GCounter {
        let copy = GCounter.init(alloc);
        errdefer copy.deinit();

        let iter = _.counts.iterator();
        while (entry := iter.next()) {
            const owned_id = alloc.dupe(u8, entry.key_ptr.*) catch return CrdtErr.OutOfMemory;
            errdefer alloc.free(owned_id);
            copy.counts.put(alloc, owned_id, entry.value_ptr.*) catch return CrdtErr.OutOfMemory;
        }
        return copy;
    }
}

/// State-based Positive-Negative Counter (`PnCounter`) CRDT built from paired `pos` and `neg` `GCounter`s.
/// Supports distributed increments and decrements with exact `i64` difference evaluation.
pub struct PnCounter {
    pos: GCounter = undefined,
    neg: GCounter = undefined,

    /// Initializes a zero-valued `PnCounter` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): PnCounter {
        return PnCounter{
            pos: GCounter.init(alloc),
            neg: GCounter.init(alloc),
        };
    }

    /// Releases both internal `GCounter` instances.
    pub fn deinit(*_) {
        _.pos.deinit();
        _.neg.deinit();
    }

    /// Adds `delta` to the positive counter for `node_id`.
    pub fn increment(*_, node_id: string, delta: u64): ! {
        _.pos.increment(node_id, delta)?;
    }

    /// Adds `delta` to the negative counter for `node_id`.
    pub fn decrement(*_, node_id: string, delta: u64): ! {
        _.neg.increment(node_id, delta)?;
    }

    /// Computes `pos.value() - neg.value()` clamped safely to `[i64.MIN, i64.MAX]`.
    pub fn value(): i64 {
        const p: i128 = @as(i128, _.pos.value());
        const n: i128 = @as(i128, _.neg.value());
        const diff: i128 = p - n;
        const min_v: i128 = @as(i128, std.math.minInt(i64));
        const max_v: i128 = @as(i128, std.math.maxInt(i64));
        if (diff < min_v) return std.math.minInt(i64);
        if (diff > max_v) return std.math.maxInt(i64);
        return diff as i64;
    }

    /// Merges `other`'s `pos` and `neg` counters into `this`.
    /// Satisfies commutativity, associativity, and idempotence.
    pub fn merge(*_, other: PnCounter): ! {
        _.pos.merge(other.pos)?;
        _.neg.merge(other.neg)?;
    }

    /// Returns `true` if `this` and `other` have identical `pos` and `neg` state.
    pub fn eql(other: PnCounter): bool {
        return _.pos.eql(other.pos) && _.neg.eql(other.neg);
    }

    /// Creates an ownership-independent deep clone of this `PnCounter` using `alloc`.
    pub fn clone(alloc: std.mem.Allocator): !PnCounter {
        let p = _.pos.clone(alloc)?;
        errdefer p.deinit();
        let n = _.neg.clone(alloc)?;
        return PnCounter{
            pos: p,
            neg: n,
        };
    }
}
