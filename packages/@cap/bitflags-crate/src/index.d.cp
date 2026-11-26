// Type definitions for CP

export fn bfcPop32(x: u32): usize;
export fn bfcPop64(x: u64): usize;
export class Flags32 {
    raw_bits: u32,
    defined_mask: u32,
    fn empty(defined_mask: u32): Flags32;
    fn all(defined_mask: u32): Flags32;
    fn fromBits(val: u32, defined_mask: u32): Flags32?;
    fn fromBitsTruncate(val: u32, defined_mask: u32): Flags32;
    fn fromBitsRetain(val: u32, defined_mask: u32): Flags32;
    fn bits(): u32;
    fn isEmpty(): bool;
    fn isAll(): bool;
    fn count(): usize;
    fn contains(other: u32): bool;
    fn intersects(other: u32): bool;
    fn containsUnknownBits(): bool;
    fn insert(_: auto, other: u32);
    fn remove(_: auto, other: u32);
    fn toggle(_: auto, other: u32);
    fn set(_: auto, other: u32, enabled: bool);
    fn clear(_: auto);
    fn truncate(_: auto);
    fn unionWith(other: Flags32): Flags32;
    fn intersection(other: Flags32): Flags32;
    fn difference(other: Flags32): Flags32;
    fn symmetricDifference(other: Flags32): Flags32;
    fn complement(): Flags32;
}

export class Flags64 {
    raw_bits: u64,
    defined_mask: u64,
    fn empty(defined_mask: u64): Flags64;
    fn all(defined_mask: u64): Flags64;
    fn fromBits(val: u64, defined_mask: u64): Flags64?;
    fn fromBitsTruncate(val: u64, defined_mask: u64): Flags64;
    fn bits(): u64;
    fn isEmpty(): bool;
    fn isAll(): bool;
    fn count(): usize;
    fn contains(other: u64): bool;
    fn intersects(other: u64): bool;
    fn insert(_: auto, other: u64);
    fn remove(_: auto, other: u64);
    fn toggle(_: auto, other: u64);
    fn clear(_: auto);
    fn complement(): Flags64;
}

