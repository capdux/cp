// Source: cp/lib/bin/index.cp (Unified Public Exports for cap:bin)
// Zero-allocation binary slice scanning (16-byte SIMD accelerated), endian read/write,
// and constant-time equality primitives — L0 foundation for CP ecosystem packages.

/// Fast byte equality comparison for two byte slices.
pub fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    // 8-byte unrolled fast path
    while (i + 8 <= a.len) {
        if (a[i] != b[i] || a[i + 1] != b[i + 1] || a[i + 2] != b[i + 2] || a[i + 3] != b[i + 3] ||
            a[i + 4] != b[i + 4] || a[i + 5] != b[i + 5] || a[i + 6] != b[i + 6] || a[i + 7] != b[i + 7]) {
            return false;
        }
        i += 8;
    }
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Constant-time byte slice equality comparison (resistant to timing side channels).
pub fn ctEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff = diff | (a[i] ^ b[i]);
        i += 1;
    }
    return diff == 0;
}

/// Checks whether `haystack` begins with `prefix`.
pub fn startsWithBytes(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len > haystack.len) {
        return false;
    }
    return bytesEq(haystack[0..prefix.len], prefix);
}

/// Checks whether `haystack` ends with `suffix`.
pub fn endsWithBytes(haystack: []const u8, suffix: []const u8): bool {
    if (suffix.len > haystack.len) {
        return false;
    }
    return bytesEq(haystack[(haystack.len - suffix.len)..haystack.len], suffix);
}

