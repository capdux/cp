// Origin: Pure-CP Stdlib (Zero-Alloc Linux Console PSF1 & PSF2 Bitmap Font Parser) | Created: 2026-11-22

/// PSF1 常量
pub const PSF1_MAGIC0: u8 = 0x36;
pub const PSF1_MAGIC1: u8 = 0x04;
pub const PSF1_HEADER_SIZE: usize = 4;
pub const PSF1_MODE512: u8 = 0x01;
pub const PSF1_MODEHASTAB: u8 = 0x02;
pub const PSF1_MODEHASSEQ: u8 = 0x04;
pub const PSF1_SEPARATOR: u16 = 0xFFFF;
pub const PSF1_STARTSEQ: u16 = 0xFFFE;

/// PSF2 常量（小端序魔数 72 B5 4A 86）
pub const PSF2_MAGIC: u32 = 0x864AB572;
pub const PSF2_HEADER_SIZE: usize = 32;
pub const PSF2_HAS_UNICODE_TABLE: u32 = 0x01;
pub const PSF2_SEPARATOR: u8 = 0xFF;
pub const PSF2_STARTSEQ: u8 = 0xFE;

/// PSF 字体版本枚举
pub enum PsfVersion: u8 {
    unknown = 0,
    psf1 = 1,
    psf2 = 2,
}

/// 统一抽象 PSF1 与 PSF2 的字体头信息
pub struct PsfHeader {
    version: PsfVersion,
    header_size: u32,
    glyph_count: u32,
    bytes_per_glyph: u32,
    width: u32,
    height: u32,
    has_unicode_table: bool,
    glyphs_offset: usize,
    unicode_table_offset: usize?,
}

// ---------------------------------------------------------------------------
// 内部小端序读取与 UTF-8 解码辅助函数
// ---------------------------------------------------------------------------

fn readU16LE(data: []const u8, offset: usize): u16 {
    const lo = u16(data[offset]);
    const hi = u16(data[offset + 1]);
    return lo | (hi << 8);
}

