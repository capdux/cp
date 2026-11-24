// Origin: Pure-CP Stdlib (Zero-Alloc Nix Expression Lexer & Flake Scanner) | Created: 2026-11-22

import std from "std";

pub const NIX_MAX_DEPTH: usize = 64;

pub enum NixTokenKind {
    ident,
    string,
    indented_string,
    path,
    uri,
    number,
    lbrace,
    rbrace,
    lbracket,
    rbracket,
    lparen,
    rparen,
    equals,
    semicolon,
    colon,
    comma,
    dot,
    ellipsis,
    punct,
    invalid,
}

pub struct NixToken {
    kind: NixTokenKind,
    lexeme: []const u8,
}

pub struct NixAttrBinding {
    path: []const u8,
    value: []const u8,
}

struct NixSpanToken {
    kind: NixTokenKind,
    start: usize,
    end: usize,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isIdentStart(c: u8): bool {
    return isAlpha(c) || c == '_';
}

fn isIdentCont(c: u8): bool {
    return isAlpha(c) || isDigit(c) || c == '_' || c == '\'' || c == '-';
}

fn isPathChar(c: u8): bool {
    return isAlpha(c) || isDigit(c) || c == '.' || c == '_' || c == '-' || c == '+' || c == '/';
}

fn isUriSchemeCont(c: u8): bool {
    return isAlpha(c) || isDigit(c) || c == '+' || c == '-' || c == '.';
}

fn isUriBodyChar(c: u8): bool {
    return isAlpha(c) || isDigit(c) ||
        c == '%' || c == '/' || c == '?' || c == ':' || c == '@' ||
        c == '&' || c == '=' || c == '+' || c == '$' || c == ',' ||
        c == '_' || c == '.' || c == '!' || c == '~' || c == '*' ||
        c == '\'' || c == '-';
}

fn trimSpaces(s: []const u8): []const u8 {
    let lo: usize = 0;
    let hi: usize = s.len;
    while (lo < hi && isSpace(s[lo])) {
        lo += 1;
    }
    while (hi > lo && isSpace(s[hi - 1])) {
        hi -= 1;
    }
    return s[lo..hi];
}

// Advance past a balanced ${ ... } interpolation starting right after "${".
// Returns new position right after the matching '}', or null if invalid/unclosed/too deep.
fn skipInterpolation(s: []const u8, start_pos: usize, depth: usize): usize? {
    if (depth >= NIX_MAX_DEPTH) {
        return null;
    }
    const n = s.len;
    let pos = start_pos;
    let brace_depth: usize = 1;

    while (pos < n) {
        const c = s[pos];
        if (c == '#') {
            pos += 1;
            while (pos < n && s[pos] != '\n') {
                pos += 1;
            }
            continue;
        }
        if (c == '/' && pos + 1 < n && s[pos + 1] == '*') {
            pos += 2;
            let closed = false;
            while (pos + 1 < n) {
                if (s[pos] == '*' && s[pos + 1] == '/') {
                    pos += 2;
                    closed = true;
                    break;
                }
                pos += 1;
            }
            if (!closed) return null;
            continue;
        }
        if (c == '"') {
            if (end_pos := skipDoubleQuoted(s, pos, depth + 1)) {
                pos = end_pos;
                continue;
            }
            return null;
        }
        if (c == '\'' && pos + 1 < n && s[pos + 1] == '\'') {
            if (end_pos := skipIndentedString(s, pos, depth + 1)) {
                pos = end_pos;
                continue;
            }
            return null;
        }
        if (c == '{') {
            if (brace_depth + depth >= NIX_MAX_DEPTH) return null;
            brace_depth += 1;
            pos += 1;
            continue;
        }
        if (c == '}') {
            brace_depth -= 1;
            pos += 1;
            if (brace_depth == 0) {
                return pos;
            }
            continue;
        }
        pos += 1;
    }
    return null;
}

// Advance past a double-quoted Nix string starting at '"', supporting escapes and ${...}.
fn skipDoubleQuoted(s: []const u8, start_pos: usize, depth: usize): usize? {
    if (depth >= NIX_MAX_DEPTH) {
        return null;
    }
    const n = s.len;
    let pos = start_pos + 1;
    while (pos < n) {
        const ch = s[pos];
        if (ch == '\\') {
            if (pos + 1 < n) {
                pos += 2;
                continue;
            }
            return null;
        }
        if (ch == '$' && pos + 1 < n && s[pos + 1] == '{') {
            if (after := skipInterpolation(s, pos + 2, depth + 1)) {
                pos = after;
                continue;
            }
            return null;
        }
        if (ch == '"') {
            return pos + 1;
        }
        pos += 1;
    }
    return null;
}

// Advance past an indented Nix string starting at "''", supporting ''' / ''$ / ''\<c> / ${...}.
fn skipIndentedString(s: []const u8, start_pos: usize, depth: usize): usize? {
    if (depth >= NIX_MAX_DEPTH) {
        return null;
    }
    const n = s.len;
    let pos = start_pos + 2;
    while (pos < n) {
        if (s[pos] == '$' && pos + 1 < n && s[pos + 1] == '{') {
            if (after := skipInterpolation(s, pos + 2, depth + 1)) {
                pos = after;
                continue;
            }
            return null;
        }
        if (s[pos] == '\'' && pos + 1 < n && s[pos + 1] == '\'') {
            if (pos + 2 < n) {
                const next_c = s[pos + 2];
                if (next_c == '\'' || next_c == '$') {
                    pos += 3;
                    continue;
                }
                if (next_c == '\\') {
                    if (pos + 3 < n) {
                        pos += 4;
                        continue;
                    }
                    return null;
                }
            }
            return pos + 2;
        }
        pos += 1;
    }
    return null;
}

pub struct NixIter {
    input: []const u8,
    pos: usize,

