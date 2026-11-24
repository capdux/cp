// Source: Absorbed from /data/dev/bun/src/install/PackageInstaller.cp | Synced: 2026-11-23T20:22:19Z
// ============================================================================
// lib/pkg/installOrchestrator.cp — Zero-Alloc Install Diff, DAG & Rollback Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven end-to-end package installation
// orchestrator for `cap install` / `cap update`: incremental lockfile snapshot
// diffing (`diffLockfileSnapshots`) with `--frozen-lockfile` and `--force` guards,
// concurrency-throttled multi-stage package installation state machine
// (`InstallOrchestrator`: CacheCheck -> Download -> VerifySRI -> Extract -> Link
// -> Postinstall -> Installed), reverse-order atomic rollback planner
// (`planInstallRollback`), and terminal summary reporter (`formatInstallSummaryReport`).

/// Default concurrent tarball download slots.
pub const ORCH_DEFAULT_MAX_DOWNLOADS: u32 = 8;

/// Default concurrent tarball extraction slots.
pub const ORCH_DEFAULT_MAX_EXTRACTS: u32 = 4;

/// Installation diffing, orchestration, SRI verification, and rollback error set.
pub err InstallOrchestratorError {
    FrozenLockfileMismatch,
    InvalidPackageIndex,
    InvalidStateTransition,
    IntegrityMismatch,
    RequiredPackageFailed,
    TransactionAborted,
    SliceCapacityExceeded,
    BufferTooSmall,
}

/// Lockfile snapshot entry representing an installed or target resolved package.
pub struct LockSnapshotEntry {
    name: []const u8 = "",
    version: []const u8 = "",
    integrity_hash64: u64 = 0,
    install_path: []const u8 = "",
    has_bin: bool = false,
    has_postinstall: bool = false,
    is_optional: bool = false,
}

/// Classification of a single package diff operation between two lockfile snapshots.
pub enum InstallDiffOpKind: u8 {
    Add = 0,
    Remove = 1,
    Upgrade = 2,
    Downgrade = 3,
    ReinstallIntegrityMismatch = 4,
    KeepUnchanged = 5,
}

/// Single package diff operation produced by `diffLockfileSnapshots`.
pub struct InstallDiffOp {
    kind: InstallDiffOpKind = InstallDiffOpKind.KeepUnchanged,
    name: []const u8 = "",
    old_version: []const u8 = "",
    new_version: []const u8 = "",
    install_path: []const u8 = "",
    new_integrity_hash64: u64 = 0,
    has_bin: bool = false,
    has_postinstall: bool = false,
    is_optional: bool = false,

    static fn empty(): InstallDiffOp {
        return InstallDiffOp{};
    }
}

/// Options controlling `diffLockfileSnapshots`.
pub struct InstallDiffOptions {
    frozen_lockfile: bool = false,
    force_reinstall: bool = false,

    static fn defaults(): InstallDiffOptions {
        return InstallDiffOptions{};
    }
}

/// Aggregate counts returned by `diffLockfileSnapshots`.
pub struct InstallDiffSummary {
    total_ops: u32 = 0,
    added: u32 = 0,
    removed: u32 = 0,
    upgraded: u32 = 0,
    downgraded: u32 = 0,
    reinstalled: u32 = 0,
    unchanged: u32 = 0,
    has_mutations: bool = false,
}

/// Fine-grained per-package installation pipeline phase.
pub enum OrchPkgPhase: u8 {
    Pending = 0,
    CacheCheck = 1,
    Downloading = 2,
    VerifyingSri = 3,
    Extracting = 4,
    Linking = 5,
    Postinstall = 6,
    Installed = 7,
    Failed = 8,
    RolledBack = 9,
}

/// Concrete work action dispatched by `InstallOrchestrator.dispatchNextBatch`.
pub enum OrchActionKind: u8 {
    CheckGlobalCache = 0,
    DownloadTarball = 1,
    VerifyIntegrity = 2,
    ExtractTarball = 3,
    LinkPackage = 4,
    RunPostinstall = 5,
    RemovePackageDir = 6,
}

/// Single dispatched pipeline work item.
pub struct OrchDispatchItem {
    pkg_index: u32 = 0,
    action: OrchActionKind = OrchActionKind.CheckGlobalCache,
    name: []const u8 = "",
    install_path: []const u8 = "",

