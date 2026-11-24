// Source: Absorbed from /data/dev/bun/src/install/PackageManager/CommandLineArguments.cp | Synced: 2026-11-23T20:22:40Z
// Pure-CP zero-allocation Package Manager CLI argument parser & validator.
// Models Bun's `install`, `add`, `remove`, `update`, `pm`, `link`, `audit`, `patch` CLI flags:
// - Supports `--flag=value`, `--flag value`, clustered short flags (`-dE`, `-pf`), and `--` separator
// - Enforces subcommand-specific flag allowlists, mutual-exclusion checks, and integer range guards
// - Normalizes `--production`/`-p`/`-P` into `omit_dev`, and tracks repeated `--omit`/`--filter`/`--cpu`/`--os`

/// 单次命令支持的最大位置参数数量（包名/路径）
pub const PKG_CLI_MAX_POSITIONALS: usize = 32;
/// 单次命令支持的最大 `--filter` 模式数量
pub const PKG_CLI_MAX_FILTERS: usize = 16;
/// 单次命令支持的最大 `--cpu` / `--os` 覆盖项数量
pub const PKG_CLI_MAX_PLATFORMS: usize = 8;
/// 默认最大并发网络请求数（对齐 Bun `--network-concurrency` 默认值 48）
pub const PKG_CLI_DEFAULT_NETWORK_CONCURRENCY: u32 = 48;

/// 包管理器 CLI 参数解析错误集
pub err PkgCliArgsError {
    UnknownFlag,
    MissingFlagValue,
    InvalidFlagValue,
    FlagNotAllowedForSubcommand,
    TooManyPositionals,
    TooManyFilters,
    TooManyPlatforms,
    ConflictingFlags,
}

/// 包管理器子命令种类（对齐 Bun `PackageManager.Subcommand`）
pub enum PkgCliSubcommand: u8 {
    Install = 0,
    Add = 1,
    Remove = 2,
    Update = 3,
    Pm = 4,
    Link = 5,
    Audit = 6,
    Patch = 7,

    /// 解析子命令及其常见别名（`i`/`install`, `a`/`add`, `rm`/`remove`/`uninstall`, `up`/`update`）
    static fn parse(raw: []const u8): PkgCliSubcommand? {
        if (bytesEql(raw, "install") || bytesEql(raw, "i")) return .Install;
        if (bytesEql(raw, "add") || bytesEql(raw, "a")) return .Add;
        if (bytesEql(raw, "remove") || bytesEql(raw, "rm") || bytesEql(raw, "uninstall")) return .Remove;
        if (bytesEql(raw, "update") || bytesEql(raw, "up")) return .Update;
        if (bytesEql(raw, "pm")) return .Pm;
        if (bytesEql(raw, "link")) return .Link;
        if (bytesEql(raw, "audit")) return .Audit;
        if (bytesEql(raw, "patch")) return .Patch;
        return null;
    }

    /// 返回规范子命令名称
    fn asStr(): []const u8 {
        return switch (_) {
            .Install => "install",
            .Add => "add",
            .Remove => "remove",
            .Update => "update",
            .Pm => "pm",
            .Link => "link",
            .Audit => "audit",
            .Patch => "patch",
        };
    }
}

/// 日志详细级别
pub enum PkgCliLogLevel: u8 {
    Default = 0,
    Silent = 1,
    Quiet = 2,
    Verbose = 3,
}

/// 链接器策略选项（`--linker`）
pub enum PkgCliLinker: u8 {
    Auto = 0,
    Hoisted = 1,
    Isolated = 2,

    static fn parse(raw: []const u8): PkgCliLinker? {
        if (bytesEql(raw, "hoisted")) return .Hoisted;
        if (bytesEql(raw, "isolated")) return .Isolated;
        return null;
    }
}

/// 物化安装后端策略（`--backend`）
pub enum PkgCliBackend: u8 {
    Auto = 0,
    Clonefile = 1,
    Hardlink = 2,
    Symlink = 3,
    Copyfile = 4,

    static fn parse(raw: []const u8): PkgCliBackend? {
        if (bytesEql(raw, "clonefile")) return .Clonefile;
        if (bytesEql(raw, "hardlink")) return .Hardlink;
        if (bytesEql(raw, "symlink")) return .Symlink;
        if (bytesEql(raw, "copyfile")) return .Copyfile;
        return null;
    }
}

