// @cap/indoc — Zero-allocation multiline string dedent, indent & stripMargin engine for CP
// Clean-room MIT implementation inspired by Rust `indoc`, npm `dedent`, and Kotlin `trimMargin`.

fn isHws(c: u8) -> bool {
    return c == ' ' || c == '\t';
}

fn isLineBlank(s: string, start: usize, end: usize) -> bool {
    let i = start;
    while (i < end) {
        const c = s[i];
        if (c != ' ' && c != '\t' && c != '\r') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendSlice(out: []u8, start_pos: usize, s: string, a: usize, b: usize) -> usize {
    let pos = start_pos;
    let i = a;
    while (i < b && i < s.len && pos < out.len) {
        out[pos] = s[i];
        pos += 1;
        i += 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    return appendSlice(out, pos, s, 0 as usize, s.len);
}

/// Strips a single leading blank line (up to and including its `\n`) and/or a single trailing
/// blank line (the final `\n` plus trailing horizontal whitespace) if present.
fn stripOuterSingleBlankLines(input: string) -> string {
    let start: usize = 0;
    let end: usize = input.len;

    // Check leading blank line
    let first_nl = start;
    while (first_nl < end && input[first_nl] != '\n') {
        first_nl += 1;
    }
    if (first_nl < end && isLineBlank(input, start, first_nl)) {
        start = first_nl + 1;
    }

    // Check trailing blank line after last '\n'
    if (end > start) {
        let last_nl = end;
        let found_nl = false;
        while (last_nl > start) {
            last_nl -= 1;
            if (input[last_nl] == '\n') {
                found_nl = true;
                break;
            }
        }
        if (found_nl && isLineBlank(input, last_nl + 1, end)) {
            end = last_nl;
            if (end > start && input[end - 1] == '\r') {
                end -= 1;
            }
        }
    }

    return input[start..end];
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Strips all leading and trailing blank lines (zero-copy slice).
pub fn trimBlankLines(input: string) -> string {
    let start: usize = 0;
    let end: usize = input.len;

    while (start < end) {
        let nl = start;
        while (nl < end && input[nl] != '\n') {
            nl += 1;
        }
        if (isLineBlank(input, start, nl)) {
            start = if (nl < end) nl + 1 else end;
        } else {
            break;
        }
    }

    while (end > start) {
        let prev_nl = end;
        let found = false;
        while (prev_nl > start) {
            prev_nl -= 1;
            if (input[prev_nl] == '\n') {
                found = true;
                break;
            }
        }
        const line_start = if (found) prev_nl + 1 else start;
        if (isLineBlank(input, line_start, end)) {
            end = if (found) prev_nl else start;
            if (end > start && input[end - 1] == '\r') {
                end -= 1;
            }
        } else {
            break;
        }
    }

    return input[start..end];
}

/// Computes the minimum common leading space/tab count across all non-blank lines in `input`.
/// Ignores blank / whitespace-only lines.
pub fn minCommonIndent(input: string) -> usize {
    let has_line = false;
    let min_ws: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= input.len) {
        if (i == input.len || input[i] == '\n') {
            if (!isLineBlank(input, line_start, i)) {
                let ws: usize = 0;
                while (line_start + ws < i && isHws(input[line_start + ws])) {
                    ws += 1;
                }
                if (!has_line || ws < min_ws) {
                    min_ws = ws;
                    has_line = true;
                }
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return min_ws;
}

/// Removes common leading indentation from multiline `input`, stripping a single leading/trailing
/// blank line if present, preserving relative nested indentation, and clearing blank lines.
pub fn dedent(input: string, out: []u8) -> string {
    if (input.len == 0) {
        return out[0..0];
    }
    const core = stripOuterSingleBlankLines(input);
    if (core.len == 0) {
        return out[0..0];
    }

    const common = minCommonIndent(core);
    let pos: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= core.len) {
        if (i == core.len || core[i] == '\n') {
            let line_end = i;
            if (line_end > line_start && core[line_end - 1] == '\r') {
                line_end -= 1;
            }
            if (!isLineBlank(core, line_start, line_end)) {
                const avail = line_end - line_start;
                const skip = if (common < avail) common else avail;
                pos = appendSlice(out, pos, core, line_start + skip, line_end);
            }
            if (i < core.len && core[i] == '\n') {
                pos = appendByte(out, pos, '\n');
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return out[0..pos];
}

/// Prepends `prefix` to each line of `input` (skipping blank lines when `skip_empty == true`).
pub fn indent(input: string, prefix: string, skip_empty: bool, out: []u8) -> string {
    if (input.len == 0) {
        return out[0..0];
    }
    let pos: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= input.len) {
        if (i == input.len || input[i] == '\n') {
            const blank = isLineBlank(input, line_start, i);
            if (!skip_empty || !blank) {
                pos = appendStr(out, pos, prefix);
            }
            pos = appendSlice(out, pos, input, line_start, i);
            if (i < input.len && input[i] == '\n') {
                pos = appendByte(out, pos, '\n');
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return out[0..pos];
}

/// Scala/Kotlin-style `stripMargin`: strips leading whitespace up to and including `margin_char`
/// (e.g. `'|'`) on each line, and strips a single outer blank line if present.
pub fn stripMargin(input: string, margin_char: u8, out: []u8) -> string {
    if (input.len == 0) {
        return out[0..0];
    }
    const core = stripOuterSingleBlankLines(input);
    if (core.len == 0) {
        return out[0..0];
    }

    let pos: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= core.len) {
        if (i == core.len || core[i] == '\n') {
            let line_end = i;
            if (line_end > line_start && core[line_end - 1] == '\r') {
                line_end -= 1;
            }
            let p = line_start;
            while (p < line_end && isHws(core[p])) {
                p += 1;
            }
            if (p < line_end && core[p] == margin_char) {
                pos = appendSlice(out, pos, core, p + 1, line_end);
            } else {
                pos = appendSlice(out, pos, core, line_start, line_end);
            }
            if (i < core.len && core[i] == '\n') {
                pos = appendByte(out, pos, '\n');
            }
            line_start = i + 1;
        }
        i += 1;
    }
    return out[0..pos];
}

/// Normalizes all `\r\n` and standalone `\r` line endings in `input` to `\n` in `out`.
pub fn normalizeNewlines(input: string, out: []u8) -> string {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == '\r') {
            pos = appendByte(out, pos, '\n');
            if (i + 1 < input.len && input[i + 1] == '\n') {
                i += 2;
            } else {
                i += 1;
            }
        } else {
            pos = appendByte(out, pos, input[i]);
            i += 1;
        }
    }
    return out[0..pos];
}
