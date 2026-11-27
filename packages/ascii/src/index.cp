// @cap/ascii — Zero-heap 16-lane SIMD ASCII validation, classification & case utilities in pure CP
// Clean-room MIT implementation of crates.io `ascii` (#844, 137.3M downloads).
// Directly composes 16-lane hardware SIMD primitives from `cap:ascii`.

import {
    isAscii,
    firstNonAscii,
    isAlpha,
    isDigit,
    isAlnum,
    isHexDigit,
    isSpace,
    isPrintable,
    isControl,
    toLowerByte,
    toUpperByte,
    eqIgnoreCase,
    toLower,
    toUpper,
    trim,
} from "cap:ascii";

pub fn isAsciiAlpha(b: u8): bool {
    return isAlpha(b);
}

pub fn isAsciiDigit(b: u8): bool {
    return isDigit(b);
}

pub fn isAsciiAlphanumeric(b: u8): bool {
    return isAlnum(b);
}

pub fn isAsciiHexDigit(b: u8): bool {
    return isHexDigit(b);
}

pub fn isAsciiOctDigit(b: u8): bool {
    return b >= 48 && b <= 55;
}

pub fn isAsciiWhitespace(b: u8): bool {
    return isSpace(b);
}

pub fn isAsciiPrintable(b: u8): bool {
    return isPrintable(b);
}

pub fn isAsciiControl(b: u8): bool {
    return isControl(b);
}

pub fn isAsciiLowercase(b: u8): bool {
    return b >= 97 && b <= 122;
}

pub fn isAsciiUppercase(b: u8): bool {
    return b >= 65 && b <= 90;
}

/// Graphic ASCII characters: visible printable bytes excluding space (`0x21..=0x7E`).
pub fn isAsciiGraph(b: u8): bool {
    return b >= 33 && b <= 126;
}

/// ASCII punctuation characters: `! " # $ % & ' ( ) * + , - . / : ; < = > ? @ [ \ ] ^ _ ` { | } ~`.
pub fn isAsciiPunctuation(b: u8): bool {
    return (b >= 33 && b <= 47)
        || (b >= 58 && b <= 64)
        || (b >= 91 && b <= 96)
        || (b >= 123 && b <= 126);
}

pub fn toAsciiLowerByte(b: u8): u8 {
    return toLowerByte(b);
}

pub fn toAsciiUpperByte(b: u8): u8 {
    return toUpperByte(b);
}

/// Rust `ascii::AsciiChar` — validated 7-bit ASCII character (`0x00..=0x7F`).
pub struct AsciiChar {
    pub byte: u8,

    /// Validates that `b <= 0x7F` and returns an `AsciiChar`, or `null` if non-ASCII.
    pub static fn fromByte(b: u8): AsciiChar? {
        if (b > 0x7F) {
            return null;
        }
        return AsciiChar { byte: b };
    }

    pub fn asByte(): u8 {
        return _.byte;
    }

    pub fn isAlphabetic(): bool {
        return isAlpha(_.byte);
    }

    pub fn isAlphanumeric(): bool {
        return isAlnum(_.byte);
    }

    pub fn isControl(): bool {
        return isAsciiControl(_.byte);
    }

    pub fn isDigit(): bool {
        return isAsciiDigit(_.byte);
    }

    pub fn isGraph(): bool {
        return isAsciiGraph(_.byte);
    }

    pub fn isHexDigit(): bool {
        return isAsciiHexDigit(_.byte);
    }

    pub fn isLowercase(): bool {
        return isAsciiLowercase(_.byte);
    }

    pub fn isPrintable(): bool {
        return isAsciiPrintable(_.byte);
    }

    pub fn isPunctuation(): bool {
        return isAsciiPunctuation(_.byte);
    }

    pub fn isUppercase(): bool {
        return isAsciiUppercase(_.byte);
    }

    pub fn isWhitespace(): bool {
        return isSpace(_.byte);
    }

    pub fn toAsciiUppercase(): AsciiChar {
        return AsciiChar { byte: toUpperByte(_.byte) };
    }

    pub fn toAsciiLowercase(): AsciiChar {
        return AsciiChar { byte: toLowerByte(_.byte) };
    }

    pub fn eqIgnoreAsciiCase(other: AsciiChar): bool {
        return toLowerByte(_.byte) == toLowerByte(other.byte);
    }
}

/// Rust `ascii::AsciiStr` — zero-copy validated 7-bit ASCII string slice.
pub struct AsciiStr {
    pub bytes: string,

    /// Validates via 16-lane SIMD `cap:ascii.isAscii` and returns an `AsciiStr` view, or `null`.
    pub static fn fromStr(s: string): AsciiStr? {
        if (!isAscii(s)) {
            return null;
        }
        return AsciiStr { bytes: s };
    }

    pub fn len(): usize {
        return _.bytes.len;
    }

    pub fn isEmpty(): bool {
        return _.bytes.len == 0;
    }

    pub fn asStr(): string {
        return _.bytes;
    }

    pub fn get(idx: usize): AsciiChar? {
        if (idx >= _.bytes.len) {
            return null;
        }
        return AsciiChar { byte: _.bytes[idx] };
    }

    pub fn eqIgnoreAsciiCase(other: AsciiStr): bool {
        return eqIgnoreCase(_.bytes, other.bytes);
    }

    pub fn trim(): AsciiStr {
        return AsciiStr { bytes: trimAscii(_.bytes) };
    }

    pub fn trimStart(): AsciiStr {
        let start: usize = 0;
        while (start < _.bytes.len && isSpace(_.bytes[start])) : (start += 1) {}
        return AsciiStr { bytes: _.bytes[start.._.bytes.len] };
    }

    pub fn trimEnd(): AsciiStr {
        let end: usize = _.bytes.len;
        while (end > 0 && isSpace(_.bytes[end - 1])) : (end -= 1) {}
        return AsciiStr { bytes: _.bytes[0..end] };
    }

    pub fn toAsciiLowercase(out: []u8): usize {
        return toLower(_.bytes, out);
    }

    pub fn toAsciiUppercase(out: []u8): usize {
        return toUpper(_.bytes, out);
    }
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

/// Alias matching Rust `to_ascii_lowercase`.
pub fn toAsciiLowercase(s: string, out: []u8): usize {
    return toLower(s, out);
}

/// Alias matching Rust `to_ascii_uppercase`.
pub fn toAsciiUppercase(s: string, out: []u8): usize {
    return toUpper(s, out);
}

/// In-place ASCII lowercase conversion on mutable byte slice.
pub fn makeAsciiLowercase(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        buf[i] = toLowerByte(buf[i]);
    }
}

/// In-place ASCII uppercase conversion on mutable byte slice.
pub fn makeAsciiUppercase(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) : (i += 1) {
        buf[i] = toUpperByte(buf[i]);
    }
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
    isAsciiOctDigit,
    isAsciiWhitespace,
    isAsciiPrintable,
    isAsciiControl,
    isAsciiLowercase,
    isAsciiUppercase,
    isAsciiGraph,
    isAsciiPunctuation,
    toAsciiLowerByte,
    toAsciiUpperByte,
    AsciiChar,
    AsciiStr,
    eqIgnoreAsciiCase,
    toAsciiLower,
    toAsciiUpper,
    toAsciiLowercase,
    toAsciiUppercase,
    makeAsciiLowercase,
    makeAsciiUppercase,
    trimAscii,
};
