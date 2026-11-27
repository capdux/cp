// @cap/encodeurl — Zero-heap idempotent URL encoder modeled on npm `encodeurl` v2 (180M+/week).
// Clean-room MIT implementation: percent-encodes spaces, control chars, non-ASCII, quotes,
// `<`, `>`, `\`, `^`, `` ` ``, `{`, `|`, `}`, and lone `%`, while preserving valid `%XX` sequences.

fn euIsHex(b: u8): bool {
    return (b >= 48 && b <= 57) || (b >= 65 && b <= 70) || (b >= 97 && b <= 102);
}

fn euNeedsEncode(b: u8): bool {
    // Encode CTL (<=0x20, 0x7F), non-ASCII (>=0x80), and unsafe ASCII:
    // '"' (34), '<' (60), '>' (62), '\' (92), '^' (94), '`' (96), '{' (123), '|' (124), '}' (125)
    if (b <= 32 || b >= 127) { return true; }
    return b == 34 || b == 60 || b == 62 || b == 92 || b == 94 || b == 96 ||
        b == 123 || b == 124 || b == 125;
}

/// Encodes a URL string into `out`, preserving existing valid `%XX` sequences (idempotent).
pub fn encodeUrl(url: string, out: []u8): usize {
    let hex: string = "0123456789ABCDEF";
    let p: usize = 0;
    let i: usize = 0;
    while (i < url.len) {
        let b = url[i];
        if (b == 37) { // '%'
            if (i + 2 < url.len && euIsHex(url[i + 1]) && euIsHex(url[i + 2])) {
                // Already valid %XX sequence — preserve verbatim
                if (p + 3 <= out.len) {
                    out[p] = 37;
                    out[p + 1] = url[i + 1];
                    out[p + 2] = url[i + 2];
                    p = p + 3;
                }
                i = i + 3;
                continue;
            }
            // Lone '%' -> "%25"
            if (p + 3 <= out.len) {
                out[p] = 37;
                out[p + 1] = 50; // '2'
                out[p + 2] = 53; // '5'
                p = p + 3;
            }
            i = i + 1;
            continue;
        }
        if (euNeedsEncode(b)) {
            if (p + 3 <= out.len) {
                out[p] = 37;
                out[p + 1] = hex[((b >> 4) & 15) as usize];
                out[p + 2] = hex[(b & 15) as usize];
                p = p + 3;
            }
        } else {
            if (p < out.len) {
                out[p] = b;
                p = p + 1;
            }
        }
        i = i + 1;
    }
    return p;
}

/// Returns true if `url` is already safely encoded and `encodeUrl` would not alter it.
pub fn isEncodedUrl(url: string): bool {
    let i: usize = 0;
    while (i < url.len) {
        let b = url[i];
        if (b == 37) {
            if (i + 2 >= url.len || !euIsHex(url[i + 1]) || !euIsHex(url[i + 2])) {
                return false;
            }
            i = i + 3;
            continue;
        }
        if (euNeedsEncode(b)) { return false; }
        i = i + 1;
    }
    return true;
}
