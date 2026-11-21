// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc X11 Portable Compiled Format .pcf Font Parser)
// Pure CP zero-heap-allocation X11 PCF binary bitmap font parser:
// TOC table iterator, compressed (5B) & uncompressed (12B LE/BE) glyph metrics,
// O(1) 1D/2D BDF encoding matrix lookup, and aligned PCF_PROPERTIES iterator.

const std = @import("std");

/// 4-byte X11 PCF binary magic `"\x01fcp"`.
pub const PCF_MAGIC: []const u8 = "\x01fcp";

/// Fixed 16-byte TOC entry size (table_type, format, size, offset — all 32-bit LE).
pub const PCF_TOC_ENTRY_SIZE: usize = 16;

// PCF Table Type Bitmasks
pub const PCF_PROPERTIES: u32 = 1;
pub const PCF_ACCELERATORS: u32 = 2;
pub const PCF_METRICS: u32 = 4;
pub const PCF_BITMAPS: u32 = 8;
pub const PCF_INK_METRICS: u32 = 16;
pub const PCF_BDF_ENCODINGS: u32 = 32;
pub const PCF_SWIDTHS: u32 = 64;
pub const PCF_GLYPH_NAMES: u32 = 128;
pub const PCF_BDF_ACCELERATORS: u32 = 256;

// PCF Table Format Flags
pub const PCF_DEFAULT_FORMAT: u32 = 0x00000000;
pub const PCF_COMPRESSED_METRICS: u32 = 0x00000100;
pub const PCF_BYTE_MASK: u32 = 0x00000004;     // Set => Big-Endian byte order inside table
pub const PCF_BIT_MASK: u32 = 0x00000008;      // Set => MSB-first bit order
pub const PCF_GLYPH_PAD_MASK: u32 = 0x00000003; // Row padding = 1 << (fmt & 3) bytes

/// Zero-copy TOC entry pointing to a table payload in the PCF buffer.
pub struct PcfTocEntry {
    table_type: u32 = 0,
    format: u32 = 0,
    size: u32 = 0,
    offset: u32 = 0,
    payload: []const u8 = "",
}

/// Decoded glyph metrics (unified for both 5B compressed and 12B uncompressed formats).
pub struct PcfMetrics {
    left_side_bearing: i16 = 0,
    right_side_bearing: i16 = 0,
    character_width: i16 = 0,
    character_ascent: i16 = 0,
    character_descent: i16 = 0,
    character_attributes: u16 = 0,
}

/// Decoded header of the `PCF_BDF_ENCODINGS` table.
pub struct PcfEncodingsHeader {
    min_char_or_byte2: u16 = 0,
    max_char_or_byte2: u16 = 0,
    min_byte1: u16 = 0,
    max_byte1: u16 = 0,
    default_char: u16 = 0,
}

/// Zero-copy font property yielded by `PcfPropIter` from `PCF_PROPERTIES`.
pub struct PcfProperty {
    name: []const u8 = "",
    is_string: bool = false,
    str_value: []const u8 = "",
    int_value: i32 = 0,
}

