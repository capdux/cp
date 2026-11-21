// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc yEnc v1.3 Binary-to-Text Codec, Header/Trailer Parser & CRC32)

// yEnc Protocol Constants
pub const YENC_OFFSET: u8 = 42;
pub const YENC_ESCAPE_OFFSET: u8 = 64;
pub const YENC_ESCAPE_CHAR: u8 = 0x3D; // '='
pub const YENC_DEFAULT_LINE_LEN: usize = 128;

fn addU8(a: u8, b: u8): u8 {
    return (((a as u16) + (b as u16)) & 0xFF) as u8;
}

fn subU8(a: u8, b: u8): u8 {
    return (((a as u16) + 256 - (b as u16)) & 0xFF) as u8;
}

fn bytesEq(a: []const u8, b: []const u8): bool {
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEq(s[0..prefix.len], prefix);
}

fn trimCrlf(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == 0x0D || s[end - 1] == 0x0A)) {
        end -= 1;
    }
    return s[0..end];
}

fn parseDecU64(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    const max_div10: u64 = 1844674407370955161;
    const max_mod10: u64 = 5;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < 0x30 || c > 0x39) {
            return null;
        }
        const digit = (c - 0x30) as u64;
        if (acc > max_div10 || (acc == max_div10 && digit > max_mod10)) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn parseDecU32(s: []const u8): u32? {
    const v = parseDecU64(s) ?? return null;
    if (v > 0xFFFFFFFF) {
        return null;
    }
    return v as u32;
}

fn parseHexU32(s: []const u8): u32? {
    let raw = s;
    if (raw.len >= 2 && raw[0] == 0x30 && (raw[1] == 0x78 || raw[1] == 0x58)) {
        raw = raw[2..raw.len];
    }
    if (raw.len == 0 || raw.len > 8) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        let nib: u32 = 0;
        if (c >= 0x30 && c <= 0x39) {
            nib = (c - 0x30) as u32;
        } else if (c >= 0x61 && c <= 0x66) {
            nib = (c - 0x61 + 10) as u32;
        } else if (c >= 0x41 && c <= 0x46) {
            nib = (c - 0x41 + 10) as u32;
        } else {
            return null;
        }
        acc = (acc << 4) | nib;
        i += 1;
    }
    return acc;
}

/// Computes standard IEEE 802.3 CRC32 (reflected polynomial `0xEDB88320`, init `0xFFFFFFFF`, xorout `0xFFFFFFFF`).
pub fn yencCrc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ (data[i] as u32);
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc ^ 0xFFFFFFFF;
}

/// Encodes binary `src` using yEnc v1.3 into `dst` with line wrapping at `line_len` bytes
/// (0 uses `YENC_DEFAULT_LINE_LEN` = 128). Returns bytes written, or `null` on `dst` overflow / invalid `line_len`.
pub fn yencEncode(src: []const u8, line_len: usize, dst: []u8): usize? {
    let max_col = line_len;
    if (max_col == 0) {
        max_col = YENC_DEFAULT_LINE_LEN;
    }
    if (max_col < 2) {
        return null;
    }

    let out: usize = 0;
    let col: usize = 0;
    let i: usize = 0;

    while (i < src.len) {
        const enc = addU8(src[i], YENC_OFFSET);
        const is_always_crit = enc == 0x00 || enc == 0x0A || enc == 0x0D || enc == YENC_ESCAPE_CHAR;
        const is_sol_crit = col == 0 && (enc == 0x2E || enc == 0x20 || enc == 0x09); // '.', ' ', '\t'
        const is_eol_crit = (col + 1 >= max_col || i + 1 == src.len) && (enc == 0x20 || enc == 0x09);

        if (is_always_crit || is_sol_crit || is_eol_crit) {
            if (dst.len - out < 2) {
                return null;
            }
            dst[out] = YENC_ESCAPE_CHAR;
            dst[out + 1] = addU8(enc, YENC_ESCAPE_OFFSET);
            out += 2;
            col += 2;
        } else {
            if (dst.len - out < 1) {
                return null;
            }
            dst[out] = enc;
            out += 1;
            col += 1;
        }

        i += 1;
        if (col >= max_col && i < src.len) {
            if (dst.len - out < 2) {
                return null;
            }
            dst[out] = 0x0D; // '\r'
            dst[out + 1] = 0x0A; // '\n'
            out += 2;
            col = 0;
        }
    }

    return out;
}

/// Decodes yEnc v1.3 encoded bytes from `src` into `dst`.
/// Skips `\r`/`\n`, unescapes `'='` sequences, and strictly rejects dangling `'='` or `dst` overflow.
pub fn yencDecode(src: []const u8, dst: []u8): usize? {
    let i: usize = 0;
    let out: usize = 0;

    while (i < src.len) {
        const b = src[i];
        i += 1;
        if (b == 0x0D || b == 0x0A) {
            continue;
        }
        if (b == YENC_ESCAPE_CHAR) {
            if (i >= src.len) {
                return null;
            }
            const next_b = src[i];
            i += 1;
            if (next_b == 0x0D || next_b == 0x0A) {
                return null;
            }
            if (out >= dst.len) {
                return null;
            }
            dst[out] = subU8(subU8(next_b, YENC_ESCAPE_OFFSET), YENC_OFFSET);
            out += 1;
        } else {
            if (out >= dst.len) {
                return null;
            }
            dst[out] = subU8(b, YENC_OFFSET);
            out += 1;
        }
    }

    return out;
}

