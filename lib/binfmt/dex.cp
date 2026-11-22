// Origin: Pure-CP Stdlib (Zero-Alloc Android DEX Binary Header & String Table Parser) | Created: 2026-11-22

import std from "std";

pub const DEX_HEADER_SIZE: u32 = 112;
pub const DEX_ENDIAN_CONSTANT: u32 = 0x1234_5678;
pub const DEX_REVERSE_ENDIAN_CONSTANT: u32 = 0x7856_3412;

// Common DEX map_item type codes
pub const DEX_TYPE_HEADER_ITEM: u16 = 0x0000;
pub const DEX_TYPE_STRING_ID_ITEM: u16 = 0x0001;
pub const DEX_TYPE_TYPE_ID_ITEM: u16 = 0x0002;
pub const DEX_TYPE_PROTO_ID_ITEM: u16 = 0x0003;
pub const DEX_TYPE_FIELD_ID_ITEM: u16 = 0x0004;
pub const DEX_TYPE_METHOD_ID_ITEM: u16 = 0x0005;
pub const DEX_TYPE_CLASS_DEF_ITEM: u16 = 0x0006;
pub const DEX_TYPE_MAP_LIST: u16 = 0x1000;
pub const DEX_TYPE_STRING_DATA_ITEM: u16 = 0x2002;

pub struct DexHeader {
    version: [3]u8,
    checksum: u32,
    file_size: u32,
    header_size: u32,
    endian_tag: u32,
    is_big_endian: bool,
    map_off: u32,
    string_ids_size: u32,
    string_ids_off: u32,
    type_ids_size: u32,
    type_ids_off: u32,
    proto_ids_size: u32,
    proto_ids_off: u32,
    field_ids_size: u32,
    field_ids_off: u32,
    method_ids_size: u32,
    method_ids_off: u32,
    class_defs_size: u32,
    class_defs_off: u32,
    data_size: u32,
    data_off: u32,
}

pub struct Uleb128Result {
    value: u32,
    bytes_read: usize,
}

