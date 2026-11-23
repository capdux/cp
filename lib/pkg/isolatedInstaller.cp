// Source: Absorbed from /data/dev/bun/src/install/isolated_install/Installer.cp | Synced: 2026-11-22T19:02:19Z
// ============================================================================
// lib/pkg/isolatedInstaller.cp — Isolated Store Materializer & Bin Shim Coordinator
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven isolated install materialization engine
// for `cap install`:
//   1. Copy-on-write / link backend & cross-filesystem fallback state machine
//      (`IsoLinkBackend`, `IsoBackendFallbackState`): `Clonefile` (macOS APFS),
//      `Reflink` (Linux Btrfs/XFS), `Hardlink`, and `CopyFile` with deterministic
//      downgrade on `EXDEV`/`ENOTSUP`/`EMLINK` and downgrade telemetry;
//   2. Complete isolated store directory tree planner (`planIsolatedMaterialization`)
//      emitting deterministically ordered atomic operations (`EnsureStoreDir` ->
//      `MaterializeFiles` -> `CreateDepSymlink` -> `PruneStaleStoreEntry`) with
//      `cap:simd`-assisted path traversal rejection and relative symlinks under
//      `cap_modules/.cap/<pkg>@<ver>/node_modules/`;
//   3. `.bin` conflict arbiter & cross-platform executable shim planner
//      (`planBinShimLinks`, `formatWindowsCmdShim`, `formatWindowsPs1Shim`)
//      preferring direct dependencies over transitive ones.

import { Simd } from "cap:simd";

/// Canonical isolated package store root directory inside a project.
pub const ISO_INST_STORE_ROOT: []const u8 = "cap_modules/.cap";

/// Canonical executable bin shim directory inside a project.
pub const ISO_INST_BIN_DIR: []const u8 = "cap_modules/.bin";

/// Maximum permitted path length for generated isolated store paths.
pub const ISO_INST_MAX_PATH_LEN: usize = 512;

/// Error set for isolated install planning, backend fallback, and bin shim generation.
pub err IsoInstallerError {
    UnsafePathTraversal,
    InvalidPackageName,
    InvalidBinName,
    SliceCapacityExceeded,
    BufferTooSmall,
    InvalidTargetIndex,
}

// ============================================================================
// Internal Zero-Alloc Byte, Path Hygiene, and Formatting Helpers
// ============================================================================

