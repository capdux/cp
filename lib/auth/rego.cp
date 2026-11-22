// Origin: Pure-CP Stdlib (Zero-Alloc OPA Rego Policy Lexer & Scanner) | Created: 2026-11-22

import std from "std";

pub const REGO_MAX_DEPTH: usize = 64;

pub enum RegoTokenKind {
    ident,
    string,
    raw_string,
    number,
    bool_lit,
    null_lit,
    assign,
    lbrace,
    rbrace,
    lbracket,
    rbracket,
    lparen,
    rparen,
    colon,
    comma,
    dot,
    semicolon,
    punct,
    invalid,
}

pub struct RegoToken {
    kind: RegoTokenKind,
    lexeme: []const u8,
}

struct RegoSpanToken {
    kind: RegoTokenKind,
    start: usize,
    end: usize,
    had_newline: bool,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentCont(c: u8): bool {
    return isIdentStart(c) || isDigit(c);
}

fn strEql(a: []const u8, b: []const u8): bool {
    return std.mem.eql(u8, a, b);
}

pub struct RegoIter {
    input: []const u8,
    pos: usize,

    fn init(input: []const u8): RegoIter {
        return RegoIter{
            .input = input,
            .pos = 0,
        };
    }

    fn nextSpan(*_): RegoSpanToken? {
        const s = _.input;
        const n = s.len;
        let had_newline = false;

        // Skip whitespace and '#' line comments
        while (_.pos < n) {
            const c = s[_.pos];
            if (isSpace(c)) {
                if (c == '\n') {
                    had_newline = true;
                }
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                _.pos += 1;
                while (_.pos < n && s[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            break;
        }

        if (_.pos >= n) {
            return null;
        }

        const start = _.pos;
        const c = s[start];

        // Double-quoted escaped string
        if (c == '"') {
            _.pos += 1;
            while (_.pos < n) {
                const ch = s[_.pos];
                if (ch == '\\') {
                    if (_.pos + 1 < n) {
                        _.pos += 2;
                        continue;
                    }
                    _.pos = n;
                    return RegoSpanToken{
                        .kind = RegoTokenKind.invalid,
                        .start = start,
                        .end = n,
                        .had_newline = had_newline,
                    };
                }
                if (ch == '"') {
                    _.pos += 1;
                    return RegoSpanToken{
                        .kind = RegoTokenKind.string,
                        .start = start,
                        .end = _.pos,
                        .had_newline = had_newline,
                    };
                }
                if (ch == '\n' || ch == '\r') {
                    // Rego double-quoted strings cannot span raw newlines
                    return RegoSpanToken{
                        .kind = RegoTokenKind.invalid,
                        .start = start,
                        .end = _.pos,
                        .had_newline = had_newline,
                    };
                }
                _.pos += 1;
            }
            return RegoSpanToken{
                .kind = RegoTokenKind.invalid,
                .start = start,
                .end = n,
                .had_newline = had_newline,
            };
        }

        // Backtick raw string (no escape sequences, may span multiple lines)
        if (c == '`') {
            _.pos += 1;
            while (_.pos < n) {
                if (s[_.pos] == '`') {
                    _.pos += 1;
                    return RegoSpanToken{
                        .kind = RegoTokenKind.raw_string,
                        .start = start,
                        .end = _.pos,
                        .had_newline = had_newline,
                    };
                }
                _.pos += 1;
            }
            return RegoSpanToken{
                .kind = RegoTokenKind.invalid,
                .start = start,
                .end = n,
                .had_newline = had_newline,
            };
        }

        // ':=' assignment vs ':' colon
        if (c == ':') {
            if (start + 1 < n && s[start + 1] == '=') {
                _.pos += 2;
                return RegoSpanToken{
                    .kind = RegoTokenKind.assign,
                    .start = start,
                    .end = _.pos,
                    .had_newline = had_newline,
                };
            }
            _.pos += 1;
            return RegoSpanToken{
                .kind = RegoTokenKind.colon,
                .start = start,
                .end = _.pos,
                .had_newline = had_newline,
            };
        }

        // Delimiters and structural punctuation
        if (c == '{') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.lbrace, .start = start, .end = _.pos, .had_newline = had_newline };
        }
        if (c == '}') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.rbrace, .start = start, .end = _.pos, .had_newline = had_newline };
        }
        if (c == '[') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.lbracket, .start = start, .end = _.pos, .had_newline = had_newline };
        }
        if (c == ']') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.rbracket, .start = start, .end = _.pos, .had_newline = had_newline };
        }
        if (c == '(') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.lparen, .start = start, .end = _.pos, .had_newline = had_newline };
        }
        if (c == ')') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.rparen, .start = start, .end = _.pos, .had_newline = had_newline };
        }
        if (c == ',') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.comma, .start = start, .end = _.pos, .had_newline = had_newline };
        }
        if (c == '.') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.dot, .start = start, .end = _.pos, .had_newline = had_newline };
        }
        if (c == ';') {
            _.pos += 1;
            return RegoSpanToken{ .kind = RegoTokenKind.semicolon, .start = start, .end = _.pos, .had_newline = had_newline };
        }

        // Numbers: integer, decimal float, scientific notation
        if (isDigit(c)) {
            while (_.pos < n && isDigit(s[_.pos])) {
                _.pos += 1;
            }
            if (_.pos + 1 < n && s[_.pos] == '.' && isDigit(s[_.pos + 1])) {
                _.pos += 1;
                while (_.pos < n && isDigit(s[_.pos])) {
                    _.pos += 1;
                }
            }
            if (_.pos < n && (s[_.pos] == 'e' || s[_.pos] == 'E')) {
                _.pos += 1;
                if (_.pos < n && (s[_.pos] == '+' || s[_.pos] == '-')) {
                    _.pos += 1;
                }
                const exp_start = _.pos;
                while (_.pos < n && isDigit(s[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos == exp_start) {
                    return RegoSpanToken{
                        .kind = RegoTokenKind.invalid,
                        .start = start,
                        .end = _.pos,
                        .had_newline = had_newline,
                    };
                }
            }
            return RegoSpanToken{
                .kind = RegoTokenKind.number,
                .start = start,
                .end = _.pos,
                .had_newline = had_newline,
            };
        }

        // Identifiers, booleans, null
        if (isIdentStart(c)) {
            _.pos += 1;
            while (_.pos < n && isIdentCont(s[_.pos])) {
                _.pos += 1;
            }
            const lex = s[start.._.pos];
            if (strEql(lex, "true") || strEql(lex, "false")) {
                return RegoSpanToken{
                    .kind = RegoTokenKind.bool_lit,
                    .start = start,
                    .end = _.pos,
                    .had_newline = had_newline,
                };
            }
            if (strEql(lex, "null")) {
                return RegoSpanToken{
                    .kind = RegoTokenKind.null_lit,
                    .start = start,
                    .end = _.pos,
                    .had_newline = had_newline,
                };
            }
            return RegoSpanToken{
                .kind = RegoTokenKind.ident,
                .start = start,
                .end = _.pos,
                .had_newline = had_newline,
            };
        }

        // Two-char punctuation operators: ==, !=, <=, >=
        if (start + 1 < n) {
            const c2 = s[start + 1];
            if ((c == '=' && c2 == '=') ||
                (c == '!' && c2 == '=') ||
                (c == '<' && c2 == '=') ||
                (c == '>' && c2 == '=')) {
                _.pos += 2;
                return RegoSpanToken{
                    .kind = RegoTokenKind.punct,
                    .start = start,
                    .end = _.pos,
                    .had_newline = had_newline,
                };
            }
        }

        // Single-char punctuation operators
        if (c == '=' || c == '<' || c == '>' || c == '+' || c == '-' ||
            c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == '!') {
            _.pos += 1;
            return RegoSpanToken{
                .kind = RegoTokenKind.punct,
                .start = start,
                .end = _.pos,
                .had_newline = had_newline,
            };
        }

        _.pos += 1;
        return RegoSpanToken{
            .kind = RegoTokenKind.invalid,
            .start = start,
            .end = _.pos,
            .had_newline = had_newline,
        };
    }

    fn next(*_): RegoToken? {
        if (sp := _.nextSpan()) {
            return RegoToken{
                .kind = sp.kind,
                .lexeme = _.input[sp.start..sp.end],
            };
        }
        return null;
    }
}

