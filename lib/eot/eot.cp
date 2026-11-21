// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Microsoft Embedded OpenType .eot Parser)
// Pure CP zero-heap-allocation W3C / Microsoft EOT v1.0/v2.1/v2.2 container parser:
// 0x504C magic & 82B fixed header validation, UTF-16LE name extraction & UTF-8 decoder,
// v2.1 root string whitelist, v2.2 signature/EUDC fields, and embedded font payload slicing.

const std = @import("std");

/// 16-bit LE magic number at offset 34 (`0x504C` => bytes `'L', 'P'`).
pub const EOT_MAGIC: u16 = 0x504C;

/// Fixed 82-byte EOT header length (up through `Padding1` at offsets 80..82).
pub const EOT_FIXED_HEADER_SIZE: usize = 82;

// Supported EOT Specification Versions
pub const EOT_VERSION_1: u32 = 0x00010000;
pub const EOT_VERSION_2_1: u32 = 0x00020001;
pub const EOT_VERSION_2_2: u32 = 0x00020002;

// EOT Processing Flags
pub const EOT_FLAG_COMPRESSED: u32 = 0x00000001;    // TTEMBED_TTCOMPRESSED (MTX compression)
pub const EOT_FLAG_SUBSET: u32 = 0x00000002;        // TTEMBED_SUBSET
pub const EOT_FLAG_XOR_ENCRYPTED: u32 = 0x10000000; // TTEMBED_XORENCRYPTDATA

/// Zero-copy parsed Embedded OpenType (`.eot`) header and embedded font data view.
pub struct EotHeader {
    eot_size: u32 = 0,
    font_data_size: u32 = 0,
    version: u32 = 0,
    flags: u32 = 0,
    charset: u8 = 0,
    italic: u8 = 0,
    weight: u32 = 0,
    fs_type: u16 = 0,
    magic_number: u16 = 0,
    unicode_range: [4]u32 = [0; 4],
    codepage_range: [2]u32 = [0; 2],
    checksum_adjustment: u32 = 0,

    // Variable-length UTF-16LE metadata slices
    family_name_utf16: []const u8 = "",
    style_name_utf16: []const u8 = "",
    version_name_utf16: []const u8 = "",
    full_name_utf16: []const u8 = "",

    // v2.1+ optional root string whitelist
    root_string_utf16: []const u8 = "",

    // v2.2 optional security & EUDC fields
    root_string_checksum: u32 = 0,
    eudc_code_page: u32 = 0,
    signature: []const u8 = "",
    eudc_flags: u32 = 0,
    eudc_font_size: u32 = 0,
    eudc_font_data: []const u8 = "",

    // Embedded font payload
    font_data_offset: usize = 0,
    font_data: []const u8 = "",

    fn isCompressed(): bool {
        return (_.flags & EOT_FLAG_COMPRESSED) != 0;
    }

    fn isXorEncrypted(): bool {
        return (_.flags & EOT_FLAG_XOR_ENCRYPTED) != 0;
    }

    fn isSubset(): bool {
        return (_.flags & EOT_FLAG_SUBSET) != 0;
    }
}

struct LenSlice {
    slice: []const u8,
    next: usize,
}

// ---------------------------------------------------------------------------
// Internal Little-Endian & UTF-8 Encoding Helpers
// ---------------------------------------------------------------------------

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn isValidEotVersion(ver: u32): bool {
    return ver == EOT_VERSION_1 || ver == EOT_VERSION_2_1 || ver == EOT_VERSION_2_2;
}

/// Reads a 2-byte LE byte length followed by that many bytes within `limit`.
fn readU16PrefixedSlice(data: []const u8, pos: usize, limit: usize): LenSlice? {
    if (pos + 2 > limit) {
        return null;
    }
    const byte_len = readU16Le(data, pos) as usize;
    const start = pos + 2;
    if (byte_len > limit - start) {
        return null;
    }
    return LenSlice {
        slice: data[start .. start + byte_len],
        next: start + byte_len,
    };
}

/// Verifies a 2-byte `0x0000` padding word at `pos` within `limit`, returning `pos + 2`.
fn expectZeroPadding(data: []const u8, pos: usize, limit: usize): usize? {
    if (pos + 2 > limit) {
        return null;
    }
    if (readU16Le(data, pos) != 0) {
        return null;
    }
    return pos + 2;
}

