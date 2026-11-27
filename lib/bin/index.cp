// Source: cp/lib/bin/index.cp (Unified Public Exports for cap:bin)
// Zero-allocation binary slice scanning (16-byte SIMD accelerated), endian read/write,
// and constant-time equality primitives — L0 foundation for CP ecosystem packages.

type BinVec16 = @Vector(16, u8);

/// 16-lane hardware SIMD byte equality comparison for two byte slices.
pub fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: BinVec16 = a[i .. i + 16][0..16].*;
        const vb: BinVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) {
            return false;
        }
        i += 16;
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

/// 16-lane hardware SIMD scanner locating the first occurrence of `needle` in `haystack` starting at `start_pos`.
pub fn findByteInBytes(haystack: []const u8, needle: u8, start_pos: usize): usize? {
    let i = start_pos;
    const needle_v: BinVec16 = @splat(needle);
    while (i + 16 <= haystack.len) {
        const v: BinVec16 = haystack[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == needle_v)) {
            let j: usize = 0;
            while (j < 16) {
                if (haystack[i + j] == needle) {
                    return i + j;
                }
                j += 1;
            }
        }
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
        const hit = findByteInBytes(haystack[0..(limit + 1)], first, pos);
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
    findByteInBytes,
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
