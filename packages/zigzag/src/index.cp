// @cap/zigzag — Zero-allocation 8/16/32/64-bit ZigZag & ZigZag-Varint Streaming Codec
// Clean-room MIT implementation in pure CP (Protobuf / Avro / Parquet / CBOR compatible).

pub struct VarintResult {
    pub value: i64,
    pub consumed: usize,
    pub valid: bool,
}

/// Encodes a signed 8-bit integer (`-128..127`) into an unsigned 8-bit ZigZag value.
pub fn encodeI8(n: i8) -> u8 {
    const bits: u8 = @bitCast(n);
    const sign_mask: u8 = (0 as u8) -% (bits >> 7);
    return (bits << 1) ^ sign_mask;
}

/// Decodes an unsigned 8-bit ZigZag value back into a signed 8-bit integer.
pub fn decodeU8(u: u8) -> i8 {
    const neg_mask: u8 = (0 as u8) -% (u & 1);
    const bits: u8 = (u >> 1) ^ neg_mask;
    const res: i8 = @bitCast(bits);
    return res;
}

/// Alias for `decodeU8`: decodes an unsigned 8-bit ZigZag value back into `i8`.
pub fn decodeI8(u: u8) -> i8 {
    return decodeU8(u);
}

/// Encodes a signed 16-bit integer (`-32768..32767`) into an unsigned 16-bit ZigZag value.
pub fn encodeI16(n: i16) -> u16 {
    const bits: u16 = @bitCast(n);
    const sign_mask: u16 = (0 as u16) -% (bits >> 15);
    return (bits << 1) ^ sign_mask;
}

/// Decodes an unsigned 16-bit ZigZag value back into a signed 16-bit integer.
pub fn decodeU16(u: u16) -> i16 {
    const neg_mask: u16 = (0 as u16) -% (u & 1);
    const bits: u16 = (u >> 1) ^ neg_mask;
    const res: i16 = @bitCast(bits);
    return res;
}

/// Alias for `decodeU16`: decodes an unsigned 16-bit ZigZag value back into `i16`.
pub fn decodeI16(u: u16) -> i16 {
    return decodeU16(u);
}

/// Encodes a signed 32-bit integer into an unsigned 32-bit ZigZag value
/// (`0->0, -1->1, 1->2, -2->3, 2147483647->4294967294, -2147483648->4294967295`).
pub fn encodeI32(n: i32) -> u32 {
    const bits: u32 = @bitCast(n);
    const sign_mask: u32 = (0 as u32) -% (bits >> 31);
    return (bits << 1) ^ sign_mask;
}

/// Decodes an unsigned 32-bit ZigZag value back into a signed 32-bit integer.
pub fn decodeI32(u: u32) -> i32 {
    const neg_mask: u32 = (0 as u32) -% (u & 1);
    const bits: u32 = (u >> 1) ^ neg_mask;
    const res: i32 = @bitCast(bits);
    return res;
}

/// Alias for `decodeI32`: decodes an unsigned 32-bit ZigZag value back into `i32`.
pub fn decodeU32(u: u32) -> i32 {
    return decodeI32(u);
}

/// Encodes a signed 64-bit integer (`i64::MIN..i64::MAX`) into an unsigned 64-bit ZigZag value without overflow.
pub fn encodeI64(n: i64) -> u64 {
    const bits: u64 = @bitCast(n);
    const sign_mask: u64 = (0 as u64) -% (bits >> 63);
    return (bits << 1) ^ sign_mask;
}

/// Decodes an unsigned 64-bit ZigZag value back into a signed 64-bit integer without overflow.
pub fn decodeI64(u: u64) -> i64 {
    const neg_mask: u64 = (0 as u64) -% (u & 1);
    const bits: u64 = (u >> 1) ^ neg_mask;
    const res: i64 = @bitCast(bits);
    return res;
}

/// Alias for `decodeI64`: decodes an unsigned 64-bit ZigZag value back into `i64`.
pub fn decodeU64(u: u64) -> i64 {
    return decodeI64(u);
}

/// Batch ZigZag-encodes `src` (`[]const i8`) into `dst` (`[]u8`). Returns elements encoded.
pub fn encodeI8Slice(src: []const i8, dst: []u8) -> usize {
    const n: usize = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = encodeI8(src[i]);
        i += 1;
    }
    return n;
}

/// Batch ZigZag-decodes `src` (`[]const u8`) into `dst` (`[]i8`). Returns elements decoded.
pub fn decodeU8Slice(src: []const u8, dst: []i8) -> usize {
    const n: usize = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = decodeU8(src[i]);
        i += 1;
    }
    return n;
}

