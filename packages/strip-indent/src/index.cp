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
