// Origin: Pure-CP Stdlib (Zero-Alloc CUE Language Lexer & Validator) | Created: 2026-11-22

/// CUE 配置与数据约束语言词法单元类型
pub enum CueTokenKind: u8 {
    ident = 0,
    def_ident = 1,
    hidden_ident = 2,
    string = 3,
    bytes_lit = 4,
    number = 5,
    bool_lit = 6,
    null_lit = 7,
    bottom = 8,
    lbrace = 9,
    rbrace = 10,
    lbracket = 11,
    rbracket = 12,
    lparen = 13,
    rparen = 14,
    colon = 15,
    comma = 16,
    pipe = 17,
    amp = 18,
    ellipsis = 19,
    attr = 20,
    invalid = 21,
}

/// CUE 单个词法单元（零堆分配切片视图）
pub struct CueToken {
    kind: CueTokenKind = CueTokenKind.invalid,
    lexeme: []const u8 = "",
}

type CueVec16 = @Vector(16, u8);

fn makeCueCharLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let f: u8 = 0;
        if (i == ' ' || i == '\t' || i == '\r' || i == '\n') f |= 1;
        if (i >= '0' && i <= '9') f |= 2 | 4 | 16;
        if ((i >= 'a' && i <= 'f') || (i >= 'A' && i <= 'F')) f |= 4;
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || i == '_') f |= 8 | 16;
        if (i == 'K' || i == 'M' || i == 'G' || i == 'T' || i == 'P') f |= 32;
        lut[i] = f;
    }
    return lut;
}

const CUE_CHAR_LUT: [u8; 256] = makeCueCharLut();

fn isAsciiSpace(c: u8): bool {
    return (CUE_CHAR_LUT[usize(c)] & 1) != 0;
}

fn isAsciiDigit(c: u8): bool {
    return (CUE_CHAR_LUT[usize(c)] & 2) != 0;
}

fn isHexDigit(c: u8): bool {
    return (CUE_CHAR_LUT[usize(c)] & 4) != 0;
}

fn isIdentStart(c: u8): bool {
    return (CUE_CHAR_LUT[usize(c)] & 8) != 0;
}

fn isIdentCont(c: u8): bool {
    return (CUE_CHAR_LUT[usize(c)] & 16) != 0;
}

fn isCueMultiplier(c: u8): bool {
    return (CUE_CHAR_LUT[usize(c)] & 32) != 0;
}

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: CueVec16 = a[i .. i + 16][0..16].*;
        const vb: CueVec16 = b[i .. i + 16][0..16].*;
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

/// 零堆分配 CUE 词法迭代器（支持 `//` 注释、`#Def`、`_hidden`、`_|_`、`...`、`@attr(...)`、SI/IEC 数字与双/单引号串）
pub struct CueIter {
    src: []const u8,
    pos: usize = 0,

