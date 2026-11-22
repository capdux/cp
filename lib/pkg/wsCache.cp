// Source: Absorbed from /data/dev/bun/src/install/PackageManager/WorkspacePackageJSONCache.cp | Synced: 2026-11-22T12:49:17Z
// Pure-CP zero-allocation Monorepo Workspace `package.json` Cache, Path Normalizer & Indentation/JSONC Analyzer
// Aligned with Bun src/install/PackageManager/WorkspacePackageJSONCache.cp & docs/pm/workspaces.mdx:
// - Enforces absolute path invariant and normalizes Windows '\' to POSIX '/' for cross-platform cache keys
// - Validates package.json top-level object structure with JSONC (comments + trailing commas) awareness
// - Automatically detects file indentation (tabs vs space width) for format-preserving package.json rewrites
// - Never pollutes cache on parse errors (mirrors Bun's `_.map.remove(key)` rollback on parse failure)
// - 100% zero heap allocation with O(1) swap-remove invalidation and cache hit/miss telemetry.

/// 最大缓存的 Workspace `package.json` 条目数
pub const WORKSPACE_PKG_CACHE_MAX_ENTRIES: usize = 32;

/// 规范化后的绝对路径最大长度
pub const WORKSPACE_PKG_PATH_MAX_LEN: usize = 256;

/// Workspace `package.json` 缓存与解析错误集
pub err WorkspacePkgCacheError {
    NotAbsolutePath,
    PathTooLong,
    BufferTooSmall,
    InvalidJsonRoot,
    CommentsNotAllowed,
    TrailingCommaNotAllowed,
    CacheFull,
}

/// 探测到的 JSON 缩进风格（对齐 Bun `JSPrinter.Options.Indentation`）
pub struct JsonIndentation {
    is_tabs: bool = false,
    width: u8 = 2,
    detected: bool = false,
}

/// `package.json` 解析与探测选项（对齐 Bun `GetJSONOptions` + `parsePackageJSONUTF8WithOpts`）
pub struct WorkspacePkgJsonOptions {
    allow_comments: bool = true,
    allow_trailing_commas: bool = true,
    guess_indentation: bool = true,
}

/// 单个 `package.json` 源码的零分配分析摘要
pub struct WorkspacePkgJsonSummary {
    content_hash: u64 = 0,
    byte_len: usize = 0,
    indentation: JsonIndentation = JsonIndentation{},
    has_comments: bool = false,
    has_trailing_comma: bool = false,
}

/// 缓存条目（以规范化 POSIX 绝对路径为键）
pub struct WorkspacePkgCacheEntry {
    path_buf: [WORKSPACE_PKG_PATH_MAX_LEN]u8 = [0; WORKSPACE_PKG_PATH_MAX_LEN],
    path_len: usize = 0,
    content_hash: u64 = 0,
    byte_len: usize = 0,
    indentation: JsonIndentation = JsonIndentation{},
    has_comments: bool = false,
    has_trailing_comma: bool = false,
    hit_count: u64 = 0,

    fn path(): []const u8 {
        return _.path_buf[0.._.path_len];
    }
}

/// 缓存统计快照
pub struct WorkspacePkgCacheStats {
    entry_count: usize = 0,
    hits: u64 = 0,
    misses: u64 = 0,
    parse_errors: u64 = 0,
    invalidations: u64 = 0,
}

fn wpIsAsciiAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn wpBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn wpFnv1a64(data: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < data.len) {
        h ^= (data[i] as u64);
        h = h *% 0x100000001b3;
        i += 1;
    }
    return h;
}

/// 判断路径是否为 POSIX 或 Windows 绝对路径
pub fn isAbsoluteWorkspacePath(path: []const u8): bool {
    if (path.len == 0) return false;
    if (path[0] == '/') return true;
    if (path.len >= 3 && wpIsAsciiAlpha(path[0]) && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
        return true;
    }
    return false;
}

/// 校验并规范化绝对路径：将 Windows `\` 转为 POSIX `/`，折叠多余连续斜杠并去除非根尾部斜杠
pub fn normalizeWorkspacePkgJsonPath(raw_path: []const u8, out: []u8): WorkspacePkgCacheError!usize {
    if (!isAbsoluteWorkspacePath(raw_path)) return err.NotAbsolutePath;
    if (raw_path.len > WORKSPACE_PKG_PATH_MAX_LEN) return err.PathTooLong;

    let pos: usize = 0;
    let prev_slash = false;
    let i: usize = 0;
    while (i < raw_path.len) {
        const c = if (raw_path[i] == '\\') ('/' as u8) else raw_path[i];
        if (c == '/') {
            if (!prev_slash) {
                if (pos >= out.len) return err.BufferTooSmall;
                out[pos] = '/';
                pos += 1;
                prev_slash = true;
            }
        } else {
            if (pos >= out.len) return err.BufferTooSmall;
            out[pos] = c;
            pos += 1;
            prev_slash = false;
        }
        i += 1;
    }

    // 保留根目录 "/" 或 "C:/"，其余移除末尾斜杠
    while (pos > 1 && out[pos - 1] == '/') {
        if (pos == 3 && out[1] == ':') break;
        pos -= 1;
    }
    return pos;
}

