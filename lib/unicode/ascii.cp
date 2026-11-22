// Source: Absorbed from /data/dev/bun/src/unicode/uucode_lib/src/ascii.cp | Synced: 2026-11-22T19:21:00Z (Unicode u21 ASCII codepoint predicates, branchless case folding, control codes & zero-alloc ASCII slice helpers)
// Pure-CP zero-allocation ASCII codepoint classification, branchless case conversion, digit/hex parsing, and byte-slice utilities.

/// Standard 7-bit ASCII C0 & DEL control codepoint constants (`u21`).
pub struct AsciiControlCode {
    static const NUL: u21 = 0x00;
    static const SOH: u21 = 0x01;
    static const STX: u21 = 0x02;
    static const ETX: u21 = 0x03;
    static const EOT: u21 = 0x04;
    static const ENQ: u21 = 0x05;
    static const ACK: u21 = 0x06;
    static const BEL: u21 = 0x07;
    static const BS: u21 = 0x08;
    static const HT: u21 = 0x09;
    static const LF: u21 = 0x0A;
    static const VT: u21 = 0x0B;
    static const FF: u21 = 0x0C;
    static const CR: u21 = 0x0D;
    static const SO: u21 = 0x0E;
    static const SI: u21 = 0x0F;
    static const ESC: u21 = 0x1B;
    static const FS: u21 = 0x1C;
    static const GS: u21 = 0x1D;
    static const RS: u21 = 0x1E;
    static const US: u21 = 0x1F;
    static const DEL: u21 = 0x7F;
}

/// Returns whether the Unicode codepoint is a 7-bit ASCII character (`0x00..=0x7F`).
pub fn isAsciiCodepoint(c: u21): bool {
    return c < 128;
}

/// Returns whether the codepoint is ASCII alphanumeric (`A-Z`, `a-z`, or `0-9`).
pub fn isAsciiAlphanumeric(c: u21): bool {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/// Returns whether the codepoint is ASCII alphabetic (`A-Z` or `a-z`).
pub fn isAsciiAlphabetic(c: u21): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/// Returns whether the codepoint is an ASCII control character (`0x00..=0x1F` or `0x7F`).
pub fn isAsciiControl(c: u21): bool {
    return c <= AsciiControlCode.US || c == AsciiControlCode.DEL;
}

/// Returns whether the codepoint is an ASCII decimal digit (`0-9`).
pub fn isAsciiDigit(c: u21): bool {
    return c >= '0' && c <= '9';
}

/// Returns whether the codepoint is an ASCII lowercase letter (`a-z`).
pub fn isAsciiLower(c: u21): bool {
    return c >= 'a' && c <= 'z';
}

/// Returns whether the codepoint is an ASCII uppercase letter (`A-Z`).
pub fn isAsciiUpper(c: u21): bool {
    return c >= 'A' && c <= 'Z';
}

/// Returns whether the codepoint is printable ASCII (`0x20..=0x7E`, including space).
pub fn isAsciiPrint(c: u21): bool {
    return isAsciiCodepoint(c) && !isAsciiControl(c);
}

/// Returns whether the codepoint is a visible ASCII graphic character (`0x21..=0x7E`, excluding space).
pub fn isAsciiGraph(c: u21): bool {
    return c >= 0x21 && c <= 0x7E;
}

/// Returns whether the codepoint is ASCII punctuation (graphic and non-alphanumeric).
pub fn isAsciiPunct(c: u21): bool {
    return isAsciiGraph(c) && !isAsciiAlphanumeric(c);
}

/// Returns whether the codepoint is ASCII whitespace (`' '`, `'\t'`, `'\n'`, `'\x0B'`, `'\x0C'`, `'\r'`).
pub fn isAsciiWhitespace(c: u21): bool {
    return c == ' ' || (c >= '\t' && c <= '\r');
}

/// Returns whether the codepoint is an ASCII hexadecimal digit (`0-9`, `A-F`, or `a-f`).
pub fn isAsciiHex(c: u21): bool {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

/// Branchlessly uppercases an ASCII lowercase codepoint; returns all other codepoints unchanged.
pub fn asciiToUpper(c: u21): u21 {
    const mask: u21 = @as(u21, @intFromBool(isAsciiLower(c))) << 5;
    return c ^ mask;
}

/// Branchlessly lowercases an ASCII uppercase codepoint; returns all other codepoints unchanged.
pub fn asciiToLower(c: u21): u21 {
    const mask: u21 = @as(u21, @intFromBool(isAsciiUpper(c))) << 5;
    return c | mask;
}

/// Parses an ASCII decimal digit codepoint into `0..=9`, or `null` if not a digit.
pub fn asciiDigitValue(c: u21): u8? {
    if (c >= '0' && c <= '9') {
        return @as(u8, @intCast(c - '0'));
    }
    return null;
}

/// Parses an ASCII hexadecimal digit codepoint into `0..=15`, or `null` if not hex.
pub fn asciiHexValue(c: u21): u8? {
    if (c >= '0' && c <= '9') return @as(u8, @intCast(c - '0'));
    if (c >= 'A' && c <= 'F') return @as(u8, @intCast(c - 'A' + 10));
    if (c >= 'a' && c <= 'f') return @as(u8, @intCast(c - 'a' + 10));
    return null;
}

/// Returns true if every byte in `bytes` is 7-bit ASCII (`< 0x80`).
pub fn isAllAscii(bytes: []const u8): bool {
    let i: usize = 0;
    while (i < bytes.len) {
        if ((bytes[i] & 0x80) != 0) return false;
        i += 1;
    }
    return true;
}

/// Compares two byte slices for ASCII case-insensitive equality without allocation.
pub fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (asciiToLower(@as(u21, a[i])) != asciiToLower(@as(u21, b[i]))) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Returns true if `haystack` starts with `needle` under ASCII case-insensitive comparison.
pub fn asciiStartsWithIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len > haystack.len) return false;
    return asciiEqlIgnoreCase(haystack[0..needle.len], needle);
}

