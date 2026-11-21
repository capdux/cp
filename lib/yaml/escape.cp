// Source: Absorbed from /data/dev/bun/src/yaml/yaml.zig | Synced: 2026-11-21T15:32:48Z (YAML 1.2 Scalar Quoting & Escaper)

const HEX_LOWER: []const u8 = "0123456789abcdef";

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn hexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn isYamlKeyword(s: []const u8): bool {
    return asciiEqlIgnoreCase(s, "true")
        || asciiEqlIgnoreCase(s, "false")
        || asciiEqlIgnoreCase(s, "null")
        || asciiEqlIgnoreCase(s, "~")
        || asciiEqlIgnoreCase(s, "yes")
        || asciiEqlIgnoreCase(s, "no")
        || asciiEqlIgnoreCase(s, "on")
        || asciiEqlIgnoreCase(s, "off")
        || asciiEqlIgnoreCase(s, ".inf")
        || asciiEqlIgnoreCase(s, "+.inf")
        || asciiEqlIgnoreCase(s, "-.inf")
        || asciiEqlIgnoreCase(s, ".nan")
        || asciiEqlIgnoreCase(s, "---")
        || asciiEqlIgnoreCase(s, "...");
}

fn looksLikeYamlNumber(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    if (s[0] == '+' || s[0] == '-') {
        i += 1;
        if (i >= s.len) return false;
    }

    // 十六进制 0x... / 八进制 0o...
    if (i + 2 <= s.len && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        if (i + 2 >= s.len) return false;
        let k = i + 2;
        while (k < s.len) {
            if (hexNibble(s[k]) == null && s[k] != '_') return false;
            k += 1;
        }
        return true;
    }
    if (i + 2 <= s.len && s[i] == '0' && (s[i + 1] == 'o' || s[i + 1] == 'O')) {
        if (i + 2 >= s.len) return false;
        let k = i + 2;
        while (k < s.len) {
            if ((s[k] < '0' || s[k] > '7') && s[k] != '_') return false;
            k += 1;
        }
        return true;
    }

    // 十进制整数、小数或科学计数
    let has_digit = false;
    let saw_dot = false;
    let saw_exp = false;
    while (i < s.len) {
        const c = s[i];
        if (c >= '0' && c <= '9') {
            has_digit = true;
        } else if (c == '.' && !saw_dot && !saw_exp) {
            saw_dot = true;
        } else if ((c == 'e' || c == 'E') && has_digit && !saw_exp) {
            saw_exp = true;
            has_digit = false;
            if (i + 1 < s.len && (s[i + 1] == '+' || s[i + 1] == '-')) {
                i += 1;
            }
        } else if (c == '_') {
            // 下划线分隔符
        } else {
            return false;
        }
        i += 1;
    }
    return has_digit;
}

fn isFirstIndicator(c: u8): bool {
    return c == '?' || c == '&' || c == '*' || c == '!' || c == '|'
        || c == '>' || c == '\'' || c == '"' || c == '%' || c == '@'
        || c == '`' || c == ',' || c == '-' || c == '[' || c == ']'
        || c == '{' || c == '}' || c == '#' || c == ':';
}

/// 判断字符串作为 YAML 1.2 标量输出时是否必须加引号
export fn needsYamlQuotes(s: []const u8): bool {
    if (s.len == 0) {
        return true;
    }
    if (isSpace(s[0]) || isSpace(s[s.len - 1])) {
        return true;
    }
    if (isFirstIndicator(s[0])) {
        return true;
    }
    if (isYamlKeyword(s) || looksLikeYamlNumber(s)) {
        return true;
    }

    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < 0x20 || c == 0x7F) {
            return true;
        }
        if (c == '[' || c == ']' || c == '{' || c == '}' || c == ',') {
            return true;
        }
        if (c == ':' && (i + 1 == s.len || isSpace(s[i + 1]))) {
            return true;
        }
        if (c == '#' && i > 0 && isSpace(s[i - 1])) {
            return true;
        }
        i += 1;
    }
    return false;
}

