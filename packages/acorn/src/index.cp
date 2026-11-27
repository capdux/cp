// @cap/acorn-parse — Zero-allocation ES2024 JavaScript lexer & ES Module scanner for CP
// Clean-room MIT implementation inspired by npm `acorn` v8.
// Tracks exact 1-based line/column positions with zero heap allocations.

pub const TOK_EOF: u8 = 0;
pub const TOK_IDENT: u8 = 1;
pub const TOK_KEYWORD: u8 = 2;
pub const TOK_NUMBER: u8 = 3;
pub const TOK_STRING: u8 = 4;
pub const TOK_TEMPLATE: u8 = 5;
pub const TOK_PUNCT: u8 = 6;
pub const TOK_COMMENT: u8 = 7;

/// A single lexical token with byte span `[start..end)` and 1-based `line`/`col`.
pub struct JsToken {
    pub kind: u8,
    pub start: usize,
    pub end: usize,
    pub line: usize,
    pub col: usize,
}

/// Summary produced by `analyzeJsModule`.
pub struct JsModuleSummary {
    pub ok: bool,
    pub token_count: usize,
    pub import_count: usize,
    pub export_count: usize,
    pub function_count: usize,
    pub class_count: usize,
    pub unmatched_bracket_pos: usize,
}

fn acnStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn acnIsSpace(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 10 || ch == 13 || ch == 11 || ch == 12;
}

fn acnIsDigit(ch: u8): bool {
    return ch >= 48 && ch <= 57;
}

fn acnIsHexDigit(ch: u8): bool {
    return (ch >= 48 && ch <= 57) || (ch >= 65 && ch <= 70) || (ch >= 97 && ch <= 102);
}

fn acnIsBinDigit(ch: u8): bool {
    return ch == 48 || ch == 49;
}

fn acnIsOctDigit(ch: u8): bool {
    return ch >= 48 && ch <= 55;
}

fn acnIsIdentStart(ch: u8): bool {
    return (ch >= 65 && ch <= 90)
        || (ch >= 97 && ch <= 122)
        || ch == 95  // '_'
        || ch == 36  // '$'
        || ch >= 0x80;
}

fn acnIsIdentPart(ch: u8): bool {
    return acnIsIdentStart(ch) || acnIsDigit(ch);
}

fn acnIsKeyword(s: string): bool {
    if (acnStrEq(s, "import") || acnStrEq(s, "export") || acnStrEq(s, "from")) {
        return true;
    }
    if (acnStrEq(s, "function") || acnStrEq(s, "const") || acnStrEq(s, "let") || acnStrEq(s, "var")) {
        return true;
    }
    if (acnStrEq(s, "class") || acnStrEq(s, "async") || acnStrEq(s, "await") || acnStrEq(s, "return")) {
        return true;
    }
    if (acnStrEq(s, "if") || acnStrEq(s, "else") || acnStrEq(s, "for") || acnStrEq(s, "while")) {
        return true;
    }
    if (acnStrEq(s, "try") || acnStrEq(s, "catch") || acnStrEq(s, "throw") || acnStrEq(s, "new")) {
        return true;
    }
    if (acnStrEq(s, "default") || acnStrEq(s, "as") || acnStrEq(s, "of") || acnStrEq(s, "in")) {
        return true;
    }
    if (acnStrEq(s, "switch") || acnStrEq(s, "case") || acnStrEq(s, "break") || acnStrEq(s, "continue")) {
        return true;
    }
    if (acnStrEq(s, "finally") || acnStrEq(s, "typeof") || acnStrEq(s, "void") || acnStrEq(s, "delete")) {
        return true;
    }
    if (acnStrEq(s, "yield") || acnStrEq(s, "this") || acnStrEq(s, "super") || acnStrEq(s, "extends")) {
        return true;
    }
    if (acnStrEq(s, "static") || acnStrEq(s, "true") || acnStrEq(s, "false") || acnStrEq(s, "null")) {
        return true;
    }
    return false;
}

