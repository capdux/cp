// Type definitions for CP

export class SmallPopU64 {
    ok: bool,
    value: u64,
}

export class SmallVecU64 {
    data: [64]u64,
    used: usize,
    fn new(): SmallVecU64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn clear(_: auto);
    fn truncate(_: auto, new_len: usize);
    fn get(idx: usize): SmallPopU64;
    fn set(_: auto, idx: usize, val: u64): bool;
    fn push(_: auto, val: u64): bool;
    fn pop(_: auto): SmallPopU64;
    fn insert(_: auto, idx: usize, val: u64): bool;
    fn remove(_: auto, idx: usize): SmallPopU64;
    fn swapRemove(_: auto, idx: usize): SmallPopU64;
    fn extendFromSlice(_: auto, src: &[]u64): bool;
    fn dedup(_: auto);
    fn reverse(_: auto);
    fn contains(needle: u64): bool;
}

export class SmallVecU8 {
    data: [256]u8,
    used: usize,
    fn new(): SmallVecU8;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn push(_: auto, b: u8): bool;
    fn extendFromSlice(_: auto, src: &[]u8): bool;
    fn copyTo(out: []u8): usize;
}

