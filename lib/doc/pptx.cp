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


// ============================================================================
// 5. Production Extension: EMU/Pt Codec, Presentation/Slide/Table/Notes Parser,
//    PresentationML Builders, Markdown Outline Exporter & Detailed Stats
// ============================================================================

/// OOXML EMU per typographic point (`1 pt = 12700 EMU`).
pub const EMU_PER_PT: u64 = 12700;

/// Standard widescreen 16:9 slide width in EMU (`13.333 in = 960 pt = 12192000 EMU`).
pub const PPTX_SLIDE_16_9_CX: u64 = 12192000;

/// Standard widescreen 16:9 slide height in EMU (`7.5 in = 540 pt = 6858000 EMU`).
pub const PPTX_SLIDE_16_9_CY: u64 = 6858000;

/// Error set returned by extended PPTX presentation, slide, notes, builder, and Markdown routines.
pub err PptxError {
    BufferTooSmall,
    SlotOverflow,
    InvalidXml,
    InvalidPresentation,
    InvalidSlide,
    InvalidNotes,
    InvalidParameter,
}

/// Converts OOXML EMU to typographic points (`emu / 12700.0`).
pub fn emuToPt(emu: u64): f64 {
    return (emu as f64) / (EMU_PER_PT as f64);
}

/// Converts typographic points to OOXML EMU (`round(pt * 12700.0)`).
pub fn ptToEmu(pt: f64): u64 {
    if (pt <= 0.0) return 0;
    return (pt * (EMU_PER_PT as f64) + 0.5) as u64;
}

fn pptxLocalName(qname: []const u8): []const u8 {
    let i: usize = 0;
    while (i < qname.len) : (i += 1) {
        if (qname[i] == ':') return qname[(i + 1)..qname.len];
    }
    return qname;
}

fn appendPptxBytes(out: []u8, pos: usize, src: []const u8): usize ! PptxError {
    if (pos + src.len > out.len) return PptxError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos + i] = src[i];
    }
    return pos + src.len;
}

fn appendPptxU64(out: []u8, pos: usize, v: u64): usize ! PptxError {
    let digs: [20]u8 = [0; 20];
    let n: usize = 0;
    let x: u64 = v;
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
    if (pos + n > out.len) return PptxError.BufferTooSmall;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[pos + i] = digs[n - 1 - i];
    }
    return pos + n;
}

fn appendPptxI64(out: []u8, pos: usize, v: i64): usize ! PptxError {
    let cur: usize = pos;
    if (v < 0) {
        cur = appendPptxBytes(out, cur, "-")?;
        return appendPptxU64(out, cur, (-v) as u64);
    }
    return appendPptxU64(out, cur, v as u64);
}

fn appendPptxXmlEsc(out: []u8, pos: usize, text: []const u8): usize ! PptxError {
    let cur: usize = pos;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        const b = text[i];
        if (b == '&') { cur = appendPptxBytes(out, cur, "&amp;")?; }
        else if (b == '<') { cur = appendPptxBytes(out, cur, "&lt;")?; }
        else if (b == '>') { cur = appendPptxBytes(out, cur, "&gt;")?; }
        else if (b == '"') { cur = appendPptxBytes(out, cur, "&quot;")?; }
        else if (b == '\'') { cur = appendPptxBytes(out, cur, "&apos;")?; }
        else {
            if (cur >= out.len) return PptxError.BufferTooSmall;
            out[cur] = b;
            cur += 1;
        }
    }
    return cur;
}

fn parseI64Dec(s: []const u8): i64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg: bool = false;
    if (s[0] == '-') { neg = true; i = 1; }
    if (i >= s.len) return null;
    const u = parseU64Dec(s[i..s.len]) ?? return null;
    return if (neg) -(u as i64) else (u as i64);
}

struct PptxCloseSpan {
    inner_end: usize = 0;
    after_close: usize = 0;
}

fn findPptxMatchingClose(xml: []const u8, body_start: usize, target_local: []const u8): PptxCloseSpan? {
    let pos: usize = body_start;
    let depth: usize = 1;
    while (pos < xml.len) {
        if (xml[pos] != '<') { pos += 1; continue; }
        if (startsWith(xml[pos..xml.len], "<!--")) { pos = skipComment(xml, pos); continue; }
        if (startsWith(xml[pos..xml.len], "<![CDATA[")) { pos = skipCdata(xml, pos); continue; }
        if (startsWith(xml[pos..xml.len], "<?")) { pos = skipPi(xml, pos); continue; }
        if (startsWith(xml[pos..xml.len], "<!")) { pos = skipDecl(xml, pos); continue; }
        const gt = findTagEnd(xml, pos) ?? return null;
        if (pos + 1 < gt && xml[pos + 1] == '/') {
            const cname = pptxLocalName(parseTagName(xml, pos + 2, gt));
            if (strEql(cname, target_local)) {
                depth -= 1;
                if (depth == 0) {
                    return PptxCloseSpan{ inner_end: pos, after_close: gt + 1 };
                }
            }
        } else if (pos + 1 < gt) {
            const oname = pptxLocalName(parseTagName(xml, pos + 1, gt));
            if (strEql(oname, target_local) && !isSelfClosingTag(xml, pos, gt)) {
                depth += 1;
            }
        }
        pos = gt + 1;
    }
    return null;
}

