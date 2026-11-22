// Source: cp/lib/lru/lru.cp (O(1) LRU Cache with TTL Expiration, Byte-Weight Limit & Hit Stats)
// Pure CP generic O(1) LRU cache with owned key copies, deterministic monotonic TTL (`now_ms: i64`),
// configurable byte-weight capacity limit, dynamic resize, and hit/miss/eviction/expiration telemetry.

const std = @import("std");

const NULL_IDX: usize = std.math.maxInt(usize);

/// Error set for `cap:lru` operations.
pub err LruError {
    InvalidCapacity,
    InvalidWeight,
    EntryTooLarge,
    OutOfMemory,
}

/// Snapshot of cache occupancy, weight usage, and hit/miss/eviction/expiration telemetry.
pub struct LruStats {
    pub len: usize = 0;
    pub capacity: usize = 0;
    pub current_weight: usize = 0;
    pub max_weight: usize = 0;
    pub hits: u64 = 0;
    pub misses: u64 = 0;
    pub evictions: u64 = 0;
    pub expirations: u64 = 0;

    /// Returns cache hit ratio in `[0.0, 1.0]`. Returns `0.0` when no lookups have occurred.
    pub fn hitRate(): f64 {
        const total = _.hits + _.misses;
        if (total == 0) return 0.0;
        return (_.hits as f64) / (total as f64);
    }
}

/// Internal slot node stored inside the contiguous LRU slot pool.
struct LruNode<V> {
    key: []u8 = undefined;
    value: V = undefined;
    weight: usize = 0;
    prev: usize = NULL_IDX;
    next: usize = NULL_IDX;
    has_ttl: bool = false;
    expires_at_ms: i64 = 0;
    occupied: bool = false;
}

/// O(1) LRU cache with deterministic TTL expiration and optional byte-weight limit.
/// Combines `std.StringHashMapUnmanaged(usize)` with an intrusive doubly-linked list
/// over a slot pool (`0..capacity`), owning deep copies of all keys.
pub struct LruCache<V> {
    allocator: std.mem.Allocator,
    nodes: []LruNode<V>,
    map: std.StringHashMapUnmanaged(usize) = .empty,
    max_cap: usize = 0,
    max_weight_limit: usize = 0,
    count: usize = 0,
    total_weight: usize = 0,
    head: usize = NULL_IDX,
    tail: usize = NULL_IDX,
    free_head: usize = NULL_IDX,
    hit_count: u64 = 0,
    miss_count: u64 = 0,
    eviction_count: u64 = 0,
    expiration_count: u64 = 0,

    /// Initializes an `LruCache<V>` with `max_entries > 0` and `max_weight` (`0` = unlimited weight).
    pub static fn init(allocator: std.mem.Allocator, max_entries: usize, max_weight: usize): LruCache<V>! {
        if (max_entries == 0) return LruError.InvalidCapacity;

        let nodes = allocator.alloc(LruNode(V), max_entries) catch return LruError.OutOfMemory;
        let i: usize = 0;
        while (i < max_entries) : (i += 1) {
            nodes[i] = LruNode<V>{
                key: undefined,
                value: undefined,
                weight: 0,
                prev: NULL_IDX,
                next: if (i + 1 < max_entries) i + 1 else NULL_IDX,
                has_ttl: false,
                expires_at_ms: 0,
                occupied: false,
            };
        }

        return LruCache<V>{
            allocator: allocator,
            nodes: nodes,
            map: .empty,
            max_cap: max_entries,
            max_weight_limit: max_weight,
            count: 0,
            total_weight: 0,
            head: NULL_IDX,
            tail: NULL_IDX,
            free_head: 0,
            hit_count: 0,
            miss_count: 0,
            eviction_count: 0,
            expiration_count: 0,
        };
    }

