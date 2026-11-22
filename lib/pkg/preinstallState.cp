// Source: Absorbed from /data/dev/bun/src/install/PackageManager/PackageManagerLifecycle.cp | Synced: 2026-11-22T14:08:00Z
// Pure-CP zero-allocation Package Manager Preinstall State Machine,
// Lifecycle Script Time Log, Slow Postinstall Monitor & Trusted Dependency Gate.
// Aligned with Bun `src/install/PackageManager/PackageManagerLifecycle.cp`
// and `docs/guides/install/trusted.mdx`:
// - `PreinstallStateTable`: tracks per-package preinstall state transitions
//   (`Unknown` -> `Done` | `CalcPatchHash` | `ApplyPatch` | `Extract`) with platform-disabled
//   fast-skip, cache-hit short-circuit, and patched-folder non-patched base extraction.
// - `LifecycleTimeLog`: records completed lifecycle script durations, finds the slowest
//   script, and formats Bun's one-decimal duration warning message.
// - `SlowLifecycleMonitor`: throttles slow running postinstall alerts (5s verbose / 30s normal,
//   per-tick deduplicated) using Bun's possessive grammar (`pkg's` vs `pkgs'`).
// - `isLifecycleScriptTrusted`: enforces Bun's trustedDependencies rules (default allowlist
//   applies only to npm packages; non-npm sources require explicit trust; trust is non-transitive).

/// 预安装状态表最大包容量
pub const PREINSTALL_MAX_PKGS: usize = 128;
/// 生命周期脚本耗时日志最大记录数
pub const LIFECYCLE_LOG_MAX_ENTRIES: usize = 64;
/// 慢脚本监控默认告警间隔（30 秒，单位纳秒）
pub const SLOW_SCRIPT_DEFAULT_INTERVAL_NS: u64 = 30_000_000_000;
/// 慢脚本监控详细模式告警间隔（5 秒，单位纳秒）
pub const SLOW_SCRIPT_VERBOSE_INTERVAL_NS: u64 = 5_000_000_000;

/// 预安装状态机与脚本监控错误集
pub err PreinstallStateError {
    PackageIdOutOfBounds,
    TimeLogFull,
    EmptyPackageName,
    MissingPatchHashMarker,
    BufferTooSmall,
}

/// 单个包的预安装阶段状态（对齐 Bun `PreinstallState`）
pub enum PreinstallStateKind: u8 {
    Unknown = 0,
    CalcPatchHash = 1,
    Extract = 2,
    ApplyPatch = 3,
    Done = 4,
}

/// 生命周期脚本类型（对齐 Bun `Lockfile.Scripts.names`）
pub enum LifecycleScriptSlot: u8 {
    Preinstall = 0,
    Install = 1,
    Postinstall = 2,
    Preprepare = 3,
    Prepare = 4,
    Postprepare = 5,

    /// 返回标准 npm 生命周期脚本名称
    fn asStr(): []const u8 {
        return switch (_) {
            .Preinstall => "preinstall",
            .Install => "install",
            .Postinstall => "postinstall",
            .Preprepare => "preprepare",
            .Prepare => "prepare",
            .Postprepare => "postprepare",
        };
    }
}

/// 慢脚本监控日志级别
pub enum MonitorLogLevel: u8 {
    Silent = 0,
    Default = 1,
    Verbose = 2,
}

/// 包来源类别（用于 `trustedDependencies` 信任裁决）
pub enum TrustSourceKind: u8 {
    Npm = 0,
    File = 1,
    Link = 2,
    Git = 3,
    Github = 4,
    Tarball = 5,
}

/// 预安装状态判定输入上下文（零堆分配）
pub struct PreinstallProbe {
    pkg_id: u32 = 0,
    /// 当前包的 cpu/os 约束是否与宿主平台不匹配（不匹配则直接标记 `.Done` 跳过下载）
    is_disabled_for_platform: bool = false,
    /// 是否命中 `patchedDependencies`
    is_patched: bool = false,
    /// 命中补丁但尚未计算补丁文件哈希
    patchfile_hash_missing: bool = false,
    /// 缓存目录名（为空时必须进入 `.Extract`）
    cached_folder_path: []const u8 = "",
    /// 完整缓存目录（含 `_patch_hash=` 后缀，若已打补丁）是否已存在于全局缓存
    is_folder_in_cache: bool = false,
    /// 若已打补丁，剥离 `_patch_hash=` 后的原始未打补丁目录是否已存在于全局缓存
    is_base_folder_in_cache: bool = false,
}

