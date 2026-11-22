// Origin: Pure-CP Stdlib (Zero-Alloc Nginx Config Lexer & Block Scanner) | Created: 2026-11-22

/// Nginx 配置词法 Token 类型
pub enum NginxTokenKind {
    word,
    string,
    lbrace,
    rbrace,
    semicolon,
    invalid,
}

/// Nginx 配置词法 Token（零拷贝指向原始输入）
pub struct NginxToken {
    kind: NginxTokenKind,
    lexeme: []const u8,
}

/// Nginx 指令或块声明（所有切片均零拷贝指向原始输入）
pub struct NginxDirective {
    name: []const u8,
    args: []const u8,
    is_block: bool,
    body: ([]const u8)?,
    depth: u16,
}

const MAX_BRACE_DEPTH: usize = 64;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

/// 跳过空白与 `#` 行注释
fn skipWsAndComments(s: []const u8, start: usize): usize {
    let i = start;
    while (i < s.len) {
        if (isSpace(s[i])) {
            i += 1;
        } else if (s[i] == '#') {
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
        } else {
            break;
        }
    }
    return i;
}

/// 扫描单引号或双引号字符串（支持 `\` 转义）；未闭合返回 `null`
fn scanQuotedString(s: []const u8, start: usize): usize? {
    if (start >= s.len) return null;
    const quote = s[start];
    if (quote != '"' && quote != '\'') return null;
    let i = start + 1;
    while (i < s.len) {
        if (s[i] == '\\' && i + 1 < s.len) {
            i += 2;
            continue;
        }
        if (s[i] == quote) {
            return i + 1;
        }
        i += 1;
    }
    return null;
}

/// 扫描 `${var}` 插值片段，返回闭合 `}` 之后的位置；未闭合返回 `null`
fn scanVarBraces(s: []const u8, dollar_pos: usize): usize? {
    if (dollar_pos + 1 >= s.len || s[dollar_pos] != '$' || s[dollar_pos + 1] != '{') {
        return null;
    }
    let i = dollar_pos + 2;
    while (i < s.len) {
        if (s[i] == '}') {
            return i + 1;
        }
        if (s[i] == '\n' || s[i] == '\r' || s[i] == '{' || s[i] == ';') {
            return null;
        }
        i += 1;
    }
    return null;
}

/// 从 `open_pos`（`'{'`）扫描到匹配的 `'}'`（感知单/双引号、`${var}`、`\` 转义与 `#` 注释，最大深度 64）
fn findMatchingBrace(s: []const u8, open_pos: usize): usize? {
    if (open_pos >= s.len || s[open_pos] != '{') {
        return null;
    }
    let i = open_pos + 1;
    let depth: usize = 1;
    let prev_space = true;

    while (i < s.len) {
        const c = s[i];
        if (isSpace(c)) {
            prev_space = true;
            i += 1;
            continue;
        }
        if (c == '#' && prev_space) {
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
            prev_space = true;
            continue;
        }
        if (c == '"' || c == '\'') {
            if (end_pos := scanQuotedString(s, i)) {
                i = end_pos;
                prev_space = false;
                continue;
            } else {
                return null;
            }
        }
        if (c == '\\' && i + 1 < s.len) {
            i += 2;
            prev_space = false;
            continue;
        }
        if (c == '$' && i + 1 < s.len && s[i + 1] == '{') {
            if (end_var := scanVarBraces(s, i)) {
                i = end_var;
                prev_space = false;
                continue;
            } else {
                return null;
            }
        }
        if (c == '{') {
            if (depth >= MAX_BRACE_DEPTH) {
                return null;
            }
            depth += 1;
            prev_space = true;
        } else if (c == '}') {
            depth -= 1;
            if (depth == 0) {
                return i;
            }
            prev_space = true;
        } else {
            prev_space = c == ';';
        }
        i += 1;
    }
    return null;
}

/// 零堆分配 Nginx 配置词法扫描迭代器
pub struct NginxIter {
    input: []const u8,
    pos: usize = 0,

    static fn init(input: []const u8): NginxIter {
        return NginxIter {
            input: input,
            pos: 0,
        };
    }

