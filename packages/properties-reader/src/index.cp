// @cap/properties — Zero-heap Java / Spring Boot / i18n .properties parser & formatter
// MIT License (clean-room CP implementation)

pub const MAX_PROPS: usize = 128;
pub const VAL_BUF_CAP: usize = 2048;

pub struct PropEntry {
    pub key_start: usize,
    pub key_len: usize,
    pub val_start: usize,
    pub val_len: usize,
}

fn isLineWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8 || c == 0x0C;
}

fn fromHex(c: u8) -> i32 {
    if (c >= '0' as u8 && c <= '9' as u8) {
        return (c - ('0' as u8)) as i32;
    }
    if (c >= 'a' as u8 && c <= 'f' as u8) {
        return (c - ('a' as u8) + 10) as i32;
    }
    if (c >= 'A' as u8 && c <= 'F' as u8) {
        return (c - ('A' as u8) + 10) as i32;
    }
    return -1;
}

fn bytesEq(a: []const u8, b: []const u8) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

struct EscDecodeResult {
    next_pos: usize,
    written: usize,
}

fn decodeEscapeInto(src: []const u8, esc_pos: usize, dst: []u8, dst_pos: usize) -> EscDecodeResult {
    // src[esc_pos] == '\\'
    let i: usize = esc_pos + 1;
    if (i >= src.len) {
        return EscDecodeResult { next_pos: i, written: 0 };
    }
    const c: u8 = src[i];

    // Line continuation: \<CR><LF> or \<LF> or \<CR>, followed by leading whitespace
    if (c == '\r' as u8 || c == '\n' as u8) {
        if (c == '\r' as u8 && i + 1 < src.len && src[i + 1] == '\n' as u8) {
            i += 2;
        } else {
            i += 1;
        }
        while (i < src.len && isLineWs(src[i])) {
            i += 1;
        }
        return EscDecodeResult { next_pos: i, written: 0 };
    }

    // \uXXXX Unicode escape -> UTF-8
    if (c == 'u' as u8 && i + 4 < src.len) {
        const h0: i32 = fromHex(src[i + 1]);
        const h1: i32 = fromHex(src[i + 2]);
        const h2: i32 = fromHex(src[i + 3]);
        const h3: i32 = fromHex(src[i + 4]);
        if (h0 >= 0 && h1 >= 0 && h2 >= 0 && h3 >= 0) {
            const cp: u32 = ((h0 as u32) << 12) | ((h1 as u32) << 8) | ((h2 as u32) << 4) | (h3 as u32);
            let w: usize = 0;
            if (cp <= 0x7F) {
                if (dst_pos < dst.len) {
                    dst[dst_pos] = cp as u8;
                    w = 1;
                }
            } else if (cp <= 0x7FF) {
                if (dst_pos + 1 < dst.len) {
                    dst[dst_pos] = (0xC0 | (cp >> 6)) as u8;
                    dst[dst_pos + 1] = (0x80 | (cp & 0x3F)) as u8;
                    w = 2;
                }
            } else {
                if (dst_pos + 2 < dst.len) {
                    dst[dst_pos] = (0xE0 | (cp >> 12)) as u8;
                    dst[dst_pos + 1] = (0x80 | ((cp >> 6) & 0x3F)) as u8;
                    dst[dst_pos + 2] = (0x80 | (cp & 0x3F)) as u8;
                    w = 3;
                }
            }
            return EscDecodeResult { next_pos: i + 5, written: w };
        }
    }

    let out_b: u8 = c;
    if (c == 'n' as u8) {
        out_b = '\n' as u8;
    } else if (c == 'r' as u8) {
        out_b = '\r' as u8;
    } else if (c == 't' as u8) {
        out_b = '\t' as u8;
    } else if (c == 'f' as u8) {
        out_b = 0x0C;
    }

    if (dst_pos < dst.len) {
        dst[dst_pos] = out_b;
        return EscDecodeResult { next_pos: i + 1, written: 1 };
    }
    return EscDecodeResult { next_pos: i + 1, written: 0 };
}

pub struct Properties {
    pub entries: [128]PropEntry,
    pub count: usize,
    pub val_buf: [2048]u8,
    pub buf_used: usize,

