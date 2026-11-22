// Origin: Pure-CP Stdlib (Zero-Alloc W3C WOFF2 Font Header & Table Directory Parser) | Created: 2026-11-22

import std from "std";

pub const WOFF2_HEADER_SIZE: usize = 48;
pub const WOFF2_SIGNATURE: u32 = 0x774F4632; // "wOF2"
pub const WOFF2_FLAVOR_TRUETYPE: u32 = 0x00010000;
pub const WOFF2_FLAVOR_CFF: u32 = 0x4F54544F; // "OTTO"
pub const WOFF2_FLAVOR_COLLECTION: u32 = 0x74746366; // "ttcf"

// W3C WOFF2 §5.1 Table 1: 63 known table tags (63 * 4 = 252 bytes)
const KNOWN_TABLE_TAGS: []const u8 =
    "cmap" ++ "head" ++ "hhea" ++ "hmtx" ++ "maxp" ++ "name" ++ "OS/2" ++ "post" ++
    "cvt " ++ "fpgm" ++ "glyf" ++ "loca" ++ "prep" ++ "CFF " ++ "VORG" ++ "EBDT" ++
    "EBLC" ++ "gasp" ++ "hdmx" ++ "kern" ++ "LTSH" ++ "PCLT" ++ "VDMX" ++ "vhea" ++
    "vmtx" ++ "BASE" ++ "GDEF" ++ "GPOS" ++ "GSUB" ++ "EBSC" ++ "JSTF" ++ "MATH" ++
    "CBDT" ++ "CBLC" ++ "COLR" ++ "CPAL" ++ "SVG " ++ "sbix" ++ "acnt" ++ "avar" ++
    "bdat" ++ "bloc" ++ "bsln" ++ "cvar" ++ "fdsc" ++ "feat" ++ "fmtx" ++ "fvar" ++
    "gvar" ++ "hsty" ++ "just" ++ "lcar" ++ "mort" ++ "morx" ++ "opbd" ++ "prop" ++
    "trak" ++ "Zapf" ++ "Silf" ++ "Glat" ++ "Gloc" ++ "Feat" ++ "Sill";

pub struct Woff2Header {
    signature: u32,
    flavor: u32,
    length: u32,
    num_tables: u16,
    reserved: u16,
    total_sfnt_size: u32,
    total_compressed_size: u32,
    major_version: u16,
    minor_version: u16,
    meta_offset: u32,
    meta_length: u32,
    meta_orig_length: u32,
    priv_offset: u32,
    priv_length: u32,

    fn isTrueType(): bool {
        return _.flavor == WOFF2_FLAVOR_TRUETYPE;
    }

    fn isCff(): bool {
        return _.flavor == WOFF2_FLAVOR_CFF;
    }

    fn isCollection(): bool {
        return _.flavor == WOFF2_FLAVOR_COLLECTION;
    }

    fn hasMetadata(): bool {
        return _.meta_offset > 0 && _.meta_length > 0;
    }
}

pub struct UIntBase128Result {
    value: u32,
    bytes_read: usize,
}

pub struct Woff2TableEntry {
    tag: [4]u8,
    flags: u8,
    transform_version: u8,
    orig_length: u32,
    transform_length: u32?,
}

