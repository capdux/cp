// Type definitions for CP

export class ThinVecI64 {
    len_val: usize,
    data: [64]i64,
    fn new(): ThinVecI64;
    fn fromSlice(src: []i64): ThinVecI64;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
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
    fn dedup(_: auto): usize;
    fn extendFromSlice(_: auto, src: []i64): usize;
}

