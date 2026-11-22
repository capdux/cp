// Source: Absorbed from /data/dev/bun/src/install/ConfigVersion.cp | Synced: 2026-11-22T05:49:35Z
// ============================================================================
// Pure-CP Zero-Allocation Lockfile ConfigVersion, Default Linker Policy & Isolated Store Path Formatter
// Absorbed from Bun: src/install/ConfigVersion.cp & docs/pm/{isolated-installs,lockfile}.mdx
// ============================================================================

/// 当前最新锁文件 `configVersion` 数值（对齐 Bun v1.3.2+ `ConfigVersion.current = .v1`）
pub const PKG_CONFIG_VERSION_CURRENT: u64 = 1;

/// Isolated Installs 中心存储子目录名 (`node_modules/.bun`)
pub const PKG_ISOLATED_STORE_DIR: []const u8 = ".bun";

/// `config_version` / Isolated Store 路径编码错误集
pub err ConfigVersionError {
    BufferTooSmall,
    EmptyPackageName,
    EmptyVersion,
    InvalidPackageName,
    InvalidVersion,
    UnknownLinkerStrategy,
}

/// 锁文件迁移来源包管理器（决定迁移后的默认 `configVersion`）
pub enum LockfileMigrationSource: u8 {
    none = 0,
    legacy_bun = 1,
    npm = 2,
    yarn = 3,
    pnpm = 4,

    /// 返回该迁移来源对应的默认 `PkgConfigVersion`：
    /// - 全新工程 (`none`) 或从 `pnpm` 迁移：`v1`（工作区默认 `isolated`）
    /// - 旧版 Bun 锁文件 (`legacy_bun`)、从 `npm` 或 `yarn` 迁移：`v0`（默认 `hoisted`）
    fn defaultConfigVersion(): PkgConfigVersion {
        return switch (_) {
            .none => PkgConfigVersion.v1,
            .pnpm => PkgConfigVersion.v1,
            .legacy_bun => PkgConfigVersion.v0,
            .npm => PkgConfigVersion.v0,
            .yarn => PkgConfigVersion.v0,
        };
    }
}

/// 包链接器安装策略（`hoisted` 传统提升模式 vs `isolated` pnpm 风格严格隔离模式）
pub enum PkgLinkerStrategy: u8 {
    hoisted = 0,
    isolated = 1,

    static fn fromStr(raw: []const u8): PkgLinkerStrategy! {
        if (sliceEql(raw, "hoisted")) {
            return PkgLinkerStrategy.hoisted;
        }
        if (sliceEql(raw, "isolated")) {
            return PkgLinkerStrategy.isolated;
        }
        return err.UnknownLinkerStrategy;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .hoisted => "hoisted",
            .isolated => "isolated",
        };
    }

    fn isIsolated(): bool {
        return _ == PkgLinkerStrategy.isolated;
    }

    fn isHoisted(): bool {
        return _ == PkgLinkerStrategy.hoisted;
    }
}

/// 锁文件配置版本枚举（严格对齐 Bun `src/install/ConfigVersion.cp`）
pub enum PkgConfigVersion: u8 {
    v0 = 0,
    v1 = 1,

    static fn current(): PkgConfigVersion {
        return PkgConfigVersion.v1;
    }

    fn toInt(): u64 {
        return switch (_) {
            .v0 => 0,
            .v1 => 1,
        };
    }

    /// 从整数解析 `PkgConfigVersion`（对齐 Bun `ConfigVersion.fromInt`：高于当前版本时前向兼容钳位至 `current`）
    static fn fromInt(v: u64): PkgConfigVersion? {
        if (v == 0) {
            return PkgConfigVersion.v0;
        }
        if (v == 1) {
            return PkgConfigVersion.v1;
        }
        if (v > PKG_CONFIG_VERSION_CURRENT) {
            return PkgConfigVersion.current();
        }
        return null;
    }

    /// 从 JSON 浮点数值解析 `PkgConfigVersion`（对齐 Bun `ConfigVersion.fromExpr`：拒绝负数、NaN/Inf 及非整数）
    static fn fromFloat(v: f64): PkgConfigVersion? {
        if (v < 0.0 || v != v) {
            return null;
        }
        if (v == 0.0) {
            return PkgConfigVersion.v0;
        }
        if (v == 1.0) {
            return PkgConfigVersion.v1;
        }
        if (v > 9007199254740991.0) {
            return null;
        }
        const trunc_val: f64 = @trunc(v);
        if (trunc_val != v) {
            return null;
        }
        if (v > 1.0) {
            return PkgConfigVersion.current();
        }
        return null;
    }

