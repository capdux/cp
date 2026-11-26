// Type definitions for CP

export fn ef2Popcount64(val: u64): usize;
export fn ef2Ctz64(val: u64): u8;
export class BitFlags64 {
    bits: u64,
    valid_mask: u64,
    fn empty(valid_mask: u64): BitFlags64;
    fn all(valid_mask: u64): BitFlags64;
    fn fromBitsTruncate(bits: u64, valid_mask: u64): BitFlags64;
    fn fromBitsStrict(bits: u64, valid_mask: u64): BitFlags64?;
    fn raw(): u64;
    fn len(): usize;
    fn isEmpty(): bool;
    fn isAll(): bool;
    fn contains(flag: u64): bool;
    fn intersects(flag: u64): bool;
    fn insert(_: auto, flag: u64);
    fn remove(_: auto, flag: u64);
    fn toggle(_: auto, flag: u64);
    fn set(_: auto, flag: u64, enabled: bool);
    fn clear(_: auto);
    fn unionFlags(other: BitFlags64): BitFlags64;
    fn intersectFlags(other: BitFlags64): BitFlags64;
    fn differenceFlags(other: BitFlags64): BitFlags64;
    fn symmetricDifferenceFlags(other: BitFlags64): BitFlags64;
    fn complement(): BitFlags64;
    fn nextFlag(start_bit: u8): u8?;
}