/// 单条已完成生命周期脚本的耗时记录
pub struct LifecycleTimeEntry {
    package_name: []const u8 = "",
    script: LifecycleScriptSlot = .Postinstall,
    duration_ns: u64 = 0,
}

/// 正在运行的慢脚本快照
pub struct ActiveLifecycleSnapshot {
    package_name: []const u8 = "",
    started_at_ns: u64 = 0,
}

fn copyInto(dst: []u8, offset: usize, src: []const u8): usize! {
    if (offset + src.len > dst.len) return err.BufferTooSmall;
    for (i of 0..src.len) {
        dst[offset + i] = src[i];
    }
    return offset + src.len;
}

fn copyByte(dst: []u8, offset: usize, b: u8): usize! {
    if (offset + 1 > dst.len) return err.BufferTooSmall;
    dst[offset] = b;
    return offset + 1;
}

fn copyU64(dst: []u8, offset: usize, val: u64): usize! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        return copyByte(dst, offset, '0');
    }
    while (v > 0) {
        tmp[n] = ((v % 10) + 48) as u8;
        v = v / 10;
        n += 1;
    }
    let pos = offset;
    while (n > 0) {
        n -= 1;
        pos = copyByte(dst, pos, tmp[n])?;
    }
    return pos;
}

fn findSubstr(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (needle.len > haystack.len) return null;
    const limit = haystack.len - needle.len + 1;
    for (i of 0..limit) {
        let matched = true;
        for (j of 0..needle.len) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
        }
        if (matched) return i;
    }
    return null;
}

/// 从带补丁后缀的缓存目录名（如 `"is-even@1.0.0_patch_hash=abc123"`）剥离 `_patch_hash=` 后缀，
/// 还原原始未打补丁的基础缓存目录名（对齐 Bun `determinePreinstallState`）。
pub fn stripPatchHashFolderSuffix(folder_path: []const u8): ([]const u8)! {
    const marker = "_patch_hash=";
    if (idx := findSubstr(folder_path, marker)) {
        return folder_path[0..idx];
    }
    return err.MissingPatchHashMarker;
}

/// 按 Bun 规则格式化一位小数耗时（`ns` / `us` / `ms` / `s`）
pub fn formatDurationOneDecimal(duration_ns: u64, out: []u8): usize! {
    let pos: usize = 0;
    if (duration_ns < 1_000) {
        pos = copyU64(out, pos, duration_ns)?;
        return copyInto(out, pos, "ns");
    }
    if (duration_ns < 1_000_000) {
        const whole = duration_ns / 1_000;
        const frac = (duration_ns % 1_000) / 100;
        pos = copyU64(out, pos, whole)?;
        pos = copyByte(out, pos, '.')?;
        pos = copyU64(out, pos, frac)?;
        return copyInto(out, pos, "us");
    }
    if (duration_ns < 1_000_000_000) {
        const whole = duration_ns / 1_000_000;
        const frac = (duration_ns % 1_000_000) / 100_000;
        pos = copyU64(out, pos, whole)?;
        pos = copyByte(out, pos, '.')?;
        pos = copyU64(out, pos, frac)?;
        return copyInto(out, pos, "ms");
    }
    const whole = duration_ns / 1_000_000_000;
    const frac = (duration_ns % 1_000_000_000) / 100_000_000;
    pos = copyU64(out, pos, whole)?;
    pos = copyByte(out, pos, '.')?;
    pos = copyU64(out, pos, frac)?;
    return copyByte(out, pos, 's');
}

/// 按 Bun `trustedDependencies` 安全规则裁决是否允许执行生命周期脚本（对齐 `/data/dev/bun/docs/guides/install/trusted.mdx`）
/// - 信任关系不传递（`is_direct_listed == false` 且仅父包被信任时仍禁止）；
/// - 默认内置白名单仅对 `.Npm` 来源生效；`file:`/`link:`/`git:`/`github:`/`tarball` 必须显式列入 `trustedDependencies`。
pub fn isLifecycleScriptTrusted(
    source: TrustSourceKind,
    explicitly_in_trusted_deps: bool,
    in_default_npm_allowlist: bool,
): bool {
    if (explicitly_in_trusted_deps) {
        return true;
    }
    if (source == .Npm && in_default_npm_allowlist) {
        return true;
    }
    return false;
}

