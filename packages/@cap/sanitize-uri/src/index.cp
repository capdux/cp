// @cap/sanitize-uri — Zero-allocation safe URI sanitizer & percent-encoder in pure CP
// Clean-room MIT implementation of npm `micromark-util-sanitize-uri` (#563, 101M/wk).

fn surToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn surEqLower(a: string, target: string): bool {
    if (a.len != target.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (surToLower(a[i]) != target[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn surIsHex(b: u8): bool {
    return (b >= 48 && b <= 57) || (b >= 65 && b <= 70) || (b >= 97 && b <= 102);
}

fn surIsSafeUriByte(b: u8): bool {
    // Unreserved + RFC 3986 reserved characters allowed in URLs
    if ((b >= 65 && b <= 90) || (b >= 97 && b <= 122) || (b >= 48 && b <= 57)) {
        return true;
    }
    if (b == 45 || b == 46 || b == 95 || b == 126) { return true; } // - . _ ~
    if (b == 58 || b == 47 || b == 63 || b == 35 || b == 91 || b == 93 || b == 64) { return true; } // : / ? # [ ] @
    if (b == 33 || b == 36 || b == 38 || b == 39 || b == 40 || b == 41 || b == 42 || b == 43 || b == 44 || b == 59 || b == 61) {
        return true; // ! $ & ' ( ) * + , ; =
    }
    return false;
}

fn surHexDigit(n: u8): u8 {
    if (n < 10) {
        return 48 + n;
    }
    return 55 + n; // 'A'..'F'
}

/// Returns `true` if `uri` uses a safe protocol (relative URI, `http`, `https`, `mailto`, `tel`, `irc`, `ircs`, `xmpp`, `ftp`).
/// Rejects `javascript:`, `vbscript:`, `data:`, `file:`, and obfuscated control-char schemes.
pub fn isSafeUri(uri: string): bool {
    // Skip leading ASCII whitespace/control bytes
    let start: usize = 0;
    while (start < uri.len && uri[start] <= 32) {
        start += 1;
    }
    const s = uri[start..uri.len];
    if (s.len == 0) {
        return true;
    }

    // Scan until first ':', '/', '?', or '#'
    let colon: usize = s.len;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b == 58) { // ':'
            colon = i;
            break;
        }
        if (b == 47 || b == 63 || b == 35) { // '/', '?', '#' -> relative path
            return true;
        }
        i += 1;
    }
    if (colon == s.len) {
        return true; // no scheme -> relative URI
    }

    // Extract scheme ignoring embedded ASCII control chars / whitespace (obfuscation defense)
    let scheme_buf: [32]u8 = [0; 32];
    let slen: usize = 0;
    let k: usize = 0;
    while (k < colon) {
        const b = s[k];
        if (b > 32) {
            if (slen >= 32) {
                return false;
            }
            scheme_buf[slen] = surToLower(b);
            slen += 1;
        }
        k += 1;
    }
    const scheme = scheme_buf[0..slen] as string;
    if (surEqLower(scheme, "http") || surEqLower(scheme, "https") ||
        surEqLower(scheme, "mailto") || surEqLower(scheme, "tel") ||
        surEqLower(scheme, "irc") || surEqLower(scheme, "ircs") ||
        surEqLower(scheme, "xmpp") || surEqLower(scheme, "ftp")) {
        return true;
    }
    return false;
}

/// Sanitizes `uri` and writes a safely percent-encoded URL into `out`.
/// Returns `0` (empty string) if the URI uses a disallowed protocol.
pub fn sanitizeUri(uri: string, out: []u8): usize {
    if (!isSafeUri(uri)) {
        return 0;
    }
    let start: usize = 0;
    let end: usize = uri.len;
    while (start < end && uri[start] <= 32) {
        start += 1;
    }
    while (end > start && uri[end - 1] <= 32) {
        end -= 1;
    }
    const s = uri[start..end];
    let p: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b == 37 && i + 2 < s.len && surIsHex(s[i + 1]) && surIsHex(s[i + 2])) {
            // Preserve existing valid %XX sequence
            if (p + 2 < out.len) {
                out[p] = 37;
                out[p + 1] = s[i + 1];
                out[p + 2] = s[i + 2];
                p += 3;
            }
            i += 3;
            continue;
        }
        if (surIsSafeUriByte(b)) {
            if (p < out.len) {
                out[p] = b;
                p += 1;
            }
        } else {
            if (p + 2 < out.len) {
                out[p] = 37; // '%'
                out[p + 1] = surHexDigit((b >> 4) & 0x0F);
                out[p + 2] = surHexDigit(b & 0x0F);
                p += 3;
            }
        }
        i += 1;
    }
    return p;
}

export {
    isSafeUri,
    sanitizeUri,
};
