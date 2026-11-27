// @cap/emoji-regex — Zero-allocation Unicode RGI Emoji UTF-8 scanner in pure CP
// Clean-room MIT implementation inspired by npm `emoji-regex`.

pub struct EmojiSpan {
    start: usize,
    end: usize,
}

struct EmjDecoded {
    cp: u32,
    len: usize,
}

fn emjDecodeUtf8(s: string, pos: usize): EmjDecoded {
    if (pos >= s.len) {
        return EmjDecoded { cp: 0, len: 0 };
    }
    const b0: u32 = s[pos] as u32;
    if (b0 < 0x80) {
        return EmjDecoded { cp: b0, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < s.len) {
        const b1: u32 = s[pos + 1] as u32;
        if ((b1 & 0xC0) == 0x80) {
            const cp: u32 = ((b0 & 0x1F) << 6) | (b1 & 0x3F);
            return EmjDecoded { cp: cp, len: 2 };
        }
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < s.len) {
        const b1: u32 = s[pos + 1] as u32;
        const b2: u32 = s[pos + 2] as u32;
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            const cp: u32 = ((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F);
            return EmjDecoded { cp: cp, len: 3 };
        }
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < s.len) {
        const b1: u32 = s[pos + 1] as u32;
        const b2: u32 = s[pos + 2] as u32;
        const b3: u32 = s[pos + 3] as u32;
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            const cp: u32 = ((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F);
            return EmjDecoded { cp: cp, len: 4 };
        }
    }
    return EmjDecoded { cp: 0, len: 1 };
}

fn emjIsRegionalIndicator(cp: u32): bool {
    return cp >= 0x1F1E6 && cp <= 0x1F1FF;
}

fn emjIsSkinTone(cp: u32): bool {
    return cp >= 0x1F3FB && cp <= 0x1F3FF;
}

fn emjIsKeycapBase(cp: u32): bool {
    return cp == 0x0023 || cp == 0x002A || (cp >= 0x0030 && cp <= 0x0039);
}

fn emjIsTagChar(cp: u32): bool {
    return cp >= 0xE0020 && cp <= 0xE007E;
}

/// Returns `true` if `cp` is a Unicode Emoji code point (pictographs, misc symbols,
/// dingbats, regional indicators, skin tone modifiers, and common standalone emoji).
pub fn isEmojiCodePoint(cp: u32): bool {
    // Main Emoji blocks (Misc Symbols & Pictographs, Emoticons, Transport, Supplemental, Extended-A)
    // Also includes Skin Tone Modifiers (0x1F3FB..=0x1F3FF)
    if (cp >= 0x1F300 && cp <= 0x1FAFF) {
        return true;
    }
    // Misc Symbols & Dingbats
    if (cp >= 0x2600 && cp <= 0x27BF) {
        return true;
    }
    // Regional Indicator Symbols (flags)
    if (cp >= 0x1F1E6 && cp <= 0x1F1FF) {
        return true;
    }
    // Enclosed Alphanumeric & Ideographic Emoji Supplement
    if (cp == 0x1F004 || cp == 0x1F0CF) {
        return true;
    }
    if ((cp >= 0x1F170 && cp <= 0x1F18E) || (cp >= 0x1F191 && cp <= 0x1F19A)) {
        return true;
    }
    if (cp == 0x1F201 || cp == 0x1F202 || cp == 0x1F21A || cp == 0x1F22F ||
        (cp >= 0x1F232 && cp <= 0x1F23A) || cp == 0x1F250 || cp == 0x1F251) {
        return true;
    }
    // Common standalone BMP Emoji
    if (cp == 0x00A9 || cp == 0x00AE || cp == 0x203C || cp == 0x2049 ||
        cp == 0x2122 || cp == 0x2139 || cp == 0x2328 || cp == 0x23CF ||
        cp == 0x24C2 || cp == 0x25B6 || cp == 0x25C0 || cp == 0x2B50 ||
        cp == 0x2B55 || cp == 0x3030 || cp == 0x303D || cp == 0x3297 ||
        cp == 0x3299) {
        return true;
    }
    if ((cp >= 0x2194 && cp <= 0x2199) || (cp >= 0x21A9 && cp <= 0x21AA) ||
        (cp >= 0x231A && cp <= 0x231B) || (cp >= 0x23E9 && cp <= 0x23F3) ||
        (cp >= 0x23F8 && cp <= 0x23FA) || (cp >= 0x25AA && cp <= 0x25AB) ||
        (cp >= 0x25FB && cp <= 0x25FE) || (cp >= 0x2934 && cp <= 0x2935) ||
        (cp >= 0x2B05 && cp <= 0x2B07) || (cp >= 0x2B1B && cp <= 0x2B1C)) {
        return true;
    }
    return false;
}

/// Consume optional VS16 (`U+FE0F`) and/or Emoji Skin Tone Modifier (`U+1F3FB..U+1F3FF`)
/// immediately following an emoji base code point.
fn emjConsumeModifiers(s: string, start_pos: usize): usize {
    let cur: usize = start_pos;
    // Optional Variation Selector-16 (U+FE0F)
    if (cur < s.len) {
        const d1: EmjDecoded = emjDecodeUtf8(s, cur);
        if (d1.cp == 0xFE0F) {
            cur += d1.len;
        }
    }
    // Optional Skin Tone Modifier
    if (cur < s.len) {
        const d2: EmjDecoded = emjDecodeUtf8(s, cur);
        if (emjIsSkinTone(d2.cp)) {
            cur += d2.len;
            if (cur < s.len) {
                const d3: EmjDecoded = emjDecodeUtf8(s, cur);
                if (d3.cp == 0xFE0F) {
                    cur += d3.len;
                }
            }
        }
    }
    return cur;
}

/// Find the next Unicode Emoji span in UTF-8 string `s` starting at byte offset `from_pos`.
/// Supports ZWJ (`U+200D`) sequences, Variation Selector-16 (`U+FE0F`), Skin Tone Modifiers,
/// Keycap sequences (`[0-9#*] + FE0F? + 20E3`), Flag pairs, and Tag sequences.
pub fn findNextEmoji(s: string, from_pos: usize): EmojiSpan? {
    let pos: usize = from_pos;
    while (pos < s.len) {
        const d0: EmjDecoded = emjDecodeUtf8(s, pos);
        if (d0.len == 0) {
            break;
        }
        const span_start: usize = pos;

        // 1. Keycap sequence: [0-9#*] + optional U+FE0F + U+20E3
        if (emjIsKeycapBase(d0.cp)) {
            let cur: usize = pos + d0.len;
            if (cur < s.len) {
                const d1: EmjDecoded = emjDecodeUtf8(s, cur);
                if (d1.cp == 0xFE0F) {
                    cur += d1.len;
                }
                if (cur < s.len) {
                    const d2: EmjDecoded = emjDecodeUtf8(s, cur);
                    if (d2.cp == 0x20E3) {
                        return EmojiSpan { start: span_start, end: cur + d2.len };
                    }
                }
            }
            pos += d0.len;
            continue;
        }

        // 2. Regional Indicator pair (Flag sequence)
        if (emjIsRegionalIndicator(d0.cp)) {
            let cur: usize = pos + d0.len;
            if (cur < s.len) {
                const d1: EmjDecoded = emjDecodeUtf8(s, cur);
                if (emjIsRegionalIndicator(d1.cp)) {
                    cur += d1.len;
                }
            }
            return EmojiSpan { start: span_start, end: cur };
        }

        // 3. Standard Emoji code point (+ modifiers + tag sequence + ZWJ chains)
        if (isEmojiCodePoint(d0.cp)) {
            let cur: usize = emjConsumeModifiers(s, pos + d0.len);

            // Optional subdivision flag tag sequence (e.g. U+1F3F4 + tags + U+E007F)
            if (d0.cp == 0x1F3F4 && cur < s.len) {
                let tag_cur: usize = cur;
                let tag_count: usize = 0;
                while (tag_cur < s.len) {
                    const dt: EmjDecoded = emjDecodeUtf8(s, tag_cur);
                    if (emjIsTagChar(dt.cp)) {
                        tag_cur += dt.len;
                        tag_count += 1;
                    } else if (dt.cp == 0xE007F && tag_count > 0) {
                        cur = tag_cur + dt.len;
                        break;
                    } else {
                        break;
                    }
                }
            }

            // ZWJ (U+200D) chained emoji sequence
            while (cur < s.len) {
                const dz: EmjDecoded = emjDecodeUtf8(s, cur);
                if (dz.cp != 0x200D) {
                    break;
                }
                const after_zwj: usize = cur + dz.len;
                if (after_zwj >= s.len) {
                    break;
                }
                const dn: EmjDecoded = emjDecodeUtf8(s, after_zwj);
                if (!isEmojiCodePoint(dn.cp)) {
                    break;
                }
                cur = emjConsumeModifiers(s, after_zwj + dn.len);
            }

            return EmojiSpan { start: span_start, end: cur };
        }

        pos += d0.len;
    }
    return null;
}

/// Count total emoji sequences in `s`.
pub fn countEmojis(s: string): usize {
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        if (span := findNextEmoji(s, pos)) {
            count += 1;
            pos = span.end;
        } else {
            break;
        }
    }
    return count;
}

/// Returns `true` if `s` contains at least one emoji sequence.
pub fn containsEmoji(s: string): bool {
    return findNextEmoji(s, 0) != null;
}

/// Copy `s` into `out` with all emoji sequences stripped.
/// Returns the number of bytes written to `out` (truncated safely if `out` is smaller).
pub fn stripEmojis(s: string, out: []u8): usize {
    let written: usize = 0;
    let pos: usize = 0;
    while (pos < s.len) {
        if (span := findNextEmoji(s, pos)) {
            let i: usize = pos;
            while (i < span.start && written < out.len) : (i += 1) {
                out[written] = s[i];
                written += 1;
            }
            pos = span.end;
        } else {
            let i: usize = pos;
            while (i < s.len && written < out.len) : (i += 1) {
                out[written] = s[i];
                written += 1;
            }
            break;
        }
    }
    return written;
}

export {
    EmojiSpan,
    isEmojiCodePoint,
    findNextEmoji,
    countEmojis,
    containsEmoji,
    stripEmojis,
};
