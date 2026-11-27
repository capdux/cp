// Type definitions for CP

export class IntHasher {
    state: u64,
    fn writeU64(_: auto, v: u64);
    fn writeU32(_: auto, v: u32);
    fn writeI64(_: auto, v: i64);
    fn finish(): u64;
}

export fn newIntHasher(): IntHasher;
export fn nohashU64(key: u64): u64;
export fn nohashU32(key: u32): u64;
export fn fibBucketIndex(key: u64, bits: u6): usize;
export fn mixU64(x: u64): u64;
