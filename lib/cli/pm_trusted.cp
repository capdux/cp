// Source: Absorbed from /data/dev/bun/src/cli/pm_trusted_command.cp | Synced: 2026-11-21T20:23:25Z
// Ported from Bun: /data/dev/bun/src/cli/pm_trusted_command.cp & /data/dev/bun/docs/pm/lifecycle.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation Package Lifecycle Script Trust & Supply-Chain Policy Engine:
// - Enforces Bun's default-secure `trustedDependencies` allowlist over `preinstall`/`install`/`postinstall`/`prepare` scripts
// - Prevents non-registry spoofing: default trusted list applies ONLY to npm registry packages, never `file:`/`link:`/`git:`/`github:`
// - Supports global `--ignore-scripts` override, version-pinned specs (`pkg@1.2.3` / `@scope/pkg@1.0.0`), and lexicographic sorting
// - Formats deterministic `trustedDependencies` JSON arrays and `pm untrusted` audit reports with zero heap allocation.

/// 显式/默认信任白名单最大条目数
pub const PM_TRUST_MAX_ALLOWLIST: usize = 64;

/// 生命周期脚本信任策略错误集
pub err PmTrustError {
    TooManyTrustedEntries,
    EmptyPackageName,
    BufferTooSmall,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn cmpBytes(a: []const u8, b: []const u8): i8 {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] < b[i]) { return -1; }
        if (a[i] > b[i]) { return 1; }
        i += 1;
    }
    if (a.len < b.len) { return -1; }
    if (a.len > b.len) { return 1; }
    return 0;
}

fn appendSlice(buf: []u8, pos: *usize, s: []const u8): PmTrustError!bool {
    if (pos.* + s.len > buf.len) {
        return PmTrustError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < s.len) {
        buf[pos.* + i] = s[i];
        i += 1;
    }
    pos.* += s.len;
    return true;
}

fn appendByte(buf: []u8, pos: *usize, b: u8): PmTrustError!bool {
    if (pos.* + 1 > buf.len) {
        return PmTrustError.BufferTooSmall;
    }
    buf[pos.*] = b;
    pos.* += 1;
    return true;
}

fn appendU32(buf: []u8, pos: *usize, val: u32): PmTrustError!bool {
    let tmp: [12]u8 = [0; 12];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (v > 0 && n < 12) {
            tmp[n] = @as(u8, @intCast(v % 10)) + '0';
            v = v / 10;
            n += 1;
        }
    }
    if (pos.* + n > buf.len) { return PmTrustError.BufferTooSmall; }
    let i = n;
    while (i > 0) {
        i -= 1;
        buf[pos.*] = tmp[i];
        pos.* += 1;
    }
    return true;
}

/// 依赖来源类别（决定是否允许命中默认信任白名单）
pub enum PmDepSourceKind: u8 {
    NpmRegistry = 0,
    FilePath = 1,
    Link = 2,
    Git = 3,
    Github = 4,
    Tarball = 5,

    /// 对齐 Bun `docs/pm/lifecycle.mdx` 安全红线：仅 npm 官方源可应用默认信任名单，防止本地/Git 伪造同名包绕过
    fn eligibleForDefaultTrust(): bool {
        return _ == .NpmRegistry;
    }
}

/// 包安装生命周期脚本阶段
pub enum PmLifecyclePhase: u8 {
    Preinstall = 0,
    Install = 1,
    Postinstall = 2,
    Prepare = 3,
    Preprepare = 4,
    Postprepare = 5,

    static fn parse(raw: []const u8): PmLifecyclePhase? {
        if (eqlBytes(raw, "preinstall")) { return .Preinstall; }
        if (eqlBytes(raw, "install")) { return .Install; }
        if (eqlBytes(raw, "postinstall")) { return .Postinstall; }
        if (eqlBytes(raw, "prepare")) { return .Prepare; }
        if (eqlBytes(raw, "preprepare")) { return .Preprepare; }
        if (eqlBytes(raw, "postprepare")) { return .Postprepare; }
        return null;
    }

    fn asStr(): []const u8 {
        return switch (_) {
            .Preinstall => "preinstall",
            .Install => "install",
            .Postinstall => "postinstall",
            .Prepare => "prepare",
            .Preprepare => "preprepare",
            .Postprepare => "postprepare",
        };
    }

    fn bitMask(): u8 {
        return switch (_) {
            .Preinstall => 1 << 0,
            .Install => 1 << 1,
            .Postinstall => 1 << 2,
            .Prepare => 1 << 3,
            .Preprepare => 1 << 4,
            .Postprepare => 1 << 5,
        };
    }
}

/// 生命周期脚本信任判定结论
pub enum PmTrustDecision: u8 {
    AllowedExplicit = 0,
    AllowedDefault = 1,
    BlockedUntrusted = 2,
    BlockedIgnoreScripts = 3,
    BlockedNonRegistrySpoof = 4,
    NoLifecycleScripts = 5,

