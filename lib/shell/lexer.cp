// Source: Absorbed from /data/dev/bun/src/shell/shell.cp | Synced: 2026-11-22T07:42:24Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/lexer.cp
// Full POSIX lexical analyzer for CP Shell (cap:shell)
// Supports single/double/ANSI-C quotes, escapes, ${VAR:-default}, $(cmd), $((arith)),
// `backticks`, comments (#), pipelines (|), negation (!), background (&),
// subshells (( / )), groups ({ / }), logical chains (&& / ||), sequence (;/\n),
// and full redirections (>, >>, <, <<<, <<, 2>, 2>>, 2>&1, 1>&2, &>, &>>).

pub enum QuoteKind: u8 {
    None = 0,
    Single = 1,
    Double = 2,
    Mixed = 3,
}

pub enum TokenKind {
    Eof,
    Word,
    Pipe,              // |
    And,               // &&
    Or,                // ||
    Semicolon,         // ;
    Newline,           // \n
    Ampersand,         // &
    Bang,              // !
    OpenParen,         // (
    CloseParen,        // )
    RedirectOut,       // > or 1>
    RedirectAppend,    // >> or 1>>
    RedirectIn,        // < or 0<
    HereString,        // <<<
    HereDoc,           // <<
    RedirectErr,       // 2>
    RedirectErrAppend, // 2>>
    RedirectErrToOut,  // 2>&1
    RedirectOutToErr,  // 1>&2 or >&2
    RedirectAll,       // &>
    RedirectAllAppend, // &>>
}

