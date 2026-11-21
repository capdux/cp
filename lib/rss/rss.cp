// Source: Absorbed from /data/dev/bun/src/ | Synced: 2026-11-21T17:37:17Z (Zero-Alloc RSS 2.0 Feed Parser & Writer)

/// RSS 2.0 频道级元数据（零堆分配切片视图）
pub struct RssChannel {
    title: []const u8 = "",
    link: []const u8 = "",
    description: []const u8 = "",
    language: ([]const u8)? = null,
    last_build_date: ([]const u8)? = null,
}

/// RSS 2.0 单条资讯条目（零堆分配切片视图）
pub struct RssItem {
    title: []const u8 = "",
    link: []const u8 = "",
    description: []const u8 = "",
    guid: ([]const u8)? = null,
    pub_date: ([]const u8)? = null,
    author: ([]const u8)? = null,
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

/// 查找开标签结束的 `>`（跳过引号内属性值），返回 `>` 索引
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

/// 剥离首尾空白以及 `<![CDATA[ ... ]]>` 包裹
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

/// 在 `[start..xml.len]` 中查找指定 `<tag ...>...</tag>` 的跨度（跳过注释与 CDATA）
fn findTagSpan(xml: []const u8, start: usize, tag: []const u8): TagSpan? {
    let i: usize = start;
    while (i < xml.len) {
        if (xml[i] != '<') {
            i += 1;
            continue;
        }
        // 跳过 XML 注释
        if (startsWithAt(xml, i, "<!--")) {
            if (end_comment := indexOfFrom(xml, i + 4, "-->")) {
                i = end_comment + 3;
                continue;
            }
            return null;
        }
        // 跳过 CDATA
        if (startsWithAt(xml, i, "<![CDATA[")) {
            if (end_cdata := indexOfFrom(xml, i + 9, "]]>")) {
                i = end_cdata + 3;
                continue;
            }
            return null;
        }
        // 跳过处理指令 <?...?>
        if (startsWithAt(xml, i, "<?")) {
            if (end_pi := indexOfFrom(xml, i + 2, "?>")) {
                i = end_pi + 2;
                continue;
            }
            return null;
        }

        if (matchesOpenTag(xml, i, tag)) {
            if (open_gt := findTagClose(xml, i + 1 + tag.len)) {
                // 检查自闭合标签 <tag ... />
                let k: usize = open_gt;
                while (k > i + 1 && isAsciiSpace(xml[k - 1])) {
                    k -= 1;
                }
                if (k > i + 1 && xml[k - 1] == '/') {
                    return TagSpan{
                        content_start: open_gt + 1,
                        content_end: open_gt + 1,
                        next_pos: open_gt + 1,
                    };
                }

                // 查找配对闭合标签（允许简单嵌套与 CDATA 内出现伪闭合标签）
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
                            let nk: usize = nested_gt;
                            while (nk > j + 1 && isAsciiSpace(xml[nk - 1])) {
                                nk -= 1;
                            }
                            if (!(nk > j + 1 && xml[nk - 1] == '/')) {
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

/// 在限定片段中提取第一个指定子标签文本（剥离 CDATA 与首尾空白）
fn extractDirectTag(xml: []const u8, tag: []const u8): ([]const u8)? {
    if (span := findTagSpan(xml, 0, tag)) {
        return stripCdata(xml[span.content_start..span.content_end]);
    }
    return null;
}

/// 在 `<channel>` 内容中提取频道级字段，自动跳过 `<item>...</item>` 与 `<image>...</image>` 子树
fn extractChannelField(channel_body: []const u8, tag: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < channel_body.len) {
        if (channel_body[i] != '<') {
            i += 1;
            continue;
        }
        if (startsWithAt(channel_body, i, "<!--")) {
            if (ec := indexOfFrom(channel_body, i + 4, "-->")) {
                i = ec + 3;
                continue;
            }
            return null;
        }
        if (startsWithAt(channel_body, i, "<![CDATA[")) {
            if (ed := indexOfFrom(channel_body, i + 9, "]]>")) {
                i = ed + 3;
                continue;
            }
            return null;
        }
        if (startsWithAt(channel_body, i, "<?")) {
            if (ep := indexOfFrom(channel_body, i + 2, "?>")) {
                i = ep + 2;
                continue;
            }
            return null;
        }
        // 跳过 <item>...</item> 与 <image>...</image> 子树
        if (matchesOpenTag(channel_body, i, "item")) {
            if (item_span := findTagSpan(channel_body, i, "item")) {
                i = item_span.next_pos;
                continue;
            }
            return null;
        }
        if (matchesOpenTag(channel_body, i, "image")) {
            if (img_span := findTagSpan(channel_body, i, "image")) {
                i = img_span.next_pos;
                continue;
            }
            return null;
        }
        if (matchesOpenTag(channel_body, i, tag)) {
            if (span := findTagSpan(channel_body, i, tag)) {
                return stripCdata(channel_body[span.content_start..span.content_end]);
            }
            return null;
        }
        i += 1;
    }
    return null;
}

/// 快速识别输入是否为合法 RSS 2.0 Feed 结构（含 `<rss` 与 `<channel>`）
pub fn isRss2Feed(xml: []const u8): bool {
    let i: usize = 0;
    // 跳过 UTF-8 BOM
    if (xml.len >= 3 && xml[0] == 0xEF && xml[1] == 0xBB && xml[2] == 0xBF) {
        i = 3;
    }
    let rss_pos: usize? = null;
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
        if (matchesOpenTag(xml, i, "rss")) {
            if (gt := findTagClose(xml, i + 4)) {
                rss_pos = gt + 1;
                break;
            }
            return false;
        }
        // 遇到其他根元素则非 RSS 2.0
        if (i + 1 < xml.len && xml[i + 1] != '!') {
            return false;
        }
        i += 1;
    }
    if (start := rss_pos) {
        if (chan := findTagSpan(xml, start, "channel")) {
            * = chan;
            return true;
        }
    }
    return false;
}

/// 提取 RSS 2.0 `<channel>` 级元数据（自动跳过 `<item>` 与 `<image>` 内部同名标签，支持 CDATA）
pub fn parseRssChannel(xml: []const u8): RssChannel? {
    if (chan_span := findTagSpan(xml, 0, "channel")) {
        const body = xml[chan_span.content_start..chan_span.content_end];
        const title = extractChannelField(body, "title") ?? "";
        const link = extractChannelField(body, "link") ?? "";
        const description = extractChannelField(body, "description") ?? "";
        const language = extractChannelField(body, "language");
        const last_build_date = extractChannelField(body, "lastBuildDate");
        return RssChannel{
            title: title,
            link: link,
            description: description,
            language: language,
            last_build_date: last_build_date,
        };
    }
    return null;
}

/// 零堆分配 RSS 2.0 `<item>` 条目迭代器
pub struct RssItemIter {
    body: []const u8,
    pos: usize = 0,

    pub static fn init(xml: []const u8): RssItemIter {
        if (chan_span := findTagSpan(xml, 0, "channel")) {
            return RssItemIter{
                body: xml[chan_span.content_start..chan_span.content_end],
                pos: 0,
            };
        }
        return RssItemIter{
            body: xml,
            pos: 0,
        };
    }

    pub fn next(*_): RssItem? {
        if (_.pos >= _.body.len) {
            return null;
        }
        if (span := findTagSpan(_.body, _.pos, "item")) {
            _.pos = span.next_pos;
            const item_xml = _.body[span.content_start..span.content_end];
            const title = extractDirectTag(item_xml, "title") ?? "";
            const link = extractDirectTag(item_xml, "link") ?? "";
            const description = extractDirectTag(item_xml, "description") ?? "";
            const guid = extractDirectTag(item_xml, "guid");
            const pub_date = extractDirectTag(item_xml, "pubDate");
            const author = extractDirectTag(item_xml, "author");
            return RssItem{
                title: title,
                link: link,
                description: description,
                guid: guid,
                pub_date: pub_date,
                author: author,
            };
        }
        _.pos = _.body.len;
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

fn appendXmlElement(out: []u8, pos: *usize, tag: []const u8, val: []const u8): bool {
    if (!appendRaw(out, pos, "<")) { return false; }
    if (!appendRaw(out, pos, tag)) { return false; }
    if (!appendRaw(out, pos, ">")) { return false; }
    if (!appendXmlEscaped(out, pos, val)) { return false; }
    if (!appendRaw(out, pos, "</")) { return false; }
    if (!appendRaw(out, pos, tag)) { return false; }
    if (!appendRaw(out, pos, ">")) { return false; }
    return true;
}

/// 规范化 XML 转义并将 `RssItem` 序列化为 `<item>...</item>`，缓冲不足返回 `null`
pub fn formatRssItem(item: RssItem, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (!appendRaw(out, &pos, "<item>")) { return null; }
    if (!appendXmlElement(out, &pos, "title", item.title)) { return null; }
    if (!appendXmlElement(out, &pos, "link", item.link)) { return null; }
    if (!appendXmlElement(out, &pos, "description", item.description)) { return null; }
    if (g := item.guid) {
        if (!appendXmlElement(out, &pos, "guid", g)) { return null; }
    }
    if (pd := item.pub_date) {
        if (!appendXmlElement(out, &pos, "pubDate", pd)) { return null; }
    }
    if (au := item.author) {
        if (!appendXmlElement(out, &pos, "author", au)) { return null; }
    }
    if (!appendRaw(out, &pos, "</item>")) { return null; }
    return out[0..pos];
}
