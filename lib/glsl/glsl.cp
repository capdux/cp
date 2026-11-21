// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc GLSL Shader Lexer & Decl Scanner)

/// GLSL `#version` 指令元数据
pub struct GlslVersion {
    number: u32,
    profile: []const u8 = "",
}

/// GLSL 顶层存储限定符枚举
pub enum GlslQualifier: u8 {
    uniform = 0,
    input = 1,
    output = 2,
}

/// 零拷贝 GLSL 顶层变量声明元数据
pub struct GlslDecl {
    qualifier: GlslQualifier,
    ty: []const u8,
    name: []const u8,
    location: u32? = null,
    binding: u32? = null,
}

/// GLSL 词法单元类型
pub enum GlslTokenKind: u8 {
    ident = 0,
    number = 1,
    preproc = 2,
    punct = 3,
    invalid = 4,
}

/// 零拷贝 GLSL 词法单元
pub struct GlslToken {
    kind: GlslTokenKind,
    lexeme: []const u8,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
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
        || c == '&' || c == '|' || c == '^' || c == '!' || c == '~' || c == '?';
}

/// 零堆分配 GLSL 流式词法扫描器（跳过 `//`/`/* */` 注释，识别 `#...` 预处理行、数字与标识符）
pub struct GlslIter {
    input: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): GlslIter {
        return GlslIter {
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
                    return false;
                }
                continue;
            }
            break;
        }
        return true;
    }

    /// 读取下一个词法单元；到达末尾返回 `null`
    pub fn next(*_): GlslToken? {
        const start_before = _.pos;
        if (!_.skipWsAndComments()) {
            return GlslToken {
                kind: GlslTokenKind.invalid,
                lexeme: _.input[start_before.._.input.len],
            };
        }
        if (_.pos >= _.input.len) {
            return null;
        }

        const start = _.pos;
        const c = _.input[start];

        // Preprocessor line (#version, #define, #extension, ...)
        if (c == '#') {
            while (_.pos < _.input.len && _.input[_.pos] != '\n') {
                _.pos += 1;
            }
            let end = _.pos;
            while (end > start && (_.input[end - 1] == '\r' || isSpaceOrTab(_.input[end - 1]))) {
                end -= 1;
            }
            return GlslToken {
                kind: GlslTokenKind.preproc,
                lexeme: _.input[start..end],
            };
        }

        // Identifier / keyword
        if (isAlpha(c)) {
            _.pos += 1;
            while (_.pos < _.input.len && isIdentContinue(_.input[_.pos])) {
                _.pos += 1;
            }
            return GlslToken {
                kind: GlslTokenKind.ident,
                lexeme: _.input[start.._.pos],
            };
        }

        // Number literal (hex, float, int with optional u/U/f/F/lf/LF suffix)
        if (isDigit(c) || (c == '.' && start + 1 < _.input.len && isDigit(_.input[start + 1]))) {
            if (c == '0' && start + 1 < _.input.len && (_.input[start + 1] == 'x' || _.input[start + 1] == 'X')) {
                _.pos = start + 2;
                while (_.pos < _.input.len && isHexDigit(_.input[_.pos])) {
                    _.pos += 1;
                }
                if (_.pos < _.input.len && (_.input[_.pos] == 'u' || _.input[_.pos] == 'U')) {
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
                if (_.pos + 1 < _.input.len && (_.input[_.pos] == 'l' || _.input[_.pos] == 'L') && (_.input[_.pos + 1] == 'f' || _.input[_.pos + 1] == 'F')) {
                    _.pos += 2;
                } else if (_.pos < _.input.len && (_.input[_.pos] == 'f' || _.input[_.pos] == 'F' || _.input[_.pos] == 'u' || _.input[_.pos] == 'U')) {
                    _.pos += 1;
                }
            }
            return GlslToken {
                kind: GlslTokenKind.number,
                lexeme: _.input[start.._.pos],
            };
        }

        // Multi-char punctuation
        if (start + 1 < _.input.len && isTwoCharPunct(c, _.input[start + 1])) {
            _.pos = start + 2;
            return GlslToken {
                kind: GlslTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        // Single-char punctuation
        if (isSinglePunct(c)) {
            _.pos = start + 1;
            return GlslToken {
                kind: GlslTokenKind.punct,
                lexeme: _.input[start.._.pos],
            };
        }

        _.pos += 1;
        return GlslToken {
            kind: GlslTokenKind.invalid,
            lexeme: _.input[start.._.pos],
        };
    }
}

fn parseDecU32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let end = s.len;
    if (s[end - 1] == 'u' || s[end - 1] == 'U') {
        end -= 1;
    }
    if (end == 0) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < end) {
        if (!isDigit(s[i])) return null;
        acc = acc * 10 + ((s[i] - '0') as u64);
        if (acc > 4294967295) return null;
        i += 1;
    }
    return acc as u32;
}