    static fn empty(): OrchDispatchItem {
        return OrchDispatchItem{};
    }
}

/// Per-package orchestrator state record stored in caller-owned slice.
pub struct OrchPkgRecord {
    pkg_index: u32 = 0,
    diff_kind: InstallDiffOpKind = InstallDiffOpKind.Add,
    name: []const u8 = "",
    old_version: []const u8 = "",
    new_version: []const u8 = "",
    install_path: []const u8 = "",
    expected_integrity_hash64: u64 = 0,
    phase: OrchPkgPhase = OrchPkgPhase.Pending,
    in_flight: bool = false,
    cache_hit: bool = false,
    has_bin: bool = false,
    has_postinstall: bool = false,
    is_optional: bool = false,
    wrote_dir: bool = false,
    backed_up_old: bool = false,
    linked_bin: bool = false,

    static fn empty(): OrchPkgRecord {
        return OrchPkgRecord{};
    }
}

/// Per-stage timing metrics for terminal summary reporting.
pub struct OrchTimingMetrics {
    resolve_ms: u64 = 0,
    download_ms: u64 = 0,
    extract_ms: u64 = 0,
    link_ms: u64 = 0,
    postinstall_ms: u64 = 0,

    fn totalMs(): u64 {
        return _.resolve_ms + _.download_ms + _.extract_ms + _.link_ms + _.postinstall_ms;
    }
}

/// Live orchestrator progress counters.
pub struct OrchStats {
    total: u32 = 0,
    installed: u32 = 0,
    cache_hits: u32 = 0,
    downloaded: u32 = 0,
    failed: u32 = 0,
    rolled_back: u32 = 0,
    active_downloads: u32 = 0,
    active_extracts: u32 = 0,
}

/// Atomic rollback action kind generated when a required package fails.
pub enum RollbackActionKind: u8 {
    UnlinkBin = 0,
    RemoveNewDir = 1,
    RestoreBackup = 2,
    CleanupTemp = 3,
}

/// Single rollback step in reverse-execution order.
pub struct RollbackAction {
    pkg_index: u32 = 0,
    kind: RollbackActionKind = RollbackActionKind.RemoveNewDir,
    name: []const u8 = "",
    target_path: []const u8 = "",

    static fn empty(): RollbackAction {
        return RollbackAction{};
    }
}

/// Summary returned by `planInstallRollback`.
pub struct RollbackPlanSummary {
    total_actions: u32 = 0,
    unlink_bins: u32 = 0,
    remove_dirs: u32 = 0,
    restore_backups: u32 = 0,
    cleanup_temps: u32 = 0,
    packages_rolled_back: u32 = 0,
}

// ============================================================================
// Internal Byte, Version Compare & Buffer Helpers (Zero-Alloc)
// ============================================================================

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn parseU32Prefix(s: []const u8, start: usize): [2]usize {
    let val: usize = 0;
    let i = start;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        val = val * 10 + @as(usize, s[i] - '0');
        i += 1;
    }
    return [val, i];
}

/// Lightweight SemVer triple comparator for Upgrade vs Downgrade classification.
fn compareVersionStrings(a: []const u8, b: []const u8): i32 {
    if (bytesEqual(a, b)) return 0;
    let ai: usize = 0;
    let bi: usize = 0;
    let part: usize = 0;
    while (part < 3) {
        const pa = parseU32Prefix(a, ai);
        const pb = parseU32Prefix(b, bi);
        if (pa[0] != pb[0]) {
            return if (pa[0] < pb[0]) -1 else 1;
        }
        ai = if (pa[1] < a.len && a[pa[1]] == '.') pa[1] + 1 else pa[1];
        bi = if (pb[1] < b.len && b[pb[1]] == '.') pb[1] + 1 else pb[1];
        part += 1;
    }
    // Tie-break on prerelease suffix presence (no suffix > suffix)
    const a_has_pre = (ai < a.len && a[ai] == '-');
    const b_has_pre = (bi < b.len && b[bi] == '-');
    if (!a_has_pre && b_has_pre) return 1;
    if (a_has_pre && !b_has_pre) return -1;
    // Lexicographic fallback
    const min_len = if (a.len < b.len) a.len else b.len;
    let k: usize = 0;
    while (k < min_len) {
        if (a[k] != b[k]) return if (a[k] < b[k]) -1 else 1;
        k += 1;
    }
    if (a.len != b.len) return if (a.len < b.len) -1 else 1;
    return 0;
}

