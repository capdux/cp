// packages/@cap/iconv-lite/src/index.cp
// Clean-room zero-allocation character encoding converter for CP.
// Parity with npm:iconv-lite core encoding matrix.

pub enum Charset {
    Utf8,
    Ascii,
    Latin1,
    Win1252,
    Utf16Le,
    Utf16Be,
    Unknown,
}

struct IcvUtf8Step {
    cp: u32,
    len: usize,
}

fn icvStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

pub fn parseCharset(name: string): Charset {
    let buf: [32]u8 = undefined;
    let n: usize = 0;
    let i: usize = 0;
    while (i < name.len) : (i += 1) {
        const c = name[i];
        if (c == '-' || c == '_' || c == ' ' || c == '\t') {
            continue;
        }
        if (n >= 32) return .Unknown;
        if (c >= 'A' && c <= 'Z') {
            buf[n] = c + 32;
        } else {
            buf[n] = c;
        }
        n += 1;
    }
    const norm = buf[0..n];
    if (icvStrEq(norm, "utf8")) return .Utf8;
    if (icvStrEq(norm, "ascii") || icvStrEq(norm, "usascii")) return .Ascii;
    if (icvStrEq(norm, "latin1") || icvStrEq(norm, "iso88591") || icvStrEq(norm, "binary") || icvStrEq(norm, "l1")) {
        return .Latin1;
    }
    if (icvStrEq(norm, "win1252") || icvStrEq(norm, "windows1252") || icvStrEq(norm, "cp1252") || icvStrEq(norm, "1252")) {
        return .Win1252;
    }
    if (icvStrEq(norm, "utf16le") || icvStrEq(norm, "ucs2") || icvStrEq(norm, "utf16")) {
        return .Utf16Le;
    }
    if (icvStrEq(norm, "utf16be")) return .Utf16Be;
    return .Unknown;
}

pub fn encodingExists(name: string): bool {
    return parseCharset(name) != .Unknown;
}

fn icvWin1252ToUnicode(b: u8): u32 {
    if (b < 0x80 || b >= 0xA0) return u32(b);
    if (b == 0x80) return 0x20AC;
    if (b == 0x82) return 0x201A;
    if (b == 0x83) return 0x0192;
    if (b == 0x84) return 0x201E;
    if (b == 0x85) return 0x2026;
    if (b == 0x86) return 0x2020;
    if (b == 0x87) return 0x2021;
    if (b == 0x88) return 0x02C6;
    if (b == 0x89) return 0x2030;
    if (b == 0x8A) return 0x0160;
    if (b == 0x8B) return 0x2039;
    if (b == 0x8C) return 0x0152;
    if (b == 0x8E) return 0x017D;
    if (b == 0x91) return 0x2018;
    if (b == 0x92) return 0x2019;
    if (b == 0x93) return 0x201C;
    if (b == 0x94) return 0x201D;
    if (b == 0x95) return 0x2022;
    if (b == 0x96) return 0x2013;
    if (b == 0x97) return 0x2014;
    if (b == 0x98) return 0x02DC;
    if (b == 0x99) return 0x2122;
    if (b == 0x9A) return 0x0161;
    if (b == 0x9B) return 0x203A;
    if (b == 0x9C) return 0x0153;
    if (b == 0x9E) return 0x017E;
    if (b == 0x9F) return 0x0178;
    return u32(b);
}

fn icvUnicodeToWin1252(cp: u32): u8? {
    if (cp < 0x80) return u8(cp);
    if (cp >= 0xA0 && cp <= 0xFF) return u8(cp);
    if (cp == 0x20AC) return 0x80;
    if (cp == 0x201A) return 0x82;
    if (cp == 0x0192) return 0x83;
    if (cp == 0x201E) return 0x84;
    if (cp == 0x2026) return 0x85;
    if (cp == 0x2020) return 0x86;
    if (cp == 0x2021) return 0x87;
    if (cp == 0x02C6) return 0x88;
    if (cp == 0x2030) return 0x89;
    if (cp == 0x0160) return 0x8A;
    if (cp == 0x2039) return 0x8B;
    if (cp == 0x0152) return 0x8C;
    if (cp == 0x017D) return 0x8E;
    if (cp == 0x2018) return 0x91;
    if (cp == 0x2019) return 0x92;
    if (cp == 0x201C) return 0x93;
    if (cp == 0x201D) return 0x94;
    if (cp == 0x2022) return 0x95;
    if (cp == 0x2013) return 0x96;
    if (cp == 0x2014) return 0x97;
    if (cp == 0x02DC) return 0x98;
    if (cp == 0x2122) return 0x99;
    if (cp == 0x0161) return 0x9A;
    if (cp == 0x203A) return 0x9B;
    if (cp == 0x0153) return 0x9C;
    if (cp == 0x017E) return 0x9E;
    if (cp == 0x0178) return 0x9F;
    return null;
}

