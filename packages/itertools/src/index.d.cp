// Type definitions for CP

export class MinMaxResult {
    empty: bool,
    min: i64,
    max: i64,
}

export class PartitionResult {
    true_count: usize,
    false_count: usize,
}

export class CartesianPair {
    found: bool,
    first: i64,
    second: i64,
}

export fn itCopyBytes(dst: []u8, pos: &mut usize, src: &[]u8): bool;
export fn minmax(slice: &[]i64): MinMaxResult;
export fn isSorted(slice: &[]i64): bool;
export fn isSortedDesc(slice: &[]i64): bool;
export fn allEqual(slice: &[]i64): bool;
export fn uniqueI64(src: &[]i64, out: []i64): usize;
export fn dedupConsecutive(src: &[]i64, out: []i64): usize;
export fn interleave(a: &[]i64, b: &[]i64, out: []i64): usize;
export fn intersperse(src: &[]i64, sep: i64, out: []i64): usize;
export fn mergeSorted(a: &[]i64, b: &[]i64, out: []i64): usize;
export fn joinStrings(items: &[]string, sep: string, out: []u8): usize;
export fn partitionByThreshold(src: &[]i64, threshold: i64, out_true: []i64, out_false: []i64): PartitionResult;
export fn cartesianProductCount(a_len: usize, b_len: usize): usize;
export fn nthCartesianPair(a: &[]i64, b: &[]i64, idx: usize): CartesianPair;
export fn chunkCount(total_len: usize, chunk_size: usize): usize;
export fn windowCount(total_len: usize, win_size: usize): usize;
