// Source: Absorbed from /data/dev/bun/src/install/PackageManager/PackageManagerDirectories.cp | Synced: 2026-11-23T20:22:40Z
// Pure-CP zero-allocation Bun package manager cache directory resolver & folder naming engine:
// multi-tier cache/temp directory resolution (explicit > BUN_INSTALL_CACHE_DIR > BUN_INSTALL > XDG > HOME > local),
// NPM/GitHub/Tarball cache folder formatters (`@@@1`, `_patch_hash=...`, long registry host hashing),
// and Global Virtual Store eligibility + symlink store path builder.

/// 当前包缓存目录格式版本号（对齐 Bun `CacheVersion.current = 1`，渲染为 `@@@1`）
pub const PKG_CACHE_VERSION_CURRENT: u32 = 1;

/// 自定义 registry hostname 内联显示最大长度（超过后截断至 12 字节并附带 URL 哈希）
pub const PKG_CACHE_MAX_HOSTNAME_INLINE: usize = 32;

/// Global Virtual Store 共享链接子目录名（`<cache_dir>/links/`）
pub const PKG_CACHE_LINKS_SUBDIR: []const u8 = "links";

/// 跨挂载点 `renameat` 失败时的同盘回退临时目录名（`<cache_dir>/.tmp`）
pub const PKG_CACHE_FALLBACK_TMP_SUBDIR: []const u8 = ".tmp";

/// 缓存目录解析与命名格式化错误集
pub err PkgCacheDirError {
    InvalidPackageName,
    InvalidRegistry,
    InvalidRepo,
    InvalidUrl,
    InvalidPath,
    BufferTooSmall,
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

fn stripTrailingSlashes(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 1 && (s[end - 1] == '/' || s[end - 1] == '\\')) {
        end -= 1;
    }
    return s[0..end];
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool! {
    if (pos >= out.len) {
        return err.BufferTooSmall;
    }
    out[pos] = b;
    pos += 1;
    return true;
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

fn writeU64Decimal(out: []u8, pos: *usize, val: u64): bool! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = @as(u8, @intCast(v % 10)) + '0';
            v /= 10;
            n += 1;
        }
    }
    if (pos + n > out.len) {
        return err.BufferTooSmall;
    }
    let i = n;
    while (i > 0) {
        i -= 1;
        out[pos] = tmp[i];
        pos += 1;
    }
    return true;
}

fn writeHexU64(out: []u8, pos: *usize, val: u64, upper: bool, fixed_16: bool): bool! {
    const digits_lower = "0123456789abcdef";
    const digits_upper = "0123456789ABCDEF";
    const table = if (upper) digits_upper else digits_lower;

    if (fixed_16) {
        if (pos + 16 > out.len) {
            return err.BufferTooSmall;
        }
        let shift: u6 = 60;
        let step: usize = 0;
        while (step < 16) {
            const nib = @as(usize, @intCast((val >> shift) & 0xF));
            out[pos] = table[nib];
            pos += 1;
            if (shift >= 4) {
                shift -= 4;
            }
            step += 1;
        }
        return true;
    }

    let tmp: [16]u8 = [0; 16];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = table[@as(usize, @intCast(v & 0xF))];
            v >>= 4;
            n += 1;
        }
    }
    if (pos + n > out.len) {
        return err.BufferTooSmall;
    }
    let i = n;
    while (i > 0) {
        i -= 1;
        out[pos] = tmp[i];
        pos += 1;
    }
    return true;
}

fn writeCacheVersionTag(out: []u8, pos: *usize): bool! {
    * = writeSlice(out, *pos, "@@@")?;
    * = writeU64Decimal(out, *pos, @as(u64, PKG_CACHE_VERSION_CURRENT))?;
    return true;
}

fn writePatchHashTag(out: []u8, pos: *usize, patch_hash: u64?): bool! {
    if (h := patch_hash) {
        * = writeSlice(out, *pos, "_patch_hash=")?;
        * = writeHexU64(out, *pos, h, false, false)?;
    }
    return true;
}

