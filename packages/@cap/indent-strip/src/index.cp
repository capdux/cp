// @cap/indent-strip — Zero-heap common leading indentation stripper (strip-indent / dedent / indoc)
// Clean-room MIT implementation for CP.

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

/// Find the minimum number of leading spaces/tabs across all non-empty (non-whitespace-only) lines.
/// Returns 0 if there are no indented non-empty lines.
pub fn minIndent(input: string): usize {
    let found: bool = false;
    let best: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        let line_start: usize = i;
        while (i < input.len && input[i] != '\n') {
            i += 1;
        }
        let line_end: usize = i;
        if (line_end > line_start && input[line_end - 1] == '\r') {
            line_end -= 1;
        }

        let j: usize = line_start;
        while (j < line_end && isHorizSpace(input[j])) {
            j += 1;
        }
        // Only count lines with at least one non-whitespace character
        if (j < line_end) {
            let ind: usize = j - line_start;
            if (!found || ind < best) {
                best = ind;
                found = true;
            }
        }

        if (i < input.len) { i += 1; }
    }
    if (!found) { return 0; }
    return best;
}

/// Strip the common minimum leading indentation from every line of `input`.
/// Matches npm `strip-indent` & Rust `textwrap::dedent` semantics.
/// Returns bytes written to `out` (0 on empty input or insufficient buffer).
pub fn stripIndent(input: string, out: []u8): usize {
    if (input.len == 0) { return 0; }
    let cut: usize = minIndent(input);
    let pos: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        let line_start: usize = i;
        while (i < input.len && input[i] != '\n') {
            i += 1;
        }
        let has_nl: bool = i < input.len;
        let line_end: usize = i;

        let skip: usize = 0;
        while (skip < cut && line_start + skip < line_end && isHorizSpace(input[line_start + skip])) {
            skip += 1;
        }
        let copy_start: usize = line_start + skip;
        let copy_len: usize = line_end - copy_start;

        let need: usize = copy_len;
        if (has_nl) { need += 1; }
        if (pos + need > out.len) {
            return 0;
        }

        let k: usize = 0;
        while (k < copy_len) {
            out[pos] = input[copy_start + k];
            pos += 1;
            k += 1;
        }
        if (has_nl) {
            out[pos] = '\n';
            pos += 1;
            i += 1;
        }
    }
    return pos;
}

/// Add `spaces` leading spaces to every non-empty line of `input`.
/// Returns bytes written to `out` (0 on empty input or insufficient buffer).
pub fn indentLines(input: string, spaces: usize, out: []u8): usize {
    if (input.len == 0) { return 0; }
    let pos: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        let line_start: usize = i;
        while (i < input.len && input[i] != '\n') {
            i += 1;
        }
        let has_nl: bool = i < input.len;
        let line_end: usize = i;
        let content_end: usize = line_end;
        if (content_end > line_start && input[content_end - 1] == '\r') {
            content_end -= 1;
        }

        let is_non_empty: bool = content_end > line_start;
        if (is_non_empty) {
            if (pos + spaces > out.len) { return 0; }
            let s: usize = 0;
            while (s < spaces) {
                out[pos] = ' ';
                pos += 1;
                s += 1;
            }
        }

        let copy_len: usize = line_end - line_start;
        let need: usize = copy_len;
        if (has_nl) { need += 1; }
        if (pos + need > out.len) { return 0; }

        let k: usize = 0;
        while (k < copy_len) {
            out[pos] = input[line_start + k];
            pos += 1;
            k += 1;
        }
        if (has_nl) {
            out[pos] = '\n';
            pos += 1;
            i += 1;
        }
    }
    return pos;
}
