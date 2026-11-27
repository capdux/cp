// Source: cp/lib/ascii/index.cp (Unified Public Exports for cap:ascii)
// Zero-allocation, high-performance ASCII & byte-string foundation for CP stdlib and ecosystem packages.

const LASC_HEX_LOWER: [16]u8 = [
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
];

const LASC_HEX_UPPER: [16]u8 = [
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
];

fn lascMinUsize(a: usize, b: usize): usize {
    return if (a < b) a else b;
}

/// Returns true if every byte in `data` is a 7-bit ASCII byte (`0x00..=0x7F`).
pub fn isAscii(data: []const u8): bool {
    let i: usize = 0;
    while (i + 8 <= data.len) : (i += 8) {
        const acc: u8 = data[i] | data[i + 1] | data[i + 2] | data[i + 3] | data[i + 4] | data[i + 5] | data[i + 6] | data[i + 7];
        if ((acc & 0x80) != 0) return false;
    }
    while (i < data.len) : (i += 1) {
        if ((data[i] & 0x80) != 0) return false;
    }
    return true;
}

/// Returns the index of the first non-ASCII byte (`>= 0x80`), or `null` if all bytes are ASCII.
pub fn firstNonAscii(data: []const u8): usize? {
    let i: usize = 0;
    while (i + 8 <= data.len) : (i += 8) {
        const acc: u8 = data[i] | data[i + 1] | data[i + 2] | data[i + 3] | data[i + 4] | data[i + 5] | data[i + 6] | data[i + 7];
        if ((acc & 0x80) != 0) {
            let j: usize = i;
            while (j < i + 8) : (j += 1) {
                if ((data[j] & 0x80) != 0) return j;
            }
        }
    }
    while (i < data.len) : (i += 1) {
        if ((data[i] & 0x80) != 0) return i;
    }
    return null;
}

/// Returns true if `b` is an ASCII whitespace character (`' '`, `'\t'`, `'\n'`, `'\r'`, `0x0B`, `0x0C`).
pub fn isSpace(b: u8): bool {
    return b == ' ' || (b >= 0x09 && b <= 0x0D);
}

/// Returns true if `b` is an ASCII alphabetic letter (`'A'..='Z'` or `'a'..='z'`).
pub fn isAlpha(b: u8): bool {
    const lo: u8 = b | 0x20;
    return lo >= 'a' && lo <= 'z';
}

/// Returns true if `b` is an ASCII decimal digit (`'0'..='9'`).
pub fn isDigit(b: u8): bool {
    return b >= '0' && b <= '9';
}

/// Returns true if `b` is an ASCII alphanumeric character.
pub fn isAlnum(b: u8): bool {
    return isAlpha(b) || isDigit(b);
}

/// Returns true if `b` is an ASCII hexadecimal digit (`'0'..='9'`, `'a'..='f'`, `'A'..='F'`).
pub fn isHexDigit(b: u8): bool {
    const lo: u8 = b | 0x20;
    return (b >= '0' && b <= '9') || (lo >= 'a' && lo <= 'f');
}

/// Returns true if `b` is a printable ASCII character (`0x20..=0x7E`).
pub fn isPrintable(b: u8): bool {
    return b >= 0x20 && b <= 0x7E;
}

/// Returns true if `b` is an ASCII control character (`0x00..=0x1F` or `0x7F`).
pub fn isControl(b: u8): bool {
    return b <= 0x1F || b == 0x7F;
}

/// Converts an ASCII hex digit byte to its numeric value `0..=15`, or `null` if invalid.
pub fn hexVal(b: u8): u8? {
    if (b >= '0' && b <= '9') return b - '0';
    const lo: u8 = b | 0x20;
    if (lo >= 'a' && lo <= 'f') return (lo - 'a') + 10;
    return null;
}

/// Converts the low 4 bits of `n` to a lowercase ASCII hex character (`'0'..='9'`, `'a'..='f'`).
pub fn hexCharLower(n: u8): u8 {
    return LASC_HEX_LOWER[usize(n & 0x0F)];
}

/// Converts the low 4 bits of `n` to an uppercase ASCII hex character (`'0'..='9'`, `'A'..='F'`).
pub fn hexCharUpper(n: u8): u8 {
    return LASC_HEX_UPPER[usize(n & 0x0F)];
}

/// Converts an uppercase ASCII letter to lowercase; leaves all other bytes unchanged.
pub fn toLowerByte(b: u8): u8 {
    if (b >= 'A' && b <= 'Z') return b | 0x20;
    return b;
}

/// Converts a lowercase ASCII letter to uppercase; leaves all other bytes unchanged.
pub fn toUpperByte(b: u8): u8 {
    if (b >= 'a' && b <= 'z') return b & 0xDF;
    return b;
}

/// Returns true if two byte strings have identical length and contents.
pub fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Returns true if two byte strings are equal ignoring ASCII letter case.
pub fn eqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (toLowerByte(a[i]) != toLowerByte(b[i])) return false;
    }
    return true;
}

/// Returns true if `s` starts with `prefix`.
pub fn startsWith(s: string, prefix: string): bool {
    if (prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
}

/// Returns true if `s` ends with `suffix`.
pub fn endsWith(s: string, suffix: string): bool {
    if (suffix.len > s.len) return false;
    const base: usize = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) : (i += 1) {
        if (s[base + i] != suffix[i]) return false;
    }
    return true;
}

/// Finds the first index `>= start_pos` where `s[i] == ch`, or `null` if not found.
pub fn findByte(s: string, ch: u8, start_pos: usize): usize? {
    if (start_pos >= s.len) return null;
    let i: usize = start_pos;
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) return i;
    }
    return null;
}

/// Finds the last index where `s[i] == ch`, or `null` if not found.
pub fn rfindByte(s: string, ch: u8): usize? {
    let i: usize = s.len;
    while (i > 0) {
        i -= 1;
        if (s[i] == ch) return i;
    }
    return null;
}

/// Returns a slice of `s` with leading ASCII whitespace removed.
pub fn trimStart(s: string): string {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) : (start += 1) {}
    return s[start..s.len];
}

/// Returns a slice of `s` with trailing ASCII whitespace removed.
pub fn trimEnd(s: string): string {
    let end: usize = s.len;
    while (end > 0 && isSpace(s[end - 1])) : (end -= 1) {}
    return s[0..end];
}

/// Returns a slice of `s` with both leading and trailing ASCII whitespace removed.
pub fn trim(s: string): string {
    return trimEnd(trimStart(s));
}

/// Writes the ASCII-lowercased bytes of `s` into `out` and returns the number of bytes written.
pub fn toLower(s: string, out: []u8): usize {
    const n: usize = lascMinUsize(s.len, out.len);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[i] = toLowerByte(s[i]);
    }
    return n;
}

/// Writes the ASCII-uppercased bytes of `s` into `out` and returns the number of bytes written.
pub fn toUpper(s: string, out: []u8): usize {
    const n: usize = lascMinUsize(s.len, out.len);
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[i] = toUpperByte(s[i]);
    }
    return n;
}
