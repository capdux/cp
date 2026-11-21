// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc W3C SPARQL 1.1 Lexer & Analyzer)

/// W3C SPARQL 1.1 查询形式分类枚举
pub enum SparqlQueryForm: u8 {
    select = 0,
    ask = 1,
    construct = 2,
    describe = 3,
    update = 4,
    unknown = 5,
}

/// SPARQL `PREFIX alias: <iri>` 前缀声明元数据
pub struct SparqlPrefix {
    /// 前缀别名（不含 `:`，空切片表示默认前缀 `:`）
    alias: []const u8,
    /// 前缀展开 IRI（不含 `<` 与 `>`）
    iri: []const u8,
}

/// SPARQL 1.1 词法单元类型
pub enum SparqlTokenKind: u8 {
    ident = 0,
    var_name = 1,
    iri = 2,
    string = 3,
    number = 4,
    lbrace = 5,
    rbrace = 6,
    lparen = 7,
    rparen = 8,
    lbracket = 9,
    rbracket = 10,
    dot = 11,
    semicolon = 12,
    comma = 13,
    punct = 14,
    invalid = 15,
}

/// 零拷贝 SPARQL 1.1 词法单元
pub struct SparqlToken {
    kind: SparqlTokenKind,
    lexeme: []const u8,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isIdentStart(c: u8): bool {
    return isAsciiAlpha(c) || c == '_' || c >= 0x80;
}

fn isIdentContinue(c: u8): bool {
    return isAsciiAlpha(c) || isDigit(c) || c == '_' || c == '-' || c >= 0x80;
}

fn isVarChar(c: u8): bool {
    return isAsciiAlpha(c) || isDigit(c) || c == '_' || c >= 0x80;
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn strEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) return false;
        i += 1;
    }
    return true;
}

fn isTwoCharPunct(a: u8, b: u8): bool {
    if (a == '^' && b == '^') return true;
    if (a == '|' && b == '|') return true;
    if (a == '&' && b == '&') return true;
    if (a == '!' && b == '=') return true;
    if (a == '<' && b == '=') return true;
    if (a == '>' && b == '=') return true;
    return false;
}

fn isSinglePunct(c: u8): bool {
    return c == '=' || c == '<' || c == '>' || c == '+' || c == '-' || c == '*'
        || c == '/' || c == '!' || c == '|' || c == '^' || c == ':' || c == '@'
        || c == '?' || c == '$' || c == '~';
}

fn isIriForbidden(c: u8): bool {
    if (c <= 0x20) return true;
    return c == '<' || c == '>' || c == '"' || c == '{' || c == '}'
        || c == '|' || c == '^' || c == '`';
}

fn isUpdateKeyword(lexeme: []const u8): bool {
    return strEqlIgnoreCase(lexeme, "INSERT")
        || strEqlIgnoreCase(lexeme, "DELETE")
        || strEqlIgnoreCase(lexeme, "LOAD")
        || strEqlIgnoreCase(lexeme, "CLEAR")
        || strEqlIgnoreCase(lexeme, "DROP")
        || strEqlIgnoreCase(lexeme, "CREATE")
        || strEqlIgnoreCase(lexeme, "COPY")
        || strEqlIgnoreCase(lexeme, "MOVE")
        || strEqlIgnoreCase(lexeme, "ADD")
        || strEqlIgnoreCase(lexeme, "WITH");
}

