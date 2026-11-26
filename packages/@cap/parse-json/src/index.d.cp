// Type definitions for CP

export enum JsonDiagKind {
    Ok,
    EmptyInput,
    UnexpectedToken,
    UnexpectedEnd,
    TrailingComma,
    SingleQuotedString,
    UnquotedKey,
    LeadingZero,
    UnescapedControlChar,
    MaxDepthExceeded
}

export class JsonDiagResult {
    ok: bool,
    kind: JsonDiagKind,
    offset: usize,
    line: usize,
    column: usize,
    value_kind: u8,
}

export class PjsParser {
    input: string,
    pos: usize,
    has_err: bool,
    err_kind: JsonDiagKind,
    err_offset: usize,
}

export class PjsLoc {
    line: usize,
    column: usize,
}

export class PjsLineSpan {
    found: bool,
    start: usize,
    end: usize,
}

export class PjsWriter {
    buf: []u8,
    pos: usize,
}

export fn pjsIsWs(ch: u8): bool;
export fn pjsIsDigit(ch: u8): bool;
export fn pjsIsHex(ch: u8): bool;
export fn pjsIsIdentStart(ch: u8): bool;
export fn pjsSkipWs(p: &mut PjsParser);
export fn pjsSetErr(p: &mut PjsParser, kind: JsonDiagKind, offset: usize);
export fn pjsLineCol(input: string, offset: usize): PjsLoc;
export fn pjsParseString(p: &mut PjsParser);
export fn pjsParseNumber(p: &mut PjsParser);
export fn pjsExpectKeyword(p: &mut PjsParser, kw: string);
export fn pjsParseValue(p: &mut PjsParser, depth: usize): u8;
export fn validateJsonWithDiag(input: string): JsonDiagResult;
export fn pjsDiagMessage(kind: JsonDiagKind): string;
export fn pjsWriteByte(w: &mut PjsWriter, b: u8);
export fn pjsWriteStr(w: &mut PjsWriter, s: string);
export fn pjsNumWidth(n: usize): usize;
export fn pjsWriteUsize(w: &mut PjsWriter, n: usize);
export fn pjsWriteSpaces(w: &mut PjsWriter, count: usize);
export fn pjsFindLineSpan(input: string, target_line: usize): PjsLineSpan;
export fn pjsWriteFrameLine(w: &mut PjsWriter, input: string, line_no: usize, span: PjsLineSpan, gutter_w: usize, is_err: bool);
export fn formatJsonError(input: string, diag: JsonDiagResult, filename: string, out: []u8): usize;
