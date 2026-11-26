// packages/@cap/unicode-segmentation/src/index.cp
// Zero-allocation UAX #29 Unicode Extended Grapheme Cluster & Word Boundary Segmentation.
// Clean-room MIT implementation inspired by Rust `unicode-segmentation`.

// ---------------------------------------------------------------------------
// Public Span Type
// ---------------------------------------------------------------------------

/// Byte-offset span `[start, end)` within the source UTF-8 string.
pub struct GraphemeSpan {
    pub start: usize,
    pub end: usize,

    pub fn byteLen(): usize {
        return _.end - _.start;
    }
}

// ---------------------------------------------------------------------------
// Private UTF-8 Decoder (with safe U+FFFD fallback on malformed bytes)
// ---------------------------------------------------------------------------

struct UsegRune {
    cp: u32,
    len: usize,
}

fn usegDecodeUtf8(src: string, offset: usize): UsegRune {
    if (offset >= src.len) {
        return UsegRune{ cp: 0, len: 0 };
    }
    const b0 = src[offset];
    if (b0 < (0x80 as u8)) {
        return UsegRune{ cp: b0 as u32, len: 1 };
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && offset + 1 < src.len) {
        const b1 = src[offset + 1];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp: u32 = (((b0 & (0x1F as u8)) as u32) << (6 as u5))
                | ((b1 & (0x3F as u8)) as u32);
            if (cp >= (0x80 as u32)) {
                return UsegRune{ cp: cp, len: 2 };
            }
        }
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && offset + 2 < src.len) {
        const b1 = src[offset + 1];
        const b2 = src[offset + 2];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8) && (b2 & (0xC0 as u8)) == (0x80 as u8)) {
            const cp: u32 = (((b0 & (0x0F as u8)) as u32) << (12 as u5))
                | (((b1 & (0x3F as u8)) as u32) << (6 as u5))
                | ((b2 & (0x3F as u8)) as u32);
            if (cp >= (0x800 as u32) && !(cp >= (0xD800 as u32) && cp <= (0xDFFF as u32))) {
                return UsegRune{ cp: cp, len: 3 };
            }
        }
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && offset + 3 < src.len) {
        const b1 = src[offset + 1];
        const b2 = src[offset + 2];
        const b3 = src[offset + 3];
        if ((b1 & (0xC0 as u8)) == (0x80 as u8)
            && (b2 & (0xC0 as u8)) == (0x80 as u8)
            && (b3 & (0xC0 as u8)) == (0x80 as u8))
        {
            const cp: u32 = (((b0 & (0x07 as u8)) as u32) << (18 as u5))
                | (((b1 & (0x3F as u8)) as u32) << (12 as u5))
                | (((b2 & (0x3F as u8)) as u32) << (6 as u5))
                | ((b3 & (0x3F as u8)) as u32);
            if (cp >= (0x10000 as u32) && cp <= (0x10FFFF as u32)) {
                return UsegRune{ cp: cp, len: 4 };
            }
        }
    }
    // Malformed UTF-8 byte: advance by 1 byte safely with replacement character U+FFFD
    return UsegRune{ cp: 0xFFFD as u32, len: 1 };
}

// ---------------------------------------------------------------------------
// UAX #29 Grapheme_Cluster_Break Property Classification
// ---------------------------------------------------------------------------

const USEG_GB_OTHER: u8 = 0;
const USEG_GB_CR: u8 = 1;
const USEG_GB_LF: u8 = 2;
const USEG_GB_CONTROL: u8 = 3;
const USEG_GB_EXTEND: u8 = 4;
const USEG_GB_ZWJ: u8 = 5;
const USEG_GB_RI: u8 = 6;
const USEG_GB_PREPEND: u8 = 7;
const USEG_GB_SPACING_MARK: u8 = 8;
const USEG_GB_L: u8 = 9;
const USEG_GB_V: u8 = 10;
const USEG_GB_T: u8 = 11;
const USEG_GB_LV: u8 = 12;
const USEG_GB_LVT: u8 = 13;
const USEG_GB_EXTPIC: u8 = 14;

