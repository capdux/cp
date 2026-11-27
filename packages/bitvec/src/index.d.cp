// Type definitions for CP

export fn bvPopcountU64(x: u64): usize;
export class BitVec512 {
    words: [8]u64,
    bit_len: usize,
    fn new(): BitVec512;
    fn withLen(n_bits: usize, initial: bool): BitVec512;
    fn len(): usize;
    fn capacity(): usize;
    fn isEmpty(): bool;
    fn clear(_: auto);
    fn get(idx: usize): bool;
    fn set(_: auto, idx: usize, val: bool): bool;
    fn toggle(_: auto, idx: usize): bool;
    fn push(_: auto, val: bool): bool;
    fn pop(_: auto): i32;
    fn countOnes(): usize;
    fn countZeros(): usize;
    fn any(): bool;
    fn all(): bool;
    fn firstOne(): i32;
    fn lastOne(): i32;
    fn andWith(_: auto, other: BitVec512);
    fn orWith(_: auto, other: BitVec512);
    fn xorWith(_: auto, other: BitVec512);
    fn notAll(_: auto);
    fn toBinStr(out: []u8): usize;
}

