// @cap/utf16-codec — Zero-heap UTF-8 <-> UTF-16 / UTF-16LE codec & BOM detector
// MIT License (clean-room CP implementation)
// Accelerated with 16-lane @Vector(16, u8) / @Vector(8, u16) SIMD ASCII fast-paths and @memcpy.

type U16Vec16 = @Vector(16, u8);
type U16Vec8 = @Vector(8, u16);

pub const BOM_NONE: u8 = 0;
pub const BOM_UTF16_LE: u8 = 1;
pub const BOM_UTF16_BE: u8 = 2;
pub const BOM_UTF8: u8 = 3;

pub fn detectBom(data: []const u8) -> u8 {
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        return BOM_UTF8;
    }
    if (data.len >= 2) {
        if (data[0] == 0xFF && data[1] == 0xFE) {
            return BOM_UTF16_LE;
        }
        if (data[0] == 0xFE && data[1] == 0xFF) {
            return BOM_UTF16_BE;
        }
    }
    return BOM_NONE;
}

fn decodeUtf8Next(s: string, pos: usize, out_cp: *u32) -> i64 {
    if (pos >= s.len) {
        return -1;
    }
    const b0: u8 = s[pos];
    if (b0 < 0x80) {
        out_cp.* = b0 as u32;
        return (pos + 1) as i64;
    }
    if ((b0 & 0xE0) == 0xC0) {
        if (pos + 1 >= s.len) {
            return -1;
        }
        const b1: u8 = s[pos + 1];
        if ((b1 & 0xC0) != 0x80) {
            return -1;
        }
        const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
        if (cp < 0x80) {
            return -1;
        }
        out_cp.* = cp;
        return (pos + 2) as i64;
    }
    if ((b0 & 0xF0) == 0xE0) {
        if (pos + 2 >= s.len) {
            return -1;
        }
        const b1: u8 = s[pos + 1];
        const b2: u8 = s[pos + 2];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
            return -1;
        }
        const cp: u32 = (((b0 & 0x0F) as u32) << 12)
            | (((b1 & 0x3F) as u32) << 6)
            | ((b2 & 0x3F) as u32);
        if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return -1;
        }
        out_cp.* = cp;
        return (pos + 3) as i64;
    }
    if ((b0 & 0xF8) == 0xF0) {
        if (pos + 3 >= s.len) {
            return -1;
        }
        const b1: u8 = s[pos + 1];
        const b2: u8 = s[pos + 2];
        const b3: u8 = s[pos + 3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
            return -1;
        }
        const cp: u32 = (((b0 & 0x07) as u32) << 18)
            | (((b1 & 0x3F) as u32) << 12)
            | (((b2 & 0x3F) as u32) << 6)
            | ((b3 & 0x3F) as u32);
        if (cp < 0x10000 || cp > 0x10FFFF) {
            return -1;
        }
        out_cp.* = cp;
        return (pos + 4) as i64;
    }
    return -1;
}

