// Source: Absorbed from /data/dev/bun/src/install/PackageInstall.cp | Synced: 2026-11-22T13:12:13Z
// ============================================================================
// lib/pkg/installContext.cp — Zero-Alloc Platform, Peer & Cache Context Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven installation context, cross-platform
// constraint filter, peer dependency arbitrator, and offline/cache routing
// matrix for `cap install`:
//   1. Cross-platform OS/CPU/Libc constraint matcher (`PlatformConstraintSpec`,
//      `matchesPlatformConstraints`) supporting npm/cap positive allowlists and
//      `!` exclusion rules across `linux`/`darwin`/`win32`/`freebsd`, `x64`/
//      `arm64`/`ia32`/`arm`/`riscv64`, and `glibc`/`musl`;
//   2. Peer dependency auto-install & SemVer conflict arbitrator
//      (`evaluatePeerDependencies`) distinguishing required vs optional peers
//      (`peerDependenciesMeta.<pkg>.optional`), `AutoInstallPeer`, and strict
//      vs warning conflict modes;
//   3. Install mode & cache fallback decision matrix (`InstallCacheStrategy`)
//      combining `--offline`, `--prefer-offline`, `--no-cache`, `--production`,
//      and `--omit=dev|optional|peer` into exact per-package fetch routes;
//   4. Preflight validation (`evaluateInstallPreflight`) and human-readable
//      installation context summary reporter (`formatInstallContextReport`).

/// Default parallel install context concurrency.
pub const INSTALL_CTX_DEFAULT_CONCURRENCY: u32 = 8;

/// Installation context preflight, platform, peer, and routing error set.
pub err InstallContextError {
    OfflineCacheMiss,
    FrozenLockfileRequired,
    IncompatiblePlatform,
    StrictPeerConflict,
    ConflictingCacheFlags,
    SliceCapacityExceeded,
    BufferTooSmall,
}

// ============================================================================
// Internal Zero-Alloc Byte, SemVer & Formatting Helpers
// ============================================================================

fn icBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn icAsciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') return c + 32;
    return c;
}

fn icBytesEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (icAsciiLower(a[i]) != icAsciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn icTrimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t')) start += 1;
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) end -= 1;
    return s[start..end];
}

fn icAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn icAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn icAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        icAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        icAppendByte(out, pos, tmp[n])?;
    }
}

struct IcSemver {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,
    valid: bool = false,
}

fn icParseSemver(raw: []const u8): IcSemver {
    let s = icTrimSpaces(raw);
    if (s.len == 0) return IcSemver{};
    if (s[0] == 'v' || s[0] == 'V') s = s[1..s.len];

    let parts: [3]u32 = [0, 0, 0];
    let p_idx: usize = 0;
    let i: usize = 0;
    let saw_digit = false;

    while (i < s.len && p_idx < 3) {
        const c = s[i];
        if (c >= '0' && c <= '9') {
            parts[p_idx] = parts[p_idx] *% 10 +% @as(u32, c - @as(u8, '0'));
            saw_digit = true;
            i += 1;
        } else if (c == '.') {
            if (!saw_digit) return IcSemver{};
            p_idx += 1;
            saw_digit = false;
            i += 1;
        } else if (c == '-' || c == '+') {
            break;
        } else {
            return IcSemver{};
        }
    }

    if (!saw_digit && p_idx == 0) return IcSemver{};
    return IcSemver{
        major: parts[0],
        minor: parts[1],
        patch: parts[2],
        valid: true,
    };
}

fn icCompareSemver(a: IcSemver, b: IcSemver): i32 {
    if (a.major != b.major) return if (a.major < b.major) -1 else 1;
    if (a.minor != b.minor) return if (a.minor < b.minor) -1 else 1;
    if (a.patch != b.patch) return if (a.patch < b.patch) -1 else 1;
    return 0;
}

