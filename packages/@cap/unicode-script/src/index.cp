// @cap/unicode-script — Zero-heap UAX #24 Unicode Script classifier (crates.io:unicode-script)

pub enum Script {
    Common,
    Latin,
    Greek,
    Cyrillic,
    Han,
    Hiragana,
    Katakana,
    Hangul,
    Arabic,
    Hebrew,
    Devanagari,
    Thai,
    Inherited,
    Unknown,
}

fn uscScriptIndex(s: Script): usize {
    if (s == Script.Common) return 0;
    if (s == Script.Latin) return 1;
    if (s == Script.Greek) return 2;
    if (s == Script.Cyrillic) return 3;
    if (s == Script.Han) return 4;
    if (s == Script.Hiragana) return 5;
    if (s == Script.Katakana) return 6;
    if (s == Script.Hangul) return 7;
    if (s == Script.Arabic) return 8;
    if (s == Script.Hebrew) return 9;
    if (s == Script.Devanagari) return 10;
    if (s == Script.Thai) return 11;
    if (s == Script.Inherited) return 12;
    return 13;
}

fn uscIndexToScript(idx: usize): Script {
    if (idx == 0) return Script.Common;
    if (idx == 1) return Script.Latin;
    if (idx == 2) return Script.Greek;
    if (idx == 3) return Script.Cyrillic;
    if (idx == 4) return Script.Han;
    if (idx == 5) return Script.Hiragana;
    if (idx == 6) return Script.Katakana;
    if (idx == 7) return Script.Hangul;
    if (idx == 8) return Script.Arabic;
    if (idx == 9) return Script.Hebrew;
    if (idx == 10) return Script.Devanagari;
    if (idx == 11) return Script.Thai;
    if (idx == 12) return Script.Inherited;
    return Script.Unknown;
}

