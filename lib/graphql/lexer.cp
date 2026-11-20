// Source: cp/lib/graphql/lexer.cp (GraphQL Oct 2021 Spec Lexer)
// Pure CP GraphQL October 2021 lexical scanner with line/column tracking,
// `#` comments, insignificant commas, single-line escaped strings,
// multi-line block strings (`"""..."""`), spread (`...`), ints, floats, and token caps.

const std = @import("std");

pub const MAX_TOKENS: usize = 65536;

/// Error set emitted by the GraphQL lexer, parser, and printer.
pub err GqlError {
    UnexpectedToken,
    UnterminatedString,
    InvalidNumber,
    MaxDepthExceeded,
    MaxTokensExceeded,
    MaxNodesExceeded,
    MaxFieldsExceeded,
    MaxAliasesExceeded,
    OutOfMemory,
}

/// Lexical token kinds defined by the GraphQL October 2021 specification.
pub enum GqlTokenKind {
    Eof,
    Name,
    Int,
    Float,
    String,
    Bang,
    Dollar,
    Amp,
    LParen,
    RParen,
    Spread,
    Colon,
    Equals,
    At,
    LBracket,
    RBracket,
    LBrace,
    Pipe,
    RBrace,
}

/// A single scanned GraphQL lexical token with 1-based line and column coordinates.
pub struct GqlToken {
    kind: GqlTokenKind = .Eof,
    lexeme: string = "",
    line: u32 = 1,
    col: u32 = 1,
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn hexDigitValue(c: u8): u32 {
    if (c >= '0' && c <= '9') return u32(c - '0');
    if (c >= 'a' && c <= 'f') return u32(c - 'a' + 10);
    return u32(c - 'A' + 10);
}

fn isNameStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isNameContinue(c: u8): bool {
    return isNameStart(c) || isDigit(c);
}

fn isLineWhitespace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn countLeadingWhitespace(s: string): usize {
    let i: usize = 0;
    while (i < s.len && isLineWhitespace(s[i])) : (i += 1) {}
    return i;
}

fn appendUtf8Codepoint(out: *std.ArrayList(u8), alloc: std.mem.Allocator, cp: u32): void ! GqlError {
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return GqlError.UnexpectedToken;
    }
    let buf: [4]u8 = [0; 4];
    const len = std.unicode.utf8Encode(@as(u21, @intCast(cp)), &buf) catch return GqlError.UnexpectedToken;
    out.appendSlice(alloc, buf[0..len]) catch return GqlError.OutOfMemory;
}

/// GraphQL October 2021 zero-copy lexical scanner.
pub struct GqlLexer {
    src: string = "",
    pos: usize = 0,
    line: u32 = 1,
    col: u32 = 1,
    token_count: usize = 0,
    max_tokens: usize = MAX_TOKENS,

    /// Initializes a lexer over `src` with the default `MAX_TOKENS` limit.
    pub static fn init(src: string): GqlLexer {
        return GqlLexer{
            src: src,
            pos: 0,
            line: 1,
            col: 1,
            token_count: 0,
            max_tokens: MAX_TOKENS,
        };
    }

    /// Initializes a lexer over `src` with an explicit token count cap.
    pub static fn initWithLimit(src: string, max_tokens: usize): GqlLexer {
        return GqlLexer{
            src: src,
            pos: 0,
            line: 1,
            col: 1,
            token_count: 0,
            max_tokens: max_tokens,
        };
    }