    fn isAllowed(): bool {
        return _ == .AllowedExplicit || _ == .AllowedDefault;
    }

    fn isBlocked(): bool {
        return _ == .BlockedUntrusted || _ == .BlockedIgnoreScripts || _ == .BlockedNonRegistrySpoof;
    }
}

/// `trustedDependencies` 单条规则（支持 `"esbuild"`、`"@swc/core"`、`"esbuild@0.24.0"`、`"@swc/core@1.7.0"`）
pub struct PmTrustSpec {
    raw: []const u8 = "",
    name: []const u8 = "",
    version: []const u8 = "",

    static fn parse(raw: []const u8): PmTrustError!PmTrustSpec {
        if (raw.len == 0) { return PmTrustError.EmptyPackageName; }
        // 作用域包 `@scope/pkg@1.0.0` 从下标 1 开始查找 `@`
        const search_start: usize = if (raw[0] == '@') 1 else 0;
        let at_pos: usize = raw.len;
        let i = search_start;
        while (i < raw.len) {
            if (raw[i] == '@') {
                at_pos = i;
                break;
            }
            i += 1;
        }
        const pkg_name = raw[0..at_pos];
        if (pkg_name.len == 0) { return PmTrustError.EmptyPackageName; }
        const ver = if (at_pos + 1 < raw.len) raw[(at_pos + 1)..raw.len] else "";
        return PmTrustSpec{
            raw: raw,
            name: pkg_name,
            version: ver,
        };
    }

    fn matches(pkg_name: []const u8, pkg_version: []const u8): bool {
        if (!eqlBytes(_.name, pkg_name)) { return false; }
        if (_.version.len == 0) { return true; }
        return eqlBytes(_.version, pkg_version);
    }
}

/// 待评估生命周期脚本的依赖包元数据
pub struct PmLifecyclePackage {
    name: []const u8 = "",
    version: []const u8 = "",
    source: PmDepSourceKind = .NpmRegistry,
    script_mask: u8 = 0,

    fn hasScripts(): bool {
        return _.script_mask != 0;
    }

    fn addScript(*_, phase: PmLifecyclePhase) {
        _.script_mask = _.script_mask | phase.bitMask();
    }

    fn hasScript(phase: PmLifecyclePhase): bool {
        return (_.script_mask & phase.bitMask()) != 0;
    }
}

/// 零堆分配生命周期脚本信任策略引擎
pub struct PmTrustPolicy {
    ignore_scripts: bool = false,
    use_default_trust_list: bool = true,
    explicit_count: usize = 0,
    explicit_specs: [PM_TRUST_MAX_ALLOWLIST]PmTrustSpec = [PmTrustSpec{}; PM_TRUST_MAX_ALLOWLIST],
    default_count: usize = 0,
    default_names: [PM_TRUST_MAX_ALLOWLIST]([]const u8) = [""; PM_TRUST_MAX_ALLOWLIST],

    static fn init(ignore_scripts: bool): PmTrustPolicy {
        return PmTrustPolicy{
            ignore_scripts: ignore_scripts,
        };
    }

    /// 初始化并预装常见原生构建包默认信任种子（如 esbuild / sharp / @swc/core / fsevents / sqlite3 等）
    static fn initWithBuiltins(ignore_scripts: bool): PmTrustPolicy {
        let pol = PmTrustPolicy{ ignore_scripts: ignore_scripts };
        const builtins: [10]([]const u8) = [
            "@swc/core",
            "bcrypt",
            "canvas",
            "cypress",
            "esbuild",
            "fsevents",
            "playwright",
            "puppeteer",
            "sharp",
            "sqlite3",
        ];
        let i: usize = 0;
        while (i < builtins.len) {
            pol.default_names[i] = builtins[i];
            i += 1;
        }
        pol.default_count = builtins.len;
        return pol;
    }

    /// 向显式 `trustedDependencies` 追加规则（去重）
    fn addExplicitTrust(*_, raw_spec: []const u8): PmTrustError!bool {
        const spec = PmTrustSpec.parse(raw_spec)?;
        let i: usize = 0;
        while (i < _.explicit_count) {
            if (eqlBytes(_.explicit_specs[i].raw, spec.raw)) {
                return false;
            }
            i += 1;
        }
        if (_.explicit_count >= PM_TRUST_MAX_ALLOWLIST) {
            return PmTrustError.TooManyTrustedEntries;
        }
        _.explicit_specs[_.explicit_count] = spec;
        _.explicit_count += 1;
        return true;
    }

