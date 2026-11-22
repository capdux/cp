// Source: Absorbed from /data/dev/bun/src/cli/test/Scanner.cp | Synced: 2026-11-22T05:46:07Z
// ============================================================================
// Pure-CP Zero-Allocation Test File Discovery, Filtering & Directory Pruning Engine
// Absorbed from Bun: src/cli/test/Scanner.cp & docs/test/discovery.mdx
// ============================================================================

/// Bun 默认测试文件名后缀常量
pub const TEST_SUFFIX_DOT_TEST: []const u8 = ".test";
pub const TEST_SUFFIX_UNDER_TEST: []const u8 = "_test";
pub const TEST_SUFFIX_DOT_SPEC: []const u8 = ".spec";
pub const TEST_SUFFIX_UNDER_SPEC: []const u8 = "_spec";

/// 默认跳过扫描的依赖目录名
pub const TEST_DEFAULT_IGNORE_DIR: []const u8 = "node_modules";

/// 路径拼接栈缓冲区上限（POSIX PATH_MAX = 4096）
pub const TEST_DISCOVERY_MAX_PATH: usize = 4096;

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

fn sliceEql(a: []const u8, b: []const u8): bool {
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

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len > haystack.len) {
        return false;
    }
    return sliceEql(haystack[0..prefix.len], prefix);
}

fn endsWithIgnoreCase(haystack: []const u8, suffix: []const u8): bool {
    if (suffix.len > haystack.len) {
        return false;
    }
    return eqIgnoreCaseAscii(haystack[haystack.len - suffix.len ..], suffix);
}

