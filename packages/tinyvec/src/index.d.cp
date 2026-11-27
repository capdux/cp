// Type definitions for CP

export class TinyVecI64 {
    data: [64]i64,
    used: usize,
    fn new(): TinyVecI64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn isFull(): bool;
    fn get(idx: usize): i64?;
    fn first(): i64?;
    fn last(): i64?;
    fn push(_: auto, val: i64): bool;
    fn pop(_: auto): i64?;
    fn insert(_: auto, idx: usize, val: i64): bool;
    fn remove(_: auto, idx: usize): i64?;
    fn swapRemove(_: auto, idx: usize): i64?;
    fn truncate(_: auto, new_len: usize);
    fn clear(_: auto);
    fn reverse(_: auto);
    fn dedup(_: auto);
    fn extendFromSlice(_: auto, src: &[]i64): usize;
    fn binarySearch(target: i64): usize?;
    fn sum(): i64;
}

export class TinyVecU32 {
    data: [64]u32,
    used: usize,
    fn new(): TinyVecU32;
    fn len(): usize;
    fn isEmpty(): bool;
    fn push(_: auto, val: u32): bool;
    fn pop(_: auto): u32?;
    fn get(idx: usize): u32?;
    fn swapRemove(_: auto, idx: usize): u32?;
    fn clear(_: auto);
}

