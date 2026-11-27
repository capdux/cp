// @cap/unicode-bidi — Zero-allocation Unicode Bidirectional Algorithm (UAX #9) core
// MIT License

pub const BIDI_L: u8 = 0;   // Left-to-Right (Latin, Greek, Cyrillic, CJK)
pub const BIDI_R: u8 = 1;   // Right-to-Left (Hebrew)
pub const BIDI_AL: u8 = 2;  // Arabic Letter
pub const BIDI_EN: u8 = 3;  // European Number (0-9)
pub const BIDI_AN: u8 = 4;  // Arabic-Indic Number (0x0660..0x0669)
pub const BIDI_WS: u8 = 5;  // Whitespace
pub const BIDI_ON: u8 = 6;  // Other Neutral
pub const BIDI_B: u8 = 7;   // Paragraph Separator

pub const DIR_LTR: u8 = 0;
pub const DIR_RTL: u8 = 1;

pub struct BidiRun {
    pub start: usize,
    pub len: usize,
    pub level: u8,
}

struct Utf8Step {
    cp: u32,
    len: usize,
}

fn decodeUtf8At(text: string, pos: usize): Utf8Step {
    let b0: u8 = text[pos];
    if (b0 < 0x80) {
        return Utf8Step { cp: b0 as u32, len: 1 };
    }
    if ((b0 & 0xE0) == 0xC0 && pos + 1 < text.len) {
        let b1: u8 = text[pos + 1];
        if ((b1 & 0xC0) == 0x80) {
            let cp: u32 = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
            return Utf8Step { cp: cp, len: 2 };
        }
    }
    if ((b0 & 0xF0) == 0xE0 && pos + 2 < text.len) {
        let b1: u8 = text[pos + 1];
        let b2: u8 = text[pos + 2];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            let cp: u32 = (((b0 & 0x0F) as u32) << 12) |
                (((b1 & 0x3F) as u32) << 6) |
                ((b2 & 0x3F) as u32);
            return Utf8Step { cp: cp, len: 3 };
        }
    }
    if ((b0 & 0xF8) == 0xF0 && pos + 3 < text.len) {
        let b1: u8 = text[pos + 1];
        let b2: u8 = text[pos + 2];
        let b3: u8 = text[pos + 3];
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            let cp: u32 = (((b0 & 0x07) as u32) << 18) |
                (((b1 & 0x3F) as u32) << 12) |
                (((b2 & 0x3F) as u32) << 6) |
                ((b3 & 0x3F) as u32);
            return Utf8Step { cp: cp, len: 4 };
        }
    }
    return Utf8Step { cp: 0xFFFD, len: 1 };
}

pub fn bidiClass(cp: u32): u8 {
    // Paragraph separators
    if (cp == 0x0A || cp == 0x0D || (cp >= 0x1C && cp <= 0x1E) || cp == 0x85 || cp == 0x2029) {
        return BIDI_B;
    }
    // Whitespace
    if (cp == 0x20 || cp == 0x09 || cp == 0x0C || cp == 0x1680 ||
        (cp >= 0x2000 && cp <= 0x200A) || cp == 0x2028 || cp == 0x205F || cp == 0x3000) {
        return BIDI_WS;
    }
    // European digits 0-9
    if (cp >= 0x30 && cp <= 0x39) {
        return BIDI_EN;
    }
    // ASCII Latin letters
    if ((cp >= 0x41 && cp <= 0x5A) || (cp >= 0x61 && cp <= 0x7A)) {
        return BIDI_L;
    }
    // Remaining ASCII punctuation & controls -> Other Neutral
    if (cp < 0x80) {
        return BIDI_ON;
    }
    // Arabic-Indic digits (checked before general Arabic block)
    if (cp >= 0x0660 && cp <= 0x0669) {
        return BIDI_AN;
    }
    // Hebrew (R)
    if ((cp >= 0x0590 && cp <= 0x05FF) || (cp >= 0xFB1D && cp <= 0xFB4F) || cp == 0x200F) {
        return BIDI_R;
    }
    // Arabic (AL)
    if ((cp >= 0x0600 && cp <= 0x06FF) || (cp >= 0x0750 && cp <= 0x077F) ||
        (cp >= 0x08A0 && cp <= 0x08FF) || (cp >= 0xFB50 && cp <= 0xFDFF) ||
        (cp >= 0xFE70 && cp <= 0xFEFF)) {
        return BIDI_AL;
    }
    // Extended Latin, Greek, Cyrillic, CJK, Kana, Hangul -> Strong LTR
    if ((cp >= 0x00C0 && cp <= 0x02AF) || (cp >= 0x0370 && cp <= 0x052F) ||
        (cp >= 0x3040 && cp <= 0x30FF) || (cp >= 0x4E00 && cp <= 0x9FFF) ||
        (cp >= 0xAC00 && cp <= 0xD7AF) || cp == 0x200E) {
        return BIDI_L;
    }
    return BIDI_ON;
}

