// @cap/lru — Zero-Allocation O(1) LRU Cache with TTL & Weight Eviction
// Clean-room pure-CP implementation inspired by npm `lru-cache` and Rust `lru` crate (MIT License).
//
// Architecture:
// - Fixed-capacity inline slot pool (`slots: [MAX_CAPACITY]LruEntry = undefined`) — zero startup zeroing overhead.
// - Intrusive doubly-linked list (`prev`/`next` u16 indices) — O(1) promote, evict, and remove with zero heap allocation.
// - Open-addressing hash index (FNV-1a 64-bit + linear probing + tombstone + automatic rehash) — O(1) key lookup.
// - Inline key/value storage (`MAX_KEY_LEN = 64`, `MAX_VAL_LEN = 256`) — safe ownership with zero dangling slices.

pub const MAX_CAPACITY: usize = 256;
pub const HASH_BUCKETS: usize = 512;
pub const MAX_KEY_LEN: usize = 64;
pub const MAX_VAL_LEN: usize = 256;

const HASH_MASK: usize = HASH_BUCKETS - 1;
const REHASH_THRESHOLD: usize = 384; // 75% load factor (entries + tombstones)
const NONE_IDX: u16 = 0xFFFF;
const BUCKET_EMPTY: u16 = 0xFFFF;
const BUCKET_TOMBSTONE: u16 = 0xFFFE;

const FNV_OFFSET_BASIS: u64 = ((0xcbf29ce4 as u64) << 32) | 0x84222325;
const FNV_PRIME: u64 = 0x100000001b3;

fn fnv1a64(data: []const u8): u64 {
    let h: u64 = FNV_OFFSET_BASIS;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        h ^= (data[i] as u64);
        h *%= FNV_PRIME;
    }
    return h;
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn copyBytes(dst: []u8, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[i] = src[i];
    }
}

/// Configuration for `LruCache.init`.
pub struct LruConfig {
    /// Maximum number of entries stored (clamped to `0..=MAX_CAPACITY`).
    max_entries: usize = MAX_CAPACITY,
    /// Optional maximum cumulative weight (0 = unlimited).
    max_weight: u64 = 0,
    /// Optional default TTL in nanoseconds (0 = no expiration).
    default_ttl_ns: u64 = 0,
}

/// Runtime observability metrics returned by `LruCache.stats()`.
pub struct LruStats {
    hits: u64 = 0,
    misses: u64 = 0,
    evictions: u64 = 0,
    expirations: u64 = 0,
    len: usize = 0,
    total_weight: u64 = 0,

    /// Cache hit ratio in `[0.0, 1.0]`; returns `0.0` if no lookups have occurred.
    pub fn hitRate(): f64 {
        const total = _.hits + _.misses;
        if (total == 0) return 0.0;
        return (_.hits as f64) / (total as f64);
    }
}

/// Internal fixed-size slot storing an inline key/value pair and intrusive list links.
struct LruEntry {
    key_buf: [MAX_KEY_LEN]u8,
    val_buf: [MAX_VAL_LEN]u8,
    key_len: u16,
    val_len: u16,
    hash: u64,
    weight: u64,
    expires_at_ns: u64,
    prev: u16,
    next: u16,
    bucket: u16,
    occupied: bool,
}

/// Industrial-grade, zero-allocation, O(1) LRU cache with TTL and byte-weight eviction.
pub struct LruCache {
    slots: [MAX_CAPACITY]LruEntry = undefined,
    buckets: [HASH_BUCKETS]u16 = [BUCKET_EMPTY; HASH_BUCKETS],
    max_entries: usize = MAX_CAPACITY,
    max_weight: u64 = 0,
    default_ttl_ns: u64 = 0,
    head: u16 = NONE_IDX,
    tail: u16 = NONE_IDX,
    free_head: u16 = NONE_IDX,
    allocated: u16 = 0,
    count: usize = 0,
    tombstones: u16 = 0,
    total_weight: u64 = 0,
    hits: u64 = 0,
    misses: u64 = 0,
    evictions: u64 = 0,
    expirations: u64 = 0,

