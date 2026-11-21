// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc W3C WebIDL Lexer & Decl Scanner)

/// W3C WebIDL 顶层声明类型
pub enum WebIdlDeclKind {
    interface_decl,
    interface_mixin,
    dictionary_decl,
    enum_decl,
    namespace_decl,
    typedef_decl,
    callback_decl,
    includes_decl,
}

/// W3C WebIDL 顶层声明（所有切片均零拷贝指向原始输入）
pub struct WebIdlDecl {
    kind: WebIdlDeclKind,
    is_partial: bool,
    name: []const u8,
    inherits: ([]const u8)?,
    ext_attrs: ([]const u8)?,
    body: ([]const u8)?,
}

/// W3C WebIDL 词法 Token 类型
pub enum WebIdlTokenKind {
    ident,
    string,
    number,
    punct,
    invalid,
}

/// W3C WebIDL 词法 Token（零拷贝指向原始输入）
pub struct WebIdlToken {
    kind: WebIdlTokenKind,
    lexeme: []const u8,
}

const MAX_BRACE_DEPTH: usize = 64;

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isIdentContinue(c: u8): bool {
    return isAlpha(c) || isDigit(c) || c == '_' || c == '-';
}

fn isPunctByte(c: u8): bool {
    return c == '{' || c == '}' || c == '(' || c == ')'
        || c == '[' || c == ']' || c == '<' || c == '>'
        || c == ';' || c == ':' || c == ',' || c == '='
        || c == '?' || c == '.' || c == '-' || c == '+'
        || c == '*' || c == '/' || c == '|' || c == '&'
        || c == '!' || c == '~';
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

/// 跳过空白与 `//` / `/* ... */` 注释；若块注释未闭合返回 `null`
fn skipWsAndComments(s: []const u8, start: usize): usize? {
    let i = start;
    while (i < s.len) {
        if (isSpace(s[i])) {
            i += 1;
        } else if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '/') {
            i += 2;
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
        } else if (i + 1 < s.len && s[i] == '/' && s[i + 1] == '*') {
            i += 2;
            let closed = false;
            while (i + 1 < s.len) {
                if (s[i] == '*' && s[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) {
                return null;
            }
        } else {
            break;
        }
    }
    return i;
}

/// 从 `open_pos` 扫描到匹配的 `close_ch`（感知字符串、注释与嵌套，最大深度 64）
fn findMatchingDelim(s: []const u8, open_pos: usize, open_ch: u8, close_ch: u8): usize? {
    if (open_pos >= s.len || s[open_pos] != open_ch) {
        return null;
    }
    let i = open_pos + 1;
    let depth: usize = 1;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            i += 1;
            let str_closed = false;
            while (i < s.len) {
                if (s[i] == '\\' && i + 1 < s.len) {
                    i += 2;
                    continue;
                }
                if (s[i] == '"') {
                    i += 1;
                    str_closed = true;
                    break;
                }
                i += 1;
            }
            if (!str_closed) return null;
            continue;
        }
        if (i + 1 < s.len && c == '/' && s[i + 1] == '/') {
            i += 2;
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
            continue;
        }
        if (i + 1 < s.len && c == '/' && s[i + 1] == '*') {
            i += 2;
            let com_closed = false;
            while (i + 1 < s.len) {
                if (s[i] == '*' && s[i + 1] == '/') {
                    i += 2;
                    com_closed = true;
                    break;
                }
                i += 1;
            }
            if (!com_closed) return null;
            continue;
        }
        if (c == open_ch) {
            if (depth >= MAX_BRACE_DEPTH) return null;
            depth += 1;
        } else if (c == close_ch) {
            depth -= 1;
            if (depth == 0) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

/// 在顶层查找结束分号 `;`（跳过字符串、注释与嵌套括号）
fn findTopLevelSemi(s: []const u8, start: usize): usize? {
    let i = start;
    let depth: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '"') {
            i += 1;
            let closed = false;
            while (i < s.len) {
                if (s[i] == '\\' && i + 1 < s.len) {
                    i += 2;
                    continue;
                }
                if (s[i] == '"') {
                    i += 1;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return null;
            continue;
        }
        if (i + 1 < s.len && c == '/' && s[i + 1] == '/') {
            i += 2;
            while (i < s.len && s[i] != '\n') {
                i += 1;
            }
            continue;
        }
        if (i + 1 < s.len && c == '/' && s[i + 1] == '*') {
            i += 2;
            let closed = false;
            while (i + 1 < s.len) {
                if (s[i] == '*' && s[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return null;
            continue;
        }
        if (c == '(' || c == '[' || c == '{' || c == '<') {
            if (depth >= MAX_BRACE_DEPTH) return null;
            depth += 1;
        } else if (c == ')' || c == ']' || c == '}' || c == '>') {
            if (depth == 0) return null;
            depth -= 1;
        } else if (c == ';' && depth == 0) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn readIdentAt(s: []const u8, start: usize): usize {
    let i = start;
    if (i >= s.len) return start;
    const c = s[i];
    if (isAlpha(c) || c == '_') {
        i += 1;
    } else if (c == '-' && i + 1 < s.len && isAlpha(s[i + 1])) {
        i += 2;
    } else {
        return start;
    }
    while (i < s.len && isIdentContinue(s[i])) {
        i += 1;
    }
    return i;
}

/// 零堆分配 W3C WebIDL 词法扫描迭代器
pub struct WebIdlIter {
    input: []const u8,
    pos: usize = 0,

    static fn init(input: []const u8): WebIdlIter {
        return WebIdlIter {
            input: input,
            pos: 0,
        };
    }

    /// 读取下一个词法 Token；到达输入末尾返回 `null`
    fn next(*_): WebIdlToken? {
        while (_.pos < _.input.len) {
            const c = _.input[_.pos];
            if (isSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (_.pos + 1 < _.input.len && c == '/' && _.input[_.pos + 1] == '/') {
                _.pos += 2;
                while (_.pos < _.input.len && _.input[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            if (_.pos + 1 < _.input.len && c == '/' && _.input[_.pos + 1] == '*') {
                const start = _.pos;
                _.pos += 2;
                let closed = false;
                while (_.pos + 1 < _.input.len) {
                    if (_.input[_.pos] == '*' && _.input[_.pos + 1] == '/') {
                        _.pos += 2;
                        closed = true;
                        break;
                    }
                    _.pos += 1;
                }
                if (!closed) {
                    _.pos = _.input.len;
                    return WebIdlToken {
                        kind: WebIdlTokenKind.invalid,
                        lexeme: _.input[start.._.input.len],
                    };
                }
                continue;
            }
            break;
        }

        if (_.pos >= _.input.len) {
            return null;
        }

        const start = _.pos;
        const c = _.input[_.pos];

        // 1. 双引号字符串字面量
        if (c == '"') {
            _.pos += 1;
            let closed = false;
            while (_.pos < _.input.len) {
                if (_.input[_.pos] == '\\' && _.pos + 1 < _.input.len) {
                    _.pos += 2;
                    continue;
                }
                if (_.input[_.pos] == '"') {
                    _.pos += 1;
                    closed = true;
                    break;
                }
                _.pos += 1;
            }
            if (!closed) {
                return WebIdlToken {
                    kind: WebIdlTokenKind.invalid,
                    lexeme: _.input[start.._.pos],
                };
            }
            return WebIdlToken {
                kind: WebIdlTokenKind.string,
                lexeme: _.input[start.._.pos],
            };
        }

        // 2. 数字字面量（整数、十六进制、浮点、可选负号）
        const is_neg_num = c == '-' && _.pos + 1 < _.input.len
            && (isDigit(_.input[_.pos + 1])
                || (_.input[_.pos + 1] == '.' && _.pos + 2 < _.input.len && isDigit(_.input[_.pos + 2])));
        const is_dot_num = c == '.' && _.pos + 1 < _.input.len && isDigit(_.input[_.pos + 1]);
        if (isDigit(c) || is_neg_num || is_dot_num) {
            if (c == '-') {
                _.pos += 1;
            }
            if (_.pos + 1 < _.input.len && _.input[_.pos] == '0'
                && (_.input[_.pos + 1] == 'x' || _.input[_.pos + 1] == 'X')) {
                _.pos += 2;
                const hex_start = _.pos;
                while (_.pos < _.input.len && isHexDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos == hex_start) {
                    return WebIdlToken {
                        kind: WebIdlTokenKind.invalid,
                        lexeme: _.input[start.._.pos],
                    };
                }
                return WebIdlToken {
                    kind: WebIdlTokenKind.number,
                    lexeme: _.input[start.._.pos],
                };
            }
            while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                _.pos += 1;
            }
            if (_.pos < _.input.len && _.input[_.pos] == '.'
                && !(_.pos + 1 < _.input.len && _.input[_.pos + 1] == '.')) {
                _.pos += 1;
                while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
            }
            if (_.pos < _.input.len && (_.input[_.pos] == 'e' || _.input[_.pos] == 'E')) {
                const exp_pos = _.pos;
                _.pos += 1;
                if (_.pos < _.input.len && (_.input[_.pos] == '+' || _.input[_.pos] == '-')) {
                    _.pos += 1;
                }
                const digits_start = _.pos;
                while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos == digits_start) {
                    _.pos = exp_pos;
                }
            }
            return WebIdlToken {
                kind: WebIdlTokenKind.number,
                lexeme: _.input[start.._.pos],
            };
        }

        // 3. 标识符
        const ident_end = readIdentAt(_.input, _.pos);
        if (ident_end > _.pos) {
            _.pos = ident_end;
            return WebIdlToken {
                kind: WebIdlTokenKind.ident,
                lexeme: _.input[start.._.pos],
            };
        }

        // 4. 多字符标点 `...`
        if (c == '.' && _.pos + 2 < _.input.len
            && _.input[_.pos + 1] == '.' && _.input[_.pos + 2] == '.') {
            _.pos += 3;
            return WebIdlToken {
                kind: WebIdlTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        // 5. 单字符标点
        if (isPunctByte(c)) {
            _.pos += 1;
            return WebIdlToken {
                kind: WebIdlTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        // 6. 非法字节
        _.pos += 1;
        return WebIdlToken {
            kind: WebIdlTokenKind.invalid,
            lexeme: _.input[start.._.pos],
        };
    }
}

/// 零堆分配 W3C WebIDL 顶层声明提取迭代器
pub struct WebIdlDeclIter {
    input: []const u8,
    pos: usize = 0,

    static fn init(input: []const u8): WebIdlDeclIter {
        return WebIdlDeclIter {
            input: input,
            pos: 0,
        };
    }

    /// 提取下一条顶层 WebIDL 声明；遍历结束或遇到无法恢复的未闭合结构返回 `null`
    fn next(*_): WebIdlDecl? {
        while (_.pos < _.input.len) {
            if (p0 := skipWsAndComments(_.input, _.pos)) {
                _.pos = p0;
            } else {
                _.pos = _.input.len;
                return null;
            }
            if (_.pos >= _.input.len) {
                return null;
            }
            // 跳过多余分号
            if (_.input[_.pos] == ';') {
                _.pos += 1;
                continue;
            }

            // 1. 可选前置扩展属性 `[Exposed=Window, ...]`
            let ext_attrs: ([]const u8)? = null;
            if (_.input[_.pos] == '[') {
                const attr_start = _.pos;
                let attr_end: usize = _.pos;
                while (_.pos < _.input.len && _.input[_.pos] == '[') {
                    if (close_idx := findMatchingDelim(_.input, _.pos, '[', ']')) {
                        attr_end = close_idx + 1;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                    if (next_p := skipWsAndComments(_.input, attr_end)) {
                        _.pos = next_p;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                }
                ext_attrs = _.input[attr_start..attr_end];
                if (_.pos >= _.input.len) {
                    return null;
                }
            }

            // 2. 读取首个标识符关键字
            const kw1_start = _.pos;
            const kw1_end = readIdentAt(_.input, _.pos);
            if (kw1_end == kw1_start) {
                _.pos += 1;
                continue;
            }
            let kw = _.input[kw1_start..kw1_end];
            _.pos = kw1_end;

            // 3. 可选 `partial` 修饰符
            let is_partial = false;
            if (eqlBytes(kw, "partial")) {
                is_partial = true;
                if (p_kw := skipWsAndComments(_.input, _.pos)) {
                    _.pos = p_kw;
                } else {
                    _.pos = _.input.len;
                    return null;
                }
                const kw2_start = _.pos;
                const kw2_end = readIdentAt(_.input, _.pos);
                if (kw2_end == kw2_start) {
                    _.pos = _.input.len;
                    return null;
                }
                kw = _.input[kw2_start..kw2_end];
                _.pos = kw2_end;
            }

            if (p_after_kw := skipWsAndComments(_.input, _.pos)) {
                _.pos = p_after_kw;
            } else {
                _.pos = _.input.len;
                return null;
            }

            // 4. 按声明关键字分支解析
            if (eqlBytes(kw, "interface") || eqlBytes(kw, "dictionary")
                || eqlBytes(kw, "enum") || eqlBytes(kw, "namespace")) {
                let kind = WebIdlDeclKind.interface_decl;
                if (eqlBytes(kw, "dictionary")) {
                    kind = WebIdlDeclKind.dictionary_decl;
                } else if (eqlBytes(kw, "enum")) {
                    kind = WebIdlDeclKind.enum_decl;
                } else if (eqlBytes(kw, "namespace")) {
                    kind = WebIdlDeclKind.namespace_decl;
                }

                const n1_start = _.pos;
                const n1_end = readIdentAt(_.input, _.pos);
                if (n1_end == n1_start) {
                    _.pos = _.input.len;
                    return null;
                }
                let name = _.input[n1_start..n1_end];
                _.pos = n1_end;

                // 处理 `interface mixin Name`
                if (kind == WebIdlDeclKind.interface_decl && eqlBytes(name, "mixin")) {
                    kind = WebIdlDeclKind.interface_mixin;
                    if (p_mix := skipWsAndComments(_.input, _.pos)) {
                        _.pos = p_mix;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                    const n2_start = _.pos;
                    const n2_end = readIdentAt(_.input, _.pos);
                    if (n2_end == n2_start) {
                        _.pos = _.input.len;
                        return null;
                    }
                    name = _.input[n2_start..n2_end];
                    _.pos = n2_end;
                }

                if (p_after_name := skipWsAndComments(_.input, _.pos)) {
                    _.pos = p_after_name;
                } else {
                    _.pos = _.input.len;
                    return null;
                }

                // 可选 `: BaseName` 继承
                let inherits: ([]const u8)? = null;
                if (_.pos < _.input.len && _.input[_.pos] == ':') {
                    _.pos += 1;
                    if (p_base := skipWsAndComments(_.input, _.pos)) {
                        _.pos = p_base;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                    const b_start = _.pos;
                    const b_end = readIdentAt(_.input, _.pos);
                    if (b_end == b_start) {
                        _.pos = _.input.len;
                        return null;
                    }
                    inherits = _.input[b_start..b_end];
                    if (p_after_base := skipWsAndComments(_.input, b_end)) {
                        _.pos = p_after_base;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                }

                // 期望 `{ ... }` 体
                if (_.pos >= _.input.len || _.input[_.pos] != '{') {
                    // 兼容前置声明 `interface Foo;`
                    if (_.pos < _.input.len && _.input[_.pos] == ';') {
                        _.pos += 1;
                        return WebIdlDecl {
                            kind: kind,
                            is_partial: is_partial,
                            name: name,
                            inherits: inherits,
                            ext_attrs: ext_attrs,
                            body: null,
                        };
                    }
                    _.pos = _.input.len;
                    return null;
                }

                let close_brace: usize = 0;
                if (cb := findMatchingDelim(_.input, _.pos, '{', '}')) {
                    close_brace = cb;
                } else {
                    _.pos = _.input.len;
                    return null;
                }
                const body_slice = trimAscii(_.input[_.pos + 1 .. close_brace]);
                _.pos = close_brace + 1;

                // 消费可选尾随 `;`
                if (p_after := skipWsAndComments(_.input, _.pos)) {
                    _.pos = p_after;
                    if (_.pos < _.input.len && _.input[_.pos] == ';') {
                        _.pos += 1;
                    }
                }

                return WebIdlDecl {
                    kind: kind,
                    is_partial: is_partial,
                    name: name,
                    inherits: inherits,
                    ext_attrs: ext_attrs,
                    body: body_slice,
                };
            }

            if (eqlBytes(kw, "typedef")) {
                let semi_pos: usize = 0;
                if (sp := findTopLevelSemi(_.input, _.pos)) {
                    semi_pos = sp;
                } else {
                    _.pos = _.input.len;
                    return null;
                }
                const raw_segment = _.input[_.pos..semi_pos];
                _.pos = semi_pos + 1;

                // 正向词法扫描，记录最后一个顶层标识符作为 typedef 名称
                let lex = WebIdlIter.init(raw_segment);
                let depth: usize = 0;
                let last_name: []const u8 = "";
                let last_name_start: usize = 0;
                let has_name = false;
                while (tok := lex.next()) {
                    if (tok.kind == WebIdlTokenKind.punct && tok.lexeme.len == 1) {
                        const ch = tok.lexeme[0];
                        if (ch == '(' || ch == '[' || ch == '<' || ch == '{') {
                            depth += 1;
                        } else if ((ch == ')' || ch == ']' || ch == '>' || ch == '}') && depth > 0) {
                            depth -= 1;
                        }
                    } else if (tok.kind == WebIdlTokenKind.ident && depth == 0) {
                        last_name = tok.lexeme;
                        last_name_start = lex.pos - tok.lexeme.len;
                        has_name = true;
                    }
                }
                if (!has_name) {
                    return null;
                }
                const type_expr = trimAscii(raw_segment[0..last_name_start]);
                return WebIdlDecl {
                    kind: WebIdlDeclKind.typedef_decl,
                    is_partial: is_partial,
                    name: last_name,
                    inherits: null,
                    ext_attrs: ext_attrs,
                    body: if (type_expr.len > 0) type_expr else null,
                };
            }

            if (eqlBytes(kw, "callback")) {
                const n1_start = _.pos;
                const n1_end = readIdentAt(_.input, _.pos);
                if (n1_end == n1_start) {
                    _.pos = _.input.len;
                    return null;
                }
                let name = _.input[n1_start..n1_end];
                if (p_cb := skipWsAndComments(_.input, n1_end)) {
                    _.pos = p_cb;
                } else {
                    _.pos = _.input.len;
                    return null;
                }

                // 分支 A: `callback interface Foo [: Base] { ... };`
                if (eqlBytes(name, "interface")) {
                    const n2_start = _.pos;
                    const n2_end = readIdentAt(_.input, _.pos);
                    if (n2_end == n2_start) {
                        _.pos = _.input.len;
                        return null;
                    }
                    name = _.input[n2_start..n2_end];
                    if (p_cbi := skipWsAndComments(_.input, n2_end)) {
                        _.pos = p_cbi;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }

                    let inherits: ([]const u8)? = null;
                    if (_.pos < _.input.len && _.input[_.pos] == ':') {
                        _.pos += 1;
                        if (p_base := skipWsAndComments(_.input, _.pos)) {
                            _.pos = p_base;
                        } else {
                            _.pos = _.input.len;
                            return null;
                        }
                        const b_start = _.pos;
                        const b_end = readIdentAt(_.input, _.pos);
                        if (b_end == b_start) {
                            _.pos = _.input.len;
                            return null;
                        }
                        inherits = _.input[b_start..b_end];
                        if (p_after_base := skipWsAndComments(_.input, b_end)) {
                            _.pos = p_after_base;
                        } else {
                            _.pos = _.input.len;
                            return null;
                        }
                    }

                    if (_.pos >= _.input.len || _.input[_.pos] != '{') {
                        _.pos = _.input.len;
                        return null;
                    }
                    let close_brace: usize = 0;
                    if (cb := findMatchingDelim(_.input, _.pos, '{', '}')) {
                        close_brace = cb;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                    const body_slice = trimAscii(_.input[_.pos + 1 .. close_brace]);
                    _.pos = close_brace + 1;
                    if (p_after := skipWsAndComments(_.input, _.pos)) {
                        _.pos = p_after;
                        if (_.pos < _.input.len && _.input[_.pos] == ';') {
                            _.pos += 1;
                        }
                    }
                    return WebIdlDecl {
                        kind: WebIdlDeclKind.callback_decl,
                        is_partial: is_partial,
                        name: name,
                        inherits: inherits,
                        ext_attrs: ext_attrs,
                        body: body_slice,
                    };
                }

                // 分支 B: `callback Name = ReturnType (Args...);`
                if (_.pos >= _.input.len || _.input[_.pos] != '=') {
                    _.pos = _.input.len;
                    return null;
                }
                _.pos += 1;
                let semi_pos: usize = 0;
                if (sp := findTopLevelSemi(_.input, _.pos)) {
                    semi_pos = sp;
                } else {
                    _.pos = _.input.len;
                    return null;
                }
                const sig_slice = trimAscii(_.input[_.pos..semi_pos]);
                _.pos = semi_pos + 1;
                return WebIdlDecl {
                    kind: WebIdlDeclKind.callback_decl,
                    is_partial: is_partial,
                    name: name,
                    inherits: null,
                    ext_attrs: ext_attrs,
                    body: sig_slice,
                };
            }

            // 5. `Target includes Mixin;`
            if (!is_partial) {
                const inc_start = _.pos;
                const inc_end = readIdentAt(_.input, _.pos);
                if (inc_end > inc_start && eqlBytes(_.input[inc_start..inc_end], "includes")) {
                    if (p_inc := skipWsAndComments(_.input, inc_end)) {
                        _.pos = p_inc;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                    const rhs_start = _.pos;
                    const rhs_end = readIdentAt(_.input, _.pos);
                    if (rhs_end == rhs_start) {
                        _.pos = _.input.len;
                        return null;
                    }
                    const rhs_name = _.input[rhs_start..rhs_end];
                    if (p_after_rhs := skipWsAndComments(_.input, rhs_end)) {
                        _.pos = p_after_rhs;
                    } else {
                        _.pos = _.input.len;
                        return null;
                    }
                    if (_.pos < _.input.len && _.input[_.pos] == ';') {
                        _.pos += 1;
                    }
                    return WebIdlDecl {
                        kind: WebIdlDeclKind.includes_decl,
                        is_partial: false,
                        name: kw,
                        inherits: rhs_name,
                        ext_attrs: ext_attrs,
                        body: null,
                    };
                }
            }

            // 未知顶层声明：尝试跳到下一个顶层 `;` 恢复，若无则结束
            if (semi_pos := findTopLevelSemi(_.input, _.pos)) {
                _.pos = semi_pos + 1;
                continue;
            }
            _.pos = _.input.len;
            return null;
        }
        return null;
    }
}

/// 字符串/注释感知校验 WebIDL `{}` / `()` / `[]` / `<>` 平衡与 64 层深度上限
pub fn validateWebIdlBraces(input: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let i: usize = 0;

    while (i < input.len) {
        const c = input[i];

        // 双引号字符串
        if (c == '"') {
            i += 1;
            let closed = false;
            while (i < input.len) {
                if (input[i] == '\\' && i + 1 < input.len) {
                    i += 2;
                    continue;
                }
                if (input[i] == '"') {
                    i += 1;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return false;
            continue;
        }

        // 单行注释 `//`
        if (i + 1 < input.len && c == '/' && input[i + 1] == '/') {
            i += 2;
            while (i < input.len && input[i] != '\n') {
                i += 1;
            }
            continue;
        }

        // 块注释 `/* ... */`
        if (i + 1 < input.len && c == '/' && input[i + 1] == '*') {
            i += 2;
            let closed = false;
            while (i + 1 < input.len) {
                if (input[i] == '*' && input[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return false;
            continue;
        }

        if (c == '{' || c == '(' || c == '[' || c == '<') {
            if (depth >= MAX_BRACE_DEPTH) {
                return false;
            }
            let expected: u8 = '}';
            if (c == '(') {
                expected = ')';
            } else if (c == '[') {
                expected = ']';
            } else if (c == '<') {
                expected = '>';
            }
            stack[depth] = expected;
            depth += 1;
        } else if (c == '}' || c == ')' || c == ']' || c == '>') {
            if (depth == 0 || stack[depth - 1] != c) {
                return false;
            }
            depth -= 1;
        }

        i += 1;
    }

    return depth == 0;
}
