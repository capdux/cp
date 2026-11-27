// Source: Absorbed from /data/dev/bun/src/string/immutable.zig | Synced: 2026-11-24T04:42:24Z (HTML Entity Escape & Unescape)
// Pure-CP zero-allocation HTML text/attribute escaper and named/numeric character reference unescaper.

type HtmlEscVec16 = @Vector(16, u8);

fn makeHtmlHexValLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 10) : (i += 1) {
        lut['0' + i] = @as(u8, @intCast(i));
    }
    let j: usize = 0;
    while (j < 6) : (j += 1) {
        lut['a' + j] = @as(u8, @intCast(10 + j));
        lut['A' + j] = @as(u8, @intCast(10 + j));
    }
    return lut;
}

const HTML_HEX_VAL_LUT: [u8; 256] = makeHtmlHexValLut();

fn htmlFindSpecial(src: []const u8, start: usize): usize {
    let i: usize = start;
    const v_amp: HtmlEscVec16 = @splat('&' as u8);
    const v_lt: HtmlEscVec16 = @splat('<' as u8);
    const v_gt: HtmlEscVec16 = @splat('>' as u8);
    const v_quot: HtmlEscVec16 = @splat('"' as u8);
    const v_apos: HtmlEscVec16 = @splat('\'' as u8);
    while (i + 16 <= src.len) : (i += 16) {
        const v: HtmlEscVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, (v == v_amp) | (v == v_lt) | (v == v_gt) | (v == v_quot) | (v == v_apos))) break;
    }
    while (i < src.len) : (i += 1) {
        const c = src[i];
        if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'') return i;
    }
    return src.len;
}

fn htmlFindAmp(src: []const u8, start: usize): usize {
    let i: usize = start;
    const v_amp: HtmlEscVec16 = @splat('&' as u8);
    while (i + 16 <= src.len) : (i += 16) {
        const v: HtmlEscVec16 = src[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == v_amp)) break;
    }
    while (i < src.len) : (i += 1) {
        if (src[i] == '&') return i;
    }
    return src.len;
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
    if (s.len > 0) {
        @memcpy(out[pos.* .. pos.* + s.len], s);
    }
    pos.* += s.len;
    return true;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: HtmlEscVec16 = a[i .. i + 16][0..16].*;
        const vb: HtmlEscVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
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
    return htmlFindSpecial(src, 0) < src.len;
}

/// Escape `&`, `<`, `>`, `"`, `'` into standard HTML entities (`&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#x27;`).
export fn escapeHtml(src: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < src.len) {
        const next_sp = htmlFindSpecial(src, i);
        if (next_sp > i) {
            const n = next_sp - i;
            if (pos + n > out.len) return null;
            @memcpy(out[pos .. pos + n], src[i..next_sp]);
            pos += n;
            i = next_sp;
            if (i >= src.len) break;
        }
        const c = src[i];
        if (c == '&') {
            if (!writeSlice(out, &pos, "&amp;")) return null;
        } else if (c == '<') {
            if (!writeSlice(out, &pos, "&lt;")) return null;
        } else if (c == '>') {
            if (!writeSlice(out, &pos, "&gt;")) return null;
        } else if (c == '"') {
            if (!writeSlice(out, &pos, "&quot;")) return null;
        } else {
            if (!writeSlice(out, &pos, "&#x27;")) return null;
        }
        i += 1;
    }
    return out[0..pos];
}

fn hexDigit(c: u8): u32? {
    const v = HTML_HEX_VAL_LUT[@as(usize, c)];
    if (v == 0xFF) return null;
    return @as(u32, v);
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
        const next_amp = htmlFindAmp(src, i);
        if (next_amp > i) {
            const n = next_amp - i;
            if (pos + n > out.len) return null;
            @memcpy(out[pos .. pos + n], src[i..next_amp]);
            pos += n;
            i = next_amp;
            if (i >= src.len) break;
        }
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
        if (!writeByte(out, &pos, src[i])) return null;
        i += 1;
    }
    return out[0..pos];
}
