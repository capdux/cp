// Source: Absorbed from /data/dev/bun/src/install/resolvers/folder_resolver.cp | Synced: 2026-11-23T20:24:05Z
// Ported from Bun: /data/dev/bun/src/install/resolvers/folder_resolver.cp & /data/dev/bun/docs/pm/workspaces.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation local folder / symlink / workspace package resolver utilities:
// `file:` / `link:` / `workspace:` / global-link / cache-folder `package.json` path normalizer,
// `WorkspaceSearchPathFormatter`, bundled-dependencies policy, publish-time `workspace:*` / `^` / `~`
// version resolver, and fixed-capacity path-hash deduplication cache (`FolderResolverCache`).

/// 单个本地目录解析缓存支持的最大条目数
pub const FOLDER_RESOLVER_MAX_ENTRIES: usize = 64;

/// 路径规范化最大段数
const FOLDER_MAX_PATH_SEGS: usize = 32;

/// 本地目录解析与路径规范化错误集
pub err FolderResolverError {
    EmptyPath,
    TooManyPathSegments,
    BufferTooSmall,
    CacheFull,
    InvalidWorkspaceSpec,
    EmptyPackageVersion,
}

/// 本地目录依赖解析来源类别（对齐 Bun `folder_resolver.cp`）
pub enum FolderResolverKind: u8 {
    /// 本地目录依赖（`file:`）
    Folder = 0,
    /// 本地软链依赖（`link:`）
    Symlink = 1,
    /// Monorepo 工作区依赖（`workspace:`）
    Workspace = 2,
    /// 全局软链目录（`~/.bun/install/global`）
    GlobalLink = 3,
    /// 全局缓存目录内的本地解压包
    CacheFolder = 4,
}

/// 缓存条目的解析状态（对齐 Bun `FolderResolutionTag`）
pub enum FolderResolutionTag: u8 {
    Empty = 0,
    PackageId = 1,
    NewPackageId = 2,
    Failed = 3,
}

/// 目录解析失败细分原因
pub enum FolderResolutionFailKind: u8 {
    None = 0,
    MissingPackageJson = 1,
    InvalidPackageJson = 2,
    MissingDirectory = 3,
}

/// `normalizeFolderPackageJsonPath` 输出的绝对 `package.json` 路径长度与相对目录路径长度
pub struct FolderNormalizedLengths {
    abs_len: usize = 0,
    rel_len: usize = 0,
}

/// 单条目录解析缓存记录
pub struct FolderResolutionEntry {
    path_hash: u64 = 0,
    rel_path: []const u8 = "",
    tag: FolderResolutionTag = FolderResolutionTag.Empty,
    package_id: u32 = 0,
    fail_kind: FolderResolutionFailKind = FolderResolutionFailKind.None,
}

/// 目录解析缓存统计
pub struct FolderResolverStats {
    entry_count: usize = 0,
    resolved_count: usize = 0,
    failed_count: usize = 0,
}

type SegSlice = []const u8;

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

fn startsWith(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) {
        return false;
    }
    return bytesEql(haystack[0..prefix.len], prefix);
}

