// @cap/cssparser — Zero-heap CSS Syntax Level 3 lexer in pure CP.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning and comptime 256-entry character classification LUT.

type CsspVec16 = @Vector(16, u8);

const CSSP_FLAG_WS: u8 = 1;
const CSSP_FLAG_DIGIT: u8 = 2;
const CSSP_FLAG_IDENT_START: u8 = 4;
const CSSP_FLAG_IDENT_CHAR: u8 = 8;

fn csspBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    // Whitespace: ' ', '\t', '\n', '\r', '\x0b', '\x0c'
    lut[9] = CSSP_FLAG_WS;
    lut[10] = CSSP_FLAG_WS;
    lut[11] = CSSP_FLAG_WS;
    lut[12] = CSSP_FLAG_WS;
    lut[13] = CSSP_FLAG_WS;
    lut[32] = CSSP_FLAG_WS;

    // Digits '0'..'9'
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = CSSP_FLAG_DIGIT | CSSP_FLAG_IDENT_CHAR;
        c += 1;
    }
    // Uppercase 'A'..'Z'
    c = 65;
    while (c <= 90) {
        lut[c] = CSSP_FLAG_IDENT_START | CSSP_FLAG_IDENT_CHAR;
        c += 1;
    }
    // Lowercase 'a'..'z'
    c = 97;
    while (c <= 122) {
        lut[c] = CSSP_FLAG_IDENT_START | CSSP_FLAG_IDENT_CHAR;
        c += 1;
    }
    // '_' and '-'
    lut[95] = CSSP_FLAG_IDENT_START | CSSP_FLAG_IDENT_CHAR;
    lut[45] = CSSP_FLAG_IDENT_CHAR;
    // Non-ASCII >= 0x80
    c = 128;
    while (c < 256) {
        lut[c] = CSSP_FLAG_IDENT_START | CSSP_FLAG_IDENT_CHAR;
        c += 1;
    }
    return lut;
}

const CSSP_CHAR_LUT: [256]u8 = csspBuildCharLut();

pub enum CssTokKind {
    Ident,
    AtKeyword,
    Hash,
    String,
    Number,
    Percentage,
    Dimension,
    Function,
    Colon,
    Semicolon,
    Comma,
    LBrace,
    RBrace,
    LParen,
    RParen,
    LBracket,
    RBracket,
    Delim,
    Eof,
}

pub struct CssToken {
    kind: CssTokKind,
    text: string,
    unit: string,
    start: usize,
    end: usize,
}

fn csspIsSpace(c: u8): bool {
    return (CSSP_CHAR_LUT[c as usize] & CSSP_FLAG_WS) != 0;
}

fn csspIsDigit(c: u8): bool {
    return (CSSP_CHAR_LUT[c as usize] & CSSP_FLAG_DIGIT) != 0;
}

fn csspIsIdentStart(c: u8): bool {
    return (CSSP_CHAR_LUT[c as usize] & CSSP_FLAG_IDENT_START) != 0;
}

fn csspIsIdentChar(c: u8): bool {
    return (CSSP_CHAR_LUT[c as usize] & CSSP_FLAG_IDENT_CHAR) != 0;
}

fn csspStartsIdent(s: string, pos: usize): bool {
    if (pos >= s.len) {
        return false;
    }
    const c = s[pos];
    if (csspIsIdentStart(c)) {
        return true;
    }
    if (c == '-') {
        if (pos + 1 < s.len) {
            const next = s[pos + 1];
            return csspIsIdentStart(next) || next == '-';
        }
    }
    return false;
}

fn csspStartsNumber(s: string, pos: usize): bool {
    if (pos >= s.len) {
        return false;
    }
    const c = s[pos];
    if (csspIsDigit(c)) {
        return true;
    }
    if (c == '.') {
        return pos + 1 < s.len && csspIsDigit(s[pos + 1]);
    }
    if (c == '+' || c == '-') {
        if (pos + 1 < s.len && csspIsDigit(s[pos + 1])) {
            return true;
        }
        if (pos + 2 < s.len && s[pos + 1] == '.' && csspIsDigit(s[pos + 2])) {
            return true;
        }
    }
    return false;
}

