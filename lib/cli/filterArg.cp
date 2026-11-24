// Source: Absorbed from /data/dev/bun/src/cli/filter_arg.cp | Synced: 2026-11-23T20:18:30Z
// Ported from Bun: /data/dev/bun/src/cli/filter_arg.cp & /data/dev/bun/docs/pm/filter.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation monorepo `--filter` / `-F` pattern parser, path normalizer,
// iterative glob matcher (`*`, `**`, `?`, `[...]`), negation rules (`!pkg-c`, `!./`),
// workspace package pattern formatter, and Foreman-style script prefix formatter.

/// 单个 `FilterSet` 支持的最大过滤规则条数
pub const FILTER_MAX_PATTERNS: usize = 32;

/// 单条规范化路径/模式缓冲区容量（字节）
pub const FILTER_PATH_BUF_CAP: usize = 256;

/// 最大路径规范化段数
const FILTER_MAX_PATH_SEGS: usize = 64;

/// 内部拼接原始路径缓冲区容量
const FILTER_RAW_BUF_CAP: usize = 512;

/// `--filter` 参数解析与格式化错误集
pub err FilterArgError {
    EmptyPattern,
    TooManyPatterns,
    PathTooLong,
    BufferTooSmall,
    InvalidWorkspacePattern,
}

/// 过滤模式类型：包名模式或路径模式
pub enum FilterPatternKind {
    name,
    path,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn copySlice(dst: []u8, src: []const u8): usize! {
    if (src.len > dst.len) {
        return FilterArgError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        dst[i] = src[i];
        i += 1;
    }
    return src.len;
}

/// 判断目录名是否属于 Bun `--filter` 遍历默认跳过列表（`.`、`node_modules`、`.git`）
pub fn isFilterSkippedDir(segment: []const u8): bool {
    if (segment.len == 0) {
        return false;
    }
    if (bytesEql(segment, ".")) {
        return true;
    }
    if (bytesEql(segment, "node_modules")) {
        return true;
    }
    if (bytesEql(segment, ".git")) {
        return true;
    }
    return false;
}

/// 将 `workspaces` 条目规范化为候选 `package.json` 匹配模式（如 `packages/*` -> `packages/*/package.json`）。
/// 去除尾部多余 `/` 或 `\`；空模式返回 `InvalidWorkspacePattern`。
pub fn formatWorkspacePackagePattern(workspace_pattern: []const u8, out: []u8): usize! {
    if (workspace_pattern.len == 0) {
        return FilterArgError.InvalidWorkspacePattern;
    }
    let end = workspace_pattern.len;
    while (end > 0 && (workspace_pattern[end - 1] == '/' || workspace_pattern[end - 1] == '\\')) {
        end -= 1;
    }
    const suffix = "/package.json";
    if (end == 0) {
        return copySlice(out, "package.json");
    }
    const total = end + suffix.len;
    if (total > out.len) {
        return FilterArgError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < end) {
        const c = workspace_pattern[i];
        out[i] = if (c == '\\') '/' else c;
        i += 1;
    }
    let j: usize = 0;
    while (j < suffix.len) {
        out[end + j] = suffix[j];
        j += 1;
    }
    return total;
}

/// 内部路径段切片视图
struct SegSpan {
    start: usize = 0,
    end: usize = 0,
}

/// 将输入路径规范化并写入 `out`：
/// - 将所有 `\` 转换为 `/`；
/// - 若 `rel` 为相对路径（不以 `/` 开头）且 `cwd` 非空，先拼接 `cwd`；
/// - 折叠 `.` 与 `..` 段、消除重复 `/`、去除非根目录尾部 `/`。
pub fn normalizeFilterPath(cwd: []const u8, rel: []const u8, out: []u8): usize! {
    let raw: [FILTER_RAW_BUF_CAP]u8 = [0; FILTER_RAW_BUF_CAP];
    let raw_len: usize = 0;

    const rel_is_abs = rel.len > 0 && (rel[0] == '/' || rel[0] == '\\');
    let is_abs = rel_is_abs;

    if (!rel_is_abs && cwd.len > 0) {
        if (cwd[0] == '/' || cwd[0] == '\\') {
            is_abs = true;
        }
        if (cwd.len + 1 + rel.len > raw.len) {
            return FilterArgError.PathTooLong;
        }
        let i: usize = 0;
        while (i < cwd.len) {
            raw[raw_len] = if (cwd[i] == '\\') '/' else cwd[i];
            raw_len += 1;
            i += 1;
        }
        if (raw_len > 0 && raw[raw_len - 1] != '/') {
            raw[raw_len] = '/';
            raw_len += 1;
        }
    } else {
        if (rel.len > raw.len) {
            return FilterArgError.PathTooLong;
        }
    }

    let k: usize = 0;
    while (k < rel.len) {
        raw[raw_len] = if (rel[k] == '\\') '/' else rel[k];
        raw_len += 1;
        k += 1;
    }

    let segs: [FILTER_MAX_PATH_SEGS]SegSpan = [SegSpan{}; FILTER_MAX_PATH_SEGS];
    let seg_count: usize = 0;

    let pos: usize = 0;
    while (pos < raw_len) {
        while (pos < raw_len && raw[pos] == '/') {
            pos += 1;
        }
        if (pos >= raw_len) {
            break;
        }
        const seg_start = pos;
        while (pos < raw_len && raw[pos] != '/') {
            pos += 1;
        }
        const seg_end = pos;
        const seg = raw[seg_start..seg_end];

        if (seg.len == 1 && seg[0] == '.') {
            continue;
        }
        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
            if (seg_count > 0) {
                const prev = segs[seg_count - 1];
                const prev_slice = raw[prev.start..prev.end];
                if (!(prev_slice.len == 2 && prev_slice[0] == '.' && prev_slice[1] == '.')) {
                    seg_count -= 1;
                    continue;
                }
            }
            if (is_abs) {
                // 绝对路径越过根目录时夹紧在 `/`
                continue;
            }
        }

        if (seg_count >= FILTER_MAX_PATH_SEGS) {
            return FilterArgError.PathTooLong;
        }
        segs[seg_count] = SegSpan{ start: seg_start, end: seg_end };
        seg_count += 1;
    }

