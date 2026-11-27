// @cap/ascii — Zero-heap 16-lane SIMD ASCII validation, classification & case utilities in pure CP
// Clean-room MIT implementation of crates.io `ascii` (#844, 137.3M downloads).
// Directly re-exports 16-lane hardware SIMD primitives from `cap:ascii`.

import { isAscii, firstNonAscii, eqIgnoreCase, toLower, toUpper, trim } from "cap:ascii";

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

/// ASCII case-insensitive equality check (16-lane SIMD via `cap:ascii`).
pub fn eqIgnoreAsciiCase(a: string, b: string): bool {
    return eqIgnoreCase(a, b);
}

/// Copies `s` into `out` converting ASCII uppercase letters to lowercase; returns bytes written.
pub fn toAsciiLower(s: string, out: []u8): usize {
    return toLower(s, out);
}

/// Copies `s` into `out` converting ASCII lowercase letters to uppercase; returns bytes written.
pub fn toAsciiUpper(s: string, out: []u8): usize {
    return toUpper(s, out);
}

/// Trims leading and trailing ASCII whitespace bytes from `s` without allocating.
pub fn trimAscii(s: string): string {
    return trim(s);
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
