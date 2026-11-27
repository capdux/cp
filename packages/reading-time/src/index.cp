// @cap/text-stats — Zero-allocation bilingual EN/CJK text statistics & reading-time engine
// Clean-room MIT implementation inspired by npm `reading-time` and `word-count`.

pub const DEFAULT_EN_WPM: usize = 200;
pub const DEFAULT_CJK_CPM: usize = 350;

pub struct TextStats {
    pub bytes: usize,
    pub chars: usize,
    pub ascii_words: usize,
    pub cjk_chars: usize,
    pub total_words: usize,
    pub lines: usize,
    pub non_empty_lines: usize,
    pub paragraphs: usize,
    pub sentences: usize,
    pub est_reading_sec: usize,
    pub est_reading_min: usize,
    pub est_llm_tokens: usize,

    pub static fn empty() -> TextStats {
        return TextStats{
            bytes: 0,
            chars: 0,
            ascii_words: 0,
            cjk_chars: 0,
            total_words: 0,
            lines: 0,
            non_empty_lines: 0,
            paragraphs: 0,
            sentences: 0,
            est_reading_sec: 0,
            est_reading_min: 0,
            est_llm_tokens: 0,
        };
    }
}

struct DecodedChar {
    cp: u32,
    len: usize,
}

fn decodeUtf8(s: string, pos: usize) -> DecodedChar {
    const b0 = s[pos];
    if (b0 < 0x80) {
        return DecodedChar{ cp: b0 as u32, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < s.len) {
        const b1 = s[pos + 1];
        if ((b1 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 2 };
        }
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < s.len) {
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x0F) as u32) << 12)
                | (((b1 & 0x3F) as u32) << 6)
                | ((b2 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 3 };
        }
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < s.len) {
        const b1 = s[pos + 1];
        const b2 = s[pos + 2];
        const b3 = s[pos + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp: u32 = (((b0 & 0x07) as u32) << 18)
                | (((b1 & 0x3F) as u32) << 12)
                | (((b2 & 0x3F) as u32) << 6)
                | ((b3 & 0x3F) as u32);
            return DecodedChar{ cp: cp, len: 4 };
        }
    }
    return DecodedChar{ cp: b0 as u32, len: 1 };
}

fn isCjkChar(cp: u32) -> bool {
    return (cp >= 0x4E00 && cp <= 0x9FFF)
        || (cp >= 0x3400 && cp <= 0x4DBF)
        || (cp >= 0xF900 && cp <= 0xFAFF)
        || (cp >= 0x20000 && cp <= 0x2A6DF)
        || (cp >= 0x3040 && cp <= 0x30FF)
        || (cp >= 0xAC00 && cp <= 0xD7AF);
}

fn isWordCoreChar(cp: u32) -> bool {
    if ((cp >= 'A' as u32 && cp <= 'Z' as u32)
        || (cp >= 'a' as u32 && cp <= 'z' as u32)
        || (cp >= '0' as u32 && cp <= '9' as u32))
    {
        return true;
    }
    // Latin-1 Supplement & Latin Extended letters
    if ((cp >= 0x00C0 && cp <= 0x00D6)
        || (cp >= 0x00D8 && cp <= 0x00F6)
        || (cp >= 0x00F8 && cp <= 0x024F))
    {
        return true;
    }
    return false;
}

