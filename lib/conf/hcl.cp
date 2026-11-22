// Origin: Pure-CP Stdlib (Zero-Alloc HCL / Terraform Lexer & Block Scanner) | Created: 2026-11-22

/// HCL / Terraform 词法单元类型
pub enum HclTokenKind: u8 {
    ident = 0,
    string = 1,
    heredoc = 2,
    number = 3,
    bool_lit = 4,
    null_lit = 5,
    equals = 6,
    lbrace = 7,
    rbrace = 8,
    lbracket = 9,
    rbracket = 10,
    lparen = 11,
    rparen = 12,
    comma = 13,
    dot = 14,
    invalid = 15,
}

/// HCL 单个词法单元（零堆分配切片视图）
pub struct HclToken {
    kind: HclTokenKind = .invalid,
    lexeme: []const u8 = "",
}

/// HCL 顶层块头与块体切片视图（零堆分配）
pub struct HclBlockHeader {
    block_type: []const u8 = "",
    label1: ([]const u8)? = null,
    label2: ([]const u8)? = null,
    body: []const u8 = "",
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentCont(c: u8): bool {
    return isIdentStart(c) || isAsciiDigit(c) || c == '-';
}

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn stripQuotes(s: []const u8): []const u8 {
    if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
        return s[1 .. s.len - 1];
    }
    return s;
}

/// 零堆分配 HCL / Terraform 词法迭代器（支持 `#`/`//`/`/* */` 注释、`${...}` 插值保护字符串、`<<EOF`/`<<-EOF` Heredoc）
pub struct HclIter {
    src: []const u8,
    pos: usize = 0,
    last_start: usize = 0,
    last_end: usize = 0,

    pub static fn init(input: []const u8): HclIter {
        let start: usize = 0;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return HclIter{
            src: input,
            pos: start,
            last_start: start,
            last_end: start,
        };
    }

    fn emit(*_, kind: HclTokenKind, start: usize, end: usize): HclToken {
        _.last_start = start;
        _.last_end = end;
        _.pos = end;
        return HclToken{
            kind: kind,
            lexeme: _.src[start..end],
        };
    }

