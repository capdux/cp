// @cap/unicode-ident — Zero-heap UAX #31 Unicode identifier classification
// Clean-room pure CP counterpart to crates.io:unicode-ident

pub struct Utf8Codepoint {
    cp: u32,
    len: usize,
    ok: bool,
}

fn uidInRange(cp: u32, lo: u32, hi: u32): bool {
    return cp >= lo && cp <= hi;
}

/// Strictly decode one UTF-8 codepoint starting at `s[offset]`.
/// Rejects truncated sequences, invalid continuation bytes, overlong encodings,
/// UTF-16 surrogates (0xD800..=0xDFFF), and codepoints > 0x10FFFF.
pub fn decodeUtf8At(s: string, offset: usize): Utf8Codepoint {
    if (offset >= s.len) {
        return Utf8Codepoint{ cp: 0, len: 0, ok: false };
    }
    const b0 = s[offset] as u32;
    if (b0 < 0x80) {
        return Utf8Codepoint{ cp: b0, len: 1, ok: true };
    }
    if (b0 >= 0xC2 && b0 <= 0xDF) {
        if (offset + 1 >= s.len) {
            return Utf8Codepoint{ cp: 0, len: 0, ok: false };
        }
        const b1 = s[offset + 1] as u32;
        if ((b1 & 0xC0) != 0x80) {
            return Utf8Codepoint{ cp: 0, len: 0, ok: false };
        }
        const cp = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
        return Utf8Codepoint{ cp: cp, len: 2, ok: true };
    }
    if (b0 >= 0xE0 && b0 <= 0xEF) {
        if (offset + 2 >= s.len) {
            return Utf8Codepoint{ cp: 0, len: 0, ok: false };
        }
        const b1 = s[offset + 1] as u32;
        const b2 = s[offset + 2] as u32;
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) {
            return Utf8Codepoint{ cp: 0, len: 0, ok: false };
        }
        const cp = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
        // Reject overlong (< 0x800) and UTF-16 surrogates (0xD800..=0xDFFF)
        if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return Utf8Codepoint{ cp: 0, len: 0, ok: false };
        }
        return Utf8Codepoint{ cp: cp, len: 3, ok: true };
    }
    if (b0 >= 0xF0 && b0 <= 0xF4) {
        if (offset + 3 >= s.len) {
            return Utf8Codepoint{ cp: 0, len: 0, ok: false };
        }
        const b1 = s[offset + 1] as u32;
        const b2 = s[offset + 2] as u32;
        const b3 = s[offset + 3] as u32;
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
            return Utf8Codepoint{ cp: 0, len: 0, ok: false };
        }
        const cp = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) {
            return Utf8Codepoint{ cp: 0, len: 0, ok: false };
        }
        return Utf8Codepoint{ cp: cp, len: 4, ok: true };
    }
    return Utf8Codepoint{ cp: 0, len: 0, ok: false };
}