// --- 5.1 Presentation (`ppt/presentation.xml`) Parser & Builder ---

/// Parsed presentation dimensions and slide list count.
pub struct PptxPresentationInfo {
    slide_size: PptxSlideSize = PptxSlideSize{ cx: 0, cy: 0 },
    has_slide_size: bool = false;
    slide_count: usize = 0;
}

/// Parses `ppt/presentation.xml` extracting `<p:sldSz>` and ordered `<p:sldId>` entries into `out_slides`.
pub fn parsePptxPresentationXml(
    pres_xml: []const u8,
    out_slides: []PptxSlideRef,
): PptxPresentationInfo ! PptxError {
    if (!isPresentationXml(pres_xml)) return PptxError.InvalidPresentation;
    let info = PptxPresentationInfo{};
    if (sz := parsePptxSlideSize(pres_xml)) {
        info.slide_size = sz;
        info.has_slide_size = true;
    }
    let it = PptxSlideRefIter.init(pres_xml);
    while (sr := it.next()) {
        if (info.slide_count >= out_slides.len) return PptxError.SlotOverflow;
        out_slides[info.slide_count] = sr;
        info.slide_count += 1;
    }
    return info;
}

/// Builds a valid OOXML `ppt/presentation.xml` with slide dimensions and ordered `<p:sldIdLst>`.
pub fn buildPptxPresentationXml(
    size: PptxSlideSize,
    slides: []const PptxSlideRef,
    out_xml: []u8,
): usize ! PptxError {
    let pos: usize = 0;
    pos = appendPptxBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendPptxBytes(out_xml, pos, "<p:presentation xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" xmlns:r=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships\"><p:sldIdLst>")?;
    let i: usize = 0;
    while (i < slides.len) : (i += 1) {
        pos = appendPptxBytes(out_xml, pos, "<p:sldId id=\"")?;
        pos = appendPptxU64(out_xml, pos, slides[i].id as u64)?;
        pos = appendPptxBytes(out_xml, pos, "\" r:id=\"")?;
        pos = appendPptxXmlEsc(out_xml, pos, slides[i].r_id)?;
        pos = appendPptxBytes(out_xml, pos, "\"/>")?;
    }
    pos = appendPptxBytes(out_xml, pos, "</p:sldIdLst><p:sldSz cx=\"")?;
    pos = appendPptxU64(out_xml, pos, size.cx)?;
    pos = appendPptxBytes(out_xml, pos, "\" cy=\"")?;
    pos = appendPptxU64(out_xml, pos, size.cy)?;
    if (size.size_type.len > 0) {
        pos = appendPptxBytes(out_xml, pos, "\" type=\"")?;
        pos = appendPptxXmlEsc(out_xml, pos, size.size_type)?;
    }
    pos = appendPptxBytes(out_xml, pos, "\"/></p:presentation>")?;
    return pos;
}

// --- 5.2 Rich Slide & Table Structures, Parser & Notes Parser ---

/// OOXML `<p:ph type="..."/>` placeholder role.
pub enum PptxPlaceholderKind {
    none,
    title,
    ctr_title,
    sub_title,
    body,
    footer,
    slide_num,
    other,
}

fn parsePlaceholderKind(t: []const u8): PptxPlaceholderKind {
    if (strEql(t, "title")) return PptxPlaceholderKind.title;
    if (strEql(t, "ctrTitle")) return PptxPlaceholderKind.ctr_title;
    if (strEql(t, "subTitle")) return PptxPlaceholderKind.sub_title;
    if (strEql(t, "body")) return PptxPlaceholderKind.body;
    if (strEql(t, "ftr")) return PptxPlaceholderKind.footer;
    if (strEql(t, "sldNum")) return PptxPlaceholderKind.slide_num;
    if (t.len == 0) return PptxPlaceholderKind.body;
    return PptxPlaceholderKind.other;
}

fn placeholderTypeStr(k: PptxPlaceholderKind): []const u8 {
    return match (k) {
        PptxPlaceholderKind.title => "title",
        PptxPlaceholderKind.ctr_title => "ctrTitle",
        PptxPlaceholderKind.sub_title => "subTitle",
        PptxPlaceholderKind.body => "body",
        PptxPlaceholderKind.footer => "ftr",
        PptxPlaceholderKind.slide_num => "sldNum",
        else => "",
    };
}

/// DrawingML shape bounding box in EMU (`<a:off x y>` & `<a:ext cx cy>`).
pub struct PptxRectEmu {
    x: i64 = 0;
    y: i64 = 0;
    cx: u64 = 0;
    cy: u64 = 0;
}