pub fn baseDirection(text: string): u8 {
    let pos: usize = 0;
    while (pos < text.len) {
        let step: Utf8Step = decodeUtf8At(text, pos);
        let cls: u8 = bidiClass(step.cp);
        if (cls == BIDI_L) {
            return DIR_LTR;
        }
        if (cls == BIDI_R || cls == BIDI_AL) {
            return DIR_RTL;
        }
        pos = pos + step.len;
    }
    return DIR_LTR;
}

pub fn hasRtl(text: string): bool {
    let pos: usize = 0;
    while (pos < text.len) {
        let step: Utf8Step = decodeUtf8At(text, pos);
        let cls: u8 = bidiClass(step.cp);
        if (cls == BIDI_R || cls == BIDI_AL) {
            return true;
        }
        pos = pos + step.len;
    }
    return false;
}

pub fn isLtrOnly(text: string): bool {
    return !hasRtl(text);
}

fn resolveCharLevel(cls: u8, base_level: u8, prev_level: u8): u8 {
    if (cls == BIDI_L) {
        if (base_level == DIR_RTL) {
            return 2; // Even (LTR) embedded in RTL
        }
        return 0;
    }
    if (cls == BIDI_R || cls == BIDI_AL) {
        return 1; // Odd (RTL)
    }
    if (cls == BIDI_EN || cls == BIDI_AN) {
        if (base_level == DIR_RTL) {
            return 2; // Numbers run LTR inside RTL
        }
        return 0;
    }
    if (cls == BIDI_B) {
        return base_level;
    }
    return prev_level;
}

pub fn computeVisualRuns(text: string, out_runs: []BidiRun): usize {
    if (text.len == 0 || out_runs.len == 0) {
        return 0;
    }
    let base_level: u8 = baseDirection(text);
    let pos: usize = 0;
    let run_count: usize = 0;

    let cur_start: usize = 0;
    let cur_len: usize = 0;
    let cur_level: u8 = base_level;
    let has_run: bool = false;

    while (pos < text.len) {
        let step: Utf8Step = decodeUtf8At(text, pos);
        let cls: u8 = bidiClass(step.cp);
        let prev_lvl: u8 = base_level;
        if (has_run) {
            prev_lvl = cur_level;
        }
        let lvl: u8 = resolveCharLevel(cls, base_level, prev_lvl);

        if (!has_run) {
            cur_start = pos;
            cur_len = step.len;
            cur_level = lvl;
            has_run = true;
        } else if (lvl == cur_level) {
            cur_len = cur_len + step.len;
        } else {
            if (run_count < out_runs.len) {
                out_runs[run_count] = BidiRun {
                    start: cur_start,
                    len: cur_len,
                    level: cur_level,
                };
                run_count = run_count + 1;
            } else {
                return run_count;
            }
            cur_start = pos;
            cur_len = step.len;
            cur_level = lvl;
        }
        pos = pos + step.len;
    }

    if (has_run && run_count < out_runs.len) {
        out_runs[run_count] = BidiRun {
            start: cur_start,
            len: cur_len,
            level: cur_level,
        };
        run_count = run_count + 1;
    }
    return run_count;
}

pub fn mirrorAsciiBracket(ch: u8): u8 {
    if (ch == 40) { return 41; }   // '(' -> ')'
    if (ch == 41) { return 40; }   // ')' -> '('
    if (ch == 91) { return 93; }   // '[' -> ']'
    if (ch == 93) { return 91; }   // ']' -> '['
    if (ch == 123) { return 125; } // '{' -> '}'
    if (ch == 125) { return 123; } // '}' -> '{'
    if (ch == 60) { return 62; }   // '<' -> '>'
    if (ch == 62) { return 60; }   // '>' -> '<'
    return ch;
}
