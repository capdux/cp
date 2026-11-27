// @cap/cssparser — Zero-heap CSS Syntax Level 3 lexer in pure CP.

import { isSpace, isAlpha, isDigit } from "cap:ascii";

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

fn csspIsIdentStart(c: u8): bool {
    return isAlpha(c) || c == '_' || c >= 0x80;
}

fn csspIsIdentChar(c: u8): bool {
    return csspIsIdentStart(c) || isDigit(c) || c == '-';
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
    if (isDigit(c)) {
        return true;
    }
    if (c == '.') {
        return pos + 1 < s.len && isDigit(s[pos + 1]);
    }
    if (c == '+' || c == '-') {
        if (pos + 1 < s.len && isDigit(s[pos + 1])) {
            return true;
        }
        if (pos + 2 < s.len && s[pos + 1] == '.' && isDigit(s[pos + 2])) {
            return true;
        }
    }
    return false;
}

fn csspScanIdent(s: string, start: usize): usize {
    let p = start;
    while (p < s.len && csspIsIdentChar(s[p])) : (p += 1) {}
    return p;
}

fn csspScanNumber(s: string, start: usize): usize {
    let p = start;
    if (p < s.len && (s[p] == '+' || s[p] == '-')) {
        p += 1;
    }
    while (p < s.len && isDigit(s[p])) : (p += 1) {}
    if (p + 1 < s.len && s[p] == '.' && isDigit(s[p + 1])) {
        p += 1;
        while (p < s.len && isDigit(s[p])) : (p += 1) {}
    }
    if (p < s.len && (s[p] == 'e' || s[p] == 'E')) {
        let ep = p + 1;
        if (ep < s.len && (s[ep] == '+' || s[ep] == '-')) {
            ep += 1;
        }
        if (ep < s.len && isDigit(s[ep])) {
            p = ep;
            while (p < s.len && isDigit(s[p])) : (p += 1) {}
        }
    }
    return p;
}

fn csspSkipWsAndComments(s: string, start: usize): usize {
    let p = start;
    while (p < s.len) {
        if (isSpace(s[p])) {
            p += 1;
            continue;
        }
        if (p + 1 < s.len && s[p] == '/' && s[p + 1] == '*') {
            p += 2;
            while (p + 1 < s.len && !(s[p] == '*' && s[p + 1] == '/')) : (p += 1) {}
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
            let p = start + 1;
            while (p < _.src.len && _.src[p] != quote) {
                if (_.src[p] == '\\' && p + 1 < _.src.len) {
                    p += 2;
                } else {
                    p += 1;
                }
            }
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
