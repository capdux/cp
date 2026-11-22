// Source: Absorbed from /data/dev/bun/src/cli/test/Scanner.cp | Synced: 2026-11-22T08:47:00Z
// Ported from Bun: /data/dev/bun/src/cli/test/Scanner.cp & /data/dev/bun/docs/test/discovery.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation `bun test` / `cap test` file discovery filter:
// `.test`/`_test`/`_spec`/`.spec` suffix & extension matcher, hidden/`node_modules` directory pruning,
// substring/prefix positional filters, `**` directory-aware `path_ignore_patterns`, and describe-chain test name formatter.

import { matchFilterGlob } from "./filterArg.cp";

/// 最大位置过滤词数量
pub const TEST_SCANNER_MAX_FILTERS: usize = 32;

/// 最大排除目录名数量
pub const TEST_SCANNER_MAX_EXCLUSIONS: usize = 32;

/// 最大相对路径忽略 Glob 模式数量
pub const TEST_SCANNER_MAX_IGNORE_PATTERNS: usize = 32;

/// 测试文件扫描过滤器错误集
pub err TestScannerError {
    EmptyPattern,
    TooManyFilters,
    TooManyExclusions,
    TooManyIgnorePatterns,
    BufferTooSmall,
}

/// 判断文件扩展名是否属于测试运行器支持的源码扩展名（大小写不敏感）
pub fn isTestSupportedExtension(ext: []const u8): bool {
    return asciiEqlIgnoreCase(ext, ".js") ||
        asciiEqlIgnoreCase(ext, ".jsx") ||
        asciiEqlIgnoreCase(ext, ".ts") ||
        asciiEqlIgnoreCase(ext, ".tsx") ||
        asciiEqlIgnoreCase(ext, ".mjs") ||
        asciiEqlIgnoreCase(ext, ".cjs") ||
        asciiEqlIgnoreCase(ext, ".mts") ||
        asciiEqlIgnoreCase(ext, ".cts") ||
        asciiEqlIgnoreCase(ext, ".cp");
}

/// 判断文件名（去除目录前缀和扩展名后）是否带有 `.test` / `_test` / `.spec` / `_spec` 后缀
pub fn hasTestNameSuffix(path_or_name: []const u8): bool {
    const base = pathBasename(path_or_name);
    const ext = pathExtension(base);
    if (ext.len == 0 || ext.len >= base.len) {
        return false;
    }
    const stem = base[0..base.len - ext.len];
    return asciiEndsWithIgnoreCase(stem, ".test") ||
        asciiEndsWithIgnoreCase(stem, "_test") ||
        asciiEndsWithIgnoreCase(stem, ".spec") ||
        asciiEndsWithIgnoreCase(stem, "_spec");
}

/// 判断文件是否可作为测试文件加载（对齐 Bun `Scanner.couldBeTestFile`）
pub fn couldBeTestFile(path_or_name: []const u8, needs_test_suffix: bool): bool {
    const base = pathBasename(path_or_name);
    if (base.len == 0) {
        return false;
    }
    const ext = pathExtension(base);
    if (ext.len == 0 || !isTestSupportedExtension(ext)) {
        return false;
    }
    if (!needs_test_suffix) {
        return true;
    }
    return hasTestNameSuffix(base);
}

/// 判断位置参数是否为精确文件路径（以 `./`、`../`、`/` 或 `.\` 开头，区别于子串过滤词）
pub fn isExactTestPathArg(arg: []const u8): bool {
    if (arg.len == 0) {
        return false;
    }
    if (arg[0] == '/' || arg[0] == '\\') {
        return true;
    }
    if (arg.len >= 2 && arg[0] == '.' && (arg[1] == '/' || arg[1] == '\\')) {
        return true;
    }
    if (arg.len >= 3 && arg[0] == '.' && arg[1] == '.' && (arg[2] == '/' || arg[2] == '\\')) {
        return true;
    }
    return false;
}

/// 拼接外层 `describe` 标签链与测试名（空格分隔，供 `-t` / `--test-name-pattern` 匹配）
pub fn formatScopedTestName(
    describe_labels: []const []const u8,
    test_name: []const u8,
    out: []u8,
): usize! {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < describe_labels.len) {
        const lbl = describe_labels[i];
        if (lbl.len > 0) {
            if (pos > 0) {
                pos = appendByte(out, pos, ' ')?;
            }
            pos = appendBytes(out, pos, lbl)?;
        }
        i += 1;
    }
    if (test_name.len > 0) {
        if (pos > 0) {
            pos = appendByte(out, pos, ' ')?;
        }
        pos = appendBytes(out, pos, test_name)?;
    }
    return pos;
}

/// 零堆分配测试文件发现过滤器（对齐 Bun `cli/test/Scanner.cp`）
pub struct TestFileScanner {
    filter_names: [TEST_SCANNER_MAX_FILTERS]([]const u8) = [""; TEST_SCANNER_MAX_FILTERS],
    filter_count: usize = 0,
    exclusion_names: [TEST_SCANNER_MAX_EXCLUSIONS]([]const u8) = [""; TEST_SCANNER_MAX_EXCLUSIONS],
    exclusion_count: usize = 0,
    path_ignore_patterns: [TEST_SCANNER_MAX_IGNORE_PATTERNS]([]const u8) = [""; TEST_SCANNER_MAX_IGNORE_PATTERNS],
    ignore_count: usize = 0,

    static fn init(): TestFileScanner {
        return TestFileScanner{};
    }

