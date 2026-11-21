// Source: Absorbed from /data/dev/bun/src/unicode/uucode_lib/src/grapheme.zig | Synced: 2026-11-21T04:56:55Z
// lib/string/grapheme.cp
// High-Speed UTF-8 Validator, Codepoint Counter & UAX #29 Extended Grapheme Cluster Analyzer
// Absorbed from Bun src/unicode/uucode_lib/src/grapheme.zig & src/simdutf_sys/

import { Utf8 } from "./utf.cp";

pub enum GraphemeBreakClass: u8 {
    Other = 0,
    Cr = 1,
    Lf = 2,
    Control = 3,
    Extend = 4,
    Zwj = 5,
    RegionalIndicator = 6,
    Prepend = 7,
    SpacingMark = 8,
    HangulL = 9,
    HangulV = 10,
    HangulT = 11,
    HangulLV = 12,
    HangulLVT = 13,
    ExtendedPictographic = 14,
}

enum GraphemeState: u8 {
    Default = 0,
    RegionalIndicator = 1,
    ExtendedPictographic = 2,
    ExtendedPictographicZwj = 3,
}

pub struct GraphemeSpan {
    pub start: usize = 0,
    pub end: usize = 0,

    pub fn len(): usize {
        return if (_.end > _.start) (_.end - _.start) else 0;
    }

    pub fn slice(src: []const u8): []const u8 {
        if (_.start >= src.len || _.end <= _.start) {
            return "";
        }
        const safe_end = if (_.end > src.len) src.len else _.end;
        return src[_.start..safe_end];
    }
}

fn classifyCodepoint(cp: u32): GraphemeBreakClass {
    if (cp == 0x0D) { return .Cr; }
    if (cp == 0x0A) { return .Lf; }
    // C0/C1 Controls and Unicode line/paragraph/format controls (excluding ZWJ/ZWNJ)
    if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F) || cp == 0x00AD ||
        cp == 0x061C || cp == 0x180E || cp == 0x200B ||
        (cp >= 0x200E && cp <= 0x200F) || cp == 0x2028 || cp == 0x2029 ||
        (cp >= 0x202A && cp <= 0x202E) || (cp >= 0x2060 && cp <= 0x206F) ||
        cp == 0xFEFF || (cp >= 0xFFF9 && cp <= 0xFFFB))
    {
        return .Control;
    }
    if (cp == 0x200D) { return .Zwj; }
    if (cp >= 0x1F1E6 && cp <= 0x1F1FF) { return .RegionalIndicator; }
    // Prepend characters (Arabic/Syriac/Indic number signs, Brahmi/Kaithi, etc.)
    if ((cp >= 0x0600 && cp <= 0x0605) || cp == 0x06DD || cp == 0x070F ||
        cp == 0x0890 || cp == 0x0891 || cp == 0x08E2 || cp == 0x0D4E ||
        cp == 0x110BD || cp == 0x110CD)
    {
        return .Prepend;
    }
    // Hangul Jamo L / V / T / LV / LVT
    if ((cp >= 0x1100 && cp <= 0x115F) || (cp >= 0xA960 && cp <= 0xA97C)) {
        return .HangulL;
    }
    if ((cp >= 0x1160 && cp <= 0x11A7) || (cp >= 0xD7B0 && cp <= 0xD7C6)) {
        return .HangulV;
    }
    if ((cp >= 0x11A8 && cp <= 0x11FF) || (cp >= 0xD7CB && cp <= 0xD7FB)) {
        return .HangulT;
    }
    if (cp >= 0xAC00 && cp <= 0xD7A3) {
        if (((cp - 0xAC00) % 28) == 0) {
            return .HangulLV;
        }
        return .HangulLVT;
    }
    // Emoji skin tone modifiers & Extending Marks (GB9)
    if (Utf8Grapheme.isGraphemeExtend(cp)) {
        return .Extend;
    }
    // SpacingMark (common Devanagari / Bengali / Tamil / Thai / Hangul spacing marks)
    if (cp == 0x0903 || cp == 0x093B || (cp >= 0x093E && cp <= 0x0940) ||
        (cp >= 0x0949 && cp <= 0x094C) || cp == 0x094E || cp == 0x094F ||
        cp == 0x0982 || cp == 0x0983 || cp == 0x09BE || cp == 0x09BF ||
        cp == 0x09C0 || cp == 0x0E30 || cp == 0x0E32 || cp == 0x0E33)
    {
        return .SpacingMark;
    }
    // Extended_Pictographic (Emoji & pictographs for GB11 ZWJ sequences)
    if (cp == 0x00A9 || cp == 0x00AE || cp == 0x203C || cp == 0x2049 ||
        cp == 0x2122 || cp == 0x2139 || (cp >= 0x2194 && cp <= 0x21AA) ||
        cp == 0x231A || cp == 0x231B || cp == 0x2328 || cp == 0x23CF ||
        (cp >= 0x23E9 && cp <= 0x23FA) || cp == 0x24C2 ||
        (cp >= 0x25AA && cp <= 0x27BF) || (cp >= 0x2934 && cp <= 0x2935) ||
        (cp >= 0x2B05 && cp <= 0x2B55) || cp == 0x3030 || cp == 0x303D ||
        cp == 0x3297 || cp == 0x3299 ||
        (cp >= 0x1F000 && cp <= 0x1F1E5) ||
        (cp >= 0x1F200 && cp <= 0x1F3FA) ||
        (cp >= 0x1F400 && cp <= 0x1FAFF))
    {
        return .ExtendedPictographic;
    }
    return .Other;
}

