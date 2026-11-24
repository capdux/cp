// Reference-hardened against cessen/ropey (MIT) & crossbeam-rs/crossbeam (MIT OR Apache-2.0)
// Source: cp/lib/skiplist/skiplist.cp (Ordered SkipList MemTable & Range Scan Index)
// Pure CP generic ordered SkipList MemTable with deterministic SplitMix64 level generation,
// contiguous node pool with free-slot recycling, owned key copies, O(log N) lookup/upsert/removal,
// lowerBound/upperBound/predecessor/successor/floor, strict [start, end) rangeScan, and invariant verifier.

const std = @import("std");

const NULL_IDX: usize = std.math.maxInt(usize);
pub const SKIPLIST_MAX_LEVEL: usize = 32;
pub const SKIPLIST_DEFAULT_MAX_LEVEL: usize = 16;
const MAX_ALLOWED_LEVEL: usize = SKIPLIST_MAX_LEVEL;
const DEFAULT_MAX_LEVEL: usize = SKIPLIST_DEFAULT_MAX_LEVEL;
const DEFAULT_SEED: u64 = (@as(u64, 0x9e3779b9) << 32) | 0x7f4a7c15;
const SPLITMIX_MULT1: u64 = (@as(u64, 0xbf58476d) << 32) | 0x1ce4e5b9;
const SPLITMIX_MULT2: u64 = (@as(u64, 0x94d049bb) << 32) | 0x133111eb;

/// Error set for `cap:skiplist` operations.
pub err SkipListError {
    InvalidMaxLevel,
    InvalidRange,
    OutOfMemory,
}

/// Read-only key-value entry view returned by `lowerBound`, `upperBound`, `predecessor`, `successor`,
/// `floor`, `first`, `last`, and `rangeScan`.
pub struct SkipEntry<V> {
    pub key: []const u8,
    pub value: V,
}

/// Internal skip list node stored inside the contiguous node pool.
struct SkipNode<V> {
    key: []const u8,
    value: V,
    forward: []usize,
    next_free: usize,
    occupied: bool,
}

/// Generic ordered SkipList MemTable with deterministic SplitMix64 height generation
/// and deep-copied byte-slice keys for dangling-pointer safety.
pub struct SkipList<V> {
    allocator: std.mem.Allocator,
    nodes: std.ArrayListUnmanaged(SkipNode(V)),
    head_idx: usize,
    tail_idx: usize,
    free_head: usize,
    max_level: usize,
    level: usize,
    count: usize,
    rng_state: u64,

    /// Initializes a `SkipList<V>` with default `max_level = 16` and default SplitMix64 seed.
    pub static fn init(allocator: std.mem.Allocator): SkipList<V> ! SkipListError {
        return SkipList<V>.initWithSeed(allocator, DEFAULT_MAX_LEVEL, DEFAULT_SEED);
    }

    /// Initializes a `SkipList<V>`, clamping `requested_max_level` into `1..=SKIPLIST_MAX_LEVEL` (32).
    pub static fn initClamped(allocator: std.mem.Allocator, requested_max_level: usize, seed: u64): SkipList<V> ! SkipListError {
        let clamped = requested_max_level;
        if (clamped == 0) {
            clamped = 1;
        } else if (clamped > MAX_ALLOWED_LEVEL) {
            clamped = MAX_ALLOWED_LEVEL;
        }
        return SkipList<V>.initWithSeed(allocator, clamped, seed);
    }

    /// Initializes a `SkipList<V>` with explicit `max_level` in `1..=32` and deterministic `seed`.
    pub static fn initWithSeed(allocator: std.mem.Allocator, max_level: usize, seed: u64): SkipList<V> ! SkipListError {
        if (max_level == 0 || max_level > MAX_ALLOWED_LEVEL) {
            return SkipListError.InvalidMaxLevel;
        }

        let nodes = std.ArrayListUnmanaged(SkipNode(V)).empty;
        errdefer nodes.deinit(allocator);

        let head_forward = allocator.alloc(usize, max_level) catch return SkipListError.OutOfMemory;
        errdefer allocator.free(head_forward);

        let i: usize = 0;
        while (i < max_level) : (i += 1) {
            head_forward[i] = NULL_IDX;
        }

        nodes.append(allocator, SkipNode<V>{
            key: "",
            value: undefined,
            forward: head_forward,
            next_free: NULL_IDX,
            occupied: true,
        }) catch return SkipListError.OutOfMemory;

        return SkipList<V>{
            allocator: allocator,
            nodes: nodes,
            head_idx: 0,
            tail_idx: NULL_IDX,
            free_head: NULL_IDX,
            max_level: max_level,
            level: 1,
            count: 0,
            rng_state: if (seed == 0) DEFAULT_SEED else seed,
        };
    }

