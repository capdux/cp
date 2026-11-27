// @cap/hex — Zero-allocation hexadecimal encoder/decoder & canonical HexDump for CP
// Clean-room MIT implementation inspired by Rust `hex`.

const HEX_LOWER: string = "0123456789abcdef";
const HEX_UPPER: string = "0123456789ABCDEF";

fn parseNibble(c: u8) -> u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return (c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return (c - 'A') + 10;
    return null;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

// ---------------------------------------------------------------------------
// 1. Hex Encoding & Decoding
// ---------------------------------------------------------------------------

/// Encodes `input` bytes as lowercase hexadecimal into `out`.
pub fn encodeHex(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len && pos + 2 <= out.len) {
        const b = input[i];
        out[pos] = HEX_LOWER[((b >> 4) & 0x0F) as usize];
        out[pos + 1] = HEX_LOWER[(b & 0x0F) as usize];
        pos += 2;
        i += 1;
    }
    return out[0..pos];
}

/// Encodes `input` bytes as uppercase hexadecimal into `out`.
pub fn encodeHexUpper(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len && pos + 2 <= out.len) {
        const b = input[i];
        out[pos] = HEX_UPPER[((b >> 4) & 0x0F) as usize];
        out[pos + 1] = HEX_UPPER[(b & 0x0F) as usize];
        pos += 2;
        i += 1;
    }
    return out[0..pos];
}

/// Returns `true` if `s` has even length and consists solely of ASCII hex digits `[0-9a-fA-F]`.
pub fn isValidHex(s: string) -> bool {
    if ((s.len % 2) != 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (parseNibble(s[i]) == null) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Decodes a hexadecimal string into `out`.
/// Returns `null` if length is odd, any byte is not a valid hex digit, or `out` is too small.
pub fn decodeHex(hex_str: string, out: []u8) -> string? {
    if ((hex_str.len % 2) != 0) {
        return null;
    }
    const out_len = hex_str.len / 2;
    if (out.len < out_len) {
        return null;
    }
    let i: usize = 0;
    while (i < out_len) {
        const hi = parseNibble(hex_str[i * 2]);
        const lo = parseNibble(hex_str[i * 2 + 1]);
        if (hi == null || lo == null) {
            return null;
        }
        out[i] = (hi.? << 4) | lo.?;
        i += 1;
    }
    return out[0..out_len];
}

/// Decodes a hexadecimal string with an optional `0x` / `0X` prefix into `out`.
pub fn decodeHexPrefixed(hex_str: string, out: []u8) -> string? {
    let s = hex_str;
    if (s.len >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s = s[2..s.len];
    }
    return decodeHex(s, out);
}

// ---------------------------------------------------------------------------
// 2. Fixed-Width Integer Hex Formatters
// ---------------------------------------------------------------------------

/// Encodes `v` as a fixed 8-character lowercase big-endian hexadecimal string into `out`.
pub fn encodeU32Hex(v: u32, out: []u8) -> string {
    if (out.len < 8) {
        return out[0..0];
    }
    let i: usize = 0;
    while (i < 8) {
        const shift: u32 = ((7 - i) * 4) as u32;
        const nib = ((v >> shift) & 0x0F) as usize;
        out[i] = HEX_LOWER[nib];
        i += 1;
    }
    return out[0..8];
}

/// Encodes `v` as a fixed 16-character lowercase big-endian hexadecimal string into `out`.
pub fn encodeU64Hex(v: u64, out: []u8) -> string {
    if (out.len < 16) {
        return out[0..0];
    }
    let i: usize = 0;
    while (i < 16) {
        const shift: u64 = ((15 - i) * 4) as u64;
        const nib = ((v >> shift) & 0x0F) as usize;
        out[i] = HEX_LOWER[nib];
        i += 1;
    }
    return out[0..16];
}

// ---------------------------------------------------------------------------
// 3. Canonical 16-Byte/Line HexDump Formatter
// ---------------------------------------------------------------------------

/// Formats `data` as a canonical 16-bytes-per-line Hex+ASCII dump into `out`:
/// `00000000  48 65 6c 6c 6f ...  |Hello...|\n`
pub fn hexDump(data: string, out: []u8) -> string {
    if (data.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let offset: usize = 0;
    while (offset < data.len) {
        // 8-digit offset
        let off_buf: [8]u8 = undefined;
        const off_str = encodeU32Hex(offset as u32, off_buf[0..8]);
        pos = appendStr(out, pos, off_str);
        pos = appendStr(out, pos, "  ");

        // 16 hex bytes with middle gap after byte 8
        let col: usize = 0;
        while (col < 16) {
            if (col == 8) {
                pos = appendByte(out, pos, ' ');
            }
            if (offset + col < data.len) {
                const b = data[offset + col];
                pos = appendByte(out, pos, HEX_LOWER[((b >> 4) & 0x0F) as usize]);
                pos = appendByte(out, pos, HEX_LOWER[(b & 0x0F) as usize]);
                pos = appendByte(out, pos, ' ');
            } else {
                pos = appendStr(out, pos, "   ");
            }
            col += 1;
        }

        // ASCII sidebar
        pos = appendStr(out, pos, " |");
        col = 0;
        while (col < 16 && offset + col < data.len) {
            const b = data[offset + col];
            if (b >= 0x20 && b <= 0x7E) {
                pos = appendByte(out, pos, b);
            } else {
                pos = appendByte(out, pos, '.');
            }
            col += 1;
        }
        pos = appendStr(out, pos, "|\n");

        offset += 16;
    }
    return out[0..pos];
}