fn icSatisfiesSingleComparator(ver: IcSemver, raw_comp: []const u8): bool {
    const comp = icTrimSpaces(raw_comp);
    if (comp.len == 0 || icBytesEql(comp, "*") || icBytesEql(comp, "x") || icBytesEqlIgnoreCase(comp, "latest")) {
        return true;
    }

    if (comp.len >= 2 && comp[0] == '>' && comp[1] == '=') {
        const rhs = icParseSemver(comp[2..comp.len]);
        return rhs.valid && icCompareSemver(ver, rhs) >= 0;
    }
    if (comp.len >= 2 && comp[0] == '<' && comp[1] == '=') {
        const rhs = icParseSemver(comp[2..comp.len]);
        return rhs.valid && icCompareSemver(ver, rhs) <= 0;
    }
    if (comp[0] == '>') {
        const rhs = icParseSemver(comp[1..comp.len]);
        return rhs.valid && icCompareSemver(ver, rhs) > 0;
    }
    if (comp[0] == '<') {
        const rhs = icParseSemver(comp[1..comp.len]);
        return rhs.valid && icCompareSemver(ver, rhs) < 0;
    }
    if (comp[0] == '=') {
        const rhs = icParseSemver(comp[1..comp.len]);
        return rhs.valid && icCompareSemver(ver, rhs) == 0;
    }
    if (comp[0] == '^') {
        const rhs = icParseSemver(comp[1..comp.len]);
        if (!rhs.valid) return false;
        if (icCompareSemver(ver, rhs) < 0) return false;
        if (rhs.major > 0) return ver.major == rhs.major;
        if (rhs.minor > 0) return ver.major == 0 && ver.minor == rhs.minor;
        return ver.major == 0 && ver.minor == 0 && ver.patch == rhs.patch;
    }
    if (comp[0] == '~') {
        const rhs = icParseSemver(comp[1..comp.len]);
        if (!rhs.valid) return false;
        if (icCompareSemver(ver, rhs) < 0) return false;
        return ver.major == rhs.major && ver.minor == rhs.minor;
    }

    const rhs = icParseSemver(comp);
    return rhs.valid && icCompareSemver(ver, rhs) == 0;
}

fn icSatisfiesAndClause(ver: IcSemver, clause: []const u8): bool {
    const s = icTrimSpaces(clause);
    if (s.len == 0) return true;
    let i: usize = 0;
    while (i < s.len) {
        while (i < s.len && (s[i] == ' ' || s[i] == '\t')) i += 1;
        if (i >= s.len) break;
        const start = i;
        while (i < s.len && s[i] != ' ' && s[i] != '\t') i += 1;
        if (!icSatisfiesSingleComparator(ver, s[start..i])) return false;
    }
    return true;
}

/// Evaluates whether `version_str` satisfies a SemVer range (`^`, `~`, `>=`, `<=`, `||`, `*`).
fn icSatisfiesPeerRange(version_str: []const u8, range_str: []const u8): bool {
    const trimmed = icTrimSpaces(range_str);
    if (trimmed.len == 0 || icBytesEql(trimmed, "*") || icBytesEqlIgnoreCase(trimmed, "latest")) {
        return true;
    }
    const ver = icParseSemver(version_str);
    if (!ver.valid) return false;

    let start: usize = 0;
    let i: usize = 0;
    while (i <= trimmed.len) {
        const is_or = (i + 1 < trimmed.len && trimmed[i] == '|' && trimmed[i + 1] == '|');
        if (i == trimmed.len || is_or) {
            if (icSatisfiesAndClause(ver, trimmed[start..i])) return true;
            if (is_or) {
                i += 2;
                start = i;
                continue;
            }
            break;
        }
        i += 1;
    }
    return false;
}

// ============================================================================
// Section 1: Cross-Platform OS / CPU / Libc Constraint Matcher
// ============================================================================

/// Host / target operating system identifier (npm `os` field vocabulary).
pub enum HostOsKind: u8 {
    Linux = 0,
    Darwin = 1,
    Win32 = 2,
    Freebsd = 3,
    Openbsd = 4,
    Sunos = 5,
    Aix = 6,
    Android = 7,

