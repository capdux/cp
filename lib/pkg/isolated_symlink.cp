// Source: Absorbed from /data/dev/bun/src/install/isolated_install/Symlinker.cp | Synced: 2026-11-22T10:39:50Z
// Ported from Bun: /data/dev/bun/src/install/isolated_install/Symlinker.cp & /data/dev/bun/docs/pm/isolated-installs.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation isolated-install symlink reconciliation engine & relative symlink target planner:
// ExpectExisting / ExpectMissing / IgnoreFailure strategies, non-destructive preservation of
// `bun patch <pkg>` real directories, Windows junction trailing-slash normalization & fallback matching,
// cross-platform relative symlink target computation, and batch link-plan statistics.

/// 单个隔离安装符号链接计划支持的最大条目数
pub const ISOLATED_LINK_MAX_OPS: usize = 64;

/// 相对路径计算支持的最大路径段数
const ISOLATED_MAX_PATH_SEGS: usize = 32;

/// 隔离安装符号链接规划与校验错误集
pub err IsolatedSymlinkError {
    EmptyPath,
    TooManyPathSegments,
    TooManyLinkOps,
    BufferTooSmall,
    InvalidLinkTarget,
}

/// 符号链接创建与对账策略（严格对齐 Bun `Symlinker.LinkStrategy`）
pub enum IsolatedLinkStrategy: u8 {
    /// 目标路径预期已存在（增量安装）：比对现有目标，保护 `bun patch` 实体目录，不一致才替换
    ExpectExisting = 0,
    /// 目标路径预期不存在（冷安装）：若已存在则整树清理后重建
    ExpectMissing = 1,
    /// 忽略失败（可选依赖或非关键链接）：仅尝试建父目录并创建，失败静默忽略
    IgnoreFailure = 2,
}

/// 目标平台（决定 Windows Junction 回退匹配与删除符号链接的系统调用语义）
pub enum IsolatedLinkPlatform: u8 {
    Posix = 0,
    Windows = 1,
}

/// 目标路径当前在文件系统上的探测状态
pub enum IsolatedDestEntryKind: u8 {
    /// 目标不存在（父目录已存在）
    Missing = 0,
    /// 目标的父目录尚不存在
    MissingParent = 1,
    /// 目标是一个符号链接或 Windows Junction
    Symlink = 2,
    /// 目标是一个普通文件
    RegularFile = 3,
    /// 目标是一个实体目录（例如 `bun patch <pkg>` 正在编辑的独立副本）
    RealDirectory = 4,
}

/// 符号链接对账状态机输出的确定性执行动作
pub enum IsolatedLinkAction: u8 {
    /// 已指向正确目标或策略允许跳过，无需文件系统写入
    Noop = 0,
    /// 直接创建符号链接
    CreateSymlink = 1,
    /// 先递归创建父目录，再创建符号链接
    MkdirAndCreateSymlink = 2,
    /// 先删除现有目标树，再创建符号链接（`ExpectMissing` 冲突处理）
    DeleteTreeAndCreateSymlink = 3,
    /// 先 `unlink` 现有文件或失效符号链接，再创建符号链接（POSIX / 普通文件）
    UnlinkAndCreateSymlink = 4,
    /// 先 `rmdir` 指向错误目标的目录符号链接/Junction，再重建（Windows 语义）
    RmdirAndCreateSymlink = 5,
    /// 目标是 `bun patch <pkg>` 实体目录，严禁覆盖以保护用户未提交的补丁修改
    PreservePatchedDir = 6,
}

/// 批量链接计划中的单条记录
pub struct IsolatedLinkOp {
    dest_path: []const u8 = "",
    desired_target: []const u8 = "",
    action: IsolatedLinkAction = IsolatedLinkAction.Noop,
}

