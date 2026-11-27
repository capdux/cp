// @cap/is-potential-custom-element-name — Zero-allocation WHATWG Custom Element Name validator in pure CP
// Clean-room MIT implementation of npm `is-potential-custom-element-name` (#407, 132M/wk).
// Reuses 16-lane SIMD `cap:ascii.strEq` and strict RFC 3629 `cap:utf8.decodeCodepoint`.

import { strEq, toLowerByte } from "cap:ascii";
import { decodeCodepoint } from "cap:utf8";

pub const RESERVED_CUSTOM_ELEMENT_NAMES_COUNT: usize = 8;

/// Detailed validation failure reason for WHATWG `PotentialCustomElementName` (HTML §4.13.2).
pub enum PcenError {
    Ok,
    Empty,
    TooShort,
    InvalidStartChar,
    MissingHyphen,
    ReservedName,
    InvalidUtf8,
    InvalidChar,
}

/// Structured validation result with exact byte offset of the offending sequence.
pub struct PcenValidationResult {
    pub valid: bool,
    pub err: PcenError,
    pub error_offset: usize,
}

/// Returns `true` if `b` is a valid leading character (`[a-z]`) for a custom element name.
pub fn isValidCustomElementStart(b: u8): bool {
    return b >= 0x61 && b <= 0x7A;
}

/// Returns `true` if `cp` is a valid WHATWG `PCENChar` (HTML Standard §4.13.2).
pub fn isPcenChar(cp: u32): bool {
    if (cp == 0x2D || cp == 0x2E || cp == 0x5F) { return true; } // '-', '.', '_'
    if (cp >= 0x30 && cp <= 0x39) { return true; } // '0'-'9'
    if (cp >= 0x61 && cp <= 0x7A) { return true; } // 'a'-'z'
    if (cp == 0xB7) { return true; }
    if (cp >= 0xC0 && cp <= 0xD6) { return true; }
    if (cp >= 0xD8 && cp <= 0xF6) { return true; }
    if (cp >= 0xF8 && cp <= 0x37D) { return true; }
    if (cp >= 0x37F && cp <= 0x1FFF) { return true; }
    if (cp == 0x200C || cp == 0x200D || cp == 0x203F || cp == 0x2040) { return true; }
    if (cp >= 0x2070 && cp <= 0x218F) { return true; }
    if (cp >= 0x2C00 && cp <= 0x2FEF) { return true; }
    if (cp >= 0x3001 && cp <= 0xD7FF) { return true; }
    if (cp >= 0xF900 && cp <= 0xFDCF) { return true; }
    if (cp >= 0xFDF0 && cp <= 0xFFFD) { return true; }
    if (cp >= 0x10000 && cp <= 0xEFFFF) { return true; }
    return false;
}

/// Enumerates the 8 WHATWG reserved hyphenated SVG/MathML element names by index (`0..7`).
pub fn reservedCustomElementNameAt(idx: usize): string? {
    if (idx == 0) { return "annotation-xml"; }
    if (idx == 1) { return "color-profile"; }
    if (idx == 2) { return "font-face"; }
    if (idx == 3) { return "font-face-src"; }
    if (idx == 4) { return "font-face-uri"; }
    if (idx == 5) { return "font-face-format"; }
    if (idx == 6) { return "font-face-name"; }
    if (idx == 7) { return "missing-glyph"; }
    return null;
}

/// Returns `true` if `name` is one of the 8 reserved hyphenated SVG/MathML element names.
pub fn isReservedCustomElementName(name: string): bool {
    return strEq(name, "annotation-xml")
        || strEq(name, "color-profile")
        || strEq(name, "font-face")
        || strEq(name, "font-face-src")
        || strEq(name, "font-face-uri")
        || strEq(name, "font-face-format")
        || strEq(name, "font-face-name")
        || strEq(name, "missing-glyph");
}