    fn asNpmName(): []const u8 {
        return switch (_) {
            .Linux => "linux",
            .Darwin => "darwin",
            .Win32 => "win32",
            .Freebsd => "freebsd",
            .Openbsd => "openbsd",
            .Sunos => "sunos",
            .Aix => "aix",
            .Android => "android",
        };
    }

    static fn fromNpmName(raw: []const u8): HostOsKind? {
        if (icBytesEqlIgnoreCase(raw, "linux")) return HostOsKind.Linux;
        if (icBytesEqlIgnoreCase(raw, "darwin") || icBytesEqlIgnoreCase(raw, "macos")) return HostOsKind.Darwin;
        if (icBytesEqlIgnoreCase(raw, "win32") || icBytesEqlIgnoreCase(raw, "windows")) return HostOsKind.Win32;
        if (icBytesEqlIgnoreCase(raw, "freebsd")) return HostOsKind.Freebsd;
        if (icBytesEqlIgnoreCase(raw, "openbsd")) return HostOsKind.Openbsd;
        if (icBytesEqlIgnoreCase(raw, "sunos")) return HostOsKind.Sunos;
        if (icBytesEqlIgnoreCase(raw, "aix")) return HostOsKind.Aix;
        if (icBytesEqlIgnoreCase(raw, "android")) return HostOsKind.Android;
        return null;
    }
}

/// Host / target CPU architecture identifier (npm `cpu` field vocabulary).
pub enum HostCpuKind: u8 {
    X64 = 0,
    Arm64 = 1,
    Ia32 = 2,
    Arm = 3,
    Riscv64 = 4,
    Ppc64 = 5,
    S390x = 6,

    fn asNpmName(): []const u8 {
        return switch (_) {
            .X64 => "x64",
            .Arm64 => "arm64",
            .Ia32 => "ia32",
            .Arm => "arm",
            .Riscv64 => "riscv64",
            .Ppc64 => "ppc64",
            .S390x => "s390x",
        };
    }

    static fn fromNpmName(raw: []const u8): HostCpuKind? {
        if (icBytesEqlIgnoreCase(raw, "x64") || icBytesEqlIgnoreCase(raw, "x86_64")) return HostCpuKind.X64;
        if (icBytesEqlIgnoreCase(raw, "arm64") || icBytesEqlIgnoreCase(raw, "aarch64")) return HostCpuKind.Arm64;
        if (icBytesEqlIgnoreCase(raw, "ia32") || icBytesEqlIgnoreCase(raw, "x86")) return HostCpuKind.Ia32;
        if (icBytesEqlIgnoreCase(raw, "arm")) return HostCpuKind.Arm;
        if (icBytesEqlIgnoreCase(raw, "riscv64")) return HostCpuKind.Riscv64;
        if (icBytesEqlIgnoreCase(raw, "ppc64")) return HostCpuKind.Ppc64;
        if (icBytesEqlIgnoreCase(raw, "s390x")) return HostCpuKind.S390x;
        return null;
    }
}

/// Linux C library variant (npm `libc` field vocabulary).
pub enum HostLibcKind: u8 {
    None = 0,
    Glibc = 1,
    Musl = 2,

    fn asNpmName(): []const u8 {
        return switch (_) {
            .None => "",
            .Glibc => "glibc",
            .Musl => "musl",
        };
    }

    static fn fromNpmName(raw: []const u8): HostLibcKind? {
        if (icBytesEqlIgnoreCase(raw, "glibc") || icBytesEqlIgnoreCase(raw, "gnu")) return HostLibcKind.Glibc;
        if (icBytesEqlIgnoreCase(raw, "musl")) return HostLibcKind.Musl;
        return null;
    }
}

/// Complete host platform target triple used during package installation.
pub struct HostPlatformTarget {
    os: HostOsKind = HostOsKind.Linux,
    cpu: HostCpuKind = HostCpuKind.X64,
    libc: HostLibcKind = HostLibcKind.Glibc,
}