/// Returns true if `haystack` ends with `needle` under ASCII case-insensitive comparison.
pub fn asciiEndsWithIgnoreCase(haystack: []const u8, needle: []const u8): bool {
    if (needle.len > haystack.len) return false;
    const start = haystack.len - needle.len;
    return asciiEqlIgnoreCase(haystack[start..haystack.len], needle);
}

/// Zero-allocation ASCII lowercase conversion into caller-provided `dst` buffer.
/// Returns the number of bytes written (`min(src.len, dst.len)`). Non-ASCII bytes are copied verbatim.
pub fn asciiToLowerBuf(src: []const u8, dst: []u8): usize {
    const n = if (src.len < dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) {
        dst[i] = @as(u8, @intCast(asciiToLower(@as(u21, src[i]))));
        i += 1;
    }
    return n;
}

/// Zero-allocation ASCII uppercase conversion into caller-provided `dst` buffer.
/// Returns the number of bytes written (`min(src.len, dst.len)`). Non-ASCII bytes are copied verbatim.
pub fn asciiToUpperBuf(src: []const u8, dst: []u8): usize {
    const n = if (src.len < dst.len) src.len else dst.len;
    let i: usize = 0;
    while (i < n) {
        dst[i] = @as(u8, @intCast(asciiToUpper(@as(u21, src[i]))));
        i += 1;
    }
    return n;
}

/// Zero-copy trim of leading and trailing ASCII whitespace bytes.
pub fn asciiTrimWhitespace(bytes: []const u8): []const u8 {
    let start: usize = 0;
    while (start < bytes.len && isAsciiWhitespace(@as(u21, bytes[start]))) {
        start += 1;
    }
    let end: usize = bytes.len;
    while (end > start && isAsciiWhitespace(@as(u21, bytes[end - 1]))) {
        end -= 1;
    }
    return bytes[start..end];
}

/// Byte-level ASCII composition summary produced by `scanAsciiStats`.
pub struct AsciiStats {
    total: usize = 0,
    ascii: usize = 0,
    letters: usize = 0,
    digits: usize = 0,
    whitespace: usize = 0,
    control: usize = 0,
    printable: usize = 0,
    non_ascii: usize = 0,

    fn isPureAscii(): bool {
        return non_ascii == 0;
    }
}

/// Scans `bytes` in a single pass with zero allocation and returns an `AsciiStats` breakdown.
pub fn scanAsciiStats(bytes: []const u8): AsciiStats {
    let stats = AsciiStats{ total: bytes.len };
    let i: usize = 0;
    while (i < bytes.len) {
        const c = @as(u21, bytes[i]);
        if (isAsciiCodepoint(c)) {
            stats.ascii += 1;
            if (isAsciiAlphabetic(c)) stats.letters += 1;
            if (isAsciiDigit(c)) stats.digits += 1;
            if (isAsciiWhitespace(c)) stats.whitespace += 1;
            if (isAsciiControl(c)) {
                stats.control += 1;
            } else {
                stats.printable += 1;
            }
        } else {
            stats.non_ascii += 1;
        }
        i += 1;
    }
    return stats;
}