/// 新增依赖的目标分组（`-d`/`--dev`, `--optional`, `--peer`）
pub enum PkgCliDepTarget: u8 {
    Prod = 0,
    Dev = 1,
    Optional = 2,
    Peer = 3,
}

/// 零堆分配包管理器 CLI 解析结果快照
pub struct PkgCliParsedArgs {
    subcommand: PkgCliSubcommand = .Install,

    // 共享字符串选项
    config_path: ([]const u8)? = null,
    cache_dir: ([]const u8)? = null,
    cwd: ([]const u8)? = null,
    registry: ([]const u8)? = null,
    cafile: ([]const u8)? = null,

    // 共享布尔开关
    yarn_lockfile: bool = false,
    production: bool = false,
    save: bool = true,
    dry_run: bool = false,
    frozen_lockfile: bool = false,
    force: bool = false,
    no_cache: bool = false,
    no_progress: bool = false,
    no_summary: bool = false,
    no_verify: bool = false,
    ignore_scripts: bool = false,
    trust: bool = false,
    global_install: bool = false,
    save_text_lockfile: bool = false,
    lockfile_only: bool = false,
    help: bool = false,

    // 枚举与数值选项
    log_level: PkgCliLogLevel = .Default,
    linker: PkgCliLinker = .Auto,
    backend: PkgCliBackend = .Auto,
    omit_dev: bool = false,
    omit_optional: bool = false,
    omit_peer: bool = false,
    concurrent_scripts: u32? = null,
    network_concurrency: u32 = PKG_CLI_DEFAULT_NETWORK_CONCURRENCY,
    minimum_release_age: u64? = null,

    // 子命令专属选项
    dep_target: PkgCliDepTarget = .Prod,
    dep_target_explicit: bool = false,
    exact: bool = false,
    analyze: bool = false,
    only_missing: bool = false,
    latest: bool = false,
    interactive: bool = false,
    recursive: bool = false,
    json_output: bool = false,
    top_only: bool = false,
    depth: u32? = null,
    all_flag: bool = false,

    // 多值列表与位置参数
    filters: [PKG_CLI_MAX_FILTERS][]const u8 = [""; PKG_CLI_MAX_FILTERS],
    filter_count: usize = 0,
    cpu_filters: [PKG_CLI_MAX_PLATFORMS][]const u8 = [""; PKG_CLI_MAX_PLATFORMS],
    cpu_count: usize = 0,
    os_filters: [PKG_CLI_MAX_PLATFORMS][]const u8 = [""; PKG_CLI_MAX_PLATFORMS],
    os_count: usize = 0,
    positionals: [PKG_CLI_MAX_POSITIONALS][]const u8 = [""; PKG_CLI_MAX_POSITIONALS],
    positional_count: usize = 0,

    /// 返回位置参数只读切片
    fn positionalSlice(): []const ([]const u8) {
        return _.positionals[0.._.positional_count];
    }

    /// 返回 `--filter` 模式只读切片
    fn filterSlice(): []const ([]const u8) {
        return _.filters[0.._.filter_count];
    }

    /// 返回 `--cpu` 覆盖项只读切片
    fn cpuSlice(): []const ([]const u8) {
        return _.cpu_filters[0.._.cpu_count];
    }

    /// 返回 `--os` 覆盖项只读切片
    fn osSlice(): []const ([]const u8) {
        return _.os_filters[0.._.os_count];
    }

    /// 判断当前配置是否需要写入磁盘（未设置 `--dry-run` 且 `save == true`）
    fn shouldPersistChanges(): bool {
        return !_.dry_run && _.save;
    }

    /// 判断当前配置是否允许修改 lockfile（未设置 `--frozen-lockfile` 且未设置 `--dry-run`）
    fn canModifyLockfile(): bool {
        return !_.frozen_lockfile && !_.dry_run;
    }
}

fn bytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn findByte(s: []const u8, target: u8): usize? {
    for (i of 0..s.len) {
        if (s[i] == target) return i;
    }
    return null;
}

