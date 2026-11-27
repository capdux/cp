// @cap/bit-field — Zero-allocation bit & bit-range manipulation utilities
// Inspired by crates.io:bit_field (https://github.com/phil-opp/rust-bit_field)

// --- Bit length constants & helpers (BitField::BIT_LENGTH) ---

pub fn bitLengthU8(): usize { return 8; }
pub fn bitLengthU16(): usize { return 16; }
pub fn bitLengthU32(): usize { return 32; }
pub fn bitLengthU64(): usize { return 64; }
pub fn bitLengthUsize(): usize { return @bitSizeOf(usize); }
pub fn bitLengthSlice(bytes: []const u8): usize { return bytes.len * 8; }

// --- Internal mask helpers ---

fn bfldMaskU8(width: u3): u8 {
    return ((1 as u8) << width) - 1;
}

fn bfldMaskU16(width: u4): u16 {
    return ((1 as u16) << width) - 1;
}

fn bfldMaskU32(width: u5): u32 {
    return ((1 as u32) << width) - 1;
}

fn bfldMaskU64(width: u6): u64 {
    return ((1 as u64) << width) - 1;
}

fn bfldFullMaskU64(width: u8): u64 {
    if (width == 0) { return 0; }
    if (width >= 64) { return ~(0 as u64); }
    return ((1 as u64) << (width as u6)) - 1;
}

// --- u8 BitField ---

pub fn getBitU8(val: u8, bit: u3): bool {
    return ((val >> bit) & 1) != 0;
}

pub fn setBitU8(val: u8, bit: u3, on: bool): u8 {
    const mask: u8 = (1 as u8) << bit;
    if (on) {
        return val | mask;
    }
    return val & ~mask;
}

pub fn getBitsU8(val: u8, start: u3, end: u3): u8 {
    if (start >= end) {
        return 0;
    }
    const width: u3 = end - start;
    return (val >> start) & bfldMaskU8(width);
}

pub fn setBitsU8(val: u8, start: u3, end: u3, bits: u8): u8 {
    if (start >= end) {
        return val;
    }
    const width: u3 = end - start;
    const mask: u8 = bfldMaskU8(width);
    const cleared: u8 = val & ~(mask << start);
    return cleared | ((bits & mask) << start);
}

// --- u16 BitField ---

pub fn getBitU16(val: u16, bit: u4): bool {
    return ((val >> bit) & 1) != 0;
}

pub fn setBitU16(val: u16, bit: u4, on: bool): u16 {
    const mask: u16 = (1 as u16) << bit;
    if (on) {
        return val | mask;
    }
    return val & ~mask;
}

pub fn getBitsU16(val: u16, start: u4, end: u4): u16 {
    if (start >= end) {
        return 0;
    }
    const width: u4 = end - start;
    return (val >> start) & bfldMaskU16(width);
}

pub fn setBitsU16(val: u16, start: u4, end: u4, bits: u16): u16 {
    if (start >= end) {
        return val;
    }
    const width: u4 = end - start;
    const mask: u16 = bfldMaskU16(width);
    const cleared: u16 = val & ~(mask << start);
    return cleared | ((bits & mask) << start);
}

// --- u64 BitField ---

pub fn getBitU64(val: u64, bit: u6): bool {
    return ((val >> bit) & 1) != 0;
}

pub fn setBitU64(val: u64, bit: u6, on: bool): u64 {
    const mask: u64 = (1 as u64) << bit;
    if (on) {
        return val | mask;
    }
    return val & ~mask;
}

pub fn getBitsU64(val: u64, start: u6, end: u6): u64 {
    if (start >= end) {
        return 0;
    }
    const width: u6 = end - start;
    return (val >> start) & bfldMaskU64(width);
}

pub fn setBitsU64(val: u64, start: u6, end: u6, bits: u64): u64 {
    if (start >= end) {
        return val;
    }
    const width: u6 = end - start;
    const mask: u64 = bfldMaskU64(width);
    const cleared: u64 = val & ~(mask << start);
    return cleared | ((bits & mask) << start);
}

/// Full-range `[start..end)` extraction for `u64` supporting `end == 64`.
pub fn getBitsRangeU64(val: u64, start: u8, end: u8): u64 {
    if (start >= end || start >= 64) {
        return 0;
    }
    const clamped_end: u8 = if (end > 64) { 64 } else { end };
    const width: u8 = clamped_end - start;
    return (val >> (start as u6)) & bfldFullMaskU64(width);
}

/// Full-range `[start..end)` insertion for `u64` supporting `end == 64`.
pub fn setBitsRangeU64(val: u64, start: u8, end: u8, bits: u64): u64 {
    if (start >= end || start >= 64) {
        return val;
    }
    const clamped_end: u8 = if (end > 64) { 64 } else { end };
    const width: u8 = clamped_end - start;
    const mask: u64 = bfldFullMaskU64(width);
    const cleared: u64 = val & ~(mask << (start as u6));
    return cleared | ((bits & mask) << (start as u6));
}

// --- u32 BitField ---

pub fn getBitU32(val: u32, bit: u5): bool {
    return ((val >> bit) & 1) != 0;
}

pub fn setBitU32(val: u32, bit: u5, on: bool): u32 {
    const mask: u32 = (1 as u32) << bit;
    if (on) {
        return val | mask;
    }
    return val & ~mask;
}

pub fn getBitsU32(val: u32, start: u5, end: u5): u32 {
    if (start >= end) {
        return 0;
    }
    const width: u5 = end - start;
    return (val >> start) & bfldMaskU32(width);
}