    let out_pos: usize = 0;
    if (is_abs) {
        if (out_pos >= out.len) {
            return FilterArgError.BufferTooSmall;
        }
        out[out_pos] = '/';
        out_pos += 1;
    }

    if (seg_count == 0) {
        if (!is_abs) {
            if (out.len < 1) {
                return FilterArgError.BufferTooSmall;
            }
            out[0] = '.';
            return 1;
        }
        return out_pos;
    }

    let s: usize = 0;
    while (s < seg_count) {
        if (s > 0) {
            if (out_pos >= out.len) {
                return FilterArgError.BufferTooSmall;
            }
            out[out_pos] = '/';
            out_pos += 1;
        }
        const span = segs[s];
        const seg_len = span.end - span.start;
        if (out_pos + seg_len > out.len) {
            return FilterArgError.BufferTooSmall;
        }
        let c: usize = 0;
        while (c < seg_len) {
            out[out_pos + c] = raw[span.start + c];
            c += 1;
        }
        out_pos += seg_len;
        s += 1;
    }

    return out_pos;
}

/// 字符类 `[...]` 匹配结果
struct CharClassMatch {
    matched: bool = false,
    next_idx: usize = 0,
    valid: bool = false,
}

fn matchCharClass(pat: []const u8, start: usize, ch: u8): CharClassMatch {
    let i = start + 1;
    if (i >= pat.len) {
        return CharClassMatch{};
    }
    let negate = false;
    if (pat[i] == '!' || pat[i] == '^') {
        negate = true;
        i += 1;
    }
    let found = false;
    let first = true;
    while (i < pat.len) {
        if (pat[i] == ']' && !first) {
            return CharClassMatch{
                matched: if (negate) !found else found,
                next_idx: i + 1,
                valid: true,
            };
        }
        first = false;
        const c1 = pat[i];
        if (i + 2 < pat.len && pat[i + 1] == '-' && pat[i + 2] != ']') {
            const c2 = pat[i + 2];
            if (c1 <= c2) {
                if (ch >= c1 && ch <= c2) {
                    found = true;
                }
            } else {
                if (ch >= c2 && ch <= c1) {
                    found = true;
                }
            }
            i += 3;
        } else {
            if (ch == c1) {
                found = true;
            }
            i += 1;
        }
    }
    return CharClassMatch{};
}