    /// 读取下一个词法 Token；到达输入末尾返回 `null`
    fn next(*_): NginxToken? {
        _.pos = skipWsAndComments(_.input, _.pos);
        if (_.pos >= _.input.len) {
            return null;
        }

        const start = _.pos;
        const c = _.input[_.pos];

        if (c == '{') {
            _.pos += 1;
            return NginxToken {
                kind: NginxTokenKind.lbrace,
                lexeme: _.input[start.._.pos],
            };
        }
        if (c == '}') {
            _.pos += 1;
            return NginxToken {
                kind: NginxTokenKind.rbrace,
                lexeme: _.input[start.._.pos],
            };
        }
        if (c == ';') {
            _.pos += 1;
            return NginxToken {
                kind: NginxTokenKind.semicolon,
                lexeme: _.input[start.._.pos],
            };
        }

        // 单引号或双引号字符串
        if (c == '"' || c == '\'') {
            if (end_pos := scanQuotedString(_.input, _.pos)) {
                _.pos = end_pos;
                return NginxToken {
                    kind: NginxTokenKind.string,
                    lexeme: _.input[start.._.pos],
                };
            } else {
                _.pos = _.input.len;
                return NginxToken {
                    kind: NginxTokenKind.invalid,
                    lexeme: _.input[start.._.pos],
                };
            }
        }

        // 常规 word（支持 `$var`、`${var}`、正则、路径与 `\` 转义）
        while (_.pos < _.input.len) {
            const ch = _.input[_.pos];
            if (isSpace(ch) || ch == ';' || ch == '{' || ch == '}') {
                break;
            }
            if (ch == '\\' && _.pos + 1 < _.input.len) {
                _.pos += 2;
                continue;
            }
            if (ch == '$' && _.pos + 1 < _.input.len && _.input[_.pos + 1] == '{') {
                if (end_var := scanVarBraces(_.input, _.pos)) {
                    _.pos = end_var;
                    continue;
                } else {
                    _.pos = _.input.len;
                    return NginxToken {
                        kind: NginxTokenKind.invalid,
                        lexeme: _.input[start.._.pos],
                    };
                }
            }
            if (ch == '"' || ch == '\'') {
                if (end_str := scanQuotedString(_.input, _.pos)) {
                    _.pos = end_str;
                    continue;
                } else {
                    _.pos = _.input.len;
                    return NginxToken {
                        kind: NginxTokenKind.invalid,
                        lexeme: _.input[start.._.pos],
                    };
                }
            }
            _.pos += 1;
        }

        if (_.pos == start) {
            _.pos += 1;
            return NginxToken {
                kind: NginxTokenKind.invalid,
                lexeme: _.input[start.._.pos],
            };
        }

        return NginxToken {
            kind: NginxTokenKind.word,
            lexeme: _.input[start.._.pos],
        };
    }
}

/// 零堆分配 Nginx 当前层级指令与块迭代器（支持通过 `body` 递归遍历子块）
pub struct NginxBlockIter {
    input: []const u8,
    pos: usize = 0,
    depth: u16 = 0,

    static fn init(input: []const u8): NginxBlockIter {
        return NginxBlockIter {
            input: input,
            pos: 0,
            depth: 0,
        };
    }

    static fn initWithDepth(input: []const u8, depth: u16): NginxBlockIter {
        return NginxBlockIter {
            input: input,
            pos: 0,
            depth: depth,
        };
    }

