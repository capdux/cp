// Source: cp/lib/cache/lru.cp (O(1) LRU + TTL In-Memory Cache)
// High-performance O(1) LRU + TTL in-memory cache with ownership-safe key copies
// and intrusive doubly-linked slot pool for AI Agent response/prompt caching.

const std = @import("std");

const NULL_IDX: usize = std.math.maxInt(usize);

/// Error set shared across `cap:cache` data structures.
pub err CacheErr {
    OutOfMemory,
    InvalidCapacity,
    InvalidParameter,
    SizeMismatch,
}

/// Snapshot of cache hit/miss/eviction/expiration telemetry.
pub struct CacheStats {
    pub hits: u64 = 0;
    pub misses: u64 = 0;
    pub evictions: u64 = 0;
    pub expirations: u64 = 0;
    pub len: usize = 0;
    pub capacity: usize = 0;

    /// Returns cache hit ratio in `[0.0, 1.0]`. Returns `0.0` when no lookups have occurred.
    pub fn hitRate(): f64 {
        const total = _.hits + _.misses;
        if (total == 0) return 0.0;
        return (_.hits as f64) / (total as f64);
    }

    /// Returns total lookup requests (`hits + misses`).
    pub fn totalRequests(): u64 {
        return _.hits + _.misses;
    }
}

/// Internal slot node stored inside the contiguous LRU slot pool.
pub struct LruNode<V> {
    pub key: []u8 = undefined;
    pub value: V = undefined;
    pub prev: usize = NULL_IDX;
    pub next: usize = NULL_IDX;
    pub has_ttl: bool = false;
    pub expires_at_ms: u64 = 0;
    pub occupied: bool = false;
}

/// O(1) LRU + TTL in-memory cache keyed by owned UTF-8/byte strings.
/// Combines `std.StringHashMapUnmanaged(usize)` with an intrusive doubly-linked list
/// over a pre-allocated slot pool (`0..capacity`).
pub struct LruCache<V> {
    allocator: std.mem.Allocator,
    nodes: []LruNode<V>,
    map: std.StringHashMapUnmanaged(usize) = .empty,
    max_cap: usize = 0,
    count: usize = 0,
    head: usize = NULL_IDX,
    tail: usize = NULL_IDX,
    free_head: usize = NULL_IDX,
    hit_count: u64 = 0,
    miss_count: u64 = 0,
    eviction_count: u64 = 0,
    expiration_count: u64 = 0,