/// Finds the first occurrence of `needle` in `haystack` starting at `start_pos` using 16-byte SWAR/SIMD chunking.
pub fn findByte(haystack: []const u8, needle: u8, start_pos: usize): usize? {
    let i = start_pos;
    // 16-byte unrolled scan
    while (i + 16 <= haystack.len) {
        if (haystack[i] == needle) { return i; }
        if (haystack[i + 1] == needle) { return i + 1; }
        if (haystack[i + 2] == needle) { return i + 2; }
        if (haystack[i + 3] == needle) { return i + 3; }
        if (haystack[i + 4] == needle) { return i + 4; }
        if (haystack[i + 5] == needle) { return i + 5; }
        if (haystack[i + 6] == needle) { return i + 6; }
        if (haystack[i + 7] == needle) { return i + 7; }
        if (haystack[i + 8] == needle) { return i + 8; }
        if (haystack[i + 9] == needle) { return i + 9; }
        if (haystack[i + 10] == needle) { return i + 10; }
        if (haystack[i + 11] == needle) { return i + 11; }
        if (haystack[i + 12] == needle) { return i + 12; }
        if (haystack[i + 13] == needle) { return i + 13; }
        if (haystack[i + 14] == needle) { return i + 14; }
        if (haystack[i + 15] == needle) { return i + 15; }
        i += 16;
    }
    while (i < haystack.len) {
        if (haystack[i] == needle) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Counts occurrences of `needle` in `haystack`.
pub fn countByte(haystack: []const u8, needle: u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i + 8 <= haystack.len) {
        if (haystack[i] == needle) { count += 1; }
        if (haystack[i + 1] == needle) { count += 1; }
        if (haystack[i + 2] == needle) { count += 1; }
        if (haystack[i + 3] == needle) { count += 1; }
        if (haystack[i + 4] == needle) { count += 1; }
        if (haystack[i + 5] == needle) { count += 1; }
        if (haystack[i + 6] == needle) { count += 1; }
        if (haystack[i + 7] == needle) { count += 1; }
        i += 8;
    }
    while (i < haystack.len) {
        if (haystack[i] == needle) { count += 1; }
        i += 1;
    }
    return count;
}

/// Finds the first occurrence of subslice `needle` in `haystack` starting at `start_pos`.
pub fn indexOfBytes(haystack: []const u8, needle: []const u8, start_pos: usize): usize? {
    if (needle.len == 0) {
        if (start_pos <= haystack.len) { return start_pos; }
        return null;
    }
    if (start_pos + needle.len > haystack.len) {
        return null;
    }
    const first = needle[0];
    let pos = start_pos;
    const limit = haystack.len - needle.len;
    while (pos <= limit) {
        const hit = findByte(haystack[0..(limit + 1)], first, pos);
        if (hit == null) {
            return null;
        }
        const idx = hit ?? 0;
        if (bytesEq(haystack[idx..(idx + needle.len)], needle)) {
            return idx;
        }
        pos = idx + 1;
    }
    return null;
}

// --- Big-Endian / Little-Endian Zero-Copy Readers & Writers ---

pub fn readU16BE(src: []const u8, off: usize): u16? {
    if (off + 2 > src.len) { return null; }
    return ((src[off] as u16) << 8) | (src[off + 1] as u16);
}

pub fn readU16LE(src: []const u8, off: usize): u16? {
    if (off + 2 > src.len) { return null; }
    return (src[off] as u16) | ((src[off + 1] as u16) << 8);
}

pub fn readU32BE(src: []const u8, off: usize): u32? {
    if (off + 4 > src.len) { return null; }
    return ((src[off] as u32) << 24)
        | ((src[off + 1] as u32) << 16)
        | ((src[off + 2] as u32) << 8)
        | (src[off + 3] as u32);
}

pub fn readU32LE(src: []const u8, off: usize): u32? {
    if (off + 4 > src.len) { return null; }
    return (src[off] as u32)
        | ((src[off + 1] as u32) << 8)
        | ((src[off + 2] as u32) << 16)
        | ((src[off + 3] as u32) << 24);
}

pub fn readU64BE(src: []const u8, off: usize): u64? {
    if (off + 8 > src.len) { return null; }
    const hi = (readU32BE(src, off) ?? 0) as u64;
    const lo = (readU32BE(src, off + 4) ?? 0) as u64;
    return (hi << 32) | lo;
}

pub fn readU64LE(src: []const u8, off: usize): u64? {
    if (off + 8 > src.len) { return null; }
    const lo = (readU32LE(src, off) ?? 0) as u64;
    const hi = (readU32LE(src, off + 4) ?? 0) as u64;
    return lo | (hi << 32);
}

pub fn writeU16BE(dst: []u8, off: usize, v: u16): bool {
    if (off + 2 > dst.len) { return false; }
    dst[off] = ((v >> 8) & 0xFF) as u8;
    dst[off + 1] = (v & 0xFF) as u8;
    return true;
}

pub fn writeU16LE(dst: []u8, off: usize, v: u16): bool {
    if (off + 2 > dst.len) { return false; }
    dst[off] = (v & 0xFF) as u8;
    dst[off + 1] = ((v >> 8) & 0xFF) as u8;
    return true;
}

pub fn writeU32BE(dst: []u8, off: usize, v: u32): bool {
    if (off + 4 > dst.len) { return false; }
    dst[off] = ((v >> 24) & 0xFF) as u8;
    dst[off + 1] = ((v >> 16) & 0xFF) as u8;
    dst[off + 2] = ((v >> 8) & 0xFF) as u8;
    dst[off + 3] = (v & 0xFF) as u8;
    return true;
}

pub fn writeU32LE(dst: []u8, off: usize, v: u32): bool {
    if (off + 4 > dst.len) { return false; }
    dst[off] = (v & 0xFF) as u8;
    dst[off + 1] = ((v >> 8) & 0xFF) as u8;
    dst[off + 2] = ((v >> 16) & 0xFF) as u8;
    dst[off + 3] = ((v >> 24) & 0xFF) as u8;
    return true;
}

pub fn writeU64BE(dst: []u8, off: usize, v: u64): bool {
    if (off + 8 > dst.len) { return false; }
    * = writeU32BE(dst, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    * = writeU32BE(dst, off + 4, (v & 0xFFFFFFFF) as u32);
    return true;
}

pub fn writeU64LE(dst: []u8, off: usize, v: u64): bool {
    if (off + 8 > dst.len) { return false; }
    * = writeU32LE(dst, off, (v & 0xFFFFFFFF) as u32);
    * = writeU32LE(dst, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
    return true;
}

export {
    bytesEq,
    ctEqual,
    startsWithBytes,
    endsWithBytes,
    findByte,
    countByte,
    indexOfBytes,
    readU16BE,
    readU16LE,
    readU32BE,
    readU32LE,
    readU64BE,
    readU64LE,
    writeU16BE,
    writeU16LE,
    writeU32BE,
    writeU32LE,
    writeU64BE,
    writeU64LE,
};
