// Source: Absorbed from /data/dev/bun/src/css/css_parser.cp | Synced: 2026-11-22T10:52:02Z
// lib/css/tokenizer.cp
// CSS Syntax Level 3 Tokenizer (Zero-Heap Allocation)
// Absorbed from Bun src/css/css_parser.cp Tokenizer

const std = @import("std");

pub enum CssTokenKind {
    Ident,
    Function,
    AtKeyword,
    Hash,
    String,
    BadString,
    Url,
    Number,
    Percentage,
    Dimension,
    Whitespace,
    Comment,
    Colon,
    Semicolon,
    Comma,
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    Delim,
    Cdo,
    Cdc,
    Eof,
}

pub struct CssToken {
    kind: CssTokenKind = CssTokenKind.Eof;
    lexeme: []const u8 = "";
    value: []const u8 = "";
    num_val: f64 = 0.0;
    unit: []const u8 = "";
    is_integer: bool = true;
    is_id_hash: bool = false;
    line: u32 = 1;
    col: u32 = 1;
    start: usize = 0;
    end: usize = 0;

    pub fn isTrivia(): bool {
        return _.kind == CssTokenKind.Whitespace || _.kind == CssTokenKind.Comment;
    }

    pub fn eqlIdent(target: []const u8): bool {
        if (_.kind != CssTokenKind.Ident && _.kind != CssTokenKind.Function && _.kind != CssTokenKind.AtKeyword) {
            return false;
        }
        return std.ascii.eqlIgnoreCase(_.value, target);
    }
}

fn isWhitespaceByte(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\x0c';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isNameStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c >= 0x80;
}

fn isNameChar(c: u8): bool {
    return isNameStart(c) || isDigit(c) || c == '-';
}

pub struct CssTokenizer {
    src: []const u8 = "";
    pos: usize = 0;
    line: u32 = 1;
    col: u32 = 1;

    pub static fn init(src: []const u8): CssTokenizer {
        return CssTokenizer { src: src, pos: 0, line: 1, col: 1 };
    }

    pub fn isEof(): bool {
        return _.pos >= _.src.len;
    }

    fn peekAt(offset: usize): u8 {
        if (_.pos + offset >= _.src.len) {
            return 0;
        }
        return _.src[_.pos + offset];
    }

    fn advance(*_, count: usize) {
        let i: usize = 0;
        while (i < count && _.pos < _.src.len) {
            if (_.src[_.pos] == '\n') {
                _.line += 1;
                _.col = 1;
            } else {
                _.col += 1;
            }
            _.pos += 1;
            i += 1;
        }
    }

    fn startsIdentAt(offset: usize): bool {
        const c0 = _.peekAt(offset);
        if (c0 == '-') {
            const c1 = _.peekAt(offset + 1);
            return isNameStart(c1) || c1 == '-' || c1 == '\\';
        }
        if (c0 == '\\') {
            return _.peekAt(offset + 1) != '\n' && _.peekAt(offset + 1) != 0;
        }
        return isNameStart(c0);
    }

    fn startsNumberAt(offset: usize): bool {
        const c0 = _.peekAt(offset);
        if (isDigit(c0)) {
            return true;
        }
        if (c0 == '.') {
            return isDigit(_.peekAt(offset + 1));
        }
        if (c0 == '+' || c0 == '-') {
            const c1 = _.peekAt(offset + 1);
            if (isDigit(c1)) {
                return true;
            }
            if (c1 == '.' && isDigit(_.peekAt(offset + 2))) {
                return true;
            }
        }
        return false;
    }