    static fn init(input: []const u8): NixIter {
        return NixIter{
            .input = input,
            .pos = 0,
        };
    }

    fn nextSpan(*_): NixSpanToken? {
        const s = _.input;
        const n = s.len;

        // Skip whitespace, '#' line comments, and '/* ... */' block comments
        while (_.pos < n) {
            const c = s[_.pos];
            if (isSpace(c)) {
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
                    return NixSpanToken{
                        .kind = NixTokenKind.invalid,
                        .start = start,
                        .end = n,
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

        // Double-quoted string with ${...} interpolation
        if (c == '"') {
            if (end_pos := skipDoubleQuoted(s, start, 0)) {
                _.pos = end_pos;
                return NixSpanToken{
                    .kind = NixTokenKind.string,
                    .start = start,
                    .end = end_pos,
                };
            }
            _.pos = n;
            return NixSpanToken{
                .kind = NixTokenKind.invalid,
                .start = start,
                .end = n,
            };
        }

        // Indented string ''...''
        if (c == '\'' && start + 1 < n && s[start + 1] == '\'') {
            if (end_pos := skipIndentedString(s, start, 0)) {
                _.pos = end_pos;
                return NixSpanToken{
                    .kind = NixTokenKind.indented_string,
                    .start = start,
                    .end = end_pos,
                };
            }
            _.pos = n;
            return NixSpanToken{
                .kind = NixTokenKind.invalid,
                .start = start,
                .end = n,
            };
        }

        // Angle-bracket search path: <nixpkgs> or <nixpkgs/lib>
        if (c == '<') {
            let p = start + 1;
            let valid_spath = false;
            if (p < n && isPathChar(s[p]) && s[p] != '/') {
                while (p < n && isPathChar(s[p])) {
                    p += 1;
                }
                if (p < n && s[p] == '>') {
                    valid_spath = true;
                }
            }
            if (valid_spath) {
                _.pos = p + 1;
                return NixSpanToken{
                    .kind = NixTokenKind.path,
                    .start = start,
                    .end = _.pos,
                };
            }
        }

        // Home path ~/...
        if (c == '~' && start + 1 < n && s[start + 1] == '/') {
            let p = start + 2;
            if (p < n && isPathChar(s[p])) {
                while (p < n && isPathChar(s[p])) {
                    p += 1;
                }
                _.pos = p;
                return NixSpanToken{
                    .kind = NixTokenKind.path,
                    .start = start,
                    .end = _.pos,
                };
            }
        }

        // Ellipsis '...'
        if (c == '.' && start + 2 < n && s[start + 1] == '.' && s[start + 2] == '.') {
            _.pos += 3;
            return NixSpanToken{
                .kind = NixTokenKind.ellipsis,
                .start = start,
                .end = _.pos,
            };
        }

        // Relative or absolute filesystem path: ./foo, ../foo, /etc/nixos
        if ((c == '.' && start + 1 < n && s[start + 1] == '/') ||
            (c == '.' && start + 2 < n && s[start + 1] == '.' && s[start + 2] == '/') ||
            (c == '/' && start + 1 < n && isPathChar(s[start + 1]) && s[start + 1] != '/')) {
            let p = start;
            while (p < n && isPathChar(s[p])) {
                p += 1;
            }
            _.pos = p;
            return NixSpanToken{
                .kind = NixTokenKind.path,
                .start = start,
                .end = _.pos,
            };
        }

        // Delimiters and single-char tokens
        if (c == '{') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.lbrace, .start = start, .end = _.pos };
        }
        if (c == '}') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.rbrace, .start = start, .end = _.pos };
        }
        if (c == '[') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.lbracket, .start = start, .end = _.pos };
        }
        if (c == ']') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.rbracket, .start = start, .end = _.pos };
        }
        if (c == '(') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.lparen, .start = start, .end = _.pos };
        }
        if (c == ')') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.rparen, .start = start, .end = _.pos };
        }
        if (c == ';') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.semicolon, .start = start, .end = _.pos };
        }
        if (c == ':') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.colon, .start = start, .end = _.pos };
        }
        if (c == ',') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.comma, .start = start, .end = _.pos };
        }
        if (c == '.') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.dot, .start = start, .end = _.pos };
        }

        // '=' vs '=='
        if (c == '=') {
            if (start + 1 < n && s[start + 1] == '=') {
                _.pos += 2;
                return NixSpanToken{ .kind = NixTokenKind.punct, .start = start, .end = _.pos };
            }
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.equals, .start = start, .end = _.pos };
        }

        // Numbers: integer, float, scientific notation
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
                    return NixSpanToken{ .kind = NixTokenKind.invalid, .start = start, .end = _.pos };
                }
            }
            return NixSpanToken{ .kind = NixTokenKind.number, .start = start, .end = _.pos };
        }

        // URI, unquoted relative path (foo/bar.nix), or identifier
        if (isIdentStart(c)) {
            // Check URI: [A-Za-z][A-Za-z0-9+.-]*:<uri-body>
            if (isAlpha(c)) {
                let u = start + 1;
                while (u < n && isUriSchemeCont(s[u])) {
                    u += 1;
                }
                if (u + 1 < n && s[u] == ':' && isUriBodyChar(s[u + 1])) {
                    u += 1;
                    while (u < n && isUriBodyChar(s[u])) {
                        u += 1;
                    }
                    _.pos = u;
                    return NixSpanToken{ .kind = NixTokenKind.uri, .start = start, .end = _.pos };
                }
            }

            // Check relative path without leading './': foo/bar.nix
            let p = start;
            let has_slash = false;
            while (p < n && isPathChar(s[p])) {
                if (s[p] == '/') {
                    if (p + 1 < n && isPathChar(s[p + 1]) && s[p + 1] != '/') {
                        has_slash = true;
                    } else {
                        break;
                    }
                }
                p += 1;
            }
            if (has_slash) {
                _.pos = p;
                return NixSpanToken{ .kind = NixTokenKind.path, .start = start, .end = _.pos };
            }

            // Standard Nix identifier (allows '-' and "'", stops before "->" or "''")
            _.pos += 1;
            while (_.pos < n && isIdentCont(s[_.pos])) {
                if (s[_.pos] == '-' && _.pos + 1 < n && s[_.pos + 1] == '>') {
                    break;
                }
                if (s[_.pos] == '\'' && _.pos + 1 < n && s[_.pos + 1] == '\'') {
                    break;
                }
                _.pos += 1;
            }
            return NixSpanToken{ .kind = NixTokenKind.ident, .start = start, .end = _.pos };
        }

        // Two-char punctuation operators: !=, <=, >=, &&, ||, //, ++, ->
        if (start + 1 < n) {
            const c2 = s[start + 1];
            if ((c == '!' && c2 == '=') ||
                (c == '<' && c2 == '=') ||
                (c == '>' && c2 == '=') ||
                (c == '&' && c2 == '&') ||
                (c == '|' && c2 == '|') ||
                (c == '/' && c2 == '/') ||
                (c == '+' && c2 == '+') ||
                (c == '-' && c2 == '>')) {
                _.pos += 2;
                return NixSpanToken{ .kind = NixTokenKind.punct, .start = start, .end = _.pos };
            }
        }

        // Single-char punctuation operators
        if (c == '?' || c == '+' || c == '-' || c == '*' || c == '/' ||
            c == '!' || c == '<' || c == '>' || c == '@') {
            _.pos += 1;
            return NixSpanToken{ .kind = NixTokenKind.punct, .start = start, .end = _.pos };
        }

        _.pos += 1;
        return NixSpanToken{ .kind = NixTokenKind.invalid, .start = start, .end = _.pos };
    }

    fn next(*_): NixToken? {
        if (sp := _.nextSpan()) {
            return NixToken{
                .kind = sp.kind,
                .lexeme = _.input[sp.start..sp.end],
            };
        }
        return null;
    }
}

