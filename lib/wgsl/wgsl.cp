// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc WebGPU WGSL Lexer & EntryPoint Scanner)

/// WebGPU WGSL 着色器阶段枚举
pub enum WgslStage: u8 {
    vertex = 0,
    fragment = 1,
    compute = 2,
}

/// WGSL 入口点函数元数据
pub struct WgslEntryPoint {
    stage: WgslStage,
    name: []const u8,
}

/// WGSL 资源绑定元数据（`@group(g) @binding(b) var... <name>`）
pub struct WgslBinding {
    group: u32,
    binding: u32,
    var_name: []const u8,
}

/// WGSL 词法单元类型
pub enum WgslTokenKind: u8 {
    ident = 0,
    number = 1,
    at_attr = 2,
    punct = 3,
    invalid = 4,
}

/// 零拷贝 WGSL 词法单元
pub struct WgslToken {
    kind: WgslTokenKind,
    lexeme: []const u8,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isHexDigit(c: u8): bool {
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isIdentContinue(c: u8): bool {
    return isAlpha(c) || isDigit(c);
}

fn isNumSuffix(c: u8): bool {
    return c == 'i' || c == 'u' || c == 'f' || c == 'h';
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn isTwoCharPunct(a: u8, b: u8): bool {
    if (a == '-' && b == '>') return true;
    if (a == '=' && b == '=') return true;
    if (a == '!' && b == '=') return true;
    if (a == '<' && (b == '=' || b == '<')) return true;
    if (a == '>' && (b == '=' || b == '>')) return true;
    if (a == '&' && (b == '&' || b == '=')) return true;
    if (a == '|' && (b == '|' || b == '=')) return true;
    if ((a == '+' || a == '-' || a == '*' || a == '/' || a == '%' || a == '^') && b == '=') return true;
    if (a == '+' && b == '+') return true;
    if (a == '-' && b == '-') return true;
    return false;
}

fn isSinglePunct(c: u8): bool {
    return c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']'
        || c == '<' || c == '>' || c == ';' || c == ':' || c == ',' || c == '.'
        || c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%'
        || c == '&' || c == '|' || c == '^' || c == '!' || c == '~';
}

/// 零堆分配 W3C WebGPU WGSL 流式词法扫描器（支持 `//` 行注释与可嵌套 `/* ... */` 块注释）
pub struct WgslIter {
    input: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): WgslIter {
        return WgslIter {
            input: input,
            pos: 0,
        };
    }

    fn skipWsAndComments(*_): bool {
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
                _.pos += 2;
                let depth: usize = 1;
                while (_.pos + 1 < _.input.len && depth > 0) {
                    if (_.input[_.pos] == '/' && _.input[_.pos + 1] == '*') {
                        depth += 1;
                        _.pos += 2;
                    } else if (_.input[_.pos] == '*' && _.input[_.pos + 1] == '/') {
                        depth -= 1;
                        _.pos += 2;
                    } else {
                        _.pos += 1;
                    }
                }
                if (depth > 0) {
                    _.pos = _.input.len;
                    return false;
                }
                continue;
            }
            break;
        }
        return true;
    }