fn acnMatch3(src: string, pos: usize, a: u8, b: u8, c: u8): bool {
    return pos + 2 < src.len && src[pos] == a && src[pos + 1] == b && src[pos + 2] == c;
}

fn acnMatch2(src: string, pos: usize, a: u8, b: u8): bool {
    return pos + 1 < src.len && src[pos] == a && src[pos + 1] == b;
}

fn acnPunctLen(src: string, pos: usize): usize {
    // 3-char operators: === !== ... >>> **= <<= >>= &&= ||= ??=
    if (acnMatch3(src, pos, 61, 61, 61)      // ===
        || acnMatch3(src, pos, 33, 61, 61)   // !==
        || acnMatch3(src, pos, 46, 46, 46)   // ...
        || acnMatch3(src, pos, 62, 62, 62)   // >>>
        || acnMatch3(src, pos, 42, 42, 61)   // **=
        || acnMatch3(src, pos, 60, 60, 61)   // <<=
        || acnMatch3(src, pos, 62, 62, 61)   // >>=
        || acnMatch3(src, pos, 38, 38, 61)   // &&=
        || acnMatch3(src, pos, 124, 124, 61) // ||=
        || acnMatch3(src, pos, 63, 63, 61)) { // ??=
        return 3;
    }
    // 2-char operators: => ?? ?. && || == != <= >= ++ -- ** << >> += -= *= /= %= &= |= ^=
    if (acnMatch2(src, pos, 61, 62)       // =>
        || acnMatch2(src, pos, 63, 63)    // ??
        || acnMatch2(src, pos, 63, 46)    // ?.
        || acnMatch2(src, pos, 38, 38)    // &&
        || acnMatch2(src, pos, 124, 124)  // ||
        || acnMatch2(src, pos, 61, 61)    // ==
        || acnMatch2(src, pos, 33, 61)    // !=
        || acnMatch2(src, pos, 60, 61)    // <=
        || acnMatch2(src, pos, 62, 61)    // >=
        || acnMatch2(src, pos, 43, 43)    // ++
        || acnMatch2(src, pos, 45, 45)    // --
        || acnMatch2(src, pos, 42, 42)    // **
        || acnMatch2(src, pos, 60, 60)    // <<
        || acnMatch2(src, pos, 62, 62)    // >>
        || acnMatch2(src, pos, 43, 61)    // +=
        || acnMatch2(src, pos, 45, 61)    // -=
        || acnMatch2(src, pos, 42, 61)    // *=
        || acnMatch2(src, pos, 47, 61)    // /=
        || acnMatch2(src, pos, 37, 61)    // %=
        || acnMatch2(src, pos, 38, 61)    // &=
        || acnMatch2(src, pos, 124, 61)   // |=
        || acnMatch2(src, pos, 94, 61)) { // ^=
        return 2;
    }
    return 1;
}

/// Zero-allocation ES2024 JavaScript lexer with 1-based line and column tracking.
pub struct JsLexer {
    pub src: string,
    pub pos: usize,
    pub line: usize,
    pub col: usize,

    /// Create a new lexer positioned at the start of `src` (line 1, col 1).
    pub static fn new(src: string): JsLexer {
        return JsLexer {
            src: src,
            pos: 0,
            line: 1,
            col: 1,
        };
    }

    fn advance(*_) {
        if (_.pos < _.src.len) {
            if (_.src[_.pos] == 10) { // '\n'
                _.line = _.line + 1;
                _.col = 1;
            } else {
                _.col = _.col + 1;
            }
            _.pos = _.pos + 1;
        }
    }

    fn skipWhitespace(*_) {
        while (_.pos < _.src.len && acnIsSpace(_.src[_.pos])) {
            _.advance();
        }
    }

