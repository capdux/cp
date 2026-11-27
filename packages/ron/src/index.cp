// @cap/ron — Zero-heap Rusty Object Notation (RON) lexer & top-level field extractor

pub enum RonTokenKind {
    Ident,
    String,
    Int,
    Float,
    Bool,
    LParen,
    RParen,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    Colon,
    Comma,
    Eof,
    Invalid,
}

pub struct RonToken {
    kind: RonTokenKind,
    text: string,
    start: usize,
    end: usize,
}

fn ronpIsWs(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn ronpIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn ronpIsHexDigit(c: u8): bool {
    return (c >= 48 && c <= 57) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102);
}

fn ronpIsIdentStart(c: u8): bool {
    return (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95;
}

fn ronpIsIdentCont(c: u8): bool {
    return ronpIsIdentStart(c) || ronpIsDigit(c);
}

fn ronpEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn ronpSkipTrivia(src: string, start_pos: usize): usize {
    let i: usize = start_pos;
    const n: usize = src.len;
    while (i < n) {
        const c: u8 = src[i];
        if (ronpIsWs(c)) {
            i += 1;
            continue;
        }
        // Line comment: // ...
        if (c == 47 && i + 1 < n && src[i + 1] == 47) {
            i += 2;
            while (i < n && src[i] != 10) : (i += 1) {}
            continue;
        }
        // Block comment: /* ... */ (supports nesting)
        if (c == 47 && i + 1 < n && src[i + 1] == 42) {
            i += 2;
            let depth: usize = 1;
            while (i < n && depth > 0) {
                if (src[i] == 47 && i + 1 < n && src[i + 1] == 42) {
                    depth += 1;
                    i += 2;
                } else if (src[i] == 42 && i + 1 < n && src[i + 1] == 47) {
                    depth -= 1;
                    i += 2;
                } else {
                    i += 1;
                }
            }
            continue;
        }
        break;
    }
    return i;
}

pub struct RonLexer {
    src: string,
    pos: usize,

    static fn new(src: string): RonLexer {
        return RonLexer{ src: src, pos: 0 };
    }

    fn nextToken(*_): RonToken {
        const s: string = _.src;
        const n: usize = s.len;
        let i: usize = ronpSkipTrivia(s, _.pos);
        if (i >= n) {
            _.pos = n;
            return RonToken{
                kind: RonTokenKind.Eof,
                text: s[n..n],
                start: n,
                end: n,
            };
        }

        const start: usize = i;
        const c: u8 = s[i];

        // Single-character punctuation
        if (c == 40) {
            _.pos = i + 1;
            return RonToken{ kind: RonTokenKind.LParen, text: s[start..i + 1], start: start, end: i + 1 };
        }
        if (c == 41) {
            _.pos = i + 1;
            return RonToken{ kind: RonTokenKind.RParen, text: s[start..i + 1], start: start, end: i + 1 };
        }
        if (c == 91) {
            _.pos = i + 1;
            return RonToken{ kind: RonTokenKind.LBracket, text: s[start..i + 1], start: start, end: i + 1 };
        }
        if (c == 93) {
            _.pos = i + 1;
            return RonToken{ kind: RonTokenKind.RBracket, text: s[start..i + 1], start: start, end: i + 1 };
        }
        if (c == 123) {
            _.pos = i + 1;
            return RonToken{ kind: RonTokenKind.LBrace, text: s[start..i + 1], start: start, end: i + 1 };
        }
        if (c == 125) {
            _.pos = i + 1;
            return RonToken{ kind: RonTokenKind.RBrace, text: s[start..i + 1], start: start, end: i + 1 };
        }
        if (c == 58) {
            _.pos = i + 1;
            return RonToken{ kind: RonTokenKind.Colon, text: s[start..i + 1], start: start, end: i + 1 };
        }
        if (c == 44) {
            _.pos = i + 1;
            return RonToken{ kind: RonTokenKind.Comma, text: s[start..i + 1], start: start, end: i + 1 };
        }

        // Double-quoted string literal with backslash escapes
        if (c == 34) {
            i += 1;
            let closed: bool = false;
            while (i < n) {
                const ch: u8 = s[i];
                if (ch == 92) {
                    if (i + 1 < n) {
                        i += 2;
                    } else {
                        i += 1;
                    }
                    continue;
                }
                if (ch == 34) {
                    i += 1;
                    closed = true;
                    break;
                }
                i += 1;
            }
            _.pos = i;
            if (closed) {
                return RonToken{ kind: RonTokenKind.String, text: s[start..i], start: start, end: i };
            }
            return RonToken{ kind: RonTokenKind.Invalid, text: s[start..i], start: start, end: i };
        }

        // Number: optional sign followed by digit
        const is_signed_num: bool = (c == 45 || c == 43) && (i + 1 < n) && ronpIsDigit(s[i + 1]);
        if (ronpIsDigit(c) || is_signed_num) {
            if (c == 45 || c == 43) {
                i += 1;
            }
            // Check 0x / 0b / 0o integer prefixes
            if (i + 1 < n && s[i] == 48 && (s[i + 1] == 120 || s[i + 1] == 88 || s[i + 1] == 98 || s[i + 1] == 66 || s[i + 1] == 111 || s[i + 1] == 79)) {
                i += 2;
                while (i < n && (ronpIsHexDigit(s[i]) || s[i] == 95)) : (i += 1) {}
                _.pos = i;
                return RonToken{ kind: RonTokenKind.Int, text: s[start..i], start: start, end: i };
            }
            let is_float: bool = false;
            while (i < n && (ronpIsDigit(s[i]) || s[i] == 95)) : (i += 1) {}
            if (i < n && s[i] == 46 && (i + 1 >= n || s[i + 1] != 46)) {
                is_float = true;
                i += 1;
                while (i < n && (ronpIsDigit(s[i]) || s[i] == 95)) : (i += 1) {}
            }
            if (i < n && (s[i] == 101 || s[i] == 69)) {
                is_float = true;
                i += 1;
                if (i < n && (s[i] == 43 || s[i] == 45)) {
                    i += 1;
                }
                while (i < n && (ronpIsDigit(s[i]) || s[i] == 95)) : (i += 1) {}
            }
            _.pos = i;
            if (is_float) {
                return RonToken{ kind: RonTokenKind.Float, text: s[start..i], start: start, end: i };
            }
            return RonToken{ kind: RonTokenKind.Int, text: s[start..i], start: start, end: i };
        }

        // Identifier or boolean keyword
        if (ronpIsIdentStart(c)) {
            i += 1;
            while (i < n && ronpIsIdentCont(s[i])) : (i += 1) {}
            _.pos = i;
            const tok_text: string = s[start..i];
            if (ronpEq(tok_text, "true") || ronpEq(tok_text, "false")) {
                return RonToken{ kind: RonTokenKind.Bool, text: tok_text, start: start, end: i };
            }
            return RonToken{ kind: RonTokenKind.Ident, text: tok_text, start: start, end: i };
        }

        // Unknown character
        _.pos = i + 1;
        return RonToken{ kind: RonTokenKind.Invalid, text: s[start..i + 1], start: start, end: i + 1 };
    }
}

fn ronpIsOpenDelim(k: RonTokenKind): bool {
    return k == RonTokenKind.LParen || k == RonTokenKind.LBracket || k == RonTokenKind.LBrace;
}

fn ronpIsCloseDelim(k: RonTokenKind): bool {
    return k == RonTokenKind.RParen || k == RonTokenKind.RBracket || k == RonTokenKind.RBrace;
}

pub fn ronGetField(ron_src: string, field_name: string): string? {
    let lex = RonLexer.new(ron_src);
    let first = lex.nextToken();
    // Allow optional leading struct name, e.g. Config(a: 1, b: "hi")
    if (first.kind == RonTokenKind.Ident) {
        first = lex.nextToken();
    }
    if (first.kind != RonTokenKind.LParen) {
        return null;
    }

    while (true) {
        const key_tok = lex.nextToken();
        if (key_tok.kind == RonTokenKind.RParen || key_tok.kind == RonTokenKind.Eof) {
            return null;
        }
        if (key_tok.kind == RonTokenKind.Comma) {
            continue;
        }
        if (key_tok.kind != RonTokenKind.Ident) {
            return null;
        }
        const colon_tok = lex.nextToken();
        if (colon_tok.kind != RonTokenKind.Colon) {
            return null;
        }

        const matched: bool = ronpEq(key_tok.text, field_name);
        let depth: usize = 0;
        let has_val: bool = false;
        let val_start: usize = 0;
        let val_end: usize = 0;

        while (true) {
            const tok = lex.nextToken();
            if (tok.kind == RonTokenKind.Eof) {
                if (matched && has_val) {
                    return ron_src[val_start..val_end];
                }
                return null;
            }
            if (depth == 0 && (tok.kind == RonTokenKind.Comma || tok.kind == RonTokenKind.RParen)) {
                if (matched) {
                    if (has_val) {
                        return ron_src[val_start..val_end];
                    }
                    return null;
                }
                if (tok.kind == RonTokenKind.RParen) {
                    return null;
                }
                break;
            }
            if (!has_val) {
                val_start = tok.start;
                has_val = true;
            }
            val_end = tok.end;
            if (ronpIsOpenDelim(tok.kind)) {
                depth += 1;
            } else if (ronpIsCloseDelim(tok.kind) && depth > 0) {
                depth -= 1;
            }
        }
    }
    return null;
}