fn csspScanIdent(s: string, start: usize): usize {
    let p = start;
    const v_zero: CsspVec16 = @splat(0 as u8);
    const v_one: CsspVec16 = @splat(1 as u8);
    const v_lo_a: CsspVec16 = @splat(97 as u8);
    const v_lo_z: CsspVec16 = @splat(122 as u8);
    const v_up_a: CsspVec16 = @splat(65 as u8);
    const v_up_z: CsspVec16 = @splat(90 as u8);
    const v_d0: CsspVec16 = @splat(48 as u8);
    const v_d9: CsspVec16 = @splat(57 as u8);
    const v_us: CsspVec16 = @splat(95 as u8);
    const v_hy: CsspVec16 = @splat(45 as u8);
    const v_hi: CsspVec16 = @splat(128 as u8);

    while (p + 16 <= s.len) {
        const v: CsspVec16 = s[p .. p + 16][0..16].*;
        const m_lo = @select(u8, v >= v_lo_a, v_one, v_zero) & @select(u8, v <= v_lo_z, v_one, v_zero);
        const m_up = @select(u8, v >= v_up_a, v_one, v_zero) & @select(u8, v <= v_up_z, v_one, v_zero);
        const m_dg = @select(u8, v >= v_d0, v_one, v_zero) & @select(u8, v <= v_d9, v_one, v_zero);
        const m_sp = @select(u8, v == v_us, v_one, v_zero)
            | @select(u8, v == v_hy, v_one, v_zero)
            | @select(u8, v >= v_hi, v_one, v_zero);
        const ok = m_lo | m_up | m_dg | m_sp;
        if (@reduce(.And, ok != v_zero)) {
            p += 16;
            continue;
        }
        break;
    }
    while (p < s.len && csspIsIdentChar(s[p])) : (p += 1) {}
    return p;
}

fn csspScanDigits(s: string, start: usize): usize {
    let p = start;
    const v_zero: CsspVec16 = @splat(0 as u8);
    const v_one: CsspVec16 = @splat(1 as u8);
    const v_d0: CsspVec16 = @splat(48 as u8);
    const v_d9: CsspVec16 = @splat(57 as u8);
    while (p + 16 <= s.len) {
        const v: CsspVec16 = s[p .. p + 16][0..16].*;
        const ok = @select(u8, v >= v_d0, v_one, v_zero) & @select(u8, v <= v_d9, v_one, v_zero);
        if (@reduce(.And, ok != v_zero)) {
            p += 16;
            continue;
        }
        break;
    }
    while (p < s.len && csspIsDigit(s[p])) : (p += 1) {}
    return p;
}

fn csspScanNumber(s: string, start: usize): usize {
    let p = start;
    if (p < s.len && (s[p] == '+' || s[p] == '-')) {
        p += 1;
    }
    p = csspScanDigits(s, p);
    if (p + 1 < s.len && s[p] == '.' && csspIsDigit(s[p + 1])) {
        p += 1;
        p = csspScanDigits(s, p);
    }
    if (p < s.len && (s[p] == 'e' || s[p] == 'E')) {
        let ep = p + 1;
        if (ep < s.len && (s[ep] == '+' || s[ep] == '-')) {
            ep += 1;
        }
        if (ep < s.len && csspIsDigit(s[ep])) {
            p = csspScanDigits(s, ep);
        }
    }
    return p;
}

fn csspSkipWsAndComments(s: string, start: usize): usize {
    let p = start;
    const v_zero: CsspVec16 = @splat(0 as u8);
    const v_one: CsspVec16 = @splat(1 as u8);
    const v_sp: CsspVec16 = @splat(32 as u8);
    const v_t0: CsspVec16 = @splat(9 as u8);
    const v_t1: CsspVec16 = @splat(13 as u8);
    const v_star: CsspVec16 = @splat(42 as u8);

    while (p < s.len) {
        if (csspIsSpace(s[p])) {
            while (p + 16 <= s.len) {
                const v: CsspVec16 = s[p .. p + 16][0..16].*;
                const m_sp = @select(u8, v == v_sp, v_one, v_zero);
                const m_ctl = @select(u8, v >= v_t0, v_one, v_zero) & @select(u8, v <= v_t1, v_one, v_zero);
                const ws = m_sp | m_ctl;
                if (@reduce(.And, ws != v_zero)) {
                    p += 16;
                    continue;
                }
                break;
            }
            while (p < s.len && csspIsSpace(s[p])) {
                p += 1;
            }
            continue;
        }
        if (p + 1 < s.len && s[p] == '/' && s[p + 1] == '*') {
            p += 2;
            while (p + 1 < s.len) {
                while (p + 16 < s.len) {
                    const v: CsspVec16 = s[p .. p + 16][0..16].*;
                    if (!@reduce(.Or, v == v_star)) {
                        p += 16;
                        continue;
                    }
                    break;
                }
                if (p + 1 < s.len && s[p] == '*' && s[p + 1] == '/') {
                    break;
                }
                p += 1;
            }
            if (p + 1 < s.len) {
                p += 2;
            } else {
                p = s.len;
            }
            continue;
        }
        break;
    }
    return p;
}

