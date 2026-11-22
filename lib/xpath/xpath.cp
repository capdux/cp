// Origin: Pure-CP Stdlib (Zero-Alloc W3C XPath Lexer & Step Scanner) | Created: 2026-11-22

/// W3C XPath 词法单元类型
pub enum XPathTokenKind: u8 {
    slash = 0,
    dslash = 1,
    at = 2,
    axis = 3,
    dot = 4,
    ddot = 5,
    star = 6,
    pipe = 7,
    lbracket = 8,
    rbracket = 9,
    lparen = 10,
    rparen = 11,
    comma = 12,
    op = 13,
    ident = 14,
    string = 15,
    number = 16,
    var_ref = 17,
    invalid = 18,
}

/// 零拷贝 W3C XPath 词法单元
pub struct XPathToken {
    kind: XPathTokenKind,
    lexeme: []const u8,
}

/// 零拷贝 W3C XPath 位置路径步进（Location Step）
pub struct XPathStep {
    /// 是否以 `//`（descendant-or-self 简写）起始
    is_descendant: bool,
    /// 可选轴名称（如 `"child"`, `"attribute"`, `"descendant"`；无显式轴时为 `null`）
    axis: ([]const u8)?,
    /// 节点测试（如 `"item"`, `"*"`, `"ns:tag"`, `"text()"`, `".."`, `"."`）
    node_test: []const u8,
    /// 可选首个谓词 `[...]` 内部切片（不含外层 `[` 与 `]`）
    predicate: ([]const u8)?,
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

fn isNameStart(c: u8): bool {
    return isAsciiAlpha(c) || c == '_' || c >= 0x80;
}

fn isNcNameChar(c: u8): bool {
    return isAsciiAlpha(c) || isDigit(c) || c == '_' || c == '-' || c >= 0x80;
}

fn scanNcName(input: []const u8, start: usize): usize {
    let i = start;
    if (i >= input.len || !isNameStart(input[i])) return start;
    i += 1;
    while (i < input.len) {
        const c = input[i];
        if (isNcNameChar(c)) {
            i += 1;
            continue;
        }
        // Single '.' allowed inside XML NCName, but stop before '..'
        if (c == '.' && !(i + 1 < input.len && input[i + 1] == '.')) {
            i += 1;
            continue;
        }
        break;
    }
    return i;
}

fn scanQNameOrWildcard(input: []const u8, start: usize): usize {
    let i = scanNcName(input, start);
    if (i == start) return start;
    // Namespace prefix `ns:local` or `ns:*` (must NOT consume `::` axis separator)
    if (i < input.len && input[i] == ':' && !(i + 1 < input.len && input[i + 1] == ':')) {
        if (i + 1 < input.len && input[i + 1] == '*') {
            return i + 2;
        }
        if (i + 1 < input.len && isNameStart(input[i + 1])) {
            return scanNcName(input, i + 1);
        }
    }
    return i;
}

/// 零堆分配 W3C XPath 流式词法扫描器
pub struct XPathIter {
    expr: []const u8,
    pos: usize = 0,

    pub static fn init(expr: []const u8): XPathIter {
        return XPathIter {
            expr: expr,
            pos: 0,
        };
    }

    fn skipWs(*_) {
        while (_.pos < _.expr.len && isSpace(_.expr[_.pos])) {
            _.pos += 1;
        }
    }

    fn scanString(*_, quote: u8): XPathToken {
        const start = _.pos;
        _.pos += 1;
        while (_.pos < _.expr.len) {
            if (_.expr[_.pos] == quote) {
                _.pos += 1;
                return XPathToken {
                    kind: XPathTokenKind.string,
                    lexeme: _.expr[start.._.pos],
                };
            }
            _.pos += 1;
        }
        return XPathToken {
            kind: XPathTokenKind.invalid,
            lexeme: _.expr[start.._.pos],
        };
    }

