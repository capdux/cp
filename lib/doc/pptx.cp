// Origin: Pure-CP Stdlib (Zero-Alloc OOXML PPTX PresentationML & DrawingML Parser) | Created: 2026-11-22

/// OOXML PresentationML 主演示文稿 Content-Type (`[Content_Types].xml`)
pub const PPTX_MAIN_CONTENT_TYPE: []const u8 = "application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml";

/// OOXML EMU (English Metric Unit) 每英寸换算常量 (1 inch = 914400 EMU)
pub const EMU_PER_INCH: f64 = 914400.0;

/// `ppt/presentation.xml` 幻灯片页面尺寸 (`<p:sldSz cx="..." cy="..." type="..."/>`)
pub struct PptxSlideSize {
    cx: u64,
    cy: u64,
    size_type: []const u8 = "",

    pub fn widthInches(): f64 {
        return (_.cx as f64) / EMU_PER_INCH;
    }

    pub fn heightInches(): f64 {
        return (_.cy as f64) / EMU_PER_INCH;
    }
}

/// `ppt/presentation.xml` 幻灯片引用项 (`<p:sldId id="..." r:id="..."/>`)
pub struct PptxSlideRef {
    id: u32,
    r_id: []const u8,
}

/// 单页幻灯片 / 备注页统计汇总
pub struct PptxSlideStats {
    shape_count: usize = 0,
    paragraph_count: usize = 0,
    text_run_count: usize = 0,
    table_count: usize = 0,
    pic_count: usize = 0,
}

fn isXmlSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isTagDelim(c: u8): bool {
    return isXmlSpace(c) || c == '>' || c == '/';
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

fn isNsTag(qname: []const u8, ns: u8, local: []const u8): bool {
    if (strEql(qname, local)) return true;
    if (qname.len == local.len + 2 && qname[0] == ns && qname[1] == ':') {
        return strEql(qname[2..qname.len], local);
    }
    return false;
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

/// 从单个 XML 开始/自闭合标签切片中按精确属性名提取值（如 `"id"` 与 `"r:id"` 严格区分）
fn extractAttrExact(tag: []const u8, target_name: []const u8): ([]const u8)? {
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
        if (strEql(attr_name, target_name)) {
            return val;
        }
    }
    return null;
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = u64(c - '0');
        if (val > (18446744073709551615 - d) / 10) return null;
        val = val * 10 + d;
        i += 1;
    }
    return val;
}

fn parseU32Dec(s: []const u8): u32? {
    const v64 = parseU64Dec(s) ?? return null;
    if (v64 > 4294967295) return null;
    return v64 as u32;
}

fn hasRootTag(xml: []const u8, ns: u8, local: []const u8): bool {
    const pos = skipProlog(xml);
    if (pos >= xml.len || xml[pos] != '<') return false;
    if (pos + 1 >= xml.len || xml[pos + 1] == '/') return false;
    const tag_end = findTagEnd(xml, pos) ?? return false;
    const name = parseTagName(xml, pos + 1, tag_end);
    return isNsTag(name, ns, local);
}

/// 判定是否为 `ppt/presentation.xml`（检测 `<p:presentation` 或 `<presentation` 根标签）
pub fn isPresentationXml(xml: []const u8): bool {
    return hasRootTag(xml, 'p', "presentation");
}

/// 判定是否为 `ppt/slides/slide*.xml`（检测 `<p:sld` 或 `<sld` 根标签）
pub fn isSlideXml(xml: []const u8): bool {
    return hasRootTag(xml, 'p', "sld");
}

/// 判定是否为 `ppt/notesSlides/notesSlide*.xml`（检测 `<p:notes` 或 `<notes` 根标签）
pub fn isNotesSlideXml(xml: []const u8): bool {
    return hasRootTag(xml, 'p', "notes");
}

