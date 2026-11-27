// @cap/urlencoding — Zero-heap RFC 3986 percent-encoding/decoding (crates.io:urlencoding v2)

fn urlencHexUpper(nibble: u8): u8 {
    if (nibble < 10) {
        return ('0' as u8) + nibble;
    }
    return ('A' as u8) + (nibble - 10);
}

fn urlencHexVal(ch: u8): i16 {
    if (ch >= '0' && ch <= '9') {
        return (ch - ('0' as u8)) as i16;
    }
    if (ch >= 'A' && ch <= 'F') {
        return ((ch - ('A' as u8)) + 10) as i16;
    }
    if (ch >= 'a' && ch <= 'f') {
        return ((ch - ('a' as u8)) + 10) as i16;
    }
    return -1;
}

/// Return true if byte `b` is an RFC 3986 unreserved character:
/// `A-Z`, `a-z`, `0-9`, `-`, `_`, `.`, `~`.
pub fn isUnreserved(b: u8): bool {
    if ((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9')) {
        return true;
    }
    return b == '-' || b == '_' || b == '.' || b == '~';
}

/// Compute the exact encoded byte length for `input` under RFC 3986.
pub fn encodedLen(input: string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (isUnreserved(input[i])) {
            total += 1;
        } else {
            total += 3;
        }
        i += 1;
    }
    return total;
}

/// Percent-encode arbitrary bytes into `out` using uppercase hex `%XX`.
/// Returns the number of bytes written to `out`.
pub fn encodeBinary(data: []const u8, out: []u8): usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < data.len) {
        const b = data[i];
        if (isUnreserved(b)) {
            if (w >= out.len) {
                break;
            }
            out[w] = b;
            w += 1;
        } else {
            if (w + 3 > out.len) {
                break;
            }
            out[w] = '%';
            out[w + 1] = urlencHexUpper((b >> 4) & 0x0F);
            out[w + 2] = urlencHexUpper(b & 0x0F);
            w += 3;
        }
        i += 1;
    }
    return w;
}

/// Percent-encode a UTF-8 string into `out` using RFC 3986 unreserved rules and uppercase `%XX`.
/// Returns the number of bytes written to `out`.
pub fn encode(input: string, out: []u8): usize {
    return encodeBinary(input, out);
}

/// Strictly decode percent-encoded `input` into `out`.
/// Returns the number of bytes written, or `-1` on malformed `%XX` escape or insufficient `out` buffer.
pub fn decode(input: string, out: []u8): i64 {
    let w: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const b = input[i];
        if (b == '%') {
            if (i + 2 >= input.len) {
                return -1;
            }
            const hi = urlencHexVal(input[i + 1]);
            const lo = urlencHexVal(input[i + 2]);
            if (hi < 0 || lo < 0) {
                return -1;
            }
            if (w >= out.len) {
                return -1;
            }
            out[w] = (((hi as u8) << 4) | (lo as u8));
            w += 1;
            i += 3;
        } else {
            if (w >= out.len) {
                return -1;
            }
            out[w] = b;
            w += 1;
            i += 1;
        }
    }
    return w as i64;
}

/// Lossy percent-decoding: valid `%XX` escapes are decoded; invalid or truncated `%` sequences are passed through verbatim.
/// Returns the number of bytes written to `out`.
pub fn decodeLossy(input: string, out: []u8): usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < input.len && w < out.len) {
        const b = input[i];
        if (b == '%' && i + 2 < input.len) {
            const hi = urlencHexVal(input[i + 1]);
            const lo = urlencHexVal(input[i + 2]);
            if (hi >= 0 && lo >= 0) {
                out[w] = (((hi as u8) << 4) | (lo as u8));
                w += 1;
                i += 3;
                continue;
            }
        }
        out[w] = b;
        w += 1;
        i += 1;
    }
    return w;
}
