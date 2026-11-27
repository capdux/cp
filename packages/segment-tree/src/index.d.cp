// Type definitions for CP

export class RangeStats {
    sum: i64,
    min_val: i64,
    max_val: i64,
    valid: bool,
}

export fn minI64(a: i64, b: i64): i64;
export fn maxI64(a: i64, b: i64): i64;
export class SegmentTree {
    size: usize,
    sums: [TREE_CAP]i64,
    mins: [TREE_CAP]i64,
    maxs: [TREE_CAP]i64,
    fn init(n: usize): SegmentTree;
    fn fromSlice(values: &[]i64): SegmentTree;
    fn len(): usize;
    fn get(idx: usize): i64;
    fn set(_: auto, idx: usize, value: i64): bool;
    fn add(_: auto, idx: usize, delta: i64): bool;
    fn query(start: usize, end_exclusive: usize): RangeStats;
    fn rangeSum(start: usize, end_exclusive: usize): i64;
    fn rangeMin(start: usize, end_exclusive: usize): i64;
    fn rangeMax(start: usize, end_exclusive: usize): i64;
}