/// Parsed `=ybegin` header line (supports single-part and multi-part archives and space-containing filenames).
pub struct YencBeginHeader {
    line: u32,
    size: u64,
    part: u32?,
    total: u32?,
    name: []const u8,

    /// Returns `true` if `part` is present (multi-part yEnc post).
    fn isMultipart(): bool {
        return _.part != null;
    }
}

/// Parsed `=ypart` header line (`begin` and `end` are 1-based inclusive byte offsets).
pub struct YencPartHeader {
    begin: u64,
    end: u64,

    /// Returns the inclusive byte length of this part (`end - begin + 1`).
    fn partSize(): u64 {
        return _.end - _.begin + 1;
    }
}

/// Parsed `=yend` trailer line.
pub struct YencEndTrailer {
    size: u64,
    part: u32?,
    crc32: u32?,
    pcrc32: u32?,
}

/// Parses a `=ybegin ... name=...` header line.
pub fn parseYencBegin(line: []const u8): YencBeginHeader? {
    const s = trimCrlf(line);
    if (!startsWith(s, "=ybegin ")) {
        return null;
    }

    let line_len: u32 = 0;
    let has_line = false;
    let size: u64 = 0;
    let has_size = false;
    let part: u32? = null;
    let total: u32? = null;
    let name: []const u8 = s[0..0];
    let has_name = false;

    let pos: usize = 8;
    while (pos < s.len) {
        while (pos < s.len && s[pos] == 0x20) {
            pos += 1;
        }
        if (pos >= s.len) {
            break;
        }
        // `name=` extends to the end of the line (spaces allowed)
        if (startsWith(s[pos..s.len], "name=")) {
            const val = s[(pos + 5)..s.len];
            if (val.len == 0) {
                return null;
            }
            name = val;
            has_name = true;
            break;
        }

        const tok_start = pos;
        while (pos < s.len && s[pos] != 0x20) {
            pos += 1;
        }
        const tok = s[tok_start..pos];
        if (startsWith(tok, "line=")) {
            const v = parseDecU32(tok[5..tok.len]) ?? return null;
            if (v == 0) {
                return null;
            }
            line_len = v;
            has_line = true;
        } else if (startsWith(tok, "size=")) {
            const v = parseDecU64(tok[5..tok.len]) ?? return null;
            size = v;
            has_size = true;
        } else if (startsWith(tok, "part=")) {
            const v = parseDecU32(tok[5..tok.len]) ?? return null;
            if (v == 0) {
                return null;
            }
            part = v;
        } else if (startsWith(tok, "total=")) {
            const v = parseDecU32(tok[6..tok.len]) ?? return null;
            if (v == 0) {
                return null;
            }
            total = v;
        }
    }

    if (!has_line || !has_size || !has_name) {
        return null;
    }

    return YencBeginHeader{
        .line = line_len,
        .size = size,
        .part = part,
        .total = total,
        .name = name,
    };
}

/// Parses a `=ypart begin=... end=...` header line.
pub fn parseYencPart(line: []const u8): YencPartHeader? {
    const s = trimCrlf(line);
    if (!startsWith(s, "=ypart ")) {
        return null;
    }

    let begin: u64 = 0;
    let has_begin = false;
    let end: u64 = 0;
    let has_end = false;

    let pos: usize = 7;
    while (pos < s.len) {
        while (pos < s.len && s[pos] == 0x20) {
            pos += 1;
        }
        if (pos >= s.len) {
            break;
        }
        const tok_start = pos;
        while (pos < s.len && s[pos] != 0x20) {
            pos += 1;
        }
        const tok = s[tok_start..pos];
        if (startsWith(tok, "begin=")) {
            begin = parseDecU64(tok[6..tok.len]) ?? return null;
            has_begin = true;
        } else if (startsWith(tok, "end=")) {
            end = parseDecU64(tok[4..tok.len]) ?? return null;
            has_end = true;
        }
    }

    if (!has_begin || !has_end || begin == 0 || end < begin) {
        return null;
    }

    return YencPartHeader{
        .begin = begin,
        .end = end,
    };
}

/// Parses a `=yend size=... [part=...] [pcrc32=...] [crc32=...]` trailer line.
pub fn parseYencEnd(line: []const u8): YencEndTrailer? {
    const s = trimCrlf(line);
    if (!startsWith(s, "=yend ")) {
        return null;
    }

    let size: u64 = 0;
    let has_size = false;
    let part: u32? = null;
    let crc32: u32? = null;
    let pcrc32: u32? = null;

    let pos: usize = 6;
    while (pos < s.len) {
        while (pos < s.len && s[pos] == 0x20) {
            pos += 1;
        }
        if (pos >= s.len) {
            break;
        }
        const tok_start = pos;
        while (pos < s.len && s[pos] != 0x20) {
            pos += 1;
        }
        const tok = s[tok_start..pos];
        if (startsWith(tok, "size=")) {
            size = parseDecU64(tok[5..tok.len]) ?? return null;
            has_size = true;
        } else if (startsWith(tok, "part=")) {
            const v = parseDecU32(tok[5..tok.len]) ?? return null;
            if (v == 0) {
                return null;
            }
            part = v;
        } else if (startsWith(tok, "crc32=")) {
            crc32 = parseHexU32(tok[6..tok.len]) ?? return null;
        } else if (startsWith(tok, "pcrc32=")) {
            pcrc32 = parseHexU32(tok[7..tok.len]) ?? return null;
        }
    }

    if (!has_size) {
        return null;
    }

    return YencEndTrailer{
        .size = size,
        .part = part,
        .crc32 = crc32,
        .pcrc32 = pcrc32,
    };
}