    /// Initializes a new `LruCache` with the given configuration.
    /// `max_entries` is clamped to `MAX_CAPACITY` (256).
    pub static fn init(cfg: LruConfig): LruCache {
        const cap = if (cfg.max_entries > MAX_CAPACITY) MAX_CAPACITY else cfg.max_entries;
        return LruCache{
            slots: undefined,
            buckets: [BUCKET_EMPTY; HASH_BUCKETS],
            max_entries: cap,
            max_weight: cfg.max_weight,
            default_ttl_ns: cfg.default_ttl_ns,
            head: NONE_IDX,
            tail: NONE_IDX,
            free_head: NONE_IDX,
            allocated: 0,
            count: 0,
            tombstones: 0,
            total_weight: 0,
            hits: 0,
            misses: 0,
            evictions: 0,
            expirations: 0,
        };
    }

    fn isExpired(idx: u16, now_ns: u64): bool {
        const exp = _.slots[idx].expires_at_ns;
        return exp != 0 && now_ns >= exp;
    }

    fn findSlot(key: []const u8, hash: u64): u16? {
        let pos: usize = (hash as usize) & HASH_MASK;
        let probed: usize = 0;
        while (probed < HASH_BUCKETS) : (probed += 1) {
            const b = _.buckets[pos];
            if (b == BUCKET_EMPTY) return null;
            if (b != BUCKET_TOMBSTONE) {
                const s_hash = _.slots[b].hash;
                const s_klen = _.slots[b].key_len as usize;
                if (s_hash == hash && s_klen == key.len) {
                    if (bytesEqual(_.slots[b].key_buf[0..s_klen], key)) {
                        return b;
                    }
                }
            }
            pos = (pos + 1) & HASH_MASK;
        }
        return null;
    }

    fn rehash(*_) {
        let i: usize = 0;
        while (i < HASH_BUCKETS) : (i += 1) {
            _.buckets[i] = BUCKET_EMPTY;
        }
        _.tombstones = 0;

        let cur = _.head;
        while (cur != NONE_IDX) {
            let pos: usize = (_.slots[cur].hash as usize) & HASH_MASK;
            while (_.buckets[pos] != BUCKET_EMPTY) {
                pos = (pos + 1) & HASH_MASK;
            }
            _.buckets[pos] = cur;
            _.slots[cur].bucket = pos as u16;
            cur = _.slots[cur].next;
        }
    }

    fn insertBucket(*_, slot_idx: u16, hash: u64) {
        if ((_.count + (_.tombstones as usize)) >= REHASH_THRESHOLD) {
            _.rehash();
        }
        let pos: usize = (hash as usize) & HASH_MASK;
        let probed: usize = 0;
        while (probed < HASH_BUCKETS) : (probed += 1) {
            const b = _.buckets[pos];
            if (b == BUCKET_EMPTY || b == BUCKET_TOMBSTONE) {
                if (b == BUCKET_TOMBSTONE) {
                    _.tombstones -= 1;
                }
                _.buckets[pos] = slot_idx;
                _.slots[slot_idx].bucket = pos as u16;
                return;
            }
            pos = (pos + 1) & HASH_MASK;
        }
    }

    fn pushFront(*_, idx: u16) {
        _.slots[idx].prev = NONE_IDX;
        _.slots[idx].next = _.head;
        if (_.head != NONE_IDX) {
            _.slots[_.head].prev = idx;
        } else {
            _.tail = idx;
        }
        _.head = idx;
    }

    fn unlinkNode(*_, idx: u16) {
        const p = _.slots[idx].prev;
        const n = _.slots[idx].next;
        if (p != NONE_IDX) {
            _.slots[p].next = n;
        } else {
            _.head = n;
        }
        if (n != NONE_IDX) {
            _.slots[n].prev = p;
        } else {
            _.tail = p;
        }
        _.slots[idx].prev = NONE_IDX;
        _.slots[idx].next = NONE_IDX;
    }

    fn moveToFront(*_, idx: u16) {
        if (_.head == idx) return;
        _.unlinkNode(idx);
        _.pushFront(idx);
    }

    fn allocSlot(*_): u16? {
        if (_.free_head != NONE_IDX) {
            const idx = _.free_head;
            _.free_head = _.slots[idx].next;
            return idx;
        }
        if ((_.allocated as usize) < _.max_entries) {
            const idx = _.allocated;
            _.allocated += 1;
            return idx;
        }
        return null;
    }

    fn unlinkAndFree(*_, idx: u16) {
        const b_pos = _.slots[idx].bucket as usize;
        if (b_pos < HASH_BUCKETS && _.buckets[b_pos] == idx) {
            _.buckets[b_pos] = BUCKET_TOMBSTONE;
            _.tombstones += 1;
        }
        _.unlinkNode(idx);
        _.total_weight -= _.slots[idx].weight;
        _.count -= 1;
        _.slots[idx].occupied = false;
        _.slots[idx].bucket = BUCKET_EMPTY;
        _.slots[idx].next = _.free_head;
        _.free_head = idx;
    }