pub struct Token {
    kind: TokenKind = .Eof,
    slice: []const u8 = "",
    raw_slice: []const u8 = "",
    quote: QuoteKind = .None,
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
            if (c == ' ' || c == '\t' || c == '\r') {
                _.cursor += 1;
            } else if (c == '\\' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '\n') {
                // Line continuation
                _.cursor += 2;
            } else if (c == '#') {
                // Comment until end of line
                while (_.cursor < _.src.len && _.src[_.cursor] != '\n') {
                    _.cursor += 1;
                }
            } else {
                break;
            }
        }
    }

    fn isWordDelimiter(ch: u8): bool {
        return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' ||
               ch == '|' || ch == '&' || ch == ';' || ch == '>' || ch == '<' ||
               ch == '(' || ch == ')';
    }

    fn skipDoubleQuoted(*_): usize {
        // cursor points at opening '"'
        _.cursor += 1;
        const inner_start = _.cursor;
        while (_.cursor < _.src.len && _.src[_.cursor] != '"') {
            const ch = _.src[_.cursor];
            if (ch == '\\' && _.cursor + 1 < _.src.len) {
                _.cursor += 2;
            } else if (ch == '$' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '(') {
                _.skipDollarParen();
            } else if (ch == '$' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '{') {
                _.skipBraceExpansion();
            } else if (ch == '`') {
                _.skipBackticks();
            } else {
                _.cursor += 1;
            }
        }
        const inner_end = _.cursor;
        if (_.cursor < _.src.len && _.src[_.cursor] == '"') {
            _.cursor += 1;
        }
        return inner_end - inner_start;
    }

    fn skipSingleQuoted(*_): usize {
        // cursor points at opening '\''
        _.cursor += 1;
        const inner_start = _.cursor;
        while (_.cursor < _.src.len && _.src[_.cursor] != '\'') {
            _.cursor += 1;
        }
        const inner_end = _.cursor;
        if (_.cursor < _.src.len && _.src[_.cursor] == '\'') {
            _.cursor += 1;
        }
        return inner_end - inner_start;
    }

    fn skipAnsiCQuoted(*_) {
        // cursor points at '$' in "$'...'"
        _.cursor += 2;
        while (_.cursor < _.src.len && _.src[_.cursor] != '\'') {
            if (_.src[_.cursor] == '\\' && _.cursor + 1 < _.src.len) {
                _.cursor += 2;
            } else {
                _.cursor += 1;
            }
        }
        if (_.cursor < _.src.len && _.src[_.cursor] == '\'') {
            _.cursor += 1;
        }
    }

    fn skipBackticks(*_) {
        // cursor points at opening '`'
        _.cursor += 1;
        while (_.cursor < _.src.len && _.src[_.cursor] != '`') {
            if (_.src[_.cursor] == '\\' && _.cursor + 1 < _.src.len) {
                _.cursor += 2;
            } else {
                _.cursor += 1;
            }
        }
        if (_.cursor < _.src.len && _.src[_.cursor] == '`') {
            _.cursor += 1;
        }
    }

    fn skipBraceExpansion(*_) {
        // cursor points at '$' in "${...}"
        _.cursor += 2; // skip "${"
        let depth: usize = 1;
        while (_.cursor < _.src.len && depth > 0) {
            const ch = _.src[_.cursor];
            if (ch == '\\' && _.cursor + 1 < _.src.len) {
                _.cursor += 2;
            } else if (ch == '"') {
                * = _.skipDoubleQuoted();
            } else if (ch == '\'') {
                * = _.skipSingleQuoted();
            } else if (ch == '$' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '(') {
                _.skipDollarParen();
            } else if (ch == '{') {
                depth += 1;
                _.cursor += 1;
            } else if (ch == '}') {
                depth -= 1;
                _.cursor += 1;
            } else {
                _.cursor += 1;
            }
        }
    }

    fn skipDollarParen(*_) {
        // cursor points at '$' in "$(...)" or "$((...))"
        if (_.cursor + 2 < _.src.len && _.src[_.cursor + 1] == '(' && _.src[_.cursor + 2] == '(') {
            // Arithmetic expansion $(( ... ))
            _.cursor += 3;
            let depth: usize = 2;
            while (_.cursor < _.src.len && depth > 0) {
                const ch = _.src[_.cursor];
                if (ch == '\\' && _.cursor + 1 < _.src.len) {
                    _.cursor += 2;
                } else if (ch == '"') {
                    * = _.skipDoubleQuoted();
                } else if (ch == '\'') {
                    * = _.skipSingleQuoted();
                } else if (ch == '(') {
                    depth += 1;
                    _.cursor += 1;
                } else if (ch == ')') {
                    depth -= 1;
                    _.cursor += 1;
                } else {
                    _.cursor += 1;
                }
            }
            return;
        }

        // Command substitution $( ... )
        _.cursor += 2; // skip "$("
        let depth: usize = 1;
        while (_.cursor < _.src.len && depth > 0) {
            const ch = _.src[_.cursor];
            if (ch == '\\' && _.cursor + 1 < _.src.len) {
                _.cursor += 2;
            } else if (ch == '"') {
                * = _.skipDoubleQuoted();
            } else if (ch == '\'') {
                * = _.skipSingleQuoted();
            } else if (ch == '`') {
                _.skipBackticks();
            } else if (ch == '$' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '(') {
                _.skipDollarParen();
            } else if (ch == '$' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '{') {
                _.skipBraceExpansion();
            } else if (ch == '#' && (_.cursor == 0 || _.src[_.cursor - 1] == ' ' || _.src[_.cursor - 1] == '\t' || _.src[_.cursor - 1] == '\n' || _.src[_.cursor - 1] == ';' || _.src[_.cursor - 1] == '(')) {
                while (_.cursor < _.src.len && _.src[_.cursor] != '\n') {
                    _.cursor += 1;
                }
            } else if (ch == '(') {
                depth += 1;
                _.cursor += 1;
            } else if (ch == ')') {
                depth -= 1;
                _.cursor += 1;
            } else {
                _.cursor += 1;
            }
        }
    }

    pub fn nextToken(*_): Token {
        _.skipWhitespace();
        if (_.cursor >= _.src.len) {
            return Token {
                kind: .Eof,
                slice: "",
                raw_slice: "",
                quote: .None,
                pos: _.cursor,
            };
        }

        const start = _.cursor;
        const c = _.src[_.cursor];

        // Newline acts as statement separator
        if (c == '\n') {
            _.cursor += 1;
            const s = _.src[start.._.cursor];
            return Token { kind: .Newline, slice: s, raw_slice: s, quote: .None, pos: start };
        }

        // Pipe / Or
        if (c == '|') {
            _.cursor += 1;
            if (_.cursor < _.src.len && _.src[_.cursor] == '|') {
                _.cursor += 1;
                const s = _.src[start.._.cursor];
                return Token { kind: .Or, slice: s, raw_slice: s, quote: .None, pos: start };
            }
            const s = _.src[start.._.cursor];
            return Token { kind: .Pipe, slice: s, raw_slice: s, quote: .None, pos: start };
        }

        // And / &> / &>> / &
        if (c == '&') {
            _.cursor += 1;
            if (_.cursor < _.src.len && _.src[_.cursor] == '&') {
                _.cursor += 1;
                const s = _.src[start.._.cursor];
                return Token { kind: .And, slice: s, raw_slice: s, quote: .None, pos: start };
            }
            if (_.cursor < _.src.len && _.src[_.cursor] == '>') {
                _.cursor += 1;
                if (_.cursor < _.src.len && _.src[_.cursor] == '>') {
                    _.cursor += 1;
                    const s = _.src[start.._.cursor];
                    return Token { kind: .RedirectAllAppend, slice: s, raw_slice: s, quote: .None, pos: start };
                }
                const s = _.src[start.._.cursor];
                return Token { kind: .RedirectAll, slice: s, raw_slice: s, quote: .None, pos: start };
            }
            const s = _.src[start.._.cursor];
            return Token { kind: .Ampersand, slice: s, raw_slice: s, quote: .None, pos: start };
        }

        // Semicolon
        if (c == ';') {
            _.cursor += 1;
            const s = _.src[start.._.cursor];
            return Token { kind: .Semicolon, slice: s, raw_slice: s, quote: .None, pos: start };
        }

        // Subshell parens
        if (c == '(') {
            _.cursor += 1;
            const s = _.src[start.._.cursor];
            return Token { kind: .OpenParen, slice: s, raw_slice: s, quote: .None, pos: start };
        }
        if (c == ')') {
            _.cursor += 1;
            const s = _.src[start.._.cursor];
            return Token { kind: .CloseParen, slice: s, raw_slice: s, quote: .None, pos: start };
        }

        // Standalone '!' (pipeline negation) when followed by delimiter
        if (c == '!' && (_.cursor + 1 >= _.src.len || Lexer.isWordDelimiter(_.src[_.cursor + 1]))) {
            _.cursor += 1;
            const s = _.src[start.._.cursor];
            return Token { kind: .Bang, slice: s, raw_slice: s, quote: .None, pos: start };
        }

        // File-descriptor prefixed redirections: 2>, 2>>, 2>&1, 1>, 1>>, 1>&2, 0<
        if ((c == '2' || c == '1' || c == '0') && _.cursor + 1 < _.src.len) {
            const next_ch = _.src[_.cursor + 1];
            if (c == '2' && next_ch == '>') {
                _.cursor += 2;
                if (_.cursor + 1 < _.src.len && _.src[_.cursor] == '&' && _.src[_.cursor + 1] == '1') {
                    _.cursor += 2;
                    const s = _.src[start.._.cursor];
                    return Token { kind: .RedirectErrToOut, slice: s, raw_slice: s, quote: .None, pos: start };
                }
                if (_.cursor < _.src.len && _.src[_.cursor] == '>') {
                    _.cursor += 1;
                    const s = _.src[start.._.cursor];
                    return Token { kind: .RedirectErrAppend, slice: s, raw_slice: s, quote: .None, pos: start };
                }
                const s = _.src[start.._.cursor];
                return Token { kind: .RedirectErr, slice: s, raw_slice: s, quote: .None, pos: start };
            }
            if (c == '1' && next_ch == '>') {
                _.cursor += 2;
                if (_.cursor + 1 < _.src.len && _.src[_.cursor] == '&' && _.src[_.cursor + 1] == '2') {
                    _.cursor += 2;
                    const s = _.src[start.._.cursor];
                    return Token { kind: .RedirectOutToErr, slice: s, raw_slice: s, quote: .None, pos: start };
                }
                if (_.cursor < _.src.len && _.src[_.cursor] == '>') {
                    _.cursor += 1;
                    const s = _.src[start.._.cursor];
                    return Token { kind: .RedirectAppend, slice: s, raw_slice: s, quote: .None, pos: start };
                }
                const s = _.src[start.._.cursor];
                return Token { kind: .RedirectOut, slice: s, raw_slice: s, quote: .None, pos: start };
            }
            if (c == '0' && next_ch == '<') {
                _.cursor += 2;
                if (_.cursor + 1 < _.src.len && _.src[_.cursor] == '<' && _.src[_.cursor + 1] == '<') {
                    _.cursor += 2;
                    const s = _.src[start.._.cursor];
                    return Token { kind: .HereString, slice: s, raw_slice: s, quote: .None, pos: start };
                }
                if (_.cursor < _.src.len && _.src[_.cursor] == '<') {
                    _.cursor += 1;
                    const s = _.src[start.._.cursor];
                    return Token { kind: .HereDoc, slice: s, raw_slice: s, quote: .None, pos: start };
                }
                const s = _.src[start.._.cursor];
                return Token { kind: .RedirectIn, slice: s, raw_slice: s, quote: .None, pos: start };
            }
        }

        // Standard > / >> / >&2
        if (c == '>') {
            _.cursor += 1;
            if (_.cursor < _.src.len && _.src[_.cursor] == '>') {
                _.cursor += 1;
                const s = _.src[start.._.cursor];
                return Token { kind: .RedirectAppend, slice: s, raw_slice: s, quote: .None, pos: start };
            }
            if (_.cursor + 1 < _.src.len && _.src[_.cursor] == '&' && _.src[_.cursor + 1] == '2') {
                _.cursor += 2;
                const s = _.src[start.._.cursor];
                return Token { kind: .RedirectOutToErr, slice: s, raw_slice: s, quote: .None, pos: start };
            }
            const s = _.src[start.._.cursor];
            return Token { kind: .RedirectOut, slice: s, raw_slice: s, quote: .None, pos: start };
        }

        // Standard < / << / <<<
        if (c == '<') {
            _.cursor += 1;
            if (_.cursor < _.src.len && _.src[_.cursor] == '<') {
                _.cursor += 1;
                if (_.cursor < _.src.len && _.src[_.cursor] == '<') {
                    _.cursor += 1;
                    const s = _.src[start.._.cursor];
                    return Token { kind: .HereString, slice: s, raw_slice: s, quote: .None, pos: start };
                }
                const s = _.src[start.._.cursor];
                return Token { kind: .HereDoc, slice: s, raw_slice: s, quote: .None, pos: start };
            }
            const s = _.src[start.._.cursor];
            return Token { kind: .RedirectIn, slice: s, raw_slice: s, quote: .None, pos: start };
        }

        // Pure quoted words ("..." or '...')
        if (c == '"' || c == '\'') {
            const quote_char = c;
            const content_start = _.cursor + 1;
            const inner_len = if (quote_char == '"') _.skipDoubleQuoted() else _.skipSingleQuoted();
            const content_end = content_start + inner_len;
            const qkind: QuoteKind = if (quote_char == '"') .Double else .Single;

            // If immediately followed by delimiter or EOF, return zero-copy inner slice
            if (_.cursor >= _.src.len || Lexer.isWordDelimiter(_.src[_.cursor])) {
                return Token {
                    kind: .Word,
                    slice: _.src[content_start..content_end],
                    raw_slice: _.src[start.._.cursor],
                    quote: qkind,
                    pos: start,
                };
            }
            // Otherwise continue scanning compound word (e.g. "foo"bar)
        }

        // Standard unquoted or compound words (handles FOO="bar", ${VAR:-def}, $(cmd), $((arith)), `cmd`, $'...', \escapes)
        let saw_quote = (c == '"' || c == '\'');
        while (_.cursor < _.src.len) {
            const ch = _.src[_.cursor];
            if (Lexer.isWordDelimiter(ch)) {
                break;
            }
            if (ch == '\\' && _.cursor + 1 < _.src.len) {
                _.cursor += 2;
            } else if (ch == '"') {
                saw_quote = true;
                * = _.skipDoubleQuoted();
            } else if (ch == '\'') {
                saw_quote = true;
                * = _.skipSingleQuoted();
            } else if (ch == '$' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '\'') {
                saw_quote = true;
                _.skipAnsiCQuoted();
            } else if (ch == '$' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '(') {
                _.skipDollarParen();
            } else if (ch == '$' && _.cursor + 1 < _.src.len && _.src[_.cursor + 1] == '{') {
                _.skipBraceExpansion();
            } else if (ch == '`') {
                _.skipBackticks();
            } else {
                _.cursor += 1;
            }
        }

        const full = _.src[start.._.cursor];
        return Token {
            kind: .Word,
            slice: full,
            raw_slice: full,
            quote: if (saw_quote) .Mixed else .None,
            pos: start,
        };
    }
}