fn appendBytes(out: []u8, pos: *usize, src: []const u8): void! {
    if (pos.* + src.len > out.len) {
        return FolderResolverError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn appendByte(out: []u8, pos: *usize, b: u8): void! {
    if (pos.* + 1 > out.len) {
        return FolderResolverError.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

/// 计算规范化目录路径的确定性 64 位 FNV-1a 哈希
pub fn hashFolderPath(normalized_path: []const u8): u64 {
    let h: u64 = 0xcbf29ce484222325;
    let i: usize = 0;
    while (i < normalized_path.len) {
        h ^= normalized_path[i] as u64;
        h *%= 0x100000001b3;
        i += 1;
    }
    return h;
}

/// 剥离 `file:` / `link:` / `workspace:` 协议前缀
pub fn stripFolderProtocolPrefix(raw: []const u8): []const u8 {
    if (startsWith(raw, "file:")) {
        return raw["file:".len..];
    }
    if (startsWith(raw, "link:")) {
        return raw["link:".len..];
    }
    if (startsWith(raw, "workspace:")) {
        return raw["workspace:".len..];
    }
    return raw;
}

/// 是否需要检查 `bundledDependencies`（对齐 Bun `NewResolver.checkBundledDependencies`：
/// `Folder` / `Symlink` / `CacheFolder` 为 `true`，`Workspace` 为 `false`）
pub fn shouldCheckBundledDependencies(kind: FolderResolverKind): bool {
    return switch (kind) {
        FolderResolverKind.Folder => true,
        FolderResolverKind.Symlink => true,
        FolderResolverKind.CacheFolder => true,
        FolderResolverKind.GlobalLink => true,
        FolderResolverKind.Workspace => false,
    };
}

fn isAbsPath(p: []const u8): bool {
    return p.len > 0 && (p[0] == '/' || p[0] == '\\');
}

fn pushSegments(path: []const u8, segs: []SegSlice, count: *usize, is_abs: bool): void! {
    let i: usize = 0;
    while (i < path.len) {
        while (i < path.len && (path[i] == '/' || path[i] == '\\')) {
            i += 1;
        }
        if (i >= path.len) {
            break;
        }
        const start = i;
        while (i < path.len && path[i] != '/' && path[i] != '\\') {
            i += 1;
        }
        const seg = path[start..i];
        if (bytesEql(seg, ".")) {
            continue;
        }
        if (bytesEql(seg, "..")) {
            if (count.* > 0 && !bytesEql(segs[count.* - 1], "..")) {
                count.* -= 1;
                continue;
            }
            if (is_abs) {
                continue;
            }
        }
        if (count.* >= segs.len) {
            return FolderResolverError.TooManyPathSegments;
        }
        segs[count.*] = seg;
        count.* += 1;
    }
}

/// 严格对齐 Bun `folder_resolver.normalizePackageJSONPath`：
/// 将本地目录/软链/工作区路径规范化为：
/// - `out_abs`：指向目标目录下 `package.json` 的规范化绝对路径（POSIX `/`）；
/// - `out_rel`：相对于 `top_level_dir` 的目录路径（不含 `/package.json`）。
pub fn normalizeFolderPackageJsonPath(
    kind: FolderResolverKind,
    top_level_dir: []const u8,
    base_override: []const u8,
    raw_input: []const u8,
    out_abs: []u8,
    out_rel: []u8,
): FolderNormalizedLengths! {
    const stripped = stripFolderProtocolPrefix(raw_input);
    if (stripped.len == 0 || top_level_dir.len == 0) {
        return FolderResolverError.EmptyPath;
    }

    // 1. 解析 top_level_dir 段
    let root_segs: [FOLDER_MAX_PATH_SEGS]SegSlice = [""; FOLDER_MAX_PATH_SEGS];
    let root_n: usize = 0;
    const root_abs = isAbsPath(top_level_dir);
    pushSegments(top_level_dir, root_segs[0..], &root_n, root_abs)?;

    // 2. 解析目标目录完整路径段
    let target_segs: [FOLDER_MAX_PATH_SEGS]SegSlice = [""; FOLDER_MAX_PATH_SEGS];
    let target_n: usize = 0;
    let target_abs = isAbsPath(stripped);
    if (!target_abs) {
        const base = if ((kind == FolderResolverKind.GlobalLink || kind == FolderResolverKind.CacheFolder) && base_override.len > 0)
            base_override
        else
            top_level_dir;
        target_abs = isAbsPath(base);
        pushSegments(base, target_segs[0..], &target_n, target_abs)?;
    }
    pushSegments(stripped, target_segs[0..], &target_n, target_abs)?;

    // 3. 写入 out_abs: <target_dir>/package.json
    let abs_pos: usize = 0;
    if (target_abs) {
        appendByte(out_abs, &abs_pos, '/')?;
    }
    let ti: usize = 0;
    while (ti < target_n) {
        if (ti > 0) {
            appendByte(out_abs, &abs_pos, '/')?;
        }
        appendBytes(out_abs, &abs_pos, target_segs[ti])?;
        ti += 1;
    }
    if (abs_pos == 0 || out_abs[abs_pos - 1] != '/') {
        appendByte(out_abs, &abs_pos, '/')?;
    }
    appendBytes(out_abs, &abs_pos, "package.json")?;

    // 4. 计算 out_rel: relative(top_level_dir, target_dir)
    let common: usize = 0;
    while (common < root_n && common < target_n && bytesEql(root_segs[common], target_segs[common])) {
        common += 1;
    }
    let rel_pos: usize = 0;
    let up: usize = root_n - common;
    let first = true;
    while (up > 0) {
        if (!first) {
            appendByte(out_rel, &rel_pos, '/')?;
        }
        appendBytes(out_rel, &rel_pos, "..")?;
        first = false;
        up -= 1;
    }
    let rj: usize = common;
    while (rj < target_n) {
        if (!first) {
            appendByte(out_rel, &rel_pos, '/')?;
        }
        appendBytes(out_rel, &rel_pos, target_segs[rj])?;
        first = false;
        rj += 1;
    }
    if (rel_pos == 0) {
        appendByte(out_rel, &rel_pos, '.')?;
    }

    return FolderNormalizedLengths{
        abs_len: abs_pos,
        rel_len: rel_pos,
    };
}

/// 对齐 Bun `WorkspaceSearchPathFormatter`：
/// 若 `rel_path` 不以 `.` 或 `/` 开头，则自动前置 `./`；`quoted = true` 时包裹双引号并转义 `"` 与 `\`。
pub fn formatWorkspaceSearchPath(rel_path: []const u8, quoted: bool, out: []u8): usize! {
    if (rel_path.len == 0) {
        return FolderResolverError.EmptyPath;
    }
    let pos: usize = 0;
    if (quoted) {
        appendByte(out, &pos, '"')?;
    }
    if (rel_path[0] != '.' && rel_path[0] != '/') {
        appendBytes(out, &pos, "./")?;
    }
    let i: usize = 0;
    while (i < rel_path.len) {
        const c = rel_path[i];
        if (quoted && (c == '"' || c == '\\')) {
            appendByte(out, &pos, '\\')?;
        }
        appendByte(out, &pos, c)?;
        i += 1;
    }
    if (quoted) {
        appendByte(out, &pos, '"')?;
    }
    return pos;
}

/// 发布阶段 `workspace:` 协议版本替换（严格对齐 `docs/pm/workspaces.mdx`）：
/// - `"workspace:*"` -> `<pkg_version>`（如 `"1.0.1"`）
/// - `"workspace:^"` -> `"^<pkg_version>"`
/// - `"workspace:~"` -> `"~<pkg_version>"`
/// - `"workspace:1.0.2"` -> `"1.0.2"`（显式版本优先于当前 `package.json` 版本）
pub fn resolveWorkspacePublishVersion(
    workspace_spec: []const u8,
    pkg_version: []const u8,
    out: []u8,
): usize! {
    if (!startsWith(workspace_spec, "workspace:")) {
        return FolderResolverError.InvalidWorkspaceSpec;
    }
    const spec = workspace_spec["workspace:".len..];
    if (spec.len == 0) {
        return FolderResolverError.InvalidWorkspaceSpec;
    }

    let pos: usize = 0;
    if (bytesEql(spec, "*")) {
        if (pkg_version.len == 0) {
            return FolderResolverError.EmptyPackageVersion;
        }
        appendBytes(out, &pos, pkg_version)?;
        return pos;
    }
    if (bytesEql(spec, "^")) {
        if (pkg_version.len == 0) {
            return FolderResolverError.EmptyPackageVersion;
        }
        appendByte(out, &pos, '^')?;
        appendBytes(out, &pos, pkg_version)?;
        return pos;
    }
    if (bytesEql(spec, "~")) {
        if (pkg_version.len == 0) {
            return FolderResolverError.EmptyPackageVersion;
        }
        appendByte(out, &pos, '~')?;
        appendBytes(out, &pos, pkg_version)?;
        return pos;
    }
    // 显式指定版本（如 workspace:1.0.2 或 workspace:^1.0.2）直接剥离协议前缀透传
    appendBytes(out, &pos, spec)?;
    return pos;
}

/// 零堆分配本地目录解析去重缓存（对齐 Bun `FolderResolver.Map`）
pub struct FolderResolverCache {
    entries: [FOLDER_RESOLVER_MAX_ENTRIES]FolderResolutionEntry = [FolderResolutionEntry{}; FOLDER_RESOLVER_MAX_ENTRIES],
    count: usize = 0,

    static fn init(): FolderResolverCache {
        return FolderResolverCache{};
    }

    fn findSlot(path_hash: u64, rel_path: []const u8): usize? {
        let i: usize = 0;
        while (i < _.count) {
            if (_.entries[i].path_hash == path_hash && bytesEql(_.entries[i].rel_path, rel_path)) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn putResolved(*_, rel_path: []const u8, package_id: u32, is_new: bool): usize! {
        if (rel_path.len == 0) {
            return FolderResolverError.EmptyPath;
        }
        const h = hashFolderPath(rel_path);
        const tag = if (is_new) FolderResolutionTag.NewPackageId else FolderResolutionTag.PackageId;
        if (existing := _.findSlot(h, rel_path)) {
            _.entries[existing].tag = tag;
            _.entries[existing].package_id = package_id;
            _.entries[existing].fail_kind = FolderResolutionFailKind.None;
            return existing;
        }
        if (_.count >= FOLDER_RESOLVER_MAX_ENTRIES) {
            return FolderResolverError.CacheFull;
        }
        const idx = _.count;
        _.entries[idx] = FolderResolutionEntry{
            path_hash: h,
            rel_path: rel_path,
            tag: tag,
            package_id: package_id,
            fail_kind: FolderResolutionFailKind.None,
        };
        _.count += 1;
        return idx;
    }

    fn putError(*_, rel_path: []const u8, fail_kind: FolderResolutionFailKind): usize! {
        if (rel_path.len == 0) {
            return FolderResolverError.EmptyPath;
        }
        const h = hashFolderPath(rel_path);
        if (existing := _.findSlot(h, rel_path)) {
            _.entries[existing].tag = FolderResolutionTag.Failed;
            _.entries[existing].fail_kind = fail_kind;
            return existing;
        }
        if (_.count >= FOLDER_RESOLVER_MAX_ENTRIES) {
            return FolderResolverError.CacheFull;
        }
        const idx = _.count;
        _.entries[idx] = FolderResolutionEntry{
            path_hash: h,
            rel_path: rel_path,
            tag: FolderResolutionTag.Failed,
            fail_kind: fail_kind,
        };
        _.count += 1;
        return idx;
    }

    fn lookup(rel_path: []const u8): FolderResolutionEntry? {
        if (rel_path.len == 0) {
            return null;
        }
        const h = hashFolderPath(rel_path);
        if (idx := _.findSlot(h, rel_path)) {
            return _.entries[idx];
        }
        return null;
    }

    fn stats(): FolderResolverStats {
        let s = FolderResolverStats{ entry_count: _.count };
        let i: usize = 0;
        while (i < _.count) {
            const t = _.entries[i].tag;
            if (t == FolderResolutionTag.PackageId || t == FolderResolutionTag.NewPackageId) {
                s.resolved_count += 1;
            } else if (t == FolderResolutionTag.Failed) {
                s.failed_count += 1;
            }
            i += 1;
        }
        return s;
    }
}
