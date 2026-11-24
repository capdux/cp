// Source: Absorbed from /data/dev/bun/src/bundler/linker_context/writeOutputFilesToDisk.cp | Synced: 2026-11-23T14:41:08Z
// Pure-CP zero-allocation Bundler disk output write planner:
// standalone HTML placeholder alignment, deduplicated parent-directory creation plan,
// duplicate output path collision detection, path-traversal guard, executable mode (0o755/0o644),
// SourceMap/Bytecode companion emission, and JS/CSS `sourceMappingURL` comment formatter.

/// 单次写入规划支持的最大输出文件条目数（含伴生 SourceMap / Bytecode / Placeholder）
pub const WRITE_PLAN_MAX_FILES: usize = 64;

/// 单次写入规划支持的最大去重父目录数
pub const WRITE_PLAN_MAX_DIRS: usize = 64;

/// 磁盘写入规划错误集
pub err WritePlanError {
    EmptyPath,
    UnsafePath,
    DuplicateOutputPath,
    TableFull,
    BufferTooSmall,
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

fn hasControlChar(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] < 0x20 || s[i] == 0x7f) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn writeSlice(out: []u8, pos: *usize, s: []const u8): bool! {
    if (pos + s.len > out.len) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos] = s[i];
        pos += 1;
        i += 1;
    }
    return true;
}

/// 校验构建产物相对落盘路径是否安全：
/// 拒绝空路径、绝对路径（前导 `/` 或 `\`）、Windows 盘符前缀、控制字符与任意 `..` 穿越段。
pub fn validateOutputRelPath(rel_path: []const u8): bool! {
    if (rel_path.len == 0) {
        return err.EmptyPath;
    }
    if (hasControlChar(rel_path)) {
        return err.UnsafePath;
    }
    if (rel_path[0] == '/' || rel_path[0] == '\\') {
        return err.UnsafePath;
    }
    if (rel_path.len >= 2 && rel_path[1] == ':') {
        return err.UnsafePath;
    }

    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= rel_path.len) {
        if (i == rel_path.len || rel_path[i] == '/' || rel_path[i] == '\\') {
            const seg = rel_path[seg_start..i];
            if (bytesEql(seg, "..")) {
                return err.UnsafePath;
            }
            seg_start = i + 1;
        }
        i += 1;
    }
    return true;
}

/// 提取相对路径的 POSIX 父目录（如 `"assets/js/app.js"` -> `"assets/js"`；顶层文件返回 `null`）
pub fn extractParentDirPosix(rel_path: []const u8): ([]const u8)? {
    let i = rel_path.len;
    while (i > 0) {
        i -= 1;
        if (rel_path[i] == '/') {
            if (i == 0) {
                return null;
            }
            return rel_path[0..i];
        }
    }
    return null;
}

/// Chunk 内容分类
pub enum WriteChunkContentKind {
    Js,
    Css,
    Html,
    Asset,
}

/// SourceMap 输出策略
pub enum WriteSourceMapMode {
    None,
    Inline,
    Linked,
    External,
}

/// 计划写入的文件角色
pub enum WriteFileRole {
    PrimaryChunk,
    SourceMap,
    Bytecode,
    Placeholder,
}

/// 格式化 JS (`//# sourceMappingURL=...`) 或 CSS (`/*# sourceMappingURL=... */`) 尾部注释
pub fn formatSourceMapUrlComment(
    content_kind: WriteChunkContentKind,
    map_rel_path: []const u8,
    public_path: []const u8,
    out: []u8,
): usize! {
    if (map_rel_path.len == 0) {
        return err.EmptyPath;
    }
    let pos: usize = 0;
    if (content_kind == WriteChunkContentKind.Css) {
        * = writeSlice(out, *pos, "/*# sourceMappingURL=")?;
        if (public_path.len > 0) {
            * = writeSlice(out, *pos, public_path)?;
        }
        * = writeSlice(out, *pos, map_rel_path)?;
        * = writeSlice(out, *pos, " */\n")?;
        return pos;
    }
    * = writeSlice(out, *pos, "//# sourceMappingURL=")?;
    if (public_path.len > 0) {
        * = writeSlice(out, *pos, public_path)?;
    }
    * = writeSlice(out, *pos, map_rel_path)?;
    * = writeSlice(out, *pos, "\n")?;
    return pos;
}

/// 单条落盘计划记录
pub struct WritePlannedFile {
    chunk_index: u32 = 0,
    role: WriteFileRole = WriteFileRole.PrimaryChunk,
    rel_path: []const u8 = "",
    file_mode: u32 = 0o644,
    size_bytes: usize = 0,
    standalone_skipped: bool = false,
}

/// 落盘规划汇总统计
pub struct WritePlanStats {
    dirs_to_create: usize = 0,
    files_to_write: usize = 0,
    placeholders_count: usize = 0,
    sourcemaps_count: usize = 0,
    bytecode_count: usize = 0,
    executable_count: usize = 0,
    total_bytes: usize = 0,
}

/// 零堆分配 Bundler 磁盘落盘规划器（对齐 `writeOutputFilesToDisk.cp`）
pub struct DiskWritePlanner {
    standalone_html_only: bool = false,
    dirs: [WRITE_PLAN_MAX_DIRS][]const u8 = [""; WRITE_PLAN_MAX_DIRS],
    dir_len: usize = 0,
    files: [WRITE_PLAN_MAX_FILES]WritePlannedFile = [WritePlannedFile{}; WRITE_PLAN_MAX_FILES],
    file_len: usize = 0,