/// 零堆分配包预安装状态表与状态转移机（对齐 Bun `determinePreinstallState`）
pub struct PreinstallStateTable {
    states: [PREINSTALL_MAX_PKGS]PreinstallStateKind = [.Unknown; PREINSTALL_MAX_PKGS],
    active_len: usize = 0,

    static fn init(): PreinstallStateTable {
        return PreinstallStateTable{};
    }

    /// 获取包当前预安装状态（超出已初始化范围时返回 `.Unknown`）
    fn get(pkg_id: u32): PreinstallStateKind {
        const idx = pkg_id as usize;
        if (idx >= _.active_len || idx >= PREINSTALL_MAX_PKGS) {
            return .Unknown;
        }
        return _.states[idx];
    }

    /// 显式设置包预安装状态
    fn set(*_, pkg_id: u32, state: PreinstallStateKind): usize! {
        const idx = pkg_id as usize;
        if (idx >= PREINSTALL_MAX_PKGS) {
            return err.PackageIdOutOfBounds;
        }
        if (idx + 1 > _.active_len) {
            _.active_len = idx + 1;
        }
        _.states[idx] = state;
        return idx;
    }

    /// 依据 Bun `determinePreinstallState` 规则决定并记录包的预安装状态
    fn determine(*_, probe: PreinstallProbe): PreinstallStateKind! {
        const idx = probe.pkg_id as usize;
        if (idx >= PREINSTALL_MAX_PKGS) {
            return err.PackageIdOutOfBounds;
        }
        const cur = _.get(probe.pkg_id);
        if (cur != .Unknown) {
            return cur;
        }

        // 1. 平台架构/系统禁用的包直接标记 .Done，不触发下载
        if (probe.is_disabled_for_platform) {
            * = _.set(probe.pkg_id, .Done)?;
            return .Done;
        }

        // 2. 命中 patchedDependencies 且尚未计算 patchfile 哈希 -> .CalcPatchHash
        if (probe.is_patched && probe.patchfile_hash_missing) {
            * = _.set(probe.pkg_id, .CalcPatchHash)?;
            return .CalcPatchHash;
        }

        // 3. 缓存目录为空 -> 必须提取
        if (probe.cached_folder_path.len == 0) {
            * = _.set(probe.pkg_id, .Extract)?;
            return .Extract;
        }

        // 4. 完整缓存目录已存在 -> .Done
        if (probe.is_folder_in_cache) {
            * = _.set(probe.pkg_id, .Done)?;
            return .Done;
        }

        // 5. 若为补丁包，检查未打补丁基础目录是否已在缓存中：
        //    存在则进入 .ApplyPatch（跳过下载解包），否则先 .Extract
        if (probe.is_patched) {
            * = stripPatchHashFolderSuffix(probe.cached_folder_path)?;
            if (probe.is_base_folder_in_cache) {
                * = _.set(probe.pkg_id, .ApplyPatch)?;
                return .ApplyPatch;
            }
            * = _.set(probe.pkg_id, .Extract)?;
            return .Extract;
        }

        * = _.set(probe.pkg_id, .Extract)?;
        return .Extract;
    }
}

/// 零堆分配生命周期脚本耗时日志器（对齐 Bun `LifecycleScriptTimeLog`）
pub struct LifecycleTimeLog {
    entries: [LIFECYCLE_LOG_MAX_ENTRIES]LifecycleTimeEntry = [LifecycleTimeEntry{}; LIFECYCLE_LOG_MAX_ENTRIES],
    count: usize = 0,

    static fn init(): LifecycleTimeLog {
        return LifecycleTimeLog{};
    }

    /// 追加一条已完成脚本的耗时记录
    fn record(*_, package_name: []const u8, script: LifecycleScriptSlot, duration_ns: u64): usize! {
        if (package_name.len == 0) return err.EmptyPackageName;
        if (_.count >= LIFECYCLE_LOG_MAX_ENTRIES) {
            return err.TimeLogFull;
        }
        const idx = _.count;
        _.entries[idx] = LifecycleTimeEntry{
            package_name: package_name,
            script: script,
            duration_ns: duration_ns,
        };
        _.count += 1;
        return idx;
    }