    /// 跳过空白与 `#` / `//` / `/* ... */` 注释；未闭合块注释返回 `false`
    fn skipTrivia(*_): bool {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (isAsciiSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '#') {
                _.pos += 1;
                while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            if (c == '/' && _.pos + 1 < _.src.len) {
                const n = _.src[_.pos + 1];
                if (n == '/') {
                    _.pos += 2;
                    while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                        _.pos += 1;
                    }
                    continue;
                }
                if (n == '*') {
                    _.pos += 2;
                    let closed = false;
                    while (_.pos + 1 < _.src.len) {
                        if (_.src[_.pos] == '*' && _.src[_.pos + 1] == '/') {
                            _.pos += 2;
                            closed = true;
                            break;
                        }
                        _.pos += 1;
                    }
                    if (!closed) {
                        _.pos = _.src.len;
                        return false;
                    }
                    continue;
                }
            }
            break;
        }
        return true;
    }

    /// 扫描双引号字符串，保护 `${...}` 与 `%{...}` 模板插值内部的引号与嵌套花括号
    fn scanString(*_, start: usize): HclToken {
        let p = start + 1;
        while (p < _.src.len) {
            const c = _.src[p];
            if (c == '\\') {
                if (p + 1 >= _.src.len) {
                    return _.emit(HclTokenKind.invalid, start, _.src.len);
                }
                p += 2;
                continue;
            }
            if ((c == '$' || c == '%') && p + 1 < _.src.len && _.src[p + 1] == '{') {
                // 跳过转义的 $${ 或 %%{
                if (p > start + 1 && _.src[p - 1] == c) {
                    p += 2;
                    continue;
                }
                p += 2;
                let brace_depth: usize = 1;
                let in_inner_str = false;
                while (p < _.src.len && brace_depth > 0) {
                    const ic = _.src[p];
                    if (in_inner_str) {
                        if (ic == '\\' && p + 1 < _.src.len) {
                            p += 2;
                            continue;
                        }
                        if (ic == '"') {
                            in_inner_str = false;
                        }
                        p += 1;
                    } else {
                        if (ic == '"') {
                            in_inner_str = true;
                        } else if (ic == '{') {
                            brace_depth += 1;
                        } else if (ic == '}') {
                            brace_depth -= 1;
                        }
                        p += 1;
                    }
                }
                if (brace_depth != 0 || in_inner_str) {
                    return _.emit(HclTokenKind.invalid, start, _.src.len);
                }
                continue;
            }
            if (c == '"') {
                return _.emit(HclTokenKind.string, start, p + 1);
            }
            if (c == '\n') {
                // HCL 标准双引号字符串不允许裸换行
                return _.emit(HclTokenKind.invalid, start, p);
            }
            p += 1;
        }
        return _.emit(HclTokenKind.invalid, start, _.src.len);
    }

    /// 扫描 `<<EOF` / `<<-EOF` Heredoc
    fn scanHeredoc(*_, start: usize): HclToken {
        let p = start + 2;
        let indented = false;
        if (p < _.src.len && _.src[p] == '-') {
            indented = true;
            p += 1;
        }
        if (p >= _.src.len || !isIdentStart(_.src[p])) {
            return _.emit(HclTokenKind.invalid, start, p);
        }
        const marker_start = p;
        while (p < _.src.len && isIdentCont(_.src[p])) {
            p += 1;
        }
        const marker = _.src[marker_start..p];

        // 标记后必须紧跟换行（允许 \r\n 或 \n）
        if (p < _.src.len && _.src[p] == '\r') {
            p += 1;
        }
        if (p >= _.src.len || _.src[p] != '\n') {
            return _.emit(HclTokenKind.invalid, start, p);
        }
        p += 1;

        // 逐行扫描直到匹配结束 marker
        while (p <= _.src.len) {
            const line_start = p;
            let line_end = p;
            while (line_end < _.src.len && _.src[line_end] != '\n') {
                line_end += 1;
            }
            let content_end = line_end;
            if (content_end > line_start && _.src[content_end - 1] == '\r') {
                content_end -= 1;
            }
            let check_start = line_start;
            if (indented) {
                while (check_start < content_end && (_.src[check_start] == ' ' || _.src[check_start] == '\t')) {
                    check_start += 1;
                }
            }
            if (eqBytes(_.src[check_start..content_end], marker)) {
                const tok_end = if (line_end < _.src.len) line_end + 1 else line_end;
                return _.emit(HclTokenKind.heredoc, start, tok_end);
            }
            if (line_end >= _.src.len) {
                break;
            }
            p = line_end + 1;
        }
        return _.emit(HclTokenKind.invalid, start, _.src.len);
    }

    fn scanNumber(*_, start: usize): HclToken {
        let p = start;
        if (_.src[p] == '-') {
            p += 1;
        }
        if (p >= _.src.len || !isAsciiDigit(_.src[p])) {
            return _.emit(HclTokenKind.invalid, start, p);
        }
        while (p < _.src.len && isAsciiDigit(_.src[p])) {
            p += 1;
        }
        if (p < _.src.len && _.src[p] == '.' && p + 1 < _.src.len && isAsciiDigit(_.src[p + 1])) {
            p += 1;
            while (p < _.src.len && isAsciiDigit(_.src[p])) {
                p += 1;
            }
        }
        if (p < _.src.len && (_.src[p] == 'e' || _.src[p] == 'E')) {
            p += 1;
            if (p < _.src.len && (_.src[p] == '+' || _.src[p] == '-')) {
                p += 1;
            }
            let exp_digits: usize = 0;
            while (p < _.src.len && isAsciiDigit(_.src[p])) {
                exp_digits += 1;
                p += 1;
            }
            if (exp_digits == 0) {
                return _.emit(HclTokenKind.invalid, start, p);
            }
        }
        return _.emit(HclTokenKind.number, start, p);
    }

    pub fn next(*_): HclToken? {
        const before = _.pos;
        if (!_.skipTrivia()) {
            return _.emit(HclTokenKind.invalid, before, _.src.len);
        }
        if (_.pos >= _.src.len) {
            return null;
        }

        const start = _.pos;
        const c = _.src[start];

        if (c == '=') { return _.emit(HclTokenKind.equals, start, start + 1); }
        if (c == '{') { return _.emit(HclTokenKind.lbrace, start, start + 1); }
        if (c == '}') { return _.emit(HclTokenKind.rbrace, start, start + 1); }
        if (c == '[') { return _.emit(HclTokenKind.lbracket, start, start + 1); }
        if (c == ']') { return _.emit(HclTokenKind.rbracket, start, start + 1); }
        if (c == '(') { return _.emit(HclTokenKind.lparen, start, start + 1); }
        if (c == ')') { return _.emit(HclTokenKind.rparen, start, start + 1); }
        if (c == ',') { return _.emit(HclTokenKind.comma, start, start + 1); }
        if (c == '.') { return _.emit(HclTokenKind.dot, start, start + 1); }
        if (c == '"') { return _.scanString(start); }

        if (c == '<' && start + 1 < _.src.len && _.src[start + 1] == '<') {
            return _.scanHeredoc(start);
        }

        if (isAsciiDigit(c) || (c == '-' && start + 1 < _.src.len && isAsciiDigit(_.src[start + 1]))) {
            return _.scanNumber(start);
        }

        if (isIdentStart(c)) {
            let p = start + 1;
            while (p < _.src.len && isIdentCont(_.src[p])) {
                p += 1;
            }
            const lex = _.src[start..p];
            if (eqBytes(lex, "true") || eqBytes(lex, "false")) {
                return _.emit(HclTokenKind.bool_lit, start, p);
            }
            if (eqBytes(lex, "null")) {
                return _.emit(HclTokenKind.null_lit, start, p);
            }
            return _.emit(HclTokenKind.ident, start, p);
        }

        return _.emit(HclTokenKind.invalid, start, start + 1);
    }
}

