// Type definitions for CP

export fn lowbit(x: usize): usize;
export class FenwickTree {
    tree: [TREE_LEN]i64,
    length: usize,
    fn init(size: usize): FenwickTree;
    fn fromSlice(values: &[]i64): FenwickTree;
    fn len(): usize;
    fn add(_: auto, idx: usize, delta: i64): bool;
    fn prefixSum(end_exclusive: usize): i64;
    fn rangeSum(start: usize, end_exclusive: usize): i64;
    fn get(idx: usize): i64;
    fn set(_: auto, idx: usize, value: i64): bool;
    fn total(): i64;
    fn lowerBound(target: i64): usize;
    fn reset(_: auto);
}

