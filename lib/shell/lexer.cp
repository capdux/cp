// Source: /data/dev/bun/src/shell/
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/lexer.cp
// Fast lexical analyzer for CP Shell (cap:shell)
// Absorbed from Bun Shell lexer architecture with pure CP semantics

pub enum TokenKind {
    Eof,
    Word,
    Pipe,           // |
    And,            // &&
    Or,             // ||
    Semicolon,      // ;
    RedirectOut,    // >
    RedirectAppend, // >>
    RedirectIn,     // <
}

pub struct Token {
    kind: TokenKind = .Eof,
    slice: []const u8 = "",
    pos: usize = 0,
}

pub struct Lexer {
    src: []const u8 = "",
    cursor: usize = 0,

    pub static fn init(source: []const u8): Lexer {
        return Lexer {
            src: source,
            cursor: 0,
        };
    }

    pub fn isEof(*_): bool {
        return _.cursor >= _.src.len;
    }

    pub fn skipWhitespace(*_) {
        while (_.cursor < _.src.len) {
            const c = _.src[_.cursor];
            if (c == ' ' as u8 || c == '\t' as u8 || c == '\r' as u8 || c == '\n' as u8) {
                _.cursor += 1;
            } else {
                break;
            }
        }
    }

    pub fn nextToken(*_): Token {
        _.skipWhitespace();
        if (_.cursor >= _.src.len) {
            return Token {
                kind: .Eof,
                slice: "",
                pos: _.cursor,
            };
        }

        const start = _.cursor;
        const c = _.src[_.cursor];

        // Operators
        if (c == '|' as u8) {
            _.cursor += 1;
            if (_.cursor < _.src.len && _.src[_.cursor] == '|' as u8) {
                _.cursor += 1;
                return Token { kind: .Or, slice: _.src[start.._.cursor], pos: start };
            }
            return Token { kind: .Pipe, slice: _.src[start.._.cursor], pos: start };
        }

        if (c == '&' as u8) {
            _.cursor += 1;
            if (_.cursor < _.src.len && _.src[_.cursor] == '&' as u8) {
                _.cursor += 1;
                return Token { kind: .And, slice: _.src[start.._.cursor], pos: start };
            }
            return Token { kind: .Word, slice: _.src[start.._.cursor], pos: start };
        }

        if (c == ';' as u8) {
            _.cursor += 1;
            return Token { kind: .Semicolon, slice: _.src[start.._.cursor], pos: start };
        }

        if (c == '>' as u8) {
            _.cursor += 1;
            if (_.cursor < _.src.len && _.src[_.cursor] == '>' as u8) {
                _.cursor += 1;
                return Token { kind: .RedirectAppend, slice: _.src[start.._.cursor], pos: start };
            }
            return Token { kind: .RedirectOut, slice: _.src[start.._.cursor], pos: start };
        }

        if (c == '<' as u8) {
            _.cursor += 1;
            return Token { kind: .RedirectIn, slice: _.src[start.._.cursor], pos: start };
        }

        // Quoted words
        if (c == '"' as u8 || c == '\'' as u8) {
            const quote = c;
            _.cursor += 1;
            const content_start = _.cursor;
            while (_.cursor < _.src.len && _.src[_.cursor] != quote) {
                if (_.src[_.cursor] == '\\' as u8 && _.cursor + 1 < _.src.len) {
                    _.cursor += 2;
                } else {
                    _.cursor += 1;
                }
            }
            const content_end = _.cursor;
            if (_.cursor < _.src.len) {
                _.cursor += 1; // consume closing quote
            }
            return Token {
                kind: .Word,
                slice: _.src[content_start..content_end],
                pos: start,
            };
        }

        // Standard unquoted words
        while (_.cursor < _.src.len) {
            const ch = _.src[_.cursor];
            if (ch == ' ' as u8 || ch == '\t' as u8 || ch == '\r' as u8 || ch == '\n' as u8 ||
                ch == '|' as u8 || ch == '&' as u8 || ch == ';' as u8 || ch == '>' as u8 || ch == '<' as u8) {
                break;
            }
            _.cursor += 1;
        }

        return Token {
            kind: .Word,
            slice: _.src[start.._.cursor],
            pos: start,
        };
    }
}
