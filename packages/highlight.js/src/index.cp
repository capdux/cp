// @cap/syntax-highlight — Zero-allocation multi-language lexer & ANSI/HTML syntax highlighter for CP
// Clean-room MIT implementation inspired by npm `highlight.js` (58M/wk) and `prismjs` (50M/wk).

import { isSpace, isDigit, isHexDigit, isAlpha, strEq, eqIgnoreCase } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const TOK_PLAIN: u8 = 0;
pub const TOK_KEYWORD: u8 = 1;
pub const TOK_STRING: u8 = 2;
pub const TOK_NUMBER: u8 = 3;
pub const TOK_COMMENT: u8 = 4;
pub const TOK_OPERATOR: u8 = 5;
pub const TOK_TYPE: u8 = 6;
pub const TOK_PUNCT: u8 = 7;

pub const LANG_CODE: u8 = 0;
pub const LANG_JSON: u8 = 1;
pub const LANG_SQL: u8 = 2;

pub struct Token {
    pub kind: u8,
    pub start: usize,
    pub len: usize,
}

fn isIdentStart(c: u8) -> bool {
    return isAlpha(c) || c == '_';
}

fn isIdentCont(c: u8) -> bool {
    return isIdentStart(c) || isDigit(c);
}

fn sliceEq(src: []const u8, start: usize, length: usize, lit: string) -> bool {
    return strEq(src[start..(start + length)], lit);
}

fn sliceEqIgnoreCase(src: []const u8, start: usize, length: usize, upperLit: string) -> bool {
    return eqIgnoreCase(src[start..(start + length)], upperLit);
}

fn isCodeKeyword(src: []const u8, s: usize, n: usize) -> bool {
    if (sliceEq(src, s, n, "fn")) { return true; }
    if (sliceEq(src, s, n, "pub")) { return true; }
    if (sliceEq(src, s, n, "let")) { return true; }
    if (sliceEq(src, s, n, "const")) { return true; }
    if (sliceEq(src, s, n, "return")) { return true; }
    if (sliceEq(src, s, n, "if")) { return true; }
    if (sliceEq(src, s, n, "else")) { return true; }
    if (sliceEq(src, s, n, "while")) { return true; }
    if (sliceEq(src, s, n, "for")) { return true; }
    if (sliceEq(src, s, n, "struct")) { return true; }
    if (sliceEq(src, s, n, "enum")) { return true; }
    if (sliceEq(src, s, n, "import")) { return true; }
    if (sliceEq(src, s, n, "export")) { return true; }
    if (sliceEq(src, s, n, "true")) { return true; }
    if (sliceEq(src, s, n, "false")) { return true; }
    if (sliceEq(src, s, n, "null")) { return true; }
    if (sliceEq(src, s, n, "static")) { return true; }
    if (sliceEq(src, s, n, "switch")) { return true; }
    if (sliceEq(src, s, n, "break")) { return true; }
    if (sliceEq(src, s, n, "continue")) { return true; }
    if (sliceEq(src, s, n, "as")) { return true; }
    if (sliceEq(src, s, n, "match")) { return true; }
    if (sliceEq(src, s, n, "impl")) { return true; }
    if (sliceEq(src, s, n, "trait")) { return true; }
    if (sliceEq(src, s, n, "async")) { return true; }
    if (sliceEq(src, s, n, "await")) { return true; }
    if (sliceEq(src, s, n, "class")) { return true; }
    if (sliceEq(src, s, n, "function")) { return true; }
    if (sliceEq(src, s, n, "var")) { return true; }
    return false;
}

fn isCodeType(src: []const u8, s: usize, n: usize) -> bool {
    if (sliceEq(src, s, n, "i8")) { return true; }
    if (sliceEq(src, s, n, "i16")) { return true; }
    if (sliceEq(src, s, n, "i32")) { return true; }
    if (sliceEq(src, s, n, "i64")) { return true; }
    if (sliceEq(src, s, n, "u8")) { return true; }
    if (sliceEq(src, s, n, "u16")) { return true; }
    if (sliceEq(src, s, n, "u32")) { return true; }
    if (sliceEq(src, s, n, "u64")) { return true; }
    if (sliceEq(src, s, n, "usize")) { return true; }
    if (sliceEq(src, s, n, "isize")) { return true; }
    if (sliceEq(src, s, n, "f32")) { return true; }
    if (sliceEq(src, s, n, "f64")) { return true; }
    if (sliceEq(src, s, n, "bool")) { return true; }
    if (sliceEq(src, s, n, "string")) { return true; }
    if (sliceEq(src, s, n, "void")) { return true; }
    return false;
}