/// 确定性 FNV-1a 64 位字符串哈希（用于长 registry URL 与 tarball URL 缓存目录摘要）
pub fn hashCacheKey64(s: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < s.len) {
        h ^= @as(u64, s[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

/// 缓存目录命中的来源层级
pub enum PkgCacheDirSource {
    ExplicitConfig,
    EnvCacheDir,
    EnvBunInstall,
    XdgCacheHome,
    HomeDefault,
    LocalNodeModulesFallback,
}

/// 包缓存目录多级环境变量输入
pub struct PkgCacheEnv {
    explicit_cache_dir: ([]const u8)? = null,
    bun_install_cache_dir: ([]const u8)? = null,
    bun_install: ([]const u8)? = null,
    xdg_cache_home: ([]const u8)? = null,
    home_dir: ([]const u8)? = null,
    project_root: ([]const u8)? = null,
    cache_enabled: bool = true,
}

/// 缓存目录路径解析结果
pub struct PkgResolvedCacheDir {
    source: PkgCacheDirSource = PkgCacheDirSource.HomeDefault,
    len: usize = 0,
}

/// 按 Bun 优先级解析全局或项目级包缓存目录路径：
/// 1. 若 `cache_enabled == false`，直接回退到 `<project_root>/node_modules/.cache`；
/// 2. 否则依次回退：`explicit_cache_dir` > `BUN_INSTALL_CACHE_DIR` > `$BUN_INSTALL/install/cache` >
///    `$XDG_CACHE_HOME/bun/install/cache` > `$HOME/.bun/install/cache` > `<project_root>/node_modules/.cache`。
pub fn resolvePkgCacheDirPath(env: PkgCacheEnv, out: []u8): PkgResolvedCacheDir! {
    let pos: usize = 0;

    if (env.cache_enabled) {
        if (exp := env.explicit_cache_dir) {
            if (exp.len > 0) {
                if (hasControlChar(exp)) return err.InvalidPath;
                const clean = stripTrailingSlashes(exp);
                * = writeSlice(out, *pos, clean)?;
                return PkgResolvedCacheDir{ source: PkgCacheDirSource.ExplicitConfig, len: pos };
            }
        }
        if (env_cache := env.bun_install_cache_dir) {
            if (env_cache.len > 0) {
                if (hasControlChar(env_cache)) return err.InvalidPath;
                const clean = stripTrailingSlashes(env_cache);
                * = writeSlice(out, *pos, clean)?;
                return PkgResolvedCacheDir{ source: PkgCacheDirSource.EnvCacheDir, len: pos };
            }
        }
        if (bi := env.bun_install) {
            if (bi.len > 0) {
                if (hasControlChar(bi)) return err.InvalidPath;
                const clean = stripTrailingSlashes(bi);
                * = writeSlice(out, *pos, clean)?;
                * = writeSlice(out, *pos, "/install/cache")?;
                return PkgResolvedCacheDir{ source: PkgCacheDirSource.EnvBunInstall, len: pos };
            }
        }
        if (xdg := env.xdg_cache_home) {
            if (xdg.len > 0) {
                if (hasControlChar(xdg)) return err.InvalidPath;
                const clean = stripTrailingSlashes(xdg);
                * = writeSlice(out, *pos, clean)?;
                * = writeSlice(out, *pos, "/bun/install/cache")?;
                return PkgResolvedCacheDir{ source: PkgCacheDirSource.XdgCacheHome, len: pos };
            }
        }
        if (home := env.home_dir) {
            if (home.len > 0) {
                if (hasControlChar(home)) return err.InvalidPath;
                const clean = stripTrailingSlashes(home);
                * = writeSlice(out, *pos, clean)?;
                * = writeSlice(out, *pos, "/.bun/install/cache")?;
                return PkgResolvedCacheDir{ source: PkgCacheDirSource.HomeDefault, len: pos };
            }
        }
    }

    const root = env.project_root ?? ".";
    if (root.len == 0 || hasControlChar(root)) {
        return err.InvalidPath;
    }
    const clean_root = stripTrailingSlashes(root);
    * = writeSlice(out, *pos, clean_root)?;
    * = writeSlice(out, *pos, "/node_modules/.cache")?;
    return PkgResolvedCacheDir{ source: PkgCacheDirSource.LocalNodeModulesFallback, len: pos };
}

/// 解析解压临时目录：当系统临时目录不可用或跨文件系统 `renameat` 触发 `EXDEV` 时，
/// 自动回退到 `<cache_dir>/.tmp` 以确保原子同盘重命名（对齐 Bun `getTemporaryDirectoryOnce`）。
pub fn resolvePkgTempDirFallback(
    cache_dir: []const u8,
    system_tmp: ([]const u8)?,
    rename_cross_device_failed: bool,
    out: []u8,
): usize! {
    if (cache_dir.len == 0 || hasControlChar(cache_dir)) {
        return err.InvalidPath;
    }
    let pos: usize = 0;
    if (!rename_cross_device_failed) {
        if (sys_tmp := system_tmp) {
            if (sys_tmp.len > 0) {
                if (hasControlChar(sys_tmp)) return err.InvalidPath;
                const clean = stripTrailingSlashes(sys_tmp);
                * = writeSlice(out, *pos, clean)?;
                return pos;
            }
        }
    }

    const clean_cache = stripTrailingSlashes(cache_dir);
    * = writeSlice(out, *pos, clean_cache)?;
    * = writeByte(out, *pos, '/')?;
    * = writeSlice(out, *pos, PKG_CACHE_FALLBACK_TMP_SUBDIR)?;
    return pos;
}

/// 缓存目录命名使用的结构化 Semver（含可选预发布哈希与构建元数据哈希）
pub struct PkgCacheSemver {
    major: u64 = 0,
    minor: u64 = 0,
    patch: u64 = 0,
    pre_hash: u64? = null,
    build_hash: u64? = null,
}

/// 格式化 NPM 包在缓存中的目录名（完全对齐 Bun `cachedNPMPackageFolderNamePrint`）：
/// - 默认 registry：`{name}@{major}.{minor}.{patch}[-{pre_hex_lower}][+{build_hex_upper}]@@@1[_patch_hash={hex}]`
/// - 自定义 registry（host <= 32）：`{name}@{ver}@@{host}@@@1[_patch_hash={hex}]`
/// - 自定义 registry（host > 32）：`{name}@{ver}@@{host[0..12]}__{url_hash}@@@1[_patch_hash={hex}]`
pub fn formatCachedNpmFolderName(
    name: []const u8,
    ver: PkgCacheSemver,
    custom_registry_host: ([]const u8)?,
    custom_registry_url: ([]const u8)?,
    patch_hash: u64?,
    out: []u8,
): usize! {
    if (name.len == 0 || hasControlChar(name)) {
        return err.InvalidPackageName;
    }
    let pos: usize = 0;
    * = writeSlice(out, *pos, name)?;
    * = writeByte(out, *pos, '@')?;
    * = writeU64Decimal(out, *pos, ver.major)?;
    * = writeByte(out, *pos, '.')?;
    * = writeU64Decimal(out, *pos, ver.minor)?;
    * = writeByte(out, *pos, '.')?;
    * = writeU64Decimal(out, *pos, ver.patch)?;

    if (pre := ver.pre_hash) {
        * = writeByte(out, *pos, '-')?;
        * = writeHexU64(out, *pos, pre, false, false)?;
    }
    if (bld := ver.build_hash) {
        * = writeByte(out, *pos, '+')?;
        * = writeHexU64(out, *pos, bld, true, false)?;
    }

    if (host := custom_registry_host) {
        if (host.len == 0 || hasControlChar(host)) {
            return err.InvalidRegistry;
        }
        * = writeSlice(out, *pos, "@@")?;
        if (host.len > PKG_CACHE_MAX_HOSTNAME_INLINE) {
            const visible = host[0..12];
            const url_for_hash = custom_registry_url ?? host;
            * = writeSlice(out, *pos, visible)?;
            * = writeSlice(out, *pos, "__")?;
            * = writeHexU64(out, *pos, hashCacheKey64(url_for_hash), false, false)?;
        } else {
            * = writeSlice(out, *pos, host)?;
        }
    }

    * = writeCacheVersionTag(out, *pos)?;
    * = writePatchHashTag(out, *pos, patch_hash)?;
    return pos;
}

/// 格式化 GitHub 仓库依赖在缓存中的目录名：`@GH@{owner}-{repo}-{committish}@@@1[_patch_hash={hex}]`
pub fn formatCachedGitHubFolderName(
    owner: []const u8,
    repo: []const u8,
    committish: []const u8,
    patch_hash: u64?,
    out: []u8,
): usize! {
    if (owner.len == 0 || repo.len == 0 || committish.len == 0) {
        return err.InvalidRepo;
    }
    if (hasControlChar(owner) || hasControlChar(repo) || hasControlChar(committish)) {
        return err.InvalidRepo;
    }
    let pos: usize = 0;
    * = writeSlice(out, *pos, "@GH@")?;
    * = writeSlice(out, *pos, owner)?;
    * = writeByte(out, *pos, '-')?;
    * = writeSlice(out, *pos, repo)?;
    * = writeByte(out, *pos, '-')?;
    * = writeSlice(out, *pos, committish)?;
    * = writeCacheVersionTag(out, *pos)?;
    * = writePatchHashTag(out, *pos, patch_hash)?;
    return pos;
}

/// 格式化 Tarball URL 依赖在缓存中的目录名：`@T@{url_hash}@@@1[_patch_hash={hex}]`
pub fn formatCachedTarballFolderName(url: []const u8, patch_hash: u64?, out: []u8): usize! {
    if (url.len == 0 || hasControlChar(url)) {
        return err.InvalidUrl;
    }
    let pos: usize = 0;
    * = writeSlice(out, *pos, "@T@")?;
    * = writeHexU64(out, *pos, hashCacheKey64(url), false, false)?;
    * = writeCacheVersionTag(out, *pos)?;
    * = writePatchHashTag(out, *pos, patch_hash)?;
    return pos;
}

/// Global Virtual Store 共享资格判定结果（对齐 Bun `docs/pm/global-store.mdx`）
pub enum PkgGlobalStoreEligibility {
    Eligible,
    IneligiblePatched,
    IneligibleTrusted,
    IneligibleLocalProtocol,
    IneligibleTransitiveLocal,
}

/// Global Virtual Store 共享资格判定输入
pub struct PkgGlobalStoreInput {
    has_patch: bool = false,
    is_trusted: bool = false,
    is_local_protocol: bool = false,
    has_local_transitive_dep: bool = false,
}

/// 判定某包条目是否可安全共享于 Global Virtual Store（`~/.bun/install/cache/links/`）：
/// 任何被 patch、被列为 `trustedDependencies`、本身为 `workspace:`/`file:`/`link:`、
/// 或传递依赖了项目本地协议的条目，均必须回退至项目本地 `node_modules/.bun/`。
pub fn checkGlobalStoreEligibility(input: PkgGlobalStoreInput): PkgGlobalStoreEligibility {
    if (input.has_patch) return PkgGlobalStoreEligibility.IneligiblePatched;
    if (input.is_trusted) return PkgGlobalStoreEligibility.IneligibleTrusted;
    if (input.is_local_protocol) return PkgGlobalStoreEligibility.IneligibleLocalProtocol;
    if (input.has_local_transitive_dep) return PkgGlobalStoreEligibility.IneligibleTransitiveLocal;
    return PkgGlobalStoreEligibility.Eligible;
}

/// 格式化 Global Virtual Store 共享链接目录路径：
/// `<cache_dir>/links/<store_path>-<16-hex-entry-hash>/node_modules`
pub fn formatGlobalStoreLinkDir(
    cache_dir: []const u8,
    store_path: []const u8,
    entry_hash: u64,
    out: []u8,
): usize! {
    if (cache_dir.len == 0 || store_path.len == 0 || hasControlChar(cache_dir) || hasControlChar(store_path)) {
        return err.InvalidPath;
    }
    let pos: usize = 0;
    const clean_cache = stripTrailingSlashes(cache_dir);
    * = writeSlice(out, *pos, clean_cache)?;
    * = writeByte(out, *pos, '/')?;
    * = writeSlice(out, *pos, PKG_CACHE_LINKS_SUBDIR)?;
    * = writeByte(out, *pos, '/')?;
    * = writeSlice(out, *pos, store_path)?;
    * = writeByte(out, *pos, '-')?;
    * = writeHexU64(out, *pos, entry_hash, false, true)?;
    * = writeSlice(out, *pos, "/node_modules")?;
    return pos;
}
