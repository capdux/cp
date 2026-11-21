// Source: Absorbed from /data/dev/bun/src/md/links.cp

/// CommonMark §6.3–6.5 链接与图片类型枚举
pub enum LinkKind: u8 {
    inline_link = 0,
    image = 1,
    full_ref = 2,
    collapsed_ref = 3,
    shortcut_ref = 4,
    autolink_uri = 5,
    autolink_email = 6,

    fn isImage(): bool {
        return _ == .image;
    }

    fn isAutolink(): bool {
        return _ == .autolink_uri || _ == .autolink_email;
    }

    fn isReference(): bool {
        return _ == .full_ref || _ == .collapsed_ref || _ == .shortcut_ref;
    }
}

/// 零拷贝解析后的链接/图片/自动链接结构体
pub struct ParsedLink {
    kind: LinkKind,
    text: []const u8 = "",
    dest: []const u8 = "",
    title: []const u8 = "",
    start_pos: usize = 0,
    end_pos: usize = 0,
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isAsciiDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAsciiAlnum(c: u8): bool {
    return isAsciiAlpha(c) || isAsciiDigit(c);
}

fn isAsciiSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn isHexDigit(c: u8): bool {
    return isAsciiDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

fn isBlankOnly(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (!isAsciiSpace(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isSchemeChar(c: u8): bool {
    return isAsciiAlnum(c) || c == '+' || c == '-' || c == '.';
}

fn isEmailLocalChar(c: u8): bool {
    if (isAsciiAlnum(c)) { return true; }
    return c == '.' || c == '!' || c == '#' || c == '$' || c == '%' ||
        c == '&' || c == '\'' || c == '*' || c == '+' || c == '/' ||
        c == '=' || c == '?' || c == '^' || c == '_' || c == '`' ||
        c == '{' || c == '|' || c == '}' || c == '~' || c == '-';
}

fn isValidEmailDomain(domain: []const u8): bool {
    if (domain.len == 0) {
        return false;
    }
    let label_start: usize = 0;
    let i: usize = 0;
    while (i <= domain.len) {
        if (i == domain.len || domain[i] == '.') {
            const label_len = i - label_start;
            if (label_len == 0 || label_len > 63) {
                return false;
            }
            if (!isAsciiAlnum(domain[label_start]) || !isAsciiAlnum(domain[i - 1])) {
                return false;
            }
            let k = label_start;
            while (k < i) {
                const c = domain[k];
                if (!isAsciiAlnum(c) && c != '-') {
                    return false;
                }
                k += 1;
            }
            label_start = i + 1;
        }
        i += 1;
    }
    return true;
}

/// 解析 CommonMark §6.5 尖括号自动链接（`<scheme:...>` 或 `<user@domain>`）
pub fn parseAutolink(text: []const u8, pos: usize): ParsedLink? {
    if (pos >= text.len || text[pos] != '<') {
        return null;
    }

    let end: usize = pos + 1;
    while (end < text.len && text[end] != '>') {
        const c = text[end];
        if (c <= 0x20 || c == 0x7F || c == '<') {
            return null;
        }
        end += 1;
    }
    if (end >= text.len || text[end] != '>') {
        return null;
    }

    const inner = text[pos + 1 .. end];
    if (inner.len < 3) {
        return null;
    }

    // 1. 尝试 URI 自动链接：scheme (2..32 chars) + ':'
    if (isAsciiAlpha(inner[0])) {
        let colon_idx: usize? = null;
        let i: usize = 1;
        while (i < inner.len && i <= 32) {
            if (inner[i] == ':') {
                colon_idx = i;
                break;
            }
            if (!isSchemeChar(inner[i])) {
                break;
            }
            i += 1;
        }
        if (ci := colon_idx) {
            if (ci >= 2 && ci <= 32) {
                return ParsedLink{
                    kind: .autolink_uri,
                    text: inner,
                    dest: inner,
                    title: "",
                    start_pos: pos,
                    end_pos: end + 1,
                };
            }
        }
    }

    // 2. 尝试 Email 自动链接：local@domain
    let at_idx: usize? = null;
    let j: usize = 0;
    while (j < inner.len) {
        if (inner[j] == '@') {
            if (at_idx != null) {
                return null;
            }
            at_idx = j;
        }
        j += 1;
    }
    if (ai := at_idx) {
        const local = inner[0..ai];
        const domain = inner[ai + 1 ..];
        if (local.len == 0 || !isValidEmailDomain(domain)) {
            return null;
        }
        let k: usize = 0;
        while (k < local.len) {
            if (!isEmailLocalChar(local[k])) {
                return null;
            }
            k += 1;
        }
        return ParsedLink{
            kind: .autolink_email,
            text: inner,
            dest: inner,
            title: "",
            start_pos: pos,
            end_pos: end + 1,
        };
    }

    return null;
}

fn skipCodeSpan(text: []const u8, start: usize): usize? {
    let ticks: usize = 0;
    while (start + ticks < text.len && text[start + ticks] == '`') {
        ticks += 1;
    }
    let i = start + ticks;
    while (i < text.len) {
        if (text[i] == '`') {
            let run: usize = 0;
            while (i + run < text.len && text[i + run] == '`') {
                run += 1;
            }
            if (run == ticks) {
                return i + run;
            }
            i += run;
            continue;
        }
        i += 1;
    }
    return null;
}

/// 解析 CommonMark §6.3–6.4 内联链接、图片以及 full/collapsed/shortcut 引用链接
pub fn parseInlineLinkOrImage(text: []const u8, start_pos: usize): ParsedLink? {
    if (start_pos >= text.len) {
        return null;
    }

    let is_image = false;
    let bracket_open = start_pos;
    if (text[start_pos] == '!') {
        if (start_pos + 1 >= text.len || text[start_pos + 1] != '[') {
            return null;
        }
        is_image = true;
        bracket_open = start_pos + 1;
    } else if (text[start_pos] != '[') {
        return null;
    }

    // 查找匹配的 ']'（跳过转义字符、代码跨度与平衡嵌套括号）
    let pos = bracket_open + 1;
    let depth: usize = 1;
    while (pos < text.len && depth > 0) {
        const c = text[pos];
        if (c == '\\' && pos + 1 < text.len) {
            pos += 2;
            continue;
        }
        if (c == '`') {
            if (after_code := skipCodeSpan(text, pos)) {
                pos = after_code;
                continue;
            }
        }
        if (c == '[') {
            depth += 1;
        } else if (c == ']') {
            depth -= 1;
            if (depth == 0) {
                break;
            }
        }
        pos += 1;
    }

    if (depth != 0) {
        return null;
    }

    const label = text[bracket_open + 1 .. pos];
    const after_bracket = pos + 1;

    // 1. 尝试内联链接/图片: [text](dest "title")
    if (after_bracket < text.len && text[after_bracket] == '(') {
        let p = after_bracket + 1;
        while (p < text.len && isAsciiSpace(text[p])) { p += 1; }

        let dest_start = p;
        let dest_end = p;
        let valid_dest = true;

        if (p < text.len && text[p] == '<') {
            p += 1;
            dest_start = p;
            while (p < text.len && text[p] != '>') {
                if (text[p] == '\n' || text[p] == '\r' || text[p] == '<') {
                    valid_dest = false;
                    break;
                }
                if (text[p] == '\\' && p + 1 < text.len) {
                    p += 2;
                } else {
                    p += 1;
                }
            }
            if (!valid_dest || p >= text.len || text[p] != '>') {
                valid_dest = false;
            } else {
                dest_end = p;
                p += 1; // skip '>'
            }
        } else {
            let paren_depth: usize = 0;
            while (p < text.len && !isAsciiSpace(text[p])) {
                if (text[p] == '\\' && p + 1 < text.len) {
                    p += 2;
                    continue;
                }
                if (text[p] == '(') {
                    paren_depth += 1;
                } else if (text[p] == ')') {
                    if (paren_depth == 0) {
                        break;
                    }
                    paren_depth -= 1;
                }
                p += 1;
            }
            if (paren_depth != 0) {
                valid_dest = false;
            }
            dest_end = p;
        }

        if (valid_dest) {
            const ws_before_title = p;
            while (p < text.len && isAsciiSpace(text[p])) { p += 1; }
            const had_ws = p > ws_before_title;

            let title: []const u8 = "";
            let valid_title = true;

            if (p < text.len && (text[p] == '"' || text[p] == '\'' || text[p] == '(')) {
                if (dest_end > dest_start && !had_ws) {
                    valid_title = false;
                } else {
                    const close_ch: u8 = if (text[p] == '(') ')' else text[p];
                    p += 1;
                    const t_start = p;
                    while (p < text.len && text[p] != close_ch) {
                        if (text[p] == '\\' && p + 1 < text.len) {
                            p += 2;
                        } else {
                            p += 1;
                        }
                    }
                    if (p >= text.len || text[p] != close_ch) {
                        valid_title = false;
                    } else {
                        title = text[t_start..p];
                        p += 1;
                    }
                }
            }

            if (valid_title) {
                while (p < text.len && isAsciiSpace(text[p])) { p += 1; }
                if (p < text.len && text[p] == ')') {
                    return ParsedLink{
                        kind: if (is_image) .image else .inline_link,
                        text: label,
                        dest: text[dest_start..dest_end],
                        title: title,
                        start_pos: start_pos,
                        end_pos: p + 1,
                    };
                }
            }
        }
    }

    // 图片暂不走纯引用降级（仅当明确为引用链接时）
    // 2. Full / Collapsed 引用链接: [text][ref] 或 [text][]
    if (!is_image && after_bracket < text.len && text[after_bracket] == '[') {
        let p = after_bracket + 1;
        const ref_start = p;
        let valid_ref = true;
        while (p < text.len && text[p] != ']') {
            if (text[p] == '[') {
                valid_ref = false;
                break;
            }
            if (text[p] == '\\' && p + 1 < text.len) {
                p += 2;
            } else {
                p += 1;
            }
        }
        if (valid_ref && p < text.len && text[p] == ']') {
            const ref_slice = text[ref_start..p];
            if (ref_slice.len == 0) {
                if (!isBlankOnly(label)) {
                    return ParsedLink{
                        kind: .collapsed_ref,
                        text: label,
                        dest: label,
                        title: "",
                        start_pos: start_pos,
                        end_pos: p + 1,
                    };
                }
            } else if (!isBlankOnly(ref_slice)) {
                return ParsedLink{
                    kind: .full_ref,
                    text: label,
                    dest: ref_slice,
                    title: "",
                    start_pos: start_pos,
                    end_pos: p + 1,
                };
            }
        }
    }

    // 3. Shortcut 引用链接: [text]（后面不紧跟 '[' 且非空白）
    if (!is_image && !isBlankOnly(label)) {
        const next_is_bracket = after_bracket < text.len && text[after_bracket] == '[';
        if (!next_is_bracket) {
            return ParsedLink{
                kind: .shortcut_ref,
                text: label,
                dest: label,
                title: "",
                start_pos: start_pos,
                end_pos: after_bracket,
            };
        }
    }

    return null;
}

fn isSafeUrlChar(c: u8): bool {
    if (isAsciiAlnum(c)) { return true; }
    return c == '-' || c == '_' || c == '.' || c == '~' ||
        c == ':' || c == '/' || c == '?' || c == '#' ||
        c == '[' || c == ']' || c == '@' || c == '!' ||
        c == '$' || c == '&' || c == '\'' || c == '(' ||
        c == ')' || c == '*' || c == '+' || c == ',' ||
        c == ';' || c == '=';
}

const HEX_UPPER: string = "0123456789ABCDEF";

/// 安全百分号编码 URL（编码空格/非 ASCII/控制字符，保留合法 `%XX` 序列，零堆分配）
pub fn percentEncodeUrl(raw: []const u8, out: []u8): ([]const u8)? {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < raw.len) {
        const c = raw[i];
        if (c == '%' && i + 2 < raw.len && isHexDigit(raw[i + 1]) && isHexDigit(raw[i + 2])) {
            if (pos + 3 > out.len) {
                return null;
            }
            out[pos] = '%';
            out[pos + 1] = raw[i + 1];
            out[pos + 2] = raw[i + 2];
            pos += 3;
            i += 3;
            continue;
        }
        if (isSafeUrlChar(c)) {
            if (pos + 1 > out.len) {
                return null;
            }
            out[pos] = c;
            pos += 1;
        } else {
            if (pos + 3 > out.len) {
                return null;
            }
            out[pos] = '%';
            out[pos + 1] = HEX_UPPER[(c >> 4) as usize];
            out[pos + 2] = HEX_UPPER[(c & 0x0F) as usize];
            pos += 3;
        }
        i += 1;
    }
    return out[0..pos];
}
