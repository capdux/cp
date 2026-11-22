// Source: Absorbed from /data/dev/bun/src/install/PackageManager/PackageManagerResolution.cp | Synced: 2026-11-22T14:15:00Z
// Pure-CP zero-allocation Package Manager Resolution Assignment,
// Scoped Registry Routing, Resolution Graph Verification & Disk-Cache Version Picker.
// Aligned with Bun `src/install/PackageManager/PackageManagerResolution.cp`
// and `docs/guides/install/registry-scope.mdx`:
// - `ScopedRegistryRouter`: extracts `@scope` from package names and routes to configured
//   scoped registries (with fallback to default registry), supporting `@scope` normalization.
// - `ResolutionVerifier`: performs `assignResolution` / `assignRootResolution` (with automatic
//   dependency name/hash backfill when empty or equal to version literal) and `verifyResolutions`
//   (skipping resolved/peer/optional/disabled deps according to local vs remote feature masks).
// - `selectBestCachedSemver`: picks the highest compatible cached Semver version from disk cache
//   in descending order (`Semver.Version.sortGt`).

/// 解析验证表最大包数
pub const RES_VERIFY_MAX_PKGS: usize = 64;
/// 解析验证表最大依赖数
pub const RES_VERIFY_MAX_DEPS: usize = 128;
/// 最大 Scoped Registry 路由规则数
pub const RES_VERIFY_MAX_SCOPES: usize = 16;
/// 单次校验记录的最大失败依赖数
pub const RES_VERIFY_MAX_FAILURES: usize = 32;
/// 无效包 ID 哨兵值（对齐 Bun `invalid_package_id`）
pub const RES_VERIFY_INVALID_PKG_ID: u32 = 0xFFFF_FFFF;

/// 依赖特性掩码位（对齐 Bun `Dependency.Behavior` / `Features`）
pub const RES_FEAT_NORMAL: u8 = 0x01;
pub const RES_FEAT_DEV: u8 = 0x02;
pub const RES_FEAT_OPTIONAL: u8 = 0x04;
pub const RES_FEAT_PEER: u8 = 0x08;
pub const RES_FEAT_LOCAL_DEFAULT: u8 = RES_FEAT_NORMAL | RES_FEAT_DEV | RES_FEAT_OPTIONAL | RES_FEAT_PEER;
pub const RES_FEAT_REMOTE_DEFAULT: u8 = RES_FEAT_NORMAL | RES_FEAT_OPTIONAL | RES_FEAT_PEER;

/// 解析赋值与验证错误集
pub err ResVerifyError {
    TooManyPackages,
    TooManyDependencies,
    TooManyScopes,
    InvalidPackageId,
    InvalidDependencyId,
    RootResolutionAlreadyAssigned,
    EmptyScopeName,
    EmptyRegistryUrl,
    BufferTooSmall,
}

/// 父包来源类型（决定使用 `local_features` 还是 `remote_features`）
pub enum ResParentKind: u8 {
    Root = 0,
    Workspace = 1,
    Folder = 2,
    Npm = 3,
    Git = 4,
    Tarball = 5,

    /// 判断是否为本地包（本地包启用 `local_package_features`，如 `devDependencies`）
    fn isLocal(): bool {
        return _ == .Root || _ == .Workspace || _ == .Folder;
    }
}

/// 包节点记录
pub struct ResPkgEntry {
    name: []const u8 = "",
    name_hash: u64 = 0,
    kind: ResParentKind = .Npm,
    dep_begin: u32 = 0,
    dep_len: u32 = 0,
}

/// 依赖边记录（含名称、字面量版本与行为掩码）
pub struct ResDepEntry {
    name: []const u8 = "",
    name_hash: u64 = 0,
    version_literal: []const u8 = "",
    behavior_mask: u8 = RES_FEAT_NORMAL,
    is_optional: bool = false,
    is_peer: bool = false,
}

/// 单条未解析失败的依赖诊断项
pub struct UnresolvedDepFailure {
    parent_pkg_id: u32 = 0,
    dep_id: u32 = 0,
    name: []const u8 = "",
    version_literal: []const u8 = "",
}

