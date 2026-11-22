// Source: Absorbed from /data/dev/bun/src/install/lockfile/Package/WorkspaceMap.cp | Synced: 2026-11-21T23:00:54Z
// Pure-CP zero-allocation Monorepo Workspace Map & Deterministic Sorter:
// - Ignored path filtering (`node_modules`, `.git`, `CMakeFiles`)
// - Bidirectional lookup by normalized relative path and package name
// - Duplicate path / duplicate package name detection with capacity guards
// - Deterministic in-place lexicographical sorting by package name (for lockfiles)

/// 静态工作区注册表最大条目数（零堆分配）
pub const MAX_WORKSPACE_MAP_ENTRIES: usize = 128;

/// 工作区映射表错误集
pub err WorkspaceMapError {
    EmptyPathOrName,
    DuplicateWorkspacePath,
    DuplicateWorkspaceName,
    CapacityExceeded,
}

/// 判断单个路径段是否为 Bun 工作区扫描默认忽略目录（`node_modules` / `.git` / `CMakeFiles`）
pub fn isIgnoredWorkspacePath(segment: string): bool {
    return std.mem.eql(u8, segment, "node_modules") ||
        std.mem.eql(u8, segment, ".git") ||
        std.mem.eql(u8, segment, "CMakeFiles");
}

/// 判断相对路径中是否包含任意被忽略目录段（兼容 `/` 与 `\` 分隔符）
pub fn hasIgnoredWorkspaceSegment(rel_path: string): bool {
    let start: usize = 0;
    let i: usize = 0;
    while (i <= rel_path.len) {
        const at_sep = (i == rel_path.len) || rel_path[i] == '/' || rel_path[i] == '\\';
        if (at_sep) {
            if (i > start) {
                if (isIgnoredWorkspacePath(rel_path[start..i])) {
                    return true;
                }
            }
            start = i + 1;
        }
        i += 1;
    }
    return false;
}

/// 剥离相对路径前导 `./` 与首尾斜杠，返回规范化视图
pub fn normalizeWorkspacePath(raw: string): string {
    let start: usize = 0;
    let end: usize = raw.len;
    while (end > start && (raw[end - 1] == '/' || raw[end - 1] == '\\')) {
        end -= 1;
    }
    while (start + 2 <= end && raw[start] == '.' && (raw[start + 1] == '/' || raw[start + 1] == '\\')) {
        start += 2;
    }
    while (start < end && (raw[start] == '/' || raw[start] == '\\')) {
        start += 1;
    }
    return raw[start..end];
}

fn lexLessThan(a: string, b: string): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) {
            return a[i] < b[i];
        }
        i += 1;
    }
    return a.len < b.len;
}

/// 单个工作区包条目（对齐 Bun `WorkspaceMap.Entry`）
pub struct WorkspaceMapEntry {
    rel_path: string = "",
    name: string = "",
    version: string? = null,
    name_loc: u32 = 0,
}

/// 零堆分配 Monorepo 工作区双向映射与确定性排序表（对齐 Bun `WorkspaceMap`）
pub struct WorkspaceMap {
    entries: [MAX_WORKSPACE_MAP_ENTRIES]WorkspaceMapEntry = [WorkspaceMapEntry{}; MAX_WORKSPACE_MAP_ENTRIES],
    len: usize = 0,

    static fn init(): WorkspaceMap {
        return WorkspaceMap{
            entries: [WorkspaceMapEntry{}; MAX_WORKSPACE_MAP_ENTRIES],
            len: 0,
        };
    }

    fn count(): usize {
        return _.len;
    }

    fn isEmpty(): bool {
        return _.len == 0;
    }

    fn getByPath(raw_path: string): WorkspaceMapEntry? {
        const norm = normalizeWorkspacePath(raw_path);
        let i: usize = 0;
        while (i < _.len) {
            if (std.mem.eql(u8, _.entries[i].rel_path, norm)) {
                return _.entries[i];
            }
            i += 1;
        }
        return null;
    }

    fn getByName(pkg_name: string): WorkspaceMapEntry? {
        let i: usize = 0;
        while (i < _.len) {
            if (std.mem.eql(u8, _.entries[i].name, pkg_name)) {
                return _.entries[i];
            }
            i += 1;
        }
        return null;
    }

    fn containsPath(raw_path: string): bool {
        return _.getByPath(raw_path) != null;
    }

    fn containsName(pkg_name: string): bool {
        return _.getByName(pkg_name) != null;
    }

    /// 注册工作区成员（自动规范化路径，拦截重复路径、重复包名与容量溢出）
    fn put(
        *_,
        raw_path: string,
        name: string,
        version: string?,
        name_loc: u32,
    ): usize! {
        const norm = normalizeWorkspacePath(raw_path);
        if (norm.len == 0 || name.len == 0) {
            return err.EmptyPathOrName;
        }
        if (_.containsPath(norm)) {
            return err.DuplicateWorkspacePath;
        }
        if (_.containsName(name)) {
            return err.DuplicateWorkspaceName;
        }
        if (_.len >= MAX_WORKSPACE_MAP_ENTRIES) {
            return err.CapacityExceeded;
        }
        const idx = _.len;
        _.entries[idx] = WorkspaceMapEntry{
            rel_path: norm,
            name: name,
            version: version,
            name_loc: name_loc,
        };
        _.len += 1;
        return idx;
    }

    /// 按包名字典序原址稳定排序（对齐 Bun `WorkspaceSortContext.lessThan`，确保 lockfile 确定性）
    fn sortByName(*_) {
        let i: usize = 1;
        while (i < _.len) {
            const key = _.entries[i];
            let j = i;
            while (j > 0 && lexLessThan(key.name, _.entries[j - 1].name)) {
                _.entries[j] = _.entries[j - 1];
                j -= 1;
            }
            _.entries[j] = key;
            i += 1;
        }
    }

    fn at(idx: usize): WorkspaceMapEntry? {
        if (idx >= _.len) {
            return null;
        }
        return _.entries[idx];
    }
}
