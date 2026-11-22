// Source: Absorbed from /data/dev/bun/src/install/isolated_install/FileCloner.cp | Synced: 2026-11-14T05:56:26Z
// Pure-CP zero-allocation Isolated Install File Materializer Policy, Recovery State Machine & Path Planner
// Aligned with Bun src/install/isolated_install/{FileCloner,Hardlinker}.cp & docs/pm/isolated-installs.mdx:
// - Resolves default linker mode (`isolated` vs `hoisted`) from lockfile `configVersion` and workspace presence
// - Selects platform-optimal materialization backend (`Clonefile` on macOS, `Hardlink` on Linux/Windows)
// - Implements exact errno recovery rules for staging materialization:
//     * EEXIST -> delete stale staging target tree and retry once (safe due to atomic commit rename)
//     * ENOENT -> create missing parent directory and retry once (fails if no parent dirname)
//     * EXDEV / ENOTSUP / EMLINK -> downgrade backend (`Clonefile` -> `Hardlink` -> `Copyfile`)
// - Resolves destination paths without prefixing `cwd` when targeting an absolute global-virtual-store path.

/// 最大可配置的跳过目录名数量
pub const ISOLATED_MAX_SKIP_DIRS: usize = 16;

/// 单个跳过目录名最大长度
pub const ISOLATED_MAX_DIRNAME_LEN: usize = 64;

/// Isolated Install 物化规划器错误集
pub err IsolatedMaterializeError {
    TooManySkipDirs,
    DirNameTooLong,
    EmptyPath,
    BufferTooSmall,
    InvalidLinkerFlag,
}

/// 依赖链接器策略（对齐 `bun install --linker isolated|hoisted`）
pub enum IsolatedLinkerMode: u8 {
    Hoisted = 0,
    Isolated = 1,
}

/// 目标操作系统平台
pub enum MaterializePlatform: u8 {
    MacOS = 0,
    Linux = 1,
    Windows = 2,
}

/// 文件物化后端策略
pub enum MaterializeBackend: u8 {
    Clonefile = 0,
    Hardlink = 1,
    Copyfile = 2,
}

/// 物化系统调用错误分类（抽象跨平台 POSIX / Windows errno）
pub enum MaterializeErrno: u8 {
    Ok = 0,
    Exist = 1,
    Noent = 2,
    Exdev = 3,
    Notsup = 4,
    Mlink = 5,
    Access = 6,
    Other = 7,
}

/// 错误自愈与降级裁决动作
pub enum MaterializeRecoveryAction: u8 {
    Succeeded = 0,
    DeleteTreeAndRetry = 1,
    MakeParentDirAndRetry = 2,
    FallbackBackend = 3,
    Fail = 4,
}

/// `decideMaterializeRecovery` 返回的完整决策
pub struct MaterializeDecision {
    action: MaterializeRecoveryAction = .Succeeded,
    next_backend: MaterializeBackend = .Clonefile,
    should_increment_retry: bool = false,
}

/// 物化执行统计快照
pub struct MaterializeStats {
    active_backend: MaterializeBackend = .Hardlink,
    cloned_count: u64 = 0,
    hardlinked_count: u64 = 0,
    copied_count: u64 = 0,
    exist_retries: u64 = 0,
    noent_retries: u64 = 0,
    backend_fallbacks: u64 = 0,
    failed_count: u64 = 0,
}