    fn addFilter(*_, pattern: []const u8)! {
        if (pattern.len == 0) {
            return TestScannerError.EmptyPattern;
        }
        if (_.filter_count >= TEST_SCANNER_MAX_FILTERS) {
            return TestScannerError.TooManyFilters;
        }
        _.filter_names[_.filter_count] = pattern;
        _.filter_count += 1;
    }

    fn addExclusionDir(*_, dir_name: []const u8)! {
        if (dir_name.len == 0) {
            return TestScannerError.EmptyPattern;
        }
        if (_.exclusion_count >= TEST_SCANNER_MAX_EXCLUSIONS) {
            return TestScannerError.TooManyExclusions;
        }
        _.exclusion_names[_.exclusion_count] = dir_name;
        _.exclusion_count += 1;
    }

    fn addPathIgnorePattern(*_, glob_pattern: []const u8)! {
        if (glob_pattern.len == 0) {
            return TestScannerError.EmptyPattern;
        }
        if (_.ignore_count >= TEST_SCANNER_MAX_IGNORE_PATTERNS) {
            return TestScannerError.TooManyIgnorePatterns;
        }
        _.path_ignore_patterns[_.ignore_count] = glob_pattern;
        _.ignore_count += 1;
    }

    /// 判断路径是否命中正向过滤词（无过滤词时默认全匹配；兼容绝对路径前缀与相对路径子串）
    fn matchesFilter(abs_or_rel_path: []const u8, rel_path: []const u8): bool {
        if (_.filter_count == 0) {
            return true;
        }
        let i: usize = 0;
        while (i < _.filter_count) {
            const f = _.filter_names[i];
            if (startsWith(abs_or_rel_path, f) || containsBytes(rel_path, f)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// 判断相对路径是否命中 `path_ignore_patterns`（含目录剪枝 `**` 语义）
    fn matchesPathIgnore(rel_path: []const u8, is_dir: bool): bool {
        if (_.ignore_count == 0) {
            return false;
        }
        let i: usize = 0;
        while (i < _.ignore_count) {
            const pat = _.path_ignore_patterns[i];
            if (is_dir) {
                // 对齐 Bun Scanner.cp：仅对精确命中或含 `**` 的模式执行目录剪枝，
                // 单 `*` 模式（如 `vendor/*`）不提前剪枝子目录。
                if (containsBytes(pat, "**")) {
                    if (matchFilterGlob(pat, rel_path)) {
                        return true;
                    }
                    // 同时尝试 `<rel_path>/_` 探测 `vendor/**` 是否覆盖其全部子项
                    let probe: [512]u8 = [0; 512];
                    if (rel_path.len + 2 <= probe.len) {
                        let k: usize = 0;
                        while (k < rel_path.len) {
                            probe[k] = rel_path[k];
                            k += 1;
                        }
                        if (rel_path.len > 0 && rel_path[rel_path.len - 1] != '/') {
                            probe[k] = '/';
                            k += 1;
                        }
                        probe[k] = '_';
                        k += 1;
                        if (matchFilterGlob(pat, probe[0..k])) {
                            return true;
                        }
                    }
                } else if (!containsBytes(pat, "*") && matchFilterGlob(pat, rel_path)) {
                    return true;
                }
            } else {
                if (matchFilterGlob(pat, rel_path)) {
                    return true;
                }
            }
            i += 1;
        }
        return false;
    }

    /// 判断目录是否应进入递归扫描（跳过隐藏目录、`node_modules`、`exclusion_names` 及忽略子树）
    fn shouldScanDir(dir_name: []const u8, rel_dir_path: []const u8): bool {
        if (dir_name.len == 0) {
            return false;
        }
        if (dir_name[0] == '.' || bytesEql(dir_name, "node_modules")) {
            return false;
        }
        let i: usize = 0;
        while (i < _.exclusion_count) {
            if (bytesEql(_.exclusion_names[i], dir_name)) {
                return false;
            }
            i += 1;
        }
        if (rel_dir_path.len > 0 && _.matchesPathIgnore(rel_dir_path, true)) {
            return false;
        }
        return true;
    }

    /// 完整测试文件发现判定：扩展名/后缀校验 && 正向过滤词 && 非忽略路径
    fn shouldIncludeFile(
        abs_or_rel_path: []const u8,
        rel_path: []const u8,
        needs_test_suffix: bool,
    ): bool {
        if (!couldBeTestFile(rel_path, needs_test_suffix)) {
            return false;
        }
        if (!_.matchesFilter(abs_or_rel_path, rel_path)) {
            return false;
        }
        if (_.matchesPathIgnore(rel_path, false)) {
            return false;
        }
        return true;
    }
}

// ============================================================================
// 内部零分配辅助函数
// ============================================================================

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn asciiEndsWithIgnoreCase(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) {
        return false;
    }
    return asciiEqlIgnoreCase(s[s.len - suffix.len..], suffix);
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    return bytesEql(s[0..prefix.len], prefix);
}

fn containsBytes(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (bytesEql(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn pathBasename(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    let i = end;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/' || path[i] == '\\') {
            return path[i + 1..end];
        }
    }
    return path[0..end];
}

fn pathExtension(base: []const u8): []const u8 {
    let i = base.len;
    while (i > 0) {
        i -= 1;
        if (base[i] == '.') {
            if (i == 0) {
                return "";
            }
            return base[i..base.len];
        }
    }
    return "";
}

fn appendByte(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) {
        return TestScannerError.BufferTooSmall;
    }
    out[pos] = b;
    return pos + 1;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return TestScannerError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}
