// @cap/ieee754 — Zero-allocation IEEE-754 f32/f64 binary reader & writer in pure CP
// Clean-room MIT implementation of npm `ieee754` (#293, 173M/wk).

/// Reads a little-endian IEEE-754 32-bit float from `buf[offset..offset+4]`.
pub fn readF32Le(buf: []const u8, offset: usize): f32 {
    if (offset + 4 > buf.len) {
        return 0.0;
    }
    const bits: u32 = (buf[offset] as u32)
        | ((buf[offset + 1] as u32) << 8)
        | ((buf[offset + 2] as u32) << 16)
        | ((buf[offset + 3] as u32) << 24);
    return @as(f32, @bitCast(bits));
}

/// Reads a big-endian IEEE-754 32-bit float from `buf[offset..offset+4]`.
pub fn readF32Be(buf: []const u8, offset: usize): f32 {
    if (offset + 4 > buf.len) {
        return 0.0;
    }
    const bits: u32 = ((buf[offset] as u32) << 24)
        | ((buf[offset + 1] as u32) << 16)
        | ((buf[offset + 2] as u32) << 8)
        | (buf[offset + 3] as u32);
    return @as(f32, @bitCast(bits));
}

/// Writes a little-endian IEEE-754 32-bit float into `buf[offset..offset+4]`.
pub fn writeF32Le(buf: []u8, offset: usize, value: f32): bool {
    if (offset + 4 > buf.len) {
        return false;
    }
    const bits = @as(u32, @bitCast(value));
    buf[offset] = (bits & 0xFF) as u8;
    buf[offset + 1] = ((bits >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((bits >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((bits >> 24) & 0xFF) as u8;
    return true;
}

/// Writes a big-endian IEEE-754 32-bit float into `buf[offset..offset+4]`.
pub fn writeF32Be(buf: []u8, offset: usize, value: f32): bool {
    if (offset + 4 > buf.len) {
        return false;
    }
    const bits = @as(u32, @bitCast(value));
    buf[offset] = ((bits >> 24) & 0xFF) as u8;
    buf[offset + 1] = ((bits >> 16) & 0xFF) as u8;
    buf[offset + 2] = ((bits >> 8) & 0xFF) as u8;
    buf[offset + 3] = (bits & 0xFF) as u8;
    return true;
}

/// Reads a little-endian IEEE-754 64-bit float from `buf[offset..offset+8]`.
pub fn readF64Le(buf: []const u8, offset: usize): f64 {
    if (offset + 8 > buf.len) {
        return 0.0;
    }
    const bits: u64 = (buf[offset] as u64)
        | ((buf[offset + 1] as u64) << 8)
        | ((buf[offset + 2] as u64) << 16)
        | ((buf[offset + 3] as u64) << 24)
        | ((buf[offset + 4] as u64) << 32)
        | ((buf[offset + 5] as u64) << 40)
        | ((buf[offset + 6] as u64) << 48)
        | ((buf[offset + 7] as u64) << 56);
    return @as(f64, @bitCast(bits));
}

/// Reads a big-endian IEEE-754 64-bit float from `buf[offset..offset+8]`.
pub fn readF64Be(buf: []const u8, offset: usize): f64 {
    if (offset + 8 > buf.len) {
        return 0.0;
    }
    const bits: u64 = ((buf[offset] as u64) << 56)
        | ((buf[offset + 1] as u64) << 48)
        | ((buf[offset + 2] as u64) << 40)
        | ((buf[offset + 3] as u64) << 32)
        | ((buf[offset + 4] as u64) << 24)
        | ((buf[offset + 5] as u64) << 16)
        | ((buf[offset + 6] as u64) << 8)
        | (buf[offset + 7] as u64);
    return @as(f64, @bitCast(bits));
}

/// Writes a little-endian IEEE-754 64-bit float into `buf[offset..offset+8]`.
pub fn writeF64Le(buf: []u8, offset: usize, value: f64): bool {
    if (offset + 8 > buf.len) {
        return false;
    }
    const bits = @as(u64, @bitCast(value));
    buf[offset] = (bits & 0xFF) as u8;
    buf[offset + 1] = ((bits >> 8) & 0xFF) as u8;
    buf[offset + 2] = ((bits >> 16) & 0xFF) as u8;
    buf[offset + 3] = ((bits >> 24) & 0xFF) as u8;
    buf[offset + 4] = ((bits >> 32) & 0xFF) as u8;
    buf[offset + 5] = ((bits >> 40) & 0xFF) as u8;
    buf[offset + 6] = ((bits >> 48) & 0xFF) as u8;
    buf[offset + 7] = ((bits >> 56) & 0xFF) as u8;
    return true;
}

/// Writes a big-endian IEEE-754 64-bit float into `buf[offset..offset+8]`.
pub fn writeF64Be(buf: []u8, offset: usize, value: f64): bool {
    if (offset + 8 > buf.len) {
        return false;
    }
    const bits = @as(u64, @bitCast(value));
    buf[offset] = ((bits >> 56) & 0xFF) as u8;
    buf[offset + 1] = ((bits >> 48) & 0xFF) as u8;
    buf[offset + 2] = ((bits >> 40) & 0xFF) as u8;
    buf[offset + 3] = ((bits >> 32) & 0xFF) as u8;
    buf[offset + 4] = ((bits >> 24) & 0xFF) as u8;
    buf[offset + 5] = ((bits >> 16) & 0xFF) as u8;
    buf[offset + 6] = ((bits >> 8) & 0xFF) as u8;
    buf[offset + 7] = (bits & 0xFF) as u8;
    return true;
}

/// Generic npm `ieee754.read(buf, offset, is_le, m_len, n_bytes)` compatible dispatcher (supports 4 & 8 bytes).
pub fn readIeee754(buf: []const u8, offset: usize, is_le: bool, n_bytes: usize): f64 {
    if (n_bytes == 4) {
        if (is_le) { return readF32Le(buf, offset) as f64; }
        return readF32Be(buf, offset) as f64;
    }
    if (n_bytes == 8) {
        if (is_le) { return readF64Le(buf, offset); }
        return readF64Be(buf, offset);
    }
    return 0.0;
}

/// Generic npm `ieee754.write(buf, value, offset, is_le, n_bytes)` compatible dispatcher (supports 4 & 8 bytes).
pub fn writeIeee754(buf: []u8, value: f64, offset: usize, is_le: bool, n_bytes: usize): bool {
    if (n_bytes == 4) {
        const v32: f32 = value as f32;
        if (is_le) { return writeF32Le(buf, offset, v32); }
        return writeF32Be(buf, offset, v32);
    }
    if (n_bytes == 8) {
        if (is_le) { return writeF64Le(buf, offset, value); }
        return writeF64Be(buf, offset, value);
    }
    return false;
}

export {
    readF32Le,
    readF32Be,
    writeF32Le,
    writeF32Be,
    readF64Le,
    readF64Be,
    writeF64Le,
    writeF64Be,
    readIeee754,
    writeIeee754,
};