/// 校验结果摘要
pub struct ResVerifyReport {
    total_checked: usize = 0,
    resolved_count: usize = 0,
    skipped_peer_count: usize = 0,
    skipped_optional_or_disabled: usize = 0,
    failure_count: usize = 0,
    failures: [RES_VERIFY_MAX_FAILURES]UnresolvedDepFailure = [UnresolvedDepFailure{}; RES_VERIFY_MAX_FAILURES],

    /// 返回是否全部必需依赖均已成功解析
    fn isOk(): bool {
        return _.failure_count == 0;
    }

    /// 返回失败诊断条目切片
    fn failureSlice(): []const UnresolvedDepFailure {
        return _.failures[0.._.failure_count];
    }
}

/// Scoped Registry 路由条目
pub struct ScopedRegistryEntry {
    /// 规范化 scope 名称（不含前导 `@`，如 `"myorg"`）
    scope_name: []const u8 = "",
    registry_url: []const u8 = "",
    has_auth_token: bool = false,
}

/// 轻量级磁盘缓存 Semver 候选条目（零堆分配）
pub struct CachedSemverCandidate {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,
    is_prerelease: bool = false,
    index: usize = 0,

    /// 比较两个 Semver 版本大小（返回 -1 / 0 / 1；正式版大于同版本号预发布版）
    fn cmp(other: CachedSemverCandidate): i32 {
        if (_.major != other.major) return if (_.major > other.major) 1 else -1;
        if (_.minor != other.minor) return if (_.minor > other.minor) 1 else -1;
        if (_.patch != other.patch) return if (_.patch > other.patch) 1 else -1;
        if (_.is_prerelease != other.is_prerelease) {
            return if (!_.is_prerelease) 1 else -1;
        }
        return 0;
    }
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

/// 计算包名 64 位 FNV-1a 哈希
pub fn pkgNameHash64(name: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    for (i of 0..name.len) {
        h = h ^ (name[i] as u64);
        h = h *% 1099511628211;
    }
    return h;
}

/// 从 npm 包名提取 scope 名称（如 `"@myorg/pkg"` -> `"myorg"`；非 scoped 包返回 `""`）
pub fn extractPackageScope(pkg_name: []const u8): []const u8 {
    if (pkg_name.len < 2 || pkg_name[0] != '@') {
        return "";
    }
    for (i of 1..pkg_name.len) {
        if (pkg_name[i] == '/') {
            return pkg_name[1..i];
        }
    }
    // 允许直接传入 `"@myorg"` 形式
    return pkg_name[1..];
}

/// 按 Bun `verifyResolutions` 规则格式化未解析依赖错误信息：
/// 若 `name` 为空或等于 `version_literal`，输出 `"<literal> failed to resolve"`；
/// 否则输出 `"<name>@<literal> failed to resolve"`。
pub fn formatUnresolvedDepError(name: []const u8, version_literal: []const u8, out: []u8): usize! {
    let pos: usize = 0;
    if (name.len == 0 || bytesEql(name, version_literal)) {
        pos = copyInto(out, pos, version_literal)?;
        return copyInto(out, pos, " failed to resolve");
    }
    pos = copyInto(out, pos, name)?;
    pos = copyInto(out, pos, "@")?;
    pos = copyInto(out, pos, version_literal)?;
    return copyInto(out, pos, " failed to resolve");
}

/// 从磁盘缓存候选版本中选择满足 `[min_ver, max_excl_major]` 范围的最高版本索引
/// （对齐 Bun `resolveFromDiskCache` 的降序匹配策略；默认跳过 prerelease 除非显式允许）
pub fn selectBestCachedSemver(
    candidates: []const CachedSemverCandidate,
    min_ver: CachedSemverCandidate,
    max_major_excl: u32,
    allow_prerelease: bool,
): usize? {
    let best_pos: usize? = null;
    for (i of 0..candidates.len) {
        const c = candidates[i];
        if (c.is_prerelease && !allow_prerelease) continue;
        if (c.cmp(min_ver) < 0) continue;
        if (max_major_excl > 0 && c.major >= max_major_excl) continue;

        if (bp := best_pos) {
            if (c.cmp(candidates[bp]) > 0) {
                best_pos = i;
            }
        } else {
            best_pos = i;
        }
    }
    return best_pos;
}

/// 零堆分配 Scoped Registry 路由器（对齐 Bun `scopeForPackageName` & `bunfig.toml [install.scopes]`）
pub struct ScopedRegistryRouter {
    default_registry_url: []const u8 = "https://registry.npmjs.org/",
    scopes: [RES_VERIFY_MAX_SCOPES]ScopedRegistryEntry = [ScopedRegistryEntry{}; RES_VERIFY_MAX_SCOPES],
    scope_count: usize = 0,

