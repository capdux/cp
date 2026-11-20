// Source: cp/lib/text/tokens.cp (Fast LLM Token Budget Estimator & UTF-8 Safe Truncation)
// Pure CP zero-allocation LLM token estimator and UTF-8 safe prefix truncator,
// distinguishing ASCII words, numbers, punctuation, whitespace, and CJK codepoints.

const std = @import("std");

/// Summary statistics and estimated LLM token count for a text slice.
pub struct TextStats {
    bytes: usize = 0;
    codepoints: usize = 0;
    words: usize = 0;
    lines: usize = 0;
    cjk_chars: usize = 0;
    estimated_tokens: usize = 0;
}

struct TokenDecode {
    cp: u32 = 0;
    byte_len: usize = 1;
}

fn decodeCodepointAt(s: string, pos: usize): TokenDecode {
    if (pos >= s.len) return TokenDecode{ cp: 0, byte_len: 0 };
    const b0 = s[pos] as u32;
    if (b0 < 0x80) return TokenDecode{ cp: b0, byte_len: 1 };
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < s.len && (s[pos + 1] & 0xC0) == 0x80) {
        const cp = ((b0 & 0x1F) << 6) | ((s[pos + 1] & 0x3F) as u32);
        return TokenDecode{ cp: cp, byte_len: 2 };
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < s.len && (s[pos + 1] & 0xC0) == 0x80 && (s[pos + 2] & 0xC0) == 0x80) {
        const cp = ((b0 & 0x0F) << 12) | (((s[pos + 1] & 0x3F) as u32) << 6) | ((s[pos + 2] & 0x3F) as u32);
        return TokenDecode{ cp: cp, byte_len: 3 };
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < s.len && (s[pos + 1] & 0xC0) == 0x80 && (s[pos + 2] & 0xC0) == 0x80 && (s[pos + 3] & 0xC0) == 0x80) {
        const cp = ((b0 & 0x07) << 18) | (((s[pos + 1] & 0x3F) as u32) << 12) | (((s[pos + 2] & 0x3F) as u32) << 6) | ((s[pos + 3] & 0x3F) as u32);
        return TokenDecode{ cp: cp, byte_len: 4 };
    }
    return TokenDecode{ cp: b0, byte_len: 1 };
}

fn isCjkCodepoint(cp: u32): bool {
    // CJK Unified Ideographs + Extension A + Compatibility Ideographs
    if (cp >= 0x4E00 && cp <= 0x9FFF) return true;
    if (cp >= 0x3400 && cp <= 0x4DBF) return true;
    if (cp >= 0xF900 && cp <= 0xFAFF) return true;
    // CJK Symbols/Punctuation, Hiragana, Katakana, Bopomofo
    if (cp >= 0x3000 && cp <= 0x31BF) return true;
    // Hangul Syllables & Jamo
    if (cp >= 0xAC00 && cp <= 0xD7AF) return true;
    if (cp >= 0x1100 && cp <= 0x11FF) return true;
    // Fullwidth ASCII / Halfwidth Katakana
    if (cp >= 0xFF00 && cp <= 0xFFEF) return true;
    // CJK Unified Ideographs Extensions B..G
    if (cp >= 0x20000 && cp <= 0x2CEAF) return true;
    return false;
}

fn isWhitespaceCp(cp: u32): bool {
    return cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r' || cp == 0x0B || cp == 0x0C;
}