fn csspScanStringEnd(s: string, start: usize, quote: u8): usize {
    let p = start;
    const v_q: CsspVec16 = @splat(quote);
    const v_bs: CsspVec16 = @splat(92 as u8);
    while (p < s.len) {
        while (p + 16 <= s.len) {
            const v: CsspVec16 = s[p .. p + 16][0..16].*;
            if (!@reduce(.Or, v == v_q) && !@reduce(.Or, v == v_bs)) {
                p += 16;
                continue;
            }
            break;
        }
        if (p >= s.len || s[p] == quote) {
            break;
        }
        if (s[p] == '\\' && p + 1 < s.len) {
            p += 2;
        } else {
            p += 1;
        }
    }
    return p;
}

pub struct CssLexer {
    src: string,
    pos: usize,

    static fn new(src: string): CssLexer {
        return CssLexer{
            src: src,
            pos: 0,
        };
    }

    fn nextToken(*_): CssToken {
        _.pos = csspSkipWsAndComments(_.src, _.pos);
        const start = _.pos;
        if (start >= _.src.len) {
            return CssToken{
                kind: CssTokKind.Eof,
                text: "",
                unit: "",
                start: start,
                end: start,
            };
        }

        const c = _.src[start];

        if (c == ':') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.Colon, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }
        if (c == ';') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.Semicolon, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }
        if (c == ',') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.Comma, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }
        if (c == '{') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.LBrace, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }
        if (c == '}') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.RBrace, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }
        if (c == '(') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.LParen, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }
        if (c == ')') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.RParen, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }
        if (c == '[') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.LBracket, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }
        if (c == ']') {
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.RBracket, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }

        if (c == '"' || c == '\'') {
            const quote = c;
            let p = csspScanStringEnd(_.src, start + 1, quote);
            const contentEnd = p;
            if (p < _.src.len && _.src[p] == quote) {
                p += 1;
            }
            _.pos = p;
            return CssToken{
                kind: CssTokKind.String,
                text: _.src[start + 1..contentEnd],
                unit: "",
                start: start,
                end: p,
            };
        }

        if (c == '#') {
            if (start + 1 < _.src.len && csspIsIdentChar(_.src[start + 1])) {
                const end = csspScanIdent(_.src, start + 1);
                _.pos = end;
                return CssToken{
                    kind: CssTokKind.Hash,
                    text: _.src[start + 1..end],
                    unit: "",
                    start: start,
                    end: end,
                };
            }
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.Delim, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }

        if (c == '@') {
            if (csspStartsIdent(_.src, start + 1)) {
                const end = csspScanIdent(_.src, start + 1);
                _.pos = end;
                return CssToken{
                    kind: CssTokKind.AtKeyword,
                    text: _.src[start + 1..end],
                    unit: "",
                    start: start,
                    end: end,
                };
            }
            _.pos = start + 1;
            return CssToken{ kind: CssTokKind.Delim, text: _.src[start.._.pos], unit: "", start: start, end: _.pos };
        }

        if (csspStartsNumber(_.src, start)) {
            const numEnd = csspScanNumber(_.src, start);
            if (numEnd < _.src.len && _.src[numEnd] == '%') {
                _.pos = numEnd + 1;
                return CssToken{
                    kind: CssTokKind.Percentage,
                    text: _.src[start..numEnd],
                    unit: "%",
                    start: start,
                    end: _.pos,
                };
            }
            if (csspStartsIdent(_.src, numEnd)) {
                const unitEnd = csspScanIdent(_.src, numEnd);
                _.pos = unitEnd;
                return CssToken{
                    kind: CssTokKind.Dimension,
                    text: _.src[start..numEnd],
                    unit: _.src[numEnd..unitEnd],
                    start: start,
                    end: unitEnd,
                };
            }
            _.pos = numEnd;
            return CssToken{
                kind: CssTokKind.Number,
                text: _.src[start..numEnd],
                unit: "",
                start: start,
                end: numEnd,
            };
        }

        if (csspStartsIdent(_.src, start)) {
            const identEnd = csspScanIdent(_.src, start);
            if (identEnd < _.src.len && _.src[identEnd] == '(') {
                _.pos = identEnd + 1;
                return CssToken{
                    kind: CssTokKind.Function,
                    text: _.src[start..identEnd],
                    unit: "",
                    start: start,
                    end: _.pos,
                };
            }
            _.pos = identEnd;
            return CssToken{
                kind: CssTokKind.Ident,
                text: _.src[start..identEnd],
                unit: "",
                start: start,
                end: identEnd,
            };
        }

        _.pos = start + 1;
        return CssToken{
            kind: CssTokKind.Delim,
            text: _.src[start.._.pos],
            unit: "",
            start: start,
            end: _.pos,
        };
    }
}
