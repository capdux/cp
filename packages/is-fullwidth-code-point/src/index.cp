// @cap/is-fullwidth-code-point — Zero-allocation Unicode fullwidth code point & display width in pure CP
// Clean-room MIT implementation of npm `is-fullwidth-code-point` v5 (UAX #11 East Asian Width).

import { decodeCodepoint } from "cap:utf8";

/// Returns `true` if `cp` is a Unicode fullwidth / wide (East Asian Width `F`/`W`) code point.
pub fn isFullwidthCodePoint(cp: u32): bool {
    if (cp < 0x1100) {
        return false;
    }
    if (cp <= 0x115F) { return true; } // Hangul Jamo init. consonants
    if (cp == 0x231A || cp == 0x231B) { return true; } // Watch, Hourglass
    if (cp == 0x2329 || cp == 0x232A) { return true; } // Angle brackets
    if ((cp >= 0x23E9 && cp <= 0x23EC) || cp == 0x23F0 || cp == 0x23F3) { return true; }
    if (cp == 0x25FD || cp == 0x25FE) { return true; }
    if (cp == 0x2614 || cp == 0x2615) { return true; } // Umbrella, Hot Beverage
    if (cp >= 0x2648 && cp <= 0x2653) { return true; } // Zodiac
    if (cp == 0x267F || cp == 0x2693 || cp == 0x26A1 || cp == 0x26AA || cp == 0x26AB) { return true; }
    if (cp == 0x26BD || cp == 0x26BE || cp == 0x26C4 || cp == 0x26C5 || cp == 0x26CE || cp == 0x26D4 || cp == 0x26EA) { return true; }
    if (cp == 0x26F2 || cp == 0x26F3 || cp == 0x26F5 || cp == 0x26FA || cp == 0x26FD) { return true; }
    if (cp == 0x2705 || cp == 0x270A || cp == 0x270B || cp == 0x2728 || cp == 0x274C || cp == 0x274E) { return true; }
    if ((cp >= 0x2753 && cp <= 0x2755) || cp == 0x2757 || (cp >= 0x2795 && cp <= 0x2797) || cp == 0x27B0 || cp == 0x27BF) { return true; }
    if (cp == 0x2B1B || cp == 0x2B1C || cp == 0x2B50 || cp == 0x2B55) { return true; }
    if (cp >= 0x2E80 && cp <= 0x3247 && cp != 0x303F) { return true; } // CJK Radicals .. Enclosed CJK
    if (cp >= 0x3250 && cp <= 0x4DBF) { return true; } // Enclosed CJK .. CJK Ext A
    if (cp >= 0x4E00 && cp <= 0xA4C6) { return true; } // CJK Unified Ideographs .. Yi Radicals
    if (cp >= 0xA960 && cp <= 0xA97C) { return true; } // Hangul Jamo Extended-A
    if (cp >= 0xAC00 && cp <= 0xD7A3) { return true; } // Hangul Syllables
    if (cp >= 0xF900 && cp <= 0xFAFF) { return true; } // CJK Compatibility Ideographs
    if (cp >= 0xFE10 && cp <= 0xFE19) { return true; } // Vertical Forms
    if (cp >= 0xFE30 && cp <= 0xFE6B) { return true; } // CJK Compatibility Forms .. Small Form Variants
    if (cp >= 0xFF01 && cp <= 0xFF60) { return true; } // Fullwidth Forms
    if (cp >= 0xFFE0 && cp <= 0xFFE6) { return true; } // Fullwidth Symbol Variants
    if (cp >= 0x16FE0 && cp <= 0x16FE4) { return true; } // Ideographic Symbols and Punctuation
    if (cp >= 0x16FF0 && cp <= 0x16FF1) { return true; }
    if (cp >= 0x17000 && cp <= 0x187F7) { return true; } // Tangut
    if (cp >= 0x18800 && cp <= 0x18CD5) { return true; } // Tangut Components
    if (cp >= 0x18D00 && cp <= 0x18D08) { return true; } // Tangut Supplement
    if (cp >= 0x1AFF0 && cp <= 0x1AFFE && cp != 0x1AFF4 && cp != 0x1AFFC) { return true; } // Kana Extended-B
    if (cp >= 0x1B000 && cp <= 0x1B122) { return true; } // Kana Supplement .. Kana Extended-A
    if (cp == 0x1B132 || (cp >= 0x1B150 && cp <= 0x1B152) || cp == 0x1B155 || (cp >= 0x1B164 && cp <= 0x1B167)) { return true; }
    if (cp >= 0x1B170 && cp <= 0x1B2FB) { return true; } // Nushu
    if (cp == 0x1F004 || cp == 0x1F0CF || cp == 0x1F18E || (cp >= 0x1F191 && cp <= 0x1F19A)) { return true; }
    if (cp >= 0x1F200 && cp <= 0x1F251) { return true; } // Enclosed Ideographic Supplement
    if (cp >= 0x1F260 && cp <= 0x1F265) { return true; }
    if (cp >= 0x1F300 && cp <= 0x1F9FF) { return true; } // Misc Symbols & Pictographs / Emoji
    if (cp >= 0x1FA70 && cp <= 0x1FAFF) { return true; } // Symbols and Pictographs Extended-A
    if (cp >= 0x20000 && cp <= 0x3FFFD) { return true; } // CJK Ext B..I & Tertiary Ideographic Plane
    return false;
}

