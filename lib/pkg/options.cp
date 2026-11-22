// Source: Absorbed from /data/dev/bun/src/install/PackageManager/PackageManagerOptions.cp | Synced: 2026-11-22T07:28:00Z
// Pure-CP zero-allocation package manager options model (`bun install` / `bun publish` / `bun update`):
// - `PkgLogLevel` (6 progress/verbosity modes)
// - `PkgPublishConfig` (`--access public|restricted`, `--auth-type legacy|web`, `--tag`, `--tolerate-republish`, `--gzip-level`)
// - `PkgDoFlags` & `PkgEnableFlags` (dry-run, frozen-lockfile, exact, only-missing, global virtual store)
// - `PkgDepOmitFlags` (`--omit=dev|optional|peer`, `--production`) & `--minimum-release-age` supply-chain cooldown gate.

/// 包管理配置与发布参数校验错误集
pub err PkgOptionsError {
    InvalidAccess,
    InvalidAuthType,
    UnscopedRestrictedPublish,
    InvalidOmitTarget,
    InvalidLogLevel,
    GlobalStoreRequiresIsolated,
    InvalidGzipLevel,
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 安装日志与进度条详细级别（严格对齐 Bun `PackageManagerOptions.LogLevel`）
pub enum PkgLogLevel: u8 {
    Default = 0,
    Verbose = 1,
    Silent = 2,
    Quiet = 3,
    DefaultNoProgress = 4,
    VerboseNoProgress = 5,

    fn isVerbose(): bool {
        return _ == .Verbose || _ == .VerboseNoProgress;
    }

    fn showProgress(): bool {
        return _ == .Default || _ == .Verbose;
    }

    fn isSilent(): bool {
        return _ == .Silent;
    }

    static fn fromStr(raw: []const u8): PkgLogLevel! {
        if (eqlIgnoreCase(raw, "default")) return .Default;
        if (eqlIgnoreCase(raw, "verbose")) return .Verbose;
        if (eqlIgnoreCase(raw, "silent")) return .Silent;
        if (eqlIgnoreCase(raw, "quiet")) return .Quiet;
        return err.InvalidLogLevel;
    }
}

/// npm 发布访问级别（`--access public|restricted`）
pub enum PkgPublishAccess: u8 {
    Public = 0,
    Restricted = 1,

    static fn fromStr(raw: []const u8): PkgPublishAccess! {
        if (eqlIgnoreCase(raw, "public")) return .Public;
        if (eqlIgnoreCase(raw, "restricted")) return .Restricted;
        return err.InvalidAccess;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Public => "public",
            .Restricted => "restricted",
        };
    }
}

/// npm 发布认证方式（`--auth-type legacy|web`）
pub enum PkgAuthType: u8 {
    Legacy = 0,
    Web = 1,

    static fn fromStr(raw: []const u8): PkgAuthType! {
        if (eqlIgnoreCase(raw, "legacy")) return .Legacy;
        if (eqlIgnoreCase(raw, "web")) return .Web;
        return err.InvalidAuthType;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Legacy => "legacy",
            .Web => "web",
        };
    }
}

/// `bun publish` 发布配置（对齐 Bun `PublishConfig` 与 `docs/pm/cli/publish.mdx`）
pub struct PkgPublishConfig {
    access: PkgPublishAccess? = null,
    tag: []const u8 = "",
    otp: []const u8 = "",
    auth_type: PkgAuthType? = null,
    tolerate_republish: bool = false,
    dry_run: bool = false,
    gzip_level: u8 = 9,

    /// 返回生效 dist-tag（未指定时默认 `"latest"`）
    fn effectiveTag(): []const u8 {
        if (_.tag.len == 0) {
            return "latest";
        }
        return _.tag;
    }

    /// 校验发布配置合法性：
    /// - 非 scoped 包（不以 `@` 开头）严禁使用 `--access restricted`；
    /// - `gzip_level` 必须位于 `0..=9`。
    fn validateForPackage(pkg_name: []const u8)! {
        if (_.gzip_level > 9) {
            return err.InvalidGzipLevel;
        }
        if (acc := _.access) {
            if (acc == .Restricted) {
                const is_scoped = pkg_name.len > 0 && pkg_name[0] == '@';
                if (!is_scoped) {
                    return err.UnscopedRestrictedPublish;
                }
            }
        }
    }
}