fn isJsonKeyword(src: []const u8, s: usize, n: usize) -> bool {
    if (sliceEq(src, s, n, "true")) { return true; }
    if (sliceEq(src, s, n, "false")) { return true; }
    if (sliceEq(src, s, n, "null")) { return true; }
    return false;
}

fn isSqlKeyword(src: []const u8, s: usize, n: usize) -> bool {
    if (sliceEqIgnoreCase(src, s, n, "SELECT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "FROM")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "WHERE")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "INSERT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "INTO")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "VALUES")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "UPDATE")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "SET")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "DELETE")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "CREATE")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "TABLE")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "DROP")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "ALTER")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "JOIN")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "LEFT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "RIGHT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "INNER")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "ON")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "AND")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "OR")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "NOT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "ORDER")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "BY")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "GROUP")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "HAVING")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "LIMIT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "OFFSET")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "AS")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "IN")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "IS")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "NULL")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "TRUE")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "FALSE")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "DISTINCT")) { return true; }
    return false;
}

fn isSqlType(src: []const u8, s: usize, n: usize) -> bool {
    if (sliceEqIgnoreCase(src, s, n, "INT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "INTEGER")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "BIGINT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "VARCHAR")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "TEXT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "BOOLEAN")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "FLOAT")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "DOUBLE")) { return true; }
    if (sliceEqIgnoreCase(src, s, n, "TIMESTAMP")) { return true; }
    return false;
}

fn isOperatorByte(c: u8) -> bool {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '=' || c == '!' || c == '<' || c == '>' || c == '&' || c == '|' || c == '^' || c == '~' || c == '?';
}

fn isPunctByte(c: u8) -> bool {
    return c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == ',' || c == ';' || c == ':' || c == '.';
}

fn nextToken(src: []const u8, pos: usize, lang: u8) -> Token {
    const start = pos;
    let i = pos;
    const c = src[i];

    // 1. Whitespace run
    if (isSpace(c)) {
        while (i < src.len && isSpace(src[i])) {
            i += 1;
        }
        return Token{ kind: TOK_PLAIN, start: start, len: i - start };
    }

    // 2. Comments
    if (lang == LANG_CODE && c == '/' && i + 1 < src.len && src[i + 1] == '/') {
        i += 2;
        while (i < src.len && src[i] != '\n') {
            i += 1;
        }
        return Token{ kind: TOK_COMMENT, start: start, len: i - start };
    }
    if (lang == LANG_SQL && c == '-' && i + 1 < src.len && src[i + 1] == '-') {
        i += 2;
        while (i < src.len && src[i] != '\n') {
            i += 1;
        }
        return Token{ kind: TOK_COMMENT, start: start, len: i - start };
    }
    if ((lang == LANG_CODE || lang == LANG_SQL) && c == '/' && i + 1 < src.len && src[i + 1] == '*') {
        i += 2;
        while (i + 1 < src.len) {
            if (src[i] == '*' && src[i + 1] == '/') {
                i += 2;
                return Token{ kind: TOK_COMMENT, start: start, len: i - start };
            }
            i += 1;
        }
        return Token{ kind: TOK_COMMENT, start: start, len: src.len - start };
    }

    // 3. Strings ("..." and '...' for code/SQL)
    if (c == '"' || ((lang == LANG_CODE || lang == LANG_SQL) && c == '\'')) {
        const quote = c;
        i += 1;
        while (i < src.len) {
            if (src[i] == '\\' && i + 1 < src.len) {
                i += 2;
                continue;
            }
            if (src[i] == quote) {
                i += 1;
                break;
            }
            i += 1;
        }
        return Token{ kind: TOK_STRING, start: start, len: i - start };
    }

    // 4. Numbers (hex 0x..., float, integer)
    if (isDigit(c)) {
        if (c == '0' && i + 1 < src.len && (src[i + 1] == 'x' || src[i + 1] == 'X')) {
            i += 2;
            while (i < src.len && (isHexDigit(src[i]) || src[i] == '_')) {
                i += 1;
            }
            return Token{ kind: TOK_NUMBER, start: start, len: i - start };
        }
        while (i < src.len && (isDigit(src[i]) || src[i] == '_')) {
            i += 1;
        }
        if (i + 1 < src.len && src[i] == '.' && isDigit(src[i + 1])) {
            i += 1;
            while (i < src.len && (isDigit(src[i]) || src[i] == '_')) {
                i += 1;
            }
        }
        if (i < src.len && (src[i] == 'e' || src[i] == 'E')) {
            let ePos = i + 1;
            if (ePos < src.len && (src[ePos] == '+' || src[ePos] == '-')) {
                ePos += 1;
            }
            if (ePos < src.len && isDigit(src[ePos])) {
                i = ePos;
                while (i < src.len && isDigit(src[i])) {
                    i += 1;
                }
            }
        }
        return Token{ kind: TOK_NUMBER, start: start, len: i - start };
    }

    // 5. Identifiers / keywords / types
    if (isIdentStart(c)) {
        i += 1;
        while (i < src.len && isIdentCont(src[i])) {
            i += 1;
        }
        const n = i - start;
        let kind: u8 = TOK_PLAIN;
        if (lang == LANG_CODE) {
            if (isCodeKeyword(src, start, n)) {
                kind = TOK_KEYWORD;
            } else if (isCodeType(src, start, n)) {
                kind = TOK_TYPE;
            }
        } else if (lang == LANG_JSON) {
            if (isJsonKeyword(src, start, n)) {
                kind = TOK_KEYWORD;
            }
        } else if (lang == LANG_SQL) {
            if (isSqlKeyword(src, start, n)) {
                kind = TOK_KEYWORD;
            } else if (isSqlType(src, start, n)) {
                kind = TOK_TYPE;
            }
        }
        return Token{ kind: kind, start: start, len: n };
    }

    // 6. Operators
    if (isOperatorByte(c)) {
        i += 1;
        // Group common 2-char operators (==, !=, <=, >=, &&, ||, ->, =>)
        if (i < src.len && isOperatorByte(src[i])) {
            const nxt = src[i];
            if ((c == '=' && (nxt == '=' || nxt == '>')) || (c == '!' && nxt == '=') || (c == '<' && nxt == '=') || (c == '>' && nxt == '=') || (c == '-' && nxt == '>') || (c == '&' && nxt == '&') || (c == '|' && nxt == '|')) {
                i += 1;
            }
        }
        return Token{ kind: TOK_OPERATOR, start: start, len: i - start };
    }

    // 7. Punctuation
    if (isPunctByte(c)) {
        return Token{ kind: TOK_PUNCT, start: start, len: 1 as usize };
    }

    // Fallback single byte
    return Token{ kind: TOK_PLAIN, start: start, len: 1 as usize };
}

pub fn tokenize(src: []const u8, lang: u8, out: []Token) -> usize {
    let pos: usize = 0;
    let count: usize = 0;
    while (pos < src.len) {
        const tok = nextToken(src, pos, lang);
        if (count < out.len) {
            out[count] = tok;
        }
        count += 1;
        pos += tok.len;
    }
    return count;
}

fn writeStrChecked(dst: []u8, pos: usize, s: string) -> i64 {
    if (pos + s.len > dst.len) {
        return -1 as i64;
    }
    return writeStr(s, dst, pos) as i64;
}

fn writeSliceChecked(dst: []u8, pos: usize, src: []const u8, start: usize, length: usize) -> i64 {
    return writeStrChecked(dst, pos, src[start..(start + length)]);
}

fn writeHtmlEscaped(dst: []u8, pos_in: usize, src: []const u8, start: usize, length: usize) -> i64 {
    let pos = pos_in;
    let i: usize = 0;
    while (i < length) {
        const c = src[start + i];
        if (c == '<') {
            const nxt = writeStrChecked(dst, pos, "&lt;");
            if (nxt < (0 as i64)) { return -1 as i64; }
            pos = nxt as usize;
        } else if (c == '>') {
            const nxt = writeStrChecked(dst, pos, "&gt;");
            if (nxt < (0 as i64)) { return -1 as i64; }
            pos = nxt as usize;
        } else if (c == '&') {
            const nxt = writeStrChecked(dst, pos, "&amp;");
            if (nxt < (0 as i64)) { return -1 as i64; }
            pos = nxt as usize;
        } else if (c == '"') {
            const nxt = writeStrChecked(dst, pos, "&quot;");
            if (nxt < (0 as i64)) { return -1 as i64; }
            pos = nxt as usize;
        } else {
            if (pos + (1 as usize) > dst.len) {
                return -1 as i64;
            }
            dst[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return pos as i64;
}

fn ansiCodeFor(kind: u8) -> string {
    if (kind == TOK_KEYWORD) { return "[35m"; }
    if (kind == TOK_STRING) { return "[32m"; }
    if (kind == TOK_NUMBER) { return "[33m"; }
    if (kind == TOK_COMMENT) { return "[90m"; }
    if (kind == TOK_OPERATOR) { return "[36m"; }
    if (kind == TOK_TYPE) { return "[34m"; }
    if (kind == TOK_PUNCT) { return "[37m"; }
    return "";
}

fn htmlClassFor(kind: u8) -> string {
    if (kind == TOK_KEYWORD) { return "hl-kw"; }
    if (kind == TOK_STRING) { return "hl-str"; }
    if (kind == TOK_NUMBER) { return "hl-num"; }
    if (kind == TOK_COMMENT) { return "hl-cmt"; }
    if (kind == TOK_OPERATOR) { return "hl-op"; }
    if (kind == TOK_TYPE) { return "hl-ty"; }
    if (kind == TOK_PUNCT) { return "hl-punc"; }
    return "";
}

pub fn highlightAnsi(src: []const u8, lang: u8, dst: []u8) -> i64 {
    let pos: usize = 0;
    let outPos: usize = 0;
    while (pos < src.len) {
        const tok = nextToken(src, pos, lang);
        const code = ansiCodeFor(tok.kind);
        if (code.len > (0 as usize)) {
            if (outPos + (1 as usize) > dst.len) { return -1 as i64; }
            dst[outPos] = 0x1B as u8;
            outPos += 1;
            const p1 = writeStrChecked(dst, outPos, code);
            if (p1 < (0 as i64)) { return -1 as i64; }
            outPos = p1 as usize;

            const p2 = writeSliceChecked(dst, outPos, src, tok.start, tok.len);
            if (p2 < (0 as i64)) { return -1 as i64; }
            outPos = p2 as usize;

            if (outPos + (1 as usize) > dst.len) { return -1 as i64; }
            dst[outPos] = 0x1B as u8;
            outPos += 1;
            const p3 = writeStrChecked(dst, outPos, "[0m");
            if (p3 < (0 as i64)) { return -1 as i64; }
            outPos = p3 as usize;
        } else {
            const pRaw = writeSliceChecked(dst, outPos, src, tok.start, tok.len);
            if (pRaw < (0 as i64)) { return -1 as i64; }
            outPos = pRaw as usize;
        }
        pos += tok.len;
    }
    return outPos as i64;
}

pub fn highlightHtml(src: []const u8, lang: u8, dst: []u8) -> i64 {
    let pos: usize = 0;
    let outPos: usize = 0;
    while (pos < src.len) {
        const tok = nextToken(src, pos, lang);
        const cls = htmlClassFor(tok.kind);
        if (cls.len > (0 as usize)) {
            const p1 = writeStrChecked(dst, outPos, "<span class=\"");
            if (p1 < (0 as i64)) { return -1 as i64; }
            outPos = p1 as usize;

            const p2 = writeStrChecked(dst, outPos, cls);
            if (p2 < (0 as i64)) { return -1 as i64; }
            outPos = p2 as usize;

            const p3 = writeStrChecked(dst, outPos, "\">");
            if (p3 < (0 as i64)) { return -1 as i64; }
            outPos = p3 as usize;

            const p4 = writeHtmlEscaped(dst, outPos, src, tok.start, tok.len);
            if (p4 < (0 as i64)) { return -1 as i64; }
            outPos = p4 as usize;

            const p5 = writeStrChecked(dst, outPos, "</span>");
            if (p5 < (0 as i64)) { return -1 as i64; }
            outPos = p5 as usize;
        } else {
            const pRaw = writeHtmlEscaped(dst, outPos, src, tok.start, tok.len);
            if (pRaw < (0 as i64)) { return -1 as i64; }
            outPos = pRaw as usize;
        }
        pos += tok.len;
    }
    return outPos as i64;
}

export {
    TOK_PLAIN,
    TOK_KEYWORD,
    TOK_STRING,
    TOK_NUMBER,
    TOK_COMMENT,
    TOK_OPERATOR,
    TOK_TYPE,
    TOK_PUNCT,
    LANG_CODE,
    LANG_JSON,
    LANG_SQL,
    Token,
    tokenize,
    highlightAnsi,
    highlightHtml,
};
