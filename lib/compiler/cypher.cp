// Origin: Pure-CP Stdlib (Zero-Alloc openCypher Graph Query Lexer & Analyzer) | Created: 2026-11-22

import std from "std";

pub const CYPHER_MAX_DEPTH: usize = 64;

pub enum CypherTokenKind {
    ident,
    param,
    string,
    backtick_ident,
    number,
    arrow_right,
    arrow_left,
    lparen,
    rparen,
    lbracket,
    rbracket,
    lbrace,
    rbrace,
    colon,
    comma,
    dot,
    punct,
    invalid,
}

pub enum CypherClauseKind {
    read,
    write,
    schema,
    call,
    other,
}

pub struct CypherToken {
    kind: CypherTokenKind,
    lexeme: []const u8,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isOctDigit(c: u8): bool {
    return c >= '0' && c <= '7';
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

fn isIdentCont(c: u8): bool {
    return isIdentStart(c) || isDigit(c);
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (asciiLower(a[i]) != asciiLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn classifyCypherKeyword(word: []const u8): CypherClauseKind {
    if (eqlIgnoreCase(word, "CREATE") ||
        eqlIgnoreCase(word, "MERGE") ||
        eqlIgnoreCase(word, "DELETE") ||
        eqlIgnoreCase(word, "DETACH") ||
        eqlIgnoreCase(word, "SET") ||
        eqlIgnoreCase(word, "REMOVE") ||
        eqlIgnoreCase(word, "FOREACH")) {
        return CypherClauseKind.write;
    }
    if (eqlIgnoreCase(word, "DROP") ||
        eqlIgnoreCase(word, "ALTER") ||
        eqlIgnoreCase(word, "GRANT") ||
        eqlIgnoreCase(word, "DENY") ||
        eqlIgnoreCase(word, "REVOKE")) {
        return CypherClauseKind.schema;
    }
    if (eqlIgnoreCase(word, "CALL")) {
        return CypherClauseKind.call;
    }
    if (eqlIgnoreCase(word, "MATCH") ||
        eqlIgnoreCase(word, "OPTIONAL") ||
        eqlIgnoreCase(word, "WHERE") ||
        eqlIgnoreCase(word, "RETURN") ||
        eqlIgnoreCase(word, "WITH") ||
        eqlIgnoreCase(word, "UNWIND") ||
        eqlIgnoreCase(word, "ORDER") ||
        eqlIgnoreCase(word, "SKIP") ||
        eqlIgnoreCase(word, "LIMIT") ||
        eqlIgnoreCase(word, "UNION") ||
        eqlIgnoreCase(word, "EXPLAIN") ||
        eqlIgnoreCase(word, "PROFILE")) {
        return CypherClauseKind.read;
    }
    return CypherClauseKind.other;
}

fn isMutatingKeyword(word: []const u8): bool {
    const cls = classifyCypherKeyword(word);
    return cls == CypherClauseKind.write || cls == CypherClauseKind.schema;
}

pub struct CypherIter {
    input: []const u8,
    pos: usize,

    static fn init(input: []const u8): CypherIter {
        return CypherIter{
            .input = input,
            .pos = 0,
        };
    }

    fn next(*_): CypherToken? {
        const s = _.input;
        const n = s.len;

        // Skip whitespace and comments (// line comments and /* block comments */)
        while (_.pos < n) {
            const c = s[_.pos];
            if (isSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '/' && _.pos + 1 < n && s[_.pos + 1] == '/') {
                _.pos += 2;
                while (_.pos < n && s[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            if (c == '/' && _.pos + 1 < n && s[_.pos + 1] == '*') {
                const start = _.pos;
                _.pos += 2;
                let closed = false;
                while (_.pos + 1 < n) {
                    if (s[_.pos] == '*' && s[_.pos + 1] == '/') {
                        _.pos += 2;
                        closed = true;
                        break;
                    }
                    _.pos += 1;
                }
                if (!closed) {
                    _.pos = n;
                    return CypherToken{
                        .kind = CypherTokenKind.invalid,
                        .lexeme = s[start..n],
                    };
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

        // Single-quoted or double-quoted string with backslash escapes
        if (c == '\'' || c == '"') {
            const quote = c;
            _.pos += 1;
            while (_.pos < n) {
                const ch = s[_.pos];
                if (ch == '\\') {
                    if (_.pos + 1 < n) {
                        _.pos += 2;
                        continue;
                    }
                    _.pos = n;
                    return CypherToken{
                        .kind = CypherTokenKind.invalid,
                        .lexeme = s[start..n],
                    };
                }
                if (ch == quote) {
                    _.pos += 1;
                    return CypherToken{
                        .kind = CypherTokenKind.string,
                        .lexeme = s[start.._.pos],
                    };
                }
                _.pos += 1;
            }
            return CypherToken{
                .kind = CypherTokenKind.invalid,
                .lexeme = s[start..n],
            };
        }

        // Backtick-escaped identifier (supports `` doubled backtick escape)
        if (c == '`') {
            _.pos += 1;
            while (_.pos < n) {
                if (s[_.pos] == '`') {
                    if (_.pos + 1 < n && s[_.pos + 1] == '`') {
                        _.pos += 2;
                        continue;
                    }
                    _.pos += 1;
                    return CypherToken{
                        .kind = CypherTokenKind.backtick_ident,
                        .lexeme = s[start.._.pos],
                    };
                }
                _.pos += 1;
            }
            return CypherToken{
                .kind = CypherTokenKind.invalid,
                .lexeme = s[start..n],
            };
        }

        // Parameter: $name or $0
        if (c == '$') {
            _.pos += 1;
            if (_.pos < n && isIdentStart(s[_.pos])) {
                _.pos += 1;
                while (_.pos < n && isIdentCont(s[_.pos])) {
                    _.pos += 1;
                }
                return CypherToken{
                    .kind = CypherTokenKind.param,
                    .lexeme = s[start.._.pos],
                };
            }
            if (_.pos < n && isDigit(s[_.pos])) {
                while (_.pos < n && isDigit(s[_.pos])) {
                    _.pos += 1;
                }
                return CypherToken{
                    .kind = CypherTokenKind.param,
                    .lexeme = s[start.._.pos],
                };
            }
            return CypherToken{
                .kind = CypherTokenKind.invalid,
                .lexeme = s[start.._.pos],
            };
        }

        // Relationship arrows: -> and <-
        if (c == '-' && start + 1 < n && s[start + 1] == '>') {
            _.pos += 2;
            return CypherToken{
                .kind = CypherTokenKind.arrow_right,
                .lexeme = s[start.._.pos],
            };
        }
        if (c == '<' && start + 1 < n && s[start + 1] == '-') {
            _.pos += 2;
            return CypherToken{
                .kind = CypherTokenKind.arrow_left,
                .lexeme = s[start.._.pos],
            };
        }

        // Delimiters and single-char structural tokens
        if (c == '(') {
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.lparen, .lexeme = s[start.._.pos] };
        }
        if (c == ')') {
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.rparen, .lexeme = s[start.._.pos] };
        }
        if (c == '[') {
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.lbracket, .lexeme = s[start.._.pos] };
        }
        if (c == ']') {
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.rbracket, .lexeme = s[start.._.pos] };
        }
        if (c == '{') {
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.lbrace, .lexeme = s[start.._.pos] };
        }
        if (c == '}') {
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.rbrace, .lexeme = s[start.._.pos] };
        }
        if (c == ':') {
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.colon, .lexeme = s[start.._.pos] };
        }
        if (c == ',') {
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.comma, .lexeme = s[start.._.pos] };
        }
        if (c == '.') {
            if (start + 1 < n && s[start + 1] == '.') {
                _.pos += 2;
                return CypherToken{ .kind = CypherTokenKind.punct, .lexeme = s[start.._.pos] };
            }
            _.pos += 1;
            return CypherToken{ .kind = CypherTokenKind.dot, .lexeme = s[start.._.pos] };
        }

        // Numbers: hex (0x..), octal (0o..), decimal integer, float, scientific notation
        if (isDigit(c)) {
            if (c == '0' && start + 1 < n && (s[start + 1] == 'x' || s[start + 1] == 'X')) {
                _.pos += 2;
                const hex_start = _.pos;
                while (_.pos < n && isHexDigit(s[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos == hex_start) {
                    return CypherToken{ .kind = CypherTokenKind.invalid, .lexeme = s[start.._.pos] };
                }
                return CypherToken{ .kind = CypherTokenKind.number, .lexeme = s[start.._.pos] };
            }
            if (c == '0' && start + 1 < n && (s[start + 1] == 'o' || s[start + 1] == 'O')) {
                _.pos += 2;
                const oct_start = _.pos;
                while (_.pos < n && isOctDigit(s[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos == oct_start) {
                    return CypherToken{ .kind = CypherTokenKind.invalid, .lexeme = s[start.._.pos] };
                }
                return CypherToken{ .kind = CypherTokenKind.number, .lexeme = s[start.._.pos] };
            }

            while (_.pos < n && isDigit(s[_.pos])) {
                _.pos += 1;
            }
            // Fractional part only if '.' is followed by a digit (preserves 1..3 range operator)
            if (_.pos + 1 < n && s[_.pos] == '.' && isDigit(s[_.pos + 1])) {
                _.pos += 1;
                while (_.pos < n && isDigit(s[_.pos])) {
                    _.pos += 1;
                }
            }
            // Exponent part
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
                    return CypherToken{ .kind = CypherTokenKind.invalid, .lexeme = s[start.._.pos] };
                }
            }
            return CypherToken{
                .kind = CypherTokenKind.number,
                .lexeme = s[start.._.pos],
            };
        }

        // Identifiers and keywords
        if (isIdentStart(c)) {
            _.pos += 1;
            while (_.pos < n && isIdentCont(s[_.pos])) {
                _.pos += 1;
            }
            return CypherToken{
                .kind = CypherTokenKind.ident,
                .lexeme = s[start.._.pos],
            };
        }

        // Multi-char punctuation operators: <>, <=, >=, =~, +=
        if (start + 1 < n) {
            const c2 = s[start + 1];
            if ((c == '<' && (c2 == '>' || c2 == '=')) ||
                (c == '>' && c2 == '=') ||
                (c == '=' && c2 == '~') ||
                (c == '+' && c2 == '=')) {
                _.pos += 2;
                return CypherToken{
                    .kind = CypherTokenKind.punct,
                    .lexeme = s[start.._.pos],
                };
            }
        }

        // Single-char punctuation operators
        if (c == '-' || c == '+' || c == '*' || c == '/' || c == '%' || c == '^' ||
            c == '=' || c == '<' || c == '>' || c == '!' || c == '~' || c == '|' ||
            c == ';' || c == '&' || c == '?' || c == '@') {
            _.pos += 1;
            return CypherToken{
                .kind = CypherTokenKind.punct,
                .lexeme = s[start.._.pos],
            };
        }

        // Unknown / control byte
        _.pos += 1;
        return CypherToken{
            .kind = CypherTokenKind.invalid,
            .lexeme = s[start.._.pos],
        };
    }
}

pub struct CypherParamIter {
    iter: CypherIter,

    fn init(query: []const u8): CypherParamIter {
        return CypherParamIter{
            .iter = CypherIter.init(query),
        };
    }

    fn next(*_): ([]const u8)? {
        while (tok := _.iter.next()) {
            if (tok.kind == CypherTokenKind.param && tok.lexeme.len > 1) {
                return tok.lexeme[1..];
            }
        }
        return null;
    }
}

pub fn validateCypherDelimiters(query: []const u8): bool {
    let stack: [CYPHER_MAX_DEPTH]u8 = std.mem.zeroes([CYPHER_MAX_DEPTH]u8);
    let depth: usize = 0;
    let it = CypherIter.init(query);

    while (tok := it.next()) {
        if (tok.kind == CypherTokenKind.invalid) {
            return false;
        }
        if (tok.kind == CypherTokenKind.lparen) {
            if (depth >= CYPHER_MAX_DEPTH) return false;
            stack[depth] = ')';
            depth += 1;
        } else if (tok.kind == CypherTokenKind.lbracket) {
            if (depth >= CYPHER_MAX_DEPTH) return false;
            stack[depth] = ']';
            depth += 1;
        } else if (tok.kind == CypherTokenKind.lbrace) {
            if (depth >= CYPHER_MAX_DEPTH) return false;
            stack[depth] = '}';
            depth += 1;
        } else if (tok.kind == CypherTokenKind.rparen) {
            if (depth == 0 || stack[depth - 1] != ')') return false;
            depth -= 1;
        } else if (tok.kind == CypherTokenKind.rbracket) {
            if (depth == 0 || stack[depth - 1] != ']') return false;
            depth -= 1;
        } else if (tok.kind == CypherTokenKind.rbrace) {
            if (depth == 0 || stack[depth - 1] != '}') return false;
            depth -= 1;
        }
    }

    return depth == 0;
}

pub fn isReadOnlyCypher(query: []const u8): bool {
    let stack: [CYPHER_MAX_DEPTH]u8 = std.mem.zeroes([CYPHER_MAX_DEPTH]u8);
    let depth: usize = 0;
    let paren_or_bracket_depth: usize = 0;
    let token_count: usize = 0;
    let prev_was_dot = false;
    let prev_was_colon = false;

    let it = CypherIter.init(query);
    let cur_opt = it.next();

    while (cur := cur_opt) {
        const next_opt = it.next();
        token_count += 1;

        if (cur.kind == CypherTokenKind.invalid) {
            return false;
        }

        if (cur.kind == CypherTokenKind.lparen) {
            if (depth >= CYPHER_MAX_DEPTH) return false;
            stack[depth] = ')';
            depth += 1;
            paren_or_bracket_depth += 1;
        } else if (cur.kind == CypherTokenKind.lbracket) {
            if (depth >= CYPHER_MAX_DEPTH) return false;
            stack[depth] = ']';
            depth += 1;
            paren_or_bracket_depth += 1;
        } else if (cur.kind == CypherTokenKind.lbrace) {
            if (depth >= CYPHER_MAX_DEPTH) return false;
            stack[depth] = '}';
            depth += 1;
        } else if (cur.kind == CypherTokenKind.rparen) {
            if (depth == 0 || stack[depth - 1] != ')') return false;
            depth -= 1;
            paren_or_bracket_depth -= 1;
        } else if (cur.kind == CypherTokenKind.rbracket) {
            if (depth == 0 || stack[depth - 1] != ']') return false;
            depth -= 1;
            paren_or_bracket_depth -= 1;
        } else if (cur.kind == CypherTokenKind.rbrace) {
            if (depth == 0 || stack[depth - 1] != '}') return false;
            depth -= 1;
        } else if (cur.kind == CypherTokenKind.ident) {
            // Check top-level identifiers and CALL { ... } subquery blocks (outside () / [])
            if (paren_or_bracket_depth == 0 && !prev_was_dot && !prev_was_colon) {
                let next_is_colon = false;
                if (nxt := next_opt) {
                    if (nxt.kind == CypherTokenKind.colon) {
                        next_is_colon = true;
                    }
                }
                // Inside { ... }, identifier followed by ':' is a map literal key
                if (!(depth > 0 && next_is_colon)) {
                    if (isMutatingKeyword(cur.lexeme)) {
                        return false;
                    }
                }
            }
        }

        prev_was_dot = cur.kind == CypherTokenKind.dot;
        prev_was_colon = cur.kind == CypherTokenKind.colon;
        cur_opt = next_opt;
    }

    if (token_count == 0 || depth != 0) {
        return false;
    }
    return true;
}
