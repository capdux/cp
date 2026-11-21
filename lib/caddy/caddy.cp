// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Caddyfile Lexer & Site Block Scanner)

/// Caddyfile 词法单元类型
pub enum CaddyTokenKind: u8 {
    word = 0,
    string = 1,
    matcher = 2,
    lbrace = 3,
    rbrace = 4,
    newline = 5,
    invalid = 6,
}

/// 零拷贝 Caddyfile 词法单元
pub struct CaddyToken {
    kind: CaddyTokenKind,
    lexeme: []const u8,
}

/// 零拷贝 Caddyfile 顶层块元数据（全局选项块 / Snippet / 站点块）
pub struct CaddySiteBlock {
    /// 站点地址头切片（如 `"example.com, www.example.com"`、`":8080"`、`"(common)"`；全局块为空 `""`）
    addresses: []const u8,
    /// 是否为无地址顶层 `{ ... }` 全局选项块
    is_global: bool,
    /// 是否为 `(name) { ... }` 可复用 Snippet 块
    is_snippet: bool,
    /// `{ ... }` 内部主体切片（不含外层 `{` 与 `}`）
    body: []const u8,
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isAnySpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isTokenBoundary(input: []const u8, idx: usize): bool {
    if (idx >= input.len) return true;
    const c = input[idx];
    return isAnySpace(c) || c == '#';
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isAnySpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isAnySpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn isSnippetHeader(addr: []const u8): bool {
    if (addr.len < 3) return false;
    if (addr[0] != '(' || addr[addr.len - 1] != ')') return false;
    let i: usize = 1;
    while (i + 1 < addr.len) {
        if (addr[i] == ')' || addr[i] == '(' || isAnySpace(addr[i]) || addr[i] == ',') {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 检查 `pos` 处的 `{` 是否为行内闭合占位符（如 `{host}`、`{$PORT}`）而非块打开 `{`
fn findInlinePlaceholderClose(input: []const u8, pos: usize): usize? {
    if (pos >= input.len || input[pos] != '{') return null;
    if (pos + 1 >= input.len || isAnySpace(input[pos + 1]) || input[pos + 1] == '#') {
        return null;
    }
    let i = pos + 1;
    while (i < input.len) {
        const c = input[i];
        if (c == '}') return i;
        if (isAnySpace(c) || c == '{') return null;
        i += 1;
    }
    return null;
}

/// 零堆分配 Caddyfile 流式词法扫描器
pub struct CaddyIter {
    input: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): CaddyIter {
        return CaddyIter {
            input: input,
            pos: 0,
        };
    }

    fn skipHorizWsAndComment(*_) {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (isHorizSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                _.pos += 1;
                while (_.pos < _.input.len && _.input[_.pos] != '\n' && _.input[_.pos] != '\r') {
                    _.pos += 1;
                }
                break;
            }
            break;
        }
    }

    fn scanDoubleQuoted(*_): CaddyToken {
        const start = _.pos;
        _.pos += 1;
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (c == '\\') {
                if (_.pos + 1 < _.input.len) {
                    _.pos += 2;
                } else {
                    _.pos = _.input.len;
                    return CaddyToken {
                        kind: CaddyTokenKind.invalid,
                        lexeme: _.input[start.._.pos],
                    };
                }
                continue;
            }
            if (c == '"') {
                _.pos += 1;
                return CaddyToken {
                    kind: CaddyTokenKind.string,
                    lexeme: _.input[start.._.pos],
                };
            }
            _.pos += 1;
        }
        return CaddyToken {
            kind: CaddyTokenKind.invalid,
            lexeme: _.input[start.._.pos],
        };
    }

    fn scanBacktick(*_): CaddyToken {
        const start = _.pos;
        _.pos += 1;
        while (_.pos < _.input.len) {
            if (_.input[_.pos] == '`') {
                _.pos += 1;
                return CaddyToken {
                    kind: CaddyTokenKind.string,
                    lexeme: _.input[start.._.pos],
                };
            }
            _.pos += 1;
        }
        return CaddyToken {
            kind: CaddyTokenKind.invalid,
            lexeme: _.input[start.._.pos],
        };
    }

    fn scanWordTail(*_) {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (isAnySpace(c)) break;
            if (c == '{') {
                if (close_idx := findInlinePlaceholderClose(_.input, _.pos)) {
                    _.pos = close_idx + 1;
                    continue;
                }
                // Trailing block-opening '{' attached to word (e.g. `site{`)
                if (isTokenBoundary(_.input, _.pos + 1)) {
                    break;
                }
            }
            if (c == '}' && isTokenBoundary(_.input, _.pos + 1)) {
                break;
            }
            _.pos += 1;
        }
    }

    /// 返回下一个词法单元；到达输入末尾时返回 `null`
    pub fn next(*_): CaddyToken? {
        _.skipHorizWsAndComment();
        if (_.pos >= _.input.len) return null;

        const start = _.pos;
        const c = _.input[_.pos];

        // Newline delimiter (\r\n or \n or \r)
        if (c == '\r') {
            _.pos += 1;
            if (_.pos < _.input.len && _.input[_.pos] == '\n') {
                _.pos += 1;
            }
            return CaddyToken {
                kind: CaddyTokenKind.newline,
                lexeme: _.input[start.._.pos],
            };
        }
        if (c == '\n') {
            _.pos += 1;
            return CaddyToken {
                kind: CaddyTokenKind.newline,
                lexeme: _.input[start.._.pos],
            };
        }

        // Quoted strings ("..." with escapes, `...` raw)
        if (c == '"') {
            return _.scanDoubleQuoted();
        }
        if (c == '`') {
            return _.scanBacktick();
        }

        // Block opening '{' vs inline placeholder '{http.request.host}'
        if (c == '{') {
            if (close_idx := findInlinePlaceholderClose(_.input, _.pos)) {
                _.pos = close_idx + 1;
                _.scanWordTail();
                return CaddyToken {
                    kind: CaddyTokenKind.word,
                    lexeme: _.input[start.._.pos],
                };
            }
            _.pos += 1;
            return CaddyToken {
                kind: CaddyTokenKind.lbrace,
                lexeme: _.input[start.._.pos],
            };
        }

        // Block closing '}'
        if (c == '}') {
            _.pos += 1;
            return CaddyToken {
                kind: CaddyTokenKind.rbrace,
                lexeme: _.input[start.._.pos],
            };
        }

        // Named matcher (@name)
        if (c == '@') {
            _.pos += 1;
            _.scanWordTail();
            if (_.pos > start + 1) {
                return CaddyToken {
                    kind: CaddyTokenKind.matcher,
                    lexeme: _.input[start.._.pos],
                };
            }
            return CaddyToken {
                kind: CaddyTokenKind.word,
                lexeme: _.input[start.._.pos],
            };
        }

        // Generic word / address / directive / placeholder-embedded token
        _.scanWordTail();
        if (_.pos == start) {
            _.pos += 1;
            return CaddyToken {
                kind: CaddyTokenKind.invalid,
                lexeme: _.input[start.._.pos],
            };
        }
        return CaddyToken {
            kind: CaddyTokenKind.word,
            lexeme: _.input[start.._.pos],
        };
    }
}

/// 零堆分配 Caddyfile 顶层块（全局选项块 / Snippet / 站点块）流式迭代器
pub struct CaddySiteIter {
    input: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): CaddySiteIter {
        return CaddySiteIter {
            input: input,
            pos: 0,
        };
    }