    /// Releases all owned keys, the index map, and the backing slot pool.
    pub fn deinit(*_) {
        let i: usize = 0;
        while (i < _.max_cap) : (i += 1) {
            if (_.nodes[i].occupied) {
                _.allocator.free(_.nodes[i].key);
                _.nodes[i].occupied = false;
            }
        }
        _.map.deinit(_.allocator);
        if (_.max_cap > 0) {
            _.allocator.free(_.nodes);
        }
        _.nodes = [];
        _.max_cap = 0;
        _.max_weight_limit = 0;
        _.count = 0;
        _.total_weight = 0;
        _.head = NULL_IDX;
        _.tail = NULL_IDX;
        _.free_head = NULL_IDX;
    }

    /// Returns the number of entries currently stored.
    pub fn len(): usize {
        return _.count;
    }

    /// Returns the maximum entry capacity of the cache.
    pub fn capacity(): usize {
        return _.max_cap;
    }

    /// Returns the sum of weights of all entries currently stored.
    pub fn currentWeight(): usize {
        return _.total_weight;
    }

    /// Returns the configured maximum weight limit (`0` = unlimited).
    pub fn maxWeight(): usize {
        return _.max_weight_limit;
    }

    /// Returns `true` if the cache currently stores zero entries.
    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    fn detach(*_, idx: usize) {
        const p = _.nodes[idx].prev;
        const n = _.nodes[idx].next;
        if (p != NULL_IDX) {
            _.nodes[p].next = n;
        } else {
            _.head = n;
        }
        if (n != NULL_IDX) {
            _.nodes[n].prev = p;
        } else {
            _.tail = p;
        }
        _.nodes[idx].prev = NULL_IDX;
        _.nodes[idx].next = NULL_IDX;
    }

    fn pushFront(*_, idx: usize) {
        _.nodes[idx].prev = NULL_IDX;
        _.nodes[idx].next = _.head;
        if (_.head != NULL_IDX) {
            _.nodes[_.head].prev = idx;
        } else {
            _.tail = idx;
        }
        _.head = idx;
    }

    fn moveToFront(*_, idx: usize) {
        if (_.head == idx) return;
        _.detach(idx);
        _.pushFront(idx);
    }

    fn freeSlot(*_, idx: usize) {
        const owned_key = _.nodes[idx].key;
        * = _.map.remove(owned_key);
        _.allocator.free(owned_key);
        _.total_weight -= _.nodes[idx].weight;
        _.nodes[idx].key = undefined;
        _.nodes[idx].value = undefined;
        _.nodes[idx].weight = 0;
        _.nodes[idx].occupied = false;
        _.nodes[idx].has_ttl = false;
        _.nodes[idx].expires_at_ms = 0;
        _.detach(idx);
        _.nodes[idx].next = _.free_head;
        _.free_head = idx;
        _.count -= 1;
    }

    /// Inserts or updates `key -> value` with default weight `1` and no TTL expiration.
    pub fn put(*_, key: []const u8, value: V): ! {
        return _.putWeighted(key, value, 1, 0, 0);
    }