/// 解析首个 `#version <number> [profile]` 指令；未找到或格式非法返回 `null`
pub fn parseGlslVersion(input: []const u8): GlslVersion? {
    let lex = GlslIter.init(input);
    while (tok := lex.next()) {
        if (tok.kind == GlslTokenKind.preproc) {
            const line = tok.lexeme;
            let i: usize = 1; // skip '#'
            while (i < line.len && isSpaceOrTab(line[i])) i += 1;
            const kw_start = i;
            while (i < line.len && isAlpha(line[i])) i += 1;
            if (!strEq(line[kw_start..i], "version")) {
                continue;
            }

            while (i < line.len && isSpaceOrTab(line[i])) i += 1;
            const num_start = i;
            while (i < line.len && isDigit(line[i])) i += 1;
            const num = parseDecU32(line[num_start..i]) ?? return null;
            if (num == 0) return null;

            while (i < line.len && isSpaceOrTab(line[i])) i += 1;
            let profile: []const u8 = "";
            if (i < line.len && isAlpha(line[i])) {
                const prof_start = i;
                while (i < line.len && isIdentContinue(line[i])) i += 1;
                profile = line[prof_start..i];
            }
            return GlslVersion {
                number: num,
                profile: profile,
            };
        }
    }
    return null;
}

fn isExtraQualifier(word: []const u8): bool {
    return strEq(word, "highp") || strEq(word, "mediump") || strEq(word, "lowp")
        || strEq(word, "flat") || strEq(word, "smooth") || strEq(word, "noperspective")
        || strEq(word, "centroid") || strEq(word, "sample")
        || strEq(word, "readonly") || strEq(word, "writeonly")
        || strEq(word, "coherent") || strEq(word, "volatile") || strEq(word, "restrict");
}

/// 零堆分配 GLSL 顶层 `uniform`/`in`/`out` 声明迭代器（支持 `layout(location=N, binding=M)`）
pub struct GlslDeclIter {
    lex: GlslIter,
    brace_depth: usize = 0,
    paren_depth: usize = 0,

    pub static fn init(input: []const u8): GlslDeclIter {
        return GlslDeclIter {
            lex: GlslIter.init(input),
            brace_depth: 0,
            paren_depth: 0,
        };
    }