/// Returns `true` if `cp` occupies 0 terminal columns (control chars, combining marks, ZWJ/ZWS, variation selectors).
pub fn isZeroWidthCodePoint(cp: u32): bool {
    if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0)) { return true; }
    if (cp >= 0x0300 && cp <= 0x036F) { return true; } // Combining Diacritical Marks
    if (cp >= 0x1AB0 && cp <= 0x1AFF) { return true; } // Combining Diacritical Marks Extended
    if (cp >= 0x1DC0 && cp <= 0x1DFF) { return true; } // Combining Diacritical Marks Supplement
    if (cp >= 0x200B && cp <= 0x200F) { return true; } // Zero-width space / ZWNJ / ZWJ / marks
    if (cp >= 0x20D0 && cp <= 0x20FF) { return true; } // Combining Diacritical Marks for Symbols
    if (cp >= 0xFE00 && cp <= 0xFE0F) { return true; } // Variation Selectors
    if (cp >= 0xFE20 && cp <= 0xFE2F) { return true; } // Combining Half Marks
    if (cp == 0xFEFF) { return true; } // BOM / ZWNBSP
    return false;
}

/// Returns display width (2 for fullwidth, 1 for narrow, 0 for control/combining chars).
pub fn codePointWidth(cp: u32): usize {
    if (isZeroWidthCodePoint(cp)) {
        return 0;
    }
    if (isFullwidthCodePoint(cp)) {
        return 2;
    }
    return 1;
}

/// Counts how many fullwidth Unicode code points appear in a UTF-8 string.
pub fn countFullwidthChars(s: string): usize {
    let i: usize = 0;
    let total: usize = 0;
    while (i < s.len) {
        if (s[i] < 0x80) {
            i += 1;
            continue;
        }
        const d = decodeCodepoint(s, i);
        if (d.valid && isFullwidthCodePoint(d.codepoint)) {
            total += 1;
        }
        i += if (d.len > 0) d.len else 1;
    }
    return total;
}

/// Fast predicate returning `true` if `s` contains at least one fullwidth/wide code point.
pub fn hasFullwidthChar(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] < 0x80) {
            i += 1;
            continue;
        }
        const d = decodeCodepoint(s, i);
        if (d.valid && isFullwidthCodePoint(d.codepoint)) {
            return true;
        }
        i += if (d.len > 0) d.len else 1;
    }
    return false;
}

/// Computes the total terminal display width (in columns) of a UTF-8 string.
/// ASCII printable = 1, East Asian Wide/Fullwidth/Emoji = 2, control & combining marks = 0.
pub fn stringDisplayWidth(s: string): usize {
    let i: usize = 0;
    let width: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < 0x80) {
            if (b >= 0x20 && b != 0x7F) {
                width += 1;
            }
            i += 1;
            continue;
        }
        const d = decodeCodepoint(s, i);
        if (d.valid) {
            width += codePointWidth(d.codepoint);
        }
        i += if (d.len > 0) d.len else 1;
    }
    return width;
}

/// Returns the longest valid UTF-8 prefix slice of `s` whose terminal display width is `<= max_width`.
pub fn truncateToDisplayWidth(s: string, max_width: usize): string {
    let i: usize = 0;
    let width: usize = 0;
    while (i < s.len) {
        const b = s[i];
        if (b < 0x80) {
            const cw: usize = if (b >= 0x20 && b != 0x7F) 1 else 0;
            if (width + cw > max_width) {
                return s[0..i];
            }
            width += cw;
            i += 1;
            continue;
        }
        const d = decodeCodepoint(s, i);
        const step: usize = if (d.len > 0) d.len else 1;
        const cw: usize = if (d.valid) codePointWidth(d.codepoint) else 0;
        if (width + cw > max_width) {
            return s[0..i];
        }
        width += cw;
        i += step;
    }
    return s;
}

export {
    isFullwidthCodePoint,
    isZeroWidthCodePoint,
    codePointWidth,
    countFullwidthChars,
    hasFullwidthChar,
    stringDisplayWidth,
    truncateToDisplayWidth,
};