    fn evictTail(*_, now_ns: u64): bool {
        if (_.tail == NONE_IDX) return false;
        const idx = _.tail;
        const expired = _.isExpired(idx, now_ns);
        _.unlinkAndFree(idx);
        if (expired) {
            _.expirations += 1;
        } else {
            _.evictions += 1;
        }
        return true;
    }

    /// Inserts or updates a key-value pair using the cache's `default_ttl_ns` and automatic byte weight (`key.len + val.len`).
    /// Returns `false` if `max_entries == 0`, `key` is empty/too long, `val` is too long, or the entry exceeds `max_weight`.
    pub fn put(*_, key: []const u8, val: []const u8, now_ns: u64): bool {
        return _.putWithOptions(key, val, _.default_ttl_ns, 0, now_ns);
    }

    /// Inserts or updates a key-value pair with explicit TTL (`ttl_ns`, 0 = no expiration) and `weight` (0 = `key.len + val.len`).
    pub fn putWithOptions(*_, key: []const u8, val: []const u8, ttl_ns: u64, weight: u64, now_ns: u64): bool {
        if (_.max_entries == 0 || key.len == 0 || key.len > MAX_KEY_LEN || val.len > MAX_VAL_LEN) {
            return false;
        }
        const eff_weight: u64 = if (weight > 0) weight else ((key.len + val.len) as u64);
        if (_.max_weight > 0 && eff_weight > _.max_weight) {
            return false;
        }

        const hash = fnv1a64(key);
        const expires_at: u64 = if (ttl_ns > 0) (now_ns +| ttl_ns) else 0;

        const existing = _.findSlot(key, hash);
        if (existing != null) {
            const idx = existing.?;
            if (_.isExpired(idx, now_ns)) {
                _.expirations += 1;
            }
            _.total_weight -= _.slots[idx].weight;
            copyBytes(_.slots[idx].val_buf[0..val.len], val);
            _.slots[idx].val_len = val.len as u16;
            _.slots[idx].weight = eff_weight;
            _.slots[idx].expires_at_ns = expires_at;
            _.total_weight += eff_weight;
            _.moveToFront(idx);

            if (_.max_weight > 0) {
                while (_.total_weight > _.max_weight && _.tail != NONE_IDX && _.tail != idx) {
                    * = _.evictTail(now_ns);
                }
            }
            return true;
        }

        // Evict LRU entries to make room for capacity and weight constraints.
        if (_.count >= _.max_entries) {
            * = _.evictTail(now_ns);
        }
        if (_.max_weight > 0) {
            while (_.count > 0 && (_.total_weight + eff_weight) > _.max_weight) {
                * = _.evictTail(now_ns);
            }
        }

        const slot_opt = _.allocSlot();
        if (slot_opt == null) return false;
        const idx = slot_opt.?;

        copyBytes(_.slots[idx].key_buf[0..key.len], key);
        copyBytes(_.slots[idx].val_buf[0..val.len], val);
        _.slots[idx].key_len = key.len as u16;
        _.slots[idx].val_len = val.len as u16;
        _.slots[idx].hash = hash;
        _.slots[idx].weight = eff_weight;
        _.slots[idx].expires_at_ns = expires_at;
        _.slots[idx].prev = NONE_IDX;
        _.slots[idx].next = NONE_IDX;
        _.slots[idx].bucket = BUCKET_EMPTY;
        _.slots[idx].occupied = true;

        _.insertBucket(idx, hash);
        _.pushFront(idx);
        _.count += 1;
        _.total_weight += eff_weight;
        return true;
    }

    /// Looks up `key`, promoting it to MRU on hit and lazily evicting it if expired.
    pub fn get(*_, key: []const u8, now_ns: u64): ([]const u8)? {
        if (key.len == 0 || key.len > MAX_KEY_LEN || _.count == 0) {
            _.misses += 1;
            return null;
        }
        const hash = fnv1a64(key);
        const slot_opt = _.findSlot(key, hash);
        if (slot_opt == null) {
            _.misses += 1;
            return null;
        }
        const idx = slot_opt.?;
        if (_.isExpired(idx, now_ns)) {
            _.unlinkAndFree(idx);
            _.expirations += 1;
            _.misses += 1;
            return null;
        }
        _.moveToFront(idx);
        _.hits += 1;
        const vlen = _.slots[idx].val_len as usize;
        return _.slots[idx].val_buf[0..vlen];
    }