    /// Initializes an `LruCache<V>` with fixed `max_capacity > 0`.
    pub static fn init(alloc: std.mem.Allocator, max_capacity: usize): LruCache<V>! {
        if (max_capacity == 0) return CacheErr.InvalidCapacity;

        let nodes = alloc.alloc(LruNode(V), max_capacity) catch return CacheErr.OutOfMemory;
        let i: usize = 0;
        while (i < max_capacity) : (i += 1) {
            nodes[i] = LruNode<V>{
                key: undefined,
                value: undefined,
                prev: NULL_IDX,
                next: if (i + 1 < max_capacity) i + 1 else NULL_IDX,
                has_ttl: false,
                expires_at_ms: 0,
                occupied: false,
            };
        }

        return LruCache<V>{
            allocator: alloc,
            nodes: nodes,
            map: .empty,
            max_cap: max_capacity,
            count: 0,
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
        _.count = 0;
        _.head = NULL_IDX;
        _.tail = NULL_IDX;
        _.free_head = NULL_IDX;
    }

    /// Returns the number of entries currently stored (including uncollected expired entries).
    pub fn len(): usize {
        return _.count;
    }

    /// Returns the maximum entry capacity of the cache.
    pub fn capacity(): usize {
        return _.max_cap;
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

    fn releaseSlot(*_, idx: usize) {
        const owned_key = _.nodes[idx].key;
        * = _.map.remove(owned_key);
        _.allocator.free(owned_key);
        _.nodes[idx].key = undefined;
        _.nodes[idx].value = undefined;
        _.nodes[idx].occupied = false;
        _.nodes[idx].has_ttl = false;
        _.nodes[idx].expires_at_ms = 0;
        _.detach(idx);
        _.nodes[idx].next = _.free_head;
        _.free_head = idx;
        _.count -= 1;
    }

    fn insertInternal(*_, key: []const u8, value: V, has_ttl: bool, expires_at_ms: u64): ! {
        if (existing_idx := _.map.get(key)) {
            _.nodes[existing_idx].value = value;
            _.nodes[existing_idx].has_ttl = has_ttl;
            _.nodes[existing_idx].expires_at_ms = expires_at_ms;
            _.moveToFront(existing_idx);
            return;
        }

        const owned_key = _.allocator.dupe(u8, key) catch return CacheErr.OutOfMemory;
        errdefer _.allocator.free(owned_key);

        if (_.free_head == NULL_IDX) {
            if (_.tail == NULL_IDX) return CacheErr.InvalidCapacity;
            _.releaseSlot(_.tail);
            _.eviction_count += 1;
        }

        const slot = _.free_head;
        _.free_head = _.nodes[slot].next;

        _.map.put(_.allocator, owned_key, slot) catch {
            _.nodes[slot].next = _.free_head;
            _.free_head = slot;
            return CacheErr.OutOfMemory;
        };

        _.nodes[slot] = LruNode<V>{
            key: owned_key,
            value: value,
            prev: NULL_IDX,
            next: NULL_IDX,
            has_ttl: has_ttl,
            expires_at_ms: expires_at_ms,
            occupied: true,
        };
        _.pushFront(slot);
        _.count += 1;
    }

    /// Inserts or updates `key -> value` with no TTL expiration and promotes it to MRU.
    /// Evicts the least-recently-used entry in O(1) if the cache is at capacity.
    pub fn put(*_, key: []const u8, value: V): ! {
        return _.insertInternal(key, value, false, 0);
    }

    /// Inserts or updates `key -> value` with a deterministic TTL (`expires_at_ms = now_ms +| ttl_ms`)
    /// and promotes it to MRU.
    pub fn putWithTtl(*_, key: []const u8, value: V, ttl_ms: u64, now_ms: u64): ! {
        const expires_at = now_ms +| ttl_ms;
        return _.insertInternal(key, value, true, expires_at);
    }

    /// Retrieves `key` in O(1), promotes it to MRU, and increments `hits`/`misses` telemetry.
    pub fn get(*_, key: []const u8): V? {
        if (idx := _.map.get(key)) {
            _.moveToFront(idx);
            _.hit_count += 1;
            return _.nodes[idx].value;
        }
        _.miss_count += 1;
        return null;
    }

    /// Deterministic TTL-aware O(1) lookup at `now_ms`.
    /// Lazily evicts an expired entry (`now_ms >= expires_at_ms`), increments `expirations` and `misses`,
    /// and returns `null`. Otherwise promotes to MRU, increments `hits`, and returns the value.
    pub fn getAt(*_, key: []const u8, now_ms: u64): V? {
        if (idx := _.map.get(key)) {
            if (_.nodes[idx].has_ttl && now_ms >= _.nodes[idx].expires_at_ms) {
                _.releaseSlot(idx);
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

    /// Looks up `key` in O(1) without altering LRU order or updating hit/miss telemetry.
    pub fn peek(key: []const u8): V? {
        if (idx := _.map.get(key)) {
            return _.nodes[idx].value;
        }
        return null;
    }

    /// TTL-aware read-only peek at `now_ms`; returns `null` if missing or expired without mutating state.
    pub fn peekAt(key: []const u8, now_ms: u64): V? {
        if (idx := _.map.get(key)) {
            if (_.nodes[idx].has_ttl && now_ms >= _.nodes[idx].expires_at_ms) {
                return null;
            }
            return _.nodes[idx].value;
        }
        return null;
    }

    /// Returns `true` if `key` is currently present in the cache without altering LRU order or stats.
    pub fn contains(key: []const u8): bool {
        return _.map.contains(key);
    }

    /// Returns `true` if `key` is present and not expired at `now_ms`, without mutating state.
    pub fn containsAt(key: []const u8, now_ms: u64): bool {
        if (idx := _.map.get(key)) {
            if (_.nodes[idx].has_ttl && now_ms >= _.nodes[idx].expires_at_ms) {
                return false;
            }
            return true;
        }
        return false;
    }

    /// Removes `key` from the cache in O(1), freeing its owned key copy. Returns `true` if removed.
    pub fn remove(*_, key: []const u8): bool {
        if (idx := _.map.get(key)) {
            _.releaseSlot(idx);
            return true;
        }
        return false;
    }

    /// Actively sweeps and evicts all entries expired at `now_ms` (`now_ms >= expires_at_ms`).
    /// Returns the number of entries expired and removed.
    pub fn evictExpired(*_, now_ms: u64): usize {
        let removed: usize = 0;
        let curr = _.head;
        while (curr != NULL_IDX) {
            const next_idx = _.nodes[curr].next;
            if (_.nodes[curr].has_ttl && now_ms >= _.nodes[curr].expires_at_ms) {
                _.releaseSlot(curr);
                _.expiration_count += 1;
                removed += 1;
            }
            curr = next_idx;
        }
        return removed;
    }

    /// Removes all entries while retaining the allocated slot pool and map capacity.
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < _.max_cap) : (i += 1) {
            if (_.nodes[i].occupied) {
                _.allocator.free(_.nodes[i].key);
            }
            _.nodes[i] = LruNode<V>{
                key: undefined,
                value: undefined,
                prev: NULL_IDX,
                next: if (i + 1 < _.max_cap) i + 1 else NULL_IDX,
                has_ttl: false,
                expires_at_ms: 0,
                occupied: false,
            };
        }
        _.map.clearRetainingCapacity();
        _.count = 0;
        _.head = NULL_IDX;
        _.tail = NULL_IDX;
        _.free_head = if (_.max_cap > 0) 0 else NULL_IDX;
    }

    /// Returns a snapshot of current cache telemetry and occupancy.
    pub fn stats(): CacheStats {
        return CacheStats{
            hits: _.hit_count,
            misses: _.miss_count,
            evictions: _.eviction_count,
            expirations: _.expiration_count,
            len: _.count,
            capacity: _.max_cap,
        };
    }
}