    fn scanNumber(*_): XPathToken {
        const start = _.pos;
        if (_.expr[_.pos] == '.') {
            _.pos += 1;
            while (_.pos < _.expr.len && isDigit(_.expr[_.pos])) {
                _.pos += 1;
            }
        } else {
            while (_.pos < _.expr.len && isDigit(_.expr[_.pos])) {
                _.pos += 1;
            }
            if (_.pos < _.expr.len && _.expr[_.pos] == '.'
                && !(_.pos + 1 < _.expr.len && _.expr[_.pos + 1] == '.')) {
                if (_.pos + 1 < _.expr.len && isDigit(_.expr[_.pos + 1])) {
                    _.pos += 1;
                    while (_.pos < _.expr.len && isDigit(_.expr[_.pos])) {
                        _.pos += 1;
                    }
                }
            }
        }

        // Optional exponent for XPath 2.0 double literals
        if (_.pos < _.expr.len && (_.expr[_.pos] == 'e' || _.expr[_.pos] == 'E')) {
            let exp_pos = _.pos + 1;
            if (exp_pos < _.expr.len && (_.expr[exp_pos] == '+' || _.expr[exp_pos] == '-')) {
                exp_pos += 1;
            }
            if (exp_pos < _.expr.len && isDigit(_.expr[exp_pos])) {
                _.pos = exp_pos;
                while (_.pos < _.expr.len && isDigit(_.expr[_.pos])) {
                    _.pos += 1;
                }
            }
        }

        return XPathToken {
            kind: XPathTokenKind.number,
            lexeme: _.expr[start.._.pos],
        };
    }

    /// 返回下一个词法单元；到达输入末尾时返回 `null`
    pub fn next(*_): XPathToken? {
        _.skipWs();
        if (_.pos >= _.expr.len) return null;

        const start = _.pos;
        const c = _.expr[_.pos];

        // Multi-char tokens: //, .., ::, !=, <=, >=
        if (c == '/') {
            if (_.pos + 1 < _.expr.len && _.expr[_.pos + 1] == '/') {
                _.pos += 2;
                return XPathToken { kind: XPathTokenKind.dslash, lexeme: _.expr[start.._.pos] };
            }
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.slash, lexeme: _.expr[start.._.pos] };
        }

        if (c == '.') {
            if (_.pos + 1 < _.expr.len && _.expr[_.pos + 1] == '.') {
                _.pos += 2;
                return XPathToken { kind: XPathTokenKind.ddot, lexeme: _.expr[start.._.pos] };
            }
            if (_.pos + 1 < _.expr.len && isDigit(_.expr[_.pos + 1])) {
                return _.scanNumber();
            }
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.dot, lexeme: _.expr[start.._.pos] };
        }

