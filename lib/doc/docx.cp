// Origin: Pure-CP Stdlib (Zero-Alloc OOXML DOCX WordprocessingML & CoreProps Parser) | Created: 2026-11-22

import { Simd } from "../simd/index.cp";

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
        if (in_quote != 0) {
            const rel: usize = Simd.indexOfByte(xml[i..xml.len], in_quote) ?? return null;
            i += rel + 1;
            in_quote = 0;
        } else {
            const rel: usize = Simd.indexOfAny4(xml[i..xml.len], '>', '"', '\'', '\'') ?? return null;
            i += rel;
            const c = xml[i];
            if (c == '>') return i;
            in_quote = c;
            i += 1;
        }
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


// ============================================================================
// 6. Production Extension: Error Set, Rels, Rich Document/Table/Comment Parser,
//    XML Builder, Markdown <-> DOCX Converter & CJK/English Word Stats
// ============================================================================

/// OOXML External Hyperlink relationship type URI.
pub const DOCX_HYPERLINK_REL_TYPE: []const u8 = "http://schemas.openxmlformats.org/officeDocument/2006/relationships/hyperlink";

/// Error set returned by extended DOCX rels, rich document, comments, builder, and Markdown routines.
pub err DocxError {
    BufferTooSmall,
    SlotOverflow,
    InvalidXml,
    InvalidDocument,
    InvalidRels,
    InvalidComments,
    InvalidMarkdown,
    InvalidParameter,
}

fn appendDocxBytes(out: []u8, pos: usize, src: []const u8): usize ! DocxError {
    if (pos + src.len > out.len) return DocxError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos + i] = src[i];
    }
    return pos + src.len;
}

fn appendDocxU32(out: []u8, pos: usize, v: u32): usize ! DocxError {
    let digs: [10]u8 = [0; 10];
    let n: usize = 0;
    let x: u32 = v;
    if (x == 0) {
        digs[0] = '0';
        n = 1;
    } else {
        while (x > 0) {
            digs[n] = ('0' as u8) + ((x % 10) as u8);
            x /= 10;
            n += 1;
        }
    }
    if (pos + n > out.len) return DocxError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos + i] = digs[n - 1 - i];
    }
    return pos + n;
}

fn appendDocxXmlEsc(out: []u8, pos: usize, text: []const u8): usize ! DocxError {
    let cur: usize = pos;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const b = text[i];
        if (b == '&') { cur = appendDocxBytes(out, cur, "&amp;")?; }
        else if (b == '<') { cur = appendDocxBytes(out, cur, "&lt;")?; }
        else if (b == '>') { cur = appendDocxBytes(out, cur, "&gt;")?; }
        else if (b == '"') { cur = appendDocxBytes(out, cur, "&quot;")?; }
        else if (b == '\'') { cur = appendDocxBytes(out, cur, "&apos;")?; }
        else {
            if (cur >= out.len) return DocxError.BufferTooSmall;
            out[cur] = b;
            cur += 1;
        }
    }
    return cur;
}

fn parseDocxU32(s: []const u8): u32? {
    if (s.len == 0 || s.len > 10) return null;
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return null;
        acc = acc * 10 + ((s[i] - '0') as u64);
        if (acc > 4294967295) return null;
    }
    return acc as u32;
}

fn headingLevelFromStyle(style: []const u8): u8 {
    if (!startsWithIgnoreCase(style, "heading")) return 0;
    let i: usize = 7;
    while (i < style.len && (style[i] == ' ' || style[i] == '_' || style[i] == '-')) : (i += 1) {}
    if (i < style.len && style[i] >= '1' && style[i] <= '6') {
        return style[i] - '0';
    }
    return 1;
}

struct DocxCloseSpan {
    inner_end: usize = 0;
    after_close: usize = 0;
}

fn findDocxMatchingClose(xml: []const u8, body_start: usize, target_local: []const u8): DocxCloseSpan? {
    let pos: usize = body_start;
    let depth: usize = 1;
    while (pos < xml.len) {
        if (xml[pos] != '<') {
            const rel: usize = Simd.indexOfByte(xml[pos..xml.len], '<') ?? return null;
            pos += rel;
        }
        if (startsWith(xml[pos..xml.len], "<!--")) { pos = skipComment(xml, pos); continue; }
        if (startsWith(xml[pos..xml.len], "<![CDATA[")) { pos = skipCdata(xml, pos); continue; }
        if (startsWith(xml[pos..xml.len], "<?")) { pos = skipPi(xml, pos); continue; }
        if (startsWith(xml[pos..xml.len], "<!")) { pos = skipDecl(xml, pos); continue; }
        const gt = findTagEnd(xml, pos) ?? return null;
        if (pos + 1 < gt && xml[pos + 1] == '/') {
            const cname = parseTagName(xml, pos + 2, gt);
            if (strEql(localName(cname), target_local)) {
                depth -= 1;
                if (depth == 0) {
                    return DocxCloseSpan{ inner_end: pos, after_close: gt + 1 };
                }
            }
        } else if (pos + 1 < gt) {
            const oname = parseTagName(xml, pos + 1, gt);
            if (strEql(localName(oname), target_local) && !isSelfClosingTag(xml, pos, gt)) {
                depth += 1;
            }
        }
        pos = gt + 1;
    }
    return null;
}

// --- 6.1 Relationships (`word/_rels/document.xml.rels`) ---

/// Parsed `<Relationship>` entry from `word/_rels/document.xml.rels`.
pub struct DocxRel {
    id: []const u8 = "";
    rel_type: []const u8 = "";
    target: []const u8 = "";
    is_external: bool = false;
}

