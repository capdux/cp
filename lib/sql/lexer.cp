// Source: cp/lib/sql_ast/lexer.cp (Pure CP SQL Lexer)

const std = @import("std");

pub enum SqlError {
    UnterminatedString,
    UnterminatedComment,
    InvalidCharacter,
    InvalidNumber,
    UnexpectedToken,
    UnexpectedEof,
    MaxDepthExceeded,
    OutOfMemory,
}

pub enum SqlTokenKind {
    Identifier,
    QuotedIdentifier,
    StringLiteral,
    NumberLiteral,
    Placeholder,
    Keyword,
    Operator,
    Comma,
    Semicolon,
    LParen,
    RParen,
    Dot,
    Star,
    Eof,
}

pub struct SqlToken {
    pub kind: SqlTokenKind,
    pub lexeme: string,
    pub line: usize,
    pub col: usize,
}

type SqlLexVec16 = @Vector(16, u8);

const SQL_CH_ALPHA: u8 = 1;
const SQL_CH_DIGIT: u8 = 2;
const SQL_CH_ALNUM: u8 = 3;
const SQL_CH_WS: u8 = 4;

fn makeSqlCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || i == '_') {
            lut[i] |= SQL_CH_ALPHA;
        }
        if (i >= '0' && i <= '9') {
            lut[i] |= SQL_CH_DIGIT;
        }
        if (i == ' ' || i == '\t' || i == '\n' || i == '\r') {
            lut[i] |= SQL_CH_WS;
        }
    }
    return lut;
}

fn makeSqlLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

const SQL_CHAR_LUT: [u8; 256] = makeSqlCharLut();
const SQL_LOWER_LUT: [u8; 256] = makeSqlLowerLut();

fn isAlpha(c: u8): bool {
    return (SQL_CHAR_LUT[usize(c)] & SQL_CH_ALPHA) != 0;
}

fn isDigit(c: u8): bool {
    return (SQL_CHAR_LUT[usize(c)] & SQL_CH_DIGIT) != 0;
}

fn isAlphaNum(c: u8): bool {
    return (SQL_CHAR_LUT[usize(c)] & SQL_CH_ALNUM) != 0;
}

fn isWhitespace(c: u8): bool {
    return (SQL_CHAR_LUT[usize(c)] & SQL_CH_WS) != 0;
}

fn findByteSimd(s: []const u8, start: usize, needle: u8): usize {
    let i = start;
    const v_needle: SqlLexVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: SqlLexVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return s.len;
}

fn findEitherByteSimd(s: []const u8, start: usize, a: u8, b: u8): usize {
    let i = start;
    const va: SqlLexVec16 = @splat(a);
    const vb: SqlLexVec16 = @splat(b);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: SqlLexVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((chunk == va) | (chunk == vb));
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == a || s[i] == b) return i;
    }
    return s.len;
}

fn scanIdentEndSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_a: SqlLexVec16 = @splat('a' as u8);
    const v_z: SqlLexVec16 = @splat('z' as u8);
    const v_A: SqlLexVec16 = @splat('A' as u8);
    const v_Z: SqlLexVec16 = @splat('Z' as u8);
    const v_0: SqlLexVec16 = @splat('0' as u8);
    const v_9: SqlLexVec16 = @splat('9' as u8);
    const v_us: SqlLexVec16 = @splat('_' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: SqlLexVec16 = s[i .. i + 16][0..16].*;
        const ok = ((chunk >= v_a) & (chunk <= v_z)) |
            ((chunk >= v_A) & (chunk <= v_Z)) |
            ((chunk >= v_0) & (chunk <= v_9)) |
            (chunk == v_us);
        const mask: u16 = @bitCast(ok);
        const bad: u16 = ~mask;
        if (bad != 0) {
            return i + usize(@ctz(bad));
        }
    }
    while (i < s.len && isAlphaNum(s[i])) : (i += 1) {}
    return i;
}

