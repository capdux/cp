// Source: Absorbed from /data/dev/bun/src/http/MimeType.zig | Synced: 2026-11-22T11:12:13Z (RFC 2045 §6.7 Quoted-Printable Codec)
// Pure-CP zero-allocation RFC 2045 Quoted-Printable encoder (76-col soft wraps) and decoder.

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return null;
}

fn hexUpper(v: u8): u8 {
    if (v < 10) return @as(u8, '0') + v;
    return @as(u8, 'A') + (v - 10);
}

fn isSafeQpLiteral(b: u8): bool {
    return (b >= 33 && b <= 60) || (b >= 62 && b <= 126);
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeEncodedByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* + 3 > out.len) return false;
    out[pos.*] = '=';
    out[pos.* + 1] = hexUpper((b >> 4) & 0x0F);
    out[pos.* + 2] = hexUpper(b & 0x0F);
    pos.* += 3;
    return true;
}

/// Encode `input` into RFC 2045 §6.7 Quoted-Printable format with 76-character max line length.
/// Encodes trailing spaces/tabs before line breaks or EOF and emits `=\r\n` soft line breaks.
export fn encodeQuotedPrintable(input: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let line_len: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        const c = input[i];
        // Handle hard line breaks (\r\n or \n)
        if (c == '\r' && i + 1 < input.len && input[i + 1] == '\n') {
            if (!writeByte(out, &pos, '\r')) return null;
            if (!writeByte(out, &pos, '\n')) return null;
            line_len = 0;
            i += 2;
            continue;
        }
        if (c == '\n') {
            if (!writeByte(out, &pos, '\r')) return null;
            if (!writeByte(out, &pos, '\n')) return null;
            line_len = 0;
            i += 1;
            continue;
        }

        const is_space_or_tab = c == ' ' || c == '\t';
        const is_end_of_line = (i + 1 == input.len) || input[i + 1] == '\r' || input[i + 1] == '\n';
        const literal_ok = isSafeQpLiteral(c) || (is_space_or_tab && !is_end_of_line);

        const needed: usize = if (literal_ok) 1 else 3;
        // Reserve 1 column for soft break '=' if we are not at the end of a line
        if (line_len + needed > 75) {
            if (!writeByte(out, &pos, '=')) return null;
            if (!writeByte(out, &pos, '\r')) return null;
            if (!writeByte(out, &pos, '\n')) return null;
            line_len = 0;
        }

        if (literal_ok) {
            if (!writeByte(out, &pos, c)) return null;
            line_len += 1;
        } else {
            if (!writeEncodedByte(out, &pos, c)) return null;
            line_len += 3;
        }
        i += 1;
    }

    return out[0..pos];
}

/// Decode RFC 2045 §6.7 Quoted-Printable input into `out`.
/// Strips soft line breaks (`=\r\n` and `=\n`) and decodes `=XX` hex octets.
/// Returns `null` if `out` is too small; preserves malformed `=` sequences verbatim when `strict == false`.
export fn decodeQuotedPrintable(input: []const u8, out: []u8, strict: bool): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        if (input[i] == '=') {
            // Soft line break: =\r\n or =\n (with optional trailing spaces/tabs before newline per RFC 2045)
            let k = i + 1;
            while (k < input.len && (input[k] == ' ' || input[k] == '\t')) {
                k += 1;
            }
            if (k < input.len && input[k] == '\r' && k + 1 < input.len && input[k + 1] == '\n') {
                i = k + 2;
                continue;
            }
            if (k < input.len && input[k] == '\n') {
                i = k + 1;
                continue;
            }
            if (i + 2 < input.len) {
                const hi = hexVal(input[i + 1]);
                const lo = hexVal(input[i + 2]);
                if (hi != null && lo != null) {
                    if (!writeByte(out, &pos, (hi.? << 4) | lo.?)) return null;
                    i += 3;
                    continue;
                }
            }
            if (strict) return null;
        }
        if (!writeByte(out, &pos, input[i])) return null;
        i += 1;
    }

    return out[0..pos];
}