/// 单路径段内的迭代式 Glob 匹配器（支持 `*`、`?`、`[...]`，线性时间无回溯爆炸）
fn matchSegmentGlob(pat: []const u8, seg: []const u8): bool {
    let pi: usize = 0;
    let si: usize = 0;
    let has_star = false;
    let star_p: usize = 0;
    let star_s: usize = 0;

    while (si < seg.len) {
        if (pi < pat.len && pat[pi] == '*') {
            while (pi < pat.len && pat[pi] == '*') {
                pi += 1;
            }
            has_star = true;
            star_p = pi;
            star_s = si;
            continue;
        }

        if (pi < pat.len && pat[pi] == '?') {
            pi += 1;
            si += 1;
            continue;
        }

        if (pi < pat.len && pat[pi] == '[') {
            const cc = matchCharClass(pat, pi, seg[si]);
            if (cc.valid) {
                if (cc.matched) {
                    pi = cc.next_idx;
                    si += 1;
                    continue;
                }
            } else if (seg[si] == '[') {
                pi += 1;
                si += 1;
                continue;
            }
        } else if (pi < pat.len && pat[pi] == seg[si]) {
            pi += 1;
            si += 1;
            continue;
        }

        if (has_star) {
            star_s += 1;
            si = star_s;
            pi = star_p;
            continue;
        }

        return false;
    }

    while (pi < pat.len && pat[pi] == '*') {
        pi += 1;
    }

    return pi == pat.len;
}

fn splitPathSegments(s: []const u8, out: []SegSpan): usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        while (i < s.len && s[i] == '/') {
            i += 1;
        }
        if (i >= s.len) {
            break;
        }
        const start = i;
        while (i < s.len && s[i] != '/') {
            i += 1;
        }
        if (count < out.len) {
            out[count] = SegSpan{ start: start, end: i };
            count += 1;
        }
    }
    return count;
}

/// 零堆分配迭代式 Glob 匹配器：
/// - 支持 `*`（单段通配）、`**`（跨目录任意层通配，含零层）、`?`（单字符通配）、`[abc]`/`[a-z]`/`[!a-z]` 字符类；
/// - 保证线性/多项式有界复杂度，避免畸形多星号导致的指数回溯。
pub fn matchFilterGlob(pattern: []const u8, target: []const u8): bool {
    if (pattern.len == 0) {
        return target.len == 0;
    }
    if (bytesEql(pattern, "**")) {
        return true;
    }

    const pat_abs = pattern[0] == '/';
    const tgt_abs = target.len > 0 && target[0] == '/';
    if (pat_abs != tgt_abs) {
        // 允许前导 `**/` 跨绝对路径前缀匹配，否则绝对/相对属性必须一致
        const starts_with_ds = pattern.len >= 3 && pattern[0] == '*' && pattern[1] == '*' && pattern[2] == '/';
        if (!starts_with_ds) {
            return false;
        }
    }

    let p_segs: [FILTER_MAX_PATH_SEGS]SegSpan = [SegSpan{}; FILTER_MAX_PATH_SEGS];
    let t_segs: [FILTER_MAX_PATH_SEGS]SegSpan = [SegSpan{}; FILTER_MAX_PATH_SEGS];
    const p_count = splitPathSegments(pattern, p_segs[0..]);
    const t_count = splitPathSegments(target, t_segs[0..]);

    if (p_count == 0 && t_count == 0) {
        return true;
    }

    let pi: usize = 0;
    let ti: usize = 0;
    let has_ds = false;
    let ds_p: usize = 0;
    let ds_t: usize = 0;

    while (ti < t_count) {
        if (pi < p_count) {
            const p_seg = pattern[p_segs[pi].start..p_segs[pi].end];
            if (bytesEql(p_seg, "**")) {
                while (pi < p_count && bytesEql(pattern[p_segs[pi].start..p_segs[pi].end], "**")) {
                    pi += 1;
                }
                has_ds = true;
                ds_p = pi;
                ds_t = ti;
                continue;
            }
            const t_seg = target[t_segs[ti].start..t_segs[ti].end];
            if (matchSegmentGlob(p_seg, t_seg)) {
                pi += 1;
                ti += 1;
                continue;
            }
        }

        if (has_ds) {
            ds_t += 1;
            ti = ds_t;
            pi = ds_p;
            continue;
        }

        return false;
    }

    while (pi < p_count && bytesEql(pattern[p_segs[pi].start..p_segs[pi].end], "**")) {
        pi += 1;
    }

    return pi == p_count;
}