pub fn eqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: SqlLexVec16 = @splat(65 as u8);
    const hi_Z: SqlLexVec16 = @splat(90 as u8);
    const case_bit: SqlLexVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: SqlLexVec16 = a[i .. i + 16][0..16].*;
        const vb: SqlLexVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) {
            return false;
        }
    }
    while (i < a.len) {
        if (SQL_LOWER_LUT[usize(a[i])] != SQL_LOWER_LUT[usize(b[i])]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isSqlKeyword(word: string): bool {
    if (eqIgnoreCase(word, "SELECT")) { return true; }
    if (eqIgnoreCase(word, "DISTINCT")) { return true; }
    if (eqIgnoreCase(word, "FROM")) { return true; }
    if (eqIgnoreCase(word, "WHERE")) { return true; }
    if (eqIgnoreCase(word, "AND")) { return true; }
    if (eqIgnoreCase(word, "OR")) { return true; }
    if (eqIgnoreCase(word, "NOT")) { return true; }
    if (eqIgnoreCase(word, "IN")) { return true; }
    if (eqIgnoreCase(word, "IS")) { return true; }
    if (eqIgnoreCase(word, "NULL")) { return true; }
    if (eqIgnoreCase(word, "LIKE")) { return true; }
    if (eqIgnoreCase(word, "AS")) { return true; }
    if (eqIgnoreCase(word, "JOIN")) { return true; }
    if (eqIgnoreCase(word, "INNER")) { return true; }
    if (eqIgnoreCase(word, "LEFT")) { return true; }
    if (eqIgnoreCase(word, "RIGHT")) { return true; }
    if (eqIgnoreCase(word, "FULL")) { return true; }
    if (eqIgnoreCase(word, "OUTER")) { return true; }
    if (eqIgnoreCase(word, "CROSS")) { return true; }
    if (eqIgnoreCase(word, "ON")) { return true; }
    if (eqIgnoreCase(word, "GROUP")) { return true; }
    if (eqIgnoreCase(word, "BY")) { return true; }
    if (eqIgnoreCase(word, "ORDER")) { return true; }
    if (eqIgnoreCase(word, "ASC")) { return true; }
    if (eqIgnoreCase(word, "DESC")) { return true; }
    if (eqIgnoreCase(word, "LIMIT")) { return true; }
    if (eqIgnoreCase(word, "OFFSET")) { return true; }
    if (eqIgnoreCase(word, "INSERT")) { return true; }
    if (eqIgnoreCase(word, "INTO")) { return true; }
    if (eqIgnoreCase(word, "VALUES")) { return true; }
    if (eqIgnoreCase(word, "UPDATE")) { return true; }
    if (eqIgnoreCase(word, "SET")) { return true; }
    if (eqIgnoreCase(word, "DELETE")) { return true; }
    if (eqIgnoreCase(word, "CREATE")) { return true; }
    if (eqIgnoreCase(word, "TABLE")) { return true; }
    if (eqIgnoreCase(word, "IF")) { return true; }
    if (eqIgnoreCase(word, "EXISTS")) { return true; }
    if (eqIgnoreCase(word, "PRIMARY")) { return true; }
    if (eqIgnoreCase(word, "KEY")) { return true; }
    if (eqIgnoreCase(word, "TRUE")) { return true; }
    if (eqIgnoreCase(word, "FALSE")) { return true; }
    return false;
}

pub fn tokenizeSql(alloc: std.mem.Allocator, sql: string): ![]SqlToken {
    let list = std.ArrayList(SqlToken).empty;
    errdefer list.deinit(alloc);

    let i: usize = 0;
    let line: usize = 1;
    let col: usize = 1;

    while (i < sql.len) {
        const c = sql[i];

        if (isWhitespace(c)) {
            if (c == '\n') {
                line += 1;
                col = 1;
            } else {
                col += 1;
            }
            i += 1;
            continue;
        }

        // Single-line comment: --
        if (c == '-' && i + 1 < sql.len && sql[i + 1] == '-') {
            i += 2;
            col += 2;
            const nl = findByteSimd(sql, i, '\n');
            col += nl - i;
            i = nl;
            continue;
        }

        // Block comment: /* ... */
        if (c == '/' && i + 1 < sql.len && sql[i + 1] == '*') {
            i += 2;
            col += 2;
            let closed = false;
            while (i + 1 < sql.len) {
                const hit = findEitherByteSimd(sql, i, '*', '\n');
                if (hit > i) {
                    const advance = if (hit < sql.len) hit - i else (sql.len - 1) - i;
                    col += advance;
                    i += advance;
                }
                if (i + 1 >= sql.len) break;
                if (sql[i] == '*' && sql[i + 1] == '/') {
                    i += 2;
                    col += 2;
                    closed = true;
                    break;
                }
                if (sql[i] == '\n') {
                    line += 1;
                    col = 1;
                } else {
                    col += 1;
                }
                i += 1;
            }
            if (!closed) {
                return SqlError.UnterminatedComment;
            }
            continue;
        }

        const tok_line = line;
        const tok_col = col;

        // Single-quoted string literal with '' escape
        if (c == '\'') {
            const start = i;
            i += 1;
            col += 1;
            let closed = false;
            while (i < sql.len) {
                const hit = findEitherByteSimd(sql, i, '\'', '\n');
                if (hit > i) {
                    col += hit - i;
                    i = hit;
                }
                if (i >= sql.len) break;
                if (sql[i] == '\'') {
                    if (i + 1 < sql.len && sql[i + 1] == '\'') {
                        i += 2;
                        col += 2;
                        continue;
                    }
                    i += 1;
                    col += 1;
                    closed = true;
                    break;
                }
                if (sql[i] == '\n') {
                    line += 1;
                    col = 1;
                } else {
                    col += 1;
                }
                i += 1;
            }
            if (!closed) {
                return SqlError.UnterminatedString;
            }
            list.append(alloc, SqlToken{
                .kind = SqlTokenKind.StringLiteral,
                .lexeme = sql[start..i],
                .line = tok_line,
                .col = tok_col,
            }) catch return SqlError.OutOfMemory;
            continue;
        }

        // Double-quoted or backtick-quoted identifier
        if (c == '"' || c == '`') {
            const quote = c;
            const start = i;
            i += 1;
            col += 1;
            let closed = false;
            while (i < sql.len) {
                const hit = findEitherByteSimd(sql, i, quote, '\n');
                if (hit > i) {
                    col += hit - i;
                    i = hit;
                }
                if (i >= sql.len) break;
                if (sql[i] == quote) {
                    if (i + 1 < sql.len && sql[i + 1] == quote) {
                        i += 2;
                        col += 2;
                        continue;
                    }
                    i += 1;
                    col += 1;
                    closed = true;
                    break;
                }
                if (sql[i] == '\n') {
                    line += 1;
                    col = 1;
                } else {
                    col += 1;
                }
                i += 1;
            }
            if (!closed) {
                return SqlError.UnterminatedString;
            }
            list.append(alloc, SqlToken{
                .kind = SqlTokenKind.QuotedIdentifier,
                .lexeme = sql[start..i],
                .line = tok_line,
                .col = tok_col,
            }) catch return SqlError.OutOfMemory;
            continue;
        }

        // Positional placeholders: ? or $1
        if (c == '?') {
            list.append(alloc, SqlToken{
                .kind = SqlTokenKind.Placeholder,
                .lexeme = sql[i..i + 1],
                .line = tok_line,
                .col = tok_col,
            }) catch return SqlError.OutOfMemory;
            i += 1;
            col += 1;
            continue;
        }

        if (c == '$') {
            const start = i;
            i += 1;
            col += 1;
            if (i >= sql.len || !isDigit(sql[i])) {
                return SqlError.InvalidCharacter;
            }
            while (i < sql.len && isDigit(sql[i])) {
                i += 1;
                col += 1;
            }
            list.append(alloc, SqlToken{
                .kind = SqlTokenKind.Placeholder,
                .lexeme = sql[start..i],
                .line = tok_line,
                .col = tok_col,
            }) catch return SqlError.OutOfMemory;
            continue;
        }

        // Number literal (integer or decimal)
        if (isDigit(c)) {
            const start = i;
            while (i < sql.len && isDigit(sql[i])) {
                i += 1;
                col += 1;
            }
            if (i < sql.len && sql[i] == '.') {
                if (i + 1 < sql.len && isDigit(sql[i + 1])) {
                    i += 1;
                    col += 1;
                    while (i < sql.len && isDigit(sql[i])) {
                        i += 1;
                        col += 1;
                    }
                }
            }
            list.append(alloc, SqlToken{
                .kind = SqlTokenKind.NumberLiteral,
                .lexeme = sql[start..i],
                .line = tok_line,
                .col = tok_col,
            }) catch return SqlError.OutOfMemory;
            continue;
        }

        // Identifier or Keyword
        if (isAlpha(c)) {
            const start = i;
            const end_idx = scanIdentEndSimd(sql, i);
            col += end_idx - i;
            i = end_idx;
            const word = sql[start..i];
            const kind = if (isSqlKeyword(word)) SqlTokenKind.Keyword else SqlTokenKind.Identifier;
            list.append(alloc, SqlToken{
                .kind = kind,
                .lexeme = word,
                .line = tok_line,
                .col = tok_col,
            }) catch return SqlError.OutOfMemory;
            continue;
        }

        // Two-character operators: <=, >=, <>, !=, ||, ::
        if (i + 1 < sql.len) {
            const c2 = sql[i + 1];
            if ((c == '<' && (c2 == '=' || c2 == '>')) ||
                (c == '>' && c2 == '=') ||
                (c == '!' && c2 == '=') ||
                (c == '|' && c2 == '|') ||
                (c == ':' && c2 == ':')) {
                list.append(alloc, SqlToken{
                    .kind = SqlTokenKind.Operator,
                    .lexeme = sql[i..i + 2],
                    .line = tok_line,
                    .col = tok_col,
                }) catch return SqlError.OutOfMemory;
                i += 2;
                col += 2;
                continue;
            }
        }

        // Single-character punctuation & operators
        if (c == ',') {
            list.append(alloc, SqlToken{ .kind = SqlTokenKind.Comma, .lexeme = sql[i..i + 1], .line = tok_line, .col = tok_col }) catch return SqlError.OutOfMemory;
            i += 1;
            col += 1;
            continue;
        }
        if (c == ';') {
            list.append(alloc, SqlToken{ .kind = SqlTokenKind.Semicolon, .lexeme = sql[i..i + 1], .line = tok_line, .col = tok_col }) catch return SqlError.OutOfMemory;
            i += 1;
            col += 1;
            continue;
        }
        if (c == '(') {
            list.append(alloc, SqlToken{ .kind = SqlTokenKind.LParen, .lexeme = sql[i..i + 1], .line = tok_line, .col = tok_col }) catch return SqlError.OutOfMemory;
            i += 1;
            col += 1;
            continue;
        }
        if (c == ')') {
            list.append(alloc, SqlToken{ .kind = SqlTokenKind.RParen, .lexeme = sql[i..i + 1], .line = tok_line, .col = tok_col }) catch return SqlError.OutOfMemory;
            i += 1;
            col += 1;
            continue;
        }
        if (c == '.') {
            list.append(alloc, SqlToken{ .kind = SqlTokenKind.Dot, .lexeme = sql[i..i + 1], .line = tok_line, .col = tok_col }) catch return SqlError.OutOfMemory;
            i += 1;
            col += 1;
            continue;
        }
        if (c == '*') {
            list.append(alloc, SqlToken{ .kind = SqlTokenKind.Star, .lexeme = sql[i..i + 1], .line = tok_line, .col = tok_col }) catch return SqlError.OutOfMemory;
            i += 1;
            col += 1;
            continue;
        }
        if (c == '=' || c == '<' || c == '>' || c == '+' || c == '-' || c == '/' || c == '%') {
            list.append(alloc, SqlToken{ .kind = SqlTokenKind.Operator, .lexeme = sql[i..i + 1], .line = tok_line, .col = tok_col }) catch return SqlError.OutOfMemory;
            i += 1;
            col += 1;
            continue;
        }

        return SqlError.InvalidCharacter;
    }

    list.append(alloc, SqlToken{
        .kind = SqlTokenKind.Eof,
        .lexeme = "",
        .line = line,
        .col = col,
    }) catch return SqlError.OutOfMemory;

    return list.toOwnedSlice(alloc) catch return SqlError.OutOfMemory;
}
