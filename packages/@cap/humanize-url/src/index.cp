// @cap/humanize-url — Zero-allocation & zero-copy URL humanizer for CP
// Clean-room MIT implementation with parity to npm `humanize-url` v3.

fn hurlIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0b || c == 0x0c;
}

fn hurlTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && hurlIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && hurlIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn hurlToLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn hurlStartsWithLower(s: string, lower_prefix: string): bool {
    if (s.len < lower_prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lower_prefix.len) : (i += 1) {
        if (hurlToLowerAscii(s[i]) != lower_prefix[i]) {
            return false;
        }
    }
    return true;
}

fn hurlHasDot(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '.') {
            return true;
        }
        if (s[i] == '/' || s[i] == '?' || s[i] == '#') {
            return false;
        }
    }
    return false;
}

/// Zero-copy humanize: trims whitespace, strips `http://`, `https://`, or `//`,
/// strips case-insensitive leading `www.` (preserving bare domains like `www.com`),
/// and strips a trailing `/` while preserving paths, query strings, and fragments.
pub fn humanizeUrlSlice(url: string): string {
    let s = hurlTrim(url);
    if (s.len == 0) {
        return "";
    }

    // Strip protocol prefix: https://, http://, or //
    if (hurlStartsWithLower(s, "https://")) {
        s = s[8..s.len];
    } else if (hurlStartsWithLower(s, "http://")) {
        s = s[7..s.len];
    } else if (s.len >= 2 && s[0] == '/' && s[1] == '/') {
        s = s[2..s.len];
    }

    // Strip leading www. (case-insensitive) only if remainder host still has a dot
    if (hurlStartsWithLower(s, "www.")) {
        const after_www = s[4..s.len];
        if (hurlHasDot(after_www)) {
            s = after_www;
        }
    }

    // Strip single trailing slash
    if (s.len > 0 && s[s.len - 1] == '/') {
        s = s[0..s.len - 1];
    }

    return s;
}

/// Humanize `url` and write the result into `out`, returning the number of bytes written.
pub fn humanizeUrl(url: string, out: []u8): usize {
    const res = humanizeUrlSlice(url);
    let i: usize = 0;
    while (i < res.len && i < out.len) : (i += 1) {
        out[i] = res[i];
    }
    return i;
}