fn readU16BE(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU32BE(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

fn tagEql(tag: [4]u8, lit: []const u8): bool {
    if (lit.len != 4) return false;
    return tag[0] == lit[0] && tag[1] == lit[1] && tag[2] == lit[2] && tag[3] == lit[3];
}

pub fn decodeUIntBase128(data: []const u8): UIntBase128Result? {
    if (data.len == 0) {
        return null;
    }
    let accum: u32 = 0;
    let i: usize = 0;
    while (i < 5 && i < data.len) {
        const b = data[i];
        // W3C WOFF2 §4.1: leading 0x80 (non-minimal encoding) is rejected
        if (i == 0 && b == 0x80) {
            return null;
        }
        // Prevent u32 overflow before left shift by 7
        if ((accum & 0xFE00_0000) != 0) {
            return null;
        }
        accum = (accum << 7) | ((b & 0x7F) as u32);
        if ((b & 0x80) == 0) {
            return UIntBase128Result{
                .value = accum,
                .bytes_read = i + 1,
            };
        }
        i += 1;
    }
    return null;
}

pub fn isWoff2(data: []const u8): bool {
    if (data.len < 4) {
        return false;
    }
    return readU32BE(data, 0) == WOFF2_SIGNATURE;
}

pub fn parseWoff2Header(data: []const u8): Woff2Header? {
    if (data.len < WOFF2_HEADER_SIZE) {
        return null;
    }
    const sig = readU32BE(data, 0);
    if (sig != WOFF2_SIGNATURE) {
        return null;
    }

    const flavor = readU32BE(data, 4);
    const length = readU32BE(data, 8);
    const num_tables = readU16BE(data, 12);
    const reserved = readU16BE(data, 14);
    const total_sfnt_size = readU32BE(data, 16);
    const total_compressed_size = readU32BE(data, 20);
    const major_version = readU16BE(data, 24);
    const minor_version = readU16BE(data, 26);
    const meta_offset = readU32BE(data, 28);
    const meta_length = readU32BE(data, 32);
    const meta_orig_length = readU32BE(data, 36);
    const priv_offset = readU32BE(data, 40);
    const priv_length = readU32BE(data, 44);

    if (reserved != 0 || num_tables == 0) {
        return null;
    }
    if ((length as usize) < WOFF2_HEADER_SIZE || (length as usize) > data.len) {
        return null;
    }

    const total_len: u64 = length as u64;

    // Validate metadata block bounds (overflow-safe)
    if (meta_length > 0) {
        if ((meta_offset as usize) < WOFF2_HEADER_SIZE) {
            return null;
        }
        if ((meta_offset as u64) + (meta_length as u64) > total_len) {
            return null;
        }
    } else if (meta_offset != 0 || meta_orig_length != 0) {
        return null;
    }

    // Validate private data block bounds (overflow-safe)
    if (priv_length > 0) {
        if ((priv_offset as usize) < WOFF2_HEADER_SIZE) {
            return null;
        }
        if ((priv_offset as u64) + (priv_length as u64) > total_len) {
            return null;
        }
    } else if (priv_offset != 0) {
        return null;
    }

    return Woff2Header{
        .signature = sig,
        .flavor = flavor,
        .length = length,
        .num_tables = num_tables,
        .reserved = reserved,
        .total_sfnt_size = total_sfnt_size,
        .total_compressed_size = total_compressed_size,
        .major_version = major_version,
        .minor_version = minor_version,
        .meta_offset = meta_offset,
        .meta_length = meta_length,
        .meta_orig_length = meta_orig_length,
        .priv_offset = priv_offset,
        .priv_length = priv_length,
    };
}

pub struct Woff2TableIter {
    data: []const u8,
    pos: usize,
    remaining: u16,

    fn init(data: []const u8): Woff2TableIter {
        if (hdr := parseWoff2Header(data)) {
            return Woff2TableIter{
                .data = data[0..(hdr.length as usize)],
                .pos = WOFF2_HEADER_SIZE,
                .remaining = hdr.num_tables,
            };
        }
        return Woff2TableIter{
            .data = data[0..0],
            .pos = 0,
            .remaining = 0,
        };
    }

    fn next(*_): Woff2TableEntry? {
        if (_.remaining == 0 || _.pos >= _.data.len) {
            _.remaining = 0;
            return null;
        }

        const flags = _.data[_.pos];
        _.pos += 1;

        const known_idx: usize = (flags & 0x3F) as usize;
        const transform_version: u8 = (flags >> 6) & 0x03;

        let tag: [4]u8 = std.mem.zeroes([4]u8);
        if (known_idx == 63) {
            if (_.pos + 4 > _.data.len) {
                _.remaining = 0;
                return null;
            }
            tag[0] = _.data[_.pos];
            tag[1] = _.data[_.pos + 1];
            tag[2] = _.data[_.pos + 2];
            tag[3] = _.data[_.pos + 3];
            _.pos += 4;
        } else {
            const off = known_idx * 4;
            tag[0] = KNOWN_TABLE_TAGS[off];
            tag[1] = KNOWN_TABLE_TAGS[off + 1];
            tag[2] = KNOWN_TABLE_TAGS[off + 2];
            tag[3] = KNOWN_TABLE_TAGS[off + 3];
        }

        const orig_opt = decodeUIntBase128(_.data[_.pos..]);
        if (orig_opt == null) {
            _.remaining = 0;
            return null;
        }
        const orig_res = orig_opt ?? unreachable;
        _.pos += orig_res.bytes_read;

        const is_glyf_or_loca = tagEql(tag, "glyf") || tagEql(tag, "loca");
        let has_transform = false;
        if (is_glyf_or_loca) {
            if (transform_version == 0) {
                has_transform = true;
            } else if (transform_version == 3) {
                has_transform = false;
            } else {
                // Reserved transform version 1 or 2 for glyf/loca
                _.remaining = 0;
                return null;
            }
        } else {
            has_transform = transform_version != 0;
        }

        let transform_length: u32? = null;
        if (has_transform) {
            const tr_opt = decodeUIntBase128(_.data[_.pos..]);
            if (tr_opt == null) {
                _.remaining = 0;
                return null;
            }
            const tr_res = tr_opt ?? unreachable;
            _.pos += tr_res.bytes_read;
            transform_length = tr_res.value;
        }

        _.remaining -= 1;
        return Woff2TableEntry{
            .tag = tag,
            .flags = flags,
            .transform_version = transform_version,
            .orig_length = orig_res.value,
            .transform_length = transform_length,
        };
    }
}