    /// Read-only lookup: returns the value if present and unexpired without altering LRU order or stats.
    pub fn peek(key: []const u8, now_ns: u64): ([]const u8)? {
        if (key.len == 0 || key.len > MAX_KEY_LEN || _.count == 0) {
            return null;
        }
        const hash = fnv1a64(key);
        const slot_opt = _.findSlot(key, hash);
        if (slot_opt == null) return null;
        const idx = slot_opt.?;
        if (_.isExpired(idx, now_ns)) return null;
        const vlen = _.slots[idx].val_len as usize;
        return _.slots[idx].val_buf[0..vlen];
    }

    /// Returns `true` if `key` exists and is not expired at `now_ns` (does not alter LRU order).
    pub fn contains(key: []const u8, now_ns: u64): bool {
        return _.peek(key, now_ns) != null;
    }

    /// Removes `key` from the cache if present. Returns `true` if removed.
    pub fn remove(*_, key: []const u8): bool {
        if (key.len == 0 || key.len > MAX_KEY_LEN || _.count == 0) {
            return false;
        }
        const hash = fnv1a64(key);
        const slot_opt = _.findSlot(key, hash);
        if (slot_opt == null) return false;
        _.unlinkAndFree(slot_opt.?);
        return true;
    }

    /// Explicitly evicts the least-recently-used entry. Returns `true` if an entry was evicted.
    pub fn popLru(*_): bool {
        if (_.tail == NONE_IDX) return false;
        _.unlinkAndFree(_.tail);
        _.evictions += 1;
        return true;
    }

    /// Scans active entries and removes all entries expired at `now_ns`. Returns the number removed.
    pub fn purgeExpired(*_, now_ns: u64): usize {
        let purged: usize = 0;
        let cur = _.head;
        while (cur != NONE_IDX) {
            const next_idx = _.slots[cur].next;
            if (_.isExpired(cur, now_ns)) {
                _.unlinkAndFree(cur);
                _.expirations += 1;
                purged += 1;
            }
            cur = next_idx;
        }
        return purged;
    }

    /// Clears all cached entries and resets weight to 0 (preserves cumulative stats).
    pub fn clear(*_) {
        let i: usize = 0;
        while (i < HASH_BUCKETS) : (i += 1) {
            _.buckets[i] = BUCKET_EMPTY;
        }
        _.head = NONE_IDX;
        _.tail = NONE_IDX;
        _.free_head = NONE_IDX;
        _.allocated = 0;
        _.count = 0;
        _.tombstones = 0;
        _.total_weight = 0;
    }

    /// Resets cumulative hit/miss/eviction/expiration counters to zero.
    pub fn resetStats(*_) {
        _.hits = 0;
        _.misses = 0;
        _.evictions = 0;
        _.expirations = 0;
    }

    /// Returns the number of entries currently stored.
    pub fn len(): usize {
        return _.count;
    }

    /// Returns `true` if the cache contains no entries.
    pub fn isEmpty(): bool {
        return _.count == 0;
    }

    /// Returns the configured maximum entry capacity.
    pub fn capacity(): usize {
        return _.max_entries;
    }

    /// Returns the current cumulative weight of all stored entries.
    pub fn totalWeight(): u64 {
        return _.total_weight;
    }

    /// Returns a snapshot of cache performance statistics.
    pub fn stats(): LruStats {
        return LruStats{
            hits: _.hits,
            misses: _.misses,
            evictions: _.evictions,
            expirations: _.expirations,
            len: _.count,
            total_weight: _.total_weight,
        };
    }

    /// Returns the key of the most-recently-used entry, or `null` if empty.
    pub fn mostRecentKey(): ([]const u8)? {
        if (_.head == NONE_IDX) return null;
        const klen = _.slots[_.head].key_len as usize;
        return _.slots[_.head].key_buf[0..klen];
    }

    /// Returns the key of the least-recently-used entry, or `null` if empty.
    pub fn leastRecentKey(): ([]const u8)? {
        if (_.tail == NONE_IDX) return null;
        const klen = _.slots[_.tail].key_len as usize;
        return _.slots[_.tail].key_buf[0..klen];
    }
}