fn appendBytes(out_buf: []u8, pos: *usize, data: []const u8)! {
    if (pos.* + data.len > out_buf.len) return InstallOrchestratorError.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendU64Decimal(out_buf: []u8, pos: *usize, value: u64)! {
    let digits: [24]u8 = [0; 24];
    let len: usize = 0;
    let v = value;
    if (v == 0) {
        digits[0] = @as(u8, 48);
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = @intCast(48 + (v % 10));
            v = v / 10;
            len += 1;
        }
    }
    if (pos.* + len > out_buf.len) return InstallOrchestratorError.BufferTooSmall;
    let i: usize = 0;
    while (i < len) {
        out_buf[pos.* + i] = digits[len - 1 - i];
        i += 1;
    }
    pos.* += len;
}

// ============================================================================
// 1. Incremental Lockfile Snapshot Diff Engine (`diffLockfileSnapshots`)
// ============================================================================

fn findSnapshotByPathOrName(list: []const LockSnapshotEntry, name: []const u8, path: []const u8): usize? {
    let i: usize = 0;
    while (i < list.len) {
        if (bytesEqual(list[i].name, name) && bytesEqual(list[i].install_path, path)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Computes an incremental diff between `installed` and `target` lockfile snapshots.
/// Classifies every package as Add, Remove, Upgrade, Downgrade, ReinstallIntegrityMismatch,
/// or KeepUnchanged. Immediately fails with `FrozenLockfileMismatch` if `opts.frozen_lockfile`
/// is enabled and any mutation is required.
pub fn diffLockfileSnapshots(
    installed: []const LockSnapshotEntry,
    target: []const LockSnapshotEntry,
    opts: InstallDiffOptions,
    ops_out: []InstallDiffOp,
): InstallDiffSummary! {
    let sum = InstallDiffSummary{};
    let out_len: usize = 0;

    // Pass 1: Process every target entry (Add / Upgrade / Downgrade / Reinstall / Keep)
    let ti: usize = 0;
    while (ti < target.len) {
        const tgt = target[ti];
        if (out_len >= ops_out.len) return InstallOrchestratorError.SliceCapacityExceeded;

        if (inst_idx := findSnapshotByPathOrName(installed, tgt.name, tgt.install_path)) {
            const cur = installed[inst_idx];
            const cmp = compareVersionStrings(cur.version, tgt.version);
            let kind = InstallDiffOpKind.KeepUnchanged;
            if (cmp < 0) {
                kind = InstallDiffOpKind.Upgrade;
                sum.upgraded += 1;
                sum.has_mutations = true;
            } else if (cmp > 0) {
                kind = InstallDiffOpKind.Downgrade;
                sum.downgraded += 1;
                sum.has_mutations = true;
            } else if (opts.force_reinstall || (cur.integrity_hash64 != tgt.integrity_hash64)) {
                kind = InstallDiffOpKind.ReinstallIntegrityMismatch;
                sum.reinstalled += 1;
                sum.has_mutations = true;
            } else {
                kind = InstallDiffOpKind.KeepUnchanged;
                sum.unchanged += 1;
            }
            ops_out[out_len] = InstallDiffOp{
                kind: kind,
                name: tgt.name,
                old_version: cur.version,
                new_version: tgt.version,
                install_path: tgt.install_path,
                new_integrity_hash64: tgt.integrity_hash64,
                has_bin: tgt.has_bin,
                has_postinstall: tgt.has_postinstall,
                is_optional: tgt.is_optional,
            };
        } else {
            sum.added += 1;
            sum.has_mutations = true;
            ops_out[out_len] = InstallDiffOp{
                kind: InstallDiffOpKind.Add,
                name: tgt.name,
                old_version: "",
                new_version: tgt.version,
                install_path: tgt.install_path,
                new_integrity_hash64: tgt.integrity_hash64,
                has_bin: tgt.has_bin,
                has_postinstall: tgt.has_postinstall,
                is_optional: tgt.is_optional,
            };
        }
        out_len += 1;
        ti += 1;
    }

    // Pass 2: Find installed packages no longer present in target -> Remove
    let ii: usize = 0;
    while (ii < installed.len) {
        const cur = installed[ii];
        if (findSnapshotByPathOrName(target, cur.name, cur.install_path) == null) {
            if (out_len >= ops_out.len) return InstallOrchestratorError.SliceCapacityExceeded;
            sum.removed += 1;
            sum.has_mutations = true;
            ops_out[out_len] = InstallDiffOp{
                kind: InstallDiffOpKind.Remove,
                name: cur.name,
                old_version: cur.version,
                new_version: "",
                install_path: cur.install_path,
                new_integrity_hash64: 0,
                has_bin: cur.has_bin,
                has_postinstall: false,
                is_optional: cur.is_optional,
            };
            out_len += 1;
        }
        ii += 1;
    }

    sum.total_ops = @intCast(out_len);

    if (opts.frozen_lockfile && sum.has_mutations) {
        return InstallOrchestratorError.FrozenLockfileMismatch;
    }
    return sum;
}

// ============================================================================
// 2. Concurrency-Throttled Multi-Stage Installation Orchestrator
// ============================================================================

/// Zero-heap installation DAG orchestrator backed by caller-owned `[]OrchPkgRecord`.
pub struct InstallOrchestrator {
    records: []OrchPkgRecord,
    records_len: usize = 0,
    max_downloads: u32 = ORCH_DEFAULT_MAX_DOWNLOADS,
    max_extracts: u32 = ORCH_DEFAULT_MAX_EXTRACTS,
    aborted: bool = false,
    downloaded_count: u32 = 0,

    static fn init(
        records_backing: []OrchPkgRecord,
        max_downloads: u32,
        max_extracts: u32,
    ): InstallOrchestrator {
        return InstallOrchestrator{
            records: records_backing,
            records_len: 0,
            max_downloads: if (max_downloads == 0) 1 else max_downloads,
            max_extracts: if (max_extracts == 0) 1 else max_extracts,
        };
    }

    /// Loads all mutating diff operations (ignoring `KeepUnchanged`) into the orchestrator.
    fn loadDiffOps(*_, ops: []const InstallDiffOp): u32! {
        let loaded: u32 = 0;
        let i: usize = 0;
        while (i < ops.len) {
            const op = ops[i];
            if (op.kind != InstallDiffOpKind.KeepUnchanged) {
                if (_.records_len >= _.records.len) return InstallOrchestratorError.SliceCapacityExceeded;
                const idx: u32 = @intCast(_.records_len);
                const needs_backup =
                    (op.kind == InstallDiffOpKind.Upgrade) ||
                    (op.kind == InstallDiffOpKind.Downgrade) ||
                    (op.kind == InstallDiffOpKind.ReinstallIntegrityMismatch) ||
                    (op.kind == InstallDiffOpKind.Remove);
                _.records[_.records_len] = OrchPkgRecord{
                    pkg_index: idx,
                    diff_kind: op.kind,
                    name: op.name,
                    old_version: op.old_version,
                    new_version: op.new_version,
                    install_path: op.install_path,
                    expected_integrity_hash64: op.new_integrity_hash64,
                    phase: OrchPkgPhase.Pending,
                    in_flight: false,
                    cache_hit: false,
                    has_bin: op.has_bin,
                    has_postinstall: op.has_postinstall,
                    is_optional: op.is_optional,
                    wrote_dir: false,
                    backed_up_old: needs_backup,
                    linked_bin: false,
                };
                _.records_len += 1;
                loaded += 1;
            }
            i += 1;
        }
        return loaded;
    }

    fn countActivePhase(phase: OrchPkgPhase): u32 {
        let c: u32 = 0;
        let i: usize = 0;
        while (i < _.records_len) {
            if (_.records[i].phase == phase && _.records[i].in_flight) c += 1;
            i += 1;
        }
        return c;
    }

    /// Dispatches the next batch of ready actions respecting `max_downloads` and `max_extracts`.
    /// Returns 0 immediately if the transaction has been aborted.
    fn dispatchNextBatch(*_, out_batch: []OrchDispatchItem): usize {
        if (_.aborted) return 0;
        let active_dl = _.countActivePhase(OrchPkgPhase.Downloading);
        let active_ex = _.countActivePhase(OrchPkgPhase.Extracting);
        let emitted: usize = 0;

        let i: usize = 0;
        while (i < _.records_len && emitted < out_batch.len) {
            let r = _.records[i];
            if (!r.in_flight) {
                if (r.phase == OrchPkgPhase.Pending) {
                    if (r.diff_kind == InstallDiffOpKind.Remove) {
                        r.phase = OrchPkgPhase.Linking;
                        r.in_flight = true;
                        _.records[i] = r;
                        out_batch[emitted] = OrchDispatchItem{
                            pkg_index: r.pkg_index,
                            action: OrchActionKind.RemovePackageDir,
                            name: r.name,
                            install_path: r.install_path,
                        };
                        emitted += 1;
                    } else {
                        r.phase = OrchPkgPhase.CacheCheck;
                        r.in_flight = true;
                        _.records[i] = r;
                        out_batch[emitted] = OrchDispatchItem{
                            pkg_index: r.pkg_index,
                            action: OrchActionKind.CheckGlobalCache,
                            name: r.name,
                            install_path: r.install_path,
                        };
                        emitted += 1;
                    }
                } else if (r.phase == OrchPkgPhase.Downloading) {
                    if (active_dl < _.max_downloads) {
                        r.in_flight = true;
                        _.records[i] = r;
                        active_dl += 1;
                        out_batch[emitted] = OrchDispatchItem{
                            pkg_index: r.pkg_index,
                            action: OrchActionKind.DownloadTarball,
                            name: r.name,
                            install_path: r.install_path,
                        };
                        emitted += 1;
                    }
                } else if (r.phase == OrchPkgPhase.VerifyingSri) {
                    r.in_flight = true;
                    _.records[i] = r;
                    out_batch[emitted] = OrchDispatchItem{
                        pkg_index: r.pkg_index,
                        action: OrchActionKind.VerifyIntegrity,
                        name: r.name,
                        install_path: r.install_path,
                    };
                    emitted += 1;
                } else if (r.phase == OrchPkgPhase.Extracting) {
                    if (active_ex < _.max_extracts) {
                        r.in_flight = true;
                        _.records[i] = r;
                        active_ex += 1;
                        out_batch[emitted] = OrchDispatchItem{
                            pkg_index: r.pkg_index,
                            action: OrchActionKind.ExtractTarball,
                            name: r.name,
                            install_path: r.install_path,
                        };
                        emitted += 1;
                    }
                } else if (r.phase == OrchPkgPhase.Linking) {
                    r.in_flight = true;
                    _.records[i] = r;
                    out_batch[emitted] = OrchDispatchItem{
                        pkg_index: r.pkg_index,
                        action: OrchActionKind.LinkPackage,
                        name: r.name,
                        install_path: r.install_path,
                    };
                    emitted += 1;
                } else if (r.phase == OrchPkgPhase.Postinstall) {
                    r.in_flight = true;
                    _.records[i] = r;
                    out_batch[emitted] = OrchDispatchItem{
                        pkg_index: r.pkg_index,
                        action: OrchActionKind.RunPostinstall,
                        name: r.name,
                        install_path: r.install_path,
                    };
                    emitted += 1;
                }
            }
            i += 1;
        }
        return emitted;
    }

    fn onCacheCheckDone(*_, pkg_index: u32, hit: bool)! {
        const idx: usize = @intCast(pkg_index);
        if (idx >= _.records_len) return InstallOrchestratorError.InvalidPackageIndex;
        let r = _.records[idx];
        if (r.phase != OrchPkgPhase.CacheCheck) return InstallOrchestratorError.InvalidStateTransition;
        r.in_flight = false;
        r.cache_hit = hit;
        // Cache hit skips Downloading, VerifyingSri, and Extracting straight to Linking!
        r.phase = if (hit) OrchPkgPhase.Linking else OrchPkgPhase.Downloading;
        _.records[idx] = r;
    }

    fn onDownloadDone(*_, pkg_index: u32)! {
        const idx: usize = @intCast(pkg_index);
        if (idx >= _.records_len) return InstallOrchestratorError.InvalidPackageIndex;
        let r = _.records[idx];
        if (r.phase != OrchPkgPhase.Downloading) return InstallOrchestratorError.InvalidStateTransition;
        r.in_flight = false;
        r.phase = OrchPkgPhase.VerifyingSri;
        _.downloaded_count += 1;
        _.records[idx] = r;
    }

    fn onSriVerified(*_, pkg_index: u32, actual_integrity_hash64: u64)! {
        const idx: usize = @intCast(pkg_index);
        if (idx >= _.records_len) return InstallOrchestratorError.InvalidPackageIndex;
        let r = _.records[idx];
        if (r.phase != OrchPkgPhase.VerifyingSri) return InstallOrchestratorError.InvalidStateTransition;
        r.in_flight = false;
        if (r.expected_integrity_hash64 != 0 && actual_integrity_hash64 != r.expected_integrity_hash64) {
            r.phase = OrchPkgPhase.Failed;
            _.records[idx] = r;
            if (!r.is_optional) _.aborted = true;
            return InstallOrchestratorError.IntegrityMismatch;
        }
        r.phase = OrchPkgPhase.Extracting;
        _.records[idx] = r;
    }

    fn onExtractDone(*_, pkg_index: u32)! {
        const idx: usize = @intCast(pkg_index);
        if (idx >= _.records_len) return InstallOrchestratorError.InvalidPackageIndex;
        let r = _.records[idx];
        if (r.phase != OrchPkgPhase.Extracting) return InstallOrchestratorError.InvalidStateTransition;
        r.in_flight = false;
        r.wrote_dir = true;
        r.phase = OrchPkgPhase.Linking;
        _.records[idx] = r;
    }

    fn onLinkDone(*_, pkg_index: u32, linked_bin: bool)! {
        const idx: usize = @intCast(pkg_index);
        if (idx >= _.records_len) return InstallOrchestratorError.InvalidPackageIndex;
        let r = _.records[idx];
        if (r.phase != OrchPkgPhase.Linking) return InstallOrchestratorError.InvalidStateTransition;
        r.in_flight = false;
        if (r.diff_kind != InstallDiffOpKind.Remove) {
            r.wrote_dir = true;
            r.linked_bin = linked_bin;
        }
        r.phase = if (r.has_postinstall && r.diff_kind != InstallDiffOpKind.Remove) OrchPkgPhase.Postinstall else OrchPkgPhase.Installed;
        _.records[idx] = r;
    }

    fn onPostinstallDone(*_, pkg_index: u32)! {
        const idx: usize = @intCast(pkg_index);
        if (idx >= _.records_len) return InstallOrchestratorError.InvalidPackageIndex;
        let r = _.records[idx];
        if (r.phase != OrchPkgPhase.Postinstall) return InstallOrchestratorError.InvalidStateTransition;
        r.in_flight = false;
        r.phase = OrchPkgPhase.Installed;
        _.records[idx] = r;
    }

    fn onPackageFailed(*_, pkg_index: u32)! {
        const idx: usize = @intCast(pkg_index);
        if (idx >= _.records_len) return InstallOrchestratorError.InvalidPackageIndex;
        let r = _.records[idx];
        r.in_flight = false;
        r.phase = OrchPkgPhase.Failed;
        _.records[idx] = r;
        if (!r.is_optional) {
            _.aborted = true;
            return InstallOrchestratorError.RequiredPackageFailed;
        }
    }

    fn isAborted(): bool {
        return _.aborted;
    }

    fn isComplete(): bool {
        if (_.aborted) return false;
        let i: usize = 0;
        while (i < _.records_len) {
            const p = _.records[i].phase;
            if (p != OrchPkgPhase.Installed && !(p == OrchPkgPhase.Failed && _.records[i].is_optional)) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn stats(): OrchStats {
        let s = OrchStats{
            total: @intCast(_.records_len),
            downloaded: _.downloaded_count,
        };
        let i: usize = 0;
        while (i < _.records_len) {
            const r = _.records[i];
            if (r.cache_hit) s.cache_hits += 1;
            if (r.phase == OrchPkgPhase.Installed) s.installed += 1;
            else if (r.phase == OrchPkgPhase.Failed) s.failed += 1;
            else if (r.phase == OrchPkgPhase.RolledBack) s.rolled_back += 1;
            else if (r.phase == OrchPkgPhase.Downloading && r.in_flight) s.active_downloads += 1;
            else if (r.phase == OrchPkgPhase.Extracting && r.in_flight) s.active_extracts += 1;
            i += 1;
        }
        return s;
    }
}

// ============================================================================
// 3. Reverse-Order Atomic Rollback Planner (`planInstallRollback`)
// ============================================================================

fn pushRollback(
    actions_out: []RollbackAction,
    count: *usize,
    pkg_index: u32,
    kind: RollbackActionKind,
    name: []const u8,
    target_path: []const u8,
)! {
    if (count.* >= actions_out.len) return InstallOrchestratorError.SliceCapacityExceeded;
    actions_out[count.*] = RollbackAction{
        pkg_index: pkg_index,
        kind: kind,
        name: name,
        target_path: target_path,
    };
    count.* += 1;
}

/// Generates a deterministic reverse-order rollback action list for all packages
/// that have written files, linked bins, or created temp artifacts prior to failure.
/// Transitions rolled-back records to `OrchPkgPhase.RolledBack`.
pub fn planInstallRollback(
    orch: *InstallOrchestrator,
    actions_out: []RollbackAction,
): RollbackPlanSummary! {
    let sum = RollbackPlanSummary{};
    let act_len: usize = 0;

    // Reverse scan to undo newest mutations first
    let i = orch.records_len;
    while (i > 0) {
        i -= 1;
        let r = orch.records[i];
        let touched = false;

        if (r.linked_bin) {
            pushRollback(actions_out, &act_len, r.pkg_index, RollbackActionKind.UnlinkBin, r.name, r.install_path)?;
            sum.unlink_bins += 1;
            touched = true;
        }
        if (r.wrote_dir) {
            pushRollback(actions_out, &act_len, r.pkg_index, RollbackActionKind.RemoveNewDir, r.name, r.install_path)?;
            sum.remove_dirs += 1;
            touched = true;
        }
        if (r.backed_up_old && (r.wrote_dir || r.diff_kind == InstallDiffOpKind.Remove)) {
            pushRollback(actions_out, &act_len, r.pkg_index, RollbackActionKind.RestoreBackup, r.name, r.install_path)?;
            sum.restore_backups += 1;
            touched = true;
        }
        if (r.phase == OrchPkgPhase.Downloading || r.phase == OrchPkgPhase.VerifyingSri || r.phase == OrchPkgPhase.Extracting) {
            pushRollback(actions_out, &act_len, r.pkg_index, RollbackActionKind.CleanupTemp, r.name, r.install_path)?;
            sum.cleanup_temps += 1;
            touched = true;
        }

        if (touched) {
            r.in_flight = false;
            r.phase = OrchPkgPhase.RolledBack;
            orch.records[i] = r;
            sum.packages_rolled_back += 1;
        }
    }

    sum.total_actions = @intCast(act_len);
    return sum;
}

// ============================================================================
// 4. Terminal Install Summary Reporter (`formatInstallSummaryReport`)
// ============================================================================

/// Formats a concise terminal install summary with diff breakdown, cache hits, and timing.
pub fn formatInstallSummaryReport(
    diff_sum: InstallDiffSummary,
    orch_stats: OrchStats,
    timing: OrchTimingMetrics,
    out_buf: []u8,
): usize! {
    let pos: usize = 0;
    appendBytes(out_buf, &pos, "install +")?;
    appendU64Decimal(out_buf, &pos, @as(u64, diff_sum.added))?;
    appendBytes(out_buf, &pos, " ^")?;
    appendU64Decimal(out_buf, &pos, @as(u64, diff_sum.upgraded))?;
    appendBytes(out_buf, &pos, " v")?;
    appendU64Decimal(out_buf, &pos, @as(u64, diff_sum.downgraded))?;
    appendBytes(out_buf, &pos, " -")?;
    appendU64Decimal(out_buf, &pos, @as(u64, diff_sum.removed))?;
    appendBytes(out_buf, &pos, " cached=")?;
    appendU64Decimal(out_buf, &pos, @as(u64, orch_stats.cache_hits))?;
    appendBytes(out_buf, &pos, " dl=")?;
    appendU64Decimal(out_buf, &pos, @as(u64, orch_stats.downloaded))?;
    appendBytes(out_buf, &pos, " total=")?;
    appendU64Decimal(out_buf, &pos, timing.totalMs())?;
    appendBytes(out_buf, &pos, "ms")?;
    return pos;
}