/// Parses `word/_rels/document.xml.rels` into caller-provided `out_rels`.
pub fn parseDocxRelsXml(rels_xml: []const u8, out_rels: []DocxRel): usize ! DocxError {
    let pos: usize = skipProlog(rels_xml);
    let count: usize = 0;
    let saw_rels: bool = false;

    while (pos < rels_xml.len) {
        if (rels_xml[pos] != '<') { pos += 1; continue; }
        if (startsWith(rels_xml[pos..rels_xml.len], "<!--")) { pos = skipComment(rels_xml, pos); continue; }
        if (startsWith(rels_xml[pos..rels_xml.len], "<?")) { pos = skipPi(rels_xml, pos); continue; }
        if (startsWith(rels_xml[pos..rels_xml.len], "<!")) { pos = skipDecl(rels_xml, pos); continue; }
        const gt = findTagEnd(rels_xml, pos) orelse return DocxError.InvalidRels;
        if (pos + 1 < gt && rels_xml[pos + 1] != '/') {
            const name = localName(parseTagName(rels_xml, pos + 1, gt));
            if (strEql(name, "Relationships")) {
                saw_rels = true;
            } else if (strEql(name, "Relationship")) {
                if (count >= out_rels.len) return DocxError.SlotOverflow;
                const tag = rels_xml[pos..(gt + 1)];
                const id_v = extractAttrValue(tag, "Id") ?? "";
                const ty_v = extractAttrValue(tag, "Type") ?? "";
                const tg_v = extractAttrValue(tag, "Target") ?? "";
                const tm_v = extractAttrValue(tag, "TargetMode") ?? "";
                out_rels[count] = DocxRel{
                    id: id_v,
                    rel_type: ty_v,
                    target: tg_v,
                    is_external: startsWithIgnoreCase(tm_v, "external"),
                };
                count += 1;
            }
        }
        pos = gt + 1;
    }
    if (!saw_rels && count == 0) return DocxError.InvalidRels;
    return count;
}

/// Resolves a relationship `rId` against `rels`, returning its `Target` URL/path or `null`.
pub fn lookupDocxRelTarget(rels: []const DocxRel, rel_id: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < rels.len) : (i += 1) {
        if (strEql(rels[i].id, rel_id)) return rels[i].target;
    }
    return null;
}

