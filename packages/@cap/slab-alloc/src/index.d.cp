// Type definitions for CP

export class SlabEntryI64 {
    occupied: bool,
    val: i64,
    next_free: usize,
}

export class SlabI64 {
    entries: [128]SlabEntryI64,
    count: usize,
    next_free: usize,
    watermark: usize,
    fn new(): SlabI64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn vacantKey(): usize?;
    fn insert(_: auto, val: i64): usize?;
    fn contains(key: usize): bool;
    fn get(key: usize): i64?;
    fn set(_: auto, key: usize, val: i64): bool;
    fn remove(_: auto, key: usize): i64?;
    fn clear(_: auto);
    fn nextOccupied(from_key: usize): usize?;
}

