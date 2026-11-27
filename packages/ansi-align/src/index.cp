// ansi-align — Zero-allocation ANSI-aware multiline text alignment for CP
// Clean-room ISC implementation with full parity to npm `ansi-align`.
// Composes `string-width` and `widest-line` with zero duplication.

import { stringWidth } from "../../string-width/src/index.cp";
import { widestLine } from "../../widest-line/src/index.cp";

/// Horizontal alignment mode for multiline terminal text.
pub enum AlignMode {
    Left,
    Center,
    Right,
}

/// Configuration options matching npm `ansi-align` (`mode`, `split`, `pad`).
pub struct AlignOptions {
    pub mode: AlignMode,
    pub split: string,
    pub pad: string,

    /// Default options: `Center`, `split = "\n"`, `pad = " "`.
    pub static fn default(): AlignOptions {
        return AlignOptions {
            mode: AlignMode.Center,
            split: "\n",
            pad: " ",
        };
    }
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

fn aaWriteRepeat(out: []u8, pos: usize, unit: string, count: usize): usize {
    const pad_unit = if (unit.len > 0) unit else " ";
    let p = pos;
    let c: usize = 0;
    while (c < count && p < out.len) : (c += 1) {
        p = aaCopyRange(out, p, pad_unit, 0, pad_unit.len);
    }
    return p;
}

fn aaMatchAt(text: string, pos: usize, sep: string): bool {
    if (sep.len == 0 || pos + sep.len > text.len) {
        return false;
    }
    let j: usize = 0;
    while (j < sep.len) : (j += 1) {
        if (text[pos + j] != sep[j]) {
            return false;
        }
    }
    return true;
}

/// Returns the ANSI-stripped visible column width of a single line.
pub fn ansiVisibleWidth(s: string): usize {
    return stringWidth(s);
}

/// Computes the maximum visible column width across lines delimited by `split` (defaults to `"\n"`).
pub fn ansiMaxLineWidth(text: string, split: string): usize {
    if (text.len == 0) {
        return 0;
    }
    const sep = if (split.len > 0) split else "\n";
    if (sep.len == 1 && sep[0] == (10 as u8)) {
        return widestLine(text);
    }
    let max_w: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= text.len) {
        if (i == text.len || aaMatchAt(text, i, sep)) {
            const w = stringWidth(text[line_start..i]);
            if (w > max_w) {
                max_w = w;
            }
            if (i == text.len) {
                break;
            }
            i += sep.len;
            line_start = i;
        } else {
            i += 1;
        }
    }
    return max_w;
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

/// Full npm `ansi-align` options alignment supporting custom `split` delimiter and `pad` string.
pub fn alignAnsiWithOptions(text: string, opts: AlignOptions, out: []u8): usize {
    const total = text.len;
    if (total == 0) {
        return 0;
    }
    if (opts.mode == AlignMode.Left) {
        return aaCopyRange(out, 0, text, 0, total);
    }
    const sep = if (opts.split.len > 0) opts.split else "\n";
    const pad_str = if (opts.pad.len > 0) opts.pad else " ";

    // Fast path for default "\n" + " "
    if (sep.len == 1 && sep[0] == (10 as u8) && pad_str.len == 1 && pad_str[0] == (32 as u8)) {
        return alignAnsi(text, opts.mode, out);
    }

    const max_w = ansiMaxLineWidth(text, sep);
    let p: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= total) {
        if (i == total || aaMatchAt(text, i, sep)) {
            let line_end = i;
            let has_cr = false;
            if (sep.len == 1 && sep[0] == (10 as u8) && line_end > line_start && text[line_end - 1] == (13 as u8)) {
                line_end -= 1;
                has_cr = true;
            }

            if (line_end > line_start) {
                const w = stringWidth(text[line_start..line_end]);
                if (max_w > w) {
                    const diff = max_w - w;
                    const pad_cnt: usize = if (opts.mode == AlignMode.Center) diff / 2 else diff;
                    if (pad_cnt > 0) {
                        p = aaWriteRepeat(out, p, pad_str, pad_cnt);
                    }
                }
                p = aaCopyRange(out, p, text, line_start, line_end);
            }

            if (has_cr && p < out.len) {
                out[p] = 13 as u8;
                p += 1;
            }
            if (i < total) {
                p = aaCopyRange(out, p, sep, 0, sep.len);
                i += sep.len;
                line_start = i;
            } else {
                break;
            }
        } else {
            i += 1;
        }
    }
    return p;
}

/// Align a slice of pre-split lines with custom `pad` and `split` joiner into `out`.
pub fn alignAnsiLines(lines: []const string, mode: AlignMode, pad: string, split: string, out: []u8): usize {
    if (lines.len == 0) {
        return 0;
    }
    const sep = if (split.len > 0) split else "\n";
    const pad_str = if (pad.len > 0) pad else " ";

    let max_w: usize = 0;
    if (mode != AlignMode.Left) {
        let idx: usize = 0;
        while (idx < lines.len) : (idx += 1) {
            const w = stringWidth(lines[idx]);
            if (w > max_w) {
                max_w = w;
            }
        }
    }

    let p: usize = 0;
    let idx: usize = 0;
    while (idx < lines.len) : (idx += 1) {
        if (idx > 0) {
            p = aaCopyRange(out, p, sep, 0, sep.len);
        }
        const line = lines[idx];
        if (line.len > 0 && mode != AlignMode.Left) {
            const w = stringWidth(line);
            if (max_w > w) {
                const diff = max_w - w;
                const pad_cnt: usize = if (mode == AlignMode.Center) diff / 2 else diff;
                if (pad_cnt > 0) {
                    p = aaWriteRepeat(out, p, pad_str, pad_cnt);
                }
            }
        }
        p = aaCopyRange(out, p, line, 0, line.len);
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

export {
    AlignMode,
    AlignOptions,
    ansiVisibleWidth,
    ansiMaxLineWidth,
    alignAnsi,
    alignAnsiWithOptions,
    alignAnsiLines,
    alignCenter,
    alignRight,
    alignLeft,
};
