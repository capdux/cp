// Source: Absorbed from /data/dev/bun/src/js_parser/lexer/ | Synced: 2026-11-21T17:06:09Z

/// ECMAScript / TypeScript 关键字分类
export enum KeywordKind {
    none,
    reserved,
    strict_reserved,
    contextual,
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

/// 分类 ECMAScript 2024 / TypeScript 关键字
export fn classifyKeyword(word: []const u8): KeywordKind {
    if (word.len < 2 || word.len > 10) {
        return KeywordKind.none;
    }

    // 36 个 ES2024 严格保留字
    if (strEql(word, "break")
        || strEql(word, "case")
        || strEql(word, "catch")
        || strEql(word, "class")
        || strEql(word, "const")
        || strEql(word, "continue")
        || strEql(word, "debugger")
        || strEql(word, "default")
        || strEql(word, "delete")
        || strEql(word, "do")
        || strEql(word, "else")
        || strEql(word, "enum")
        || strEql(word, "export")
        || strEql(word, "extends")
        || strEql(word, "false")
        || strEql(word, "finally")
        || strEql(word, "for")
        || strEql(word, "function")
        || strEql(word, "if")
        || strEql(word, "import")
        || strEql(word, "in")
        || strEql(word, "instanceof")
        || strEql(word, "new")
        || strEql(word, "null")
        || strEql(word, "return")
        || strEql(word, "super")
        || strEql(word, "switch")
        || strEql(word, "this")
        || strEql(word, "throw")
        || strEql(word, "true")
        || strEql(word, "try")
        || strEql(word, "typeof")
        || strEql(word, "var")
        || strEql(word, "void")
        || strEql(word, "while")
        || strEql(word, "with"))
    {
        return KeywordKind.reserved;
    }

    // 10 个严格模式保留字
    if (strEql(word, "implements")
        || strEql(word, "interface")
        || strEql(word, "let")
        || strEql(word, "package")
        || strEql(word, "private")
        || strEql(word, "protected")
        || strEql(word, "public")
        || strEql(word, "static")
        || strEql(word, "yield")
        || strEql(word, "await"))
    {
        return KeywordKind.strict_reserved;
    }

    // 10 个常用上下文关键字
    if (strEql(word, "as")
        || strEql(word, "async")
        || strEql(word, "from")
        || strEql(word, "get")
        || strEql(word, "meta")
        || strEql(word, "of")
        || strEql(word, "set")
        || strEql(word, "target")
        || strEql(word, "type")
        || strEql(word, "satisfies"))
    {
        return KeywordKind.contextual;
    }

    return KeywordKind.none;
}

/// 判断词法单元在指定模式下是否为保留字
export fn isReservedWord(word: []const u8, strict_mode: bool): bool {
    const kind = classifyKeyword(word);
    if (kind == KeywordKind.reserved) {
        return true;
    }
    if (strict_mode && kind == KeywordKind.strict_reserved) {
        return true;
    }
    return false;
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

fn isIdentContinue(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

/// 判断字符串是否为合法 ASCII JS 标识符（`[A-Za-z_$][A-Za-z0-9_$]*`）
export fn isValidAsciiIdent(s: []const u8): bool {
    if (s.len == 0 || !isIdentStart(s[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!isIdentContinue(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 判断字符串是否为合法 ES2022 私有类字段标识符（`#` + 合法标识符）
export fn isValidPrivateIdent(s: []const u8): bool {
    if (s.len < 2 || s[0] != '#') {
        return false;
    }
    return isValidAsciiIdent(s[1..]);
}

/// 判断对象属性键在 JS 代码生成中是否需要加引号
export fn needsPropertyQuoting(key: []const u8, strict_mode: bool): bool {
    if (!isValidAsciiIdent(key)) {
        return true;
    }
    return isReservedWord(key, strict_mode);
}

struct StrWriter {
    out: []u8;
    pos: usize;

    fn put(*_, b: u8): bool {
        if (_.pos >= _.out.len) {
            return false;
        }
        _.out[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn putSlice(*_, s: []const u8): bool {
        if (_.pos + s.len > _.out.len) {
            return false;
        }
        let i: usize = 0;
        while (i < s.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    fn putHexByte(*_, b: u8): bool {
        const hex = "0123456789abcdef";
        if (!_.put(hex[usize(b >> 4)])) return false;
        return _.put(hex[usize(b & 0x0F)]);
    }
}

/// 将原始 UTF-8 字符串安全转义为带外层引号的 JS 字符串字面量（零堆分配）
export fn escapeJsString(raw: []const u8, quote: u8, out: []u8): ([]const u8)? {
    let w = StrWriter{ out: out, pos: 0 };
    if (!w.put(quote)) return null;

    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];

        // UTF-8 编码的 U+2028 (E2 80 A8) 与 U+2029 (E2 80 A9) 行分隔符
        if (c == 0xE2 && i + 2 < raw.len && raw[i + 1] == 0x80 && (raw[i + 2] == 0xA8 || raw[i + 2] == 0xA9)) {
            if (raw[i + 2] == 0xA8) {
                if (!w.putSlice("\\u2028")) return null;
            } else {
                if (!w.putSlice("\\u2029")) return null;
            }
            i += 3;
            continue;
        }

        if (c == quote) {
            if (!w.put('\\')) return null;
            if (!w.put(quote)) return null;
        } else if (c == '\\') {
            if (!w.putSlice("\\\\")) return null;
        } else if (c == '\n') {
            if (!w.putSlice("\\n")) return null;
        } else if (c == '\r') {
            if (!w.putSlice("\\r")) return null;
        } else if (c == '\t') {
            if (!w.putSlice("\\t")) return null;
        } else if (c == 0x08) {
            if (!w.putSlice("\\b")) return null;
        } else if (c == 0x0C) {
            if (!w.putSlice("\\f")) return null;
        } else if (c == 0x00) {
            const next_is_digit = (i + 1 < raw.len) && (raw[i + 1] >= '0' && raw[i + 1] <= '9');
            if (next_is_digit) {
                if (!w.putSlice("\\x00")) return null;
            } else {
                if (!w.putSlice("\\0")) return null;
            }
        } else if (c < 0x20) {
            if (!w.putSlice("\\x")) return null;
            if (!w.putHexByte(c)) return null;
        } else {
            if (!w.put(c)) return null;
        }

        i += 1;
    }

    if (!w.put(quote)) return null;
    return out[0..w.pos];
}
