// Origin: Pure-CP Stdlib (Zero-Alloc RON Lexer & Validator) | Created: 2026-11-22

/// RON (Rusty Object Notation) 词法单元类型
pub enum RonTokenKind: u8 {
    lparen = 0,
    rparen = 1,
    lbracket = 2,
    rbracket = 3,
    lbrace = 4,
    rbrace = 5,
    colon = 6,
    comma = 7,
    ident = 8,
    string = 9,
    char_lit = 10,
    number = 11,
    bool_lit = 12,
    invalid = 13,
}

/// RON 单个词法单元（零堆分配切片视图）
pub struct RonToken {
    kind: RonTokenKind = .invalid,
    lexeme: []const u8 = "",
}

type RonVec16 = @Vector(16, u8);

fn makeRonCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let f: u8 = 0;
        if (i == ' ' || i == '\t' || i == '\r' || i == '\n') f |= 1;
        if (i >= '0' && i <= '9') f |= 2 | 4 | 16;
        if ((i >= 'a' && i <= 'f') || (i >= 'A' && i <= 'F')) f |= 4;
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || i == '_') f |= 8 | 16;
        lut[i] = f;
    }
    return lut;
}

const RON_CHAR_LUT: [u8; 256] = makeRonCharLut();

fn isAsciiSpace(c: u8): bool {
    return (RON_CHAR_LUT[usize(c)] & 1) != 0;
}

fn isAsciiDigit(c: u8): bool {
    return (RON_CHAR_LUT[usize(c)] & 2) != 0;
}

fn isHexDigit(c: u8): bool {
    return (RON_CHAR_LUT[usize(c)] & 4) != 0;
}

fn isIdentStart(c: u8): bool {
    return (RON_CHAR_LUT[usize(c)] & 8) != 0;
}

fn isIdentCont(c: u8): bool {
    return (RON_CHAR_LUT[usize(c)] & 16) != 0;
}

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: RonVec16 = a[i .. i + 16][0..16].*;
        const vb: RonVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 零堆分配 RON 词法迭代器（支持 `//` 行注释、可嵌套 `/* ... */` 块注释、原始字符串 `r#"..."#`、字符与多进制下划线数字）
pub struct RonIter {
    src: []const u8,
    pos: usize = 0,
    last_start: usize = 0,
    last_end: usize = 0,

    pub static fn init(input: []const u8): RonIter {
        let start: usize = 0;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return RonIter{
            src: input,
            pos: start,
            last_start: start,
            last_end: start,
        };
    }

    fn emit(*_, kind: RonTokenKind, start: usize, end: usize): RonToken {
        _.last_start = start;
        _.last_end = end;
        _.pos = end;
        return RonToken{
            kind: kind,
            lexeme: _.src[start..end],
        };
    }

