// Type definitions for CP

export fn fnv1a64(data: &[]u8): u64;
export fn bytesEqual(a: &[]u8, b: &[]u8): bool;
export fn copyBytes(dst: []u8, src: &[]u8);
export class LruConfig {
    max_entries: usize,
    max_weight: u64,
    default_ttl_ns: u64,
}

export class LruStats {
    hits: u64,
    misses: u64,
    evictions: u64,
    expirations: u64,
    len: usize,
    total_weight: u64,
    fn hitRate(): f64;
}

export class LruEntry {
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

export class LruCache {
    slots: [MAX_CAPACITY]LruEntry,
    buckets: [HASH_BUCKETS]u16,
    max_entries: usize,
    max_weight: u64,
    default_ttl_ns: u64,
    head: u16,
    tail: u16,
    free_head: u16,
    allocated: u16,
    count: usize,
    tombstones: u16,
    total_weight: u64,
    hits: u64,
    misses: u64,
    evictions: u64,
    expirations: u64,
    fn init(cfg: LruConfig): LruCache;
    fn isExpired(idx: u16, now_ns: u64): bool;
    fn findSlot(key: &[]u8, hash: u64): u16?;
    fn rehash(_: auto);
    fn insertBucket(_: auto, slot_idx: u16, hash: u64);
    fn pushFront(_: auto, idx: u16);
    fn unlinkNode(_: auto, idx: u16);
    fn moveToFront(_: auto, idx: u16);
    fn allocSlot(_: auto): u16?;
    fn unlinkAndFree(_: auto, idx: u16);
    fn evictTail(_: auto, now_ns: u64): bool;
    fn put(_: auto, key: &[]u8, val: &[]u8, now_ns: u64): bool;
    fn putWithOptions(_: auto, key: &[]u8, val: &[]u8, ttl_ns: u64, weight: u64, now_ns: u64): bool;
    fn get(_: auto, key: &[]u8, now_ns: u64): (&[]u8)?;
    fn peek(key: &[]u8, now_ns: u64): (&[]u8)?;
    fn contains(key: &[]u8, now_ns: u64): bool;
    fn remove(_: auto, key: &[]u8): bool;
    fn popLru(_: auto): bool;
    fn purgeExpired(_: auto, now_ns: u64): usize;
    fn clear(_: auto);
    fn resetStats(_: auto);
    fn len(): usize;
    fn isEmpty(): bool;
    fn capacity(): usize;
    fn totalWeight(): u64;
    fn stats(): LruStats;
    fn mostRecentKey(): (&[]u8)?;
    fn leastRecentKey(): (&[]u8)?;
}

