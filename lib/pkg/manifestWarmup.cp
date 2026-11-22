// Source: Absorbed from /data/dev/bun/src/install/PackageManager/PopulateManifestCache.cp | Synced: 2026-11-22T14:05:00Z
// Pure-CP zero-allocation Package Manager Manifest Cache Warmup Planner
// & Supply-Chain `minimumReleaseAge` Gate / Stability Evaluator.
// Aligned with Bun `src/install/PackageManager/PopulateManifestCache.cp`
// and `docs/pm/cli/install.mdx`:
// - Supports both `.AllLockfile` (full lockfile scan with `pkg_id` deduplication) and
//   `.RootDirectIds` (direct dependencies of selected root/workspace packages).
// - Filters out invalid package IDs and non-npm resolutions (git/github/tarball/folder/workspace).
// - Automatically requests extended manifests (containing `time` timestamps) when
//   `minimum_release_age_secs` is active, treating `AbbreviatedOnly` cache entries as misses.
// - Deduplicates network manifest tasks by deterministic package-name hash, upgrading
//   an existing optional task to required when a non-optional dependency references the same package.
// - Evaluates `minimumReleaseAge`, `minimumReleaseAgeExcludes`, exact-version stability bypass,
//   7-day rapid-release stability check, and missing-timestamp fallback.

/// 单个预热规划器支持的最大包节点数
pub const MANIFEST_WARMUP_MAX_PKGS: usize = 64;
/// 单个预热规划器支持的最大依赖边数
pub const MANIFEST_WARMUP_MAX_DEPS: usize = 128;
/// 单次预热规划产出的最大网络任务数
pub const MANIFEST_WARMUP_MAX_TASKS: usize = 64;
/// `minimumReleaseAgeExcludes` 白名单最大条目数
pub const MANIFEST_WARMUP_MAX_EXCLUDES: usize = 16;
/// 无效包 ID 哨兵值（对齐 Bun `invalid_package_id`）
pub const MANIFEST_WARMUP_INVALID_PKG_ID: u32 = 0xFFFF_FFFF;
/// 稳定性检查窗口（7 天，单位秒，对齐 Bun `minimumReleaseAge` 文档）
pub const MANIFEST_WARMUP_STABILITY_WINDOW_SECS: u64 = 7 * 86400;

/// Manifest 缓存预热规划器错误集
pub err ManifestWarmupError {
    TooManyPackages,
    TooManyDependencies,
    TooManyTasks,
    TooManyExcludes,
    InvalidPackageId,
    EmptyPackageName,
}

/// 包解析来源类型（对齐 Bun `Resolution.Tag`）
pub enum WarmupResolutionKind: u8 {
    Npm = 0,
    Git = 1,
    Github = 2,
    Tarball = 3,
    Folder = 4,
    Workspace = 5,
    Root = 6,
}

/// 本地 Manifest 缓存命中状态
pub enum WarmupCacheState: u8 {
    /// 内存与磁盘均未命中
    Miss = 0,
    /// 内存中存在完整（含扩展）缓存
    MemoryHit = 1,
    /// 磁盘中存在完整（含扩展）缓存
    DiskHit = 2,
    /// 仅缓存了精简版（abbreviated）manifest；启用 `minimumReleaseAge` 时需回源拉取扩展版
    AbbreviatedOnly = 3,
}

/// 供应链最小发布年龄门控裁决结果
pub enum ReleaseAgeDecision: u8 {
    /// 满足最小发布年龄及稳定性要求，允许解析
    Allowed = 0,
    /// 命中 `minimumReleaseAgeExcludes` 信任白名单，跳过门控
    AllowedExcluded = 1,
    /// Registry 未提供 `time` 字段，按 Bun 策略默认放行
    AllowedNoTimestamp = 2,
    /// 发布时间新于 `minimumReleaseAge` 阈值，拦截
    BlockedTooNew = 3,
    /// 处于门控外 7 天稳定性窗口且存在密集快速修复版本，拦截以回退至更成熟版本
    BlockedUnstableRapidRelease = 4,
}