fn isOpenDelim(kind: NixTokenKind): bool {
    return kind == NixTokenKind.lbrace || kind == NixTokenKind.lbracket || kind == NixTokenKind.lparen;
}

fn isCloseDelim(kind: NixTokenKind): bool {
    return kind == NixTokenKind.rbrace || kind == NixTokenKind.rbracket || kind == NixTokenKind.rparen;
}

pub struct NixAttrIter {
    iter: NixIter,
    done: bool,

    fn init(attrset_body: []const u8): NixAttrIter {
        let it = NixIter.init(attrset_body);
        const saved = it.pos;
        if (first := it.nextSpan()) {
            if (first.kind == NixTokenKind.lbrace) {
                // Start inside the outer '{ ... }'
                return NixAttrIter{ .iter = it, .done = false };
            }
            if (first.kind == NixTokenKind.ident && std.mem.eql(u8, attrset_body[first.start..first.end], "rec")) {
                if (second := it.nextSpan()) {
                    if (second.kind == NixTokenKind.lbrace) {
                        return NixAttrIter{ .iter = it, .done = false };
                    }
                }
            }
        }
        it.pos = saved;
        return NixAttrIter{ .iter = it, .done = false };
    }

    fn next(*_): NixAttrBinding? {
        if (_.done) {
            return null;
        }
        const s = _.iter.input;

        while (true) {
            const first_opt = _.iter.nextSpan();
            if (first_opt == null) {
                _.done = true;
                return null;
            }
            const first = first_opt ?? unreachable;

            if (first.kind == NixTokenKind.semicolon) {
                continue;
            }
            if (first.kind == NixTokenKind.rbrace || first.kind == NixTokenKind.invalid) {
                _.done = true;
                return null;
            }

            // Skip `inherit ...;` declarations
            if (first.kind == NixTokenKind.ident && std.mem.eql(u8, s[first.start..first.end], "inherit")) {
                let skip_depth: usize = 0;
                while (sp := _.iter.nextSpan()) {
                    if (isOpenDelim(sp.kind)) {
                        skip_depth += 1;
                    } else if (isCloseDelim(sp.kind)) {
                        if (skip_depth == 0) {
                            _.done = true;
                            return null;
                        }
                        skip_depth -= 1;
                    } else if (skip_depth == 0 && sp.kind == NixTokenKind.semicolon) {
                        break;
                    }
                }
                continue;
            }

            // Attribute path: (ident | string) ('.' (ident | string))*
            if (first.kind != NixTokenKind.ident && first.kind != NixTokenKind.string) {
                _.done = true;
                return null;
            }

            const path_start = first.start;
            let path_end = first.end;
            let saw_equals = false;

            while (sp := _.iter.nextSpan()) {
                if (sp.kind == NixTokenKind.equals) {
                    saw_equals = true;
                    break;
                }
                if (sp.kind == NixTokenKind.dot) {
                    const seg_opt = _.iter.nextSpan();
                    if (seg_opt == null) {
                        _.done = true;
                        return null;
                    }
                    const seg = seg_opt ?? unreachable;
                    if (seg.kind != NixTokenKind.ident && seg.kind != NixTokenKind.string) {
                        _.done = true;
                        return null;
                    }
                    path_end = seg.end;
                    continue;
                }
                // Unexpected token before '='
                _.done = true;
                return null;
            }

            if (!saw_equals) {
                _.done = true;
                return null;
            }

            const val_start = _.iter.pos;
            let depth: usize = 0;
            while (sp := _.iter.nextSpan()) {
                if (sp.kind == NixTokenKind.invalid) {
                    _.done = true;
                    return null;
                }
                if (isOpenDelim(sp.kind)) {
                    depth += 1;
                    continue;
                }
                if (isCloseDelim(sp.kind)) {
                    if (depth == 0) {
                        _.done = true;
                        return null;
                    }
                    depth -= 1;
                    continue;
                }
                if (depth == 0 && sp.kind == NixTokenKind.semicolon) {
                    return NixAttrBinding{
                        .path = s[path_start..path_end],
                        .value = trimSpaces(s[val_start..sp.start]),
                    };
                }
            }

            _.done = true;
            return null;
        }
    }
}

