// Source: cp/lib/crdt/register.cp (LWW-Register & Observed-Remove Set CRDTs)
// Deterministic Last-Write-Wins string register (`LwwRegister`) and Observed-Remove Set
// (`OrSet`) guaranteeing concurrent add-wins semantics and multi-replica convergence.

const std = @import("std");
import { CrdtErr } from "./vclock.cp";

/// Last-Write-Wins Register (`LwwRegister`) for string values.
/// Resolves concurrent updates deterministically by `(timestamp_ms, node_id)` total order.
pub struct LwwRegister {
    allocator: std.mem.Allocator = undefined,
    has_value: bool = false,
    val_buf: []u8 = "",
    node_id_buf: []u8 = "",
    timestamp_ms: i64 = 0,

    /// Initializes an empty `LwwRegister` (`get() == null`).
    pub static fn init(alloc: std.mem.Allocator): LwwRegister {
        return LwwRegister{
            allocator: alloc,
            has_value: false,
            val_buf: "",
            node_id_buf: "",
            timestamp_ms: 0,
        };
    }

    /// Releases any owned value and writer node identifier buffers.
    pub fn deinit(*_) {
        if (_.has_value) {
            _.allocator.free(_.val_buf);
            _.allocator.free(_.node_id_buf);
            _.val_buf = "";
            _.node_id_buf = "";
            _.has_value = false;
        }
    }

    /// Sets `val` at logical/physical timestamp `ts` authored by `node_id`.
    /// Updates state if unset, `ts > current_ts`, or `(ts == current_ts && node_id >= current_node_id)`.
    pub fn set(*_, val: string, ts: i64, node_id: string): ! {
        if (node_id.len == 0) {
            return CrdtErr.InvalidParameter;
        }
        if (_.has_value) {
            if (ts < _.timestamp_ms) {
                return;
            }
            if (ts == _.timestamp_ms) {
                const ord = std.mem.order(u8, node_id, _.node_id_buf);
                if (ord == .lt) {
                    return;
                }
            }
        }

        const new_val = _.allocator.dupe(u8, val) catch return CrdtErr.OutOfMemory;
        errdefer _.allocator.free(new_val);
        const new_node = _.allocator.dupe(u8, node_id) catch return CrdtErr.OutOfMemory;

        if (_.has_value) {
            _.allocator.free(_.val_buf);
            _.allocator.free(_.node_id_buf);
        }
        _.val_buf = new_val;
        _.node_id_buf = new_node;
        _.timestamp_ms = ts;
        _.has_value = true;
    }

    /// Returns the winning string value, or `null` if never written.
    pub fn get(): string? {
        if (!_.has_value) return null;
        return _.val_buf;
    }

    /// Returns the timestamp of the current winning write (`0` if unset).
    pub fn timestamp(): i64 {
        return _.timestamp_ms;
    }

    /// Returns the writer `node_id` of the current winning write, or `null` if unset.
    pub fn writerNode(): string? {
        if (!_.has_value) return null;
        return _.node_id_buf;
    }

    /// Merges `other` into `this` using deterministic `(timestamp_ms, node_id)` winner selection
    /// (with lexicographical `val` tie-break on identical `(ts, node_id)` to guarantee commutativity).
    pub fn merge(*_, other: LwwRegister): ! {
        if (!other.has_value) return;

        if (_.has_value) {
            if (other.timestamp_ms < _.timestamp_ms) {
                return;
            }
            if (other.timestamp_ms == _.timestamp_ms) {
                const node_ord = std.mem.order(u8, other.node_id_buf, _.node_id_buf);
                if (node_ord == .lt) {
                    return;
                }
                if (node_ord == .eq) {
                    if (std.mem.order(u8, other.val_buf, _.val_buf) != .gt) {
                        return;
                    }
                }
            }
        }

        const new_val = _.allocator.dupe(u8, other.val_buf) catch return CrdtErr.OutOfMemory;
        errdefer _.allocator.free(new_val);
        const new_node = _.allocator.dupe(u8, other.node_id_buf) catch return CrdtErr.OutOfMemory;

        if (_.has_value) {
            _.allocator.free(_.val_buf);
            _.allocator.free(_.node_id_buf);
        }
        _.val_buf = new_val;
        _.node_id_buf = new_node;
        _.timestamp_ms = other.timestamp_ms;
        _.has_value = true;
    }

    /// Creates an ownership-independent deep clone of this `LwwRegister` using `alloc`.
    pub fn clone(alloc: std.mem.Allocator): !LwwRegister {
        let copy = LwwRegister.init(alloc);
        if (_.has_value) {
            errdefer copy.deinit();
            copy.set(_.val_buf, _.timestamp_ms, _.node_id_buf)?;
        }
        return copy;
    }
}