fn parseUint64(s: []const u8): u64! {
    if (s.len == 0) return err.InvalidFlagValue;
    let acc: u64 = 0;
    for (i of 0..s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return err.InvalidFlagValue;
        const digit = (c - '0') as u64;
        if (acc > (18446744073709551615 - digit) / 10) {
            return err.InvalidFlagValue;
        }
        acc = acc * 10 + digit;
    }
    return acc;
}

fn parsePositiveU32(s: []const u8): u32! {
    const v = parseUint64(s)?;
    if (v == 0 || v > 4294967295) {
        return err.InvalidFlagValue;
    }
    return v as u32;
}

fn parseNonNegU32(s: []const u8): u32! {
    const v = parseUint64(s)?;
    if (v > 4294967295) {
        return err.InvalidFlagValue;
    }
    return v as u32;
}

fn setDepTarget(out: *PkgCliParsedArgs, target: PkgCliDepTarget): bool! {
    if (out.subcommand != .Install && out.subcommand != .Add) {
        return err.FlagNotAllowedForSubcommand;
    }
    if (out.dep_target_explicit && out.dep_target != target) {
        return err.ConflictingFlags;
    }
    out.dep_target = target;
    out.dep_target_explicit = true;
    return true;
}

fn setLogLevel(out: *PkgCliParsedArgs, level: PkgCliLogLevel): bool! {
    if (out.log_level != .Default && out.log_level != level) {
        return err.ConflictingFlags;
    }
    out.log_level = level;
    return true;
}

fn applyOmitValue(out: *PkgCliParsedArgs, raw: []const u8): bool! {
    if (bytesEql(raw, "dev")) {
        out.omit_dev = true;
        return true;
    }
    if (bytesEql(raw, "optional")) {
        out.omit_optional = true;
        return true;
    }
    if (bytesEql(raw, "peer")) {
        out.omit_peer = true;
        return true;
    }
    return err.InvalidFlagValue;
}

fn pushPositional(out: *PkgCliParsedArgs, arg: []const u8): bool! {
    if (out.positional_count >= PKG_CLI_MAX_POSITIONALS) {
        return err.TooManyPositionals;
    }
    out.positionals[out.positional_count] = arg;
    out.positional_count += 1;
    return true;
}

fn pushFilter(out: *PkgCliParsedArgs, pat: []const u8): bool! {
    if (out.subcommand != .Install && out.subcommand != .Update) {
        return err.FlagNotAllowedForSubcommand;
    }
    if (pat.len == 0) {
        return err.InvalidFlagValue;
    }
    if (out.filter_count >= PKG_CLI_MAX_FILTERS) {
        return err.TooManyFilters;
    }
    out.filters[out.filter_count] = pat;
    out.filter_count += 1;
    return true;
}

fn pushCpu(out: *PkgCliParsedArgs, val: []const u8): bool! {
    if (val.len == 0) return err.InvalidFlagValue;
    if (out.cpu_count >= PKG_CLI_MAX_PLATFORMS) return err.TooManyPlatforms;
    out.cpu_filters[out.cpu_count] = val;
    out.cpu_count += 1;
    return true;
}

fn pushOs(out: *PkgCliParsedArgs, val: []const u8): bool! {
    if (val.len == 0) return err.InvalidFlagValue;
    if (out.os_count >= PKG_CLI_MAX_PLATFORMS) return err.TooManyPlatforms;
    out.os_filters[out.os_count] = val;
    out.os_count += 1;
    return true;
}

fn consumeValue(
    inline_val: ([]const u8)?,
    argv: []const ([]const u8),
    idx: *usize,
): ([]const u8)! {
    if (v := inline_val) {
        return v;
    }
    if (idx.* + 1 >= argv.len) {
        return err.MissingFlagValue;
    }
    idx.* += 1;
    return argv[idx.*];
}

