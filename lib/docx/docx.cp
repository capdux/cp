// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc OOXML DOCX WordprocessingML & CoreProps Parser)

/// OOXML WordprocessingML 主文档 Content-Type (`[Content_Types].xml`)
pub const DOCX_MAIN_CONTENT_TYPE: []const u8 = "application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml";

/// `docProps/core.xml` 零拷贝核心元数据视图
pub struct DocxCoreProps {
    title: []const u8 = "",
    subject: []const u8 = "",
    creator: []const u8 = "",
    keywords: []const u8 = "",
    description: []const u8 = "",
    last_modified_by: []const u8 = "",
    revision: []const u8 = "",
    created: []const u8 = "",
    modified: []const u8 = "",
}

/// 零拷贝 WordprocessingML 段落视图
pub struct DocxParagraph {
    raw_xml: []const u8,
    style: []const u8,
    is_heading: bool,
}

/// WordprocessingML 文档统计汇总
pub struct DocxStats {
    paragraph_count: usize = 0,
    heading_count: usize = 0,
    table_count: usize = 0,
    run_count: usize = 0,
}

fn isXmlSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isTagDelim(c: u8): bool {
    return isXmlSpace(c) || c == '>' || c == '/';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (haystack[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCase(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (toLowerAscii(haystack[i]) != toLowerAscii(prefix[i])) return false;
        i += 1;
    }
    return true;
}

fn containsSlice(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        if (haystack[i] == needle[0]) {
            let j: usize = 1;
            while (j < needle.len && haystack[i + j] == needle[j]) {
                j += 1;
            }
            if (j == needle.len) return true;
        }
        i += 1;
    }
    return false;
}

fn localName(qname: []const u8): []const u8 {
    let i: usize = 0;
    while (i < qname.len) {
        if (qname[i] == ':') {
            return qname[i + 1 .. qname.len];
        }
        i += 1;
    }
    return qname;
}

fn isWordTag(qname: []const u8, local: []const u8): bool {
    if (strEql(qname, local)) return true;
    if (qname.len == local.len + 2 && qname[0] == 'w' && qname[1] == ':') {
        return strEql(qname[2..qname.len], local);
    }
    return false;
}

fn isHeadingStyle(style: []const u8): bool {
    return startsWithIgnoreCase(style, "heading");
}

fn skipComment(xml: []const u8, pos: usize): usize {
    let i = pos + 4;
    while (i + 2 < xml.len) {
        if (xml[i] == '-' && xml[i + 1] == '-' && xml[i + 2] == '>') {
            return i + 3;
        }
        i += 1;
    }
    return xml.len;
}

fn skipCdata(xml: []const u8, pos: usize): usize {
    let i = pos + 9;
    while (i + 2 < xml.len) {
        if (xml[i] == ']' && xml[i + 1] == ']' && xml[i + 2] == '>') {
            return i + 3;
        }
        i += 1;
    }
    return xml.len;
}

fn skipPi(xml: []const u8, pos: usize): usize {
    let i = pos + 2;
    while (i + 1 < xml.len) {
        if (xml[i] == '?' && xml[i + 1] == '>') {
            return i + 2;
        }
        i += 1;
    }
    return xml.len;
}

fn skipDecl(xml: []const u8, pos: usize): usize {
    let i = pos + 2;
    let in_quote: u8 = 0;
    while (i < xml.len) {
        const c = xml[i];
        if (in_quote != 0) {
            if (c == in_quote) in_quote = 0;
        } else if (c == '"' || c == '\'') {
            in_quote = c;
        } else if (c == '>') {
            return i + 1;
        }
        i += 1;
    }
    return xml.len;
}

fn findTagEnd(xml: []const u8, lt_pos: usize): usize? {
    let i = lt_pos + 1;
    let in_quote: u8 = 0;
    while (i < xml.len) {
        const c = xml[i];
        if (in_quote != 0) {
            if (c == in_quote) in_quote = 0;
        } else if (c == '"' || c == '\'') {
            in_quote = c;
        } else if (c == '>') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn parseTagName(xml: []const u8, name_start: usize, tag_end: usize): []const u8 {
    let i = name_start;
    while (i < tag_end && !isTagDelim(xml[i])) {
        i += 1;
    }
    return xml[name_start..i];
}

fn isSelfClosingTag(xml: []const u8, lt_pos: usize, tag_end: usize): bool {
    if (tag_end <= lt_pos + 1) return false;
    let i = tag_end - 1;
    while (i > lt_pos && isXmlSpace(xml[i])) {
        i -= 1;
    }
    return xml[i] == '/';
}

/// 跳过 UTF-8 BOM、空白、XML 声明/PI、注释和 DOCTYPE，定位首个根元素 `<`
fn skipProlog(xml: []const u8): usize {
    let pos: usize = 0;
    if (xml.len >= 3 && xml[0] == 0xEF && xml[1] == 0xBB && xml[2] == 0xBF) {
        pos = 3;
    }
    while (pos < xml.len) {
        while (pos < xml.len && isXmlSpace(xml[pos])) {
            pos += 1;
        }
        if (pos >= xml.len || xml[pos] != '<') {
            return pos;
        }
        if (startsWith(xml[pos..xml.len], "<!--")) {
            pos = skipComment(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<?")) {
            pos = skipPi(xml, pos);
            continue;
        }
        if (startsWith(xml[pos..xml.len], "<!")) {
            pos = skipDecl(xml, pos);
            continue;
        }
        return pos;
    }
    return pos;
}

/// 从单个 XML 开始/自闭合标签切片中提取指定本地名的属性值（零拷贝）
fn extractAttrValue(tag: []const u8, target_local: []const u8): ([]const u8)? {
    if (tag.len < 3 || tag[0] != '<') return null;
    let i: usize = 1;
    while (i < tag.len && !isTagDelim(tag[i])) {
        i += 1;
    }
    while (i < tag.len) {
        while (i < tag.len && (isXmlSpace(tag[i]) || tag[i] == '/')) {
            i += 1;
        }
        if (i >= tag.len || tag[i] == '>') break;
        const name_start = i;
        while (i < tag.len && !isXmlSpace(tag[i]) && tag[i] != '=' && tag[i] != '>' && tag[i] != '/') {
            i += 1;
        }
        const attr_name = tag[name_start..i];
        while (i < tag.len && isXmlSpace(tag[i])) {
            i += 1;
        }
        if (i >= tag.len || tag[i] != '=') {
            continue;
        }
        i += 1;
        while (i < tag.len && isXmlSpace(tag[i])) {
            i += 1;
        }
        if (i >= tag.len) break;
        const quote = tag[i];
        if (quote != '"' && quote != '\'') {
            while (i < tag.len && !isXmlSpace(tag[i]) && tag[i] != '>' && tag[i] != '/') {
                i += 1;
            }
            continue;
        }
        i += 1;
        const val_start = i;
        while (i < tag.len && tag[i] != quote) {
            i += 1;
        }
        if (i >= tag.len) break;
        const val = tag[val_start..i];
        i += 1;
        if (strEql(localName(attr_name), target_local)) {
            return val;
        }
    }
    return null;
}

/// 从段落 XML 中提取 `<w:pStyle w:val="..."/>` 样式名
fn extractParagraphStyle(para_xml: []const u8): []const u8 {
    let pos: usize = 0;
    while (pos < para_xml.len) {
        if (para_xml[pos] != '<') {
            pos += 1;
            continue;
        }
        if (startsWith(para_xml[pos..para_xml.len], "<!--")) {
            pos = skipComment(para_xml, pos);
            continue;
        }
        if (startsWith(para_xml[pos..para_xml.len], "<![CDATA[")) {
            pos = skipCdata(para_xml, pos);
            continue;
        }
        if (startsWith(para_xml[pos..para_xml.len], "<?")) {
            pos = skipPi(para_xml, pos);
            continue;
        }
        const tag_end = findTagEnd(para_xml, pos) ?? break;
        if (pos + 1 < tag_end && para_xml[pos + 1] != '/') {
            const name = parseTagName(para_xml, pos + 1, tag_end);
            if (isWordTag(name, "pStyle")) {
                if (v := extractAttrValue(para_xml[pos .. tag_end + 1], "val")) {
                    return v;
                }
            } else if (isWordTag(name, "r")) {
                // `<w:pPr>` always precedes `<w:r>` runs; stop early if no style found
                return "";
            }
        }
        pos = tag_end + 1;
    }
    return "";
}

/// 判定是否为 WordprocessingML `word/document.xml`（检测 `<w:document` 或 `<document` 根标签）
pub fn isWordDocumentXml(xml: []const u8): bool {
    const pos = skipProlog(xml);
    if (pos >= xml.len || xml[pos] != '<') return false;
    if (pos + 1 >= xml.len || xml[pos + 1] == '/') return false;
    const tag_end = findTagEnd(xml, pos) ?? return false;
    const name = parseTagName(xml, pos + 1, tag_end);
    return isWordTag(name, "document");
}

/// 判定 `[Content_Types].xml` 是否声明了 DOCX 主文档类型
pub fn isDocxContentTypes(xml: []const u8): bool {
    return containsSlice(xml, DOCX_MAIN_CONTENT_TYPE);
}

/// 解析 `docProps/core.xml` 元数据（零拷贝，兼容 `dc:` / `cp:` / `dcterms:` 命名空间前缀）
pub fn parseDocxCoreProps(core_xml: []const u8): DocxCoreProps? {
    let pos = skipProlog(core_xml);
    if (pos >= core_xml.len || core_xml[pos] != '<') return null;
    if (pos + 1 >= core_xml.len || core_xml[pos + 1] == '/') return null;

    const root_end = findTagEnd(core_xml, pos) ?? return null;
    const root_name = parseTagName(core_xml, pos + 1, root_end);
    if (!strEql(localName(root_name), "coreProperties")) {
        return null;
    }

    let props = DocxCoreProps {};
    if (isSelfClosingTag(core_xml, pos, root_end)) {
        return props;
    }

    pos = root_end + 1;
    while (pos < core_xml.len) {
        if (core_xml[pos] != '<') {
            pos += 1;
            continue;
        }
        if (startsWith(core_xml[pos..core_xml.len], "<!--")) {
            pos = skipComment(core_xml, pos);
            continue;
        }
        if (startsWith(core_xml[pos..core_xml.len], "<![CDATA[")) {
            pos = skipCdata(core_xml, pos);
            continue;
        }
        if (startsWith(core_xml[pos..core_xml.len], "<?")) {
            pos = skipPi(core_xml, pos);
            continue;
        }
        const tag_end = findTagEnd(core_xml, pos) ?? break;
        if (pos + 1 < tag_end && core_xml[pos + 1] == '/') {
            const close_name = parseTagName(core_xml, pos + 2, tag_end);
            if (strEql(localName(close_name), "coreProperties")) {
                break;
            }
            pos = tag_end + 1;
            continue;
        }

        const tag_name = parseTagName(core_xml, pos + 1, tag_end);
        const lname = localName(tag_name);
        if (isSelfClosingTag(core_xml, pos, tag_end)) {
            pos = tag_end + 1;
            continue;
        }

        const content_start = tag_end + 1;
        let scan = content_start;
        let close_start = content_start;
        let found_close = false;
        while (scan < core_xml.len) {
            if (core_xml[scan] != '<') {
                scan += 1;
                continue;
            }
            if (startsWith(core_xml[scan..core_xml.len], "<!--")) {
                scan = skipComment(core_xml, scan);
                continue;
            }
            if (startsWith(core_xml[scan..core_xml.len], "<![CDATA[")) {
                scan = skipCdata(core_xml, scan);
                continue;
            }
            const c_end = findTagEnd(core_xml, scan) ?? break;
            if (scan + 1 < c_end && core_xml[scan + 1] == '/') {
                const cname = parseTagName(core_xml, scan + 2, c_end);
                if (strEql(localName(cname), lname)) {
                    close_start = scan;
                    pos = c_end + 1;
                    found_close = true;
                    break;
                }
            }
            scan = c_end + 1;
        }
        if (!found_close) {
            break;
        }

        const val = core_xml[content_start..close_start];
        if (strEql(lname, "title")) {
            props.title = val;
        } else if (strEql(lname, "subject")) {
            props.subject = val;
        } else if (strEql(lname, "creator")) {
            props.creator = val;
        } else if (strEql(lname, "keywords")) {
            props.keywords = val;
        } else if (strEql(lname, "description")) {
            props.description = val;
        } else if (strEql(lname, "lastModifiedBy")) {
            props.last_modified_by = val;
        } else if (strEql(lname, "revision")) {
            props.revision = val;
        } else if (strEql(lname, "created")) {
            props.created = val;
        } else if (strEql(lname, "modified")) {
            props.modified = val;
        }
    }

    return props;
}

/// 零堆分配 WordprocessingML 段落迭代器（精准区分 `<w:p>` 与 `<w:pPr>` / `<w:pStyle>` 等同前缀标签）
pub struct DocxParagraphIter {
    xml: []const u8,
    pos: usize = 0,

    pub static fn init(doc_xml: []const u8): DocxParagraphIter {
        return DocxParagraphIter {
            xml: doc_xml,
            pos: 0,
        };
    }

    pub fn next(*_): DocxParagraph? {
        while (_.pos < _.xml.len) {
            if (_.xml[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<!--")) {
                _.pos = skipComment(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<![CDATA[")) {
                _.pos = skipCdata(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<?")) {
                _.pos = skipPi(_.xml, _.pos);
                continue;
            }
            if (startsWith(_.xml[_.pos.._.xml.len], "<!")) {
                _.pos = skipDecl(_.xml, _.pos);
                continue;
            }

            const tag_end_opt = findTagEnd(_.xml, _.pos);
            if (tag_end_opt == null) {
                _.pos = _.xml.len;
                return null;
            }
            const tag_end = tag_end_opt ?? return null;

            if (_.pos + 1 < tag_end && _.xml[_.pos + 1] == '/') {
                _.pos = tag_end + 1;
                continue;
            }

            const name = parseTagName(_.xml, _.pos + 1, tag_end);
            if (!isWordTag(name, "p")) {
                _.pos = tag_end + 1;
                continue;
            }

            const para_start = _.pos;
            if (isSelfClosingTag(_.xml, para_start, tag_end)) {
                _.pos = tag_end + 1;
                return DocxParagraph {
                    raw_xml: _.xml[para_start .. tag_end + 1],
                    style: "",
                    is_heading: false,
                };
            }

            // Scan to matching `</w:p>` (tracking nested `<w:p>` depth)
            let depth: usize = 1;
            let scan = tag_end + 1;
            while (scan < _.xml.len && depth > 0) {
                if (_.xml[scan] != '<') {
                    scan += 1;
                    continue;
                }
                if (startsWith(_.xml[scan.._.xml.len], "<!--")) {
                    scan = skipComment(_.xml, scan);
                    continue;
                }
                if (startsWith(_.xml[scan.._.xml.len], "<![CDATA[")) {
                    scan = skipCdata(_.xml, scan);
                    continue;
                }
                if (startsWith(_.xml[scan.._.xml.len], "<?")) {
                    scan = skipPi(_.xml, scan);
                    continue;
                }
                if (startsWith(_.xml[scan.._.xml.len], "<!")) {
                    scan = skipDecl(_.xml, scan);
                    continue;
                }
                const sub_end_opt = findTagEnd(_.xml, scan);
                if (sub_end_opt == null) {
                    _.pos = _.xml.len;
                    return null;
                }
                const sub_end = sub_end_opt ?? return null;
                if (scan + 1 < sub_end && _.xml[scan + 1] == '/') {
                    const cname = parseTagName(_.xml, scan + 2, sub_end);
                    if (isWordTag(cname, "p")) {
                        depth -= 1;
                        if (depth == 0) {
                            const para_end = sub_end + 1;
                            _.pos = para_end;
                            const raw = _.xml[para_start..para_end];
                            const style = extractParagraphStyle(raw);
                            return DocxParagraph {
                                raw_xml: raw,
                                style: style,
                                is_heading: isHeadingStyle(style),
                            };
                        }
                    }
                } else {
                    const oname = parseTagName(_.xml, scan + 1, sub_end);
                    if (isWordTag(oname, "p") && !isSelfClosingTag(_.xml, scan, sub_end)) {
                        depth += 1;
                    }
                }
                scan = sub_end + 1;
            }

            _.pos = _.xml.len;
            return null;
        }
        return null;
    }
}

fn emitByte(out: []u8, out_len: *usize, b: u8): bool {
    const p = out_len.*;
    if (p >= out.len) return false;
    out[p] = b;
    out_len.* = p + 1;
    return true;
}

fn emitUtf8(out: []u8, out_len: *usize, cp: u32): bool {
    if (cp <= 0x7F) {
        return emitByte(out, out_len, cp as u8);
    }
    if (cp <= 0x7FF) {
        if (!emitByte(out, out_len, (0xC0 | ((cp >> 6) & 0x1F)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    if (cp <= 0xFFFF) {
        if (!emitByte(out, out_len, (0xE0 | ((cp >> 12) & 0x0F)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 6) & 0x3F)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    if (cp <= 0x10FFFF) {
        if (!emitByte(out, out_len, (0xF0 | ((cp >> 18) & 0x07)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 12) & 0x3F)) as u8)) return false;
        if (!emitByte(out, out_len, (0x80 | ((cp >> 6) & 0x3F)) as u8)) return false;
        return emitByte(out, out_len, (0x80 | (cp & 0x3F)) as u8);
    }
    return false;
}

fn parseHexDigit(c: u8): u32? {
    if (c >= '0' && c <= '9') return u32(c - '0');
    if (c >= 'a' && c <= 'f') return u32(c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return u32(c - 'A') + 10;
    return null;
}

fn parseNumericEntity(body: []const u8): u32? {
    if (body.len == 0) return null;
    let cp: u32 = 0;
    if (body[0] == 'x' || body[0] == 'X') {
        if (body.len == 1) return null;
        let i: usize = 1;
        while (i < body.len) {
            const d = parseHexDigit(body[i]) ?? return null;
            if (cp > 0x10FFFF) return null;
            cp = cp * 16 + d;
            if (cp > 0x10FFFF) return null;
            i += 1;
        }
    } else {
        let i: usize = 0;
        while (i < body.len) {
            const c = body[i];
            if (c < '0' || c > '9') return null;
            if (cp > 0x10FFFF) return null;
            cp = cp * 10 + u32(c - '0');
            if (cp > 0x10FFFF) return null;
            i += 1;
        }
    }
    if (cp == 0 || (cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF) {
        return null;
    }
    return cp;
}

fn emitUnescapedText(src: []const u8, out: []u8, out_len: *usize): bool {
    let i: usize = 0;
    while (i < src.len) {
        if (src[i] == '&') {
            let semi = i + 1;
            const max_semi = if (i + 16 < src.len) i + 16 else src.len;
            while (semi < max_semi && src[semi] != ';' && src[semi] != '&' && src[semi] != '<') {
                semi += 1;
            }
            if (semi < max_semi && src[semi] == ';') {
                const ent = src[i + 1 .. semi];
                if (strEql(ent, "amp")) {
                    if (!emitByte(out, out_len, '&')) return false;
                    i = semi + 1;
                    continue;
                } else if (strEql(ent, "lt")) {
                    if (!emitByte(out, out_len, '<')) return false;
                    i = semi + 1;
                    continue;
                } else if (strEql(ent, "gt")) {
                    if (!emitByte(out, out_len, '>')) return false;
                    i = semi + 1;
                    continue;
                } else if (strEql(ent, "quot")) {
                    if (!emitByte(out, out_len, '"')) return false;
                    i = semi + 1;
                    continue;
                } else if (strEql(ent, "apos")) {
                    if (!emitByte(out, out_len, '\'')) return false;
                    i = semi + 1;
                    continue;
                } else if (ent.len >= 2 && ent[0] == '#') {
                    if (cp := parseNumericEntity(ent[1..ent.len])) {
                        if (!emitUtf8(out, out_len, cp)) return false;
                        i = semi + 1;
                        continue;
                    }
                }
            }
        }
        if (!emitByte(out, out_len, src[i])) return false;
        i += 1;
    }
    return true;
}

/// 零堆分配提取单个 `<w:p>` 段落内的纯文本（支持 `<w:t>`、XML 实体反转义、`<w:tab/>`、`<w:br/>`、`<w:cr/>`；缓冲区不足返回 `null`）
pub fn extractDocxParagraphText(para_xml: []const u8, out: []u8): usize? {
    let out_len: usize = 0;
    let pos: usize = 0;

    while (pos < para_xml.len) {
        if (para_xml[pos] != '<') {
            pos += 1;
            continue;
        }
        if (startsWith(para_xml[pos..para_xml.len], "<!--")) {
            pos = skipComment(para_xml, pos);
            continue;
        }
        if (startsWith(para_xml[pos..para_xml.len], "<![CDATA[")) {
            pos = skipCdata(para_xml, pos);
            continue;
        }
        if (startsWith(para_xml[pos..para_xml.len], "<?")) {
            pos = skipPi(para_xml, pos);
            continue;
        }
        if (startsWith(para_xml[pos..para_xml.len], "<!")) {
            pos = skipDecl(para_xml, pos);
            continue;
        }

        const tag_end = findTagEnd(para_xml, pos) ?? break;
        if (pos + 1 < tag_end && para_xml[pos + 1] == '/') {
            pos = tag_end + 1;
            continue;
        }

        const name = parseTagName(para_xml, pos + 1, tag_end);
        if (isWordTag(name, "t")) {
            if (isSelfClosingTag(para_xml, pos, tag_end)) {
                pos = tag_end + 1;
                continue;
            }
            pos = tag_end + 1;
            while (pos < para_xml.len) {
                if (para_xml[pos] == '<') {
                    if (startsWith(para_xml[pos..para_xml.len], "<![CDATA[")) {
                        const cdata_start = pos + 9;
                        const cdata_after = skipCdata(para_xml, pos);
                        const cdata_end = if (cdata_after >= cdata_start + 3 && startsWith(para_xml[cdata_after - 3 .. cdata_after], "]]>")) cdata_after - 3 else cdata_after;
                        let k = cdata_start;
                        while (k < cdata_end) {
                            if (!emitByte(out, *out_len, para_xml[k])) return null;
                            k += 1;
                        }
                        pos = cdata_after;
                        continue;
                    }
                    if (startsWith(para_xml[pos..para_xml.len], "<!--")) {
                        pos = skipComment(para_xml, pos);
                        continue;
                    }
                    const c_end_opt = findTagEnd(para_xml, pos);
                    if (c_end_opt == null) {
                        pos = para_xml.len;
                        break;
                    }
                    const c_end = c_end_opt ?? break;
                    if (pos + 1 < c_end && para_xml[pos + 1] == '/') {
                        const cname = parseTagName(para_xml, pos + 2, c_end);
                        if (isWordTag(cname, "t")) {
                            pos = c_end + 1;
                            break;
                        }
                    }
                    pos = c_end + 1;
                    continue;
                }
                const text_start = pos;
                while (pos < para_xml.len && para_xml[pos] != '<') {
                    pos += 1;
                }
                if (!emitUnescapedText(para_xml[text_start..pos], out, *out_len)) {
                    return null;
                }
            }
            continue;
        } else if (isWordTag(name, "tab")) {
            if (!emitByte(out, *out_len, '\t')) return null;
            pos = tag_end + 1;
            continue;
        } else if (isWordTag(name, "br") || isWordTag(name, "cr")) {
            if (!emitByte(out, *out_len, '\n')) return null;
            pos = tag_end + 1;
            continue;
        }

        pos = tag_end + 1;
    }

    return out_len;
}

/// 零堆分配提取整份 `word/document.xml` 纯文本（段落间以 `'\n'` 分隔；非 DOCX XML 或缓冲区不足返回 `null`）
pub fn extractDocxText(doc_xml: []const u8, out: []u8): usize? {
    if (!isWordDocumentXml(doc_xml)) return null;

    let it = DocxParagraphIter.init(doc_xml);
    let out_len: usize = 0;
    let para_idx: usize = 0;

    while (para := it.next()) {
        if (para_idx > 0) {
            if (!emitByte(out, *out_len, '\n')) return null;
        }
        const written = extractDocxParagraphText(para.raw_xml, out[out_len..out.len]) ?? return null;
        out_len += written;
        para_idx += 1;
    }

    return out_len;
}

/// 统计 `word/document.xml` 的段落数、标题数、表格数与文本运行块（`<w:r>`）数量
pub fn summarizeDocx(doc_xml: []const u8): DocxStats {
    let stats = DocxStats {};
    if (!isWordDocumentXml(doc_xml)) return stats;

    // 1. 段落与标题统计
    let it = DocxParagraphIter.init(doc_xml);
    while (para := it.next()) {
        stats.paragraph_count += 1;
        if (para.is_heading) {
            stats.heading_count += 1;
        }
    }

    // 2. 表格 (`<w:tbl>`) 与运行块 (`<w:r>`) 统计
    let pos: usize = 0;
    while (pos < doc_xml.len) {
        if (doc_xml[pos] != '<') {
            pos += 1;
            continue;
        }
        if (startsWith(doc_xml[pos..doc_xml.len], "<!--")) {
            pos = skipComment(doc_xml, pos);
            continue;
        }
        if (startsWith(doc_xml[pos..doc_xml.len], "<![CDATA[")) {
            pos = skipCdata(doc_xml, pos);
            continue;
        }
        if (startsWith(doc_xml[pos..doc_xml.len], "<?")) {
            pos = skipPi(doc_xml, pos);
            continue;
        }
        if (startsWith(doc_xml[pos..doc_xml.len], "<!")) {
            pos = skipDecl(doc_xml, pos);
            continue;
        }
        const tag_end = findTagEnd(doc_xml, pos) ?? break;
        if (pos + 1 < tag_end && doc_xml[pos + 1] != '/') {
            const name = parseTagName(doc_xml, pos + 1, tag_end);
            if (isWordTag(name, "tbl")) {
                stats.table_count += 1;
            } else if (isWordTag(name, "r")) {
                stats.run_count += 1;
            }
        }
        pos = tag_end + 1;
    }

    return stats;
}
