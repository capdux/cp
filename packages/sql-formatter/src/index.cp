// @cap/sqlformat — Zero-allocation SQL formatter & minifier for CP
// Clean-room MIT implementation inspired by npm `sql-formatter` and Rust `sqlformat`.
//
// Features:
// - Lexer-aware protection of single-quoted strings ('...' with '' escapes),
//   double-quoted/backtick identifiers ("..." / `...`), line comments (-- ...),
//   and block comments (/* ... */).
// - Automatic SQL keyword uppercasing & multi-word clause recognition.
// - Clause-aware line breaks, indentation, parenthesis depth tracking, and multi-statement separation.
// - Accelerated with 16-lane `@Vector(16, u8)` lexer scanning & keyword uppercasing,
//   comptime 256-entry character-class / uppercase LUTs, and bulk `@memcpy` / `@memset`.

import { eqIgnoreCase } from "cap:ascii";

type SqlVec16 = @Vector(16, u8);
type SqlBool16 = @Vector(16, bool);

const SQL_CLS_WS: u8 = 1;
const SQL_CLS_IDENT_START: u8 = 2;
const SQL_CLS_WORD_CONT: u8 = 4;
const SQL_CLS_DIGIT: u8 = 8;

fn buildSqlCharLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 11 || c == 12) {
            f |= SQL_CLS_WS;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
            f |= SQL_CLS_IDENT_START | SQL_CLS_WORD_CONT;
        }
        if (c >= '0' && c <= '9') {
            f |= SQL_CLS_DIGIT | SQL_CLS_WORD_CONT;
        }
        if (c == '$') {
            f |= SQL_CLS_WORD_CONT;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

fn buildSqlUpperLut() -> [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = undefined;
    let c: usize = 0;
    while (c < 256) {
        if (c >= 'a' && c <= 'z') {
            lut[c] = (c - 32) as u8;
        } else {
            lut[c] = c as u8;
        }
        c += 1;
    }
    return lut;
}

const SQL_CHAR_LUT: [256]u8 = buildSqlCharLut();
const SQL_UPPER_LUT: [256]u8 = buildSqlUpperLut();

const TOK_EOF: u8 = 0;
const TOK_WORD: u8 = 1;
const TOK_NUMBER: u8 = 2;
const TOK_STRING: u8 = 3;
const TOK_QUOTED_IDENT: u8 = 4;
const TOK_LINE_COMMENT: u8 = 5;
const TOK_BLOCK_COMMENT: u8 = 6;
const TOK_COMMA: u8 = 7;
const TOK_SEMICOLON: u8 = 8;
const TOK_LPAREN: u8 = 9;
const TOK_RPAREN: u8 = 10;
const TOK_DOT: u8 = 11;
const TOK_OPERATOR: u8 = 12;

struct Token {
    kind: u8,
    start: usize,
    end: usize,
}

pub struct SqlFormatOptions {
    pub indent_width: u8,
    pub uppercase_keywords: bool,
    pub lines_between_queries: u8,

    pub static fn defaults() -> SqlFormatOptions {
        return SqlFormatOptions{
            indent_width: 2,
            uppercase_keywords: true,
            lines_between_queries: 1,
        };
    }
}

fn sqlEqIgnoreCase(sql: string, start: usize, end: usize, upper: string) -> bool {
    return eqIgnoreCase(sql[start..end], upper);
}

fn nextToken(sql: string, start_pos: usize) -> Token {
    let i = start_pos;
    // 16-lane SIMD whitespace skip
    const v_sp: SqlVec16 = @splat(' ' as u8);
    const v_tab: SqlVec16 = @splat('\t' as u8);
    const v_nl: SqlVec16 = @splat('\n' as u8);
    const v_cr: SqlVec16 = @splat('\r' as u8);
    while (i + 16 <= sql.len) {
        const v: SqlVec16 = sql[i .. i + 16][0..16].*;
        if (!@reduce(.And, (v == v_sp) | (v == v_tab) | (v == v_nl) | (v == v_cr))) {
            break;
        }
        i += 16;
    }
    while (i < sql.len && (SQL_CHAR_LUT[sql[i] as usize] & SQL_CLS_WS) != 0) {
        i += 1;
    }
    if (i >= sql.len) {
        return Token{ kind: TOK_EOF, start: i, end: i };
    }

    const c = sql[i];

    // Line comment: -- ...
    if (c == '-' && i + 1 < sql.len && sql[i + 1] == '-') {
        const s = i;
        i += 2;
        while (i + 16 <= sql.len) {
            const v: SqlVec16 = sql[i .. i + 16][0..16].*;
            if (@reduce(.Or, (v == v_nl) | (v == v_cr))) {
                break;
            }
            i += 16;
        }
        while (i < sql.len && sql[i] != '\n' && sql[i] != '\r') {
            i += 1;
        }
        return Token{ kind: TOK_LINE_COMMENT, start: s, end: i };
    }

    // Block comment: /* ... */
    if (c == '/' && i + 1 < sql.len && sql[i + 1] == '*') {
        const s = i;
        const v_star: SqlVec16 = @splat('*' as u8);
        i += 2;
        while (i + 1 < sql.len) {
            while (i + 17 <= sql.len) {
                const v: SqlVec16 = sql[i .. i + 16][0..16].*;
                if (@reduce(.Or, v == v_star)) {
                    break;
                }
                i += 16;
            }
            if (sql[i] == '*' && sql[i + 1] == '/') {
                i += 2;
                return Token{ kind: TOK_BLOCK_COMMENT, start: s, end: i };
            }
            i += 1;
        }
        return Token{ kind: TOK_BLOCK_COMMENT, start: s, end: sql.len };
    }

    // Single-quoted string literal: '...' (with '' or \' escape)
    if (c == '\'') {
        const s = i;
        const v_sq: SqlVec16 = @splat('\'' as u8);
        const v_bs: SqlVec16 = @splat('\\' as u8);
        i += 1;
        while (i < sql.len) {
            while (i + 16 <= sql.len) {
                const v: SqlVec16 = sql[i .. i + 16][0..16].*;
                if (@reduce(.Or, (v == v_sq) | (v == v_bs))) {
                    break;
                }
                i += 16;
            }
            if (i >= sql.len) {
                break;
            }
            if (sql[i] == '\'') {
                if (i + 1 < sql.len && sql[i + 1] == '\'') {
                    i += 2;
                    continue;
                }
                i += 1;
                break;
            }
            if (sql[i] == '\\' && i + 1 < sql.len) {
                i += 2;
                continue;
            }
            i += 1;
        }
        return Token{ kind: TOK_STRING, start: s, end: i };
    }

    // Double-quoted or backtick identifier: "..." / `...`
    if (c == '"' || c == '`') {
        const quote = c;
        const v_q: SqlVec16 = @splat(quote);
        const s = i;
        i += 1;
        while (i < sql.len) {
            while (i + 16 <= sql.len) {
                const v: SqlVec16 = sql[i .. i + 16][0..16].*;
                if (@reduce(.Or, v == v_q)) {
                    break;
                }
                i += 16;
            }
            if (i >= sql.len) {
                break;
            }
            if (sql[i] == quote) {
                if (i + 1 < sql.len && sql[i + 1] == quote) {
                    i += 2;
                    continue;
                }
                i += 1;
                break;
            }
            i += 1;
        }
        return Token{ kind: TOK_QUOTED_IDENT, start: s, end: i };
    }

    const cls = SQL_CHAR_LUT[c as usize];

    // Word (keyword or identifier)
    if ((cls & SQL_CLS_IDENT_START) != 0) {
        const s = i;
        i += 1;
        while (i < sql.len && (SQL_CHAR_LUT[sql[i] as usize] & SQL_CLS_WORD_CONT) != 0) {
            i += 1;
        }
        return Token{ kind: TOK_WORD, start: s, end: i };
    }

    // Number literal
    if ((cls & SQL_CLS_DIGIT) != 0) {
        const s = i;
        i += 1;
        while (i < sql.len && ((SQL_CHAR_LUT[sql[i] as usize] & SQL_CLS_DIGIT) != 0 || sql[i] == '.')) {
            i += 1;
        }
        return Token{ kind: TOK_NUMBER, start: s, end: i };
    }

    // Single-char delimiters
    if (c == ',') {
        return Token{ kind: TOK_COMMA, start: i, end: i + 1 };
    }
    if (c == ';') {
        return Token{ kind: TOK_SEMICOLON, start: i, end: i + 1 };
    }
    if (c == '(') {
        return Token{ kind: TOK_LPAREN, start: i, end: i + 1 };
    }
    if (c == ')') {
        return Token{ kind: TOK_RPAREN, start: i, end: i + 1 };
    }
    if (c == '.') {
        return Token{ kind: TOK_DOT, start: i, end: i + 1 };
    }

    // Multi-char operators: !=, <>, <=, >=, ||, ::, ->>, ->
    if (i + 1 < sql.len) {
        const n = sql[i + 1];
        if ((c == '!' && n == '=') ||
            (c == '<' && (n == '>' || n == '=')) ||
            (c == '>' && n == '=') ||
            (c == '|' && n == '|') ||
            (c == ':' && n == ':')) {
            return Token{ kind: TOK_OPERATOR, start: i, end: i + 2 };
        }
        if (c == '-' && n == '>') {
            if (i + 2 < sql.len && sql[i + 2] == '>') {
                return Token{ kind: TOK_OPERATOR, start: i, end: i + 3 };
            }
            return Token{ kind: TOK_OPERATOR, start: i, end: i + 2 };
        }
    }

    return Token{ kind: TOK_OPERATOR, start: i, end: i + 1 };
}

fn isSqlFunctionKeyword(sql: string, s: usize, e: usize) -> bool {
    const n = e - s;
    if (n < 3 || n > 8) {
        return false;
    }
    return sqlEqIgnoreCase(sql, s, e, "COUNT") ||
        sqlEqIgnoreCase(sql, s, e, "SUM") ||
        sqlEqIgnoreCase(sql, s, e, "AVG") ||
        sqlEqIgnoreCase(sql, s, e, "MIN") ||
        sqlEqIgnoreCase(sql, s, e, "MAX") ||
        sqlEqIgnoreCase(sql, s, e, "COALESCE") ||
        sqlEqIgnoreCase(sql, s, e, "CAST") ||
        sqlEqIgnoreCase(sql, s, e, "NULLIF") ||
        sqlEqIgnoreCase(sql, s, e, "LOWER") ||
        sqlEqIgnoreCase(sql, s, e, "UPPER") ||
        sqlEqIgnoreCase(sql, s, e, "LENGTH") ||
        sqlEqIgnoreCase(sql, s, e, "CONCAT");
}

fn isSqlKeyword(sql: string, s: usize, e: usize) -> bool {
    const n = e - s;
    if (n < 2 || n > 10) {
        return false;
    }
    if (isSqlFunctionKeyword(sql, s, e)) {
        return true;
    }
    return sqlEqIgnoreCase(sql, s, e, "SELECT") ||
        sqlEqIgnoreCase(sql, s, e, "FROM") ||
        sqlEqIgnoreCase(sql, s, e, "WHERE") ||
        sqlEqIgnoreCase(sql, s, e, "AND") ||
        sqlEqIgnoreCase(sql, s, e, "OR") ||
        sqlEqIgnoreCase(sql, s, e, "INNER") ||
        sqlEqIgnoreCase(sql, s, e, "LEFT") ||
        sqlEqIgnoreCase(sql, s, e, "RIGHT") ||
        sqlEqIgnoreCase(sql, s, e, "FULL") ||
        sqlEqIgnoreCase(sql, s, e, "OUTER") ||
        sqlEqIgnoreCase(sql, s, e, "CROSS") ||
        sqlEqIgnoreCase(sql, s, e, "JOIN") ||
        sqlEqIgnoreCase(sql, s, e, "GROUP") ||
        sqlEqIgnoreCase(sql, s, e, "ORDER") ||
        sqlEqIgnoreCase(sql, s, e, "BY") ||
        sqlEqIgnoreCase(sql, s, e, "HAVING") ||
        sqlEqIgnoreCase(sql, s, e, "LIMIT") ||
        sqlEqIgnoreCase(sql, s, e, "OFFSET") ||
        sqlEqIgnoreCase(sql, s, e, "INSERT") ||
        sqlEqIgnoreCase(sql, s, e, "INTO") ||
        sqlEqIgnoreCase(sql, s, e, "VALUES") ||
        sqlEqIgnoreCase(sql, s, e, "UPDATE") ||
        sqlEqIgnoreCase(sql, s, e, "SET") ||
        sqlEqIgnoreCase(sql, s, e, "DELETE") ||
        sqlEqIgnoreCase(sql, s, e, "CREATE") ||
        sqlEqIgnoreCase(sql, s, e, "TABLE") ||
        sqlEqIgnoreCase(sql, s, e, "DROP") ||
        sqlEqIgnoreCase(sql, s, e, "ALTER") ||
        sqlEqIgnoreCase(sql, s, e, "INDEX") ||
        sqlEqIgnoreCase(sql, s, e, "VIEW") ||
        sqlEqIgnoreCase(sql, s, e, "AS") ||
        sqlEqIgnoreCase(sql, s, e, "ON") ||
        sqlEqIgnoreCase(sql, s, e, "IN") ||
        sqlEqIgnoreCase(sql, s, e, "NOT") ||
        sqlEqIgnoreCase(sql, s, e, "NULL") ||
        sqlEqIgnoreCase(sql, s, e, "IS") ||
        sqlEqIgnoreCase(sql, s, e, "DISTINCT") ||
        sqlEqIgnoreCase(sql, s, e, "UNION") ||
        sqlEqIgnoreCase(sql, s, e, "ALL") ||
        sqlEqIgnoreCase(sql, s, e, "CASE") ||
        sqlEqIgnoreCase(sql, s, e, "WHEN") ||
        sqlEqIgnoreCase(sql, s, e, "THEN") ||
        sqlEqIgnoreCase(sql, s, e, "ELSE") ||
        sqlEqIgnoreCase(sql, s, e, "END") ||
        sqlEqIgnoreCase(sql, s, e, "ASC") ||
        sqlEqIgnoreCase(sql, s, e, "DESC") ||
        sqlEqIgnoreCase(sql, s, e, "BETWEEN") ||
        sqlEqIgnoreCase(sql, s, e, "LIKE") ||
        sqlEqIgnoreCase(sql, s, e, "ILIKE") ||
        sqlEqIgnoreCase(sql, s, e, "EXISTS") ||
        sqlEqIgnoreCase(sql, s, e, "PRIMARY") ||
        sqlEqIgnoreCase(sql, s, e, "KEY") ||
        sqlEqIgnoreCase(sql, s, e, "FOREIGN") ||
        sqlEqIgnoreCase(sql, s, e, "REFERENCES") ||
        sqlEqIgnoreCase(sql, s, e, "DEFAULT") ||
        sqlEqIgnoreCase(sql, s, e, "UNIQUE") ||
        sqlEqIgnoreCase(sql, s, e, "CHECK") ||
        sqlEqIgnoreCase(sql, s, e, "CONSTRAINT") ||
        sqlEqIgnoreCase(sql, s, e, "IF") ||
        sqlEqIgnoreCase(sql, s, e, "RETURNING") ||
        sqlEqIgnoreCase(sql, s, e, "WITH") ||
        sqlEqIgnoreCase(sql, s, e, "USING") ||
        sqlEqIgnoreCase(sql, s, e, "TRUE") ||
        sqlEqIgnoreCase(sql, s, e, "FALSE");
}

fn pushByte(out: []u8, pos: usize, b: u8) -> usize {
    if (pos < out.len) {
        out[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn pushSlice(out: []u8, pos: usize, src: string, start: usize, end: usize, upper: bool) -> usize {
    if (pos >= out.len || start >= end) {
        return pos;
    }
    const avail = out.len - pos;
    const raw_len = end - start;
    const n = if (raw_len < avail) { raw_len } else { avail };
    if (!upper) {
        @memcpy(out[pos .. pos + n], src[start .. start + n]);
        return pos + n;
    }
    let k: usize = 0;
    const v_a: SqlVec16 = @splat('a' as u8);
    const v_z: SqlVec16 = @splat('z' as u8);
    const v_mask: SqlVec16 = @splat(0xDF as u8);
    const v_false: SqlBool16 = @splat(false);
    while (k + 16 <= n) {
        const v: SqlVec16 = src[start + k .. start + k + 16][0..16].*;
        const in_range = @select(bool, v >= v_a, v <= v_z, v_false);
        const up: [16]u8 = @select(u8, in_range, v & v_mask, v);
        @memcpy(out[pos + k .. pos + k + 16], up[0..16]);
        k += 16;
    }
    while (k < n) {
        out[pos + k] = SQL_UPPER_LUT[src[start + k] as usize];
        k += 1;
    }
    return pos + n;
}

fn pushIndent(out: []u8, pos: usize, level: usize, indent_width: u8) -> usize {
    if (pos >= out.len) {
        return pos;
    }
    const spaces = level * (indent_width as usize);
    const avail = out.len - pos;
    const n = if (spaces < avail) { spaces } else { avail };
    if (n > 0) {
        @memset(out[pos .. pos + n], ' ' as u8);
    }
    return pos + n;
}

fn pushNewlineIndent(out: []u8, pos: usize, level: usize, indent_width: u8) -> usize {
    let p = pushByte(out, pos, '\n');
    p = pushIndent(out, p, level, indent_width);
    return p;
}

fn isJoinLeadWord(sql: string, s: usize, e: usize) -> bool {
    const n = e - s;
    if (n < 4 || n > 5) {
        return false;
    }
    return sqlEqIgnoreCase(sql, s, e, "INNER") ||
        sqlEqIgnoreCase(sql, s, e, "LEFT") ||
        sqlEqIgnoreCase(sql, s, e, "RIGHT") ||
        sqlEqIgnoreCase(sql, s, e, "FULL") ||
        sqlEqIgnoreCase(sql, s, e, "CROSS");
}

fn isTopClauseWord(sql: string, s: usize, e: usize) -> bool {
    const n = e - s;
    if (n < 3 || n > 9) {
        return false;
    }
    return sqlEqIgnoreCase(sql, s, e, "FROM") ||
        sqlEqIgnoreCase(sql, s, e, "WHERE") ||
        sqlEqIgnoreCase(sql, s, e, "JOIN") ||
        sqlEqIgnoreCase(sql, s, e, "HAVING") ||
        sqlEqIgnoreCase(sql, s, e, "LIMIT") ||
        sqlEqIgnoreCase(sql, s, e, "OFFSET") ||
        sqlEqIgnoreCase(sql, s, e, "SET") ||
        sqlEqIgnoreCase(sql, s, e, "VALUES") ||
        sqlEqIgnoreCase(sql, s, e, "UNION") ||
        sqlEqIgnoreCase(sql, s, e, "RETURNING");
}

/// Formats SQL using `SqlFormatOptions.defaults()` into caller-provided `out`.
pub fn formatSql(sql: string, out: []u8) -> string {
    return formatSqlWith(sql, SqlFormatOptions.defaults(), out);
}

/// Formats SQL with custom options into caller-provided `out` (zero heap allocation).
pub fn formatSqlWith(sql: string, opts: SqlFormatOptions, out: []u8) -> string {
    let out_pos: usize = 0;
    let cursor: usize = 0;

    let paren_depth: usize = 0;
    let list_indent_active = false;
    let list_paren_depth: usize = 0;
    let in_between = false;
    let stmt_started = false;
    let at_line_start = true;
    let prev_kind: u8 = TOK_EOF;
    let prev_end: usize = 0;
    let prev_was_non_func_kw = false;
    let prev_was_func_kw = false;

    while (true) {
        const tok = nextToken(sql, cursor);
        if (tok.kind == TOK_EOF) {
            break;
        }
        cursor = tok.end;

        // Line comment: preserve verbatim and end line
        if (tok.kind == TOK_LINE_COMMENT) {
            if (!at_line_start) {
                out_pos = pushByte(out, out_pos, ' ');
            } else if (stmt_started) {
                out_pos = pushIndent(out, out_pos, paren_depth, opts.indent_width);
            }
            out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, false);
            const peek = nextToken(sql, cursor);
            if (peek.kind != TOK_EOF) {
                out_pos = pushByte(out, out_pos, '\n');
            }
            at_line_start = true;
            prev_kind = TOK_LINE_COMMENT;
            prev_was_non_func_kw = false;
            continue;
        }

        // Block comment: preserve verbatim
        if (tok.kind == TOK_BLOCK_COMMENT) {
            if (!at_line_start) {
                out_pos = pushByte(out, out_pos, ' ');
            }
            out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, false);
            at_line_start = false;
            prev_kind = TOK_BLOCK_COMMENT;
            prev_was_non_func_kw = false;
            continue;
        }

        // Semicolon: terminate statement & insert blank lines before next statement
        if (tok.kind == TOK_SEMICOLON) {
            out_pos = pushByte(out, out_pos, ';');
            list_indent_active = false;
            in_between = false;
            stmt_started = false;
            prev_kind = TOK_SEMICOLON;
            prev_was_non_func_kw = false;

            const peek = nextToken(sql, cursor);
            if (peek.kind != TOK_EOF) {
                let nl: u8 = 0;
                const total_nl = opts.lines_between_queries + 1;
                while (nl < total_nl) {
                    out_pos = pushByte(out, out_pos, '\n');
                    nl += 1;
                }
                at_line_start = true;
            } else {
                at_line_start = false;
            }
            continue;
        }

        // Handle SQL keywords & multi-word clauses
        if (tok.kind == TOK_WORD && prev_kind != TOK_DOT) {
            // SELECT [DISTINCT] -> newline + indent for projection list
            if (sqlEqIgnoreCase(sql, tok.start, tok.end, "SELECT")) {
                if (stmt_started && !at_line_start) {
                    out_pos = pushNewlineIndent(out, out_pos, paren_depth, opts.indent_width);
                } else if (at_line_start && paren_depth > 0) {
                    out_pos = pushIndent(out, out_pos, paren_depth, opts.indent_width);
                }
                out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, opts.uppercase_keywords);
                stmt_started = true;

                // Keep DISTINCT on the same line as SELECT
                const peek = nextToken(sql, cursor);
                if (peek.kind == TOK_WORD && sqlEqIgnoreCase(sql, peek.start, peek.end, "DISTINCT")) {
                    cursor = peek.end;
                    out_pos = pushByte(out, out_pos, ' ');
                    out_pos = pushSlice(out, out_pos, sql, peek.start, peek.end, opts.uppercase_keywords);
                }

                list_indent_active = true;
                list_paren_depth = paren_depth;
                out_pos = pushNewlineIndent(out, out_pos, paren_depth + 1, opts.indent_width);
                at_line_start = true;
                prev_kind = TOK_WORD;
                prev_was_non_func_kw = true;
                continue;
            }

            // DELETE FROM stays on one line
            if (sqlEqIgnoreCase(sql, tok.start, tok.end, "DELETE")) {
                const peek = nextToken(sql, cursor);
                if (peek.kind == TOK_WORD && sqlEqIgnoreCase(sql, peek.start, peek.end, "FROM")) {
                    cursor = peek.end;
                    list_indent_active = false;
                    in_between = false;
                    if (stmt_started && !at_line_start) {
                        out_pos = pushNewlineIndent(out, out_pos, paren_depth, opts.indent_width);
                    } else if (at_line_start && paren_depth > 0) {
                        out_pos = pushIndent(out, out_pos, paren_depth, opts.indent_width);
                    }
                    out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, opts.uppercase_keywords);
                    out_pos = pushByte(out, out_pos, ' ');
                    out_pos = pushSlice(out, out_pos, sql, peek.start, peek.end, opts.uppercase_keywords);
                    stmt_started = true;
                    at_line_start = false;
                    prev_kind = TOK_WORD;
                    prev_end = peek.end;
                    prev_was_non_func_kw = true;
                    prev_was_func_kw = false;
                    continue;
                }
            }

            // GROUP BY / ORDER BY
            if (sqlEqIgnoreCase(sql, tok.start, tok.end, "GROUP") || sqlEqIgnoreCase(sql, tok.start, tok.end, "ORDER")) {
                const peek = nextToken(sql, cursor);
                if (peek.kind == TOK_WORD && sqlEqIgnoreCase(sql, peek.start, peek.end, "BY")) {
                    cursor = peek.end;
                    list_indent_active = false;
                    in_between = false;
                    if (stmt_started && !at_line_start) {
                        out_pos = pushNewlineIndent(out, out_pos, paren_depth, opts.indent_width);
                    } else if (at_line_start && paren_depth > 0) {
                        out_pos = pushIndent(out, out_pos, paren_depth, opts.indent_width);
                    }
                    out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, opts.uppercase_keywords);
                    out_pos = pushByte(out, out_pos, ' ');
                    out_pos = pushSlice(out, out_pos, sql, peek.start, peek.end, opts.uppercase_keywords);
                    stmt_started = true;
                    at_line_start = false;
                    prev_kind = TOK_WORD;
                    prev_was_non_func_kw = true;
                    continue;
                }
            }

            // INNER/LEFT/RIGHT/FULL/CROSS [OUTER] JOIN
            if (isJoinLeadWord(sql, tok.start, tok.end)) {
                const p1 = nextToken(sql, cursor);
                if (p1.kind == TOK_WORD && sqlEqIgnoreCase(sql, p1.start, p1.end, "JOIN")) {
                    cursor = p1.end;
                    list_indent_active = false;
                    in_between = false;
                    if (stmt_started && !at_line_start) {
                        out_pos = pushNewlineIndent(out, out_pos, paren_depth, opts.indent_width);
                    } else if (at_line_start && paren_depth > 0) {
                        out_pos = pushIndent(out, out_pos, paren_depth, opts.indent_width);
                    }
                    out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, opts.uppercase_keywords);
                    out_pos = pushByte(out, out_pos, ' ');
                    out_pos = pushSlice(out, out_pos, sql, p1.start, p1.end, opts.uppercase_keywords);
                    stmt_started = true;
                    at_line_start = false;
                    prev_kind = TOK_WORD;
                    prev_was_non_func_kw = true;
                    continue;
                }
                if (p1.kind == TOK_WORD && sqlEqIgnoreCase(sql, p1.start, p1.end, "OUTER")) {
                    const p2 = nextToken(sql, p1.end);
                    if (p2.kind == TOK_WORD && sqlEqIgnoreCase(sql, p2.start, p2.end, "JOIN")) {
                        cursor = p2.end;
                        list_indent_active = false;
                        in_between = false;
                        if (stmt_started && !at_line_start) {
                            out_pos = pushNewlineIndent(out, out_pos, paren_depth, opts.indent_width);
                        } else if (at_line_start && paren_depth > 0) {
                            out_pos = pushIndent(out, out_pos, paren_depth, opts.indent_width);
                        }
                        out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, opts.uppercase_keywords);
                        out_pos = pushByte(out, out_pos, ' ');
                        out_pos = pushSlice(out, out_pos, sql, p1.start, p1.end, opts.uppercase_keywords);
                        out_pos = pushByte(out, out_pos, ' ');
                        out_pos = pushSlice(out, out_pos, sql, p2.start, p2.end, opts.uppercase_keywords);
                        stmt_started = true;
                        at_line_start = false;
                        prev_kind = TOK_WORD;
                        prev_was_non_func_kw = true;
                        continue;
                    }
                }
            }

            // Top-level clauses: FROM, WHERE, JOIN, HAVING, LIMIT, OFFSET, SET, VALUES, UNION, RETURNING
            if (isTopClauseWord(sql, tok.start, tok.end)) {
                const is_set = sqlEqIgnoreCase(sql, tok.start, tok.end, "SET");
                list_indent_active = is_set;
                if (is_set) {
                    list_paren_depth = paren_depth;
                }
                in_between = false;

                if (stmt_started && !at_line_start) {
                    out_pos = pushNewlineIndent(out, out_pos, paren_depth, opts.indent_width);
                } else if (at_line_start && paren_depth > 0) {
                    out_pos = pushIndent(out, out_pos, paren_depth, opts.indent_width);
                }
                out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, opts.uppercase_keywords);

                // UNION ALL stays on one line
                if (sqlEqIgnoreCase(sql, tok.start, tok.end, "UNION")) {
                    const peek = nextToken(sql, cursor);
                    if (peek.kind == TOK_WORD && sqlEqIgnoreCase(sql, peek.start, peek.end, "ALL")) {
                        cursor = peek.end;
                        out_pos = pushByte(out, out_pos, ' ');
                        out_pos = pushSlice(out, out_pos, sql, peek.start, peek.end, opts.uppercase_keywords);
                    }
                }

                stmt_started = true;
                at_line_start = false;
                prev_kind = TOK_WORD;
                prev_was_non_func_kw = true;
                continue;
            }

            // Indented logical operators: AND / OR
            if (sqlEqIgnoreCase(sql, tok.start, tok.end, "AND") || sqlEqIgnoreCase(sql, tok.start, tok.end, "OR")) {
                if (in_between && sqlEqIgnoreCase(sql, tok.start, tok.end, "AND")) {
                    in_between = false;
                    if (!at_line_start) {
                        out_pos = pushByte(out, out_pos, ' ');
                    }
                    out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, opts.uppercase_keywords);
                } else {
                    if (stmt_started && !at_line_start) {
                        out_pos = pushNewlineIndent(out, out_pos, paren_depth + 1, opts.indent_width);
                    } else if (at_line_start) {
                        out_pos = pushIndent(out, out_pos, paren_depth + 1, opts.indent_width);
                    }
                    out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, opts.uppercase_keywords);
                }
                stmt_started = true;
                at_line_start = false;
                prev_kind = TOK_WORD;
                prev_was_non_func_kw = true;
                continue;
            }

            if (sqlEqIgnoreCase(sql, tok.start, tok.end, "BETWEEN")) {
                in_between = true;
            }
        }

        // Comma: break & indent in SELECT / SET lists at matching paren depth
        if (tok.kind == TOK_COMMA) {
            out_pos = pushByte(out, out_pos, ',');
            if (list_indent_active && paren_depth == list_paren_depth) {
                out_pos = pushNewlineIndent(out, out_pos, paren_depth + 1, opts.indent_width);
                at_line_start = true;
            } else {
                at_line_start = false;
            }
            prev_kind = TOK_COMMA;
            prev_was_non_func_kw = false;
            continue;
        }

        // Dot: no spaces around '.'
        if (tok.kind == TOK_DOT) {
            out_pos = pushByte(out, out_pos, '.');
            at_line_start = false;
            prev_kind = TOK_DOT;
            prev_was_non_func_kw = false;
            continue;
        }

        // Left parenthesis
        if (tok.kind == TOK_LPAREN) {
            if (!at_line_start && prev_kind != TOK_LPAREN && prev_kind != TOK_DOT) {
                const had_space_before = tok.start > prev_end;
                if (prev_kind != TOK_WORD || prev_was_non_func_kw || (had_space_before && !prev_was_func_kw)) {
                    out_pos = pushByte(out, out_pos, ' ');
                }
            }
            out_pos = pushByte(out, out_pos, '(');
            paren_depth += 1;
            stmt_started = true;
            at_line_start = false;
            prev_kind = TOK_LPAREN;
            prev_end = tok.end;
            prev_was_non_func_kw = false;
            prev_was_func_kw = false;
            continue;
        }

        // Right parenthesis
        if (tok.kind == TOK_RPAREN) {
            if (paren_depth > 0) {
                paren_depth -= 1;
            }
            out_pos = pushByte(out, out_pos, ')');
            at_line_start = false;
            prev_kind = TOK_RPAREN;
            prev_was_non_func_kw = false;
            continue;
        }

        // PostgreSQL cast operator '::' has no surrounding spaces
        if (tok.kind == TOK_OPERATOR && tok.end - tok.start == 2 && sql[tok.start] == ':' && sql[tok.start + 1] == ':') {
            out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, false);
            at_line_start = false;
            prev_kind = TOK_DOT; // suppress trailing space as well
            prev_was_non_func_kw = false;
            continue;
        }

        // General spacing before token
        if (!at_line_start && prev_kind != TOK_LPAREN && prev_kind != TOK_DOT) {
            out_pos = pushByte(out, out_pos, ' ');
        }

        // Emit token payload
        if (tok.kind == TOK_WORD) {
            const is_kw = prev_kind != TOK_DOT && isSqlKeyword(sql, tok.start, tok.end);
            const is_func = prev_kind != TOK_DOT && isSqlFunctionKeyword(sql, tok.start, tok.end);
            const upper = opts.uppercase_keywords && is_kw;
            out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, upper);
            prev_was_non_func_kw = is_kw && !is_func;
            prev_was_func_kw = is_func;
        } else {
            out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, false);
            prev_was_non_func_kw = false;
            prev_was_func_kw = false;
        }

        stmt_started = true;
        at_line_start = false;
        prev_kind = tok.kind;
        prev_end = tok.end;
    }

    return out[0..out_pos];
}