/// 批量链接计划统计摘要
pub struct IsolatedLinkPlanStats {
    total: usize = 0,
    noop_count: usize = 0,
    create_count: usize = 0,
    replace_count: usize = 0,
    preserved_patch_count: usize = 0,
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

/// 剥离符号链接/Windows Junction 尾部多余的 `/` 或 `\`（对齐 Bun `strings.withoutTrailingSlash`）
pub fn stripTrailingSlash(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 1 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    return path[0..end];
}

/// 严格对齐 Bun `Symlinker.ensureSymlink` 的纯逻辑对账决策引擎：
/// - `IgnoreFailure`：`Missing` -> `CreateSymlink`，`MissingParent` -> `MkdirAndCreateSymlink`，其余均 `Noop`；
/// - `ExpectMissing`：`Missing` -> `CreateSymlink`，`MissingParent` -> `MkdirAndCreateSymlink`，已存在则 `DeleteTreeAndCreateSymlink`；
/// - `ExpectExisting`：
///   - `RealDirectory` -> `PreservePatchedDir`（保护 `bun patch <pkg>` 工作区不被静默销毁）；
///   - `RegularFile` -> `UnlinkAndCreateSymlink`；
///   - `Symlink` -> 剥离 libuv Junction 尾斜杠后比对 `desired_target`（Windows 下同时比对 `fallback_junction_target`），一致返回 `Noop`，不一致在 Windows 下返回 `RmdirAndCreateSymlink`、POSIX 下返回 `UnlinkAndCreateSymlink`。
pub fn reconcileIsolatedSymlink(
    strategy: IsolatedLinkStrategy,
    platform: IsolatedLinkPlatform,
    dest_kind: IsolatedDestEntryKind,
    current_link: []const u8,
    desired_target: []const u8,
    fallback_junction_target: []const u8,
): IsolatedLinkAction {
    switch (strategy) {
        IsolatedLinkStrategy.IgnoreFailure => {
            return switch (dest_kind) {
                IsolatedDestEntryKind.Missing => IsolatedLinkAction.CreateSymlink,
                IsolatedDestEntryKind.MissingParent => IsolatedLinkAction.MkdirAndCreateSymlink,
                IsolatedDestEntryKind.Symlink => IsolatedLinkAction.Noop,
                IsolatedDestEntryKind.RegularFile => IsolatedLinkAction.Noop,
                IsolatedDestEntryKind.RealDirectory => IsolatedLinkAction.Noop,
            };
        },
        IsolatedLinkStrategy.ExpectMissing => {
            return switch (dest_kind) {
                IsolatedDestEntryKind.Missing => IsolatedLinkAction.CreateSymlink,
                IsolatedDestEntryKind.MissingParent => IsolatedLinkAction.MkdirAndCreateSymlink,
                IsolatedDestEntryKind.Symlink => IsolatedLinkAction.DeleteTreeAndCreateSymlink,
                IsolatedDestEntryKind.RegularFile => IsolatedLinkAction.DeleteTreeAndCreateSymlink,
                IsolatedDestEntryKind.RealDirectory => IsolatedLinkAction.DeleteTreeAndCreateSymlink,
            };
        },
        IsolatedLinkStrategy.ExpectExisting => {
            switch (dest_kind) {
                IsolatedDestEntryKind.Missing => {
                    return IsolatedLinkAction.CreateSymlink;
                },
                IsolatedDestEntryKind.MissingParent => {
                    return IsolatedLinkAction.MkdirAndCreateSymlink;
                },
                IsolatedDestEntryKind.RealDirectory => {
                    return IsolatedLinkAction.PreservePatchedDir;
                },
                IsolatedDestEntryKind.RegularFile => {
                    return IsolatedLinkAction.UnlinkAndCreateSymlink;
                },
                IsolatedDestEntryKind.Symlink => {
                    const norm_cur = stripTrailingSlash(current_link);
                    const norm_want = stripTrailingSlash(desired_target);
                    if (bytesEql(norm_cur, norm_want)) {
                        return IsolatedLinkAction.Noop;
                    }
                    if (platform == IsolatedLinkPlatform.Windows && fallback_junction_target.len > 0) {
                        const norm_fb = stripTrailingSlash(fallback_junction_target);
                        if (bytesEql(norm_cur, norm_fb)) {
                            return IsolatedLinkAction.Noop;
                        }
                        return IsolatedLinkAction.RmdirAndCreateSymlink;
                    }
                    if (platform == IsolatedLinkPlatform.Windows) {
                        return IsolatedLinkAction.RmdirAndCreateSymlink;
                    }
                    return IsolatedLinkAction.UnlinkAndCreateSymlink;
                },
            }
        },
    }
}

type SegSlice = []const u8;

fn splitNormalizedSegments(path: []const u8, out_segs: []SegSlice): usize! {
    let count: usize = 0;
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
            if (count > 0 && !bytesEql(out_segs[count - 1], "..")) {
                count -= 1;
                continue;
            }
        }
        if (count >= out_segs.len) {
            return IsolatedSymlinkError.TooManyPathSegments;
        }
        out_segs[count] = seg;
        count += 1;
    }
    return count;
}

fn appendBytes(out: []u8, pos: *usize, src: []const u8): void! {
    if (pos.* + src.len > out.len) {
        return IsolatedSymlinkError.BufferTooSmall;
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
        return IsolatedSymlinkError.BufferTooSmall;
    }
    out[pos.*] = b;
    pos.* += 1;
}