    static fn init(default_registry_url: []const u8): ScopedRegistryRouter {
        return ScopedRegistryRouter{
            default_registry_url: if (default_registry_url.len > 0) default_registry_url else "https://registry.npmjs.org/",
        };
    }

    /// 注册 `@scope` 对应的私有 Registry URL（自动剥离前导 `@`）
    fn addScope(*_, raw_scope: []const u8, registry_url: []const u8, has_auth_token: bool): usize! {
        let scope = raw_scope;
        if (scope.len > 0 && scope[0] == '@') {
            scope = scope[1..];
        }
        if (scope.len == 0) return err.EmptyScopeName;
        if (registry_url.len == 0) return err.EmptyRegistryUrl;

        // 同名 scope 覆盖更新
        for (i of 0.._.scope_count) {
            if (bytesEql(_.scopes[i].scope_name, scope)) {
                _.scopes[i].registry_url = registry_url;
                _.scopes[i].has_auth_token = has_auth_token;
                return i;
            }
        }
        if (_.scope_count >= RES_VERIFY_MAX_SCOPES) {
            return err.TooManyScopes;
        }
        const idx = _.scope_count;
        _.scopes[idx] = ScopedRegistryEntry{
            scope_name: scope,
            registry_url: registry_url,
            has_auth_token: has_auth_token,
        };
        _.scope_count += 1;
        return idx;
    }

    /// 根据包名路由至匹配的 Registry URL（非 scoped 或未命中时回退至默认 Registry）
    fn resolveRegistryUrl(pkg_name: []const u8): []const u8 {
        const scope = extractPackageScope(pkg_name);
        if (scope.len == 0) {
            return _.default_registry_url;
        }
        for (i of 0.._.scope_count) {
            if (bytesEql(_.scopes[i].scope_name, scope)) {
                return _.scopes[i].registry_url;
            }
        }
        return _.default_registry_url;
    }
}

/// 零堆分配包解析赋值与一致性验证器（对齐 Bun `assignResolution`/`assignRootResolution`/`verifyResolutions`）
pub struct ResolutionVerifier {
    pkgs: [RES_VERIFY_MAX_PKGS]ResPkgEntry = [ResPkgEntry{}; RES_VERIFY_MAX_PKGS],
    pkg_count: usize = 0,

    deps: [RES_VERIFY_MAX_DEPS]ResDepEntry = [ResDepEntry{}; RES_VERIFY_MAX_DEPS],
    resolutions: [RES_VERIFY_MAX_DEPS]u32 = [RES_VERIFY_INVALID_PKG_ID; RES_VERIFY_MAX_DEPS],
    dep_count: usize = 0,

    local_features: u8 = RES_FEAT_LOCAL_DEFAULT,
    remote_features: u8 = RES_FEAT_REMOTE_DEFAULT,

    static fn init(local_features: u8, remote_features: u8): ResolutionVerifier {
        return ResolutionVerifier{
            local_features: local_features,
            remote_features: remote_features,
        };
    }

    /// 注册包节点，自动计算 `name_hash`（若未指定）
    fn addPackage(*_, pkg: ResPkgEntry): u32! {
        if (_.pkg_count >= RES_VERIFY_MAX_PKGS) {
            return err.TooManyPackages;
        }
        const idx = _.pkg_count;
        _.pkgs[idx] = pkg;
        if (_.pkgs[idx].name_hash == 0 && _.pkgs[idx].name.len > 0) {
            _.pkgs[idx].name_hash = pkgNameHash64(_.pkgs[idx].name);
        }
        const id = idx as u32;
        _.pkg_count += 1;
        return id;
    }

