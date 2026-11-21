// Source: Absorbed from /data/dev/bun/src/meta/bits.cp | Synced: 2026-11-21T13:59:55Z
// Pure-CP zero-allocation bitfield, bit-mask, power-of-two alignment, and
// bit manipulation primitives for u32/u64 systems programming.

/// Test whether bit `idx` (0..63) is set in `mask`.
pub fn testBit64(mask: u64, idx: u6): bool {
    return ((mask >> idx) & 1) != 0;
}

/// Set bit `idx` (0..63) in `mask`.
pub fn setBit64(mask: u64, idx: u6): u64 {
    return mask | (@as(u64, 1) << idx);
}

/// Clear bit `idx` (0..63) in `mask`.
pub fn clearBit64(mask: u64, idx: u6): u64 {
    return mask & ~(@as(u64, 1) << idx);
}

/// Toggle bit `idx` (0..63) in `mask`.
pub fn toggleBit64(mask: u64, idx: u6): u64 {
    return mask ^ (@as(u64, 1) << idx);
}

/// True if `lhs` and `rhs` share at least one set bit.
pub fn intersects64(lhs: u64, rhs: u64): bool {
    return (lhs & rhs) != 0;
}

/// True if every set bit in `sub` is also set in `sup`.
pub fn containsAll64(sup: u64, sub: u64): bool {
    return (sup & sub) == sub;
}

/// Clear all bits in `remove_mask` from `lhs`.
pub fn maskOut64(lhs: u64, remove_mask: u64): u64 {
    return lhs & ~remove_mask;
}

/// Extract `width` bits starting at bit offset `shift` (0..63).
pub fn extractBits64(val: u64, shift: u6, width: u7): u64 {
    if (width == 0) return 0;
    if (width >= 64) return val >> shift;
    const mask: u64 = (@as(u64, 1) << @as(u6, @intCast(width))) - 1;
    return (val >> shift) & mask;
}

/// Insert the lowest `width` bits of `field` into `val` starting at bit offset `shift`.
pub fn insertBits64(val: u64, field: u64, shift: u6, width: u7): u64 {
    if (width == 0) return val;
    if (width >= 64) return field;
    const mask: u64 = (@as(u64, 1) << @as(u6, @intCast(width))) - 1;
    const shifted_mask = mask << shift;
    return (val & ~shifted_mask) | ((field & mask) << shift);
}

/// Count set bits (population count) in `x`.
pub fn popcount64(x: u64): u7 {
    return @popCount(x);
}

/// Count leading zero bits in `x`.
pub fn clz64(x: u64): u7 {
    return @clz(x);
}

/// Count trailing zero bits in `x`.
pub fn ctz64(x: u64): u7 {
    return @ctz(x);
}

/// Return true if `x` is a non-zero power of two.
pub fn isPowerOfTwo(x: u64): bool {
    return x != 0 && (x & (x - 1)) == 0;
}

/// Return the smallest power of two >= `x` (returns 1 for 0, returns 0 on overflow > 2^63).
pub fn nextPowerOfTwo(x: u64): u64 {
    if (x <= 1) return 1;
    if (x > (@as(u64, 1) << 63)) return 0;
    const lz: u7 = @clz(x - 1);
    const shift: u6 = @intCast(64 - lz);
    return @as(u64, 1) << shift;
}

/// Align `val` upward to the next multiple of `alignment` (must be power of two).
pub fn alignUp(val: usize, alignment: usize): usize {
    if (alignment <= 1) return val;
    return (val + (alignment - 1)) & ~(alignment - 1);
}

/// Align `val` downward to the previous multiple of `alignment` (must be power of two).
pub fn alignDown(val: usize, alignment: usize): usize {
    if (alignment <= 1) return val;
    return val & ~(alignment - 1);
}

/// Clear the lowest set bit in `x` (`x & (x - 1)`).
pub fn clearLowestSetBit(x: u64): u64 {
    if (x == 0) return 0;
    return x & (x - 1);
}

/// Isolate the lowest set bit in `x` (`x & -x`).
pub fn isolateLowestSetBit(x: u64): u64 {
    if (x == 0) return 0;
    return x & (0 -% x);
}

/// Bit-set iterator over set bit indices in ascending order (0..63).
pub struct BitSetIter64 {
    remaining: u64,

    pub static fn init(mask: u64): BitSetIter64 {
        return BitSetIter64{ remaining: mask };
    }

    pub fn next(*_): u6? {
        if (_.remaining == 0) return null;
        const idx: u6 = @intCast(@ctz(_.remaining));
        _.remaining = _.remaining & (_.remaining - 1);
        return idx;
    }
}