fn containsSlice(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    const limit = haystack.len - needle.len;
    while (i <= limit) {
        if (sliceEql(haystack[i .. i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn basenameOf(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    let i = end;
    while (i > 0) {
        const ch = path[i - 1];
        if (ch == '/' || ch == '\\') {
            return path[i..end];
        }
        i -= 1;
    }
    return path[0..end];
}

fn extensionOf(base: []const u8): []const u8 {
    if (base.len == 0) {
        return "";
    }
    let i = base.len;
    while (i > 0) {
        i -= 1;
        if (base[i] == '.') {
            if (i == 0) {
                return "";
            }
            return base[i..];
        }
    }
    return "";
}

/// 判断扩展名是否为受支持的测试脚本扩展名（覆盖 Bun JS/TS/JSX/TSX/MJS/CJS/MTS/CTS 及 CP 原生 `.cp`）
pub fn isSupportedTestExt(ext: []const u8): bool {
    if (ext.len < 3) {
        return false;
    }
    return eqIgnoreCaseAscii(ext, ".cp") ||
        eqIgnoreCaseAscii(ext, ".js") ||
        eqIgnoreCaseAscii(ext, ".jsx") ||
        eqIgnoreCaseAscii(ext, ".ts") ||
        eqIgnoreCaseAscii(ext, ".tsx") ||
        eqIgnoreCaseAscii(ext, ".mjs") ||
        eqIgnoreCaseAscii(ext, ".cjs") ||
        eqIgnoreCaseAscii(ext, ".mts") ||
        eqIgnoreCaseAscii(ext, ".cts");
}

/// 严格对齐 Bun `Scanner.cp`：判断路径是否匹配 `.test` / `_test` / `.spec` / `_spec` 四类标准后缀
pub fn hasBunTestSuffix(path: []const u8): bool {
    const base = basenameOf(path);
    const ext = extensionOf(base);
    if (!isSupportedTestExt(ext)) {
        return false;
    }
    const stem = base[0 .. base.len - ext.len];
    return endsWithIgnoreCase(stem, TEST_SUFFIX_DOT_TEST) ||
        endsWithIgnoreCase(stem, TEST_SUFFIX_UNDER_TEST) ||
        endsWithIgnoreCase(stem, TEST_SUFFIX_DOT_SPEC) ||
        endsWithIgnoreCase(stem, TEST_SUFFIX_UNDER_SPEC);
}

/// 判断路径是否为有效测试文件名（包含 Bun 四类标准后缀以及可选 CP 原生 `test_*.cp` 前缀约定）
pub fn hasTestNameSuffix(path: []const u8, include_cp_prefix: bool): bool {
    if (hasBunTestSuffix(path)) {
        return true;
    }
    if (!include_cp_prefix) {
        return false;
    }
    const base = basenameOf(path);
    const ext = extensionOf(base);
    if (!eqIgnoreCaseAscii(ext, ".cp")) {
        return false;
    }
    const stem = base[0 .. base.len - ext.len];
    if (stem.len <= 5) {
        return false;
    }
    return eqIgnoreCaseAscii(stem[0..5], "test_");
}

/// 判断位置参数是否为显式路径（以 `./`、`../` 或 `/` 开头，对齐 Bun `docs/test/discovery.mdx`）
pub fn isExplicitPathArg(arg: []const u8): bool {
    if (arg.len == 0) {
        return false;
    }
    if (arg[0] == '/') {
        return true;
    }
    if (startsWith(arg, "./") || startsWith(arg, "../")) {
        return true;
    }
    return false;
}

/// 判断目录名是否应默认跳过（隐藏目录 `.xxx`、`node_modules` 或命中自定义排除列表）
pub fn shouldSkipDirectory(dir_name: []const u8, exclusion_names: []const ([]const u8)): bool {
    if (dir_name.len == 0) {
        return false;
    }
    if (dir_name[0] == '.') {
        return true;
    }
    if (sliceEql(dir_name, TEST_DEFAULT_IGNORE_DIR)) {
        return true;
    }
    let i: usize = 0;
    while (i < exclusion_names.len) {
        if (sliceEql(dir_name, exclusion_names[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn matchGlobCore(pattern: []const u8, text: []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = 0;
    let star_ti: usize = 0;
    let has_star = false;
    let star_crosses_slash = false;

    while (ti < text.len) {
        if (pi + 1 < pattern.len && pattern[pi] == '*' && pattern[pi + 1] == '*') {
            while (pi < pattern.len && pattern[pi] == '*') {
                pi += 1;
            }
            // Handle optional trailing slash after "**" so "a/**/b" matches "a/b"
            if (pi < pattern.len && pattern[pi] == '/') {
                if (matchGlobCore(pattern[pi + 1 ..], text[ti..])) {
                    return true;
                }
            }
            star_pi = pi;
            star_ti = ti;
            has_star = true;
            star_crosses_slash = true;
        } else if (pi < pattern.len && pattern[pi] == '*') {
            pi += 1;
            star_pi = pi;
            star_ti = ti;
            has_star = true;
            star_crosses_slash = false;
        } else if (pi < pattern.len && pattern[pi] == '?' && text[ti] != '/') {
            pi += 1;
            ti += 1;
        } else if (pi < pattern.len && pattern[pi] == text[ti]) {
            pi += 1;
            ti += 1;
        } else if (has_star) {
            if (!star_crosses_slash && text[star_ti] == '/') {
                return false;
            }
            star_ti += 1;
            ti = star_ti;
            pi = star_pi;
        } else {
            return false;
        }
    }

    while (pi < pattern.len) {
        if (pattern[pi] == '*') {
            pi += 1;
        } else if (pi + 1 < pattern.len && pattern[pi] == '/' && pattern[pi + 1] == '*') {
            break;
        } else {
            break;
        }
    }
    return pi == pattern.len;
}

/// 零堆分配路径忽略通配匹配（对齐 Bun `Scanner.matchesPathIgnorePattern` 语义）：
/// - 支持 `**`（跨目录）、`*`（单目录段）、`?`（单字符非 `/`）；
/// - 仅当 `pattern` 含 `**` 时追加末尾 `'/'` 重试，确保 `vendor/**` 可提前剪枝 `vendor` 目录，
///   而单星号 `vendor/*` 不会错误剪枝整个 `vendor` 目录树。
pub fn matchesDiscoveryGlob(pattern: []const u8, rel_path: []const u8): bool {
    if (pattern.len == 0) {
        return false;
    }
    if (matchGlobCore(pattern, rel_path)) {
        return true;
    }
    if (rel_path.len > 0 &&
        rel_path.len + 1 <= TEST_DISCOVERY_MAX_PATH &&
        rel_path[rel_path.len - 1] != '/' &&
        containsSlice(pattern, "**"))
    {
        let buf: [4096]u8 = [0; 4096];
        let i: usize = 0;
        while (i < rel_path.len) {
            buf[i] = rel_path[i];
            i += 1;
        }
        buf[rel_path.len] = '/';
        if (matchGlobCore(pattern, buf[0 .. rel_path.len + 1])) {
            return true;
        }
    }
    return false;
}

/// 零堆分配测试文件发现与过滤策略引擎（对齐 Bun `src/cli/test/Scanner.cp`）
pub struct TestDiscoveryFilter {
    exclusion_names: []const ([]const u8),
    filter_names: []const ([]const u8),
    path_ignore_patterns: []const ([]const u8),
    include_cp_prefix: bool = true,

    /// 判断候选文件是否满足扩展名与（可选）测试后缀要求
    fn couldBeTestFile(path: []const u8, needs_test_suffix: bool): bool {
        const base = basenameOf(path);
        const ext = extensionOf(base);
        if (!isSupportedTestExt(ext)) {
            return false;
        }
        if (!needs_test_suffix) {
            return true;
        }
        return hasTestNameSuffix(base, _.include_cp_prefix);
    }

    /// 绝对路径前缀过滤（空过滤表放行全部）
    fn doesAbsolutePathMatchFilter(abs_path: []const u8): bool {
        if (_.filter_names.len == 0) {
            return true;
        }
        let i: usize = 0;
        while (i < _.filter_names.len) {
            if (startsWith(abs_path, _.filter_names[i])) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 相对路径子串过滤（空过滤表放行全部，对齐 Bun 位置参数过滤语义）
    fn doesPathMatchFilter(rel_path: []const u8): bool {
        if (_.filter_names.len == 0) {
            return true;
        }
        let i: usize = 0;
        while (i < _.filter_names.len) {
            if (containsSlice(rel_path, _.filter_names[i])) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 判断相对路径是否命中任一 `path_ignore_patterns` 忽略规则
    fn matchesPathIgnorePattern(rel_path: []const u8): bool {
        if (_.path_ignore_patterns.len == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.path_ignore_patterns.len) {
            if (matchesDiscoveryGlob(_.path_ignore_patterns[i], rel_path)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 目录遍历剪枝决策：隐藏目录 / `node_modules` / 自定义排除目录 / 命中 `**` 忽略模式均提前剪枝
    fn shouldPruneDirectory(dir_name: []const u8, rel_dir_path: []const u8): bool {
        if (shouldSkipDirectory(dir_name, _.exclusion_names)) {
            return true;
        }
        return _.matchesPathIgnorePattern(rel_dir_path);
    }

    /// 综合文件过滤决策（对齐 Bun `Scanner.next` 文件分支逻辑）
    fn shouldRunFile(abs_path: []const u8, rel_path: []const u8, needs_test_suffix: bool): bool {
        if (!_.couldBeTestFile(rel_path, needs_test_suffix)) {
            return false;
        }
        if (!_.doesAbsolutePathMatchFilter(abs_path)) {
            if (!_.doesPathMatchFilter(rel_path)) {
                return false;
            }
        }
        if (_.matchesPathIgnorePattern(rel_path)) {
            return false;
        }
        return true;
    }
}