    /// 依据锁文件 `configVersion` 与是否启用 Monorepo Workspaces 计算默认链接器策略
    /// - `v1` + `has_workspaces == true` => `isolated`
    /// - 其余所有组合 => `hoisted`
    fn defaultLinker(has_workspaces: bool): PkgLinkerStrategy {
        if (_ == PkgConfigVersion.v1 && has_workspaces) {
            return PkgLinkerStrategy.isolated;
        }
        return PkgLinkerStrategy.hoisted;
    }
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

fn hasTraversalOrControl(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < 0x20 || ch == '\\' || ch == 0x7f) {
            return true;
        }
        if (ch == '.' && i + 1 < s.len && s[i + 1] == '.') {
            return true;
        }
        i += 1;
    }
    return false;
}

fn validatePackageName(pkg_name: []const u8): bool {
    if (pkg_name.len == 0 || hasTraversalOrControl(pkg_name)) {
        return false;
    }
    if (pkg_name[0] == '/' || pkg_name[pkg_name.len - 1] == '/') {
        return false;
    }
    let slash_count: usize = 0;
    let i: usize = 0;
    while (i < pkg_name.len) {
        if (pkg_name[i] == '/') {
            slash_count += 1;
        }
        i += 1;
    }
    if (pkg_name[0] == '@') {
        // Scoped package must have exact single '/' separating @scope and name
        return slash_count == 1 && pkg_name.len >= 4 && pkg_name[1] != '/';
    }
    return slash_count == 0;
}

fn validateVersion(version: []const u8): bool {
    if (version.len == 0 || hasTraversalOrControl(version)) {
        return false;
    }
    let i: usize = 0;
    while (i < version.len) {
        if (version[i] == '/') {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 零堆分配生成 Isolated Store 条目目录名（Scoped 包将 `/` 替换为 `+`）：
/// - `("lodash", "4.17.21")` -> `"lodash@4.17.21"`
/// - `("@scope/pkg", "2.1.0")` -> `"@scope+pkg@2.1.0"`
pub fn formatIsolatedStoreEntry(
    buf: []u8,
    pkg_name: []const u8,
    version: []const u8,
): ([]const u8)! {
    if (pkg_name.len == 0) {
        return err.EmptyPackageName;
    }
    if (version.len == 0) {
        return err.EmptyVersion;
    }
    if (!validatePackageName(pkg_name)) {
        return err.InvalidPackageName;
    }
    if (!validateVersion(version)) {
        return err.InvalidVersion;
    }
    const needed = pkg_name.len + 1 + version.len;
    if (needed > buf.len) {
        return err.BufferTooSmall;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < pkg_name.len) {
        const ch = pkg_name[i];
        buf[pos] = if (ch == '/') '+' else ch;
        pos += 1;
        i += 1;
    }
    buf[pos] = '@';
    pos += 1;
    i = 0;
    while (i < version.len) {
        buf[pos] = version[i];
        pos += 1;
        i += 1;
    }
    return buf[0..pos];
}

/// 零堆分配生成完整 Isolated Store 包路径：
/// `node_modules/.bun/<entry>/node_modules/<pkg_name>`
pub fn formatIsolatedStorePath(
    buf: []u8,
    pkg_name: []const u8,
    version: []const u8,
): ([]const u8)! {
    const prefix = "node_modules/.bun/";
    const mid = "/node_modules/";
    const entry_len = pkg_name.len + 1 + version.len;
    const needed = prefix.len + entry_len + mid.len + pkg_name.len;
    if (pkg_name.len == 0) {
        return err.EmptyPackageName;
    }
    if (version.len == 0) {
        return err.EmptyVersion;
    }
    if (!validatePackageName(pkg_name)) {
        return err.InvalidPackageName;
    }
    if (!validateVersion(version)) {
        return err.InvalidVersion;
    }
    if (needed > buf.len) {
        return err.BufferTooSmall;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < prefix.len) {
        buf[pos] = prefix[i];
        pos += 1;
        i += 1;
    }
    const entry = formatIsolatedStoreEntry(buf[pos .. pos + entry_len], pkg_name, version)?;
    pos += entry.len;
    i = 0;
    while (i < mid.len) {
        buf[pos] = mid[i];
        pos += 1;
        i += 1;
    }
    i = 0;
    while (i < pkg_name.len) {
        buf[pos] = pkg_name[i];
        pos += 1;
        i += 1;
    }
    return buf[0..pos];
}
