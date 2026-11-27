// Origin: Pure-CP Stdlib (Zero-Alloc EPUB 2/3 OCF & OPF Package Parser) | Created: 2026-11-22

/// EPUB OCF 规范规定的 mimetype 文件内容
pub const EPUB_MIMETYPE: []const u8 = "application/epub+zip";

/// EPUB OPF Package 文档的标准 media-type
pub const EPUB_OPF_MEDIA_TYPE: []const u8 = "application/oebps-package+xml";

/// META-INF/container.xml 中的 <rootfile> 条目（零拷贝切片指向原始 XML）
pub struct EpubRootfile {
    full_path: []const u8,
    media_type: []const u8,
}

/// OPF <package> 与 <metadata> 元数据（零拷贝切片指向原始 OPF XML）
pub struct EpubMetadata {
    version: []const u8,
    unique_identifier: []const u8,
    title: []const u8,
    creator: []const u8,
    language: []const u8,
    identifier: []const u8,
    publisher: []const u8,
    date: []const u8,
    description: []const u8,
}

/// OPF <manifest> 中的 <item> 条目（零拷贝切片指向原始 OPF XML）
pub struct EpubManifestItem {
    id: []const u8,
    href: []const u8,
    media_type: []const u8,
    properties: []const u8,
}

/// OPF <spine> 中的 <itemref> 条目（零拷贝切片指向原始 OPF XML）
pub struct EpubSpineItem {
    idref: []const u8,
    linear: bool,
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

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
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

// ---------------------------------------------------------------------------
// 内部零分配 XML 扫描辅助结构与函数
// ---------------------------------------------------------------------------

struct OpenTagInfo {
    full_name: []const u8,
    attrs: []const u8,
    tag_end: usize,
    self_closing: bool,
}

struct ElementSpan {
    attrs: []const u8,
    inner: []const u8,
    next_pos: usize,
}

/// 跳过位于 pos 处的 XML 注释、CDATA、处理指令或 DOCTYPE，返回跳过后的新位置
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

/// 查找元素标签结束位置 `>`（忽略引号属性内部的 `>`）
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

/// 解析位于 `pos`（`xml[pos] == '<'`）处的开始标签或自闭合标签
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

/// 解析闭合标签 `</name>`，返回标签名与 `>` 后的位置
struct CloseTagInfo {
    full_name: []const u8,
    tag_end: usize,
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

/// 从属性串中零分配提取指定属性值（兼容单双引号、空白与任意顺序）
fn getAttr(attrs: []const u8, target_name: []const u8): ([]const u8)? {
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
        i += 1; // skip '='
        while (i < attrs.len && isSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const quote = attrs[i];
        if (quote != '"' && quote != '\'') {
            // 无引号属性容错跳过
            while (i < attrs.len && !isSpace(attrs[i])) {
                i += 1;
            }
            continue;
        }
        i += 1; // skip opening quote
        const val_start = i;
        while (i < attrs.len && attrs[i] != quote) {
            i += 1;
        }
        if (i >= attrs.len) {
            // 未闭合引号
            return null;
        }
        const val = attrs[val_start..i];
        i += 1; // skip closing quote
        if (eqlBytes(attr_name, target_name)) {
            return val;
        }
    }
    return null;
}

/// 在 `xml[start..]` 中查找与 `tag_local` 匹配的闭合标签，支持同名嵌套
struct CloseMatch {
    inner_end: usize,
    after_close: usize,
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

/// 在 `xml[start..]` 中查找首个指定 local name 的非自闭合元素，并返回其属性与内部切片
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
// 1. OCF ZIP 首文件 mimetype 严格校验
// ---------------------------------------------------------------------------

/// 按 EPUB OCF 规范校验 ZIP 首个本地文件头：
/// - 签名 `PK\x03\x04` (offset 0..4)
/// - 未压缩 method == 0 (offset 8..10)
/// - 压缩/未压缩长度合法（0 或 20，offset 18..26）
/// - 文件名长度 == 8 (offset 26..28)，无 extra 字段 extra_len == 0 (offset 28..30)
/// - offset 30..38 == "mimetype" 且 offset 38..58 == "application/epub+zip"
pub fn isEpubZip(data: []const u8): bool {
    // 30 bytes local file header + 8 bytes "mimetype" + 20 bytes "application/epub+zip" = 58
    if (data.len < 58) {
        return false;
    }
    // ZIP Local File Header signature: PK\x03\x04
    if (data[0] != 0x50 || data[1] != 0x4B || data[2] != 0x03 || data[3] != 0x04) {
        return false;
    }
    // Compression method must be 0 (Stored / uncompressed)
    const method = readU16LE(data, 8);
    if (method != 0) {
        return false;
    }
    // Sizes: if present (non-zero), must match EPUB_MIMETYPE.len (20)
    const comp_size = readU32LE(data, 18);
    const uncomp_size = readU32LE(data, 22);
    if (comp_size != 0 && comp_size != EPUB_MIMETYPE.len) {
        return false;
    }
    if (uncomp_size != 0 && uncomp_size != EPUB_MIMETYPE.len) {
        return false;
    }
    // Filename length must be 8 ("mimetype") and extra field length must be 0
    const fn_len = readU16LE(data, 26);
    const extra_len = readU16LE(data, 28);
    if (fn_len != 8 || extra_len != 0) {
        return false;
    }
    // Exact match at offsets 30..38 and 38..58
    if (!eqlBytes(data[30..38], "mimetype")) {
        return false;
    }
    if (!eqlBytes(data[38..58], EPUB_MIMETYPE)) {
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 2. META-INF/container.xml 解析
// ---------------------------------------------------------------------------

/// 零分配解析 META-INF/container.xml，提取 OPF <rootfile full-path="..." media-type="..." />。
/// 优先返回 media-type 为 "application/oebps-package+xml" 的条目；若无则回退首个含 full-path 的条目。
pub fn parseContainerXml(xml: []const u8): EpubRootfile? {
    let i: usize = 0;
    let fallback: EpubRootfile? = null;
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
            if (eqlBytes(localName(ot.full_name), "rootfile")) {
                if (fp := getAttr(ot.attrs, "full-path")) {
                    if (fp.len > 0) {
                        const mt = getAttr(ot.attrs, "media-type") ?? "";
                        const rf = EpubRootfile {
                            full_path: fp,
                            media_type: mt,
                        };
                        if (eqlBytes(mt, EPUB_OPF_MEDIA_TYPE)) {
                            return rf;
                        }
                        if (fallback == null) {
                            fallback = rf;
                        }
                    }
                }
            }
            i = ot.tag_end;
            continue;
        }
        i += 1;
    }
    return fallback;
}

// ---------------------------------------------------------------------------
// 3. OPF Package <metadata> 解析
// ---------------------------------------------------------------------------

/// 零分配解析 OPF Package 文档的 <package> 属性与 <metadata> 区块。
/// 兼容 `dc:title` 与 `title` 等命名空间前缀、CDATA 与带属性标签。
pub fn parseOpfMetadata(opf_xml: []const u8): EpubMetadata? {
    const pkg = findElementSpan(opf_xml, 0, "package") ?? return null;
    const version = getAttr(pkg.attrs, "version") ?? "";
    const unique_id = getAttr(pkg.attrs, "unique-identifier") ?? "";

    const meta_span = findElementSpan(pkg.inner, 0, "metadata") ?? return null;

    let title: []const u8 = "";
    let creator: []const u8 = "";
    let language: []const u8 = "";
    let identifier: []const u8 = "";
    let identifier_matched_unique = false;
    let publisher: []const u8 = "";
    let date: []const u8 = "";
    let description: []const u8 = "";

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
            if (ot.self_closing) {
                i = ot.tag_end;
                continue;
            }
            const lname = localName(ot.full_name);
            if (cm := findMatchingCloseTag(inner, ot.tag_end, lname)) {
                const text = stripCdata(inner[ot.tag_end..cm.inner_end]);
                if (text.len > 0) {
                    if (eqlBytes(lname, "title")) {
                        if (title.len == 0) {
                            title = text;
                        }
                    } else if (eqlBytes(lname, "creator")) {
                        if (creator.len == 0) {
                            creator = text;
                        }
                    } else if (eqlBytes(lname, "language")) {
                        if (language.len == 0) {
                            language = text;
                        }
                    } else if (eqlBytes(lname, "identifier")) {
                        const elem_id = getAttr(ot.attrs, "id") ?? "";
                        if (unique_id.len > 0 && eqlBytes(elem_id, unique_id)) {
                            identifier = text;
                            identifier_matched_unique = true;
                        } else if (!identifier_matched_unique && identifier.len == 0) {
                            identifier = text;
                        }
                    } else if (eqlBytes(lname, "publisher")) {
                        if (publisher.len == 0) {
                            publisher = text;
                        }
                    } else if (eqlBytes(lname, "date")) {
                        if (date.len == 0) {
                            date = text;
                        }
                    } else if (eqlBytes(lname, "description")) {
                        if (description.len == 0) {
                            description = text;
                        }
                    }
                }
                i = cm.after_close;
                continue;
            }
            // 未闭合子标签，跳过开始标签继续扫描防止死循环
            i = ot.tag_end;
            continue;
        }
        i += 1;
    }