/// 单条已编译过滤规则（值语义，零堆分配）
pub struct FilterRule {
    kind: FilterPatternKind = FilterPatternKind.name,
    negated: bool = false,
    buf: [FILTER_PATH_BUF_CAP]u8 = [0; FILTER_PATH_BUF_CAP],
    len: usize = 0,

    fn pattern(): []const u8 {
        return _.buf[0.._.len];
    }

    fn isPath(): bool {
        return _.kind == FilterPatternKind.path;
    }

    fn isNegated(): bool {
        return _.negated;
    }
}

/// Monorepo `--filter` 规则集合（对应 Bun `FilterSet`，扩展支持 `!` 排除规则）
pub struct FilterSet {
    rules: [FILTER_MAX_PATTERNS]FilterRule = [FilterRule{}; FILTER_MAX_PATTERNS],
    rule_count: usize = 0,
    has_name_filters: bool = false,
    has_path_filters: bool = false,
    has_positive_filters: bool = false,
    has_negated_filters: bool = false,
    match_all: bool = false,

    static fn init(): FilterSet {
        return FilterSet{};
    }

    /// 添加一条 `--filter` 规则（支持 `!` 前缀排除、`*`/`**` 全匹配、`.`/`/` 路径规范化与名称通配）
    fn addPattern(*_, raw_pattern: []const u8, cwd: []const u8)! {
        if (raw_pattern.len == 0) {
            return FilterArgError.EmptyPattern;
        }

        let negated = false;
        let pat = raw_pattern;
        if (pat[0] == '!') {
            negated = true;
            pat = pat[1..];
            if (pat.len == 0) {
                return FilterArgError.EmptyPattern;
            }
        }

        if (!negated && (bytesEql(pat, "*") || bytesEql(pat, "**"))) {
            _.match_all = true;
            _.has_positive_filters = true;
            return;
        }

        if (_.rule_count >= FILTER_MAX_PATTERNS) {
            return FilterArgError.TooManyPatterns;
        }

        const is_path = pat[0] == '.' || pat[0] == '/' || pat[0] == '\\';
        let rule = FilterRule{
            kind: if (is_path) FilterPatternKind.path else FilterPatternKind.name,
            negated: negated,
        };

        if (is_path) {
            const norm_len = normalizeFilterPath(cwd, pat, rule.buf[0..])?;
            rule.len = norm_len;
            _.has_path_filters = true;
        } else {
            if (pat.len > FILTER_PATH_BUF_CAP) {
                return FilterArgError.PathTooLong;
            }
            let i: usize = 0;
            while (i < pat.len) {
                rule.buf[i] = pat[i];
                i += 1;
            }
            rule.len = pat.len;
            _.has_name_filters = true;
        }

        if (negated) {
            _.has_negated_filters = true;
        } else {
            _.has_positive_filters = true;
        }

        _.rules[_.rule_count] = rule;
        _.rule_count += 1;
    }

    /// 批量从 `--filter` 参数列表构建 `FilterSet`
    static fn fromPatterns(patterns: []const []const u8, cwd: []const u8): FilterSet! {
        let fs = FilterSet.init();
        let i: usize = 0;
        while (i < patterns.len) {
            fs.addPattern(patterns[i], cwd)?;
            i += 1;
        }
        return fs;
    }

    fn count(): usize {
        return _.rule_count;
    }

    fn ruleAt(idx: usize): FilterRule? {
        if (idx >= _.rule_count) {
            return null;
        }
        return _.rules[idx];
    }

