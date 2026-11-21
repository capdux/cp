// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc OPML 2.0 Outline Parser & Writer)

/// OPML 2.0 `<head>` 元数据（零堆分配切片视图）
pub struct OpmlHead {
    title: ([]const u8)? = null,
    date_created: ([]const u8)? = null,
    owner_name: ([]const u8)? = null,
}

/// OPML 2.0 `<outline>` 节点（零堆分配切片视图，含嵌套深度）
pub struct OpmlOutline {
    text: []const u8 = "",
    title: ([]const u8)? = null,
    kind: ([]const u8)? = null,
    xml_url: ([]const u8)? = null,
    html_url: ([]const u8)? = null,
    depth: u16 = 0,

    pub fn isFeed(): bool {
        return _.xml_url != null;
    }
}

struct TagSpan {
    content_start: usize,
    content_end: usize,
    next_pos: usize,
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
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

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isAsciiSpace(s[start])) {
        start += 1;
    }
    while (end > start && isAsciiSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
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

fn indexOfFrom(haystack: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0) {
        return if (start <= haystack.len) start else null;
    }
    if (start + needle.len > haystack.len) {
        return null;
    }
    let i: usize = start;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        if (startsWithAt(haystack, i, needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn findTagClose(xml: []const u8, start: usize): usize? {
    let i: usize = start;
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

fn isTagNameTerminator(c: u8): bool {
    return isAsciiSpace(c) || c == '>' || c == '/';
}

fn matchesOpenTag(xml: []const u8, lt_pos: usize, tag: []const u8): bool {
    const name_start = lt_pos + 1;
    if (name_start + tag.len > xml.len) {
        return false;
    }
    if (!eqIgnoreCaseAscii(xml[name_start .. name_start + tag.len], tag)) {
        return false;
    }
    const after = name_start + tag.len;
    if (after >= xml.len) {
        return false;
    }
    return isTagNameTerminator(xml[after]);
}

fn matchesCloseTag(xml: []const u8, lt_pos: usize, tag: []const u8): bool {
    if (lt_pos + 2 + tag.len > xml.len) {
        return false;
    }
    if (xml[lt_pos + 1] != '/') {
        return false;
    }
    const name_start = lt_pos + 2;
    if (!eqIgnoreCaseAscii(xml[name_start .. name_start + tag.len], tag)) {
        return false;
    }
    const after = name_start + tag.len;
    if (after >= xml.len) {
        return false;
    }
    return isAsciiSpace(xml[after]) || xml[after] == '>';
}

fn isSelfClosing(xml: []const u8, lt_pos: usize, gt_pos: usize): bool {
    let k: usize = gt_pos;
    while (k > lt_pos + 1 && isAsciiSpace(xml[k - 1])) {
        k -= 1;
    }
    return k > lt_pos + 1 && xml[k - 1] == '/';
}

fn stripCdata(raw: []const u8): []const u8 {
    const trimmed = trimAscii(raw);
    const prefix = "<![CDATA[";
    const suffix = "]]>";
    if (trimmed.len >= prefix.len + suffix.len &&
        startsWithAt(trimmed, 0, prefix) &&
        startsWithAt(trimmed, trimmed.len - suffix.len, suffix)) {
        return trimmed[prefix.len .. trimmed.len - suffix.len];
    }
    return trimmed;
}

fn findTagSpan(xml: []const u8, start: usize, tag: []const u8): TagSpan? {
    let i: usize = start;
    while (i < xml.len) {
        if (xml[i] != '<') {
            i += 1;
            continue;
        }
        if (startsWithAt(xml, i, "<!--")) {
            if (ec := indexOfFrom(xml, i + 4, "-->")) {
                i = ec + 3;
                continue;
            }
            return null;
        }
        if (startsWithAt(xml, i, "<![CDATA[")) {
            if (ed := indexOfFrom(xml, i + 9, "]]>")) {
                i = ed + 3;
                continue;
            }
            return null;
        }
        if (startsWithAt(xml, i, "<?")) {
            if (ep := indexOfFrom(xml, i + 2, "?>")) {
                i = ep + 2;
                continue;
            }
            return null;
        }
        if (matchesOpenTag(xml, i, tag)) {
            if (open_gt := findTagClose(xml, i + 1 + tag.len)) {
                if (isSelfClosing(xml, i, open_gt)) {
                    return TagSpan{
                        content_start: open_gt + 1,
                        content_end: open_gt + 1,
                        next_pos: open_gt + 1,
                    };
                }
                const content_start = open_gt + 1;
                let j: usize = content_start;
                let depth: usize = 1;
                while (j < xml.len) {
                    if (xml[j] != '<') {
                        j += 1;
                        continue;
                    }
                    if (startsWithAt(xml, j, "<!--")) {
                        if (ec := indexOfFrom(xml, j + 4, "-->")) {
                            j = ec + 3;
                            continue;
                        }
                        return null;
                    }
                    if (startsWithAt(xml, j, "<![CDATA[")) {
                        if (ed := indexOfFrom(xml, j + 9, "]]>")) {
                            j = ed + 3;
                            continue;
                        }
                        return null;
                    }
                    if (matchesCloseTag(xml, j, tag)) {
                        if (close_gt := findTagClose(xml, j + 2 + tag.len)) {
                            depth -= 1;
                            if (depth == 0) {
                                return TagSpan{
                                    content_start: content_start,
                                    content_end: j,
                                    next_pos: close_gt + 1,
                                };
                            }
                            j = close_gt + 1;
                            continue;
                        }
                        return null;
                    }
                    if (matchesOpenTag(xml, j, tag)) {
                        if (nested_gt := findTagClose(xml, j + 1 + tag.len)) {
                            if (!isSelfClosing(xml, j, nested_gt)) {
                                depth += 1;
                            }
                            j = nested_gt + 1;
                            continue;
                        }
                        return null;
                    }
                    j += 1;
                }
                return null;
            }
            return null;
        }
        i += 1;
    }
    return null;
}

fn extractDirectTag(xml: []const u8, tag: []const u8): ([]const u8)? {
    if (span := findTagSpan(xml, 0, tag)) {
        return stripCdata(xml[span.content_start..span.content_end]);
    }
    return null;
}

/// 快速识别输入是否为合法 OPML 2.0 文档（跳过 BOM/XML 声明/注释，校验 `<opml` + `<body>`）
pub fn isOpmlDocument(xml: []const u8): bool {
    let i: usize = 0;
    if (xml.len >= 3 && xml[0] == 0xEF && xml[1] == 0xBB && xml[2] == 0xBF) {
        i = 3;
    }
    let opml_start: usize? = null;
    while (i < xml.len) {
        if (xml[i] != '<') {
            i += 1;
            continue;
        }
        if (startsWithAt(xml, i, "<!--")) {
            if (ec := indexOfFrom(xml, i + 4, "-->")) {
                i = ec + 3;
                continue;
            }
            return false;
        }
        if (startsWithAt(xml, i, "<?")) {
            if (ep := indexOfFrom(xml, i + 2, "?>")) {
                i = ep + 2;
                continue;
            }
            return false;
        }
        if (matchesOpenTag(xml, i, "opml")) {
            if (gt := findTagClose(xml, i + 5)) {
                opml_start = gt + 1;
                break;
            }
            return false;
        }
        if (i + 1 < xml.len && xml[i + 1] != '!') {
            return false;
        }
        i += 1;
    }
    if (start := opml_start) {
        if (body := findTagSpan(xml, start, "body")) {
            * = body;
            return true;
        }
    }
    return false;
}

/// 提取 OPML `<head>` 元数据（title / dateCreated / ownerName）
pub fn parseOpmlHead(xml: []const u8): OpmlHead? {
    if (head_span := findTagSpan(xml, 0, "head")) {
        const head_xml = xml[head_span.content_start..head_span.content_end];
        return OpmlHead{
            title: extractDirectTag(head_xml, "title"),
            date_created: extractDirectTag(head_xml, "dateCreated"),
            owner_name: extractDirectTag(head_xml, "ownerName"),
        };
    }
    return null;
}

/// 从 `<outline ...>` 属性片段中解析各标准属性（支持单双引号，零堆分配）
fn parseOutlineAttrs(attrs: []const u8, depth: u16): OpmlOutline {
    let out = OpmlOutline{ depth: depth };
    let i: usize = 0;
    while (i < attrs.len) {
        // 跳过空白与末尾 `/`
        while (i < attrs.len && (isAsciiSpace(attrs[i]) || attrs[i] == '/')) {
            i += 1;
        }
        if (i >= attrs.len) {
            break;
        }
        const name_start = i;
        while (i < attrs.len && !isAsciiSpace(attrs[i]) && attrs[i] != '=' && attrs[i] != '/') {
            i += 1;
        }
        const name = attrs[name_start..i];
        while (i < attrs.len && isAsciiSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || attrs[i] != '=') {
            continue;
        }
        i += 1; // skip '='
        while (i < attrs.len && isAsciiSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) {
            break;
        }
        let val: []const u8 = "";
        if (attrs[i] == '"' || attrs[i] == '\'') {
            const q = attrs[i];
            i += 1;
            const val_start = i;
            while (i < attrs.len && attrs[i] != q) {
                i += 1;
            }
            val = attrs[val_start..i];
            if (i < attrs.len && attrs[i] == q) {
                i += 1;
            }
        } else {
            const val_start = i;
            while (i < attrs.len && !isAsciiSpace(attrs[i]) && attrs[i] != '/') {
                i += 1;
            }
            val = attrs[val_start..i];
        }

        if (eqIgnoreCaseAscii(name, "text")) {
            out.text = val;
        } else if (eqIgnoreCaseAscii(name, "title")) {
            out.title = val;
        } else if (eqIgnoreCaseAscii(name, "type")) {
            out.kind = val;
        } else if (eqIgnoreCaseAscii(name, "xmlUrl")) {
            out.xml_url = val;
        } else if (eqIgnoreCaseAscii(name, "htmlUrl")) {
            out.html_url = val;
        }
    }
    return out;
}

/// 零堆分配 OPML 2.0 `<outline>` 流式迭代器，精确维护嵌套深度 `depth`
pub struct OpmlOutlineIter {
    body: []const u8,
    pos: usize = 0,
    depth: u16 = 0,

    pub static fn init(xml: []const u8): OpmlOutlineIter {
        if (body_span := findTagSpan(xml, 0, "body")) {
            return OpmlOutlineIter{
                body: xml[body_span.content_start..body_span.content_end],
                pos: 0,
                depth: 0,
            };
        }
        return OpmlOutlineIter{
            body: "",
            pos: 0,
            depth: 0,
        };
    }

    pub fn next(*_): OpmlOutline? {
        while (_.pos < _.body.len) {
            const i = _.pos;
            if (_.body[i] != '<') {
                _.pos += 1;
                continue;
            }
            if (startsWithAt(_.body, i, "<!--")) {
                if (ec := indexOfFrom(_.body, i + 4, "-->")) {
                    _.pos = ec + 3;
                    continue;
                }
                _.pos = _.body.len;
                return null;
            }
            if (startsWithAt(_.body, i, "<![CDATA[")) {
                if (ed := indexOfFrom(_.body, i + 9, "]]>")) {
                    _.pos = ed + 3;
                    continue;
                }
                _.pos = _.body.len;
                return null;
            }
            if (startsWithAt(_.body, i, "<?")) {
                if (ep := indexOfFrom(_.body, i + 2, "?>")) {
                    _.pos = ep + 2;
                    continue;
                }
                _.pos = _.body.len;
                return null;
            }
            if (matchesCloseTag(_.body, i, "outline")) {
                if (_.depth > 0) {
                    _.depth -= 1;
                }
                if (gt := findTagClose(_.body, i + 9)) {
                    _.pos = gt + 1;
                    continue;
                }
                _.pos = _.body.len;
                return null;
            }
            if (matchesOpenTag(_.body, i, "outline")) {
                if (gt := findTagClose(_.body, i + 8)) {
                    const cur_depth = _.depth;
                    const self_closing = isSelfClosing(_.body, i, gt);
                    const attr_end = if (self_closing && gt > i + 8) gt - 1 else gt;
                    const attrs = _.body[i + 8 .. attr_end];
                    _.pos = gt + 1;
                    if (!self_closing && _.depth < 65535) {
                        _.depth += 1;
                    }
                    return parseOutlineAttrs(attrs, cur_depth);
                }
                _.pos = _.body.len;
                return null;
            }
            _.pos += 1;
        }
        return null;
    }
}

fn appendRaw(out: []u8, pos: *usize, s: []const u8): bool {
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

fn appendByte(out: []u8, pos: *usize, c: u8): bool {
    if (pos.* >= out.len) {
        return false;
    }
    out[pos.*] = c;
    pos.* += 1;
    return true;
}

fn appendXmlEscaped(out: []u8, pos: *usize, s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        const ok = switch (c) {
            '&' => appendRaw(out, pos, "&amp;"),
            '<' => appendRaw(out, pos, "&lt;"),
            '>' => appendRaw(out, pos, "&gt;"),
            '"' => appendRaw(out, pos, "&quot;"),
            '\'' => appendRaw(out, pos, "&apos;"),
            else => appendByte(out, pos, c),
        };
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 规范化 XML 属性转义并序列化为 `<outline type="rss" .../>`，缓冲不足返回 `null`
pub fn formatOpmlFeedOutline(
    text: []const u8,
    xml_url: []const u8,
    html_url: ([]const u8)?,
    out: []u8,
): ([]const u8)? {
    let pos: usize = 0;
    if (!appendRaw(out, &pos, "<outline type=\"rss\" text=\"")) { return null; }
    if (!appendXmlEscaped(out, &pos, text)) { return null; }
    if (!appendRaw(out, &pos, "\" xmlUrl=\"")) { return null; }
    if (!appendXmlEscaped(out, &pos, xml_url)) { return null; }
    if (!appendRaw(out, &pos, "\"")) { return null; }
    if (h := html_url) {
        if (!appendRaw(out, &pos, " htmlUrl=\"")) { return null; }
        if (!appendXmlEscaped(out, &pos, h)) { return null; }
        if (!appendRaw(out, &pos, "\"")) { return null; }
    }
    if (!appendRaw(out, &pos, "/>")) { return null; }
    return out[0..pos];
}
