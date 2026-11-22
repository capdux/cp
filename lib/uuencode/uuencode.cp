// Origin: Pure-CP Stdlib (Zero-Alloc POSIX uuencode/uudecode & xxencode Codec) | Created: 2026-11-22

pub const UU_MAX_LINE_BYTES: usize = 45;
pub const UU_DEFAULT_MODE: u16 = 0o644;
pub const XX_ALPHABET: []const u8 = "+-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/// Parsed `begin <octal_mode> <filename>` header.
pub struct UuHeader {
    mode: u16,
    filename: []const u8,
    body_offset: usize,
}

/// Result of decoding a full `uuencode` or `xxencode` payload.
pub struct UuDecoded {
    mode: u16,
    filename: []const u8,
    len: usize,
}

fn uuEncodeChar(v: u8): u8 {
    const masked = v & 0x3F;
    if (masked == 0) {
        return 0x60; // '`'
    }
    return masked + 0x20;
}

fn uuDecodeChar(ch: u8): u8? {
    if (ch == 0x20 || ch == 0x60) {
        return 0;
    }
    if (ch >= 0x21 && ch <= 0x5F) {
        return ch - 0x20;
    }
    return null;
}

fn xxEncodeChar(v: u8): u8 {
    return XX_ALPHABET[(v & 0x3F) as usize];
}

fn xxDecodeChar(ch: u8): u8? {
    if (ch == '+') {
        return 0;
    }
    if (ch == '-') {
        return 1;
    }
    if (ch >= '0' && ch <= '9') {
        return 2 + (ch - '0');
    }
    if (ch >= 'A' && ch <= 'Z') {
        return 12 + (ch - 'A');
    }
    if (ch >= 'a' && ch <= 'z') {
        return 38 + (ch - 'a');
    }
    return null;
}