fn emitUtf8Codepoint(out: []u8, out_len: *usize, cp: u32): bool {
    if (cp <= 0x7F) {
        if (out_len.* >= out.len) return false;
        out[out_len.*] = cp as u8;
        out_len.* += 1;
        return true;
    }
    if (cp <= 0x7FF) {
        if (out_len.* + 2 > out.len) return false;
        out[out_len.*] = (0xC0 | (cp >> 6)) as u8;
        out[out_len.* + 1] = (0x80 | (cp & 0x3F)) as u8;
        out_len.* += 2;
        return true;
    }
    if (cp <= 0xFFFF) {
        if (out_len.* + 3 > out.len) return false;
        out[out_len.*] = (0xE0 | (cp >> 12)) as u8;
        out[out_len.* + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[out_len.* + 2] = (0x80 | (cp & 0x3F)) as u8;
        out_len.* += 3;
        return true;
    }
    if (cp <= 0x10FFFF) {
        if (out_len.* + 4 > out.len) return false;
        out[out_len.*] = (0xF0 | (cp >> 18)) as u8;
        out[out_len.* + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
        out[out_len.* + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[out_len.* + 3] = (0x80 | (cp & 0x3F)) as u8;
        out_len.* += 4;
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Public API: UTF-16LE Name Decoder
// ---------------------------------------------------------------------------

/// Decodes a UTF-16LE byte slice (such as `family_name_utf16` or `root_string_utf16`)
/// into UTF-8 inside `out`. Supports BMP and UTF-16 surrogate pairs.
/// Returns total UTF-8 bytes written, or `null` on odd length, invalid surrogate, or buffer overflow.
pub fn decodeUtf16LeName(raw: []const u8, out: []u8): usize? {
    if ((raw.len % 2) != 0) {
        return null;
    }
    let i: usize = 0;
    let out_len: usize = 0;
    while (i < raw.len) {
        const unit1 = readU16Le(raw, i);
        i += 2;
        if (unit1 == 0) {
            continue;
        }
        if (unit1 >= 0xD800 && unit1 <= 0xDBFF) {
            if (i + 2 > raw.len) {
                return null;
            }
            const unit2 = readU16Le(raw, i);
            if (unit2 < 0xDC00 || unit2 > 0xDFFF) {
                return null;
            }
            i += 2;
            const hi: u32 = (unit1 - 0xD800) as u32;
            const lo: u32 = (unit2 - 0xDC00) as u32;
            const cp: u32 = ((hi << 10) | lo) + 0x10000;
            if (!emitUtf8Codepoint(out, *out_len, cp)) {
                return null;
            }
        } else if (unit1 >= 0xDC00 && unit1 <= 0xDFFF) {
            return null;
        } else {
            if (!emitUtf8Codepoint(out, *out_len, unit1 as u32)) {
                return null;
            }
        }
    }
    return out_len;
}

// ---------------------------------------------------------------------------
// Public API: Detection & Full Header Parser
// ---------------------------------------------------------------------------

/// Returns `true` if `data` has >= 82 bytes, valid `0x504C` magic at offset 34,
/// a recognized EOT version (`0x00010000`/`0x00020001`/`0x00020002`), and `eot_size <= data.len`.
pub fn isEot(data: []const u8): bool {
    if (data.len < EOT_FIXED_HEADER_SIZE) {
        return false;
    }
    const magic = readU16Le(data, 34);
    if (magic != EOT_MAGIC) {
        return false;
    }
    const ver = readU32Le(data, 8);
    if (!isValidEotVersion(ver)) {
        return false;
    }
    const eot_size = readU32Le(data, 0) as usize;
    return eot_size >= EOT_FIXED_HEADER_SIZE && eot_size <= data.len;
}

/// Strictly parses an Embedded OpenType (`.eot`) v1.0 / v2.1 / v2.2 header and
/// returns zero-copy slices to all UTF-16LE metadata fields and the embedded font payload.
pub fn parseEotHeader(data: []const u8): EotHeader? {
    if (!isEot(data)) {
        return null;
    }

    const eot_size = readU32Le(data, 0);
    const font_data_size = readU32Le(data, 4);
    const version = readU32Le(data, 8);
    const flags = readU32Le(data, 12);
    const limit = eot_size as usize;

    // Strictly validate Reserved1..4 (offsets 64..80) and Padding1 (80..82) are all zero
    if (readU32Le(data, 64) != 0 ||
        readU32Le(data, 68) != 0 ||
        readU32Le(data, 72) != 0 ||
        readU32Le(data, 76) != 0 ||
        readU16Le(data, 80) != 0)
    {
        return null;
    }

    let hdr = EotHeader {
        eot_size: eot_size,
        font_data_size: font_data_size,
        version: version,
        flags: flags,
        charset: data[26],
        italic: data[27],
        weight: readU32Le(data, 28),
        fs_type: readU16Le(data, 32),
        magic_number: readU16Le(data, 34),
        unicode_range: [
            readU32Le(data, 36),
            readU32Le(data, 40),
            readU32Le(data, 44),
            readU32Le(data, 48),
        ],
        codepage_range: [
            readU32Le(data, 52),
            readU32Le(data, 56),
        ],
        checksum_adjustment: readU32Le(data, 60),
    };

    // 4 variable-length UTF-16LE name fields separated by 2-byte zero paddings
    let pos: usize = EOT_FIXED_HEADER_SIZE;

    const fam = readU16PrefixedSlice(data, pos, limit) ?? return null;
    hdr.family_name_utf16 = fam.slice;
    pos = expectZeroPadding(data, fam.next, limit) ?? return null;

    const sty = readU16PrefixedSlice(data, pos, limit) ?? return null;
    hdr.style_name_utf16 = sty.slice;
    pos = expectZeroPadding(data, sty.next, limit) ?? return null;

    const ver_name = readU16PrefixedSlice(data, pos, limit) ?? return null;
    hdr.version_name_utf16 = ver_name.slice;
    pos = expectZeroPadding(data, ver_name.next, limit) ?? return null;

    const full = readU16PrefixedSlice(data, pos, limit) ?? return null;
    hdr.full_name_utf16 = full.slice;
    pos = full.next;

    const fd_size = font_data_size as usize;

    if (version == EOT_VERSION_1) {
        // W3C EOT v1.0 places FontData directly after FullName; also allow optional 2B 0x0000 trailing padding
        if (pos + fd_size != limit) {
            pos = expectZeroPadding(data, pos, limit) ?? return null;
        }
    } else if (version == EOT_VERSION_2_1 || version == EOT_VERSION_2_2) {
        // Padding5 before RootStringSize
        pos = expectZeroPadding(data, pos, limit) ?? return null;
        const root_str = readU16PrefixedSlice(data, pos, limit) ?? return null;
        hdr.root_string_utf16 = root_str.slice;
        pos = root_str.next;

        if (version == EOT_VERSION_2_1) {
            if (pos + fd_size != limit) {
                pos = expectZeroPadding(data, pos, limit) ?? return null;
            }
        } else {
            // v2.2: RootStringCheckSum (4B), EUDCCodePage (4B), Padding6 (2B == 0),
            //       SignatureSize (2B) + Signature, EUDCFlags (4B), EUDCFontSize (4B) + EUDCFontData
            if (pos + 8 > limit) {
                return null;
            }
            hdr.root_string_checksum = readU32Le(data, pos);
            hdr.eudc_code_page = readU32Le(data, pos + 4);
            pos += 8;

            pos = expectZeroPadding(data, pos, limit) ?? return null;

            const sig = readU16PrefixedSlice(data, pos, limit) ?? return null;
            hdr.signature = sig.slice;
            pos = sig.next;

            if (pos + 8 > limit) {
                return null;
            }
            hdr.eudc_flags = readU32Le(data, pos);
            hdr.eudc_font_size = readU32Le(data, pos + 4);
            pos += 8;

            const eudc_sz = hdr.eudc_font_size as usize;
            if (eudc_sz > limit - pos) {
                return null;
            }
            hdr.eudc_font_data = data[pos .. pos + eudc_sz];
            pos += eudc_sz;
        }
    }

    // Strictly validate font_data_offset + font_data_size <= eot_size <= data.len
    if (fd_size > limit - pos) {
        return null;
    }
    hdr.font_data_offset = pos;
    hdr.font_data = data[pos .. pos + fd_size];
    return hdr;
}