        if (c == ':') {
            if (_.pos + 1 < _.expr.len && _.expr[_.pos + 1] == ':') {
                _.pos += 2;
                return XPathToken { kind: XPathTokenKind.axis, lexeme: _.expr[start.._.pos] };
            }
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.invalid, lexeme: _.expr[start.._.pos] };
        }

        if (c == '@') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.at, lexeme: _.expr[start.._.pos] };
        }
        if (c == '*') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.star, lexeme: _.expr[start.._.pos] };
        }
        if (c == '|') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.pipe, lexeme: _.expr[start.._.pos] };
        }
        if (c == '[') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.lbracket, lexeme: _.expr[start.._.pos] };
        }
        if (c == ']') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.rbracket, lexeme: _.expr[start.._.pos] };
        }
        if (c == '(') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.lparen, lexeme: _.expr[start.._.pos] };
        }
        if (c == ')') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.rparen, lexeme: _.expr[start.._.pos] };
        }
        if (c == ',') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.comma, lexeme: _.expr[start.._.pos] };
        }

        // Operators: !=, <=, >=, =, <, >, +, -
        if (c == '!') {
            if (_.pos + 1 < _.expr.len && _.expr[_.pos + 1] == '=') {
                _.pos += 2;
                return XPathToken { kind: XPathTokenKind.op, lexeme: _.expr[start.._.pos] };
            }
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.invalid, lexeme: _.expr[start.._.pos] };
        }
        if (c == '<' || c == '>') {
            if (_.pos + 1 < _.expr.len && _.expr[_.pos + 1] == '=') {
                _.pos += 2;
                return XPathToken { kind: XPathTokenKind.op, lexeme: _.expr[start.._.pos] };
            }
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.op, lexeme: _.expr[start.._.pos] };
        }
        if (c == '=' || c == '+' || c == '-') {
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.op, lexeme: _.expr[start.._.pos] };
        }

        // String literal ('...' or "...")
        if (c == '\'' || c == '"') {
            return _.scanString(c);
        }

        // Variable reference ($name or $ns:name)
        if (c == '$') {
            if (_.pos + 1 < _.expr.len && isNameStart(_.expr[_.pos + 1])) {
                let end = scanNcName(_.expr, _.pos + 1);
                if (end < _.expr.len && _.expr[end] == ':'
                    && !(end + 1 < _.expr.len && _.expr[end + 1] == ':')
                    && end + 1 < _.expr.len && isNameStart(_.expr[end + 1])) {
                    end = scanNcName(_.expr, end + 1);
                }
                _.pos = end;
                return XPathToken {
                    kind: XPathTokenKind.var_ref,
                    lexeme: _.expr[start.._.pos],
                };
            }
            _.pos += 1;
            return XPathToken { kind: XPathTokenKind.invalid, lexeme: _.expr[start.._.pos] };
        }

        // Number literal
        if (isDigit(c)) {
            return _.scanNumber();
        }

        // Identifier / QName / ns:*
        if (isNameStart(c)) {
            _.pos = scanQNameOrWildcard(_.expr, _.pos);
            return XPathToken {
                kind: XPathTokenKind.ident,
                lexeme: _.expr[start.._.pos],
            };
        }

        _.pos += 1;
        return XPathToken {
            kind: XPathTokenKind.invalid,
            lexeme: _.expr[start.._.pos],
        };
    }
}

/// 零堆分配 W3C XPath 位置路径步进迭代器（自动跳过谓词内部 `/` 与字符串）
pub struct XPathStepIter {
    expr: []const u8,
    pos: usize = 0,

    pub static fn init(expr: []const u8): XPathStepIter {
        return XPathStepIter {
            expr: expr,
            pos: 0,
        };
    }

    fn skipWs(*_) {
        while (_.pos < _.expr.len && isSpace(_.expr[_.pos])) {
            _.pos += 1;
        }
    }

    fn scanBalanced(*_, open: u8, close: u8): usize? {
        if (_.pos >= _.expr.len || _.expr[_.pos] != open) return null;
        let i = _.pos + 1;
        let depth: usize = 1;
        while (i < _.expr.len) {
            const c = _.expr[i];
            if (c == '\'' || c == '"') {
                const q = c;
                i += 1;
                while (i < _.expr.len && _.expr[i] != q) {
                    i += 1;
                }
                if (i >= _.expr.len) return null;
                i += 1;
                continue;
            }
            if (c == open) {
                if (depth >= 64) return null;
                depth += 1;
            } else if (c == close) {
                depth -= 1;
                if (depth == 0) {
                    _.pos = i + 1;
                    return i;
                }
            }
            i += 1;
        }
        return null;
    }