    static fn init(standalone_html_only: bool): DiskWritePlanner {
        return DiskWritePlanner{ standalone_html_only: standalone_html_only };
    }

    fn ensureParentDir(*_, rel_path: []const u8): bool! {
        const parent = extractParentDirPosix(rel_path) ?? return false;
        let i: usize = 0;
        while (i < _.dir_len) {
            if (bytesEql(_.dirs[i], parent)) {
                return false;
            }
            i += 1;
        }
        if (_.dir_len >= WRITE_PLAN_MAX_DIRS) {
            return err.TableFull;
        }
        _.dirs[_.dir_len] = parent;
        _.dir_len += 1;
        return true;
    }

    fn hasPlannedPath(rel_path: []const u8): bool {
        let i: usize = 0;
        while (i < _.file_len) {
            if (!_.files[i].standalone_skipped && bytesEql(_.files[i].rel_path, rel_path)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    fn pushFile(*_, item: WritePlannedFile): bool! {
        if (_.file_len >= WRITE_PLAN_MAX_FILES) {
            return err.TableFull;
        }
        _.files[_.file_len] = item;
        _.file_len += 1;
        return true;
    }

    /// 规划单个 Chunk 及其伴生 SourceMap / Bytecode 的磁盘写入：
    /// - `standalone_html_only = true` 且 `kind != Html` 时，仅写入索引对齐占位符（对齐 Bun 行 110-129）；
    /// - 严格执行安全路径防线与全局路径去重，自动记录去重后的父目录创建列表与 `0o755`/`0o644` 权限。
    fn planChunk(
        *_,
        chunk_index: u32,
        kind: WriteChunkContentKind,
        rel_path: []const u8,
        size_bytes: usize,
        is_executable: bool,
        sm_mode: WriteSourceMapMode,
        map_rel_path: []const u8,
        has_bytecode: bool,
        bytecode_rel_path: []const u8,
    ): bool! {
        if (_.standalone_html_only && kind != WriteChunkContentKind.Html) {
            * = _.pushFile(WritePlannedFile{
                chunk_index: chunk_index,
                role: WriteFileRole.Placeholder,
                rel_path: "",
                file_mode: 0,
                size_bytes: 0,
                standalone_skipped: true,
            })?;
            return false;
        }

        * = validateOutputRelPath(rel_path)?;
        if (_.hasPlannedPath(rel_path)) {
            return err.DuplicateOutputPath;
        }
        * = _.ensureParentDir(rel_path)?;

        const mode: u32 = if (is_executable) 0o755 else 0o644;
        * = _.pushFile(WritePlannedFile{
            chunk_index: chunk_index,
            role: WriteFileRole.PrimaryChunk,
            rel_path: rel_path,
            file_mode: mode,
            size_bytes: size_bytes,
            standalone_skipped: false,
        })?;

        if (sm_mode == WriteSourceMapMode.Linked || sm_mode == WriteSourceMapMode.External) {
            * = validateOutputRelPath(map_rel_path)?;
            if (_.hasPlannedPath(map_rel_path)) {
                return err.DuplicateOutputPath;
            }
            * = _.ensureParentDir(map_rel_path)?;
            * = _.pushFile(WritePlannedFile{
                chunk_index: chunk_index,
                role: WriteFileRole.SourceMap,
                rel_path: map_rel_path,
                file_mode: 0o644,
                size_bytes: 0,
                standalone_skipped: false,
            })?;
        }

        if (has_bytecode) {
            * = validateOutputRelPath(bytecode_rel_path)?;
            if (_.hasPlannedPath(bytecode_rel_path)) {
                return err.DuplicateOutputPath;
            }
            * = _.ensureParentDir(bytecode_rel_path)?;
            * = _.pushFile(WritePlannedFile{
                chunk_index: chunk_index,
                role: WriteFileRole.Bytecode,
                rel_path: bytecode_rel_path,
                file_mode: 0o644,
                size_bytes: 0,
                standalone_skipped: false,
            })?;
        }

        return true;
    }

    fn dirAt(idx: usize): ([]const u8)? {
        if (idx >= _.dir_len) {
            return null;
        }
        return _.dirs[idx];
    }

    fn fileAt(idx: usize): WritePlannedFile? {
        if (idx >= _.file_len) {
            return null;
        }
        return _.files[idx];
    }

    fn stats(): WritePlanStats {
        let to_write: usize = 0;
        let placeholders: usize = 0;
        let sourcemaps: usize = 0;
        let bytecode: usize = 0;
        let executables: usize = 0;
        let total_bytes: usize = 0;

        let i: usize = 0;
        while (i < _.file_len) {
            const f = _.files[i];
            if (f.role == WriteFileRole.Placeholder) {
                placeholders += 1;
            } else {
                to_write += 1;
                total_bytes += f.size_bytes;
                if (f.role == WriteFileRole.SourceMap) sourcemaps += 1;
                if (f.role == WriteFileRole.Bytecode) bytecode += 1;
                if (f.file_mode == 0o755) executables += 1;
            }
            i += 1;
        }

        return WritePlanStats{
            dirs_to_create: _.dir_len,
            files_to_write: to_write,
            placeholders_count: placeholders,
            sourcemaps_count: sourcemaps,
            bytecode_count: bytecode,
            executable_count: executables,
            total_bytes: total_bytes,
        };
    }
}
