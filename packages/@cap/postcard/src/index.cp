// @cap/postcard — Zero-heap compact binary wire protocol codecs
// Varint (LEB128), ZigZag signed integer encoding, and COBS framing.

pub struct VarintU64Res {
    ok: bool,
    value: u64,
    consumed: usize,
}

pub struct VarintI64Res {
    ok: bool,
    value: i64,
    consumed: usize,
}

fn pcrdU64Max(): u64 {
    return ((u64(0xFFFF_FFFF) << 32) | u64(0xFFFF_FFFF));
}

pub fn encodeVarintU64(val: u64, out: []u8): usize {
    let v: u64 = val;
    let pos: usize = 0;
    while (true) {
        if (pos >= out.len) {
            return 0;
        }
        let b: u8 = u8(v & 0x7F);
        v = v >> 7;
        if (v != 0) {
            b |= 0x80;
        }
        out[pos] = b;
        pos += 1;
        if (v == 0) {
            break;
        }
    }
    return pos;
}

pub fn decodeVarintU64(data: []const u8): VarintU64Res {
    let val: u64 = 0;
    let shift: usize = 0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        if (i >= 10) {
            return VarintU64Res{ ok: false, value: 0, consumed: 0 };
        }
        const byte: u8 = data[i];
        if (i == 9 && (byte & 0xFE) != 0) {
            return VarintU64Res{ ok: false, value: 0, consumed: 0 };
        }
        const sh = @as(u6, @intCast(shift));
        val |= u64(byte & 0x7F) << sh;
        if ((byte & 0x80) == 0) {
            return VarintU64Res{ ok: true, value: val, consumed: i + 1 };
        }
        shift += 7;
    }
    return VarintU64Res{ ok: false, value: 0, consumed: 0 };
}

pub fn zigzagEncode(val: i64): u64 {
    const u = @as(u64, @bitCast(val));
    const sign_mask = @as(u64, @bitCast(val >> 63));
    return (u << 1) ^ sign_mask;
}

pub fn zigzagDecode(val: u64): i64 {
    const half: u64 = val >> 1;
    let mask: u64 = 0;
    if ((val & 1) != 0) {
        mask = pcrdU64Max();
    }
    const raw: u64 = half ^ mask;
    return @as(i64, @bitCast(raw));
}

pub fn encodeVarintI64(val: i64, out: []u8): usize {
    return encodeVarintU64(zigzagEncode(val), out);
}

pub fn decodeVarintI64(data: []const u8): VarintI64Res {
    const r = decodeVarintU64(data);
    if (!r.ok) {
        return VarintI64Res{ ok: false, value: 0, consumed: 0 };
    }
    return VarintI64Res{ ok: true, value: zigzagDecode(r.value), consumed: r.consumed };
}

pub fn cobsEncode(src: []const u8, out: []u8): usize {
    if (out.len == 0) {
        return 0;
    }
    if (src.len == 0) {
        out[0] = 1;
        return 1;
    }
    let code_idx: usize = 0;
    let write_idx: usize = 1;
    let code: u8 = 1;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        const b: u8 = src[i];
        if (b == 0) {
            out[code_idx] = code;
            if (write_idx >= out.len) {
                return 0;
            }
            code_idx = write_idx;
            write_idx += 1;
            code = 1;
        } else {
            if (write_idx >= out.len) {
                return 0;
            }
            out[write_idx] = b;
            write_idx += 1;
            code += 1;
            if (code == 0xFF && i + 1 < src.len) {
                out[code_idx] = code;
                if (write_idx >= out.len) {
                    return 0;
                }
                code_idx = write_idx;
                write_idx += 1;
                code = 1;
            }
        }
    }
    out[code_idx] = code;
    return write_idx;
}

pub fn cobsDecode(src: []const u8, out: []u8): i64 {
    if (src.len == 0) {
        return -1;
    }
    let read_idx: usize = 0;
    let write_idx: usize = 0;
    while (read_idx < src.len) {
        const code: u8 = src[read_idx];
        if (code == 0) {
            return -1;
        }
        read_idx += 1;
        const copy_len: usize = usize(code - 1);
        if (read_idx + copy_len > src.len) {
            return -1;
        }
        let j: usize = 0;
        while (j < copy_len) : (j += 1) {
            const b: u8 = src[read_idx];
            if (b == 0) {
                return -1;
            }
            if (write_idx >= out.len) {
                return -1;
            }
            out[write_idx] = b;
            write_idx += 1;
            read_idx += 1;
        }
        if (code < 0xFF && read_idx < src.len) {
            if (write_idx >= out.len) {
                return -1;
            }
            out[write_idx] = 0;
            write_idx += 1;
        }
    }
    return i64(write_idx);
}
