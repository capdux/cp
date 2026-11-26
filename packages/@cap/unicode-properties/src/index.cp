// packages/@cap/unicode-properties/src/index.cp
// Zero-allocation Unicode character properties, UAX #31 identifier (XID_Start/XID_Continue),
// White_Space, Default_Ignorable, and UTS #51 Emoji classification in pure CP.
// Clean-room MIT implementation.

/// Strictly decode the next UTF-8 codepoint from `s` starting at `pos`.
/// Writes the next byte offset into `next_pos.*`.
/// Returns `0xFFFFFFFF` on malformed UTF-8 (overlong, truncated, invalid continuation,
/// surrogate U+D800..U+DFFF, or codepoint > U+10FFFF).
fn decodeUtf8Strict(s: string, pos: usize, next_pos: *usize): u32 {
    if (pos >= s.len) {
        next_pos.* = pos;
        return 0xFFFFFFFF as u32;
    }
    const b0 = s[pos];
    // 1-byte ASCII: 0x00..0x7F
    if (b0 < (0x80 as u8)) {
        next_pos.* = pos + 1;
        return b0 as u32;
    }
    // 2-byte sequence: 0xC2..0xDF (0xC0..0xC1 are overlong)
    if (b0 >= (0xC2 as u8) && b0 <= (0xDF as u8)) {
        if (pos + 1 >= s.len) {
            next_pos.* = s.len;
            return 0xFFFFFFFF as u32;
        }
        const b1 = s[pos + 1];
        if ((b1 & (0xC0 as u8)) != (0x80 as u8)) {
            next_pos.* = pos + 1;
            return 0xFFFFFFFF as u32;
        }
        next_pos.* = pos + 2;
        return (((b0 & (0x1F as u8)) as u32) << (6 as u5)) | ((b1 & (0x3F as u8)) as u32);
    }
    // 3-byte sequence: 0xE0..0xEF
    if (b0 >= (0xE0 as u8) && b0 <= (0xEF as u8)) {
        if (pos + 2 >= s.len) {
            next_pos.* = s.len;
            return 0xFFFFFFFF as u32;
        }
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        if ((b1 & (0xC0 as u8)) != (0x80 as u8) || (b2 & (0xC0 as u8)) != (0x80 as u8)) {
            next_pos.* = pos + 1;
            return 0xFFFFFFFF as u32;
        }
        const cp = (((b0 & (0x0F as u8)) as u32) << (12 as u5))
            | (((b1 & (0x3F as u8)) as u32) << (6 as u5))
            | ((b2 & (0x3F as u8)) as u32);
        // Reject overlong (< 0x0800) and UTF-16 surrogates (0xD800..0xDFFF)
        if (cp < (0x0800 as u32) || (cp >= (0xD800 as u32) && cp <= (0xDFFF as u32))) {
            next_pos.* = pos + 3;
            return 0xFFFFFFFF as u32;
        }
        next_pos.* = pos + 3;
        return cp;
    }
    // 4-byte sequence: 0xF0..0xF4
    if (b0 >= (0xF0 as u8) && b0 <= (0xF4 as u8)) {
        if (pos + 3 >= s.len) {
            next_pos.* = s.len;
            return 0xFFFFFFFF as u32;
        }
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        const b3 = s[pos + 3];
        if ((b1 & (0xC0 as u8)) != (0x80 as u8)
            || (b2 & (0xC0 as u8)) != (0x80 as u8)
            || (b3 & (0xC0 as u8)) != (0x80 as u8)) {
            next_pos.* = pos + 1;
            return 0xFFFFFFFF as u32;
        }
        const cp = (((b0 & (0x07 as u8)) as u32) << (18 as u5))
            | (((b1 & (0x3F as u8)) as u32) << (12 as u5))
            | (((b2 & (0x3F as u8)) as u32) << (6 as u5))
            | ((b3 & (0x3F as u8)) as u32);
        if (cp < (0x10000 as u32) || cp > (0x10FFFF as u32)) {
            next_pos.* = pos + 4;
            return 0xFFFFFFFF as u32;
        }
        next_pos.* = pos + 4;
        return cp;
    }
    next_pos.* = pos + 1;
    return 0xFFFFFFFF as u32;
}

