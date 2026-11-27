// @cap/parse-json — Zero-allocation RFC 8259 JSON diagnostic validator & codeFrame renderer
// Clean-room MIT implementation inspired by Sindre Sorhus parse-json v8
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning, comptime 256-entry LUT, and @memcpy.

type PjsVec16 = @Vector(16, u8);
type PjsBool16 = @Vector(16, bool);

const PJS_SPACES_64: string = "                                                                ";

const PJS_FLAG_WS: u8 = 1;
const PJS_FLAG_DIGIT: u8 = 2;
const PJS_FLAG_HEX: u8 = 4;
const PJS_FLAG_IDENT_START: u8 = 8;
const PJS_FLAG_SIMPLE_ESC: u8 = 16;

fn pjsBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            f |= PJS_FLAG_WS;
        }
        if (c >= '0' && c <= '9') {
            f |= PJS_FLAG_DIGIT | PJS_FLAG_HEX;
        }
        if ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
            f |= PJS_FLAG_HEX;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$') {
            f |= PJS_FLAG_IDENT_START;
        }
        if (c == '"' || c == '\\' || c == '/' || c == 'b' || c == 'f' || c == 'n' || c == 'r' || c == 't') {
            f |= PJS_FLAG_SIMPLE_ESC;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const PJS_CHAR_LUT: [256]u8 = pjsBuildCharLut();

fn pjsFindByteSimd(s: string, target: u8, start: usize): usize {
    let i = start;
    const len = s.len;
    const vt: PjsVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: PjsVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

pub const JSON_VAL_NONE: u8 = 0;
pub const JSON_VAL_OBJECT: u8 = 1;
pub const JSON_VAL_ARRAY: u8 = 2;
pub const JSON_VAL_STRING: u8 = 3;
pub const JSON_VAL_NUMBER: u8 = 4;
pub const JSON_VAL_BOOL: u8 = 5;
pub const JSON_VAL_NULL: u8 = 6;

const PJS_MAX_DEPTH: usize = 128;

pub enum JsonDiagKind {
    Ok,
    EmptyInput,
    UnexpectedToken,
    UnexpectedEnd,
    TrailingComma,
    SingleQuotedString,
    UnquotedKey,
    LeadingZero,
    UnescapedControlChar,
    MaxDepthExceeded,
}

pub struct JsonDiagResult {
    ok: bool,
    kind: JsonDiagKind,
    offset: usize,
    line: usize,
    column: usize,
    value_kind: u8,
}

struct PjsParser {
    input: string,
    pos: usize,
    has_err: bool,
    err_kind: JsonDiagKind,
    err_offset: usize,
}

struct PjsLoc {
    line: usize,
    column: usize,
}

struct PjsLineSpan {
    found: bool,
    start: usize,
    end: usize,
}

struct PjsWriter {
    buf: []u8,
    pos: usize,
}

fn pjsIsWs(ch: u8): bool {
    return (PJS_CHAR_LUT[ch as usize] & PJS_FLAG_WS) != 0;
}

fn pjsIsDigit(ch: u8): bool {
    return (PJS_CHAR_LUT[ch as usize] & PJS_FLAG_DIGIT) != 0;
}

fn pjsIsHex(ch: u8): bool {
    return (PJS_CHAR_LUT[ch as usize] & PJS_FLAG_HEX) != 0;
}

fn pjsIsIdentStart(ch: u8): bool {
    return (PJS_CHAR_LUT[ch as usize] & PJS_FLAG_IDENT_START) != 0;
}

fn pjsSkipWs(p: *PjsParser) {
    const len = p.input.len;
    const v_sp: PjsVec16 = @splat(' ' as u8);
    const v_tb: PjsVec16 = @splat('\t' as u8);
    const v_nl: PjsVec16 = @splat('\n' as u8);
    const v_cr: PjsVec16 = @splat('\r' as u8);
    const v_true: PjsBool16 = @splat(true);
    while (p.pos + 16 <= len) {
        const v: PjsVec16 = p.input[p.pos .. p.pos + 16][0..16].*;
        const w1 = @select(bool, v == v_sp, v_true, v == v_tb);
        const w2 = @select(bool, v == v_nl, v_true, v == v_cr);
        const is_ws = @select(bool, w1, v_true, w2);
        if (!@reduce(.And, is_ws)) {
            break;
        }
        p.pos += 16;
    }
    while (p.pos < len && (PJS_CHAR_LUT[p.input[p.pos] as usize] & PJS_FLAG_WS) != 0) {
        p.pos += 1;
    }
}

fn pjsSetErr(p: *PjsParser, kind: JsonDiagKind, offset: usize) {
    if (!p.has_err) {
        p.has_err = true;
        p.err_kind = kind;
        p.err_offset = offset;
    }
}

fn pjsLineCol(input: string, offset: usize): PjsLoc {
    const limit = if (offset > input.len) input.len else offset;
    let line: usize = 1;
    let col: usize = 1;
    let i: usize = 0;
    const v_nl: PjsVec16 = @splat('\n' as u8);
    const v_cr: PjsVec16 = @splat('\r' as u8);
    while (i < limit) {
        if (i + 16 <= limit) {
            const v: PjsVec16 = input[i .. i + 16][0..16].*;
            if (!@reduce(.Or, v == v_nl) && !@reduce(.Or, v == v_cr)) {
                col += 16;
                i += 16;
                continue;
            }
        }
        const ch = input[i];
        if (ch == '\n') {
            line += 1;
            col = 1;
        } else if (ch == '\r') {
            if (i + 1 < limit && input[i + 1] == '\n') {
                // wait for '\n'
            } else {
                line += 1;
                col = 1;
            }
        } else {
            col += 1;
        }
        i += 1;
    }
    return PjsLoc{ line: line, column: col };
}

fn pjsParseString(p: *PjsParser) {
    // caller verified input[p.pos] == '"'
    p.pos += 1;
    const len = p.input.len;
    const v_qt: PjsVec16 = @splat('"' as u8);
    const v_bs: PjsVec16 = @splat('\\' as u8);
    const v_sp: PjsVec16 = @splat(0x20 as u8);
    while (p.pos < len) {
        if (p.pos + 16 <= len) {
            const v: PjsVec16 = p.input[p.pos .. p.pos + 16][0..16].*;
            if (!@reduce(.Or, v == v_qt) && !@reduce(.Or, v == v_bs) && !@reduce(.Or, v < v_sp)) {
                p.pos += 16;
                continue;
            }
        }
        const ch = p.input[p.pos];
        if (ch == '"') {
            p.pos += 1;
            return;
        }
        if (ch == '\\') {
            p.pos += 1;
            if (p.pos >= len) {
                pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
                return;
            }
            const esc = p.input[p.pos];
            if ((PJS_CHAR_LUT[esc as usize] & PJS_FLAG_SIMPLE_ESC) != 0) {
                p.pos += 1;
                continue;
            }
            if (esc == 'u') {
                p.pos += 1;
                let k: usize = 0;
                while (k < 4) : (k += 1) {
                    if (p.pos >= len) {
                        pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
                        return;
                    }
                    if ((PJS_CHAR_LUT[p.input[p.pos] as usize] & PJS_FLAG_HEX) == 0) {
                        pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
                        return;
                    }
                    p.pos += 1;
                }
                continue;
            }
            pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
            return;
        }
        if (ch < 0x20) {
            pjsSetErr(p, JsonDiagKind.UnescapedControlChar, p.pos);
            return;
        }
        p.pos += 1;
    }
    pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
}

fn pjsParseNumber(p: *PjsParser) {
    const start = p.pos;
    if (p.input[p.pos] == '-') {
        p.pos += 1;
        if (p.pos >= p.input.len) {
            pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
            return;
        }
        if (!pjsIsDigit(p.input[p.pos])) {
            pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
            return;
        }
    }

    if (p.input[p.pos] == '0') {
        p.pos += 1;
        if (p.pos < p.input.len && pjsIsDigit(p.input[p.pos])) {
            pjsSetErr(p, JsonDiagKind.LeadingZero, start);
            return;
        }
    } else {
        while (p.pos < p.input.len && pjsIsDigit(p.input[p.pos])) {
            p.pos += 1;
        }
    }

    if (p.pos < p.input.len && p.input[p.pos] == '.') {
        p.pos += 1;
        if (p.pos >= p.input.len) {
            pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
            return;
        }
        if (!pjsIsDigit(p.input[p.pos])) {
            pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
            return;
        }
        while (p.pos < p.input.len && pjsIsDigit(p.input[p.pos])) {
            p.pos += 1;
        }
    }

    if (p.pos < p.input.len && (p.input[p.pos] == 'e' || p.input[p.pos] == 'E')) {
        p.pos += 1;
        if (p.pos < p.input.len && (p.input[p.pos] == '+' || p.input[p.pos] == '-')) {
            p.pos += 1;
        }
        if (p.pos >= p.input.len) {
            pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
            return;
        }
        if (!pjsIsDigit(p.input[p.pos])) {
            pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
            return;
        }
        while (p.pos < p.input.len && pjsIsDigit(p.input[p.pos])) {
            p.pos += 1;
        }
    }
}

fn pjsExpectKeyword(p: *PjsParser, kw: string) {
    let i: usize = 0;
    while (i < kw.len) : (i += 1) {
        if (p.pos >= p.input.len) {
            pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
            return;
        }
        if (p.input[p.pos] != kw[i]) {
            pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
            return;
        }
        p.pos += 1;
    }
}

fn pjsParseValue(p: *PjsParser, depth: usize): u8 {
    pjsSkipWs(p);
    if (p.pos >= p.input.len) {
        pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
        return JSON_VAL_NONE;
    }

    const ch = p.input[p.pos];
    if (ch == '{') {
        if (depth >= PJS_MAX_DEPTH) {
            pjsSetErr(p, JsonDiagKind.MaxDepthExceeded, p.pos);
            return JSON_VAL_NONE;
        }
        p.pos += 1;
        pjsSkipWs(p);
        if (p.pos >= p.input.len) {
            pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
            return JSON_VAL_NONE;
        }
        if (p.input[p.pos] == '}') {
            p.pos += 1;
            return JSON_VAL_OBJECT;
        }
        while (true) {
            pjsSkipWs(p);
            if (p.pos >= p.input.len) {
                pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
                return JSON_VAL_NONE;
            }
            const kch = p.input[p.pos];
            if (kch == '"') {
                pjsParseString(p);
                if (p.has_err) return JSON_VAL_NONE;
            } else if (kch == '\'') {
                pjsSetErr(p, JsonDiagKind.SingleQuotedString, p.pos);
                return JSON_VAL_NONE;
            } else if (pjsIsIdentStart(kch)) {
                pjsSetErr(p, JsonDiagKind.UnquotedKey, p.pos);
                return JSON_VAL_NONE;
            } else {
                pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
                return JSON_VAL_NONE;
            }

            pjsSkipWs(p);
            if (p.pos >= p.input.len) {
                pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
                return JSON_VAL_NONE;
            }
            if (p.input[p.pos] != ':') {
                pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
                return JSON_VAL_NONE;
            }
            p.pos += 1;

            * = pjsParseValue(p, depth + 1);
            if (p.has_err) return JSON_VAL_NONE;

            pjsSkipWs(p);
            if (p.pos >= p.input.len) {
                pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
                return JSON_VAL_NONE;
            }
            const sep = p.input[p.pos];
            if (sep == ',') {
                const comma_pos = p.pos;
                p.pos += 1;
                pjsSkipWs(p);
                if (p.pos >= p.input.len) {
                    pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
                    return JSON_VAL_NONE;
                }
                if (p.input[p.pos] == '}') {
                    pjsSetErr(p, JsonDiagKind.TrailingComma, comma_pos);
                    return JSON_VAL_NONE;
                }
                continue;
            }
            if (sep == '}') {
                p.pos += 1;
                return JSON_VAL_OBJECT;
            }
            pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
            return JSON_VAL_NONE;
        }
    }

    if (ch == '[') {
        if (depth >= PJS_MAX_DEPTH) {
            pjsSetErr(p, JsonDiagKind.MaxDepthExceeded, p.pos);
            return JSON_VAL_NONE;
        }
        p.pos += 1;
        pjsSkipWs(p);
        if (p.pos >= p.input.len) {
            pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
            return JSON_VAL_NONE;
        }
        if (p.input[p.pos] == ']') {
            p.pos += 1;
            return JSON_VAL_ARRAY;
        }
        while (true) {
            * = pjsParseValue(p, depth + 1);
            if (p.has_err) return JSON_VAL_NONE;

            pjsSkipWs(p);
            if (p.pos >= p.input.len) {
                pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
                return JSON_VAL_NONE;
            }
            const sep = p.input[p.pos];
            if (sep == ',') {
                const comma_pos = p.pos;
                p.pos += 1;
                pjsSkipWs(p);
                if (p.pos >= p.input.len) {
                    pjsSetErr(p, JsonDiagKind.UnexpectedEnd, p.pos);
                    return JSON_VAL_NONE;
                }
                if (p.input[p.pos] == ']') {
                    pjsSetErr(p, JsonDiagKind.TrailingComma, comma_pos);
                    return JSON_VAL_NONE;
                }
                continue;
            }
            if (sep == ']') {
                p.pos += 1;
                return JSON_VAL_ARRAY;
            }
            pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
            return JSON_VAL_NONE;
        }
    }

    if (ch == '"') {
        pjsParseString(p);
        return JSON_VAL_STRING;
    }
    if (ch == '\'') {
        pjsSetErr(p, JsonDiagKind.SingleQuotedString, p.pos);
        return JSON_VAL_NONE;
    }
    if (ch == '-' || pjsIsDigit(ch)) {
        pjsParseNumber(p);
        return JSON_VAL_NUMBER;
    }
    if (ch == 't') {
        pjsExpectKeyword(p, "true");
        return JSON_VAL_BOOL;
    }
    if (ch == 'f') {
        pjsExpectKeyword(p, "false");
        return JSON_VAL_BOOL;
    }
    if (ch == 'n') {
        pjsExpectKeyword(p, "null");
        return JSON_VAL_NULL;
    }

    pjsSetErr(p, JsonDiagKind.UnexpectedToken, p.pos);
    return JSON_VAL_NONE;
}

pub fn validateJsonWithDiag(input: string): JsonDiagResult {
    let p = PjsParser{
        input: input,
        pos: 0,
        has_err: false,
        err_kind: JsonDiagKind.Ok,
        err_offset: 0,
    };

    pjsSkipWs(*p);
    if (p.pos >= input.len) {
        return JsonDiagResult{
            ok: false,
            kind: JsonDiagKind.EmptyInput,
            offset: p.pos,
            line: 1,
            column: 1,
            value_kind: JSON_VAL_NONE,
        };
    }

    const vkind = pjsParseValue(*p, 0);
    if (p.has_err) {
        const loc = pjsLineCol(input, p.err_offset);
        return JsonDiagResult{
            ok: false,
            kind: p.err_kind,
            offset: p.err_offset,
            line: loc.line,
            column: loc.column,
            value_kind: JSON_VAL_NONE,
        };
    }

    pjsSkipWs(*p);
    if (p.pos < input.len) {
        const loc = pjsLineCol(input, p.pos);
        return JsonDiagResult{
            ok: false,
            kind: JsonDiagKind.UnexpectedToken,
            offset: p.pos,
            line: loc.line,
            column: loc.column,
            value_kind: JSON_VAL_NONE,
        };
    }

    return JsonDiagResult{
        ok: true,
        kind: JsonDiagKind.Ok,
        offset: input.len,
        line: 1,
        column: 1,
        value_kind: vkind,
    };
}

fn pjsDiagMessage(kind: JsonDiagKind): string {
    if (kind == JsonDiagKind.Ok) return "OK";
    if (kind == JsonDiagKind.EmptyInput) return "Unexpected end of JSON input (empty input)";
    if (kind == JsonDiagKind.UnexpectedToken) return "Unexpected token in JSON";
    if (kind == JsonDiagKind.UnexpectedEnd) return "Unexpected end of JSON input";
    if (kind == JsonDiagKind.TrailingComma) return "Trailing comma in JSON";
    if (kind == JsonDiagKind.SingleQuotedString) return "Single-quoted string is not valid JSON";
    if (kind == JsonDiagKind.UnquotedKey) return "Unquoted property name is not valid JSON";
    if (kind == JsonDiagKind.LeadingZero) return "Leading zeros in decimal number are not allowed in JSON";
    if (kind == JsonDiagKind.UnescapedControlChar) return "Unescaped control character in JSON string";
    return "Maximum JSON nesting depth exceeded";
}

fn pjsWriteByte(w: *PjsWriter, b: u8) {
    if (w.pos < w.buf.len) {
        w.buf[w.pos] = b;
        w.pos += 1;
    }
}

fn pjsWriteStr(w: *PjsWriter, s: string) {
    if (w.pos >= w.buf.len || s.len == 0) return;
    const avail = w.buf.len - w.pos;
    const n = if (s.len < avail) s.len else avail;
    @memcpy(w.buf[w.pos .. w.pos + n], s[0..n]);
    w.pos += n;
}

fn pjsNumWidth(n: usize): usize {
    if (n < 10) return 1;
    let v = n;
    let d: usize = 0;
    while (v > 0) : (v /= 10) {
        d += 1;
    }
    return d;
}

fn pjsWriteUsize(w: *PjsWriter, n: usize) {
    if (n == 0) {
        pjsWriteByte(w, '0');
        return;
    }
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    let v = n;
    while (v > 0 && len < 24) : (v /= 10) {
        digits[len] = u8('0' + (v % 10));
        len += 1;
    }
    while (len > 0) {
        len -= 1;
        pjsWriteByte(w, digits[len]);
    }
}

fn pjsWriteSpaces(w: *PjsWriter, count: usize) {
    if (w.pos >= w.buf.len || count == 0) return;
    const avail = w.buf.len - w.pos;
    let rem = if (count < avail) count else avail;
    while (rem >= 64) {
        @memcpy(w.buf[w.pos .. w.pos + 64], PJS_SPACES_64[0..64]);
        w.pos += 64;
        rem -= 64;
    }
    if (rem > 0) {
        @memcpy(w.buf[w.pos .. w.pos + rem], PJS_SPACES_64[0..rem]);
        w.pos += rem;
    }
}

fn pjsFindLineSpan(input: string, target_line: usize): PjsLineSpan {
    if (target_line == 0 || input.len == 0) {
        return PjsLineSpan{ found: false, start: 0, end: 0 };
    }
    let cur_line: usize = 1;
    let line_start: usize = 0;
    while (true) {
        const i = pjsFindByteSimd(input, '\n' as u8, line_start);
        let line_end = i;
        if (line_end > line_start && input[line_end - 1] == '\r') {
            line_end -= 1;
        }
        if (cur_line == target_line) {
            return PjsLineSpan{ found: true, start: line_start, end: line_end };
        }
        if (i >= input.len) {
            break;
        }
        cur_line += 1;
        line_start = i + 1;
    }
    return PjsLineSpan{ found: false, start: 0, end: 0 };
}

fn pjsWriteFrameLine(w: *PjsWriter, input: string, line_no: usize, span: PjsLineSpan, gutter_w: usize, is_err: bool) {
    if (is_err) {
        pjsWriteStr(w, "> ");
    } else {
        pjsWriteStr(w, "  ");
    }
    const nw = pjsNumWidth(line_no);
    if (gutter_w > nw) {
        pjsWriteSpaces(w, gutter_w - nw);
    }
    pjsWriteUsize(w, line_no);
    pjsWriteStr(w, " | ");
    pjsWriteStr(w, input[span.start..span.end]);
    pjsWriteByte(w, '\n');
}

pub fn formatJsonError(input: string, diag: JsonDiagResult, filename: string, out: []u8): usize {
    if (diag.ok || out.len == 0) return 0;

    let w = PjsWriter{ buf: out, pos: 0 };
    pjsWriteStr(*w, "JSONError: ");
    pjsWriteStr(*w, pjsDiagMessage(diag.kind));
    pjsWriteStr(*w, " at line ");
    pjsWriteUsize(*w, diag.line);
    pjsWriteStr(*w, ", column ");
    pjsWriteUsize(*w, diag.column);
    if (filename.len > 0) {
        pjsWriteStr(*w, " in ");
        pjsWriteStr(*w, filename);
    }
    pjsWriteByte(*w, '\n');

    if (input.len == 0) {
        return w.pos;
    }

    const prev_span = if (diag.line > 1) pjsFindLineSpan(input, diag.line - 1) else PjsLineSpan{ found: false, start: 0, end: 0 };
    const err_span = pjsFindLineSpan(input, diag.line);
    const next_span = pjsFindLineSpan(input, diag.line + 1);

    const max_line = if (next_span.found) diag.line + 1 else diag.line;
    const nw = pjsNumWidth(max_line);
    const gutter_w = if (nw < 2) 2 else nw;

    if (prev_span.found) {
        pjsWriteFrameLine(*w, input, diag.line - 1, prev_span, gutter_w, false);
    }
    if (err_span.found) {
        pjsWriteFrameLine(*w, input, diag.line, err_span, gutter_w, true);
        pjsWriteStr(*w, "  ");
        pjsWriteSpaces(*w, gutter_w);
        pjsWriteStr(*w, " | ");
        const caret_pad = if (diag.column > 1) diag.column - 1 else 0;
        pjsWriteSpaces(*w, caret_pad);
        pjsWriteStr(*w, "^\n");
    }
    if (next_span.found) {
        pjsWriteFrameLine(*w, input, diag.line + 1, next_span, gutter_w, false);
    }

    return w.pos;
}
