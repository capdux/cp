// Source: Absorbed from /data/dev/bun/src/bundler/HTMLScanner.cp | Synced: 2026-11-21T22:51:57Z

/// HTML 资源引用类型枚举
pub enum AssetKind: u8 {
    script = 0,
    stylesheet = 1,
    image = 2,
    media = 3,
    font = 4,
    icon = 5,
    manifest = 6,
    worker = 7,

    fn asStr(): string {
        return switch (_) {
            .script => "script",
            .stylesheet => "stylesheet",
            .image => "image",
            .media => "media",
            .font => "font",
            .icon => "icon",
            .manifest => "manifest",
            .worker => "worker",
        };
    }
}

/// 零拷贝 HTML 资源引用（含精确字节偏移，便于 bundler 原位重写 URL）
pub struct AssetRef {
    kind: AssetKind = .image,
    tag: []const u8 = "",
    attr: []const u8 = "",
    url: []const u8 = "",
    start_offset: usize = 0,
    end_offset: usize = 0,
}

/// `srcset` 单条候选项（URL + 可选宽度/像素密度描述符，如 `480w`、`2x`）
pub struct SrcsetCandidate {
    url: []const u8 = "",
    descriptor: []const u8 = "",
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0C;
}

fn isTagNameChar(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == ':';
}

