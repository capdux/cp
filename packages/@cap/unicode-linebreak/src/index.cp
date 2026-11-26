// @cap/unicode-linebreak — Zero-allocation pure CP UAX #14 Unicode line break detector
// Parity with axelf4/unicode-linebreak crates.io:unicode-linebreak (Apache-2.0)

/// Whether a line break opportunity is mandatory or merely allowed.
pub enum BreakKind {
    Mandatory,
    Allowed,
}

/// A line break opportunity at byte `offset` in the input UTF-8 string.
pub struct LineBreak {
    offset: usize;
    kind: BreakKind;
}

const ULBR_AL: u8 = 0;
const ULBR_LF: u8 = 1;
const ULBR_CR: u8 = 2;
const ULBR_BK: u8 = 3;
const ULBR_SP: u8 = 4;
const ULBR_GL: u8 = 5;
const ULBR_OP: u8 = 6;
const ULBR_CL: u8 = 7;
const ULBR_IS: u8 = 8;
const ULBR_HY: u8 = 9;
const ULBR_ID: u8 = 10;

struct UlbrDecoded {
    cp: u32;
    len: usize;
}

fn ulbrDecodeUtf8(s: string, pos: usize): UlbrDecoded {
    const b0: u8 = s[pos];
    if (b0 < (0x80 as u8)) {
        return UlbrDecoded{ cp: b0 as u32, len: 1 };
    }
    if ((b0 & (0xE0 as u8)) == (0xC0 as u8) && pos + 1 < s.len) {
        const b1: u8 = s[pos + 1];
        const cp: u32 = (((b0 & (0x1F as u8)) as u32) << 6) | ((b1 & (0x3F as u8)) as u32);
        return UlbrDecoded{ cp: cp, len: 2 };
    }
    if ((b0 & (0xF0 as u8)) == (0xE0 as u8) && pos + 2 < s.len) {
        const b1: u8 = s[pos + 1];
        const b2: u8 = s[pos + 2];
        const cp: u32 = (((b0 & (0x0F as u8)) as u32) << 12)
            | (((b1 & (0x3F as u8)) as u32) << 6)
            | ((b2 & (0x3F as u8)) as u32);
        return UlbrDecoded{ cp: cp, len: 3 };
    }
    if ((b0 & (0xF8 as u8)) == (0xF0 as u8) && pos + 3 < s.len) {
        const b1: u8 = s[pos + 1];
        const b2: u8 = s[pos + 2];
        const b3: u8 = s[pos + 3];
        const cp: u32 = (((b0 & (0x07 as u8)) as u32) << 18)
            | (((b1 & (0x3F as u8)) as u32) << 12)
            | (((b2 & (0x3F as u8)) as u32) << 6)
            | ((b3 & (0x3F as u8)) as u32);
        return UlbrDecoded{ cp: cp, len: 4 };
    }
    return UlbrDecoded{ cp: 0xFFFD, len: 1 };
}

fn ulbrClassify(cp: u32): u8 {
    if (cp == 0x000A) {
        return ULBR_LF;
    }
    if (cp == 0x000D) {
        return ULBR_CR;
    }
    if (cp == 0x000B || cp == 0x000C || cp == 0x0085 || cp == 0x2028 || cp == 0x2029) {
        return ULBR_BK;
    }
    if (cp == 0x0020 || cp == 0x0009) {
        return ULBR_SP;
    }
    if (cp == 0x00A0 || cp == 0x202F || cp == 0x2060 || cp == 0xFEFF) {
        return ULBR_GL;
    }
    if (cp == 0x0028 || cp == 0x005B || cp == 0x007B || cp == 0xFF08 || cp == 0x3010 || cp == 0x300C || cp == 0x300E || cp == 0x300A || cp == 0x3008) {
        return ULBR_OP;
    }
    if (cp == 0x0029 || cp == 0x005D || cp == 0x007D || cp == 0xFF09 || cp == 0x3011 || cp == 0x300D || cp == 0x300F || cp == 0x300B || cp == 0x3009) {
        return ULBR_CL;
    }
    if (cp == 0x002C || cp == 0x002E || cp == 0x003B || cp == 0x003A || cp == 0x0021 || cp == 0x003F || cp == 0xFF0C || cp == 0x3002 || cp == 0x3001 || cp == 0xFF1B || cp == 0xFF1A || cp == 0xFF01 || cp == 0xFF1F) {
        return ULBR_IS;
    }
    if (cp == 0x002D || cp == 0x2010) {
        return ULBR_HY;
    }
    if ((cp >= 0x4E00 && cp <= 0x9FFF)
        || (cp >= 0x3400 && cp <= 0x4DBF)
        || (cp >= 0xF900 && cp <= 0xFAFF)
        || (cp >= 0x20000 && cp <= 0x2A6DF)
        || (cp >= 0x3040 && cp <= 0x30FF)
        || (cp >= 0xAC00 && cp <= 0xD7AF)) {
        return ULBR_ID;
    }
    return ULBR_AL;
}