fn matBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn matIsAlpha(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn matIsAbsolutePath(path: []const u8): bool {
    if (path.len == 0) return false;
    if (path[0] == '/' || path[0] == '\\') return true;
    if (path.len >= 3 && matIsAlpha(path[0]) && path[1] == ':' && (path[2] == '\\' || path[2] == '/')) {
        return true;
    }
    return false;
}

/// 根据 lockfile `configVersion` 与是否启用 workspaces 决定默认 Linker（对齐 `docs/pm/isolated-installs.mdx`）
pub fn resolveDefaultLinkerMode(config_version: u8, uses_workspaces: bool): IsolatedLinkerMode {
    if (config_version >= 1 && uses_workspaces) return .Isolated;
    return .Hoisted;
}

/// 解析 CLI `--linker` 参数字符串（`"isolated"` / `"hoisted"`）
pub fn parseLinkerModeFlag(flag: []const u8): IsolatedMaterializeError!IsolatedLinkerMode {
    if (matBytesEql(flag, "isolated")) return .Isolated;
    if (matBytesEql(flag, "hoisted")) return .Hoisted;
    return err.InvalidLinkerFlag;
}

/// 返回各平台默认首选文件物化后端
pub fn defaultMaterializeBackend(platform: MaterializePlatform): MaterializeBackend {
    return switch (platform) {
        .MacOS => .Clonefile,
        .Linux => .Hardlink,
        .Windows => .Hardlink,
    };
}

/// 返回当前后端失败时的下一级降级后端（`Clonefile` -> `Hardlink` -> `Copyfile` -> `null`）
pub fn fallbackMaterializeBackend(current: MaterializeBackend): MaterializeBackend? {
    return switch (current) {
        .Clonefile => .Hardlink,
        .Hardlink => .Copyfile,
        .Copyfile => null,
    };
}

/// 根据系统调用错误码、是否存在父目录与是否已重试，裁决自愈或后端降级动作（对齐 `FileCloner.cp` & `Hardlinker.cp`）
pub fn decideMaterializeRecovery(
    backend: MaterializeBackend,
    errno_kind: MaterializeErrno,
    has_parent_dir: bool,
    already_retried: bool,
): MaterializeDecision {
    return switch (errno_kind) {
        .Ok => MaterializeDecision{
            action: .Succeeded,
            next_backend: backend,
            should_increment_retry: false,
        },
        .Exist => {
            if (already_retried) {
                return MaterializeDecision{
                    action: .Fail,
                    next_backend: backend,
                    should_increment_retry: false,
                };
            }
            return MaterializeDecision{
                action: .DeleteTreeAndRetry,
                next_backend: backend,
                should_increment_retry: true,
            };
        },
        .Noent => {
            if (already_retried || !has_parent_dir) {
                return MaterializeDecision{
                    action: .Fail,
                    next_backend: backend,
                    should_increment_retry: false,
                };
            }
            return MaterializeDecision{
                action: .MakeParentDirAndRetry,
                next_backend: backend,
                should_increment_retry: true,
            };
        },
        .Exdev, .Notsup, .Mlink => {
            if (next_b := fallbackMaterializeBackend(backend)) {
                return MaterializeDecision{
                    action: .FallbackBackend,
                    next_backend: next_b,
                    should_increment_retry: false,
                };
            }
            return MaterializeDecision{
                action: .Fail,
                next_backend: backend,
                should_increment_retry: false,
            };
        },
        .Access, .Other => MaterializeDecision{
            action: .Fail,
            next_backend: backend,
            should_increment_retry: false,
        },
    };
}

/// 解析目标物化路径（对齐 `Hardlinker.cp`：若 `dest` 已是绝对路径如 global virtual store，则不叠加 `cwd`）
pub fn resolveMaterializeDestPath(
    cwd: []const u8,
    dest: []const u8,
    out: []u8,
): IsolatedMaterializeError!usize {
    if (dest.len == 0) return err.EmptyPath;

    if (matIsAbsolutePath(dest)) {
        if (dest.len > out.len) return err.BufferTooSmall;
        let i: usize = 0;
        while (i < dest.len) {
            out[i] = dest[i];
            i += 1;
        }
        return dest.len;
    }

    if (cwd.len == 0) return err.EmptyPath;
    let cwd_end = cwd.len;
    while (cwd_end > 1 && (cwd[cwd_end - 1] == '/' || cwd[cwd_end - 1] == '\\')) {
        cwd_end -= 1;
    }
    const need_sep = !(cwd_end == 1 && (cwd[0] == '/' || cwd[0] == '\\'));
    const total = cwd_end + (if (need_sep) (1 as usize) else (0 as usize)) + dest.len;
    if (total > out.len) return err.BufferTooSmall;

    let pos: usize = 0;
    let i: usize = 0;
    while (i < cwd_end) {
        out[pos] = cwd[i];
        pos += 1;
        i += 1;
    }
    if (need_sep) {
        out[pos] = '/';
        pos += 1;
    }
    i = 0;
    while (i < dest.len) {
        out[pos] = dest[i];
        pos += 1;
        i += 1;
    }
    return pos;
}

struct SkipDirSlot {
    buf: [ISOLATED_MAX_DIRNAME_LEN]u8 = [0; ISOLATED_MAX_DIRNAME_LEN],
    len: usize = 0,

    fn slice(): []const u8 {
        return _.buf[0.._.len];
    }
}

/// 零堆分配 Isolated Install 物化协调器（管理后端降级、跳过目录表与统计计数）
pub struct IsolatedMaterializer {
    active_backend: MaterializeBackend = .Hardlink,
    skip_dirs: [ISOLATED_MAX_SKIP_DIRS]SkipDirSlot = [SkipDirSlot{}; ISOLATED_MAX_SKIP_DIRS],
    skip_dir_count: usize = 0,
    cloned_count: u64 = 0,
    hardlinked_count: u64 = 0,
    copied_count: u64 = 0,
    exist_retries: u64 = 0,
    noent_retries: u64 = 0,
    backend_fallbacks: u64 = 0,
    failed_count: u64 = 0,

    static fn initForPlatform(platform: MaterializePlatform): IsolatedMaterializer {
        let m = IsolatedMaterializer{};
        m.active_backend = defaultMaterializeBackend(platform);
        return m;
    }

    static fn initWithBackend(backend: MaterializeBackend): IsolatedMaterializer {
        let m = IsolatedMaterializer{};
        m.active_backend = backend;
        return m;
    }

    fn currentBackend(): MaterializeBackend {
        return _.active_backend;
    }

    fn addSkipDir(*_, dirname: []const u8): IsolatedMaterializeError!usize {
        if (dirname.len == 0) return err.EmptyPath;
        if (dirname.len > ISOLATED_MAX_DIRNAME_LEN) return err.DirNameTooLong;
        if (_.shouldSkipDir(dirname)) return _.skip_dir_count;
        if (_.skip_dir_count >= ISOLATED_MAX_SKIP_DIRS) return err.TooManySkipDirs;

        let slot = SkipDirSlot{};
        slot.len = dirname.len;
        let i: usize = 0;
        while (i < dirname.len) {
            slot.buf[i] = dirname[i];
            i += 1;
        }
        _.skip_dirs[_.skip_dir_count] = slot;
        _.skip_dir_count += 1;
        return _.skip_dir_count;
    }

    fn shouldSkipDir(dirname: []const u8): bool {
        let i: usize = 0;
        while (i < _.skip_dir_count) {
            if (matBytesEql(_.skip_dirs[i].slice(), dirname)) return true;
            i += 1;
        }
        return false;
    }

    /// 记录一次文件物化结果，自动推进重试统计或执行永久后端降级
    fn recordAttempt(
        *_,
        errno_kind: MaterializeErrno,
        has_parent_dir: bool,
        already_retried: bool,
    ): MaterializeDecision {
        const decision = decideMaterializeRecovery(_.active_backend, errno_kind, has_parent_dir, already_retried);
        switch (decision.action) {
            .Succeeded => {
                switch (_.active_backend) {
                    .Clonefile => { _.cloned_count += 1; },
                    .Hardlink => { _.hardlinked_count += 1; },
                    .Copyfile => { _.copied_count += 1; },
                }
            },
            .DeleteTreeAndRetry => {
                _.exist_retries += 1;
            },
            .MakeParentDirAndRetry => {
                _.noent_retries += 1;
            },
            .FallbackBackend => {
                _.active_backend = decision.next_backend;
                _.backend_fallbacks += 1;
            },
            .Fail => {
                _.failed_count += 1;
            },
        }
        return decision;
    }

    fn stats(): MaterializeStats {
        return MaterializeStats{
            active_backend: _.active_backend,
            cloned_count: _.cloned_count,
            hardlinked_count: _.hardlinked_count,
            copied_count: _.copied_count,
            exist_retries: _.exist_retries,
            noent_retries: _.noent_retries,
            backend_fallbacks: _.backend_fallbacks,
            failed_count: _.failed_count,
        };
    }
}
