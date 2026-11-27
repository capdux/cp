// packages/@cap/enumflags2/src/index.cp
// Zero-allocation 64-bit type-safe enum bitflags set with valid-mask protection.
// Clean-room MIT pure CP implementation inspired by enumflags2 v0.7.

/// Count set bits in a 64-bit integer using Brian Kernighan's loop (zero-alloc, portable).
fn ef2Popcount64(val: u64): usize {
    let x: u64 = val;
    let count: usize = 0;
    while (x != 0) {
        x = x & (x - 1);
        count += 1;
    }
    return count;
}

/// Count trailing zero bits in a non-zero 64-bit integer using binary search.
fn ef2Ctz64(val: u64): u8 {
    let x: u64 = val;
    let n: u8 = 0;
    if ((x & 0x00000000FFFFFFFF) == 0) {
        n += 32;
        x = x >> (32 as u6);
    }
    if ((x & 0x000000000000FFFF) == 0) {
        n += 16;
        x = x >> (16 as u6);
    }
    if ((x & 0x00000000000000FF) == 0) {
        n += 8;
        x = x >> (8 as u6);
    }
    if ((x & 0x000000000000000F) == 0) {
        n += 4;
        x = x >> (4 as u6);
    }
    if ((x & 0x0000000000000003) == 0) {
        n += 2;
        x = x >> (2 as u6);
    }
    if ((x & 0x0000000000000001) == 0) {
        n += 1;
    }
    return n;
}

/// A 64-bit bitflag set guarded by `valid_mask` to prevent out-of-range bit injection.
pub struct BitFlags64 {
    pub bits: u64,
    pub valid_mask: u64,

    /// Create an empty flag set restricted to `valid_mask`.
    pub static fn empty(valid_mask: u64): BitFlags64 {
        return BitFlags64 {
            bits: 0,
            valid_mask: valid_mask,
        };
    }

    /// Create a flag set with all valid bits in `valid_mask` set.
    pub static fn all(valid_mask: u64): BitFlags64 {
        return BitFlags64 {
            bits: valid_mask,
            valid_mask: valid_mask,
        };
    }

    /// Construct from raw `bits`, silently masking off any bits outside `valid_mask`.
    pub static fn fromBitsTruncate(bits: u64, valid_mask: u64): BitFlags64 {
        return BitFlags64 {
            bits: bits & valid_mask,
            valid_mask: valid_mask,
        };
    }

    /// Construct from raw `bits`, returning `null` if any bit outside `valid_mask` is set.
    pub static fn fromBitsStrict(bits: u64, valid_mask: u64): BitFlags64? {
        if ((bits & (~valid_mask)) != 0) {
            return null;
        }
        return BitFlags64 {
            bits: bits,
            valid_mask: valid_mask,
        };
    }

    /// Return the underlying raw 64-bit bitmask.
    pub fn raw(): u64 {
        return _.bits;
    }

    /// Return the number of set flags (popcount).
    pub fn len(): usize {
        return ef2Popcount64(_.bits);
    }

    /// Return true if no flags are set.
    pub fn isEmpty(): bool {
        return _.bits == 0;
    }

    /// Return true if all valid flags in `valid_mask` are set.
    pub fn isAll(): bool {
        return _.bits == _.valid_mask;
    }

    /// Return true if all bits in `flag` are valid and currently set.
    pub fn contains(flag: u64): bool {
        if ((flag & (~_.valid_mask)) != 0) {
            return false;
        }
        return (_.bits & flag) == flag;
    }

    /// Return true if any valid bit in `flag` is currently set.
    pub fn intersects(flag: u64): bool {
        return (_.bits & (flag & _.valid_mask)) != 0;
    }

    /// Set the valid bits of `flag`.
    pub fn insert(*_, flag: u64) {
        _.bits = (_.bits | flag) & _.valid_mask;
    }

    /// Clear the bits of `flag`.
    pub fn remove(*_, flag: u64) {
        _.bits = _.bits & (~(flag & _.valid_mask));
    }

    /// Flip the valid bits of `flag`.
    pub fn toggle(*_, flag: u64) {
        _.bits = (_.bits ^ (flag & _.valid_mask)) & _.valid_mask;
    }

    /// Set or clear `flag` depending on `enabled`.
    pub fn set(*_, flag: u64, enabled: bool) {
        if (enabled) {
            _.bits = (_.bits | flag) & _.valid_mask;
        } else {
            _.bits = _.bits & (~(flag & _.valid_mask));
        }
    }

    /// Clear all flags.
    pub fn clear(*_) {
        _.bits = 0;
    }

    /// Return a new set containing flags present in either `_` or `other` (clamped to `_.valid_mask`).
    pub fn unionFlags(other: BitFlags64): BitFlags64 {
        return BitFlags64 {
            bits: (_.bits | other.bits) & _.valid_mask,
            valid_mask: _.valid_mask,
        };
    }

    /// Return a new set containing flags present in both `_` and `other`.
    pub fn intersectFlags(other: BitFlags64): BitFlags64 {
        return BitFlags64 {
            bits: (_.bits & other.bits) & _.valid_mask,
            valid_mask: _.valid_mask,
        };
    }

    /// Return a new set containing flags present in `_` but not in `other`.
    pub fn differenceFlags(other: BitFlags64): BitFlags64 {
        return BitFlags64 {
            bits: (_.bits & (~other.bits)) & _.valid_mask,
            valid_mask: _.valid_mask,
        };
    }

    /// Return a new set containing flags present in exactly one of `_` or `other`.
    pub fn symmetricDifferenceFlags(other: BitFlags64): BitFlags64 {
        return BitFlags64 {
            bits: (_.bits ^ other.bits) & _.valid_mask,
            valid_mask: _.valid_mask,
        };
    }

    /// Return the bitwise complement within `_.valid_mask`.
    pub fn complement(): BitFlags64 {
        return BitFlags64 {
            bits: (~_.bits) & _.valid_mask,
            valid_mask: _.valid_mask,
        };
    }

    /// Return the lowest set bit index `>= start_bit` in `0..=63`, or `null` if none remain.
    pub fn nextFlag(start_bit: u8): u8? {
        if (start_bit >= (64 as u8)) {
            return null;
        }
        const shifted: u64 = _.bits >> (start_bit as u6);
        if (shifted == 0) {
            return null;
        }
        return start_bit + ef2Ctz64(shifted);
    }
}
