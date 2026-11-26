// @cap/encodeurl — Zero-allocation RFC 3986 Non-Double-Encoding URL Encoder & RegExp Escaper
// Clean-room MIT implementation in pure CP.

fn isHexDigit(b: u8) -> bool {
    if (b >= 48 && b <= 57) {
        return true; // '0'..'9'
    }
    if (b >= 65 && b <= 70) {
        return true; // 'A'..'F'
    }
    if (b >= 97 && b <= 102) {
        return true; // 'a'..'f'
    }
    return false;
}

fn hexUpper(nibble: u8) -> u8 {
    if (nibble < 10) {
        return 48 + nibble;
    }
    return 55 + nibble; // 'A' = 65 = 55 + 10
}

/// Returns true if `b` is an unescaped RFC 3986 URI character (excluding '%').
fn isSafeUriByte(b: u8) -> bool {
    if (b >= 48 && b <= 57) { return true; }   // 0-9
    if (b >= 65 && b <= 90) { return true; }   // A-Z
    if (b >= 97 && b <= 122) { return true; }  // a-z
    // Unreserved: - . _ ~
    if (b == 45 || b == 46 || b == 95 || b == 126) { return true; }
    // Gen-delims: : / ? # [ ] @
    if (b == 58 || b == 47 || b == 63 || b == 35 || b == 91 || b == 93 || b == 64) { return true; }
    // Sub-delims: ! $ & ' ( ) * + , ; =
    if (b == 33 || b == 36 || b == 38 || b == 39 || b == 40 || b == 41 || b == 42 || b == 43 || b == 44 || b == 59 || b == 61) {
        return true;
    }
    return false;
}

/// Encodes a URL string to a valid RFC 3986 URI without double-encoding existing `%XX` sequences.
/// Unpaired `%` is encoded as `%25`; spaces, controls, `< > " \ ^ ` { | }`, and non-ASCII bytes
/// are percent-encoded using uppercase hex. Returns bytes written, or `-1` if `dst` is too small.
pub fn encodeUrl(url: []const u8, dst: []u8) -> i64 {
    let i: usize = 0;
    let out: usize = 0;
    const n = url.len;

    while (i < n) {
        const b = url[i];
        if (b == 37) {
            // '%'
            if (i + 2 < n && isHexDigit(url[i + 1]) && isHexDigit(url[i + 2])) {
                if (out + 3 > dst.len) {
                    return -1;
                }
                dst[out] = 37;
                dst[out + 1] = url[i + 1];
                dst[out + 2] = url[i + 2];
                out += 3;
                i += 3;
            } else {
                if (out + 3 > dst.len) {
                    return -1;
                }
                dst[out] = 37;     // '%'
                dst[out + 1] = 50; // '2'
                dst[out + 2] = 53; // '5'
                out += 3;
                i += 1;
            }
        } else if (isSafeUriByte(b)) {
            if (out + 1 > dst.len) {
                return -1;
            }
            dst[out] = b;
            out += 1;
            i += 1;
        } else {
            if (out + 3 > dst.len) {
                return -1;
            }
            dst[out] = 37;
            dst[out + 1] = hexUpper((b >> 4) & 0x0F);
            dst[out + 2] = hexUpper(b & 0x0F);
            out += 3;
            i += 1;
        }
    }

    return out as i64;
}

/// Zero-copy check returning true if `url` is already validly RFC 3986 encoded.
pub fn isEncodedUrl(url: []const u8) -> bool {
    let i: usize = 0;
    const n = url.len;
    while (i < n) {
        const b = url[i];
        if (b == 37) {
            if (i + 2 < n && isHexDigit(url[i + 1]) && isHexDigit(url[i + 2])) {
                i += 3;
            } else {
                return false;
            }
        } else if (isSafeUriByte(b)) {
            i += 1;
        } else {
            return false;
        }
    }
    return true;
}

fn isRegExpMeta(b: u8) -> bool {
    // \ ^ $ * + ? . ( ) | { } [ ] -
    if (b == 92 || b == 94 || b == 36 || b == 42 || b == 43 || b == 63 || b == 46) {
        return true;
    }
    if (b == 40 || b == 41 || b == 124 || b == 123 || b == 125 || b == 91 || b == 93 || b == 45) {
        return true;
    }
    return false;
}

/// Escapes all regular expression metacharacters (`\ ^ $ * + ? . ( ) | { } [ ] -`).
/// Returns bytes written into `dst`, or `-1` if `dst` is too small.
pub fn escapeRegExp(src: []const u8, dst: []u8) -> i64 {
    let i: usize = 0;
    let out: usize = 0;
    const n = src.len;

    while (i < n) {
        const b = src[i];
        if (isRegExpMeta(b)) {
            if (out + 2 > dst.len) {
                return -1;
            }
            dst[out] = 92; // '\'
            dst[out + 1] = b;
            out += 2;
        } else {
            if (out + 1 > dst.len) {
                return -1;
            }
            dst[out] = b;
            out += 1;
        }
        i += 1;
    }

    return out as i64;
}