// Returns 0 = no break, 1 = Allowed, 2 = Mandatory
fn ulbrEvalBreak(prev: u8, prevNonSp: u8, curr: u8): u8 {
    // LB5: CR × LF
    if (prev == ULBR_CR && curr == ULBR_LF) {
        return 0;
    }
    // LB4 / LB5: mandatory break after BK, CR, LF
    if (prev == ULBR_BK || prev == ULBR_CR || prev == ULBR_LF) {
        return 2;
    }
    // LB6: do not break before hard line breaks
    if (curr == ULBR_BK || curr == ULBR_CR || curr == ULBR_LF) {
        return 0;
    }
    // LB7: do not break before spaces
    if (curr == ULBR_SP) {
        return 0;
    }
    // LB12 / LB12a: do not break around non-breaking glue (U+00A0 etc.)
    if (prev == ULBR_GL || curr == ULBR_GL) {
        return 0;
    }
    // LB13: do not break before closing/infix punctuation
    if (curr == ULBR_CL || curr == ULBR_IS) {
        return 0;
    }
    // LB14: do not break after opening punctuation (even with intervening spaces)
    if (prev == ULBR_OP || (prev == ULBR_SP && prevNonSp == ULBR_OP)) {
        return 0;
    }
    // LB18: break after spaces
    if (prev == ULBR_SP) {
        return 1;
    }
    // LB21: break after hyphen (unless followed by another hyphen)
    if (prev == ULBR_HY && curr != ULBR_HY) {
        return 1;
    }
    // Ideographic (CJK): break allowed before or after ideographs
    if (prev == ULBR_ID || curr == ULBR_ID) {
        return 1;
    }
    return 0;
}

/// Find UAX #14 line break opportunities in `utf8` and write up to `out.len` entries into `out`.
/// Returns the number of `LineBreak` entries written.
pub fn findLineBreaks(utf8: string, out: []LineBreak): usize {
    if (utf8.len == 0) {
        return 0;
    }
    let written: usize = 0;
    const first = ulbrDecodeUtf8(utf8, 0);
    let prev: u8 = ulbrClassify(first.cp);
    let prevNonSp: u8 = if (prev == ULBR_SP) ULBR_AL else prev;
    let pos: usize = first.len;

    while (pos < utf8.len) {
        const decd = ulbrDecodeUtf8(utf8, pos);
        const curr: u8 = ulbrClassify(decd.cp);
        const brk: u8 = ulbrEvalBreak(prev, prevNonSp, curr);
        if (brk != (0 as u8) && written < out.len) {
            const kind: BreakKind = if (brk == (2 as u8)) BreakKind.Mandatory else BreakKind.Allowed;
            out[written] = LineBreak{ offset: pos, kind: kind };
            written += 1;
        }
        prev = curr;
        if (curr != ULBR_SP) {
            prevNonSp = curr;
        }
        pos += decd.len;
    }

    // LB3: Always break at the end of text (Mandatory)
    if (written < out.len) {
        out[written] = LineBreak{ offset: utf8.len, kind: BreakKind.Mandatory };
        written += 1;
    }
    return written;
}

/// Count total UAX #14 line break opportunities in `utf8`.
pub fn countLineBreaks(utf8: string): usize {
    if (utf8.len == 0) {
        return 0;
    }
    let count: usize = 0;
    const first = ulbrDecodeUtf8(utf8, 0);
    let prev: u8 = ulbrClassify(first.cp);
    let prevNonSp: u8 = if (prev == ULBR_SP) ULBR_AL else prev;
    let pos: usize = first.len;

    while (pos < utf8.len) {
        const decd = ulbrDecodeUtf8(utf8, pos);
        const curr: u8 = ulbrClassify(decd.cp);
        const brk: u8 = ulbrEvalBreak(prev, prevNonSp, curr);
        if (brk != (0 as u8)) {
            count += 1;
        }
        prev = curr;
        if (curr != ULBR_SP) {
            prevNonSp = curr;
        }
        pos += decd.len;
    }

    // LB3: Always break at the end of text
    count += 1;
    return count;
}