fn encodeUtf8Scalar(cp: u32, out: []u8, pos: usize) -> i64 {
    if (cp <= 0x7F) {
        if (pos >= out.len) {
            return -1;
        }
        out[pos] = cp as u8;
        return (pos + 1) as i64;
    }
    if (cp <= 0x7FF) {
        if (pos + 2 > out.len) {
            return -1;
        }
        out[pos] = (0xC0 | (cp >> 6)) as u8;
        out[pos + 1] = (0x80 | (cp & 0x3F)) as u8;
        return (pos + 2) as i64;
    }
    if (cp <= 0xFFFF) {
        if (pos + 3 > out.len) {
            return -1;
        }
        out[pos] = (0xE0 | (cp >> 12)) as u8;
        out[pos + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[pos + 2] = (0x80 | (cp & 0x3F)) as u8;
        return (pos + 3) as i64;
    }
    if (cp <= 0x10FFFF) {
        if (pos + 4 > out.len) {
            return -1;
        }
        out[pos] = (0xF0 | (cp >> 18)) as u8;
        out[pos + 1] = (0x80 | ((cp >> 12) & 0x3F)) as u8;
        out[pos + 2] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
        out[pos + 3] = (0x80 | (cp & 0x3F)) as u8;
        return (pos + 4) as i64;
    }
    return -1;
}

pub fn countUtf16Units(utf8: string) -> i64 {
    const v_hi: U16Vec16 = @splat(0x80 as u8);
    const v_zero: U16Vec16 = @splat(0 as u8);
    let pos: usize = 0;
    let units: usize = 0;
    while (pos < utf8.len) {
        if (pos + 16 <= utf8.len) {
            const v: U16Vec16 = utf8[pos .. pos + 16][0..16].*;
            if (!@reduce(.Or, (v & v_hi) != v_zero)) {
                units += 16;
                pos += 16;
                continue;
            }
        }
        if (utf8[pos] < 0x80) {
            units += 1;
            pos += 1;
            continue;
        }
        let cp: u32 = 0;
        const nxt: i64 = decodeUtf8Next(utf8, pos, &cp);
        if (nxt < 0) {
            return -1;
        }
        if (cp <= 0xFFFF) {
            units += 1;
        } else {
            units += 2;
        }
        pos = nxt as usize;
    }
    return units as i64;
}

pub fn utf8ToUtf16(utf8: string, out: []u16) -> i64 {
    const v_hi: U16Vec16 = @splat(0x80 as u8);
    const v_zero: U16Vec16 = @splat(0 as u8);
    let pos: usize = 0;
    let out_pos: usize = 0;
    while (pos < utf8.len) {
        if (pos + 16 <= utf8.len && out_pos + 16 <= out.len) {
            const v: U16Vec16 = utf8[pos .. pos + 16][0..16].*;
            if (!@reduce(.Or, (v & v_hi) != v_zero)) {
                const a: [16]u8 = v;
                out[out_pos + 0] = a[0] as u16;
                out[out_pos + 1] = a[1] as u16;
                out[out_pos + 2] = a[2] as u16;
                out[out_pos + 3] = a[3] as u16;
                out[out_pos + 4] = a[4] as u16;
                out[out_pos + 5] = a[5] as u16;
                out[out_pos + 6] = a[6] as u16;
                out[out_pos + 7] = a[7] as u16;
                out[out_pos + 8] = a[8] as u16;
                out[out_pos + 9] = a[9] as u16;
                out[out_pos + 10] = a[10] as u16;
                out[out_pos + 11] = a[11] as u16;
                out[out_pos + 12] = a[12] as u16;
                out[out_pos + 13] = a[13] as u16;
                out[out_pos + 14] = a[14] as u16;
                out[out_pos + 15] = a[15] as u16;
                pos += 16;
                out_pos += 16;
                continue;
            }
        }
        let cp: u32 = 0;
        const nxt: i64 = decodeUtf8Next(utf8, pos, &cp);
        if (nxt < 0) {
            return -1;
        }
        if (cp <= 0xFFFF) {
            if (out_pos >= out.len) {
                return -1;
            }
            out[out_pos] = cp as u16;
            out_pos += 1;
        } else {
            if (out_pos + 2 > out.len) {
                return -1;
            }
            const adj: u32 = cp - 0x10000;
            out[out_pos] = (0xD800 | (adj >> 10)) as u16;
            out[out_pos + 1] = (0xDC00 | (adj & 0x3FF)) as u16;
            out_pos += 2;
        }
        pos = nxt as usize;
    }
    return out_pos as i64;
}

pub fn utf16ToUtf8(utf16: []const u16, out: []u8) -> i64 {
    const v_mask: U16Vec8 = @splat(0xFF80 as u16);
    const v_zero: U16Vec8 = @splat(0 as u16);
    let i: usize = 0;
    let out_pos: usize = 0;
    while (i < utf16.len) {
        if (i + 8 <= utf16.len && out_pos + 8 <= out.len) {
            const v: U16Vec8 = utf16[i .. i + 8][0..8].*;
            if (!@reduce(.Or, (v & v_mask) != v_zero)) {
                const a: [8]u16 = v;
                out[out_pos + 0] = a[0] as u8;
                out[out_pos + 1] = a[1] as u8;
                out[out_pos + 2] = a[2] as u8;
                out[out_pos + 3] = a[3] as u8;
                out[out_pos + 4] = a[4] as u8;
                out[out_pos + 5] = a[5] as u8;
                out[out_pos + 6] = a[6] as u8;
                out[out_pos + 7] = a[7] as u8;
                i += 8;
                out_pos += 8;
                continue;
            }
        }
        const unit: u16 = utf16[i];
        let cp: u32 = 0;
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (i + 1 >= utf16.len) {
                return -1;
            }
            const low: u16 = utf16[i + 1];
            if (low < 0xDC00 || low > 0xDFFF) {
                return -1;
            }
            const hi_part: u32 = ((unit - 0xD800) as u32) << 10;
            const lo_part: u32 = (low - 0xDC00) as u32;
            cp = hi_part + lo_part + 0x10000;
            i += 2;
        } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
            return -1;
        } else {
            cp = unit as u32;
            i += 1;
        }

        const nxt_out: i64 = encodeUtf8Scalar(cp, out, out_pos);
        if (nxt_out < 0) {
            return -1;
        }
        out_pos = nxt_out as usize;
    }
    return out_pos as i64;
}