/// Batch ZigZag-encodes `src` (`[]const i16`) into `dst` (`[]u16`). Returns elements encoded.
pub fn encodeI16Slice(src: []const i16, dst: []u16) -> usize {
    const n: usize = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = encodeI16(src[i]);
        i += 1;
    }
    return n;
}

/// Batch ZigZag-decodes `src` (`[]const u16`) into `dst` (`[]i16`). Returns elements decoded.
pub fn decodeU16Slice(src: []const u16, dst: []i16) -> usize {
    const n: usize = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = decodeU16(src[i]);
        i += 1;
    }
    return n;
}

/// Batch ZigZag-encodes `src` (`[]const i32`) into `dst` (`[]u32`). Returns elements encoded.
pub fn encodeI32Slice(src: []const i32, dst: []u32) -> usize {
    const n: usize = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = encodeI32(src[i]);
        i += 1;
    }
    return n;
}

/// Batch ZigZag-decodes `src` (`[]const u32`) into `dst` (`[]i32`). Returns elements decoded.
pub fn decodeU32Slice(src: []const u32, dst: []i32) -> usize {
    const n: usize = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = decodeU32(src[i]);
        i += 1;
    }
    return n;
}

/// Batch ZigZag-encodes `src` (`[]const i64`) into `dst` (`[]u64`). Returns elements encoded.
pub fn encodeI64Slice(src: []const i64, dst: []u64) -> usize {
    const n: usize = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = encodeI64(src[i]);
        i += 1;
    }
    return n;
}

/// Batch ZigZag-decodes `src` (`[]const u64`) into `dst` (`[]i64`). Returns elements decoded.
pub fn decodeU64Slice(src: []const u64, dst: []i64) -> usize {
    const n: usize = if (src.len < dst.len) { src.len } else { dst.len };
    let i: usize = 0;
    while (i < n) {
        dst[i] = decodeU64(src[i]);
        i += 1;
    }
    return n;
}

/// Computes the exact byte length (1..10) required to ZigZag-Varint encode `n`.
pub fn varintLen(n: i64) -> usize {
    let v: u64 = encodeI64(n);
    let len: usize = 1;
    while (v >= 0x80) {
        v = v >> 7;
        len += 1;
    }
    return len;
}

/// ZigZag-encodes `n` and writes it as a 1..10-byte 7-bit LEB128 Varint into `out`.
/// Returns bytes written, or `0` if `out` is too small.
pub fn encodeZigzagVarint(n: i64, out: []u8) -> usize {
    let v = encodeI64(n);
    let w: usize = 0;

    while (true) {
        if (w >= out.len) {
            return 0;
        }
        if (v < 0x80) {
            out[w] = v as u8;
            return w + 1;
        }
        out[w] = ((v & 0x7F) | 0x80) as u8;
        v = v >> 7;
        w += 1;
    }
}

/// Decodes a ZigZag-Varint starting at `data[start]`.
/// Guards against truncated streams and > 64-bit 10-byte overflow.
pub fn decodeZigzagVarint(data: []const u8, start: usize) -> VarintResult {
    if (start >= data.len) {
        return VarintResult { value: 0, consumed: 0, valid: false };
    }

    let acc: u64 = 0;
    let shift: u32 = 0;
    let i = start;

    while (i < data.len) {
        const b = data[i];
        i += 1;

        if (shift == 63) {
            // 10th byte may only carry bit 0 (value 0 or 1) and must terminate
            if (b > 1) {
                return VarintResult { value: 0, consumed: 0, valid: false };
            }
            acc |= (b as u64) << 63;
            return VarintResult {
                value: decodeI64(acc),
                consumed: i - start,
                valid: true,
            };
        }

        acc |= ((b & 0x7F) as u64) << (shift as u6);
        if ((b & 0x80) == 0) {
            return VarintResult {
                value: decodeI64(acc),
                consumed: i - start,
                valid: true,
            };
        }
        shift += 7;
    }

    // Truncated varint (reached EOF while continuation bit still set)
    return VarintResult { value: 0, consumed: 0, valid: false };
}

/// Batch ZigZag-Varint encodes `src` into `dst`. Returns total bytes written, or `0` on buffer overflow.
pub fn encodeZigzagVarintSlice(src: []const i64, dst: []u8) -> usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        const n = encodeZigzagVarint(src[i], dst[w..dst.len]);
        if (n == 0) {
            return 0;
        }
        w += n;
        i += 1;
    }
    return w;
}

/// Batch ZigZag-Varint decodes values from `src` into `dst`. Returns number of valid integers decoded.
pub fn decodeZigzagVarintSlice(src: []const u8, dst: []i64) -> usize {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < src.len && count < dst.len) {
        const r = decodeZigzagVarint(src, pos);
        if (!r.valid) {
            return count;
        }
        dst[count] = r.value;
        pos += r.consumed;
        count += 1;
    }
    return count;
}