/// Performs full WHATWG HTML §4.13.2 validation and returns a structured `PcenValidationResult`
/// with the exact error category and byte offset.
pub fn validateCustomElementName(name: string): PcenValidationResult {
    if (name.len == 0) {
        return PcenValidationResult { valid: false, err: PcenError.Empty, error_offset: 0 };
    }
    if (!isValidCustomElementStart(name[0])) {
        return PcenValidationResult { valid: false, err: PcenError.InvalidStartChar, error_offset: 0 };
    }
    if (name.len < 2) {
        return PcenValidationResult { valid: false, err: PcenError.TooShort, error_offset: 1 };
    }
    if (isReservedCustomElementName(name)) {
        return PcenValidationResult { valid: false, err: PcenError.ReservedName, error_offset: 0 };
    }

    let has_hyphen: bool = false;
    let i: usize = 1;
    while (i < name.len) {
        const d = decodeCodepoint(name, i);
        if (!d.valid || d.len == 0) {
            return PcenValidationResult { valid: false, err: PcenError.InvalidUtf8, error_offset: i };
        }
        if (d.codepoint == 0x2D) {
            has_hyphen = true;
        }
        if (!isPcenChar(d.codepoint)) {
            return PcenValidationResult { valid: false, err: PcenError.InvalidChar, error_offset: i };
        }
        i += d.len;
    }

    if (!has_hyphen) {
        return PcenValidationResult { valid: false, err: PcenError.MissingHyphen, error_offset: name.len };
    }
    return PcenValidationResult { valid: true, err: PcenError.Ok, error_offset: 0 };
}

/// Validates whether `name` is a legal WHATWG `PotentialCustomElementName` (HTML §4.13.2).
pub fn isPotentialCustomElementName(name: string): bool {
    return validateCustomElementName(name).valid;
}

/// Sanitizes an arbitrary string into a valid WHATWG custom element name written into `out`.
/// Lowercases ASCII letters, maps invalid characters to `-`, ensures a leading `[a-z]` (prefixing `x-`),
/// ensures at least one `-`, and appends `-el` if the result collides with a reserved name.
pub fn sanitizeCustomElementName(input: string, out: []u8): usize {
    if (out.len < 2) {
        return 0;
    }
    let pos: usize = 0;
    let has_hyphen: bool = false;

    // If empty or doesn't start with ASCII alpha, prefix "x-"
    const need_prefix = (input.len == 0) || !isValidCustomElementStart(toLowerByte(input[0]));
    if (need_prefix) {
        out[0] = 'x';
        out[1] = '-';
        pos = 2;
        has_hyphen = true;
    }

    let i: usize = 0;
    while (i < input.len && pos < out.len) {
        const b = input[i];
        if (b < 0x80) {
            const lb = toLowerByte(b);
            if (pos == 0) {
                out[pos] = lb;
                pos += 1;
            } else if (isPcenChar(lb as u32)) {
                out[pos] = lb;
                if (lb == '-') {
                    has_hyphen = true;
                }
                pos += 1;
            } else if (out[pos - 1] != '-') {
                out[pos] = '-';
                has_hyphen = true;
                pos += 1;
            }
            i += 1;
            continue;
        }
        const d = decodeCodepoint(input, i);
        if (d.valid && d.len > 0 && isPcenChar(d.codepoint) && pos + d.len <= out.len) {
            let k: usize = 0;
            while (k < d.len) : (k += 1) {
                out[pos] = input[i + k];
                pos += 1;
            }
        } else if (pos > 0 && out[pos - 1] != '-' && pos < out.len) {
            out[pos] = '-';
            has_hyphen = true;
            pos += 1;
        }
        i += if (d.len > 0) d.len else 1;
    }

    if (!has_hyphen && pos < out.len) {
        out[pos] = '-';
        pos += 1;
    }

    if (isReservedCustomElementName(out[0..pos] as string)) {
        if (pos + 3 <= out.len) {
            out[pos] = '-';
            out[pos + 1] = 'e';
            out[pos + 2] = 'l';
            pos += 3;
        }
    }

    return pos;
}

export {
    RESERVED_CUSTOM_ELEMENT_NAMES_COUNT,
    PcenError,
    PcenValidationResult,
    isValidCustomElementStart,
    isPcenChar,
    reservedCustomElementNameAt,
    isReservedCustomElementName,
    validateCustomElementName,
    isPotentialCustomElementName,
    sanitizeCustomElementName,
};