/// 从 `ppt/presentation.xml` 解析 `<p:sldSz cx="..." cy="..." type="..."/>` 幻灯片页面尺寸
pub fn parsePptxSlideSize(pres_xml: []const u8): PptxSlideSize? {
    if (!isPresentationXml(pres_xml)) return null;

    let pos: usize = 0;
    while (pos < pres_xml.len) {
        if (pres_xml[pos] != '<') {
            pos += 1;
            continue;
        }
        if (startsWith(pres_xml[pos..pres_xml.len], "<!--")) {
            pos = skipComment(pres_xml, pos);
            continue;
        }
        if (startsWith(pres_xml[pos..pres_xml.len], "<![CDATA[")) {
            pos = skipCdata(pres_xml, pos);
            continue;
        }
        if (startsWith(pres_xml[pos..pres_xml.len], "<?")) {
            pos = skipPi(pres_xml, pos);
            continue;
        }
        if (startsWith(pres_xml[pos..pres_xml.len], "<!")) {
            pos = skipDecl(pres_xml, pos);
            continue;
        }
        const tag_end = findTagEnd(pres_xml, pos) ?? break;
        if (pos + 1 < tag_end && pres_xml[pos + 1] != '/') {
            const name = parseTagName(pres_xml, pos + 1, tag_end);
            if (isNsTag(name, 'p', "sldSz")) {
                const tag = pres_xml[pos .. tag_end + 1];
                const cx_str = extractAttrExact(tag, "cx") ?? return null;
                const cy_str = extractAttrExact(tag, "cy") ?? return null;
                const cx = parseU64Dec(cx_str) ?? return null;
                const cy = parseU64Dec(cy_str) ?? return null;
                const st = extractAttrExact(tag, "type") ?? "";
                return PptxSlideSize {
                    cx: cx,
                    cy: cy,
                    size_type: st,
                };
            }
        }
        pos = tag_end + 1;
    }
    return null;
}

/// 零堆分配 `<p:sldIdLst>` 幻灯片引用迭代器（产出 `<p:sldId id="..." r:id="..."/>`）
pub struct PptxSlideRefIter {
    xml: []const u8,
    pos: usize = 0,
    in_list: bool = false,
    done: bool = false,

    pub static fn init(pres_xml: []const u8): PptxSlideRefIter {
        return PptxSlideRefIter {
            xml: pres_xml,
            pos: 0,
            in_list: false,
            done: false,
        };
    }

    pub fn next(*_): PptxSlideRef? {
        if (_.done) return null;

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
                _.done = true;
                _.pos = _.xml.len;
                return null;
            }
            const tag_end = tag_end_opt ?? return null;

            if (_.pos + 1 < tag_end && _.xml[_.pos + 1] == '/') {
                const cname = parseTagName(_.xml, _.pos + 2, tag_end);
                _.pos = tag_end + 1;
                if (_.in_list && isNsTag(cname, 'p', "sldIdLst")) {
                    _.done = true;
                    return null;
                }
                continue;
            }

            const name = parseTagName(_.xml, _.pos + 1, tag_end);
            if (!_.in_list) {
                if (isNsTag(name, 'p', "sldIdLst")) {
                    if (isSelfClosingTag(_.xml, _.pos, tag_end)) {
                        _.done = true;
                        _.pos = tag_end + 1;
                        return null;
                    }
                    _.in_list = true;
                }
                _.pos = tag_end + 1;
                continue;
            }

            const tag = _.xml[_.pos .. tag_end + 1];
            _.pos = tag_end + 1;
            if (isNsTag(name, 'p', "sldId")) {
                const id_str = extractAttrExact(tag, "id") ?? continue;
                const r_id = extractAttrExact(tag, "r:id") ?? continue;
                const id = parseU32Dec(id_str) ?? continue;
                return PptxSlideRef {
                    id: id,
                    r_id: r_id,
                };
            }
        }

        _.done = true;
        return null;
    }
}

/// 零堆分配 DrawingML `<a:p>...</a:p>` 段落迭代器（精准区分 `<a:p>` 与 `<a:pPr>` / `<a:pBdr>` 等同前缀标签）
pub struct PptxParagraphIter {
    xml: []const u8,
    pos: usize = 0,

    pub static fn init(slide_xml: []const u8): PptxParagraphIter {
        return PptxParagraphIter {
            xml: slide_xml,
            pos: 0,
        };
    }