/// 安装事务执行行为开关集合（严格对齐 Bun `PackageManagerOptions.Do`）
pub struct PkgDoFlags {
    save_lockfile: bool = true,
    load_lockfile: bool = true,
    install_packages: bool = true,
    write_package_json: bool = true,
    run_scripts: bool = true,
    save_yarn_lock: bool = false,
    print_meta_hash_string: bool = false,
    verify_integrity: bool = true,
    summary: bool = true,
    trust_dependencies_from_args: bool = false,
    update_to_latest: bool = false,
    analyze: bool = false,
    recursive: bool = false,
    prefetch_resolved_tarballs: bool = true,
}

/// 安装特性开关集合（严格对齐 Bun `PackageManagerOptions.Enable`）
pub struct PkgEnableFlags {
    manifest_cache: bool = true,
    manifest_cache_control: bool = true,
    cache: bool = true,
    fail_early: bool = false,
    frozen_lockfile: bool = false,
    force_save_lockfile: bool = false,
    force_install: bool = false,
    exact_versions: bool = false,
    only_missing: bool = false,
    /// 仅对 isolated 链接器生效：启用全局虚拟存储（`<cache>/links/` 软链共享）
    global_virtual_store: bool = false,
}

/// 依赖类型过滤标志（`--omit=dev|optional|peer` 与 `--production`）
pub struct PkgDepOmitFlags {
    omit_dev: bool = false,
    omit_optional: bool = false,
    omit_peer: bool = false,

    fn applyOmitStr(*_, target: []const u8)! {
        if (eqlIgnoreCase(target, "dev")) {
            _.omit_dev = true;
            return;
        }
        if (eqlIgnoreCase(target, "optional")) {
            _.omit_optional = true;
            return;
        }
        if (eqlIgnoreCase(target, "peer")) {
            _.omit_peer = true;
            return;
        }
        return err.InvalidOmitTarget;
    }

    fn applyProduction(*_) {
        _.omit_dev = true;
        _.omit_optional = true;
    }
}

/// 统一包管理器运行选项（吸收自 Bun `PackageManagerOptions.cp`）
pub struct PkgInstallOptions {
    log_level: PkgLogLevel = .Default,
    do_flags: PkgDoFlags = PkgDoFlags{},
    enable: PkgEnableFlags = PkgEnableFlags{},
    omit: PkgDepOmitFlags = PkgDepOmitFlags{},
    publish: PkgPublishConfig = PkgPublishConfig{},
    /// 供应链冷却期安全门禁（秒）：`0` 表示不限制，否则拒绝发布不足该秒数的新版本
    minimum_release_age_secs: u64 = 0,
    max_concurrent_scripts: usize = 0,

    static fn init(): PkgInstallOptions {
        return PkgInstallOptions{};
    }

    /// 启用 `--dry-run`：禁止写入 lockfile、`package.json`、磁盘安装与生命周期脚本
    fn applyDryRun(*_) {
        _.do_flags.save_lockfile = false;
        _.do_flags.install_packages = false;
        _.do_flags.write_package_json = false;
        _.do_flags.run_scripts = false;
        _.publish.dry_run = true;
    }

    /// 启用 `--frozen-lockfile`：强制要求现有锁文件匹配且禁止改写锁文件
    fn applyFrozenLockfile(*_) {
        _.enable.frozen_lockfile = true;
        _.do_flags.save_lockfile = false;
    }

    /// 启用 `--production`：省略 dev 与 optional 依赖
    fn applyProduction(*_) {
        _.omit.applyProduction();
    }

    /// 应用 `--omit=<dev|optional|peer>`
    fn applyOmit(*_, target: []const u8)! {
        _.omit.applyOmitStr(target)?;
    }

    /// 启用全局虚拟存储（仅允许在 `isolated` 链接器下启用，否则返回 `GlobalStoreRequiresIsolated`）
    fn enableGlobalVirtualStore(*_, is_isolated_linker: bool)! {
        if (!is_isolated_linker) {
            return err.GlobalStoreRequiresIsolated;
        }
        _.enable.global_virtual_store = true;
    }

    /// 判定当前安装是否实际使用全局虚拟存储（hoisted 链接器下始终为 `false`）
    fn shouldUseGlobalVirtualStore(is_isolated_linker: bool): bool {
        return is_isolated_linker && _.enable.global_virtual_store;
    }

    /// 供应链冷却期检查（`--minimum-release-age`）：
    /// 若 `minimum_release_age_secs == 0` 始终放行；否则要求 `now_unix_sec >= published_unix_sec + minimum_release_age_secs`。
    fn isReleaseAgeSatisfied(published_unix_sec: u64, now_unix_sec: u64): bool {
        if (_.minimum_release_age_secs == 0) {
            return true;
        }
        if (now_unix_sec < published_unix_sec) {
            return false;
        }
        return (now_unix_sec - published_unix_sec) >= _.minimum_release_age_secs;
    }
}