/// Styled DrawingML text run (`<a:r>`).
pub struct PptxTextRun {
    text: []const u8 = "";
    bold: bool = false;
    italic: bool = false;
    font_size_centipt: u32 = 0;
    color_hex: [6]u8 = ['0'; 6];
    has_color: bool = false;
}

/// DrawingML paragraph (`<a:p>`) with bullet level, alignment, and run slice range.
pub struct PptxTextParagraph {
    level: u8 = 0;
    alignment: []const u8 = "";
    run_start: usize = 0;
    run_count: usize = 0;
}

/// PresentationML shape (`<p:sp>`) with placeholder role, EMU geometry, and paragraph range.
pub struct PptxShape {
    placeholder: PptxPlaceholderKind = PptxPlaceholderKind.none;
    rect: PptxRectEmu = PptxRectEmu{},
    has_rect: bool = false;
    para_start: usize = 0;
    para_count: usize = 0;
}

/// DrawingML table cell (`<a:tc>`).
pub struct PptxTableCell {
    row_index: usize = 0;
    col_index: usize = 0;
    text: []const u8 = "";
}

/// DrawingML native table (`<a:tbl>`) referencing its cells in `out_cells`.
pub struct PptxTable {
    row_count: usize = 0;
    col_count: usize = 0;
    cell_start: usize = 0;
    cell_count: usize = 0;
}

/// Summary counts returned by `parsePptxSlideXml`.
pub struct PptxParsedSlide {
    shape_count: usize = 0;
    para_count: usize = 0;
    run_count: usize = 0;
    table_count: usize = 0;
    cell_count: usize = 0;
}

fn parseDrawingRun(run_inner: []const u8, text_slab: []u8, slab_pos: *usize): PptxTextRun ! PptxError {
    let run = PptxTextRun{};
    const tstart: usize = slab_pos.*;
    let pos: usize = 0;
    let in_rpr: bool = false;

    while (pos < run_inner.len) {
        if (run_inner[pos] != '<') { pos += 1; continue; }
        const gt = findTagEnd(run_inner, pos) orelse break;
        if (pos + 1 < gt && run_inner[pos + 1] == '/') {
            if (strEql(pptxLocalName(parseTagName(run_inner, pos + 2, gt)), "rPr")) in_rpr = false;
            pos = gt + 1;
            continue;
        }
        const tag = run_inner[pos..(gt + 1)];
        const lname = pptxLocalName(parseTagName(run_inner, pos + 1, gt));
        const sc = isSelfClosingTag(run_inner, pos, gt);

        if (strEql(lname, "rPr")) {
            if (bv := extractAttrExact(tag, "b")) {
                run.bold = strEql(bv, "1") || strEql(bv, "true");
            }
            if (iv := extractAttrExact(tag, "i")) {
                run.italic = strEql(iv, "1") || strEql(iv, "true");
            }
            if (sv := extractAttrExact(tag, "sz")) {
                if (sz := parseU32Dec(sv)) run.font_size_centipt = sz;
            }
            in_rpr = !sc;
        } else if (in_rpr && strEql(lname, "srgbClr")) {
            if (cv := extractAttrExact(tag, "val")) {
                if (cv.len == 6) {
                    let ci: usize = 0;
                    while (ci < 6) : (ci += 1) {
                        run.color_hex[ci] = cv[ci];
                    }
                    run.has_color = true;
                }
            }
        } else if (strEql(lname, "t") && !sc) {
            if (cl := findPptxMatchingClose(run_inner, gt + 1, "t")) {
                if (!emitUnescapedText(run_inner[(gt + 1)..cl.inner_end], text_slab, slab_pos)) {
                    return PptxError.BufferTooSmall;
                }
                pos = cl.after_close;
                continue;
            }
        }
        pos = gt + 1;
    }
    run.text = text_slab[tstart..slab_pos.*];
    return run;
}

fn parseDrawingParagraph(
    p_inner: []const u8,
    out_runs: []PptxTextRun,
    run_cnt: *usize,
    text_slab: []u8,
    slab_pos: *usize,
): PptxTextParagraph ! PptxError {
    let para = PptxTextParagraph{ run_start: run_cnt.* };
    let pos: usize = 0;

    while (pos < p_inner.len) {
        if (p_inner[pos] != '<') { pos += 1; continue; }
        const gt = findTagEnd(p_inner, pos) orelse break;
        if (pos + 1 < gt && p_inner[pos + 1] != '/') {
            const tag = p_inner[pos..(gt + 1)];
            const lname = pptxLocalName(parseTagName(p_inner, pos + 1, gt));
            const sc = isSelfClosingTag(p_inner, pos, gt);
            if (strEql(lname, "pPr")) {
                if (lv := extractAttrExact(tag, "lvl")) {
                    if (n := parseU32Dec(lv)) para.level = n as u8;
                }
                if (al := extractAttrExact(tag, "algn")) {
                    para.alignment = al;
                }
            } else if (strEql(lname, "r") && !sc) {
                if (cl := findPptxMatchingClose(p_inner, gt + 1, "r")) {
                    if (run_cnt.* >= out_runs.len) return PptxError.SlotOverflow;
                    const r = parseDrawingRun(p_inner[(gt + 1)..cl.inner_end], text_slab, slab_pos)?;
                    if (r.text.len > 0) {
                        out_runs[run_cnt.*] = r;
                        run_cnt.* += 1;
                        para.run_count += 1;
                    }
                    pos = cl.after_close;
                    continue;
                }
            }
        }
        pos = gt + 1;
    }
    return para;
}

