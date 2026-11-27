// @cap/bit-field — Zero-allocation bit & bit-range manipulation utilities
// Inspired by crates.io:bit_field (https://github.com/phil-opp/rust-bit_field)

fn bfldMaskU64(width: u6): u64 {
    return ((1 as u64) << width) - 1;
}

fn bfldMaskU32(width: u5): u32 {
    return ((1 as u32) << width) - 1;
}

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
