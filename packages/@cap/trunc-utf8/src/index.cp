// @cap/trunc-utf8 — Zero-heap UTF-8 codepoint-boundary safe string truncator
// Clean-room MIT implementation for CP (compatible with npm cli-truncate & Rust unicode-truncate).

/// Returns true if byte `b` is a UTF-8 continuation byte (`10xxxxxx`).
pub fn isContinuationByte(b: u8): bool {
    return (b & 0xc0) == 0x80;
}

/// Find the largest byte index `<= max_bytes` that falls on a valid UTF-8 codepoint boundary.
/// Never splits a multibyte UTF-8 sequence (CJK, Emoji, accented Latin).
pub fn floorCharBoundary(s: string, max_bytes: usize): usize {
    if (max_bytes >= s.len) {
        return s.len;
    }
    let idx: usize = max_bytes;
    while (idx > 0 && isContinuationByte(s[idx])) {
        idx -= 1;
    }
    return idx;
}

/// Zero-copy truncate `s` to at most `max_bytes` bytes without splitting any UTF-8 codepoint.
pub fn truncateBytes(s: string, max_bytes: usize): string {
    let end: usize = floorCharBoundary(s, max_bytes);
    return s[0..end];
}

/// Count UTF-8 scalar codepoints in `s`.
pub fn countCodepoints(s: string): usize {
    let cnt: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isContinuationByte(s[i])) {
            cnt += 1;
        }
        i += 1;
    }
    return cnt;
}

/// Zero-copy truncate `s` to at most `max_chars` UTF-8 codepoints.
pub fn truncateChars(s: string, max_chars: usize): string {
    if (max_chars == 0 || s.len == 0) {
        return "";
    }
    let chars: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isContinuationByte(s[i])) {
            if (chars == max_chars) {
                return s[0..i];
            }
            chars += 1;
        }
        i += 1;
    }
    return s;
}

/// Truncate `s` to at most `max_bytes` bytes into `out`, appending `ellipsis` (e.g. `"..."`)
/// if truncation occurred. Guarantees total output bytes `<= max_bytes` and valid UTF-8 boundary.
/// Returns bytes written to `out` (0 on insufficient `out` buffer for non-empty result).
pub fn truncateWithEllipsis(s: string, max_bytes: usize, ellipsis: string, out: []u8): usize {
    if (s.len <= max_bytes) {
        if (s.len > out.len) { return 0; }
        let i: usize = 0;
        while (i < s.len) { out[i] = s[i]; i += 1; }
        return s.len;
    }
    if (max_bytes == 0) {
        return 0;
    }
    if (ellipsis.len >= max_bytes) {
        let cut: usize = floorCharBoundary(ellipsis, max_bytes);
        if (cut > out.len) { return 0; }
        let i: usize = 0;
        while (i < cut) { out[i] = ellipsis[i]; i += 1; }
        return cut;
    }
    let prefix_budget: usize = max_bytes - ellipsis.len;
    let cut: usize = floorCharBoundary(s, prefix_budget);
    let total: usize = cut + ellipsis.len;
    if (total > out.len) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < cut) { out[pos] = s[i]; pos += 1; i += 1; }
    i = 0;
    while (i < ellipsis.len) { out[pos] = ellipsis[i]; pos += 1; i += 1; }
    return pos;
}