/// Lockfile 中的包节点元数据
pub struct WarmupPkgNode {
    name: []const u8 = "",
    resolution: WarmupResolutionKind = .Npm,
    dep_begin: u32 = 0,
    dep_len: u32 = 0,
    cache_state: WarmupCacheState = .Miss,
}

/// Lockfile 中的依赖边记录
pub struct WarmupDepEdge {
    resolved_pkg_id: u32 = MANIFEST_WARMUP_INVALID_PKG_ID,
    is_optional: bool = false,
}

/// 待调度的网络 Manifest 拉取任务
pub struct WarmupTaskItem {
    pkg_id: u32 = 0,
    pkg_name: []const u8 = "",
    is_optional: bool = false,
    needs_extended_manifest: bool = false,
    task_hash: u64 = 0,
}

/// Manifest 预热与发布年龄门控配置
pub struct WarmupOptions {
    /// 最小发布年龄（秒）；非 `null` 时强制拉取含 `time` 字段的 extended manifest
    minimum_release_age_secs: u64? = null,
    exclude_count: usize = 0,
    excludes: [MANIFEST_WARMUP_MAX_EXCLUDES][]const u8 = [""; MANIFEST_WARMUP_MAX_EXCLUDES],

    /// 追加受信任白名单包名（`minimumReleaseAgeExcludes`）
    fn addExclude(*_, pkg_name: []const u8): usize! {
        if (pkg_name.len == 0) return err.EmptyPackageName;
        if (_.exclude_count >= MANIFEST_WARMUP_MAX_EXCLUDES) {
            return err.TooManyExcludes;
        }
        const idx = _.exclude_count;
        _.excludes[idx] = pkg_name;
        _.exclude_count += 1;
        return idx;
    }

    /// 判断包名是否位于 `minimumReleaseAgeExcludes` 白名单中
    fn isExcluded(pkg_name: []const u8): bool {
        for (i of 0.._.exclude_count) {
            if (bytesEql(_.excludes[i], pkg_name)) return true;
        }
        return false;
    }

    /// 判断当前配置是否要求拉取含发布时间戳的 extended manifest
    fn needsExtendedManifest(): bool {
        if (age := _.minimum_release_age_secs) {
            return age > 0;
        }
        return false;
    }
}

/// 单次预热规划统计摘要
pub struct WarmupPlanStats {
    scanned_deps: usize = 0,
    skipped_invalid_pkg: usize = 0,
    skipped_non_npm: usize = 0,
    skipped_duplicate_pkg: usize = 0,
    skipped_cache_hit: usize = 0,
    enqueued_tasks: usize = 0,
    extended_manifest_tasks: usize = 0,
    optional_upgraded_to_required: usize = 0,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// 基于包名计算确定性 64 位 FNV-1a Manifest 任务去重哈希（对齐 `Task.Id.forManifest`）
pub fn manifestTaskHash(pkg_name: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    for (i of 0..pkg_name.len) {
        h = h ^ (pkg_name[i] as u64);
        h = h *% 1099511628211;
    }
    return h;
}

/// 判断给定缓存状态是否满足当前预热请求（`needs_extended` 时 `AbbreviatedOnly` 视为未命中）
pub fn isManifestCacheSatisfied(state: WarmupCacheState, needs_extended: bool): bool {
    return switch (state) {
        .Miss => false,
        .MemoryHit => true,
        .DiskHit => true,
        .AbbreviatedOnly => !needs_extended,
    };
}

/// 按 Bun `minimumReleaseAge` 规则裁决单个包版本是否允许解析（对齐 `/data/dev/bun/docs/pm/cli/install.mdx`）
pub fn evaluateReleaseAgeGate(
    pkg_name: []const u8,
    published_unix_sec: u64?,
    now_unix_sec: u64,
    is_exact_request: bool,
    has_rapid_followup_in_window: bool,
    opts: WarmupOptions,
): ReleaseAgeDecision {
    const min_age = opts.minimum_release_age_secs ?? 0;
    if (min_age == 0) {
        return .Allowed;
    }
    if (opts.isExcluded(pkg_name)) {
        return .AllowedExcluded;
    }
    const pub_ts = published_unix_sec ?? {
        return .AllowedNoTimestamp;
    };
    if (pub_ts > now_unix_sec) {
        return .BlockedTooNew;
    }
    const age = now_unix_sec - pub_ts;
    if (age < min_age) {
        return .BlockedTooNew;
    }
    // 精确版本请求（如 `pkg@1.1.1`）遵守年龄门控，但绕过 7 天稳定性检查
    if (!is_exact_request && has_rapid_followup_in_window) {
        const extra_age = age - min_age;
        if (extra_age <= MANIFEST_WARMUP_STABILITY_WINDOW_SECS) {
            return .BlockedUnstableRapidRelease;
        }
    }
    return .Allowed;
}

/// 零堆分配 Manifest Cache 预热规划器（对齐 Bun `populateManifestCache`）
pub struct ManifestWarmupPlanner {
    pkgs: [MANIFEST_WARMUP_MAX_PKGS]WarmupPkgNode = [WarmupPkgNode{}; MANIFEST_WARMUP_MAX_PKGS],
    pkg_count: usize = 0,

