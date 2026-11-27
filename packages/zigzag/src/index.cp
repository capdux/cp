// @cap/zigzag — Zero-allocation 32-bit/64-bit ZigZag & ZigZag-Varint Streaming Codec
// Clean-room MIT implementation in pure CP (Protobuf / Avro / Parquet / CBOR compatible).

pub struct VarintResult {
    pub value: i64,
    pub consumed: usize,
    pub valid: bool,
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
