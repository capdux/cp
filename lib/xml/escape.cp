// Reference-hardened against RazrFalcon/roxmltree (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/string/immutable.zig | Synced: 2026-11-22T13:00:28Z (XML 1.0 Text / Attribute / CDATA Escaper)

type XmlEscVec16 = @Vector(16, u8);

fn makeXmlHexLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0xFF; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= '0' && i <= '9') {
            lut[i] = @as(u8, @intCast(i - '0'));
        } else if (i >= 'a' && i <= 'f') {
            lut[i] = @as(u8, @intCast(i - 'a' + 10));
        } else if (i >= 'A' && i <= 'F') {
            lut[i] = @as(u8, @intCast(i - 'A' + 10));
        }
    }
    return lut;
}

const XML_HEX_LUT: [u8; 256] = makeXmlHexLut();

fn findByteSimd(s: []const u8, start: usize, needle: u8): usize {
    let i = start;
    const v_needle: XmlEscVec16 = @splat(needle);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: XmlEscVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(chunk == v_needle);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == needle) return i;
    }
    return s.len;
}

fn findXmlTextSpecialSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_amp: XmlEscVec16 = @splat('&' as u8);
    const v_lt: XmlEscVec16 = @splat('<' as u8);
    const v_gt: XmlEscVec16 = @splat('>' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: XmlEscVec16 = s[i .. i + 16][0..16].*;
        const hit = (chunk == v_amp) | (chunk == v_lt) | (chunk == v_gt);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '&' || c == '<' || c == '>') return i;
    }
    return s.len;
}

fn findXmlAttrSpecialSimd(s: []const u8, start: usize): usize {
    let i = start;
    const v_amp: XmlEscVec16 = @splat('&' as u8);
    const v_lt: XmlEscVec16 = @splat('<' as u8);
    const v_gt: XmlEscVec16 = @splat('>' as u8);
    const v_quot: XmlEscVec16 = @splat('"' as u8);
    const v_apos: XmlEscVec16 = @splat('\'' as u8);
    const v_cr: XmlEscVec16 = @splat('\r' as u8);
    const v_lf: XmlEscVec16 = @splat('\n' as u8);
    const v_tab: XmlEscVec16 = @splat('\t' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const chunk: XmlEscVec16 = s[i .. i + 16][0..16].*;
        const hit = (chunk == v_amp) | (chunk == v_lt) | (chunk == v_gt) |
            (chunk == v_quot) | (chunk == v_apos) |
            (chunk == v_cr) | (chunk == v_lf) | (chunk == v_tab);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + usize(@ctz(mask));
        }
    }
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'' || c == '\r' || c == '\n' || c == '\t') {
            return i;
        }
    }
    return s.len;
}

/// XML 1.0 Fifth Edition §2.2 合法字符码点校验
pub fn isValidXmlChar(cp: u32): bool {
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
    while (i + 16 <= a.len) : (i += 16) {
        const va: XmlEscVec16 = a[i .. i + 16][0..16].*;
        const vb: XmlEscVec16 = b[i .. i + 16][0..16].*;
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

struct XmlEscapeBuf {
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
        if (s.len > 0) {
            @memcpy(_.out[_.pos .. _.pos + s.len], s);
            _.pos += s.len;
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
pub fn escapeXmlText(src: []const u8, out: []u8): ([]const u8)? {
    let w = XmlEscapeBuf{ out: out, pos: 0 };
    let i: usize = 0;
    while (i < src.len) {
        const next = findXmlTextSpecialSimd(src, i);
        if (next > i) {
            if (!w.writeSlice(src[i..next])) return null;
        }
        if (next >= src.len) break;
        const c = src[next];
        if (c == '&') {
            if (!w.writeSlice("&amp;")) return null;
        } else if (c == '<') {
            if (!w.writeSlice("&lt;")) return null;
        } else {
            if (!w.writeSlice("&gt;")) return null;
        }
        i = next + 1;
    }
    return out[0..w.pos];
}

/// 转义 XML 1.0 属性值（转义 `&`, `<`, `>`, `"`, `'`, `\r`, `\n`, `\t`）
pub fn escapeXmlAttr(src: []const u8, out: []u8): ([]const u8)? {
    let w = XmlEscapeBuf{ out: out, pos: 0 };
    let i: usize = 0;
    while (i < src.len) {
        const next = findXmlAttrSpecialSimd(src, i);
        if (next > i) {
            if (!w.writeSlice(src[i..next])) return null;
        }
        if (next >= src.len) break;
        const c = src[next];
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
        } else {
            if (!w.writeSlice("&#x9;")) return null;
        }
        i = next + 1;
    }
    return out[0..w.pos];
}

/// 用 `<![CDATA[...]]>` 包裹文本，遇到内部 `]]>` 自动安全拆分为 `]]]]><![CDATA[>`
pub fn wrapXmlCdata(src: []const u8, out: []u8): ([]const u8)? {
    let w = XmlEscapeBuf{ out: out, pos: 0 };
    if (!w.writeSlice("<![CDATA[")) return null;

    let i: usize = 0;
    while (i < src.len) {
        const next = findByteSimd(src, i, ']');
        if (next > i) {
            if (!w.writeSlice(src[i..next])) return null;
        }
        if (next >= src.len) break;
        if (next + 2 < src.len && src[next + 1] == ']' && src[next + 2] == '>') {
            if (!w.writeSlice("]]]]><![CDATA[>")) return null;
            i = next + 3;
        } else {
            if (!w.writeByte(']')) return null;
            i = next + 1;
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
        const d = XML_HEX_LUT[usize(c)];
        if (d == 0xFF) return null;
        val = (val << 4) | u32(d);
        if (val > 0x10_FFFF) return null;
    }
    return val;
}

/// 反转义 XML 1.0 文本/属性：还原 5 个预定义实体及十进制/十六进制数字字符引用并严格校验 isValidXmlChar
pub fn unescapeXml(src: []const u8, out: []u8): ([]const u8)? {
    let w = XmlEscapeBuf{ out: out, pos: 0 };
    let i: usize = 0;

    while (i < src.len) {
        const amp = findByteSimd(src, i, '&');
        if (amp > i) {
            if (!w.writeSlice(src[i..amp])) return null;
        }
        if (amp >= src.len) break;
        i = amp;

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