    /// Read and return the next lexical token (or `TOK_EOF` at end of input).
    pub fn nextToken(*_): JsToken {
        _.skipWhitespace();

        if (_.pos >= _.src.len) {
            return JsToken {
                kind: TOK_EOF,
                start: _.pos,
                end: _.pos,
                line: _.line,
                col: _.col,
            };
        }

        const start = _.pos;
        const start_line = _.line;
        const start_col = _.col;
        const ch = _.src[_.pos];

        // 1. Line comment // ...
        if (ch == 47 && _.pos + 1 < _.src.len && _.src[_.pos + 1] == 47) {
            _.advance();
            _.advance();
            while (_.pos < _.src.len && _.src[_.pos] != 10) {
                _.advance();
            }
            return JsToken {
                kind: TOK_COMMENT,
                start: start,
                end: _.pos,
                line: start_line,
                col: start_col,
            };
        }

        // 2. Block comment /* ... */
        if (ch == 47 && _.pos + 1 < _.src.len && _.src[_.pos + 1] == 42) {
            _.advance();
            _.advance();
            while (_.pos < _.src.len) {
                if (_.src[_.pos] == 42 && _.pos + 1 < _.src.len && _.src[_.pos + 1] == 47) {
                    _.advance();
                    _.advance();
                    break;
                }
                _.advance();
            }
            return JsToken {
                kind: TOK_COMMENT,
                start: start,
                end: _.pos,
                line: start_line,
                col: start_col,
            };
        }

        // 3. Single or double quoted string literal
        if (ch == 34 || ch == 39) {
            const quote = ch;
            _.advance();
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (c == 92) { // '\\'
                    _.advance();
                    if (_.pos < _.src.len) {
                        _.advance();
                    }
                    continue;
                }
                _.advance();
                if (c == quote) {
                    break;
                }
            }
            return JsToken {
                kind: TOK_STRING,
                start: start,
                end: _.pos,
                line: start_line,
                col: start_col,
            };
        }

        // 4. Template literal `...` (tracks ${ ... } interpolation depth)
        if (ch == 96) { // '`'
            _.advance();
            let expr_depth: usize = 0;
            while (_.pos < _.src.len) {
                const c = _.src[_.pos];
                if (c == 92) { // '\\'
                    _.advance();
                    if (_.pos < _.src.len) {
                        _.advance();
                    }
                    continue;
                }
                if (c == 36 && _.pos + 1 < _.src.len && _.src[_.pos + 1] == 123) { // "${"
                    expr_depth = expr_depth + 1;
                    _.advance();
                    _.advance();
                    continue;
                }
                if (expr_depth > 0) {
                    if (c == 123) { // '{'
                        expr_depth = expr_depth + 1;
                    } else if (c == 125) { // '}'
                        expr_depth = expr_depth - 1;
                    }
                    _.advance();
                    continue;
                }
                _.advance();
                if (c == 96) {
                    break;
                }
            }
            return JsToken {
                kind: TOK_TEMPLATE,
                start: start,
                end: _.pos,
                line: start_line,
                col: start_col,
            };
        }