/// 零堆分配 HCL 顶层块迭代器，括号平衡跳过嵌套块
pub struct HclBlockIter {
    src: []const u8,
    it: HclIter,

    pub static fn init(input: []const u8): HclBlockIter {
        return HclBlockIter{
            src: input,
            it: HclIter.init(input),
        };
    }

    pub fn next(*_): HclBlockHeader? {
        while (tok := _.it.next()) {
            if (tok.kind == HclTokenKind.invalid) {
                return null;
            }
            if (tok.kind != HclTokenKind.ident) {
                continue;
            }

            const block_type = tok.lexeme;
            let label1: ([]const u8)? = null;
            let label2: ([]const u8)? = null;
            let label_count: usize = 0;

            // 读取后续标签直到 `{`；若遇到 `=` 则是顶层属性赋值，跳过其值
            let found_lbrace = false;
            let body_start: usize = 0;
            while (nt := _.it.next()) {
                if (nt.kind == HclTokenKind.invalid) {
                    return null;
                }
                if (nt.kind == HclTokenKind.lbrace) {
                    found_lbrace = true;
                    body_start = _.it.last_end;
                    break;
                }
                if (nt.kind == HclTokenKind.equals) {
                    // 跳过顶层属性赋值（如 key = { ... } / [ ... ] / value）
                    if (vt := _.it.next()) {
                        if (vt.kind == HclTokenKind.lbrace || vt.kind == HclTokenKind.lbracket || vt.kind == HclTokenKind.lparen) {
                            let d: usize = 1;
                            while (d > 0) {
                                const st = _.it.next() ?? return null;
                                if (st.kind == HclTokenKind.invalid) { return null; }
                                if (st.kind == HclTokenKind.lbrace || st.kind == HclTokenKind.lbracket || st.kind == HclTokenKind.lparen) {
                                    d += 1;
                                } else if (st.kind == HclTokenKind.rbrace || st.kind == HclTokenKind.rbracket || st.kind == HclTokenKind.rparen) {
                                    d -= 1;
                                }
                            }
                        }
                    }
                    break;
                }
                if (nt.kind == HclTokenKind.string || nt.kind == HclTokenKind.ident) {
                    const lbl = stripQuotes(nt.lexeme);
                    if (label_count == 0) {
                        label1 = lbl;
                    } else if (label_count == 1) {
                        label2 = lbl;
                    }
                    label_count += 1;
                    continue;
                }
                break;
            }

            if (!found_lbrace) {
                continue;
            }

            // 平衡扫描块体直到配对 `}`
            let depth: usize = 1;
            let body_end: usize = body_start;
            while (depth > 0) {
                const bt = _.it.next() ?? return null;
                if (bt.kind == HclTokenKind.invalid) {
                    return null;
                }
                if (bt.kind == HclTokenKind.lbrace) {
                    depth += 1;
                } else if (bt.kind == HclTokenKind.rbrace) {
                    depth -= 1;
                    if (depth == 0) {
                        body_end = _.it.last_start;
                    }
                }
            }

            return HclBlockHeader{
                block_type: block_type,
                label1: label1,
                label2: label2,
                body: _.src[body_start..body_end],
            };
        }
        return null;
    }
}

/// 校验 HCL 结构平衡（`{}`/`[]`/`()` 匹配、最大 64 层深度、字符串/Heredoc/块注释闭合、无非法 token 且非空）
pub fn validateHcl(input: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let count: usize = 0;
    let it = HclIter.init(input);

    while (tok := it.next()) {
        count += 1;
        if (tok.kind == HclTokenKind.invalid) {
            return false;
        }
        if (tok.kind == HclTokenKind.lbrace) {
            if (depth >= 64) { return false; }
            stack[depth] = '}';
            depth += 1;
        } else if (tok.kind == HclTokenKind.lbracket) {
            if (depth >= 64) { return false; }
            stack[depth] = ']';
            depth += 1;
        } else if (tok.kind == HclTokenKind.lparen) {
            if (depth >= 64) { return false; }
            stack[depth] = ')';
            depth += 1;
        } else if (tok.kind == HclTokenKind.rbrace) {
            if (depth == 0 || stack[depth - 1] != '}') { return false; }
            depth -= 1;
        } else if (tok.kind == HclTokenKind.rbracket) {
            if (depth == 0 || stack[depth - 1] != ']') { return false; }
            depth -= 1;
        } else if (tok.kind == HclTokenKind.rparen) {
            if (depth == 0 || stack[depth - 1] != ')') { return false; }
            depth -= 1;
        }
    }

    return count > 0 && depth == 0;
}