/// Package-declared `os`, `cpu`, and `libc` constraint arrays (supporting `!` negations).
pub struct PlatformConstraintSpec {
    os_list: []const []const u8 = [],
    cpu_list: []const []const u8 = [],
    libc_list: []const []const u8 = [],

    static fn any(): PlatformConstraintSpec {
        return PlatformConstraintSpec{};
    }
}

/// Specific dimension that caused a platform mismatch.
pub enum PlatformMatchReason: u8 {
    Matched = 0,
    OsMismatch = 1,
    CpuMismatch = 2,
    LibcMismatch = 3,
}

/// Outcome returned by `matchesPlatformConstraints`.
pub struct PlatformMatchResult {
    matched: bool = true,
    reason: PlatformMatchReason = PlatformMatchReason.Matched,
}

/// Evaluates a single npm constraint array (`["linux", "darwin"]` or `["!win32"]`)
/// against `target_name`.
pub fn matchesConstraintDimension(target_name: []const u8, patterns: []const []const u8): bool {
    if (patterns.len == 0) return true;

    let has_positive = false;
    let matched_positive = false;

    let i: usize = 0;
    while (i < patterns.len) {
        const pat = icTrimSpaces(patterns[i]);
        if (pat.len > 0) {
            if (pat[0] == '!') {
                const negated = pat[1..pat.len];
                if (negated.len > 0 && icBytesEqlIgnoreCase(target_name, negated)) {
                    return false;
                }
            } else {
                has_positive = true;
                if (icBytesEqlIgnoreCase(target_name, pat)) {
                    matched_positive = true;
                }
            }
        }
        i += 1;
    }

    if (has_positive) return matched_positive;
    return true;
}

/// Evaluates whether a package's `os`, `cpu`, and `libc` constraints match `host`.
pub fn matchesPlatformConstraints(
    host: HostPlatformTarget,
    spec: PlatformConstraintSpec,
): PlatformMatchResult {
    if (!matchesConstraintDimension(host.os.asNpmName(), spec.os_list)) {
        return PlatformMatchResult{ matched: false, reason: PlatformMatchReason.OsMismatch };
    }
    if (!matchesConstraintDimension(host.cpu.asNpmName(), spec.cpu_list)) {
        return PlatformMatchResult{ matched: false, reason: PlatformMatchReason.CpuMismatch };
    }
    if (spec.libc_list.len > 0) {
        const libc_name = host.libc.asNpmName();
        if (libc_name.len == 0) {
            // Non-Linux hosts with no libc cannot satisfy positive libc constraints
            let has_pos = false;
            let i: usize = 0;
            while (i < spec.libc_list.len) {
                const p = icTrimSpaces(spec.libc_list[i]);
                if (p.len > 0 && p[0] != '!') has_pos = true;
                i += 1;
            }
            if (has_pos) {
                return PlatformMatchResult{ matched: false, reason: PlatformMatchReason.LibcMismatch };
            }
        } else if (!matchesConstraintDimension(libc_name, spec.libc_list)) {
            return PlatformMatchResult{ matched: false, reason: PlatformMatchReason.LibcMismatch };
        }
    }
    return PlatformMatchResult{ matched: true, reason: PlatformMatchReason.Matched };
}

// ============================================================================
// Section 2: Peer Dependency Auto-Install & Conflict Arbitrator
// ============================================================================

/// Peer dependency requirement declared by a package.
pub struct PeerDepRequirement {
    requester_name: []const u8 = "",
    peer_name: []const u8 = "",
    required_range: []const u8 = "*",
    is_optional: bool = false,

    static fn empty(): PeerDepRequirement {
        return PeerDepRequirement{};
    }
}

/// Package currently resolved/installed in the host dependency graph.
pub struct InstalledPeerCandidate {
    name: []const u8 = "",
    version: []const u8 = "",

    static fn empty(): InstalledPeerCandidate {
        return InstalledPeerCandidate{};
    }
}