        // 5. Numeric literal (decimal, hex, binary, octal, float, scientific, BigInt)
        if (acnIsDigit(ch) || (ch == 46 && _.pos + 1 < _.src.len && acnIsDigit(_.src[_.pos + 1]))) {
            if (ch == 48 && _.pos + 1 < _.src.len) {
                const next_c = _.src[_.pos + 1];
                if (next_c == 120 || next_c == 88) { // 0x / 0X
                    _.advance();
                    _.advance();
                    while (_.pos < _.src.len && (acnIsHexDigit(_.src[_.pos]) || _.src[_.pos] == 95)) {
                        _.advance();
                    }
                    if (_.pos < _.src.len && _.src[_.pos] == 110) { // 'n'
                        _.advance();
                    }
                    return JsToken { kind: TOK_NUMBER, start: start, end: _.pos, line: start_line, col: start_col };
                }
                if (next_c == 98 || next_c == 66) { // 0b / 0B
                    _.advance();
                    _.advance();
                    while (_.pos < _.src.len && (acnIsBinDigit(_.src[_.pos]) || _.src[_.pos] == 95)) {
                        _.advance();
                    }
                    if (_.pos < _.src.len && _.src[_.pos] == 110) {
                        _.advance();
                    }
                    return JsToken { kind: TOK_NUMBER, start: start, end: _.pos, line: start_line, col: start_col };
                }
                if (next_c == 111 || next_c == 79) { // 0o / 0O
                    _.advance();
                    _.advance();
                    while (_.pos < _.src.len && (acnIsOctDigit(_.src[_.pos]) || _.src[_.pos] == 95)) {
                        _.advance();
                    }
                    if (_.pos < _.src.len && _.src[_.pos] == 110) {
                        _.advance();
                    }
                    return JsToken { kind: TOK_NUMBER, start: start, end: _.pos, line: start_line, col: start_col };
                }
            }

            while (_.pos < _.src.len && (acnIsDigit(_.src[_.pos]) || _.src[_.pos] == 95)) {
                _.advance();
            }
            if (_.pos < _.src.len && _.src[_.pos] == 46) { // '.'
                _.advance();
                while (_.pos < _.src.len && (acnIsDigit(_.src[_.pos]) || _.src[_.pos] == 95)) {
                    _.advance();
                }
            }
            if (_.pos < _.src.len && (_.src[_.pos] == 101 || _.src[_.pos] == 69)) { // 'e' / 'E'
                _.advance();
                if (_.pos < _.src.len && (_.src[_.pos] == 43 || _.src[_.pos] == 45)) { // '+' / '-'
                    _.advance();
                }
                while (_.pos < _.src.len && (acnIsDigit(_.src[_.pos]) || _.src[_.pos] == 95)) {
                    _.advance();
                }
            }
            if (_.pos < _.src.len && _.src[_.pos] == 110) { // BigInt 'n'
                _.advance();
            }
            return JsToken {
                kind: TOK_NUMBER,
                start: start,
                end: _.pos,
                line: start_line,
                col: start_col,
            };
        }

        // 6. Identifier or ES Keyword
        if (acnIsIdentStart(ch)) {
            _.advance();
            while (_.pos < _.src.len && acnIsIdentPart(_.src[_.pos])) {
                _.advance();
            }
            const lexeme = _.src[start.._.pos];
            const kind = if (acnIsKeyword(lexeme)) TOK_KEYWORD else TOK_IDENT;
            return JsToken {
                kind: kind,
                start: start,
                end: _.pos,
                line: start_line,
                col: start_col,
            };
        }

        // 7. Punctuator / Multi-character operator
        const plen = acnPunctLen(_.src, _.pos);
        let k: usize = 0;
        while (k < plen) {
            _.advance();
            k = k + 1;
        }
        return JsToken {
            kind: TOK_PUNCT,
            start: start,
            end: _.pos,
            line: start_line,
            col: start_col,
        };
    }
}

fn acnMatchingOpen(close_ch: u8): u8 {
    if (close_ch == 41) { // ')'
        return 40;        // '('
    }
    if (close_ch == 93) { // ']'
        return 91;        // '['
    }
    return 123;           // '{' for '}'
}

