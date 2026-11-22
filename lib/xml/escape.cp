// Source: Absorbed from /data/dev/bun/src/string/immutable.zig | Synced: 2026-11-22T12:49:17Z (XML 1.0 Text / Attribute / CDATA Escaper)

/// XML 1.0 Fifth Edition §2.2 合法字符码点校验
export fn isValidXmlChar(cp: u32): bool {
    if (cp == 0x09 || cp == 0x0A || cp == 0x0D) return true;
    if (cp >= 0x20 && cp <= 0xD7FF) return true;
    if (cp >= 0xE000 && cp <= 0xFFFD) return true;
    if (cp >= 0x1_0000 && cp <= 0x10_FFFF) return true;
    return false;
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

struct XmlWriter {
    out: []u8;
    pos: usize;

    fn writeByte(*_, b: u8): bool {
        if (_.pos >= _.out.len) {
            return false;
        }
        _.out[_.pos] = b;
        _.pos += 1;
        return true;
    }

    fn writeSlice(*_, s: []const u8): bool {
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

    fn writeUtf8(*_, cp: u32): bool {
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

/// 转义 XML 1.0 元素文本节点（转义 `&`, `<`, `>`，并安全消解 `]]>`）
export fn escapeXmlText(src: []const u8, out: []u8): ([]const u8)? {
    let w = XmlWriter{ out: out, pos: 0 };
    for (c of src) {
        if (c == '&') {
            if (!w.writeSlice("&amp;")) return null;
        } else if (c == '<') {
            if (!w.writeSlice("&lt;")) return null;
        } else if (c == '>') {
            if (!w.writeSlice("&gt;")) return null;
        } else {
            if (!w.writeByte(c)) return null;
        }
    }
    return out[0..w.pos];
}

/// 转义 XML 1.0 属性值（转义 `&`, `<`, `>`, `"`, `'`, `\r`, `\n`, `\t`）
export fn escapeXmlAttr(src: []const u8, out: []u8): ([]const u8)? {
    let w = XmlWriter{ out: out, pos: 0 };
    for (c of src) {
        if (c == '&') {
            if (!w.writeSlice("&amp;")) return null;
        } else if (c == '<') {
            if (!w.writeSlice("&lt;")) return null;
        } else if (c == '>') {
            if (!w.writeSlice("&gt;")) return null;
        } else if (c == '"') {
            if (!w.writeSlice("&quot;")) return null;
        } else if (c == '\'') {
            if (!w.writeSlice("&apos;")) return null;
        } else if (c == '\r') {
            if (!w.writeSlice("&#xD;")) return null;
        } else if (c == '\n') {
            if (!w.writeSlice("&#xA;")) return null;
        } else if (c == '\t') {
            if (!w.writeSlice("&#x9;")) return null;
        } else {
            if (!w.writeByte(c)) return null;
        }
    }
    return out[0..w.pos];
}

/// 用 `<![CDATA[...]]>` 包裹文本，遇到内部 `]]>` 自动安全拆分为 `]]]]><![CDATA[>`
export fn wrapXmlCdata(src: []const u8, out: []u8): ([]const u8)? {
    let w = XmlWriter{ out: out, pos: 0 };
    if (!w.writeSlice("<![CDATA[")) return null;

    let i: usize = 0;
    while (i < src.len) {
        if (i + 2 < src.len && src[i] == ']' && src[i + 1] == ']' && src[i + 2] == '>') {
            if (!w.writeSlice("]]]]><![CDATA[>")) return null;
            i += 3;
        } else {
            if (!w.writeByte(src[i])) return null;
            i += 1;
        }
    }

    if (!w.writeSlice("]]>")) return null;
    return out[0..w.pos];
}

fn parseDecCodePoint(s: []const u8): u32? {
    if (s.len == 0 || s.len > 8) return null;
    let val: u32 = 0;
    for (c of s) {
        if (c < '0' || c > '9') return null;
        val = val * 10 + u32(c - '0');
        if (val > 0x10_FFFF) return null;
    }
    return val;
}

fn parseHexCodePoint(s: []const u8): u32? {
    if (s.len == 0 || s.len > 6) return null;
    let val: u32 = 0;
    for (c of s) {
        let d: u32 = 0;
        if (c >= '0' && c <= '9') {
            d = u32(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            d = u32(c - 'a') + 10;
        } else if (c >= 'A' && c <= 'F') {
            d = u32(c - 'A') + 10;
        } else {
            return null;
        }
        val = (val << 4) | d;
        if (val > 0x10_FFFF) return null;
    }
    return val;
}

/// 反转义 XML 1.0 文本/属性：还原 5 个预定义实体及十进制/十六进制数字字符引用并严格校验 isValidXmlChar
export fn unescapeXml(src: []const u8, out: []u8): ([]const u8)? {
    let w = XmlWriter{ out: out, pos: 0 };
    let i: usize = 0;

    while (i < src.len) {
        if (src[i] != '&') {
            if (!w.writeByte(src[i])) return null;
            i += 1;
            continue;
        }

        // 查找闭合分号 ';'
        let semi: usize? = null;
        let scan = i + 1;
        while (scan < src.len && scan - i <= 12) {
            if (src[scan] == ';') {
                semi = scan;
                break;
            }
            if (src[scan] == '&') {
                return null;
            }
            scan += 1;
        }
        const semi_idx = semi ?? return null;
        const body = src[i + 1..semi_idx];
        if (body.len == 0) {
            return null;
        }

        if (body[0] == '#') {
            let cp: u32 = 0;
            if (body.len >= 2 && body[1] == 'x') {
                cp = parseHexCodePoint(body[2..]) ?? return null;
            } else {
                cp = parseDecCodePoint(body[1..]) ?? return null;
            }
            if (!isValidXmlChar(cp)) {
                return null;
            }
            if (!w.writeUtf8(cp)) return null;
        } else if (strEql(body, "amp")) {
            if (!w.writeByte('&')) return null;
        } else if (strEql(body, "lt")) {
            if (!w.writeByte('<')) return null;
        } else if (strEql(body, "gt")) {
            if (!w.writeByte('>')) return null;
        } else if (strEql(body, "quot")) {
            if (!w.writeByte('"')) return null;
        } else if (strEql(body, "apos")) {
            if (!w.writeByte('\'')) return null;
        } else {
            return null;
        }

        i = semi_idx + 1;
    }

    return out[0..w.pos];
}