struct YamlWriter {
    out: []u8;
    pos: usize;

    fn writeByte(*_, b: u8): bool {
        if (_.pos >= _.out.len) return false;
        _.out[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeSlice(*_, s: []const u8): bool {
        if (_.pos + s.len > _.out.len) return false;
        let i: usize = 0;
        while (i < s.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
        return true;
    }

    fn writeUtf8(*_, cp: u32): bool {
        if (cp > 0x10_FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
            return false;
        }
        if (cp < 0x80) {
            return _.writeByte(u8(cp));
        }
        if (cp < 0x800) {
            if (_.pos + 2 > _.out.len) return false;
            _.out[_.pos] = u8(0xC0) | u8(cp >> 6);
            _.out[_.pos + 1] = u8(0x80) | u8(cp & 0x3F);
            _.pos += 2;
            return true;
        }
        if (cp < 0x1_0000) {
            if (_.pos + 3 > _.out.len) return false;
            _.out[_.pos] = u8(0xE0) | u8(cp >> 12);
            _.out[_.pos + 1] = u8(0x80) | u8((cp >> 6) & 0x3F);
            _.out[_.pos + 2] = u8(0x80) | u8(cp & 0x3F);
            _.pos += 3;
            return true;
        }
        if (_.pos + 4 > _.out.len) return false;
        _.out[_.pos] = u8(0xF0) | u8(cp >> 18);
        _.out[_.pos + 1] = u8(0x80) | u8((cp >> 12) & 0x3F);
        _.out[_.pos + 2] = u8(0x80) | u8((cp >> 6) & 0x3F);
        _.out[_.pos + 3] = u8(0x80) | u8(cp & 0x3F);
        _.pos += 4;
        return true;
    }
}

/// YAML 1.2 单引号转义：内部 `'` 写为 `''`；含控制字符 `< 0x20` 或 `0x7F` 时返回 null
export fn quoteYamlSingle(s: []const u8, out: []u8): ([]const u8)? {
    let w = YamlWriter{ out: out, pos: 0 };
    if (!w.writeByte('\'')) return null;

    for (c of s) {
        if (c < 0x20 || c == 0x7F) {
            return null;
        }
        if (c == '\'') {
            if (!w.writeSlice("''")) return null;
        } else {
            if (!w.writeByte(c)) return null;
        }
    }

    if (!w.writeByte('\'')) return null;
    return out[0..w.pos];
}

/// YAML 1.2 双引号转义：转义 `\"`, `\\`, `\0`, `\a`, `\b`, `\t`, `\n`, `\v`, `\f`, `\r`, `\e` 与 `\xHH`
export fn quoteYamlDouble(s: []const u8, out: []u8): ([]const u8)? {
    let w = YamlWriter{ out: out, pos: 0 };
    if (!w.writeByte('"')) return null;

    for (c of s) {
        if (c == '"') {
            if (!w.writeSlice("\\\"")) return null;
        } else if (c == '\\') {
            if (!w.writeSlice("\\\\")) return null;
        } else if (c == 0x00) {
            if (!w.writeSlice("\\0")) return null;
        } else if (c == 0x07) {
            if (!w.writeSlice("\\a")) return null;
        } else if (c == 0x08) {
            if (!w.writeSlice("\\b")) return null;
        } else if (c == 0x09) {
            if (!w.writeSlice("\\t")) return null;
        } else if (c == 0x0A) {
            if (!w.writeSlice("\\n")) return null;
        } else if (c == 0x0B) {
            if (!w.writeSlice("\\v")) return null;
        } else if (c == 0x0C) {
            if (!w.writeSlice("\\f")) return null;
        } else if (c == 0x0D) {
            if (!w.writeSlice("\\r")) return null;
        } else if (c == 0x1B) {
            if (!w.writeSlice("\\e")) return null;
        } else if (c < 0x20 || c == 0x7F) {
            if (!w.writeSlice("\\x")) return null;
            if (!w.writeByte(HEX_LOWER[usize(c >> 4)])) return null;
            if (!w.writeByte(HEX_LOWER[usize(c & 0x0F)])) return null;
        } else {
            if (!w.writeByte(c)) return null;
        }
    }

    if (!w.writeByte('"')) return null;
    return out[0..w.pos];
}

/// 智能格式化 YAML 1.2 标量：无需引号原样输出，否则使用双引号安全转义
export fn formatYamlScalar(s: []const u8, out: []u8): ([]const u8)? {
    if (!needsYamlQuotes(s)) {
        if (out.len < s.len) return null;
        let i: usize = 0;
        while (i < s.len) {
            out[i] = s[i];
            i += 1;
        }
        return out[0..s.len];
    }
    return quoteYamlDouble(s, out);
}

fn parseHexExact(s: []const u8): u32? {
    let val: u32 = 0;
    for (c of s) {
        const nib = hexNibble(c) ?? return null;
        val = (val << 4) | u32(nib);
    }
    return val;
}

/// 还原 YAML 1.2 双引号字符串转义（支持外层带或不带引号，支持 `\xHH` / `\uXXXX` / `\UXXXXXXXX`）
export fn unescapeYamlDouble(escaped: []const u8, out: []u8): ([]const u8)? {
    let src = escaped;
    if (src.len >= 2 && src[0] == '"' && src[src.len - 1] == '"') {
        src = src[1..src.len - 1];
    }

    let w = YamlWriter{ out: out, pos: 0 };
    let i: usize = 0;

    while (i < src.len) {
        if (src[i] != '\\') {
            if (!w.writeByte(src[i])) return null;
            i += 1;
            continue;
        }

        if (i + 1 >= src.len) {
            return null;
        }
        const esc = src[i + 1];
        if (esc == '"') {
            if (!w.writeByte('"')) return null;
            i += 2;
        } else if (esc == '\\') {
            if (!w.writeByte('\\')) return null;
            i += 2;
        } else if (esc == '/') {
            if (!w.writeByte('/')) return null;
            i += 2;
        } else if (esc == ' ') {
            if (!w.writeByte(' ')) return null;
            i += 2;
        } else if (esc == '0') {
            if (!w.writeByte(0x00)) return null;
            i += 2;
        } else if (esc == 'a') {
            if (!w.writeByte(0x07)) return null;
            i += 2;
        } else if (esc == 'b') {
            if (!w.writeByte(0x08)) return null;
            i += 2;
        } else if (esc == 't') {
            if (!w.writeByte(0x09)) return null;
            i += 2;
        } else if (esc == 'n') {
            if (!w.writeByte(0x0A)) return null;
            i += 2;
        } else if (esc == 'v') {
            if (!w.writeByte(0x0B)) return null;
            i += 2;
        } else if (esc == 'f') {
            if (!w.writeByte(0x0C)) return null;
            i += 2;
        } else if (esc == 'r') {
            if (!w.writeByte(0x0D)) return null;
            i += 2;
        } else if (esc == 'e') {
            if (!w.writeByte(0x1B)) return null;
            i += 2;
        } else if (esc == 'x') {
            if (i + 4 > src.len) return null;
            const cp = parseHexExact(src[i + 2..i + 4]) ?? return null;
            if (!w.writeUtf8(cp)) return null;
            i += 4;
        } else if (esc == 'u') {
            if (i + 6 > src.len) return null;
            const cp = parseHexExact(src[i + 2..i + 6]) ?? return null;
            if (!w.writeUtf8(cp)) return null;
            i += 6;
        } else if (esc == 'U') {
            if (i + 10 > src.len) return null;
            const cp = parseHexExact(src[i + 2..i + 10]) ?? return null;
            if (!w.writeUtf8(cp)) return null;
            i += 10;
        } else {
            return null;
        }
    }

    return out[0..w.pos];
}