    /// Inserts or updates `key -> value` with explicit `weight` (`> 0`) and optional TTL (`ttl_ms > 0`).
    /// Returns `LruError.EntryTooLarge` if `max_weight > 0` and `weight > max_weight`.
    /// Evicts least-recently-used entries until `len <= max_entries` and `current_weight <= max_weight`.
    pub fn putWeighted(*_, key: []const u8, value: V, weight: usize, ttl_ms: i64, now_ms: i64): ! {
        if (weight == 0) return LruError.InvalidWeight;
        if (_.max_weight_limit > 0 && weight > _.max_weight_limit) {
            return LruError.EntryTooLarge;
        }

        const has_ttl = ttl_ms > 0;
        const expires_at: i64 = if (has_ttl) (now_ms +| ttl_ms) else 0;

        if (existing_idx := _.map.get(key)) {
            const old_weight = _.nodes[existing_idx].weight;
            _.total_weight = _.total_weight - old_weight + weight;
            _.nodes[existing_idx].value = value;
            _.nodes[existing_idx].weight = weight;
            _.nodes[existing_idx].has_ttl = has_ttl;
            _.nodes[existing_idx].expires_at_ms = expires_at;
            _.moveToFront(existing_idx);

            while (_.max_weight_limit > 0 && _.total_weight > _.max_weight_limit && _.tail != NULL_IDX) {
                _.freeSlot(_.tail);
                _.eviction_count += 1;
            }
            return;
        }

        const owned_key = _.allocator.dupe(u8, key) catch return LruError.OutOfMemory;
        errdefer _.allocator.free(owned_key);

        while (_.free_head == NULL_IDX || (_.max_weight_limit > 0 && _.total_weight + weight > _.max_weight_limit && _.tail != NULL_IDX)) {
            if (_.tail == NULL_IDX) return LruError.InvalidCapacity;
            _.freeSlot(_.tail);
            _.eviction_count += 1;
        }

        const slot = _.free_head;
        _.free_head = _.nodes[slot].next;

        _.map.put(_.allocator, owned_key, slot) catch {
            _.nodes[slot].next = _.free_head;
            _.free_head = slot;
            return LruError.OutOfMemory;
        };

        _.nodes[slot] = LruNode<V>{
            key: owned_key,
            value: value,
            weight: weight,
            prev: NULL_IDX,
            next: NULL_IDX,
            has_ttl: has_ttl,
            expires_at_ms: expires_at,
            occupied: true,
        };
        _.pushFront(slot);
        _.count += 1;
        _.total_weight += weight;
    }

    /// Retrieves `key` at `now_ms` in O(1).
    /// Lazily removes expired entries (`now_ms >= expires_at_ms`), incrementing `expirations` and `misses`.
    /// On hit, promotes the entry to MRU and increments `hits`.
    pub fn get(*_, key: []const u8, now_ms: i64): V? {
        if (idx := _.map.get(key)) {
            if (_.nodes[idx].has_ttl && now_ms >= _.nodes[idx].expires_at_ms) {
                _.freeSlot(idx);
                _.expiration_count += 1;
                _.miss_count += 1;
                return null;
            }
            _.moveToFront(idx);
            _.hit_count += 1;
            return _.nodes[idx].value;
        }
        _.miss_count += 1;
        return null;
    }

    /// Read-only O(1) lookup at `now_ms`. Does not alter LRU order or hit/miss telemetry.
    pub fn peek(key: []const u8, now_ms: i64): V? {
        if (idx := _.map.get(key)) {
            if (_.nodes[idx].has_ttl && now_ms >= _.nodes[idx].expires_at_ms) {
                return null;
            }
            return _.nodes[idx].value;
        }
        return null;
    }

    /// Returns `true` if `key` exists and is not expired at `now_ms`, without altering LRU order or stats.
    pub fn contains(key: []const u8, now_ms: i64): bool {
        if (idx := _.map.get(key)) {
            if (_.nodes[idx].has_ttl && now_ms >= _.nodes[idx].expires_at_ms) {
                return false;
            }
            return true;
        }
        return false;
    }

    /// Removes `key` from the cache in O(1), freeing its owned key copy and returning its value if present.
    pub fn removeEntry(*_, key: []const u8): V? {
        if (idx := _.map.get(key)) {
            const val = _.nodes[idx].value;
            _.freeSlot(idx);
            return val;
        }
        return null;
    }

    /// Actively sweeps and removes all entries expired at `now_ms` (`now_ms >= expires_at_ms`).
    /// Returns the number of expired entries removed.
    pub fn purgeExpired(*_, now_ms: i64): usize {
        let removed: usize = 0;
        let curr = _.head;
        while (curr != NULL_IDX) {
            const next_idx = _.nodes[curr].next;
            if (_.nodes[curr].has_ttl && now_ms >= _.nodes[curr].expires_at_ms) {
                _.freeSlot(curr);
                _.expiration_count += 1;
                removed += 1;
            }
            curr = next_idx;
        }
        return removed;
    }