fn appendBytes(dst: *[]u8, pos: *usize, s: []const u8): bool {
    if (dst.*.len - pos.* < s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        dst.*[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendByte(dst: *[]u8, pos: *usize, b: u8): bool {
    if (pos.* >= dst.*.len) {
        return false;
    }
    dst.*[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeOctalMode(dst: *[]u8, pos: *usize, mode: u16): bool {
    let tmp: [6]u8 = undefined;
    let m = mode;
    let n: usize = 0;
    while (m > 0 || n < 3) {
        tmp[n] = ('0' as u8) + ((m & 7) as u8);
        m >>= 3;
        n += 1;
    }
    if (dst.*.len - pos.* < n) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        dst.*[pos.* + i] = tmp[n - 1 - i];
        i += 1;
    }
    pos.* += n;
    return true;
}

/// Parses a `begin <octal_mode> <filename>\n` header (supporting LF or CRLF).
pub fn parseUuHeader(data: []const u8): UuHeader? {
    if (data.len < 10) {
        return null;
    }
    if (data[0] != 'b' || data[1] != 'e' || data[2] != 'g' || data[3] != 'i' || data[4] != 'n' || data[5] != ' ') {
        return null;
    }

    let pos: usize = 6;
    const mode_start = pos;
    let mode: u32 = 0;
    while (pos < data.len && data[pos] != ' ' && data[pos] != '\n' && data[pos] != '\r') {
        const ch = data[pos];
        if (ch < '0' || ch > '7') {
            return null;
        }
        mode = (mode << 3) | ((ch - '0') as u32);
        if (mode > 0xFFFF) {
            return null;
        }
        pos += 1;
    }
    if (pos == mode_start || pos >= data.len || data[pos] != ' ') {
        return null;
    }
    pos += 1; // skip space

    const name_start = pos;
    while (pos < data.len && data[pos] != '\n' && data[pos] != '\r') {
        pos += 1;
    }
    const name_end = pos;
    if (name_end == name_start || pos >= data.len) {
        return null;
    }
    if (data[pos] == '\r') {
        pos += 1;
        if (pos >= data.len || data[pos] != '\n') {
            return null;
        }
    }
    pos += 1; // skip '\n'

    return UuHeader {
        mode: mode as u16,
        filename: data[name_start..name_end],
        body_offset: pos,
    };
}

fn encodeBodyImpl(src: []const u8, dst: *[]u8, pos: *usize, is_xx: bool): bool {
    let offset: usize = 0;
    while (offset < src.len) {
        let line_len = src.len - offset;
        if (line_len > UU_MAX_LINE_BYTES) {
            line_len = UU_MAX_LINE_BYTES;
        }
        const prefix_ch = if (is_xx) {
            xxEncodeChar(line_len as u8)
        } else {
            uuEncodeChar(line_len as u8)
        };
        if (!appendByte(dst, pos, prefix_ch)) {
            return false;
        }

        let i: usize = 0;
        while (i < line_len) {
            const b0 = src[offset + i];
            const b1: u8 = if (i + 1 < line_len) { src[offset + i + 1] } else { 0 };
            const b2: u8 = if (i + 2 < line_len) { src[offset + i + 2] } else { 0 };

            const c0 = b0 >> 2;
            const c1 = ((b0 & 0x03) << 4) | (b1 >> 4);
            const c2 = ((b1 & 0x0F) << 2) | (b2 >> 6);
            const c3 = b2 & 0x3F;

            if (is_xx) {
                if (!appendByte(dst, pos, xxEncodeChar(c0)) ||
                    !appendByte(dst, pos, xxEncodeChar(c1)) ||
                    !appendByte(dst, pos, xxEncodeChar(c2)) ||
                    !appendByte(dst, pos, xxEncodeChar(c3))) {
                    return false;
                }
            } else {
                if (!appendByte(dst, pos, uuEncodeChar(c0)) ||
                    !appendByte(dst, pos, uuEncodeChar(c1)) ||
                    !appendByte(dst, pos, uuEncodeChar(c2)) ||
                    !appendByte(dst, pos, uuEncodeChar(c3))) {
                    return false;
                }
            }
            i += 3;
        }

        if (!appendByte(dst, pos, '\n')) {
            return false;
        }
        offset += line_len;
    }
    return true;
}

fn decodeLineImpl(line: []const u8, dst: *[]u8, out_pos: *usize, is_xx: bool): usize? {
    if (line.len == 0) {
        return null;
    }
    let line_bytes: usize = 0;
    if (is_xx) {
        if (v := xxDecodeChar(line[0])) {
            line_bytes = v as usize;
        } else {
            return null;
        }
    } else {
        if (v := uuDecodeChar(line[0])) {
            line_bytes = v as usize;
        } else {
            return null;
        }
    }

    if (line_bytes == 0) {
        return 0;
    }
    if (line_bytes > UU_MAX_LINE_BYTES) {
        return null;
    }

    const groups = (line_bytes + 2) / 3;
    const needed_chars = 1 + groups * 4;
    if (line.len < needed_chars) {
        return null;
    }
    if (dst.*.len - out_pos.* < line_bytes) {
        return null;
    }

    let g: usize = 0;
    let produced: usize = 0;
    while (g < groups) {
        const base = 1 + g * 4;
        let v0: u8 = 0;
        let v1: u8 = 0;
        let v2: u8 = 0;
        let v3: u8 = 0;
        if (is_xx) {
            if (d0 := xxDecodeChar(line[base + 0])) { v0 = d0; } else { return null; }
            if (d1 := xxDecodeChar(line[base + 1])) { v1 = d1; } else { return null; }
            if (d2 := xxDecodeChar(line[base + 2])) { v2 = d2; } else { return null; }
            if (d3 := xxDecodeChar(line[base + 3])) { v3 = d3; } else { return null; }
        } else {
            if (d0 := uuDecodeChar(line[base + 0])) { v0 = d0; } else { return null; }
            if (d1 := uuDecodeChar(line[base + 1])) { v1 = d1; } else { return null; }
            if (d2 := uuDecodeChar(line[base + 2])) { v2 = d2; } else { return null; }
            if (d3 := uuDecodeChar(line[base + 3])) { v3 = d3; } else { return null; }
        }

        const b0 = (v0 << 2) | (v1 >> 4);
        const b1 = ((v1 & 0x0F) << 4) | (v2 >> 2);
        const b2 = ((v2 & 0x03) << 6) | v3;

        dst.*[out_pos.*] = b0;
        out_pos.* += 1;
        produced += 1;
        if (produced < line_bytes) {
            dst.*[out_pos.*] = b1;
            out_pos.* += 1;
            produced += 1;
        }
        if (produced < line_bytes) {
            dst.*[out_pos.*] = b2;
            out_pos.* += 1;
            produced += 1;
        }
        g += 1;
    }
    return line_bytes;
}

fn nextLine(data: []const u8, pos: *usize): ([]const u8)? {
    if (pos.* >= data.len) {
        return null;
    }
    const start = pos.*;
    while (pos.* < data.len && data[pos.*] != '\n') {
        pos.* += 1;
    }
    let end = pos.*;
    if (pos.* < data.len && data[pos.*] == '\n') {
        pos.* += 1;
    }
    if (end > start && data[end - 1] == '\r') {
        end -= 1;
    }
    return data[start..end];
}

fn isEndLine(line: []const u8): bool {
    return line.len == 3 && line[0] == 'e' && line[1] == 'n' && line[2] == 'd';
}

/// Encodes `src` into raw `uuencode` body lines (no `begin`/`end` wrapper).
pub fn uuEncodeRaw(src: []const u8, dst: *[]u8): usize? {
    let pos: usize = 0;
    if (!encodeBodyImpl(src, dst, *pos, false)) {
        return null;
    }
    return pos;
}

/// Decodes raw `uuencode` body lines (no `begin`/`end` wrapper).
pub fn uuDecodeRaw(src: []const u8, dst: *[]u8): usize? {
    let pos: usize = 0;
    let out_pos: usize = 0;
    while (line := nextLine(src, *pos)) {
        if (line.len == 0) {
            continue;
        }
        if (n := decodeLineImpl(line, dst, *out_pos, false)) {
            if (n == 0) {
                break;
            }
        } else {
            return null;
        }
    }
    return out_pos;
}

fn encodeFullImpl(src: []const u8, mode: u16, filename: []const u8, dst: *[]u8, is_xx: bool): usize? {
    if (filename.len == 0) {
        return null;
    }
    let pos: usize = 0;
    if (!appendBytes(dst, *pos, "begin ")) {
        return null;
    }
    if (!writeOctalMode(dst, *pos, mode)) {
        return null;
    }
    if (!appendByte(dst, *pos, ' ') || !appendBytes(dst, *pos, filename) || !appendByte(dst, *pos, '\n')) {
        return null;
    }
    if (!encodeBodyImpl(src, dst, *pos, is_xx)) {
        return null;
    }
    const zero_char: u8 = if (is_xx) { '+' as u8 } else { 0x60 as u8 };
    if (!appendByte(dst, *pos, zero_char) || !appendBytes(dst, *pos, "\nend\n")) {
        return null;
    }
    return pos;
}

fn decodeFullImpl(src: []const u8, dst: *[]u8, is_xx: bool): UuDecoded? {
    if (hdr := parseUuHeader(src)) {
        let pos = hdr.body_offset;
        let out_pos: usize = 0;
        let saw_zero_line = false;

        while (line := nextLine(src, *pos)) {
            if (isEndLine(line)) {
                return UuDecoded {
                    mode: hdr.mode,
                    filename: hdr.filename,
                    len: out_pos,
                };
            }
            if (saw_zero_line) {
                return null;
            }
            if (n := decodeLineImpl(line, dst, *out_pos, is_xx)) {
                if (n == 0) {
                    saw_zero_line = true;
                }
            } else {
                return null;
            }
        }
    }
    return null;
}

/// Encodes `src` into full POSIX `uuencode` format (`begin <octal> <filename>\n...\n\`\nend\n`).
pub fn uuEncode(src: []const u8, mode: u16, filename: []const u8, dst: *[]u8): usize? {
    return encodeFullImpl(src, mode, filename, dst, false);
}

/// Decodes a full POSIX `uuencode` payload, supporting both historical space (`0x20`) and backtick (`` ` ``).
pub fn uuDecode(src: []const u8, dst: *[]u8): UuDecoded? {
    return decodeFullImpl(src, dst, false);
}

/// Encodes `src` into full `xxencode` format using the `+-0-9A-Za-z` 64-character alphabet.
pub fn xxEncode(src: []const u8, mode: u16, filename: []const u8, dst: *[]u8): usize? {
    return encodeFullImpl(src, mode, filename, dst, true);
}

/// Decodes a full `xxencode` payload.
pub fn xxDecode(src: []const u8, dst: *[]u8): UuDecoded? {
    return decodeFullImpl(src, dst, true);
}