/// Arbitration outcome for a single peer dependency requirement.
pub enum PeerEvalOutcome: u8 {
    Satisfied = 0,
    AutoInstallPeer = 1,
    SkipOptionalMissing = 2,
    ConflictWarn = 3,
    ConflictBlock = 4,
    Omitted = 5,
}

/// Per-requirement peer evaluation decision.
pub struct PeerEvalDecision {
    requester_name: []const u8 = "",
    peer_name: []const u8 = "",
    required_range: []const u8 = "",
    installed_version: []const u8 = "",
    outcome: PeerEvalOutcome = PeerEvalOutcome.Satisfied,

    static fn empty(): PeerEvalDecision {
        return PeerEvalDecision{};
    }
}

/// Options governing `evaluatePeerDependencies`.
pub struct PeerEvalOptions {
    auto_install_peers: bool = true,
    strict_peer_deps: bool = false,
    omit_peer: bool = false,

    static fn defaults(): PeerEvalOptions {
        return PeerEvalOptions{};
    }
}

/// Aggregate counts returned by `evaluatePeerDependencies`.
pub struct PeerEvalSummary {
    total_evaluated: u32 = 0,
    satisfied_count: u32 = 0,
    auto_install_count: u32 = 0,
    skipped_optional_count: u32 = 0,
    conflict_warn_count: u32 = 0,
    conflict_block_count: u32 = 0,
    omitted_count: u32 = 0,
}

fn findInstalledPeer(installed: []const InstalledPeerCandidate, peer_name: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < installed.len) {
        if (icBytesEql(installed[i].name, peer_name)) {
            return installed[i].version;
        }
        i += 1;
    }
    return null;
}

/// Evaluates all peer dependency requirements against `installed` packages,
/// triggering `AutoInstallPeer` for missing required peers, skipping missing
/// optional peers, and flagging version conflicts according to `strict_peer_deps`.
pub fn evaluatePeerDependencies(
    reqs: []const PeerDepRequirement,
    installed: []const InstalledPeerCandidate,
    opts: PeerEvalOptions,
    out_decisions: []PeerEvalDecision,
): PeerEvalSummary! {
    if (reqs.len > out_decisions.len) return err.SliceCapacityExceeded;

    let sum = PeerEvalSummary{};
    let i: usize = 0;
    while (i < reqs.len) {
        const r = reqs[i];
        sum.total_evaluated += 1;

        if (opts.omit_peer) {
            out_decisions[i] = PeerEvalDecision{
                requester_name: r.requester_name,
                peer_name: r.peer_name,
                required_range: r.required_range,
                outcome: PeerEvalOutcome.Omitted,
            };
            sum.omitted_count += 1;
            i += 1;
            continue;
        }

        if (inst_ver := findInstalledPeer(installed, r.peer_name)) {
            if (icSatisfiesPeerRange(inst_ver, r.required_range)) {
                out_decisions[i] = PeerEvalDecision{
                    requester_name: r.requester_name,
                    peer_name: r.peer_name,
                    required_range: r.required_range,
                    installed_version: inst_ver,
                    outcome: PeerEvalOutcome.Satisfied,
                };
                sum.satisfied_count += 1;
            } else if (opts.strict_peer_deps && !r.is_optional) {
                out_decisions[i] = PeerEvalDecision{
                    requester_name: r.requester_name,
                    peer_name: r.peer_name,
                    required_range: r.required_range,
                    installed_version: inst_ver,
                    outcome: PeerEvalOutcome.ConflictBlock,
                };
                sum.conflict_block_count += 1;
            } else {
                out_decisions[i] = PeerEvalDecision{
                    requester_name: r.requester_name,
                    peer_name: r.peer_name,
                    required_range: r.required_range,
                    installed_version: inst_ver,
                    outcome: PeerEvalOutcome.ConflictWarn,
                };
                sum.conflict_warn_count += 1;
            }
        } else {
            if (r.is_optional) {
                out_decisions[i] = PeerEvalDecision{
                    requester_name: r.requester_name,
                    peer_name: r.peer_name,
                    required_range: r.required_range,
                    outcome: PeerEvalOutcome.SkipOptionalMissing,
                };
                sum.skipped_optional_count += 1;
            } else if (opts.auto_install_peers) {
                out_decisions[i] = PeerEvalDecision{
                    requester_name: r.requester_name,
                    peer_name: r.peer_name,
                    required_range: r.required_range,
                    outcome: PeerEvalOutcome.AutoInstallPeer,
                };
                sum.auto_install_count += 1;
            } else if (opts.strict_peer_deps) {
                out_decisions[i] = PeerEvalDecision{
                    requester_name: r.requester_name,
                    peer_name: r.peer_name,
                    required_range: r.required_range,
                    outcome: PeerEvalOutcome.ConflictBlock,
                };
                sum.conflict_block_count += 1;
            } else {
                out_decisions[i] = PeerEvalDecision{
                    requester_name: r.requester_name,
                    peer_name: r.peer_name,
                    required_range: r.required_range,
                    outcome: PeerEvalOutcome.ConflictWarn,
                };
                sum.conflict_warn_count += 1;
            }
        }
        i += 1;
    }

    return sum;
}

