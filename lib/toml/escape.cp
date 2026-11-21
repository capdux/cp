// Source: Absorbed from /data/dev/bun/src/toml/toml_parser.zig | Synced: 2026-11-21T15:30:01Z (TOML 1.0 String & Key Escaper / Unescaper)
// Pure-CP zero-allocation TOML 1.0 bare-key validator, basic string escaper, and strict unescaper (\uXXXX / \UXXXXXXXX).

fn hexVal(c: u8): u32? {
    if (c >= '0' && c <= '9') return @as(u32, c - '0');
    if (c >= 'a' && c <= 'f') return @as(u32, c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return @as(u32, c - 'A' + 10);
    return null;
}

fn hexUpper(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'A') + (v - 10);
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeUtf8(out: []u8, pos: *usize, cp: u32): bool {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) return false;
    if (cp < 0x80) return writeByte(out, pos, @as(u8, cp));
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

/// TOML 1.0 bare key validator (`[A-Za-z0-9_-]+`).
export fn isBareTomlKey(key: []const u8): bool {
    if (key.len == 0) return false;
    let i: usize = 0;
    while (i < key.len) {
        const c = key[i];
        const ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
            c == '_' || c == '-';
        if (!ok) return false;
        i += 1;
    }
    return true;
}

/// Escape `src` into a TOML 1.0 basic string literal `"..."` in `out`.
export fn formatTomlBasicString(src: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (!writeByte(out, &pos, '"')) return null;
    let i: usize = 0;
    while (i < src.len) {
        const c = src[i];
        if (c == '"') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, '"')) return null;
        } else if (c == '\\') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, '\\')) return null;
        } else if (c == 0x08) {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 'b')) return null;
        } else if (c == '\t') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 't')) return null;
        } else if (c == '\n') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 'n')) return null;
        } else if (c == 0x0C) {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 'f')) return null;
        } else if (c == '\r') {
            if (!writeByte(out, &pos, '\\') || !writeByte(out, &pos, 'r')) return null;
        } else if (c < 0x20 || c == 0x7F) {
            if (pos + 6 > out.len) return null;
            out[pos] = '\\';
            out[pos + 1] = 'u';
            out[pos + 2] = '0';
            out[pos + 3] = '0';
            out[pos + 4] = hexUpper((c >> 4) & 0x0F);
            out[pos + 5] = hexUpper(c & 0x0F);
            pos += 6;
        } else {
            if (!writeByte(out, &pos, c)) return null;
        }
        i += 1;
    }
    if (!writeByte(out, &pos, '"')) return null;
    return out[0..pos];
}

/// Format a TOML key: bare if valid `[A-Za-z0-9_-]+`, otherwise basic-quoted `"..."`.
export fn formatTomlKey(key: []const u8, out: []u8): ([]const u8)? {
    if (isBareTomlKey(key)) {
        if (key.len > out.len) return null;
        let i: usize = 0;
        while (i < key.len) {
            out[i] = key[i];
            i += 1;
        }
        return out[0..key.len];
    }
    return formatTomlBasicString(key, out);
}

fn parseHexN(s: []const u8, count: usize): u32? {
    if (s.len < count) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < count) {
        const d = hexVal(s[i]) ?? return null;
        val = (val << 4) | d;
        i += 1;
    }
    return val;
}

/// Unescape a TOML 1.0 basic string body (without surrounding quotes) into UTF-8 in `out`.
/// Supports `\b`, `\t`, `\n`, `\f`, `\r`, `\"`, `\\`, `\uXXXX`, and `\UXXXXXXXX` (strict Unicode scalar values).
export fn unescapeTomlBasic(escaped: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < escaped.len) {
        const c = escaped[i];
        if ((c < 0x20 && c != '\t') || c == 0x7F) return null;
        if (c != '\\') {
            if (!writeByte(out, &pos, c)) return null;
            i += 1;
            continue;
        }
        i += 1;
        if (i >= escaped.len) return null;
        const esc = escaped[i];
        i += 1;
        if (esc == '"' || esc == '\\') {
            if (!writeByte(out, &pos, esc)) return null;
        } else if (esc == 'b') {
            if (!writeByte(out, &pos, 0x08)) return null;
        } else if (esc == 't') {
            if (!writeByte(out, &pos, '\t')) return null;
        } else if (esc == 'n') {
            if (!writeByte(out, &pos, '\n')) return null;
        } else if (esc == 'f') {
            if (!writeByte(out, &pos, 0x0C)) return null;
        } else if (esc == 'r') {
            if (!writeByte(out, &pos, '\r')) return null;
        } else if (esc == 'u') {
            const cp = parseHexN(escaped[i..], 4) ?? return null;
            i += 4;
            if (!writeUtf8(out, &pos, cp)) return null;
        } else if (esc == 'U') {
            const cp = parseHexN(escaped[i..], 8) ?? return null;
            i += 8;
            if (!writeUtf8(out, &pos, cp)) return null;
        } else {
            return null;
        }
    }
    return out[0..pos];
}
