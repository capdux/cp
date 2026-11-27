// Type definitions for CP

export class GetResult {
    ok: bool,
    val: u64,
}

export class EvictResult {
    idx: i32,
    key: u64,
    val: u64,
}

export class SlabEntry {
    occupied: bool,
    key: u64,
    val: u64,
    prev: i32,
    next: i32,
}

export class LruSlab {
    head: i32,
    tail: i32,
    free_head: i32,
    count: usize,
    capacity: usize,
    fn new(): LruSlab;
    fn init(_: auto, entries: []SlabEntry): bool;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn unlinkLru(_: auto, entries: []SlabEntry, idx: i32);
    fn pushFrontLru(_: auto, entries: []SlabEntry, idx: i32);
    fn insert(_: auto, entries: []SlabEntry, key: u64, val: u64): i32;
    fn touch(_: auto, entries: []SlabEntry, idx: i32): bool;
    fn get(_: auto, entries: []SlabEntry, idx: i32): GetResult;
    fn remove(_: auto, entries: []SlabEntry, idx: i32): bool;
    fn evictLru(_: auto, entries: []SlabEntry): EvictResult;
    fn peekMru(): i32;
    fn peekLru(): i32;
}

