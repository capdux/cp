// packages/@cap/mime-sniff/src/index.cp
// Zero-allocation WHATWG MIME Sniffing Standard binary/text & format sniffer.
// MIT clean-room pure CP implementation.

pub const CT_BINARY: u8 = 0;
pub const CT_TEXT: u8 = 1;
pub const CT_HTML: u8 = 2;
pub const CT_XML: u8 = 3;
pub const CT_JSON: u8 = 4;
pub const CT_SVG: u8 = 5;

fn msToLower(c: u8): u8 {
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn msIsSpace(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8) || c == ('\r' as u8) || c == ('\n' as u8) || c == (0x0C as u8);
}

fn msIsTagTerminator(c: u8): bool {
    return msIsSpace(c) || c == ('>' as u8) || c == ('/' as u8);
}

fn msStartsWithIgnoreCase(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (msToLower(s[i]) != msToLower(prefix[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn msMatchTag(s: []const u8, tagPrefix: []const u8): bool {
    if (!msStartsWithIgnoreCase(s, tagPrefix)) {
        return false;
    }
    if (s.len == tagPrefix.len) {
        return true;
    }
    return msIsTagTerminator(s[tagPrefix.len]);
}

/// Inspect up to the first 8192 bytes to classify binary vs text payloads.
/// - UTF-8 (`EF BB BF`) and UTF-16 (`FF FE` / `FE FF`) BOMs are classified as text.
/// - Presence of NUL (`0x00`) or disallowed control bytes (excluding `\t`, `\n`, `\r`, `\f`, `ESC`) classifies as binary.
pub fn isBinary(data: []const u8): bool {
    if (data.len == 0) {
        return false;
    }
    // Check UTF-8 / UTF-16 BOMs first.
    if (data.len >= 3 && data[0] == (0xEF as u8) && data[1] == (0xBB as u8) && data[2] == (0xBF as u8)) {
        return false;
    }
    if (data.len >= 2) {
        if ((data[0] == (0xFF as u8) && data[1] == (0xFE as u8)) || (data[0] == (0xFE as u8) && data[1] == (0xFF as u8))) {
            return false;
        }
    }

    let limit = data.len;
    if (limit > 8192) {
        limit = 8192;
    }

    let i: usize = 0;
    while (i < limit) {
        const c = data[i];
        if (c == (0x00 as u8) || c == (0x7F as u8)) {
            return true;
        }
        if (c < (0x20 as u8)) {
            const allowed = c == (0x09 as u8) || c == (0x0A as u8) || c == (0x0C as u8) || c == (0x0D as u8) || c == (0x1B as u8);
            if (!allowed) {
                return true;
            }
        }
        i += 1;
    }
    return false;
}

/// Returns true if `data` is classified as text (`!isBinary(data)`).
pub fn isText(data: []const u8): bool {
    return !isBinary(data);
}

/// Sniff content type (`CT_BINARY`, `CT_TEXT`, `CT_HTML`, `CT_XML`, `CT_JSON`, `CT_SVG`)
/// per WHATWG MIME Sniffing heuristics after skipping UTF-8 BOM and leading whitespace.
pub fn sniffContentType(data: []const u8): u8 {
    if (isBinary(data)) {
        return CT_BINARY;
    }

    let start: usize = 0;
    if (data.len >= 3 && data[0] == (0xEF as u8) && data[1] == (0xBB as u8) && data[2] == (0xBF as u8)) {
        start = 3;
    }
    while (start < data.len && msIsSpace(data[start])) {
        start += 1;
    }
    if (start >= data.len) {
        return CT_TEXT;
    }

    const s = data[start..data.len];

    if (msMatchTag(s, "<!DOCTYPE html") || msMatchTag(s, "<html") || msMatchTag(s, "<head") || msMatchTag(s, "<script")) {
        return CT_HTML;
    }
    if (msMatchTag(s, "<svg")) {
        return CT_SVG;
    }
    if (msStartsWithIgnoreCase(s, "<?xml")) {
        return CT_XML;
    }
    if (s[0] == ('{' as u8) || s[0] == ('[' as u8)) {
        return CT_JSON;
    }

    return CT_TEXT;
}

/// Map a `CT_*` content type constant to its canonical MIME type string.
pub fn contentTypeToMime(ct: u8): string {
    if (ct == CT_TEXT) { return "text/plain; charset=utf-8"; }
    if (ct == CT_HTML) { return "text/html; charset=utf-8"; }
    if (ct == CT_XML) { return "application/xml"; }
    if (ct == CT_JSON) { return "application/json"; }
    if (ct == CT_SVG) { return "image/svg+xml"; }
    return "application/octet-stream";
}

/// Sniff `data` and directly return its canonical MIME type string.
pub fn sniffMime(data: []const u8): string {
    return contentTypeToMime(sniffContentType(data));
}

export {
    CT_BINARY,
    CT_TEXT,
    CT_HTML,
    CT_XML,
    CT_JSON,
    CT_SVG,
    isBinary,
    isText,
    sniffContentType,
    contentTypeToMime,
    sniffMime,
};