fn icvWriteUtf8(out: []u8, pos: usize, cp: u32): usize {
    if (cp <= 0x7F) {
        if (pos + 1 > out.len) return pos;
        out[pos] = u8(cp);
        return pos + 1;
    }
    if (cp <= 0x7FF) {
        if (pos + 2 > out.len) return pos;
        out[pos] = u8(0xC0 | (cp >> 6));
        out[pos + 1] = u8(0x80 | (cp & 0x3F));
        return pos + 2;
    }
    if (cp <= 0xFFFF) {
        if (pos + 3 > out.len) return pos;
        out[pos] = u8(0xE0 | (cp >> 12));
        out[pos + 1] = u8(0x80 | ((cp >> 6) & 0x3F));
        out[pos + 2] = u8(0x80 | (cp & 0x3F));
        return pos + 3;
    }
    if (cp <= 0x10FFFF) {
        if (pos + 4 > out.len) return pos;
        out[pos] = u8(0xF0 | (cp >> 18));
        out[pos + 1] = u8(0x80 | ((cp >> 12) & 0x3F));
        out[pos + 2] = u8(0x80 | ((cp >> 6) & 0x3F));
        out[pos + 3] = u8(0x80 | (cp & 0x3F));
        return pos + 4;
    }
    return icvWriteUtf8(out, pos, 0xFFFD);
}

fn icvReadUtf8(s: string, pos: usize): IcvUtf8Step {
    if (pos >= s.len) return IcvUtf8Step{ cp: 0, len: 0 };
    const b0 = u32(s[pos]);
    if (b0 < 0x80) {
        return IcvUtf8Step{ cp: b0, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < s.len) {
        const b1 = u32(s[pos + 1]);
        if ((b1 & 0xC0) == 0x80) {
            const cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
            return IcvUtf8Step{ cp: cp, len: 2 };
        }
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < s.len) {
        const b1 = u32(s[pos + 1]);
        const b2 = u32(s[pos + 2]);
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
            return IcvUtf8Step{ cp: cp, len: 3 };
        }
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < s.len) {
        const b1 = u32(s[pos + 1]);
        const b2 = u32(s[pos + 2]);
        const b3 = u32(s[pos + 3]);
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
            return IcvUtf8Step{ cp: cp, len: 4 };
        }
    }
    return IcvUtf8Step{ cp: 0xFFFD, len: 1 };
}

fn icvReadU16(data: []const u8, pos: usize, is_le: bool): u16 {
    if (is_le) {
        return u16(data[pos]) | (u16(data[pos + 1]) << 8);
    }
    return (u16(data[pos]) << 8) | u16(data[pos + 1]);
}

fn icvWriteU16(out: []u8, pos: usize, val: u16, is_le: bool): usize {
    if (pos + 2 > out.len) return pos;
    if (is_le) {
        out[pos] = u8(val & 0xFF);
        out[pos + 1] = u8((val >> 8) & 0xFF);
    } else {
        out[pos] = u8((val >> 8) & 0xFF);
        out[pos + 1] = u8(val & 0xFF);
    }
    return pos + 2;
}

