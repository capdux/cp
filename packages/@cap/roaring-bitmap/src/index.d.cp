// Type definitions for CP

export fn rbmHigh16(val: u32): u16;
export fn rbmLow16(val: u32): u16;
export fn rbmCombine(hi: u16, lo: u16): u32;
export class RoaringBitmap {
    chunk_keys: [16]u16,
    chunk_lens: [16]usize,
    vals: [1024]u16,
    chunk_count: usize,
    total_len: u64,
    fn new(): RoaringBitmap;
    fn len(): u64;
    fn isEmpty(): bool;
    fn contains(val: u32): bool;
    fn insert(_: auto, val: u32): bool;
    fn remove(_: auto, val: u32): bool;
    fn min(): u32?;
    fn max(): u32?;
    fn rank(val: u32): u64;
    fn clear(_: auto);
    fn insertRange(_: auto, start: u32, end: u32): u64;
    fn unionWith(_: auto, other: RoaringBitmap);
    fn intersectWith(_: auto, other: RoaringBitmap);
    fn differenceWith(_: auto, other: RoaringBitmap);
    fn isSubset(other: RoaringBitmap): bool;
    fn isDisjoint(other: RoaringBitmap): bool;
}

