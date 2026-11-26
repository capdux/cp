// @cap/bitflags-crate — Zero-heap Rust `bitflags` v2 style `Flags32` & `Flags64`.
// Clean-room MIT port of crates.io #12 `bitflags` (1.95B+ downloads).

fn bfcPop32(x: u32): usize {
    let v = x;
    let c: usize = 0;
    while (v != 0) {
        v = v & (v - 1);
        c = c + 1;
    }
    return c;
}

fn bfcPop64(x: u64): usize {
    let v = x;
    let c: usize = 0;
    while (v != 0) {
        v = v & (v - 1);
        c = c + 1;
    }
    return c;
}

/// 32-bit defined-mask bitflags container (`bitflags!` u32).
pub struct Flags32 {
    pub raw_bits: u32,
    pub defined_mask: u32,

    pub static fn empty(defined_mask: u32): Flags32 {
        return Flags32 { raw_bits: 0, defined_mask: defined_mask };
    }

    pub static fn all(defined_mask: u32): Flags32 {
        return Flags32 { raw_bits: defined_mask, defined_mask: defined_mask };
    }

    pub static fn fromBits(val: u32, defined_mask: u32): Flags32? {
        if ((val & (~defined_mask)) != 0) { return null; }
        return Flags32 { raw_bits: val, defined_mask: defined_mask };
    }

    pub static fn fromBitsTruncate(val: u32, defined_mask: u32): Flags32 {
        return Flags32 { raw_bits: val & defined_mask, defined_mask: defined_mask };
    }

    pub static fn fromBitsRetain(val: u32, defined_mask: u32): Flags32 {
        return Flags32 { raw_bits: val, defined_mask: defined_mask };
    }

    pub fn bits(): u32 { return _.raw_bits; }
    pub fn isEmpty(): bool { return _.raw_bits == 0; }
    pub fn isAll(): bool { return (_.raw_bits & _.defined_mask) == _.defined_mask; }
    pub fn count(): usize { return bfcPop32(_.raw_bits); }
    pub fn contains(other: u32): bool { return (_.raw_bits & other) == other; }
    pub fn intersects(other: u32): bool { return (_.raw_bits & other) != 0; }
    pub fn containsUnknownBits(): bool { return (_.raw_bits & (~_.defined_mask)) != 0; }

    pub fn insert(*_, other: u32) { _.raw_bits = _.raw_bits | (other & _.defined_mask); }
    pub fn remove(*_, other: u32) { _.raw_bits = _.raw_bits & (~other); }
    pub fn toggle(*_, other: u32) { _.raw_bits = _.raw_bits ^ (other & _.defined_mask); }
    pub fn set(*_, other: u32, enabled: bool) {
        if (enabled) { _.insert(other); } else { _.remove(other); }
    }
    pub fn clear(*_) { _.raw_bits = 0; }
    pub fn truncate(*_) { _.raw_bits = _.raw_bits & _.defined_mask; }

    pub fn unionWith(other: Flags32): Flags32 {
        return Flags32 { raw_bits: _.raw_bits | other.raw_bits, defined_mask: _.defined_mask };
    }
    pub fn intersection(other: Flags32): Flags32 {
        return Flags32 { raw_bits: _.raw_bits & other.raw_bits, defined_mask: _.defined_mask };
    }
    pub fn difference(other: Flags32): Flags32 {
        return Flags32 { raw_bits: _.raw_bits & (~other.raw_bits), defined_mask: _.defined_mask };
    }
    pub fn symmetricDifference(other: Flags32): Flags32 {
        return Flags32 { raw_bits: _.raw_bits ^ other.raw_bits, defined_mask: _.defined_mask };
    }
    pub fn complement(): Flags32 {
        return Flags32 { raw_bits: (~_.raw_bits) & _.defined_mask, defined_mask: _.defined_mask };
    }
}

/// 64-bit defined-mask bitflags container (`bitflags!` u64).
pub struct Flags64 {
    pub raw_bits: u64,
    pub defined_mask: u64,

    pub static fn empty(defined_mask: u64): Flags64 {
        return Flags64 { raw_bits: 0, defined_mask: defined_mask };
    }

    pub static fn all(defined_mask: u64): Flags64 {
        return Flags64 { raw_bits: defined_mask, defined_mask: defined_mask };
    }

    pub static fn fromBits(val: u64, defined_mask: u64): Flags64? {
        if ((val & (~defined_mask)) != 0) { return null; }
        return Flags64 { raw_bits: val, defined_mask: defined_mask };
    }

    pub static fn fromBitsTruncate(val: u64, defined_mask: u64): Flags64 {
        return Flags64 { raw_bits: val & defined_mask, defined_mask: defined_mask };
    }

    pub fn bits(): u64 { return _.raw_bits; }
    pub fn isEmpty(): bool { return _.raw_bits == 0; }
    pub fn isAll(): bool { return (_.raw_bits & _.defined_mask) == _.defined_mask; }
    pub fn count(): usize { return bfcPop64(_.raw_bits); }
    pub fn contains(other: u64): bool { return (_.raw_bits & other) == other; }
    pub fn intersects(other: u64): bool { return (_.raw_bits & other) != 0; }

    pub fn insert(*_, other: u64) { _.raw_bits = _.raw_bits | (other & _.defined_mask); }
    pub fn remove(*_, other: u64) { _.raw_bits = _.raw_bits & (~other); }
    pub fn toggle(*_, other: u64) { _.raw_bits = _.raw_bits ^ (other & _.defined_mask); }
    pub fn clear(*_) { _.raw_bits = 0; }
    pub fn complement(): Flags64 {
        return Flags64 { raw_bits: (~_.raw_bits) & _.defined_mask, defined_mask: _.defined_mask };
    }
}