pub fn utf8ToUtf16LeBytes(utf8: string, out: []u8) -> i64 {
    const v_hi: U16Vec16 = @splat(0x80 as u8);
    const v_zero: U16Vec16 = @splat(0 as u8);
    let pos: usize = 0;
    let out_pos: usize = 0;
    while (pos < utf8.len) {
        if (pos + 16 <= utf8.len && out_pos + 32 <= out.len) {
            const v: U16Vec16 = utf8[pos .. pos + 16][0..16].*;
            if (!@reduce(.Or, (v & v_hi) != v_zero)) {
                const a: [16]u8 = v;
                let tmp: [32]u8 = undefined;
                tmp[0] = a[0]; tmp[1] = 0;
                tmp[2] = a[1]; tmp[3] = 0;
                tmp[4] = a[2]; tmp[5] = 0;
                tmp[6] = a[3]; tmp[7] = 0;
                tmp[8] = a[4]; tmp[9] = 0;
                tmp[10] = a[5]; tmp[11] = 0;
                tmp[12] = a[6]; tmp[13] = 0;
                tmp[14] = a[7]; tmp[15] = 0;
                tmp[16] = a[8]; tmp[17] = 0;
                tmp[18] = a[9]; tmp[19] = 0;
                tmp[20] = a[10]; tmp[21] = 0;
                tmp[22] = a[11]; tmp[23] = 0;
                tmp[24] = a[12]; tmp[25] = 0;
                tmp[26] = a[13]; tmp[27] = 0;
                tmp[28] = a[14]; tmp[29] = 0;
                tmp[30] = a[15]; tmp[31] = 0;
                @memcpy(out[out_pos .. out_pos + 32], tmp[0..32]);
                pos += 16;
                out_pos += 32;
                continue;
            }
        }
        let cp: u32 = 0;
        const nxt: i64 = decodeUtf8Next(utf8, pos, &cp);
        if (nxt < 0) {
            return -1;
        }
        if (cp <= 0xFFFF) {
            if (out_pos + 2 > out.len) {
                return -1;
            }
            out[out_pos] = (cp & 0xFF) as u8;
            out[out_pos + 1] = ((cp >> 8) & 0xFF) as u8;
            out_pos += 2;
        } else {
            if (out_pos + 4 > out.len) {
                return -1;
            }
            const adj: u32 = cp - 0x10000;
            const hi: u32 = (adj >> 10) | 0xD800;
            const lo: u32 = (adj & 0x3FF) | 0xDC00;
            out[out_pos] = (hi & 0xFF) as u8;
            out[out_pos + 1] = ((hi >> 8) & 0xFF) as u8;
            out[out_pos + 2] = (lo & 0xFF) as u8;
            out[out_pos + 3] = ((lo >> 8) & 0xFF) as u8;
            out_pos += 4;
        }
        pos = nxt as usize;
    }
    return out_pos as i64;
}

pub fn utf16LeBytesToUtf8(bytes: []const u8, out: []u8) -> i64 {
    if ((bytes.len % 2) != 0) {
        return -1;
    }
    // Mask checking: even bytes < 0x80 (bit 0x80 clear) and odd bytes == 0x00
    const v_ascii_mask: U16Vec16 = [
        0x80, 0xFF, 0x80, 0xFF, 0x80, 0xFF, 0x80, 0xFF,
        0x80, 0xFF, 0x80, 0xFF, 0x80, 0xFF, 0x80, 0xFF,
    ];
    const v_zero: U16Vec16 = @splat(0 as u8);
    let i: usize = 0;
    let out_pos: usize = 0;
    while (i < bytes.len) {
        if (i + 16 <= bytes.len && out_pos + 8 <= out.len) {
            const v: U16Vec16 = bytes[i .. i + 16][0..16].*;
            if (!@reduce(.Or, (v & v_ascii_mask) != v_zero)) {
                const a: [16]u8 = v;
                out[out_pos + 0] = a[0];
                out[out_pos + 1] = a[2];
                out[out_pos + 2] = a[4];
                out[out_pos + 3] = a[6];
                out[out_pos + 4] = a[8];
                out[out_pos + 5] = a[10];
                out[out_pos + 6] = a[12];
                out[out_pos + 7] = a[14];
                i += 16;
                out_pos += 8;
                continue;
            }
        }
        const unit: u16 = (bytes[i] as u16) | ((bytes[i + 1] as u16) << 8);
        let cp: u32 = 0;
        if (unit >= 0xD800 && unit <= 0xDBFF) {
            if (i + 3 >= bytes.len) {
                return -1;
            }
            const low: u16 = (bytes[i + 2] as u16) | ((bytes[i + 3] as u16) << 8);
            if (low < 0xDC00 || low > 0xDFFF) {
                return -1;
            }
            const hi_part: u32 = ((unit - 0xD800) as u32) << 10;
            const lo_part: u32 = (low - 0xDC00) as u32;
            cp = hi_part + lo_part + 0x10000;
            i += 4;
        } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
            return -1;
        } else {
            cp = unit as u32;
            i += 2;
        }

        const nxt_out: i64 = encodeUtf8Scalar(cp, out, out_pos);
        if (nxt_out < 0) {
            return -1;
        }
        out_pos = nxt_out as usize;
    }
    return out_pos as i64;
}