    /// 仅按路径规则判断是否命中（支持排除规则）
    fn matchesPath(pkg_path: []const u8): bool {
        let norm_buf: [FILTER_PATH_BUF_CAP]u8 = [0; FILTER_PATH_BUF_CAP];
        let target_path = pkg_path;
        if (pkg_path.len > 0) {
            if (n := normalizeFilterPath("", pkg_path, norm_buf[0..]) catch null) {
                target_path = norm_buf[0..n];
            }
        }

        let positive = false;
        if (!_.has_positive_filters && _.has_negated_filters) {
            positive = target_path.len > 0;
        } else {
            let i: usize = 0;
            while (i < _.rule_count) {
                const r = _.rules[i];
                if (!r.negated && r.kind == FilterPatternKind.path) {
                    if (matchFilterGlob(r.pattern(), target_path)) {
                        positive = true;
                        break;
                    }
                }
                i += 1;
            }
        }

        if (!positive) {
            return false;
        }

        if (_.has_negated_filters) {
            let j: usize = 0;
            while (j < _.rule_count) {
                const r = _.rules[j];
                if (r.negated && r.kind == FilterPatternKind.path) {
                    if (matchFilterGlob(r.pattern(), target_path)) {
                        return false;
                    }
                }
                j += 1;
            }
        }

        return true;
    }

    /// 综合评估候选包（路径 + 名称）是否满足当前 `FilterSet`：
    /// - `match_all` 对非空包名默认命中（同时允许路径过滤规则匹配无名包）；
    /// - 若仅指定了 `!` 排除规则（如 `--filter '!pkg-c'`），默认全选后剔除被排除项；
    /// - 任意一条 `!` 排除规则命中即返回 `false`。
    fn matches(pkg_path: []const u8, pkg_name: []const u8): bool {
        let norm_buf: [FILTER_PATH_BUF_CAP]u8 = [0; FILTER_PATH_BUF_CAP];
        let target_path = pkg_path;
        if (pkg_path.len > 0) {
            if (n := normalizeFilterPath("", pkg_path, norm_buf[0..]) catch null) {
                target_path = norm_buf[0..n];
            }
        }

        let positive = false;
        if (_.match_all && pkg_name.len > 0) {
            positive = true;
        } else if (!_.has_positive_filters && _.has_negated_filters) {
            positive = pkg_name.len > 0 || target_path.len > 0;
        } else {
            let i: usize = 0;
            while (i < _.rule_count) {
                const r = _.rules[i];
                if (!r.negated) {
                    const target = if (r.kind == FilterPatternKind.name) pkg_name else target_path;
                    if (target.len > 0 && matchFilterGlob(r.pattern(), target)) {
                        positive = true;
                        break;
                    }
                }
                i += 1;
            }
        }

        if (!positive) {
            return false;
        }

        if (_.has_negated_filters) {
            let j: usize = 0;
            while (j < _.rule_count) {
                const r = _.rules[j];
                if (r.negated) {
                    const target = if (r.kind == FilterPatternKind.name) pkg_name else target_path;
                    if (target.len > 0 && matchFilterGlob(r.pattern(), target)) {
                        return false;
                    }
                }
                j += 1;
            }
        }

        return true;
    }
}

/// 格式化 Foreman 风格多包脚本输出前缀（参考 `/data/dev/bun/docs/pm/filter.mdx`）：
/// - 有包名 → `<pkg_name>:<script_name>`；
/// - 无包名但有相对路径 → `<rel_path>:<script_name>`；
/// - 均为空 → `<script_name>`。
pub fn formatFilterScriptPrefix(
    pkg_name: []const u8,
    rel_path: []const u8,
    script_name: []const u8,
    out: []u8,
): usize! {
    const label = if (pkg_name.len > 0) pkg_name else rel_path;
    if (label.len == 0) {
        return copySlice(out, script_name);
    }
    const total = label.len + 1 + script_name.len;
    if (total > out.len) {
        return FilterArgError.BufferTooSmall;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < label.len) {
        out[pos] = label[i];
        pos += 1;
        i += 1;
    }
    out[pos] = ':';
    pos += 1;
    let j: usize = 0;
    while (j < script_name.len) {
        out[pos] = script_name[j];
        pos += 1;
        j += 1;
    }
    return pos;
}
