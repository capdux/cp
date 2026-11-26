// Type definitions for CP

export fn vmapMask(bit: usize): u64;
export fn vmapHasBit(bits_lo: u64, bits_hi: u64, k: usize): bool;
export class VecMapI64 {
    slots: [128]i64,
    bits_lo: u64,
    bits_hi: u64,
    count: usize,
    fn new(): VecMapI64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn containsKey(k: usize): bool;
    fn get(k: usize): i64?;
    fn insert(_: auto, k: usize, v: i64): i64?;
    fn remove(_: auto, k: usize): i64?;
    fn clear(_: auto);
    fn keys(out: []usize): usize;
    fn values(out: []i64): usize;
    fn retainGreaterThan(_: auto, min_val: i64): usize;
}

