// @cap/utf16-codec — Zero-heap UTF-8 <-> UTF-16 / UTF-16LE codec & BOM detector
// MIT License (clean-room CP implementation)

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
    let pos: usize = 0;
    let units: usize = 0;
    while (pos < utf8.len) {
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
    let pos: usize = 0;
    let out_pos: usize = 0;
    while (pos < utf8.len) {
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
    let i: usize = 0;
    let out_pos: usize = 0;
    while (i < utf16.len) {
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
    let pos: usize = 0;
    let out_pos: usize = 0;
    while (pos < utf8.len) {
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
    let i: usize = 0;
    let out_pos: usize = 0;
    while (i < bytes.len) {
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