/// Classify a Unicode scalar value into its UAX #24 `Script`.
pub fn scriptOf(cp: u32): Script {
    if (cp > 0x10FFFF) {
        return Script.Unknown;
    }

    // Latin
    if ((cp >= 0x0041 && cp <= 0x005A) || (cp >= 0x0061 && cp <= 0x007A) ||
        cp == 0x00AA || cp == 0x00BA ||
        (cp >= 0x00C0 && cp <= 0x00D6) || (cp >= 0x00D8 && cp <= 0x00F6) ||
        (cp >= 0x00F8 && cp <= 0x02B8) || (cp >= 0x02E0 && cp <= 0x02E4) ||
        (cp >= 0x1D00 && cp <= 0x1D25) || (cp >= 0x1D2C && cp <= 0x1D5C) ||
        (cp >= 0x1D62 && cp <= 0x1D65) || (cp >= 0x1D6B && cp <= 0x1D77) ||
        (cp >= 0x1D79 && cp <= 0x1DBE) || (cp >= 0x1E00 && cp <= 0x1EFF) ||
        cp == 0x2071 || cp == 0x207F || (cp >= 0x2090 && cp <= 0x209C) ||
        cp == 0x212A || cp == 0x212B || cp == 0x2132 || cp == 0x214E ||
        (cp >= 0x2160 && cp <= 0x2188) || (cp >= 0x2C60 && cp <= 0x2C7F) ||
        (cp >= 0xA722 && cp <= 0xA787) || (cp >= 0xA78B && cp <= 0xA7FF) ||
        (cp >= 0xAB30 && cp <= 0xAB6B) || (cp >= 0xFB00 && cp <= 0xFB06) ||
        (cp >= 0xFF21 && cp <= 0xFF3A) || (cp >= 0xFF41 && cp <= 0xFF5A)) {
        return Script.Latin;
    }

    // Inherited combining marks & variation selectors
    if ((cp >= 0x0300 && cp <= 0x036F) || cp == 0x0485 || cp == 0x0486 ||
        (cp >= 0x064B && cp <= 0x0655) || cp == 0x0670 ||
        (cp >= 0x1AB0 && cp <= 0x1AFF) || (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        cp == 0x200C || cp == 0x200D || (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0x302A && cp <= 0x302D) || cp == 0x3099 || cp == 0x309A ||
        (cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0xFE20 && cp <= 0xFE2F) ||
        (cp >= 0xE0100 && cp <= 0xE01EF)) {
        return Script.Inherited;
    }

    // Greek
    if ((cp >= 0x0370 && cp <= 0x0373) || (cp >= 0x0375 && cp <= 0x0377) ||
        (cp >= 0x037A && cp <= 0x037D) || cp == 0x037F || cp == 0x0384 ||
        cp == 0x0386 || (cp >= 0x0388 && cp <= 0x038A) || cp == 0x038C ||
        (cp >= 0x038E && cp <= 0x03A1) || (cp >= 0x03A3 && cp <= 0x03E1) ||
        (cp >= 0x03F0 && cp <= 0x03FF) || (cp >= 0x1D26 && cp <= 0x1D2A) ||
        (cp >= 0x1D5D && cp <= 0x1D61) || (cp >= 0x1D66 && cp <= 0x1D6A) ||
        (cp >= 0x1F00 && cp <= 0x1FFF)) {
        return Script.Greek;
    }

    // Cyrillic
    if ((cp >= 0x0400 && cp <= 0x0484) || (cp >= 0x0487 && cp <= 0x052F) ||
        (cp >= 0x1C80 && cp <= 0x1C88) || (cp >= 0x2DE0 && cp <= 0x2DFF) ||
        (cp >= 0xA640 && cp <= 0xA69F)) {
        return Script.Cyrillic;
    }

    // Hebrew
    if ((cp >= 0x0591 && cp <= 0x05C7) || (cp >= 0x05D0 && cp <= 0x05EA) ||
        (cp >= 0x05EF && cp <= 0x05F4) || (cp >= 0xFB1D && cp <= 0xFB36) ||
        (cp >= 0xFB38 && cp <= 0xFB3C) || cp == 0xFB3E ||
        cp == 0xFB40 || cp == 0xFB41 || cp == 0xFB43 || cp == 0xFB44 ||
        (cp >= 0xFB46 && cp <= 0xFB4F)) {
        return Script.Hebrew;
    }

    // Arabic
    if ((cp >= 0x0600 && cp <= 0x0604) || (cp >= 0x0606 && cp <= 0x060B) ||
        (cp >= 0x060D && cp <= 0x061A) || (cp >= 0x061C && cp <= 0x061E) ||
        (cp >= 0x0620 && cp <= 0x063F) || (cp >= 0x0641 && cp <= 0x064A) ||
        (cp >= 0x0656 && cp <= 0x066F) || (cp >= 0x0671 && cp <= 0x06DC) ||
        (cp >= 0x06DE && cp <= 0x06FF) || (cp >= 0x0750 && cp <= 0x077F) ||
        (cp >= 0x0870 && cp <= 0x08FF) || (cp >= 0xFB50 && cp <= 0xFDFF) ||
        (cp >= 0xFE70 && cp <= 0xFEFF)) {
        return Script.Arabic;
    }

    // Devanagari
    if ((cp >= 0x0900 && cp <= 0x0950) || (cp >= 0x0955 && cp <= 0x0963) ||
        (cp >= 0x0966 && cp <= 0x097F) || (cp >= 0xA8E0 && cp <= 0xA8FF)) {
        return Script.Devanagari;
    }

    // Thai
    if ((cp >= 0x0E01 && cp <= 0x0E3A) || (cp >= 0x0E40 && cp <= 0x0E5B)) {
        return Script.Thai;
    }

    // Hangul
    if ((cp >= 0x1100 && cp <= 0x11FF) || (cp >= 0x3131 && cp <= 0x318E) ||
        (cp >= 0x3200 && cp <= 0x321E) || (cp >= 0x3260 && cp <= 0x327E) ||
        (cp >= 0xA960 && cp <= 0xA97C) || (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xD7B0 && cp <= 0xD7FB) || (cp >= 0xFFA0 && cp <= 0xFFDC)) {
        return Script.Hangul;
    }

    // Hiragana
    if ((cp >= 0x3041 && cp <= 0x3096) || (cp >= 0x309D && cp <= 0x309F) ||
        (cp >= 0x1B001 && cp <= 0x1B11F) || cp == 0x1B132 ||
        (cp >= 0x1B150 && cp <= 0x1B152) || cp == 0x1F200) {
        return Script.Hiragana;
    }

    // Katakana
    if ((cp >= 0x30A1 && cp <= 0x30FA) || (cp >= 0x30FD && cp <= 0x30FF) ||
        (cp >= 0x31F0 && cp <= 0x31FF) || (cp >= 0x32D0 && cp <= 0x32FE) ||
        (cp >= 0x3300 && cp <= 0x3357) || (cp >= 0xFF66 && cp <= 0xFF6F) ||
        (cp >= 0xFF71 && cp <= 0xFF9D) || cp == 0x1B000 || cp == 0x1B155 ||
        (cp >= 0x1B164 && cp <= 0x1B167)) {
        return Script.Katakana;
    }

    // Han (CJK Unified Ideographs & Extensions)
    if ((cp >= 0x2E80 && cp <= 0x2EF3) || (cp >= 0x2F00 && cp <= 0x2FD5) ||
        cp == 0x3005 || cp == 0x3007 || (cp >= 0x3021 && cp <= 0x3029) ||
        (cp >= 0x3038 && cp <= 0x303B) || (cp >= 0x3400 && cp <= 0x4DBF) ||
        (cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0x20000 && cp <= 0x2A6DF) || (cp >= 0x2A700 && cp <= 0x2CEAF) ||
        (cp >= 0x2CEB0 && cp <= 0x2EBEF) || (cp >= 0x2F800 && cp <= 0x2FA1F) ||
        (cp >= 0x30000 && cp <= 0x323AF)) {
        return Script.Han;
    }

    // Common (ASCII controls/digits/punctuation, symbols, CJK punctuation, emoji)
    if ((cp <= 0x0040) || (cp >= 0x005B && cp <= 0x0060) ||
        (cp >= 0x007B && cp <= 0x00BF) || cp == 0x00D7 || cp == 0x00F7 ||
        (cp >= 0x02B9 && cp <= 0x02DF) || (cp >= 0x02E5 && cp <= 0x02FF) ||
        (cp >= 0x2000 && cp <= 0x20CF) || (cp >= 0x2100 && cp <= 0x27BF) ||
        (cp >= 0x3000 && cp <= 0x303F) || cp == 0x309B || cp == 0x309C ||
        cp == 0x30A0 || cp == 0x30FB || cp == 0x30FC ||
        (cp >= 0xFE10 && cp <= 0xFE6F) || (cp >= 0xFF01 && cp <= 0xFF20) ||
        (cp >= 0xFF3B && cp <= 0xFF40) || (cp >= 0xFF5B && cp <= 0xFF65) ||
        (cp >= 0x1F000 && cp <= 0x1FAFF)) {
        return Script.Common;
    }

    return Script.Unknown;
}

/// Return the canonical UAX #24 English name of a `Script`.
pub fn scriptName(s: Script): string {
    if (s == Script.Common) return "Common";
    if (s == Script.Latin) return "Latin";
    if (s == Script.Greek) return "Greek";
    if (s == Script.Cyrillic) return "Cyrillic";
    if (s == Script.Han) return "Han";
    if (s == Script.Hiragana) return "Hiragana";
    if (s == Script.Katakana) return "Katakana";
    if (s == Script.Hangul) return "Hangul";
    if (s == Script.Arabic) return "Arabic";
    if (s == Script.Hebrew) return "Hebrew";
    if (s == Script.Devanagari) return "Devanagari";
    if (s == Script.Thai) return "Thai";
    if (s == Script.Inherited) return "Inherited";
    return "Unknown";
}

/// Return true if `cp` belongs to the `Latin` script.
pub fn isLatin(cp: u32): bool {
    return scriptOf(cp) == Script.Latin;
}

/// Return true if `cp` belongs to the `Han` script.
pub fn isHan(cp: u32): bool {
    return scriptOf(cp) == Script.Han;
}

/// Return true if `s` is one of the East Asian CJK scripts (`Han`, `Hiragana`, `Katakana`, `Hangul`).
pub fn isCjkScript(s: Script): bool {
    return s == Script.Han || s == Script.Hiragana || s == Script.Katakana || s == Script.Hangul;
}

struct UscDecoded {
    cp: u32,
    len: usize,
}

fn uscDecodeUtf8(text: string, pos: usize): UscDecoded {
    const b0 = text[pos];
    if (b0 < 0x80) {
        return UscDecoded{ cp: b0 as u32, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < text.len) {
        const b1 = text[pos + 1];
        const cp = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
        return UscDecoded{ cp: cp, len: 2 };
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < text.len) {
        const b1 = text[pos + 1];
        const b2 = text[pos + 2];
        const cp = (((b0 & 0x0F) as u32) << 12) | (((b1 & 0x3F) as u32) << 6) | ((b2 & 0x3F) as u32);
        return UscDecoded{ cp: cp, len: 3 };
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < text.len) {
        const b1 = text[pos + 1];
        const b2 = text[pos + 2];
        const b3 = text[pos + 3];
        const cp = (((b0 & 0x07) as u32) << 18) | (((b1 & 0x3F) as u32) << 12) | (((b2 & 0x3F) as u32) << 6) | ((b3 & 0x3F) as u32);
        return UscDecoded{ cp: cp, len: 4 };
    }
    return UscDecoded{ cp: 0xFFFD, len: 1 };
}

/// Detect the dominant `Script` of a UTF-8 string, skipping `Common` and `Inherited` characters.
/// Returns `Script.Common` if all characters are Common/Inherited, or `Script.Unknown` if empty.
pub fn dominantScript(text: string): Script {
    if (text.len == 0) {
        return Script.Unknown;
    }
    const z: usize = 0;
    let counts: [14]usize = [z, z, z, z, z, z, z, z, z, z, z, z, z, z];
    let saw_common_or_inh: bool = false;

    let pos: usize = 0;
    while (pos < text.len) {
        const d = uscDecodeUtf8(text, pos);
        pos += d.len;
        const sc = scriptOf(d.cp);
        if (sc == Script.Common || sc == Script.Inherited) {
            saw_common_or_inh = true;
            continue;
        }
        const idx = uscScriptIndex(sc);
        counts[idx] += 1;
    }

    let best_idx: usize = 13;
    let best_count: usize = 0;
    let i: usize = 1;
    while (i < 12) {
        if (counts[i] > best_count) {
            best_count = counts[i];
            best_idx = i;
        }
        i += 1;
    }

    if (best_count > 0) {
        return uscIndexToScript(best_idx);
    }
    if (saw_common_or_inh) {
        return Script.Common;
    }
    return Script.Unknown;
}