    /// 注册依赖边，初始解析状态为 `RES_VERIFY_INVALID_PKG_ID`
    fn addDependency(*_, dep: ResDepEntry): u32! {
        if (_.dep_count >= RES_VERIFY_MAX_DEPS) {
            return err.TooManyDependencies;
        }
        const idx = _.dep_count;
        _.deps[idx] = dep;
        if (_.deps[idx].name_hash == 0 && _.deps[idx].name.len > 0) {
            _.deps[idx].name_hash = pkgNameHash64(_.deps[idx].name);
        }
        _.resolutions[idx] = RES_VERIFY_INVALID_PKG_ID;
        const id = idx as u32;
        _.dep_count += 1;
        return id;
    }

    /// 读取指定依赖记录
    fn getDependency(dep_id: u32): ResDepEntry? {
        const idx = dep_id as usize;
        if (idx >= _.dep_count) return null;
        return _.deps[idx];
    }

    /// 读取指定依赖解析结果
    fn getResolution(dep_id: u32): u32? {
        const idx = dep_id as usize;
        if (idx >= _.dep_count) return null;
        return _.resolutions[idx];
    }

    /// 为依赖边赋值解析目标包，并在 `dep.name` 为空或等于 `version_literal` 时回填包名与哈希
    fn assignResolution(*_, dep_id: u32, pkg_id: u32): u32! {
        const d_idx = dep_id as usize;
        const p_idx = pkg_id as usize;
        if (d_idx >= _.dep_count) return err.InvalidDependencyId;
        if (p_idx >= _.pkg_count) return err.InvalidPackageId;

        _.resolutions[d_idx] = pkg_id;
        const dep = _.deps[d_idx];
        if (dep.name.len == 0 || bytesEql(dep.name, dep.version_literal)) {
            _.deps[d_idx].name = _.pkgs[p_idx].name;
            _.deps[d_idx].name_hash = _.pkgs[p_idx].name_hash;
        }
        return pkg_id;
    }

    /// 为根依赖边赋值解析目标包（严格要求该槽位此前尚未赋值，防止静默覆盖）
    fn assignRootResolution(*_, dep_id: u32, pkg_id: u32): u32! {
        const d_idx = dep_id as usize;
        if (d_idx >= _.dep_count) return err.InvalidDependencyId;
        if (_.resolutions[d_idx] != RES_VERIFY_INVALID_PKG_ID) {
            return err.RootResolutionAlreadyAssigned;
        }
        return _.assignResolution(dep_id, pkg_id);
    }

    /// 全图校验全部父包的依赖解析完整性（对齐 Bun `verifyResolutions`）
    fn verifyResolutions(): ResVerifyReport {
        let report = ResVerifyReport{};
        const end_id = _.pkg_count as u32;

        for (parent_idx of 0.._.pkg_count) {
            const parent = _.pkgs[parent_idx];
            const features = if (parent.kind.isLocal()) _.local_features else _.remote_features;
            const begin = parent.dep_begin as usize;
            const end = begin + (parent.dep_len as usize);

            let d_idx = begin;
            while (d_idx < end && d_idx < _.dep_count) {
                report.total_checked += 1;
                const resolved_id = _.resolutions[d_idx];
                const dep = _.deps[d_idx];

                if (resolved_id < end_id) {
                    report.resolved_count += 1;
                    d_idx += 1;
                    continue;
                }
                if (dep.is_peer) {
                    report.skipped_peer_count += 1;
                    d_idx += 1;
                    continue;
                }
                const enabled = (dep.behavior_mask & features) != 0;
                if (dep.is_optional || !enabled) {
                    report.skipped_optional_or_disabled += 1;
                    d_idx += 1;
                    continue;
                }

                if (report.failure_count < RES_VERIFY_MAX_FAILURES) {
                    report.failures[report.failure_count] = UnresolvedDepFailure{
                        parent_pkg_id: parent_idx as u32,
                        dep_id: d_idx as u32,
                        name: dep.name,
                        version_literal: dep.version_literal,
                    };
                }
                report.failure_count += 1;
                d_idx += 1;
            }
        }
        return report;
    }
}