    /// 跳过空白与注释；若遇到未闭合的 `/*` 块注释返回 `false`
    fn skipTrivia(*_): bool {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (isAsciiSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '/' && _.pos + 1 < _.src.len) {
                const n = _.src[_.pos + 1];
                // 行注释 // ...
                if (n == '/') {
                    _.pos += 2;
                    const vnl: RonVec16 = @splat('\n' as u8);
                    while (_.pos + 16 <= _.src.len) : (_.pos += 16) {
                        const v: RonVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
                        if (@reduce(.Or, v == vnl)) break;
                    }
                    while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                        _.pos += 1;
                    }
                    continue;
                }
                // 可嵌套块注释 /* ... */
                if (n == '*') {
                    _.pos += 2;
                    let depth: usize = 1;
                    const vsl: RonVec16 = @splat('/' as u8);
                    const vst: RonVec16 = @splat('*' as u8);
                    while (_.pos < _.src.len && depth > 0) {
                        if (_.pos + 16 <= _.src.len) {
                            const v: RonVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
                            if (!@reduce(.Or, (v == vsl) | (v == vst))) {
                                _.pos += 16;
                                continue;
                            }
                        }
                        if (_.src[_.pos] == '/' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '*') {
                            depth += 1;
                            _.pos += 2;
                        } else if (_.src[_.pos] == '*' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '/') {
                            depth -= 1;
                            _.pos += 2;
                        } else {
                            _.pos += 1;
                        }
                    }
                    if (depth != 0) {
                        return false;
                    }
                    continue;
                }
            }
            break;
        }
        return true;
    }

    fn scanRawString(*_, start: usize): RonToken? {
        // 检查 r"..." 或 r#"..."#
        let p = start + 1;
        let hashes: usize = 0;
        while (p < _.src.len && _.src[p] == '#') {
            hashes += 1;
            p += 1;
        }
        if (p >= _.src.len || _.src[p] != '"') {
            return null;
        }
        p += 1; // 跳过开引号
        const vqt: RonVec16 = @splat('"' as u8);
        while (p < _.src.len) {
            if (p + 16 <= _.src.len) {
                const v: RonVec16 = _.src[p .. p + 16][0..16].*;
                if (!@reduce(.Or, v == vqt)) {
                    p += 16;
                    continue;
                }
            }
            if (_.src[p] == '"') {
                let matched: usize = 0;
                while (matched < hashes && p + 1 + matched < _.src.len && _.src[p + 1 + matched] == '#') {
                    matched += 1;
                }
                if (matched == hashes) {
                    return _.emit(RonTokenKind.string, start, p + 1 + hashes);
                }
            }
            p += 1;
        }
        // 未闭合原始字符串
        return _.emit(.invalid, start, _.src.len);
    }

    fn scanString(*_, start: usize): RonToken {
        let p = start + 1;
        const vqt: RonVec16 = @splat('"' as u8);
        const vbs: RonVec16 = @splat('\\' as u8);
        while (p < _.src.len) {
            if (p + 16 <= _.src.len) {
                const v: RonVec16 = _.src[p .. p + 16][0..16].*;
                if (!@reduce(.Or, (v == vqt) | (v == vbs))) {
                    p += 16;
                    continue;
                }
            }
            const c = _.src[p];
            if (c == '\\') {
                if (p + 1 >= _.src.len) {
                    return _.emit(.invalid, start, _.src.len);
                }
                p += 2;
                continue;
            }
            if (c == '"') {
                return _.emit(RonTokenKind.string, start, p + 1);
            }
            p += 1;
        }
        return _.emit(.invalid, start, _.src.len);
    }

    fn scanChar(*_, start: usize): RonToken {
        let p = start + 1;
        if (p >= _.src.len || _.src[p] == '\'') {
            return _.emit(.invalid, start, if (p < _.src.len) p + 1 else _.src.len);
        }
        if (_.src[p] == '\\') {
            p += 1;
            if (p >= _.src.len) {
                return _.emit(.invalid, start, _.src.len);
            }
            const esc = _.src[p];
            if (esc == 'u' && p + 1 < _.src.len && _.src[p + 1] == '{') {
                p += 2;
                while (p < _.src.len && _.src[p] != '}' && _.src[p] != '\'') {
                    p += 1;
                }
                if (p >= _.src.len || _.src[p] != '}') {
                    return _.emit(.invalid, start, p);
                }
                p += 1;
            } else if (esc == 'x') {
                p += 1;
                if (p + 2 > _.src.len || !isHexDigit(_.src[p]) || !isHexDigit(_.src[p + 1])) {
                    return _.emit(.invalid, start, _.src.len);
                }
                p += 2;
            } else {
                p += 1;
            }
        } else {
            // 跳过单个 UTF-8 字符（首字节 + 后续 continuation bytes）
            p += 1;
            while (p < _.src.len && (_.src[p] & 0xC0) == 0x80) {
                p += 1;
            }
        }
        if (p < _.src.len && _.src[p] == '\'') {
            return _.emit(.char_lit, start, p + 1);
        }
        return _.emit(.invalid, start, p);
    }

    fn scanNumber(*_, start: usize): RonToken {
        let p = start;
        if (_.src[p] == '+' || _.src[p] == '-') {
            p += 1;
        }
        if (p >= _.src.len || !isAsciiDigit(_.src[p])) {
            return _.emit(.invalid, start, p);
        }
        // 多进制前缀 0x / 0b / 0o
        if (_.src[p] == '0' && p + 1 < _.src.len) {
            const next_c = _.src[p + 1];
            if (next_c == 'x' || next_c == 'X') {
                p += 2;
                let digits: usize = 0;
                while (p < _.src.len && (isHexDigit(_.src[p]) || _.src[p] == '_')) {
                    if (isHexDigit(_.src[p])) { digits += 1; }
                    p += 1;
                }
                if (digits == 0) { return _.emit(.invalid, start, p); }
                return _.emit(.number, start, p);
            }
            if (next_c == 'b' || next_c == 'B') {
                p += 2;
                let digits: usize = 0;
                while (p < _.src.len && (_.src[p] == '0' || _.src[p] == '1' || _.src[p] == '_')) {
                    if (_.src[p] != '_') { digits += 1; }
                    p += 1;
                }
                if (digits == 0) { return _.emit(.invalid, start, p); }
                return _.emit(.number, start, p);
            }
            if (next_c == 'o' || next_c == 'O') {
                p += 2;
                let digits: usize = 0;
                while (p < _.src.len && ((_.src[p] >= '0' && _.src[p] <= '7') || _.src[p] == '_')) {
                    if (_.src[p] != '_') { digits += 1; }
                    p += 1;
                }
                if (digits == 0) { return _.emit(.invalid, start, p); }
                return _.emit(.number, start, p);
            }
        }
        // 十进制整数部分
        while (p < _.src.len && (isAsciiDigit(_.src[p]) || _.src[p] == '_')) {
            p += 1;
        }
        // 小数部分
        if (p < _.src.len && _.src[p] == '.') {
            p += 1;
            while (p < _.src.len && (isAsciiDigit(_.src[p]) || _.src[p] == '_')) {
                p += 1;
            }
        }
        // 指数部分
        if (p < _.src.len && (_.src[p] == 'e' || _.src[p] == 'E')) {
            p += 1;
            if (p < _.src.len && (_.src[p] == '+' || _.src[p] == '-')) {
                p += 1;
            }
            let exp_digits: usize = 0;
            while (p < _.src.len && (isAsciiDigit(_.src[p]) || _.src[p] == '_')) {
                if (isAsciiDigit(_.src[p])) { exp_digits += 1; }
                p += 1;
            }
            if (exp_digits == 0) {
                return _.emit(.invalid, start, p);
            }
        }
        return _.emit(.number, start, p);
    }

    pub fn next(*_): RonToken? {
        const start_before_trivia = _.pos;
        if (!_.skipTrivia()) {
            return _.emit(.invalid, start_before_trivia, _.src.len);
        }
        if (_.pos >= _.src.len) {
            return null;
        }

        const start = _.pos;
        const c = _.src[start];

        if (c == '(') { return _.emit(.lparen, start, start + 1); }
        if (c == ')') { return _.emit(.rparen, start, start + 1); }
        if (c == '[') { return _.emit(.lbracket, start, start + 1); }
        if (c == ']') { return _.emit(.rbracket, start, start + 1); }
        if (c == '{') { return _.emit(.lbrace, start, start + 1); }
        if (c == '}') { return _.emit(.rbrace, start, start + 1); }
        if (c == ':') { return _.emit(.colon, start, start + 1); }
        if (c == ',') { return _.emit(.comma, start, start + 1); }
        if (c == '"') { return _.scanString(start); }
        if (c == '\'') { return _.scanChar(start); }

        if (isAsciiDigit(c) || ((c == '-' || c == '+') && start + 1 < _.src.len && isAsciiDigit(_.src[start + 1]))) {
            return _.scanNumber(start);
        }

        if (c == 'r') {
            if (raw_tok := _.scanRawString(start)) {
                return raw_tok;
            }
        }

        if (isIdentStart(c)) {
            let p = start + 1;
            while (p < _.src.len && isIdentCont(_.src[p])) {
                p += 1;
            }
            const lex = _.src[start..p];
            if (eqBytes(lex, "true") || eqBytes(lex, "false")) {
                return _.emit(.bool_lit, start, p);
            }
            return _.emit(.ident, start, p);
        }

        return _.emit(.invalid, start, start + 1);
    }
}

