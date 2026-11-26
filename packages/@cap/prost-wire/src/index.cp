// @cap/prost-wire — Zero-allocation Protocol Buffers Wire Format codec
// Reuses @cap/varint (ULEB128 + ZigZag) with zero duplicated varint/zigzag code
// MIT License

import { U64Decode, encodeUleb128, decodeUleb128, zigzagEncode, zigzagDecode } from "../../varint/src/index.cp";

pub const WIRE_VARINT: u8 = 0;
pub const WIRE_I64: u8 = 1;
pub const WIRE_LEN: u8 = 2;
pub const WIRE_I32: u8 = 5;

const MAX_FIELD_NUMBER: u64 = 536870911; // (1 << 29) - 1

pub struct FieldTag {
    pub field_number: u32,
    pub wire_type: u8,
    pub bytes_read: usize,
    pub ok: bool,
}

pub struct BytesField {
    pub start: usize,
    pub len: usize,
    pub next_pos: usize,
    pub ok: bool,
}

pub struct Sint64Decode {
    pub value: i64,
    pub bytes_read: usize,
    pub ok: bool,
}

fn isValidWireType(wt: u8): bool {
    return wt == WIRE_VARINT || wt == WIRE_I64 || wt == WIRE_LEN || wt == WIRE_I32;
}

pub fn encodeTag(field_number: u32, wire_type: u8, out: []u8): usize {
    let fnum: u64 = field_number as u64;
    if (fnum == 0 || fnum > MAX_FIELD_NUMBER || !isValidWireType(wire_type)) {
        return 0;
    }
    let key: u64 = (fnum << 3) | (wire_type as u64);
    return encodeUleb128(key, out) ?? 0;
}

pub fn decodeTag(buf: []const u8): FieldTag {
    let d: U64Decode = decodeUleb128(buf);
    if (!d.ok) {
        return FieldTag { field_number: 0, wire_type: 0, bytes_read: 0, ok: false };
    }
    let fnum: u64 = d.value >> 3;
    let wt: u8 = (d.value & 7) as u8;
    if (fnum == 0 || fnum > MAX_FIELD_NUMBER || !isValidWireType(wt)) {
        return FieldTag { field_number: 0, wire_type: 0, bytes_read: 0, ok: false };
    }
    return FieldTag {
        field_number: fnum as u32,
        wire_type: wt,
        bytes_read: d.bytes_read,
        ok: true,
    };
}

pub fn encodeVarintField(field_number: u32, val: u64, out: []u8): usize {
    let tag_n: usize = encodeTag(field_number, WIRE_VARINT, out);
    if (tag_n == 0) {
        return 0;
    }
    let val_n: usize = encodeUleb128(val, out[tag_n..out.len]) ?? 0;
    if (val_n == 0) {
        return 0;
    }
    return tag_n + val_n;
}

pub fn encodeSint64Field(field_number: u32, val: i64, out: []u8): usize {
    let zz: u64 = zigzagEncode(val);
    return encodeVarintField(field_number, zz, out);
}

pub fn decodeVarintValue(buf: []const u8, pos: usize): U64Decode {
    if (pos >= buf.len) {
        return U64Decode { ok: false, value: 0, bytes_read: 0 };
    }
    return decodeUleb128(buf[pos..buf.len]);
}

pub fn decodeSint64Value(buf: []const u8, pos: usize): Sint64Decode {
    let d: U64Decode = decodeVarintValue(buf, pos);
    if (!d.ok) {
        return Sint64Decode { value: 0, bytes_read: 0, ok: false };
    }
    return Sint64Decode {
        value: zigzagDecode(d.value),
        bytes_read: d.bytes_read,
        ok: true,
    };
}

pub fn encodeFixed32Field(field_number: u32, val: u32, out: []u8): usize {
    let tag_n: usize = encodeTag(field_number, WIRE_I32, out);
    if (tag_n == 0 || out.len - tag_n < 4) {
        return 0;
    }
    out[tag_n] = (val & 0xFF) as u8;
    out[tag_n + 1] = ((val >> 8) & 0xFF) as u8;
    out[tag_n + 2] = ((val >> 16) & 0xFF) as u8;
    out[tag_n + 3] = ((val >> 24) & 0xFF) as u8;
    return tag_n + 4;
}

