// Source: Absorbed from /data/dev/bun/src/json_parser.zig (RFC 8259 JSON String Escaper & Unescaper)
// Pure-CP zero-allocation RFC 8259 JSON string escaper and unescaper with UTF-16 surrogate pair decoding.

fn hexVal(c: u8): u32? {
    if (c >= '0' && c <= '9') return @as(u32, c - '0');
    if (c >= 'a' && c <= 'f') return @as(u32, c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return @as(u32, c - 'A' + 10);
    return null;
}

fn hexLower(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'a') + (v - 10);
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeUtf8(out: []u8, pos: *usize, cp: u32): bool {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
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

fn parseHex4(s: []const u8): u32? {
    if (s.len < 4) return null;
    const h0 = hexVal(s[0]) ?? return null;
    const h1 = hexVal(s[1]) ?? return null;
    const h2 = hexVal(s[2]) ?? return null;
    const h3 = hexVal(s[3]) ?? return null;
    return (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
}

/// Fast check whether `src` contains any byte requiring RFC 8259 escaping (`"`, `\`, or control `< 0x20`).
export fn needsJsonEscape(src: []const u8): bool {
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        if (c == '"' || c == '\\' || c < 0x20) return true;
        i += 1;
    }
    return false;
}

/// Escape `src` into RFC 8259 JSON string body (without surrounding quotes) into `out`.
export fn escapeJsonContent(src: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        if (c == '"') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, '"')) return null;
        } else if (c == '\\') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, '\\')) return null;
        } else if (c == '\b') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 'b')) return null;
        } else if (c == 0x0C) {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 'f')) return null;
        } else if (c == '\n') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 'n')) return null;
        } else if (c == '\r') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 'r')) return null;
        } else if (c == '\t') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 't')) return null;
        } else if (c < 0x20) {
            if (pos + 6 > out.len) return null;
            out[pos] = '\\';
            out[pos + 1] = 'u';
            out[pos + 2] = '0';
            out[pos + 3] = '0';
            out[pos + 4] = hexLower((c >> 4) & 0x0F);
            out[pos + 5] = hexLower(c & 0x0F);
            pos += 6;
        } else {
            if (!writeByte(out, &pos, c)) return null;
        }
        i += 1;
    }
    return out[0..pos];
}

/// Escape `src` as a quoted RFC 8259 JSON string literal `"..."` into `out`.
export fn quoteJsonString(src: []const u8, out: []u8): ([]const u8)? {
    if (out.len < 2) return null;
    out[0] = '"';
    const body = escapeJsonContent(src, out[1..out.len - 1]) ?? return null;
    out[1 + body.len] = '"';
    return out[0..body.len + 2];
}

/// Unescape an RFC 8259 JSON string body (without surrounding quotes) into UTF-8 in `out`.
/// Strictly validates escape sequences, control characters (< 0x20 forbidden unescaped), and UTF-16 surrogate pairs.
export fn unescapeJsonContent(escaped: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < escaped.len) {
        const c = escaped[i];
        if (c < 0x20) return null; // unescaped control character illegal in RFC 8259
        if (c != '\\') {
            if (!writeByte(out, &pos, c)) return null;
            i += 1;
            continue;
        }
        i += 1;
        if (i >= escaped.len) return null;
        const esc = escaped[i];
        i += 1;
        if (esc == '"' || esc == '\\' || esc == '/') {
            if (!writeByte(out, &pos, esc)) return null;
        } else if (esc == 'b') {
            if (!writeByte(out, &pos, 0x08)) return null;
        } else if (esc == 'f') {
            if (!writeByte(out, &pos, 0x0C)) return null;
        } else if (esc == 'n') {
            if (!writeByte(out, &pos, '\n')) return null;
        } else if (esc == 'r') {
            if (!writeByte(out, &pos, '\r')) return null;
        } else if (esc == 't') {
            if (!writeByte(out, &pos, '\t')) return null;
        } else if (esc == 'u') {
            if (i + 4 > escaped.len) return null;
            const cu1 = parseHex4(escaped[i..i + 4]) ?? return null;
            i += 4;
            if (cu1 >= 0xD800 && cu1 <= 0xDBFF) {
                // High surrogate MUST be followed by \uXXXX low surrogate
                if (i + 6 > escaped.len || escaped[i] != '\\' || escaped[i + 1] != 'u') return null;
                const cu2 = parseHex4(escaped[i + 2..i + 6]) ?? return null;
                if (cu2 < 0xDC00 || cu2 > 0xDFFF) return null;
                i += 6;
                const cp: u32 = @as(u32, 0x10000) + (((cu1 - 0xD800) << 10) | (cu2 - 0xDC00));
                if (!writeUtf8(out, &pos, cp)) return null;
            } else if (cu1 >= 0xDC00 && cu1 <= 0xDFFF) {
                return null; // lone low surrogate invalid
            } else {
                if (!writeUtf8(out, &pos, cu1)) return null;
            }
        } else {
            return null;
        }
    }
    return out[0..pos];
}