/// Returns `true` if `cp` has the Unicode `XID_Start` property (UAX #31).
/// Covers ASCII letters, Latin-1/Extended, Greek, Cyrillic, Armenian, Hebrew, Arabic,
/// Devanagari, Hiragana, Katakana, Hangul, and CJK Unified Ideographs.
pub fn isXidStart(cp: u32): bool {
    // Fast path: ASCII
    if (cp < (0x80 as u32)) {
        return (cp >= (0x41 as u32) && cp <= (0x5A as u32))
            || (cp >= (0x61 as u32) && cp <= (0x7A as u32));
    }
    if (cp > (0x10FFFF as u32)) {
        return false;
    }

    // Latin-1 Supplement letters (excluding 0x00D7 × and 0x00F7 ÷)
    if (cp <= (0x00FF as u32)) {
        if (cp == (0x00AA as u32) || cp == (0x00B5 as u32) || cp == (0x00BA as u32)) {
            return true;
        }
        if (cp >= (0x00C0 as u32) && cp <= (0x00D6 as u32)) { return true; }
        if (cp >= (0x00D8 as u32) && cp <= (0x00F6 as u32)) { return true; }
        if (cp >= (0x00F8 as u32) && cp <= (0x00FF as u32)) { return true; }
        return false;
    }

    // Latin Extended-A, Latin Extended-B, IPA Extensions, Spacing Modifier Letters
    if (cp >= (0x0100 as u32) && cp <= (0x02C1 as u32)) { return true; }
    if (cp >= (0x02C6 as u32) && cp <= (0x02D1 as u32)) { return true; }
    if (cp >= (0x02E0 as u32) && cp <= (0x02E4 as u32)) { return true; }
    if (cp == (0x02EC as u32) || cp == (0x02EE as u32)) { return true; }

    // Greek and Coptic
    if (cp >= (0x0370 as u32) && cp <= (0x0374 as u32)) { return true; }
    if (cp >= (0x0376 as u32) && cp <= (0x0377 as u32)) { return true; }
    if (cp >= (0x037B as u32) && cp <= (0x037D as u32)) { return true; }
    if (cp == (0x037F as u32) || cp == (0x0386 as u32) || cp == (0x038C as u32)) { return true; }
    if (cp >= (0x0388 as u32) && cp <= (0x038A as u32)) { return true; }
    if (cp >= (0x038E as u32) && cp <= (0x03A1 as u32)) { return true; }
    if (cp >= (0x03A3 as u32) && cp <= (0x03F5 as u32)) { return true; }
    if (cp >= (0x03F7 as u32) && cp <= (0x03FF as u32)) { return true; }

    // Cyrillic & Cyrillic Supplement
    if (cp >= (0x0400 as u32) && cp <= (0x0481 as u32)) { return true; }
    if (cp >= (0x048A as u32) && cp <= (0x052F as u32)) { return true; }

    // Armenian, Hebrew, Arabic, Devanagari
    if (cp >= (0x0531 as u32) && cp <= (0x0556 as u32)) { return true; }
    if (cp >= (0x0560 as u32) && cp <= (0x0588 as u32)) { return true; }
    if (cp >= (0x05D0 as u32) && cp <= (0x05EA as u32)) { return true; }
    if (cp >= (0x0620 as u32) && cp <= (0x064A as u32)) { return true; }
    if (cp >= (0x066E as u32) && cp <= (0x066F as u32)) { return true; }
    if (cp >= (0x0671 as u32) && cp <= (0x06D3 as u32)) { return true; }
    if (cp >= (0x0904 as u32) && cp <= (0x0939 as u32)) { return true; }
    if (cp >= (0x0958 as u32) && cp <= (0x0961 as u32)) { return true; }

    // Hangul Jamo
    if (cp >= (0x1100 as u32) && cp <= (0x11FF as u32)) { return true; }

    // Latin Extended Additional & Greek Extended
    if (cp >= (0x1E00 as u32) && cp <= (0x1EFF as u32)) { return true; }
    if (cp >= (0x1F00 as u32) && cp <= (0x1F15 as u32)) { return true; }
    if (cp >= (0x1F18 as u32) && cp <= (0x1F1D as u32)) { return true; }
    if (cp >= (0x1F20 as u32) && cp <= (0x1F45 as u32)) { return true; }
    if (cp >= (0x1F48 as u32) && cp <= (0x1F4D as u32)) { return true; }
    if (cp >= (0x1F50 as u32) && cp <= (0x1F57 as u32)) { return true; }
    if (cp == (0x1F59 as u32) || cp == (0x1F5B as u32) || cp == (0x1F5D as u32)) { return true; }
    if (cp >= (0x1F5F as u32) && cp <= (0x1F7D as u32)) { return true; }
    if (cp >= (0x1F80 as u32) && cp <= (0x1FB4 as u32)) { return true; }
    if (cp >= (0x1FB6 as u32) && cp <= (0x1FBC as u32)) { return true; }
    if (cp >= (0x1FC2 as u32) && cp <= (0x1FC4 as u32)) { return true; }
    if (cp >= (0x1FC6 as u32) && cp <= (0x1FCC as u32)) { return true; }
    if (cp >= (0x1FD0 as u32) && cp <= (0x1FD3 as u32)) { return true; }
    if (cp >= (0x1FD6 as u32) && cp <= (0x1FDB as u32)) { return true; }
    if (cp >= (0x1FE0 as u32) && cp <= (0x1FEC as u32)) { return true; }
    if (cp >= (0x1FF2 as u32) && cp <= (0x1FF4 as u32)) { return true; }
    if (cp >= (0x1FF6 as u32) && cp <= (0x1FFC as u32)) { return true; }

    // CJK Symbols (iteration mark 々, closing mark 〆, ideographic zero 〇)
    if (cp >= (0x3005 as u32) && cp <= (0x3007 as u32)) { return true; }
    // Hiragana
    if (cp >= (0x3041 as u32) && cp <= (0x3096 as u32)) { return true; }
    if (cp >= (0x309D as u32) && cp <= (0x309F as u32)) { return true; }
    // Katakana & Katakana Phonetic Extensions
    if (cp >= (0x30A1 as u32) && cp <= (0x30FA as u32)) { return true; }
    if (cp >= (0x30FC as u32) && cp <= (0x30FF as u32)) { return true; }
    if (cp >= (0x31F0 as u32) && cp <= (0x31FF as u32)) { return true; }
    // Bopomofo & Hangul Compatibility Jamo
    if (cp >= (0x3105 as u32) && cp <= (0x312F as u32)) { return true; }
    if (cp >= (0x3131 as u32) && cp <= (0x318E as u32)) { return true; }

    // CJK Unified Ideographs Extension A
    if (cp >= (0x3400 as u32) && cp <= (0x4DBF as u32)) { return true; }
    // CJK Unified Ideographs
    if (cp >= (0x4E00 as u32) && cp <= (0x9FFF as u32)) { return true; }

    // Hangul Jamo Extended-A, Hangul Syllables, Hangul Jamo Extended-B
    if (cp >= (0xA960 as u32) && cp <= (0xA97C as u32)) { return true; }
    if (cp >= (0xAC00 as u32) && cp <= (0xD7A3 as u32)) { return true; }
    if (cp >= (0xD7B0 as u32) && cp <= (0xD7C6 as u32)) { return true; }
    if (cp >= (0xD7CB as u32) && cp <= (0xD7FB as u32)) { return true; }

    // CJK Compatibility Ideographs
    if (cp >= (0xF900 as u32) && cp <= (0xFA6D as u32)) { return true; }
    if (cp >= (0xFA70 as u32) && cp <= (0xFAD9 as u32)) { return true; }

    // Halfwidth Katakana & Hangul
    if (cp >= (0xFF66 as u32) && cp <= (0xFFBE as u32)) { return true; }
    if (cp >= (0xFFC2 as u32) && cp <= (0xFFC7 as u32)) { return true; }
    if (cp >= (0xFFCA as u32) && cp <= (0xFFCF as u32)) { return true; }
    if (cp >= (0xFFD2 as u32) && cp <= (0xFFD7 as u32)) { return true; }
    if (cp >= (0xFFDA as u32) && cp <= (0xFFDC as u32)) { return true; }

    // CJK Unified Ideographs Extensions B..I
    if (cp >= (0x20000 as u32) && cp <= (0x2A6DF as u32)) { return true; }
    if (cp >= (0x2A700 as u32) && cp <= (0x2B739 as u32)) { return true; }
    if (cp >= (0x2B740 as u32) && cp <= (0x2B81D as u32)) { return true; }
    if (cp >= (0x2B820 as u32) && cp <= (0x2CEA1 as u32)) { return true; }
    if (cp >= (0x2CEB0 as u32) && cp <= (0x2EBE0 as u32)) { return true; }
    if (cp >= (0x2EBF0 as u32) && cp <= (0x2EE5D as u32)) { return true; }
    if (cp >= (0x30000 as u32) && cp <= (0x3134A as u32)) { return true; }
    if (cp >= (0x31350 as u32) && cp <= (0x323AF as u32)) { return true; }

    return false;
}