    deps: [MANIFEST_WARMUP_MAX_DEPS]WarmupDepEdge = [WarmupDepEdge{}; MANIFEST_WARMUP_MAX_DEPS],
    dep_count: usize = 0,

    task_list: [MANIFEST_WARMUP_MAX_TASKS]WarmupTaskItem = [WarmupTaskItem{}; MANIFEST_WARMUP_MAX_TASKS],
    task_count: usize = 0,

    static fn init(): ManifestWarmupPlanner {
        return ManifestWarmupPlanner{};
    }

    /// 注册包节点，返回其 `PackageID`
    fn addPackage(*_, pkg: WarmupPkgNode): u32! {
        if (pkg.name.len == 0) return err.EmptyPackageName;
        if (_.pkg_count >= MANIFEST_WARMUP_MAX_PKGS) {
            return err.TooManyPackages;
        }
        const id = _.pkg_count as u32;
        _.pkgs[_.pkg_count] = pkg;
        _.pkg_count += 1;
        return id;
    }

    /// 注册依赖边，返回其 `DependencyID`
    fn addDependency(*_, edge: WarmupDepEdge): u32! {
        if (_.dep_count >= MANIFEST_WARMUP_MAX_DEPS) {
            return err.TooManyDependencies;
        }
        const id = _.dep_count as u32;
        _.deps[_.dep_count] = edge;
        _.dep_count += 1;
        return id;
    }

    /// 返回最近一次规划生成的待调度任务切片
    fn tasks(): []const WarmupTaskItem {
        return _.task_list[0.._.task_count];
    }

    fn enqueueTask(*_, pkg_id: u32, pkg_name: []const u8, is_optional: bool, needs_extended: bool, stats: *WarmupPlanStats): usize! {
        const th = manifestTaskHash(pkg_name);
        // 对齐 `hasCreatedNetworkTask(task_id, dep.behavior.optional)`：
        // 若同名任务已入队，避免重复请求；若原先是 optional 而本次为 required，则升级为 required
        for (i of 0.._.task_count) {
            if (_.task_list[i].task_hash == th && bytesEql(_.task_list[i].pkg_name, pkg_name)) {
                if (_.task_list[i].is_optional && !is_optional) {
                    _.task_list[i].is_optional = false;
                    stats.optional_upgraded_to_required += 1;
                }
                if (!_.task_list[i].needs_extended_manifest && needs_extended) {
                    _.task_list[i].needs_extended_manifest = true;
                    stats.extended_manifest_tasks += 1;
                }
                return i;
            }
        }
        if (_.task_count >= MANIFEST_WARMUP_MAX_TASKS) {
            return err.TooManyTasks;
        }
        const idx = _.task_count;
        _.task_list[idx] = WarmupTaskItem{
            pkg_id: pkg_id,
            pkg_name: pkg_name,
            is_optional: is_optional,
            needs_extended_manifest: needs_extended,
            task_hash: th,
        };
        _.task_count += 1;
        stats.enqueued_tasks += 1;
        if (needs_extended) {
            stats.extended_manifest_tasks += 1;
        }
        return idx;
    }

