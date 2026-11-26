// @cap/ansi-align — Zero-allocation ANSI-aware multiline text alignment for CP
// Clean-room ISC implementation with parity to npm `ansi-align`.
// Composes `@cap/string-width` and `@cap/widest-line` with zero duplication.

import { stringWidth } from "../../string-width/src/index.cp";
import { widestLine } from "../../widest-line/src/index.cp";

/// Horizontal alignment mode for multiline terminal text.
pub enum AlignMode {
    Left,
    Center,
    Right,
}

fn aaWriteSpaces(out: []u8, pos: usize, count: usize): usize {
    let p = pos;
    let c: usize = 0;
    while (c < count && p < out.len) : (c += 1) {
        out[p] = 32 as u8;
        p += 1;
    }
    return p;
}

fn aaCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize {
    let p = pos;
    let i = start;
    while (i < end && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

/// Align multiline `text` according to `mode` (`Left`, `Center`, or `Right`) based on
/// visual column width (handling ANSI escape codes, CJK double-width characters, Emoji,
/// and `\n`/`\r\n` line endings). Writes into `out` and returns bytes written.
pub fn alignAnsi(text: string, mode: AlignMode, out: []u8): usize {
    const total = text.len;
    if (total == 0) {
        return 0;
    }
    if (mode == AlignMode.Left) {
        return aaCopyRange(out, 0, text, 0, total);
    }

    const max_w = widestLine(text);
    let p: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= total) : (i += 1) {
        if (i == total || text[i] == (10 as u8)) {
            let line_end = i;
            let has_cr = false;
            if (line_end > line_start && text[line_end - 1] == (13 as u8)) {
                line_end -= 1;
                has_cr = true;
            }

            if (line_end > line_start) {
                const w = stringWidth(text[line_start..line_end]);
                if (max_w > w) {
                    const diff = max_w - w;
                    const pad: usize = if (mode == AlignMode.Center) diff / 2 else diff;
                    if (pad > 0) {
                        p = aaWriteSpaces(out, p, pad);
                    }
                }
                p = aaCopyRange(out, p, text, line_start, line_end);
            }

            if (has_cr && p < out.len) {
                out[p] = 13 as u8;
                p += 1;
            }
            if (i < total && p < out.len) {
                out[p] = 10 as u8;
                p += 1;
            }
            line_start = i + 1;
        }
    }
    return p;
}

/// Center-align multiline `text` by visual column width into `out`.
pub fn alignCenter(text: string, out: []u8): usize {
    return alignAnsi(text, AlignMode.Center, out);
}

/// Right-align multiline `text` by visual column width into `out`.
pub fn alignRight(text: string, out: []u8): usize {
    return alignAnsi(text, AlignMode.Right, out);
}

/// Left-align multiline `text` into `out` (verbatim copy preserving line endings).
pub fn alignLeft(text: string, out: []u8): usize {
    return alignAnsi(text, AlignMode.Left, out);
}