/// Returns `true` if `cp` has the Unicode `XID_Continue` property (UAX #31).
/// Includes `isXidStart(cp)`, ASCII digits `0-9`, underscore `_`, combining marks,
/// connector punctuation, and decimal digits.
pub fn isXidContinue(cp: u32): bool {
    // Fast path: ASCII
    if (cp < (0x80 as u32)) {
        return (cp >= (0x41 as u32) && cp <= (0x5A as u32))
            || (cp >= (0x61 as u32) && cp <= (0x7A as u32))
            || (cp >= (0x30 as u32) && cp <= (0x39 as u32))
            || cp == (0x5F as u32);
    }
    if (isXidStart(cp)) {
        return true;
    }

    // Other_ID_Continue & connector punctuation
    if (cp == (0x00B7 as u32) || cp == (0x0387 as u32)) { return true; }
    if (cp >= (0x203F as u32) && cp <= (0x2040 as u32)) { return true; }
    if (cp == (0x2054 as u32)) { return true; }
    if (cp >= (0xFE33 as u32) && cp <= (0xFE34 as u32)) { return true; }
    if (cp >= (0xFE4D as u32) && cp <= (0xFE4F as u32)) { return true; }
    if (cp == (0xFF3F as u32)) { return true; }

    // Combining Diacritical Marks & Extensions
    if (cp >= (0x0300 as u32) && cp <= (0x036F as u32)) { return true; }
    if (cp >= (0x1AB0 as u32) && cp <= (0x1AFF as u32)) { return true; }
    if (cp >= (0x1DC0 as u32) && cp <= (0x1DFF as u32)) { return true; }
    if (cp >= (0x20D0 as u32) && cp <= (0x20FF as u32)) { return true; }
    if (cp >= (0xFE20 as u32) && cp <= (0xFE2F as u32)) { return true; }
    // Katakana-Hiragana combining voiced sound marks
    if (cp >= (0x3099 as u32) && cp <= (0x309A as u32)) { return true; }

    // Non-ASCII decimal digits (Arabic-Indic, Extended Arabic-Indic, Devanagari, Fullwidth)
    if (cp >= (0x0660 as u32) && cp <= (0x0669 as u32)) { return true; }
    if (cp >= (0x06F0 as u32) && cp <= (0x06F9 as u32)) { return true; }
    if (cp >= (0x0966 as u32) && cp <= (0x096F as u32)) { return true; }
    if (cp >= (0xFF10 as u32) && cp <= (0xFF19 as u32)) { return true; }

    return false;
}