    /// Resizes entry capacity and weight limit, evicting LRU entries as needed to fit the new limits.
    pub fn resize(*_, max_entries: usize, max_weight: usize): ! {
        if (max_entries == 0) return LruError.InvalidCapacity;

        if (max_entries == _.max_cap) {
            _.max_weight_limit = max_weight;
            while (_.max_weight_limit > 0 && _.total_weight > _.max_weight_limit && _.tail != NULL_IDX) {
                _.freeSlot(_.tail);
                _.eviction_count += 1;
            }
            return;
        }

        let new_nodes = _.allocator.alloc(LruNode(V), max_entries) catch return LruError.OutOfMemory;

        while (_.count > max_entries || (max_weight > 0 && _.total_weight > max_weight && _.tail != NULL_IDX)) {
            _.freeSlot(_.tail);
            _.eviction_count += 1;
        }

        let new_idx: usize = 0;
        let curr = _.head;
        while (curr != NULL_IDX) {
            const next_idx = _.nodes[curr].next;
            new_nodes[new_idx] = LruNode<V>{
                key: _.nodes[curr].key,
                value: _.nodes[curr].value,
                weight: _.nodes[curr].weight,
                prev: if (new_idx == 0) NULL_IDX else new_idx - 1,
                next: if (new_idx + 1 < _.count) new_idx + 1 else NULL_IDX,
                has_ttl: _.nodes[curr].has_ttl,
                expires_at_ms: _.nodes[curr].expires_at_ms,
                occupied: true,
            };
            if (ptr := _.map.getPtr(_.nodes[curr].key)) {
                ptr.* = new_idx;
            }
            new_idx += 1;
            curr = next_idx;
        }

        let f_idx = _.count;
        while (f_idx < max_entries) : (f_idx += 1) {
            new_nodes[f_idx] = LruNode<V>{
                key: undefined,
                value: undefined,
                weight: 0,
                prev: NULL_IDX,
                next: if (f_idx + 1 < max_entries) f_idx + 1 else NULL_IDX,
                has_ttl: false,
                expires_at_ms: 0,
                occupied: false,
            };
        }

        if (_.max_cap > 0) {
            _.allocator.free(_.nodes);
        }
        _.nodes = new_nodes;
        _.max_cap = max_entries;
        _.max_weight_limit = max_weight;
        _.head = if (_.count > 0) 0 else NULL_IDX;
        _.tail = if (_.count > 0) _.count - 1 else NULL_IDX;
        _.free_head = if (_.count < max_entries) _.count else NULL_IDX;
    }

    /// Removes all entries while retaining allocated slot pool and map capacity.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.max_cap) : (i += 1) {
            if (_.nodes[i].occupied) {
                _.allocator.free(_.nodes[i].key);
            }
            _.nodes[i] = LruNode<V>{
                key: undefined,
                value: undefined,
                weight: 0,
                prev: NULL_IDX,
                next: if (i + 1 < _.max_cap) i + 1 else NULL_IDX,
                has_ttl: false,
                expires_at_ms: 0,
                occupied: false,
            };
        }
        _.map.clearRetainingCapacity();
        _.count = 0;
        _.total_weight = 0;
        _.head = NULL_IDX;
        _.tail = NULL_IDX;
        _.free_head = if (_.max_cap > 0) 0 else NULL_IDX;
    }

    /// Returns a snapshot of current cache occupancy, weights, and hit/miss/eviction/expiration stats.
    pub fn stats(): LruStats {
        return LruStats{
            len: _.count,
            capacity: _.max_cap,
            current_weight: _.total_weight,
            max_weight: _.max_weight_limit,
            hits: _.hit_count,
            misses: _.miss_count,
            evictions: _.eviction_count,
            expirations: _.expiration_count,
        };
    }
}
