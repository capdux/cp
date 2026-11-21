// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 8794 EBML / WebM / MKV Parser)

import std from "std";

pub const EBML_ID_HEADER: u32 = 0x1A45DFA3;
pub const EBML_ID_VERSION: u32 = 0x4286;
pub const EBML_ID_DOCTYPE: u32 = 0x4282;
pub const EBML_ID_SEGMENT: u32 = 0x18538067;

pub struct EbmlVint {
    value: u64,
    raw_id: u32,
    bytes_read: u8,
    is_unknown_size: bool,
}

pub struct EbmlElement {
    id: u32,
    size: u64,
    is_unknown_size: bool,
    payload: []const u8,
}

fn vintWidth(first_byte: u8): u8 {
    if (first_byte == 0) return 0;
    if ((first_byte & 0x80) != 0) return 1;
    if ((first_byte & 0x40) != 0) return 2;
    if ((first_byte & 0x20) != 0) return 3;
    if ((first_byte & 0x10) != 0) return 4;
    if ((first_byte & 0x08) != 0) return 5;
    if ((first_byte & 0x04) != 0) return 6;
    if ((first_byte & 0x02) != 0) return 7;
    return 8;
}

fn vintDataMask(width: u8): u8 {
    if (width == 1) return 0x7F;
    if (width == 2) return 0x3F;
    if (width == 3) return 0x1F;
    if (width == 4) return 0x0F;
    if (width == 5) return 0x07;
    if (width == 6) return 0x03;
    if (width == 7) return 0x01;
    return 0x00;
}

fn vintAllOnes(width: u8): u64 {
    const bits: u6 = (width * 7) as u6;
    return ((1 as u64) << bits) - (1 as u64);
}

pub fn decodeEbmlId(data: []const u8): EbmlVint? {
    if (data.len == 0) {
        return null;
    }
    const width = vintWidth(data[0]);
    // RFC 8794 §5: Element ID width must be 1..4 bytes
    if (width == 0 || width > 4 || (width as usize) > data.len) {
        return null;
    }

    const mask = vintDataMask(width);
    let value: u64 = (data[0] & mask) as u64;
    let raw_id: u32 = data[0] as u32;

    let i: usize = 1;
    while (i < (width as usize)) {
        value = (value << 8) | (data[i] as u64);
        raw_id = (raw_id << 8) | (data[i] as u32);
        i += 1;
    }

    // RFC 8794 §5.1: all-zero and all-one VINT_DATA are reserved/invalid IDs
    const all_ones = vintAllOnes(width);
    if (value == 0 || value == all_ones) {
        return null;
    }

    return EbmlVint{
        .value = value,
        .raw_id = raw_id,
        .bytes_read = width,
        .is_unknown_size = false,
    };
}

pub fn decodeEbmlSize(data: []const u8): EbmlVint? {
    if (data.len == 0) {
        return null;
    }
    const width = vintWidth(data[0]);
    if (width == 0 || (width as usize) > data.len) {
        return null;
    }

    const mask = vintDataMask(width);
    let value: u64 = (data[0] & mask) as u64;

    let i: usize = 1;
    while (i < (width as usize)) {
        value = (value << 8) | (data[i] as u64);
        i += 1;
    }

    const all_ones = vintAllOnes(width);
    return EbmlVint{
        .value = value,
        .raw_id = 0,
        .bytes_read = width,
        .is_unknown_size = value == all_ones,
    };
}

pub fn isEbmlHeader(data: []const u8): bool {
    if (data.len < 4) {
        return false;
    }
    return data[0] == 0x1A && data[1] == 0x45 && data[2] == 0xDF && data[3] == 0xA3;
}

pub struct EbmlElementIter {
    data: []const u8,
    pos: usize,

    fn init(data: []const u8): EbmlElementIter {
        return EbmlElementIter{
            .data = data,
            .pos = 0,
        };
    }

    fn next(*_): EbmlElement? {
        if (_.pos >= _.data.len) {
            return null;
        }

        const id_opt = decodeEbmlId(_.data[_.pos..]);
        if (id_opt == null) {
            _.pos = _.data.len;
            return null;
        }
        const id_vint = id_opt ?? unreachable;
        const size_pos = _.pos + (id_vint.bytes_read as usize);

        const size_opt = decodeEbmlSize(_.data[size_pos..]);
        if (size_opt == null) {
            _.pos = _.data.len;
            return null;
        }
        const size_vint = size_opt ?? unreachable;
        const payload_start = size_pos + (size_vint.bytes_read as usize);

        if (size_vint.is_unknown_size) {
            _.pos = _.data.len;
            return EbmlElement{
                .id = id_vint.raw_id,
                .size = size_vint.value,
                .is_unknown_size = true,
                .payload = _.data[payload_start.._.data.len],
            };
        }

        const remaining: u64 = (_.data.len - payload_start) as u64;
        if (size_vint.value > remaining) {
            _.pos = _.data.len;
            return null;
        }

        const payload_end = payload_start + (size_vint.value as usize);
        _.pos = payload_end;

        return EbmlElement{
            .id = id_vint.raw_id,
            .size = size_vint.value,
            .is_unknown_size = false,
            .payload = _.data[payload_start..payload_end],
        };
    }
}

pub fn decodeEbmlUint(payload: []const u8): u64? {
    if (payload.len > 8) {
        return null;
    }
    let v: u64 = 0;
    let i: usize = 0;
    while (i < payload.len) {
        v = (v << 8) | (payload[i] as u64);
        i += 1;
    }
    return v;
}