    pub static fn parse(src: []const u8) -> Properties {
        let p = Properties {
            entries: undefined,
            count: 0,
            val_buf: undefined,
            buf_used: 0,
        };

        let pos: usize = 0;
        while (pos < src.len) {
            // Skip blank lines and leading whitespace
            while (pos < src.len && (isLineWs(src[pos]) || src[pos] == '\r' as u8 || src[pos] == '\n' as u8)) {
                pos += 1;
            }
            if (pos >= src.len) {
                break;
            }

            // Comment line: '#' or '!'
            if (src[pos] == '#' as u8 || src[pos] == '!' as u8) {
                while (pos < src.len && src[pos] != '\n' as u8) {
                    pos += 1;
                }
                if (pos < src.len) {
                    pos += 1;
                }
                continue;
            }

            // Parse key
            const key_start: usize = p.buf_used;
            while (pos < src.len) {
                const c: u8 = src[pos];
                if (c == '\\' as u8) {
                    const dr: EscDecodeResult = decodeEscapeInto(src, pos, p.val_buf[0..VAL_BUF_CAP], p.buf_used);
                    p.buf_used += dr.written;
                    pos = dr.next_pos;
                    continue;
                }
                if (c == '=' as u8 || c == ':' as u8 || isLineWs(c) || c == '\r' as u8 || c == '\n' as u8) {
                    break;
                }
                if (p.buf_used < VAL_BUF_CAP) {
                    p.val_buf[p.buf_used] = c;
                    p.buf_used += 1;
                }
                pos += 1;
            }
            const key_len: usize = p.buf_used - key_start;

            // Skip separator whitespace and optional '=' or ':'
            while (pos < src.len && isLineWs(src[pos])) {
                pos += 1;
            }
            if (pos < src.len && (src[pos] == '=' as u8 || src[pos] == ':' as u8)) {
                pos += 1;
                while (pos < src.len && isLineWs(src[pos])) {
                    pos += 1;
                }
            }

            // Parse value until unescaped newline
            const val_start: usize = p.buf_used;
            while (pos < src.len) {
                const c: u8 = src[pos];
                if (c == '\r' as u8 || c == '\n' as u8) {
                    if (c == '\r' as u8 && pos + 1 < src.len && src[pos + 1] == '\n' as u8) {
                        pos += 2;
                    } else {
                        pos += 1;
                    }
                    break;
                }
                if (c == '\\' as u8) {
                    const dr: EscDecodeResult = decodeEscapeInto(src, pos, p.val_buf[0..VAL_BUF_CAP], p.buf_used);
                    p.buf_used += dr.written;
                    pos = dr.next_pos;
                    continue;
                }
                if (p.buf_used < VAL_BUF_CAP) {
                    p.val_buf[p.buf_used] = c;
                    p.buf_used += 1;
                }
                pos += 1;
            }
            const val_len: usize = p.buf_used - val_start;

            if (key_len > 0 && p.count < MAX_PROPS) {
                p.entries[p.count] = PropEntry {
                    key_start: key_start,
                    key_len: key_len,
                    val_start: val_start,
                    val_len: val_len,
                };
                p.count += 1;
            }
        }

        return p;
    }

    pub fn len() -> usize {
        return _.count;
    }

    fn findIndex(key: []const u8) -> i64 {
        let i: usize = _.count;
        while (i > 0) {
            i -= 1;
            const e: PropEntry = _.entries[i];
            if (bytesEq(_.val_buf[e.key_start..(e.key_start + e.key_len)], key)) {
                return i as i64;
            }
        }
        return -1;
    }

    pub fn has(key: []const u8) -> bool {
        return _.findIndex(key) >= 0;
    }

    pub fn get(key: []const u8) -> []const u8 {
        const idx: i64 = _.findIndex(key);
        if (idx < 0) {
            return "";
        }
        const e: PropEntry = _.entries[idx as usize];
        return _.val_buf[e.val_start..(e.val_start + e.val_len)];
    }
}

fn pushByte(dst: []u8, pos: usize, b: u8) -> i64 {
    if (pos >= dst.len) {
        return -1;
    }
    dst[pos] = b;
    return (pos + 1) as i64;
}

fn pushEscaped(dst: []u8, pos: usize, ch: u8) -> i64 {
    const p1: i64 = pushByte(dst, pos, '\\' as u8);
    if (p1 < 0) {
        return -1;
    }
    return pushByte(dst, p1 as usize, ch);
}

pub fn formatEntry(key: []const u8, value: []const u8, dst: []u8) -> i64 {
    if (key.len == 0) {
        return -1;
    }
    let p: i64 = 0;
    let i: usize = 0;
    while (i < key.len) {
        const c: u8 = key[i];
        if (c == '\\' as u8 || c == '=' as u8 || c == ':' as u8 || c == ' ' as u8) {
            p = pushEscaped(dst, p as usize, c);
        } else if (c == '\t' as u8) {
            p = pushEscaped(dst, p as usize, 't' as u8);
        } else if (c == '\n' as u8) {
            p = pushEscaped(dst, p as usize, 'n' as u8);
        } else if (c == '\r' as u8) {
            p = pushEscaped(dst, p as usize, 'r' as u8);
        } else if (i == 0 && (c == '#' as u8 || c == '!' as u8)) {
            p = pushEscaped(dst, p as usize, c);
        } else {
            p = pushByte(dst, p as usize, c);
        }
        if (p < 0) {
            return -1;
        }
        i += 1;
    }

    p = pushByte(dst, p as usize, '=' as u8);
    if (p < 0) {
        return -1;
    }

    i = 0;
    while (i < value.len) {
        const c: u8 = value[i];
        if (c == '\\' as u8) {
            p = pushEscaped(dst, p as usize, '\\' as u8);
        } else if (c == '\n' as u8) {
            p = pushEscaped(dst, p as usize, 'n' as u8);
        } else if (c == '\r' as u8) {
            p = pushEscaped(dst, p as usize, 'r' as u8);
        } else if (c == '\t' as u8) {
            p = pushEscaped(dst, p as usize, 't' as u8);
        } else if (i == 0 && c == ' ' as u8) {
            p = pushEscaped(dst, p as usize, ' ' as u8);
        } else {
            p = pushByte(dst, p as usize, c);
        }
        if (p < 0) {
            return -1;
        }
        i += 1;
    }

    return pushByte(dst, p as usize, '\n' as u8);
}
