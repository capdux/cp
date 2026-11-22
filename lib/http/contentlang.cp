// Source: Absorbed from /data/dev/bun/src/http/ | Synced: 2026-11-22T11:12:13Z (RFC 9110 Content-Language Parser)

fn isOws(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimOws(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isOws(s[start])) {
        start += 1;
    }
    while (end > start && isOws(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn isAlpha(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAlnum(c: u8): bool {
    return isAlpha(c) || isDigit(c);
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
}

fn isAllAlpha(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isAlpha(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isAllDigit(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
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

fn matchesLangOrPrefix(item: []const u8, target: []const u8): bool {
    if (item.len < target.len) {
        return false;
    }
    if (!eqlIgnoreCaseAscii(item[0..target.len], target)) {
        return false;
    }
    if (item.len == target.len) {
        return true;
    }
    return item[target.len] == '-';
}

/// BCP 47 语言标签核心结构拆解（主语种 / 文字脚本 / 国家地区）
export struct LanguageTagParts {
    primary: []const u8;
    script: ([]const u8)?;
    region: ([]const u8)?;
}

/// 校验基本 BCP 47 语法：首段 `2..=3` 字母，后续 `"-"` 分隔 `1..=8` 字母数字，拒绝空段与非法字符
export fn isValidBcp47Tag(tag: []const u8): bool {
    if (tag.len == 0) {
        return false;
    }

    let pos: usize = 0;
    let seg_idx: usize = 0;
    while (pos < tag.len) {
        const seg_start = pos;
        while (pos < tag.len && tag[pos] != '-') {
            const c = tag[pos];
            if (seg_idx == 0) {
                if (!isAlpha(c)) {
                    return false;
                }
            } else {
                if (!isAlnum(c)) {
                    return false;
                }
            }
            pos += 1;
        }

        const seg_len = pos - seg_start;
        if (seg_idx == 0) {
            if (seg_len < 2 || seg_len > 3) {
                return false;
            }
        } else {
            if (seg_len == 0 || seg_len > 8) {
                return false;
            }
        }

        if (pos < tag.len && tag[pos] == '-') {
            pos += 1;
            if (pos == tag.len) {
                return false;
            }
        }
        seg_idx += 1;
    }

    return seg_idx > 0;
}

/// 提取合法 BCP 47 语言标签中的 `primary`、`script`（4 字母）与 `region`（2 字母或 3 数字）
export fn parseLanguageTagParts(tag: []const u8): LanguageTagParts? {
    if (!isValidBcp47Tag(tag)) {
        return null;
    }

    let pos: usize = 0;
    while (pos < tag.len && tag[pos] != '-') {
        pos += 1;
    }
    const primary = tag[0..pos];
    let script: ([]const u8)? = null;
    let region: ([]const u8)? = null;

    if (pos < tag.len && tag[pos] == '-') {
        pos += 1;
    }

    while (pos < tag.len) {
        const seg_start = pos;
        while (pos < tag.len && tag[pos] != '-') {
            pos += 1;
        }
        const seg = tag[seg_start..pos];
        if (pos < tag.len && tag[pos] == '-') {
            pos += 1;
        }

        // Singleton extension / private-use prefix terminates script/region parsing
        if (seg.len == 1) {
            break;
        }

        if (script == null && region == null && seg.len == 4 && isAllAlpha(seg)) {
            script = seg;
            continue;
        }

        if (region == null && ((seg.len == 2 && isAllAlpha(seg)) || (seg.len == 3 && isAllDigit(seg)))) {
            region = seg;
            break;
        }

        // Allow 3-letter extlang subtags before script/region; stop on variants
        if (!(script == null && region == null && seg.len == 3 && isAllAlpha(seg))) {
            break;
        }
    }

    return LanguageTagParts{
        primary: primary,
        script: script,
        region: region,
    };
}

/// RFC 9110 §8.5 `Content-Language` 零分配 BCP 47 标签迭代器
export struct ContentLanguageIter {
    src: []const u8;
    pos: usize;

    fn init(header: []const u8): ContentLanguageIter {
        return ContentLanguageIter{
            src: header,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.src.len) {
            const seg_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != ',') {
                _.pos += 1;
            }
            const seg = trimOws(_.src[seg_start.._.pos]);
            if (_.pos < _.src.len && _.src[_.pos] == ',') {
                _.pos += 1;
            }
            if (isValidBcp47Tag(seg)) {
                return seg;
            }
        }
        return null;
    }
}

/// 按 RFC 5646 §2.1.1 规范化 BCP 47 语言标签大小写：
/// - `primary` 全小写（如 `en`, `zh`）
/// - `script`（4 字母）首字母大写其余小写（如 `Hans`）
/// - `region`（2 字母）全大写（如 `CN`, `US`）
/// - singleton（单字符扩展前缀）及其后续子标签全小写
/// 非法标签或输出缓冲不足返回 `null`
export fn canonicalizeLanguageTag(tag: []const u8, out: []u8): ([]const u8)? {
    if (!isValidBcp47Tag(tag) || out.len < tag.len) {
        return null;
    }

    let pos: usize = 0;
    let seg_idx: usize = 0;
    let in_singleton = false;

    while (pos < tag.len) {
        const seg_start = pos;
        while (pos < tag.len && tag[pos] != '-') {
            pos += 1;
        }
        const seg = tag[seg_start..pos];

        if (seg_idx == 0) {
            let i: usize = 0;
            while (i < seg.len) {
                out[seg_start + i] = toLowerAscii(seg[i]);
                i += 1;
            }
        } else if (in_singleton) {
            let i: usize = 0;
            while (i < seg.len) {
                out[seg_start + i] = toLowerAscii(seg[i]);
                i += 1;
            }
        } else if (seg.len == 1) {
            in_singleton = true;
            out[seg_start] = toLowerAscii(seg[0]);
        } else if (seg.len == 4 && isAllAlpha(seg)) {
            out[seg_start] = toUpperAscii(seg[0]);
            let i: usize = 1;
            while (i < seg.len) {
                out[seg_start + i] = toLowerAscii(seg[i]);
                i += 1;
            }
        } else if (seg.len == 2 && isAllAlpha(seg)) {
            out[seg_start] = toUpperAscii(seg[0]);
            out[seg_start + 1] = toUpperAscii(seg[1]);
        } else {
            let i: usize = 0;
            while (i < seg.len) {
                out[seg_start + i] = toLowerAscii(seg[i]);
                i += 1;
            }
        }

        if (pos < tag.len && tag[pos] == '-') {
            out[pos] = '-';
            pos += 1;
        }
        seg_idx += 1;
    }

    return out[0..tag.len];
}

/// 大小写不敏感查询 `Content-Language` 头部是否包含指定语言或主语种前缀
export fn hasContentLanguage(header: []const u8, target: []const u8): bool {
    if (!isValidBcp47Tag(target)) {
        return false;
    }
    let it = ContentLanguageIter.init(header);
    while (item := it.next()) {
        if (matchesLangOrPrefix(item, target)) {
            return true;
        }
    }
    return false;
}