    /// 扫描全量 Lockfile 依赖规划预热任务（对齐 Bun `Packages.all`）
    fn planAll(*_, opts: WarmupOptions): WarmupPlanStats! {
        _.task_count = 0;
        let stats = WarmupPlanStats{};
        let seen: [MANIFEST_WARMUP_MAX_PKGS]bool = [false; MANIFEST_WARMUP_MAX_PKGS];
        const needs_extended = opts.needsExtendedManifest();

        for (dep_idx of 0.._.dep_count) {
            stats.scanned_deps += 1;
            const edge = _.deps[dep_idx];
            if (edge.resolved_pkg_id == MANIFEST_WARMUP_INVALID_PKG_ID || (edge.resolved_pkg_id as usize) >= _.pkg_count) {
                stats.skipped_invalid_pkg += 1;
                continue;
            }
            const pid = edge.resolved_pkg_id as usize;
            const pkg = _.pkgs[pid];
            if (pkg.resolution != .Npm) {
                stats.skipped_non_npm += 1;
                continue;
            }
            if (seen[pid]) {
                // 若先前以 optional 入队而本次为 required，仍允许升级任务优先级
                if (!edge.is_optional) {
                    const pkg_needs_ext = needs_extended && !opts.isExcluded(pkg.name);
                    if (!isManifestCacheSatisfied(pkg.cache_state, pkg_needs_ext)) {
                        * = _.enqueueTask(edge.resolved_pkg_id, pkg.name, false, pkg_needs_ext, &stats)?;
                    }
                }
                stats.skipped_duplicate_pkg += 1;
                continue;
            }
            seen[pid] = true;

            const pkg_needs_ext = needs_extended && !opts.isExcluded(pkg.name);
            if (isManifestCacheSatisfied(pkg.cache_state, pkg_needs_ext)) {
                stats.skipped_cache_hit += 1;
                continue;
            }

            * = _.enqueueTask(edge.resolved_pkg_id, pkg.name, edge.is_optional, pkg_needs_ext, &stats)?;
        }

        return stats;
    }

    /// 仅针对指定根包 ID 列表的直接依赖规划预热任务（对齐 Bun `Packages.ids`）
    fn planForRootIds(*_, root_ids: []const u32, opts: WarmupOptions): WarmupPlanStats! {
        _.task_count = 0;
        let stats = WarmupPlanStats{};
        const needs_extended = opts.needsExtendedManifest();

        for (r_idx of 0..root_ids.len) {
            const root_id = root_ids[r_idx];
            if (root_id == MANIFEST_WARMUP_INVALID_PKG_ID || (root_id as usize) >= _.pkg_count) {
                return err.InvalidPackageId;
            }
            const root_pkg = _.pkgs[root_id as usize];
            const begin = root_pkg.dep_begin as usize;
            const end = begin + (root_pkg.dep_len as usize);

            let dep_idx = begin;
            while (dep_idx < end) {
                if (dep_idx >= _.dep_count) {
                    dep_idx += 1;
                    continue;
                }
                stats.scanned_deps += 1;
                const edge = _.deps[dep_idx];
                if (edge.resolved_pkg_id == MANIFEST_WARMUP_INVALID_PKG_ID || (edge.resolved_pkg_id as usize) >= _.pkg_count) {
                    stats.skipped_invalid_pkg += 1;
                    dep_idx += 1;
                    continue;
                }
                const pkg = _.pkgs[edge.resolved_pkg_id as usize];
                if (pkg.resolution != .Npm) {
                    stats.skipped_non_npm += 1;
                    dep_idx += 1;
                    continue;
                }
                const pkg_needs_ext = needs_extended && !opts.isExcluded(pkg.name);
                if (isManifestCacheSatisfied(pkg.cache_state, pkg_needs_ext)) {
                    stats.skipped_cache_hit += 1;
                    dep_idx += 1;
                    continue;
                }
                * = _.enqueueTask(edge.resolved_pkg_id, pkg.name, edge.is_optional, pkg_needs_ext, &stats)?;
                dep_idx += 1;
            }
        }

        return stats;
    }
}