fn isAttrNameChar(c: u8): bool {
    return !isSpace(c) && c != '=' && c != '>' && c != '/' && c != '"' && c != '\'' && c != '<';
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpace(s[start])) { start += 1; }
    while (end > start && isSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn eqlIgnoreCaseAscii(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        let ca = a[i];
        let cb = b[i];
        if (ca >= 'A' && ca <= 'Z') { ca += 32; }
        if (cb >= 'A' && cb <= 'Z') { cb += 32; }
        if (ca != cb) { return false; }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCaseAscii(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) { return false; }
    return eqlIgnoreCaseAscii(s[0..prefix.len], prefix);
}

fn containsSpaceTokenIgnoreCase(list: []const u8, target: []const u8): bool {
    let i: usize = 0;
    while (i < list.len) {
        while (i < list.len && isSpace(list[i])) { i += 1; }
        if (i >= list.len) { break; }
        const start = i;
        while (i < list.len && !isSpace(list[i])) { i += 1; }
        if (eqlIgnoreCaseAscii(list[start..i], target)) {
            return true;
        }
    }
    return false;
}

/// 零堆分配解析 HTML `srcset` 属性值为候选项列表，返回实际写入数量
pub fn parseSrcset(srcset: []const u8, out: []SrcsetCandidate): usize {
    let count: usize = 0;
    let i: usize = 0;

    while (i < srcset.len && count < out.len) {
        while (i < srcset.len && (isSpace(srcset[i]) || srcset[i] == ',')) {
            i += 1;
        }
        if (i >= srcset.len) { break; }

        const url_start = i;
        while (i < srcset.len && !isSpace(srcset[i])) {
            i += 1;
        }
        let url_end = i;
        let has_trailing_comma = false;
        while (url_end > url_start && srcset[url_end - 1] == ',') {
            url_end -= 1;
            has_trailing_comma = true;
        }

        if (url_end == url_start) {
            continue;
        }

        let desc: []const u8 = "";
        if (!has_trailing_comma) {
            while (i < srcset.len && isSpace(srcset[i])) { i += 1; }
            const desc_start = i;
            let paren_depth: usize = 0;
            while (i < srcset.len) {
                if (srcset[i] == '(') {
                    paren_depth += 1;
                } else if (srcset[i] == ')' && paren_depth > 0) {
                    paren_depth -= 1;
                } else if (paren_depth == 0 && srcset[i] == ',') {
                    break;
                }
                i += 1;
            }
            desc = trimSpaces(srcset[desc_start..i]);
            if (i < srcset.len && srcset[i] == ',') {
                i += 1;
            }
        }

        out[count] = SrcsetCandidate{
            url: srcset[url_start..url_end],
            descriptor: desc,
        };
        count += 1;
    }

    return count;
}

struct ParsedAttr {
    name: []const u8 = "",
    value: []const u8 = "",
    val_start: usize = 0,
    val_end: usize = 0,
}

fn findClosingRawTag(html: []const u8, start: usize, tag_name: []const u8): usize {
    let i = start;
    while (i + 2 + tag_name.len < html.len) {
        if (html[i] == '<' && html[i + 1] == '/') {
            const name_start = i + 2;
            const name_end = name_start + tag_name.len;
            if (eqlIgnoreCaseAscii(html[name_start..name_end], tag_name)) {
                let p = name_end;
                while (p < html.len && isSpace(html[p])) { p += 1; }
                if (p < html.len && html[p] == '>') {
                    return p + 1;
                }
            }
        }
        i += 1;
    }
    return html.len;
}

fn pushRef(out: []AssetRef, count: *usize, kind: AssetKind, tag: []const u8, attr: ParsedAttr) {
    if (attr.value.len == 0 || count.* >= out.len) {
        return;
    }
    out[count.*] = AssetRef{
        kind: kind,
        tag: tag,
        attr: attr.name,
        url: attr.value,
        start_offset: attr.val_start,
        end_offset: attr.val_end,
    };
    count.* += 1;
}

/// 纯 CP、零堆分配 HTML 资源引用扫描器
pub struct HtmlAssetScanner {
    /// 扫描 HTML 源码中的所有外部资源引用，写入 `out` 并返回实际写入数量
    static fn scan(html: []const u8, out: []AssetRef): usize {
        let count: usize = 0;
        let i: usize = 0;

        while (i < html.len && count < out.len) {
            if (html[i] != '<') {
                i += 1;
                continue;
            }

            // 1. HTML 注释 <!-- ... -->
            if (i + 4 <= html.len &&
                html[i + 1] == '!' && html[i + 2] == '-' && html[i + 3] == '-')
            {
                i += 4;
                let closed = false;
                while (i + 2 < html.len) {
                    if (html[i] == '-' && html[i + 1] == '-' && html[i + 2] == '>') {
                        i += 3;
                        closed = true;
                        break;
                    }
                    i += 1;
                }
                if (!closed) {
                    i = html.len;
                }
                continue;
            }

            // 2. <!DOCTYPE ...> / <?...?> / </closing>
            if (i + 1 < html.len && (html[i + 1] == '!' || html[i + 1] == '?' || html[i + 1] == '/')) {
                i += 2;
                while (i < html.len && html[i] != '>') { i += 1; }
                if (i < html.len) { i += 1; }
                continue;
            }

            // 3. 开始标签 <tag ...>
            i += 1;
            const tag_start = i;
            while (i < html.len && isTagNameChar(html[i])) { i += 1; }
            const tag_name = html[tag_start..i];
            if (tag_name.len == 0) {
                continue;
            }

            let attrs: [16]ParsedAttr = [ParsedAttr{}; 16];
            let attr_count: usize = 0;
            let self_closing = false;

            while (i < html.len) {
                while (i < html.len && isSpace(html[i])) { i += 1; }
                if (i >= html.len) { break; }

                if (html[i] == '>') {
                    i += 1;
                    break;
                }
                if (html[i] == '/') {
                    if (i + 1 < html.len && html[i + 1] == '>') {
                        self_closing = true;
                        i += 2;
                        break;
                    }
                    i += 1;
                    continue;
                }

                const an_start = i;
                while (i < html.len && isAttrNameChar(html[i])) { i += 1; }
                const an_end = i;
                if (an_end == an_start) {
                    i += 1;
                    continue;
                }
                const attr_name = html[an_start..an_end];

                while (i < html.len && isSpace(html[i])) { i += 1; }

                let val_start: usize = i;
                let val_end: usize = i;
                if (i < html.len && html[i] == '=') {
                    i += 1;
                    while (i < html.len && isSpace(html[i])) { i += 1; }
                    if (i < html.len && (html[i] == '"' || html[i] == '\'')) {
                        const quote = html[i];
                        i += 1;
                        val_start = i;
                        while (i < html.len && html[i] != quote) { i += 1; }
                        val_end = i;
                        if (i < html.len && html[i] == quote) { i += 1; }
                    } else {
                        val_start = i;
                        while (i < html.len && !isSpace(html[i]) && html[i] != '>' && html[i] != '"' && html[i] != '\'') {
                            if (html[i] == '/' && i + 1 < html.len && html[i + 1] == '>') {
                                break;
                            }
                            i += 1;
                        }
                        val_end = i;
                    }
                }

                if (attr_count < 16) {
                    attrs[attr_count] = ParsedAttr{
                        name: attr_name,
                        value: html[val_start..val_end],
                        val_start: val_start,
                        val_end: val_end,
                    };
                    attr_count += 1;
                }
            }

            // 匹配各类资产标签
            const is_script = eqlIgnoreCaseAscii(tag_name, "script");
            const is_style = eqlIgnoreCaseAscii(tag_name, "style");

            if (is_script) {
                let k: usize = 0;
                while (k < attr_count) {
                    if (eqlIgnoreCaseAscii(attrs[k].name, "src")) {
                        pushRef(out, *count, .script, tag_name, attrs[k]);
                    }
                    k += 1;
                }
            } else if (eqlIgnoreCaseAscii(tag_name, "link")) {
                let rel_val: []const u8 = "";
                let as_val: []const u8 = "";
                let type_val: []const u8 = "";
                let href_idx: usize? = null;
                let k: usize = 0;
                while (k < attr_count) {
                    if (eqlIgnoreCaseAscii(attrs[k].name, "rel")) {
                        rel_val = attrs[k].value;
                    } else if (eqlIgnoreCaseAscii(attrs[k].name, "as")) {
                        as_val = attrs[k].value;
                    } else if (eqlIgnoreCaseAscii(attrs[k].name, "type")) {
                        type_val = attrs[k].value;
                    } else if (eqlIgnoreCaseAscii(attrs[k].name, "href")) {
                        href_idx = k;
                    }
                    k += 1;
                }
                if (hi := href_idx) {
                    let link_kind: AssetKind? = null;
                    if (containsSpaceTokenIgnoreCase(rel_val, "stylesheet") || eqlIgnoreCaseAscii(as_val, "style")) {
                        link_kind = .stylesheet;
                    } else if (containsSpaceTokenIgnoreCase(rel_val, "modulepreload") || eqlIgnoreCaseAscii(as_val, "script")) {
                        link_kind = .script;
                    } else if (eqlIgnoreCaseAscii(as_val, "font") || startsWithIgnoreCaseAscii(type_val, "font/")) {
                        link_kind = .font;
                    } else if (eqlIgnoreCaseAscii(as_val, "image")) {
                        link_kind = .image;
                    } else if (eqlIgnoreCaseAscii(as_val, "video") || eqlIgnoreCaseAscii(as_val, "audio")) {
                        link_kind = .media;
                    } else if (eqlIgnoreCaseAscii(as_val, "worker")) {
                        link_kind = .worker;
                    } else if (containsSpaceTokenIgnoreCase(rel_val, "manifest")) {
                        link_kind = .manifest;
                    } else if (containsSpaceTokenIgnoreCase(rel_val, "icon") || containsSpaceTokenIgnoreCase(rel_val, "apple-touch-icon")) {
                        link_kind = .icon;
                    }
                    if (lk := link_kind) {
                        pushRef(out, *count, lk, tag_name, attrs[hi]);
                    }
                }
            } else if (eqlIgnoreCaseAscii(tag_name, "img")) {
                let k: usize = 0;
                while (k < attr_count) {
                    if (eqlIgnoreCaseAscii(attrs[k].name, "src") || eqlIgnoreCaseAscii(attrs[k].name, "srcset")) {
                        pushRef(out, *count, .image, tag_name, attrs[k]);
                    }
                    k += 1;
                }
            } else if (eqlIgnoreCaseAscii(tag_name, "video")) {
                let k: usize = 0;
                while (k < attr_count) {
                    if (eqlIgnoreCaseAscii(attrs[k].name, "src")) {
                        pushRef(out, *count, .media, tag_name, attrs[k]);
                    } else if (eqlIgnoreCaseAscii(attrs[k].name, "poster")) {
                        pushRef(out, *count, .image, tag_name, attrs[k]);
                    }
                    k += 1;
                }
            } else if (eqlIgnoreCaseAscii(tag_name, "audio")) {
                let k: usize = 0;
                while (k < attr_count) {
                    if (eqlIgnoreCaseAscii(attrs[k].name, "src")) {
                        pushRef(out, *count, .media, tag_name, attrs[k]);
                    }
                    k += 1;
                }
            } else if (eqlIgnoreCaseAscii(tag_name, "source")) {
                let k: usize = 0;
                while (k < attr_count) {
                    if (eqlIgnoreCaseAscii(attrs[k].name, "src")) {
                        pushRef(out, *count, .media, tag_name, attrs[k]);
                    } else if (eqlIgnoreCaseAscii(attrs[k].name, "srcset")) {
                        pushRef(out, *count, .image, tag_name, attrs[k]);
                    }
                    k += 1;
                }
            }

            // 跳过 <script>...</script> 与 <style>...</style> 原始文本内容
            if (!self_closing && (is_script || is_style)) {
                i = findClosingRawTag(html, i, tag_name);
            }
        }

        return count;
    }
}
