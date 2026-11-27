// Type definitions for CP

export const INT_TABLE_CAP: usize;

export class IntHasher {
    state: u64,
    static fn new(): IntHasher;
    fn writeU8(_: auto, v: u8);
    fn writeU16(_: auto, v: u16);
    fn writeU32(_: auto, v: u32);
    fn writeU64(_: auto, v: u64);
    fn writeUsize(_: auto, v: usize);
    fn writeI8(_: auto, v: i8);
    fn writeI16(_: auto, v: i16);
    fn writeI32(_: auto, v: i32);
    fn writeI64(_: auto, v: i64);
    fn writeIsize(_: auto, v: isize);
    fn finish(): u64;
    fn reset(_: auto);
    fn clone(): IntHasher;
}

export type NoHashHasher = IntHasher;

export class BuildNoHashHasher {
    seed: u64,
    static fn new(): BuildNoHashHasher;
    fn buildHasher(): IntHasher;
}

export fn newIntHasher(): IntHasher;
export fn newNoHashHasher(): NoHashHasher;
export fn nohashU8(key: u8): u64;
export fn nohashU16(key: u16): u64;
export fn nohashU32(key: u32): u64;
export fn nohashU64(key: u64): u64;
export fn nohashUsize(key: usize): u64;
export fn nohashI8(key: i8): u64;
export fn nohashI16(key: i16): u64;
export fn nohashI32(key: i32): u64;
export fn nohashI64(key: i64): u64;
export fn nohashIsize(key: isize): u64;
export fn fibBucketIndex(key: u64, bits: u6): usize;
export fn mixU64(x: u64): u64;
export fn unmixU64(x: u64): u64;

export class IntMap64 {
    keys: [64]u64,
    vals: [64]u64,
    states: [64]u8,
    count: usize,
    static fn init(): IntMap64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn insert(_: auto, key: u64, val: u64): bool;
    fn get(key: u64): u64?;
    fn containsKey(key: u64): bool;
    fn remove(_: auto, key: u64): u64?;
}

export class IntSet64 {
    keys: [64]u64,
    states: [64]u8,
    count: usize,
    static fn init(): IntSet64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn insert(_: auto, key: u64): bool;
    fn contains(key: u64): bool;
    fn remove(_: auto, key: u64): bool;
}