fn parsePptxShapeElement(
    sp_inner: []const u8,
    out_paras: []PptxTextParagraph,
    para_cnt: *usize,
    out_runs: []PptxTextRun,
    run_cnt: *usize,
    text_slab: []u8,
    slab_pos: *usize,
): PptxShape ! PptxError {
    let sh = PptxShape{ para_start: para_cnt.* };
    let pos: usize = 0;

    while (pos < sp_inner.len) {
        if (sp_inner[pos] != '<') { pos += 1; continue; }
        const gt = findTagEnd(sp_inner, pos) orelse break;
        if (pos + 1 < gt && sp_inner[pos + 1] != '/') {
            const tag = sp_inner[pos..(gt + 1)];
            const lname = pptxLocalName(parseTagName(sp_inner, pos + 1, gt));
            const sc = isSelfClosingTag(sp_inner, pos, gt);
            if (strEql(lname, "ph")) {
                const pt = extractAttrExact(tag, "type") ?? "";
                sh.placeholder = parsePlaceholderKind(pt);
            } else if (strEql(lname, "off")) {
                if (xs := extractAttrExact(tag, "x")) {
                    if (xv := parseI64Dec(xs)) sh.rect.x = xv;
                }
                if (ys := extractAttrExact(tag, "y")) {
                    if (yv := parseI64Dec(ys)) sh.rect.y = yv;
                }
                sh.has_rect = true;
            } else if (strEql(lname, "ext")) {
                if (cxs := extractAttrExact(tag, "cx")) {
                    if (cxv := parseU64Dec(cxs)) sh.rect.cx = cxv;
                }
                if (cys := extractAttrExact(tag, "cy")) {
                    if (cyv := parseU64Dec(cys)) sh.rect.cy = cyv;
                }
                sh.has_rect = true;
            } else if (strEql(lname, "p") && !sc) {
                if (cl := findPptxMatchingClose(sp_inner, gt + 1, "p")) {
                    if (para_cnt.* >= out_paras.len) return PptxError.SlotOverflow;
                    const p = parseDrawingParagraph(sp_inner[(gt + 1)..cl.inner_end], out_runs, run_cnt, text_slab, slab_pos)?;
                    if (p.run_count > 0) {
                        out_paras[para_cnt.*] = p;
                        para_cnt.* += 1;
                        sh.para_count += 1;
                    }
                    pos = cl.after_close;
                    continue;
                }
            }
        }
        pos = gt + 1;
    }
    return sh;
}