/// Returns `true` if `cp` is one of the 25 Unicode `White_Space` codepoints:
/// U+0009..U+000D, U+0020, U+0085, U+00A0, U+1680, U+2000..U+200A,
/// U+2028, U+2029, U+202F, U+205F, U+3000.
pub fn isUnicodeWhitespace(cp: u32): bool {
    if (cp == (0x0020 as u32)) { return true; }
    if (cp >= (0x0009 as u32) && cp <= (0x000D as u32)) { return true; }
    if (cp < (0x0085 as u32)) { return false; }
    if (cp == (0x0085 as u32) || cp == (0x00A0 as u32) || cp == (0x1680 as u32)) { return true; }
    if (cp >= (0x2000 as u32) && cp <= (0x200A as u32)) { return true; }
    if (cp == (0x2028 as u32) || cp == (0x2029 as u32) || cp == (0x202F as u32)) { return true; }
    if (cp == (0x205F as u32) || cp == (0x3000 as u32)) { return true; }
    return false;
}

/// Returns `true` if `cp` has the UTS #51 `Emoji_Presentation` property
/// (characters that default to colorful emoji presentation without U+FE0F).
pub fn isEmojiPresentation(cp: u32): bool {
    if (cp < (0x231A as u32) || cp > (0x1FAFF as u32)) {
        return false;
    }
    // BMP Emoji_Presentation codepoints
    if (cp <= (0x2B55 as u32)) {
        if (cp >= (0x231A as u32) && cp <= (0x231B as u32)) { return true; }
        if (cp >= (0x23E9 as u32) && cp <= (0x23EC as u32)) { return true; }
        if (cp == (0x23F0 as u32) || cp == (0x23F3 as u32)) { return true; }
        if (cp >= (0x25FD as u32) && cp <= (0x25FE as u32)) { return true; }
        if (cp >= (0x2614 as u32) && cp <= (0x2615 as u32)) { return true; }
        if (cp >= (0x2648 as u32) && cp <= (0x2653 as u32)) { return true; }
        if (cp == (0x267F as u32) || cp == (0x2693 as u32) || cp == (0x26A1 as u32)) { return true; }
        if (cp >= (0x26AA as u32) && cp <= (0x26AB as u32)) { return true; }
        if (cp >= (0x26BD as u32) && cp <= (0x26BE as u32)) { return true; }
        if (cp >= (0x26C4 as u32) && cp <= (0x26C5 as u32)) { return true; }
        if (cp == (0x26CE as u32) || cp == (0x26D4 as u32) || cp == (0x26EA as u32)) { return true; }
        if (cp >= (0x26F2 as u32) && cp <= (0x26F3 as u32)) { return true; }
        if (cp == (0x26F5 as u32) || cp == (0x26FA as u32) || cp == (0x26FD as u32)) { return true; }
        if (cp == (0x2705 as u32) || cp == (0x2728 as u32)) { return true; }
        if (cp >= (0x270A as u32) && cp <= (0x270B as u32)) { return true; }
        if (cp == (0x274C as u32) || cp == (0x274E as u32)) { return true; }
        if (cp >= (0x2753 as u32) && cp <= (0x2755 as u32)) { return true; }
        if (cp == (0x2757 as u32)) { return true; }
        if (cp >= (0x2795 as u32) && cp <= (0x2797 as u32)) { return true; }
        if (cp == (0x27B0 as u32) || cp == (0x27BF as u32)) { return true; }
        if (cp >= (0x2B1B as u32) && cp <= (0x2B1C as u32)) { return true; }
        if (cp == (0x2B50 as u32) || cp == (0x2B55 as u32)) { return true; }
        return false;
    }

    // SMP Emoji blocks (U+1F004..U+1FAFF)
    if (cp == (0x1F004 as u32) || cp == (0x1F0CF as u32) || cp == (0x1F18E as u32)) { return true; }
    if (cp >= (0x1F191 as u32) && cp <= (0x1F19A as u32)) { return true; }
    if (cp >= (0x1F1E6 as u32) && cp <= (0x1F1FF as u32)) { return true; }
    if (cp == (0x1F201 as u32) || cp == (0x1F21A as u32) || cp == (0x1F22F as u32)) { return true; }
    if (cp >= (0x1F232 as u32) && cp <= (0x1F236 as u32)) { return true; }
    if (cp >= (0x1F238 as u32) && cp <= (0x1F23A as u32)) { return true; }
    if (cp >= (0x1F250 as u32) && cp <= (0x1F251 as u32)) { return true; }

    // Miscellaneous Symbols and Pictographs, Emoticons, Transport & Map Symbols
    if (cp >= (0x1F300 as u32) && cp <= (0x1F320 as u32)) { return true; }
    if (cp >= (0x1F32D as u32) && cp <= (0x1F335 as u32)) { return true; }
    if (cp >= (0x1F337 as u32) && cp <= (0x1F37C as u32)) { return true; }
    if (cp >= (0x1F37E as u32) && cp <= (0x1F393 as u32)) { return true; }
    if (cp >= (0x1F3A0 as u32) && cp <= (0x1F3CA as u32)) { return true; }
    if (cp >= (0x1F3CF as u32) && cp <= (0x1F3D3 as u32)) { return true; }
    if (cp >= (0x1F3E0 as u32) && cp <= (0x1F3F0 as u32)) { return true; }
    if (cp == (0x1F3F4 as u32)) { return true; }
    if (cp >= (0x1F3F8 as u32) && cp <= (0x1F43E as u32)) { return true; }
    if (cp == (0x1F440 as u32)) { return true; }
    if (cp >= (0x1F442 as u32) && cp <= (0x1F4FC as u32)) { return true; }
    if (cp >= (0x1F4FF as u32) && cp <= (0x1F53D as u32)) { return true; }
    if (cp >= (0x1F54B as u32) && cp <= (0x1F54E as u32)) { return true; }
    if (cp >= (0x1F550 as u32) && cp <= (0x1F567 as u32)) { return true; }
    if (cp == (0x1F57A as u32) || cp == (0x1F5A4 as u32)) { return true; }
    if (cp >= (0x1F595 as u32) && cp <= (0x1F596 as u32)) { return true; }
    if (cp >= (0x1F5FB as u32) && cp <= (0x1F64F as u32)) { return true; }
    if (cp >= (0x1F680 as u32) && cp <= (0x1F6C5 as u32)) { return true; }
    if (cp == (0x1F6CC as u32)) { return true; }
    if (cp >= (0x1F6D0 as u32) && cp <= (0x1F6D2 as u32)) { return true; }
    if (cp >= (0x1F6D5 as u32) && cp <= (0x1F6D7 as u32)) { return true; }
    if (cp >= (0x1F6DC as u32) && cp <= (0x1F6DF as u32)) { return true; }
    if (cp >= (0x1F6EB as u32) && cp <= (0x1F6EC as u32)) { return true; }
    if (cp >= (0x1F6F4 as u32) && cp <= (0x1F6FC as u32)) { return true; }

    // Geometric Shapes Extended, Supplemental Symbols and Pictographs, Symbols Extended-A
    if (cp >= (0x1F7E0 as u32) && cp <= (0x1F7EB as u32)) { return true; }
    if (cp == (0x1F7F0 as u32)) { return true; }
    if (cp >= (0x1F90C as u32) && cp <= (0x1F93A as u32)) { return true; }
    if (cp >= (0x1F93C as u32) && cp <= (0x1F945 as u32)) { return true; }
    if (cp >= (0x1F947 as u32) && cp <= (0x1F9FF as u32)) { return true; }
    if (cp >= (0x1FA70 as u32) && cp <= (0x1FA7C as u32)) { return true; }
    if (cp >= (0x1FA80 as u32) && cp <= (0x1FA88 as u32)) { return true; }
    if (cp >= (0x1FA90 as u32) && cp <= (0x1FABD as u32)) { return true; }
    if (cp >= (0x1FABF as u32) && cp <= (0x1FAC5 as u32)) { return true; }
    if (cp >= (0x1FACE as u32) && cp <= (0x1FADB as u32)) { return true; }
    if (cp >= (0x1FAE0 as u32) && cp <= (0x1FAE8 as u32)) { return true; }
    if (cp >= (0x1FAF0 as u32) && cp <= (0x1FAF8 as u32)) { return true; }

    return false;
}