pub fn setBitsU32(val: u32, start: u5, end: u5, bits: u32): u32 {
    if (start >= end) {
        return val;
    }
    const width: u5 = end - start;
    const mask: u32 = bfldMaskU32(width);
    const cleared: u32 = val & ~(mask << start);
    return cleared | ((bits & mask) << start);
}

// --- usize BitField ---

pub fn getBitUsize(val: usize, bit: u8): bool {
    if ((bit as usize) >= @bitSizeOf(usize)) { return false; }
    return getBitU64(val as u64, bit as u6);
}

pub fn setBitUsize(val: usize, bit: u8, on: bool): usize {
    if ((bit as usize) >= @bitSizeOf(usize)) { return val; }
    return setBitU64(val as u64, bit as u6, on) as usize;
}

pub fn getBitsUsize(val: usize, start: u8, end: u8): usize {
    const max_bits: u8 = @bitSizeOf(usize) as u8;
    const clamped_end: u8 = if (end > max_bits) { max_bits } else { end };
    return getBitsRangeU64(val as u64, start, clamped_end) as usize;
}

pub fn setBitsUsize(val: usize, start: u8, end: u8, bits: usize): usize {
    const max_bits: u8 = @bitSizeOf(usize) as u8;
    const clamped_end: u8 = if (end > max_bits) { max_bits } else { end };
    return setBitsRangeU64(val as u64, start, clamped_end, bits as u64) as usize;
}

// --- Byte-slice BitArray ---

pub fn getBitSlice(bytes: []const u8, bit_index: usize): bool {
    const byte_idx: usize = bit_index >> 3;
    if (byte_idx >= bytes.len) {
        return false;
    }
    const bit_pos: u3 = (bit_index & 7) as u3;
    return ((bytes[byte_idx] >> bit_pos) & 1) != 0;
}

pub fn setBitSlice(bytes: []u8, bit_index: usize, on: bool) {
    const byte_idx: usize = bit_index >> 3;
    if (byte_idx >= bytes.len) {
        return;
    }
    const bit_pos: u3 = (bit_index & 7) as u3;
    const mask: u8 = (1 as u8) << bit_pos;
    if (on) {
        bytes[byte_idx] = bytes[byte_idx] | mask;
    } else {
        bytes[byte_idx] = bytes[byte_idx] & ~mask;
    }
}

pub fn toggleBitSlice(bytes: []u8, bit_index: usize) {
    const byte_idx: usize = bit_index >> 3;
    if (byte_idx >= bytes.len) {
        return;
    }
    const bit_pos: u3 = (bit_index & 7) as u3;
    const mask: u8 = (1 as u8) << bit_pos;
    bytes[byte_idx] = bytes[byte_idx] ^ mask;
}

/// Extracts up to 64 bits from `[start..end)` across a byte slice (LSB-first per byte).
pub fn getBitsSlice(bytes: []const u8, start: usize, end: usize): u64 {
    const total_bits = bytes.len * 8;
    if (start >= end || start >= total_bits) {
        return 0;
    }
    let stop = if (end > total_bits) { total_bits } else { end };
    if (stop - start > 64) {
        stop = start + 64;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    const count = stop - start;
    while (i < count) : (i += 1) {
        if (getBitSlice(bytes, start + i)) {
            acc = acc | ((1 as u64) << (i as u6));
        }
    }
    return acc;
}

/// Inserts up to 64 bits from `value` into `[start..end)` of a byte slice (LSB-first per byte).
pub fn setBitsSlice(bytes: []u8, start: usize, end: usize, value: u64) {
    const total_bits = bytes.len * 8;
    if (start >= end || start >= total_bits) {
        return;
    }
    let stop = if (end > total_bits) { total_bits } else { end };
    if (stop - start > 64) {
        stop = start + 64;
    }
    let i: usize = 0;
    const count = stop - start;
    while (i < count) : (i += 1) {
        const bit_on = ((value >> (i as u6)) & 1) != 0;
        setBitSlice(bytes, start + i, bit_on);
    }
}

/// Counts total set bits across a byte slice.
pub fn countOnesSlice(bytes: []const u8): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        let b = bytes[i];
        while (b != 0) {
            total += (b & 1) as usize;
            b = b >> 1;
        }
    }
    return total;
}

/// Object-oriented `BitArray` wrapper over a mutable byte slice (matching `bit_field::BitArray`).
pub struct BitArray {
    pub data: []u8,

    pub static fn fromSlice(buf: []u8): BitArray {
        return BitArray { data: buf };
    }

    pub fn bitLength(): usize {
        return _.data.len * 8;
    }

    pub fn getBit(bit_index: usize): bool {
        return getBitSlice(_.data, bit_index);
    }

    pub fn setBit(*_, bit_index: usize, on: bool) {
        setBitSlice(_.data, bit_index, on);
    }

    pub fn toggleBit(*_, bit_index: usize) {
        toggleBitSlice(_.data, bit_index);
    }

    pub fn getBits(start: usize, end: usize): u64 {
        return getBitsSlice(_.data, start, end);
    }

    pub fn setBits(*_, start: usize, end: usize, value: u64) {
        setBitsSlice(_.data, start, end, value);
    }

    pub fn countOnes(): usize {
        return countOnesSlice(_.data);
    }

    pub fn clearAll(*_) {
        let i: usize = 0;
        while (i < _.data.len) : (i += 1) {
            _.data[i] = 0;
        }
    }

    pub fn setAll(*_) {
        let i: usize = 0;
        while (i < _.data.len) : (i += 1) {
            _.data[i] = 0xFF;
        }
    }
}