fn uidIsXidStartNonAscii(cp: u32): bool {
    if (cp > 0x10FFFF || uidInRange(cp, 0xD800, 0xDFFF)) {
        return false;
    }

    // Latin-1 Supplement
    if (cp <= 0x00FF) {
        return cp == 0x00AA || cp == 0x00B5 || cp == 0x00BA ||
            uidInRange(cp, 0x00C0, 0x00D6) ||
            uidInRange(cp, 0x00D8, 0x00F6) ||
            uidInRange(cp, 0x00F8, 0x00FF);
    }

    // Latin Extended-A/B, IPA, Spacing Modifier Letters
    if (cp <= 0x02FF) {
        return uidInRange(cp, 0x0100, 0x02C1) ||
            uidInRange(cp, 0x02C6, 0x02D1) ||
            uidInRange(cp, 0x02E0, 0x02E4) ||
            cp == 0x02EC || cp == 0x02EE;
    }

    // Greek and Coptic
    if (cp <= 0x03FF) {
        return uidInRange(cp, 0x0370, 0x0374) ||
            uidInRange(cp, 0x0376, 0x0377) ||
            uidInRange(cp, 0x037B, 0x037D) ||
            cp == 0x037F || cp == 0x0386 ||
            uidInRange(cp, 0x0388, 0x038A) ||
            cp == 0x038C ||
            uidInRange(cp, 0x038E, 0x03A1) ||
            uidInRange(cp, 0x03A3, 0x03F5) ||
            uidInRange(cp, 0x03F7, 0x03FF);
    }

    // Cyrillic & Cyrillic Supplement
    if (cp <= 0x052F) {
        return uidInRange(cp, 0x0400, 0x0481) || uidInRange(cp, 0x048A, 0x052F);
    }

    // Armenian, Hebrew, Arabic
    if (cp <= 0x06FF) {
        return uidInRange(cp, 0x0531, 0x0556) || cp == 0x0559 ||
            uidInRange(cp, 0x0560, 0x0588) ||
            uidInRange(cp, 0x05D0, 0x05EA) ||
            uidInRange(cp, 0x05EF, 0x05F2) ||
            uidInRange(cp, 0x0620, 0x064A) ||
            uidInRange(cp, 0x066E, 0x066F) ||
            uidInRange(cp, 0x0671, 0x06D3) ||
            cp == 0x06D5 ||
            uidInRange(cp, 0x06E5, 0x06E6) ||
            uidInRange(cp, 0x06EE, 0x06EF) ||
            uidInRange(cp, 0x06FA, 0x06FC) ||
            cp == 0x06FF;
    }

    // Devanagari, Thai, Georgian, Hangul Jamo
    if (cp <= 0x11FF) {
        return uidInRange(cp, 0x0904, 0x0939) || cp == 0x093D || cp == 0x0950 ||
            uidInRange(cp, 0x0958, 0x0961) ||
            uidInRange(cp, 0x0971, 0x0980) ||
            uidInRange(cp, 0x0E01, 0x0E30) || cp == 0x0E32 ||
            uidInRange(cp, 0x0E40, 0x0E46) ||
            uidInRange(cp, 0x10A0, 0x10C5) || cp == 0x10C7 || cp == 0x10CD ||
            uidInRange(cp, 0x10D0, 0x10FA) ||
            uidInRange(cp, 0x10FC, 0x10FF) ||
            uidInRange(cp, 0x1100, 0x11FF);
    }

    // Latin Extended Additional, Greek Extended, Letterlike & Number Forms
    if (cp <= 0x218F) {
        return uidInRange(cp, 0x1E00, 0x1EFF) ||
            uidInRange(cp, 0x1F00, 0x1F15) ||
            uidInRange(cp, 0x1F18, 0x1F1D) ||
            uidInRange(cp, 0x1F20, 0x1F45) ||
            uidInRange(cp, 0x1F48, 0x1F4D) ||
            uidInRange(cp, 0x1F50, 0x1F57) ||
            cp == 0x1F59 || cp == 0x1F5B || cp == 0x1F5D ||
            uidInRange(cp, 0x1F5F, 0x1F7D) ||
            uidInRange(cp, 0x1F80, 0x1FB4) ||
            uidInRange(cp, 0x1FB6, 0x1FBC) || cp == 0x1FBE ||
            uidInRange(cp, 0x1FC2, 0x1FC4) ||
            uidInRange(cp, 0x1FC6, 0x1FCC) ||
            uidInRange(cp, 0x1FD0, 0x1FD3) ||
            uidInRange(cp, 0x1FD6, 0x1FDB) ||
            uidInRange(cp, 0x1FE0, 0x1FEC) ||
            uidInRange(cp, 0x1FF2, 0x1FF4) ||
            uidInRange(cp, 0x1FF6, 0x1FFC) ||
            cp == 0x2102 || cp == 0x2107 ||
            uidInRange(cp, 0x210A, 0x2113) || cp == 0x2115 ||
            uidInRange(cp, 0x2118, 0x211D) ||
            cp == 0x2124 || cp == 0x2126 || cp == 0x2128 ||
            uidInRange(cp, 0x212A, 0x2139) ||
            uidInRange(cp, 0x213C, 0x213F) ||
            uidInRange(cp, 0x2145, 0x2149) || cp == 0x214E ||
            uidInRange(cp, 0x2160, 0x2188);
    }

    // Hiragana, Katakana, Bopomofo, Hangul Compat Jamo
    if (cp <= 0x31FF) {
        return uidInRange(cp, 0x3005, 0x3007) ||
            uidInRange(cp, 0x3021, 0x3029) ||
            uidInRange(cp, 0x3031, 0x3035) ||
            uidInRange(cp, 0x3038, 0x303C) ||
            uidInRange(cp, 0x3041, 0x3096) ||
            uidInRange(cp, 0x309D, 0x309F) ||
            uidInRange(cp, 0x30A1, 0x30FA) ||
            uidInRange(cp, 0x30FC, 0x30FF) ||
            uidInRange(cp, 0x3105, 0x312F) ||
            uidInRange(cp, 0x3131, 0x318E) ||
            uidInRange(cp, 0x31A0, 0x31BF) ||
            uidInRange(cp, 0x31F0, 0x31FF);
    }

    // CJK Unified Ideographs Ext-A, CJK Unified Ideographs, Hangul Syllables
    if (uidInRange(cp, 0x3400, 0x4DBF) ||
        uidInRange(cp, 0x4E00, 0x9FFF) ||
        uidInRange(cp, 0xAC00, 0xD7A3)) {
        return true;
    }

    // CJK Compatibility Ideographs & Halfwidth/Fullwidth Forms
    if (uidInRange(cp, 0xF900, 0xFA6D) ||
        uidInRange(cp, 0xFA70, 0xFAD9) ||
        uidInRange(cp, 0xFF21, 0xFF3A) ||
        uidInRange(cp, 0xFF41, 0xFF5A) ||
        uidInRange(cp, 0xFF66, 0xFFBE) ||
        uidInRange(cp, 0xFFC2, 0xFFC7) ||
        uidInRange(cp, 0xFFCA, 0xFFCF) ||
        uidInRange(cp, 0xFFD2, 0xFFD7) ||
        uidInRange(cp, 0xFFDA, 0xFFDC)) {
        return true;
    }

    // Supplementary Plane: Math Alphanumeric letters & CJK Extensions B..G
    if (uidInRange(cp, 0x1D400, 0x1D454) ||
        uidInRange(cp, 0x1D456, 0x1D49C) ||
        uidInRange(cp, 0x20000, 0x2A6DF) ||
        uidInRange(cp, 0x2A700, 0x2B739) ||
        uidInRange(cp, 0x2B740, 0x2B81D) ||
        uidInRange(cp, 0x2B820, 0x2CEA1) ||
        uidInRange(cp, 0x2CEB0, 0x2EBE0) ||
        uidInRange(cp, 0x30000, 0x3134A)) {
        return true;
    }

    return false;
}

