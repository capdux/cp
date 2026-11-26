// @cap/unicode-seg — Zero-allocation UAX #29 Unicode grapheme & word segmentation for CP
// Clean-room MIT implementation inspired by Rust `unicode-segmentation`.

pub struct DecodedRune {
    pub cp: u32,
    pub byte_len: usize,
}

pub struct TextSpan {
    pub start: usize,
    pub end: usize,
    pub slice: string,
}

// ---------------------------------------------------------------------------
// 1. UTF-8 Decoder (Strict with U+FFFD fallback)
// ---------------------------------------------------------------------------

/// Decodes one UTF-8 rune starting at `s[offset]`.
/// Malformed sequences safely advance by 1 byte and yield `U+FFFD`.
pub fn decodeUtf8At(s: string, offset: usize) -> DecodedRune {
    if (offset >= s.len) {
        return DecodedRune{ cp: 0, byte_len: 0 };
    }
    const b0 = s[offset];
    if (b0 < 0x80) {
        return DecodedRune{ cp: b0 as u32, byte_len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && offset + 1 < s.len) {
        const b1 = s[offset + 1];
        if ((b1 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            if (cp >= 0x80) {
                return DecodedRune{ cp: cp, byte_len: 2 };
            }
        }
    }
    if ((b0 & 0xF0) == 0xE0 && offset + 2 < s.len) {
        const b1 = s[offset + 1];
        const b2 = s[offset + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x0F) as u32) << 12)
                | (((b1 & 0x3F) as u32) << 6)
                | ((b2 & 0x3F) as u32);
            if (cp >= 0x800 && !(cp >= 0xD800 && cp <= 0xDFFF)) {
                return DecodedRune{ cp: cp, byte_len: 3 };
            }
        }
    }
    if ((b0 & 0xF8) == 0xF0 && offset + 3 < s.len) {
        const b1 = s[offset + 1];
        const b2 = s[offset + 2];
        const b3 = s[offset + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x07) as u32) << 18)
                | (((b1 & 0x3F) as u32) << 12)
                | (((b2 & 0x3F) as u32) << 6)
                | ((b3 & 0x3F) as u32);
            if (cp >= 0x10000 && cp <= 0x10FFFF) {
                return DecodedRune{ cp: cp, byte_len: 4 };
            }
        }
    }
    return DecodedRune{ cp: 0xFFFD, byte_len: 1 };
}

// ---------------------------------------------------------------------------
// UAX #29 Character Property Classifiers
// ---------------------------------------------------------------------------

fn isControl(cp: u32) -> bool {
    if (cp == 0x200D) return false; // ZWJ is handled specially
    return (cp < 0x20) || (cp >= 0x7F && cp <= 0x9F) || cp == 0x2028 || cp == 0x2029;
}

fn isRegionalIndicator(cp: u32) -> bool {
    return cp >= 0x1F1E6 && cp <= 0x1F1FF;
}

fn isCombiningOrExtend(cp: u32) -> bool {
    return (cp >= 0x0300 && cp <= 0x036F)       // Combining Diacritical Marks
        || (cp >= 0x1AB0 && cp <= 0x1AFF)       // Combining Diacritical Marks Extended
        || (cp >= 0x1DC0 && cp <= 0x1DFF)       // Combining Diacritical Marks Supplement
        || (cp >= 0x20D0 && cp <= 0x20FF)       // Combining Marks for Symbols (incl. U+20E3 keycap)
        || (cp >= 0xFE00 && cp <= 0xFE0F)       // Variation Selectors (VS1..VS16)
        || (cp >= 0xFE20 && cp <= 0xFE2F)       // Combining Half Marks
        || (cp >= 0x1F3FB && cp <= 0x1F3FF)     // Emoji Fitzpatrick Skin Tone Modifiers
        || (cp >= 0xE0020 && cp <= 0xE007F)     // Emoji Tag Sequences
        || (cp >= 0xE0100 && cp <= 0xE01EF);    // Variation Selectors Supplement
}

fn isCjkIdeograph(cp: u32) -> bool {
    return (cp >= 0x4E00 && cp <= 0x9FFF)
        || (cp >= 0x3400 && cp <= 0x4DBF)
        || (cp >= 0xF900 && cp <= 0xFAFF)
        || (cp >= 0x20000 && cp <= 0x2A6DF)
        || (cp >= 0x3040 && cp <= 0x30FF)
        || (cp >= 0xAC00 && cp <= 0xD7AF);
}

fn isLatinWordChar(cp: u32) -> bool {
    if ((cp >= 'A' as u32 && cp <= 'Z' as u32)
        || (cp >= 'a' as u32 && cp <= 'z' as u32)
        || (cp >= '0' as u32 && cp <= '9' as u32))
    {
        return true;
    }
    return (cp >= 0x00C0 && cp <= 0x00D6)
        || (cp >= 0x00D8 && cp <= 0x00F6)
        || (cp >= 0x00F8 && cp <= 0x024F);
}

// ---------------------------------------------------------------------------
// 2. UAX #29 Extended Grapheme Cluster Segmentation
// ---------------------------------------------------------------------------

