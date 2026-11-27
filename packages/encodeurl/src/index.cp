// @cap/encodeurl — Zero-heap idempotent URL encoder modeled on npm `encodeurl` v2 (180M+/week).
// Clean-room MIT implementation reusing `cap:ascii` & `cap:fmt`: percent-encodes spaces,
// control chars, non-ASCII UTF-8 bytes, quotes, `<`, `>`, `\`, `^`, `` ` ``, `{`, `|`, `}`,
// and lone `%`, while preserving valid `%XX` sequences.

import { isHexDigit, hexVal, toUpperByte } from "cap:ascii";
import { writeHexByte } from "cap:fmt";

/// Returns true if byte `b` must be percent-encoded by `encodeUrl`.
/// Encodes CTL (<= 0x20, 0x7F), non-ASCII (>= 0x80), and unsafe ASCII:
/// `"` (34), `<` (60), `>` (62), `\` (92), `^` (94), `` ` `` (96), `{` (123), `|` (124), `}` (125).
pub fn needsUrlEncode(b: u8): bool {
    if (b <= 32 || b >= 127) { return true; }
    return b == 34 || b == 60 || b == 62 || b == 92 || b == 94 || b == 96 ||
        b == 123 || b == 124 || b == 125;
}

/// Computes the exact byte length `encodeUrl` will produce for `url` without writing any buffer.
pub fn encodedUrlLen(url: string): usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < url.len) {
        let b = url[i];
        if (b == 37) {
            if (i + 2 < url.len && isHexDigit(url[i + 1]) && isHexDigit(url[i + 2])) {
                total = total + 3;
                i = i + 3;
                continue;
            }
            total = total + 3;
            i = i + 1;
            continue;
        }
        if (needsUrlEncode(b)) {
            total = total + 3;
        } else {
            total = total + 1;
        }
        i = i + 1;
    }
    return total;
}

/// Encodes a URL string into `out`, preserving existing valid `%XX` sequences (idempotent).
pub fn encodeUrl(url: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < url.len) {
        let b = url[i];
        if (b == 37) { // '%'
            if (i + 2 < url.len && isHexDigit(url[i + 1]) && isHexDigit(url[i + 2])) {
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
                p = writeHexByte(37, true, out, p + 1);
            }
            i = i + 1;
            continue;
        }
        if (needsUrlEncode(b)) {
            if (p + 3 <= out.len) {
                out[p] = 37;
                p = writeHexByte(b, true, out, p + 1);
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

/// Checked variant of `encodeUrl`: returns `null` if `out` cannot hold the full encoded URL.
pub fn encodeUrlChecked(url: string, out: []u8): usize? {
    let req = encodedUrlLen(url);
    if (req > out.len) { return null; }
    return encodeUrl(url, out);
}

/// Encodes `url` like `encodeUrl` and additionally normalizes existing `%xx` hex digits
/// to uppercase `%XX` per RFC 3986 §6.2.2.1.
pub fn encodeUrlNormalized(url: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < url.len) {
        let b = url[i];
        if (b == 37) {
            if (i + 2 < url.len && isHexDigit(url[i + 1]) && isHexDigit(url[i + 2])) {
                if (p + 3 <= out.len) {
                    out[p] = 37;
                    out[p + 1] = toUpperByte(url[i + 1]);
                    out[p + 2] = toUpperByte(url[i + 2]);
                    p = p + 3;
                }
                i = i + 3;
                continue;
            }
            if (p + 3 <= out.len) {
                out[p] = 37;
                p = writeHexByte(37, true, out, p + 1);
            }
            i = i + 1;
            continue;
        }
        if (needsUrlEncode(b)) {
            if (p + 3 <= out.len) {
                out[p] = 37;
                p = writeHexByte(b, true, out, p + 1);
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

/// Percent-decodes valid `%XX` sequences in `url` into `out`, preserving malformed `%` verbatim.
pub fn decodeUrl(url: string, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < url.len && p < out.len) {
        let b = url[i];
        if (b == 37 && i + 2 < url.len) {
            let hi = hexVal(url[i + 1]);
            let lo = hexVal(url[i + 2]);
            if (hi != null && lo != null) {
                let h = hi ?? 0;
                let l = lo ?? 0;
                out[p] = (h << 4) | l;
                p = p + 1;
                i = i + 3;
                continue;
            }
        }
        out[p] = b;
        p = p + 1;
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
            if (i + 2 >= url.len || !isHexDigit(url[i + 1]) || !isHexDigit(url[i + 2])) {
                return false;
            }
            i = i + 3;
            continue;
        }
        if (needsUrlEncode(b)) { return false; }
        i = i + 1;
    }
    return true;
}