/// 解析包管理器子命令的 `argv` 参数列表（不包含可执行文件名与子命令名本身）
pub fn parsePkgCliArgs(
    subcommand: PkgCliSubcommand,
    argv: []const ([]const u8),
): PkgCliParsedArgs! {
    let out = PkgCliParsedArgs{ subcommand: subcommand };
    let idx: usize = 0;
    let only_positionals = false;

    while (idx < argv.len) {
        const arg = argv[idx];

        if (only_positionals || arg.len == 0 || arg[0] != '-' || bytesEql(arg, "-")) {
            * = pushPositional(&out, arg)?;
            idx += 1;
            continue;
        }

        if (bytesEql(arg, "--")) {
            only_positionals = true;
            idx += 1;
            continue;
        }

        // 长选项 `--name` 或 `--name=value`
        if (arg.len >= 2 && arg[0] == '-' && arg[1] == '-') {
            const body = arg[2..];
            let flag_name = body;
            let inline_val: ([]const u8)? = null;
            if (eq_pos := findByte(body, '=')) {
                flag_name = body[0..eq_pos];
                inline_val = body[eq_pos + 1..];
            }

            if (bytesEql(flag_name, "config")) {
                out.config_path = consumeValue(inline_val, argv, &idx)?;
            } else if (bytesEql(flag_name, "yarn")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.yarn_lockfile = true;
            } else if (bytesEql(flag_name, "production") || bytesEql(flag_name, "prod")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.production = true;
                out.omit_dev = true;
            } else if (bytesEql(flag_name, "no-save")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.save = false;
            } else if (bytesEql(flag_name, "save")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.save = true;
            } else if (bytesEql(flag_name, "cafile")) {
                out.cafile = consumeValue(inline_val, argv, &idx)?;
            } else if (bytesEql(flag_name, "dry-run")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.dry_run = true;
            } else if (bytesEql(flag_name, "frozen-lockfile")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.frozen_lockfile = true;
            } else if (bytesEql(flag_name, "force")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.force = true;
            } else if (bytesEql(flag_name, "cache-dir")) {
                out.cache_dir = consumeValue(inline_val, argv, &idx)?;
            } else if (bytesEql(flag_name, "no-cache")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.no_cache = true;
            } else if (bytesEql(flag_name, "silent")) {
                if (inline_val != null) return err.InvalidFlagValue;
                * = setLogLevel(&out, .Silent)?;
            } else if (bytesEql(flag_name, "quiet")) {
                if (inline_val != null) return err.InvalidFlagValue;
                * = setLogLevel(&out, .Quiet)?;
            } else if (bytesEql(flag_name, "verbose")) {
                if (inline_val != null) return err.InvalidFlagValue;
                * = setLogLevel(&out, .Verbose)?;
            } else if (bytesEql(flag_name, "no-progress")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.no_progress = true;
            } else if (bytesEql(flag_name, "no-summary")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.no_summary = true;
            } else if (bytesEql(flag_name, "no-verify")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.no_verify = true;
            } else if (bytesEql(flag_name, "ignore-scripts")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.ignore_scripts = true;
            } else if (bytesEql(flag_name, "trust")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.trust = true;
            } else if (bytesEql(flag_name, "global")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.global_install = true;
            } else if (bytesEql(flag_name, "cwd")) {
                out.cwd = consumeValue(inline_val, argv, &idx)?;
            } else if (bytesEql(flag_name, "backend")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                out.backend = PkgCliBackend.parse(v) ?? return err.InvalidFlagValue;
            } else if (bytesEql(flag_name, "registry")) {
                out.registry = consumeValue(inline_val, argv, &idx)?;
            } else if (bytesEql(flag_name, "concurrent-scripts")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                out.concurrent_scripts = parsePositiveU32(v)?;
            } else if (bytesEql(flag_name, "network-concurrency")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                out.network_concurrency = parsePositiveU32(v)?;
            } else if (bytesEql(flag_name, "save-text-lockfile")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.save_text_lockfile = true;
            } else if (bytesEql(flag_name, "omit")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                * = applyOmitValue(&out, v)?;
            } else if (bytesEql(flag_name, "lockfile-only")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.lockfile_only = true;
            } else if (bytesEql(flag_name, "linker")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                out.linker = PkgCliLinker.parse(v) ?? return err.InvalidFlagValue;
            } else if (bytesEql(flag_name, "minimum-release-age")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                out.minimum_release_age = parseUint64(v)?;
            } else if (bytesEql(flag_name, "cpu")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                * = pushCpu(&out, v)?;
            } else if (bytesEql(flag_name, "os")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                * = pushOs(&out, v)?;
            } else if (bytesEql(flag_name, "help")) {
                if (inline_val != null) return err.InvalidFlagValue;
                out.help = true;
            } else if (bytesEql(flag_name, "dev") || bytesEql(flag_name, "development")) {
                if (inline_val != null) return err.InvalidFlagValue;
                * = setDepTarget(&out, .Dev)?;
            } else if (bytesEql(flag_name, "optional")) {
                if (inline_val != null) return err.InvalidFlagValue;
                * = setDepTarget(&out, .Optional)?;
            } else if (bytesEql(flag_name, "peer")) {
                if (inline_val != null) return err.InvalidFlagValue;
                * = setDepTarget(&out, .Peer)?;
            } else if (bytesEql(flag_name, "exact")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Install && subcommand != .Add) {
                    return err.FlagNotAllowedForSubcommand;
                }
                out.exact = true;
            } else if (bytesEql(flag_name, "analyze")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Install && subcommand != .Add) {
                    return err.FlagNotAllowedForSubcommand;
                }
                out.analyze = true;
            } else if (bytesEql(flag_name, "only-missing")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Install && subcommand != .Add) {
                    return err.FlagNotAllowedForSubcommand;
                }
                out.only_missing = true;
            } else if (bytesEql(flag_name, "filter")) {
                const v = consumeValue(inline_val, argv, &idx)?;
                * = pushFilter(&out, v)?;
            } else if (bytesEql(flag_name, "latest")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Update) return err.FlagNotAllowedForSubcommand;
                out.latest = true;
            } else if (bytesEql(flag_name, "interactive")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Update) return err.FlagNotAllowedForSubcommand;
                out.interactive = true;
            } else if (bytesEql(flag_name, "recursive")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Update) return err.FlagNotAllowedForSubcommand;
                out.recursive = true;
            } else if (bytesEql(flag_name, "json")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Pm && subcommand != .Audit) return err.FlagNotAllowedForSubcommand;
                out.json_output = true;
            } else if (bytesEql(flag_name, "top")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Pm) return err.FlagNotAllowedForSubcommand;
                out.top_only = true;
            } else if (bytesEql(flag_name, "depth")) {
                if (subcommand != .Pm) return err.FlagNotAllowedForSubcommand;
                const v = consumeValue(inline_val, argv, &idx)?;
                out.depth = parseNonNegU32(v)?;
            } else if (bytesEql(flag_name, "all")) {
                if (inline_val != null) return err.InvalidFlagValue;
                if (subcommand != .Pm) return err.FlagNotAllowedForSubcommand;
                out.all_flag = true;
            } else {
                return err.UnknownFlag;
            }

            idx += 1;
            continue;
        }

        // 短选项簇（如 `-dE`, `-pf`, `-c bunfig.toml`）
        let ci: usize = 1;
        while (ci < arg.len) {
            const c = arg[ci];
            if (c == 'c') {
                if (ci + 1 < arg.len) {
                    out.config_path = arg[ci + 1..];
                    break;
                }
                out.config_path = consumeValue(null, argv, &idx)?;
                break;
            } else if (c == 'y') {
                out.yarn_lockfile = true;
            } else if (c == 'p' || c == 'P') {
                out.production = true;
                out.omit_dev = true;
            } else if (c == 'f') {
                out.force = true;
            } else if (c == 'g') {
                out.global_install = true;
            } else if (c == 'h') {
                out.help = true;
            } else if (c == 'd' || c == 'D') {
                * = setDepTarget(&out, .Dev)?;
            } else if (c == 'E') {
                if (subcommand != .Install && subcommand != .Add) {
                    return err.FlagNotAllowedForSubcommand;
                }
                out.exact = true;
            } else if (c == 'a') {
                if (subcommand == .Install || subcommand == .Add) {
                    out.analyze = true;
                } else if (subcommand == .Pm) {
                    out.all_flag = true;
                } else {
                    return err.FlagNotAllowedForSubcommand;
                }
            } else if (c == 'i') {
                if (subcommand != .Update) return err.FlagNotAllowedForSubcommand;
                out.interactive = true;
            } else if (c == 'r') {
                if (subcommand != .Update) return err.FlagNotAllowedForSubcommand;
                out.recursive = true;
            } else {
                return err.UnknownFlag;
            }
            ci += 1;
        }
        idx += 1;
    }

    return out;
}