    /// 读取下一条顶层 `GlslDecl`；遍历结束返回 `null`
    pub fn next(*_): GlslDecl? {
        let pending_loc: u32? = null;
        let pending_bind: u32? = null;

        while (tok := _.lex.next()) {
            if (tok.kind == GlslTokenKind.punct && tok.lexeme.len == 1) {
                const c = tok.lexeme[0];
                if (c == '{') {
                    _.brace_depth += 1;
                    pending_loc = null;
                    pending_bind = null;
                } else if (c == '}') {
                    if (_.brace_depth > 0) _.brace_depth -= 1;
                    pending_loc = null;
                    pending_bind = null;
                } else if (c == '(') {
                    _.paren_depth += 1;
                } else if (c == ')') {
                    if (_.paren_depth > 0) _.paren_depth -= 1;
                } else if (c == ';' && _.brace_depth == 0 && _.paren_depth == 0) {
                    pending_loc = null;
                    pending_bind = null;
                }
                continue;
            }

            if (_.brace_depth != 0 || _.paren_depth != 0) {
                continue;
            }

            if (tok.kind != GlslTokenKind.ident) {
                continue;
            }

            // Parse layout(...)
            if (strEq(tok.lexeme, "layout")) {
                const lp = _.lex.next() ?? return null;
                if (lp.kind != GlslTokenKind.punct || !strEq(lp.lexeme, "(")) {
                    continue;
                }
                let ldepth: usize = 1;
                while (ldepth > 0) {
                    const lt = _.lex.next() ?? return null;
                    if (lt.kind == GlslTokenKind.punct && strEq(lt.lexeme, "(")) {
                        ldepth += 1;
                    } else if (lt.kind == GlslTokenKind.punct && strEq(lt.lexeme, ")")) {
                        ldepth -= 1;
                    } else if (ldepth == 1 && lt.kind == GlslTokenKind.ident) {
                        const is_loc = strEq(lt.lexeme, "location");
                        const is_bind = strEq(lt.lexeme, "binding");
                        if (is_loc || is_bind) {
                            let look = _.lex;
                            if (eq := look.next()) {
                                if (eq.kind == GlslTokenKind.punct && strEq(eq.lexeme, "=")) {
                                    if (num := look.next()) {
                                        if (num.kind == GlslTokenKind.number) {
                                            if (v := parseDecU32(num.lexeme)) {
                                                if (is_loc) pending_loc = v;
                                                if (is_bind) pending_bind = v;
                                                _.lex = look;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                continue;
            }

            // Match uniform / in / out
            let qual_opt: GlslQualifier? = null;
            if (strEq(tok.lexeme, "uniform")) {
                qual_opt = GlslQualifier.uniform;
            } else if (strEq(tok.lexeme, "in")) {
                qual_opt = GlslQualifier.input;
            } else if (strEq(tok.lexeme, "out")) {
                qual_opt = GlslQualifier.output;
            }

            if (qual := qual_opt) {
                let ty_tok = _.lex.next() ?? return null;
                while (ty_tok.kind == GlslTokenKind.ident && isExtraQualifier(ty_tok.lexeme)) {
                    ty_tok = _.lex.next() ?? return null;
                }
                if (ty_tok.kind != GlslTokenKind.ident) {
                    pending_loc = null;
                    pending_bind = null;
                    continue;
                }
                const name_tok = _.lex.next() ?? return null;
                if (name_tok.kind == GlslTokenKind.punct && strEq(name_tok.lexeme, "{")) {
                    // Interface/uniform block: skip block body
                    _.brace_depth += 1;
                    pending_loc = null;
                    pending_bind = null;
                    continue;
                }
                if (name_tok.kind != GlslTokenKind.ident) {
                    pending_loc = null;
                    pending_bind = null;
                    continue;
                }

                const decl = GlslDecl {
                    qualifier: qual,
                    ty: ty_tok.lexeme,
                    name: name_tok.lexeme,
                    location: pending_loc,
                    binding: pending_bind,
                };
                pending_loc = null;
                pending_bind = null;

                // Advance to ';' for clean next declaration
                while (tail := _.lex.next()) {
                    if (tail.kind == GlslTokenKind.punct && strEq(tail.lexeme, ";")) {
                        break;
                    }
                    if (tail.kind == GlslTokenKind.punct && strEq(tail.lexeme, "{")) {
                        _.brace_depth += 1;
                        break;
                    }
                }
                return decl;
            }
        }
        return null;
    }
}

/// 注释感知校验 GLSL 源码中 `{}`/`()`/`[]` 的嵌套平衡性
pub fn validateGlslBraces(input: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let lex = GlslIter.init(input);

    while (tok := lex.next()) {
        if (tok.kind == GlslTokenKind.invalid) {
            return false;
        }
        if (tok.kind == GlslTokenKind.punct && tok.lexeme.len == 1) {
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