pub fn decodeFixed32(buf: []const u8, pos: usize): u32 {
    if (pos > buf.len || buf.len - pos < 4) {
        return 0;
    }
    let b0: u32 = buf[pos] as u32;
    let b1: u32 = (buf[pos + 1] as u32) << 8;
    let b2: u32 = (buf[pos + 2] as u32) << 16;
    let b3: u32 = (buf[pos + 3] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

pub fn encodeFixed64Field(field_number: u32, val: u64, out: []u8): usize {
    let tag_n: usize = encodeTag(field_number, WIRE_I64, out);
    if (tag_n == 0 || out.len - tag_n < 8) {
        return 0;
    }
    out[tag_n] = (val & 0xFF) as u8;
    out[tag_n + 1] = ((val >> 8) & 0xFF) as u8;
    out[tag_n + 2] = ((val >> 16) & 0xFF) as u8;
    out[tag_n + 3] = ((val >> 24) & 0xFF) as u8;
    out[tag_n + 4] = ((val >> 32) & 0xFF) as u8;
    out[tag_n + 5] = ((val >> 40) & 0xFF) as u8;
    out[tag_n + 6] = ((val >> 48) & 0xFF) as u8;
    out[tag_n + 7] = ((val >> 56) & 0xFF) as u8;
    return tag_n + 8;
}

pub fn decodeFixed64(buf: []const u8, pos: usize): u64 {
    if (pos > buf.len || buf.len - pos < 8) {
        return 0;
    }
    let lo: u64 = decodeFixed32(buf, pos) as u64;
    let hi: u64 = decodeFixed32(buf, pos + 4) as u64;
    return lo | (hi << 32);
}

pub fn encodeBytesField(field_number: u32, payload: string, out: []u8): usize {
    let tag_n: usize = encodeTag(field_number, WIRE_LEN, out);
    if (tag_n == 0) {
        return 0;
    }
    let len_n: usize = encodeUleb128(payload.len as u64, out[tag_n..out.len]) ?? 0;
    if (len_n == 0) {
        return 0;
    }
    let hdr_n: usize = tag_n + len_n;
    if (out.len - hdr_n < payload.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < payload.len) {
        out[hdr_n + i] = payload[i];
        i = i + 1;
    }
    return hdr_n + payload.len;
}

pub fn decodeBytesField(buf: []const u8, pos: usize): BytesField {
    if (pos >= buf.len) {
        return BytesField { start: 0, len: 0, next_pos: 0, ok: false };
    }
    let d: U64Decode = decodeUleb128(buf[pos..buf.len]);
    if (!d.ok) {
        return BytesField { start: 0, len: 0, next_pos: 0, ok: false };
    }
    let start: usize = pos + d.bytes_read;
    let avail: usize = buf.len - start;
    if (d.value > (avail as u64)) {
        return BytesField { start: 0, len: 0, next_pos: 0, ok: false };
    }
    let plen: usize = d.value as usize;
    return BytesField {
        start: start,
        len: plen,
        next_pos: start + plen,
        ok: true,
    };
}

pub fn skipField(buf: []const u8, pos: usize, wire_type: u8): i64 {
    if (pos > buf.len) {
        return -1;
    }
    if (wire_type == WIRE_VARINT) {
        let d: U64Decode = decodeVarintValue(buf, pos);
        if (!d.ok) {
            return -1;
        }
        return (pos + d.bytes_read) as i64;
    }
    if (wire_type == WIRE_I64) {
        if (buf.len - pos < 8) {
            return -1;
        }
        return (pos + 8) as i64;
    }
    if (wire_type == WIRE_LEN) {
        let bf: BytesField = decodeBytesField(buf, pos);
        if (!bf.ok) {
            return -1;
        }
        return bf.next_pos as i64;
    }
    if (wire_type == WIRE_I32) {
        if (buf.len - pos < 4) {
            return -1;
        }
        return (pos + 4) as i64;
    }
    return -1;
}