fn iiBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn iiBytesLessThan(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn iiAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn iiAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

/// Validates that `rel_path` is strictly relative, free of NUL/control bytes,
/// backslashes, and `..` directory traversal segments (accelerated with `cap:simd`).
pub fn isSafeIsoInstPath(rel_path: []const u8): bool {
    if (rel_path.len == 0 || rel_path.len > ISO_INST_MAX_PATH_LEN) return false;
    if (rel_path[0] == '/' || rel_path[0] == '\\') return false;
    // Reject NUL, CR, LF, backslash via SIMD 4-byte scan
    if (Simd.indexOfAny4(rel_path, 0, '\r', '\n', '\\') != null) return false;

    let start: usize = 0;
    while (start <= rel_path.len) {
        let end = rel_path.len;
        if (slash_rel := Simd.indexOfByte(rel_path[start..], '/')) {
            end = start + slash_rel;
        }
        const seg = rel_path[start..end];
        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') return false;
        if (end >= rel_path.len) break;
        start = end + 1;
    }
    return true;
}

/// Validates a single executable `.bin` command name (no path separators, no dots-only).
pub fn isSafeIsoBinName(bin_name: []const u8): bool {
    if (bin_name.len == 0 || bin_name.len > 128) return false;
    if (Simd.indexOfAny4(bin_name, '/', '\\', 0, '\n') != null) return false;
    if (Simd.indexOfAny4(bin_name, '\r', ':', '"', '\'') != null) return false;
    if (iiBytesEql(bin_name, ".") || iiBytesEql(bin_name, "..")) return false;
    return true;
}

fn iiAppendEscapedStoreKey(out: []u8, pos: *usize, pkg_name: []const u8, version: []const u8)! {
    let i: usize = 0;
    while (i < pkg_name.len) {
        const c = pkg_name[i];
        if (c == '/') {
            iiAppendByte(out, pos, '+')?;
        } else {
            iiAppendByte(out, pos, c)?;
        }
        i += 1;
    }
    iiAppendByte(out, pos, '@')?;
    iiAppendBytes(out, pos, version)?;
}

// ============================================================================
// Section 1: Materialization Backend & Cross-Filesystem Fallback State Machine
// ============================================================================

/// File materialization strategy ordered from fastest CoW to universal copy.
pub enum IsoLinkBackend: u8 {
    Clonefile = 0,
    Reflink = 1,
    Hardlink = 2,
    CopyFile = 3,
}

/// Returns the canonical lowercase name for `backend`.
pub fn isoLinkBackendName(backend: IsoLinkBackend): []const u8 {
    if (backend == IsoLinkBackend.Clonefile) return "clonefile";
    if (backend == IsoLinkBackend.Reflink) return "reflink";
    if (backend == IsoLinkBackend.Hardlink) return "hardlink";
    return "copyfile";
}

/// Target host OS for backend selection and bin shim formatting.
pub enum IsoHostOs: u8 {
    Linux = 0,
    MacOS = 1,
    Windows = 2,
}

/// Categorized OS filesystem error encountered during materialization.
pub enum IsoFsErrorCode: u8 {
    None = 0,
    Exdev = 1,
    Enotsup = 2,
    Emlink = 3,
    Eacces = 4,
    Eexist = 5,
    Enospc = 6,
    Eio = 7,
}

/// Decision produced by `IsoBackendFallbackState.recordFailure`.
pub struct IsoFallbackDecision {
    should_retry: bool = false,
    next_backend: IsoLinkBackend = IsoLinkBackend.CopyFile,
    permanent_error: bool = false,
    idempotent_skip: bool = false,
}

/// Deterministic cross-filesystem downgrade state machine tracking active
/// backend and fallback telemetry (`EXDEV`, `ENOTSUP`, `EMLINK`).
pub struct IsoBackendFallbackState {
    host_os: IsoHostOs = IsoHostOs.Linux,
    active_backend: IsoLinkBackend = IsoLinkBackend.Reflink,
    downgrade_count: u32 = 0,
    exdev_hits: u32 = 0,
    enotsup_hits: u32 = 0,
    emlink_hits: u32 = 0,

    static fn init(host_os: IsoHostOs, preferred: IsoLinkBackend?): IsoBackendFallbackState {
        let initial = IsoLinkBackend.Reflink;
        if (pref := preferred) {
            initial = pref;
        } else if (host_os == IsoHostOs.MacOS) {
            initial = IsoLinkBackend.Clonefile;
        } else if (host_os == IsoHostOs.Windows) {
            initial = IsoLinkBackend.Hardlink;
        } else {
            initial = IsoLinkBackend.Reflink;
        }
        return IsoBackendFallbackState{
            host_os: host_os,
            active_backend: initial,
        };
    }

    /// Evaluates a filesystem error code and transitions `active_backend` to the
    /// next deterministic fallback when recoverable (`EXDEV`/`ENOTSUP`/`EMLINK`).
    fn recordFailure(*_, code: IsoFsErrorCode): IsoFallbackDecision {
        if (code == IsoFsErrorCode.None) {
            return IsoFallbackDecision{
                should_retry: false,
                next_backend: _.active_backend,
                permanent_error: false,
                idempotent_skip: false,
            };
        }
        if (code == IsoFsErrorCode.Eexist) {
            return IsoFallbackDecision{
                should_retry: false,
                next_backend: _.active_backend,
                permanent_error: false,
                idempotent_skip: true,
            };
        }
        if (code == IsoFsErrorCode.Eacces || code == IsoFsErrorCode.Enospc || code == IsoFsErrorCode.Eio) {
            return IsoFallbackDecision{
                should_retry: false,
                next_backend: _.active_backend,
                permanent_error: true,
                idempotent_skip: false,
            };
        }

        if (code == IsoFsErrorCode.Exdev) _.exdev_hits += 1;
        if (code == IsoFsErrorCode.Enotsup) _.enotsup_hits += 1;
        if (code == IsoFsErrorCode.Emlink) _.emlink_hits += 1;

        if (_.active_backend == IsoLinkBackend.CopyFile) {
            return IsoFallbackDecision{
                should_retry: false,
                next_backend: IsoLinkBackend.CopyFile,
                permanent_error: true,
                idempotent_skip: false,
            };
        }

        // Cross-device EXDEV rules out both clonefile/reflink AND hardlink -> jump straight to CopyFile.
        let next = IsoLinkBackend.CopyFile;
        if (code == IsoFsErrorCode.Exdev) {
            next = IsoLinkBackend.CopyFile;
        } else if (_.active_backend == IsoLinkBackend.Clonefile || _.active_backend == IsoLinkBackend.Reflink) {
            next = IsoLinkBackend.Hardlink;
        } else if (_.active_backend == IsoLinkBackend.Hardlink) {
            next = IsoLinkBackend.CopyFile;
        }

        _.active_backend = next;
        _.downgrade_count += 1;
        return IsoFallbackDecision{
            should_retry: true,
            next_backend: next,
            permanent_error: false,
            idempotent_skip: false,
        };
    }
}

// ============================================================================
// Section 2: Isolated Store Directory Tree Materialization Planner
// ============================================================================

/// Atomic filesystem operation kind emitted by `planIsolatedMaterialization`.
pub enum IsoInstOpKind: u8 {
    EnsureStoreDir = 0,
    MaterializeFiles = 1,
    CreateDepSymlink = 2,
    CreateBinLink = 3,
    PruneStaleStoreEntry = 4,
}

/// Directed dependency symlink edge from an owner package to `target_pkg_index`.
pub struct IsoInstDepLink {
    dep_alias: []const u8 = "",
    target_pkg_index: usize = 0,

    static fn empty(): IsoInstDepLink {
        return IsoInstDepLink{};
    }
}

/// Package entry to materialize inside `cap_modules/.cap/`.
pub struct IsoInstStorePackage {
    name: []const u8 = "",
    version: []const u8 = "",
    cache_source_dir: []const u8 = "",
    is_direct: bool = false,
    deps_start: usize = 0,
    deps_len: usize = 0,

    static fn empty(): IsoInstStorePackage {
        return IsoInstStorePackage{};
    }
}

/// Planned atomic operation referencing slices in caller-owned `path_arena`.
pub struct IsoInstOp {
    kind: IsoInstOpKind = IsoInstOpKind.EnsureStoreDir,
    backend: IsoLinkBackend = IsoLinkBackend.CopyFile,
    src_start: usize = 0,
    src_len: usize = 0,
    dst_start: usize = 0,
    dst_len: usize = 0,
    pkg_index: usize = 0,

    static fn empty(): IsoInstOp {
        return IsoInstOp{};
    }
}

/// Aggregate summary returned by `planIsolatedMaterialization`.
pub struct IsoInstPlanSummary {
    op_count: usize = 0,
    ensure_dir_ops: u32 = 0,
    materialize_ops: u32 = 0,
    symlink_ops: u32 = 0,
    prune_ops: u32 = 0,
    arena_bytes_used: usize = 0,
}

/// Extracts the source path slice for `op` from `path_arena`.
pub fn getIsoOpSrc(op: IsoInstOp, path_arena: []const u8): []const u8 {
    if (op.src_start > path_arena.len || op.src_len > path_arena.len - op.src_start) return "";
    return path_arena[op.src_start..op.src_start + op.src_len];
}

/// Extracts the destination path slice for `op` from `path_arena`.
pub fn getIsoOpDst(op: IsoInstOp, path_arena: []const u8): []const u8 {
    if (op.dst_start > path_arena.len || op.dst_len > path_arena.len - op.dst_start) return "";
    return path_arena[op.dst_start..op.dst_start + op.dst_len];
}

fn iiPushOp(
    out_ops: []IsoInstOp,
    op_count: *usize,
    op: IsoInstOp,
)! {
    if (op_count.* >= out_ops.len) return err.SliceCapacityExceeded;
    out_ops[op_count.*] = op;
    op_count.* += 1;
}

/// Plans the complete, deterministically ordered isolated store materialization:
///   1. `EnsureStoreDir`: `cap_modules/.cap/<pkg>@<ver>/node_modules/<pkg>`
///   2. `MaterializeFiles`: `<cache_source_dir>` -> store package directory
///   3. `CreateDepSymlink`: relative symlinks for inter-package deps and top-level
///      `cap_modules/<name>` links for direct dependencies
///   4. `PruneStaleStoreEntry`: removes stale directories in `existing_store_keys`
///      that are no longer present in `packages`.
pub fn planIsolatedMaterialization(
    packages: []const IsoInstStorePackage,
    dep_links: []const IsoInstDepLink,
    existing_store_keys: []const []const u8,
    backend: IsoLinkBackend,
    out_ops: []IsoInstOp,
    path_arena: []u8,
): IsoInstallerError!IsoInstPlanSummary {
    let sum = IsoInstPlanSummary{};
    let arena_pos: usize = 0;

    // Validate all package inputs and dependency edges upfront (path traversal guard)
    let v: usize = 0;
    while (v < packages.len) {
        const pkg = packages[v];
        if (!isSafeIsoInstPath(pkg.name)) return err.InvalidPackageName;
        if (!isSafeIsoInstPath(pkg.version)) return err.UnsafePathTraversal;
        if (pkg.deps_start > dep_links.len || pkg.deps_len > dep_links.len - pkg.deps_start) {
            return err.InvalidTargetIndex;
        }
        let d: usize = 0;
        while (d < pkg.deps_len) {
            const dl = dep_links[pkg.deps_start + d];
            if (dl.target_pkg_index >= packages.len) return err.InvalidTargetIndex;
            if (!isSafeIsoInstPath(dl.dep_alias)) return err.UnsafePathTraversal;
            d += 1;
        }
        v += 1;
    }

    // Phase 1: EnsureStoreDir + Phase 2: MaterializeFiles
    let i: usize = 0;
    while (i < packages.len) {
        const pkg = packages[i];
        const dst_start = arena_pos;
        iiAppendBytes(path_arena, &arena_pos, ISO_INST_STORE_ROOT)?;
        iiAppendByte(path_arena, &arena_pos, '/')?;
        iiAppendEscapedStoreKey(path_arena, &arena_pos, pkg.name, pkg.version)?;
        iiAppendBytes(path_arena, &arena_pos, "/node_modules/")?;
        iiAppendBytes(path_arena, &arena_pos, pkg.name)?;
        const dst_len = arena_pos - dst_start;

        iiPushOp(out_ops, &sum.op_count, IsoInstOp{
            kind: IsoInstOpKind.EnsureStoreDir,
            backend: backend,
            dst_start: dst_start,
            dst_len: dst_len,
            pkg_index: i,
        })?;
        sum.ensure_dir_ops += 1;

        const src_start = arena_pos;
        iiAppendBytes(path_arena, &arena_pos, pkg.cache_source_dir)?;
        const src_len = arena_pos - src_start;

        iiPushOp(out_ops, &sum.op_count, IsoInstOp{
            kind: IsoInstOpKind.MaterializeFiles,
            backend: backend,
            src_start: src_start,
            src_len: src_len,
            dst_start: dst_start,
            dst_len: dst_len,
            pkg_index: i,
        })?;
        sum.materialize_ops += 1;
        i += 1;
    }

    // Phase 3: CreateDepSymlink (direct root links + inter-package relative symlinks)
    let p: usize = 0;
    while (p < packages.len) {
        const pkg = packages[p];

        if (pkg.is_direct) {
            // Top-level symlink: cap_modules/<name> -> .cap/<escaped_key>/node_modules/<name>
            // For scoped packages (@scope/pkg), target needs "../.cap/..."
            const is_scoped = Simd.indexOfByte(pkg.name, '/') != null;
            const src_start = arena_pos;
            if (is_scoped) {
                iiAppendBytes(path_arena, &arena_pos, "../.cap/")?;
            } else {
                iiAppendBytes(path_arena, &arena_pos, ".cap/")?;
            }
            iiAppendEscapedStoreKey(path_arena, &arena_pos, pkg.name, pkg.version)?;
            iiAppendBytes(path_arena, &arena_pos, "/node_modules/")?;
            iiAppendBytes(path_arena, &arena_pos, pkg.name)?;
            const src_len = arena_pos - src_start;

            const dst_start = arena_pos;
            iiAppendBytes(path_arena, &arena_pos, "cap_modules/")?;
            iiAppendBytes(path_arena, &arena_pos, pkg.name)?;
            const dst_len = arena_pos - dst_start;

            iiPushOp(out_ops, &sum.op_count, IsoInstOp{
                kind: IsoInstOpKind.CreateDepSymlink,
                backend: backend,
                src_start: src_start,
                src_len: src_len,
                dst_start: dst_start,
                dst_len: dst_len,
                pkg_index: p,
            })?;
            sum.symlink_ops += 1;
        }

        // Inter-package dependency symlinks inside owner's node_modules
        let d: usize = 0;
        while (d < pkg.deps_len) {
            const dl = dep_links[pkg.deps_start + d];
            const target = packages[dl.target_pkg_index];
            const alias_scoped = Simd.indexOfByte(dl.dep_alias, '/') != null;

            const src_start = arena_pos;
            if (alias_scoped) {
                iiAppendBytes(path_arena, &arena_pos, "../../../")?;
            } else {
                iiAppendBytes(path_arena, &arena_pos, "../../")?;
            }
            iiAppendEscapedStoreKey(path_arena, &arena_pos, target.name, target.version)?;
            iiAppendBytes(path_arena, &arena_pos, "/node_modules/")?;
            iiAppendBytes(path_arena, &arena_pos, target.name)?;
            const src_len = arena_pos - src_start;

            const dst_start = arena_pos;
            iiAppendBytes(path_arena, &arena_pos, ISO_INST_STORE_ROOT)?;
            iiAppendByte(path_arena, &arena_pos, '/')?;
            iiAppendEscapedStoreKey(path_arena, &arena_pos, pkg.name, pkg.version)?;
            iiAppendBytes(path_arena, &arena_pos, "/node_modules/")?;
            iiAppendBytes(path_arena, &arena_pos, dl.dep_alias)?;
            const dst_len = arena_pos - dst_start;

            iiPushOp(out_ops, &sum.op_count, IsoInstOp{
                kind: IsoInstOpKind.CreateDepSymlink,
                backend: backend,
                src_start: src_start,
                src_len: src_len,
                dst_start: dst_start,
                dst_len: dst_len,
                pkg_index: p,
            })?;
            sum.symlink_ops += 1;
            d += 1;
        }
        p += 1;
    }

    // Phase 4: PruneStaleStoreEntry for any existing store key not in desired `packages`
    let key_buf: [256]u8 = [0; 256];
    let ex: usize = 0;
    while (ex < existing_store_keys.len) {
        const existing_key = existing_store_keys[ex];
        if (!isSafeIsoInstPath(existing_key)) return err.UnsafePathTraversal;

        let still_needed = false;
        let k: usize = 0;
        while (k < packages.len) {
            let kpos: usize = 0;
            iiAppendEscapedStoreKey(key_buf[0..], &kpos, packages[k].name, packages[k].version)?;
            if (iiBytesEql(existing_key, key_buf[0..kpos])) {
                still_needed = true;
                break;
            }
            k += 1;
        }

        if (!still_needed) {
            const dst_start = arena_pos;
            iiAppendBytes(path_arena, &arena_pos, ISO_INST_STORE_ROOT)?;
            iiAppendByte(path_arena, &arena_pos, '/')?;
            iiAppendBytes(path_arena, &arena_pos, existing_key)?;
            const dst_len = arena_pos - dst_start;

            iiPushOp(out_ops, &sum.op_count, IsoInstOp{
                kind: IsoInstOpKind.PruneStaleStoreEntry,
                backend: backend,
                dst_start: dst_start,
                dst_len: dst_len,
                pkg_index: 0,
            })?;
            sum.prune_ops += 1;
        }
        ex += 1;
    }

    sum.arena_bytes_used = arena_pos;
    return sum;
}

// ============================================================================
// Section 3: `.bin` Conflict Arbiter & Cross-Platform Shim Generators
// ============================================================================

/// Candidate executable bin link exported by a package.
pub struct IsoBinCandidate {
    bin_name: []const u8 = "",
    owner_pkg: []const u8 = "",
    target_rel_path: []const u8 = "",
    is_direct_dep: bool = false,

    static fn empty(): IsoBinCandidate {
        return IsoBinCandidate{};
    }
}

/// Platform shim format selected for a `.bin` entry.
pub enum IsoBinShimFormat: u8 {
    PosixSymlink = 0,
    WindowsCmdAndPs1 = 1,
}

/// Resolved `.bin` link emitted by `planBinShimLinks`.
pub struct IsoBinPlannedLink {
    bin_name: []const u8 = "",
    owner_pkg: []const u8 = "",
    target_rel_path: []const u8 = "",
    format: IsoBinShimFormat = IsoBinShimFormat.PosixSymlink,
    is_direct_dep: bool = false,
    overrode_transitive: bool = false,

    static fn empty(): IsoBinPlannedLink {
        return IsoBinPlannedLink{};
    }
}

/// Summary returned by `planBinShimLinks`.
pub struct IsoBinPlanSummary {
    total_candidates: usize = 0,
    planned_count: usize = 0,
    conflicts_resolved: u32 = 0,
    rejected_unsafe: u32 = 0,
}

/// Arbitrates `.bin` executable name conflicts across direct and transitive
/// dependencies (direct dependencies always win over transitive ones; ties break
/// deterministically by `owner_pkg` ascending) and outputs sorted `out_links`.
pub fn planBinShimLinks(
    candidates: []const IsoBinCandidate,
    host_os: IsoHostOs,
    out_links: []IsoBinPlannedLink,
): IsoInstallerError!IsoBinPlanSummary {
    let sum = IsoBinPlanSummary{ total_candidates: candidates.len };
    const fmt = if (host_os == IsoHostOs.Windows) IsoBinShimFormat.WindowsCmdAndPs1 else IsoBinShimFormat.PosixSymlink;

    let i: usize = 0;
    while (i < candidates.len) {
        const cand = candidates[i];
        if (!isSafeIsoBinName(cand.bin_name) || cand.target_rel_path.len == 0 ||
            Simd.indexOfAny4(cand.target_rel_path, 0, '\r', '\n', '"') != null)
        {
            sum.rejected_unsafe += 1;
            i += 1;
            continue;
        }

        // Check for existing entry with the same bin_name
        let existing_idx: usize = sum.planned_count;
        let k: usize = 0;
        while (k < sum.planned_count) {
            if (iiBytesEql(out_links[k].bin_name, cand.bin_name)) {
                existing_idx = k;
                break;
            }
            k += 1;
        }

        if (existing_idx < sum.planned_count) {
            sum.conflicts_resolved += 1;
            const cur = out_links[existing_idx];
            const cand_wins = if (cand.is_direct_dep != cur.is_direct_dep)
                cand.is_direct_dep
            else
                iiBytesLessThan(cand.owner_pkg, cur.owner_pkg);
            if (cand_wins) {
                out_links[existing_idx] = IsoBinPlannedLink{
                    bin_name: cand.bin_name,
                    owner_pkg: cand.owner_pkg,
                    target_rel_path: cand.target_rel_path,
                    format: fmt,
                    is_direct_dep: cand.is_direct_dep,
                    overrode_transitive: cand.is_direct_dep && !cur.is_direct_dep,
                };
            }
        } else {
            if (sum.planned_count >= out_links.len) return err.SliceCapacityExceeded;
            out_links[sum.planned_count] = IsoBinPlannedLink{
                bin_name: cand.bin_name,
                owner_pkg: cand.owner_pkg,
                target_rel_path: cand.target_rel_path,
                format: fmt,
                is_direct_dep: cand.is_direct_dep,
                overrode_transitive: false,
            };
            sum.planned_count += 1;
        }
        i += 1;
    }

    // Insertion-sort final links by `bin_name` ascending for determinism
    let s: usize = 1;
    while (s < sum.planned_count) {
        const key = out_links[s];
        let j = s;
        while (j > 0 && iiBytesLessThan(key.bin_name, out_links[j - 1].bin_name)) {
            out_links[j] = out_links[j - 1];
            j -= 1;
        }
        out_links[j] = key;
        s += 1;
    }

    return sum;
}

/// Generates a Windows `.cmd` wrapper shim invoking `target_rel_path` relative to `%~dp0`.
pub fn formatWindowsCmdShim(
    target_rel_path: []const u8,
    out_buf: []u8,
): IsoInstallerError!usize {
    if (target_rel_path.len == 0 || Simd.indexOfAny4(target_rel_path, 0, '\r', '\n', '"') != null) {
        return err.UnsafePathTraversal;
    }
    let pos: usize = 0;
    iiAppendBytes(out_buf, &pos, "@ECHO off\r\n\"%~dp0\\")?;
    let i: usize = 0;
    while (i < target_rel_path.len) {
        const c = target_rel_path[i];
        iiAppendByte(out_buf, &pos, if (c == '/') '\\' else c)?;
        i += 1;
    }
    iiAppendBytes(out_buf, &pos, "\" %*\r\n")?;
    return pos;
}

/// Generates a Windows PowerShell `.ps1` wrapper shim invoking `target_rel_path`.
pub fn formatWindowsPs1Shim(
    target_rel_path: []const u8,
    out_buf: []u8,
): IsoInstallerError!usize {
    if (target_rel_path.len == 0 || Simd.indexOfAny4(target_rel_path, 0, '\r', '\n', '"') != null) {
        return err.UnsafePathTraversal;
    }
    let pos: usize = 0;
    iiAppendBytes(out_buf, &pos, "#!/usr/bin/env pwsh\n$basedir = Split-Path $MyInvocation.MyCommand.Definition -Parent\n& \"$basedir/")?;
    iiAppendBytes(out_buf, &pos, target_rel_path)?;
    iiAppendBytes(out_buf, &pos, "\" $args\nexit $LASTEXITCODE\n")?;
    return pos;
}

// ============================================================================
// Section 4: Atomic Staging Directory Planner & Materialization Report
// ============================================================================

fn iiAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        iiAppendByte(out, pos, '0')?;
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
        iiAppendByte(out, pos, tmp[n])?;
    }
}