pub struct DexMapItem {
    item_type: u16,
    size: u32,
    offset: u32,
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn readU16LE(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn readU16BE(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readU16(buf: []const u8, off: usize, is_big_endian: bool): u16 {
    if (is_big_endian) return readU16BE(buf, off);
    return readU16LE(buf, off);
}

fn readU32LE(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32) |
        ((buf[off + 1] as u32) << 8) |
        ((buf[off + 2] as u32) << 16) |
        ((buf[off + 3] as u32) << 24);
}

fn readU32BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn readU32(buf: []const u8, off: usize, is_big_endian: bool): u32 {
    if (is_big_endian) return readU32BE(buf, off);
    return readU32LE(buf, off);
}

fn checkSection(count: u32, off: u32, elem_size: u64, file_size: u64): bool {
    if (count == 0) {
        return true;
    }
    if (off == 0) {
        return false;
    }
    const end_u64: u64 = (off as u64) + (count as u64) * elem_size;
    return end_u64 <= file_size;
}

pub fn isDex(data: []const u8): bool {
    if (data.len < 8) {
        return false;
    }
    return data[0] == 'd' &&
        data[1] == 'e' &&
        data[2] == 'x' &&
        data[3] == '\n' &&
        isAsciiDigit(data[4]) &&
        isAsciiDigit(data[5]) &&
        isAsciiDigit(data[6]) &&
        data[7] == 0;
}

pub fn parseDexHeader(data: []const u8): DexHeader? {
    if (data.len < (DEX_HEADER_SIZE as usize) || !isDex(data)) {
        return null;
    }

    // Endian tag at offset 40 determines byte order of all multi-byte header fields
    const raw_endian_le = readU32LE(data, 40);
    let is_big_endian = false;
    if (raw_endian_le == DEX_ENDIAN_CONSTANT) {
        is_big_endian = false;
    } else if (raw_endian_le == DEX_REVERSE_ENDIAN_CONSTANT) {
        is_big_endian = true;
    } else {
        return null;
    }

    const checksum = readU32(data, 8, is_big_endian);
    const file_size = readU32(data, 32, is_big_endian);
    const header_size = readU32(data, 36, is_big_endian);
    const endian_tag = readU32(data, 40, is_big_endian);
    const map_off = readU32(data, 52, is_big_endian);

    const string_ids_size = readU32(data, 56, is_big_endian);
    const string_ids_off = readU32(data, 60, is_big_endian);
    const type_ids_size = readU32(data, 64, is_big_endian);
    const type_ids_off = readU32(data, 68, is_big_endian);
    const proto_ids_size = readU32(data, 72, is_big_endian);
    const proto_ids_off = readU32(data, 76, is_big_endian);
    const field_ids_size = readU32(data, 80, is_big_endian);
    const field_ids_off = readU32(data, 84, is_big_endian);
    const method_ids_size = readU32(data, 88, is_big_endian);
    const method_ids_off = readU32(data, 92, is_big_endian);
    const class_defs_size = readU32(data, 96, is_big_endian);
    const class_defs_off = readU32(data, 100, is_big_endian);
    const data_size = readU32(data, 104, is_big_endian);
    const data_off = readU32(data, 108, is_big_endian);

    if (header_size != DEX_HEADER_SIZE) {
        return null;
    }
    if (file_size < DEX_HEADER_SIZE || (file_size as usize) > data.len) {
        return null;
    }

    const fsize: u64 = file_size as u64;
    if (map_off != 0 && ((map_off as u64) + (4 as u64) > fsize)) {
        return null;
    }
    if (!checkSection(string_ids_size, string_ids_off, 4, fsize)) return null;
    if (!checkSection(type_ids_size, type_ids_off, 4, fsize)) return null;
    if (!checkSection(proto_ids_size, proto_ids_off, 12, fsize)) return null;
    if (!checkSection(field_ids_size, field_ids_off, 8, fsize)) return null;
    if (!checkSection(method_ids_size, method_ids_off, 8, fsize)) return null;
    if (!checkSection(class_defs_size, class_defs_off, 32, fsize)) return null;
    if (!checkSection(data_size, data_off, 1, fsize)) return null;

    let version: [3]u8 = std.mem.zeroes([3]u8);
    version[0] = data[4];
    version[1] = data[5];
    version[2] = data[6];

    return DexHeader{
        .version = version,
        .checksum = checksum,
        .file_size = file_size,
        .header_size = header_size,
        .endian_tag = endian_tag,
        .is_big_endian = is_big_endian,
        .map_off = map_off,
        .string_ids_size = string_ids_size,
        .string_ids_off = string_ids_off,
        .type_ids_size = type_ids_size,
        .type_ids_off = type_ids_off,
        .proto_ids_size = proto_ids_size,
        .proto_ids_off = proto_ids_off,
        .field_ids_size = field_ids_size,
        .field_ids_off = field_ids_off,
        .method_ids_size = method_ids_size,
        .method_ids_off = method_ids_off,
        .class_defs_size = class_defs_size,
        .class_defs_off = class_defs_off,
        .data_size = data_size,
        .data_off = data_off,
    };
}

pub fn readUleb128(data: []const u8, pos: usize): Uleb128Result? {
    if (pos >= data.len) {
        return null;
    }
    let result: u32 = 0;
    let shift: u5 = 0;
    let i: usize = 0;

    while (i < 5 && pos + i < data.len) {
        const b = data[pos + i];
        // 5th byte may only carry 4 low bits (no continuation bit and no bits 4..6)
        if (i == 4 && b > 0x0F) {
            return null;
        }
        result = result | (((b & 0x7F) as u32) << shift);
        if ((b & 0x80) == 0) {
            return Uleb128Result{
                .value = result,
                .bytes_read = i + 1,
            };
        }
        shift += 7;
        i += 1;
    }
    return null;
}

pub fn readDexString(data: []const u8, header: DexHeader, idx: u32): ([]const u8)? {
    if (idx >= header.string_ids_size) {
        return null;
    }
    const bound: usize = if ((header.file_size as usize) <= data.len) (header.file_size as usize) else data.len;
    const entry_off_u64: u64 = (header.string_ids_off as u64) + (idx as u64) * (4 as u64);
    if (entry_off_u64 + (4 as u64) > (bound as u64)) {
        return null;
    }

    const str_data_off: usize = readU32(data, entry_off_u64 as usize, header.is_big_endian) as usize;
    if (str_data_off >= bound) {
        return null;
    }

    const uleb = readUleb128(data[0..bound], str_data_off) ?? return null;
    const str_start = str_data_off + uleb.bytes_read;
    let p = str_start;
    while (p < bound) {
        if (data[p] == 0) {
            return data[str_start..p];
        }
        p += 1;
    }
    return null;
}

pub struct DexMapIter {
    data: []const u8,
    pos: usize,
    remaining: u32,
    is_big_endian: bool,

    fn init(data: []const u8, header: DexHeader): DexMapIter {
        const bound: usize = if ((header.file_size as usize) <= data.len) (header.file_size as usize) else data.len;
        if (header.map_off != 0 && (header.map_off as usize) + 4 <= bound) {
            const count = readU32(data, header.map_off as usize, header.is_big_endian);
            const list_start = (header.map_off as usize) + 4;
            const avail: u64 = (bound - list_start) as u64;
            const needed: u64 = (count as u64) * (12 as u64);
            if (needed <= avail) {
                return DexMapIter{
                    .data = data[0..bound],
                    .pos = list_start,
                    .remaining = count,
                    .is_big_endian = header.is_big_endian,
                };
            }
        }
        return DexMapIter{
            .data = data[0..0],
            .pos = 0,
            .remaining = 0,
            .is_big_endian = header.is_big_endian,
        };
    }

    fn next(*_): DexMapItem? {
        if (_.remaining == 0 || _.pos + 12 > _.data.len) {
            _.remaining = 0;
            return null;
        }
        const item = DexMapItem{
            .item_type = readU16(_.data, _.pos, _.is_big_endian),
            .size = readU32(_.data, _.pos + 4, _.is_big_endian),
            .offset = readU32(_.data, _.pos + 8, _.is_big_endian),
        };
        _.pos += 12;
        _.remaining -= 1;
        return item;
    }
}
