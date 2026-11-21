// Source: Absorbed from /data/dev/bun/src/glob.zig | Synced: 2026-11-21T15:06:44Z (CODEOWNERS Rule Parser & Owner Resolver)

/// 单条解析后的 `CODEOWNERS` 规则
pub struct CodeownersRule {
    pattern: []const u8 = "",
    owners_raw: []const u8 = "",
    line_no: usize = 0,
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

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
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

/// 零堆分配 Owner 列表迭代器（按空白切分 `@user`/`@org/team`/`a@b.com`，遇 `#` 注释停止）
pub struct OwnerIter {
    raw: []const u8,
    pos: usize = 0,

    static fn init(owners_raw: []const u8): OwnerIter {
        return OwnerIter{ raw: owners_raw, pos: 0 };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.raw.len && isHorizSpace(_.raw[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.raw.len || _.raw[_.pos] == '#') {
            _.pos = _.raw.len;
            return null;
        }
        const start = _.pos;
        while (_.pos < _.raw.len && !isHorizSpace(_.raw[_.pos]) && _.raw[_.pos] != '#') {
            _.pos += 1;
        }
        return _.raw[start.._.pos];
    }
}

/// 零堆分配 `CODEOWNERS` 规则迭代器
/// 自动剥离 UTF-8 BOM，忽略空行、`#` 注释行与 GitLab `[Section]` 头行，支持路径内 `\ ` 转义空格
pub struct CodeownersIter {
    src: []const u8,
    pos: usize = 0,
    line_no: usize = 1,

    static fn init(content: []const u8): CodeownersIter {
        let start_pos: usize = 0;
        if (content.len >= 3 && content[0] == 0xEF && content[1] == 0xBB && content[2] == 0xBF) {
            start_pos = 3;
        }
        return CodeownersIter{
            src: content,
            pos: start_pos,
            line_no: 1,
        };
    }

    fn next(*_): CodeownersRule? {
        while (_.pos < _.src.len) {
            const cur_line = _.line_no;
            const line_start = _.pos;
            while (_.pos < _.src.len && _.src[_.pos] != '\n') {
                _.pos += 1;
            }
            let line_end = _.pos;
            if (line_end > line_start && _.src[line_end - 1] == '\r') {
                line_end -= 1;
            }
            if (_.pos < _.src.len && _.src[_.pos] == '\n') {
                _.pos += 1;
            }
            _.line_no += 1;

            const line = trimHoriz(_.src[line_start..line_end]);
            if (line.len == 0 || line[0] == '#') {
                continue;
            }
            // 忽略 GitLab [Section] / ^[Section] 头行
            if (line[0] == '[' || (line.len >= 2 && line[0] == '^' && line[1] == '[')) {
                continue;
            }

            // 扫描 pattern（支持 '\ ' 转义空格）
            let i: usize = 0;
            while (i < line.len) {
                if (line[i] == '\\' && i + 1 < line.len) {
                    i += 2;
                    continue;
                }
                if (isHorizSpace(line[i]) || line[i] == '#') {
                    break;
                }
                i += 1;
            }

            const pat = line[0..i];
            if (pat.len == 0) {
                continue;
            }

            // 提取剩余行中 '#' 之前的 owners 区
            let rest = trimHoriz(line[i..line.len]);
            let comment_pos: usize = rest.len;
            let k: usize = 0;
            while (k < rest.len) {
                if (rest[k] == '#') {
                    comment_pos = k;
                    break;
                }
                k += 1;
            }
            const owners_raw = trimHoriz(rest[0..comment_pos]);

            return CodeownersRule{
                pattern: pat,
                owners_raw: owners_raw,
                line_no: cur_line,
            };
        }
        return null;
    }
}

fn matchSingleSegment(pat: []const u8, seg: []const u8): bool {
    let pi: usize = 0;
    let si: usize = 0;
    let star_pi: usize? = null;
    let star_si: usize = 0;

    while (si < seg.len || pi < pat.len) {
        if (pi < pat.len && pat[pi] == '*') {
            star_pi = pi;
            star_si = si;
            pi += 1;
            continue;
        }
        if (pi < pat.len && si < seg.len) {
            if (pat[pi] == '?') {
                pi += 1;
                si += 1;
                continue;
            }
            if (pat[pi] == '\\' && pi + 1 < pat.len) {
                if (pat[pi + 1] == seg[si]) {
                    pi += 2;
                    si += 1;
                    continue;
                }
            } else if (pat[pi] == seg[si]) {
                pi += 1;
                si += 1;
                continue;
            }
        }
        if (sp := star_pi) {
            star_si += 1;
            if (star_si > seg.len) { return false; }
            si = star_si;
            pi = sp + 1;
        } else {
            return false;
        }
    }
    return true;
}

fn splitSegments(path: []const u8, out: [][]const u8): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < path.len && count < out.len) {
        while (i < path.len && path[i] == '/') { i += 1; }
        if (i >= path.len) { break; }
        const start = i;
        while (i < path.len && path[i] != '/') { i += 1; }
        out[count] = path[start..i];
        count += 1;
    }
    return count;
}