/// Atomic staging-to-final publish path pair (mirrors Bun's `TopWhich.staging` -> `TopWhich.final`).
pub struct IsoStagingPathPair {
    staging_start: usize = 0,
    staging_len: usize = 0,
    final_start: usize = 0,
    final_len: usize = 0,
}

/// Formats both the per-process staging path (`cap_modules/.cap/<pkg>@<ver>.tmp-<nonce>`)
/// and the atomic publish target (`cap_modules/.cap/<pkg>@<ver>`) into `out_buf`.
pub fn formatStagingStorePaths(
    pkg_name: []const u8,
    version: []const u8,
    staging_nonce: u64,
    out_buf: []u8,
): IsoInstallerError!IsoStagingPathPair {
    if (!isSafeIsoInstPath(pkg_name)) return err.InvalidPackageName;
    if (!isSafeIsoInstPath(version)) return err.UnsafePathTraversal;

    let pos: usize = 0;
    const staging_start = pos;
    iiAppendBytes(out_buf, &pos, ISO_INST_STORE_ROOT)?;
    iiAppendByte(out_buf, &pos, '/')?;
    iiAppendEscapedStoreKey(out_buf, &pos, pkg_name, version)?;
    iiAppendBytes(out_buf, &pos, ".tmp-")?;
    iiAppendU64(out_buf, &pos, staging_nonce)?;
    const staging_len = pos - staging_start;

    const final_start = pos;
    iiAppendBytes(out_buf, &pos, ISO_INST_STORE_ROOT)?;
    iiAppendByte(out_buf, &pos, '/')?;
    iiAppendEscapedStoreKey(out_buf, &pos, pkg_name, version)?;
    const final_len = pos - final_start;

    return IsoStagingPathPair{
        staging_start: staging_start,
        staging_len: staging_len,
        final_start: final_start,
        final_len: final_len,
    };
}