fn usegIsExtend(cp: u32): bool {
    // Combining Diacritical Marks & common script combining marks
    if (cp >= 0x0300 && cp <= 0x036F) { return true; }
    if (cp >= 0x0483 && cp <= 0x0489) { return true; }
    if (cp >= 0x0591 && cp <= 0x05BD) { return true; }
    if (cp == 0x05BF || cp == 0x05C1 || cp == 0x05C2 || cp == 0x05C4 || cp == 0x05C5 || cp == 0x05C7) { return true; }
    if (cp >= 0x0610 && cp <= 0x061A) { return true; }
    if (cp >= 0x064B && cp <= 0x065F) { return true; }
    if (cp == 0x0670) { return true; }
    if (cp >= 0x06D6 && cp <= 0x06DC) { return true; }
    if (cp >= 0x06DF && cp <= 0x06E4) { return true; }
    if (cp == 0x06E7 || cp == 0x06E8 || (cp >= 0x06EA && cp <= 0x06ED)) { return true; }
    if (cp >= 0x0900 && cp <= 0x0902) { return true; }
    if (cp == 0x093C || (cp >= 0x0941 && cp <= 0x0948) || cp == 0x094D || (cp >= 0x0951 && cp <= 0x0957)) { return true; }
    if (cp == 0x0E31 || (cp >= 0x0E34 && cp <= 0x0E3A) || (cp >= 0x0E47 && cp <= 0x0E4E)) { return true; }
    if (cp >= 0x1AB0 && cp <= 0x1AFF) { return true; }
    if (cp >= 0x1DC0 && cp <= 0x1DFF) { return true; }
    if (cp == 0x200C) { return true; } // ZWNJ
    if (cp >= 0x20D0 && cp <= 0x20FF) { return true; } // Combining Diacritical Marks for Symbols (incl. U+20E3 keycap)
    if (cp >= 0x302A && cp <= 0x302F) { return true; }
    if (cp == 0x3099 || cp == 0x309A) { return true; }
    // Variation Selectors (VS1..VS16)
    if (cp >= 0xFE00 && cp <= 0xFE0F) { return true; }
    if (cp >= 0xFE20 && cp <= 0xFE2F) { return true; }
    // Emoji Fitzpatrick Skin Tone Modifiers
    if (cp >= 0x1F3FB && cp <= 0x1F3FF) { return true; }
    // Emoji Tag Sequences (subdivision flags) & Variation Selectors Supplement
    if (cp >= 0xE0020 && cp <= 0xE007F) { return true; }
    if (cp >= 0xE0100 && cp <= 0xE01EF) { return true; }
    return false;
}

fn usegIsPrepend(cp: u32): bool {
    if (cp >= 0x0600 && cp <= 0x0605) { return true; }
    if (cp == 0x06DD || cp == 0x070F || cp == 0x0890 || cp == 0x0891 || cp == 0x08E2) { return true; }
    if (cp == 0x110BD || cp == 0x110CD) { return true; }
    return false;
}

fn usegIsSpacingMark(cp: u32): bool {
    if (cp == 0x0903 || cp == 0x093B || (cp >= 0x093E && cp <= 0x0940)) { return true; }
    if ((cp >= 0x0949 && cp <= 0x094C) || cp == 0x094E || cp == 0x094F) { return true; }
    return false;
}

fn usegIsControl(cp: u32): bool {
    if (cp <= 0x001F) {
        return cp != 0x000A && cp != 0x000D;
    }
    if (cp >= 0x007F && cp <= 0x009F) { return true; }
    if (cp == 0x00AD || cp == 0x061C || cp == 0x180E) { return true; }
    if (cp == 0x200B || cp == 0x200E || cp == 0x200F) { return true; }
    if (cp >= 0x2028 && cp <= 0x202E) { return true; }
    if (cp >= 0x2060 && cp <= 0x206F) { return true; }
    if (cp == 0xFEFF || (cp >= 0xFFF9 && cp <= 0xFFFB)) { return true; }
    if (cp == 0xE0001) { return true; }
    return false;
}