// ============================================================================
// Section 3: Install Mode & Cache Fallback Decision Matrix
// ============================================================================

/// Dependency section category for `--production` and `--omit=...` filtering.
pub enum InstallDepCategory: u8 {
    Production = 0,
    Development = 1,
    Optional = 2,
    Peer = 3,
}

/// Complete installation context configuration passed into `InstallCacheStrategy` & preflight.
pub struct InstallContextConfig {
    host: HostPlatformTarget = HostPlatformTarget{},
    offline: bool = false,
    prefer_offline: bool = false,
    no_cache: bool = false,
    frozen_lockfile: bool = false,
    production: bool = false,
    omit_dev: bool = false,
    omit_optional: bool = false,
    omit_peer: bool = false,
    auto_install_peers: bool = true,
    strict_peer_deps: bool = false,
    has_lockfile: bool = true,
    lockfile_in_sync: bool = true,

    static fn defaults(): InstallContextConfig {
        return InstallContextConfig{};
    }
}

/// Package candidate evaluated by `InstallCacheStrategy`.
pub struct InstallPkgCandidate {
    name: []const u8 = "",
    version: []const u8 = "",
    category: InstallDepCategory = InstallDepCategory.Production,
    platform: PlatformConstraintSpec = PlatformConstraintSpec{},
    in_global_store: bool = false,
    has_valid_etag: bool = false,
    is_direct_tarball: bool = false,

    static fn empty(): InstallPkgCandidate {
        return InstallPkgCandidate{};
    }
}

/// Concrete fetch / link / skip route chosen for a package candidate.
pub enum InstallFetchRoute: u8 {
    LocalGlobalStoreLink = 0,
    RegistryFetchWithEtag = 1,
    RegistryFetchFresh = 2,
    TarballDirect = 3,
    SkipOmitted = 4,
    SkipIncompatiblePlatform = 5,
    FailOfflineMiss = 6,
    FailIncompatibleRequired = 7,
}

/// Per-package routing decision returned by `InstallCacheStrategy`.
pub struct InstallRouteDecision {
    name: []const u8 = "",
    route: InstallFetchRoute = InstallFetchRoute.LocalGlobalStoreLink,
    platform_reason: PlatformMatchReason = PlatformMatchReason.Matched,

    static fn empty(): InstallRouteDecision {
        return InstallRouteDecision{};
    }
}

/// Aggregate route counts across a batch of packages.
pub struct InstallRouteSummary {
    total_packages: u32 = 0,
    local_store_links: u32 = 0,
    registry_etag_fetches: u32 = 0,
    registry_fresh_fetches: u32 = 0,
    tarball_direct_fetches: u32 = 0,
    skipped_omitted: u32 = 0,
    skipped_platform: u32 = 0,
    failed_offline_miss: u32 = 0,
    failed_platform_required: u32 = 0,
}

