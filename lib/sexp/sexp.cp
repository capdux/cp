// Source: Absorbed from /data/dev/bun/src/ | Synced: 2026-11-21T17:33:13Z (Zero-Alloc S-Expression Lexer & Formatter)

/// S-Expression 词法单元类型
pub enum SexpTokenKind: u8 {
    lparen = 0,
    rparen = 1,
    atom = 2,
    string = 3,
    invalid = 4,
}

/// 零拷贝 S-Expression 词法单元（`lexeme` 直接切片指向原始输入）
pub struct SexpToken {
    kind: SexpTokenKind,
    lexeme: []const u8,
}

fn isWhitespace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAtomDelimiter(c: u8): bool {
    return isWhitespace(c) || c == '(' || c == ')' || c == ';' || c == '"';
}

/// 零堆分配 S-Expression 流式词法扫描器
pub struct SexpIter {
    input: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): SexpIter {
        return SexpIter {
            input: input,
            pos: 0,
        };
    }

    fn skipWhitespaceAndComments(*_) {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (isWhitespace(c)) {
                _.pos += 1;
            } else if (c == ';') {
                while (_.pos < _.input.len && _.input[_.pos] != '\n') {
                    _.pos += 1;
                }
            } else {
                break;
            }
        }
    }

    /// 读取下一个词法单元；到达输入末尾返回 `null`
    pub fn next(*_): SexpToken? {
        _.skipWhitespaceAndComments();
        if (_.pos >= _.input.len) {
            return null;
        }

        const start = _.pos;
        const c = _.input[start];

        if (c == '(') {
            _.pos += 1;
            return SexpToken {
                kind: SexpTokenKind.lparen,
                lexeme: _.input[start.._.pos],
            };
        }

        if (c == ')') {
            _.pos += 1;
            return SexpToken {
                kind: SexpTokenKind.rparen,
                lexeme: _.input[start.._.pos],
            };
        }

        if (c == '"') {
            _.pos += 1;
            let closed = false;
            while (_.pos < _.input.len) {
                const ch = _.input[_.pos];
                if (ch == '\\') {
                    if (_.pos + 1 >= _.input.len) {
                        _.pos = _.input.len;
                        return SexpToken {
                            kind: SexpTokenKind.invalid,
                            lexeme: _.input[start.._.pos],
                        };
                    }
                    _.pos += 2;
                } else if (ch == '"') {
                    _.pos += 1;
                    closed = true;
                    break;
                } else {
                    _.pos += 1;
                }
            }
            if (!closed) {
                return SexpToken {
                    kind: SexpTokenKind.invalid,
                    lexeme: _.input[start.._.pos],
                };
            }
            return SexpToken {
                kind: SexpTokenKind.string,
                lexeme: _.input[start.._.pos],
            };
        }

        // Atom (symbol / number / operator / keyword)
        while (_.pos < _.input.len && !isAtomDelimiter(_.input[_.pos])) {
            _.pos += 1;
        }
        return SexpToken {
            kind: SexpTokenKind.atom,
            lexeme: _.input[start.._.pos],
        };
    }
}

/// 计算 S-Expression 的最大括号嵌套深度；语法非法、括号失衡或无有效 token 时返回 `null`
pub fn maxSexpDepth(input: []const u8): usize? {
    let it = SexpIter.init(input);
    let depth: usize = 0;
    let max_depth: usize = 0;
    let count: usize = 0;

    while (tok := it.next()) {
        count += 1;
        if (tok.kind == SexpTokenKind.invalid) {
            return null;
        } else if (tok.kind == SexpTokenKind.lparen) {
            depth += 1;
            if (depth > max_depth) {
                max_depth = depth;
            }
        } else if (tok.kind == SexpTokenKind.rparen) {
            if (depth == 0) {
                return null;
            }
            depth -= 1;
        }
    }

    if (depth != 0 || count == 0) {
        return null;
    }
    return max_depth;
}

/// 校验 S-Expression 语法合法性（括号平衡、无未闭合字符串、至少含 1 个有效 token）
pub fn validateSexp(input: []const u8): bool {
    return maxSexpDepth(input) != null;
}

/// 去除注释与多余空白并输出紧凑规范 S-Expression；语法非法或输出缓冲不足返回 `null`
pub fn formatSexpCompact(input: []const u8, out: []u8): ([]const u8)? {
    let it = SexpIter.init(input);
    let depth: usize = 0;
    let count: usize = 0;
    let out_pos: usize = 0;
    let has_prev = false;
    let prev_kind = SexpTokenKind.invalid;

    while (tok := it.next()) {
        count += 1;
        if (tok.kind == SexpTokenKind.invalid) {
            return null;
        } else if (tok.kind == SexpTokenKind.lparen) {
            depth += 1;
        } else if (tok.kind == SexpTokenKind.rparen) {
            if (depth == 0) {
                return null;
            }
            depth -= 1;
        }

        if (has_prev && prev_kind != SexpTokenKind.lparen && tok.kind != SexpTokenKind.rparen) {
            if (out_pos >= out.len) {
                return null;
            }
            out[out_pos] = ' ';
            out_pos += 1;
        }

        if (out_pos + tok.lexeme.len > out.len) {
            return null;
        }
        let i: usize = 0;
        while (i < tok.lexeme.len) {
            out[out_pos + i] = tok.lexeme[i];
            i += 1;
        }
        out_pos += tok.lexeme.len;
        has_prev = true;
        prev_kind = tok.kind;
    }

    if (depth != 0 || count == 0) {
        return null;
    }
    return out[0..out_pos];
}
