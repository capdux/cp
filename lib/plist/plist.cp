// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Apple XML Plist Parser & Writer)

/// Apple XML Property List 值类型枚举
pub enum PlistValueKind: u8 {
    string = 0,
    integer = 1,
    real = 2,
    bool_true = 3,
    bool_false = 4,
    date = 5,
    data = 6,
    dict = 7,
    array = 8,
}

/// 顶层 `<dict>` 键值对条目（零拷贝切片指向原始 XML）
pub struct PlistDictEntry {
    key: []const u8,
    kind: PlistValueKind,
    raw_value: []const u8,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isTagBoundary(c: u8): bool {
    return isSpace(c) || c == '>' || c == '/';
}

fn stripBom(s: []const u8): []const u8 {
    if (s.len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
        return s[3..];
    }
    return s;
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

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn startsWithAt(s: []const u8, pos: usize, needle: []const u8): bool {
    if (pos + needle.len > s.len) return false;
    let i: usize = 0;
    while (i < needle.len) {
        if (s[pos + i] != needle[i]) return false;
        i += 1;
    }
    return true;
}

fn findSubstr(s: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0) return if (start <= s.len) start else null;
    if (start + needle.len > s.len) return null;
    let i = start;
    const limit = s.len - needle.len;
    while (i <= limit) {
        if (startsWithAt(s, i, needle)) return i;
        i += 1;
    }
    return null;
}

fn findTagClose(xml: []const u8, open_pos: usize): usize? {
    let i = open_pos;
    let quote: u8 = 0;
    while (i < xml.len) {
        const c = xml[i];
        if (quote != 0) {
            if (c == quote) quote = 0;
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

fn skipTrivia(xml: []const u8, start: usize): usize {
    let i = start;
    while (i < xml.len) {
        if (isSpace(xml[i])) {
            i += 1;
            continue;
        }
        if (startsWithAt(xml, i, "<!--")) {
            if (end_c := findSubstr(xml, i + 4, "-->")) {
                i = end_c + 3;
                continue;
            }
            return xml.len;
        }
        if (startsWithAt(xml, i, "<?")) {
            if (end_pi := findSubstr(xml, i + 2, "?>")) {
                i = end_pi + 2;
                continue;
            }
            return xml.len;
        }
        if (startsWithAt(xml, i, "<!DOCTYPE")) {
            if (gt := findTagClose(xml, i)) {
                i = gt + 1;
                continue;
            }
            return xml.len;
        }
        break;
    }
    return i;
}

fn isOpenTagAt(xml: []const u8, pos: usize, tag: []const u8): bool {
    if (pos + 1 + tag.len >= xml.len) return false;
    if (xml[pos] != '<') return false;
    if (!startsWithAt(xml, pos + 1, tag)) return false;
    return isTagBoundary(xml[pos + 1 + tag.len]);
}

fn isCloseTagAt(xml: []const u8, pos: usize, tag: []const u8): bool {
    if (pos + 2 + tag.len >= xml.len) return false;
    if (xml[pos] != '<' || xml[pos + 1] != '/') return false;
    if (!startsWithAt(xml, pos + 2, tag)) return false;
    const after = xml[pos + 2 + tag.len];
    return after == '>' || isSpace(after);
}

struct BalancedSpan {
    inner_start: usize,
    inner_end: usize,
    after_close: usize,
}

/// 从 `open_pos` 处的 `<tag...>` 起，深度平衡匹配到对应的 `</tag>`（支持嵌套同名 `<dict>`/`<array>`）
fn matchBalancedElement(xml: []const u8, open_pos: usize, tag: []const u8): BalancedSpan? {
    const gt = findTagClose(xml, open_pos) ?? return null;
    if (gt > open_pos && xml[gt - 1] == '/') {
        return BalancedSpan {
            inner_start: gt,
            inner_end: gt,
            after_close: gt + 1,
        };
    }

    let depth: usize = 1;
    let i = gt + 1;
    while (i < xml.len) {
        if (xml[i] == '<') {
            if (startsWithAt(xml, i, "<!--")) {
                const end_c = findSubstr(xml, i + 4, "-->") ?? return null;
                i = end_c + 3;
                continue;
            }
            if (isCloseTagAt(xml, i, tag)) {
                const close_gt = findTagClose(xml, i) ?? return null;
                depth -= 1;
                if (depth == 0) {
                    return BalancedSpan {
                        inner_start: gt + 1,
                        inner_end: i,
                        after_close: close_gt + 1,
                    };
                }
                i = close_gt + 1;
                continue;
            }
            if (isOpenTagAt(xml, i, tag)) {
                const nested_gt = findTagClose(xml, i) ?? return null;
                if (!(nested_gt > i && xml[nested_gt - 1] == '/')) {
                    depth += 1;
                }
                i = nested_gt + 1;
                continue;
            }
        }
        i += 1;
    }
    return null;
}

fn extractPlistInner(xml: []const u8): ([]const u8)? {
    const s = stripBom(xml);
    const pos = skipTrivia(s, 0);
    if (!isOpenTagAt(s, pos, "plist")) {
        return null;
    }
    const gt = findTagClose(s, pos) ?? return null;
    if (gt > pos && s[gt - 1] == '/') {
        return null;
    }
    const span = matchBalancedElement(s, pos, "plist") ?? return null;
    return s[span.inner_start..span.inner_end];
}

/// 快速校验是否为合法 Apple XML Property List（跳过 UTF-8 BOM / XML 声明 / DOCTYPE）
pub fn isXmlPlist(xml: []const u8): bool {
    return extractPlistInner(xml) != null;
}

fn kindFromTagName(tag: []const u8): PlistValueKind? {
    if (strEq(tag, "string")) return PlistValueKind.string;
    if (strEq(tag, "integer")) return PlistValueKind.integer;
    if (strEq(tag, "real")) return PlistValueKind.real;
    if (strEq(tag, "true")) return PlistValueKind.bool_true;
    if (strEq(tag, "false")) return PlistValueKind.bool_false;
    if (strEq(tag, "date")) return PlistValueKind.date;
    if (strEq(tag, "data")) return PlistValueKind.data;
    if (strEq(tag, "dict")) return PlistValueKind.dict;
    if (strEq(tag, "array")) return PlistValueKind.array;
    return null;
}

/// 零堆分配 Apple XML Plist 顶层 `<dict>` 键值迭代器（自动跳过嵌套 `<dict>`/`<array>` 内部键）
pub struct PlistDictIter {
    body: []const u8,
    pos: usize = 0,

    pub static fn init(xml: []const u8): PlistDictIter {
        const inner_opt = extractPlistInner(xml);
        if (inner_opt == null) {
            return PlistDictIter { body: "", pos: 0 };
        }
        const inner = inner_opt.?;
        const p = skipTrivia(inner, 0);
        if (!isOpenTagAt(inner, p, "dict")) {
            return PlistDictIter { body: "", pos: 0 };
        }
        const gt_opt = findTagClose(inner, p);
        if (gt_opt == null) {
            return PlistDictIter { body: "", pos: 0 };
        }
        const gt = gt_opt.?;
        if (gt > p && inner[gt - 1] == '/') {
            return PlistDictIter { body: "", pos: 0 };
        }
        const span_opt = matchBalancedElement(inner, p, "dict");
        if (span_opt == null) {
            return PlistDictIter { body: "", pos: 0 };
        }
        const span = span_opt.?;
        return PlistDictIter {
            body: inner[span.inner_start..span.inner_end],
            pos: 0,
        };
    }

    /// 读取顶层 `<dict>` 下一组 `<key>` 与对应值；遍历结束或遇到非法结构返回 `null`
    pub fn next(*_): PlistDictEntry? {
        _.pos = skipTrivia(_.body, _.pos);
        if (_.pos >= _.body.len) {
            return null;
        }
        if (!isOpenTagAt(_.body, _.pos, "key")) {
            _.pos = _.body.len;
            return null;
        }
        const key_span_opt = matchBalancedElement(_.body, _.pos, "key");
        if (key_span_opt == null) {
            _.pos = _.body.len;
            return null;
        }
        const key_span = key_span_opt.?;
        const key = _.body[key_span.inner_start..key_span.inner_end];
        _.pos = skipTrivia(_.body, key_span.after_close);

        if (_.pos >= _.body.len || _.body[_.pos] != '<' || _.pos + 1 >= _.body.len || _.body[_.pos + 1] == '/') {
            _.pos = _.body.len;
            return null;
        }

        // Read value tag name
        const name_start = _.pos + 1;
        let name_end = name_start;
        while (name_end < _.body.len && !isTagBoundary(_.body[name_end])) {
            name_end += 1;
        }
        const tag_name = _.body[name_start..name_end];
        const kind_opt = kindFromTagName(tag_name);
        if (kind_opt == null) {
            _.pos = _.body.len;
            return null;
        }
        const kind = kind_opt.?;

        const val_span_opt = matchBalancedElement(_.body, _.pos, tag_name);
        if (val_span_opt == null) {
            _.pos = _.body.len;
            return null;
        }
        const val_span = val_span_opt.?;
        _.pos = val_span.after_close;

        let raw_val: []const u8 = _.body[val_span.inner_start..val_span.inner_end];
        if (kind == PlistValueKind.bool_true || kind == PlistValueKind.bool_false) {
            raw_val = "";
        } else if (kind == PlistValueKind.integer || kind == PlistValueKind.real || kind == PlistValueKind.date) {
            raw_val = trimAscii(raw_val);
        }

        return PlistDictEntry {
            key: key,
            kind: kind,
            raw_value: raw_val,
        };
    }
}

/// 按键查找顶层 `<string>` 值
pub fn findPlistString(xml: []const u8, key: []const u8): ([]const u8)? {
    let it = PlistDictIter.init(xml);
    while (entry := it.next()) {
        if (strEq(entry.key, key)) {
            if (entry.kind == PlistValueKind.string) {
                return entry.raw_value;
            }
            return null;
        }
    }
    return null;
}

fn parseI64(s: []const u8): i64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) return null;

    let acc: u64 = 0;
    const limit: u64 = if (neg) 9223372036854775808 else 9223372036854775807;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const d: u64 = (c - '0') as u64;
        if (acc > (limit - d) / 10) return null;
        acc = acc * 10 + d;
        i += 1;
    }
    if (neg) {
        if (acc == 9223372036854775808) {
            return -9223372036854775807 - 1;
        }
        return -(acc as i64);
    }
    return acc as i64;
}

/// 按键查找顶层 `<integer>` 值并解析为 `i64`
pub fn findPlistInt(xml: []const u8, key: []const u8): i64? {
    let it = PlistDictIter.init(xml);
    while (entry := it.next()) {
        if (strEq(entry.key, key)) {
            if (entry.kind == PlistValueKind.integer) {
                return parseI64(entry.raw_value);
            }
            return null;
        }
    }
    return null;
}

/// 按键查找顶层 `<true/>` / `<false/>` 布尔值
pub fn findPlistBool(xml: []const u8, key: []const u8): bool? {
    let it = PlistDictIter.init(xml);
    while (entry := it.next()) {
        if (strEq(entry.key, key)) {
            if (entry.kind == PlistValueKind.bool_true) return true;
            if (entry.kind == PlistValueKind.bool_false) return false;
            return null;
        }
    }
    return null;
}

fn writeRaw(out: []u8, pos: *usize, s: []const u8): bool {
    if (pos.* + s.len > out.len) return false;
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

/// 规范化 XML 实体转义并输出 `<key>...</key>\n<string>...</string>\n`；输出缓冲不足返回 `null`
pub fn formatPlistStringEntry(key: []const u8, value: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    if (!writeRaw(out, &pos, "<key>")) return null;
    if (!writeXmlEscaped(out, &pos, key)) return null;
    if (!writeRaw(out, &pos, "</key>\n<string>")) return null;
    if (!writeXmlEscaped(out, &pos, value)) return null;
    if (!writeRaw(out, &pos, "</string>\n")) return null;
    return out[0..pos];
}