/// Stateless decision matrix mapping install flags + cache state + platform constraints to fetch routes.
pub struct InstallCacheStrategy {
    static fn decidePackageRoute(cfg: InstallContextConfig, pkg: InstallPkgCandidate): InstallRouteDecision {
        // 1. `--production` / `--omit=...` category pruning
        if (pkg.category == InstallDepCategory.Development && (cfg.production || cfg.omit_dev)) {
            return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.SkipOmitted };
        }
        if (pkg.category == InstallDepCategory.Optional && cfg.omit_optional) {
            return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.SkipOmitted };
        }
        if (pkg.category == InstallDepCategory.Peer && cfg.omit_peer) {
            return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.SkipOmitted };
        }

        // 2. Cross-platform OS / CPU / Libc constraint check
        const pm = matchesPlatformConstraints(cfg.host, pkg.platform);
        if (!pm.matched) {
            if (pkg.category == InstallDepCategory.Optional) {
                return InstallRouteDecision{
                    name: pkg.name,
                    route: InstallFetchRoute.SkipIncompatiblePlatform,
                    platform_reason: pm.reason,
                };
            }
            return InstallRouteDecision{
                name: pkg.name,
                route: InstallFetchRoute.FailIncompatibleRequired,
                platform_reason: pm.reason,
            };
        }

        // 3. Offline / cache-bypass / global-store routing
        if (cfg.offline) {
            if (pkg.in_global_store) {
                return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.LocalGlobalStoreLink };
            }
            return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.FailOfflineMiss };
        }

        if (cfg.no_cache) {
            if (pkg.is_direct_tarball) {
                return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.TarballDirect };
            }
            return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.RegistryFetchFresh };
        }

        if (pkg.in_global_store) {
            return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.LocalGlobalStoreLink };
        }

        if (pkg.is_direct_tarball) {
            return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.TarballDirect };
        }

        if (pkg.has_valid_etag && !cfg.prefer_offline) {
            return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.RegistryFetchWithEtag };
        }

        return InstallRouteDecision{ name: pkg.name, route: InstallFetchRoute.RegistryFetchWithEtag };
    }

    static fn decideBatch(
        cfg: InstallContextConfig,
        pkgs: []const InstallPkgCandidate,
        out: []InstallRouteDecision,
    ): InstallRouteSummary! {
        if (pkgs.len > out.len) return err.SliceCapacityExceeded;

        let sum = InstallRouteSummary{};
        let i: usize = 0;
        while (i < pkgs.len) {
            const decision = InstallCacheStrategy.decidePackageRoute(cfg, pkgs[i]);
            out[i] = decision;
            sum.total_packages += 1;
            switch (decision.route) {
                .LocalGlobalStoreLink => { sum.local_store_links += 1; },
                .RegistryFetchWithEtag => { sum.registry_etag_fetches += 1; },
                .RegistryFetchFresh => { sum.registry_fresh_fetches += 1; },
                .TarballDirect => { sum.tarball_direct_fetches += 1; },
                .SkipOmitted => { sum.skipped_omitted += 1; },
                .SkipIncompatiblePlatform => { sum.skipped_platform += 1; },
                .FailOfflineMiss => { sum.failed_offline_miss += 1; },
                .FailIncompatibleRequired => { sum.failed_platform_required += 1; },
            }
            i += 1;
        }
        return sum;
    }
}

// ============================================================================
// Section 4: Preflight Validation & Context Summary Reporter
// ============================================================================

/// Preflight diagnostic code produced by `evaluateInstallPreflight`.
pub enum InstallPreflightIssue: u8 {
    None = 0,
    OfflineAndNoCacheConflict = 1,
    FrozenLockfileMissing = 2,
    FrozenLockfileOutOfSync = 3,
}

/// Preflight validation outcome.
pub struct InstallPreflightResult {
    ok: bool = true,
    issue: InstallPreflightIssue = InstallPreflightIssue.None,
    message: []const u8 = "",
}