    /// 返回当前记录切片
    fn slice(): []const LifecycleTimeEntry {
        return _.entries[0.._.count];
    }

    /// 查找耗时最长的脚本条目（对齐 Bun `LifecycleScriptTimeLog.printAndDeinit`）
    fn longestEntry(): LifecycleTimeEntry? {
        if (_.count == 0) return null;
        let best_idx: usize = 0;
        let best_dur: u64 = _.entries[0].duration_ns;
        for (i of 1.._.count) {
            if (_.entries[i].duration_ns > best_dur) {
                best_idx = i;
                best_dur = _.entries[i].duration_ns;
            }
        }
        return _.entries[best_idx];
    }

    /// 格式化最慢脚本告警文本：`"<pkg>'s <script> script took <dur>"`
    fn formatLongestWarning(out: []u8): usize! {
        const longest = _.longestEntry() ?? {
            return 0;
        };
        let pos: usize = 0;
        pos = copyInto(out, pos, longest.package_name)?;
        pos = copyInto(out, pos, "'s ")?;
        pos = copyInto(out, pos, longest.script.asStr())?;
        pos = copyInto(out, pos, " script took ")?;
        pos = formatDurationOneDecimal(longest.duration_ns, out[pos..])? + pos;
        return pos;
    }
}

/// 零堆分配慢生命周期脚本运行时监控器（对齐 Bun `reportSlowLifecycleScripts`）
pub struct SlowLifecycleMonitor {
    log_level: MonitorLogLevel = .Default,
    disabled_by_flag: bool = false,
    last_checked_tick: u64 = 0,
    has_checked_tick: bool = false,
    last_reported_at_ns: u64 = 0,
    report_count: usize = 0,

    static fn init(log_level: MonitorLogLevel, disabled_by_flag: bool): SlowLifecycleMonitor {
        return SlowLifecycleMonitor{
            log_level: log_level,
            disabled_by_flag: disabled_by_flag,
        };
    }

    /// 返回当前日志级别对应的告警时间阈值（verbose 5s / default 30s）
    fn thresholdNs(): u64 {
        return if (_.log_level == .Verbose) SLOW_SCRIPT_VERBOSE_INTERVAL_NS else SLOW_SCRIPT_DEFAULT_INTERVAL_NS;
    }

    /// 检查当前运行最久的活跃脚本是否应当触发告警，若触发则写入告警文本并返回写入字节数（否则返回 `0`）
    fn pollAndFormatWarning(
        *_,
        event_loop_tick: u64,
        now_ns: u64,
        active: ActiveLifecycleSnapshot?,
        out: []u8,
    ): usize! {
        if (_.log_level == .Silent || _.disabled_by_flag) {
            return 0;
        }
        const snap = active ?? {
            return 0;
        };
        if (_.has_checked_tick && _.last_checked_tick == event_loop_tick) {
            return 0;
        }
        _.has_checked_tick = true;
        _.last_checked_tick = event_loop_tick;

        const running_ns = if (now_ns > snap.started_at_ns) (now_ns - snap.started_at_ns) else 0;
        const interval = _.thresholdNs();
        const since_last = if (now_ns > _.last_reported_at_ns) (now_ns - _.last_reported_at_ns) else 0;

        if (running_ns > interval && since_last > interval) {
            _.last_reported_at_ns = now_ns;
            _.report_count += 1;

            let pos: usize = 0;
            pos = copyInto(out, pos, snap.package_name)?;
            // 对齐 Bun 英语所有格规则：长度 > 1 且以 's' 结尾时用 "'"，否则用 "'s"
            if (snap.package_name.len > 1 && snap.package_name[snap.package_name.len - 1] == 's') {
                pos = copyInto(out, pos, "' postinstall cost you ")?;
            } else {
                pos = copyInto(out, pos, "'s postinstall cost you ")?;
            }
            pos = formatDurationOneDecimal(running_ns, out[pos..])? + pos;
            return pos;
        }
        return 0;
    }
}
