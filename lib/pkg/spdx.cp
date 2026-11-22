// Source: Absorbed from /data/dev/bun/src/install/PackageManager.zig | Synced: 2026-11-22T08:20:58Z (SPDX 2.3 License Expression Parser & Validator)

/// SPDX 2.3 词法记号类型
export enum SpdxTokenKind {
    ident,
    and_op,
    or_op,
    with_op,
    lparen,
    rparen,
    invalid,
}

/// SPDX 2.3 词法记号（零拷贝切片）
export struct SpdxToken {
    kind: SpdxTokenKind;
    text: []const u8;
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isIdChar(c: u8): bool {
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || (c >= '0' && c <= '9')
        || c == '-'
        || c == '.';
}

fn strEql(a: []const u8, b: []const u8): bool {
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return strEql(s[0..prefix.len], prefix);
}

fn isValidLicenseId(tok: []const u8): bool {
    if (tok.len == 0) {
        return false;
    }
    // 允许可选尾部单个 '+'（如 GPL-2.0+）
    let body = tok;
    if (body[body.len - 1] == '+') {
        body = body[0..body.len - 1];
        if (body.len == 0) {
            return false;
        }
    }

    // 检查是否包含 DocumentRef-xxx:LicenseRef-yyy 冒号分隔
    let colon_idx: usize? = null;
    let i: usize = 0;
    while (i < body.len) {
        if (body[i] == ':') {
            if (colon_idx != null) {
                return false;
            }
            colon_idx = i;
        } else if (!isIdChar(body[i])) {
            return false;
        }
        i += 1;
    }

    if (ci := colon_idx) {
        const doc_part = body[0..ci];
        const lic_part = body[ci + 1..];
        if (!startsWith(doc_part, "DocumentRef-") || doc_part.len <= 12) {
            return false;
        }
        if (!startsWith(lic_part, "LicenseRef-") || lic_part.len <= 11) {
            return false;
        }
        return true;
    }

    if (startsWith(body, "LicenseRef-") && body.len <= 11) {
        return false;
    }
    return true;
}

fn isValidExceptionId(tok: []const u8): bool {
    if (tok.len == 0) {
        return false;
    }
    // 异常 ID 不能带 '+' 后缀，不能含 ':'
    for (c of tok) {
        if (!isIdChar(c)) {
            return false;
        }
    }
    return true;
}

/// SPDX 2.3 零堆分配词法扫描器
export struct SpdxLexer {
    src: []const u8;
    pos: usize;

    fn init(expr: []const u8): SpdxLexer {
        return SpdxLexer{
            src: expr,
            pos: 0,
        };
    }

