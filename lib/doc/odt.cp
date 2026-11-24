// Origin: Pure-CP Stdlib (Zero-Alloc OASIS OpenDocument ODT/ODS/ODP Parser) | Created: 2026-11-22

/// OASIS OpenDocument Text (.odt) MIME 类型
pub const ODT_MIMETYPE: []const u8 = "application/vnd.oasis.opendocument.text";

/// OASIS OpenDocument Spreadsheet (.ods) MIME 类型
pub const ODS_MIMETYPE: []const u8 = "application/vnd.oasis.opendocument.spreadsheet";

/// OASIS OpenDocument Presentation (.odp) MIME 类型
pub const ODP_MIMETYPE: []const u8 = "application/vnd.oasis.opendocument.presentation";

/// meta.xml 解析出的文档元数据与统计信息（字符串字段零拷贝指向原始 XML）
pub struct OdtMeta {
    title: []const u8,
    description: []const u8,
    subject: []const u8,
    creator: []const u8,
    initial_creator: []const u8,
    creation_date: []const u8,
    date: []const u8,
    language: []const u8,
    generator: []const u8,
    page_count: u32,
    word_count: u32,
    character_count: u32,
}

/// META-INF/manifest.xml 中的 <manifest:file-entry> 条目（零拷贝切片指向原始 XML）
pub struct OdtManifestEntry {
    full_path: []const u8,
    media_type: []const u8,
    version: []const u8,
}

/// content.xml 中的文本块（<text:h> 标题或 <text:p> 段落，零拷贝切片指向原始 XML）
pub struct OdtBlock {
    is_heading: bool,
    outline_level: u8,
    style_name: []const u8,
    raw_xml: []const u8,
}

// ---------------------------------------------------------------------------
// 内部零分配基础辅助函数
// ---------------------------------------------------------------------------

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isTagBoundary(c: u8): bool {
    return isSpace(c) || c == '>' || c == '/';
}

fn isNameStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
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

