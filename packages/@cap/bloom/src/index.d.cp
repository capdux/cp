// Type definitions for CP

export fn makeU64(hi: u32, lo: u32): u64;
export fn addWrap64(a: u64, b: u64): u64;
export fn mulWrap64(a: u64, b: u64): u64;
export fn splitMix64(x_in: u64): u64;
export class HashPair {
    h1: u64,
    h2: u64,
}

export fn hashBytesPair(s: string): HashPair;
export fn hashU64Pair(val: u64): HashPair;
export fn bitMask64(bit: usize): u64;
export fn popcount64(x_in: u64): usize;
export fn clampHashes(k: u8): u8;
export fn expNeg(x_in: f64): f64;
export fn computeFpr(k: u8, n: usize, m: usize): f64;
export class BloomFilter1024 {
    words: [16]u64,
    num_hashes: u8,
    items: usize,
    fn init(num_hashes: u8): BloomFilter1024;
    fn insert(_: auto, item: string);
    fn mightContain(item: string): bool;
    fn insertU64(_: auto, val: u64);
    fn mightContainU64(val: u64): bool;
    fn bitCount(): usize;
    fn setBits(): usize;
    fn itemCount(): usize;
    fn clear(_: auto);
    fn unionWith(_: auto, other: BloomFilter1024);
    fn estimatedFpr(): f64;
}

export class BloomFilter4096 {
    words: [64]u64,
    num_hashes: u8,
    items: usize,
    fn init(num_hashes: u8): BloomFilter4096;
    fn insert(_: auto, item: string);
    fn mightContain(item: string): bool;
    fn insertU64(_: auto, val: u64);
    fn mightContainU64(val: u64): bool;
    fn bitCount(): usize;
    fn setBits(): usize;
    fn itemCount(): usize;
    fn clear(_: auto);
    fn unionWith(_: auto, other: BloomFilter4096);
    fn estimatedFpr(): f64;
}

