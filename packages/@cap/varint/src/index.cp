// @cap/varint — Zero-allocation 3-in-1 varint codec (LEB128, ZigZag, QUIC RFC 9000) for CP
// Clean-room MIT implementation inspired by Rust `unsigned-varint`, `leb128`, and `integer-encoding`.

const U64_MAX: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
const U64_SIGN_BIT: u64 = (1 as u64) << 63;

pub const QUIC_VARINT_MAX: u64 = ((0x3FFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);

pub struct U64Decode {
    pub ok: bool,
    pub value: u64,
    pub bytes_read: usize,

    pub static fn fail() -> U64Decode {
        return U64Decode{ ok: false, value: 0, bytes_read: 0 };
    }
}

pub struct I64Decode {
    pub ok: bool,
    pub value: i64,
    pub bytes_read: usize,

    pub static fn fail() -> I64Decode {
        return I64Decode{ ok: false, value: 0, bytes_read: 0 };
    }
}

// ---------------------------------------------------------------------------
// Overflow-Safe Two's-Complement Bit Helpers
// ---------------------------------------------------------------------------

fn i64ToBits(v: i64) -> u64 {
    if (v >= 0) {
        return v as u64;
    }
    const mag_minus_1 = (0 - (v + 1)) as u64;
    return U64_MAX - mag_minus_1;
}

fn bitsToI64(u: u64) -> i64 {
    if ((u & U64_SIGN_BIT) == 0) {
        return u as i64;
    }
    const mag_minus_1 = (U64_MAX - u) as i64;
    return -mag_minus_1 - 1;
}

// ---------------------------------------------------------------------------
// 1. Unsigned LEB128 (WebAssembly / Protobuf / libp2p / DWARF)
// ---------------------------------------------------------------------------

/// Computes the encoded byte length (`1..=10`) of `v` in Unsigned LEB128.
pub fn uleb128Size(v: u64) -> usize {
    let val = v;
    let n: usize = 1;
    while (val >= 0x80) {
        val >>= 7;
        n += 1;
    }
    return n;
}

/// Encodes `v` as Unsigned LEB128 into `out`, returning bytes written (`1..=10`), or `null` if `out` is too small.
pub fn encodeUleb128(v: u64, out: []u8) -> usize? {
    let val = v;
    let pos: usize = 0;
    while (true) {
        if (pos >= out.len) {
            return null;
        }
        let byte = (val & 0x7F) as u8;
        val >>= 7;
        if (val != 0) {
            byte |= 0x80;
        }
        out[pos] = byte;
        pos += 1;
        if (val == 0) {
            break;
        }
    }
    return pos;
}

/// Strictly decodes an Unsigned LEB128 value from `data`.
/// Rejects truncated streams and 64-bit overflows (10th byte `> 0x01`).
pub fn decodeUleb128(data: string) -> U64Decode {
    let res: u64 = 0;
    let shift: u64 = 0;
    let i: usize = 0;
    while (i < data.len) {
        const b = data[i];
        if (shift == 63 && b > 0x01) {
            return U64Decode.fail();
        }
        res |= ((b & 0x7F) as u64) << shift;
        i += 1;
        if ((b & 0x80) == 0) {
            return U64Decode{ ok: true, value: res, bytes_read: i };
        }
        if (shift >= 63) {
            return U64Decode.fail();
        }
        shift += 7;
    }
    return U64Decode.fail();
}

// ---------------------------------------------------------------------------
// 2. ZigZag & Signed LEB128 (Protobuf `sint64` & DWARF/WASM SLEB128)
// ---------------------------------------------------------------------------

/// Maps signed `i64` to `u64` via ZigZag (`0->0, -1->1, 1->2, -2->3, ...`), overflow-safe across full `i64`.
pub fn zigzagEncode(v: i64) -> u64 {
    if (v >= 0) {
        return (v as u64) << 1;
    }
    const mag_minus_1 = (0 - (v + 1)) as u64;
    return (mag_minus_1 << 1) | 1;
}

/// Reverses `zigzagEncode` across the full `u64` -> `i64` range.
pub fn zigzagDecode(u: u64) -> i64 {
    const half = u >> 1;
    if ((u & 1) == 0) {
        return half as i64;
    }
    return -(half as i64) - 1;
}

/// Encodes `v` as Signed LEB128 into `out`, returning bytes written (`1..=10`), or `null` if `out` is too small.
pub fn encodeSleb128(v: i64, out: []u8) -> usize? {
    let u = i64ToBits(v);
    const sign_fill: u64 = (0x7F as u64) << 57;
    let pos: usize = 0;
    while (true) {
        if (pos >= out.len) {
            return null;
        }
        let byte = (u & 0x7F) as u8;
        const is_neg = (u & U64_SIGN_BIT) != 0;
        u = if (is_neg) ((u >> 7) | sign_fill) else (u >> 7);

        const done = (u == 0 && (byte & 0x40) == 0) || (u == U64_MAX && (byte & 0x40) != 0);
        if (!done) {
            byte |= 0x80;
        }
        out[pos] = byte;
        pos += 1;
        if (done) {
            break;
        }
    }
    return pos;
}

/// Strictly decodes a Signed LEB128 value from `data`, rejecting truncated streams and 64-bit overflows.
pub fn decodeSleb128(data: string) -> I64Decode {
    let res: u64 = 0;
    let shift: u64 = 0;
    let i: usize = 0;
    while (i < data.len) {
        const b = data[i];
        if (shift == 63) {
            // 10th byte must terminate and carry valid sign extension (0x00 for >=0, 0x7F for <0)
            if ((b & 0x80) != 0 || (b != 0x00 && b != 0x7F)) {
                return I64Decode.fail();
            }
            res |= ((b & 0x01) as u64) << 63;
            return I64Decode{ ok: true, value: bitsToI64(res), bytes_read: i + 1 };
        }

        res |= ((b & 0x7F) as u64) << shift;
        shift += 7;
        i += 1;

        if ((b & 0x80) == 0) {
            if (shift < 64 && (b & 0x40) != 0) {
                res |= U64_MAX << shift;
            }
            return I64Decode{ ok: true, value: bitsToI64(res), bytes_read: i };
        }
    }
    return I64Decode.fail();
}

/// Encodes `v` as Protobuf `sint64` (ZigZag + ULEB128) into `out`.
pub fn encodeZigzagVarint(v: i64, out: []u8) -> usize? {
    return encodeUleb128(zigzagEncode(v), out);
}

/// Decodes Protobuf `sint64` (ULEB128 + ZigZag) from `data`.
pub fn decodeZigzagVarint(data: string) -> I64Decode {
    const u = decodeUleb128(data);
    if (!u.ok) {
        return I64Decode.fail();
    }
    return I64Decode{
        ok: true,
        value: zigzagDecode(u.value),
        bytes_read: u.bytes_read,
    };
}

// ---------------------------------------------------------------------------
// 3. QUIC RFC 9000 Variable-Length Integer (1 / 2 / 4 / 8 bytes, 0..2^62-1)
// ---------------------------------------------------------------------------

/// Encodes `v` (`0..=0x3FFFFFFFFFFFFFFF`) as a QUIC RFC 9000 variable-length integer into `out`.
/// Returns `null` if `v > QUIC_VARINT_MAX` or `out` is too small.
pub fn encodeQuicVarint(v: u64, out: []u8) -> usize? {
    if (v <= 0x3F) {
        if (out.len < 1) return null;
        out[0] = v as u8;
        return 1 as usize;
    }
    if (v <= 0x3FFF) {
        if (out.len < 2) return null;
        out[0] = (0x40 | ((v >> 8) & 0x3F)) as u8;
        out[1] = (v & 0xFF) as u8;
        return 2 as usize;
    }
    if (v <= 0x3FFFFFFF) {
        if (out.len < 4) return null;
        out[0] = (0x80 | ((v >> 24) & 0x3F)) as u8;
        out[1] = ((v >> 16) & 0xFF) as u8;
        out[2] = ((v >> 8) & 0xFF) as u8;
        out[3] = (v & 0xFF) as u8;
        return 4 as usize;
    }
    if (v <= QUIC_VARINT_MAX) {
        if (out.len < 8) return null;
        out[0] = (0xC0 | ((v >> 56) & 0x3F)) as u8;
        out[1] = ((v >> 48) & 0xFF) as u8;
        out[2] = ((v >> 40) & 0xFF) as u8;
        out[3] = ((v >> 32) & 0xFF) as u8;
        out[4] = ((v >> 24) & 0xFF) as u8;
        out[5] = ((v >> 16) & 0xFF) as u8;
        out[6] = ((v >> 8) & 0xFF) as u8;
        out[7] = (v & 0xFF) as u8;
        return 8 as usize;
    }
    return null;
}

/// Decodes a QUIC RFC 9000 variable-length integer (1/2/4/8 bytes) from `data`.
pub fn decodeQuicVarint(data: string) -> U64Decode {
    if (data.len == 0) {
        return U64Decode.fail();
    }
    const prefix = data[0] >> 6;
    const width: usize = (1 as usize) << (prefix as usize);
    if (data.len < width) {
        return U64Decode.fail();
    }
    let val: u64 = (data[0] & 0x3F) as u64;
    let i: usize = 1;
    while (i < width) {
        val = (val << 8) | (data[i] as u64);
        i += 1;
    }
    return U64Decode{ ok: true, value: val, bytes_read: width };
}
