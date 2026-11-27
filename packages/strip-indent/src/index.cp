// @cap/strip-indent — Zero-allocation leading indentation stripper for CP
// Clean-room MIT implementation with parity to npm `strip-indent` + `min-indent`.

fn siIsIndentChar(ch: u8): bool {
    return ch == (32 as u8) || ch == (9 as u8);
}

fn siCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize {
    let p = pos;
    let i = start;
    while (i < end && p < out.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

/// Find the minimum number of leading spaces/tabs across all non-empty lines in `s`.
/// Returns `0` when `s` is empty or contains only blank/whitespace lines.
pub fn minIndent(s: string): usize {
    const n = s.len;
    if (n == 0) {
        return 0;
    }
    let has_non_empty = false;
    let min_val: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= n) : (i += 1) {
        if (i == n || s[i] == (10 as u8)) {
            let line_end = i;
            if (line_end > line_start && s[line_end - 1] == (13 as u8)) {
                line_end -= 1;
            }
            let lead: usize = 0;
            while (line_start + lead < line_end && siIsIndentChar(s[line_start + lead])) {
                lead += 1;
            }
            // Only count lines that have at least one non-whitespace character
            if (line_start + lead < line_end) {
                if (!has_non_empty || lead < min_val) {
                    min_val = lead;
                    has_non_empty = true;
                    if (min_val == 0) {
                        return 0;
                    }
                }
            }
            line_start = i + 1;
        }
    }
    if (!has_non_empty) {
        return 0;
    }
    return min_val;
}

/// Strip exactly `n` leading space/tab characters from every line that starts with
/// at least `n` spaces/tabs, preserving `\n`, `\r\n`, and blank lines.
/// Writes into `out` and returns the number of bytes written.
pub fn stripIndentBy(s: string, n: usize, out: []u8): usize {
    const total = s.len;
    if (total == 0) {
        return 0;
    }
    if (n == 0) {
        return siCopyRange(out, 0, s, 0, total);
    }

    let p: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= total) : (i += 1) {
        if (i == total || s[i] == (10 as u8)) {
            let line_end = i;
            let has_cr = false;
            if (line_end > line_start && s[line_end - 1] == (13 as u8)) {
                line_end -= 1;
                has_cr = true;
            }

            let lead: usize = 0;
            while (line_start + lead < line_end && lead < n && siIsIndentChar(s[line_start + lead])) {
                lead += 1;
            }

            const copy_start = if (lead == n) line_start + n else line_start;
            p = siCopyRange(out, p, s, copy_start, line_end);

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

/// Strip leading whitespace from each line in `s` based on the shortest non-empty line's
/// indentation (`minIndent(s)`), writing into `out` and returning bytes written.
pub fn stripIndent(s: string, out: []u8): usize {
    const indent = minIndent(s);
    return stripIndentBy(s, indent, out);
}

/// Alias for `stripIndent`: strips common leading indentation into `out` and returns bytes written.
pub fn stripIndentInto(s: string, out: []u8): usize {
    return stripIndent(s, out);
}

pub struct StripIndentOptions {
    pub tab_width: usize,
    pub trim_blank_lines: bool,

    pub static fn defaults(): StripIndentOptions {
        return StripIndentOptions {
            tab_width: 1,
            trim_blank_lines: false,
        };
    }
}

/// Compute minimum leading visual column indentation across all non-empty lines,
/// treating `\t` as `tab_width` columns (minimum 1) and ignoring blank/whitespace-only lines.
pub fn minIndentWithTabWidth(s: string, tab_width: usize): usize {
    const n = s.len;
    if (n == 0) {
        return 0;
    }
    const tw: usize = if (tab_width == 0) 1 else tab_width;
    let has_non_empty = false;
    let min_cols: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= n) : (i += 1) {
        if (i == n || s[i] == (10 as u8)) {
            let line_end = i;
            if (line_end > line_start && s[line_end - 1] == (13 as u8)) {
                line_end -= 1;
            }
            let pos = line_start;
            let cols: usize = 0;
            while (pos < line_end && siIsIndentChar(s[pos])) {
                if (s[pos] == (9 as u8)) {
                    cols += tw;
                } else {
                    cols += 1;
                }
                pos += 1;
            }
            if (pos < line_end) {
                if (!has_non_empty || cols < min_cols) {
                    min_cols = cols;
                    has_non_empty = true;
                    if (min_cols == 0) {
                        return 0;
                    }
                }
            }
            line_start = i + 1;
        }
    }
    if (!has_non_empty) {
        return 0;
    }
    return min_cols;
}

/// Strip leading indentation using configurable `tab_width` (handling mixed `\t` and spaces)
/// and optional blank-line whitespace trimming.
pub fn stripIndentWith(s: string, opts: StripIndentOptions, out: []u8): usize {
    const total = s.len;
    if (total == 0) {
        return 0;
    }
    const tw: usize = if (opts.tab_width == 0) 1 else opts.tab_width;
    const min_cols = minIndentWithTabWidth(s, tw);

    let p: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= total) : (i += 1) {
        if (i == total || s[i] == (10 as u8)) {
            let line_end = i;
            let has_cr = false;
            if (line_end > line_start && s[line_end - 1] == (13 as u8)) {
                line_end -= 1;
                has_cr = true;
            }

            // Check if line is blank/whitespace-only
            let scan = line_start;
            while (scan < line_end && siIsIndentChar(s[scan])) {
                scan += 1;
            }
            const is_blank = scan == line_end;

            if (is_blank && opts.trim_blank_lines) {
                // Emit empty line content
            } else if (min_cols == 0) {
                p = siCopyRange(out, p, s, line_start, line_end);
            } else {
                let pos = line_start;
                let cols: usize = 0;
                let extra_spaces: usize = 0;
                while (pos < line_end && cols < min_cols && siIsIndentChar(s[pos])) {
                    if (s[pos] == (9 as u8)) {
                        if (cols + tw > min_cols) {
                            extra_spaces = (cols + tw) - min_cols;
                            cols = min_cols;
                            pos += 1;
                            break;
                        }
                        cols += tw;
                    } else {
                        cols += 1;
                    }
                    pos += 1;
                }
                if (cols >= min_cols) {
                    let sp: usize = 0;
                    while (sp < extra_spaces && p < out.len) : (sp += 1) {
                        out[p] = 32 as u8;
                        p += 1;
                    }
                    p = siCopyRange(out, p, s, pos, line_end);
                } else {
                    p = siCopyRange(out, p, s, line_start, line_end);
                }
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

/// Add `count` copies of `indent_char` to the start of every line (optionally including empty lines).
pub fn indentLines(s: string, count: usize, indent_char: u8, include_empty: bool, out: []u8): usize {
    const total = s.len;
    if (total == 0) {
        return 0;
    }
    let p: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= total) : (i += 1) {
        if (i == total || s[i] == (10 as u8)) {
            let line_end = i;
            let has_cr = false;
            if (line_end > line_start && s[line_end - 1] == (13 as u8)) {
                line_end -= 1;
                has_cr = true;
            }
            const is_empty = line_end == line_start;
            if (!is_empty || include_empty) {
                let k: usize = 0;
                while (k < count && p < out.len) : (k += 1) {
                    out[p] = indent_char;
                    p += 1;
                }
            }
            p = siCopyRange(out, p, s, line_start, line_end);
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
