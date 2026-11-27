// @cap/indent-string — Zero-heap multi-line string indenter
// Matching npm indent-string (180M+/wk). Clean-room MIT in pure CP.

fn writeRepeat(out: []u8, pos: usize, unit: string, count: usize): usize {
    let p = pos;
    let c: usize = 0;
    while (c < count) {
        let j: usize = 0;
        while (j < unit.len && p < out.len) {
            out[p] = unit[j];
            p = p + 1;
            j = j + 1;
        }
        c = c + 1;
    }
    return p;
}

fn isLineBlank(input: string, start: usize, end: usize): bool {
    let i = start;
    while (i < end) {
        let ch = input[i];
        if (ch != 32 && ch != 9 && ch != 13) { return false; }
        i = i + 1;
    }
    return true;
}

// Indents each line of `input` by `count` copies of `indent_unit`.
// If `include_empty_lines` is false, blank/whitespace-only lines are left unindented.
pub fn indentStringWith(input: string, count: usize, indent_unit: string, include_empty_lines: bool, out: []u8): usize {
    let n = input.len;
    if (n == 0) { return 0; }
    let p: usize = 0;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i <= n) {
        if (i == n || input[i] == 10) {
            let line_end = i;
            let should_indent = include_empty_lines || !isLineBlank(input, line_start, line_end);
            if (should_indent && count > 0 && indent_unit.len > 0) {
                p = writeRepeat(out, p, indent_unit, count);
            }
            let k = line_start;
            while (k < line_end && p < out.len) {
                out[p] = input[k];
                p = p + 1;
                k = k + 1;
            }
            if (i < n && p < out.len) {
                out[p] = 10;
                p = p + 1;
            }
            line_start = i + 1;
        }
        i = i + 1;
    }
    return p;
}

pub fn indentString(input: string, count: usize, out: []u8): usize {
    return indentStringWith(input, count, " ", false, out);
}