/// Returns `true` if `cp` has the UTS #51 `Emoji` property
/// (both `Emoji_Presentation` and text-presentation emoji codepoints).
pub fn isEmoji(cp: u32): bool {
    if (isEmojiPresentation(cp)) {
        return true;
    }
    // ASCII keycap bases & Latin-1 copyright/registered
    if (cp == (0x0023 as u32) || cp == (0x002A as u32)) { return true; }
    if (cp >= (0x0030 as u32) && cp <= (0x0039 as u32)) { return true; }
    if (cp == (0x00A9 as u32) || cp == (0x00AE as u32)) { return true; }

    // General Punctuation, Letterlike Symbols, Arrows
    if (cp == (0x203C as u32) || cp == (0x2049 as u32)) { return true; }
    if (cp == (0x2122 as u32) || cp == (0x2139 as u32)) { return true; }
    if (cp >= (0x2194 as u32) && cp <= (0x2199 as u32)) { return true; }
    if (cp >= (0x21A9 as u32) && cp <= (0x21AA as u32)) { return true; }

    // Miscellaneous Technical & Geometric Shapes
    if (cp == (0x2328 as u32) || cp == (0x23CF as u32)) { return true; }
    if (cp >= (0x23ED as u32) && cp <= (0x23EF as u32)) { return true; }
    if (cp >= (0x23F1 as u32) && cp <= (0x23F2 as u32)) { return true; }
    if (cp >= (0x23F8 as u32) && cp <= (0x23FA as u32)) { return true; }
    if (cp == (0x24C2 as u32)) { return true; }
    if (cp >= (0x25AA as u32) && cp <= (0x25AB as u32)) { return true; }
    if (cp == (0x25B6 as u32) || cp == (0x25C0 as u32)) { return true; }
    if (cp >= (0x25FB as u32) && cp <= (0x25FC as u32)) { return true; }

    // Miscellaneous Symbols (text-default emoji)
    if (cp >= (0x2600 as u32) && cp <= (0x2604 as u32)) { return true; }
    if (cp == (0x260E as u32) || cp == (0x2611 as u32) || cp == (0x2618 as u32) || cp == (0x261D as u32)) { return true; }
    if (cp == (0x2620 as u32) || cp == (0x2626 as u32) || cp == (0x262A as u32)) { return true; }
    if (cp >= (0x2622 as u32) && cp <= (0x2623 as u32)) { return true; }
    if (cp >= (0x262E as u32) && cp <= (0x262F as u32)) { return true; }
    if (cp >= (0x2638 as u32) && cp <= (0x263A as u32)) { return true; }
    if (cp == (0x2640 as u32) || cp == (0x2642 as u32)) { return true; }
    if (cp == (0x265F as u32) || cp == (0x2660 as u32) || cp == (0x2663 as u32)) { return true; }
    if (cp >= (0x2665 as u32) && cp <= (0x2666 as u32)) { return true; }
    if (cp == (0x2668 as u32) || cp == (0x267B as u32) || cp == (0x267E as u32)) { return true; }
    if (cp >= (0x2692 as u32) && cp <= (0x2697 as u32)) { return true; }
    if (cp == (0x2699 as u32) || cp == (0x26A0 as u32) || cp == (0x26A7 as u32)) { return true; }
    if (cp >= (0x269B as u32) && cp <= (0x269C as u32)) { return true; }
    if (cp >= (0x26B0 as u32) && cp <= (0x26B1 as u32)) { return true; }
    if (cp == (0x26C8 as u32) || cp == (0x26CF as u32) || cp == (0x26D1 as u32) || cp == (0x26D3 as u32)) { return true; }
    if (cp >= (0x26E9 as u32) && cp <= (0x26F1 as u32)) { return true; }
    if (cp == (0x26F4 as u32) || cp == (0x26FC as u32)) { return true; }
    if (cp >= (0x26F7 as u32) && cp <= (0x26F9 as u32)) { return true; }

    // Dingbats & CJK Symbols
    if (cp == (0x2702 as u32) || cp == (0x270F as u32) || cp == (0x2712 as u32)) { return true; }
    if (cp >= (0x2708 as u32) && cp <= (0x270D as u32)) { return true; }
    if (cp == (0x2714 as u32) || cp == (0x2716 as u32) || cp == (0x271D as u32) || cp == (0x2721 as u32)) { return true; }
    if (cp >= (0x2733 as u32) && cp <= (0x2734 as u32)) { return true; }
    if (cp == (0x2744 as u32) || cp == (0x2747 as u32) || cp == (0x27A1 as u32)) { return true; }
    if (cp >= (0x2763 as u32) && cp <= (0x2764 as u32)) { return true; }
    if (cp >= (0x2934 as u32) && cp <= (0x2935 as u32)) { return true; }
    if (cp >= (0x2B05 as u32) && cp <= (0x2B07 as u32)) { return true; }
    if (cp == (0x3030 as u32) || cp == (0x303D as u32) || cp == (0x3297 as u32) || cp == (0x3299 as u32)) { return true; }

    // SMP text-default emoji
    if (cp >= (0x1F170 as u32) && cp <= (0x1F171 as u32)) { return true; }
    if (cp >= (0x1F17E as u32) && cp <= (0x1F17F as u32)) { return true; }
    if (cp == (0x1F202 as u32) || cp == (0x1F237 as u32)) { return true; }
    if (cp == (0x1F321 as u32) || cp == (0x1F336 as u32) || cp == (0x1F37D as u32)) { return true; }
    if (cp >= (0x1F324 as u32) && cp <= (0x1F32C as u32)) { return true; }
    if (cp >= (0x1F396 as u32) && cp <= (0x1F397 as u32)) { return true; }
    if (cp >= (0x1F399 as u32) && cp <= (0x1F39B as u32)) { return true; }
    if (cp >= (0x1F39E as u32) && cp <= (0x1F39F as u32)) { return true; }
    if (cp >= (0x1F3CB as u32) && cp <= (0x1F3CE as u32)) { return true; }
    if (cp >= (0x1F3D4 as u32) && cp <= (0x1F3DF as u32)) { return true; }
    if (cp == (0x1F3F3 as u32) || cp == (0x1F3F5 as u32) || cp == (0x1F3F7 as u32)) { return true; }
    if (cp == (0x1F43F as u32) || cp == (0x1F441 as u32) || cp == (0x1F4FD as u32)) { return true; }
    if (cp >= (0x1F549 as u32) && cp <= (0x1F54A as u32)) { return true; }
    if (cp >= (0x1F56F as u32) && cp <= (0x1F570 as u32)) { return true; }
    if (cp >= (0x1F573 as u32) && cp <= (0x1F579 as u32)) { return true; }
    if (cp == (0x1F587 as u32) || cp == (0x1F590 as u32) || cp == (0x1F5A5 as u32) || cp == (0x1F5A8 as u32)) { return true; }
    if (cp >= (0x1F58A as u32) && cp <= (0x1F58D as u32)) { return true; }
    if (cp >= (0x1F5B1 as u32) && cp <= (0x1F5B2 as u32)) { return true; }
    if (cp == (0x1F5BC as u32)) { return true; }
    if (cp >= (0x1F5C2 as u32) && cp <= (0x1F5C4 as u32)) { return true; }
    if (cp >= (0x1F5D1 as u32) && cp <= (0x1F5D3 as u32)) { return true; }
    if (cp >= (0x1F5DC as u32) && cp <= (0x1F5DE as u32)) { return true; }
    if (cp == (0x1F5E1 as u32) || cp == (0x1F5E3 as u32) || cp == (0x1F5E8 as u32)) { return true; }
    if (cp == (0x1F5EF as u32) || cp == (0x1F5F3 as u32) || cp == (0x1F5FA as u32)) { return true; }
    if (cp == (0x1F6CB as u32)) { return true; }
    if (cp >= (0x1F6CD as u32) && cp <= (0x1F6CF as u32)) { return true; }
    if (cp >= (0x1F6E0 as u32) && cp <= (0x1F6E5 as u32)) { return true; }
    if (cp == (0x1F6E9 as u32) || cp == (0x1F6F0 as u32) || cp == (0x1F6F3 as u32)) { return true; }

    return false;
}

