// Type definitions for CP

export fn fbsPopcount64(x: u64): usize;
export fn fbsCtz64(x: u64): usize;
export class FixedBitSet {
    words: [16]u64,
    bit_len: usize,
    fn withCapacity(bits: usize): FixedBitSet;
    fn len(): usize;
    fn isEmpty(): bool;
    fn contains(bit: usize): bool;
    fn insert(_: auto, bit: usize): bool;
    fn put(_: auto, bit: usize): bool;
    fn remove(_: auto, bit: usize): bool;
    fn toggle(_: auto, bit: usize);
    fn set(_: auto, bit: usize, enabled: bool);
    fn insertRange(_: auto, start: usize, end: usize);
    fn clear(_: auto);
    fn countOnes(): usize;
    fn isClear(): bool;
    fn unionWith(_: auto, other: FixedBitSet);
    fn intersectWith(_: auto, other: FixedBitSet);
    fn differenceWith(_: auto, other: FixedBitSet);
    fn symmetricDifferenceWith(_: auto, other: FixedBitSet);
    fn isSubset(other: FixedBitSet): bool;
    fn isDisjoint(other: FixedBitSet): bool;
    fn nextOne(from_bit: usize): usize?;
    fn maskTail(_: auto);
}

