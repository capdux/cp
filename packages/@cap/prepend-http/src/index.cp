// @cap/prepend-http — Zero-allocation URL protocol prepender for CP
// Clean-room MIT implementation with parity to npm `prepend-http` v4.

fn phtIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0x0b || c == 0x0c;
}

fn phtTrim(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && phtIsSpace(s[start])) {
        start += 1;
    }
    while (end > start && phtIsSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn phtToLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn phtIsAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn phtIsSchemeChar(c: u8): bool {
    return phtIsAlpha(c) || (c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.';
}

fn phtStartsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

fn phtStartsWithIgnoreCase(s: string, prefix_lower: string): bool {
    if (s.len < prefix_lower.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix_lower.len) : (i += 1) {
        if (phtToLowerAscii(s[i]) != prefix_lower[i]) {
            return false;
        }
    }
    return true;
}

fn phtCopyInto(src: string, out: []u8, offset: usize): usize {
    let pos = offset;
    let i: usize = 0;
    while (i < src.len && pos < out.len) : (i += 1) {
        out[pos] = src[i];
        pos += 1;
    }
    return pos;
}

/// Returns `true` if `url` (after trimming leading/trailing whitespace) starts with
/// a protocol-relative prefix (`//`).
pub fn isProtocolRelative(url: string): bool {
    const trimmed = phtTrim(url);
    return trimmed.len >= 2 && trimmed[0] == '/' && trimmed[1] == '/';
}

/// Returns `true` if `url` (after trimming leading/trailing whitespace) already has
/// an explicit URL scheme (`scheme://` per RFC 3986 or `mailto:`).
pub fn hasScheme(url: string): bool {
    const trimmed = phtTrim(url);
    if (trimmed.len == 0) {
        return false;
    }
    if (phtStartsWithIgnoreCase(trimmed, "mailto:")) {
        return true;
    }
    if (!phtIsAlpha(trimmed[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < trimmed.len && phtIsSchemeChar(trimmed[i])) : (i += 1) {}
    if (i + 2 < trimmed.len && trimmed[i] == ':' && trimmed[i + 1] == '/' && trimmed[i + 2] == '/') {
        return true;
    }
    return false;
}

/// Prepend `https://` (when `https` is `true`) or `http://` (when `https` is `false`)
/// to `url` if it is missing a scheme and is not protocol-relative or a relative path
/// (`scheme://`, `//`, `mailto:`, `./`, `../`, `/`).
/// Trims leading/trailing whitespace, writes the result into `out`, and returns bytes written.
pub fn prependHttp(url: string, https: bool, out: []u8): usize {
    const trimmed = phtTrim(url);
    if (trimmed.len == 0) {
        return 0;
    }
    const keep_as_is =
        hasScheme(trimmed) ||
        isProtocolRelative(trimmed) ||
        phtStartsWith(trimmed, "./") ||
        phtStartsWith(trimmed, "../") ||
        phtStartsWith(trimmed, "/");

    if (keep_as_is) {
        return phtCopyInto(trimmed, out, 0);
    }

    const prefix = if (https) "https://" else "http://";
    const pos = phtCopyInto(prefix, out, 0);
    return phtCopyInto(trimmed, out, pos);
}