    fn skipIgnored(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            // Unicode BOM (U+FEFF: EF BB BF)
            if (c == 0xEF && _.pos + 2 < _.src.len && _.src[_.pos + 1] == 0xBB && _.src[_.pos + 2] == 0xBF) {
                _.pos += 3;
                continue;
            }
            // Space, horizontal tab, or insignificant comma
            if (c == ' ' || c == '\t' || c == ',') {
                _.pos += 1;
                _.col += 1;
                continue;
            }
            // Line terminators: \r\n, \r, \n
            if (c == '\r') {
                if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
                    _.pos += 2;
                } else {
                    _.pos += 1;
                }
                _.line += 1;
                _.col = 1;
                continue;
            }
            if (c == '\n') {
                _.pos += 1;
                _.line += 1;
                _.col = 1;
                continue;
            }
            // Comments: '#' until line terminator or EOF
            if (c == '#') {
                _.pos += 1;
                _.col += 1;
                while (_.pos < _.src.len) {
                    const ch = _.src[_.pos];
                    if (ch == '\n' || ch == '\r') {
                        break;
                    }
                    if ((ch & 0xC0) != 0x80) {
                        _.col += 1;
                    }
                    _.pos += 1;
                }
                continue;
            }
            break;
        }
    }

    fn scanSingleLineString(*_, start_pos: usize, start_line: u32, start_col: u32): GqlToken ! GqlError {
        // Consume opening '"'
        _.pos += 1;
        _.col += 1;

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (c == '"') {
                _.pos += 1;
                _.col += 1;
                return GqlToken{
                    kind: .String,
                    lexeme: _.src[start_pos.._.pos],
                    line: start_line,
                    col: start_col,
                };
            }
            if (c == '\n' || c == '\r') {
                return GqlError.UnterminatedString;
            }
            if (c < 0x20 && c != '\t') {
                return GqlError.UnexpectedToken;
            }
            if (c == '\\') {
                _.pos += 1;
                _.col += 1;
                if (_.pos >= _.src.len) {
                    return GqlError.UnterminatedString;
                }
                const esc = _.src[_.pos];
                if (esc == '"' || esc == '\\' || esc == '/' || esc == 'b' || esc == 'f' || esc == 'n' || esc == 'r' || esc == 't') {
                    _.pos += 1;
                    _.col += 1;
                    continue;
                }
                if (esc == 'u') {
                    _.pos += 1;
                    _.col += 1;
                    if (_.pos < _.src.len && _.src[_.pos] == '{') {
                        _.pos += 1;
                        _.col += 1;
                        let hex_count: usize = 0;
                        while (_.pos < _.src.len && _.src[_.pos] != '}') {
                            if (!isHexDigit(_.src[_.pos])) {
                                return GqlError.UnexpectedToken;
                            }
                            _.pos += 1;
                            _.col += 1;
                            hex_count += 1;
                        }
                        if (_.pos >= _.src.len) {
                            return GqlError.UnterminatedString;
                        }
                        if (hex_count == 0 || hex_count > 6) {
                            return GqlError.UnexpectedToken;
                        }
                        _.pos += 1;
                        _.col += 1;
                        continue;
                    }
                    let i: usize = 0;
                    while (i < 4) : (i += 1) {
                        if (_.pos >= _.src.len) {
                            return GqlError.UnterminatedString;
                        }
                        if (!isHexDigit(_.src[_.pos])) {
                            return GqlError.UnexpectedToken;
                        }
                        _.pos += 1;
                        _.col += 1;
                    }
                    continue;
                }
                return GqlError.UnexpectedToken;
            }
            if ((c & 0xC0) != 0x80) {
                _.col += 1;
            }
            _.pos += 1;
        }
        return GqlError.UnterminatedString;
    }

    fn scanBlockString(*_, start_pos: usize, start_line: u32, start_col: u32): GqlToken ! GqlError {
        // Consume opening '"""'
        _.pos += 3;
        _.col += 3;

        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            // Escaped triple quote \"""
            if (c == '\\' && _.pos + 3 < _.src.len && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"' && _.src[_.pos + 3] == '"') {
                _.pos += 4;
                _.col += 4;
                continue;
            }
            // Closing triple quote """
            if (c == '"' && _.pos + 2 < _.src.len && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"') {
                _.pos += 3;
                _.col += 3;
                return GqlToken{
                    kind: .String,
                    lexeme: _.src[start_pos.._.pos],
                    line: start_line,
                    col: start_col,
                };
            }
            if (c == '\r') {
                if (_.pos + 1 < _.src.len && _.src[_.pos + 1] == '\n') {
                    _.pos += 2;
                } else {
                    _.pos += 1;
                }
                _.line += 1;
                _.col = 1;
                continue;
            }
            if (c == '\n') {
                _.pos += 1;
                _.line += 1;
                _.col = 1;
                continue;
            }
            if ((c & 0xC0) != 0x80) {
                _.col += 1;
            }
            _.pos += 1;
        }
        return GqlError.UnterminatedString;
    }

    fn scanNumber(*_, start_pos: usize, start_line: u32, start_col: u32): GqlToken ! GqlError {
        let is_float = false;

        if (_.src[_.pos] == '-') {
            _.pos += 1;
            _.col += 1;
        }

        if (_.pos >= _.src.len || !isDigit(_.src[_.pos])) {
            return GqlError.InvalidNumber;
        }

        if (_.src[_.pos] == '0') {
            _.pos += 1;
            _.col += 1;
            if (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                return GqlError.InvalidNumber;
            }
        } else {
            while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                _.pos += 1;
                _.col += 1;
            }
        }

        if (_.pos < _.src.len && _.src[_.pos] == '.') {
            is_float = true;
            _.pos += 1;
            _.col += 1;
            if (_.pos >= _.src.len || !isDigit(_.src[_.pos])) {
                return GqlError.InvalidNumber;
            }
            while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                _.pos += 1;
                _.col += 1;
            }
        }

        if (_.pos < _.src.len && (_.src[_.pos] == 'e' || _.src[_.pos] == 'E')) {
            is_float = true;
            _.pos += 1;
            _.col += 1;
            if (_.pos < _.src.len && (_.src[_.pos] == '+' || _.src[_.pos] == '-')) {
                _.pos += 1;
                _.col += 1;
            }
            if (_.pos >= _.src.len || !isDigit(_.src[_.pos])) {
                return GqlError.InvalidNumber;
            }
            while (_.pos < _.src.len && isDigit(_.src[_.pos])) {
                _.pos += 1;
                _.col += 1;
            }
        }

        // Spec lookahead restriction: IntValue/FloatValue cannot be followed by NameStart or '.'
        if (_.pos < _.src.len && (isNameStart(_.src[_.pos]) || _.src[_.pos] == '.')) {
            return GqlError.InvalidNumber;
        }

        return GqlToken{
            kind: if (is_float) .Float else .Int,
            lexeme: _.src[start_pos.._.pos],
            line: start_line,
            col: start_col,
        };
    }

    /// Scans and returns the next lexical token (or `.Eof` at end of input).
    pub fn nextToken(*_): GqlToken ! GqlError {
        _.skipIgnored();

        if (_.pos >= _.src.len) {
            return GqlToken{
                kind: .Eof,
                lexeme: "",
                line: _.line,
                col: _.col,
            };
        }

        if (_.token_count >= _.max_tokens) {
            return GqlError.MaxTokensExceeded;
        }
        _.token_count += 1;

        const start_pos = _.pos;
        const start_line = _.line;
        const start_col = _.col;
        const c = _.src[_.pos];

        switch (c) {
            '!' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .Bang, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '$' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .Dollar, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '&' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .Amp, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '(' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .LParen, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            ')' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .RParen, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            ':' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .Colon, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '=' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .Equals, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '@' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .At, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '[' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .LBracket, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            ']' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .RBracket, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '{' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .LBrace, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '|' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .Pipe, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '}' => {
                _.pos += 1;
                _.col += 1;
                return GqlToken{ kind: .RBrace, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
            },
            '.' => {
                if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '.' && _.src[_.pos + 2] == '.') {
                    _.pos += 3;
                    _.col += 3;
                    return GqlToken{ kind: .Spread, lexeme: _.src[start_pos.._.pos], line: start_line, col: start_col };
                }
                return GqlError.UnexpectedToken;
            },
            '"' => {
                if (_.pos + 2 < _.src.len && _.src[_.pos + 1] == '"' && _.src[_.pos + 2] == '"') {
                    return _.scanBlockString(start_pos, start_line, start_col);
                }
                return _.scanSingleLineString(start_pos, start_line, start_col);
            },
            '-' | '0'...'9' => {
                return _.scanNumber(start_pos, start_line, start_col);
            },
            else => {
                if (isNameStart(c)) {
                    _.pos += 1;
                    _.col += 1;
                    while (_.pos < _.src.len && isNameContinue(_.src[_.pos])) {
                        _.pos += 1;
                        _.col += 1;
                    }
                    return GqlToken{
                        kind: .Name,
                        lexeme: _.src[start_pos.._.pos],
                        line: start_line,
                        col: start_col,
                    };
                }
                return GqlError.UnexpectedToken;
            },
        }
    }

    /// Scans all tokens up to and including `.Eof`. Caller owns the returned slice.
    pub fn tokenize(*_, alloc: std.mem.Allocator): ([]GqlToken) ! GqlError {
        let list: std.ArrayList(GqlToken) = .empty;
        errdefer list.deinit(alloc);

        while (true) {
            const tok = _.nextToken()?;
            list.append(alloc, tok) catch return GqlError.OutOfMemory;
            if (tok.kind == .Eof) {
                break;
            }
        }
        return list.toOwnedSlice(alloc) catch return GqlError.OutOfMemory;
    }

    static fn decodeSingleLineString(alloc: std.mem.Allocator, raw: string): ([]u8) ! GqlError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let i: usize = 0;
        while (i < raw.len) {
            const c = raw[i];
            if (c != '\\') {
                out.append(alloc, c) catch return GqlError.OutOfMemory;
                i += 1;
                continue;
            }
            i += 1;
            if (i >= raw.len) return GqlError.UnterminatedString;
            const esc = raw[i];
            i += 1;
            switch (esc) {
                '"' => out.append(alloc, '"') catch return GqlError.OutOfMemory,
                '\\' => out.append(alloc, '\\') catch return GqlError.OutOfMemory,
                '/' => out.append(alloc, '/') catch return GqlError.OutOfMemory,
                'b' => out.append(alloc, 0x08) catch return GqlError.OutOfMemory,
                'f' => out.append(alloc, 0x0C) catch return GqlError.OutOfMemory,
                'n' => out.append(alloc, '\n') catch return GqlError.OutOfMemory,
                'r' => out.append(alloc, '\r') catch return GqlError.OutOfMemory,
                't' => out.append(alloc, '\t') catch return GqlError.OutOfMemory,
                'u' => {
                    if (i < raw.len && raw[i] == '{') {
                        i += 1;
                        let cp: u32 = 0;
                        let digits: usize = 0;
                        while (i < raw.len && raw[i] != '}') : (i += 1) {
                            if (!isHexDigit(raw[i])) return GqlError.UnexpectedToken;
                            cp = (cp << 4) | hexDigitValue(raw[i]);
                            digits += 1;
                        }
                        if (i >= raw.len || digits == 0 || digits > 6) return GqlError.UnexpectedToken;
                        i += 1;
                        appendUtf8Codepoint(&out, alloc, cp)?;
                    } else {
                        if (i + 4 > raw.len) return GqlError.UnterminatedString;
                        let cp: u32 = 0;
                        let k: usize = 0;
                        while (k < 4) : (k += 1) {
                            if (!isHexDigit(raw[i + k])) return GqlError.UnexpectedToken;
                            cp = (cp << 4) | hexDigitValue(raw[i + k]);
                        }
                        i += 4;
                        // Handle UTF-16 surrogate pairs
                        if (cp >= 0xD800 && cp <= 0xDBFF) {
                            if (i + 6 <= raw.len && raw[i] == '\\' && raw[i + 1] == 'u') {
                                let low: u32 = 0;
                                let m: usize = 0;
                                while (m < 4) : (m += 1) {
                                    if (!isHexDigit(raw[i + 2 + m])) return GqlError.UnexpectedToken;
                                    low = (low << 4) | hexDigitValue(raw[i + 2 + m]);
                                }
                                if (low < 0xDC00 || low > 0xDFFF) return GqlError.UnexpectedToken;
                                i += 6;
                                const combined = 0x10000 + (((cp - 0xD800) << 10) | (low - 0xDC00));
                                appendUtf8Codepoint(&out, alloc, combined)?;
                            } else {
                                return GqlError.UnexpectedToken;
                            }
                        } else {
                            appendUtf8Codepoint(&out, alloc, cp)?;
                        }
                    }
                },
                else => return GqlError.UnexpectedToken,
            }
        }
        return out.toOwnedSlice(alloc) catch return GqlError.OutOfMemory;
    }

    static fn decodeBlockString(alloc: std.mem.Allocator, raw: string): ([]u8) ! GqlError {
        // 1. Replace \""" with """
        let unescaped: std.ArrayList(u8) = .empty;
        defer unescaped.deinit(alloc);

        let i: usize = 0;
        while (i < raw.len) {
            if (raw[i] == '\\' && i + 3 < raw.len && raw[i + 1] == '"' && raw[i + 2] == '"' && raw[i + 3] == '"') {
                unescaped.appendSlice(alloc, "\"\"\"") catch return GqlError.OutOfMemory;
                i += 4;
            } else {
                unescaped.append(alloc, raw[i]) catch return GqlError.OutOfMemory;
                i += 1;
            }
        }

        // 2. Split into lines by \r\n, \r, or \n
        let lines: std.ArrayList(string) = .empty;
        defer lines.deinit(alloc);

        const text = unescaped.items;
        let line_start: usize = 0;
        let scan_idx: usize = 0;
        while (scan_idx < text.len) {
            if (text[scan_idx] == '\r') {
                lines.append(alloc, text[line_start..scan_idx]) catch return GqlError.OutOfMemory;
                if (scan_idx + 1 < text.len && text[scan_idx + 1] == '\n') {
                    scan_idx += 2;
                } else {
                    scan_idx += 1;
                }
                line_start = scan_idx;
            } else if (text[scan_idx] == '\n') {
                lines.append(alloc, text[line_start..scan_idx]) catch return GqlError.OutOfMemory;
                scan_idx += 1;
                line_start = scan_idx;
            } else {
                scan_idx += 1;
            }
        }
        lines.append(alloc, text[line_start..text.len]) catch return GqlError.OutOfMemory;

        // 3. Compute commonIndent over lines[1..]
        let has_common_indent = false;
        let common_indent: usize = 0;
        let li: usize = 1;
        while (li < lines.items.len) : (li += 1) {
            const ln = lines.items[li];
            const indent = countLeadingWhitespace(ln);
            if (indent < ln.len) {
                if (!has_common_indent || indent < common_indent) {
                    common_indent = indent;
                    has_common_indent = true;
                }
            }
        }

        // 4. Strip commonIndent from lines[1..]
        if (has_common_indent && common_indent > 0) {
            li = 1;
            while (li < lines.items.len) : (li += 1) {
                const ln = lines.items[li];
                if (ln.len >= common_indent) {
                    lines.items[li] = ln[common_indent..];
                } else {
                    lines.items[li] = "";
                }
            }
        }

        // 5. Trim leading and trailing whitespace-only lines
        let first_idx: usize = 0;
        while (first_idx < lines.items.len) {
            const ln = lines.items[first_idx];
            if (countLeadingWhitespace(ln) == ln.len) {
                first_idx += 1;
            } else {
                break;
            }
        }

        let end_idx: usize = lines.items.len;
        while (end_idx > first_idx) {
            const ln = lines.items[end_idx - 1];
            if (countLeadingWhitespace(ln) == ln.len) {
                end_idx -= 1;
            } else {
                break;
            }
        }

        // 6. Join remaining lines with '\n'
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);
        let idx = first_idx;
        while (idx < end_idx) : (idx += 1) {
            if (idx > first_idx) {
                out.append(alloc, '\n') catch return GqlError.OutOfMemory;
            }
            out.appendSlice(alloc, lines.items[idx]) catch return GqlError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return GqlError.OutOfMemory;
    }

    /// Decodes a `GqlToken` of kind `.String` (either `"..."` or `"""..."""`).
    /// Caller owns the returned slice.
    pub static fn decodeString(alloc: std.mem.Allocator, lexeme: string): ([]u8) ! GqlError {
        if (lexeme.len >= 6 &&
            lexeme[0] == '"' && lexeme[1] == '"' && lexeme[2] == '"' &&
            lexeme[lexeme.len - 3] == '"' && lexeme[lexeme.len - 2] == '"' && lexeme[lexeme.len - 1] == '"')
        {
            return GqlLexer.decodeBlockString(alloc, lexeme[3..(lexeme.len - 3)]);
        }
        if (lexeme.len >= 2 && lexeme[0] == '"' && lexeme[lexeme.len - 1] == '"') {
            return GqlLexer.decodeSingleLineString(alloc, lexeme[1..(lexeme.len - 1)]);
        }
        return GqlError.UnexpectedToken;
    }
}
