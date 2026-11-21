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

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAlphaNum(c: u8): bool {
    return isAlpha(c) || isDigit(c);
}

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

pub fn eqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') {
            ca = ca + 32;
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = cb + 32;
        }
        if (ca != cb) {
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
            while (i < sql.len && sql[i] != '\n') {
                i += 1;
                col += 1;
            }
            continue;
        }

        // Block comment: /* ... */
        if (c == '/' && i + 1 < sql.len && sql[i + 1] == '*') {
            i += 2;
            col += 2;
            let closed = false;
            while (i + 1 < sql.len) {
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
            while (i < sql.len && isAlphaNum(sql[i])) {
                i += 1;
                col += 1;
            }
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
