// ansi-align — Zero-allocation ANSI-aware multiline text alignment for CP
// Clean-room ISC implementation with full parity to npm `ansi-align`.
// Composes `string-width` and `widest-line` with zero duplication.
// Accelerated with 16-lane @Vector(16, u8) SIMD delimiter scanning and bulk @memcpy.

import { stringWidth } from "../../string-width/src/index.cp";
import { widestLine } from "../../widest-line/src/index.cp";

type AaVec16 = @Vector(16, u8);

const AA_SPACES_64: string = "                                                                ";

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

fn aaFindByteSimd(s: string, target: u8, start: usize): usize {
    let i = start;
    const len = s.len;
    const vt: AaVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: AaVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

fn aaWriteSpaces(out: []u8, pos: usize, count: usize): usize {
    if (pos >= out.len || count == 0) {
        return pos;
    }
    const avail = out.len - pos;
    let rem = if (count < avail) count else avail;
    let p = pos;
    while (rem >= 64) {
        @memcpy(out[p .. p + 64], AA_SPACES_64[0..64]);
        p += 64;
        rem -= 64;
    }
    if (rem > 0) {
        @memcpy(out[p .. p + rem], AA_SPACES_64[0..rem]);
        p += rem;
    }
    return p;
}

fn aaCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize {
    if (pos >= out.len || end <= start) {
        return pos;
    }
    const avail = out.len - pos;
    const len = end - start;
    const n = if (len < avail) len else avail;
    @memcpy(out[pos .. pos + n], s[start .. start + n]);
    return pos + n;
}

fn aaWriteRepeat(out: []u8, pos: usize, unit: string, count: usize): usize {
    const pad_unit = if (unit.len > 0) unit else " ";
    if (pad_unit.len == 1 && pad_unit[0] == (32 as u8)) {
        return aaWriteSpaces(out, pos, count);
    }
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

fn aaFindSepSimd(text: string, sep: string, start: usize): usize {
    const first = sep[0];
    let i = start;
    while (i < text.len) {
        const hit = aaFindByteSimd(text, first, i);
        if (hit >= text.len) {
            return text.len;
        }
        if (sep.len == 1 || aaMatchAt(text, hit, sep)) {
            return hit;
        }
        i = hit + 1;
    }
    return text.len;
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
    while (true) {
        const i = aaFindSepSimd(text, sep, line_start);
        const w = stringWidth(text[line_start..i]);
        if (w > max_w) {
            max_w = w;
        }
        if (i >= text.len) {
            break;
        }
        line_start = i + sep.len;
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
    while (true) {
        const i = aaFindByteSimd(text, 10 as u8, line_start);
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
        if (i < total) {
            if (p < out.len) {
                out[p] = 10 as u8;
                p += 1;
            }
            line_start = i + 1;
        } else {
            break;
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
    while (true) {
        const i = aaFindSepSimd(text, sep, line_start);
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
            line_start = i + sep.len;
        } else {
            break;
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