fn usegIsExtPic(cp: u32): bool {
    if (cp == 0x00A9 || cp == 0x00AE) { return true; }
    if (cp == 0x203C || cp == 0x2049 || cp == 0x2122 || cp == 0x2139) { return true; }
    if ((cp >= 0x2194 && cp <= 0x2199) || cp == 0x21A9 || cp == 0x21AA) { return true; }
    if (cp == 0x231A || cp == 0x231B || cp == 0x2328 || cp == 0x23CF) { return true; }
    if ((cp >= 0x23E9 && cp <= 0x23F3) || (cp >= 0x23F8 && cp <= 0x23FA)) { return true; }
    if (cp == 0x24C2 || cp == 0x25AA || cp == 0x25AB || cp == 0x25B6 || cp == 0x25C0) { return true; }
    if (cp >= 0x25FB && cp <= 0x25FE) { return true; }
    if (cp >= 0x2600 && cp <= 0x27BF) { return true; }
    if (cp == 0x2934 || cp == 0x2935 || (cp >= 0x2B05 && cp <= 0x2B07)) { return true; }
    if (cp == 0x2B1B || cp == 0x2B1C || cp == 0x2B50 || cp == 0x2B55) { return true; }
    if (cp == 0x3030 || cp == 0x303D || cp == 0x3297 || cp == 0x3299) { return true; }
    if (cp >= 0x1F000 && cp <= 0x1F0FF) { return true; }
    if ((cp >= 0x1F10D && cp <= 0x1F10F) || cp == 0x1F12F || (cp >= 0x1F16C && cp <= 0x1F171)) { return true; }
    if (cp == 0x1F17E || cp == 0x1F17F || cp == 0x1F18E || (cp >= 0x1F191 && cp <= 0x1F19A)) { return true; }
    if (cp == 0x1F201 || cp == 0x1F202 || cp == 0x1F21A || cp == 0x1F22F) { return true; }
    if ((cp >= 0x1F232 && cp <= 0x1F23A) || cp == 0x1F250 || cp == 0x1F251) { return true; }
    // Note: 0x1F3FB..0x1F3FF are Fitzpatrick skin-tone modifiers (Extend), excluded here
    if (cp >= 0x1F300 && cp <= 0x1F3FA) { return true; }
    if (cp >= 0x1F400 && cp <= 0x1FAFF) { return true; }
    if (cp >= 0x1FC00 && cp <= 0x1FFFD) { return true; }
    return false;
}

fn usegGraphemeProp(cp: u32): u8 {
    if (cp == 0x000D) { return USEG_GB_CR; }
    if (cp == 0x000A) { return USEG_GB_LF; }
    if (cp == 0x200D) { return USEG_GB_ZWJ; }
    if (cp >= 0x1F1E6 && cp <= 0x1F1FF) { return USEG_GB_RI; }
    // Hangul Jamo L / V / T
    if ((cp >= 0x1100 && cp <= 0x115F) || (cp >= 0xA960 && cp <= 0xA97C)) { return USEG_GB_L; }
    if ((cp >= 0x1160 && cp <= 0x11A7) || (cp >= 0xD7B0 && cp <= 0xD7C6)) { return USEG_GB_V; }
    if ((cp >= 0x11A8 && cp <= 0x11FF) || (cp >= 0xD7CB && cp <= 0xD7FB)) { return USEG_GB_T; }
    // Hangul Precomposed Syllables LV / LVT
    if (cp >= 0xAC00 && cp <= 0xD7A3) {
        const diff: u32 = cp - (0xAC00 as u32);
        if ((diff % (28 as u32)) == (0 as u32)) {
            return USEG_GB_LV;
        }
        return USEG_GB_LVT;
    }
    if (usegIsExtend(cp)) { return USEG_GB_EXTEND; }
    if (usegIsPrepend(cp)) { return USEG_GB_PREPEND; }
    if (usegIsSpacingMark(cp)) { return USEG_GB_SPACING_MARK; }
    if (usegIsControl(cp)) { return USEG_GB_CONTROL; }
    if (usegIsExtPic(cp)) { return USEG_GB_EXTPIC; }
    return USEG_GB_OTHER;
}

