// Type definitions for CP

export fn mulWrap64(a: u64, b: u64): u64;
export fn avalanche64(x: u64): u64;
export fn hashBytes(data: &[]u8): u64;
export fn fingerprintFromHash(h: u64): u16;
export fn altBucket(bucket: usize, fp: u16): usize;
export class CuckooFilter {
    buckets: [1024]u16,
    item_count: usize,
    rng_state: u64,
    fn init(): CuckooFilter;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn loadFactor(): f64;
    fn clear(_: auto);
    fn nextRand(_: auto): u64;
    fn insertIntoBucket(_: auto, bucket: usize, fp: u16): bool;
    fn bucketContains(bucket: usize, fp: u16): bool;
    fn deleteFromBucket(_: auto, bucket: usize, fp: u16): bool;
    fn insert(_: auto, data: &[]u8): bool;
    fn contains(data: &[]u8): bool;
    fn delete(_: auto, data: &[]u8): bool;
}

