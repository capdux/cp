// @cap/codespan-reporting — Zero-heap Rustc-style source diagnostic renderer in pure CP
// Clean-room Apache-2.0 implementation of crates.io `codespan-reporting` (#702, 177.4M downloads).

pub enum DiagSeverity {
    Bug,
    Error,
    Warning,
    Note,
    Help,
}

pub struct LineCol {
    pub line: usize,       // 1-based line number
    pub col: usize,        // 1-based column
    pub line_start: usize, // byte offset of line start
    pub line_end: usize,   // byte offset of line end (excluding '\n')
}

pub struct Diagnostic {
    pub severity: DiagSeverity,
    pub code: string,
    pub message: string,
    pub file_name: string,
    pub span_start: usize,
    pub span_end: usize,
    pub label: string,
    pub note: string,
}

fn csrWriteStr(s: string, out: []u8, pos: usize): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn csrWriteRepeat(ch: u8, count: usize, out: []u8, pos: usize): usize {
    let p = pos;
    let i: usize = 0;
    while (i < count && p < out.len) {
        out[p] = ch;
        p += 1;
        i += 1;
    }
    return p;
}

fn csrWriteUsize(val: usize, out: []u8, pos: usize): usize {
    let digits: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        digits[0] = 48;
        n = 1;
    } else {
        while (v > 0 && n < 24) {
            digits[n] = ((v % 10) as u8) + 48;
            v = v / 10;
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = digits[k];
        p += 1;
    }
    return p;
}

fn csrDigitWidth(val: usize): usize {
    if (val < 10) { return 1; }
    let w: usize = 0;
    let v = val;
    while (v > 0) {
        w += 1;
        v = v / 10;
    }
    return w;
}

pub fn severityLabel(sev: DiagSeverity): string {
    if (sev == .Bug) { return "bug"; }
    if (sev == .Error) { return "error"; }
    if (sev == .Warning) { return "warning"; }
    if (sev == .Note) { return "note"; }
    return "help";
}

/// Resolves a byte `offset` in `source` into 1-based `(line, col)` and the enclosing line's byte range.
pub fn locateOffset(source: string, offset: usize): LineCol {
    let target = offset;
    if (target > source.len) {
        target = source.len;
    }
    let line: usize = 1;
    let line_start: usize = 0;
    let i: usize = 0;
    while (i < target) {
        if (source[i] == 10) { // '\n'
            line += 1;
            line_start = i + 1;
        }
        i += 1;
    }
    let line_end = line_start;
    while (line_end < source.len && source[line_end] != 10 && source[line_end] != 13) {
        line_end += 1;
    }
    return LineCol {
        line: line,
        col: (target - line_start) + 1,
        line_start: line_start,
        line_end: line_end,
    };
}

/// Renders a Rustc/codespan-style diagnostic snippet into `out`; returns bytes written.
pub fn renderDiagnostic(source: string, diag: Diagnostic, out: []u8): usize {
    let p: usize = 0;
    // Header: error[E0105]: message
    p = csrWriteStr(severityLabel(diag.severity), out, p);
    if (diag.code.len > 0) {
        p = csrWriteStr("[", out, p);
        p = csrWriteStr(diag.code, out, p);
        p = csrWriteStr("]", out, p);
    }
    p = csrWriteStr(": ", out, p);
    p = csrWriteStr(diag.message, out, p);
    p = csrWriteStr("\n", out, p);

    const loc = locateOffset(source, diag.span_start);
    const gutter = csrDigitWidth(loc.line);

    // Location line: "  --> file:line:col"
    p = csrWriteRepeat(32, gutter, out, p);
    p = csrWriteStr(" --> ", out, p);
    p = csrWriteStr(diag.file_name, out, p);
    p = csrWriteStr(":", out, p);
    p = csrWriteUsize(loc.line, out, p);
    p = csrWriteStr(":", out, p);
    p = csrWriteUsize(loc.col, out, p);
    p = csrWriteStr("\n", out, p);

    // Empty gutter line
    p = csrWriteRepeat(32, gutter + 1, out, p);
    p = csrWriteStr("|\n", out, p);

    // Source code line
    p = csrWriteUsize(loc.line, out, p);
    p = csrWriteStr(" | ", out, p);
    p = csrWriteStr(source[loc.line_start..loc.line_end], out, p);
    p = csrWriteStr("\n", out, p);

    // Caret underline line
    p = csrWriteRepeat(32, gutter + 1, out, p);
    p = csrWriteStr("| ", out, p);
    const caret_pad = loc.col - 1;
    p = csrWriteRepeat(32, caret_pad, out, p);

    let span_len: usize = 1;
    if (diag.span_end > diag.span_start) {
        let clamped_end = diag.span_end;
        if (clamped_end > loc.line_end) {
            clamped_end = loc.line_end;
        }
        if (clamped_end > diag.span_start) {
            span_len = clamped_end - diag.span_start;
        }
    }
    p = csrWriteRepeat(94, span_len, out, p); // '^'
    if (diag.label.len > 0) {
        p = csrWriteStr(" ", out, p);
        p = csrWriteStr(diag.label, out, p);
    }
    p = csrWriteStr("\n", out, p);

    if (diag.note.len > 0) {
        p = csrWriteRepeat(32, gutter + 1, out, p);
        p = csrWriteStr("= note: ", out, p);
        p = csrWriteStr(diag.note, out, p);
        p = csrWriteStr("\n", out, p);
    }
    return p;
}

export {
    DiagSeverity,
    LineCol,
    Diagnostic,
    severityLabel,
    locateOffset,
    renderDiagnostic,
};