fn isDigitByte(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

fn isWsCp(cp: u32) -> bool {
    return cp == ' ' as u32 || cp == '\t' as u32 || cp == '\r' as u32 || cp == '\n' as u32 || cp == 0x3000;
}

fn isSentenceEnd(cp: u32) -> bool {
    return cp == '.' as u32
        || cp == '!' as u32
        || cp == '?' as u32
        || cp == 0x3002 // 。
        || cp == 0xFF01 // ！
        || cp == 0xFF1F; // ？
}

// ---------------------------------------------------------------------------
// Core Analysis API
// ---------------------------------------------------------------------------

/// Analyzes `text` with custom English words-per-minute (`en_wpm`) and CJK chars-per-minute (`cjk_cpm`).
pub fn analyzeTextWithWpm(text: string, en_wpm: usize, cjk_cpm: usize) -> TextStats {
    if (text.len == 0) {
        return TextStats.empty();
    }

    const eff_en_wpm = if (en_wpm == 0) DEFAULT_EN_WPM else en_wpm;
    const eff_cjk_cpm = if (cjk_cpm == 0) DEFAULT_CJK_CPM else cjk_cpm;

    let chars: usize = 0;
    let ascii_words: usize = 0;
    let cjk_chars: usize = 0;
    let lines: usize = 0;
    let non_empty_lines: usize = 0;
    let paragraphs: usize = 0;
    let sentences: usize = 0;

    let in_word = false;
    let in_para = false;
    let cur_line_has_content = false;
    let has_word_in_sentence = false;

    let i: usize = 0;
    while (i < text.len) {
        const ch = decodeUtf8(text, i);
        const cp = ch.cp;
        chars += 1;

        if (cp == '\n' as u32) {
            lines += 1;
            if (cur_line_has_content) {
                non_empty_lines += 1;
                if (!in_para) {
                    paragraphs += 1;
                    in_para = true;
                }
            } else {
                in_para = false;
            }
            cur_line_has_content = false;
            in_word = false;
            i += ch.len;
            continue;
        }

        if (!isWsCp(cp)) {
            cur_line_has_content = true;
        }

        if (isCjkChar(cp)) {
            cjk_chars += 1;
            in_word = false;
            has_word_in_sentence = true;
        } else if (isWordCoreChar(cp)) {
            if (!in_word) {
                ascii_words += 1;
                in_word = true;
            }
            has_word_in_sentence = true;
        } else if ((cp == '\'' as u32 || cp == '-' as u32) && in_word && i + 1 < text.len) {
            // Keep contractions/hyphenated words (e.g. `don't`, `zero-alloc`) as a single word
            const next_ch = decodeUtf8(text, i + 1);
            if (!isWordCoreChar(next_ch.cp)) {
                in_word = false;
            }
        } else if (cp == '.' as u32 && in_word && i > 0 && i + 1 < text.len && isDigitByte(text[i - 1]) && isDigitByte(text[i + 1])) {
            // Keep decimal numbers like `3.14` as a single word and not a sentence end
        } else {
            in_word = false;
            if (isSentenceEnd(cp)) {
                if (has_word_in_sentence) {
                    sentences += 1;
                    has_word_in_sentence = false;
                }
            }
        }

        i += ch.len;
    }

    // Account for the final line if text did not end with '\n'
    if (text[text.len - 1] != '\n') {
        lines += 1;
        if (cur_line_has_content) {
            non_empty_lines += 1;
            if (!in_para) {
                paragraphs += 1;
            }
        }
    }

    if (has_word_in_sentence) {
        sentences += 1;
    }

    const total_words = ascii_words + cjk_chars;

    let est_reading_sec: usize = 0;
    let est_reading_min: usize = 0;
    if (total_words > 0) {
        const en_sec = (ascii_words as f64) * 60.0 / (eff_en_wpm as f64);
        const cjk_sec = (cjk_chars as f64) * 60.0 / (eff_cjk_cpm as f64);
        est_reading_sec = (en_sec + cjk_sec + 0.5) as usize;
        if (est_reading_sec == 0) {
            est_reading_sec = 1;
        }
        if (est_reading_sec >= 60) {
            est_reading_min = (est_reading_sec + 30) / 60;
        }
    }

    const est_llm_tokens = ((ascii_words * 4 + 2) / 3) + ((cjk_chars * 3 + 1) / 2);

    return TextStats{
        bytes: text.len,
        chars: chars,
        ascii_words: ascii_words,
        cjk_chars: cjk_chars,
        total_words: total_words,
        lines: lines,
        non_empty_lines: non_empty_lines,
        paragraphs: paragraphs,
        sentences: sentences,
        est_reading_sec: est_reading_sec,
        est_reading_min: est_reading_min,
        est_llm_tokens: est_llm_tokens,
    };
}

/// Analyzes `text` using default reading speeds (200 WPM English / 350 CPM CJK).
pub fn analyzeText(text: string) -> TextStats {
    return analyzeTextWithWpm(text, DEFAULT_EN_WPM, DEFAULT_CJK_CPM);
}

// ---------------------------------------------------------------------------
// Reading Time Formatter (Zero Heap Allocation)
// ---------------------------------------------------------------------------

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendUsize(out: []u8, pos: usize, val: usize) -> usize {
    if (val == 0) {
        return appendByte(out, pos, '0');
    }
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        digits[len] = ('0' as usize + (v % 10)) as u8;
        v /= 10;
        len += 1;
    }
    let p = pos;
    let k = len;
    while (k > 0) {
        k -= 1;
        p = appendByte(out, p, digits[k]);
    }
    return p;
}

/// Formats `stats` reading time into `out`.
/// - English (`zh = false`): `"< 1 min read"` or `"N min read"`
/// - Chinese (`zh = true`): `"不足 1 分钟"` or `"约 N 分钟读完"`
pub fn formatReadingTime(stats: TextStats, zh: bool, out: []u8) -> string {
    let pos: usize = 0;
    if (stats.est_reading_min == 0) {
        if (zh) {
            pos = appendStr(out, pos, "不足 1 分钟");
        } else {
            pos = appendStr(out, pos, "< 1 min read");
        }
        return out[0..pos];
    }

    if (zh) {
        pos = appendStr(out, pos, "约 ");
        pos = appendUsize(out, pos, stats.est_reading_min);
        pos = appendStr(out, pos, " 分钟读完");
    } else {
        pos = appendUsize(out, pos, stats.est_reading_min);
        pos = appendStr(out, pos, " min read");
    }
    return out[0..pos];
}
