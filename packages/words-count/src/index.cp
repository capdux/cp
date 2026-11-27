// packages/@cap/word-count/src/index.cp
// Zero-allocation multilingual (Latin + CJK) text counter & reading-time estimator.
// MIT clean-room pure CP implementation.

/// Aggregated multilingual text statistics returned by `analyze`.
pub struct TextCounts {
    pub bytes: usize,
    pub chars: usize,
    pub words: usize,
    pub cjk_chars: usize,
    pub latin_words: usize,
    pub lines: usize,
}

/// Returns true if Unicode code point `cp` is a CJK ideograph, Kana, or Hangul syllable:
/// - CJK Unified Ideographs (`0x4E00..=0x9FFF`)
/// - CJK Extension A (`0x3400..=0x4DBF`)
/// - Hiragana & Katakana (`0x3040..=0x30FF`)
/// - Hangul Syllables (`0xAC00..=0xD7AF`)
pub fn isCjkCodepoint(cp: u32): bool {
    if (cp >= (0x4E00 as u32) && cp <= (0x9FFF as u32)) { return true; }
    if (cp >= (0x3400 as u32) && cp <= (0x4DBF as u32)) { return true; }
    if (cp >= (0x3040 as u32) && cp <= (0x30FF as u32)) { return true; }
    if (cp >= (0xAC00 as u32) && cp <= (0xD7AF as u32)) { return true; }
    return false;
}

fn wcIsLatinWordChar(cp: u32): bool {
    if ((cp >= ('a' as u32) && cp <= ('z' as u32)) || (cp >= ('A' as u32) && cp <= ('Z' as u32)) || (cp >= ('0' as u32) && cp <= ('9' as u32))) {
        return true;
    }
    // Latin-1 Supplement & Latin Extended letters (excluding × 0xD7 and ÷ 0xF7).
    if (cp >= (0x00C0 as u32) && cp <= (0x024F as u32) && cp != (0x00D7 as u32) && cp != (0x00F7 as u32)) {
        return true;
    }
    return false;
}

/// Decode one UTF-8 code point starting at `text[pos]`, returning its code point and advancing `next_pos.*`.
fn wcNextCodepoint(text: []const u8, pos: usize, next_pos: *usize): u32 {
    const b0 = text[pos];
    if (b0 < (0x80 as u8)) {
        next_pos.* = pos + 1;
        return b0 as u32;
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && pos + 1 < text.len && (text[pos + 1] & (0xC0 as u8)) == (0x80 as u8)) {
        next_pos.* = pos + 2;
        return (((b0 & (0x1F as u8)) as u32) << (6 as u5)) | ((text[pos + 1] & (0x3F as u8)) as u32);
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && pos + 2 < text.len && (text[pos + 1] & (0xC0 as u8)) == (0x80 as u8) && (text[pos + 2] & (0xC0 as u8)) == (0x80 as u8)) {
        next_pos.* = pos + 3;
        return (((b0 & (0x0F as u8)) as u32) << (12 as u5)) | (((text[pos + 1] & (0x3F as u8)) as u32) << (6 as u5)) | ((text[pos + 2] & (0x3F as u8)) as u32);
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && pos + 3 < text.len && (text[pos + 1] & (0xC0 as u8)) == (0x80 as u8) && (text[pos + 2] & (0xC0 as u8)) == (0x80 as u8) && (text[pos + 3] & (0xC0 as u8)) == (0x80 as u8)) {
        next_pos.* = pos + 4;
        return (((b0 & (0x07 as u8)) as u32) << (18 as u5)) | (((text[pos + 1] & (0x3F as u8)) as u32) << (12 as u5)) | (((text[pos + 2] & (0x3F as u8)) as u32) << (6 as u5)) | ((text[pos + 3] & (0x3F as u8)) as u32);
    }
    // Malformed byte fallback.
    next_pos.* = pos + 1;
    return 0xFFFD as u32;
}

/// Single-pass O(N) zero-allocation multilingual text analyzer.
/// Computes bytes, UTF-8 characters, CJK characters, Latin words (preserving apostrophe contractions
/// like `It's` and hyphenated compounds), total words (`latin_words + cjk_chars`), and lines.
pub fn analyze(text: string): TextCounts {
    if (text.len == 0) {
        return TextCounts{
            bytes: 0,
            chars: 0,
            words: 0,
            cjk_chars: 0,
            latin_words: 0,
            lines: 0,
        };
    }

    let chars: usize = 0;
    let cjkChars: usize = 0;
    let latinWords: usize = 0;
    let lines: usize = 1;
    let inWord = false;

    let pos: usize = 0;
    while (pos < text.len) {
        let nextPos: usize = pos + 1;
        const cp = wcNextCodepoint(text, pos, &nextPos);
        chars += 1;

        if (cp == ('\n' as u32)) {
            if (nextPos < text.len) {
                lines += 1;
            }
            inWord = false;
        } else if (isCjkCodepoint(cp)) {
            cjkChars += 1;
            inWord = false;
        } else if (wcIsLatinWordChar(cp)) {
            if (!inWord) {
                latinWords += 1;
                inWord = true;
            }
        } else if (inWord && (cp == ('\'' as u32) || cp == ('-' as u32)) && nextPos < text.len) {
            // Keep contraction/hyphenated word intact if immediately followed by a word character.
            let peekNext: usize = nextPos;
            const peekCp = wcNextCodepoint(text, nextPos, &peekNext);
            if (!wcIsLatinWordChar(peekCp)) {
                inWord = false;
            }
        } else {
            inWord = false;
        }

        pos = nextPos;
    }

    return TextCounts{
        bytes: text.len,
        chars: chars,
        words: latinWords + cjkChars,
        cjk_chars: cjkChars,
        latin_words: latinWords,
        lines: lines,
    };
}

/// Count total words (`latin_words + cjk_chars`) in `text`.
pub fn countWords(text: string): usize {
    return analyze(text).words;
}

/// Estimate reading time in seconds (rounded up) at `words_per_minute` (defaults to 200 if 0).
/// Returns `0` for empty or whitespace-only text.
pub fn estimateReadingSeconds(text: string, words_per_minute: u32): u32 {
    const w = countWords(text);
    if (w == 0) {
        return 0 as u32;
    }
    let wpm: u64 = words_per_minute as u64;
    if (wpm == (0 as u64)) {
        wpm = 200 as u64;
    }
    const secs: u64 = (((w as u64) * (60 as u64)) + wpm - (1 as u64)) / wpm;
    return secs as u32;
}

export {
    TextCounts,
    isCjkCodepoint,
    analyze,
    countWords,
    estimateReadingSeconds,
};