    /// 读取下一条指令或块声明；到达末尾或遇到畸形未闭合结构返回 `null`
    fn next(*_): NginxDirective? {
        while (_.pos < _.input.len) {
            _.pos = skipWsAndComments(_.input, _.pos);
            if (_.pos >= _.input.len) {
                return null;
            }
            if (_.input[_.pos] == ';') {
                _.pos += 1;
                continue;
            }
            if (_.input[_.pos] == '}' || _.input[_.pos] == '{') {
                _.pos = _.input.len;
                return null;
            }

            // 1. 读取指令名
            let lex = NginxIter {
                input: _.input,
                pos: _.pos,
            };
            let name_tok = NginxToken {
                kind: NginxTokenKind.invalid,
                lexeme: "",
            };
            if (tok := lex.next()) {
                name_tok = tok;
            } else {
                _.pos = _.input.len;
                return null;
            }
            if (name_tok.kind != NginxTokenKind.word && name_tok.kind != NginxTokenKind.string) {
                _.pos = _.input.len;
                return null;
            }
            const name = name_tok.lexeme;
            const args_start = lex.pos;

            // 2. 扫描参数直到当前层级的 `;` 或 `{`
            let i = args_start;
            let term_ch: u8 = 0;
            let term_pos: usize = args_start;
            let prev_space = true;

            while (i < _.input.len) {
                const c = _.input[i];
                if (isSpace(c)) {
                    prev_space = true;
                    i += 1;
                    continue;
                }
                if (c == '#' && prev_space) {
                    while (i < _.input.len && _.input[i] != '\n') {
                        i += 1;
                    }
                    prev_space = true;
                    continue;
                }
                if (c == '"' || c == '\'') {
                    if (end_str := scanQuotedString(_.input, i)) {
                        i = end_str;
                        prev_space = false;
                        continue;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                }
                if (c == '\\' && i + 1 < _.input.len) {
                    i += 2;
                    prev_space = false;
                    continue;
                }
                if (c == '$' && i + 1 < _.input.len && _.input[i + 1] == '{') {
                    if (end_var := scanVarBraces(_.input, i)) {
                        i = end_var;
                        prev_space = false;
                        continue;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                }
                if (c == ';' || c == '{') {
                    term_ch = c;
                    term_pos = i;
                    break;
                }
                if (c == '}') {
                    _.pos = _.input.len;
                    return null;
                }
                prev_space = false;
                i += 1;
            }

            if (term_ch == 0) {
                _.pos = _.input.len;
                return null;
            }

            const args = trimAscii(_.input[args_start..term_pos]);

            if (term_ch == ';') {
                _.pos = term_pos + 1;
                return NginxDirective {
                    name: name,
                    args: args,
                    is_block: false,
                    body: null,
                    depth: _.depth,
                };
            }

            // 块指令 `{ ... }`
            let close_brace: usize = 0;
            if (cb := findMatchingBrace(_.input, term_pos)) {
                close_brace = cb;
            } else {
                _.pos = _.input.len;
                return null;
            }
            const body_slice = trimAscii(_.input[term_pos + 1 .. close_brace]);
            _.pos = close_brace + 1;
            return NginxDirective {
                name: name,
                args: args,
                is_block: true,
                body: body_slice,
                depth: _.depth,
            };
        }
        return null;
    }
}

/// 在当前块层级按指令名查找首个匹配参数切片（零堆分配）
pub fn findNginxDirective(block: []const u8, name: []const u8): ([]const u8)? {
    let it = NginxBlockIter.init(block);
    while (d := it.next()) {
        if (eqlBytes(d.name, name)) {
            return d.args;
        }
    }
    return null;
}

/// 字符串/注释感知校验 Nginx 配置 `{}` 平衡、64 层深度上限与无未闭合引号
pub fn validateNginxConfig(input: []const u8): bool {
    let depth: usize = 0;
    let i: usize = 0;
    let prev_space = true;

    while (i < input.len) {
        const c = input[i];

        if (isSpace(c)) {
            prev_space = true;
            i += 1;
            continue;
        }
        if (c == '#' && prev_space) {
            while (i < input.len && input[i] != '\n') {
                i += 1;
            }
            prev_space = true;
            continue;
        }
        if (c == '"' || c == '\'') {
            if (end_str := scanQuotedString(input, i)) {
                i = end_str;
                prev_space = false;
                continue;
            } else {
                return false;
            }
        }
        if (c == '\\' && i + 1 < input.len) {
            i += 2;
            prev_space = false;
            continue;
        }
        if (c == '$' && i + 1 < input.len && input[i + 1] == '{') {
            if (end_var := scanVarBraces(input, i)) {
                i = end_var;
                prev_space = false;
                continue;
            } else {
                return false;
            }
        }
        if (c == '{') {
            if (depth >= MAX_BRACE_DEPTH) {
                return false;
            }
            depth += 1;
            prev_space = true;
        } else if (c == '}') {
            if (depth == 0) {
                return false;
            }
            depth -= 1;
            prev_space = true;
        } else {
            prev_space = c == ';';
        }
        i += 1;
    }

    return depth == 0;
}
