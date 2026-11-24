// ============================================================================
// cap:leb128 — Zero-Heap ULEB128 & SLEB128 Variable-Length Integer Codec
// ============================================================================

const std = @import("std");

pub err Leb128Error {
    BufferTooSmall,
    UnexpectedEof,
    Overflow,
}

/// Maximum encoded byte length for 64-bit ULEB128 / SLEB128 values.
pub const MAX_U64_BYTES: usize = 10;
pub const MAX_I64_BYTES: usize = 10;

pub struct DecodeU64Result {
    value: u64,
    bytes_read: usize,
}

pub struct DecodeI64Result {
    value: i64,
    bytes_read: usize,
}

pub struct DecodeUsizeResult {
    value: usize,
    bytes_read: usize,
}

/// Computes the exact ULEB128 byte length of an unsigned 64-bit integer.
pub fn encodedLenU64(value: u64): usize {
    let v: u64 = value;
    let count: usize = 1;
    while (v >= 0x80) {
        v >>= 7;
        count += 1;
    }
    return count;
}

/// Computes the exact SLEB128 byte length of a signed 64-bit integer.
pub fn encodedLenI64(value: i64): usize {
    let v: i64 = value;
    let count: usize = 0;
    while (true) {
        const byte_val: u8 = @as(u8, @truncate(@as(u64, @bitCast(v)) & 0x7F));
        v >>= 7;
        count += 1;
        const sign_set = (byte_val & 0x40) != 0;
        if ((v == 0 && !sign_set) || (v == -1 && sign_set)) {
            break;
        }
    }
    return count;
}

/// Encodes an unsigned `u64` into `buf` using ULEB128. Returns bytes written.
pub fn encodeU64(buf: []u8, value: u64): usize! {
    let v: u64 = value;
    let pos: usize = 0;
    while (true) {
        if (pos >= buf.len) return error.BufferTooSmall;
        let byte_val: u8 = @as(u8, @truncate(v & 0x7F));
        v >>= 7;
        if (v != 0) {
            byte_val |= 0x80;
        }
        buf[pos] = byte_val;
        pos += 1;
        if (v == 0) break;
    }
    return pos;
}

/// Decodes an unsigned `u64` from ULEB128 bytes with strict EOF and overflow guards.
pub fn decodeU64(data: []const u8): DecodeU64Result! {
    let result: u64 = 0;
    let shift: u32 = 0;
    let pos: usize = 0;
    while (true) {
        if (pos >= data.len) return error.UnexpectedEof;
        const byte_val = data[pos];
        pos += 1;
        if (shift >= 64) return error.Overflow;
        const low: u64 = @as(u64, byte_val & 0x7F);
        if (shift == 63) {
            if (low > 1 || (byte_val & 0x80) != 0) return error.Overflow;
        }
        result |= (low << @as(u6, @intCast(shift)));
        if ((byte_val & 0x80) == 0) {
            return DecodeU64Result{
                value: result,
                bytes_read: pos,
            };
        }
        shift += 7;
    }
}

/// Encodes a signed `i64` into `buf` using SLEB128. Returns bytes written.
pub fn encodeI64(buf: []u8, value: i64): usize! {
    let v: i64 = value;
    let pos: usize = 0;
    while (true) {
        if (pos >= buf.len) return error.BufferTooSmall;
        let byte_val: u8 = @as(u8, @truncate(@as(u64, @bitCast(v)) & 0x7F));
        v >>= 7;
        const sign_set = (byte_val & 0x40) != 0;
        const done = (v == 0 && !sign_set) || (v == -1 && sign_set);
        if (!done) {
            byte_val |= 0x80;
        }
        buf[pos] = byte_val;
        pos += 1;
        if (done) break;
    }
    return pos;
}

/// Decodes a signed `i64` from SLEB128 bytes with strict EOF and 64-bit overflow guards.
pub fn decodeI64(data: []const u8): DecodeI64Result! {
    let result: u64 = 0;
    let shift: u32 = 0;
    let pos: usize = 0;
    while (true) {
        if (pos >= data.len) return error.UnexpectedEof;
        const byte_val = data[pos];
        pos += 1;
        if (shift >= 64) return error.Overflow;
        if (shift == 63) {
            if ((byte_val & 0x80) != 0) return error.Overflow;
            if (byte_val != 0x00 && byte_val != 0x7F) return error.Overflow;
        }
        const low: u64 = @as(u64, byte_val & 0x7F);
        result |= (low << @as(u6, @intCast(shift)));
        shift += 7;
        if ((byte_val & 0x80) == 0) {
            if (shift < 64 && (byte_val & 0x40) != 0) {
                result |= (~@as(u64, 0)) << @as(u6, @intCast(shift));
            }
            return DecodeI64Result{
                value: @as(i64, @bitCast(result)),
                bytes_read: pos,
            };
        }
    }
}

/// Encodes a `usize` into `buf` using ULEB128. Returns bytes written.
pub fn encodeUsize(buf: []u8, value: usize): usize! {
    return encodeU64(buf, @as(u64, @intCast(value)));
}

/// Decodes a `usize` from ULEB128 bytes with target-width overflow guard.
pub fn decodeUsize(data: []const u8): DecodeUsizeResult! {
    const res = try decodeU64(data);
    if (res.value > @as(u64, std.math.maxInt(usize))) {
        return error.Overflow;
    }
    return DecodeUsizeResult{
        value: @as(usize, @intCast(res.value)),
        bytes_read: res.bytes_read,
    };
}

/// Writes a fixed-width padded ULEB128 into `slice` (delegates to Zig `std.leb.writeUnsignedExtended`).
pub fn writeUnsignedExtended(slice: []u8, value: u64): void! {
    if (slice.len == 0) return error.BufferTooSmall;
    const needed = encodedLenU64(value);
    if (slice.len < needed) return error.BufferTooSmall;
    std.leb.writeUnsignedExtended(slice, value);
}