/// Formats a structured summary report of isolated materialization ops, backend
/// fallback telemetry, and `.bin` conflict resolution into `out_buf`.
pub fn formatIsoInstallerReport(
    plan: IsoInstPlanSummary,
    fallback: IsoBackendFallbackState,
    bins: IsoBinPlanSummary,
    out_buf: []u8,
): IsoInstallerError!usize {
    let pos: usize = 0;
    iiAppendBytes(out_buf, &pos, "IsolatedInstaller Summary\n")?;
    iiAppendBytes(out_buf, &pos, "  backend: active=")?;
    iiAppendBytes(out_buf, &pos, isoLinkBackendName(fallback.active_backend))?;
    iiAppendBytes(out_buf, &pos, " downgrades=")?;
    iiAppendU64(out_buf, &pos, @as(u64, fallback.downgrade_count))?;
    iiAppendBytes(out_buf, &pos, " exdev=")?;
    iiAppendU64(out_buf, &pos, @as(u64, fallback.exdev_hits))?;
    iiAppendBytes(out_buf, &pos, " enotsup=")?;
    iiAppendU64(out_buf, &pos, @as(u64, fallback.enotsup_hits))?;
    iiAppendBytes(out_buf, &pos, " emlink=")?;
    iiAppendU64(out_buf, &pos, @as(u64, fallback.emlink_hits))?;
    iiAppendByte(out_buf, &pos, '\n')?;

    iiAppendBytes(out_buf, &pos, "  ops: total=")?;
    iiAppendU64(out_buf, &pos, @as(u64, plan.op_count))?;
    iiAppendBytes(out_buf, &pos, " dirs=")?;
    iiAppendU64(out_buf, &pos, @as(u64, plan.ensure_dir_ops))?;
    iiAppendBytes(out_buf, &pos, " materialize=")?;
    iiAppendU64(out_buf, &pos, @as(u64, plan.materialize_ops))?;
    iiAppendBytes(out_buf, &pos, " symlinks=")?;
    iiAppendU64(out_buf, &pos, @as(u64, plan.symlink_ops))?;
    iiAppendBytes(out_buf, &pos, " pruned=")?;
    iiAppendU64(out_buf, &pos, @as(u64, plan.prune_ops))?;
    iiAppendByte(out_buf, &pos, '\n')?;

    iiAppendBytes(out_buf, &pos, "  bins: planned=")?;
    iiAppendU64(out_buf, &pos, @as(u64, bins.planned_count))?;
    iiAppendBytes(out_buf, &pos, " conflicts=")?;
    iiAppendU64(out_buf, &pos, @as(u64, bins.conflicts_resolved))?;
    iiAppendBytes(out_buf, &pos, " rejected=")?;
    iiAppendU64(out_buf, &pos, @as(u64, bins.rejected_unsafe))?;
    iiAppendByte(out_buf, &pos, '\n')?;
    return pos;
}
