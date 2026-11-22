// Source: cp/lib/varint_leb128/varint.cp (Pure CP Unsigned/Signed LEB128 & ZigZag Varint)
const std = @import("std");

pub const VarintError = error{
    UnexpectedEof,
    Overflow,
    BufferTooSmall,
};

pub const DecodedU64 = struct {
    value: u64,
    bytes_read: usize,
};

pub const DecodedI64 = struct {
    value: i64,
    bytes_read: usize,
};

pub fn zigzagEncode64(v: i64): u64 {
    const uv: u64 = @bitCast(v);
    const sign_mask: u64 = @bitCast(v >> 63);
    return (uv << 1) ^ sign_mask;
}

pub fn zigzagDecode64(v: u64): i64 {
    const shifted = v >> 1;
    const neg_mask: u64 = @as(u64, 0) -% (v & 1);
    return @bitCast(shifted ^ neg_mask);
}

pub fn zigzagEncode32(v: i32): u32 {
    const uv: u32 = @bitCast(v);
    const sign_mask: u32 = @bitCast(v >> 31);
    return (uv << 1) ^ sign_mask;
}

pub fn zigzagDecode32(v: u32): i32 {
    const shifted = v >> 1;
    const neg_mask: u32 = @as(u32, 0) -% (v & 1);
    return @bitCast(shifted ^ neg_mask);
}

pub fn encodeUleb128(val: u64, out: []u8): VarintError!usize {
    let v = val;
    let i: usize = 0;
    while (true) {
        if (i >= out.len) { return VarintError.BufferTooSmall; }
        let b: u8 = @intCast(v & 0x7F);
        v >>= 7;
        if (v != 0) {
            b |= 0x80;
            out[i] = b;
            i += 1;
        } else {
            out[i] = b;
            return i + 1;
        }
    }
}

pub fn decodeUleb128(data: []const u8): VarintError!DecodedU64 {
    let result: u64 = 0;
    let shift: u6 = 0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (i >= 10) { return VarintError.Overflow; }
        const b = data[i];
        const low: u64 = @intCast(b & 0x7F);
        if (i == 9 and low > 1) { return VarintError.Overflow; }
        result |= low << shift;
        if ((b & 0x80) == 0) {
            return DecodedU64{ .value = result, .bytes_read = i + 1 };
        }
        shift += 7;
    }
    return VarintError.UnexpectedEof;
}

pub fn encodeSleb128(val: i64, out: []u8): VarintError!usize {
    let v = val;
    let i: usize = 0;
    while (true) {
        if (i >= out.len) { return VarintError.BufferTooSmall; }
        let b: u8 = @intCast(@as(i64, v) & 0x7F);
        v >>= 7;
        const sign_bit = (b & 0x40) != 0;
        const done = (v == 0 and !sign_bit) or (v == -1 and sign_bit);
        if (!done) {
            b |= 0x80;
            out[i] = b;
            i += 1;
        } else {
            out[i] = b;
            return i + 1;
        }
    }
}

pub fn decodeSleb128(data: []const u8): VarintError!DecodedI64 {
    let result: u64 = 0;
    let shift: u6 = 0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (i >= 10) { return VarintError.Overflow; }
        const b = data[i];
        const low: u64 = @intCast(b & 0x7F);
        result |= low << shift;
        shift = if (shift + 7 > 63) 63 else shift + 7;
        if ((b & 0x80) == 0) {
            if (i < 9 and (b & 0x40) != 0) {
                const mask_shift: u6 = @intCast((i + 1) * 7);
                result |= (~@as(u64, 0)) << mask_shift;
            }
            return DecodedI64{
                .value = @bitCast(result),
                .bytes_read = i + 1,
            };
        }
    }
    return VarintError.UnexpectedEof;
}
