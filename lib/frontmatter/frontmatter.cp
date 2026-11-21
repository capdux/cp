// Source: Absorbed from /data/dev/bun/src/md/parser.zig (Markdown YAML/TOML Frontmatter Extractor)

/// Frontmatter 格式类型枚举
pub enum FrontmatterKind: u8 {
    none = 0,
    yaml = 1,
    toml = 2,

    fn asStr(): string {
        return switch (_) {
            .none => "none",
            .yaml => "yaml",
            .toml => "toml",
        };
    }
}

/// Markdown 文档头块与正文零拷贝切分结果
pub struct FrontmatterSplit {
    kind: FrontmatterKind = .none,
    raw_meta: []const u8 = "",
    body: []const u8 = "",
}

fn isHorizSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn trimHoriz(s: []const u8): []const u8 {
    let start: usize = 0;
    let end = s.len;
    while (start < end && isHorizSpace(s[start])) { start += 1; }
    while (end > start && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[start..end];
}

fn trimRightHoriz(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && isHorizSpace(s[end - 1])) { end -= 1; }
    return s[0..end];
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

/// 零拷贝切分 Markdown 文档中的 YAML (`---`) 或 TOML (`+++`) Frontmatter 头块与正文
/// 自动剥离可选 UTF-8 BOM，支持 LF/CRLF；未闭合或无头块时返回 `kind = .none` 且 `body = doc`
pub fn splitFrontmatter(doc: []const u8): FrontmatterSplit {
    let src = doc;
    if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
        src = src[3..src.len];
    }

    // 读取首行
    let pos: usize = 0;
    while (pos < src.len && src[pos] != '\n') {
        pos += 1;
    }
    if (pos >= src.len) {
        // 无换行，不可能存在闭合分隔行
        return FrontmatterSplit{ kind: .none, raw_meta: "", body: doc };
    }
    let first_end = pos;
    if (first_end > 0 && src[first_end - 1] == '\r') {
        first_end -= 1;
    }
    const first_line = trimRightHoriz(src[0..first_end]);

    let kind: FrontmatterKind = .none;
    if (eqlBytes(first_line, "---")) {
        kind = .yaml;
    } else if (eqlBytes(first_line, "+++")) {
        kind = .toml;
    } else {
        return FrontmatterSplit{ kind: .none, raw_meta: "", body: doc };
    }

    pos += 1; // 跳过首行 '\n'
    const meta_start = pos;

    while (pos < src.len) {
        const line_start = pos;
        while (pos < src.len && src[pos] != '\n') {
            pos += 1;
        }
        let line_end = pos;
        if (line_end > line_start && src[line_end - 1] == '\r') {
            line_end -= 1;
        }
        const next_pos = if (pos < src.len && src[pos] == '\n') pos + 1 else pos;

        const trimmed = trimRightHoriz(src[line_start..line_end]);
        const is_close = if (kind == .yaml)
            (eqlBytes(trimmed, "---") || eqlBytes(trimmed, "..."))
        else
            eqlBytes(trimmed, "+++");

        if (is_close) {
            return FrontmatterSplit{
                kind: kind,
                raw_meta: src[meta_start..line_start],
                body: src[next_pos..src.len],
            };
        }

        pos = next_pos;
    }

    // 未找到闭合分隔行，退回无 Frontmatter
    return FrontmatterSplit{ kind: .none, raw_meta: "", body: doc };
}

fn cleanFieldValue(raw_val: []const u8, kind: FrontmatterKind): []const u8 {
    const s = trimHoriz(raw_val);
    if (s.len == 0) { return ""; }

    if (s[0] == '"' || s[0] == '\'') {
        const q = s[0];
        let i: usize = 1;
        while (i < s.len) {
            if (q == '"' && s[i] == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (s[i] == q) {
                return s[1..i];
            }
            i += 1;
        }
        return s[1..s.len];
    }

    // 无引号值：剥离行尾 # 注释
    let end = s.len;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '#') {
            if (kind == .toml || i == 0 || isHorizSpace(s[i - 1])) {
                end = i;
                break;
            }
        }
        i += 1;
    }
    return trimRightHoriz(s[0..end]);
}

/// 在 Frontmatter 原始元数据区中零拷贝查询指定键的值
/// 支持 YAML `key: value` 与 TOML `key = value`，自动剥离首尾单/双引号与行尾注释，忽略 `#` 注释行
pub fn getFrontmatterField(raw_meta: []const u8, kind: FrontmatterKind, key: []const u8): ([]const u8)? {
    if (kind == .none || key.len == 0) {
        return null;
    }
    const sep: u8 = if (kind == .yaml) ':' else '=';

    let pos: usize = 0;
    while (pos < raw_meta.len) {
        const line_start = pos;
        while (pos < raw_meta.len && raw_meta[pos] != '\n') {
            pos += 1;
        }
        let line_end = pos;
        if (line_end > line_start && raw_meta[line_end - 1] == '\r') {
            line_end -= 1;
        }
        if (pos < raw_meta.len && raw_meta[pos] == '\n') {
            pos += 1;
        }

        const line = trimHoriz(raw_meta[line_start..line_end]);
        if (line.len == 0 || line[0] == '#') {
            continue;
        }

        let sep_idx: usize? = null;
        let k: usize = 0;
        while (k < line.len) {
            if (line[k] == sep) {
                sep_idx = k;
                break;
            }
            k += 1;
        }

        if (si := sep_idx) {
            const line_key = trimHoriz(line[0..si]);
            if (eqlBytes(line_key, key)) {
                return cleanFieldValue(line[(si + 1)..line.len], kind);
            }
        }
    }

    return null;
}