fn scanDottedPath(it: *RegoIter): ([]const u8)? {
    const first = it.nextSpan() ?? return null;
    if (first.kind != RegoTokenKind.ident) {
        return null;
    }
    const path_start = first.start;
    let path_end = first.end;

    while (true) {
        const saved_pos = it.pos;
        const dot_tok = it.nextSpan() ?? break;
        if (dot_tok.kind != RegoTokenKind.dot || dot_tok.had_newline) {
            it.pos = saved_pos;
            break;
        }
        const seg = it.nextSpan() ?? return null;
        if (seg.kind != RegoTokenKind.ident || seg.had_newline) {
            return null;
        }
        path_end = seg.end;
    }

    return it.input[path_start..path_end];
}

pub fn extractRegoPackage(input: []const u8): ([]const u8)? {
    let it = RegoIter.init(input);
    let depth: usize = 0;

    while (sp := it.nextSpan()) {
        if (sp.kind == RegoTokenKind.lbrace || sp.kind == RegoTokenKind.lbracket || sp.kind == RegoTokenKind.lparen) {
            depth += 1;
            continue;
        }
        if (sp.kind == RegoTokenKind.rbrace || sp.kind == RegoTokenKind.rbracket || sp.kind == RegoTokenKind.rparen) {
            if (depth > 0) depth -= 1;
            continue;
        }
        if (depth == 0 && sp.kind == RegoTokenKind.ident && strEql(input[sp.start..sp.end], "package")) {
            return scanDottedPath(*it);
        }
    }
    return null;
}

