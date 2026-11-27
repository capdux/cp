// @cap/codespan-reporting — Zero-heap Rustc-style source diagnostic renderer in pure CP
// Clean-room Apache-2.0 implementation of crates.io `codespan-reporting` (#702, 177.4M downloads).

import { BufWriter } from "cap:fmt";

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
    let w = BufWriter.new(out);
    // Header: error[E0105]: message
    w.writeStr(severityLabel(diag.severity));
    if (diag.code.len > 0) {
        w.writeStr("[");
        w.writeStr(diag.code);
        w.writeStr("]");
    }
    w.writeStr(": ");
    w.writeStr(diag.message);
    w.writeStr("\n");

    const loc = locateOffset(source, diag.span_start);
    const gutter = csrDigitWidth(loc.line);

    // Location line: "  --> file:line:col"
    w.writeRepeat(32, gutter);
    w.writeStr(" --> ");
    w.writeStr(diag.file_name);
    w.writeStr(":");
    w.writeU64(loc.line as u64);
    w.writeStr(":");
    w.writeU64(loc.col as u64);
    w.writeStr("\n");

    // Empty gutter line
    w.writeRepeat(32, gutter + 1);
    w.writeStr("|\n");

    // Source code line
    w.writeU64(loc.line as u64);
    w.writeStr(" | ");
    w.writeStr(source[loc.line_start..loc.line_end]);
    w.writeStr("\n");

    // Caret underline line
    w.writeRepeat(32, gutter + 1);
    w.writeStr("| ");
    const caret_pad = loc.col - 1;
    w.writeRepeat(32, caret_pad);

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
    w.writeRepeat(94, span_len); // '^'
    if (diag.label.len > 0) {
        w.writeStr(" ");
        w.writeStr(diag.label);
    }
    w.writeStr("\n");

    if (diag.note.len > 0) {
        w.writeRepeat(32, gutter + 1);
        w.writeStr("= note: ");
        w.writeStr(diag.note);
        w.writeStr("\n");
    }
    return w.written();
}

export {
    DiagSeverity,
    LineCol,
    Diagnostic,
    severityLabel,
    locateOffset,
    renderDiagnostic,
};