    /// 读取下一个词法单元；到达末尾返回 `null`
    pub fn next(*_): WgslToken? {
        const comment_start = _.pos;
        if (!_.skipWsAndComments()) {
            return WgslToken {
                kind: WgslTokenKind.invalid,
                lexeme: _.input[comment_start.._.input.len],
            };
        }
        if (_.pos >= _.input.len) {
            return null;
        }

        const start = _.pos;
        const c = _.input[start];

        // @attribute
        if (c == '@') {
            if (start + 1 < _.input.len && isAlpha(_.input[start + 1])) {
                _.pos = start + 2;
                while (_.pos < _.input.len && isIdentContinue(_.input[_.pos])) {
                    _.pos += 1;
                }
                return WgslToken {
                    kind: WgslTokenKind.at_attr,
                    lexeme: _.input[start.._.pos],
                };
            }
            _.pos += 1;
            return WgslToken {
                kind: WgslTokenKind.invalid,
                lexeme: _.input[start.._.pos],
            };
        }

        // Identifier / keyword
        if (isAlpha(c)) {
            _.pos += 1;
            while (_.pos < _.input.len && isIdentContinue(_.input[_.pos])) {
                _.pos += 1;
            }
            return WgslToken {
                kind: WgslTokenKind.ident,
                lexeme: _.input[start.._.pos],
            };
        }

        // Number literal (hex, float, int with optional i/u/f/h suffix)
        if (isDigit(c) || (c == '.' && start + 1 < _.input.len && isDigit(_.input[start + 1]))) {
            if (c == '0' && start + 1 < _.input.len && (_.input[start + 1] == 'x' || _.input[start + 1] == 'X')) {
                _.pos = start + 2;
                while (_.pos < _.input.len && isHexDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos < _.input.len && _.input[_.pos] == '.') {
                    _.pos += 1;
                    while (_.pos < _.input.len && isHexDigit(_.input[_.pos])) {
                        _.pos += 1;
                    }
                }
                if (_.pos < _.input.len && (_.input[_.pos] == 'p' || _.input[_.pos] == 'P')) {
                    _.pos += 1;
                    if (_.pos < _.input.len && (_.input[_.pos] == '+' || _.input[_.pos] == '-')) {
                        _.pos += 1;
                    }
                    while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                        _.pos += 1;
                    }
                }
                if (_.pos < _.input.len && isNumSuffix(_.input[_.pos])) {
                    _.pos += 1;
                }
            } else {
                while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos < _.input.len && _.input[_.pos] == '.') {
                    _.pos += 1;
                    while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                        _.pos += 1;
                    }
                }
                if (_.pos < _.input.len && (_.input[_.pos] == 'e' || _.input[_.pos] == 'E')) {
                    _.pos += 1;
                    if (_.pos < _.input.len && (_.input[_.pos] == '+' || _.input[_.pos] == '-')) {
                        _.pos += 1;
                    }
                    while (_.pos < _.input.len && isDigit(_.input[_.pos])) {
                        _.pos += 1;
                    }
                }
                if (_.pos < _.input.len && isNumSuffix(_.input[_.pos])) {
                    _.pos += 1;
                }
            }
            return WgslToken {
                kind: WgslTokenKind.number,
                lexeme: _.input[start.._.pos],
            };
        }

        // Multi-char punctuation
        if (start + 1 < _.input.len && isTwoCharPunct(c, _.input[start + 1])) {
            _.pos = start + 2;
            return WgslToken {
                kind: WgslTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        // Single-char punctuation
        if (isSinglePunct(c)) {
            _.pos = start + 1;
            return WgslToken {
                kind: WgslTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        _.pos += 1;
        return WgslToken {
            kind: WgslTokenKind.invalid,
            lexeme: _.input[start.._.pos],
        };
    }
}

fn parseWgslU32(lexeme: []const u8): u32? {
    if (lexeme.len == 0) return null;
    let end = lexeme.len;
    if (isNumSuffix(lexeme[end - 1])) {
        end -= 1;
    }
    if (end == 0) return null;

    if (end >= 3 && lexeme[0] == '0' && (lexeme[1] == 'x' || lexeme[1] == 'X')) {
        let acc: u64 = 0;
        let i: usize = 2;
        while (i < end) {
            const c = lexeme[i];
            let d: u64 = 0;
            if (c >= '0' && c <= '9') {
                d = (c - '0') as u64;
            } else if (c >= 'a' && c <= 'f') {
                d = ((c - 'a') + 10) as u64;
            } else if (c >= 'A' && c <= 'F') {
                d = ((c - 'A') + 10) as u64;
            } else {
                return null;
            }
            acc = acc * 16 + d;
            if (acc > 4294967295) return null;
            i += 1;
        }
        return acc as u32;
    }

    let acc: u64 = 0;
    let i: usize = 0;
    while (i < end) {
        const c = lexeme[i];
        if (!isDigit(c)) return null;
        acc = acc * 10 + ((c - '0') as u64);
        if (acc > 4294967295) return null;
        i += 1;
    }
    return acc as u32;
}

/// 零堆分配 WGSL 入口点扫描器（识别 `@vertex`/`@fragment`/`@compute` 修饰的 `fn <name>`）
pub struct WgslEntryPointIter {
    lex: WgslIter,

    pub static fn init(input: []const u8): WgslEntryPointIter {
        return WgslEntryPointIter {
            lex: WgslIter.init(input),
        };
    }

    /// 读取下一个 `WgslEntryPoint`；遍历结束返回 `null`
    pub fn next(*_): WgslEntryPoint? {
        let pending_stage: WgslStage? = null;
        let attr_paren_depth: usize = 0;

        while (tok := _.lex.next()) {
            if (attr_paren_depth > 0) {
                if (tok.kind == WgslTokenKind.punct && strEq(tok.lexeme, "(")) {
                    attr_paren_depth += 1;
                } else if (tok.kind == WgslTokenKind.punct && strEq(tok.lexeme, ")")) {
                    attr_paren_depth -= 1;
                }
                continue;
            }

            if (tok.kind == WgslTokenKind.at_attr) {
                if (strEq(tok.lexeme, "@vertex")) {
                    pending_stage = WgslStage.vertex;
                } else if (strEq(tok.lexeme, "@fragment")) {
                    pending_stage = WgslStage.fragment;
                } else if (strEq(tok.lexeme, "@compute")) {
                    pending_stage = WgslStage.compute;
                } else {
                    // Skip optional attribute argument list e.g. @workgroup_size(64, 1, 1)
                    let look = _.lex;
                    if (nxt := look.next()) {
                        if (nxt.kind == WgslTokenKind.punct && strEq(nxt.lexeme, "(")) {
                            _.lex = look;
                            attr_paren_depth = 1;
                        }
                    }
                }
                continue;
            }

            if (tok.kind == WgslTokenKind.ident) {
                if (strEq(tok.lexeme, "fn")) {
                    if (stage := pending_stage) {
                        pending_stage = null;
                        if (name_tok := _.lex.next()) {
                            if (name_tok.kind == WgslTokenKind.ident) {
                                return WgslEntryPoint {
                                    stage: stage,
                                    name: name_tok.lexeme,
                                };
                            }
                        }
                    }
                } else if (strEq(tok.lexeme, "var") || strEq(tok.lexeme, "const") || strEq(tok.lexeme, "let") || strEq(tok.lexeme, "struct") || strEq(tok.lexeme, "alias") || strEq(tok.lexeme, "override")) {
                    pending_stage = null;
                }
            } else if (tok.kind == WgslTokenKind.punct) {
                if (strEq(tok.lexeme, ";") || strEq(tok.lexeme, "{") || strEq(tok.lexeme, "}")) {
                    pending_stage = null;
                }
            }
        }
        return null;
    }
}

/// 零堆分配 WGSL 资源绑定扫描器（提取 `@group(g) @binding(b) var... <name>`）
pub struct WgslBindingIter {
    lex: WgslIter,

    pub static fn init(input: []const u8): WgslBindingIter {
        return WgslBindingIter {
            lex: WgslIter.init(input),
        };
    }

    fn parseSingleU32Arg(*_): u32? {
        const lp = _.lex.next() ?? return null;
        if (lp.kind != WgslTokenKind.punct || !strEq(lp.lexeme, "(")) return null;
        const num = _.lex.next() ?? return null;
        if (num.kind != WgslTokenKind.number) return null;
        const val = parseWgslU32(num.lexeme) ?? return null;
        const rp = _.lex.next() ?? return null;
        if (rp.kind != WgslTokenKind.punct || !strEq(rp.lexeme, ")")) return null;
        return val;
    }

    /// 读取下一条 `WgslBinding`；遍历结束返回 `null`
    pub fn next(*_): WgslBinding? {
        let pending_group: u32? = null;
        let pending_binding: u32? = null;

        while (tok := _.lex.next()) {
            if (tok.kind == WgslTokenKind.at_attr) {
                if (strEq(tok.lexeme, "@group")) {
                    pending_group = _.parseSingleU32Arg();
                } else if (strEq(tok.lexeme, "@binding")) {
                    pending_binding = _.parseSingleU32Arg();
                }
                continue;
            }

            if (tok.kind == WgslTokenKind.ident) {
                if (strEq(tok.lexeme, "var")) {
                    if (g := pending_group) {
                        if (b := pending_binding) {
                            pending_group = null;
                            pending_binding = null;

                            // Skip optional template <uniform> / <storage, read_write>
                            let nxt = _.lex.next() ?? return null;
                            if (nxt.kind == WgslTokenKind.punct && strEq(nxt.lexeme, "<")) {
                                let angle_depth: usize = 1;
                                while (angle_depth > 0) {
                                    const t = _.lex.next() ?? return null;
                                    if (t.kind == WgslTokenKind.punct && strEq(t.lexeme, "<")) {
                                        angle_depth += 1;
                                    } else if (t.kind == WgslTokenKind.punct && strEq(t.lexeme, ">")) {
                                        angle_depth -= 1;
                                    }
                                }
                                nxt = _.lex.next() ?? return null;
                            }

                            if (nxt.kind == WgslTokenKind.ident) {
                                return WgslBinding {
                                    group: g,
                                    binding: b,
                                    var_name: nxt.lexeme,
                                };
                            }
                            continue;
                        }
                    }
                    pending_group = null;
                    pending_binding = null;
                } else if (strEq(tok.lexeme, "fn") || strEq(tok.lexeme, "struct")) {
                    pending_group = null;
                    pending_binding = null;
                }
            } else if (tok.kind == WgslTokenKind.punct) {
                if (strEq(tok.lexeme, ";") || strEq(tok.lexeme, "{") || strEq(tok.lexeme, "}")) {
                    pending_group = null;
                    pending_binding = null;
                }
            }
        }
        return null;
    }
}

/// 校验 WGSL 源码中 `{}`/`()`/`[]` 括号平衡以及嵌套 `/* ... */` 块注释闭合
pub fn validateWgslDelimiters(input: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let lex = WgslIter.init(input);

    while (tok := lex.next()) {
        if (tok.kind == WgslTokenKind.invalid) {
            return false;
        }
        if (tok.kind == WgslTokenKind.punct && tok.lexeme.len == 1) {
            const c = tok.lexeme[0];
            if (c == '{' || c == '(' || c == '[') {
                if (depth >= 64) return false;
                stack[depth] = c;
                depth += 1;
            } else if (c == '}') {
                if (depth == 0 || stack[depth - 1] != '{') return false;
                depth -= 1;
            } else if (c == ')') {
                if (depth == 0 || stack[depth - 1] != '(') return false;
                depth -= 1;
            } else if (c == ']') {
                if (depth == 0 || stack[depth - 1] != '[') return false;
                depth -= 1;
            }
        }
    }
    return depth == 0;
}