fn uidIsXidContinueExtra(cp: u32): bool {
    if (cp == 0x00B7 || cp == 0x0387) {
        return true;
    }
    // Combining Diacritical Marks & script combining marks / digits
    if (uidInRange(cp, 0x0300, 0x036F) ||
        uidInRange(cp, 0x0483, 0x0487) ||
        uidInRange(cp, 0x0591, 0x05BD) || cp == 0x05BF ||
        uidInRange(cp, 0x05C1, 0x05C2) ||
        uidInRange(cp, 0x05C4, 0x05C5) || cp == 0x05C7 ||
        uidInRange(cp, 0x0610, 0x061A) ||
        uidInRange(cp, 0x064B, 0x0669) || cp == 0x0670 ||
        uidInRange(cp, 0x06D6, 0x06DC) ||
        uidInRange(cp, 0x06DF, 0x06E4) ||
        uidInRange(cp, 0x06E7, 0x06E8) ||
        uidInRange(cp, 0x06EA, 0x06ED) ||
        uidInRange(cp, 0x06F0, 0x06F9)) {
        return true;
    }
    // Devanagari & Thai combining / digits
    if (uidInRange(cp, 0x0900, 0x0903) ||
        uidInRange(cp, 0x093A, 0x093C) ||
        uidInRange(cp, 0x093E, 0x094F) ||
        uidInRange(cp, 0x0951, 0x0957) ||
        uidInRange(cp, 0x0962, 0x0963) ||
        uidInRange(cp, 0x0966, 0x096F) ||
        cp == 0x0E31 ||
        uidInRange(cp, 0x0E34, 0x0E3A) ||
        uidInRange(cp, 0x0E47, 0x0E4E) ||
        uidInRange(cp, 0x0E50, 0x0E59)) {
        return true;
    }
    // Connector punctuation, combining supplements, CJK voiced marks, fullwidth digits/underscore
    if (uidInRange(cp, 0x1369, 0x1371) ||
        uidInRange(cp, 0x1AB0, 0x1ABD) ||
        uidInRange(cp, 0x1DC0, 0x1DFF) ||
        uidInRange(cp, 0x203F, 0x2040) || cp == 0x2054 ||
        uidInRange(cp, 0x20D0, 0x20DC) || cp == 0x20E1 ||
        uidInRange(cp, 0x20E5, 0x20F0) ||
        uidInRange(cp, 0x302A, 0x302F) ||
        uidInRange(cp, 0x3099, 0x309A) ||
        uidInRange(cp, 0xFE00, 0xFE0F) ||
        uidInRange(cp, 0xFE20, 0xFE2F) ||
        uidInRange(cp, 0xFE33, 0xFE34) ||
        uidInRange(cp, 0xFE4D, 0xFE4F) ||
        uidInRange(cp, 0xFF10, 0xFF19) ||
        cp == 0xFF3F ||
        uidInRange(cp, 0x1D7CE, 0x1D7FF) ||
        uidInRange(cp, 0xE0100, 0xE01EF)) {
        return true;
    }
    return false;
}