/// Validates global `cap install` context invariants before network or disk work begins.
pub fn evaluateInstallPreflight(cfg: InstallContextConfig): InstallPreflightResult {
    if (cfg.offline && cfg.no_cache) {
        return InstallPreflightResult{
            ok: false,
            issue: InstallPreflightIssue.OfflineAndNoCacheConflict,
            message: "cannot combine --offline with --no-cache",
        };
    }
    if (cfg.frozen_lockfile && !cfg.has_lockfile) {
        return InstallPreflightResult{
            ok: false,
            issue: InstallPreflightIssue.FrozenLockfileMissing,
            message: "--frozen-lockfile requires an existing cap.lock",
        };
    }
    if (cfg.frozen_lockfile && !cfg.lockfile_in_sync) {
        return InstallPreflightResult{
            ok: false,
            issue: InstallPreflightIssue.FrozenLockfileOutOfSync,
            message: "cap.lock is out of sync with cap.json under --frozen-lockfile",
        };
    }
    return InstallPreflightResult{ ok: true, issue: InstallPreflightIssue.None, message: "" };
}

/// Formats a concise installation context, routing, and peer arbitration summary report.
pub fn formatInstallContextReport(
    cfg: InstallContextConfig,
    route_sum: InstallRouteSummary,
    peer_sum: PeerEvalSummary,
    out: []u8,
): usize! {
    let pos: usize = 0;
    icAppendBytes(out, &pos, "Install Context [")?;
    icAppendBytes(out, &pos, cfg.host.os.asNpmName())?;
    icAppendByte(out, &pos, '-')?;
    icAppendBytes(out, &pos, cfg.host.cpu.asNpmName())?;
    if (cfg.host.libc != HostLibcKind.None) {
        icAppendByte(out, &pos, '-')?;
        icAppendBytes(out, &pos, cfg.host.libc.asNpmName())?;
    }
    icAppendBytes(out, &pos, "]")?;
    if (cfg.offline) icAppendBytes(out, &pos, " --offline")?;
    if (cfg.production) icAppendBytes(out, &pos, " --production")?;
    if (cfg.frozen_lockfile) icAppendBytes(out, &pos, " --frozen-lockfile")?;
    icAppendByte(out, &pos, '\n')?;

    icAppendBytes(out, &pos, "Routes: ")?;
    icAppendU64(out, &pos, @as(u64, route_sum.local_store_links))?;
    icAppendBytes(out, &pos, " cache-link, ")?;
    icAppendU64(out, &pos, @as(u64, route_sum.registry_etag_fetches + route_sum.registry_fresh_fetches + route_sum.tarball_direct_fetches))?;
    icAppendBytes(out, &pos, " network, ")?;
    icAppendU64(out, &pos, @as(u64, route_sum.skipped_omitted + route_sum.skipped_platform))?;
    icAppendBytes(out, &pos, " skipped")?;
    if (route_sum.failed_offline_miss > 0 || route_sum.failed_platform_required > 0) {
        icAppendBytes(out, &pos, ", ")?;
        icAppendU64(out, &pos, @as(u64, route_sum.failed_offline_miss + route_sum.failed_platform_required))?;
        icAppendBytes(out, &pos, " blocked")?;
    }
    icAppendByte(out, &pos, '\n')?;

    icAppendBytes(out, &pos, "Peers: ")?;
    icAppendU64(out, &pos, @as(u64, peer_sum.satisfied_count))?;
    icAppendBytes(out, &pos, " satisfied, ")?;
    icAppendU64(out, &pos, @as(u64, peer_sum.auto_install_count))?;
    icAppendBytes(out, &pos, " auto-install, ")?;
    icAppendU64(out, &pos, @as(u64, peer_sum.conflict_warn_count))?;
    icAppendBytes(out, &pos, " warn, ")?;
    icAppendU64(out, &pos, @as(u64, peer_sum.conflict_block_count))?;
    icAppendBytes(out, &pos, " blocked\n")?;

    return pos;
}
