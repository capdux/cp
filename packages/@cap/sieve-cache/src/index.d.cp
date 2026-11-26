// Type definitions for CP

export class SieveNode {
    key: u64,
    value: i64,
    visited: bool,
    in_use: bool,
    prev: u16,
    next: u16,
}

export fn emptyNode(): SieveNode;
export fn mulWrap64(a: u64, b: u64): u64;
export fn hashSlot(key: u64): usize;
export class SieveCache {
    nodes: [128]SieveNode,
    buckets: [256]u16,
    free_stack: [128]u16,
    free_top: usize,
    head: u16,
    tail: u16,
    hand: u16,
    cap: usize,
    count: usize,
    tomb_count: usize,
    hit_count: u64,
    miss_count: u64,
    eviction_count: u64,
    fn init(max_items: usize): SieveCache;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn hits(): u64;
    fn misses(): u64;
    fn evictions(): u64;
    fn findNodeIdx(key: u64): u16;
    fn htInsert(_: auto, key: u64, node_idx: u16);
    fn htRemove(_: auto, key: u64);
    fn rehash(_: auto);
    fn unlinkNode(_: auto, idx: u16);
    fn linkAtHead(_: auto, idx: u16);
    fn evictOne(_: auto);
    fn contains(key: u64): bool;
    fn peek(key: u64): i64;
    fn get(_: auto, key: u64): i64;
    fn put(_: auto, key: u64, value: i64): bool;
    fn remove(_: auto, key: u64): bool;
    fn clear(_: auto);
}

