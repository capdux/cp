// @cap/line-col — Zero-allocation Byte Offset <-> 1-based (line, col) & Source Line Extractor
// Clean-room MIT implementation in pure CP.

pub struct LineCol {
    pub line: usize,
    pub col: usize,
    pub valid: bool,
}

/// Converts a byte `offset` (`0 <= offset <= text.len`) into a 1-based `(line, col)` coordinate.
/// Handles both LF (`\n`) and CRLF (`\r\n`) line endings. Returns `valid: false` if `offset > text.len`.
pub fn offsetToLineCol(text: string, offset: usize) -> LineCol {
    if (offset > text.len) {
        return LineCol { line: 0, col: 0, valid: false };
    }
    let line: usize = 1;
    let col: usize = 1;
    let i: usize = 0;

    while (i < offset) {
        const ch = text[i];
        if (ch == 10) {
            // '\n'
            line += 1;
            col = 1;
            i += 1;
        } else if (ch == 13 && i + 1 < text.len && text[i + 1] == 10) {
            // '\r\n'
            if (i + 1 < offset) {
                line += 1;
                col = 1;
                i += 2;
            } else {
                col += 1;
                i += 1;
            }
        } else {
            col += 1;
            i += 1;
        }
    }

    return LineCol { line: line, col: col, valid: true };
}

/// Converts a 1-based `(line, col)` coordinate back into a byte offset into `text`.
/// Returns `-1` if `line == 0`, `col == 0`, or either coordinate is out of bounds.
pub fn lineColToOffset(text: string, line: usize, col: usize) -> i64 {
    if (line == 0 || col == 0) {
        return -1;
    }
    if (text.len == 0) {
        if (line == 1 && col == 1) {
            return 0;
        }
        return -1;
    }

    let cur_line: usize = 1;
    let pos: usize = 0;
    const n = text.len;

    while (true) {
        const line_start = pos;
        while (pos < n && text[pos] != 10) {
            pos += 1;
        }
        let line_end = pos;
        if (line_end > line_start && text[line_end - 1] == 13) {
            line_end -= 1;
        }

        if (cur_line == line) {
            const line_len = line_end - line_start;
            if (col - 1 > line_len) {
                return -1;
            }
            return (line_start + (col - 1)) as i64;
        }

        if (pos >= n) {
            return -1;
        }
        pos += 1; // consume '\n'
        cur_line += 1;
    }
}

/// Zero-copy extraction of the 1-based `line_1based` line from `text`, stripping trailing `\r\n` or `\n`.
/// Returns `""` if `line_1based == 0` or out of bounds.
pub fn getLine(text: string, line_1based: usize) -> string {
    if (line_1based == 0 || text.len == 0) {
        return "";
    }

    let cur_line: usize = 1;
    let pos: usize = 0;
    const n = text.len;

    while (pos < n) {
        const line_start = pos;
        while (pos < n && text[pos] != 10) {
            pos += 1;
        }
        let line_end = pos;
        if (line_end > line_start && text[line_end - 1] == 13) {
            line_end -= 1;
        }

        if (cur_line == line_1based) {
            return text[line_start..line_end];
        }

        if (pos >= n) {
            break;
        }
        pos += 1; // consume '\n'
        cur_line += 1;
    }

    return "";
}

/// Counts the number of lines in `text` (`""` -> `0`, trailing `\n` terminates the final line).
pub fn countLines(text: string) -> usize {
    const n = text.len;
    if (n == 0) {
        return 0;
    }
    let count: usize = 1;
    let i: usize = 0;
    while (i < n) {
        if (text[i] == 10 && i + 1 < n) {
            count += 1;
        }
        i += 1;
    }
    return count;
}