fn startsWithAt(haystack: []const u8, pos: usize, needle: []const u8): bool {
    if (pos + needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) {
        if (haystack[pos + i] != needle[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn findSubstr(haystack: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0) {
        return if (start <= haystack.len) start else null;
    }
    if (start + needle.len > haystack.len) {
        return null;
    }
    let i = start;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        if (startsWithAt(haystack, i, needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn stripCdata(raw: []const u8): []const u8 {
    const s = trimAscii(raw);
    const prefix = "<![CDATA[";
    const suffix = "]]>";
    if (s.len >= prefix.len + suffix.len && startsWithAt(s, 0, prefix) && startsWithAt(s, s.len - suffix.len, suffix)) {
        return trimAscii(s[prefix.len .. s.len - suffix.len]);
    }
    return s;
}

fn localName(name: []const u8): []const u8 {
    let i: usize = 0;
    while (i < name.len) {
        if (name[i] == ':') {
            return name[i + 1 ..];
        }
        i += 1;
    }
    return name;
}

fn readU16LE(data: []const u8, offset: usize): u16 {
    const lo = u16(data[offset]);
    const hi = u16(data[offset + 1]);
    return lo | (hi << 8);
}

fn readU32LE(data: []const u8, offset: usize): u32 {
    const b0 = u32(data[offset]);
    const b1 = u32(data[offset + 1]);
    const b2 = u32(data[offset + 2]);
    const b3 = u32(data[offset + 3]);
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn parseU32(s: []const u8): u32? {
    const trimmed = trimAscii(s);
    if (trimmed.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < trimmed.len) {
        const c = trimmed[i];
        if (c < '0' || c > '9') {
            return null;
        }
        acc = acc * 10 + u64(c - '0');
        if (acc > 0xFFFFFFFF) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

fn parseU8(s: []const u8): u8? {
    const val = parseU32(s) ?? return null;
    if (val > 255) {
        return null;
    }
    return val as u8;
}

// ---------------------------------------------------------------------------
// 内部零分配 XML 扫描辅助结构与函数
// ---------------------------------------------------------------------------

struct OpenTagInfo {
    full_name: []const u8,
    attrs: []const u8,
    tag_end: usize,
    self_closing: bool,
}

struct CloseTagInfo {
    full_name: []const u8,
    tag_end: usize,
}

struct CloseMatch {
    inner_end: usize,
    after_close: usize,
}

struct ElementSpan {
    attrs: []const u8,
    inner: []const u8,
    next_pos: usize,
}

fn findTagClose(xml: []const u8, open_pos: usize): usize? {
    let i = open_pos;
    let quote: u8 = 0;
    while (i < xml.len) {
        const c = xml[i];
        if (quote != 0) {
            if (c == quote) {
                quote = 0;
            }
        } else {
            if (c == '"' || c == '\'') {
                quote = c;
            } else if (c == '>') {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn skipNonElementMarkup(xml: []const u8, pos: usize): usize {
    if (startsWithAt(xml, pos, "<!--")) {
        if (end_idx := findSubstr(xml, pos + 4, "-->")) {
            return end_idx + 3;
        }
        return xml.len;
    }
    if (startsWithAt(xml, pos, "<![CDATA[")) {
        if (end_idx := findSubstr(xml, pos + 9, "]]>")) {
            return end_idx + 3;
        }
        return xml.len;
    }
    if (startsWithAt(xml, pos, "<?")) {
        if (end_idx := findSubstr(xml, pos + 2, "?>")) {
            return end_idx + 2;
        }
        return xml.len;
    }
    if (startsWithAt(xml, pos, "<!")) {
        if (end_idx := findTagClose(xml, pos + 2)) {
            return end_idx + 1;
        }
        return xml.len;
    }
    return pos + 1;
}

fn parseOpenTagAt(xml: []const u8, pos: usize): OpenTagInfo? {
    if (pos + 2 >= xml.len || xml[pos] != '<') {
        return null;
    }
    const name_start = pos + 1;
    if (!isNameStart(xml[name_start])) {
        return null;
    }
    let name_end = name_start;
    while (name_end < xml.len && !isTagBoundary(xml[name_end])) {
        name_end += 1;
    }
    if (name_end == name_start) {
        return null;
    }
    const close_gt = findTagClose(xml, name_end) ?? return null;
    let attr_end = close_gt;
    let self_closing = false;
    if (close_gt > name_end && xml[close_gt - 1] == '/') {
        self_closing = true;
        attr_end = close_gt - 1;
    }
    return OpenTagInfo {
        full_name: xml[name_start..name_end],
        attrs: xml[name_end..attr_end],
        tag_end: close_gt + 1,
        self_closing: self_closing,
    };
}

fn parseCloseTagAt(xml: []const u8, pos: usize): CloseTagInfo? {
    if (pos + 3 >= xml.len || xml[pos] != '<' || xml[pos + 1] != '/') {
        return null;
    }
    let name_start = pos + 2;
    while (name_start < xml.len && isSpace(xml[name_start])) {
        name_start += 1;
    }
    if (name_start >= xml.len || !isNameStart(xml[name_start])) {
        return null;
    }
    let name_end = name_start;
    while (name_end < xml.len && !isTagBoundary(xml[name_end])) {
        name_end += 1;
    }
    let i = name_end;
    while (i < xml.len && isSpace(xml[i])) {
        i += 1;
    }
    if (i >= xml.len || xml[i] != '>') {
        return null;
    }
    return CloseTagInfo {
        full_name: xml[name_start..name_end],
        tag_end: i + 1,
    };
}

/// 按属性 local name 提取属性值（同时兼容带命名空间前缀如 `meta:page-count` 与无前缀 `page-count`）
fn getAttrLocal(attrs: []const u8, target_local: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < attrs.len) {
        while (i < attrs.len && isSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) {
            break;
        }
        const name_start = i;
        while (i < attrs.len && !isSpace(attrs[i]) && attrs[i] != '=' && attrs[i] != '/' && attrs[i] != '>') {
            i += 1;
        }
        const attr_name = attrs[name_start..i];
        while (i < attrs.len && isSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || attrs[i] != '=') {
            continue;
        }
        i += 1;
        while (i < attrs.len && isSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const quote = attrs[i];
        if (quote != '"' && quote != '\'') {
            while (i < attrs.len && !isSpace(attrs[i])) {
                i += 1;
            }
            continue;
        }
        i += 1;
        const val_start = i;
        while (i < attrs.len && attrs[i] != quote) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const val = attrs[val_start..i];
        i += 1;
        if (eqlBytes(localName(attr_name), target_local)) {
            return val;
        }
    }
    return null;
}

fn findMatchingCloseTag(xml: []const u8, start: usize, tag_local: []const u8): CloseMatch? {
    let i = start;
    let depth: usize = 0;
    while (i < xml.len) {
        if (xml[i] != '<') {
            i += 1;
            continue;
        }
        if (i + 1 < xml.len && (xml[i + 1] == '!' || xml[i + 1] == '?')) {
            i = skipNonElementMarkup(xml, i);
            continue;
        }
        if (i + 1 < xml.len && xml[i + 1] == '/') {
            if (ct := parseCloseTagAt(xml, i)) {
                if (eqlBytes(localName(ct.full_name), tag_local)) {
                    if (depth == 0) {
                        return CloseMatch {
                            inner_end: i,
                            after_close: ct.tag_end,
                        };
                    }
                    depth -= 1;
                }
                i = ct.tag_end;
                continue;
            }
            i += 1;
            continue;
        }
        if (ot := parseOpenTagAt(xml, i)) {
            if (!ot.self_closing && eqlBytes(localName(ot.full_name), tag_local)) {
                depth += 1;
            }
            i = ot.tag_end;
            continue;
        }
        i += 1;
    }
    return null;
}

fn findElementSpan(xml: []const u8, start: usize, target_local: []const u8): ElementSpan? {
    let i = start;
    while (i < xml.len) {
        if (xml[i] != '<') {
            i += 1;
            continue;
        }
        if (i + 1 < xml.len && (xml[i + 1] == '!' || xml[i + 1] == '?')) {
            i = skipNonElementMarkup(xml, i);
            continue;
        }
        if (i + 1 < xml.len && xml[i + 1] == '/') {
            if (ct := parseCloseTagAt(xml, i)) {
                i = ct.tag_end;
            } else {
                i += 1;
            }
            continue;
        }
        if (ot := parseOpenTagAt(xml, i)) {
            if (eqlBytes(localName(ot.full_name), target_local)) {
                if (ot.self_closing) {
                    return ElementSpan {
                        attrs: ot.attrs,
                        inner: xml[ot.tag_end..ot.tag_end],
                        next_pos: ot.tag_end,
                    };
                }
                const cm = findMatchingCloseTag(xml, ot.tag_end, target_local) ?? return null;
                return ElementSpan {
                    attrs: ot.attrs,
                    inner: xml[ot.tag_end..cm.inner_end],
                    next_pos: cm.after_close,
                };
            }
            i = ot.tag_end;
            continue;
        }
        i += 1;
    }
    return null;
}

// ---------------------------------------------------------------------------
// 1. OCF ZIP 首文件 mimetype 校验
// ---------------------------------------------------------------------------

/// 按 OASIS OpenDocument OCF 规范校验 ZIP 首个本地文件头：
/// - 签名 `PK\x03\x04` (offset 0..4)
/// - 未压缩 method == 0 (offset 8..10)
/// - 压缩长度 == 未压缩长度 > 0，且数据区未截断
/// - filename_len == 8 ("mimetype")，extra_len == 0
/// 成功时返回零拷贝 MIME 切片（如 ODT_MIMETYPE / ODS_MIMETYPE / ODP_MIMETYPE）
pub fn detectOdfMimetype(zip_data: []const u8): ([]const u8)? {
    if (zip_data.len < 38) {
        return null;
    }
    if (zip_data[0] != 0x50 || zip_data[1] != 0x4B || zip_data[2] != 0x03 || zip_data[3] != 0x04) {
        return null;
    }
    const method = readU16LE(zip_data, 8);
    if (method != 0) {
        return null;
    }
    const comp_size = readU32LE(zip_data, 18);
    const uncomp_size = readU32LE(zip_data, 22);
    if (comp_size == 0 || comp_size != uncomp_size) {
        return null;
    }
    const fn_len = readU16LE(zip_data, 26);
    const extra_len = readU16LE(zip_data, 28);
    if (fn_len != 8 || extra_len != 0) {
        return null;
    }
    if (!eqlBytes(zip_data[30..38], "mimetype")) {
        return null;
    }
    const mime_len = usize(comp_size);
    if (mime_len > zip_data.len - 38) {
        return null;
    }
    return zip_data[38 .. 38 + mime_len];
}

/// 判断给定 ZIP 缓冲区是否为合法 ODT（OpenDocument Text）文档
pub fn isOdtZip(zip_data: []const u8): bool {
    if (mime := detectOdfMimetype(zip_data)) {
        return eqlBytes(mime, ODT_MIMETYPE);
    }
    return false;
}

// ---------------------------------------------------------------------------
// 2. meta.xml 元数据解析
// ---------------------------------------------------------------------------

/// 零分配解析 ODF meta.xml 元数据（dc:*、meta:* 与 <meta:document-statistic>）
pub fn parseOdtMeta(meta_xml: []const u8): OdtMeta? {
    let search_scope = meta_xml;
    if (doc_meta := findElementSpan(meta_xml, 0, "document-meta")) {
        search_scope = doc_meta.inner;
    }
    const meta_span = findElementSpan(search_scope, 0, "meta") ?? return null;

    let title: []const u8 = "";
    let description: []const u8 = "";
    let subject: []const u8 = "";
    let creator: []const u8 = "";
    let initial_creator: []const u8 = "";
    let creation_date: []const u8 = "";
    let date: []const u8 = "";
    let language: []const u8 = "";
    let generator: []const u8 = "";
    let page_count: u32 = 0;
    let word_count: u32 = 0;
    let character_count: u32 = 0;

    const inner = meta_span.inner;
    let i: usize = 0;
    while (i < inner.len) {
        if (inner[i] != '<') {
            i += 1;
            continue;
        }
        if (i + 1 < inner.len && (inner[i + 1] == '!' || inner[i + 1] == '?')) {
            i = skipNonElementMarkup(inner, i);
            continue;
        }
        if (i + 1 < inner.len && inner[i + 1] == '/') {
            if (ct := parseCloseTagAt(inner, i)) {
                i = ct.tag_end;
            } else {
                i += 1;
            }
            continue;
        }
        if (ot := parseOpenTagAt(inner, i)) {
            const lname = localName(ot.full_name);
            if (eqlBytes(lname, "document-statistic")) {
                if (v := getAttrLocal(ot.attrs, "page-count")) {
                    if (n := parseU32(v)) {
                        page_count = n;
                    }
                }
                if (v := getAttrLocal(ot.attrs, "word-count")) {
                    if (n := parseU32(v)) {
                        word_count = n;
                    }
                }
                if (v := getAttrLocal(ot.attrs, "character-count")) {
                    if (n := parseU32(v)) {
                        character_count = n;
                    }
                }
                if (ot.self_closing) {
                    i = ot.tag_end;
                } else if (cm := findMatchingCloseTag(inner, ot.tag_end, lname)) {
                    i = cm.after_close;
                } else {
                    i = ot.tag_end;
                }
                continue;
            }
            if (ot.self_closing) {
                i = ot.tag_end;
                continue;
            }
            if (cm := findMatchingCloseTag(inner, ot.tag_end, lname)) {
                const text = stripCdata(inner[ot.tag_end..cm.inner_end]);
                if (text.len > 0) {
                    if (eqlBytes(lname, "title")) {
                        if (title.len == 0) title = text;
                    } else if (eqlBytes(lname, "description")) {
                        if (description.len == 0) description = text;
                    } else if (eqlBytes(lname, "subject")) {
                        if (subject.len == 0) subject = text;
                    } else if (eqlBytes(lname, "creator")) {
                        if (creator.len == 0) creator = text;
                    } else if (eqlBytes(lname, "initial-creator")) {
                        if (initial_creator.len == 0) initial_creator = text;
                    } else if (eqlBytes(lname, "creation-date")) {
                        if (creation_date.len == 0) creation_date = text;
                    } else if (eqlBytes(lname, "date")) {
                        if (date.len == 0) date = text;
                    } else if (eqlBytes(lname, "language")) {
                        if (language.len == 0) language = text;
                    } else if (eqlBytes(lname, "generator")) {
                        if (generator.len == 0) generator = text;
                    }
                }
                i = cm.after_close;
                continue;
            }
            i = ot.tag_end;
            continue;
        }
        i += 1;
    }

    return OdtMeta {
        title: title,
        description: description,
        subject: subject,
        creator: creator,
        initial_creator: initial_creator,
        creation_date: creation_date,
        date: date,
        language: language,
        generator: generator,
        page_count: page_count,
        word_count: word_count,
        character_count: character_count,
    };
}

// ---------------------------------------------------------------------------
// 3. META-INF/manifest.xml 清单迭代器
// ---------------------------------------------------------------------------

/// 零分配 ODF META-INF/manifest.xml <manifest:file-entry> 迭代器
pub struct OdtManifestIter {
    section: []const u8,
    pos: usize,

    pub fn init(manifest_xml: []const u8): OdtManifestIter {
        if (man := findElementSpan(manifest_xml, 0, "manifest")) {
            return OdtManifestIter {
                section: man.inner,
                pos: 0,
            };
        }
        return OdtManifestIter {
            section: "",
            pos: 0,
        };
    }

    pub fn next(*_): OdtManifestEntry? {
        const xml = _.section;
        while (_.pos < xml.len) {
            if (xml[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (_.pos + 1 < xml.len && (xml[_.pos + 1] == '!' || xml[_.pos + 1] == '?')) {
                _.pos = skipNonElementMarkup(xml, _.pos);
                continue;
            }
            if (_.pos + 1 < xml.len && xml[_.pos + 1] == '/') {
                if (ct := parseCloseTagAt(xml, _.pos)) {
                    _.pos = ct.tag_end;
                } else {
                    _.pos += 1;
                }
                continue;
            }
            if (ot := parseOpenTagAt(xml, _.pos)) {
                _.pos = ot.tag_end;
                if (eqlBytes(localName(ot.full_name), "file-entry")) {
                    const fp = getAttrLocal(ot.attrs, "full-path") ?? "";
                    if (fp.len > 0) {
                        const mt = getAttrLocal(ot.attrs, "media-type") ?? "";
                        const ver = getAttrLocal(ot.attrs, "version") ?? "";
                        return OdtManifestEntry {
                            full_path: fp,
                            media_type: mt,
                            version: ver,
                        };
                    }
                }
                continue;
            }
            _.pos += 1;
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// 4. content.xml 文本块迭代器 (<text:h> / <text:p>)
// ---------------------------------------------------------------------------

/// 零分配 ODF content.xml 文本块迭代器（迭代 <text:h> 与 <text:p>）
pub struct OdtBlockIter {
    section: []const u8,
    pos: usize,

    pub static fn init(content_xml: []const u8): OdtBlockIter {
        if (body := findElementSpan(content_xml, 0, "body")) {
            if (txt := findElementSpan(body.inner, 0, "text")) {
                return OdtBlockIter {
                    section: txt.inner,
                    pos: 0,
                };
            }
            return OdtBlockIter {
                section: body.inner,
                pos: 0,
            };
        }
        if (txt := findElementSpan(content_xml, 0, "text")) {
            return OdtBlockIter {
                section: txt.inner,
                pos: 0,
            };
        }
        return OdtBlockIter {
            section: "",
            pos: 0,
        };
    }

    pub fn next(*_): OdtBlock? {
        const xml = _.section;
        while (_.pos < xml.len) {
            if (xml[_.pos] != '<') {
                _.pos += 1;
                continue;
            }
            if (_.pos + 1 < xml.len && (xml[_.pos + 1] == '!' || xml[_.pos + 1] == '?')) {
                _.pos = skipNonElementMarkup(xml, _.pos);
                continue;
            }
            if (_.pos + 1 < xml.len && xml[_.pos + 1] == '/') {
                if (ct := parseCloseTagAt(xml, _.pos)) {
                    _.pos = ct.tag_end;
                } else {
                    _.pos += 1;
                }
                continue;
            }
            if (ot := parseOpenTagAt(xml, _.pos)) {
                const is_h = eqlBytes(ot.full_name, "text:h") || eqlBytes(ot.full_name, "h");
                const is_p = eqlBytes(ot.full_name, "text:p") || eqlBytes(ot.full_name, "p");
                if (is_h || is_p) {
                    const style_name = getAttrLocal(ot.attrs, "style-name") ?? "";
                    let outline_level: u8 = 0;
                    if (is_h) {
                        outline_level = 1;
                        if (lvl_str := getAttrLocal(ot.attrs, "outline-level")) {
                            if (lvl := parseU8(lvl_str)) {
                                outline_level = lvl;
                            }
                        }
                    }
                    if (ot.self_closing) {
                        _.pos = ot.tag_end;
                        return OdtBlock {
                            is_heading: is_h,
                            outline_level: outline_level,
                            style_name: style_name,
                            raw_xml: xml[ot.tag_end..ot.tag_end],
                        };
                    }
                    const lname = localName(ot.full_name);
                    if (cm := findMatchingCloseTag(xml, ot.tag_end, lname)) {
                        _.pos = cm.after_close;
                        return OdtBlock {
                            is_heading: is_h,
                            outline_level: outline_level,
                            style_name: style_name,
                            raw_xml: xml[ot.tag_end..cm.inner_end],
                        };
                    }
                    // 未闭合块标签：安全终止迭代
                    _.pos = xml.len;
                    return null;
                }
                _.pos = ot.tag_end;
                continue;
            }
            _.pos += 1;
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// 5. content.xml 零分配纯文本提取
// ---------------------------------------------------------------------------

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

fn writeBytes(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn writeRepeatedByte(out: []u8, pos: *usize, b: u8, count: u32): bool {
    const n = usize(count);
    if (n > out.len - pos.*) {
        return false;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos.* + i] = b;
        i += 1;
    }
    pos.* += n;
    return true;
}

fn extractBlockText(raw_xml: []const u8, out: []u8, pos: *usize): bool {
    let i: usize = 0;
    while (i < raw_xml.len) {
        const c = raw_xml[i];
        if (c == '<') {
            if (startsWithAt(raw_xml, i, "<![CDATA[")) {
                if (end_idx := findSubstr(raw_xml, i + 9, "]]>")) {
                    if (!writeBytes(out, pos, raw_xml[i + 9 .. end_idx])) {
                        return false;
                    }
                    i = end_idx + 3;
                    continue;
                }
                break;
            }
            if (i + 1 < raw_xml.len && (raw_xml[i + 1] == '!' || raw_xml[i + 1] == '?')) {
                i = skipNonElementMarkup(raw_xml, i);
                continue;
            }
            if (i + 1 < raw_xml.len && raw_xml[i + 1] == '/') {
                if (ct := parseCloseTagAt(raw_xml, i)) {
                    i = ct.tag_end;
                } else {
                    i += 1;
                }
                continue;
            }
            if (ot := parseOpenTagAt(raw_xml, i)) {
                const lname = localName(ot.full_name);
                if (eqlBytes(lname, "s")) {
                    let count: u32 = 1;
                    if (c_attr := getAttrLocal(ot.attrs, "c")) {
                        if (parsed := parseU32(c_attr)) {
                            count = parsed;
                        }
                    }
                    if (!writeRepeatedByte(out, pos, ' ', count)) {
                        return false;
                    }
                } else if (eqlBytes(lname, "tab")) {
                    if (!writeByte(out, pos, '\t')) {
                        return false;
                    }
                } else if (eqlBytes(lname, "line-break")) {
                    if (!writeByte(out, pos, '\n')) {
                        return false;
                    }
                }
                i = ot.tag_end;
                continue;
            }
            // 未闭合标签，直接终止本块
            break;
        }
        if (c == '&') {
            if (startsWithAt(raw_xml, i, "&amp;")) {
                if (!writeByte(out, pos, '&')) return false;
                i += 5;
                continue;
            }
            if (startsWithAt(raw_xml, i, "&lt;")) {
                if (!writeByte(out, pos, '<')) return false;
                i += 4;
                continue;
            }
            if (startsWithAt(raw_xml, i, "&gt;")) {
                if (!writeByte(out, pos, '>')) return false;
                i += 4;
                continue;
            }
            if (startsWithAt(raw_xml, i, "&quot;")) {
                if (!writeByte(out, pos, '"')) return false;
                i += 6;
                continue;
            }
            if (startsWithAt(raw_xml, i, "&apos;")) {
                if (!writeByte(out, pos, '\'')) return false;
                i += 6;
                continue;
            }
        }
        if (!writeByte(out, pos, c)) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 零堆分配从 ODF content.xml 提取纯文本到调用方缓冲区 `out`：
/// - 支持 `<text:s text:c="N"/>`（默认 1 个空格）、`<text:tab/>`、`<text:line-break/>`
/// - 自动穿透嵌套 `<text:span>` 等内联标签，反转义 `&amp;`/`&lt;`/`&gt;`/`&quot;`/`&apos;`
/// - 块间以 `'\n'` 分隔；输出缓冲区不足时返回 `null`
pub fn extractOdtText(content_xml: []const u8, out: []u8): usize? {
    let it = OdtBlockIter.init(content_xml);
    let pos: usize = 0;
    let block_idx: usize = 0;
    while (blk := it.next()) {
        if (block_idx > 0) {
            if (!writeByte(out, &pos, '\n')) {
                return null;
            }
        }
        if (!extractBlockText(blk.raw_xml, out, &pos)) {
            return null;
        }
        block_idx += 1;
    }
    return pos;
}