fn usegNextGraphemeEnd(src: string, start: usize): usize {
    const r0 = usegDecodeUtf8(src, start);
    if (r0.len == 0) {
        return start;
    }
    let cursor: usize = start + r0.len;
    let prev: u8 = usegGraphemeProp(r0.cp);
    let in_extpic: bool = prev == USEG_GB_EXTPIC;
    let after_extpic_zwj: bool = false;
    let ri_count: usize = if (prev == USEG_GB_RI) { 1 as usize } else { 0 as usize };

    while (cursor < src.len) {
        const rn = usegDecodeUtf8(src, cursor);
        if (rn.len == 0) {
            break;
        }
        const pn: u8 = usegGraphemeProp(rn.cp);

        let join: bool = false;
        // GB3: CR x LF
        if (prev == USEG_GB_CR && pn == USEG_GB_LF) {
            join = true;
        // GB4 / GB5: (Control | CR | LF) ÷  and  ÷ (Control | CR | LF)
        } else if (prev == USEG_GB_CR || prev == USEG_GB_LF || prev == USEG_GB_CONTROL
            || pn == USEG_GB_CR || pn == USEG_GB_LF || pn == USEG_GB_CONTROL)
        {
            join = false;
        // GB6: L x (L | V | LV | LVT)
        } else if (prev == USEG_GB_L
            && (pn == USEG_GB_L || pn == USEG_GB_V || pn == USEG_GB_LV || pn == USEG_GB_LVT))
        {
            join = true;
        // GB7: (LV | V) x (V | T)
        } else if ((prev == USEG_GB_LV || prev == USEG_GB_V)
            && (pn == USEG_GB_V || pn == USEG_GB_T))
        {
            join = true;
        // GB8: (LVT | T) x T
        } else if ((prev == USEG_GB_LVT || prev == USEG_GB_T) && pn == USEG_GB_T) {
            join = true;
        // GB9: x (Extend | ZWJ)
        } else if (pn == USEG_GB_EXTEND || pn == USEG_GB_ZWJ) {
            join = true;
        // GB9a: x SpacingMark
        } else if (pn == USEG_GB_SPACING_MARK) {
            join = true;
        // GB9b: Prepend x
        } else if (prev == USEG_GB_PREPEND) {
            join = true;
        // GB11: Extended_Pictographic Extend* ZWJ x Extended_Pictographic
        } else if (after_extpic_zwj && pn == USEG_GB_EXTPIC) {
            join = true;
        // GB12 / GB13: RI x RI (pair-wise)
        } else if (prev == USEG_GB_RI && pn == USEG_GB_RI && ((ri_count % (2 as usize)) == (1 as usize))) {
            join = true;
        }

        if (!join) {
            break;
        }

        cursor += rn.len;
        if (pn == USEG_GB_EXTPIC) {
            in_extpic = true;
            after_extpic_zwj = false;
        } else if (pn == USEG_GB_EXTEND) {
            after_extpic_zwj = false;
        } else if (pn == USEG_GB_ZWJ) {
            after_extpic_zwj = in_extpic;
            in_extpic = false;
        } else {
            in_extpic = false;
            after_extpic_zwj = false;
        }

        if (pn == USEG_GB_RI) {
            ri_count += 1;
        } else {
            ri_count = 0;
        }
        prev = pn;
    }

    return cursor;
}

// ---------------------------------------------------------------------------
// Public Grapheme Cluster Iterator & Helpers
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over UAX #29 extended grapheme clusters.
pub struct GraphemeIter {
    pub src: string,
    pub pos: usize,

    pub static fn new(src: string): GraphemeIter {
        return GraphemeIter{ src: src, pos: 0 };
    }

    pub fn next(*_): GraphemeSpan? {
        if (_.pos >= _.src.len) {
            return null;
        }
        const start: usize = _.pos;
        const end: usize = usegNextGraphemeEnd(_.src, start);
        _.pos = end;
        return GraphemeSpan{ start: start, end: end };
    }

    pub fn reset(*_) {
        _.pos = 0;
    }
}