/// Returns the next UAX #29 extended grapheme cluster starting at `offset`, or `null` at EOF.
/// Keeps CRLF (`\r\n`), combining diacritics, variation selectors, skin-tone modifiers,
/// ZWJ emoji sequences (`👨‍👩‍👧`), and paired Regional Indicator flags (`🇨🇳`) intact.
pub fn nextGrapheme(s: string, offset: usize) -> TextSpan? {
    if (offset >= s.len) {
        return null;
    }

    const first = decodeUtf8At(s, offset);
    let pos = offset + first.byte_len;

    // GB3: CR x LF
    if (first.cp == 0x0D) {
        if (pos < s.len && s[pos] == 0x0A) {
            pos += 1;
        }
        return TextSpan{ start: offset, end: pos, slice: s[offset..pos] };
    }

    // GB4: (Control | LF) ÷
    if (first.cp == 0x0A || isControl(first.cp)) {
        return TextSpan{ start: offset, end: pos, slice: s[offset..pos] };
    }

    // GB12/GB13: Regional Indicator pairs (flags like 🇨🇳 🇺🇸)
    if (isRegionalIndicator(first.cp)) {
        if (pos < s.len) {
            const second = decodeUtf8At(s, pos);
            if (isRegionalIndicator(second.cp)) {
                pos += second.byte_len;
            }
        }
        while (pos < s.len) {
            const ext = decodeUtf8At(s, pos);
            if (isCombiningOrExtend(ext.cp)) {
                pos += ext.byte_len;
            } else {
                break;
            }
        }
        return TextSpan{ start: offset, end: pos, slice: s[offset..pos] };
    }

    // GB9 / GB11: Extend / ZWJ sequences
    let prev_was_zwj = (first.cp == 0x200D);
    while (pos < s.len) {
        const next = decodeUtf8At(s, pos);
        if (next.byte_len == 0) {
            break;
        }
        if (next.cp == 0x0A || next.cp == 0x0D || isControl(next.cp)) {
            break;
        }
        if (next.cp == 0x200D) {
            pos += next.byte_len;
            prev_was_zwj = true;
            continue;
        }
        if (prev_was_zwj) {
            // Join the pictograph/character immediately following ZWJ
            pos += next.byte_len;
            prev_was_zwj = false;
            continue;
        }
        if (isCombiningOrExtend(next.cp)) {
            pos += next.byte_len;
            prev_was_zwj = false;
            continue;
        }
        break;
    }

    return TextSpan{ start: offset, end: pos, slice: s[offset..pos] };
}

/// Counts the total number of UAX #29 extended grapheme clusters in `s`.
pub fn graphemeCount(s: string) -> usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        const g = nextGrapheme(s, pos);
        if (g == null) break;
        count += 1;
        pos = g.?.end;
    }
    return count;
}

/// Safely truncates `s` to at most `max_graphemes` grapheme clusters (zero-copy slice),
/// never splitting UTF-8 sequences, combining diacritics, or ZWJ emoji clusters.
pub fn truncateGraphemes(s: string, max_graphemes: usize) -> string {
    if (max_graphemes == 0 || s.len == 0) {
        return s[0..0];
    }
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len && count < max_graphemes) {
        const g = nextGrapheme(s, pos);
        if (g == null) break;
        pos = g.?.end;
        count += 1;
    }
    return s[0..pos];
}

// ---------------------------------------------------------------------------
// 3. Unicode Word Boundary Segmentation
// ---------------------------------------------------------------------------

/// Returns the next Unicode word starting at or after `offset`, skipping whitespace & punctuation.
/// Latin/numeric words are grouped (preserving internal `'`/`-`/decimal `.`), while CJK ideographs
/// are segmented individually.
pub fn nextWord(s: string, offset: usize) -> TextSpan? {
    let pos = offset;
    // Skip non-word characters
    while (pos < s.len) {
        const ch = decodeUtf8At(s, pos);
        if (ch.byte_len == 0) return null;
        if (isCjkIdeograph(ch.cp) || isLatinWordChar(ch.cp)) {
            break;
        }
        pos += ch.byte_len;
    }

    if (pos >= s.len) {
        return null;
    }

    const start = pos;
    const first = decodeUtf8At(s, pos);

    // CJK ideograph: single character word (+ trailing combining marks if any)
    if (isCjkIdeograph(first.cp)) {
        pos += first.byte_len;
        while (pos < s.len) {
            const ext = decodeUtf8At(s, pos);
            if (isCombiningOrExtend(ext.cp)) {
                pos += ext.byte_len;
            } else {
                break;
            }
        }
        return TextSpan{ start: start, end: pos, slice: s[start..pos] };
    }

    // Latin / numeric word
    pos += first.byte_len;
    while (pos < s.len) {
        const ch = decodeUtf8At(s, pos);
        if (ch.byte_len == 0) break;
        if (isLatinWordChar(ch.cp) || isCombiningOrExtend(ch.cp)) {
            pos += ch.byte_len;
            continue;
        }
        // Keep internal apostrophe/hyphen/decimal dot when followed by another Latin word char
        if ((ch.cp == '\'' as u32 || ch.cp == '-' as u32 || ch.cp == '.' as u32) && pos + 1 < s.len) {
            const peek = decodeUtf8At(s, pos + 1);
            if (isLatinWordChar(peek.cp)) {
                pos += ch.byte_len;
                continue;
            }
        }
        break;
    }

    return TextSpan{ start: start, end: pos, slice: s[start..pos] };
}

/// Counts total words in `s` using `nextWord`.
pub fn wordCount(s: string) -> usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        const w = nextWord(s, pos);
        if (w == null) break;
        count += 1;
        pos = w.?.end;
    }
    return count;
}
