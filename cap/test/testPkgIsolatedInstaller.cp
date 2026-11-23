import { test, assert, assertEq } from "cap:test";
import {
    IsoInstallerError,
    isSafeIsoInstPath,
    isSafeIsoBinName,
    IsoLinkBackend,
    isoLinkBackendName,
    IsoHostOs,
    IsoFsErrorCode,
    IsoBackendFallbackState,
    IsoInstOpKind,
    IsoInstDepLink,
    IsoInstStorePackage,
    IsoInstOp,
    getIsoOpSrc,
    getIsoOpDst,
    planIsolatedMaterialization,
    IsoBinCandidate,
    IsoBinShimFormat,
    IsoBinPlannedLink,
    planBinShimLinks,
    formatWindowsCmdShim,
    formatWindowsPs1Shim,
    formatStagingStorePaths,
    formatIsoInstallerReport,
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

test("IsoBackendFallbackState performs deterministic cross-filesystem downgrades on EXDEV/ENOTSUP/EMLINK", () => {
    // 1. macOS: Clonefile -> (ENOTSUP) -> Hardlink -> (EMLINK) -> CopyFile -> (permanent)
    let mac = IsoBackendFallbackState.init(IsoHostOs.MacOS, null);
    assert(mac.active_backend == IsoLinkBackend.Clonefile);

    const d1 = mac.recordFailure(IsoFsErrorCode.Enotsup);
    assert(d1.should_retry && d1.next_backend == IsoLinkBackend.Hardlink && !d1.permanent_error);

    const d2 = mac.recordFailure(IsoFsErrorCode.Emlink);
    assert(d2.should_retry && d2.next_backend == IsoLinkBackend.CopyFile && !d2.permanent_error);

    const d3 = mac.recordFailure(IsoFsErrorCode.Enotsup);
    assert(!d3.should_retry && d3.permanent_error);
    assertEq(mac.downgrade_count, @as(u32, 2));
    assertEq(mac.enotsup_hits, @as(u32, 2));
    assertEq(mac.emlink_hits, @as(u32, 1));

    // 2. Linux: Reflink -> (EXDEV) jumps directly to CopyFile (hardlink also fails across mounts)
    let lin = IsoBackendFallbackState.init(IsoHostOs.Linux, null);
    assert(lin.active_backend == IsoLinkBackend.Reflink);
    const dl = lin.recordFailure(IsoFsErrorCode.Exdev);
    assert(dl.should_retry && dl.next_backend == IsoLinkBackend.CopyFile);
    assertEq(lin.exdev_hits, @as(u32, 1));

    // 3. EEXIST is idempotent skip; ENOSPC is permanent error without downgrade
    const de = lin.recordFailure(IsoFsErrorCode.Eexist);
    assert(de.idempotent_skip && !de.should_retry && !de.permanent_error);
    const dn = lin.recordFailure(IsoFsErrorCode.Enospc);
    assert(dn.permanent_error && !dn.should_retry);
    assert(strEql(isoLinkBackendName(IsoLinkBackend.CopyFile), "copyfile"));
});

test("planIsolatedMaterialization builds deterministic store dirs, relative symlinks, and stale entry prunes", () => {
    // Direct scoped dep `@org/app@1.0.0` depends on transitive `zod@3.23.8`
    const dep_links: [1]IsoInstDepLink = [
        IsoInstDepLink{ dep_alias: "zod", target_pkg_index: 1 },
    ];
    const pkgs: [2]IsoInstStorePackage = [
        IsoInstStorePackage{
            name: "@org/app",
            version: "1.0.0",
            cache_source_dir: "/cache/@org+app@1.0.0",
            is_direct: true,
            deps_start: 0,
            deps_len: 1,
        },
        IsoInstStorePackage{
            name: "zod",
            version: "3.23.8",
            cache_source_dir: "/cache/zod@3.23.8",
            is_direct: false,
            deps_start: 0,
            deps_len: 0,
        },
    ];
    const existing_keys: [2][]const u8 = ["@org+app@1.0.0", "old-pkg@0.9.0"];

    let ops: [16]IsoInstOp = [IsoInstOp.empty(); 16];
    let arena: [2048]u8 = [0; 2048];
    const sum = planIsolatedMaterialization(
        pkgs[0..],
        dep_links[0..],
        existing_keys[0..],
        IsoLinkBackend.Reflink,
        ops[0..],
        arena[0..],
    ) catch unreachable;

    assertEq(sum.ensure_dir_ops, @as(u32, 2));
    assertEq(sum.materialize_ops, @as(u32, 2));
    assertEq(sum.symlink_ops, @as(u32, 2)); // 1 direct root link + 1 inter-pkg link
    assertEq(sum.prune_ops, @as(u32, 1));   // old-pkg@0.9.0 pruned
    assertEq(sum.op_count, @as(usize, 7));

    // Verify scoped direct root symlink uses "../.cap/..."
    assert(ops[4].kind == IsoInstOpKind.CreateDepSymlink);
    assert(strEql(getIsoOpDst(ops[4], arena[0..]), "cap_modules/@org/app"));
    assert(strEql(getIsoOpSrc(ops[4], arena[0..]), "../.cap/@org+app@1.0.0/node_modules/@org/app"));

    // Verify inter-package relative symlink
    assert(ops[5].kind == IsoInstOpKind.CreateDepSymlink);
    assert(strEql(getIsoOpDst(ops[5], arena[0..]), "cap_modules/.cap/@org+app@1.0.0/node_modules/zod"));
    assert(strEql(getIsoOpSrc(ops[5], arena[0..]), "../../zod@3.23.8/node_modules/zod"));

    // Verify stale store prune
    assert(ops[6].kind == IsoInstOpKind.PruneStaleStoreEntry);
    assert(strEql(getIsoOpDst(ops[6], arena[0..]), "cap_modules/.cap/old-pkg@0.9.0"));
});

test("planIsolatedMaterialization rejects path traversal in package names, versions, aliases, and stale keys", () => {
    assert(isSafeIsoInstPath("@scope/pkg"));
    assert(!isSafeIsoInstPath("../etc/passwd"));
    assert(!isSafeIsoInstPath("/abs/path"));
    assert(!isSafeIsoInstPath("a\\b"));
    assert(isSafeIsoBinName("tsc"));
    assert(!isSafeIsoBinName("../evil"));
    assert(!isSafeIsoBinName("sub/cmd"));

    const bad_pkgs: [1]IsoInstStorePackage = [
        IsoInstStorePackage{ name: "../escape", version: "1.0.0", cache_source_dir: "/c" },
    ];
    const no_deps: [0]IsoInstDepLink = [];
    const no_stale: [0][]const u8 = [];
    let ops: [4]IsoInstOp = [IsoInstOp.empty(); 4];
    let arena: [256]u8 = [0; 256];

    let blocked = false;
    * = planIsolatedMaterialization(bad_pkgs[0..], no_deps[0..], no_stale[0..], IsoLinkBackend.CopyFile, ops[0..], arena[0..]) catch {
        blocked = true;
    };
    assert(blocked);
});

test("planBinShimLinks arbitrates direct vs transitive conflicts and rejects unsafe bin names", () => {
    const cands: [4]IsoBinCandidate = [
        IsoBinCandidate{ bin_name: "tsc", owner_pkg: "ts-old", target_rel_path: "../.cap/ts-old/bin/tsc", is_direct_dep: false },
        IsoBinCandidate{ bin_name: "tsc", owner_pkg: "typescript", target_rel_path: "../.cap/typescript/bin/tsc", is_direct_dep: true },
        IsoBinCandidate{ bin_name: "esbuild", owner_pkg: "esbuild", target_rel_path: "../.cap/esbuild/bin/esbuild", is_direct_dep: true },
        IsoBinCandidate{ bin_name: "../bad", owner_pkg: "evil", target_rel_path: "../x", is_direct_dep: true },
    ];

    let planned: [4]IsoBinPlannedLink = [IsoBinPlannedLink.empty(); 4];
    const sum = planBinShimLinks(cands[0..], IsoHostOs.Windows, planned[0..]) catch unreachable;

    assertEq(sum.planned_count, @as(usize, 2));
    assertEq(sum.conflicts_resolved, @as(u32, 1));
    assertEq(sum.rejected_unsafe, @as(u32, 1));

    // Sorted alphabetically: esbuild, tsc
    assert(strEql(planned[0].bin_name, "esbuild"));
    assert(planned[0].format == IsoBinShimFormat.WindowsCmdAndPs1);
    assert(strEql(planned[1].bin_name, "tsc"));
    assert(strEql(planned[1].owner_pkg, "typescript"));
    assert(planned[1].overrode_transitive);
});

test("formatWindowsCmdShim and formatWindowsPs1Shim emit valid cross-platform wrapper scripts", () => {
    let cmd_buf: [256]u8 = [0; 256];
    const clen = formatWindowsCmdShim("../.cap/esbuild@0.24.0/node_modules/esbuild/bin/esbuild", cmd_buf[0..]) catch unreachable;
    assert(strContains(cmd_buf[0..clen], "@ECHO off\r\n"));
    assert(strContains(cmd_buf[0..clen], "\"%~dp0\\..\\.cap\\esbuild@0.24.0\\node_modules\\esbuild\\bin\\esbuild\" %*\r\n"));

    let ps1_buf: [256]u8 = [0; 256];
    const plen = formatWindowsPs1Shim("../.cap/esbuild@0.24.0/node_modules/esbuild/bin/esbuild", ps1_buf[0..]) catch unreachable;
    assert(strContains(ps1_buf[0..plen], "#!/usr/bin/env pwsh\n"));
    assert(strContains(ps1_buf[0..plen], "$basedir/../.cap/esbuild@0.24.0/node_modules/esbuild/bin/esbuild"));
});

test("formatStagingStorePaths and formatIsoInstallerReport emit atomic staging paths and telemetry", () => {
    let pbuf: [256]u8 = [0; 256];
    const pair = formatStagingStorePaths("@cap/core", "2.1.0", 98765, pbuf[0..]) catch unreachable;
    assert(strEql(pbuf[pair.staging_start..pair.staging_start + pair.staging_len], "cap_modules/.cap/@cap+core@2.1.0.tmp-98765"));
    assert(strEql(pbuf[pair.final_start..pair.final_start + pair.final_len], "cap_modules/.cap/@cap+core@2.1.0"));

    let fb = IsoBackendFallbackState.init(IsoHostOs.MacOS, null);
    * = fb.recordFailure(IsoFsErrorCode.Exdev);
    let rep: [512]u8 = [0; 512];
    const rlen = formatIsoInstallerReport(
        planIsolatedMaterialization(&[], &[], &[], fb.active_backend, &[], pbuf[0..]) catch unreachable,
        fb,
        planBinShimLinks(&[], IsoHostOs.MacOS, &[]) catch unreachable,
        rep[0..],
    ) catch unreachable;
    assert(strContains(rep[0..rlen], "backend: active=copyfile downgrades=1 exdev=1"));
});

test("deterministic 500-iteration fuzz over random paths, bin names, and filesystem error downgrades causes 0 panics", () => {
    let seed: u64 = 0xa0761d6478bd642f;
    const path_chars: [10]u8 = ['a', 'b', '/', '.', '\\', '-', '@', '_', '\n', 0];
    const err_codes: [8]IsoFsErrorCode = [
        IsoFsErrorCode.None,
        IsoFsErrorCode.Exdev,
        IsoFsErrorCode.Enotsup,
        IsoFsErrorCode.Emlink,
        IsoFsErrorCode.Eacces,
        IsoFsErrorCode.Eexist,
        IsoFsErrorCode.Enospc,
        IsoFsErrorCode.Eio,
    ];
    const hosts: [3]IsoHostOs = [IsoHostOs.Linux, IsoHostOs.MacOS, IsoHostOs.Windows];

    let pbuf: [48]u8 = [0; 48];
    let iter: u32 = 0;
    let safe_paths: u32 = 0;
    let unsafe_paths: u32 = 0;

    while (iter < 500) {
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        let r = seed *% 2685821657736338717;

        // 1. Fuzz path & bin hygiene
        const plen: usize = @as(usize, @intCast(r % 32));
        let k: usize = 0;
        while (k < plen) {
            r = r *% 6364136223846793005 +% 1442695040888963407;
            pbuf[k] = path_chars[@as(usize, @intCast((r >> 16) % 10))];
            k += 1;
        }
        if (isSafeIsoInstPath(pbuf[0..plen])) {
            safe_paths += 1;
        } else {
            unsafe_paths += 1;
        }
        * = isSafeIsoBinName(pbuf[0..plen]);

        // 2. Fuzz backend fallback state machine: downgrades must be strictly monotonic and <= 2
        let fb = IsoBackendFallbackState.init(hosts[@as(usize, @intCast(iter % 3))], null);
        let step: u32 = 0;
        while (step < 6) {
            r = r *% 6364136223846793005 +% 1442695040888963407;
            const ec = err_codes[@as(usize, @intCast((r >> 8) % 8))];
            const prev_ord = @as(u8, @intFromEnum(fb.active_backend));
            * = fb.recordFailure(ec);
            const next_ord = @as(u8, @intFromEnum(fb.active_backend));
            assert(next_ord >= prev_ord);
            step += 1;
        }
        assert(fb.downgrade_count <= 2);
        iter += 1;
    }

    assert(unsafe_paths > 200);
    assertEq(safe_paths + unsafe_paths, @as(u32, 500));
});