    pub fn next(*_): ([]const u8)? {
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
            if (!isNsTag(name, 'a', "p")) {
                _.pos = tag_end + 1;
                continue;
            }

            const para_start = _.pos;
            if (isSelfClosingTag(_.xml, para_start, tag_end)) {
                _.pos = tag_end + 1;
                return _.xml[para_start .. tag_end + 1];
            }

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
                    if (isNsTag(cname, 'a', "p")) {
                        depth -= 1;
                        if (depth == 0) {
                            const para_end = sub_end + 1;
                            _.pos = para_end;
                            return _.xml[para_start..para_end];
                        }
                    }
                } else {
                    const oname = parseTagName(_.xml, scan + 1, sub_end);
                    if (isNsTag(oname, 'a', "p") && !isSelfClosingTag(_.xml, scan, sub_end)) {
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

fn extractDrawingParagraphText(para_xml: []const u8, out: []u8): usize? {
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
        if (isNsTag(name, 'a', "t")) {
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
                        if (isNsTag(cname, 'a', "t")) {
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
        } else if (isNsTag(name, 'a', "br")) {
            if (!emitByte(out, *out_len, '\n')) return null;
            pos = tag_end + 1;
            continue;
        } else if (isNsTag(name, 'a', "tab")) {
            if (!emitByte(out, *out_len, '\t')) return null;
            pos = tag_end + 1;
            continue;
        }

        pos = tag_end + 1;
    }

    return out_len;
}

/// 零堆分配提取单页幻灯片（`slide*.xml`）或演讲者备注（`notesSlide*.xml`）全部纯文本
/// 支持 `<a:t>`、`<a:br/>` -> `'\n'`、`<a:tab/>` -> `'\t'`、段落间 `'\n'` 分隔与 XML 实体反转义；非幻灯片/备注 XML 或缓冲区溢出返回 `null`
pub fn extractSlideText(slide_xml: []const u8, out: []u8): usize? {
    if (!isSlideXml(slide_xml) && !isNotesSlideXml(slide_xml)) {
        return null;
    }

    let it = PptxParagraphIter.init(slide_xml);
    let out_len: usize = 0;
    let para_idx: usize = 0;

    while (para := it.next()) {
        if (para_idx > 0) {
            if (!emitByte(out, *out_len, '\n')) return null;
        }
        const written = extractDrawingParagraphText(para, out[out_len..out.len]) ?? return null;
        out_len += written;
        para_idx += 1;
    }

    return out_len;
}

/// 统计单页幻灯片 / 备注页中的形状（`<p:sp>`）、段落（`<a:p>`）、文本运行块（`<a:r>`）、表格（`<a:tbl>`）与图片（`<p:pic>`）数量
pub fn summarizeSlide(slide_xml: []const u8): PptxSlideStats {
    let stats = PptxSlideStats {};
    if (!isSlideXml(slide_xml) && !isNotesSlideXml(slide_xml)) {
        return stats;
    }

    let pos: usize = 0;
    while (pos < slide_xml.len) {
        if (slide_xml[pos] != '<') {
            pos += 1;
            continue;
        }
        if (startsWith(slide_xml[pos..slide_xml.len], "<!--")) {
            pos = skipComment(slide_xml, pos);
            continue;
        }
        if (startsWith(slide_xml[pos..slide_xml.len], "<![CDATA[")) {
            pos = skipCdata(slide_xml, pos);
            continue;
        }
        if (startsWith(slide_xml[pos..slide_xml.len], "<?")) {
            pos = skipPi(slide_xml, pos);
            continue;
        }
        if (startsWith(slide_xml[pos..slide_xml.len], "<!")) {
            pos = skipDecl(slide_xml, pos);
            continue;
        }

        const tag_end = findTagEnd(slide_xml, pos) ?? break;
        if (pos + 1 < tag_end && slide_xml[pos + 1] != '/') {
            const name = parseTagName(slide_xml, pos + 1, tag_end);
            if (isNsTag(name, 'p', "sp")) {
                stats.shape_count += 1;
            } else if (isNsTag(name, 'a', "p")) {
                stats.paragraph_count += 1;
            } else if (isNsTag(name, 'a', "r")) {
                stats.text_run_count += 1;
            } else if (isNsTag(name, 'a', "tbl")) {
                stats.table_count += 1;
            } else if (isNsTag(name, 'p', "pic")) {
                stats.pic_count += 1;
            }
        }
        pos = tag_end + 1;
    }

    return stats;
}
