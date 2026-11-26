// @cap/bitflags — Zero-heap 64-bit bitflags set algebra
// Clean-room MIT implementation for CP (compatible with Rust bitflags).

pub struct Flags64 {
    pub bits: u64,
}

pub fn empty(): Flags64 {
    return Flags64 { bits: 0 };
}

pub fn fromBits(raw: u64): Flags64 {
    return Flags64 { bits: raw };
}

pub fn fromBitIndex(idx: u8): Flags64 {
    if (idx >= 64) { return Flags64 { bits: 0 }; }
    return Flags64 { bits: (1 as u64) << (idx as u6) };
}

pub fn isEmpty(f: Flags64): bool {
    return f.bits == 0;
}

/// Returns true if all bits in `other` are set in `f`.
pub fn contains(f: Flags64, other: Flags64): bool {
    return (f.bits & other.bits) == other.bits;
}

/// Returns true if any bit in `other` is set in `f`.
pub fn intersects(f: Flags64, other: Flags64): bool {
    return (f.bits & other.bits) != 0;
}

pub fn unionFlags(a: Flags64, b: Flags64): Flags64 {
    return Flags64 { bits: a.bits | b.bits };
}

pub fn intersection(a: Flags64, b: Flags64): Flags64 {
    return Flags64 { bits: a.bits & b.bits };
}

pub fn difference(a: Flags64, b: Flags64): Flags64 {
    return Flags64 { bits: a.bits & (~b.bits) };
}

pub fn symmetricDifference(a: Flags64, b: Flags64): Flags64 {
    return Flags64 { bits: a.bits ^ b.bits };
}

pub fn insert(f: *Flags64, other: Flags64) {
    f.bits = f.bits | other.bits;
}

pub fn remove(f: *Flags64, other: Flags64) {
    f.bits = f.bits & (~other.bits);
}

pub fn toggle(f: *Flags64, other: Flags64) {
    f.bits = f.bits ^ other.bits;
}

pub fn setCond(f: *Flags64, other: Flags64, enabled: bool) {
    if (enabled) {
        f.bits = f.bits | other.bits;
    } else {
        f.bits = f.bits & (~other.bits);
    }
}

/// Truncate to only bits allowed by `valid_mask` (equivalent to Rust `from_bits_truncate`).
pub fn truncate(raw: u64, valid_mask: u64): Flags64 {
    return Flags64 { bits: raw & valid_mask };
}

/// Count number of set bits (Kernighan popcount).
pub fn countBits(f: Flags64): u32 {
    let v: u64 = f.bits;
    let c: u32 = 0;
    while (v != 0) {
        v = v & (v - 1);
        c += 1;
    }
    return c;
}

/// Return index (0..63) of lowest set bit, or -1 if empty.
pub fn lowestBitIndex(f: Flags64): i32 {
    if (f.bits == 0) { return -1; }
    let v: u64 = f.bits;
    let idx: i32 = 0;
    while ((v & 1) == 0) {
        v = v >> 1;
        idx += 1;
    }
    return idx;
}

/// Format flags as `0x<hex>` into `out`.
pub fn formatHex(f: Flags64, out: []u8): usize {
    if (out.len < 3) { return 0; }
    out[0] = '0';
    out[1] = 'x';
    if (f.bits == 0) {
        out[2] = '0';
        return 3;
    }
    let tmp: [16]u8 = undefined;
    let n: usize = 0;
    let v: u64 = f.bits;
    while (v > 0) {
        let d: u8 = (v & 0x0f) as u8;
        if (d < 10) {
            tmp[n] = ('0' as u8) + d;
        } else {
            tmp[n] = ('a' as u8) + (d - 10);
        }
        v = v >> 4;
        n += 1;
    }
    if (2 + n > out.len) { return 0; }
    let i: usize = 0;
    while (i < n) {
        out[2 + i] = tmp[n - 1 - i];
        i += 1;
    }
    return 2 + n;
}
