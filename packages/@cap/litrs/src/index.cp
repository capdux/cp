// @cap/litrs — Zero-allocation Rust literal lexer, validator & string unescaper
// Clean-room parity with crates.io:litrs (MIT OR Apache-2.0)

pub enum LiteralKind: u8 {
    Invalid = 0,
    Bool = 1,
    Integer = 2,
    Float = 3,
    Char = 4,
    Byte = 5,
    String = 6,
    ByteString = 7,
}

pub struct ParsedLiteral {
    kind: LiteralKind,
    raw: string,
    suffix: string,
    int_val: u64,
    bool_val: bool,
    is_raw_string: bool,
    hash_count: usize,
}

fn ltrInvalid(src: string): ParsedLiteral {
    return ParsedLiteral{
        kind: LiteralKind.Invalid,
        raw: src,
        suffix: "",
        int_val: 0,
        bool_val: false,
        is_raw_string: false,
        hash_count: 0,
    };
}

fn ltrStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn ltrIsIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn ltrIsIdentCont(c: u8): bool {
    return ltrIsIdentStart(c) || (c >= '0' && c <= '9');
}

fn ltrIsValidSuffix(s: string): bool {
    if (s.len == 0) {
        return true;
    }
    if (!ltrIsIdentStart(s[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) : (i += 1) {
        if (!ltrIsIdentCont(s[i])) {
            return false;
        }
    }
    return true;
}

fn ltrHexVal(c: u8): i32 {
    if (c >= '0' && c <= '9') {
        return i32(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
        return i32(c - 'a') + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return i32(c - 'A') + 10;
    }
    return -1;
}

fn ltrDigitVal(c: u8, radix: u64): i32 {
    const v = ltrHexVal(c);
    if (v < 0 || u64(v) >= radix) {
        return -1;
    }
    return v;
}

fn ltrParseRawString(src: string, start: usize, kind: LiteralKind): ParsedLiteral {
    let pos: usize = start;
    let hashes: usize = 0;
    while (pos < src.len && src[pos] == '#') {
        hashes += 1;
        pos += 1;
    }
    if (pos >= src.len || src[pos] != '"') {
        return ltrInvalid(src);
    }
    pos += 1;

    while (pos < src.len) {
        if (src[pos] == '"') {
            let ok: bool = true;
            if (pos + 1 + hashes > src.len) {
                ok = false;
            } else {
                let h: usize = 0;
                while (h < hashes) : (h += 1) {
                    if (src[pos + 1 + h] != '#') {
                        ok = false;
                        break;
                    }
                }
            }
            if (ok) {
                const after = pos + 1 + hashes;
                const suf = src[after..src.len];
                if (!ltrIsValidSuffix(suf)) {
                    return ltrInvalid(src);
                }
                return ParsedLiteral{
                    kind: kind,
                    raw: src,
                    suffix: suf,
                    int_val: 0,
                    bool_val: false,
                    is_raw_string: true,
                    hash_count: hashes,
                };
            }
        }
        pos += 1;
    }
    return ltrInvalid(src);
}

fn ltrParseQuotedString(src: string, quote_pos: usize, kind: LiteralKind): ParsedLiteral {
    let pos: usize = quote_pos + 1;
    while (pos < src.len) {
        const c = src[pos];
        if (c == '\\') {
            pos += 2;
            if (pos > src.len) {
                return ltrInvalid(src);
            }
            continue;
        }
        if (c == '"') {
            const after = pos + 1;
            const suf = src[after..src.len];
            if (!ltrIsValidSuffix(suf)) {
                return ltrInvalid(src);
            }
            return ParsedLiteral{
                kind: kind,
                raw: src,
                suffix: suf,
                int_val: 0,
                bool_val: false,
                is_raw_string: false,
                hash_count: 0,
            };
        }
        pos += 1;
    }
    return ltrInvalid(src);
}

fn ltrParseCharOrByte(src: string, quote_pos: usize, is_byte: bool): ParsedLiteral {
    let pos: usize = quote_pos + 1;
    if (pos >= src.len || src[pos] == '\'') {
        return ltrInvalid(src);
    }
    if (src[pos] == '\\') {
        pos += 1;
        if (pos >= src.len) {
            return ltrInvalid(src);
        }
        const esc = src[pos];
        if (esc == 'n' || esc == 'r' || esc == 't' || esc == '\\' || esc == '\'' || esc == '"' || esc == '0') {
            pos += 1;
        } else if (esc == 'x') {
            if (pos + 2 >= src.len) {
                return ltrInvalid(src);
            }
            if (ltrHexVal(src[pos + 1]) < 0 || ltrHexVal(src[pos + 2]) < 0) {
                return ltrInvalid(src);
            }
            pos += 3;
        } else if (esc == 'u' && !is_byte) {
            if (pos + 1 >= src.len || src[pos + 1] != '{') {
                return ltrInvalid(src);
            }
            pos += 2;
            let digits: usize = 0;
            while (pos < src.len && src[pos] != '}') {
                if (src[pos] == '_') {
                    pos += 1;
                    continue;
                }
                if (ltrHexVal(src[pos]) < 0) {
                    return ltrInvalid(src);
                }
                digits += 1;
                pos += 1;
            }
            if (digits == 0 || digits > 6 || pos >= src.len || src[pos] != '}') {
                return ltrInvalid(src);
            }
            pos += 1;
        } else {
            return ltrInvalid(src);
        }
    } else {
        const b0 = src[pos];
        if (is_byte) {
            if (b0 > 0x7F) {
                return ltrInvalid(src);
            }
            pos += 1;
        } else {
            // Advance 1 UTF-8 codepoint
            let step: usize = 1;
            if ((b0 & 0xE0) == 0xC0) {
                step = 2;
            } else if ((b0 & 0xF0) == 0xE0) {
                step = 3;
            } else if ((b0 & 0xF8) == 0xF0) {
                step = 4;
            }
            if (pos + step > src.len) {
                return ltrInvalid(src);
            }
            pos += step;
        }
    }

    if (pos >= src.len || src[pos] != '\'') {
        return ltrInvalid(src);
    }
    const after = pos + 1;
    const suf = src[after..src.len];
    if (!ltrIsValidSuffix(suf)) {
        return ltrInvalid(src);
    }
    return ParsedLiteral{
        kind: if (is_byte) LiteralKind.Byte else LiteralKind.Char,
        raw: src,
        suffix: suf,
        int_val: 0,
        bool_val: false,
        is_raw_string: false,
        hash_count: 0,
    };
}

fn ltrParseNumber(src: string): ParsedLiteral {
    let pos: usize = 0;
    let radix: u64 = 10;
    let has_prefix: bool = false;

    if (src.len >= 2 && src[0] == '0') {
        const c1 = src[1];
        if (c1 == 'x' || c1 == 'X') {
            radix = 16;
            pos = 2;
            has_prefix = true;
        } else if (c1 == 'b' || c1 == 'B') {
            radix = 2;
            pos = 2;
            has_prefix = true;
        } else if (c1 == 'o' || c1 == 'O') {
            radix = 8;
            pos = 2;
            has_prefix = true;
        }
    }

    let val: u64 = 0;
    let digit_count: usize = 0;
    while (pos < src.len) {
        const c = src[pos];
        if (c == '_') {
            pos += 1;
            continue;
        }
        const d = ltrDigitVal(c, radix);
        if (d < 0) {
            break;
        }
        val = val *% radix +% u64(d);
        digit_count += 1;
        pos += 1;
    }

    if (digit_count == 0) {
        return ltrInvalid(src);
    }

    if (has_prefix) {
        const suf = src[pos..src.len];
        if (!ltrIsValidSuffix(suf)) {
            return ltrInvalid(src);
        }
        return ParsedLiteral{
            kind: LiteralKind.Integer,
            raw: src,
            suffix: suf,
            int_val: val,
            bool_val: false,
            is_raw_string: false,
            hash_count: 0,
        };
    }

    let is_float: bool = false;
    if (pos < src.len && src[pos] == '.') {
        let dot_is_float: bool = true;
        if (pos + 1 < src.len) {
            const nxt = src[pos + 1];
            if (nxt == '.' || ltrIsIdentStart(nxt)) {
                dot_is_float = false;
            }
        }
        if (dot_is_float) {
            is_float = true;
            pos += 1;
            while (pos < src.len) {
                const c = src[pos];
                if (c == '_') {
                    pos += 1;
                    continue;
                }
                if (c >= '0' && c <= '9') {
                    pos += 1;
                    continue;
                }
                break;
            }
        }
    }

    if (pos < src.len && (src[pos] == 'e' || src[pos] == 'E')) {
        is_float = true;
        pos += 1;
        if (pos < src.len && (src[pos] == '+' || src[pos] == '-')) {
            pos += 1;
        }
        let exp_digits: usize = 0;
        while (pos < src.len) {
            const c = src[pos];
            if (c == '_') {
                pos += 1;
                continue;
            }
            if (c >= '0' && c <= '9') {
                exp_digits += 1;
                pos += 1;
                continue;
            }
            break;
        }
        if (exp_digits == 0) {
            return ltrInvalid(src);
        }
    }

    const suf = src[pos..src.len];
    if (!ltrIsValidSuffix(suf)) {
        return ltrInvalid(src);
    }
    if (!is_float && (ltrStrEq(suf, "f32") || ltrStrEq(suf, "f64"))) {
        is_float = true;
    }

    if (is_float) {
        return ParsedLiteral{
            kind: LiteralKind.Float,
            raw: src,
            suffix: suf,
            int_val: 0,
            bool_val: false,
            is_raw_string: false,
            hash_count: 0,
        };
    }
    return ParsedLiteral{
        kind: LiteralKind.Integer,
        raw: src,
        suffix: suf,
        int_val: val,
        bool_val: false,
        is_raw_string: false,
        hash_count: 0,
    };
}

pub fn parseLiteral(src: string): ParsedLiteral {
    if (src.len == 0) {
        return ltrInvalid(src);
    }
    if (ltrStrEq(src, "true")) {
        return ParsedLiteral{
            kind: LiteralKind.Bool,
            raw: src,
            suffix: "",
            int_val: 0,
            bool_val: true,
            is_raw_string: false,
            hash_count: 0,
        };
    }
    if (ltrStrEq(src, "false")) {
        return ParsedLiteral{
            kind: LiteralKind.Bool,
            raw: src,
            suffix: "",
            int_val: 0,
            bool_val: false,
            is_raw_string: false,
            hash_count: 0,
        };
    }

    const c0 = src[0];
    if (c0 == '"') {
        return ltrParseQuotedString(src, 0, LiteralKind.String);
    }
    if (c0 == '\'') {
        return ltrParseCharOrByte(src, 0, false);
    }
    if (c0 == 'r') {
        return ltrParseRawString(src, 1, LiteralKind.String);
    }
    if (c0 == 'b' && src.len >= 2) {
        const c1 = src[1];
        if (c1 == '"') {
            return ltrParseQuotedString(src, 1, LiteralKind.ByteString);
        }
        if (c1 == '\'') {
            return ltrParseCharOrByte(src, 1, true);
        }
        if (c1 == 'r') {
            return ltrParseRawString(src, 2, LiteralKind.ByteString);
        }
    }
    if (c0 >= '0' && c0 <= '9') {
        return ltrParseNumber(src);
    }
    return ltrInvalid(src);
}

fn ltrEncodeUtf8(cp: u32, out: []u8, pos: usize): usize? {
    if (cp <= 0x7F) {
        if (pos + 1 > out.len) {
            return null;
        }
        out[pos] = u8(cp);
        return pos + 1;
    }
    if (cp <= 0x7FF) {
        if (pos + 2 > out.len) {
            return null;
        }
        out[pos] = u8(0xC0 | (cp >> 6));
        out[pos + 1] = u8(0x80 | (cp & 0x3F));
        return pos + 2;
    }
    if (cp <= 0xFFFF) {
        if (pos + 3 > out.len) {
            return null;
        }
        out[pos] = u8(0xE0 | (cp >> 12));
        out[pos + 1] = u8(0x80 | ((cp >> 6) & 0x3F));
        out[pos + 2] = u8(0x80 | (cp & 0x3F));
        return pos + 3;
    }
    if (cp <= 0x10FFFF) {
        if (pos + 4 > out.len) {
            return null;
        }
        out[pos] = u8(0xF0 | (cp >> 18));
        out[pos + 1] = u8(0x80 | ((cp >> 12) & 0x3F));
        out[pos + 2] = u8(0x80 | ((cp >> 6) & 0x3F));
        out[pos + 3] = u8(0x80 | (cp & 0x3F));
        return pos + 4;
    }
    return null;
}

pub fn unescapeString(lit_raw: string, out: []u8): i64 {
    const parsed = parseLiteral(lit_raw);
    if (parsed.kind != LiteralKind.String && parsed.kind != LiteralKind.ByteString) {
        return -1;
    }
    const is_byte = parsed.kind == LiteralKind.ByteString;
    const content_end = lit_raw.len - parsed.suffix.len;

    if (parsed.is_raw_string) {
        const prefix_len: usize = (if (is_byte) usize(2) else usize(1)) + parsed.hash_count + 1;
        const tail_len: usize = 1 + parsed.hash_count;
        if (prefix_len + tail_len > content_end) {
            return -1;
        }
        const body = lit_raw[prefix_len..(content_end - tail_len)];
        if (body.len > out.len) {
            return -1;
        }
        let i: usize = 0;
        while (i < body.len) : (i += 1) {
            out[i] = body[i];
        }
        return i64(body.len);
    }

    const start_idx: usize = if (is_byte) 2 else 1;
    if (start_idx + 1 > content_end) {
        return -1;
    }
    const end_idx: usize = content_end - 1;
    let i: usize = start_idx;
    let w: usize = 0;

    while (i < end_idx) {
        const c = lit_raw[i];
        if (c != '\\') {
            if (w >= out.len) {
                return -1;
            }
            out[w] = c;
            w += 1;
            i += 1;
            continue;
        }
        i += 1;
        if (i >= end_idx) {
            return -1;
        }
        const esc = lit_raw[i];
        if (esc == 'n') {
            if (w >= out.len) { return -1; }
            out[w] = '\n';
            w += 1;
            i += 1;
        } else if (esc == 'r') {
            if (w >= out.len) { return -1; }
            out[w] = '\r';
            w += 1;
            i += 1;
        } else if (esc == 't') {
            if (w >= out.len) { return -1; }
            out[w] = '\t';
            w += 1;
            i += 1;
        } else if (esc == '\\') {
            if (w >= out.len) { return -1; }
            out[w] = '\\';
            w += 1;
            i += 1;
        } else if (esc == '\'') {
            if (w >= out.len) { return -1; }
            out[w] = '\'';
            w += 1;
            i += 1;
        } else if (esc == '"') {
            if (w >= out.len) { return -1; }
            out[w] = '"';
            w += 1;
            i += 1;
        } else if (esc == '0') {
            if (w >= out.len) { return -1; }
            out[w] = 0;
            w += 1;
            i += 1;
        } else if (esc == 'x') {
            if (i + 2 >= end_idx) { return -1; }
            const h1 = ltrHexVal(lit_raw[i + 1]);
            const h2 = ltrHexVal(lit_raw[i + 2]);
            if (h1 < 0 || h2 < 0) { return -1; }
            if (w >= out.len) { return -1; }
            out[w] = u8((h1 << 4) | h2);
            w += 1;
            i += 3;
        } else if (esc == 'u' && !is_byte) {
            if (i + 1 >= end_idx || lit_raw[i + 1] != '{') { return -1; }
            i += 2;
            let cp: u32 = 0;
            let digits: usize = 0;
            while (i < end_idx && lit_raw[i] != '}') {
                if (lit_raw[i] == '_') {
                    i += 1;
                    continue;
                }
                const hv = ltrHexVal(lit_raw[i]);
                if (hv < 0) { return -1; }
                cp = (cp << 4) | u32(hv);
                digits += 1;
                i += 1;
            }
            if (digits == 0 || digits > 6 || i >= end_idx || lit_raw[i] != '}') {
                return -1;
            }
            i += 1;
            if (nw := ltrEncodeUtf8(cp, out, w)) {
                w = nw;
            } else {
                return -1;
            }
        } else if (esc == '\n') {
            i += 1;
            while (i < end_idx && (lit_raw[i] == ' ' || lit_raw[i] == '\t' || lit_raw[i] == '\r' || lit_raw[i] == '\n')) {
                i += 1;
            }
        } else {
            return -1;
        }
    }
    return i64(w);
}