/// 零堆分配 W3C SPARQL 1.1 流式词法扫描器
pub struct SparqlIter {
    input: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): SparqlIter {
        return SparqlIter {
            input: input,
            pos: 0,
        };
    }

    fn skipWsAndComments(*_) {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (isSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                _.pos += 1;
                while (_.pos < _.input.len && _.input[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            break;
        }
    }

    fn scanString(*_, quote: u8): SparqlToken {
        const start = _.pos;
        const is_triple = _.pos + 2 < _.input.len
            && _.input[_.pos + 1] == quote
            && _.input[_.pos + 2] == quote;

        if (is_triple) {
            _.pos += 3;
            while (_.pos < _.input.len) {
                const c = _.input[_.pos];
                if (c == '\\') {
                    if (_.pos + 1 < _.input.len) {
                        _.pos += 2;
                    } else {
                        _.pos = _.input.len;
                        return SparqlToken {
                            kind: SparqlTokenKind.invalid,
                            lexeme: _.input[start.._.pos],
                        };
                    }
                    continue;
                }
                if (c == quote && _.pos + 2 < _.input.len
                    && _.input[_.pos + 1] == quote
                    && _.input[_.pos + 2] == quote) {
                    _.pos += 3;
                    return SparqlToken {
                        kind: SparqlTokenKind.string,
                        lexeme: _.input[start.._.pos],
                    };
                }
                _.pos += 1;
            }
            return SparqlToken {
                kind: SparqlTokenKind.invalid,
                lexeme: _.input[start.._.pos],
            };
        }

        // Single-quoted or double-quoted short string literal
        _.pos += 1;
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == '\\') {
                if (_.pos + 1 < _.input.len) {
                    _.pos += 2;
                } else {
                    _.pos = _.input.len;
                    return SparqlToken {
                        kind: SparqlTokenKind.invalid,
                        lexeme: _.input[start.._.pos],
                    };
                }
                continue;
            }
            if (c == '\n' || c == '\r') {
                return SparqlToken {
                    kind: SparqlTokenKind.invalid,
                    lexeme: _.input[start.._.pos],
                };
            }
            if (c == quote) {
                _.pos += 1;
                return SparqlToken {
                    kind: SparqlTokenKind.string,
                    lexeme: _.input[start.._.pos],
                };
            }
            _.pos += 1;
        }
        return SparqlToken {
            kind: SparqlTokenKind.invalid,
            lexeme: _.input[start.._.pos],
        };
    }

    fn scanIriOrLess(*_): SparqlToken {
        const start = _.pos;
        if (_.pos + 1 < _.input.len && _.input[_.pos + 1] == '=') {
            _.pos += 2;
            return SparqlToken {
                kind: SparqlTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }
        if (_.pos + 1 >= _.input.len || isSpace(_.input[_.pos + 1])) {
            _.pos += 1;
            return SparqlToken {
                kind: SparqlTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        // Attempt to scan IRIREF: '<' ([^<>"{}|^`\]-[#x00-#x20] | UCHAR)* '>'
        let i = _.pos + 1;
        let saw_iri_hint = false;
        while (i < _.input.len) {
            const c = _.input[i];
            if (c == '>') {
                _.pos = i + 1;
                return SparqlToken {
                    kind: SparqlTokenKind.iri,
                    lexeme: _.input[start.._.pos],
                };
            }
            if (c == '\\') {
                if (i + 1 < _.input.len) {
                    i += 2;
                    saw_iri_hint = true;
                    continue;
                }
                _.pos = _.input.len;
                return SparqlToken {
                    kind: SparqlTokenKind.invalid,
                    lexeme: _.input[start.._.pos],
                };
            }
            if (c == ':' || c == '/') {
                saw_iri_hint = true;
            }
            if (isIriForbidden(c)) {
                if (saw_iri_hint) {
                    _.pos = i;
                    return SparqlToken {
                        kind: SparqlTokenKind.invalid,
                        lexeme: _.input[start.._.pos],
                    };
                }
                // Treat as '<' comparison operator (e.g., `?x<?y` or `1<2`)
                _.pos += 1;
                return SparqlToken {
                    kind: SparqlTokenKind.punct,
                    lexeme: _.input[start.._.pos],
                };
            }
            i += 1;
        }

        if (saw_iri_hint) {
            _.pos = _.input.len;
            return SparqlToken {
                kind: SparqlTokenKind.invalid,
                lexeme: _.input[start.._.pos],
            };
        }
        _.pos += 1;
        return SparqlToken {
            kind: SparqlTokenKind.punct,
            lexeme: _.input[start.._.pos],
        };
    }

    fn scanNumber(*_): SparqlToken {
        const start = _.pos;
        if (_.input[_.pos] == '.') {
            _.pos += 1;
            while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                _.pos += 1;
            }
        } else {
            while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                _.pos += 1;
            }
            if (_.pos + 1 < _.input.len && _.input[_.pos] == '.' && isDigit(_.input[_.pos + 1])) {
                _.pos += 1;
                while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
            }
        }

        // Optional exponent: [eE][+-]?[0-9]+
        if (_.pos < _.input.len && (_.input[_.pos] == 'e' || _.input[_.pos] == 'E')) {
            let exp_pos = _.pos + 1;
            if (exp_pos < _.input.len && (_.input[exp_pos] == '+' || _.input[exp_pos] == '-')) {
                exp_pos += 1;
            }
            if (exp_pos < _.input.len && isDigit(_.input[exp_pos])) {
                _.pos = exp_pos;
                while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
            }
        }

        return SparqlToken {
            kind: SparqlTokenKind.number,
            lexeme: _.input[start.._.pos],
        };
    }

    /// 返回下一个词法单元；到达输入末尾时返回 `null`
    pub fn next(*_): SparqlToken? {
        _.skipWsAndComments();
        if (_.pos >= _.input.len) return null;

        const start = _.pos;
        const c = _.input[_.pos];

        // Delimiters
        if (c == '{') {
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.lbrace, lexeme: _.input[start.._.pos] };
        }
        if (c == '}') {
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.rbrace, lexeme: _.input[start.._.pos] };
        }
        if (c == '(') {
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.lparen, lexeme: _.input[start.._.pos] };
        }
        if (c == ')') {
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.rparen, lexeme: _.input[start.._.pos] };
        }
        if (c == '[') {
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.lbracket, lexeme: _.input[start.._.pos] };
        }
        if (c == ']') {
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.rbracket, lexeme: _.input[start.._.pos] };
        }
        if (c == ';') {
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.semicolon, lexeme: _.input[start.._.pos] };
        }
        if (c == ',') {
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.comma, lexeme: _.input[start.._.pos] };
        }

        // Dot or leading-dot decimal number (.5)
        if (c == '.') {
            if (_.pos + 1 < _.input.len && isDigit(_.input[_.pos + 1])) {
                return _.scanNumber();
            }
            _.pos += 1;
            return SparqlToken { kind: SparqlTokenKind.dot, lexeme: _.input[start.._.pos] };
        }

        // String literals ('...', "...", '''...''', """...""")
        if (c == '"' || c == '\'') {
            return _.scanString(c);
        }

        // IRIREF (<http://...>) or '<' / '<='
        if (c == '<') {
            return _.scanIriOrLess();
        }

        // Variable (?x / $x) or '?' / '$' operator
        if (c == '?' || c == '$') {
            if (_.pos + 1 < _.input.len && isVarChar(_.input[_.pos + 1])) {
                _.pos += 2;
                while (_.pos < _.input.len && isVarChar(_.input[_.pos])) {
                    _.pos += 1;
                }
                return SparqlToken {
                    kind: SparqlTokenKind.var_name,
                    lexeme: _.input[start.._.pos],
                };
            }
            _.pos += 1;
            return SparqlToken {
                kind: SparqlTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        // Numeric literal
        if (isDigit(c)) {
            return _.scanNumber();
        }

        // Identifier / keyword / prefix alias
        if (isIdentStart(c)) {
            _.pos += 1;
            while (_.pos < _.input.len && isIdentContinue(_.input[_.pos])) {
                _.pos += 1;
            }
            return SparqlToken {
                kind: SparqlTokenKind.ident,
                lexeme: _.input[start.._.pos],
            };
        }

        // Two-char punctuation
        if (_.pos + 1 < _.input.len && isTwoCharPunct(c, _.input[_.pos + 1])) {
            _.pos += 2;
            return SparqlToken {
                kind: SparqlTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        // Single-char punctuation
        if (isSinglePunct(c)) {
            _.pos += 1;
            return SparqlToken {
                kind: SparqlTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        // Unrecognized / invalid byte
        _.pos += 1;
        return SparqlToken {
            kind: SparqlTokenKind.invalid,
            lexeme: _.input[start.._.pos],
        };
    }
}

/// 零堆分配 SPARQL `PREFIX alias: <iri>` 前缀声明流式提取器（大小写不敏感）
pub struct SparqlPrefixIter {
    lex: SparqlIter,

    pub static fn init(query: []const u8): SparqlPrefixIter {
        return SparqlPrefixIter {
            lex: SparqlIter.init(query),
        };
    }

    /// 提取下一条合法的 `PREFIX [alias]: <iri>` 声明
    pub fn next(*_): SparqlPrefix? {
        while (tok := _.lex.next()) {
            if (tok.kind == SparqlTokenKind.ident && strEqlIgnoreCase(tok.lexeme, "PREFIX")) {
                const t1 = _.lex.next() ?? return null;
                if (t1.kind == SparqlTokenKind.punct && t1.lexeme.len == 1 && t1.lexeme[0] == ':') {
                    // Default prefix: PREFIX : <iri>
                    const t2 = _.lex.next() ?? return null;
                    if (t2.kind == SparqlTokenKind.iri && t2.lexeme.len >= 2) {
                        return SparqlPrefix {
                            alias: "",
                            iri: t2.lexeme[1..t2.lexeme.len - 1],
                        };
                    }
                    continue;
                }
                if (t1.kind == SparqlTokenKind.ident) {
                    const t2 = _.lex.next() ?? return null;
                    if (t2.kind == SparqlTokenKind.punct && t2.lexeme.len == 1 && t2.lexeme[0] == ':') {
                        const t3 = _.lex.next() ?? return null;
                        if (t3.kind == SparqlTokenKind.iri && t3.lexeme.len >= 2) {
                            return SparqlPrefix {
                                alias: t1.lexeme,
                                iri: t3.lexeme[1..t3.lexeme.len - 1],
                            };
                        }
                    }
                }
            }
        }
        return null;
    }
}

/// 跳过前导 `BASE`/`PREFIX` 声明后判定 SPARQL 查询/更新形式
pub fn detectSparqlQueryForm(query: []const u8): SparqlQueryForm {
    let lex = SparqlIter.init(query);

    while (tok := lex.next()) {
        if (tok.kind != SparqlTokenKind.ident) {
            return SparqlQueryForm.unknown;
        }

        if (strEqlIgnoreCase(tok.lexeme, "BASE")) {
            const iri_tok = lex.next() ?? return SparqlQueryForm.unknown;
            if (iri_tok.kind != SparqlTokenKind.iri) {
                return SparqlQueryForm.unknown;
            }
            continue;
        }

        if (strEqlIgnoreCase(tok.lexeme, "PREFIX")) {
            const t1 = lex.next() ?? return SparqlQueryForm.unknown;
            if (t1.kind == SparqlTokenKind.punct && t1.lexeme.len == 1 && t1.lexeme[0] == ':') {
                const t2 = lex.next() ?? return SparqlQueryForm.unknown;
                if (t2.kind != SparqlTokenKind.iri) {
                    return SparqlQueryForm.unknown;
                }
                continue;
            }
            if (t1.kind == SparqlTokenKind.ident) {
                const t2 = lex.next() ?? return SparqlQueryForm.unknown;
                if (t2.kind != SparqlTokenKind.punct || t2.lexeme.len != 1 || t2.lexeme[0] != ':') {
                    return SparqlQueryForm.unknown;
                }
                const t3 = lex.next() ?? return SparqlQueryForm.unknown;
                if (t3.kind != SparqlTokenKind.iri) {
                    return SparqlQueryForm.unknown;
                }
                continue;
            }
            return SparqlQueryForm.unknown;
        }

        if (strEqlIgnoreCase(tok.lexeme, "SELECT")) return SparqlQueryForm.select;
        if (strEqlIgnoreCase(tok.lexeme, "ASK")) return SparqlQueryForm.ask;
        if (strEqlIgnoreCase(tok.lexeme, "CONSTRUCT")) return SparqlQueryForm.construct;
        if (strEqlIgnoreCase(tok.lexeme, "DESCRIBE")) return SparqlQueryForm.describe;
        if (isUpdateKeyword(tok.lexeme)) return SparqlQueryForm.update;

        return SparqlQueryForm.unknown;
    }

    return SparqlQueryForm.unknown;
}

/// 判定 SPARQL 语句是否为严格只读查询（`SELECT`/`ASK`/`CONSTRUCT`/`DESCRIBE` 且无非法词法或多语句写操作）
pub fn isReadOnlySparql(query: []const u8): bool {
    const form = detectSparqlQueryForm(query);
    if (form != SparqlQueryForm.select
        && form != SparqlQueryForm.ask
        && form != SparqlQueryForm.construct
        && form != SparqlQueryForm.describe) {
        return false;
    }

    let lex = SparqlIter.init(query);
    let depth: usize = 0;
    let after_top_semi = false;

    while (tok := lex.next()) {
        if (tok.kind == SparqlTokenKind.invalid) {
            return false;
        }
        if (tok.kind == SparqlTokenKind.lbrace
            || tok.kind == SparqlTokenKind.lparen
            || tok.kind == SparqlTokenKind.lbracket) {
            depth += 1;
            after_top_semi = false;
            continue;
        }
        if (tok.kind == SparqlTokenKind.rbrace
            || tok.kind == SparqlTokenKind.rparen
            || tok.kind == SparqlTokenKind.rbracket) {
            if (depth == 0) return false;
            depth -= 1;
            after_top_semi = false;
            continue;
        }
        if (depth == 0 && tok.kind == SparqlTokenKind.semicolon) {
            after_top_semi = true;
            continue;
        }
        if (after_top_semi) {
            if (tok.kind == SparqlTokenKind.ident && isUpdateKeyword(tok.lexeme)) {
                return false;
            }
            after_top_semi = false;
        }
    }

    return depth == 0;
}

/// 字符串/IRI/注释感知校验 `{}`/`()`/`[]` 括号平衡，并强制 64 层最大嵌套深度上限
pub fn validateSparqlDelimiters(query: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let lex = SparqlIter.init(query);

    while (tok := lex.next()) {
        if (tok.kind == SparqlTokenKind.invalid) {
            return false;
        }
        if (tok.kind == SparqlTokenKind.lbrace) {
            if (depth >= 64) return false;
            stack[depth] = '{';
            depth += 1;
        } else if (tok.kind == SparqlTokenKind.lparen) {
            if (depth >= 64) return false;
            stack[depth] = '(';
            depth += 1;
        } else if (tok.kind == SparqlTokenKind.lbracket) {
            if (depth >= 64) return false;
            stack[depth] = '[';
            depth += 1;
        } else if (tok.kind == SparqlTokenKind.rbrace) {
            if (depth == 0 || stack[depth - 1] != '{') return false;
            depth -= 1;
        } else if (tok.kind == SparqlTokenKind.rparen) {
            if (depth == 0 || stack[depth - 1] != '(') return false;
            depth -= 1;
        } else if (tok.kind == SparqlTokenKind.rbracket) {
            if (depth == 0 || stack[depth - 1] != '[') return false;
            depth -= 1;
        }
    }

    return depth == 0;
}