fn isBreakBetween(gb1: GraphemeBreakClass, gb2: GraphemeBreakClass, state: *GraphemeState): bool {
    // Update multi-codepoint state transitions
    let cur_state = state.*;
    if (cur_state == .RegionalIndicator) {
        if (gb1 != .RegionalIndicator || gb2 != .RegionalIndicator) {
            cur_state = .Default;
        }
    } else if (cur_state == .ExtendedPictographic) {
        if (gb2 == .Zwj) {
            cur_state = .ExtendedPictographicZwj;
        } else if (gb2 != .Extend) {
            cur_state = .Default;
        }
    } else if (cur_state == .ExtendedPictographicZwj) {
        // Consumed inside GB11 check below
    }

    // GB3: CR × LF
    if (gb1 == .Cr && gb2 == .Lf) {
        state.* = .Default;
        return false;
    }
    // GB4: (Control | CR | LF) ÷
    if (gb1 == .Control || gb1 == .Cr || gb1 == .Lf) {
        state.* = .Default;
        return true;
    }
    // GB5: ÷ (Control | CR | LF)
    if (gb2 == .Control || gb2 == .Cr || gb2 == .Lf) {
        state.* = .Default;
        return true;
    }
    // GB6: L × (L | V | LV | LVT)
    if (gb1 == .HangulL && (gb2 == .HangulL || gb2 == .HangulV || gb2 == .HangulLV || gb2 == .HangulLVT)) {
        state.* = .Default;
        return false;
    }
    // GB7: (LV | V) × (V | T)
    if ((gb1 == .HangulLV || gb1 == .HangulV) && (gb2 == .HangulV || gb2 == .HangulT)) {
        state.* = .Default;
        return false;
    }
    // GB8: (LVT | T) × T
    if ((gb1 == .HangulLVT || gb1 == .HangulT) && gb2 == .HangulT) {
        state.* = .Default;
        return false;
    }
    // GB11: Extended_Pictographic Extend* ZWJ × Extended_Pictographic
    if (gb1 == .ExtendedPictographic) {
        if (gb2 == .Extend) {
            state.* = .ExtendedPictographic;
            return false;
        }
        if (gb2 == .Zwj) {
            state.* = .ExtendedPictographicZwj;
            return false;
        }
    }
    if (state.* == .ExtendedPictographic && (gb2 == .Extend || gb2 == .Zwj)) {
        state.* = if (gb2 == .Zwj) .ExtendedPictographicZwj else .ExtendedPictographic;
        return false;
    }
    if (state.* == .ExtendedPictographicZwj && gb1 == .Zwj && gb2 == .ExtendedPictographic) {
        state.* = .ExtendedPictographic;
        return false;
    }
    // GB9: × (Extend | ZWJ)
    if (gb2 == .Extend || gb2 == .Zwj) {
        if (gb1 == .ExtendedPictographic) {
            state.* = if (gb2 == .Zwj) .ExtendedPictographicZwj else .ExtendedPictographic;
        } else {
            state.* = .Default;
        }
        return false;
    }
    // GB9a: × SpacingMark
    if (gb2 == .SpacingMark) {
        state.* = .Default;
        return false;
    }
    // GB9b: Prepend ×
    if (gb1 == .Prepend) {
        state.* = .Default;
        return false;
    }
    // GB12 / GB13: RI × RI (paired two-by-two)
    if (gb1 == .RegionalIndicator && gb2 == .RegionalIndicator) {
        if (state.* == .Default) {
            state.* = .RegionalIndicator;
            return false;
        } else {
            state.* = .Default;
            return true;
        }
    }
    state.* = .Default;
    return true;
}