/// 自动探测 `package.json` 首个缩进行的缩进风格（Tab 或 1..8 空格）
pub fn guessJsonIndentation(source: []const u8): JsonIndentation {
    let i: usize = 0;
    while (i < source.len) {
        // 定位下一行行首
        while (i < source.len && source[i] != '\n') {
            i += 1;
        }
        if (i >= source.len) break;
        i += 1; // 跳过 '\n'

        // 统计本行前导空白
        let tabs: usize = 0;
        let spaces: usize = 0;
        while (i < source.len && (source[i] == ' ' || source[i] == '\t')) {
            if (source[i] == '\t') {
                tabs += 1;
            } else {
                spaces += 1;
            }
            i += 1;
        }
        if (i >= source.len || source[i] == '\n' || source[i] == '\r') {
            continue;
        }
        // 跳过纯注释行或闭合括号行
        if (source[i] == '}' || source[i] == ']') {
            continue;
        }
        if (source[i] == '/' && i + 1 < source.len && (source[i + 1] == '/' || source[i + 1] == '*')) {
            continue;
        }
        if (tabs > 0) {
            return JsonIndentation{ is_tabs: true, width: 1, detected: true };
        }
        if (spaces > 0) {
            const w = if (spaces > 8) (8 as u8) else (spaces as u8);
            return JsonIndentation{ is_tabs: false, width: w, detected: true };
        }
    }
    return JsonIndentation{ is_tabs: false, width: 2, detected: false };
}