    pub static fn init(input: []const u8): CueIter {
        let start: usize = 0;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            start = 3;
        }
        return CueIter{
            src: input,
            pos: start,
        };
    }

    fn emit(*_, kind: CueTokenKind, start: usize, end: usize): CueToken {
        _.pos = end;
        return CueToken{
            kind: kind,
            lexeme: _.src[start..end],
        };
    }

    fn skipTrivia(*_) {
        while (_.pos < _.src.len) {
            const c = _.src[_.pos];
            if (isAsciiSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == '/' && _.pos + 1 < _.src.len && _.src[_.pos + 1] == '/') {
                _.pos += 2;
                const vnl: CueVec16 = @splat('\n' as u8);
                while (_.pos + 16 <= _.src.len) : (_.pos += 16) {
                    const v: CueVec16 = _.src[_.pos .. _.pos + 16][0..16].*;
                    if (@reduce(.Or, v == vnl)) break;
                }
                while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                    _.pos += 1;
                }
                continue;
            }
            break;
        }
    }

    fn scanQuoted(*_, start: usize, quote: u8, kind: CueTokenKind): CueToken {
        const vqt: CueVec16 = @splat(quote);
        const vbs: CueVec16 = @splat('\\' as u8);
        // 多行三引号 """...""" 或 '''...'''
        if (start + 2 < _.src.len && _.src[start + 1] == quote && _.src[start + 2] == quote) {
            let p = start + 3;
            while (p + 2 < _.src.len) {
                if (p + 16 <= _.src.len) {
                    const v: CueVec16 = _.src[p .. p + 16][0..16].*;
                    if (!@reduce(.Or, (v == vqt) | (v == vbs))) {
                        p += 16;
                        continue;
                    }
                }
                if (_.src[p] == '\\') {
                    p += 2;
                    continue;
                }
                if (_.src[p] == quote && _.src[p + 1] == quote && _.src[p + 2] == quote) {
                    return _.emit(kind, start, p + 3);
                }
                p += 1;
            }
            return _.emit(CueTokenKind.invalid, start, _.src.len);
        }

        // 单行引号串
        let p = start + 1;
        const vnl: CueVec16 = @splat('\n' as u8);
        while (p < _.src.len) {
            if (p + 16 <= _.src.len) {
                const v: CueVec16 = _.src[p .. p + 16][0..16].*;
                if (!@reduce(.Or, (v == vqt) | (v == vbs) | (v == vnl))) {
                    p += 16;
                    continue;
                }
            }
            const c = _.src[p];
            if (c == '\\') {
                if (p + 1 >= _.src.len) {
                    return _.emit(CueTokenKind.invalid, start, _.src.len);
                }
                p += 2;
                continue;
            }
            if (c == quote) {
                return _.emit(kind, start, p + 1);
            }
            if (c == '\n') {
                return _.emit(CueTokenKind.invalid, start, p);
            }
            p += 1;
        }
        return _.emit(CueTokenKind.invalid, start, _.src.len);
    }

    fn scanAttr(*_, start: usize): CueToken {
        // @ident(...)
        let p = start + 1;
        if (p >= _.src.len || !isIdentStart(_.src[p])) {
            return _.emit(CueTokenKind.invalid, start, p);
        }
        while (p < _.src.len && isIdentCont(_.src[p])) {
            p += 1;
        }
        if (p >= _.src.len || _.src[p] != '(') {
            return _.emit(CueTokenKind.invalid, start, p);
        }
        p += 1;
        let depth: usize = 1;
        let in_quote: u8 = 0;
        while (p < _.src.len && depth > 0) {
            const c = _.src[p];
            if (in_quote != 0) {
                if (c == '\\' && p + 1 < _.src.len) {
                    p += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                p += 1;
            } else {
                if (c == '"' || c == '\'') {
                    in_quote = c;
                } else if (c == '(') {
                    depth += 1;
                } else if (c == ')') {
                    depth -= 1;
                }
                p += 1;
            }
        }
        if (depth != 0 || in_quote != 0) {
            return _.emit(CueTokenKind.invalid, start, _.src.len);
        }
        return _.emit(CueTokenKind.attr, start, p);
    }

    fn scanNumber(*_, start: usize): CueToken {
        let p = start;
        if (_.src[p] == '-') {
            p += 1;
        }
        if (p >= _.src.len || !isAsciiDigit(_.src[p])) {
            return _.emit(CueTokenKind.invalid, start, p);
        }
        // 多进制 0x / 0b / 0o
        if (_.src[p] == '0' && p + 1 < _.src.len) {
            const n = _.src[p + 1];
            if (n == 'x' || n == 'X') {
                p += 2;
                let digits: usize = 0;
                while (p < _.src.len && (isHexDigit(_.src[p]) || _.src[p] == '_')) {
                    if (isHexDigit(_.src[p])) { digits += 1; }
                    p += 1;
                }
                if (digits == 0) { return _.emit(CueTokenKind.invalid, start, p); }
                return _.emit(CueTokenKind.number, start, p);
            }
            if (n == 'b' || n == 'B') {
                p += 2;
                let digits: usize = 0;
                while (p < _.src.len && (_.src[p] == '0' || _.src[p] == '1' || _.src[p] == '_')) {
                    if (_.src[p] != '_') { digits += 1; }
                    p += 1;
                }
                if (digits == 0) { return _.emit(CueTokenKind.invalid, start, p); }
                return _.emit(CueTokenKind.number, start, p);
            }
            if (n == 'o' || n == 'O') {
                p += 2;
                let digits: usize = 0;
                while (p < _.src.len && ((_.src[p] >= '0' && _.src[p] <= '7') || _.src[p] == '_')) {
                    if (_.src[p] != '_') { digits += 1; }
                    p += 1;
                }
                if (digits == 0) { return _.emit(CueTokenKind.invalid, start, p); }
                return _.emit(CueTokenKind.number, start, p);
            }
        }

        // 十进制整数 + 下划线
        while (p < _.src.len && (isAsciiDigit(_.src[p]) || _.src[p] == '_')) {
            p += 1;
        }
        // 小数部分（避免吞掉 `...`）
        if (p < _.src.len && _.src[p] == '.' && (p + 1 >= _.src.len || _.src[p + 1] != '.')) {
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
            if (exp_digits == 0) { return _.emit(CueTokenKind.invalid, start, p); }
        } else if (p < _.src.len && isCueMultiplier(_.src[p])) {
            // CUE SI/IEC 乘数后缀：K / M / G / T / P 或 Ki / Mi / Gi / Ti / Pi
            p += 1;
            if (p < _.src.len && _.src[p] == 'i') {
                p += 1;
            }
        }
        return _.emit(CueTokenKind.number, start, p);
    }

    pub fn next(*_): CueToken? {
        _.skipTrivia();
        if (_.pos >= _.src.len) {
            return null;
        }

        const start = _.pos;
        const c = _.src[start];

        if (c == '{') { return _.emit(CueTokenKind.lbrace, start, start + 1); }
        if (c == '}') { return _.emit(CueTokenKind.rbrace, start, start + 1); }
        if (c == '[') { return _.emit(CueTokenKind.lbracket, start, start + 1); }
        if (c == ']') { return _.emit(CueTokenKind.rbracket, start, start + 1); }
        if (c == '(') { return _.emit(CueTokenKind.lparen, start, start + 1); }
        if (c == ')') { return _.emit(CueTokenKind.rparen, start, start + 1); }
        if (c == ':') { return _.emit(CueTokenKind.colon, start, start + 1); }
        if (c == ',') { return _.emit(CueTokenKind.comma, start, start + 1); }
        if (c == '|') { return _.emit(CueTokenKind.pipe, start, start + 1); }
        if (c == '&') { return _.emit(CueTokenKind.amp, start, start + 1); }

        if (c == '.' && start + 2 < _.src.len && _.src[start + 1] == '.' && _.src[start + 2] == '.') {
            return _.emit(CueTokenKind.ellipsis, start, start + 3);
        }

        if (c == '"') { return _.scanQuoted(start, '"', CueTokenKind.string); }
        if (c == '\'') { return _.scanQuoted(start, '\'', CueTokenKind.bytes_lit); }
        if (c == '@') { return _.scanAttr(start); }

        // Bottom: _|_
        if (c == '_' && start + 2 < _.src.len && _.src[start + 1] == '|' && _.src[start + 2] == '_') {
            return _.emit(CueTokenKind.bottom, start, start + 3);
        }

        // Definition identifier: #Def or _#Def
        if (c == '#') {
            let p = start + 1;
            while (p < _.src.len && isIdentCont(_.src[p])) {
                p += 1;
            }
            if (p == start + 1) {
                return _.emit(CueTokenKind.invalid, start, p);
            }
            return _.emit(CueTokenKind.def_ident, start, p);
        }
        if (c == '_' && start + 1 < _.src.len && _.src[start + 1] == '#') {
            let p = start + 2;
            while (p < _.src.len && isIdentCont(_.src[p])) {
                p += 1;
            }
            if (p == start + 2) {
                return _.emit(CueTokenKind.invalid, start, p);
            }
            return _.emit(CueTokenKind.def_ident, start, p);
        }

        // Numbers
        if (isAsciiDigit(c) || (c == '-' && start + 1 < _.src.len && isAsciiDigit(_.src[start + 1]))) {
            return _.scanNumber(start);
        }

        // Identifiers, hidden identifiers (_foo), bool_lit, null_lit
        if (isIdentStart(c)) {
            let p = start + 1;
            while (p < _.src.len && isIdentCont(_.src[p])) {
                p += 1;
            }
            const lex = _.src[start..p];
            if (eqBytes(lex, "true") || eqBytes(lex, "false")) {
                return _.emit(CueTokenKind.bool_lit, start, p);
            }
            if (eqBytes(lex, "null")) {
                return _.emit(CueTokenKind.null_lit, start, p);
            }
            if (c == '_' && lex.len > 1) {
                return _.emit(CueTokenKind.hidden_ident, start, p);
            }
            return _.emit(CueTokenKind.ident, start, p);
        }

        return _.emit(CueTokenKind.invalid, start, start + 1);
    }
}

