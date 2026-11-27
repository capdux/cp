// @cap/bearer-auth — Zero-allocation RFC 6750 HTTP Bearer Token & Challenge Utilities
// Clean-room MIT implementation in pure CP.

fn isSpace(b: u8) -> bool {
    return b == 32 || b == 9;
}

fn toLowerAscii(b: u8) -> u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn isB64TokenCore(b: u8) -> bool {
    if (b >= 65 && b <= 90) { return true; }  // A-Z
    if (b >= 97 && b <= 122) { return true; } // a-z
    if (b >= 48 && b <= 57) { return true; }  // 0-9
    // - . _ ~ + /
    return b == 45 || b == 46 || b == 95 || b == 126 || b == 43 || b == 47;
}

/// Validates RFC 6750 `b64token` characters (`A-Z a-z 0-9 - . _ ~ + / =`, non-empty).
pub fn isValidTokenChars(token: string) -> bool {
    if (token.len == 0) {
        return false;
    }
    let seen_pad = false;
    let seen_core = false;
    let i: usize = 0;
    while (i < token.len) {
        const ch = token[i];
        if (ch == 61) {
            // '=' padding only allowed at the tail
            seen_pad = true;
        } else if (isB64TokenCore(ch)) {
            if (seen_pad) {
                return false;
            }
            seen_core = true;
        } else {
            return false;
        }
        i += 1;
    }
    return seen_core;
}

/// Zero-copy extraction of the Bearer token from an HTTP `Authorization` header.
/// Matches `Bearer` case-insensitively, skips extra spaces, and validates `b64token` chars.
/// Returns `""` if missing or invalid.
pub fn extractBearer(header: string) -> string {
    let a: usize = 0;
    let b: usize = header.len;
    while (a < b && isSpace(header[a])) {
        a += 1;
    }
    while (b > a && isSpace(header[b - 1])) {
        b -= 1;
    }
    if (b - a < 7) {
        return "";
    }
    // Check "bearer" case-insensitively followed by space/tab
    if (toLowerAscii(header[a]) != 98
        || toLowerAscii(header[a + 1]) != 101
        || toLowerAscii(header[a + 2]) != 97
        || toLowerAscii(header[a + 3]) != 114
        || toLowerAscii(header[a + 4]) != 101
        || toLowerAscii(header[a + 5]) != 114
        || !isSpace(header[a + 6]))
    {
        return "";
    }

    let tok_start = a + 7;
    while (tok_start < b && isSpace(header[tok_start])) {
        tok_start += 1;
    }
    if (tok_start >= b) {
        return "";
    }
    const tok = header[tok_start..b];
    if (!isValidTokenChars(tok)) {
        return "";
    }
    return tok;
}

/// Formats `Bearer <token>` into `out`. Returns bytes written, or `0` if invalid or buffer too small.
pub fn formatBearer(token: string, out: []u8) -> usize {
    if (!isValidTokenChars(token)) {
        return 0;
    }
    const needed = 7 + token.len;
    if (needed > out.len) {
        return 0;
    }
    const prefix = "Bearer ";
    let i: usize = 0;
    while (i < 7) {
        out[i] = prefix[i];
        i += 1;
    }
    let k: usize = 0;
    while (k < token.len) {
        out[7 + k] = token[k];
        k += 1;
    }
    return needed;
}

fn isBase64UrlChar(b: u8) -> bool {
    if (b >= 65 && b <= 90) { return true; }
    if (b >= 97 && b <= 122) { return true; }
    if (b >= 48 && b <= 57) { return true; }
    return b == 45 || b == 95; // '-', '_'
}

/// Zero-copy structural precheck: returns true if `token` has exactly 3 non-empty
/// dot-separated base64url segments (`header.payload.signature`).
pub fn isJwtLike(token: string) -> bool {
    if (token.len < 5) {
        return false;
    }
    let dots: usize = 0;
    let seg_len: usize = 0;
    let i: usize = 0;
    while (i < token.len) {
        const ch = token[i];
        if (ch == 46) {
            if (seg_len == 0) {
                return false;
            }
            dots += 1;
            if (dots > 2) {
                return false;
            }
            seg_len = 0;
        } else if (isBase64UrlChar(ch)) {
            seg_len += 1;
        } else {
            return false;
        }
        i += 1;
    }
    return dots == 2 && seg_len > 0;
}

/// Constant-time byte slice equality check resistant to timing side-channels.
pub fn constantTimeEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let diff: u8 = 0;
    let i: usize = 0;
    while (i < a.len) {
        diff |= a[i] ^ b[i];
        i += 1;
    }
    return diff == 0;
}

/// Extracts a Bearer token from `header` and verifies it in constant time against `expected_token`.
pub fn verifyBearer(header: string, expected_token: string) -> bool {
    const tok = extractBearer(header);
    if (tok.len == 0 || expected_token.len == 0) {
        return false;
    }
    return constantTimeEq(tok, expected_token);
}

fn pushSlice(dst: []u8, pos: usize, src: string) -> i64 {
    if (pos + src.len > dst.len) {
        return -1;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return (pos + src.len) as i64;
}

fn pushQuotedParam(dst: []u8, pos: usize, first: bool, key: string, val: string) -> i64 {
    let p = pos as i64;
    if (first) {
        p = pushSlice(dst, p as usize, " ");
    } else {
        p = pushSlice(dst, p as usize, ", ");
    }
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, key);
    if (p < 0) { return -1; }
    p = pushSlice(dst, p as usize, "=\"");
    if (p < 0) { return -1; }

    let i: usize = 0;
    while (i < val.len) {
        const ch = val[i];
        if (ch == 34 || ch == 92) {
            if ((p as usize) + 2 > dst.len) { return -1; }
            dst[p as usize] = 92;
            dst[(p as usize) + 1] = ch;
            p += 2;
        } else {
            if ((p as usize) >= dst.len) { return -1; }
            dst[p as usize] = ch;
            p += 1;
        }
        i += 1;
    }
    return pushSlice(dst, p as usize, "\"");
}

/// Formats an RFC 6750 `WWW-Authenticate` Bearer challenge header into `out`:
/// `Bearer realm="...", error="...", error_description="..."`.
/// Empty parameters are omitted. Returns bytes written, or `0` if `out` is too small.
pub fn formatChallenge(realm: string, err_code: string, err_desc: string, out: []u8) -> usize {
    let p = pushSlice(out, 0, "Bearer");
    if (p < 0) { return 0; }
    let first = true;

    if (realm.len > 0) {
        p = pushQuotedParam(out, p as usize, first, "realm", realm);
        if (p < 0) { return 0; }
        first = false;
    }
    if (err_code.len > 0) {
        p = pushQuotedParam(out, p as usize, first, "error", err_code);
        if (p < 0) { return 0; }
        first = false;
    }
    if (err_desc.len > 0) {
        p = pushQuotedParam(out, p as usize, first, "error_description", err_desc);
        if (p < 0) { return 0; }
    }

    return p as usize;
}