    fn consumeName(*_): []const u8 {
        const start = _.pos;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (isNameChar(c)) {
                _.advance(1);
            } else if (c == '\\' && _.pos + 1 < _.src.len && _.src[_.pos + 1] != '\n') {
                _.advance(2);
            } else {
                break;
            }
        }
        return _.src[start.._.pos];
    }

    fn consumeNumeric(*_, tok_start: usize, tok_line: u32, tok_col: u32): CssToken {
        let is_int = true;
        const num_start = _.pos;

        if (_.peekAt(0) == '+' || _.peekAt(0) == '-') {
            _.advance(1);
        }
        while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
            _.advance(1);
        }
        if (_.peekAt(0) == '.' && isDigit(_.peekAt(1))) {
            is_int = false;
            _.advance(1);
            while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                _.advance(1);
            }
        }
        if ((_.peekAt(0) == 'e' || _.peekAt(0) == 'E')) {
            const e1 = _.peekAt(1);
            const e2 = _.peekAt(2);
            if (isDigit(e1) || ((e1 == '+' || e1 == '-') && isDigit(e2))) {
                is_int = false;
                _.advance(1);
                if (_.peekAt(0) == '+' || _.peekAt(0) == '-') {
                    _.advance(1);
                }
                while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                    _.advance(1);
                }
            }
        }

        const num_slice = _.src[num_start.._.pos];
        const parsed = std.fmt.parseFloat(f64, num_slice) catch 0.0;

        if (_.startsIdentAt(0)) {
            const unit_slice = _.consumeName();
            return CssToken {
                kind: CssTokenKind.Dimension,
                lexeme: _.src[tok_start.._.pos],
                value: num_slice,
                num_val: parsed,
                unit: unit_slice,
                is_integer: is_int,
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        if (_.peekAt(0) == '%') {
            _.advance(1);
            return CssToken {
                kind: CssTokenKind.Percentage,
                lexeme: _.src[tok_start.._.pos],
                value: num_slice,
                num_val: parsed,
                unit: "%",
                is_integer: is_int,
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        return CssToken {
            kind: CssTokenKind.Number,
            lexeme: _.src[tok_start.._.pos],
            value: num_slice,
            num_val: parsed,
            is_integer: is_int,
            line: tok_line,
            col: tok_col,
            start: tok_start,
            end: _.pos,
        };
    }

    fn consumeString(*_, quote: u8, tok_start: usize, tok_line: u32, tok_col: u32): CssToken {
        _.advance(1); // opening quote
        const val_start = _.pos;
        let bad = false;
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == quote) {
                const content = _.src[val_start.._.pos];
                _.advance(1);
                return CssToken {
                    kind: CssTokenKind.String,
                    lexeme: _.src[tok_start.._.pos],
                    value: content,
                    line: tok_line,
                    col: tok_col,
                    start: tok_start,
                    end: _.pos,
                };
            }
            if (c == '\n' || c == '\r') {
                bad = true;
                break;
            }
            if (c == '\\') {
                if (_.pos + 1 < _.src.len) {
                    _.advance(2);
                    continue;
                }
            }
            _.advance(1);
        }
        return CssToken {
            kind: if (bad) CssTokenKind.BadString else CssTokenKind.String,
            lexeme: _.src[tok_start.._.pos],
            value: _.src[val_start.._.pos],
            line: tok_line,
            col: tok_col,
            start: tok_start,
            end: _.pos,
        };
    }

    fn consumeIdentLike(*_, tok_start: usize, tok_line: u32, tok_col: u32): CssToken {
        const name = _.consumeName();
        if (_.peekAt(0) == '(') {
            if (std.ascii.eqlIgnoreCase(name, "url")) {
                // Check for unquoted url(...) vs quoted url("...")
                let look: usize = 1;
                while (_.pos + look < _.src.len && isWhitespaceByte(_.src[_.pos + look])) {
                    look += 1;
                }
                const next_ch = _.peekAt(look);
                if (next_ch != '"' && next_ch != '\'' && next_ch != 0) {
                    _.advance(look);
                    const url_start = _.pos;
                    while (_.pos < _.src.len && _.src[_.pos] != ')' && !isWhitespaceByte(_.src[_.pos])) {
                        _.advance(1);
                    }
                    const url_val = _.src[url_start.._.pos];
                    while (_.pos < _.src.len && isWhitespaceByte(_.src[_.pos])) {
                        _.advance(1);
                    }
                    if (_.peekAt(0) == ')') {
                        _.advance(1);
                    }
                    return CssToken {
                        kind: CssTokenKind.Url,
                        lexeme: _.src[tok_start.._.pos],
                        value: url_val,
                        line: tok_line,
                        col: tok_col,
                        start: tok_start,
                        end: _.pos,
                    };
                }
            }
            _.advance(1); // consume '('
            return CssToken {
                kind: CssTokenKind.Function,
                lexeme: _.src[tok_start.._.pos],
                value: name,
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        return CssToken {
            kind: CssTokenKind.Ident,
            lexeme: _.src[tok_start.._.pos],
            value: name,
            line: tok_line,
            col: tok_col,
            start: tok_start,
            end: _.pos,
        };
    }

    pub fn nextToken(*_): CssToken {
        if (_.pos >= _.src.len) {
            return CssToken {
                kind: CssTokenKind.Eof,
                line: _.line,
                col: _.col,
                start: _.pos,
                end: _.pos,
            };
        }

        const tok_start = _.pos;
        const tok_line = _.line;
        const tok_col = _.col;
        const c = _.src[_.pos];

        // Comments: /* ... */
        if (c == '/' && _.peekAt(1) == '*') {
            _.advance(2);
            const body_start = _.pos;
            while (_.pos + 1 < _.src.len && !(_.src[_.pos] == '*' && _.src[_.pos + 1] == '/')) {
                _.advance(1);
            }
            const body_end = _.pos;
            if (_.pos + 1 < _.src.len) {
                _.advance(2);
            } else if (_.pos < _.src.len) {
                _.advance(1);
            }
            return CssToken {
                kind: CssTokenKind.Comment,
                lexeme: _.src[tok_start.._.pos],
                value: _.src[body_start..body_end],
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        // Whitespace
        if (isWhitespaceByte(c)) {
            while (_.pos < _.src.len && isWhitespaceByte(_.src[_.pos])) {
                _.advance(1);
            }
            return CssToken {
                kind: CssTokenKind.Whitespace,
                lexeme: _.src[tok_start.._.pos],
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        // Strings
        if (c == '"' || c == '\'') {
            return _.consumeString(c, tok_start, tok_line, tok_col);
        }

        // Hash (#name)
        if (c == '#') {
            _.advance(1);
            const next_ch = _.peekAt(0);
            if (isNameChar(next_ch) || (next_ch == '\\' && _.peekAt(1) != '\n')) {
                const is_id = _.startsIdentAt(0);
                const name = _.consumeName();
                return CssToken {
                    kind: CssTokenKind.Hash,
                    lexeme: _.src[tok_start.._.pos],
                    value: name,
                    is_id_hash: is_id,
                    line: tok_line,
                    col: tok_col,
                    start: tok_start,
                    end: _.pos,
                };
            }
            return CssToken {
                kind: CssTokenKind.Delim,
                lexeme: _.src[tok_start.._.pos],
                value: "#",
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        // At-keyword (@name)
        if (c == '@') {
            if (_.startsIdentAt(1)) {
                _.advance(1);
                const name = _.consumeName();
                return CssToken {
                    kind: CssTokenKind.AtKeyword,
                    lexeme: _.src[tok_start.._.pos],
                    value: name,
                    line: tok_line,
                    col: tok_col,
                    start: tok_start,
                    end: _.pos,
                };
            }
            _.advance(1);
            return CssToken {
                kind: CssTokenKind.Delim,
                lexeme: "@",
                value: "@",
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        // CDO <!--
        if (c == '<' && _.peekAt(1) == '!' && _.peekAt(2) == '-' && _.peekAt(3) == '-') {
            _.advance(4);
            return CssToken {
                kind: CssTokenKind.Cdo,
                lexeme: "<!--",
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        // CDC -->
        if (c == '-' && _.peekAt(1) == '-' && _.peekAt(2) == '>') {
            _.advance(3);
            return CssToken {
                kind: CssTokenKind.Cdc,
                lexeme: "-->",
                line: tok_line,
                col: tok_col,
                start: tok_start,
                end: _.pos,
            };
        }

        // Numbers / Dimensions / Percentages
        if (_.startsNumberAt(0)) {
            return _.consumeNumeric(tok_start, tok_line, tok_col);
        }

        // Identifiers / Functions / Urls
        if (_.startsIdentAt(0)) {
            return _.consumeIdentLike(tok_start, tok_line, tok_col);
        }

        // Single-character punctuation tokens
        _.advance(1);
        const kind = switch (c) {
            ':' => CssTokenKind.Colon,
            ';' => CssTokenKind.Semicolon,
            ',' => CssTokenKind.Comma,
            '(' => CssTokenKind.LeftParen,
            ')' => CssTokenKind.RightParen,
            '[' => CssTokenKind.LeftBracket,
            ']' => CssTokenKind.RightBracket,
            '{' => CssTokenKind.LeftBrace,
            '}' => CssTokenKind.RightBrace,
            else => CssTokenKind.Delim,
        };

        return CssToken {
            kind: kind,
            lexeme: _.src[tok_start.._.pos],
            value: _.src[tok_start.._.pos],
            line: tok_line,
            col: tok_col,
            start: tok_start,
            end: _.pos,
        };
    }

    pub fn nextNonTrivia(*_): CssToken {
        while (true) {
            const tok = _.nextToken();
            if (tok.kind == CssTokenKind.Eof || !tok.isTrivia()) {
                return tok;
            }
        }
    }

    pub fn tokenizeAll(*_, out_tokens: []CssToken, skip_trivia: bool): usize {
        let count: usize = 0;
        while (count < out_tokens.len) {
            const tok = if (skip_trivia) _.nextNonTrivia() else _.nextToken();
            if (tok.kind == CssTokenKind.Eof) {
                break;
            }
            out_tokens[count] = tok;
            count += 1;
        }
        return count;
    }
}