/// 校验 CUE 文本结构平衡（`{}`/`[]`/`()` 匹配、最大 64 层深度、字符串/属性闭合、无非法 token 且非空）
pub fn validateCue(input: []const u8): bool {
    let stack: [64]u8 = undefined;
    let depth: usize = 0;
    let count: usize = 0;
    let it = CueIter.init(input);

    while (tok := it.next()) {
        count += 1;
        if (tok.kind == CueTokenKind.invalid) {
            return false;
        }
        if (tok.kind == CueTokenKind.lbrace) {
            if (depth >= 64) { return false; }
            stack[depth] = '}';
            depth += 1;
        } else if (tok.kind == CueTokenKind.lbracket) {
            if (depth >= 64) { return false; }
            stack[depth] = ']';
            depth += 1;
        } else if (tok.kind == CueTokenKind.lparen) {
            if (depth >= 64) { return false; }
            stack[depth] = ')';
            depth += 1;
        } else if (tok.kind == CueTokenKind.rbrace) {
            if (depth == 0 || stack[depth - 1] != '}') { return false; }
            depth -= 1;
        } else if (tok.kind == CueTokenKind.rbracket) {
            if (depth == 0 || stack[depth - 1] != ']') { return false; }
            depth -= 1;
        } else if (tok.kind == CueTokenKind.rparen) {
            if (depth == 0 || stack[depth - 1] != ')') { return false; }
            depth -= 1;
        }
    }

    return count > 0 && depth == 0;
}

/// 提取文件头 `package <name>` 声明（自动跳过 BOM 与前导 `//` 注释）
pub fn extractCuePackage(input: []const u8): ([]const u8)? {
    let it = CueIter.init(input);
    const first = it.next() ?? return null;
    if (first.kind != CueTokenKind.ident || !eqBytes(first.lexeme, "package")) {
        return null;
    }
    const name = it.next() ?? return null;
    if (name.kind != CueTokenKind.ident) {
        return null;
    }
    return name.lexeme;
}
