// @cap/ascii — Zero-heap ASCII validation, classification & case utilities in pure CP
// Clean-room MIT implementation of crates.io `ascii` (#844, 137.3M downloads).

/// Returns `true` if every byte in `s` is 7-bit ASCII (`<= 0x7F`).
pub fn isAscii(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] > 0x7F) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Returns the first non-ASCII byte offset in `s`, or `null` if all bytes are ASCII.
pub fn firstNonAscii(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] > 0x7F) {
            return i;
        }
        i += 1;
    }
    return null;
}

pub fn isAsciiAlpha(b: u8): bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122);
}

pub fn isAsciiDigit(b: u8): bool {
    return b >= 48 && b <= 57;
}

pub fn isAsciiAlphanumeric(b: u8): bool {
    return isAsciiAlpha(b) || isAsciiDigit(b);
}

pub fn isAsciiHexDigit(b: u8): bool {
    return isAsciiDigit(b) || (b >= 65 && b <= 70) || (b >= 97 && b <= 102);
}

pub fn isAsciiWhitespace(b: u8): bool {
    return b == 32 || b == 9 || b == 10 || b == 12 || b == 13;
}

pub fn isAsciiPrintable(b: u8): bool {
    return b >= 32 && b <= 126;
}

pub fn isAsciiControl(b: u8): bool {
    return b < 32 || b == 127;
}

pub fn toAsciiLowerByte(b: u8): u8 {
    if (b >= 65 && b <= 90) { return b + 32; }
    return b;
}

pub fn toAsciiUpperByte(b: u8): u8 {
    if (b >= 97 && b <= 122) { return b - 32; }
    return b;
}

/// ASCII case-insensitive equality check.
pub fn eqIgnoreAsciiCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toAsciiLowerByte(a[i]) != toAsciiLowerByte(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Copies `s` into `out` converting ASCII uppercase letters to lowercase; returns bytes written.
pub fn toAsciiLower(s: string, out: []u8): usize {
    let n = s.len;
    if (n > out.len) { n = out.len; }
    let i: usize = 0;
    while (i < n) {
        out[i] = toAsciiLowerByte(s[i]);
        i += 1;
    }
    return n;
}

/// Copies `s` into `out` converting ASCII lowercase letters to uppercase; returns bytes written.
pub fn toAsciiUpper(s: string, out: []u8): usize {
    let n = s.len;
    if (n > out.len) { n = out.len; }
    let i: usize = 0;
    while (i < n) {
        out[i] = toAsciiUpperByte(s[i]);
        i += 1;
    }
    return n;
}

/// Trims leading and trailing ASCII whitespace bytes from `s` without allocating.
pub fn trimAscii(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isAsciiWhitespace(s[start])) {
        start += 1;
    }
    while (end > start && isAsciiWhitespace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

export {
    isAscii,
    firstNonAscii,
    isAsciiAlpha,
    isAsciiDigit,
    isAsciiAlphanumeric,
    isAsciiHexDigit,
    isAsciiWhitespace,
    isAsciiPrintable,
    isAsciiControl,
    toAsciiLowerByte,
    toAsciiUpperByte,
    eqIgnoreAsciiCase,
    toAsciiLower,
    toAsciiUpper,
    trimAscii,
};
