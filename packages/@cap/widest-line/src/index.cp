// @cap/widest-line — Zero-allocation widest visual line calculator for CP
// Clean-room MIT implementation with parity to npm `widest-line`.
// Composes `@cap/string-width` for ANSI escape stripping and Unicode/CJK/Emoji column width.

import {
    StringWidthOptions,
    stringWidth,
    stringWidthWithOptions,
} from "../../string-width/src/index.cp";

fn wlTrimCr(line: string): string {
    if (line.len > 0 && line[line.len - 1] == (13 as u8)) {
        return line[0..line.len - 1];
    }
    return line;
}

fn wlLineWidthWithTab(line: string, tab_width: usize): usize {
    let col: usize = 0;
    let seg_start: usize = 0;
    let i: usize = 0;
    while (i < line.len) : (i += 1) {
        if (line[i] == (9 as u8)) {
            if (i > seg_start) {
                col += stringWidth(line[seg_start..i]);
            }
            if (tab_width > 0) {
                col += tab_width - (col % tab_width);
            }
            seg_start = i + 1;
        }
    }
    if (seg_start < line.len) {
        col += stringWidth(line[seg_start..line.len]);
    }
    return col;
}

/// Count the number of lines in `s` (`0` for empty string, otherwise `1 + count('\n')`).
pub fn countLines(s: string): usize {
    if (s.len == 0) {
        return 0;
    }
    let count: usize = 1;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == (10 as u8)) {
            count += 1;
        }
    }
    return count;
}

/// Return the visual column width of the widest line in `s` (handles `\n`, `\r\n`,
/// ANSI escape sequences, CJK double-width characters, and Emoji).
pub fn widestLine(s: string): usize {
    if (s.len == 0) {
        return 0;
    }
    let max_w: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == (10 as u8)) {
            const line = wlTrimCr(s[line_start..i]);
            const w = stringWidth(line);
            if (w > max_w) {
                max_w = w;
            }
            line_start = i + 1;
        }
    }
    const tail = wlTrimCr(s[line_start..s.len]);
    const tail_w = stringWidth(tail);
    if (tail_w > max_w) {
        max_w = tail_w;
    }
    return max_w;
}

/// Return the visual column width of the widest line in `s` using custom `StringWidthOptions`.
pub fn widestLineWithOptions(s: string, opts: StringWidthOptions): usize {
    if (s.len == 0) {
        return 0;
    }
    let max_w: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == (10 as u8)) {
            const line = wlTrimCr(s[line_start..i]);
            const w = stringWidthWithOptions(line, opts);
            if (w > max_w) {
                max_w = w;
            }
            line_start = i + 1;
        }
    }
    const tail = wlTrimCr(s[line_start..s.len]);
    const tail_w = stringWidthWithOptions(tail, opts);
    if (tail_w > max_w) {
        max_w = tail_w;
    }
    return max_w;
}

/// Compute visual column width for each line in `s` and write into `out`.
/// Returns the total number of lines in `s` (`0` for empty string).
/// If `out.len` is smaller than the line count, writes up to `out.len` entries safely.
pub fn lineWidths(s: string, out: []usize): usize {
    if (s.len == 0) {
        return 0;
    }
    let idx: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == (10 as u8)) {
            const line = wlTrimCr(s[line_start..i]);
            if (idx < out.len) {
                out[idx] = stringWidth(line);
            }
            idx += 1;
            line_start = i + 1;
        }
    }
    const tail = wlTrimCr(s[line_start..s.len]);
    if (idx < out.len) {
        out[idx] = stringWidth(tail);
    }
    return idx + 1;
}

/// Compute per-line visual widths using custom `StringWidthOptions`.
pub fn lineWidthsWithOptions(s: string, opts: StringWidthOptions, out: []usize): usize {
    if (s.len == 0) {
        return 0;
    }
    let idx: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == (10 as u8)) {
            const line = wlTrimCr(s[line_start..i]);
            if (idx < out.len) {
                out[idx] = stringWidthWithOptions(line, opts);
            }
            idx += 1;
            line_start = i + 1;
        }
    }
    const tail = wlTrimCr(s[line_start..s.len]);
    if (idx < out.len) {
        out[idx] = stringWidthWithOptions(tail, opts);
    }
    return idx + 1;
}

/// Return the widest visual line width in `s`, expanding `\t` characters to the next
/// `tab_width`-aligned column stop (`tab_width == 0` treats `\t` as 0 columns).
pub fn widestLineWithTab(s: string, tab_width: usize): usize {
    if (s.len == 0) {
        return 0;
    }
    let max_w: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == (10 as u8)) {
            const line = wlTrimCr(s[line_start..i]);
            const w = wlLineWidthWithTab(line, tab_width);
            if (w > max_w) {
                max_w = w;
            }
            line_start = i + 1;
        }
    }
    const tail = wlTrimCr(s[line_start..s.len]);
    const tail_w = wlLineWidthWithTab(tail, tab_width);
    if (tail_w > max_w) {
        max_w = tail_w;
    }
    return max_w;
}