    /// 解析并返回下一个位置路径步进；到达路径末尾或遇到非法结构时返回 `null`
    pub fn next(*_): XPathStep? {
        _.skipWs();
        if (_.pos >= _.expr.len) return null;

        let is_descendant = false;
        if (_.expr[_.pos] == '/') {
            if (_.pos + 1 < _.expr.len && _.expr[_.pos + 1] == '/') {
                is_descendant = true;
                _.pos += 2;
            } else {
                _.pos += 1;
            }
            _.skipWs();
            if (_.pos >= _.expr.len) return null;
        }

        // Optional axis: `@` shorthand or `axis-name::`
        let axis: ([]const u8)? = null;
        if (_.expr[_.pos] == '@') {
            axis = "attribute";
            _.pos += 1;
            _.skipWs();
            if (_.pos >= _.expr.len) return null;
        } else if (isNameStart(_.expr[_.pos])) {
            const axis_end = scanNcName(_.expr, _.pos);
            let look = axis_end;
            while (look < _.expr.len && isSpace(_.expr[look])) {
                look += 1;
            }
            if (look + 1 < _.expr.len && _.expr[look] == ':' && _.expr[look + 1] == ':') {
                axis = _.expr[_.pos..axis_end];
                _.pos = look + 2;
                _.skipWs();
                if (_.pos >= _.expr.len) return null;
            }
        }

        // Node test: `..`, `.`, `*`, QName, `ns:*`, or node-type test `text()` / `node()` / `processing-instruction(...)`
        const nt_start = _.pos;
        const c = _.expr[_.pos];
        if (c == '.') {
            if (_.pos + 1 < _.expr.len && _.expr[_.pos + 1] == '.') {
                _.pos += 2;
            } else {
                _.pos += 1;
            }
        } else if (c == '*') {
            _.pos += 1;
        } else if (isNameStart(c)) {
            _.pos = scanQNameOrWildcard(_.expr, _.pos);
            // Node-type test such as text(), node(), comment(), processing-instruction('pi')
            let look = _.pos;
            while (look < _.expr.len && isSpace(_.expr[look])) {
                look += 1;
            }
            if (look < _.expr.len && _.expr[look] == '(') {
                _.pos = look;
                if (_.scanBalanced('(', ')') == null) {
                    _.pos = _.expr.len;
                    return null;
                }
            }
        } else {
            _.pos = _.expr.len;
            return null;
        }

        const node_test = _.expr[nt_start.._.pos];
        if (node_test.len == 0) return null;

        // Zero or more predicates `[...]`
        let predicate: ([]const u8)? = null;
        _.skipWs();
        while (_.pos < _.expr.len && _.expr[_.pos] == '[') {
            const pred_start = _.pos + 1;
            if (close_idx := _.scanBalanced('[', ']')) {
                if (predicate == null) {
                    predicate = _.expr[pred_start..close_idx];
                }
            } else {
                _.pos = _.expr.len;
                return null;
            }
            _.skipWs();
        }

        // Stop if trailing non-slash delimiter (e.g. union `|` or stray token)
        if (_.pos < _.expr.len && _.expr[_.pos] != '/') {
            _.pos = _.expr.len;
        }

        return XPathStep {
            is_descendant: is_descendant,
            axis: axis,
            node_test: node_test,
            predicate: predicate,
        };
    }
}

/// 字符串感知校验 XPath 表达式中 `[]`/`()` 平衡、无未闭合引号与 64 层深度上限
pub fn validateXPathDelimiters(expr: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let lex = XPathIter.init(expr);

    while (tok := lex.next()) {
        if (tok.kind == XPathTokenKind.invalid) {
            return false;
        }
        if (tok.kind == XPathTokenKind.lbracket) {
            if (depth >= 64) return false;
            stack[depth] = '[';
            depth += 1;
        } else if (tok.kind == XPathTokenKind.lparen) {
            if (depth >= 64) return false;
            stack[depth] = '(';
            depth += 1;
        } else if (tok.kind == XPathTokenKind.rbracket) {
            if (depth == 0 || stack[depth - 1] != '[') return false;
            depth -= 1;
        } else if (tok.kind == XPathTokenKind.rparen) {
            if (depth == 0 || stack[depth - 1] != '(') return false;
            depth -= 1;
        }
    }

    return depth == 0;
}