pub struct RegoImportIter {
    iter: RegoIter,
    depth: usize,

    fn init(input: []const u8): RegoImportIter {
        return RegoImportIter{
            .iter = RegoIter.init(input),
            .depth = 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (sp := _.iter.nextSpan()) {
            if (sp.kind == RegoTokenKind.lbrace || sp.kind == RegoTokenKind.lbracket || sp.kind == RegoTokenKind.lparen) {
                _.depth += 1;
                continue;
            }
            if (sp.kind == RegoTokenKind.rbrace || sp.kind == RegoTokenKind.rbracket || sp.kind == RegoTokenKind.rparen) {
                if (_.depth > 0) _.depth -= 1;
                continue;
            }
            if (_.depth == 0 && sp.kind == RegoTokenKind.ident && strEql(_.iter.input[sp.start..sp.end], "import")) {
                if (path := scanDottedPath(*_.iter)) {
                    return path;
                }
            }
        }
        return null;
    }
}

fn isOpenDelim(kind: RegoTokenKind): bool {
    return kind == RegoTokenKind.lbrace || kind == RegoTokenKind.lbracket || kind == RegoTokenKind.lparen;
}

fn isCloseDelim(kind: RegoTokenKind): bool {
    return kind == RegoTokenKind.rbrace || kind == RegoTokenKind.rbracket || kind == RegoTokenKind.rparen;
}

pub struct RegoRuleIter {
    iter: RegoIter,
    depth: usize,
    at_stmt_start: bool,

    fn init(input: []const u8): RegoRuleIter {
        return RegoRuleIter{
            .iter = RegoIter.init(input),
            .depth = 0,
            .at_stmt_start = true,
        };
    }