/// Returns true if `cp` has the Unicode UAX #31 `XID_Start` property.
pub fn isXidStart(cp: u32): bool {
    if (cp < 0x80) {
        return (cp >= 0x41 && cp <= 0x5A) || (cp >= 0x61 && cp <= 0x7A);
    }
    return uidIsXidStartNonAscii(cp);
}

/// Returns true if `cp` has the Unicode UAX #31 `XID_Continue` property.
pub fn isXidContinue(cp: u32): bool {
    if (cp < 0x80) {
        return (cp >= 0x30 && cp <= 0x39) ||
            (cp >= 0x41 && cp <= 0x5A) ||
            (cp >= 0x61 && cp <= 0x7A) ||
            cp == 0x5F;
    }
    return uidIsXidStartNonAscii(cp) || uidIsXidContinueExtra(cp);
}

/// Returns true if `cp` may start a Rust/CP-style identifier (`_` or `XID_Start`).
pub fn isIdentStart(cp: u32): bool {
    return cp == 0x5F || isXidStart(cp);
}

/// Scan the longest prefix of `s` that is a valid UAX #31 identifier (`Ident_Start` + `XID_Continue*`).
/// Returns the byte length of that prefix (0 if none or invalid UTF-8 at start).
pub fn scanIdentLen(s: string): usize {
    if (s.len == 0) {
        return 0;
    }
    const first = decodeUtf8At(s, 0);
    if (!first.ok || !isIdentStart(first.cp)) {
        return 0;
    }
    let pos = first.len;
    while (pos < s.len) {
        const next = decodeUtf8At(s, pos);
        if (!next.ok || !isXidContinue(next.cp)) {
            break;
        }
        pos += next.len;
    }
    return pos;
}

/// Returns true if the entire string `s` is a valid non-empty UAX #31 identifier.
pub fn isValidIdent(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    return scanIdentLen(s) == s.len;
}