    /// 扫描下一个 SPDX 词法记号；到达末尾返回 null
    fn next(*_): SpdxToken? {
        while (_.pos < _.src.len && isSpace(_.src[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.src.len) {
            return null;
        }

        const c = _.src[_.pos];
        if (c == '(') {
            const s = _.src[_.pos.._.pos + 1];
            _.pos += 1;
            return SpdxToken{ kind: SpdxTokenKind.lparen, text: s };
        }
        if (c == ')') {
            const s = _.src[_.pos.._.pos + 1];
            _.pos += 1;
            return SpdxToken{ kind: SpdxTokenKind.rparen, text: s };
        }

        if (isIdChar(c)) {
            const start = _.pos;
            while (_.pos < _.src.len && (isIdChar(_.src[_.pos]) || _.src[_.pos] == ':' || _.src[_.pos] == '+')) {
                _.pos += 1;
            }
            const text = _.src[start.._.pos];
            if (strEql(text, "AND")) {
                return SpdxToken{ kind: SpdxTokenKind.and_op, text: text };
            }
            if (strEql(text, "OR")) {
                return SpdxToken{ kind: SpdxTokenKind.or_op, text: text };
            }
            if (strEql(text, "WITH")) {
                return SpdxToken{ kind: SpdxTokenKind.with_op, text: text };
            }
            if (isValidLicenseId(text)) {
                return SpdxToken{ kind: SpdxTokenKind.ident, text: text };
            }
            return SpdxToken{ kind: SpdxTokenKind.invalid, text: text };
        }

        const bad = _.src[_.pos.._.pos + 1];
        _.pos += 1;
        return SpdxToken{ kind: SpdxTokenKind.invalid, text: bad };
    }

    fn peek(): SpdxToken? {
        let copy = SpdxLexer{ src: _.src, pos: _.pos };
        return copy.next();
    }
}

struct SpdxParser {
    lexer: SpdxLexer;

    fn parseCompound(*_, depth: u8): bool {
        if (depth > 64) {
            return false;
        }
        if (!_.parseAtom(depth)) {
            return false;
        }
        while (true) {
            if (tok := _.lexer.peek()) {
                if (tok.kind == SpdxTokenKind.and_op || tok.kind == SpdxTokenKind.or_op) {
                    * = _.lexer.next();
                    if (!_.parseAtom(depth)) {
                        return false;
                    }
                    continue;
                }
            }
            break;
        }
        return true;
    }

    fn parseAtom(*_, depth: u8): bool {
        const tok = _.lexer.next() ?? return false;

        if (tok.kind == SpdxTokenKind.lparen) {
            if (!_.parseCompound(depth + 1)) {
                return false;
            }
            const close = _.lexer.next() ?? return false;
            return close.kind == SpdxTokenKind.rparen;
        }

        if (tok.kind == SpdxTokenKind.ident) {
            if (nxt := _.lexer.peek()) {
                if (nxt.kind == SpdxTokenKind.with_op) {
                    * = _.lexer.next();
                    const exc = _.lexer.next() ?? return false;
                    if (exc.kind != SpdxTokenKind.ident || !isValidExceptionId(exc.text)) {
                        return false;
                    }
                }
            }
            return true;
        }

        return false;
    }
}

/// 严格校验 SPDX 2.3 许可证表达式语法（括号平衡、WITH 左右约束、禁止空括号与连续/尾随运算符）
export fn isValidSpdxExpression(expr: []const u8): bool {
    let parser = SpdxParser{ lexer: SpdxLexer.init(expr) };
    if (!parser.parseCompound(0)) {
        return false;
    }
    return parser.lexer.next() == null;
}

/// 迭代提取 SPDX 表达式中的所有许可证 ID（自动跳过 AND/OR/WITH 与异常 ID）
export struct SpdxIdIter {
    lexer: SpdxLexer;
    skip_exception: bool;

    fn init(expr: []const u8): SpdxIdIter {
        return SpdxIdIter{
            lexer: SpdxLexer.init(expr),
            skip_exception: false,
        };
    }

    fn next(*_): ([]const u8)? {
        while (true) {
            if (tok := _.lexer.next()) {
                if (tok.kind == SpdxTokenKind.with_op) {
                    _.skip_exception = true;
                    continue;
                }
                if (tok.kind == SpdxTokenKind.ident) {
                    if (_.skip_exception) {
                        _.skip_exception = false;
                        continue;
                    }
                    return tok.text;
                }
                if (tok.kind == SpdxTokenKind.invalid) {
                    return null;
                }
            } else {
                return null;
            }
        }
    }
}

/// 判断给定许可证 ID 是否属于常见宽松开源许可证（Permissive）
export fn isCommonPermissive(id: []const u8): bool {
    return strEql(id, "MIT")
        || strEql(id, "ISC")
        || strEql(id, "BSD-2-Clause")
        || strEql(id, "BSD-3-Clause")
        || strEql(id, "Apache-2.0")
        || strEql(id, "0BSD")
        || strEql(id, "Unlicense")
        || strEql(id, "CC0-1.0")
        || strEql(id, "Zlib")
        || strEql(id, "BlueOak-1.0.0");
}