/// Unique birth tag `(node_id, seq)` attached to an `OrSet` element addition.
pub struct OrSetTag {
    node_id: string = "",
    seq: u64 = 0,
    removed: bool = false,
}

struct OrSetBucket {
    tags: std.ArrayList(OrSetTag) = .empty,
    active_count: usize = 0,
}

fn stringLessThan(a: string, b: string): bool {
    return std.mem.order(u8, a, b) == .lt;
}

fn sortStrings(items: []string) {
    zig {
        std.sort.pdq([]const u8, items, {}, struct {
            fn lessThan(_: void, a: []const u8, b: []const u8) bool {
                return stringLessThan(a, b);
            }
        }.lessThan);
    }
}

/// Observed-Remove Set (`OrSet`) CRDT for string elements.
/// Every `add(elem, node_id)` allocates a unique `(node_id, seq)` tag; `remove(elem)` tombstones
/// only the tags currently observed on the local replica, ensuring concurrent add-wins convergence.
pub struct OrSet {
    allocator: std.mem.Allocator = undefined,
    node_seqs: std.StringHashMapUnmanaged(u64) = .empty,
    entries: std.StringHashMapUnmanaged(OrSetBucket) = .empty,
    live_count: usize = 0,

    /// Initializes an empty `OrSet` backed by `alloc`.
    pub static fn init(alloc: std.mem.Allocator): OrSet {
        return OrSet{
            allocator: alloc,
            node_seqs: .empty,
            entries: .empty,
            live_count: 0,
        };
    }

    /// Releases all owned element keys, interned node IDs, and tag buckets.
    pub fn deinit(*_) {
        let entry_iter = _.entries.iterator();
        while (kv := entry_iter.next()) {
            kv.value_ptr.tags.deinit(_.allocator);
            _.allocator.free(kv.key_ptr.*);
        }
        _.entries.deinit(_.allocator);

        let node_iter = _.node_seqs.iterator();
        while (kv := node_iter.next()) {
            _.allocator.free(kv.key_ptr.*);
        }
        _.node_seqs.deinit(_.allocator);
        _.live_count = 0;
    }

    fn internNodeWithSeq(*_, node_id: string, seq: u64): !string {
        if (kv := _.node_seqs.getEntry(node_id)) {
            if (seq > kv.value_ptr.*) {
                kv.value_ptr.* = seq;
            }
            return kv.key_ptr.*;
        }
        const owned = _.allocator.dupe(u8, node_id) catch return CrdtErr.OutOfMemory;
        errdefer _.allocator.free(owned);
        _.node_seqs.put(_.allocator, owned, seq) catch return CrdtErr.OutOfMemory;
        return owned;
    }

    fn allocateNextTag(*_, node_id: string): !OrSetTag {
        if (kv := _.node_seqs.getEntry(node_id)) {
            if (kv.value_ptr.* == std.math.maxInt(u64)) {
                return CrdtErr.Overflow;
            }
            kv.value_ptr.* += 1;
            return OrSetTag{
                node_id: kv.key_ptr.*,
                seq: kv.value_ptr.*,
                removed: false,
            };
        }
        const owned = _.allocator.dupe(u8, node_id) catch return CrdtErr.OutOfMemory;
        errdefer _.allocator.free(owned);
        _.node_seqs.put(_.allocator, owned, 1) catch return CrdtErr.OutOfMemory;
        return OrSetTag{
            node_id: owned,
            seq: 1,
            removed: false,
        };
    }

    fn getOrCreateBucket(*_, elem: string): !(*OrSetBucket) {
        if (ptr := _.entries.getPtr(elem)) {
            return ptr;
        }
        const owned_elem = _.allocator.dupe(u8, elem) catch return CrdtErr.OutOfMemory;
        errdefer _.allocator.free(owned_elem);
        _.entries.put(_.allocator, owned_elem, OrSetBucket{
            tags: .empty,
            active_count: 0,
        }) catch return CrdtErr.OutOfMemory;
        return _.entries.getPtr(owned_elem).?;
    }

    /// Adds `elem` with a fresh unique `(node_id, seq)` tag generated for `node_id`.
    /// Rejects empty `node_id` with `CrdtErr.InvalidParameter`.
    pub fn add(*_, elem: string, node_id: string): ! {
        if (node_id.len == 0) {
            return CrdtErr.InvalidParameter;
        }
        const tag = _.allocateNextTag(node_id)?;
        const bucket = _.getOrCreateBucket(elem)?;
        bucket.tags.append(_.allocator, tag) catch return CrdtErr.OutOfMemory;
        if (bucket.active_count == 0) {
            _.live_count += 1;
        }
        bucket.active_count += 1;
    }

