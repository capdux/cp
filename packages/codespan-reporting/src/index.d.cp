// Type definitions for CP

export enum DiagSeverity {
    Bug,
    Error,
    Warning,
    Note,
    Help
}

export class LineCol {
    line: usize,
    col: usize,
    line_start: usize,
    line_end: usize,
}

export class Diagnostic {
    severity: DiagSeverity,
    code: string,
    message: string,
    file_name: string,
    span_start: usize,
    span_end: usize,
    label: string,
    note: string,
}

export fn csrWriteStr(s: string, out: []u8, pos: usize): usize;
export fn csrWriteRepeat(ch: u8, count: usize, out: []u8, pos: usize): usize;
export fn csrWriteUsize(val: usize, out: []u8, pos: usize): usize;
export fn csrDigitWidth(val: usize): usize;
export fn severityLabel(sev: DiagSeverity): string;
export fn locateOffset(source: string, offset: usize): LineCol;
export fn renderDiagnostic(source: string, diag: Diagnostic, out: []u8): usize;