/// 校验 RON 文本结构平衡（`()`/`[]`/`{}` 栈式匹配、最大 64 层深度、注释/字符串闭合、无非法 token 且非空）
pub fn validateRon(input: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let count: usize = 0;
    let it = RonIter.init(input);

    while (tok := it.next()) {
        count += 1;
        if (tok.kind == .invalid) {
            return false;
        }
        if (tok.kind == .lparen) {
            if (depth >= 64) { return false; }
            stack[depth] = ')';
            depth += 1;
        } else if (tok.kind == .lbracket) {
            if (depth >= 64) { return false; }
            stack[depth] = ']';
            depth += 1;
        } else if (tok.kind == .lbrace) {
            if (depth >= 64) { return false; }
            stack[depth] = '}';
            depth += 1;
        } else if (tok.kind == .rparen) {
            if (depth == 0 || stack[depth - 1] != ')') { return false; }
            depth -= 1;
        } else if (tok.kind == .rbracket) {
            if (depth == 0 || stack[depth - 1] != ']') { return false; }
            depth -= 1;
        } else if (tok.kind == .rbrace) {
            if (depth == 0 || stack[depth - 1] != '}') { return false; }
            depth -= 1;
        }
    }

    return count > 0 && depth == 0;
}

/// 零分配定位顶层 `field_name: <value>` 的值切片，正确跳过嵌套括号与字符串内冒号
pub fn extractRonField(input: []const u8, field_name: []const u8): ([]const u8)? {
    if (field_name.len == 0) {
        return null;
    }

    // 先判断目标字段所在深度：外层是 `(...)`、`StructName(...)` 或 `{...}` 时为深度 1，否则为深度 0
    let probe = RonIter.init(input);
    let target_depth: usize = 0;
    if (t0 := probe.next()) {
        if (t0.kind == .lparen || t0.kind == .lbrace) {
            target_depth = 1;
        } else if (t0.kind == .ident) {
            if (t1 := probe.next()) {
                if (t1.kind == .lparen) {
                    target_depth = 1;
                }
            }
        }
    } else {
        return null;
    }

    let it = RonIter.init(input);
    let depth: usize = 0;
    let prev_is_match = false;
    let prev_at_target = false;

    while (tok := it.next()) {
        if (tok.kind == .invalid) {
            return null;
        }
        if (tok.kind == .lparen || tok.kind == .lbracket || tok.kind == .lbrace) {
            depth += 1;
            prev_is_match = false;
            prev_at_target = false;
            continue;
        }
        if (tok.kind == .rparen || tok.kind == .rbracket || tok.kind == .rbrace) {
            if (depth == 0) {
                return null;
            }
            depth -= 1;
            prev_is_match = false;
            prev_at_target = false;
            continue;
        }

        if (tok.kind == .colon && prev_is_match && prev_at_target && depth == target_depth) {
            // 捕获值的起止范围，直到同层 `,` 或跳出 target_depth 或 EOF
            let val_start: usize? = null;
            let val_end: usize = 0;
            let val_depth = depth;
            while (vtok := it.next()) {
                if (vtok.kind == .invalid) {
                    return null;
                }
                if (vtok.kind == .comma && val_depth == target_depth) {
                    break;
                }
                if (vtok.kind == .rparen || vtok.kind == .rbracket || vtok.kind == .rbrace) {
                    if (val_depth == target_depth) {
                        break;
                    }
                    val_depth -= 1;
                    if (val_start == null) {
                        val_start = it.last_start;
                    }
                    val_end = it.last_end;
                    continue;
                }
                if (vtok.kind == .lparen || vtok.kind == .lbracket || vtok.kind == .lbrace) {
                    val_depth += 1;
                }
                if (val_start == null) {
                    val_start = it.last_start;
                }
                val_end = it.last_end;
            }
            if (vs := val_start) {
                if (val_end >= vs) {
                    return input[vs..val_end];
                }
            }
            return null;
        }

        if (depth == target_depth && tok.kind == .ident && eqBytes(tok.lexeme, field_name)) {
            prev_is_match = true;
            prev_at_target = true;
        } else {
            prev_is_match = false;
            prev_at_target = false;
        }
    }

    return null;
}
