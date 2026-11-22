// Origin: Pure-CP Stdlib (RFC 4287 Atom 1.0 Feed Parser & Writer) | Created: 2026-11-22

/// RFC 4287 Atom 1.0 Feed 顶层元数据（零拷贝切片指向原始 XML）
pub struct AtomFeed {
    id: []const u8,
    title: []const u8,
    updated: []const u8,
    link_href: ([]const u8)? = null,
    subtitle: ([]const u8)? = null,
}

/// RFC 4287 Atom 1.0 Entry 条目元数据（零拷贝切片指向原始 XML）
pub struct AtomEntry {
    id: []const u8,
    title: []const u8,
    updated: []const u8,
    link_href: ([]const u8)? = null,
    summary: ([]const u8)? = null,
    author_name: ([]const u8)? = null,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isTagBoundary(c: u8): bool {
    return isSpace(c) || c == '>' || c == '/';
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

fn stripCdata(raw: []const u8): []const u8 {
    const s = trimAscii(raw);
    const prefix = "<![CDATA[";
    const suffix = "]]>";
    if (s.len >= prefix.len + suffix.len && startsWithAt(s, 0, prefix) && startsWithAt(s, s.len - suffix.len, suffix)) {
        return s[prefix.len .. s.len - suffix.len];
    }
    return s;
}

/// 查找元素开始标签结束位置 `>`（忽略引号属性内部的 `>`）
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

fn isOpenTagAt(xml: []const u8, pos: usize, tag: []const u8): bool {
    if (pos + 1 + tag.len >= xml.len) {
        return false;
    }
    if (xml[pos] != '<') {
        return false;
    }
    if (!startsWithAt(xml, pos + 1, tag)) {
        return false;
    }
    return isTagBoundary(xml[pos + 1 + tag.len]);
}

fn isCloseTagAt(xml: []const u8, pos: usize, tag: []const u8): bool {
    if (pos + 2 + tag.len >= xml.len) {
        return false;
    }
    if (xml[pos] != '<' || xml[pos + 1] != '/') {
        return false;
    }
    if (!startsWithAt(xml, pos + 2, tag)) {
        return false;
    }
    const after = xml[pos + 2 + tag.len];
    return after == '>' || isSpace(after);
}

/// 在 XML 中查找 `<tag...>` 的起始下标，跳过注释、CDATA，且可选跳过 `<entry>...</entry>` 子树
fn findOpenTag(xml: []const u8, start: usize, tag: []const u8, skip_entries: bool): usize? {
    let i = start;
    while (i < xml.len) {
        if (xml[i] == '<') {
            if (startsWithAt(xml, i, "<!--")) {
                if (end_c := findSubstr(xml, i + 4, "-->")) {
                    i = end_c + 3;
                    continue;
                }
                return null;
            }
            if (startsWithAt(xml, i, "<![CDATA[")) {
                if (end_cd := findSubstr(xml, i + 9, "]]>")) {
                    i = end_cd + 3;
                    continue;
                }
                return null;
            }
            if (skip_entries && isOpenTagAt(xml, i, "entry")) {
                if (entry_open_end := findTagClose(xml, i)) {
                    if (entry_open_end > i && xml[entry_open_end - 1] == '/') {
                        i = entry_open_end + 1;
                        continue;
                    }
                    if (entry_close := findCloseTag(xml, entry_open_end + 1, "entry")) {
                        if (close_end := findTagClose(xml, entry_close)) {
                            i = close_end + 1;
                            continue;
                        }
                    }
                }
                return null;
            }
            if (isOpenTagAt(xml, i, tag)) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn findCloseTag(xml: []const u8, start: usize, tag: []const u8): usize? {
    let i = start;
    while (i < xml.len) {
        if (xml[i] == '<') {
            if (startsWithAt(xml, i, "<!--")) {
                if (end_c := findSubstr(xml, i + 4, "-->")) {
                    i = end_c + 3;
                    continue;
                }
                return null;
            }
            if (startsWithAt(xml, i, "<![CDATA[")) {
                if (end_cd := findSubstr(xml, i + 9, "]]>")) {
                    i = end_cd + 3;
                    continue;
                }
                return null;
            }
            if (isCloseTagAt(xml, i, tag)) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn extractTagBody(xml: []const u8, tag: []const u8, skip_entries: bool): ([]const u8)? {
    let search_pos: usize = 0;
    while (open_pos := findOpenTag(xml, search_pos, tag, skip_entries)) {
        const gt_pos = findTagClose(xml, open_pos) ?? return null;
        if (gt_pos > open_pos && xml[gt_pos - 1] == '/') {
            search_pos = gt_pos + 1;
            continue;
        }
        const close_pos = findCloseTag(xml, gt_pos + 1, tag) ?? return null;
        return stripCdata(xml[gt_pos + 1 .. close_pos]);
    }
    return null;
}

fn extractAttr(attrs: []const u8, name: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < attrs.len) {
        while (i < attrs.len && isSpace(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || attrs[i] == '/' || attrs[i] == '>') {
            break;
        }
        const key_start = i;
        while (i < attrs.len && !isSpace(attrs[i]) && attrs[i] != '=' && attrs[i] != '/' && attrs[i] != '>') {
            i += 1;
        }
        const key = attrs[key_start..i];
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
            break;
        }
        const q = attrs[i];
        if (q != '"' && q != '\'') {
            while (i < attrs.len && !isSpace(attrs[i]) && attrs[i] != '/' && attrs[i] != '>') {
                i += 1;
            }
            continue;
        }
        i += 1;
        const val_start = i;
        while (i < attrs.len && attrs[i] != q) {
            i += 1;
        }
        if (i >= attrs.len) {
            return null;
        }
        const val = attrs[val_start..i];
        i += 1;

        if (key.len == name.len && startsWithAt(key, 0, name)) {
            return val;
        }
    }
    return null;
}

fn extractLinkHref(xml: []const u8, skip_entries: bool): ([]const u8)? {
    let fallback: ([]const u8)? = null;
    let search_pos: usize = 0;
    while (open_pos := findOpenTag(xml, search_pos, "link", skip_entries)) {
        const gt_pos = findTagClose(xml, open_pos) ?? break;
        const attrs_start = open_pos + 5; // len("<link")
        const attrs_end = if (gt_pos > attrs_start && xml[gt_pos - 1] == '/') gt_pos - 1 else gt_pos;
        if (attrs_start < attrs_end) {
            const attrs = xml[attrs_start..attrs_end];
            if (href := extractAttr(attrs, "href")) {
                if (rel := extractAttr(attrs, "rel")) {
                    if (rel.len == 9 && startsWithAt(rel, 0, "alternate")) {
                        return href;
                    }
                    if (fallback == null) {
                        fallback = href;
                    }
                } else {
                    // RFC 4287 §4.2.7.2: missing rel defaults to "alternate"
                    return href;
                }
            }
        }
        search_pos = gt_pos + 1;
    }
    return fallback;
}

fn extractFeedInner(xml: []const u8): ([]const u8)? {
    const open_pos = findOpenTag(xml, 0, "feed", false) ?? return null;
    const gt_pos = findTagClose(xml, open_pos) ?? return null;
    if (gt_pos > open_pos && xml[gt_pos - 1] == '/') {
        return null;
    }
    const close_pos = findCloseTag(xml, gt_pos + 1, "feed") ?? return null;
    return xml[gt_pos + 1 .. close_pos];
}

/// 快速校验是否为合法 RFC 4287 `<feed>...</feed>` 文档结构
pub fn isAtomFeed(xml: []const u8): bool {
    return extractFeedInner(xml) != null;
}

/// 解析 `<feed>` 顶层元数据（自动跳过 `<entry>...</entry>` 同名子标签，支持 CDATA 与 `<link>` 属性提取）
pub fn parseAtomFeed(xml: []const u8): AtomFeed? {
    const inner = extractFeedInner(xml) ?? return null;
    const id = extractTagBody(inner, "id", true) ?? return null;
    const title = extractTagBody(inner, "title", true) ?? return null;
    const updated = extractTagBody(inner, "updated", true) ?? return null;
    const link_href = extractLinkHref(inner, true);
    const subtitle = extractTagBody(inner, "subtitle", true);

    return AtomFeed {
        id: id,
        title: title,
        updated: updated,
        link_href: link_href,
        subtitle: subtitle,
    };
}

/// 零堆分配 `<entry>...</entry>` 流式迭代器
pub struct AtomEntryIter {
    xml: []const u8,
    pos: usize = 0,

    pub static fn init(xml: []const u8): AtomEntryIter {
        return AtomEntryIter {
            xml: xml,
            pos: 0,
        };
    }

    /// 读取下一条合法 `<entry>`；遍历结束返回 `null`
    pub fn next(*_): AtomEntry? {
        while (open_pos := findOpenTag(_.xml, _.pos, "entry", false)) {
            const gt_opt = findTagClose(_.xml, open_pos);
            if (gt_opt == null) {
                _.pos = _.xml.len;
                return null;
            }
            const gt_pos = gt_opt.?;
            if (gt_pos > open_pos && _.xml[gt_pos - 1] == '/') {
                _.pos = gt_pos + 1;
                continue;
            }
            const close_opt = findCloseTag(_.xml, gt_pos + 1, "entry");
            if (close_opt == null) {
                _.pos = _.xml.len;
                return null;
            }
            const close_pos = close_opt.?;
            const close_end_opt = findTagClose(_.xml, close_pos);
            if (close_end_opt == null) {
                _.pos = _.xml.len;
                return null;
            }
            const close_end = close_end_opt.?;
            _.pos = close_end + 1;

            const body = _.xml[gt_pos + 1 .. close_pos];
            const id = extractTagBody(body, "id", false) ?? continue;
            const title = extractTagBody(body, "title", false) ?? continue;
            const updated = extractTagBody(body, "updated", false) ?? continue;
            const link_href = extractLinkHref(body, false);
            const summary = extractTagBody(body, "summary", false);

            let author_name: ([]const u8)? = null;
            if (author_body := extractTagBody(body, "author", false)) {
                author_name = extractTagBody(author_body, "name", false);
            }

            return AtomEntry {
                id: id,
                title: title,
                updated: updated,
                link_href: link_href,
                summary: summary,
                author_name: author_name,
            };
        }
        return null;
    }
}

fn writeRaw(out: []u8, pos: *usize, s: []const u8): bool {
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

fn writeXmlEscaped(out: []u8, pos: *usize, s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '&') {
            if (!writeRaw(out, pos, "&amp;")) return false;
        } else if (c == '<') {
            if (!writeRaw(out, pos, "&lt;")) return false;
        } else if (c == '>') {
            if (!writeRaw(out, pos, "&gt;")) return false;
        } else if (c == '"') {
            if (!writeRaw(out, pos, "&quot;")) return false;
        } else if (c == '\'') {
            if (!writeRaw(out, pos, "&apos;")) return false;
        } else {
            if (pos.* >= out.len) return false;
            out[pos.*] = c;
            pos.* += 1;
        }
        i += 1;
    }
    return true;
}

/// 规范化 XML 转义并序列化 `<entry>...</entry>`；输出缓冲不足返回 `null`
pub fn formatAtomEntry(entry: AtomEntry, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (!writeRaw(out, &pos, "<entry><id>")) return null;
    if (!writeXmlEscaped(out, &pos, entry.id)) return null;
    if (!writeRaw(out, &pos, "</id><title>")) return null;
    if (!writeXmlEscaped(out, &pos, entry.title)) return null;
    if (!writeRaw(out, &pos, "</title><updated>")) return null;
    if (!writeXmlEscaped(out, &pos, entry.updated)) return null;
    if (!writeRaw(out, &pos, "</updated>")) return null;

    if (href := entry.link_href) {
        if (!writeRaw(out, &pos, "<link rel=\"alternate\" href=\"")) return null;
        if (!writeXmlEscaped(out, &pos, href)) return null;
        if (!writeRaw(out, &pos, "\"/>")) return null;
    }
    if (author := entry.author_name) {
        if (!writeRaw(out, &pos, "<author><name>")) return null;
        if (!writeXmlEscaped(out, &pos, author)) return null;
        if (!writeRaw(out, &pos, "</name></author>")) return null;
    }
    if (summary := entry.summary) {
        if (!writeRaw(out, &pos, "<summary>")) return null;
        if (!writeXmlEscaped(out, &pos, summary)) return null;
        if (!writeRaw(out, &pos, "</summary>")) return null;
    }
    if (!writeRaw(out, &pos, "</entry>")) return null;
    return out[0..pos];
}
