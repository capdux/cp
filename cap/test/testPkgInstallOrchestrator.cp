import { test, assert, assertEq } from "cap:test";
import {
    InstallOrchestratorError,
    LockSnapshotEntry,
    InstallDiffOpKind,
    InstallDiffOp,
    InstallDiffOptions,
    OrchPkgPhase,
    OrchActionKind,
    OrchDispatchItem,
    OrchPkgRecord,
    OrchTimingMetrics,
    RollbackActionKind,
    RollbackAction,
    diffLockfileSnapshots,
    InstallOrchestrator,
    planInstallRollback,
    formatInstallSummaryReport,
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

test("diffLockfileSnapshots classifies Add, Remove, Upgrade, Downgrade, Reinstall, and KeepUnchanged", () => {
    const installed: [5]LockSnapshotEntry = [
        LockSnapshotEntry{ name: "keep", version: "1.0.0", integrity_hash64: 0x11, install_path: "cap_modules/keep" },
        LockSnapshotEntry{ name: "up", version: "1.2.0", integrity_hash64: 0x22, install_path: "cap_modules/up" },
        LockSnapshotEntry{ name: "down", version: "3.0.0", integrity_hash64: 0x33, install_path: "cap_modules/down" },
        LockSnapshotEntry{ name: "corrupt", version: "2.0.0", integrity_hash64: 0x44, install_path: "cap_modules/corrupt" },
        LockSnapshotEntry{ name: "old", version: "0.9.0", integrity_hash64: 0x55, install_path: "cap_modules/old" },
    ];
    const target: [5]LockSnapshotEntry = [
        LockSnapshotEntry{ name: "keep", version: "1.0.0", integrity_hash64: 0x11, install_path: "cap_modules/keep" },
        LockSnapshotEntry{ name: "up", version: "1.5.0", integrity_hash64: 0x29, install_path: "cap_modules/up" },
        LockSnapshotEntry{ name: "down", version: "2.4.0", integrity_hash64: 0x39, install_path: "cap_modules/down" },
        LockSnapshotEntry{ name: "corrupt", version: "2.0.0", integrity_hash64: 0x49, install_path: "cap_modules/corrupt" },
        LockSnapshotEntry{ name: "added", version: "4.0.0", integrity_hash64: 0x66, install_path: "cap_modules/added" },
    ];

    let ops: [8]InstallDiffOp = [InstallDiffOp.empty(); 8];
    const sum = diffLockfileSnapshots(installed[0..], target[0..], InstallDiffOptions.defaults(), ops[0..]) catch unreachable;

    assertEq(sum.total_ops, @as(u32, 6));
    assertEq(sum.unchanged, @as(u32, 1));
    assertEq(sum.upgraded, @as(u32, 1));
    assertEq(sum.downgraded, @as(u32, 1));
    assertEq(sum.reinstalled, @as(u32, 1));
    assertEq(sum.added, @as(u32, 1));
    assertEq(sum.removed, @as(u32, 1));
    assert(sum.has_mutations);

    assert(ops[0].kind == InstallDiffOpKind.KeepUnchanged);
    assert(ops[1].kind == InstallDiffOpKind.Upgrade);
    assert(ops[2].kind == InstallDiffOpKind.Downgrade);
    assert(ops[3].kind == InstallDiffOpKind.ReinstallIntegrityMismatch);
    assert(ops[4].kind == InstallDiffOpKind.Add);
    assert(ops[5].kind == InstallDiffOpKind.Remove);
});

test("Frozen lockfile blocks mutations while allowing clean no-op snapshots, and --force triggers reinstall", () => {
    const snap: [1]LockSnapshotEntry = [
        LockSnapshotEntry{ name: "zod", version: "3.23.8", integrity_hash64: 0xAA, install_path: "cap_modules/zod" },
    ];
    const changed: [1]LockSnapshotEntry = [
        LockSnapshotEntry{ name: "zod", version: "3.24.0", integrity_hash64: 0xBB, install_path: "cap_modules/zod" },
    ];
    let ops: [4]InstallDiffOp = [InstallDiffOp.empty(); 4];

    // Identical snapshot passes --frozen-lockfile
    const ok_sum = diffLockfileSnapshots(snap[0..], snap[0..], InstallDiffOptions{ frozen_lockfile: true }, ops[0..]) catch unreachable;
    assert(!ok_sum.has_mutations);

    // Mutated snapshot blocked by --frozen-lockfile
    let frozen_err = false;
    * = diffLockfileSnapshots(snap[0..], changed[0..], InstallDiffOptions{ frozen_lockfile: true }, ops[0..]) catch |e| {
        frozen_err = (e == InstallOrchestratorError.FrozenLockfileMismatch);
    };
    assert(frozen_err);

    // --force forces reinstall even when version & integrity match
    const force_sum = diffLockfileSnapshots(snap[0..], snap[0..], InstallDiffOptions{ force_reinstall: true }, ops[0..]) catch unreachable;
    assertEq(force_sum.reinstalled, @as(u32, 1));
});

test("InstallOrchestrator fast-paths cache hits and runs full 6-stage pipeline for cache misses", () => {
    const ops: [2]InstallDiffOp = [
        InstallDiffOp{ kind: InstallDiffOpKind.Add, name: "cached-pkg", new_version: "1.0.0", install_path: "cap_modules/cached-pkg", new_integrity_hash64: 0x111, has_bin: true, has_postinstall: false },
        InstallDiffOp{ kind: InstallDiffOpKind.Add, name: "fresh-pkg", new_version: "2.0.0", install_path: "cap_modules/fresh-pkg", new_integrity_hash64: 0x222, has_bin: false, has_postinstall: true },
    ];
    let recs: [4]OrchPkgRecord = [OrchPkgRecord.empty(); 4];
    let orch = InstallOrchestrator.init(recs[0..], 4, 2);
    assertEq(orch.loadDiffOps(ops[0..]) catch unreachable, @as(u32, 2));

    let batch: [4]OrchDispatchItem = [OrchDispatchItem.empty(); 4];
    // Batch 1: CacheCheck for both
    assertEq(orch.dispatchNextBatch(batch[0..]), @as(usize, 2));
    assert(batch[0].action == OrchActionKind.CheckGlobalCache);
    assert(batch[1].action == OrchActionKind.CheckGlobalCache);

    // pkg 0 hits cache -> straight to Linking; pkg 1 misses -> Downloading
    orch.onCacheCheckDone(0, true) catch unreachable;
    orch.onCacheCheckDone(1, false) catch unreachable;

    assertEq(orch.dispatchNextBatch(batch[0..]), @as(usize, 2));
    assert(batch[0].action == OrchActionKind.LinkPackage);
    assert(batch[1].action == OrchActionKind.DownloadTarball);

    orch.onLinkDone(0, true) catch unreachable;
    assert(recs[0].phase == OrchPkgPhase.Installed);

    // pkg 1 continues Download -> VerifySRI -> Extract -> Link -> Postinstall -> Installed
    orch.onDownloadDone(1) catch unreachable;
    assertEq(orch.dispatchNextBatch(batch[0..]), @as(usize, 1));
    assert(batch[0].action == OrchActionKind.VerifyIntegrity);

    orch.onSriVerified(1, 0x222) catch unreachable;
    assertEq(orch.dispatchNextBatch(batch[0..]), @as(usize, 1));
    assert(batch[0].action == OrchActionKind.ExtractTarball);

    orch.onExtractDone(1) catch unreachable;
    assertEq(orch.dispatchNextBatch(batch[0..]), @as(usize, 1));
    assert(batch[0].action == OrchActionKind.LinkPackage);

    orch.onLinkDone(1, false) catch unreachable;
    assertEq(orch.dispatchNextBatch(batch[0..]), @as(usize, 1));
    assert(batch[0].action == OrchActionKind.RunPostinstall);

    orch.onPostinstallDone(1) catch unreachable;
    assert(orch.isComplete());

    const st = orch.stats();
    assertEq(st.installed, @as(u32, 2));
    assertEq(st.cache_hits, @as(u32, 1));
    assertEq(st.downloaded, @as(u32, 1));
});

test("Download concurrency throttling respects max_downloads limit", () => {
    const ops: [3]InstallDiffOp = [
        InstallDiffOp{ kind: InstallDiffOpKind.Add, name: "a", install_path: "cap_modules/a" },
        InstallDiffOp{ kind: InstallDiffOpKind.Add, name: "b", install_path: "cap_modules/b" },
        InstallDiffOp{ kind: InstallDiffOpKind.Add, name: "c", install_path: "cap_modules/c" },
    ];
    let recs: [4]OrchPkgRecord = [OrchPkgRecord.empty(); 4];
    let orch = InstallOrchestrator.init(recs[0..], 2, 1);
    * = orch.loadDiffOps(ops[0..]) catch unreachable;

    let batch: [4]OrchDispatchItem = [OrchDispatchItem.empty(); 4];
    * = orch.dispatchNextBatch(batch[0..]);
    orch.onCacheCheckDone(0, false) catch unreachable;
    orch.onCacheCheckDone(1, false) catch unreachable;
    orch.onCacheCheckDone(2, false) catch unreachable;

    // Only 2 downloads dispatched despite 3 waiting
    const n_dl = orch.dispatchNextBatch(batch[0..]);
    assertEq(n_dl, @as(usize, 2));
    assertEq(orch.stats().active_downloads, @as(u32, 2));
});

test("SRI integrity mismatch aborts transaction and blocks further dispatch", () => {
    const ops: [1]InstallDiffOp = [
        InstallDiffOp{ kind: InstallDiffOpKind.Add, name: "tampered", new_integrity_hash64: 0xDEAD, install_path: "cap_modules/tampered" },
    ];
    let recs: [2]OrchPkgRecord = [OrchPkgRecord.empty(); 2];
    let orch = InstallOrchestrator.init(recs[0..], 4, 2);
    * = orch.loadDiffOps(ops[0..]) catch unreachable;

    let batch: [2]OrchDispatchItem = [OrchDispatchItem.empty(); 2];
    * = orch.dispatchNextBatch(batch[0..]);
    orch.onCacheCheckDone(0, false) catch unreachable;
    * = orch.dispatchNextBatch(batch[0..]);
    orch.onDownloadDone(0) catch unreachable;
    * = orch.dispatchNextBatch(batch[0..]);

    let sri_err = false;
    orch.onSriVerified(0, 0xBEEF) catch |e| {
        sri_err = (e == InstallOrchestratorError.IntegrityMismatch);
    };
    assert(sri_err);
    assert(orch.isAborted());
    assertEq(orch.dispatchNextBatch(batch[0..]), @as(usize, 0));
});

test("Reverse-order atomic rollback restores backups, unlinks bins, and cleans temp artifacts", () => {
    const ops: [2]InstallDiffOp = [
        InstallDiffOp{ kind: InstallDiffOpKind.Upgrade, name: "pkgA", old_version: "1.0.0", new_version: "2.0.0", install_path: "cap_modules/pkgA", has_bin: true },
        InstallDiffOp{ kind: InstallDiffOpKind.Add, name: "pkgB", new_version: "1.0.0", install_path: "cap_modules/pkgB" },
    ];
    let recs: [4]OrchPkgRecord = [OrchPkgRecord.empty(); 4];
    let orch = InstallOrchestrator.init(recs[0..], 4, 2);
    * = orch.loadDiffOps(ops[0..]) catch unreachable;

    let batch: [4]OrchDispatchItem = [OrchDispatchItem.empty(); 4];
    * = orch.dispatchNextBatch(batch[0..]);
    orch.onCacheCheckDone(0, true) catch unreachable;
    orch.onCacheCheckDone(1, false) catch unreachable;
    * = orch.dispatchNextBatch(batch[0..]);
    // pkgA finishes linking with bin; pkgB is mid-download when failure occurs
    orch.onLinkDone(0, true) catch unreachable;

    let actions: [8]RollbackAction = [RollbackAction.empty(); 8];
    const rb = planInstallRollback(&orch, actions[0..]) catch unreachable;

    assertEq(rb.packages_rolled_back, @as(u32, 2));
    assertEq(rb.cleanup_temps, @as(u32, 1)); // pkgB temp download
    assertEq(rb.unlink_bins, @as(u32, 1)); // pkgA bin
    assertEq(rb.remove_dirs, @as(u32, 1)); // pkgA new dir
    assertEq(rb.restore_backups, @as(u32, 1)); // pkgA 1.0.0 backup
    assertEq(rb.total_actions, @as(u32, 4));

    // Verify reverse order: pkgB (index 1) cleaned up before pkgA (index 0)
    assertEq(actions[0].pkg_index, @as(u32, 1));
    assert(actions[0].kind == RollbackActionKind.CleanupTemp);
    assertEq(actions[1].pkg_index, @as(u32, 0));
    assert(actions[1].kind == RollbackActionKind.UnlinkBin);
    assert(actions[2].kind == RollbackActionKind.RemoveNewDir);
    assert(actions[3].kind == RollbackActionKind.RestoreBackup);
    assert(recs[0].phase == OrchPkgPhase.RolledBack);
    assert(recs[1].phase == OrchPkgPhase.RolledBack);
});

test("Optional package failure does not abort install and terminal summary formats cleanly", () => {
    const installed: [0]LockSnapshotEntry = [];
    const target: [2]LockSnapshotEntry = [
        LockSnapshotEntry{ name: "core", version: "1.0.0", install_path: "cap_modules/core", is_optional: false },
        LockSnapshotEntry{ name: "opt", version: "1.0.0", install_path: "cap_modules/opt", is_optional: true },
    ];
    let ops: [4]InstallDiffOp = [InstallDiffOp.empty(); 4];
    const dsum = diffLockfileSnapshots(installed[0..], target[0..], InstallDiffOptions.defaults(), ops[0..]) catch unreachable;

    let recs: [4]OrchPkgRecord = [OrchPkgRecord.empty(); 4];
    let orch = InstallOrchestrator.init(recs[0..], 4, 2);
    * = orch.loadDiffOps(ops[0..@as(usize, dsum.total_ops)]) catch unreachable;

    let batch: [4]OrchDispatchItem = [OrchDispatchItem.empty(); 4];
    * = orch.dispatchNextBatch(batch[0..]);
    orch.onCacheCheckDone(0, true) catch unreachable;
    orch.onPackageFailed(1) catch unreachable; // optional failure does NOT abort!
    assert(!orch.isAborted());

    * = orch.dispatchNextBatch(batch[0..]);
    orch.onLinkDone(0, false) catch unreachable;
    assert(orch.isComplete());

    const timing = OrchTimingMetrics{ resolve_ms: 12, download_ms: 0, extract_ms: 0, link_ms: 8, postinstall_ms: 5 };
    let rep_buf: [256]u8 = [0; 256];
    const rlen = formatInstallSummaryReport(dsum, orch.stats(), timing, rep_buf[0..]) catch unreachable;
    assert(strEql(rep_buf[0..rlen], "install +2 ^0 v0 -0 cached=1 dl=0 total=25ms"));
});