fn matchGlobSegments(psegs: []const []const u8, ssegs: []const []const u8): bool {
    const pn = psegs.len;
    const sn = ssegs.len;
    if (pn == 0 || sn == 0) { return false; }

    let pi: usize = 0;
    let si: usize = 0;
    let star_pi: usize? = null;
    let star_si: usize = 0;

    while (si < sn || pi < pn) {
        if (pi < pn && eqlBytes(psegs[pi], "**")) {
            star_pi = pi;
            star_si = si;
            pi += 1;
            continue;
        }
        if (pi < pn && si < sn && matchSingleSegment(psegs[pi], ssegs[si])) {
            pi += 1;
            si += 1;
            continue;
        }
        if (sp := star_pi) {
            star_si += 1;
            if (star_si > sn) { return false; }
            si = star_si;
            pi = sp + 1;
        } else {
            return false;
        }
    }
    return true;
}

fn stripSlashes(p: []const u8): []const u8 {
    let start: usize = 0;
    let end = p.len;
    while (start < end && p[start] == '/') { start += 1; }
    while (end > start && p[end - 1] == '/') { end -= 1; }
    return p[start..end];
}

fn matchCodeownersPattern(pattern: []const u8, rel_path: []const u8): bool {
    const clean = stripSlashes(rel_path);
    if (clean.len == 0 || pattern.len == 0) { return false; }

    // 全局通配 '*'
    if (eqlBytes(pattern, "*")) {
        return true;
    }

    let ssegs: [32][]const u8 = [""; 32];
    const sn = splitSegments(clean, ssegs[0..]);
    if (sn == 0) { return false; }

    // 目录规则（以 '/' 结尾，如 "/docs/" 或 "docs/"）：匹配该目录下任意深度的文件
    if (pattern[pattern.len - 1] == '/') {
        const anchored_dir = (pattern[0] == '/');
        const dir_body = stripSlashes(pattern);
        if (dir_body.len == 0) { return true; }

        let dsegs: [32][]const u8 = [""; 32];
        const dn = splitSegments(dir_body, dsegs[0..]);
        if (dn == 0 || sn <= dn) { return false; } // 文件必须位于目录内部

        if (anchored_dir) {
            return matchGlobSegments(dsegs[0..dn], ssegs[0..dn]);
        }

        // 未锚定目录（如 "docs/"）：路径中任意父目录段序列匹配即命中
        let offset: usize = 0;
        while (offset + dn < sn) {
            if (matchGlobSegments(dsegs[0..dn], ssegs[offset..(offset + dn)])) {
                return true;
            }
            offset += 1;
        }
        return false;
    }

    // 检查是否包含 '/'（锚定路径）
    let has_slash = false;
    let i: usize = 0;
    while (i < pattern.len) {
        if (pattern[i] == '/') {
            has_slash = true;
            break;
        }
        i += 1;
    }

    if (!has_slash) {
        // 未锚定文件名模式（如 "*.cp"、"Makefile"）匹配任意路径的 basename
        return matchSingleSegment(pattern, ssegs[sn - 1]);
    }

    const body = if (pattern[0] == '/') pattern[1..pattern.len] else pattern;
    let psegs: [32][]const u8 = [""; 32];
    const pn = splitSegments(body, psegs[0..]);
    return matchGlobSegments(psegs[0..pn], ssegs[0..sn]);
}

/// 按 GitHub CODEOWNERS 语义解析路径归属（后匹配规则覆盖前规则；命中空 owner 清空规则返回 `null`）
pub fn resolveOwners(content: []const u8, rel_path: []const u8): ([]const u8)? {
    let it = CodeownersIter.init(content);
    let last_owners: ([]const u8)? = null;

    while (rule := it.next()) {
        if (matchCodeownersPattern(rule.pattern, rel_path)) {
            if (rule.owners_raw.len == 0) {
                last_owners = null;
            } else {
                last_owners = rule.owners_raw;
            }
        }
    }

    return last_owners;
}

/// 大小写不敏感判断指定 `owner` 是否归属 `rel_path`
pub fn isOwnedBy(content: []const u8, rel_path: []const u8, owner: []const u8): bool {
    if (owner.len == 0) { return false; }
    if (owners_raw := resolveOwners(content, rel_path)) {
        let it = OwnerIter.init(owners_raw);
        while (tok := it.next()) {
            if (eqlIgnoreCaseAscii(tok, owner)) {
                return true;
            }
        }
    }
    return false;
}