/// Builds a valid OOXML `word/_rels/document.xml.rels` from `rels`.
pub fn buildDocxRelsXml(rels: []const DocxRel, out_xml: []u8): usize ! DocxError {
    let pos: usize = 0;
    pos = appendDocxBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendDocxBytes(out_xml, pos, "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">")?;
    let i: usize = 0;
    while (i < rels.len) : (i += 1) {
        const r = rels[i];
        pos = appendDocxBytes(out_xml, pos, "<Relationship Id=\"")?;
        pos = appendDocxXmlEsc(out_xml, pos, r.id)?;
        pos = appendDocxBytes(out_xml, pos, "\" Type=\"")?;
        pos = appendDocxXmlEsc(out_xml, pos, if (r.rel_type.len > 0) r.rel_type else DOCX_HYPERLINK_REL_TYPE)?;
        pos = appendDocxBytes(out_xml, pos, "\" Target=\"")?;
        pos = appendDocxXmlEsc(out_xml, pos, r.target)?;
        if (r.is_external) {
            pos = appendDocxBytes(out_xml, pos, "\" TargetMode=\"External\"/>")?;
        } else {
            pos = appendDocxBytes(out_xml, pos, "\"/>")?;
        }
    }
    pos = appendDocxBytes(out_xml, pos, "</Relationships>")?;
    return pos;
}

// --- 6.2 Rich Document, Table & Comment Data Structures ---

/// Paragraph horizontal alignment (`<w:jc w:val="..."/>`).
pub enum DocxAlign {
    left,
    center,
    right,
    both,
    unknown,
}

/// Styled rich-text run (`<w:r>`) optionally wrapped by a `<w:hyperlink>`.
pub struct DocxRun {
    text: []const u8 = "";
    bold: bool = false;
    italic: bool = false;
    underline: bool = false;
    strike: bool = false;
    font_size_half_pt: u16 = 0;
    link_url: []const u8 = "";
}

/// Structured paragraph metadata referencing its slice of runs in `out_runs`.
pub struct DocxRichParagraph {
    style: []const u8 = "";
    heading_level: u8 = 0;
    is_list_item: bool = false;
    list_level: u8 = 0;
    list_num_id: u32 = 0;
    alignment: DocxAlign = DocxAlign.left;
    run_start: usize = 0;
    run_count: usize = 0;
}

/// Structured table cell (`<w:tc>`) with `gridSpan` column-merge metadata.
pub struct DocxTableCell {
    row_index: usize = 0;
    col_index: usize = 0;
    grid_span: u16 = 1;
    text: []const u8 = "";
    is_header_bold: bool = false;
}

/// Structured table (`<w:tbl>`) referencing its slice of cells in `out_cells`.
pub struct DocxTable {
    row_count: usize = 0;
    col_count: usize = 0;
    cell_start: usize = 0;
    cell_count: usize = 0;
}

/// Top-level document block discriminator preserving interleaved paragraph/table order.
pub enum DocxBlockKind {
    paragraph,
    table,
}

/// Ordered top-level block entry pointing into `out_paras` or `out_tables`.
pub struct DocxBlockRef {
    kind: DocxBlockKind = DocxBlockKind.paragraph;
    index: usize = 0;
}

/// Summary counts returned by `parseDocxDocumentXml`.
pub struct DocxParsedDocument {
    block_count: usize = 0;
    paragraph_count: usize = 0;
    run_count: usize = 0;
    table_count: usize = 0;
    cell_count: usize = 0;
}

/// Review comment entry parsed from `word/comments.xml`.
pub struct DocxComment {
    id: []const u8 = "";
    author: []const u8 = "";
    date: []const u8 = "";
    text: []const u8 = "";
}

fn parseRunElement(
    run_inner: []const u8,
    link_url: []const u8,
    text_slab: []u8,
    slab_pos: *usize,
): DocxRun ! DocxError {
    let run = DocxRun{ link_url: link_url };
    const text_start: usize = slab_pos.*;
    let pos: usize = 0;
    let in_rpr: bool = false;

    while (pos < run_inner.len) {
        if (run_inner[pos] != '<') { pos += 1; continue; }
        if (startsWith(run_inner[pos..run_inner.len], "<!--")) { pos = skipComment(run_inner, pos); continue; }
        const gt = findTagEnd(run_inner, pos) orelse break;
        if (pos + 1 < gt && run_inner[pos + 1] == '/') {
            const cname = localName(parseTagName(run_inner, pos + 2, gt));
            if (strEql(cname, "rPr")) in_rpr = false;
            pos = gt + 1;
            continue;
        }
        const tag = run_inner[pos..(gt + 1)];
        const lname = localName(parseTagName(run_inner, pos + 1, gt));
        const sc = isSelfClosingTag(run_inner, pos, gt);

        if (strEql(lname, "rPr")) {
            in_rpr = !sc;
        } else if (in_rpr && strEql(lname, "b")) {
            const v = extractAttrValue(tag, "val") ?? "true";
            run.bold = !(strEql(v, "0") || startsWithIgnoreCase(v, "false"));
        } else if (in_rpr && strEql(lname, "i")) {
            const v = extractAttrValue(tag, "val") ?? "true";
            run.italic = !(strEql(v, "0") || startsWithIgnoreCase(v, "false"));
        } else if (in_rpr && strEql(lname, "u")) {
            const v = extractAttrValue(tag, "val") ?? "single";
            run.underline = !startsWithIgnoreCase(v, "none");
        } else if (in_rpr && strEql(lname, "strike")) {
            const v = extractAttrValue(tag, "val") ?? "true";
            run.strike = !(strEql(v, "0") || startsWithIgnoreCase(v, "false"));
        } else if (in_rpr && strEql(lname, "sz")) {
            if (sv := extractAttrValue(tag, "val")) {
                if (sz := parseDocxU32(sv)) run.font_size_half_pt = sz as u16;
            }
        } else if (strEql(lname, "tab")) {
            if (!emitByte(text_slab, slab_pos, '\t')) return DocxError.BufferTooSmall;
        } else if (strEql(lname, "br") || strEql(lname, "cr")) {
            if (!emitByte(text_slab, slab_pos, '\n')) return DocxError.BufferTooSmall;
        } else if (strEql(lname, "t") && !sc) {
            if (cl := findDocxMatchingClose(run_inner, gt + 1, "t")) {
                if (!emitUnescapedText(run_inner[(gt + 1)..cl.inner_end], text_slab, slab_pos)) {
                    return DocxError.BufferTooSmall;
                }
                pos = cl.after_close;
                continue;
            }
        }
        pos = gt + 1;
    }

    run.text = text_slab[text_start..slab_pos.*];
    return run;
}

fn parseRichParagraphElement(
    para_inner: []const u8,
    rels: []const DocxRel,
    out_runs: []DocxRun,
    run_cnt: *usize,
    text_slab: []u8,
    slab_pos: *usize,
): DocxRichParagraph ! DocxError {
    let para = DocxRichParagraph{ run_start: run_cnt.* };
    let pos: usize = 0;
    let in_ppr: bool = false;
    let in_numpr: bool = false;
    let cur_link: []const u8 = "";

    while (pos < para_inner.len) {
        if (para_inner[pos] != '<') { pos += 1; continue; }
        if (startsWith(para_inner[pos..para_inner.len], "<!--")) { pos = skipComment(para_inner, pos); continue; }
        const gt = findTagEnd(para_inner, pos) orelse break;
        if (pos + 1 < gt && para_inner[pos + 1] == '/') {
            const cname = localName(parseTagName(para_inner, pos + 2, gt));
            if (strEql(cname, "pPr")) in_ppr = false;
            if (strEql(cname, "numPr")) in_numpr = false;
            if (strEql(cname, "hyperlink")) cur_link = "";
            pos = gt + 1;
            continue;
        }

        const tag = para_inner[pos..(gt + 1)];
        const lname = localName(parseTagName(para_inner, pos + 1, gt));
        const sc = isSelfClosingTag(para_inner, pos, gt);

        if (strEql(lname, "pPr")) {
            in_ppr = !sc;
        } else if (in_ppr && strEql(lname, "pStyle")) {
            if (sv := extractAttrValue(tag, "val")) {
                para.style = sv;
                para.heading_level = headingLevelFromStyle(sv);
            }
        } else if (in_ppr && strEql(lname, "numPr")) {
            in_numpr = !sc;
            para.is_list_item = true;
        } else if (in_numpr && strEql(lname, "ilvl")) {
            if (lv := extractAttrValue(tag, "val")) {
                if (n := parseDocxU32(lv)) para.list_level = n as u8;
            }
        } else if (in_numpr && strEql(lname, "numId")) {
            if (nv := extractAttrValue(tag, "val")) {
                if (n := parseDocxU32(nv)) para.list_num_id = n;
            }
        } else if (in_ppr && strEql(lname, "jc")) {
            if (jv := extractAttrValue(tag, "val")) {
                if (strEql(jv, "center")) { para.alignment = DocxAlign.center; }
                else if (strEql(jv, "right")) { para.alignment = DocxAlign.right; }
                else if (strEql(jv, "both")) { para.alignment = DocxAlign.both; }
                else if (strEql(jv, "left")) { para.alignment = DocxAlign.left; }
                else { para.alignment = DocxAlign.unknown; }
            }
        } else if (strEql(lname, "hyperlink") && !sc) {
            if (rid := extractAttrValue(tag, "id")) {
                cur_link = lookupDocxRelTarget(rels, rid) ?? "";
            }
        } else if (strEql(lname, "r") && !sc) {
            if (cl := findDocxMatchingClose(para_inner, gt + 1, "r")) {
                if (run_cnt.* >= out_runs.len) return DocxError.SlotOverflow;
                const r = parseRunElement(para_inner[(gt + 1)..cl.inner_end], cur_link, text_slab, slab_pos)?;
                if (r.text.len > 0 || r.link_url.len > 0) {
                    out_runs[run_cnt.*] = r;
                    run_cnt.* += 1;
                    para.run_count += 1;
                }
                pos = cl.after_close;
                continue;
            }
        }
        pos = gt + 1;
    }
    return para;
}

fn parseTableElement(
    tbl_inner: []const u8,
    out_cells: []DocxTableCell,
    cell_cnt: *usize,
    text_slab: []u8,
    slab_pos: *usize,
): DocxTable ! DocxError {
    let tbl = DocxTable{ cell_start: cell_cnt.* };
    let pos: usize = 0;

    while (pos < tbl_inner.len) {
        if (tbl_inner[pos] != '<') { pos += 1; continue; }
        const gt = findTagEnd(tbl_inner, pos) orelse break;
        if (pos + 1 < gt && tbl_inner[pos + 1] != '/') {
            const lname = localName(parseTagName(tbl_inner, pos + 1, gt));
            if (strEql(lname, "tr") && !isSelfClosingTag(tbl_inner, pos, gt)) {
                if (tr_cl := findDocxMatchingClose(tbl_inner, gt + 1, "tr")) {
                    const tr_body = tbl_inner[(gt + 1)..tr_cl.inner_end];
                    let rpos: usize = 0;
                    let col_idx: usize = 0;
                    while (rpos < tr_body.len) {
                        if (tr_body[rpos] != '<') { rpos += 1; continue; }
                        const cgt = findTagEnd(tr_body, rpos) orelse break;
                        if (rpos + 1 < cgt && tr_body[rpos + 1] != '/') {
                            const clname = localName(parseTagName(tr_body, rpos + 1, cgt));
                            if (strEql(clname, "tc") && !isSelfClosingTag(tr_body, rpos, cgt)) {
                                if (tc_cl := findDocxMatchingClose(tr_body, cgt + 1, "tc")) {
                                    if (cell_cnt.* >= out_cells.len) return DocxError.SlotOverflow;
                                    const tc_body = tr_body[(cgt + 1)..tc_cl.inner_end];

                                    // Detect gridSpan and bold inside cell
                                    let gspan: u16 = 1;
                                    let has_bold: bool = false;
                                    let sp: usize = 0;
                                    while (sp < tc_body.len) {
                                        if (tc_body[sp] != '<') { sp += 1; continue; }
                                        const sgt = findTagEnd(tc_body, sp) orelse break;
                                        if (sp + 1 < sgt && tc_body[sp + 1] != '/') {
                                            const sln = localName(parseTagName(tc_body, sp + 1, sgt));
                                            if (strEql(sln, "gridSpan")) {
                                                if (gv := extractAttrValue(tc_body[sp..(sgt + 1)], "val")) {
                                                    if (gn := parseDocxU32(gv)) gspan = gn as u16;
                                                }
                                            } else if (strEql(sln, "b")) {
                                                has_bold = true;
                                            }
                                        }
                                        sp = sgt + 1;
                                    }

                                    const tstart: usize = slab_pos.*;
                                    const written = extractDocxParagraphText(tc_body, text_slab[tstart..text_slab.len]) orelse return DocxError.BufferTooSmall;
                                    slab_pos.* = tstart + written;

                                    out_cells[cell_cnt.*] = DocxTableCell{
                                        row_index: tbl.row_count,
                                        col_index: col_idx,
                                        grid_span: gspan,
                                        text: text_slab[tstart..slab_pos.*],
                                        is_header_bold: has_bold,
                                    };
                                    cell_cnt.* += 1;
                                    tbl.cell_count += 1;
                                    col_idx += gspan as usize;
                                    rpos = tc_cl.after_close;
                                    continue;
                                }
                            }
                        }
                        rpos = cgt + 1;
                    }
                    if (col_idx > tbl.col_count) tbl.col_count = col_idx;
                    tbl.row_count += 1;
                    pos = tr_cl.after_close;
                    continue;
                }
            }
        }
        pos = gt + 1;
    }
    return tbl;
}

/// Zero-heap structured parser for `word/document.xml`, resolving hyperlinks via `rels` and
/// preserving exact interleaved order of paragraphs (`DocxRichParagraph` + `DocxRun`) and tables (`DocxTable` + `DocxTableCell`).
pub fn parseDocxDocumentXml(
    doc_xml: []const u8,
    rels: []const DocxRel,
    out_blocks: []DocxBlockRef,
    out_paras: []DocxRichParagraph,
    out_runs: []DocxRun,
    out_tables: []DocxTable,
    out_cells: []DocxTableCell,
    text_slab: []u8,
): DocxParsedDocument ! DocxError {
    if (!isWordDocumentXml(doc_xml)) return DocxError.InvalidDocument;

    let res = DocxParsedDocument{};
    let slab_pos: usize = 0;

    // Locate <w:body> (or scan from root if absent)
    let pos: usize = skipProlog(doc_xml);
    let body_end: usize = doc_xml.len;
    while (pos < doc_xml.len) {
        if (doc_xml[pos] != '<') { pos += 1; continue; }
        if (startsWith(doc_xml[pos..doc_xml.len], "<!--")) { pos = skipComment(doc_xml, pos); continue; }
        const gt = findTagEnd(doc_xml, pos) orelse return DocxError.InvalidXml;
        if (pos + 1 < gt && doc_xml[pos + 1] != '/') {
            const lname = localName(parseTagName(doc_xml, pos + 1, gt));
            if (strEql(lname, "body") && !isSelfClosingTag(doc_xml, pos, gt)) {
                if (bcl := findDocxMatchingClose(doc_xml, gt + 1, "body")) {
                    pos = gt + 1;
                    body_end = bcl.inner_end;
                    break;
                }
            }
        }
        pos = gt + 1;
    }

    while (pos < body_end) {
        if (doc_xml[pos] != '<') { pos += 1; continue; }
        if (startsWith(doc_xml[pos..body_end], "<!--")) { pos = skipComment(doc_xml, pos); continue; }
        const gt = findTagEnd(doc_xml, pos) orelse return DocxError.InvalidXml;
        if (pos + 1 < gt && doc_xml[pos + 1] != '/') {
            const lname = localName(parseTagName(doc_xml, pos + 1, gt));
            const sc = isSelfClosingTag(doc_xml, pos, gt);
            if (strEql(lname, "p") && !sc) {
                const pcl = findDocxMatchingClose(doc_xml, gt + 1, "p") orelse return DocxError.InvalidXml;
                if (res.paragraph_count >= out_paras.len || res.block_count >= out_blocks.len) return DocxError.SlotOverflow;
                out_paras[res.paragraph_count] = parseRichParagraphElement(
                    doc_xml[(gt + 1)..pcl.inner_end],
                    rels,
                    out_runs,
                    *res.run_count,
                    text_slab,
                    *slab_pos,
                )?;
                out_blocks[res.block_count] = DocxBlockRef{ kind: DocxBlockKind.paragraph, index: res.paragraph_count };
                res.paragraph_count += 1;
                res.block_count += 1;
                pos = pcl.after_close;
                continue;
            } else if (strEql(lname, "tbl") && !sc) {
                const tcl = findDocxMatchingClose(doc_xml, gt + 1, "tbl") orelse return DocxError.InvalidXml;
                if (res.table_count >= out_tables.len || res.block_count >= out_blocks.len) return DocxError.SlotOverflow;
                out_tables[res.table_count] = parseTableElement(
                    doc_xml[(gt + 1)..tcl.inner_end],
                    out_cells,
                    *res.cell_count,
                    text_slab,
                    *slab_pos,
                )?;
                out_blocks[res.block_count] = DocxBlockRef{ kind: DocxBlockKind.table, index: res.table_count };
                res.table_count += 1;
                res.block_count += 1;
                pos = tcl.after_close;
                continue;
            }
        }
        pos = gt + 1;
    }
    return res;
}

/// Parses `word/comments.xml` extracting review comments (`id`/`author`/`date`/unescaped text).
pub fn parseDocxCommentsXml(
    comments_xml: []const u8,
    out_comments: []DocxComment,
    text_slab: []u8,
): usize ! DocxError {
    let pos: usize = skipProlog(comments_xml);
    let count: usize = 0;
    let slab_pos: usize = 0;
    let saw_comments: bool = false;

    while (pos < comments_xml.len) {
        if (comments_xml[pos] != '<') { pos += 1; continue; }
        if (startsWith(comments_xml[pos..comments_xml.len], "<!--")) { pos = skipComment(comments_xml, pos); continue; }
        const gt = findTagEnd(comments_xml, pos) orelse return DocxError.InvalidComments;
        if (pos + 1 < gt && comments_xml[pos + 1] != '/') {
            const lname = localName(parseTagName(comments_xml, pos + 1, gt));
            if (strEql(lname, "comments")) {
                saw_comments = true;
            } else if (strEql(lname, "comment") && !isSelfClosingTag(comments_xml, pos, gt)) {
                const ccl = findDocxMatchingClose(comments_xml, gt + 1, "comment") orelse return DocxError.InvalidComments;
                if (count >= out_comments.len) return DocxError.SlotOverflow;
                const tag = comments_xml[pos..(gt + 1)];
                const tstart: usize = slab_pos;
                const wn = extractDocxParagraphText(comments_xml[(gt + 1)..ccl.inner_end], text_slab[tstart..text_slab.len]) orelse return DocxError.BufferTooSmall;
                slab_pos += wn;
                out_comments[count] = DocxComment{
                    id: extractAttrValue(tag, "id") ?? "",
                    author: extractAttrValue(tag, "author") ?? "",
                    date: extractAttrValue(tag, "date") ?? "",
                    text: text_slab[tstart..slab_pos],
                };
                count += 1;
                pos = ccl.after_close;
                continue;
            }
        }
        pos = gt + 1;
    }
    if (!saw_comments && count == 0) return DocxError.InvalidComments;
    return count;
}

// --- 6.3 Zero-Heap `word/document.xml` Builder ---

/// Builds a valid OOXML `word/document.xml` from structured blocks/paragraphs/runs/tables/cells,
/// auto-registering external hyperlinks into `out_rels` (`rId1`, `rId2`, ...) with `rid_slab` backing IDs.
pub fn buildDocxDocumentXml(
    blocks: []const DocxBlockRef,
    paras: []const DocxRichParagraph,
    runs: []const DocxRun,
    tables: []const DocxTable,
    cells: []const DocxTableCell,
    out_rels: []DocxRel,
    out_rel_count: *usize,
    rid_slab: []u8,
    out_xml: []u8,
): usize ! DocxError {
    let pos: usize = 0;
    let rid_pos: usize = 0;
    out_rel_count.* = 0;

    pos = appendDocxBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendDocxBytes(out_xml, pos, "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><w:body>")?;

    let bi: usize = 0;
    while (bi < blocks.len) : (bi += 1) {
        const blk = blocks[bi];
        if (blk.kind == DocxBlockKind.paragraph) {
            const p = paras[blk.index];
            pos = appendDocxBytes(out_xml, pos, "<w:p>")?;
            const has_ppr: bool = p.heading_level > 0 || p.style.len > 0 || p.is_list_item || p.alignment != DocxAlign.left;
            if (has_ppr) {
                pos = appendDocxBytes(out_xml, pos, "<w:pPr>")?;
                if (p.heading_level >= 1 && p.heading_level <= 6) {
                    pos = appendDocxBytes(out_xml, pos, "<w:pStyle w:val=\"Heading")?;
                    pos = appendDocxU32(out_xml, pos, p.heading_level as u32)?;
                    pos = appendDocxBytes(out_xml, pos, "\"/>")?;
                } else if (p.style.len > 0) {
                    pos = appendDocxBytes(out_xml, pos, "<w:pStyle w:val=\"")?;
                    pos = appendDocxXmlEsc(out_xml, pos, p.style)?;
                    pos = appendDocxBytes(out_xml, pos, "\"/>")?;
                }
                if (p.is_list_item) {
                    pos = appendDocxBytes(out_xml, pos, "<w:numPr><w:ilvl w:val=\"")?;
                    pos = appendDocxU32(out_xml, pos, p.list_level as u32)?;
                    pos = appendDocxBytes(out_xml, pos, "\"/><w:numId w:val=\"")?;
                    pos = appendDocxU32(out_xml, pos, if (p.list_num_id == 0) 1 else p.list_num_id)?;
                    pos = appendDocxBytes(out_xml, pos, "\"/></w:numPr>")?;
                }
                if (p.alignment == DocxAlign.center) {
                    pos = appendDocxBytes(out_xml, pos, "<w:jc w:val=\"center\"/>")?;
                } else if (p.alignment == DocxAlign.right) {
                    pos = appendDocxBytes(out_xml, pos, "<w:jc w:val=\"right\"/>")?;
                } else if (p.alignment == DocxAlign.both) {
                    pos = appendDocxBytes(out_xml, pos, "<w:jc w:val=\"both\"/>")?;
                }
                pos = appendDocxBytes(out_xml, pos, "</w:pPr>")?;
            }

            let ri: usize = 0;
            while (ri < p.run_count) : (ri += 1) {
                const r = runs[p.run_start + ri];
                if (r.link_url.len > 0) {
                    if (out_rel_count.* >= out_rels.len) return DocxError.SlotOverflow;
                    const rid_start: usize = rid_pos;
                    rid_pos = appendDocxBytes(rid_slab, rid_pos, "rId")?;
                    rid_pos = appendDocxU32(rid_slab, rid_pos, (out_rel_count.* + 1) as u32)?;
                    const rid_slice = rid_slab[rid_start..rid_pos];
                    out_rels[out_rel_count.*] = DocxRel{
                        id: rid_slice,
                        rel_type: DOCX_HYPERLINK_REL_TYPE,
                        target: r.link_url,
                        is_external: true,
                    };
                    out_rel_count.* += 1;
                    pos = appendDocxBytes(out_xml, pos, "<w:hyperlink r:id=\"")?;
                    pos = appendDocxBytes(out_xml, pos, rid_slice)?;
                    pos = appendDocxBytes(out_xml, pos, "\">")?;
                }
                pos = appendDocxBytes(out_xml, pos, "<w:r>")?;
                if (r.bold || r.italic || r.underline || r.strike || r.font_size_half_pt > 0) {
                    pos = appendDocxBytes(out_xml, pos, "<w:rPr>")?;
                    if (r.bold) pos = appendDocxBytes(out_xml, pos, "<w:b/>")?;
                    if (r.italic) pos = appendDocxBytes(out_xml, pos, "<w:i/>")?;
                    if (r.underline) pos = appendDocxBytes(out_xml, pos, "<w:u w:val=\"single\"/>")?;
                    if (r.strike) pos = appendDocxBytes(out_xml, pos, "<w:strike/>")?;
                    if (r.font_size_half_pt > 0) {
                        pos = appendDocxBytes(out_xml, pos, "<w:sz w:val=\"")?;
                        pos = appendDocxU32(out_xml, pos, r.font_size_half_pt as u32)?;
                        pos = appendDocxBytes(out_xml, pos, "\"/>")?;
                    }
                    pos = appendDocxBytes(out_xml, pos, "</w:rPr>")?;
                }
                pos = appendDocxBytes(out_xml, pos, "<w:t xml:space=\"preserve\">")?;
                pos = appendDocxXmlEsc(out_xml, pos, r.text)?;
                pos = appendDocxBytes(out_xml, pos, "</w:t></w:r>")?;
                if (r.link_url.len > 0) {
                    pos = appendDocxBytes(out_xml, pos, "</w:hyperlink>")?;
                }
            }
            pos = appendDocxBytes(out_xml, pos, "</w:p>")?;
        } else {
            const t = tables[blk.index];
            pos = appendDocxBytes(out_xml, pos, "<w:tbl>")?;
            let row_i: usize = 0;
            while (row_i < t.row_count) : (row_i += 1) {
                pos = appendDocxBytes(out_xml, pos, "<w:tr>")?;
                let ci: usize = 0;
                while (ci < t.cell_count) : (ci += 1) {
                    const c = cells[t.cell_start + ci];
                    if (c.row_index != row_i) continue;
                    pos = appendDocxBytes(out_xml, pos, "<w:tc>")?;
                    if (c.grid_span > 1) {
                        pos = appendDocxBytes(out_xml, pos, "<w:tcPr><w:gridSpan w:val=\"")?;
                        pos = appendDocxU32(out_xml, pos, c.grid_span as u32)?;
                        pos = appendDocxBytes(out_xml, pos, "\"/></w:tcPr>")?;
                    }
                    pos = appendDocxBytes(out_xml, pos, "<w:p><w:r>")?;
                    if (c.is_header_bold) pos = appendDocxBytes(out_xml, pos, "<w:rPr><w:b/></w:rPr>")?;
                    pos = appendDocxBytes(out_xml, pos, "<w:t xml:space=\"preserve\">")?;
                    pos = appendDocxXmlEsc(out_xml, pos, c.text)?;
                    pos = appendDocxBytes(out_xml, pos, "</w:t></w:r></w:p></w:tc>")?;
                }
                pos = appendDocxBytes(out_xml, pos, "</w:tr>")?;
            }
            pos = appendDocxBytes(out_xml, pos, "</w:tbl>")?;
        }
    }

    pos = appendDocxBytes(out_xml, pos, "</w:body></w:document>")?;
    return pos;
}

// --- 6.4 High-Fidelity DOCX <-> Markdown Converter & Word/CJK Statistics ---

/// Converts `word/document.xml` (+ `rels`) into clean GFM Markdown (headings, bold/italic/strike,
/// lists, `[text](url)` hyperlinks, and pipe tables) into caller-provided `out_md`.
pub fn docxXmlToMarkdown(
    doc_xml: []const u8,
    rels: []const DocxRel,
    scratch_blocks: []DocxBlockRef,
    scratch_paras: []DocxRichParagraph,
    scratch_runs: []DocxRun,
    scratch_tables: []DocxTable,
    scratch_cells: []DocxTableCell,
    text_slab: []u8,
    out_md: []u8,
): usize ! DocxError {
    const doc = parseDocxDocumentXml(
        doc_xml,
        rels,
        scratch_blocks,
        scratch_paras,
        scratch_runs,
        scratch_tables,
        scratch_cells,
        text_slab,
    )?;

    let pos: usize = 0;
    let bi: usize = 0;
    while (bi < doc.block_count) : (bi += 1) {
        if (bi > 0) pos = appendDocxBytes(out_md, pos, "\n")?;
        const blk = scratch_blocks[bi];
        if (blk.kind == DocxBlockKind.paragraph) {
            const p = scratch_paras[blk.index];
            if (p.heading_level >= 1 && p.heading_level <= 6) {
                let h: u8 = 0;
                while (h < p.heading_level) : (h += 1) {
                    pos = appendDocxBytes(out_md, pos, "#")?;
                }
                pos = appendDocxBytes(out_md, pos, " ")?;
            } else if (p.is_list_item) {
                let ind: u8 = 0;
                while (ind < p.list_level) : (ind += 1) {
                    pos = appendDocxBytes(out_md, pos, "  ")?;
                }
                pos = appendDocxBytes(out_md, pos, "- ")?;
            }

            let ri: usize = 0;
            while (ri < p.run_count) : (ri += 1) {
                const r = scratch_runs[p.run_start + ri];
                if (r.link_url.len > 0) {
                    pos = appendDocxBytes(out_md, pos, "[")?;
                    pos = appendDocxBytes(out_md, pos, r.text)?;
                    pos = appendDocxBytes(out_md, pos, "](")?;
                    pos = appendDocxBytes(out_md, pos, r.link_url)?;
                    pos = appendDocxBytes(out_md, pos, ")")?;
                } else {
                    if (r.strike) pos = appendDocxBytes(out_md, pos, "~~")?;
                    if (r.bold) pos = appendDocxBytes(out_md, pos, "**")?;
                    if (r.italic) pos = appendDocxBytes(out_md, pos, "*")?;
                    pos = appendDocxBytes(out_md, pos, r.text)?;
                    if (r.italic) pos = appendDocxBytes(out_md, pos, "*")?;
                    if (r.bold) pos = appendDocxBytes(out_md, pos, "**")?;
                    if (r.strike) pos = appendDocxBytes(out_md, pos, "~~")?;
                }
            }
            pos = appendDocxBytes(out_md, pos, "\n")?;
        } else {
            const t = scratch_tables[blk.index];
            let r_i: usize = 0;
            while (r_i < t.row_count) : (r_i += 1) {
                pos = appendDocxBytes(out_md, pos, "|")?;
                let ci: usize = 0;
                while (ci < t.cell_count) : (ci += 1) {
                    const c = scratch_cells[t.cell_start + ci];
                    if (c.row_index != r_i) continue;
                    pos = appendDocxBytes(out_md, pos, " ")?;
                    pos = appendDocxBytes(out_md, pos, c.text)?;
                    pos = appendDocxBytes(out_md, pos, " |")?;
                }
                pos = appendDocxBytes(out_md, pos, "\n")?;
                if (r_i == 0 && t.col_count > 0) {
                    pos = appendDocxBytes(out_md, pos, "|")?;
                    let cc: usize = 0;
                    while (cc < t.col_count) : (cc += 1) {
                        pos = appendDocxBytes(out_md, pos, " --- |")?;
                    }
                    pos = appendDocxBytes(out_md, pos, "\n")?;
                }
            }
        }
    }
    return pos;
}

/// Converts Markdown (headings `#..######`, `- ` lists, `**bold**`, `[text](url)` links)
/// directly into valid OOXML `word/document.xml`, auto-collecting external hyperlink rels.
pub fn markdownToDocxXml(
    md: []const u8,
    out_rels: []DocxRel,
    out_rel_count: *usize,
    rid_slab: []u8,
    out_xml: []u8,
): usize ! DocxError {
    let pos: usize = 0;
    let rid_pos: usize = 0;
    out_rel_count.* = 0;

    pos = appendDocxBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendDocxBytes(out_xml, pos, "<w:document xmlns:w=\"http://schemas.openxmlformats.org/wordprocessingml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><w:body>")?;

    let cur: usize = 0;
    while (cur < md.len) {
        let lend: usize = cur;
        while (lend < md.len && md[lend] != '\n') : (lend += 1) {}
        let trim_end: usize = lend;
        if (trim_end > cur && md[trim_end - 1] == '\r') trim_end -= 1;
        const line = md[cur..trim_end];
        cur = if (lend < md.len) (lend + 1) else md.len;
        if (line.len == 0) continue;

        let h_level: u8 = 0;
        let is_list: bool = false;
        let content_start: usize = 0;
        while (content_start < line.len && line[content_start] == '#' && h_level < 6) {
            h_level += 1;
            content_start += 1;
        }
        if (h_level > 0 && content_start < line.len && line[content_start] == ' ') {
            content_start += 1;
        } else {
            h_level = 0;
            content_start = 0;
            if (line.len >= 2 && (line[0] == '-' || line[0] == '*') && line[1] == ' ') {
                is_list = true;
                content_start = 2;
            }
        }

        pos = appendDocxBytes(out_xml, pos, "<w:p>")?;
        if (h_level > 0) {
            pos = appendDocxBytes(out_xml, pos, "<w:pPr><w:pStyle w:val=\"Heading")?;
            pos = appendDocxU32(out_xml, pos, h_level as u32)?;
            pos = appendDocxBytes(out_xml, pos, "\"/></w:pPr>")?;
        } else if (is_list) {
            pos = appendDocxBytes(out_xml, pos, "<w:pPr><w:numPr><w:ilvl w:val=\"0\"/><w:numId w:val=\"1\"/></w:numPr></w:pPr>")?;
        }

        // Inline scanner for **bold** and [text](url)
        let i: usize = content_start;
        while (i < line.len) {
            if (i + 3 < line.len && line[i] == '*' && line[i + 1] == '*') {
                let end_b: usize = i + 2;
                let found_b: bool = false;
                while (end_b + 1 < line.len) : (end_b += 1) {
                    if (line[end_b] == '*' && line[end_b + 1] == '*') {
                        found_b = true;
                        break;
                    }
                }
                if (found_b) {
                    pos = appendDocxBytes(out_xml, pos, "<w:r><w:rPr><w:b/></w:rPr><w:t xml:space=\"preserve\">")?;
                    pos = appendDocxXmlEsc(out_xml, pos, line[(i + 2)..end_b])?;
                    pos = appendDocxBytes(out_xml, pos, "</w:t></w:r>")?;
                    i = end_b + 2;
                    continue;
                }
            }
            if (line[i] == '[') {
                let cb: usize = i + 1;
                while (cb < line.len && line[cb] != ']') : (cb += 1) {}
                if (cb + 2 < line.len && line[cb] == ']' && line[cb + 1] == '(') {
                    let cp: usize = cb + 2;
                    while (cp < line.len && line[cp] != ')') : (cp += 1) {}
                    if (cp < line.len && line[cp] == ')') {
                        if (out_rel_count.* >= out_rels.len) return DocxError.SlotOverflow;
                        const rid_start: usize = rid_pos;
                        rid_pos = appendDocxBytes(rid_slab, rid_pos, "rId")?;
                        rid_pos = appendDocxU32(rid_slab, rid_pos, (out_rel_count.* + 1) as u32)?;
                        const rid_slice = rid_slab[rid_start..rid_pos];
                        out_rels[out_rel_count.*] = DocxRel{
                            id: rid_slice,
                            rel_type: DOCX_HYPERLINK_REL_TYPE,
                            target: line[(cb + 2)..cp],
                            is_external: true,
                        };
                        out_rel_count.* += 1;
                        pos = appendDocxBytes(out_xml, pos, "<w:hyperlink r:id=\"")?;
                        pos = appendDocxBytes(out_xml, pos, rid_slice)?;
                        pos = appendDocxBytes(out_xml, pos, "\"><w:r><w:t xml:space=\"preserve\">")?;
                        pos = appendDocxXmlEsc(out_xml, pos, line[(i + 1)..cb])?;
                        pos = appendDocxBytes(out_xml, pos, "</w:t></w:r></w:hyperlink>")?;
                        i = cp + 1;
                        continue;
                    }
                }
            }
            // Plain text run until next '**' or '['
            const tstart: usize = i;
            i += 1;
            while (i < line.len && line[i] != '[' && !(i + 1 < line.len && line[i] == '*' && line[i + 1] == '*')) : (i += 1) {}
            pos = appendDocxBytes(out_xml, pos, "<w:r><w:t xml:space=\"preserve\">")?;
            pos = appendDocxXmlEsc(out_xml, pos, line[tstart..i])?;
            pos = appendDocxBytes(out_xml, pos, "</w:t></w:r>")?;
        }

        pos = appendDocxBytes(out_xml, pos, "</w:p>")?;
    }

    pos = appendDocxBytes(out_xml, pos, "</w:body></w:document>")?;
    return pos;
}

/// Multilingual document word & character count statistics.
pub struct DocxWordStats {
    paragraph_count: usize = 0;
    table_count: usize = 0;
    char_count: usize = 0;
    english_word_count: usize = 0;
    cjk_char_count: usize = 0;
}

fn isAsciiAlnum(b: u8): bool {
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9');
}

/// Computes paragraph/table counts, UTF-8 char count, English word count, and CJK Han/Kana character count.
pub fn computeDocxWordStats(doc_xml: []const u8, text_scratch: []u8): DocxWordStats ! DocxError {
    if (!isWordDocumentXml(doc_xml)) return DocxError.InvalidDocument;
    const base = summarizeDocx(doc_xml);
    const tlen = extractDocxText(doc_xml, text_scratch) orelse return DocxError.BufferTooSmall;
    const text = text_scratch[0..tlen];

    let stats = DocxWordStats{
        paragraph_count: base.paragraph_count,
        table_count: base.table_count,
    };

    let in_word: bool = false;
    let i: usize = 0;
    while (i < text.len) {
        const b0 = text[i];
        if (b0 < 0x80) {
            if (b0 != '\n' && b0 != '\r') stats.char_count += 1;
            if (isAsciiAlnum(b0)) {
                if (!in_word) {
                    stats.english_word_count += 1;
                    in_word = true;
                }
            } else {
                in_word = false;
            }
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0 && i + 1 < text.len) {
            stats.char_count += 1;
            in_word = false;
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0 && i + 2 < text.len) {
            const cp: u32 = (((b0 & 0x0F) as u32) << 12)
                | (((text[i + 1] & 0x3F) as u32) << 6)
                | ((text[i + 2] & 0x3F) as u32);
            stats.char_count += 1;
            if ((cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0x3400 && cp <= 0x4DBF) || (cp >= 0x3040 && cp <= 0x30FF)) {
                stats.cjk_char_count += 1;
            }
            in_word = false;
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0 && i + 3 < text.len) {
            stats.char_count += 1;
            in_word = false;
            i += 4;
        } else {
            in_word = false;
            i += 1;
        }
    }
    return stats;
}