    fn next(*_): ([]const u8)? {
        const s = _.iter.input;

        while (sp := _.iter.nextSpan()) {
            if (isOpenDelim(sp.kind)) {
                _.depth += 1;
                _.at_stmt_start = false;
                continue;
            }
            if (isCloseDelim(sp.kind)) {
                if (_.depth > 0) _.depth -= 1;
                if (_.depth == 0 && sp.kind == RegoTokenKind.rbrace) {
                    _.at_stmt_start = true;
                }
                continue;
            }

            if (_.depth > 0) {
                continue;
            }

            if (sp.kind == RegoTokenKind.semicolon) {
                _.at_stmt_start = true;
                continue;
            }

            if (sp.had_newline) {
                _.at_stmt_start = true;
            }

            if (!_.at_stmt_start) {
                continue;
            }

            if (sp.kind != RegoTokenKind.ident) {
                _.at_stmt_start = false;
                continue;
            }

            const word = s[sp.start..sp.end];

            // Skip top-level package, import, and else clauses
            if (strEql(word, "package") || strEql(word, "import") || strEql(word, "else")) {
                _.at_stmt_start = false;
                continue;
            }

            // default <rule_name> := <expr>
            if (strEql(word, "default")) {
                _.at_stmt_start = false;
                if (name_tok := _.iter.nextSpan()) {
                    if (name_tok.kind == RegoTokenKind.ident) {
                        return s[name_tok.start..name_tok.end];
                    }
                }
                continue;
            }

            // Peek next token to confirm this identifier starts a rule head
            const saved_pos = _.iter.pos;
            const nxt_opt = _.iter.nextSpan();
            _.iter.pos = saved_pos;

            if (nxt := nxt_opt) {
                const nxt_lex = s[nxt.start..nxt.end];
                const is_rule_head =
                    nxt.kind == RegoTokenKind.assign ||
                    nxt.kind == RegoTokenKind.lbrace ||
                    nxt.kind == RegoTokenKind.lbracket ||
                    nxt.kind == RegoTokenKind.lparen ||
                    nxt.kind == RegoTokenKind.dot ||
                    (nxt.kind == RegoTokenKind.punct && strEql(nxt_lex, "=")) ||
                    (nxt.kind == RegoTokenKind.ident && (strEql(nxt_lex, "if") || strEql(nxt_lex, "contains")));

                if (is_rule_head) {
                    _.at_stmt_start = false;
                    return word;
                }
            }

            _.at_stmt_start = false;
        }

        return null;
    }
}

pub fn validateRegoDelimiters(input: []const u8): bool {
    let stack: [REGO_MAX_DEPTH]u8 = std.mem.zeroes([REGO_MAX_DEPTH]u8);
    let depth: usize = 0;
    let it = RegoIter.init(input);

    while (tok := it.next()) {
        if (tok.kind == RegoTokenKind.invalid) {
            return false;
        }
        if (tok.kind == RegoTokenKind.lbrace) {
            if (depth >= REGO_MAX_DEPTH) return false;
            stack[depth] = '}';
            depth += 1;
        } else if (tok.kind == RegoTokenKind.lbracket) {
            if (depth >= REGO_MAX_DEPTH) return false;
            stack[depth] = ']';
            depth += 1;
        } else if (tok.kind == RegoTokenKind.lparen) {
            if (depth >= REGO_MAX_DEPTH) return false;
            stack[depth] = ')';
            depth += 1;
        } else if (tok.kind == RegoTokenKind.rbrace) {
            if (depth == 0 || stack[depth - 1] != '}') return false;
            depth -= 1;
        } else if (tok.kind == RegoTokenKind.rbracket) {
            if (depth == 0 || stack[depth - 1] != ']') return false;
            depth -= 1;
        } else if (tok.kind == RegoTokenKind.rparen) {
            if (depth == 0 || stack[depth - 1] != ')') return false;
            depth -= 1;
        }
    }

    return depth == 0;
}