/// 计算从符号链接所在目录 `from_dir` 指向目标路径 `to_path` 的 POSIX 相对路径。
/// 例如：
/// - `from_dir = "node_modules"`, `to_path = "node_modules/.bun/foo@1.0.0/node_modules/foo"`
///   -> `".bun/foo@1.0.0/node_modules/foo"`
/// - `from_dir = "node_modules/@scope"`, `to_path = "node_modules/.bun/@scope+bar@2.0.0/node_modules/@scope/bar"`
///   -> `"../.bun/@scope+bar@2.0.0/node_modules/@scope/bar"`
/// - `from_dir = "packages/app/node_modules"`, `to_path = "packages/core"`
///   -> `"../../core"`
pub fn computeRelativeSymlinkTarget(from_dir: []const u8, to_path: []const u8, out: []u8): usize! {
    if (from_dir.len == 0 || to_path.len == 0) {
        return IsolatedSymlinkError.EmptyPath;
    }
    let from_segs: [ISOLATED_MAX_PATH_SEGS]SegSlice = [""; ISOLATED_MAX_PATH_SEGS];
    let to_segs: [ISOLATED_MAX_PATH_SEGS]SegSlice = [""; ISOLATED_MAX_PATH_SEGS];

    const from_n = splitNormalizedSegments(from_dir, from_segs[0..])?;
    const to_n = splitNormalizedSegments(to_path, to_segs[0..])?;

    let common: usize = 0;
    while (common < from_n && common < to_n && bytesEql(from_segs[common], to_segs[common])) {
        common += 1;
    }

    let pos: usize = 0;
    let up: usize = from_n - common;
    let first = true;
    while (up > 0) {
        if (!first) {
            appendByte(out, &pos, '/')?;
        }
        appendBytes(out, &pos, "..")?;
        first = false;
        up -= 1;
    }

    let j: usize = common;
    while (j < to_n) {
        if (!first) {
            appendByte(out, &pos, '/')?;
        }
        appendBytes(out, &pos, to_segs[j])?;
        first = false;
        j += 1;
    }

    if (pos == 0) {
        appendByte(out, &pos, '.')?;
    }
    return pos;
}

/// 零堆分配隔离安装符号链接批量对账计划器
pub struct IsolatedLinkPlan {
    ops: [ISOLATED_LINK_MAX_OPS]IsolatedLinkOp = [IsolatedLinkOp{}; ISOLATED_LINK_MAX_OPS],
    op_count: usize = 0,
    platform: IsolatedLinkPlatform = IsolatedLinkPlatform.Posix,

    static fn init(platform: IsolatedLinkPlatform): IsolatedLinkPlan {
        return IsolatedLinkPlan{ platform: platform };
    }

    /// 评估并记录一条符号链接对账操作，返回计算得到的动作
    fn planLink(
        *_,
        strategy: IsolatedLinkStrategy,
        dest_path: []const u8,
        dest_kind: IsolatedDestEntryKind,
        current_link: []const u8,
        desired_target: []const u8,
        fallback_junction_target: []const u8,
    ): IsolatedLinkAction! {
        if (dest_path.len == 0 || desired_target.len == 0) {
            return IsolatedSymlinkError.InvalidLinkTarget;
        }
        if (_.op_count >= ISOLATED_LINK_MAX_OPS) {
            return IsolatedSymlinkError.TooManyLinkOps;
        }
        const action = reconcileIsolatedSymlink(
            strategy,
            _.platform,
            dest_kind,
            current_link,
            desired_target,
            fallback_junction_target,
        );
        _.ops[_.op_count] = IsolatedLinkOp{
            dest_path: dest_path,
            desired_target: desired_target,
            action: action,
        };
        _.op_count += 1;
        return action;
    }

    /// 汇总批量计划统计指标（空操作 / 新建 / 替换 / 保留补丁目录）
    fn stats(): IsolatedLinkPlanStats {
        let s = IsolatedLinkPlanStats{ total: _.op_count };
        let i: usize = 0;
        while (i < _.op_count) {
            switch (_.ops[i].action) {
                IsolatedLinkAction.Noop => { s.noop_count += 1; },
                IsolatedLinkAction.CreateSymlink => { s.create_count += 1; },
                IsolatedLinkAction.MkdirAndCreateSymlink => { s.create_count += 1; },
                IsolatedLinkAction.DeleteTreeAndCreateSymlink => { s.replace_count += 1; },
                IsolatedLinkAction.UnlinkAndCreateSymlink => { s.replace_count += 1; },
                IsolatedLinkAction.RmdirAndCreateSymlink => { s.replace_count += 1; },
                IsolatedLinkAction.PreservePatchedDir => { s.preserved_patch_count += 1; },
            }
            i += 1;
        }
        return s;
    }
}