fn wpIsSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/// 校验并分析 `package.json` 源码（支持 JSONC 注释与尾逗号、括号/字符串闭合校验及缩进探测）
pub fn analyzeWorkspacePackageJson(
    source: []const u8,
    opts: WorkspacePkgJsonOptions,
): WorkspacePkgCacheError!WorkspacePkgJsonSummary {
    let i: usize = 0;
    let has_comments = false;
    let has_trailing_comma = false;
    let seen_root_open = false;
    let seen_root_close = false;
    let depth: usize = 0;
    let last_sig: u8 = 0;

    // 跳过 UTF-8 BOM
    if (source.len >= 3 && source[0] == 0xEF && source[1] == 0xBB && source[2] == 0xBF) {
        i = 3;
    }

    while (i < source.len) {
        const c = source[i];
        if (wpIsSpace(c)) {
            i += 1;
            continue;
        }
        // 处理单行注释 //
        if (c == '/' && i + 1 < source.len && source[i + 1] == '/') {
            has_comments = true;
            if (!opts.allow_comments) return err.CommentsNotAllowed;
            i += 2;
            while (i < source.len && source[i] != '\n') {
                i += 1;
            }
            continue;
        }
        // 处理块注释 /* ... */
        if (c == '/' && i + 1 < source.len && source[i + 1] == '*') {
            has_comments = true;
            if (!opts.allow_comments) return err.CommentsNotAllowed;
            i += 2;
            let closed = false;
            while (i + 1 < source.len) {
                if (source[i] == '*' && source[i + 1] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) return err.InvalidJsonRoot;
            continue;
        }

        // 根闭合之后不允许再出现有效 token
        if (seen_root_close) return err.InvalidJsonRoot;

        if (!seen_root_open) {
            if (c != '{') return err.InvalidJsonRoot;
            seen_root_open = true;
            depth = 1;
            last_sig = '{';
            i += 1;
            continue;
        }

        // 字符串字面量
        if (c == '"') {
            i += 1;
            let str_closed = false;
            while (i < source.len) {
                const sc = source[i];
                if (sc == '\\') {
                    if (i + 1 >= source.len) return err.InvalidJsonRoot;
                    i += 2;
                    continue;
                }
                if (sc == '"') {
                    i += 1;
                    str_closed = true;
                    break;
                }
                if (sc < 0x20) return err.InvalidJsonRoot;
                i += 1;
            }
            if (!str_closed) return err.InvalidJsonRoot;
            last_sig = '"';
            continue;
        }

        if (c == '{' || c == '[') {
            depth += 1;
            last_sig = c;
            i += 1;
            continue;
        }
        if (c == '}' || c == ']') {
            if (last_sig == ',') {
                has_trailing_comma = true;
                if (!opts.allow_trailing_commas) return err.TrailingCommaNotAllowed;
            }
            if (depth == 0) return err.InvalidJsonRoot;
            depth -= 1;
            if (depth == 0) {
                if (c != '}') return err.InvalidJsonRoot;
                seen_root_close = true;
            }
            last_sig = c;
            i += 1;
            continue;
        }

        last_sig = c;
        i += 1;
    }

    if (!seen_root_open || !seen_root_close || depth != 0) {
        return err.InvalidJsonRoot;
    }

    const indent = if (opts.guess_indentation) guessJsonIndentation(source) else JsonIndentation{};
    return WorkspacePkgJsonSummary{
        content_hash: wpFnv1a64(source),
        byte_len: source.len,
        indentation: indent,
        has_comments: has_comments,
        has_trailing_comma: has_trailing_comma,
    };
}

/// 零堆分配 Monorepo Workspace `package.json` 缓存（对齐 Bun `WorkspacePackageJSONCache`）
pub struct WorkspacePkgJsonCache {
    entries: [WORKSPACE_PKG_CACHE_MAX_ENTRIES]WorkspacePkgCacheEntry = [WorkspacePkgCacheEntry{}; WORKSPACE_PKG_CACHE_MAX_ENTRIES],
    count: usize = 0,
    hits: u64 = 0,
    misses: u64 = 0,
    parse_errors: u64 = 0,
    invalidations: u64 = 0,

    static fn init(): WorkspacePkgJsonCache {
        return WorkspacePkgJsonCache{};
    }

    fn len(): usize {
        return _.count;
    }

    fn stats(): WorkspacePkgCacheStats {
        return WorkspacePkgCacheStats{
            entry_count: _.count,
            hits: _.hits,
            misses: _.misses,
            parse_errors: _.parse_errors,
            invalidations: _.invalidations,
        };
    }

    fn findNormalizedIndex(norm_path: []const u8): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (wpBytesEql(_.entries[i].path(), norm_path)) return i;
            i += 1;
        }
        return null;
    }

    /// 按绝对路径查找缓存（自动执行 POSIX 路径规范化）
    fn lookup(*_, abs_path: []const u8): WorkspacePkgCacheError!WorkspacePkgCacheEntry? {
        let norm_buf: [WORKSPACE_PKG_PATH_MAX_LEN]u8 = [0; WORKSPACE_PKG_PATH_MAX_LEN];
        const norm_len = normalizeWorkspacePkgJsonPath(abs_path, norm_buf[0..])?;
        if (idx := _.findNormalizedIndex(norm_buf[0..norm_len])) {
            _.hits += 1;
            _.entries[idx].hit_count += 1;
            return _.entries[idx];
        }
        return null;
    }

    /// 获取已有缓存条目，或在未命中时解析 `source` 并缓存（解析失败绝不污染缓存）
    fn getOrInsert(
        *_,
        abs_path: []const u8,
        source: []const u8,
        opts: WorkspacePkgJsonOptions,
    ): WorkspacePkgCacheError!WorkspacePkgCacheEntry {
        let norm_buf: [WORKSPACE_PKG_PATH_MAX_LEN]u8 = [0; WORKSPACE_PKG_PATH_MAX_LEN];
        const norm_len = normalizeWorkspacePkgJsonPath(abs_path, norm_buf[0..])?;
        const norm_path = norm_buf[0..norm_len];

        if (idx := _.findNormalizedIndex(norm_path)) {
            _.hits += 1;
            _.entries[idx].hit_count += 1;
            return _.entries[idx];
        }

        _.misses += 1;
        if (_.count >= WORKSPACE_PKG_CACHE_MAX_ENTRIES) return err.CacheFull;

        const summary = analyzeWorkspacePackageJson(source, opts) catch |e| {
            _.parse_errors += 1;
            return e;
        };

        let entry = WorkspacePkgCacheEntry{};
        entry.path_len = norm_len;
        let i: usize = 0;
        while (i < norm_len) {
            entry.path_buf[i] = norm_path[i];
            i += 1;
        }
        entry.content_hash = summary.content_hash;
        entry.byte_len = summary.byte_len;
        entry.indentation = summary.indentation;
        entry.has_comments = summary.has_comments;
        entry.has_trailing_comma = summary.has_trailing_comma;
        entry.hit_count = 0;

        _.entries[_.count] = entry;
        _.count += 1;
        return entry;
    }

    /// 失效指定路径的缓存条目（O(1) swap-remove）
    fn invalidate(*_, abs_path: []const u8): WorkspacePkgCacheError!bool {
        let norm_buf: [WORKSPACE_PKG_PATH_MAX_LEN]u8 = [0; WORKSPACE_PKG_PATH_MAX_LEN];
        const norm_len = normalizeWorkspacePkgJsonPath(abs_path, norm_buf[0..])?;
        if (idx := _.findNormalizedIndex(norm_buf[0..norm_len])) {
            const last = _.count - 1;
            if (idx != last) {
                _.entries[idx] = _.entries[last];
            }
            _.entries[last] = WorkspacePkgCacheEntry{};
            _.count -= 1;
            _.invalidations += 1;
            return true;
        }
        return false;
    }

    fn clear(*_) {
        let i: usize = 0;
        while (i < _.count) {
            _.entries[i] = WorkspacePkgCacheEntry{};
            i += 1;
        }
        _.count = 0;
    }
}