/// Scan a JavaScript/ESM source file: validates `()`, `[]`, `{}` bracket balance
/// and counts non-comment tokens as well as `import`, `export`, `function`, and `class` declarations.
pub fn analyzeJsModule(src: string): JsModuleSummary {
    let lex = JsLexer.new(src);
    let ok = true;
    let token_count: usize = 0;
    let import_count: usize = 0;
    let export_count: usize = 0;
    let function_count: usize = 0;
    let class_count: usize = 0;
    let unmatched_pos: usize = 0;

    let stack_ch: [128]u8 = undefined;
    let stack_pos: [128]usize = undefined;
    let sp: usize = 0;

    while (true) {
        const tok = lex.nextToken();
        if (tok.kind == TOK_EOF) {
            break;
        }
        if (tok.kind == TOK_COMMENT) {
            continue;
        }

        token_count = token_count + 1;

        if (tok.kind == TOK_KEYWORD) {
            const word = src[tok.start..tok.end];
            if (acnStrEq(word, "import")) {
                import_count = import_count + 1;
            } else if (acnStrEq(word, "export")) {
                export_count = export_count + 1;
            } else if (acnStrEq(word, "function")) {
                function_count = function_count + 1;
            } else if (acnStrEq(word, "class")) {
                class_count = class_count + 1;
            }
        } else if (tok.kind == TOK_PUNCT && tok.end == tok.start + 1) {
            const c = src[tok.start];
            if (c == 40 || c == 91 || c == 123) { // '(' '[' '{'
                if (sp < 128) {
                    stack_ch[sp] = c;
                    stack_pos[sp] = tok.start;
                    sp = sp + 1;
                }
            } else if (c == 41 || c == 93 || c == 125) { // ')' ']' '}'
                const expected = acnMatchingOpen(c);
                if (sp == 0 || stack_ch[sp - 1] != expected) {
                    if (ok) {
                        ok = false;
                        unmatched_pos = tok.start;
                    }
                } else {
                    sp = sp - 1;
                }
            }
        }
    }

    if (ok && sp > 0) {
        ok = false;
        unmatched_pos = stack_pos[sp - 1];
    }

    return JsModuleSummary {
        ok: ok,
        token_count: token_count,
        import_count: import_count,
        export_count: export_count,
        function_count: function_count,
        class_count: class_count,
        unmatched_bracket_pos: unmatched_pos,
    };
}

/// Extract static ES Module import specifiers (`import ... from "spec"` and `import "spec"`)
/// as zero-copy substrings into `out_specs`. Returns the number of specifiers extracted.
pub fn extractImportSpecifiers(src: string, out_specs: []string): usize {
    if (src.len == 0 || out_specs.len == 0) {
        return 0;
    }

    let lex = JsLexer.new(src);
    let count: usize = 0;
    // state: 0 = idle, 1 = saw `import`, 2 = inside import clause awaiting `from`, 3 = saw `from`
    let state: u8 = 0;

    while (count < out_specs.len) {
        const tok = lex.nextToken();
        if (tok.kind == TOK_EOF) {
            break;
        }
        if (tok.kind == TOK_COMMENT) {
            continue;
        }

        if (state == 0) {
            if (tok.kind == TOK_KEYWORD && acnStrEq(src[tok.start..tok.end], "import")) {
                state = 1;
            }
            continue;
        }

        if (state == 1) {
            if (tok.kind == TOK_STRING && tok.end >= tok.start + 2) {
                out_specs[count] = src[(tok.start + 1)..(tok.end - 1)];
                count = count + 1;
                state = 0;
            } else if (tok.kind == TOK_PUNCT && tok.end == tok.start + 1 && src[tok.start] == 40) {
                // Dynamic import(...) call — skip
                state = 0;
            } else {
                state = 2;
            }
            continue;
        }

        if (state == 2) {
            if (tok.kind == TOK_KEYWORD && acnStrEq(src[tok.start..tok.end], "from")) {
                state = 3;
            } else if (tok.kind == TOK_PUNCT && tok.end == tok.start + 1 && src[tok.start] == 59) {
                state = 0;
            }
            continue;
        }

        if (state == 3) {
            if (tok.kind == TOK_STRING && tok.end >= tok.start + 2) {
                out_specs[count] = src[(tok.start + 1)..(tok.end - 1)];
                count = count + 1;
            }
            state = 0;
        }
    }

    return count;
}

export {
    TOK_EOF,
    TOK_IDENT,
    TOK_KEYWORD,
    TOK_NUMBER,
    TOK_STRING,
    TOK_TEMPLATE,
    TOK_PUNCT,
    TOK_COMMENT,
    JsToken,
    JsLexer,
    JsModuleSummary,
    analyzeJsModule,
    extractImportSpecifiers,
};