fn isTightDelimiter(kind: u8) -> bool {
    return kind == TOK_COMMA || kind == TOK_SEMICOLON || kind == TOK_LPAREN || kind == TOK_RPAREN || kind == TOK_DOT;
}

/// Minifies SQL into a single compact line, stripping comments and redundant whitespace
/// while preserving string literals and quoted identifiers verbatim (zero heap allocation).
pub fn minifySql(sql: string, out: []u8) -> string {
    let out_pos: usize = 0;
    let cursor: usize = 0;
    let prev_kind: u8 = TOK_EOF;

    while (true) {
        const tok = nextToken(sql, cursor);
        if (tok.kind == TOK_EOF) {
            break;
        }
        cursor = tok.end;

        // Strip comments completely
        if (tok.kind == TOK_LINE_COMMENT || tok.kind == TOK_BLOCK_COMMENT) {
            continue;
        }

        // Insert a single space when required to keep tokens unambiguously separated
        if (prev_kind != TOK_EOF) {
            const need_space =
                (!isTightDelimiter(prev_kind) && !isTightDelimiter(tok.kind)) ||
                (prev_kind == TOK_RPAREN && (tok.kind == TOK_WORD || tok.kind == TOK_NUMBER || tok.kind == TOK_STRING || tok.kind == TOK_QUOTED_IDENT)) ||
                (prev_kind == TOK_SEMICOLON && tok.kind != TOK_EOF);
            if (need_space) {
                out_pos = pushByte(out, out_pos, ' ');
            }
        }

        out_pos = pushSlice(out, out_pos, sql, tok.start, tok.end, false);
        prev_kind = tok.kind;
    }

    return out[0..out_pos];
}