// ---------------------------------------------------------------------------
// Internal Endian-Aware Readers
// ---------------------------------------------------------------------------

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU16Be(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU16(data: []const u8, off: usize, is_be: bool): u16 {
    if (is_be) return readU16Be(data, off);
    return readU16Le(data, off);
}

fn readI16(data: []const u8, off: usize, is_be: bool): i16 {
    const bits = readU16(data, off, is_be);
    return @bitCast(bits);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readU32Be(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

fn readU32(data: []const u8, off: usize, is_be: bool): u32 {
    if (is_be) return readU32Be(data, off);
    return readU32Le(data, off);
}

fn readI32(data: []const u8, off: usize, is_be: bool): i32 {
    const bits = readU32(data, off, is_be);
    return @bitCast(bits);
}

fn readCStringAt(pool: []const u8, off: usize): ([]const u8)? {
    if (off >= pool.len) {
        return null;
    }
    let end = off;
    while (end < pool.len && pool[end] != 0) {
        end += 1;
    }
    return pool[off..end];
}

// ---------------------------------------------------------------------------
// 1. Magic Detection & TOC Iteration
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with `"\x01fcp"` and has enough bytes for its TOC table.
pub fn isPcf(data: []const u8): bool {
    if (data.len < 8) {
        return false;
    }
    if (!std.mem.eql(u8, data[0..4], PCF_MAGIC)) {
        return false;
    }
    const table_count = readU32Le(data, 4) as usize;
    const max_tables = (data.len - 8) / PCF_TOC_ENTRY_SIZE;
    return table_count <= max_tables;
}

/// Zero-allocation iterator over all 16-byte TOC entries in an X11 PCF file.
pub struct PcfTocIter {
    data: []const u8 = "",
    pos: usize = 0,
    remaining: u32 = 0,
    done: bool = true,

    fn init(data: []const u8): PcfTocIter {
        if (!isPcf(data)) {
            return PcfTocIter { data: data, pos: 0, remaining: 0, done: true };
        }
        const count = readU32Le(data, 4);
        return PcfTocIter {
            data: data,
            pos: 8,
            remaining: count,
            done: count == 0,
        };
    }

    fn next(*_): PcfTocEntry? {
        if (_.done || _.remaining == 0) {
            _.done = true;
            return null;
        }
        if (_.pos + PCF_TOC_ENTRY_SIZE > _.data.len) {
            _.done = true;
            return null;
        }

        const ttype = readU32Le(_.data, _.pos);
        const fmt = readU32Le(_.data, _.pos + 4);
        const size = readU32Le(_.data, _.pos + 8);
        const off = readU32Le(_.data, _.pos + 12);
        _.pos += PCF_TOC_ENTRY_SIZE;
        _.remaining -= 1;
        if (_.remaining == 0) {
            _.done = true;
        }

        const off_u = off as usize;
        const size_u = size as usize;
        if (off_u > _.data.len || size_u > _.data.len - off_u) {
            _.done = true;
            return null;
        }

        return PcfTocEntry {
            table_type: ttype,
            format: fmt,
            size: size,
            offset: off,
            payload: _.data[off_u .. off_u + size_u],
        };
    }
}

/// Finds the first TOC table matching `table_type` (e.g. `PCF_METRICS`, `PCF_BDF_ENCODINGS`, `PCF_PROPERTIES`).
pub fn findPcfTable(data: []const u8, table_type: u32): PcfTocEntry? {
    let it = PcfTocIter.init(data);
    while (entry := it.next()) {
        if (entry.table_type == table_type) {
            return entry;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// 2. Metrics Table (Compressed 5B & Uncompressed 12B LE/BE)
// ---------------------------------------------------------------------------

/// Returns the number of glyph metric entries in a `PCF_METRICS` or `PCF_INK_METRICS` table payload.
pub fn parsePcfMetricsCount(table_payload: []const u8): u32? {
    if (table_payload.len < 6) {
        return null;
    }
    const fmt = readU32Le(table_payload, 0);
    const is_be = (fmt & PCF_BYTE_MASK) != 0;
    const is_compressed = (fmt & PCF_COMPRESSED_METRICS) != 0;

    if (is_compressed) {
        const count = readU16(table_payload, 4, is_be) as u32;
        const avail = (table_payload.len - 6) / 5;
        if ((count as usize) > avail) {
            return null;
        }
        return count;
    }
    if (table_payload.len < 8) {
        return null;
    }
    const count = readU32(table_payload, 4, is_be);
    const avail = (table_payload.len - 8) / 12;
    if ((count as usize) > avail) {
        return null;
    }
    return count;
}

/// Reads the `idx`-th `PcfMetrics` record from a `PCF_METRICS` or `PCF_INK_METRICS` table payload.
/// Automatically decodes compressed 5B (`byte - 0x80`) and uncompressed 12B (LE or BE) layouts.
pub fn readPcfMetric(table_payload: []const u8, idx: u32): PcfMetrics? {
    const count = parsePcfMetricsCount(table_payload) ?? return null;
    if (idx >= count) {
        return null;
    }
    const fmt = readU32Le(table_payload, 0);
    const is_be = (fmt & PCF_BYTE_MASK) != 0;
    const is_compressed = (fmt & PCF_COMPRESSED_METRICS) != 0;

    if (is_compressed) {
        const off: usize = (idx as usize) * 5 + 6;
        return PcfMetrics {
            left_side_bearing: (table_payload[off] as i16) - 128,
            right_side_bearing: (table_payload[off + 1] as i16) - 128,
            character_width: (table_payload[off + 2] as i16) - 128,
            character_ascent: (table_payload[off + 3] as i16) - 128,
            character_descent: (table_payload[off + 4] as i16) - 128,
            character_attributes: 0,
        };
    }

    const off: usize = (idx as usize) * 12 + 8;
    return PcfMetrics {
        left_side_bearing: readI16(table_payload, off, is_be),
        right_side_bearing: readI16(table_payload, off + 2, is_be),
        character_width: readI16(table_payload, off + 4, is_be),
        character_ascent: readI16(table_payload, off + 6, is_be),
        character_descent: readI16(table_payload, off + 8, is_be),
        character_attributes: readU16(table_payload, off + 10, is_be),
    };
}

// ---------------------------------------------------------------------------
// 3. BDF Encodings Table (O(1) 1D/2D Glyph Index Lookup)
// ---------------------------------------------------------------------------

/// Parses the header of a `PCF_BDF_ENCODINGS` table payload and validates matrix bounds.
pub fn parsePcfEncodings(table_payload: []const u8): PcfEncodingsHeader? {
    if (table_payload.len < 14) {
        return null;
    }
    const fmt = readU32Le(table_payload, 0);
    const is_be = (fmt & PCF_BYTE_MASK) != 0;

    const min_c = readU16(table_payload, 4, is_be);
    const max_c = readU16(table_payload, 6, is_be);
    const min_b1 = readU16(table_payload, 8, is_be);
    const max_b1 = readU16(table_payload, 10, is_be);
    const def_c = readU16(table_payload, 12, is_be);

    if (min_c > max_c || min_b1 > max_b1) {
        return null;
    }

    const cols: usize = ((max_c - min_c) as usize) + 1;
    const rows: usize = ((max_b1 - min_b1) as usize) + 1;
    const max_entries = (table_payload.len - 14) / 2;
    if (rows > max_entries / cols) {
        return null;
    }

    return PcfEncodingsHeader {
        min_char_or_byte2: min_c,
        max_char_or_byte2: max_c,
        min_byte1: min_b1,
        max_byte1: max_b1,
        default_char: def_c,
    };
}

/// O(1) lookup of glyph index for `codepoint` in a `PCF_BDF_ENCODINGS` table payload.
/// Returns `null` if out of range or mapped to the missing-glyph sentinel `0xFFFF`.
pub fn lookupPcfGlyphIndex(table_payload: []const u8, codepoint: u32): u16? {
    if (codepoint > 0xFFFF) {
        return null;
    }
    const enc = parsePcfEncodings(table_payload) ?? return null;
    const fmt = readU32Le(table_payload, 0);
    const is_be = (fmt & PCF_BYTE_MASK) != 0;

    const cols: usize = ((enc.max_char_or_byte2 - enc.min_char_or_byte2) as usize) + 1;
    let matrix_idx: usize = 0;

    if (enc.min_byte1 == 0 && enc.max_byte1 == 0) {
        if (codepoint < (enc.min_char_or_byte2 as u32) || codepoint > (enc.max_char_or_byte2 as u32)) {
            return null;
        }
        matrix_idx = (codepoint - (enc.min_char_or_byte2 as u32)) as usize;
    } else {
        const b1: u16 = ((codepoint >> 8) & 0xFF) as u16;
        const b2: u16 = (codepoint & 0xFF) as u16;
        if (b1 < enc.min_byte1 || b1 > enc.max_byte1) {
            return null;
        }
        if (b2 < enc.min_char_or_byte2 || b2 > enc.max_char_or_byte2) {
            return null;
        }
        matrix_idx = ((b1 - enc.min_byte1) as usize) * cols + ((b2 - enc.min_char_or_byte2) as usize);
    }

    const off: usize = matrix_idx * 2 + 14;
    const glyph_idx = readU16(table_payload, off, is_be);
    if (glyph_idx == 0xFFFF) {
        return null;
    }
    return glyph_idx;
}

// ---------------------------------------------------------------------------
// 4. Properties Table Iterator (`PCF_PROPERTIES`)
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over properties in a `PCF_PROPERTIES` table payload.
pub struct PcfPropIter {
    payload: []const u8 = "",
    str_pool: []const u8 = "",
    is_be: bool = false,
    idx: u32 = 0,
    count: u32 = 0,
    done: bool = true,

    fn init(table_payload: []const u8): PcfPropIter {
        if (table_payload.len < 12) {
            return PcfPropIter { done: true };
        }
        const fmt = readU32Le(table_payload, 0);
        const is_be = (fmt & PCF_BYTE_MASK) != 0;
        const nprops = readU32(table_payload, 4, is_be);

        const max_props = (table_payload.len - 8) / 9;
        if ((nprops as usize) > max_props) {
            return PcfPropIter { done: true };
        }
        const props_end: usize = (nprops as usize) * 9 + 8;
        const rem: usize = props_end % 4;
        const pad: usize = if (rem == 0) 0 else (usize(4) - rem);
        const str_size_off: usize = props_end + pad;
        if (str_size_off + 4 > table_payload.len) {
            return PcfPropIter { done: true };
        }
        const str_size = readU32(table_payload, str_size_off, is_be) as usize;
        const str_start = str_size_off + 4;
        if (str_size > table_payload.len - str_start) {
            return PcfPropIter { done: true };
        }

        return PcfPropIter {
            payload: table_payload,
            str_pool: table_payload[str_start .. str_start + str_size],
            is_be: is_be,
            idx: 0,
            count: nprops,
            done: nprops == 0,
        };
    }

    fn next(*_): PcfProperty? {
        if (_.done || _.idx >= _.count) {
            _.done = true;
            return null;
        }
        const off: usize = (_.idx as usize) * 9 + 8;
        const name_off = readU32(_.payload, off, _.is_be) as usize;
        const is_str = _.payload[off + 4] != 0;
        const val_raw = readI32(_.payload, off + 5, _.is_be);
        _.idx += 1;
        if (_.idx >= _.count) {
            _.done = true;
        }

        const name_opt = readCStringAt(_.str_pool, name_off);
        if (name_opt == null) {
            _.done = true;
            return null;
        }
        if (is_str) {
            if (val_raw < 0) {
                _.done = true;
                return null;
            }
            const sval_opt = readCStringAt(_.str_pool, val_raw as usize);
            if (sval_opt == null) {
                _.done = true;
                return null;
            }
            return PcfProperty {
                name: name_opt.?,
                is_string: true,
                str_value: sval_opt.?,
                int_value: 0,
            };
        }
        return PcfProperty {
            name: name_opt.?,
            is_string: false,
            str_value: "",
            int_value: val_raw,
        };
    }
}