fn readU32LE(data: []const u8, offset: usize): u32 {
    const b0 = u32(data[offset]);
    const b1 = u32(data[offset + 1]);
    const b2 = u32(data[offset + 2]);
    const b3 = u32(data[offset + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

struct Utf8Decoded {
    cp: u32,
    len: usize,
}

fn decodeUtf8At(data: []const u8, pos: usize): Utf8Decoded? {
    if (pos >= data.len) {
        return null;
    }
    const b0 = data[pos];
    if (b0 < 0x80) {
        return Utf8Decoded { cp: u32(b0), len: 1 };
    }
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (pos + 2 > data.len) return null;
        const b1 = data[pos + 1];
        if ((b1 & 0xC0) != 0x80) return null;
        const cp = (u32(b0 & 0x1F) << 6) | u32(b1 & 0x3F);
        return Utf8Decoded { cp: cp, len: 2 };
    }
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (pos + 3 > data.len) return null;
        const b1 = data[pos + 1];
        const b2 = data[pos + 2];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return null;
        const cp = (u32(b0 & 0x0F) << 12) | (u32(b1 & 0x3F) << 6) | u32(b2 & 0x3F);
        if (cp < 0x800) return null;
        return Utf8Decoded { cp: cp, len: 3 };
    }
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (pos + 4 > data.len) return null;
        const b1 = data[pos + 1];
        const b2 = data[pos + 2];
        const b3 = data[pos + 3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return null;
        const cp = (u32(b0 & 0x07) << 18) | (u32(b1 & 0x3F) << 12) | (u32(b2 & 0x3F) << 6) | u32(b3 & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) return null;
        return Utf8Decoded { cp: cp, len: 4 };
    }
    return null;
}

// ---------------------------------------------------------------------------
// 1. 版本探测与头解析 (PSF1 / PSF2)
// ---------------------------------------------------------------------------

/// 探测缓冲区对应的 PSF 版本（PSF1 / PSF2 / unknown）
pub fn detectPsfVersion(data: []const u8): PsfVersion {
    if (data.len >= PSF2_HEADER_SIZE && readU32LE(data, 0) == PSF2_MAGIC) {
        return PsfVersion.psf2;
    }
    if (data.len >= PSF1_HEADER_SIZE && data[0] == PSF1_MAGIC0 && data[1] == PSF1_MAGIC1) {
        return PsfVersion.psf1;
    }
    return PsfVersion.unknown;
}

/// 快速检测缓冲区是否带有合法 PSF1 或 PSF2 魔数
pub fn isPsf(data: []const u8): bool {
    return detectPsfVersion(data) != PsfVersion.unknown;
}

/// 零分配解析 PSF1 或 PSF2 字体头，严格校验位图区尺寸、行步幅与截断边界
pub fn parsePsfHeader(data: []const u8): PsfHeader? {
    const ver = detectPsfVersion(data);
    if (ver == PsfVersion.psf1) {
        if (data.len < PSF1_HEADER_SIZE) {
            return null;
        }
        const mode = data[2];
        const charsize = u32(data[3]);
        if (charsize == 0) {
            return null;
        }
        const glyph_count: u32 = if ((mode & PSF1_MODE512) != 0) 512 else 256;
        const has_tab = (mode & (PSF1_MODEHASTAB | PSF1_MODEHASSEQ)) != 0;
        const total_glyph_bytes = u64(glyph_count) * u64(charsize);
        const end_u64 = u64(PSF1_HEADER_SIZE) + total_glyph_bytes;
        if (end_u64 > u64(data.len)) {
            return null;
        }
        const uni_off: usize? = if (has_tab) (usize(end_u64)) else null;
        return PsfHeader {
            version: PsfVersion.psf1,
            header_size: PSF1_HEADER_SIZE as u32,
            glyph_count: glyph_count,
            bytes_per_glyph: charsize,
            width: 8,
            height: charsize,
            has_unicode_table: has_tab,
            glyphs_offset: PSF1_HEADER_SIZE,
            unicode_table_offset: uni_off,
        };
    }

    if (ver == PsfVersion.psf2) {
        if (data.len < PSF2_HEADER_SIZE) {
            return null;
        }
        const format_ver = readU32LE(data, 4);
        if (format_ver != 0) {
            return null;
        }
        const header_size = readU32LE(data, 8);
        const flags = readU32LE(data, 12);
        const glyph_count = readU32LE(data, 16);
        const bytes_per_glyph = readU32LE(data, 20);
        const height = readU32LE(data, 24);
        const width = readU32LE(data, 28);

        if (usize(header_size) < PSF2_HEADER_SIZE || usize(header_size) > data.len) {
            return null;
        }
        if (glyph_count == 0 || width == 0 || height == 0) {
            return null;
        }
        // 校验每字形字节数足以容纳 ((width + 7) / 8) * height
        const row_bytes = (u64(width) + 7) / 8;
        const min_glyph_bytes = row_bytes * u64(height);
        if (u64(bytes_per_glyph) < min_glyph_bytes) {
            return null;
        }
        const total_glyph_bytes = u64(glyph_count) * u64(bytes_per_glyph);
        if (total_glyph_bytes > u64(data.len) - u64(header_size)) {
            return null;
        }
        const has_tab = (flags & PSF2_HAS_UNICODE_TABLE) != 0;
        const uni_off: usize? = if (has_tab) (usize(u64(header_size) + total_glyph_bytes)) else null;
        return PsfHeader {
            version: PsfVersion.psf2,
            header_size: header_size,
            glyph_count: glyph_count,
            bytes_per_glyph: bytes_per_glyph,
            width: width,
            height: height,
            has_unicode_table: has_tab,
            glyphs_offset: usize(header_size),
            unicode_table_offset: uni_off,
        };
    }

    return null;
}

// ---------------------------------------------------------------------------
// 2. 字形位图与 MSB-first 像素读取
// ---------------------------------------------------------------------------

/// 零拷贝获取指定字形索引的原始位图切片
pub fn psfGlyphBitmap(data: []const u8, header: PsfHeader, glyph_idx: u32): ([]const u8)? {
    if (glyph_idx >= header.glyph_count) {
        return null;
    }
    const offset_u64 = u64(header.glyphs_offset) + u64(glyph_idx) * u64(header.bytes_per_glyph);
    const end_u64 = offset_u64 + u64(header.bytes_per_glyph);
    if (end_u64 > u64(data.len)) {
        return null;
    }
    return data[usize(offset_u64)..usize(end_u64)];
}

/// 按每行 `(width + 7) / 8` 字节、MSB-first 位序精准读取字形 `(row, col)` 像素
pub fn psfGlyphPixel(data: []const u8, header: PsfHeader, glyph_idx: u32, row: u32, col: u32): bool? {
    if (row >= header.height || col >= header.width) {
        return null;
    }
    const bm = psfGlyphBitmap(data, header, glyph_idx) ?? return null;
    const row_bytes = (header.width + 7) / 8;
    const byte_idx_u64 = u64(row) * u64(row_bytes) + u64(col / 8);
    if (byte_idx_u64 >= u64(bm.len)) {
        return null;
    }
    const masks: [8]u8 = [0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01];
    const bit_idx = usize(col % 8);
    return (bm[usize(byte_idx_u64)] & masks[bit_idx]) != 0;
}

// ---------------------------------------------------------------------------
// 3. Unicode 映射表零分配查找 (PSF1 UCS-2 / PSF2 UTF-8)
// ---------------------------------------------------------------------------

/// 在 PSF1（UCS-2 LE）或 PSF2（UTF-8）Unicode 映射表中查找码点对应的字形索引；
/// 无 Unicode 表或未命中返回 `null`；自动跳过 `STARTSEQ` 组合序列
pub fn findPsfGlyphByCodepoint(data: []const u8, header: PsfHeader, codepoint: u32): u32? {
    if (!header.has_unicode_table) {
        return null;
    }
    const table_off = header.unicode_table_offset ?? return null;
    if (table_off >= data.len) {
        return null;
    }

    if (header.version == PsfVersion.psf1) {
        if (codepoint > 0xFFFF) {
            return null;
        }
        let pos = table_off;
        let glyph_idx: u32 = 0;
        let in_seq = false;
        while (pos + 2 <= data.len && glyph_idx < header.glyph_count) {
            const val = readU16LE(data, pos);
            pos += 2;
            if (val == PSF1_SEPARATOR) {
                glyph_idx += 1;
                in_seq = false;
            } else if (val == PSF1_STARTSEQ) {
                in_seq = true;
            } else if (!in_seq && u32(val) == codepoint) {
                return glyph_idx;
            }
        }
        return null;
    }

    if (header.version == PsfVersion.psf2) {
        let pos = table_off;
        let glyph_idx: u32 = 0;
        let in_seq = false;
        while (pos < data.len && glyph_idx < header.glyph_count) {
            const b0 = data[pos];
            if (b0 == PSF2_SEPARATOR) {
                pos += 1;
                glyph_idx += 1;
                in_seq = false;
                continue;
            }
            if (b0 == PSF2_STARTSEQ) {
                pos += 1;
                in_seq = true;
                continue;
            }
            const decoded = decodeUtf8At(data, pos) ?? return null;
            pos += decoded.len;
            if (!in_seq && decoded.cp == codepoint) {
                return glyph_idx;
            }
        }
        return null;
    }

    return null;
}