/// Returns `true` if `cp` has the Unicode `Default_Ignorable_Code_Point` property
/// (soft hyphen U+00AD, zero-width characters U+200B..U+200F, BOM U+FEFF,
/// variation selectors U+FE00..U+FE0F / U+E0100..U+E01EF, bidi/format controls, etc.).
pub fn isDefaultIgnorable(cp: u32): bool {
    if (cp < (0x00AD as u32)) {
        return false;
    }
    if (cp == (0x00AD as u32) || cp == (0x034F as u32) || cp == (0x061C as u32)) { return true; }
    if (cp >= (0x115F as u32) && cp <= (0x1160 as u32)) { return true; }
    if (cp >= (0x17B4 as u32) && cp <= (0x17B5 as u32)) { return true; }
    if (cp >= (0x180B as u32) && cp <= (0x180F as u32)) { return true; }
    if (cp >= (0x200B as u32) && cp <= (0x200F as u32)) { return true; }
    if (cp >= (0x202A as u32) && cp <= (0x202E as u32)) { return true; }
    if (cp >= (0x2060 as u32) && cp <= (0x206F as u32)) { return true; }
    if (cp == (0x3164 as u32)) { return true; }
    if (cp >= (0xFE00 as u32) && cp <= (0xFE0F as u32)) { return true; }
    if (cp == (0xFEFF as u32) || cp == (0xFFA0 as u32)) { return true; }
    if (cp >= (0xFFF0 as u32) && cp <= (0xFFF8 as u32)) { return true; }
    if (cp >= (0x1BCA0 as u32) && cp <= (0x1BCA3 as u32)) { return true; }
    if (cp >= (0x1D173 as u32) && cp <= (0x1D17A as u32)) { return true; }
    if (cp >= (0xE0000 as u32) && cp <= (0xE0FFF as u32)) { return true; }
    return false;
}

/// Validates whether UTF-8 string `s` is a valid Unicode identifier (UAX #31 + `_` prefix).
/// - Must be non-empty and valid UTF-8.
/// - First codepoint must be `_` or satisfy `isXidStart`.
/// - All subsequent codepoints must satisfy `isXidContinue`.
pub fn isValidIdentifier(s: string): bool {
    if (s.len == (0 as usize)) {
        return false;
    }
    let pos: usize = 0;
    let next_pos: usize = 0;
    const first = decodeUtf8Strict(s, pos, &next_pos);
    if (first == (0xFFFFFFFF as u32)) {
        return false;
    }
    if (first != (0x5F as u32) && !isXidStart(first)) {
        return false;
    }
    pos = next_pos;
    while (pos < s.len) {
        const cp = decodeUtf8Strict(s, pos, &next_pos);
        if (cp == (0xFFFFFFFF as u32) || !isXidContinue(cp)) {
            return false;
        }
        pos = next_pos;
    }
    return true;
}