    fn skipBlankAndComments(*_) {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (isAnySpace(c)) {
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

    /// 从当前块打开 `{` 之后扫描到配对的 `}`，返回 `}` 的字节索引并将 `_.pos` 推进至 `}` 之后
    fn scanBlockBody(*_): usize? {
        let depth: usize = 1;
        let lex = CaddyIter.init(_.input[_.pos.._.input.len]);
        const base = _.pos;

        while (tok := lex.next()) {
            if (tok.kind == CaddyTokenKind.invalid) {
                _.pos = _.input.len;
                return null;
            }
            if (tok.kind == CaddyTokenKind.lbrace) {
                if (depth >= 64) {
                    _.pos = _.input.len;
                    return null;
                }
                depth += 1;
            } else if (tok.kind == CaddyTokenKind.rbrace) {
                depth -= 1;
                if (depth == 0) {
                    const close_idx = base + lex.pos - 1;
                    _.pos = base + lex.pos;
                    return close_idx;
                }
            }
        }

        _.pos = _.input.len;
        return null;
    }

    /// 提取下一个顶层全局选项块、Snippet 或站点块
    pub fn next(*_): CaddySiteBlock? {
        _.skipBlankAndComments();
        if (_.pos >= _.input.len) return null;

        // Global options block: top-level `{ ... }` without site address
        if (_.input[_.pos] == '{' && findInlinePlaceholderClose(_.input, _.pos) == null) {
            _.pos += 1;
            const body_start = _.pos;
            const close_idx = _.scanBlockBody() ?? return null;
            return CaddySiteBlock {
                addresses: "",
                is_global: true,
                is_snippet: false,
                body: _.input[body_start..close_idx],
            };
        }

        // Site block or snippet: `<addresses...> { ... }`
        const addr_start = _.pos;
        let lex = CaddyIter.init(_.input[_.pos.._.input.len]);
        const base = _.pos;
        let found_lbrace = false;
        let lbrace_pos: usize = 0;

        while (tok := lex.next()) {
            if (tok.kind == CaddyTokenKind.invalid) {
                _.pos = _.input.len;
                return null;
            }
            if (tok.kind == CaddyTokenKind.lbrace) {
                found_lbrace = true;
                lbrace_pos = base + lex.pos - 1;
                _.pos = base + lex.pos;
                break;
            }
            if (tok.kind == CaddyTokenKind.rbrace) {
                _.pos = _.input.len;
                return null;
            }
        }

        if (!found_lbrace) {
            _.pos = _.input.len;
            return null;
        }

        const addresses = trimSpaces(_.input[addr_start..lbrace_pos]);
        const body_start = _.pos;
        const close_idx = _.scanBlockBody() ?? return null;

        return CaddySiteBlock {
            addresses: addresses,
            is_global: false,
            is_snippet: isSnippetHeader(addresses),
            body: _.input[body_start..close_idx],
        };
    }
}

/// 字符串/注释感知校验 Caddyfile `{}` 平衡、64 层深度上限与无未闭合引号
pub fn validateCaddyfile(input: []const u8): bool {
    let depth: usize = 0;
    let lex = CaddyIter.init(input);

    while (tok := lex.next()) {
        if (tok.kind == CaddyTokenKind.invalid) {
            return false;
        }
        if (tok.kind == CaddyTokenKind.lbrace) {
            if (depth >= 64) return false;
            depth += 1;
        } else if (tok.kind == CaddyTokenKind.rbrace) {
            if (depth == 0) return false;
            depth -= 1;
        }
    }

    return depth == 0;
}
