// Source: Absorbed from /data/dev/bun/src/string/immutable.zig (HTML Entity Escape & Unescape)
// Pure-CP zero-allocation HTML text/attribute escaper and named/numeric character reference unescaper.

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn writeUtf8Codepoint(out: []u8, pos: *usize, cp_in: u32): bool {
    let cp = cp_in;
    // Reject NUL, surrogates, and > 0x10FFFF -> U+FFFD replacement character
    if (cp == 0 || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        cp = 0xFFFD;
    }
    if (cp < 0x80) {
        return writeByte(out, pos, @as(u8, cp));
    }
    if (cp < 0x800) {
        if (pos.* + 2 > out.len) return false;
        out[pos.*] = @as(u8, 0xC0 | (cp >> 6));
        out[pos.* + 1] = @as(u8, 0x80 | (cp & 0x3F));
        pos.* += 2;
        return true;
    }
    if (cp < 0x10000) {
        if (pos.* + 3 > out.len) return false;
        out[pos.*] = @as(u8, 0xE0 | (cp >> 12));
        out[pos.* + 1] = @as(u8, 0x80 | ((cp >> 6) & 0x3F));
        out[pos.* + 2] = @as(u8, 0x80 | (cp & 0x3F));
        pos.* += 3;
        return true;
    }
    if (pos.* + 4 > out.len) return false;
    out[pos.*] = @as(u8, 0xF0 | (cp >> 18));
    out[pos.* + 1] = @as(u8, 0x80 | ((cp >> 12) & 0x3F));
    out[pos.* + 2] = @as(u8, 0x80 | ((cp >> 6) & 0x3F));
    out[pos.* + 3] = @as(u8, 0x80 | (cp & 0x3F));
    pos.* += 4;
    return true;
}

/// Fast check whether `src` contains any HTML-special character (`&`, `<`, `>`, `"`, `'`).
export fn needsHtmlEscape(src: []const u8): bool {
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'') return true;
        i += 1;
    }
    return false;
}

/// Escape `&`, `<`, `>`, `"`, `'` into standard HTML entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#x27;`).
export fn escapeHtml(src: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        if (c == '&') {
            if (!writeSlice(out, &pos, "&amp;")) return null;
        } else if (c == '<') {
            if (!writeSlice(out, &pos, "&lt;")) return null;
        } else if (c == '>') {
            if (!writeSlice(out, &pos, "&gt;")) return null;
        } else if (c == '"') {
            if (!writeSlice(out, &pos, "&quot;")) return null;
        } else if (c == '\'') {
            if (!writeSlice(out, &pos, "&#x27;")) return null;
        } else {
            if (!writeByte(out, &pos, c)) return null;
        }
        i += 1;
    }
    return out[0..pos];
}

fn hexDigit(c: u8): u32? {
    if (c >= '0' && c <= '9') return @as(u32, c - '0');
    if (c >= 'a' && c <= 'f') return @as(u32, c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return @as(u32, c - 'A' + 10);
    return null;
}

fn decodeNumericEntity(body: []const u8): u32? {
    if (body.len < 2 || body[0] != '#') return null;
    if (body[1] == 'x' || body[1] == 'X') {
        if (body.len < 3) return null;
        let val: u32 = 0;
        let i: usize = 2;
        while (i < body.len) {
            const d = hexDigit(body[i]) ?? return null;
            if (val <= 0x10FFFF) {
                val = (val << 4) | d;
            }
            i += 1;
        }
        return val;
    }
    let val: u32 = 0;
    let i: usize = 1;
    while (i < body.len) {
        const c = body[i];
        if (c < '0' || c > '9') return null;
        if (val <= 0x10FFFF) {
            val = val * 10 + @as(u32, c - '0');
        }
        i += 1;
    }
    return val;
}

fn decodeNamedEntity(name: []const u8): u32? {
    if (strEq(name, "amp") || strEq(name, "AMP")) return '&';
    if (strEq(name, "lt") || strEq(name, "LT")) return '<';
    if (strEq(name, "gt") || strEq(name, "GT")) return '>';
    if (strEq(name, "quot") || strEq(name, "QUOT")) return '"';
    if (strEq(name, "apos")) return '\'';
    if (strEq(name, "nbsp")) return 0x00A0;
    if (strEq(name, "copy") || strEq(name, "COPY")) return 0x00A9;
    if (strEq(name, "reg") || strEq(name, "REG")) return 0x00AE;
    if (strEq(name, "trade")) return 0x2122;
    if (strEq(name, "mdash")) return 0x2014;
    if (strEq(name, "ndash")) return 0x2013;
    if (strEq(name, "hellip")) return 0x2026;
    return null;
}

/// Unescape common named entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&apos;`, `&nbsp;`, etc.)
/// and decimal/hex numeric character references (`&#39;`, `&#x1F600;`) into UTF-8 in `out`.
/// Unknown or unterminated `&...` sequences are preserved verbatim.
export fn unescapeHtml(src: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        if (src[i] == '&') {
            let semi: usize? = null;
            let k = i + 1;
            const max_scan = if (i + 16 < src.len) i + 16 else src.len;
            while (k < max_scan) {
                if (src[k] == ';') {
                    semi = k;
                    break;
                }
                if (src[k] == '&' || src[k] == ' ' || src[k] == '<') break;
                k += 1;
            }
            if (sp := semi) {
                const body = src[i + 1..sp];
                if (cp := decodeNumericEntity(body)) {
                    if (!writeUtf8Codepoint(out, &pos, cp)) return null;
                    i = sp + 1;
                    continue;
                }
                if (cp := decodeNamedEntity(body)) {
                    if (!writeUtf8Codepoint(out, &pos, cp)) return null;
                    i = sp + 1;
                    continue;
                }
            }
        }
        if (!writeByte(out, &pos, src[i])) return null;
        i += 1;
    }
    return out[0..pos];
}
