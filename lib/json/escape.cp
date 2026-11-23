// Source: Absorbed from /data/dev/bun/src/json_parser.zig | Synced: 2026-11-21T15:10:23Z (RFC 8259 JSON String Escaper & Unescaper)
// Reference-hardened against serde-rs/json (MIT/Apache-2.0)
// Pure-CP zero-allocation RFC 8259 JSON string escaper and unescaper with 16-lane SIMD scanning
// and strict UTF-16 surrogate pair decoding.

type JsonVec16u8 = @Vector(16, u8);

fn escHexVal(c: u8): u32? {
    if (c >= '0' && c <= '9') return @as(u32, c - '0');
    if (c >= 'a' && c <= 'f') return @as(u32, c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return @as(u32, c - 'A' + 10);
    return null;
}

fn escHexLower(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'a') + (v - 10);
}

fn escWriteByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn escWriteSlice(out: []u8, pos: *usize, seg: []const u8): bool {
    if (seg.len == 0) return true;
    if (pos.* + seg.len > out.len) return false;
    @memcpy(out[pos.* .. pos.* + seg.len], seg);
    pos.* += seg.len;
    return true;
}

fn escWriteUtf8(out: []u8, pos: *usize, cp: u32): bool {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
    if (cp < 0x80) {
        return escWriteByte(out, pos, @as(u8, cp));
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

fn escParseHex4(s: []const u8): u32? {
    if (s.len < 4) return null;
    const h0 = escHexVal(s[0]) ?? return null;
    const h1 = escHexVal(s[1]) ?? return null;
    const h2 = escHexVal(s[2]) ?? return null;
    const h3 = escHexVal(s[3]) ?? return null;
    return (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
}

/// 16-lane `@Vector(16, u8)` SIMD scan returning the first index in `src` that requires
/// RFC 8259 JSON escaping (`"`, `\`, or ASCII control character `< 0x20`), or `null` if none.
pub fn simdIndexOfJsonEscape(src: []const u8): usize? {
    let i: usize = 0;
    const n = src.len;
    const v_quote: JsonVec16u8 = @splat('"');
    const v_bs: JsonVec16u8 = @splat('\\');
    const v_ctrl: JsonVec16u8 = @splat(0x20);

    while (i + 16 <= n) {
        const chunk: JsonVec16u8 = src[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_quote);
        const m_b: u16 = @bitCast(chunk == v_bs);
        const m_c: u16 = @bitCast(chunk < v_ctrl);
        const mask: u16 = m_q | m_b | m_c;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }

    while (i < n) {
        const c = src[i];
        if (c == '"' || c == '\\' || c < 0x20) return i;
        i += 1;
    }
    return null;
}

/// 16-lane SIMD fast check whether `src` contains any byte requiring RFC 8259 escaping (`"`, `\`, or control `< 0x20`).
pub fn needsJsonEscape(src: []const u8): bool {
    return simdIndexOfJsonEscape(src) != null;
}

/// Escape `src` into RFC 8259 JSON string body (without surrounding quotes) into `out`.
/// Uses 16-lane SIMD block skipping to bulk-copy unescaped spans.
pub fn escapeJsonContent(src: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        const rem = src[i..];
        const hit_opt = simdIndexOfJsonEscape(rem);
        if (hit_opt == null) {
            if (!escWriteSlice(out, &pos, rem)) return null;
            break;
        }
        const hit = hit_opt.?;
        if (hit > 0) {
            if (!escWriteSlice(out, &pos, rem[0..hit])) return null;
            i += hit;
        }

        const c = src[i];
        if (c == '"') {
            if (!escWriteByte(out, &pos, '\\') || !escWriteByte(out, &pos, '"')) return null;
        } else if (c == '\\') {
            if (!escWriteByte(out, &pos, '\\') || !escWriteByte(out, &pos, '\\')) return null;
        } else if (c == '\b') {
            if (!escWriteByte(out, &pos, '\\') || !escWriteByte(out, &pos, 'b')) return null;
        } else if (c == 0x0C) {
            if (!escWriteByte(out, &pos, '\\') || !escWriteByte(out, &pos, 'f')) return null;
        } else if (c == '\n') {
            if (!escWriteByte(out, &pos, '\\') || !escWriteByte(out, &pos, 'n')) return null;
        } else if (c == '\r') {
            if (!escWriteByte(out, &pos, '\\') || !escWriteByte(out, &pos, 'r')) return null;
        } else if (c == '\t') {
            if (!escWriteByte(out, &pos, '\\') || !escWriteByte(out, &pos, 't')) return null;
        } else {
            if (pos + 6 > out.len) return null;
            out[pos] = '\\';
            out[pos + 1] = 'u';
            out[pos + 2] = '0';
            out[pos + 3] = '0';
            out[pos + 4] = escHexLower((c >> 4) & 0x0F);
            out[pos + 5] = escHexLower(c & 0x0F);
            pos += 6;
        }
        i += 1;
    }
    return out[0..pos];
}

/// Escape `src` as a quoted RFC 8259 JSON string literal `"..."` into `out`.
pub fn quoteJsonString(src: []const u8, out: []u8): ([]const u8)? {
    if (out.len < 2) return null;
    out[0] = '"';
    const body = escapeJsonContent(src, out[1..out.len - 1]) ?? return null;
    out[1 + body.len] = '"';
    return out[0..body.len + 2];
}

/// Unescape an RFC 8259 JSON string body (without surrounding quotes) into UTF-8 in `out`.
/// Strictly validates escape sequences, control characters (< 0x20 forbidden unescaped), and UTF-16 surrogate pairs.
pub fn unescapeJsonContent(escaped: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < escaped.len) {
        const rem = escaped[i..];
        const hit_opt = simdIndexOfJsonEscape(rem);
        if (hit_opt == null) {
            if (!escWriteSlice(out, &pos, rem)) return null;
            break;
        }
        const hit = hit_opt.?;
        if (hit > 0) {
            if (!escWriteSlice(out, &pos, rem[0..hit])) return null;
            i += hit;
        }

        const c = escaped[i];
        if (c < 0x20) return null; // unescaped control character illegal in RFC 8259
        if (c == '"') {
            // Unescaped quote inside body is copied as-is if caller passes body; keep backward compat
            if (!escWriteByte(out, &pos, c)) return null;
            i += 1;
            continue;
        }

        // c == '\\'
        i += 1;
        if (i >= escaped.len) return null;
        const esc = escaped[i];
        i += 1;
        if (esc == '"' || esc == '\\' || esc == '/') {
            if (!escWriteByte(out, &pos, esc)) return null;
        } else if (esc == 'b') {
            if (!escWriteByte(out, &pos, 0x08)) return null;
        } else if (esc == 'f') {
            if (!escWriteByte(out, &pos, 0x0C)) return null;
        } else if (esc == 'n') {
            if (!escWriteByte(out, &pos, '\n')) return null;
        } else if (esc == 'r') {
            if (!escWriteByte(out, &pos, '\r')) return null;
        } else if (esc == 't') {
            if (!escWriteByte(out, &pos, '\t')) return null;
        } else if (esc == 'u') {
            if (i + 4 > escaped.len) return null;
            const cu1 = escParseHex4(escaped[i..i + 4]) ?? return null;
            i += 4;
            if (cu1 >= 0xD800 && cu1 <= 0xDBFF) {
                // High surrogate MUST be followed by \uXXXX low surrogate
                if (i + 6 > escaped.len || escaped[i] != '\\' || escaped[i + 1] != 'u') return null;
                const cu2 = escParseHex4(escaped[i + 2..i + 6]) ?? return null;
                if (cu2 < 0xDC00 || cu2 > 0xDFFF) return null;
                i += 6;
                const cp: u32 = @as(u32, 0x10000) + (((cu1 - 0xD800) << 10) | (cu2 - 0xDC00));
                if (!escWriteUtf8(out, &pos, cp)) return null;
            } else if (cu1 >= 0xDC00 && cu1 <= 0xDFFF) {
                return null; // lone low surrogate invalid
            } else {
                if (!escWriteUtf8(out, &pos, cu1)) return null;
            }
        } else {
            return null;
        }
    }
    return out[0..pos];
}
