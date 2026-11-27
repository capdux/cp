// Type definitions for CP

export fn allOnes64(): u64;
export fn bitMask64(bit: usize): u64;
export fn popcount64(x_in: u64): usize;
export fn ctz64(w_in: u64): usize;
export class BitSet256 {
    words: [4]u64,
    fn init(): BitSet256;
    fn full(): BitSet256;
    fn len(): usize;
    fn clear(_: auto);
    fn contains(bit: usize): bool;
    fn insert(_: auto, bit: usize): bool;
    fn remove(_: auto, bit: usize): bool;
    fn toggle(_: auto, bit: usize);
    fn setRange(_: auto, start: usize, end: usize);
    fn countOnes(): usize;
    fn isEmpty(): bool;
    fn firstOne(): usize?;
    fn nextOne(from_bit: usize): usize?;
    fn unionWith(_: auto, other: BitSet256);
    fn intersectWith(_: auto, other: BitSet256);
    fn differenceWith(_: auto, other: BitSet256);
    fn symmetricDiffWith(_: auto, other: BitSet256);
    fn isSubsetOf(other: BitSet256): bool;
    fn isDisjoint(other: BitSet256): bool;
    fn equals(other: BitSet256): bool;
}

export class BitSet1024 {
    words: [16]u64,
    fn init(): BitSet1024;
    fn full(): BitSet1024;
    fn len(): usize;
    fn clear(_: auto);
    fn contains(bit: usize): bool;
    fn insert(_: auto, bit: usize): bool;
    fn remove(_: auto, bit: usize): bool;
    fn toggle(_: auto, bit: usize);
    fn setRange(_: auto, start: usize, end: usize);
    fn countOnes(): usize;
    fn isEmpty(): bool;
    fn firstOne(): usize?;
    fn nextOne(from_bit: usize): usize?;
    fn unionWith(_: auto, other: BitSet1024);
    fn intersectWith(_: auto, other: BitSet1024);
    fn differenceWith(_: auto, other: BitSet1024);
    fn symmetricDiffWith(_: auto, other: BitSet1024);
    fn isSubsetOf(other: BitSet1024): bool;
    fn isDisjoint(other: BitSet1024): bool;
    fn equals(other: BitSet1024): bool;
}