    /// Removes `elem` by tombstoning all tags currently observed for `elem` on this replica.
    /// Returns `true` if `elem` had at least one active tag, `false` otherwise.
    pub fn remove(*_, elem: string): bool {
        if (bucket := _.entries.getPtr(elem)) {
            if (bucket.active_count == 0) {
                return false;
            }
            let i: usize = 0;
            while (i < bucket.tags.items.len) : (i += 1) {
                bucket.tags.items[i].removed = true;
            }
            bucket.active_count = 0;
            _.live_count -= 1;
            return true;
        }
        return false;
    }

    /// Returns `true` if `elem` currently has at least one non-tombstoned tag.
    pub fn contains(elem: string): bool {
        if (bucket := _.entries.get(elem)) {
            return bucket.active_count > 0;
        }
        return false;
    }

    /// Returns the number of distinct active elements in the set.
    pub fn len(): usize {
        return _.live_count;
    }

    /// Returns a deterministically sorted slice of all active elements.
    /// Caller owns the returned slice (free with `alloc.free`); string elements are borrowed from `OrSet`.
    pub fn elements(alloc: std.mem.Allocator): ([]string)! {
        let out = alloc.alloc(string, _.live_count) catch return CrdtErr.OutOfMemory;
        if (_.live_count == 0) return out;

        let idx: usize = 0;
        let iter = _.entries.iterator();
        while (kv := iter.next()) {
            if (kv.value_ptr.active_count > 0) {
                out[idx] = kv.key_ptr.*;
                idx += 1;
            }
        }
        sortStrings(out);
        return out;
    }

    /// Merges `other` into `this` via tag-set union and tombstone propagation (`removed = self || other`).
    /// Concurrent `add` vs `remove` strictly preserves newly added unseen tags (add-wins).
    pub fn merge(*_, other: OrSet): ! {
        let node_iter = other.node_seqs.iterator();
        while (nkv := node_iter.next()) {
            * = _.internNodeWithSeq(nkv.key_ptr.*, nkv.value_ptr.*)?;
        }

        let entry_iter = other.entries.iterator();
        while (ekv := entry_iter.next()) {
            const elem = ekv.key_ptr.*;
            const other_bucket = ekv.value_ptr.*;
            const self_bucket = _.getOrCreateBucket(elem)?;
            const was_live = self_bucket.active_count > 0;

            let i: usize = 0;
            while (i < other_bucket.tags.items.len) : (i += 1) {
                const otag = other_bucket.tags.items[i];

                let matched = false;
                let j: usize = 0;
                while (j < self_bucket.tags.items.len) : (j += 1) {
                    const stag = self_bucket.tags.items[j];
                    if (stag.seq == otag.seq && std.mem.eql(u8, stag.node_id, otag.node_id)) {
                        matched = true;
                        if (otag.removed && !stag.removed) {
                            self_bucket.tags.items[j].removed = true;
                            self_bucket.active_count -= 1;
                        }
                        break;
                    }
                }

                if (!matched) {
                    const interned = _.internNodeWithSeq(otag.node_id, otag.seq)?;
                    self_bucket.tags.append(_.allocator, OrSetTag{
                        node_id: interned,
                        seq: otag.seq,
                        removed: otag.removed,
                    }) catch return CrdtErr.OutOfMemory;
                    if (!otag.removed) {
                        self_bucket.active_count += 1;
                    }
                }
            }

            const is_live = self_bucket.active_count > 0;
            if (!was_live && is_live) {
                _.live_count += 1;
            } else if (was_live && !is_live) {
                _.live_count -= 1;
            }
        }
    }

    /// Creates an ownership-independent deep clone of this `OrSet` using `alloc`.
    pub fn clone(alloc: std.mem.Allocator): !OrSet {
        let copy = OrSet.init(alloc);
        errdefer copy.deinit();

        let node_iter = _.node_seqs.iterator();
        while (nkv := node_iter.next()) {
            * = copy.internNodeWithSeq(nkv.key_ptr.*, nkv.value_ptr.*)?;
        }

        let entry_iter = _.entries.iterator();
        while (ekv := entry_iter.next()) {
            const dst_bucket = copy.getOrCreateBucket(ekv.key_ptr.*)?;
            const src_bucket = ekv.value_ptr.*;
            let i: usize = 0;
            while (i < src_bucket.tags.items.len) : (i += 1) {
                const src_tag = src_bucket.tags.items[i];
                const interned = copy.internNodeWithSeq(src_tag.node_id, src_tag.seq)?;
                dst_bucket.tags.append(alloc, OrSetTag{
                    node_id: interned,
                    seq: src_tag.seq,
                    removed: src_tag.removed,
                }) catch return CrdtErr.OutOfMemory;
                if (!src_tag.removed) {
                    dst_bucket.active_count += 1;
                }
            }
            if (dst_bucket.active_count > 0) {
                copy.live_count += 1;
            }
        }
        return copy;
    }
}
