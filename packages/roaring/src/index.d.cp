// Type definitions for CP

export fn highKey(v: u32): u16;
export fn lowVal(v: u32): u16;
export fn combineKeyVal(hi: u16, lo: u16): u32;
export class BinSearch {
    found: bool,
    pos: usize,
}

export class RoaringContainer {
    key: u16,
    values: [MAX_CONTAINER_ITEMS]u16,
    count: usize,
}

export fn emptyContainer(): RoaringContainer;
export fn searchU16(arr: [MAX_CONTAINER_ITEMS]u16, count: usize, target: u16): BinSearch;
export fn searchContainer(containers: [MAX_CONTAINERS]RoaringContainer, count: usize, key: u16): BinSearch;
export fn unionContainers(a: RoaringContainer, b: RoaringContainer): RoaringContainer;
export fn intersectContainers(a: RoaringContainer, b: RoaringContainer): RoaringContainer;
export fn diffContainers(a: RoaringContainer, b: RoaringContainer): RoaringContainer;
export class RoaringBitmap {
    containers: [MAX_CONTAINERS]RoaringContainer,
    container_count: usize,
    total_len: u64,
    fn init(): RoaringBitmap;
    fn insert(_: auto, value: u32): bool;
    fn remove(_: auto, value: u32): bool;
    fn contains(value: u32): bool;
    fn len(): u64;
    fn isEmpty(): bool;
    fn min(): i64;
    fn max(): i64;
    fn rank(value: u32): u64;
    fn unionWith(other: RoaringBitmap): RoaringBitmap;
    fn intersectWith(other: RoaringBitmap): RoaringBitmap;
    fn differenceWith(other: RoaringBitmap): RoaringBitmap;
    fn clear(_: auto);
}

