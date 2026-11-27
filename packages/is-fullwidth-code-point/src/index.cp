// @cap/is-fullwidth-code-point — Zero-allocation Unicode fullwidth code point check in pure CP
// Clean-room MIT implementation of npm `is-fullwidth-code-point` v5 (UAX #11).

/// Returns `true` if `cp` is a Unicode fullwidth / wide (East Asian Width `F`/`W`) code point.
pub fn isFullwidthCodePoint(cp: u32): bool {
    if (cp < 0x1100) {
        return false;
    }
    if (cp <= 0x115F) { return true; } // Hangul Jamo init. consonants
    if (cp == 0x2329 || cp == 0x232A) { return true; } // Angle brackets
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
    if (cp >= 0x1B000 && cp <= 0x1B001) { return true; } // Kana Supplement
    if (cp >= 0x1F200 && cp <= 0x1F251) { return true; } // Enclosed Ideographic Supplement
    if (cp >= 0x1F300 && cp <= 0x1F9FF) { return true; } // Misc Symbols & Pictographs / Emoji
    if (cp >= 0x20000 && cp <= 0x3FFFD) { return true; } // CJK Ext B..I & Tertiary Ideographic Plane
    return false;
}

/// Returns display width (2 for fullwidth, 1 for narrow, 0 for control chars).
pub fn codePointWidth(cp: u32): usize {
    if (cp == 0 || cp < 0x20 || (cp >= 0x7F && cp < 0xA0)) {
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
        const b0 = s[i];
        if (b0 < 0x80) {
            i += 1;
            continue;
        }
        if ((b0 & 0xE0) == 0xC0 && i + 1 < s.len) {
            const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((s[i + 1] & 0x3F) as u32);
            if (isFullwidthCodePoint(cp)) { total += 1; }
            i += 2;
            continue;
        }
        if ((b0 & 0xF0) == 0xE0 && i + 2 < s.len) {
            const cp: u32 = (((b0 & 0x0F) as u32) << 12)
                | (((s[i + 1] & 0x3F) as u32) << 6)
                | ((s[i + 2] & 0x3F) as u32);
            if (isFullwidthCodePoint(cp)) { total += 1; }
            i += 3;
            continue;
        }
        if ((b0 & 0xF8) == 0xF0 && i + 3 < s.len) {
            const cp: u32 = (((b0 & 0x07) as u32) << 18)
                | (((s[i + 1] & 0x3F) as u32) << 12)
                | (((s[i + 2] & 0x3F) as u32) << 6)
                | ((s[i + 3] & 0x3F) as u32);
            if (isFullwidthCodePoint(cp)) { total += 1; }
            i += 4;
            continue;
        }
        i += 1;
    }
    return total;
}

export {
    isFullwidthCodePoint,
    codePointWidth,
    countFullwidthChars,
};