/// Counts total UAX #29 extended grapheme clusters in `src`.
pub fn countGraphemes(src: string): usize {
    let iter = GraphemeIter.new(src);
    let total: usize = 0;
    while (iter.next() != null) {
        total += 1;
    }
    return total;
}

/// Returns the `idx`-th (0-based) extended grapheme cluster span, or `null` if out of bounds.
pub fn nthGrapheme(src: string, idx: usize): GraphemeSpan? {
    let iter = GraphemeIter.new(src);
    let cur: usize = 0;
    while (true) {
        const sp = iter.next();
        if (sp == null) {
            return null;
        }
        if (cur == idx) {
            return sp;
        }
        cur += 1;
    }
    return null;
}

/// Returns a byte span covering up to `max_count` extended grapheme clusters starting at `start_idx`.
pub fn sliceGraphemes(src: string, start_idx: usize, max_count: usize): GraphemeSpan {
    let iter = GraphemeIter.new(src);
    let cur: usize = 0;
    let byte_start: usize = src.len;
    let byte_end: usize = src.len;
    let started: bool = false;

    if (start_idx == 0) {
        byte_start = 0;
        byte_end = 0;
        started = true;
    }

    while (cur < start_idx) {
        const sp = iter.next();
        if (sp == null) {
            return GraphemeSpan{ start: src.len, end: src.len };
        }
        cur += 1;
        if (cur == start_idx) {
            byte_start = iter.pos;
            byte_end = iter.pos;
            started = true;
        }
    }

    if (!started || max_count == 0) {
        return GraphemeSpan{ start: byte_start, end: byte_end };
    }

    let taken: usize = 0;
    while (taken < max_count) {
        const sp = iter.next();
        if (sp == null) {
            break;
        }
        byte_end = iter.pos;
        taken += 1;
    }

    return GraphemeSpan{ start: byte_start, end: byte_end };
}

// ---------------------------------------------------------------------------
// UAX #29 Word Boundary Classification & WordIter
// ---------------------------------------------------------------------------

const USEG_WB_OTHER: u8 = 0;
const USEG_WB_CR: u8 = 1;
const USEG_WB_LF: u8 = 2;
const USEG_WB_NEWLINE: u8 = 3;
const USEG_WB_EXTEND: u8 = 4;
const USEG_WB_ALETTER: u8 = 5;
const USEG_WB_NUMERIC: u8 = 6;
const USEG_WB_MIDNUMLET: u8 = 7;
const USEG_WB_MIDLETTER: u8 = 8;
const USEG_WB_MIDNUM: u8 = 9;
const USEG_WB_EXTENDNUMLET: u8 = 10;
const USEG_WB_WSEGSPACE: u8 = 11;
const USEG_WB_CJK: u8 = 12;

fn usegIsCjkWordChar(cp: u32): bool {
    if (cp >= 0x4E00 && cp <= 0x9FFF) { return true; }   // CJK Unified Ideographs
    if (cp >= 0x3400 && cp <= 0x4DBF) { return true; }   // CJK Extension A
    if (cp >= 0xF900 && cp <= 0xFAFF) { return true; }   // CJK Compatibility Ideographs
    if (cp >= 0x20000 && cp <= 0x2A6DF) { return true; } // CJK Extension B
    if (cp >= 0x3040 && cp <= 0x309F) { return true; }   // Hiragana
    if (cp >= 0x30A0 && cp <= 0x30FF) { return true; }   // Katakana
    return false;
}

fn usegIsALetter(cp: u32): bool {
    if ((cp >= 0x0041 && cp <= 0x005A) || (cp >= 0x0061 && cp <= 0x007A)) { return true; } // ASCII A-Z, a-z
    if ((cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00F6) || (cp >= 0x00F8 && cp <= 0x024F)) { return true; } // Latin-1 & Ext-A/B
    if (cp >= 0x0370 && cp <= 0x03FF) { return true; } // Greek
    if (cp >= 0x0400 && cp <= 0x04FF) { return true; } // Cyrillic
    if (cp >= 0x0590 && cp <= 0x05FF) { return true; } // Hebrew
    if (cp >= 0x0620 && cp <= 0x064A) { return true; } // Arabic letters
    if (cp >= 0x1100 && cp <= 0x11FF) { return true; } // Hangul Jamo
    if (cp >= 0xAC00 && cp <= 0xD7A3) { return true; } // Hangul Syllables
    return false;
}