/// Zero-allocation UAX #29 Extended Grapheme Cluster iterator over UTF-8 bytes.
/// Supports CRLF, combining marks, ZWJ Emoji sequences, Hangul Jamo, and Regional Indicator flag pairs.
pub struct GraphemeIterator {
    bytes: []const u8 = "",
    pos: usize = 0,

    pub static fn init(bytes: []const u8): GraphemeIterator {
        return GraphemeIterator{
            bytes: bytes,
            pos: 0,
        };
    }

    pub fn reset(*_) {
        _.pos = 0;
    }

    /// Advances and returns the byte span `{ start, end }` of the next grapheme cluster.
    pub fn nextSpan(*_): GraphemeSpan? {
        if (_.pos >= _.bytes.len) {
            return null;
        }
        const cluster_start = _.pos;
        let state: GraphemeState = .Default;

        let dec1 = Utf8.decodeNext(_.bytes, _.pos);
        let step1: usize = if (dec1.bytes_consumed > 0) (dec1.bytes_consumed as usize) else 1;
        _.pos += step1;
        let gb1 = classifyCodepoint(dec1.codepoint);

        if (gb1 == .ExtendedPictographic) {
            state = .ExtendedPictographic;
        }

        while (_.pos < _.bytes.len) {
            const dec2 = Utf8.decodeNext(_.bytes, _.pos);
            const gb2 = classifyCodepoint(dec2.codepoint);
            if (isBreakBetween(gb1, gb2, *state)) {
                break;
            }
            const step2: usize = if (dec2.bytes_consumed > 0) (dec2.bytes_consumed as usize) else 1;
            _.pos += step2;
            gb1 = gb2;
        }

        return GraphemeSpan{
            start: cluster_start,
            end: _.pos,
        };
    }

    /// Advances and returns the next grapheme cluster as a UTF-8 slice (`[]const u8`), or `null` at EOF.
    pub fn next(*_): ([]const u8)? {
        if (span := _.nextSpan()) {
            return _.bytes[span.start..span.end];
        }
        return null;
    }

    /// Peeks the next grapheme cluster slice without advancing the cursor.
    pub fn peek(*_): ([]const u8)? {
        let copy = GraphemeIterator{
            bytes: _.bytes,
            pos: _.pos,
        };
        if (span := copy.nextSpan()) {
            return _.bytes[span.start..span.end];
        }
        return null;
    }
}

