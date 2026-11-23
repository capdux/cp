import { test, assert, assertEq } from "cap:test";
import {
    HostOsKind,
    HostCpuKind,
    HostLibcKind,
    HostPlatformTarget,
    PlatformConstraintSpec,
    PlatformMatchReason,
    matchesPlatformConstraints,
    PeerDepRequirement,
    InstalledPeerCandidate,
    PeerEvalOutcome,
    PeerEvalDecision,
    PeerEvalOptions,
    evaluatePeerDependencies,
    InstallDepCategory,
    InstallContextConfig,
    InstallPkgCandidate,
    InstallFetchRoute,
    InstallRouteDecision,
    InstallCacheStrategy,
    InstallPreflightIssue,
    evaluateInstallPreflight,
    formatInstallContextReport,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("matchesPlatformConstraints filters OS, CPU, and Libc including ! negations for native binaries", () => {
    const linux_x64_glibc = HostPlatformTarget{ os: HostOsKind.Linux, cpu: HostCpuKind.X64, libc: HostLibcKind.Glibc };
    const darwin_arm64 = HostPlatformTarget{ os: HostOsKind.Darwin, cpu: HostCpuKind.Arm64, libc: HostLibcKind.None };

    const os_linux: [1][]const u8 = ["linux"];
    const cpu_x64: [1][]const u8 = ["x64"];
    const libc_glibc: [1][]const u8 = ["glibc"];
    const libc_musl: [1][]const u8 = ["musl"];
    const not_win: [1][]const u8 = ["!win32"];
    const not_linux: [1][]const u8 = ["!linux"];

    // @cap/vips-linux-x64 matches linux-x64-glibc, fails on darwin-arm64 (OsMismatch)
    const vips_linux = PlatformConstraintSpec{ os_list: os_linux[0..], cpu_list: cpu_x64[0..], libc_list: libc_glibc[0..] };
    const m1 = matchesPlatformConstraints(linux_x64_glibc, vips_linux);
    assert(m1.matched);

    const m2 = matchesPlatformConstraints(darwin_arm64, vips_linux);
    assert(!m2.matched && m2.reason == PlatformMatchReason.OsMismatch);

    // Musl binary rejected on glibc Linux host
    const m3 = matchesPlatformConstraints(linux_x64_glibc, PlatformConstraintSpec{ os_list: os_linux[0..], libc_list: libc_musl[0..] });
    assert(!m3.matched && m3.reason == PlatformMatchReason.LibcMismatch);

    // Non-Linux host with no libc rejects positive libc constraint
    const m4 = matchesPlatformConstraints(darwin_arm64, PlatformConstraintSpec{ libc_list: libc_glibc[0..] });
    assert(!m4.matched && m4.reason == PlatformMatchReason.LibcMismatch);

    // Negations: !win32 allows linux & darwin; !linux blocks linux
    assert(matchesPlatformConstraints(linux_x64_glibc, PlatformConstraintSpec{ os_list: not_win[0..] }).matched);
    assert(!matchesPlatformConstraints(linux_x64_glibc, PlatformConstraintSpec{ os_list: not_linux[0..] }).matched);
});

test("HostOsKind, HostCpuKind, and HostLibcKind parse npm/cap aliases cleanly", () => {
    assert(HostOsKind.fromNpmName("darwin") == HostOsKind.Darwin);
    assert(HostOsKind.fromNpmName("macos") == HostOsKind.Darwin);
    assert(HostOsKind.fromNpmName("win32") == HostOsKind.Win32);
    assert(HostCpuKind.fromNpmName("aarch64") == HostCpuKind.Arm64);
    assert(HostCpuKind.fromNpmName("x86_64") == HostCpuKind.X64);
    assert(HostCpuKind.fromNpmName("riscv64") == HostCpuKind.Riscv64);
    assert(HostLibcKind.fromNpmName("gnu") == HostLibcKind.Glibc);
    assert(HostLibcKind.fromNpmName("musl") == HostLibcKind.Musl);
});

test("evaluatePeerDependencies handles Satisfied, AutoInstallPeer, SkipOptionalMissing, and ConflictWarn vs ConflictBlock", () => {
    const installed: [2]InstalledPeerCandidate = [
        InstalledPeerCandidate{ name: "react", version: "18.3.1" },
        InstalledPeerCandidate{ name: "typescript", version: "4.9.5" },
    ];
    const reqs: [4]PeerDepRequirement = [
        PeerDepRequirement{ requester_name: "ui-kit", peer_name: "react", required_range: "^18.0.0 || ^19.0.0" },
        PeerDepRequirement{ requester_name: "ui-kit", peer_name: "react-dom", required_range: "^18.0.0", is_optional: false },
        PeerDepRequirement{ requester_name: "ui-kit", peer_name: "framer-motion", required_range: "^11.0.0", is_optional: true },
        PeerDepRequirement{ requester_name: "ts-plugin", peer_name: "typescript", required_range: ">=5.0.0", is_optional: false },
    ];

    let out: [4]PeerEvalDecision = [PeerEvalDecision.empty(); 4];
    const s1 = evaluatePeerDependencies(reqs[0..], installed[0..], PeerEvalOptions.defaults(), out[0..]) catch unreachable;
    assertEq(s1.satisfied_count, @as(u32, 1));
    assertEq(s1.auto_install_count, @as(u32, 1));
    assertEq(s1.skipped_optional_count, @as(u32, 1));
    assertEq(s1.conflict_warn_count, @as(u32, 1));
    assert(out[0].outcome == PeerEvalOutcome.Satisfied);
    assert(out[1].outcome == PeerEvalOutcome.AutoInstallPeer);
    assert(out[2].outcome == PeerEvalOutcome.SkipOptionalMissing);
    assert(out[3].outcome == PeerEvalOutcome.ConflictWarn);

    // Strict peer mode upgrades version conflict to ConflictBlock
    const s2 = evaluatePeerDependencies(
        reqs[0..],
        installed[0..],
        PeerEvalOptions{ auto_install_peers: true, strict_peer_deps: true },
        out[0..],
    ) catch unreachable;
    assertEq(s2.conflict_block_count, @as(u32, 1));
    assert(out[3].outcome == PeerEvalOutcome.ConflictBlock);
});

test("InstallCacheStrategy prunes --production devDeps, skips incompatible optional binaries, and blocks incompatible required pkgs", () => {
    const host = HostPlatformTarget{ os: HostOsKind.Linux, cpu: HostCpuKind.X64, libc: HostLibcKind.Glibc };
    const darwin_only: [1][]const u8 = ["darwin"];
    const cfg = InstallContextConfig{ host: host, production: true };

    const pkgs: [5]InstallPkgCandidate = [
        InstallPkgCandidate{ name: "zod", category: InstallDepCategory.Production, in_global_store: true },
        InstallPkgCandidate{ name: "vitest", category: InstallDepCategory.Development, in_global_store: true },
        InstallPkgCandidate{ name: "@cap/vips-darwin-arm64", category: InstallDepCategory.Optional, platform: PlatformConstraintSpec{ os_list: darwin_only[0..] } },
        InstallPkgCandidate{ name: "macos-only-required", category: InstallDepCategory.Production, platform: PlatformConstraintSpec{ os_list: darwin_only[0..] } },
        InstallPkgCandidate{ name: "fresh-pkg", category: InstallDepCategory.Production, in_global_store: false, has_valid_etag: true },
    ];

    let routes: [5]InstallRouteDecision = [InstallRouteDecision.empty(); 5];
    const sum = InstallCacheStrategy.decideBatch(cfg, pkgs[0..], routes[0..]) catch unreachable;

    assertEq(sum.local_store_links, @as(u32, 1));
    assertEq(sum.skipped_omitted, @as(u32, 1));
    assertEq(sum.skipped_platform, @as(u32, 1));
    assertEq(sum.failed_platform_required, @as(u32, 1));
    assertEq(sum.registry_etag_fetches, @as(u32, 1));

    assert(routes[0].route == InstallFetchRoute.LocalGlobalStoreLink);
    assert(routes[1].route == InstallFetchRoute.SkipOmitted);
    assert(routes[2].route == InstallFetchRoute.SkipIncompatiblePlatform);
    assert(routes[3].route == InstallFetchRoute.FailIncompatibleRequired);
    assert(routes[4].route == InstallFetchRoute.RegistryFetchWithEtag);
});

test("InstallCacheStrategy enforces --offline cache misses and --no-cache fresh fetches", () => {
    const cached = InstallPkgCandidate{ name: "cached-lib", in_global_store: true };
    const uncached = InstallPkgCandidate{ name: "missing-lib", in_global_store: false };
    const tarball = InstallPkgCandidate{ name: "direct-tgz", in_global_store: true, is_direct_tarball: true };

    // --offline links cached, fails uncached
    const off_cfg = InstallContextConfig{ offline: true };
    assert(InstallCacheStrategy.decidePackageRoute(off_cfg, cached).route == InstallFetchRoute.LocalGlobalStoreLink);
    assert(InstallCacheStrategy.decidePackageRoute(off_cfg, uncached).route == InstallFetchRoute.FailOfflineMiss);

    // --no-cache bypasses global store
    const nocache_cfg = InstallContextConfig{ no_cache: true };
    assert(InstallCacheStrategy.decidePackageRoute(nocache_cfg, cached).route == InstallFetchRoute.RegistryFetchFresh);
    assert(InstallCacheStrategy.decidePackageRoute(nocache_cfg, tarball).route == InstallFetchRoute.TarballDirect);
});

test("evaluateInstallPreflight catches --offline+--no-cache conflict and --frozen-lockfile violations", () => {
    const p1 = evaluateInstallPreflight(InstallContextConfig{ offline: true, no_cache: true });
    assert(!p1.ok && p1.issue == InstallPreflightIssue.OfflineAndNoCacheConflict);

    const p2 = evaluateInstallPreflight(InstallContextConfig{ frozen_lockfile: true, has_lockfile: false });
    assert(!p2.ok && p2.issue == InstallPreflightIssue.FrozenLockfileMissing);

    const p3 = evaluateInstallPreflight(InstallContextConfig{ frozen_lockfile: true, has_lockfile: true, lockfile_in_sync: false });
    assert(!p3.ok && p3.issue == InstallPreflightIssue.FrozenLockfileOutOfSync);

    const p4 = evaluateInstallPreflight(InstallContextConfig{ frozen_lockfile: true, has_lockfile: true, lockfile_in_sync: true });
    assert(p4.ok);
});

test("formatInstallContextReport renders host triple, flags, route counts, and peer summary", () => {
    const cfg = InstallContextConfig{
        host: HostPlatformTarget{ os: HostOsKind.Linux, cpu: HostCpuKind.Arm64, libc: HostLibcKind.Musl },
        offline: true,
        production: true,
        frozen_lockfile: true,
    };
    const pkgs: [2]InstallPkgCandidate = [
        InstallPkgCandidate{ name: "zod", in_global_store: true },
        InstallPkgCandidate{ name: "dev-only", category: InstallDepCategory.Development },
    ];
    let routes: [2]InstallRouteDecision = [InstallRouteDecision.empty(); 2];
    const rsum = InstallCacheStrategy.decideBatch(cfg, pkgs[0..], routes[0..]) catch unreachable;

    const reqs: [1]PeerDepRequirement = [
        PeerDepRequirement{ requester_name: "a", peer_name: "b", required_range: "^1.0.0" },
    ];
    const inst: [0]InstalledPeerCandidate = [];
    let pdec: [1]PeerEvalDecision = [PeerEvalDecision.empty(); 1];
    const psum = evaluatePeerDependencies(reqs[0..], inst[0..], PeerEvalOptions.defaults(), pdec[0..]) catch unreachable;

    let buf: [512]u8 = [0; 512];
    const n = formatInstallContextReport(cfg, rsum, psum, buf[0..]) catch unreachable;
    const text = buf[0..n];
    assert(strContains(text, "Install Context [linux-arm64-musl] --offline --production --frozen-lockfile"));
    assert(strContains(text, "Routes: 1 cache-link, 0 network, 1 skipped"));
    assert(strContains(text, "Peers: 0 satisfied, 1 auto-install, 0 warn, 0 blocked"));
});