pub fn validateNixDelimiters(input: []const u8): bool {
    let stack: [NIX_MAX_DEPTH]u8 = std.mem.zeroes([NIX_MAX_DEPTH]u8);
    let depth: usize = 0;
    let it = NixIter.init(input);

    while (tok := it.next()) {
        if (tok.kind == NixTokenKind.invalid) {
            return false;
        }
        if (tok.kind == NixTokenKind.lbrace) {
            if (depth >= NIX_MAX_DEPTH) return false;
            stack[depth] = '}';
            depth += 1;
        } else if (tok.kind == NixTokenKind.lbracket) {
            if (depth >= NIX_MAX_DEPTH) return false;
            stack[depth] = ']';
            depth += 1;
        } else if (tok.kind == NixTokenKind.lparen) {
            if (depth >= NIX_MAX_DEPTH) return false;
            stack[depth] = ')';
            depth += 1;
        } else if (tok.kind == NixTokenKind.rbrace) {
            if (depth == 0 || stack[depth - 1] != '}') return false;
            depth -= 1;
        } else if (tok.kind == NixTokenKind.rbracket) {
            if (depth == 0 || stack[depth - 1] != ']') return false;
            depth -= 1;
        } else if (tok.kind == NixTokenKind.rparen) {
            if (depth == 0 || stack[depth - 1] != ')') return false;
            depth -= 1;
        }
    }

    return depth == 0;
}