/// Counts user-perceived characters (UAX #29 extended grapheme clusters) in `bytes`.
pub fn graphemeCount(bytes: []const u8): usize {
    let it = GraphemeIterator.init(bytes);
    let count: usize = 0;
    while (it.nextSpan() != null) {
        count += 1;
    }
    return count;
}

pub struct Utf8Grapheme {
    /// Validates whether `bytes` is well-formed UTF-8 according to RFC 3629.
    pub static fn isValidUtf8(bytes: []const u8): bool {
        return Utf8.isValid(bytes);
    }

    /// Returns the byte length (1..4) of the UTF-8 sequence starting at `lead_byte`.
    pub static fn sequenceLength(lead_byte: u8): usize {
        const len = Utf8.sequenceLength(lead_byte);
        return if (len == 0) 1 else (len as usize);
    }

    /// Decodes a single Unicode scalar value (codepoint) at `bytes[offset..]`.
    pub static fn decodeCodepointAt(bytes: []const u8, offset: usize): u32 {
        return Utf8.decodeNext(bytes, offset).codepoint;
    }

    /// Returns true if `cp` is a Unicode extending mark, variation selector, skin-tone modifier, or ZWJ.
    pub static fn isGraphemeExtend(cp: u32): bool {
        if (cp >= 0x0300 && cp <= 0x036F) { return true; }
        if (cp >= 0x0483 && cp <= 0x0489) { return true; }
        if ((cp >= 0x0591 && cp <= 0x05BD) || cp == 0x05BF || cp == 0x05C1 || cp == 0x05C2 || cp == 0x05C4 || cp == 0x05C5 || cp == 0x05C7) { return true; }
        if ((cp >= 0x0610 && cp <= 0x061A) || (cp >= 0x064B && cp <= 0x065F) || cp == 0x0670 || (cp >= 0x06D6 && cp <= 0x06DC) || (cp >= 0x06DF && cp <= 0x06E4) || cp == 0x06E7 || cp == 0x06E8 || (cp >= 0x06EA && cp <= 0x06ED)) { return true; }
        if ((cp >= 0x0901 && cp <= 0x0902) || cp == 0x093C || (cp >= 0x0941 && cp <= 0x0948) || cp == 0x094D || (cp >= 0x0951 && cp <= 0x0957) || (cp >= 0x0962 && cp <= 0x0963)) { return true; }
        if (cp == 0x0E31 || (cp >= 0x0E34 && cp <= 0x0E3A) || (cp >= 0x0E47 && cp <= 0x0E4E)) { return true; }
        if ((cp >= 0x1AB0 && cp <= 0x1AFF) || (cp >= 0x1DC0 && cp <= 0x1DFF)) { return true; }
        if (cp == 0x200C || cp == 0x200D) { return true; } // ZWNJ / ZWJ
        if (cp >= 0x20D0 && cp <= 0x20FF) { return true; }
        if ((cp >= 0x302A && cp <= 0x302F) || cp == 0x3099 || cp == 0x309A) { return true; }
        if ((cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0xFE20 && cp <= 0xFE2F)) { return true; }
        if (cp >= 0x1F3FB && cp <= 0x1F3FF) { return true; }
        if ((cp >= 0xE0020 && cp <= 0xE007F) || (cp >= 0xE0100 && cp <= 0xE01EF)) { return true; }
        return false;
    }

    /// Counts Unicode scalar values (codepoints) in `bytes`.
    pub static fn countCodepoints(bytes: []const u8): usize {
        return Utf8.codepointCount(bytes);
    }

    /// Counts user-perceived characters (UAX #29 extended grapheme clusters).
    pub static fn countGraphemes(bytes: []const u8): usize {
        return graphemeCount(bytes);
    }

    /// Returns a zero-allocation `GraphemeIterator` over `bytes`.
    pub static fn iterator(bytes: []const u8): GraphemeIterator {
        return GraphemeIterator.init(bytes);
    }
}