fn parseDrawingTable(
    tbl_inner: []const u8,
    out_cells: []PptxTableCell,
    cell_cnt: *usize,
    text_slab: []u8,
    slab_pos: *usize,
): PptxTable ! PptxError {
    let tbl = PptxTable{ cell_start: cell_cnt.* };
    let pos: usize = 0;

    while (pos < tbl_inner.len) {
        if (tbl_inner[pos] != '<') { pos += 1; continue; }
        const gt = findTagEnd(tbl_inner, pos) orelse break;
        if (pos + 1 < gt && tbl_inner[pos + 1] != '/') {
            const lname = pptxLocalName(parseTagName(tbl_inner, pos + 1, gt));
            if (strEql(lname, "tr") && !isSelfClosingTag(tbl_inner, pos, gt)) {
                if (tr_cl := findPptxMatchingClose(tbl_inner, gt + 1, "tr")) {
                    const tr_body = tbl_inner[(gt + 1)..tr_cl.inner_end];
                    let rpos: usize = 0;
                    let col_i: usize = 0;
                    while (rpos < tr_body.len) {
                        if (tr_body[rpos] != '<') { rpos += 1; continue; }
                        const cgt = findTagEnd(tr_body, rpos) orelse break;
                        if (rpos + 1 < cgt && tr_body[rpos + 1] != '/') {
                            const cln = pptxLocalName(parseTagName(tr_body, rpos + 1, cgt));
                            if (strEql(cln, "tc") && !isSelfClosingTag(tr_body, rpos, cgt)) {
                                if (tc_cl := findPptxMatchingClose(tr_body, cgt + 1, "tc")) {
                                    if (cell_cnt.* >= out_cells.len) return PptxError.SlotOverflow;
                                    const tstart: usize = slab_pos.*;
                                    const wn = extractDrawingParagraphText(tr_body[(cgt + 1)..tc_cl.inner_end], text_slab[tstart..text_slab.len]) orelse return PptxError.BufferTooSmall;
                                    slab_pos.* = tstart + wn;
                                    out_cells[cell_cnt.*] = PptxTableCell{
                                        row_index: tbl.row_count,
                                        col_index: col_i,
                                        text: text_slab[tstart..slab_pos.*],
                                    };
                                    cell_cnt.* += 1;
                                    tbl.cell_count += 1;
                                    col_i += 1;
                                    rpos = tc_cl.after_close;
                                    continue;
                                }
                            }
                        }
                        rpos = cgt + 1;
                    }
                    if (col_i > tbl.col_count) tbl.col_count = col_i;
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

/// Zero-heap structured parser for `ppt/slides/slideN.xml`, extracting shapes, placeholders,
/// EMU geometry, bullet paragraphs, styled runs, and native DrawingML `<a:tbl>` tables.
pub fn parsePptxSlideXml(
    slide_xml: []const u8,
    out_shapes: []PptxShape,
    out_paras: []PptxTextParagraph,
    out_runs: []PptxTextRun,
    out_tables: []PptxTable,
    out_cells: []PptxTableCell,
    text_slab: []u8,
): PptxParsedSlide ! PptxError {
    if (!isSlideXml(slide_xml)) return PptxError.InvalidSlide;
    let res = PptxParsedSlide{};
    let slab_pos: usize = 0;
    let pos: usize = skipProlog(slide_xml);

    while (pos < slide_xml.len) {
        if (slide_xml[pos] != '<') { pos += 1; continue; }
        if (startsWith(slide_xml[pos..slide_xml.len], "<!--")) { pos = skipComment(slide_xml, pos); continue; }
        const gt = findTagEnd(slide_xml, pos) orelse return PptxError.InvalidXml;
        if (pos + 1 < gt && slide_xml[pos + 1] != '/') {
            const qname = parseTagName(slide_xml, pos + 1, gt);
            const sc = isSelfClosingTag(slide_xml, pos, gt);
            if (isNsTag(qname, 'p', "sp") && !sc) {
                const cl = findPptxMatchingClose(slide_xml, gt + 1, "sp") orelse return PptxError.InvalidXml;
                if (res.shape_count >= out_shapes.len) return PptxError.SlotOverflow;
                out_shapes[res.shape_count] = parsePptxShapeElement(
                    slide_xml[(gt + 1)..cl.inner_end],
                    out_paras,
                    *res.para_count,
                    out_runs,
                    *res.run_count,
                    text_slab,
                    *slab_pos,
                )?;
                res.shape_count += 1;
                pos = cl.after_close;
                continue;
            } else if (isNsTag(qname, 'a', "tbl") && !sc) {
                const cl = findPptxMatchingClose(slide_xml, gt + 1, "tbl") orelse return PptxError.InvalidXml;
                if (res.table_count >= out_tables.len) return PptxError.SlotOverflow;
                out_tables[res.table_count] = parseDrawingTable(
                    slide_xml[(gt + 1)..cl.inner_end],
                    out_cells,
                    *res.cell_count,
                    text_slab,
                    *slab_pos,
                )?;
                res.table_count += 1;
                pos = cl.after_close;
                continue;
            }
        }
        pos = gt + 1;
    }
    return res;
}

/// Extracts speaker notes body text from `ppt/notesSlides/notesSlideN.xml` into `out_text`.
pub fn parsePptxNotesXml(notes_xml: []const u8, out_text: []u8): usize ! PptxError {
    if (!isNotesSlideXml(notes_xml)) return PptxError.InvalidNotes;
    return extractSlideText(notes_xml, out_text) orelse return PptxError.BufferTooSmall;
}

// --- 5.3 Zero-Heap PresentationML Slide & Notes Builders ---

/// Builds a valid OOXML `ppt/slides/slideN.xml` from structured shapes/paragraphs/runs and native tables.
pub fn buildPptxSlideXml(
    shapes: []const PptxShape,
    paras: []const PptxTextParagraph,
    runs: []const PptxTextRun,
    tables: []const PptxTable,
    cells: []const PptxTableCell,
    out_xml: []u8,
): usize ! PptxError {
    let pos: usize = 0;
    pos = appendPptxBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendPptxBytes(out_xml, pos, "<p:sld xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\"><p:cSld><p:spTree>")?;

    let si: usize = 0;
    while (si < shapes.len) : (si += 1) {
        const sh = shapes[si];
        pos = appendPptxBytes(out_xml, pos, "<p:sp><p:nvSpPr><p:cNvPr id=\"")?;
        pos = appendPptxU64(out_xml, pos, (si + 2) as u64)?;
        pos = appendPptxBytes(out_xml, pos, "\" name=\"Shape")?;
        pos = appendPptxU64(out_xml, pos, (si + 1) as u64)?;
        pos = appendPptxBytes(out_xml, pos, "\"/><p:cNvSpPr/><p:nvPr>")?;
        const ph_str = placeholderTypeStr(sh.placeholder);
        if (ph_str.len > 0) {
            pos = appendPptxBytes(out_xml, pos, "<p:ph type=\"")?;
            pos = appendPptxBytes(out_xml, pos, ph_str)?;
            pos = appendPptxBytes(out_xml, pos, "\"/>")?;
        }
        pos = appendPptxBytes(out_xml, pos, "</p:nvPr></p:nvSpPr>")?;

        if (sh.has_rect) {
            pos = appendPptxBytes(out_xml, pos, "<p:spPr><a:xfrm><a:off x=\"")?;
            pos = appendPptxI64(out_xml, pos, sh.rect.x)?;
            pos = appendPptxBytes(out_xml, pos, "\" y=\"")?;
            pos = appendPptxI64(out_xml, pos, sh.rect.y)?;
            pos = appendPptxBytes(out_xml, pos, "\"/><a:ext cx=\"")?;
            pos = appendPptxU64(out_xml, pos, sh.rect.cx)?;
            pos = appendPptxBytes(out_xml, pos, "\" cy=\"")?;
            pos = appendPptxU64(out_xml, pos, sh.rect.cy)?;
            pos = appendPptxBytes(out_xml, pos, "\"/></a:xfrm></p:spPr>")?;
        }

        pos = appendPptxBytes(out_xml, pos, "<p:txBody><a:bodyPr/>")?;
        let pi: usize = 0;
        while (pi < sh.para_count) : (pi += 1) {
            const p = paras[sh.para_start + pi];
            pos = appendPptxBytes(out_xml, pos, "<a:p>")?;
            if (p.level > 0 || p.alignment.len > 0) {
                pos = appendPptxBytes(out_xml, pos, "<a:pPr")?;
                if (p.level > 0) {
                    pos = appendPptxBytes(out_xml, pos, " lvl=\"")?;
                    pos = appendPptxU64(out_xml, pos, p.level as u64)?;
                    pos = appendPptxBytes(out_xml, pos, "\"")?;
                }
                if (p.alignment.len > 0) {
                    pos = appendPptxBytes(out_xml, pos, " algn=\"")?;
                    pos = appendPptxXmlEsc(out_xml, pos, p.alignment)?;
                    pos = appendPptxBytes(out_xml, pos, "\"")?;
                }
                pos = appendPptxBytes(out_xml, pos, "/>")?;
            }
            let ri: usize = 0;
            while (ri < p.run_count) : (ri += 1) {
                const r = runs[p.run_start + ri];
                pos = appendPptxBytes(out_xml, pos, "<a:r>")?;
                if (r.bold || r.italic || r.font_size_centipt > 0 || r.has_color) {
                    pos = appendPptxBytes(out_xml, pos, "<a:rPr")?;
                    if (r.bold) pos = appendPptxBytes(out_xml, pos, " b=\"1\"")?;
                    if (r.italic) pos = appendPptxBytes(out_xml, pos, " i=\"1\"")?;
                    if (r.font_size_centipt > 0) {
                        pos = appendPptxBytes(out_xml, pos, " sz=\"")?;
                        pos = appendPptxU64(out_xml, pos, r.font_size_centipt as u64)?;
                        pos = appendPptxBytes(out_xml, pos, "\"")?;
                    }
                    if (r.has_color) {
                        pos = appendPptxBytes(out_xml, pos, "><a:solidFill><a:srgbClr val=\"")?;
                        pos = appendPptxBytes(out_xml, pos, r.color_hex[0..6])?;
                        pos = appendPptxBytes(out_xml, pos, "\"/></a:solidFill></a:rPr>")?;
                    } else {
                        pos = appendPptxBytes(out_xml, pos, "/>")?;
                    }
                }
                pos = appendPptxBytes(out_xml, pos, "<a:t>")?;
                pos = appendPptxXmlEsc(out_xml, pos, r.text)?;
                pos = appendPptxBytes(out_xml, pos, "</a:t></a:r>")?;
            }
            pos = appendPptxBytes(out_xml, pos, "</a:p>")?;
        }
        pos = appendPptxBytes(out_xml, pos, "</p:txBody></p:sp>")?;
    }

    let ti: usize = 0;
    while (ti < tables.len) : (ti += 1) {
        const t = tables[ti];
        pos = appendPptxBytes(out_xml, pos, "<p:graphicFrame><a:graphic><a:graphicData><a:tbl>")?;
        let r_i: usize = 0;
        while (r_i < t.row_count) : (r_i += 1) {
            pos = appendPptxBytes(out_xml, pos, "<a:tr h=\"370840\">")?;
            let ci: usize = 0;
            while (ci < t.cell_count) : (ci += 1) {
                const c = cells[t.cell_start + ci];
                if (c.row_index != r_i) continue;
                pos = appendPptxBytes(out_xml, pos, "<a:tc><a:txBody><a:p><a:r><a:t>")?;
                pos = appendPptxXmlEsc(out_xml, pos, c.text)?;
                pos = appendPptxBytes(out_xml, pos, "</a:t></a:r></a:p></a:txBody></a:tc>")?;
            }
            pos = appendPptxBytes(out_xml, pos, "</a:tr>")?;
        }
        pos = appendPptxBytes(out_xml, pos, "</a:tbl></a:graphicData></a:graphic></p:graphicFrame>")?;
    }

    pos = appendPptxBytes(out_xml, pos, "</p:spTree></p:cSld></p:sld>")?;
    return pos;
}

/// Builds a valid OOXML `ppt/notesSlides/notesSlideN.xml` carrying `notes_text`.
pub fn buildPptxNotesXml(notes_text: []const u8, out_xml: []u8): usize ! PptxError {
    let pos: usize = 0;
    pos = appendPptxBytes(out_xml, pos, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>")?;
    pos = appendPptxBytes(out_xml, pos, "<p:notes xmlns:p=\"http://schemas.openxmlformats.org/presentationml/2006/main\" xmlns:a=\"http://schemas.openxmlformats.org/drawingml/2006/main\"><p:cSld><p:spTree><p:sp><p:nvSpPr><p:cNvPr id=\"2\" name=\"Notes\"/><p:cNvSpPr/><p:nvPr><p:ph type=\"body\"/></p:nvPr></p:nvSpPr><p:txBody><a:bodyPr/><a:p><a:r><a:t>")?;
    pos = appendPptxXmlEsc(out_xml, pos, notes_text)?;
    pos = appendPptxBytes(out_xml, pos, "</a:t></a:r></a:p></p:txBody></p:sp></p:spTree></p:cSld></p:notes>")?;
    return pos;
}

// --- 5.4 Markdown Outline Exporter & Multilingual Slide Statistics ---

/// Renders one slide (and optional speaker notes XML) into clean GFM Markdown outline.
pub fn pptxSlideXmlToMarkdown(
    slide_number: u32,
    slide_xml: []const u8,
    notes_xml: []const u8,
    scratch_shapes: []PptxShape,
    scratch_paras: []PptxTextParagraph,
    scratch_runs: []PptxTextRun,
    scratch_tables: []PptxTable,
    scratch_cells: []PptxTableCell,
    text_slab: []u8,
    notes_scratch: []u8,
    out_md: []u8,
): usize ! PptxError {
    const parsed = parsePptxSlideXml(
        slide_xml,
        scratch_shapes,
        scratch_paras,
        scratch_runs,
        scratch_tables,
        scratch_cells,
        text_slab,
    )?;

    let pos: usize = 0;
    // Find primary title shape if present
    let title_shape_idx: usize = parsed.shape_count;
    let si: usize = 0;
    while (si < parsed.shape_count) : (si += 1) {
        const ph = scratch_shapes[si].placeholder;
        if (ph == PptxPlaceholderKind.title || ph == PptxPlaceholderKind.ctr_title) {
            title_shape_idx = si;
            break;
        }
    }

    pos = appendPptxBytes(out_md, pos, "## Slide ")?;
    pos = appendPptxU64(out_md, pos, slide_number as u64)?;
    if (title_shape_idx < parsed.shape_count && scratch_shapes[title_shape_idx].para_count > 0) {
        pos = appendPptxBytes(out_md, pos, ": ")?;
        const tp = scratch_paras[scratch_shapes[title_shape_idx].para_start];
        let ri: usize = 0;
        while (ri < tp.run_count) : (ri += 1) {
            pos = appendPptxBytes(out_md, pos, scratch_runs[tp.run_start + ri].text)?;
        }
    }
    pos = appendPptxBytes(out_md, pos, "\n")?;

    // Emit body/subtitle shapes
    si = 0;
    while (si < parsed.shape_count) : (si += 1) {
        if (si == title_shape_idx) continue;
        const sh = scratch_shapes[si];
        let pi: usize = 0;
        while (pi < sh.para_count) : (pi += 1) {
            const p = scratch_paras[sh.para_start + pi];
            let ind: u8 = 0;
            while (ind < p.level) : (ind += 1) {
                pos = appendPptxBytes(out_md, pos, "  ")?;
            }
            pos = appendPptxBytes(out_md, pos, "- ")?;
            let ri: usize = 0;
            while (ri < p.run_count) : (ri += 1) {
                const r = scratch_runs[p.run_start + ri];
                if (r.bold) pos = appendPptxBytes(out_md, pos, "**")?;
                if (r.italic) pos = appendPptxBytes(out_md, pos, "*")?;
                pos = appendPptxBytes(out_md, pos, r.text)?;
                if (r.italic) pos = appendPptxBytes(out_md, pos, "*")?;
                if (r.bold) pos = appendPptxBytes(out_md, pos, "**")?;
            }
            pos = appendPptxBytes(out_md, pos, "\n")?;
        }
    }

    // Emit native tables as GFM pipe tables
    let ti: usize = 0;
    while (ti < parsed.table_count) : (ti += 1) {
        const t = scratch_tables[ti];
        let r_i: usize = 0;
        while (r_i < t.row_count) : (r_i += 1) {
            pos = appendPptxBytes(out_md, pos, "|")?;
            let ci: usize = 0;
            while (ci < t.cell_count) : (ci += 1) {
                const c = scratch_cells[t.cell_start + ci];
                if (c.row_index != r_i) continue;
                pos = appendPptxBytes(out_md, pos, " ")?;
                pos = appendPptxBytes(out_md, pos, c.text)?;
                pos = appendPptxBytes(out_md, pos, " |")?;
            }
            pos = appendPptxBytes(out_md, pos, "\n")?;
            if (r_i == 0 && t.col_count > 0) {
                pos = appendPptxBytes(out_md, pos, "|")?;
                let cc: usize = 0;
                while (cc < t.col_count) : (cc += 1) {
                    pos = appendPptxBytes(out_md, pos, " --- |")?;
                }
                pos = appendPptxBytes(out_md, pos, "\n")?;
            }
        }
    }

    // Optional speaker notes
    if (notes_xml.len > 0 && isNotesSlideXml(notes_xml)) {
        const nlen = parsePptxNotesXml(notes_xml, notes_scratch)?;
        if (nlen > 0) {
            pos = appendPptxBytes(out_md, pos, "> Notes: ")?;
            pos = appendPptxBytes(out_md, pos, notes_scratch[0..nlen])?;
            pos = appendPptxBytes(out_md, pos, "\n")?;
        }
    }
    return pos;
}

/// Detailed multilingual slide & speaker notes statistics.
pub struct PptxDetailedStats {
    shape_count: usize = 0;
    table_count: usize = 0;
    paragraph_count: usize = 0;
    char_count: usize = 0;
    english_word_count: usize = 0;
    cjk_char_count: usize = 0;
    notes_word_count: usize = 0;
}

fn isPptxAsciiAlnum(b: u8): bool {
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9');
}

fn countTextWordsAndCjk(text: []const u8, chars: *usize, en_words: *usize, cjk: *usize) {
    let in_word: bool = false;
    let i: usize = 0;
    while (i < text.len) {
        const b0 = text[i];
        if (b0 < 0x80) {
            if (b0 != '\n' && b0 != '\r') chars.* += 1;
            if (isPptxAsciiAlnum(b0)) {
                if (!in_word) {
                    en_words.* += 1;
                    in_word = true;
                }
            } else {
                in_word = false;
            }
            i += 1;
        } else if ((b0 & 0xE0) == 0xC0 && i + 1 < text.len) {
            chars.* += 1;
            in_word = false;
            i += 2;
        } else if ((b0 & 0xF0) == 0xE0 && i + 2 < text.len) {
            const cp: u32 = (((b0 & 0x0F) as u32) << 12)
                | (((text[i + 1] & 0x3F) as u32) << 6)
                | ((text[i + 2] & 0x3F) as u32);
            chars.* += 1;
            if ((cp >= 0x4E00 && cp <= 0x9FFF) || (cp >= 0x3400 && cp <= 0x4DBF) || (cp >= 0x3040 && cp <= 0x30FF)) {
                cjk.* += 1;
            }
            in_word = false;
            i += 3;
        } else if ((b0 & 0xF8) == 0xF0 && i + 3 < text.len) {
            chars.* += 1;
            in_word = false;
            i += 4;
        } else {
            in_word = false;
            i += 1;
        }
    }
}

/// Computes shape/table/paragraph counts, English/CJK character & word counts, and speaker notes word count.
pub fn computePptxSlideStats(
    slide_xml: []const u8,
    notes_xml: []const u8,
    text_scratch: []u8,
): PptxDetailedStats ! PptxError {
    if (!isSlideXml(slide_xml)) return PptxError.InvalidSlide;
    const base = summarizeSlide(slide_xml);
    const tlen = extractSlideText(slide_xml, text_scratch) orelse return PptxError.BufferTooSmall;

    let st = PptxDetailedStats{
        shape_count: base.shape_count,
        table_count: base.table_count,
        paragraph_count: base.paragraph_count,
    };
    countTextWordsAndCjk(text_scratch[0..tlen], *st.char_count, *st.english_word_count, *st.cjk_char_count);

    if (notes_xml.len > 0 && isNotesSlideXml(notes_xml)) {
        const nlen = parsePptxNotesXml(notes_xml, text_scratch)?;
        let nc: usize = 0;
        let nw: usize = 0;
        let ncjk: usize = 0;
        countTextWordsAndCjk(text_scratch[0..nlen], *nc, *nw, *ncjk);
        st.notes_word_count = nw + ncjk;
    }
    return st;
}