pub fn decodeToUtf8(data: []const u8, charset: Charset, strip_bom: bool, out: []u8): usize {
    if (charset == .Utf8) {
        let start: usize = 0;
        if (strip_bom && data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
            start = 3;
        }
        let w: usize = 0;
        let i = start;
        while (i < data.len && w < out.len) {
            out[w] = data[i];
            w += 1;
            i += 1;
        }
        return w;
    }

    if (charset == .Ascii) {
        let w: usize = 0;
        let i: usize = 0;
        while (i < data.len && w < out.len) {
            const b = data[i];
            out[w] = if (b < 0x80) b else '?';
            w += 1;
            i += 1;
        }
        return w;
    }

    if (charset == .Latin1) {
        let w: usize = 0;
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            const next_w = icvWriteUtf8(out, w, u32(data[i]));
            if (next_w == w) break;
            w = next_w;
        }
        return w;
    }

    if (charset == .Win1252) {
        let w: usize = 0;
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            const cp = icvWin1252ToUnicode(data[i]);
            const next_w = icvWriteUtf8(out, w, cp);
            if (next_w == w) break;
            w = next_w;
        }
        return w;
    }

    if (charset == .Utf16Le || charset == .Utf16Be) {
        const is_le = charset == .Utf16Le;
        let i: usize = 0;
        if (strip_bom && data.len >= 2) {
            const first = icvReadU16(data, 0, is_le);
            if (first == 0xFEFF) {
                i = 2;
            }
        }
        let w: usize = 0;
        while (i + 1 < data.len) {
            const cu0 = icvReadU16(data, i, is_le);
            i += 2;
            let cp: u32 = u32(cu0);
            if (cu0 >= 0xD800 && cu0 <= 0xDBFF) {
                if (i + 1 < data.len) {
                    const cu1 = icvReadU16(data, i, is_le);
                    if (cu1 >= 0xDC00 && cu1 <= 0xDFFF) {
                        i += 2;
                        cp = u32(0x10000) + ((u32(cu0 - 0xD800) << 10) | u32(cu1 - 0xDC00));
                    } else {
                        cp = 0xFFFD;
                    }
                } else {
                    cp = 0xFFFD;
                }
            } else if (cu0 >= 0xDC00 && cu0 <= 0xDFFF) {
                cp = 0xFFFD;
            }
            const next_w = icvWriteUtf8(out, w, cp);
            if (next_w == w) break;
            w = next_w;
        }
        if (i < data.len) {
            w = icvWriteUtf8(out, w, 0xFFFD);
        }
        return w;
    }

    return 0;
}

pub fn encodeFromUtf8(utf8: string, charset: Charset, add_bom: bool, out: []u8): usize {
    if (charset == .Utf8) {
        let w: usize = 0;
        if (add_bom) {
            if (out.len >= 3) {
                out[0] = 0xEF;
                out[1] = 0xBB;
                out[2] = 0xBF;
                w = 3;
            }
        }
        let i: usize = 0;
        while (i < utf8.len && w < out.len) {
            out[w] = utf8[i];
            w += 1;
            i += 1;
        }
        return w;
    }

    if (charset == .Ascii) {
        let w: usize = 0;
        let i: usize = 0;
        while (i < utf8.len && w < out.len) {
            const step = icvReadUtf8(utf8, i);
            if (step.len == 0) break;
            i += step.len;
            out[w] = if (step.cp < 0x80) u8(step.cp) else '?';
            w += 1;
        }
        return w;
    }

    if (charset == .Latin1) {
        let w: usize = 0;
        let i: usize = 0;
        while (i < utf8.len && w < out.len) {
            const step = icvReadUtf8(utf8, i);
            if (step.len == 0) break;
            i += step.len;
            out[w] = if (step.cp <= 0xFF) u8(step.cp) else '?';
            w += 1;
        }
        return w;
    }

    if (charset == .Win1252) {
        let w: usize = 0;
        let i: usize = 0;
        while (i < utf8.len && w < out.len) {
            const step = icvReadUtf8(utf8, i);
            if (step.len == 0) break;
            i += step.len;
            if (b := icvUnicodeToWin1252(step.cp)) {
                out[w] = b;
            } else {
                out[w] = '?';
            }
            w += 1;
        }
        return w;
    }

    if (charset == .Utf16Le || charset == .Utf16Be) {
        const is_le = charset == .Utf16Le;
        let w: usize = 0;
        if (add_bom) {
            const next_w = icvWriteU16(out, w, 0xFEFF, is_le);
            if (next_w == w) return w;
            w = next_w;
        }
        let i: usize = 0;
        while (i < utf8.len) {
            const step = icvReadUtf8(utf8, i);
            if (step.len == 0) break;
            i += step.len;
            if (step.cp <= 0xFFFF) {
                const next_w = icvWriteU16(out, w, u16(step.cp), is_le);
                if (next_w == w) break;
                w = next_w;
            } else if (step.cp <= 0x10FFFF) {
                const v = step.cp - 0x10000;
                const hi = u16(0xD800 + (v >> 10));
                const lo = u16(0xDC00 + (v & 0x3FF));
                if (w + 4 > out.len) break;
                w = icvWriteU16(out, w, hi, is_le);
                w = icvWriteU16(out, w, lo, is_le);
            } else {
                const next_w = icvWriteU16(out, w, 0xFFFD, is_le);
                if (next_w == w) break;
                w = next_w;
            }
        }
        return w;
    }

    return 0;
}
