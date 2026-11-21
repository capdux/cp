// Source: Absorbed from /data/dev/bun/src/glob.zig (.gitignore Rule Parser & Path Matcher)

/// 解析后的单条 `.gitignore` 规则
pub struct GitignoreRule {
    pattern: []const u8 = "",
    negated: bool = false,
    dir_only: bool = false,
    anchored: bool = false,
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

fn stripBom(src: []const u8): []const u8 {
    if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
        return src[3..src.len];
    }
    return src;
}

/// 解析单行 `.gitignore` 规则：
/// - 忽略空行与未转义 `#` 注释行
/// - 剥离尾部未转义空格
/// - 识别前导 `!` 否定与 `\!`/`\#` 转义
/// - 识别尾部 `/` 仅匹配目录，以及前导/中间 `/` 为路径锚定
pub fn parseGitignoreLine(raw_line: []const u8): GitignoreRule? {
    let end = raw_line.len;
    if (end > 0 && raw_line[end - 1] == '\r') {
        end -= 1;
    }

    // 剥离尾部未转义空格（统计紧邻空格前的连续反斜杠数量，偶数个则该空格未转义）
    while (end > 0 && raw_line[end - 1] == ' ') {
        let bs: usize = 0;
        let k = end - 1;
        while (k > 0 && raw_line[k - 1] == '\\') {
            bs += 1;
            k -= 1;
        }
        if ((bs & 1) == 1) {
            break;
        }
        end -= 1;
    }

    let s = raw_line[0..end];
    if (s.len == 0) { return null; }

    // 未转义的前导 '#' 为注释行
    if (s[0] == '#') { return null; }

    let negated = false;
    if (s[0] == '!') {
        negated = true;
        s = s[1..s.len];
        if (s.len == 0) { return null; }
    }

    let dir_only = false;
    if (s[s.len - 1] == '/') {
        dir_only = true;
        while (s.len > 0 && s[s.len - 1] == '/') {
            s = s[0..(s.len - 1)];
        }
        if (s.len == 0) { return null; }
    }

    // 前导或中间包含 '/' 则视为路径锚定
    let anchored = false;
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '/') {
            anchored = true;
            break;
        }
        i += 1;
    }
    if (s[0] == '/') {
        s = s[1..s.len];
        if (s.len == 0) { return null; }
    }

    return GitignoreRule{
        pattern: s,
        negated: negated,
        dir_only: dir_only,
        anchored: anchored,
    };
}

/// 单路径段通配符匹配（支持 `*`、`?`、`[abc]`/`[a-z]`/`[!a-z]` 与 `\` 转义，不跨 `/`）
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
            } else if (pat[pi] == '[') {
                // 解析字符类 [...]
                let close_idx: usize? = null;
                let k = pi + 1;
                if (k < pat.len && (pat[k] == '!' || pat[k] == '^')) { k += 1; }
                if (k < pat.len && pat[k] == ']') { k += 1; }
                while (k < pat.len) {
                    if (pat[k] == ']') {
                        close_idx = k;
                        break;
                    }
                    k += 1;
                }
                if (ci := close_idx) {
                    let invert = false;
                    let m = pi + 1;
                    if (m < ci && (pat[m] == '!' || pat[m] == '^')) {
                        invert = true;
                        m += 1;
                    }
                    let matched_cls = false;
                    const ch = seg[si];
                    while (m < ci) {
                        if (m + 2 < ci && pat[m + 1] == '-') {
                            if (ch >= pat[m] && ch <= pat[m + 2]) {
                                matched_cls = true;
                            }
                            m += 3;
                        } else {
                            if (ch == pat[m]) {
                                matched_cls = true;
                            }
                            m += 1;
                        }
                    }
                    if (invert) { matched_cls = !matched_cls; }
                    if (matched_cls) {
                        pi = ci + 1;
                        si += 1;
                        continue;
                    }
                } else if (seg[si] == '[') {
                    pi += 1;
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
            if (star_si > seg.len) {
                return false;
            }
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

fn matchGlobPath(pat: []const u8, path: []const u8): bool {
    let psegs: [32][]const u8 = [""; 32];
    let ssegs: [32][]const u8 = [""; 32];
    const pn = splitSegments(pat, psegs[0..]);
    const sn = splitSegments(path, ssegs[0..]);
    if (pn == 0 || sn == 0) { return false; }

    // 尾随 /**（如 "abc/**"）要求在前缀之后至少匹配 1 个子路径段
    const trailing_globstar = (pn >= 2 && eqlBytes(psegs[pn - 1], "**"));

    let pi: usize = 0;
    let si: usize = 0;
    let star_pi: usize? = null;
    let star_si: usize = 0;

    while (si < sn || pi < pn) {
        if (pi < pn && eqlBytes(psegs[pi], "**")) {
            if (trailing_globstar && pi == pn - 1) {
                // 需要至少剩余 1 个路径段
                if (si < sn) {
                    return true;
                }
                if (sp := star_pi) {
                    star_si += 1;
                    if (star_si > sn) { return false; }
                    si = star_si;
                    pi = sp + 1;
                    continue;
                }
                return false;
            }
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
            if (star_si > sn) {
                return false;
            }
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

fn basenameOf(p: []const u8): []const u8 {
    let i = p.len;
    while (i > 0) {
        i -= 1;
        if (p[i] == '/') {
            return p[(i + 1)..p.len];
        }
    }
    return p;
}

/// 判断单条 `GitignoreRule` 是否命中指定相对路径 `rel_path`
pub fn ruleMatchesPath(rule: GitignoreRule, rel_path: []const u8, is_dir: bool): bool {
    const clean = stripSlashes(rel_path);
    if (clean.len == 0 || rule.pattern.len == 0) {
        return false;
    }
    if (rule.dir_only && !is_dir) {
        return false;
    }
    if (rule.anchored) {
        return matchGlobPath(rule.pattern, clean);
    }
    return matchSingleSegment(rule.pattern, basenameOf(clean));
}

/// 按行扫描整份 `.gitignore` 内容，遵循后出现规则覆盖前规则的 Git 语义（支持 `!pattern` 反忽略）
pub fn isIgnored(gitignore_content: []const u8, rel_path: []const u8, is_dir: bool): bool {
    const src = stripBom(gitignore_content);
    let ignored = false;
    let pos: usize = 0;

    while (pos < src.len) {
        const line_start = pos;
        while (pos < src.len && src[pos] != '\n') {
            pos += 1;
        }
        const line_end = pos;
        if (pos < src.len && src[pos] == '\n') {
            pos += 1;
        }

        if (rule := parseGitignoreLine(src[line_start..line_end])) {
            if (ruleMatchesPath(rule, rel_path, is_dir)) {
                ignored = !rule.negated;
            }
        }
    }

    return ignored;
}