fn usegIsNumeric(cp: u32): bool {
    if (cp >= 0x0030 && cp <= 0x0039) { return true; } // ASCII 0-9
    if (cp >= 0xFF10 && cp <= 0xFF19) { return true; } // Fullwidth digits
    return false;
}

fn usegWordProp(cp: u32): u8 {
    if (cp == 0x000D) { return USEG_WB_CR; }
    if (cp == 0x000A) { return USEG_WB_LF; }
    if (cp == 0x000B || cp == 0x000C || cp == 0x0085 || cp == 0x2028 || cp == 0x2029) { return USEG_WB_NEWLINE; }
    if (cp == 0x0020 || cp == 0x0009 || cp == 0x00A0 || cp == 0x1680 || (cp >= 0x2000 && cp <= 0x200A) || cp == 0x202F || cp == 0x205F || cp == 0x3000) {
        return USEG_WB_WSEGSPACE;
    }
    if (cp == 0x0027 || cp == 0x002E || cp == 0x2018 || cp == 0x2019 || cp == 0x2024) { return USEG_WB_MIDNUMLET; }
    if (cp == 0x003A || cp == 0x00B7 || cp == 0x2027) { return USEG_WB_MIDLETTER; }
    if (cp == 0x002C || cp == 0x003B) { return USEG_WB_MIDNUM; }
    if (cp == 0x005F) { return USEG_WB_EXTENDNUMLET; }
    if (cp == 0x200D || usegIsExtend(cp)) { return USEG_WB_EXTEND; }
    if (usegIsNumeric(cp)) { return USEG_WB_NUMERIC; }
    if (usegIsCjkWordChar(cp)) { return USEG_WB_CJK; }
    if (usegIsALetter(cp)) { return USEG_WB_ALETTER; }
    return USEG_WB_OTHER;
}

/// Peeks the next non-Extend word property starting at `offset`.
fn usegPeekWordProp(src: string, offset: usize): u8 {
    let cur: usize = offset;
    while (cur < src.len) {
        const r = usegDecodeUtf8(src, cur);
        if (r.len == 0) {
            return USEG_WB_OTHER;
        }
        const p: u8 = usegWordProp(r.cp);
        if (p != USEG_WB_EXTEND) {
            return p;
        }
        cur += r.len;
    }
    return USEG_WB_OTHER;
}

/// Returns true if the span `[start, end)` contains at least one word-bearing character.
fn usegIsWordSpan(src: string, start: usize, end: usize): bool {
    let cur: usize = start;
    while (cur < end) {
        const r = usegDecodeUtf8(src, cur);
        if (r.len == 0) {
            break;
        }
        const p: u8 = usegWordProp(r.cp);
        if (p == USEG_WB_ALETTER || p == USEG_WB_NUMERIC || p == USEG_WB_CJK) {
            return true;
        }
        cur += r.len;
    }
    return false;
}

/// Zero-allocation UAX #29 word boundary iterator.
pub struct WordIter {
    pub src: string,
    pub pos: usize,

    pub static fn new(src: string): WordIter {
        return WordIter{ src: src, pos: 0 };
    }

    pub fn reset(*_) {
        _.pos = 0;
    }