    return EpubMetadata {
        version: version,
        unique_identifier: unique_id,
        title: title,
        creator: creator,
        language: language,
        identifier: identifier,
        publisher: publisher,
        date: date,
        description: description,
    };
}

// ---------------------------------------------------------------------------
// 4. OPF <manifest> 迭代器与按 ID 查找
// ---------------------------------------------------------------------------

/// 零分配 OPF <manifest> <item> 迭代器
pub struct EpubManifestIter {
    section: []const u8,
    pos: usize,

    pub static fn init(opf_xml: []const u8): EpubManifestIter {
        if (pkg := findElementSpan(opf_xml, 0, "package")) {
            if (man := findElementSpan(pkg.inner, 0, "manifest")) {
                return EpubManifestIter {
                    section: man.inner,
                    pos: 0,
                };
            }
        }
        return EpubManifestIter {
            section: "",
            pos: 0,
        };
    }

    pub fn next(*_): EpubManifestItem? {
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
                if (eqlBytes(localName(ot.full_name), "item")) {
                    const id = getAttr(ot.attrs, "id") ?? "";
                    const href = getAttr(ot.attrs, "href") ?? "";
                    if (id.len > 0 && href.len > 0) {
                        const mt = getAttr(ot.attrs, "media-type") ?? "";
                        const props = getAttr(ot.attrs, "properties") ?? "";
                        return EpubManifestItem {
                            id: id,
                            href: href,
                            media_type: mt,
                            properties: props,
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

/// 在 OPF <manifest> 中按 id 零分配查找对应的 <item>
pub fn findManifestItem(opf_xml: []const u8, id: []const u8): EpubManifestItem? {
    if (id.len == 0) {
        return null;
    }
    let it = EpubManifestIter.init(opf_xml);
    while (item := it.next()) {
        if (eqlBytes(item.id, id)) {
            return item;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// 5. OPF <spine> 迭代器
// ---------------------------------------------------------------------------

/// 零分配 OPF <spine> <itemref> 迭代器
pub struct EpubSpineIter {
    section: []const u8,
    pos: usize,

    pub static fn init(opf_xml: []const u8): EpubSpineIter {
        if (pkg := findElementSpan(opf_xml, 0, "package")) {
            if (sp := findElementSpan(pkg.inner, 0, "spine")) {
                return EpubSpineIter {
                    section: sp.inner,
                    pos: 0,
                };
            }
        }
        return EpubSpineIter {
            section: "",
            pos: 0,
        };
    }

    pub fn next(*_): EpubSpineItem? {
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
                if (eqlBytes(localName(ot.full_name), "itemref")) {
                    const idref = getAttr(ot.attrs, "idref") ?? "";
                    if (idref.len > 0) {
                        let linear = true;
                        if (lin_attr := getAttr(ot.attrs, "linear")) {
                            if (eqlAsciiIgnoreCase(lin_attr, "no")) {
                                linear = false;
                            }
                        }
                        return EpubSpineItem {
                            idref: idref,
                            linear: linear,
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