fn isAsciiLetterCp(cp: u32): bool {
    return (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z');
}

fn isAsciiDigitCp(cp: u32): bool {
    return cp >= '0' && cp <= '9';
}

struct StreamCounters {
    tokens: usize = 0;
    ascii_word_len: usize = 0;
    digit_len: usize = 0;
    ws_run: usize = 0;
    other_run: usize = 0;
    cjk_chars: usize = 0;
    words: usize = 0;
    lines: usize = 0;
    in_word: bool = false;

    fn step(*_, cp: u32): usize {
        if (cp == '\n') {
            _.lines += 1;
        }
        if (isWhitespaceCp(cp)) {
            _.in_word = false;
            _.ascii_word_len = 0;
            _.digit_len = 0;
            _.other_run = 0;
            if (cp == '\n') {
                _.ws_run = 0;
                _.tokens += 1;
            } else {
                _.ws_run += 1;
                if (_.ws_run > 1 && (_.ws_run % 4) == 2) {
                    _.tokens += 1;
                }
            }
            return _.tokens;
        }

        if (!_.in_word) {
            _.words += 1;
            _.in_word = true;
        }
        _.ws_run = 0;

        if (isAsciiLetterCp(cp)) {
            _.digit_len = 0;
            _.other_run = 0;
            _.ascii_word_len += 1;
            if ((_.ascii_word_len % 4) == 1) {
                _.tokens += 1;
            }
        } else if (isAsciiDigitCp(cp)) {
            _.ascii_word_len = 0;
            _.other_run = 0;
            _.digit_len += 1;
            if ((_.digit_len % 3) == 1) {
                _.tokens += 1;
            }
        } else if (cp < 0x80) {
            // ASCII punctuation / symbol
            _.ascii_word_len = 0;
            _.digit_len = 0;
            _.other_run = 0;
            _.tokens += 1;
        } else if (isCjkCodepoint(cp)) {
            _.ascii_word_len = 0;
            _.digit_len = 0;
            _.other_run = 0;
            _.cjk_chars += 1;
            _.tokens += 1;
        } else {
            _.ascii_word_len = 0;
            _.digit_len = 0;
            if (cp >= 0x1F000) {
                _.other_run = 0;
                _.tokens += 2;
            } else {
                _.other_run += 1;
                if ((_.other_run % 2) == 1) {
                    _.tokens += 1;
                }
            }
        }
        return _.tokens;
    }
}

/// Zero-allocation fast LLM token budget estimator and UTF-8 safe prefix truncator.
pub static struct TokenEstimator {
    /// Computes full text metrics (`bytes`, `codepoints`, `words`, `lines`, `cjk_chars`,
    /// and `estimated_tokens`) in a single O(n) pass with zero heap allocation.
    static fn analyze(text: string): TextStats {
        if (text.len == 0) return TextStats{};

        let st = StreamCounters{};
        st.lines = 1;
        let codepoints: usize = 0;
        let pos: usize = 0;

        while (pos < text.len) {
            const decoded = decodeCodepointAt(text, pos);
            if (decoded.byte_len == 0) break;
            * = st.step(decoded.cp);
            codepoints += 1;
            pos += decoded.byte_len;
        }

        return TextStats{
            bytes: text.len,
            codepoints: codepoints,
            words: st.words,
            lines: st.lines,
            cjk_chars: st.cjk_chars,
            estimated_tokens: st.tokens,
        };
    }

    /// Estimates LLM token count for `text` with zero heap allocation,
    /// distinguishing ASCII words, numbers, punctuation, whitespace, and CJK characters.
    static fn estimateTokens(text: string): usize {
        if (text.len == 0) return 0;
        let st = StreamCounters{};
        let pos: usize = 0;
        while (pos < text.len) {
            const decoded = decodeCodepointAt(text, pos);
            if (decoded.byte_len == 0) break;
            * = st.step(decoded.cp);
            pos += decoded.byte_len;
        }
        return st.tokens;
    }

    /// Truncates `text` to a zero-copy prefix whose estimated token count does not exceed
    /// `max_tokens`, guaranteed to cut strictly on a valid UTF-8 codepoint boundary.
    static fn truncateToTokenBudget(text: string, max_tokens: usize): string {
        if (text.len == 0 || max_tokens == 0) return text[0..0];

        let st = StreamCounters{};
        let pos: usize = 0;
        while (pos < text.len) {
            const decoded = decodeCodepointAt(text, pos);
            if (decoded.byte_len == 0) break;
            const next_tokens = st.step(decoded.cp);
            if (next_tokens > max_tokens) {
                return text[0..pos];
            }
            pos += decoded.byte_len;
        }
        return text;
    }
}