    /// Releases all entries, owned keys, forward pointer arrays, and the backing node pool.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.nodes.items.len) : (i += 1) {
            if (_.nodes.items[i].occupied) {
                if (i > 0) {
                    _.allocator.free(_.nodes.items[i].key);
                }
                _.allocator.free(_.nodes.items[i].forward);
                _.nodes.items[i].occupied = false;
            }
        }
        _.nodes.deinit(_.allocator);
        _.head_idx = 0;
        _.tail_idx = NULL_IDX;
        _.free_head = NULL_IDX;
        _.level = 1;
        _.count = 0;
    }

    fn nextRandomU64(*_): u64 {
        _.rng_state = _.rng_state +% DEFAULT_SEED;
        let z: u64 = _.rng_state;
        z = (z ^ (z >> 30)) *% SPLITMIX_MULT1;
        z = (z ^ (z >> 27)) *% SPLITMIX_MULT2;
        return z ^ (z >> 31);
    }

    fn randomLevel(*_): usize {
        const cap_lvl = if (_.max_level > MAX_ALLOWED_LEVEL) MAX_ALLOWED_LEVEL else _.max_level;
        let lvl: usize = 1;
        let r = _.nextRandomU64();
        while (lvl < cap_lvl && (r & 1) == 0) {
            lvl += 1;
            r >>= 1;
        }
        return lvl;
    }

    fn findPredecessorIdx(key: []const u8): usize {
        if (_.nodes.items.len == 0) return _.head_idx;
        let curr = _.head_idx;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (true) {
                const next_idx = _.nodes.items[curr].forward[i];
                if (next_idx == NULL_IDX) break;
                if (std.mem.order(u8, _.nodes.items[next_idx].key, key) == .lt) {
                    curr = next_idx;
                } else {
                    break;
                }
            }
        }
        return curr;
    }

    fn findFirstGreaterOrEqual(key: []const u8): usize {
        if (_.nodes.items.len == 0) return NULL_IDX;
        const pred = _.findPredecessorIdx(key);
        return _.nodes.items[pred].forward[0];
    }

    /// Inserts a new key-value pair or overwrites an existing key's value.
    /// Returns `true` if `key` was newly inserted, or `false` if an existing key was updated.
    pub fn upsert(*_, key: []const u8, value: V): bool ! SkipListError {
        if (_.nodes.items.len == 0) return SkipListError.OutOfMemory;

        let update: [MAX_ALLOWED_LEVEL]usize = undefined;
        let u_idx: usize = 0;
        while (u_idx < MAX_ALLOWED_LEVEL) : (u_idx += 1) {
            update[u_idx] = _.head_idx;
        }

        let curr = _.head_idx;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (true) {
                const next_idx = _.nodes.items[curr].forward[i];
                if (next_idx == NULL_IDX) break;
                if (std.mem.order(u8, _.nodes.items[next_idx].key, key) == .lt) {
                    curr = next_idx;
                } else {
                    break;
                }
            }
            update[i] = curr;
        }

        const cand_idx = _.nodes.items[curr].forward[0];
        if (cand_idx != NULL_IDX && std.mem.order(u8, _.nodes.items[cand_idx].key, key) == .eq) {
            _.nodes.items[cand_idx].value = value;
            return false;
        }

        const new_lvl = _.randomLevel();
        if (new_lvl > _.level) {
            let lvl_idx: usize = _.level;
            while (lvl_idx < new_lvl) : (lvl_idx += 1) {
                update[lvl_idx] = _.head_idx;
            }
            _.level = new_lvl;
        }

        const owned_key = _.allocator.dupe(u8, key) catch return SkipListError.OutOfMemory;
        errdefer _.allocator.free(owned_key);

        let forward = _.allocator.alloc(usize, new_lvl) catch return SkipListError.OutOfMemory;
        errdefer _.allocator.free(forward);

        let new_idx: usize = NULL_IDX;
        if (_.free_head != NULL_IDX) {
            new_idx = _.free_head;
            _.free_head = _.nodes.items[new_idx].next_free;
            _.nodes.items[new_idx] = SkipNode<V>{
                key: owned_key,
                value: value,
                forward: forward,
                next_free: NULL_IDX,
                occupied: true,
            };
        } else {
            new_idx = _.nodes.items.len;
            _.nodes.append(_.allocator, SkipNode<V>{
                key: owned_key,
                value: value,
                forward: forward,
                next_free: NULL_IDX,
                occupied: true,
            }) catch return SkipListError.OutOfMemory;
        }

        let k: usize = 0;
        while (k < new_lvl) : (k += 1) {
            const prev_idx = update[k];
            _.nodes.items[new_idx].forward[k] = _.nodes.items[prev_idx].forward[k];
            _.nodes.items[prev_idx].forward[k] = new_idx;
        }

        if (_.nodes.items[new_idx].forward[0] == NULL_IDX) {
            _.tail_idx = new_idx;
        }

        _.count += 1;
        return true;
    }

    /// Looks up `key` in O(log N) expected time. Returns `null` if absent.
    pub fn get(key: []const u8): V? {
        const idx = _.findFirstGreaterOrEqual(key);
        if (idx != NULL_IDX && std.mem.order(u8, _.nodes.items[idx].key, key) == .eq) {
            return _.nodes.items[idx].value;
        }
        return null;
    }

    /// Returns `true` if `key` exists in the skip list.
    pub fn contains(key: []const u8): bool {
        return _.get(key) != null;
    }

    /// Removes `key` from the skip list, relinks predecessor pointers, frees node memory,
    /// and returns the removed value (or `null` if `key` was not present).
    pub fn removeKey(*_, key: []const u8): V? {
        if (_.nodes.items.len == 0 || _.count == 0) return null;

        let update: [MAX_ALLOWED_LEVEL]usize = undefined;
        let u_idx: usize = 0;
        while (u_idx < MAX_ALLOWED_LEVEL) : (u_idx += 1) {
            update[u_idx] = _.head_idx;
        }

        let curr = _.head_idx;
        let i: usize = _.level;
        while (i > 0) {
            i -= 1;
            while (true) {
                const next_idx = _.nodes.items[curr].forward[i];
                if (next_idx == NULL_IDX) break;
                if (std.mem.order(u8, _.nodes.items[next_idx].key, key) == .lt) {
                    curr = next_idx;
                } else {
                    break;
                }
            }
            update[i] = curr;
        }

        const target_idx = _.nodes.items[curr].forward[0];
        if (target_idx == NULL_IDX || std.mem.order(u8, _.nodes.items[target_idx].key, key) != .eq) {
            return null;
        }

        const removed_value = _.nodes.items[target_idx].value;
        const target_lvl = _.nodes.items[target_idx].forward.len;
        let k: usize = 0;
        while (k < target_lvl) : (k += 1) {
            const prev_idx = update[k];
            if (_.nodes.items[prev_idx].forward[k] != target_idx) break;
            _.nodes.items[prev_idx].forward[k] = _.nodes.items[target_idx].forward[k];
        }

        if (_.tail_idx == target_idx) {
            const prev0 = update[0];
            _.tail_idx = if (prev0 == _.head_idx) NULL_IDX else prev0;
        }

        _.allocator.free(_.nodes.items[target_idx].key);
        _.allocator.free(_.nodes.items[target_idx].forward);
        _.nodes.items[target_idx].key = "";
        _.nodes.items[target_idx].value = undefined;
        _.nodes.items[target_idx].forward = [];
        _.nodes.items[target_idx].occupied = false;
        _.nodes.items[target_idx].next_free = _.free_head;
        _.free_head = target_idx;

        while (_.level > 1 && _.nodes.items[_.head_idx].forward[_.level - 1] == NULL_IDX) {
            _.level -= 1;
        }

        _.count -= 1;
        return removed_value;
    }

    /// Returns the first entry with `entry.key >= key`, or `null` if none exists.
    pub fn lowerBound(key: []const u8): SkipEntry<V>? {
        const idx = _.findFirstGreaterOrEqual(key);
        if (idx != NULL_IDX) {
            return SkipEntry<V>{
                key: _.nodes.items[idx].key,
                value: _.nodes.items[idx].value,
            };
        }
        return null;
    }

    /// Returns the first entry with `entry.key > key`, or `null` if none exists.
    pub fn upperBound(key: []const u8): SkipEntry<V>? {
        if (_.nodes.items.len == 0) return null;
        let idx = _.findFirstGreaterOrEqual(key);
        if (idx != NULL_IDX && std.mem.order(u8, _.nodes.items[idx].key, key) == .eq) {
            idx = _.nodes.items[idx].forward[0];
        }
        if (idx != NULL_IDX) {
            return SkipEntry<V>{
                key: _.nodes.items[idx].key,
                value: _.nodes.items[idx].value,
            };
        }
        return null;
    }

    /// Returns the strict successor entry (`entry.key > key`), or `null` if none exists.
    pub fn successor(key: []const u8): SkipEntry<V>? {
        return _.upperBound(key);
    }

    /// Returns the strict predecessor entry (`entry.key < key`), or `null` if none exists.
    pub fn predecessor(key: []const u8): SkipEntry<V>? {
        if (_.nodes.items.len == 0 || _.count == 0) return null;
        const pred_idx = _.findPredecessorIdx(key);
        if (pred_idx != _.head_idx) {
            return SkipEntry<V>{
                key: _.nodes.items[pred_idx].key,
                value: _.nodes.items[pred_idx].value,
            };
        }
        return null;
    }

    /// Returns the greatest entry with `entry.key <= key`, or `null` if none exists.
    pub fn floor(key: []const u8): SkipEntry<V>? {
        if (_.nodes.items.len == 0 || _.count == 0) return null;
        const pred_idx = _.findPredecessorIdx(key);
        const next_idx = _.nodes.items[pred_idx].forward[0];
        if (next_idx != NULL_IDX && std.mem.order(u8, _.nodes.items[next_idx].key, key) == .eq) {
            return SkipEntry<V>{
                key: _.nodes.items[next_idx].key,
                value: _.nodes.items[next_idx].value,
            };
        }
        if (pred_idx != _.head_idx) {
            return SkipEntry<V>{
                key: _.nodes.items[pred_idx].key,
                value: _.nodes.items[pred_idx].value,
            };
        }
        return null;
    }

    /// Returns the smallest-key entry in the skip list, or `null` when empty.
    pub fn first(): SkipEntry<V>? {
        if (_.nodes.items.len == 0) return null;
        const idx = _.nodes.items[_.head_idx].forward[0];
        if (idx != NULL_IDX) {
            return SkipEntry<V>{
                key: _.nodes.items[idx].key,
                value: _.nodes.items[idx].value,
            };
        }
        return null;
    }

    /// Returns the largest-key entry in the skip list in O(1), or `null` when empty.
    pub fn last(): SkipEntry<V>? {
        if (_.tail_idx != NULL_IDX) {
            return SkipEntry<V>{
                key: _.nodes.items[_.tail_idx].key,
                value: _.nodes.items[_.tail_idx].value,
            };
        }
        return null;
    }

    /// Returns all entries with `start_inclusive <= entry.key < end_exclusive` in strict
    /// lexicographical order. Caller owns the returned slice and must free it with `allocator.free`.
    /// Returns `SkipListError.InvalidRange` if `start_inclusive > end_exclusive`.
    pub fn rangeScan(allocator: std.mem.Allocator, start_inclusive: []const u8, end_exclusive: []const u8): []SkipEntry<V> ! SkipListError {
        const range_ord = std.mem.order(u8, start_inclusive, end_exclusive);
        if (range_ord == .gt) {
            return SkipListError.InvalidRange;
        }

        let result = std.ArrayListUnmanaged(SkipEntry(V)).empty;
        errdefer result.deinit(allocator);

        if (range_ord == .eq || _.count == 0) {
            return result.toOwnedSlice(allocator) catch return SkipListError.OutOfMemory;
        }

        let curr = _.findFirstGreaterOrEqual(start_inclusive);
        while (curr != NULL_IDX) {
            if (std.mem.order(u8, _.nodes.items[curr].key, end_exclusive) != .lt) {
                break;
            }
            result.append(allocator, SkipEntry<V>{
                key: _.nodes.items[curr].key,
                value: _.nodes.items[curr].value,
            }) catch return SkipListError.OutOfMemory;
            curr = _.nodes.items[curr].forward[0];
        }

        return result.toOwnedSlice(allocator) catch return SkipListError.OutOfMemory;
    }

    /// Verifies strict monotonic key ordering, level bounds, count accuracy, and tail pointer invariants.
    pub fn verifyInvariants(): bool {
        if (_.nodes.items.len == 0) return _.count == 0 && _.tail_idx == NULL_IDX;
        if (_.max_level == 0 || _.max_level > MAX_ALLOWED_LEVEL) return false;
        if (_.level == 0 || _.level > _.max_level) return false;

        let seen: usize = 0;
        let curr = _.nodes.items[_.head_idx].forward[0];
        let prev_idx: usize = NULL_IDX;
        while (curr != NULL_IDX) {
            if (curr >= _.nodes.items.len || !_.nodes.items[curr].occupied) return false;
            if (_.nodes.items[curr].forward.len == 0 || _.nodes.items[curr].forward.len > _.max_level) return false;
            if (prev_idx != NULL_IDX) {
                if (std.mem.order(u8, _.nodes.items[prev_idx].key, _.nodes.items[curr].key) != .lt) {
                    return false;
                }
            }
            prev_idx = curr;
            seen += 1;
            if (seen > _.count) return false;
            curr = _.nodes.items[curr].forward[0];
        }
        if (seen != _.count) return false;
        if (_.count == 0) {
            if (_.tail_idx != NULL_IDX) return false;
        } else {
            if (_.tail_idx != prev_idx) return false;
        }

        // Verify higher levels remain strictly monotonic and point to occupied nodes
        let lvl: usize = 1;
        while (lvl < _.level) : (lvl += 1) {
            let c = _.nodes.items[_.head_idx].forward[lvl];
            let p: usize = NULL_IDX;
            let steps: usize = 0;
            while (c != NULL_IDX) {
                if (c >= _.nodes.items.len || !_.nodes.items[c].occupied) return false;
                if (_.nodes.items[c].forward.len <= lvl) return false;
                if (p != NULL_IDX) {
                    if (std.mem.order(u8, _.nodes.items[p].key, _.nodes.items[c].key) != .lt) {
                        return false;
                    }
                }
                p = c;
                steps += 1;
                if (steps > _.count) return false;
                c = _.nodes.items[c].forward[lvl];
            }
        }
        return true;
    }

    /// Returns the number of entries currently stored in the skip list.
    pub fn len(): usize {
        return _.count;
    }

    /// Returns `true` if the skip list currently contains zero entries.
    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Removes and frees all entries while retaining the sentinel head node and pool capacity for reuse.
    pub fn clear(*_) {
        if (_.nodes.items.len == 0) return;
        let i: usize = 1;
        while (i < _.nodes.items.len) : (i += 1) {
            if (_.nodes.items[i].occupied) {
                _.allocator.free(_.nodes.items[i].key);
                _.allocator.free(_.nodes.items[i].forward);
                _.nodes.items[i].occupied = false;
            }
        }
        _.nodes.shrinkRetainingCapacity(1);

        let j: usize = 0;
        while (j < _.max_level) : (j += 1) {
            _.nodes.items[_.head_idx].forward[j] = NULL_IDX;
        }
        _.tail_idx = NULL_IDX;
        _.free_head = NULL_IDX;
        _.level = 1;
        _.count = 0;
    }
}