    /// 向默认信任白名单追加包名（去重）
    fn addDefaultTrust(*_, pkg_name: []const u8): PmTrustError!bool {
        if (pkg_name.len == 0) { return PmTrustError.EmptyPackageName; }
        let i: usize = 0;
        while (i < _.default_count) {
            if (eqlBytes(_.default_names[i], pkg_name)) {
                return false;
            }
            i += 1;
        }
        if (_.default_count >= PM_TRUST_MAX_ALLOWLIST) {
            return PmTrustError.TooManyTrustedEntries;
        }
        _.default_names[_.default_count] = pkg_name;
        _.default_count += 1;
        return true;
    }

    /// 评估单个依赖包的生命周期脚本是否允许执行
    fn evaluate(pkg: PmLifecyclePackage): PmTrustDecision {
        if (!pkg.hasScripts()) {
            return .NoLifecycleScripts;
        }
        if (_.ignore_scripts) {
            return .BlockedIgnoreScripts;
        }

        // 1. 显式 trustedDependencies 优先级最高（可放行任意来源包括 file:/git:）
        let ei: usize = 0;
        while (ei < _.explicit_count) {
            if (_.explicit_specs[ei].matches(pkg.name, pkg.version)) {
                return .AllowedExplicit;
            }
            ei += 1;
        }

        // 2. 默认信任名单仅对 npm Registry 来源生效，非 Registry 源命中时标记为防伪造阻断
        if (_.use_default_trust_list) {
            let di: usize = 0;
            while (di < _.default_count) {
                if (eqlBytes(_.default_names[di], pkg.name)) {
                    if (pkg.source.eligibleForDefaultTrust()) {
                        return .AllowedDefault;
                    }
                    return .BlockedNonRegistrySpoof;
                }
                di += 1;
            }
        }

        return .BlockedUntrusted;
    }

    /// 对齐 Bun `TrustSorter`：按字典序原地排序显式 `trustedDependencies`
    fn sortExplicitAllowlist(*_) {
        let i: usize = 1;
        while (i < _.explicit_count) {
            const cur = _.explicit_specs[i];
            let j = i;
            while (j > 0 && cmpBytes(cur.raw, _.explicit_specs[j - 1].raw) < 0) {
                _.explicit_specs[j] = _.explicit_specs[j - 1];
                j -= 1;
            }
            _.explicit_specs[j] = cur;
            i += 1;
        }
    }

    /// 序列化显式 `trustedDependencies` 为 JSON 字符串数组（供写回 `package.json`）
    fn formatTrustedJsonArray(buf: []u8): PmTrustError!usize {
        let pos: usize = 0;
        * = appendByte(buf, &pos, '[')?;
        let i: usize = 0;
        while (i < _.explicit_count) {
            if (i > 0) {
                * = appendByte(buf, &pos, ',')?;
            }
            * = appendByte(buf, &pos, '"')?;
            * = appendSlice(buf, &pos, _.explicit_specs[i].raw)?;
            * = appendByte(buf, &pos, '"')?;
            i += 1;
        }
        * = appendByte(buf, &pos, ']')?;
        return pos;
    }

    /// 生成 `pm untrusted` 风格的人类可读安全审阅报告
    fn formatUntrustedReport(pkgs: []const PmLifecyclePackage, buf: []u8): PmTrustError!usize {
        let pos: usize = 0;
        let blocked_count: u32 = 0;

        let i: usize = 0;
        while (i < pkgs.len) {
            const p = pkgs[i];
            const decision = _.evaluate(p);
            if (decision.isBlocked()) {
                blocked_count += 1;
                * = appendSlice(buf, &pos, p.name)?;
                if (p.version.len > 0) {
                    * = appendByte(buf, &pos, '@')?;
                    * = appendSlice(buf, &pos, p.version)?;
                }
                * = appendSlice(buf, &pos, " [")?;
                let first_s = true;
                const phases: [6]PmLifecyclePhase = [
                    .Preinstall,
                    .Install,
                    .Postinstall,
                    .Prepare,
                    .Preprepare,
                    .Postprepare,
                ];
                let pi: usize = 0;
                while (pi < phases.len) {
                    if (p.hasScript(phases[pi])) {
                        if (!first_s) { * = appendSlice(buf, &pos, ", ")?; }
                        * = appendSlice(buf, &pos, phases[pi].asStr())?;
                        first_s = false;
                    }
                    pi += 1;
                }
                * = appendSlice(buf, &pos, "]\n")?;
            }
            i += 1;
        }

        if (blocked_count == 0) {
            * = appendSlice(buf, &pos, "Found 0 untrusted dependencies with scripts.\n")?;
            return pos;
        }

        * = appendSlice(buf, &pos, "\nFound ")?;
        * = appendU32(buf, &pos, blocked_count)?;
        * = appendSlice(buf, &pos, " untrusted ")?;
        * = appendSlice(buf, &pos, if (blocked_count == 1) "dependency" else "dependencies")?;
        * = appendSlice(buf, &pos, " with scripts. Run `cap pm trust <names>` to allow.\n")?;
        return pos;
    }
}