    /// Returns the next UAX #29 word-boundary segment (including whitespace and punctuation).
    pub fn next(*_): GraphemeSpan? {
        if (_.pos >= _.src.len) {
            return null;
        }
        const start: usize = _.pos;
        const r0 = usegDecodeUtf8(_.src, start);
        if (r0.len == 0) {
            return null;
        }
        let cursor: usize = start + r0.len;
        const p0: u8 = usegWordProp(r0.cp);

        // WB3: CR x LF
        if (p0 == USEG_WB_CR) {
            if (cursor < _.src.len) {
                const r1 = usegDecodeUtf8(_.src, cursor);
                if (r1.len > 0 && usegWordProp(r1.cp) == USEG_WB_LF) {
                    cursor += r1.len;
                }
            }
            _.pos = cursor;
            return GraphemeSpan{ start: start, end: cursor };
        }

        // WB3a / WB3b: LF or Newline
        if (p0 == USEG_WB_LF || p0 == USEG_WB_NEWLINE) {
            _.pos = cursor;
            return GraphemeSpan{ start: start, end: cursor };
        }

        // WB3d: WSegSpace x WSegSpace
        if (p0 == USEG_WB_WSEGSPACE) {
            while (cursor < _.src.len) {
                const rn = usegDecodeUtf8(_.src, cursor);
                if (rn.len == 0 || usegWordProp(rn.cp) != USEG_WB_WSEGSPACE) {
                    break;
                }
                cursor += rn.len;
            }
            _.pos = cursor;
            return GraphemeSpan{ start: start, end: cursor };
        }

        // CJK characters segment individually (plus any trailing combining marks)
        if (p0 == USEG_WB_CJK) {
            while (cursor < _.src.len) {
                const rn = usegDecodeUtf8(_.src, cursor);
                if (rn.len == 0 || usegWordProp(rn.cp) != USEG_WB_EXTEND) {
                    break;
                }
                cursor += rn.len;
            }
            _.pos = cursor;
            return GraphemeSpan{ start: start, end: cursor };
        }

        // Alphanumeric / ExtendNumLet word run (WB5..WB13b)
        if (p0 == USEG_WB_ALETTER || p0 == USEG_WB_NUMERIC || p0 == USEG_WB_EXTENDNUMLET) {
            let last_core: u8 = p0;
            while (cursor < _.src.len) {
                const rn = usegDecodeUtf8(_.src, cursor);
                if (rn.len == 0) {
                    break;
                }
                const pn: u8 = usegWordProp(rn.cp);
                if (pn == USEG_WB_EXTEND) {
                    cursor += rn.len;
                    continue;
                }
                if (pn == USEG_WB_ALETTER || pn == USEG_WB_NUMERIC || pn == USEG_WB_EXTENDNUMLET) {
                    cursor += rn.len;
                    last_core = pn;
                    continue;
                }
                // WB6/WB7: ALetter x (MidLetter | MidNumLet) ALetter (e.g. don't, we're)
                if ((pn == USEG_WB_MIDNUMLET || pn == USEG_WB_MIDLETTER) && last_core == USEG_WB_ALETTER) {
                    const next_prop: u8 = usegPeekWordProp(_.src, cursor + rn.len);
                    if (next_prop == USEG_WB_ALETTER) {
                        cursor += rn.len;
                        continue;
                    }
                }
                // WB11/WB12: Numeric x (MidNum | MidNumLet) Numeric (e.g. 3.14, 1,000)
                if ((pn == USEG_WB_MIDNUMLET || pn == USEG_WB_MIDNUM) && last_core == USEG_WB_NUMERIC) {
                    const next_prop: u8 = usegPeekWordProp(_.src, cursor + rn.len);
                    if (next_prop == USEG_WB_NUMERIC) {
                        cursor += rn.len;
                        continue;
                    }
                }
                break;
            }
            _.pos = cursor;
            return GraphemeSpan{ start: start, end: cursor };
        }

        // Other (punctuation, symbols, emoji): keep full grapheme cluster intact
        const g_end: usize = usegNextGraphemeEnd(_.src, start);
        _.pos = g_end;
        return GraphemeSpan{ start: start, end: g_end };
    }

    /// Returns the next Unicode word span, skipping whitespace and non-word punctuation.
    pub fn nextUnicodeWord(*_): GraphemeSpan? {
        while (true) {
            const sp = _.next();
            if (sp == null) {
                return null;
            }
            const val = sp.?;
            if (usegIsWordSpan(_.src, val.start, val.end)) {
                return val;
            }
        }
        return null;
    }
}

/// Counts the number of Unicode words (alphanumeric & CJK words, excluding whitespace/punctuation).
pub fn countUnicodeWords(src: string): usize {
    let iter = WordIter.new(src);
    let total: usize = 0;
    while (iter.nextUnicodeWord() != null) {
        total += 1;
    }
    return total;
}
