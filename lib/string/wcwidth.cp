// Source: Absorbed from /data/dev/bun/src/unicode/uucode_lib/src/x/config_x/wcwidth.cp | Synced: 2026-11-22T12:49:17Z
// lib/string/wcwidth.cp
// Unicode Monospace Display Column Width (`wcwidth` & `stringWidth` / `Bun.stringWidth` parity)
// Absorbed from Bun src/unicode/uucode_lib/src/x/config_x/wcwidth.cp

import { Utf8 } from "./utf.cp";

fn isZeroWidthCodepoint(cp: u32): bool {
    // C0 & C1 control characters (0x00..0x1F, 0x7F..0x9F)
    if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F)) {
        return true;
    }
    // Combining Diacritical Marks & Extended/Supplement/Symbols/Half Marks
    if ((cp >= 0x0300 && cp <= 0x036F) ||
        (cp >= 0x1AB0 && cp <= 0x1AFF) ||
        (cp >= 0x1DC0 && cp <= 0x1DFF) ||
        (cp >= 0x20D0 && cp <= 0x20FF) ||
        (cp >= 0xFE20 && cp <= 0xFE2F))
    {
        return true;
    }
    // Zero-width spaces, ZWJ/ZWNJ, bidi formatting, word joiners, BOM
    if ((cp >= 0x200B && cp <= 0x200F) ||
        (cp >= 0x202A && cp <= 0x202E) ||
        (cp >= 0x2060 && cp <= 0x206F) ||
        cp == 0xFEFF)
    {
        return true;
    }
    // Variation Selectors (VS1..VS16 and Supplemental)
    if ((cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0xE0100 && cp <= 0xE01EF)) {
        return true;
    }
    // Emoji skin-tone Fitzpatrick modifiers (zero advance when combined)
    if (cp >= 0x1F3FB && cp <= 0x1F3FF) {
        return true;
    }
    // Hangul Jamo medial vowels & trailing consonants
    if (cp >= 0x1160 && cp <= 0x11FF) {
        return true;
    }
    return false;
}

fn isWideCodepoint(cp: u32): bool {
    // Hangul Jamo leading consonants
    if (cp >= 0x1100 && cp <= 0x115F) {
        return true;
    }
    // Watch / Hourglass / Technical emoji
    if (cp == 0x231A || cp == 0x231B || cp == 0x2329 || cp == 0x232A || (cp >= 0x23E9 && cp <= 0x23F3)) {
        return true;
    }
    // Misc Symbols & Dingbats commonly rendered as 2-cell emoji
    if ((cp >= 0x2600 && cp <= 0x27BF) || (cp >= 0x2B50 && cp <= 0x2B55)) {
        return true;
    }
    // CJK Radicals, Ideographic Description, Punctuation, Hiragana, Katakana, Bopomofo,
    // CJK Unified Ideographs, Yi Syllables
    if ((cp >= 0x2E80 && cp <= 0x303E) ||
        (cp >= 0x3040 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE19) ||
        (cp >= 0xFE30 && cp <= 0xFE6F) ||
        (cp >= 0xFF01 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6))
    {
        return true;
    }
    // Regional Indicator Symbols (flags) & Main Emoji Blocks
    if ((cp >= 0x1F1E6 && cp <= 0x1F1FF) ||
        (cp >= 0x1F300 && cp <= 0x1F64F) ||
        (cp >= 0x1F680 && cp <= 0x1F6FF) ||
        (cp >= 0x1F900 && cp <= 0x1F9FF) ||
        (cp >= 0x1FA70 && cp <= 0x1FAFF))
    {
        return true;
    }
    // CJK Unified Ideographs Extension B..G & Compatibility Supplement
    if ((cp >= 0x20000 && cp <= 0x2FFFD) || (cp >= 0x30000 && cp <= 0x3FFFD)) {
        return true;
    }
    return false;
}

/// Returns the terminal monospace column width (`0`, `1`, or `2`) of a Unicode scalar code point.
/// Follows Bun's `uucode` wcwidth conventions:
/// - Control chars & combining/zero-width marks -> `0`
/// - Soft hyphen (`U+00AD`) & printable ASCII -> `1`
/// - East Asian Wide / Fullwidth CJK & Emoji -> `2`
pub fn wcwidth(cp: u32): u8 {
    if (cp == 0x00AD) {
        return 1;
    }
    if (cp >= 0x20 && cp <= 0x7E) {
        return 1;
    }
    if (isZeroWidthCodepoint(cp)) {
        return 0;
    }
    if (isWideCodepoint(cp)) {
        return 2;
    }
    return 1;
}

/// Computes the visual monospace terminal column width of a UTF-8 string (`Bun.stringWidth` parity).
/// When `count_ansi` is `false`, ANSI CSI (`\x1b[...m`) and OSC (`\x1b]...BEL/ST`) escape sequences are skipped.
pub fn stringWidth(s: []const u8, count_ansi: bool = false): usize {
    let width: usize = 0;
    let i: usize = 0;
    let after_zwj: bool = false;

    while (i < s.len) {
        const b = s[i];

        // Skip ANSI escape sequences unless count_ansi is true
        if (!count_ansi && b == 0x1B && i + 1 < s.len) {
            const next_b = s[i + 1];
            if (next_b == '[') {
                // CSI sequence: ESC [ ... final_byte (0x40..0x7E)
                i += 2;
                while (i < s.len) {
                    const c = s[i];
                    i += 1;
                    if (c >= 0x40 && c <= 0x7E) {
                        break;
                    }
                }
                continue;
            }
            if (next_b == ']') {
                // OSC sequence: ESC ] ... BEL (0x07) or ST (ESC \)
                i += 2;
                while (i < s.len) {
                    if (s[i] == 0x07) {
                        i += 1;
                        break;
                    }
                    if (s[i] == 0x1B && i + 1 < s.len && s[i + 1] == '\\') {
                        i += 2;
                        break;
                    }
                    i += 1;
                }
                continue;
            }
        }

        // Fast path for printable ASCII
        if (b >= 0x20 && b <= 0x7E) {
            width += 1;
            after_zwj = false;
            i += 1;
            continue;
        }

        if (b < 0x80) {
            // Control character (width 0)
            after_zwj = false;
            i += 1;
            continue;
        }

        const decoded = Utf8.decodeNext(s, i);
        const step: usize = if (decoded.bytes_consumed > 0) (decoded.bytes_consumed as usize) else 1;
        i += step;

        const cp = decoded.codepoint;
        if (cp == 0x200D) {
            // Zero Width Joiner: joins following emoji into the preceding grapheme cluster
            after_zwj = true;
            continue;
        }

        if (after_zwj) {
            // Joined emoji/symbol shares the visual cell of the base cluster
            after_zwj = false;
            continue;
        }

        width += wcwidth(cp) as usize;
    }

    return width;
}

/// Namespace facade mirroring `Bun.stringWidth` and `wcwidth`.
pub struct StringWidth {
    pub static fn codepoint(cp: u32): u8 {
        return wcwidth(cp);
    }

    pub static fn measure(s: []const u8): usize {
        return stringWidth(s, false);
    }

    pub static fn measureWithAnsi(s: []const u8, count_ansi: bool): usize {
        return stringWidth(s, count_ansi);
    }
}
