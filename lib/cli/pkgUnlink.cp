// Source: Absorbed from /data/dev/bun/src/cli/unlink_command.cp | Synced: 2026-11-22T01:23:37Z
// ============================================================================
// lib/cli/pkgUnlink.cp — `cap unlink` Transaction Planner & Symlink Safety Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven package unlink planner for `cap unlink`:
//   1. Two unlink modes (`UnlinkMode`):
//        - `GlobalSelfUnlink` (`cap unlink`): unregisters the current project's
//          global symlink under `~/.cap/install/global/node_modules/<pkg>` and
//          associated global `.bin/` shims;
//        - `ProjectDepUnlink` (`cap unlink <pkg...>`): detaches local linked
//          dependencies in `cap_modules/<pkg>`, cleans `.bin/` symlinks, and
//          plans Registry version restoration;
//   2. `cap:simd`-accelerated package name & symlink safety guard
//      (`parseUnlinkPackageName`, `validateUnlinkTarget`): blocks path traversal,
//      absolute paths, invalid `@scope/` syntax, non-ASCII control bytes, and
//      protects real directories/files from accidental deletion (symlink-only);
//   3. Transactional cleanup planner (`planPkgUnlink`) with automatic empty
//      `@scope/` directory reclamation, `.bin/` shim removal, and human-readable
//      summary report formatting (`formatPkgUnlinkReport`).

import { Simd } from "cap:simd";

/// Maximum legal package name length (aligned with npm/cap specification).
pub const PKG_UNLINK_MAX_NAME_LEN: usize = 214;

/// Maximum normalized path length handled by internal stack scratch buffers.
pub const PKG_UNLINK_MAX_PATH_LEN: usize = 1024;

/// Relative subdirectory under the global cap root holding globally linked packages.
pub const PKG_UNLINK_GLOBAL_SUBDIR: []const u8 = "install/global/node_modules";

/// Relative subdirectory under the global cap root holding global binary shims.
pub const PKG_UNLINK_GLOBAL_BIN_SUBDIR: []const u8 = "bin";

/// Local project module directory name.
pub const PKG_UNLINK_LOCAL_MODULES_DIR: []const u8 = "cap_modules";

/// Error set for `cap unlink` argument parsing, safety validation, and planning.
pub err PkgUnlinkError {
    EmptyPackageName,
    PackageNameTooLong,
    InvalidPackageName,
    InvalidScopeName,
    PathTraversalDetected,
    AbsolutePathForbidden,
    InvalidTargetPath,
    NotASymlink,
    SymlinkTargetOutsideRoot,
    BufferTooSmall,
    CapacityExceeded,
    MissingManifestName,
    InvalidBinName,
    UnknownFlag,
}

// ============================================================================
// Internal Zero-Alloc Byte, Slice, and Arena Formatting Helpers
// ============================================================================

fn ulBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn ulAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn ulAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn ulAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        ulAppendByte(out, pos, '0')?;
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
        ulAppendByte(out, pos, tmp[n])?;
    }
}

fn ulIsNameChar(c: u8): bool {
    return (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') ||
        c == '-' || c == '_' || c == '.';
}

fn ulValidateSegment(seg: []const u8): bool {
    if (seg.len == 0) return false;
    if (seg[0] == '.' || seg[0] == '_') return false;
    let i: usize = 0;
    while (i < seg.len) {
        if (!ulIsNameChar(seg[i])) return false;
        i += 1;
    }
    return true;
}

// ============================================================================
// Section 1: SIMD-Accelerated Package Name, Scope & Path Normalization
// ============================================================================

/// Parsed representation of an unscoped (`foo`) or scoped (`@org/foo`) package name.
pub struct UnlinkParsedPkgName {
    full_name: []const u8 = "",
    scope: []const u8 = "",
    short_name: []const u8 = "",
    is_scoped: bool = false,

    static fn empty(): UnlinkParsedPkgName {
        return UnlinkParsedPkgName{};
    }
}

/// Validates and parses a package name using `cap:simd` (`Simd.isAscii`,
/// `Simd.indexOfByte`, `Simd.indexOfAny4`) to reject non-ASCII bytes,
/// control characters, path traversal (`..`), absolute paths, and malformed `@scope/`.
pub fn parseUnlinkPackageName(raw: []const u8): PkgUnlinkError!UnlinkParsedPkgName {
    if (raw.len == 0) return err.EmptyPackageName;
    if (raw.len > PKG_UNLINK_MAX_NAME_LEN) return err.PackageNameTooLong;
    if (!Simd.isAscii(raw)) return err.InvalidPackageName;
    if (Simd.indexOfAny4(raw, 0, '\\', ' ', '\t') != null) {
        return err.InvalidPackageName;
    }
    if (raw[0] == '/') return err.AbsolutePathForbidden;
    if (raw.len >= 2 && raw[0] == '.' && raw[1] == '.') {
        return err.PathTraversalDetected;
    }

    if (raw[0] == '@') {
        const slash_opt = Simd.indexOfByte(raw[1..], '/');
        if (slash_opt == null) return err.InvalidScopeName;
        const slash = 1 + (slash_opt ?? 0);
        const scope_part = raw[1..slash];
        const pkg_part = raw[slash + 1..];

        // Reject additional slashes inside scoped name
        if (Simd.indexOfByte(pkg_part, '/') != null) {
            return err.InvalidPackageName;
        }
        if (!ulValidateSegment(scope_part)) return err.InvalidScopeName;
        if (!ulValidateSegment(pkg_part)) return err.InvalidPackageName;

        return UnlinkParsedPkgName{
            full_name: raw,
            scope: scope_part,
            short_name: pkg_part,
            is_scoped: true,
        };
    }

    // Unscoped package name must not contain '/' or '@'
    if (Simd.indexOfAny2(raw, '/', '@') != null) {
        return err.InvalidPackageName;
    }
    if (!ulValidateSegment(raw)) return err.InvalidPackageName;

    return UnlinkParsedPkgName{
        full_name: raw,
        scope: "",
        short_name: raw,
        is_scoped: false,
    };
}

/// Normalizes a filesystem path into canonical forward-slash form using `cap:simd`
/// (`Simd.indexOfAny2`). Preserves leading `/` for absolute paths, folds repeated
/// separators, strips `.` segments, and rejects NUL bytes or `..` traversal.
pub fn normalizeUnlinkPath(raw: []const u8, out: []u8): PkgUnlinkError!usize {
    if (raw.len == 0) return err.InvalidTargetPath;
    if (Simd.indexOfByte(raw, 0) != null) return err.InvalidTargetPath;

    const is_abs = raw[0] == '/' || raw[0] == '\\';
    let pos: usize = 0;
    let out_pos: usize = 0;

    if (is_abs) {
        ulAppendByte(out, &out_pos, '/')?;
    }

    while (pos < raw.len) {
        while (pos < raw.len && (raw[pos] == '/' || raw[pos] == '\\')) {
            pos += 1;
        }
        if (pos >= raw.len) break;

        let seg_end = raw.len;
        if (rel := Simd.indexOfAny2(raw[pos..], '/', '\\')) {
            seg_end = pos + rel;
        }
        const seg = raw[pos..seg_end];
        pos = seg_end;

        if (seg.len == 1 && seg[0] == '.') {
            continue;
        }
        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
            return err.PathTraversalDetected;
        }

        if (out_pos > 0 && out[out_pos - 1] != '/') {
            ulAppendByte(out, &out_pos, '/')?;
        }
        ulAppendBytes(out, &out_pos, seg)?;
    }

    if (out_pos == 0) return err.InvalidTargetPath;
    return out_pos;
}

/// Checks whether canonical path `candidate` lies strictly inside or equals
/// canonical directory `root_prefix`, preventing sibling-prefix bypasses
/// (e.g. `/a/bc` does NOT match prefix `/a/b`).
pub fn isUnlinkPathWithinRoot(candidate: []const u8, root_prefix: []const u8): bool {
    if (root_prefix.len == 0 || candidate.len < root_prefix.len) return false;
    if (!ulBytesEql(candidate[0..root_prefix.len], root_prefix)) return false;
    if (candidate.len == root_prefix.len) return true;
    if (root_prefix[root_prefix.len - 1] == '/') return true;
    return candidate[root_prefix.len] == '/';
}

/// Validates a `.bin/` executable name via `cap:simd`, rejecting path separators,
/// control characters, NUL bytes, `.`, and `..`.
pub fn isSafeUnlinkBinName(bin_name: []const u8): bool {
    if (bin_name.len == 0 || bin_name.len > 128) return false;
    if (!Simd.isAscii(bin_name)) return false;
    if (Simd.indexOfAny4(bin_name, '/', '\\', 0, ':') != null) return false;
    if (bin_name.len == 1 && bin_name[0] == '.') return false;
    if (bin_name.len == 2 && bin_name[0] == '.' && bin_name[1] == '.') return false;
    let i: usize = 0;
    while (i < bin_name.len) {
        if (bin_name[i] < 0x21 || bin_name[i] == 0x7F) return false;
        i += 1;
    }
    return true;
}

/// Formats the canonical global symlink path: `<global_root>/install/global/node_modules/<pkg>`.
pub fn formatGlobalLinkPath(global_root: []const u8, pkg_name: []const u8, out: []u8): PkgUnlinkError!usize {
    * = parseUnlinkPackageName(pkg_name)?;
    let norm_root: [PKG_UNLINK_MAX_PATH_LEN]u8 = [0; PKG_UNLINK_MAX_PATH_LEN];
    const root_len = normalizeUnlinkPath(global_root, norm_root[0..])?;

    let pos: usize = 0;
    ulAppendBytes(out, &pos, norm_root[0..root_len])?;
    if (pos > 0 && out[pos - 1] != '/') ulAppendByte(out, &pos, '/')?;
    ulAppendBytes(out, &pos, PKG_UNLINK_GLOBAL_SUBDIR)?;
    ulAppendByte(out, &pos, '/')?;
    ulAppendBytes(out, &pos, pkg_name)?;
    return pos;
}

/// Formats the canonical local project link path: `<project_root>/cap_modules/<pkg>`.
pub fn formatLocalLinkPath(project_root: []const u8, pkg_name: []const u8, out: []u8): PkgUnlinkError!usize {
    * = parseUnlinkPackageName(pkg_name)?;
    let norm_root: [PKG_UNLINK_MAX_PATH_LEN]u8 = [0; PKG_UNLINK_MAX_PATH_LEN];
    const root_len = normalizeUnlinkPath(project_root, norm_root[0..])?;

    let pos: usize = 0;
    ulAppendBytes(out, &pos, norm_root[0..root_len])?;
    if (pos > 0 && out[pos - 1] != '/') ulAppendByte(out, &pos, '/')?;
    ulAppendBytes(out, &pos, PKG_UNLINK_LOCAL_MODULES_DIR)?;
    ulAppendByte(out, &pos, '/')?;
    ulAppendBytes(out, &pos, pkg_name)?;
    return pos;
}

// ============================================================================
// Section 2: Unlink Modes & Symlink Safety Guard (`validateUnlinkTarget`)
// ============================================================================

/// Execution mode for `cap unlink`.
pub enum UnlinkMode {
    /// `cap unlink` (no package arguments): unregister current project from global link dir.
    GlobalSelfUnlink,
    /// `cap unlink <pkg...>`: unlink local packages from `cap_modules/` and restore Registry deps.
    ProjectDepUnlink,
}

/// Returns the canonical ASCII label of an `UnlinkMode`.
pub fn unlinkModeName(mode: UnlinkMode): []const u8 {
    return match (mode) {
        UnlinkMode.GlobalSelfUnlink => "global-self",
        UnlinkMode.ProjectDepUnlink => "project-dep",
    };
}

/// Filesystem `lstat` classification of the candidate link path.
pub enum UnlinkFsEntryKind {
    Missing,
    Symlink,
    Directory,
    RegularFile,
    Other,
}

/// Filesystem probe describing one candidate package to unlink.
pub struct UnlinkTargetProbe {
    package_name: []const u8 = "",
    link_path: []const u8 = "",
    entry_kind: UnlinkFsEntryKind = UnlinkFsEntryKind.Missing,
    symlink_target: []const u8 = "",
    expected_root_dir: []const u8 = "",
    scope_remaining_siblings: u32 = 0,
    bin_names: []const []const u8 = [],
    registry_fallback_version: []const u8 = "",
}

/// Safety decision produced by `validateUnlinkTarget`.
pub enum UnlinkGuardStatus {
    SafeToUnlink,
    AlreadyNotLinked,
    RefusedNotSymlink,
    RefusedTargetEscape,
    RefusedInvalidName,
    RefusedInvalidPath,
}

/// Returns a short diagnostic explanation for an `UnlinkGuardStatus`.
pub fn unlinkGuardStatusMessage(status: UnlinkGuardStatus): []const u8 {
    return match (status) {
        UnlinkGuardStatus.SafeToUnlink => "safe symlink verified",
        UnlinkGuardStatus.AlreadyNotLinked => "package is not linked, nothing to do",
        UnlinkGuardStatus.RefusedNotSymlink => "refused: target is not a symlink (protecting real directory/file)",
        UnlinkGuardStatus.RefusedTargetEscape => "refused: symlink target escapes expected link root",
        UnlinkGuardStatus.RefusedInvalidName => "refused: invalid package name",
        UnlinkGuardStatus.RefusedInvalidPath => "refused: invalid or unsafe link path",
    };
}

/// Result returned by `validateUnlinkTarget`.
pub struct UnlinkTargetValidation {
    status: UnlinkGuardStatus = UnlinkGuardStatus.RefusedInvalidName,
    parsed_name: UnlinkParsedPkgName = UnlinkParsedPkgName.empty(),
    can_remove_empty_scope: bool = false,

    fn isSafe(): bool {
        return _.status == UnlinkGuardStatus.SafeToUnlink;
    }
}

/// Validates package name, link path safety, symlink-only protection, and
/// symlink target containment against `expected_root_dir`.
pub fn validateUnlinkTarget(probe: UnlinkTargetProbe, mode: UnlinkMode): UnlinkTargetValidation {
    const parsed = parseUnlinkPackageName(probe.package_name) catch {
        return UnlinkTargetValidation{ status: UnlinkGuardStatus.RefusedInvalidName };
    };

    // Validate link_path itself has no traversal or NUL bytes
    let norm_link: [PKG_UNLINK_MAX_PATH_LEN]u8 = [0; PKG_UNLINK_MAX_PATH_LEN];
    const link_len = normalizeUnlinkPath(probe.link_path, norm_link[0..]) catch {
        return UnlinkTargetValidation{
            status: UnlinkGuardStatus.RefusedInvalidPath,
            parsed_name: parsed,
        };
    };
    * = link_len;

    // Missing entry -> idempotent no-op
    if (probe.entry_kind == UnlinkFsEntryKind.Missing) {
        return UnlinkTargetValidation{
            status: UnlinkGuardStatus.AlreadyNotLinked,
            parsed_name: parsed,
        };
    }

    // Protect real directories and regular files from accidental deletion
    if (probe.entry_kind != UnlinkFsEntryKind.Symlink) {
        return UnlinkTargetValidation{
            status: UnlinkGuardStatus.RefusedNotSymlink,
            parsed_name: parsed,
        };
    }

    // Validate symlink_target is non-empty and free of traversal / NUL
    let norm_target: [PKG_UNLINK_MAX_PATH_LEN]u8 = [0; PKG_UNLINK_MAX_PATH_LEN];
    const target_len = normalizeUnlinkPath(probe.symlink_target, norm_target[0..]) catch {
        return UnlinkTargetValidation{
            status: UnlinkGuardStatus.RefusedTargetEscape,
            parsed_name: parsed,
        };
    };

    // Enforce expected_root_dir containment when specified (mandatory in ProjectDepUnlink)
    if (probe.expected_root_dir.len > 0 || mode == UnlinkMode.ProjectDepUnlink) {
        let norm_root: [PKG_UNLINK_MAX_PATH_LEN]u8 = [0; PKG_UNLINK_MAX_PATH_LEN];
        const root_len = normalizeUnlinkPath(probe.expected_root_dir, norm_root[0..]) catch {
            return UnlinkTargetValidation{
                status: UnlinkGuardStatus.RefusedTargetEscape,
                parsed_name: parsed,
            };
        };
        if (!isUnlinkPathWithinRoot(norm_target[0..target_len], norm_root[0..root_len])) {
            return UnlinkTargetValidation{
                status: UnlinkGuardStatus.RefusedTargetEscape,
                parsed_name: parsed,
            };
        }
    }

    const can_rm_scope = parsed.is_scoped && probe.scope_remaining_siblings == 0;
    return UnlinkTargetValidation{
        status: UnlinkGuardStatus.SafeToUnlink,
        parsed_name: parsed,
        can_remove_empty_scope: can_rm_scope,
    };
}

// ============================================================================
// Section 3: Transactional Unlink Planner & Empty Scope Reclamation
// ============================================================================

/// Atomic action emitted by `planPkgUnlink`.
pub enum UnlinkOpKind {
    RemovePackageSymlink,
    RemoveBinSymlink,
    RemoveEmptyScopeDir,
    RestoreRegistryDep,
    SkipAlreadyUnlinked,
    BlockedUnsafeTarget,
}

/// Compact 8-byte slice handle into caller-provided `text_arena`.
pub struct UnlinkTextSpan {
    offset: u32 = 0,
    len: u32 = 0,

    static fn empty(): UnlinkTextSpan {
        return UnlinkTextSpan{ offset: 0, len: 0 };
    }
}

/// Resolves an `UnlinkTextSpan` against `text_arena`.
pub fn getUnlinkOpText(span: UnlinkTextSpan, text_arena: []const u8): []const u8 {
    if (span.len == 0) return "";
    const off = @as(usize, span.offset);
    const l = @as(usize, span.len);
    if (off > text_arena.len || l > text_arena.len - off) return "";
    return text_arena[off..off + l];
}

/// Single planned unlink transaction operation.
pub struct UnlinkPlannedOp {
    kind: UnlinkOpKind = UnlinkOpKind.SkipAlreadyUnlinked,
    package_name: UnlinkTextSpan = UnlinkTextSpan.empty(),
    path: UnlinkTextSpan = UnlinkTextSpan.empty(),
    detail: UnlinkTextSpan = UnlinkTextSpan.empty(),

    static fn empty(): UnlinkPlannedOp {
        return UnlinkPlannedOp{};
    }
}

/// Options controlling `planPkgUnlink`.
pub struct UnlinkPlanOptions {
    mode: UnlinkMode = UnlinkMode.GlobalSelfUnlink,
    global_root_dir: []const u8 = "",
    project_root_dir: []const u8 = "",
    fail_on_unsafe: bool = false,
}

/// Summary statistics returned by `planPkgUnlink`.
pub struct UnlinkPlanSummary {
    mode: UnlinkMode = UnlinkMode.GlobalSelfUnlink,
    total_targets: usize = 0,
    ops_count: usize = 0,
    removed_pkg_symlinks: usize = 0,
    removed_bin_symlinks: usize = 0,
    removed_empty_scopes: usize = 0,
    planned_registry_restores: usize = 0,
    skipped_not_linked: usize = 0,
    blocked_unsafe: usize = 0,
    arena_bytes_used: usize = 0,
}

fn ulArenaPut(arena: []u8, pos: *usize, s: []const u8): PkgUnlinkError!UnlinkTextSpan {
    if (s.len == 0) return UnlinkTextSpan.empty();
    if (s.len > arena.len - pos.*) return err.BufferTooSmall;
    const start = pos.*;
    ulAppendBytes(arena, pos, s)?;
    return UnlinkTextSpan{
        offset: @as(u32, @intCast(start)),
        len: @as(u32, @intCast(s.len)),
    };
}

fn ulEmitOp(
    out_ops: []UnlinkPlannedOp,
    ops_count: *usize,
    op: UnlinkPlannedOp,
)! {
    if (ops_count.* >= out_ops.len) return err.CapacityExceeded;
    out_ops[ops_count.*] = op;
    ops_count.* += 1;
}

fn ulFormatBinPath(
    opts: UnlinkPlanOptions,
    bin_name: []const u8,
    out: []u8,
): PkgUnlinkError!usize {
    const base_dir = if (opts.mode == UnlinkMode.GlobalSelfUnlink)
        opts.global_root_dir
    else
        opts.project_root_dir;
    let norm_base: [PKG_UNLINK_MAX_PATH_LEN]u8 = [0; PKG_UNLINK_MAX_PATH_LEN];
    const base_len = normalizeUnlinkPath(base_dir, norm_base[0..])?;

    let pos: usize = 0;
    ulAppendBytes(out, &pos, norm_base[0..base_len])?;
    if (pos > 0 && out[pos - 1] != '/') ulAppendByte(out, &pos, '/')?;
    if (opts.mode == UnlinkMode.GlobalSelfUnlink) {
        ulAppendBytes(out, &pos, PKG_UNLINK_GLOBAL_BIN_SUBDIR)?;
    } else {
        ulAppendBytes(out, &pos, PKG_UNLINK_LOCAL_MODULES_DIR)?;
        ulAppendBytes(out, &pos, "/.bin")?;
    }
    ulAppendByte(out, &pos, '/')?;
    ulAppendBytes(out, &pos, bin_name)?;
    return pos;
}

fn ulFormatScopeDirPath(
    opts: UnlinkPlanOptions,
    scope: []const u8,
    out: []u8,
): PkgUnlinkError!usize {
    const base_dir = if (opts.mode == UnlinkMode.GlobalSelfUnlink)
        opts.global_root_dir
    else
        opts.project_root_dir;
    let norm_base: [PKG_UNLINK_MAX_PATH_LEN]u8 = [0; PKG_UNLINK_MAX_PATH_LEN];
    const base_len = normalizeUnlinkPath(base_dir, norm_base[0..])?;

    let pos: usize = 0;
    ulAppendBytes(out, &pos, norm_base[0..base_len])?;
    if (pos > 0 && out[pos - 1] != '/') ulAppendByte(out, &pos, '/')?;
    if (opts.mode == UnlinkMode.GlobalSelfUnlink) {
        ulAppendBytes(out, &pos, PKG_UNLINK_GLOBAL_SUBDIR)?;
    } else {
        ulAppendBytes(out, &pos, PKG_UNLINK_LOCAL_MODULES_DIR)?;
    }
    ulAppendBytes(out, &pos, "/@")?;
    ulAppendBytes(out, &pos, scope)?;
    return pos;
}

/// Plans a complete, zero-heap unlink transaction for all `probes`, emitting
/// ordered cleanup actions (`.bin/` shims -> package symlink -> empty `@scope/` dir
/// -> optional Registry restore) into caller-provided `out_ops` and `text_arena`.
pub fn planPkgUnlink(
    probes: []const UnlinkTargetProbe,
    opts: UnlinkPlanOptions,
    out_ops: []UnlinkPlannedOp,
    text_arena: []u8,
): PkgUnlinkError!UnlinkPlanSummary {
    let summary = UnlinkPlanSummary{
        mode: opts.mode,
        total_targets: probes.len,
    };
    let arena_pos: usize = 0;
    let ops_count: usize = 0;

    let tmp_path: [PKG_UNLINK_MAX_PATH_LEN]u8 = [0; PKG_UNLINK_MAX_PATH_LEN];

    let i: usize = 0;
    while (i < probes.len) {
        const probe = probes[i];
        const v = validateUnlinkTarget(probe, opts.mode);

        if (v.status == UnlinkGuardStatus.AlreadyNotLinked) {
            const pkg_span = ulArenaPut(text_arena, &arena_pos, probe.package_name)?;
            const path_span = ulArenaPut(text_arena, &arena_pos, probe.link_path)?;
            const det_span = ulArenaPut(text_arena, &arena_pos, unlinkGuardStatusMessage(v.status))?;
            ulEmitOp(out_ops, &ops_count, UnlinkPlannedOp{
                kind: UnlinkOpKind.SkipAlreadyUnlinked,
                package_name: pkg_span,
                path: path_span,
                detail: det_span,
            })?;
            summary.skipped_not_linked += 1;
            i += 1;
            continue;
        }

        if (!v.isSafe()) {
            if (opts.fail_on_unsafe) {
                if (v.status == UnlinkGuardStatus.RefusedNotSymlink) return err.NotASymlink;
                if (v.status == UnlinkGuardStatus.RefusedTargetEscape) return err.SymlinkTargetOutsideRoot;
                if (v.status == UnlinkGuardStatus.RefusedInvalidPath) return err.InvalidTargetPath;
                return err.InvalidPackageName;
            }
            const pkg_span = ulArenaPut(text_arena, &arena_pos, probe.package_name)?;
            const path_span = ulArenaPut(text_arena, &arena_pos, probe.link_path)?;
            const det_span = ulArenaPut(text_arena, &arena_pos, unlinkGuardStatusMessage(v.status))?;
            ulEmitOp(out_ops, &ops_count, UnlinkPlannedOp{
                kind: UnlinkOpKind.BlockedUnsafeTarget,
                package_name: pkg_span,
                path: path_span,
                detail: det_span,
            })?;
            summary.blocked_unsafe += 1;
            i += 1;
            continue;
        }

        const pkg_span = ulArenaPut(text_arena, &arena_pos, v.parsed_name.full_name)?;

        // Step 1: Unlink associated `.bin/` shims first
        let b: usize = 0;
        while (b < probe.bin_names.len) {
            const bname = probe.bin_names[b];
            if (!isSafeUnlinkBinName(bname)) {
                return err.InvalidBinName;
            }
            const blen = ulFormatBinPath(opts, bname, tmp_path[0..])?;
            const bpath_span = ulArenaPut(text_arena, &arena_pos, tmp_path[0..blen])?;
            const bdet_span = ulArenaPut(text_arena, &arena_pos, bname)?;
            ulEmitOp(out_ops, &ops_count, UnlinkPlannedOp{
                kind: UnlinkOpKind.RemoveBinSymlink,
                package_name: pkg_span,
                path: bpath_span,
                detail: bdet_span,
            })?;
            summary.removed_bin_symlinks += 1;
            b += 1;
        }

        // Step 2: Remove the verified package symlink
        const link_len = normalizeUnlinkPath(probe.link_path, tmp_path[0..])?;
        const link_span = ulArenaPut(text_arena, &arena_pos, tmp_path[0..link_len])?;
        const target_span = ulArenaPut(text_arena, &arena_pos, probe.symlink_target)?;
        ulEmitOp(out_ops, &ops_count, UnlinkPlannedOp{
            kind: UnlinkOpKind.RemovePackageSymlink,
            package_name: pkg_span,
            path: link_span,
            detail: target_span,
        })?;
        summary.removed_pkg_symlinks += 1;

        // Step 3: Reclaim empty `@scope/` directory if this was the last linked package in scope
        if (v.can_remove_empty_scope) {
            const slen = ulFormatScopeDirPath(opts, v.parsed_name.scope, tmp_path[0..])?;
            const spath_span = ulArenaPut(text_arena, &arena_pos, tmp_path[0..slen])?;
            const sdet_span = ulArenaPut(text_arena, &arena_pos, v.parsed_name.scope)?;
            ulEmitOp(out_ops, &ops_count, UnlinkPlannedOp{
                kind: UnlinkOpKind.RemoveEmptyScopeDir,
                package_name: pkg_span,
                path: spath_span,
                detail: sdet_span,
            })?;
            summary.removed_empty_scopes += 1;
        }

        // Step 4: In ProjectDepUnlink mode, plan Registry restoration
        if (opts.mode == UnlinkMode.ProjectDepUnlink) {
            const ver_spec = if (probe.registry_fallback_version.len > 0)
                probe.registry_fallback_version
            else
                "latest";
            const ver_span = ulArenaPut(text_arena, &arena_pos, ver_spec)?;
            ulEmitOp(out_ops, &ops_count, UnlinkPlannedOp{
                kind: UnlinkOpKind.RestoreRegistryDep,
                package_name: pkg_span,
                path: link_span,
                detail: ver_span,
            })?;
            summary.planned_registry_restores += 1;
        }

        i += 1;
    }

    summary.ops_count = ops_count;
    summary.arena_bytes_used = arena_pos;
    return summary;
}

// ============================================================================
// Section 4: CLI Argument Parser & Human-Readable Execution Summary Report
// ============================================================================

/// Parsed CLI invocation for `cap unlink [flags...] [pkg...]`.
pub struct UnlinkCliArgs {
    mode: UnlinkMode = UnlinkMode.GlobalSelfUnlink,
    dry_run: bool = false,
    verbose: bool = false,
    fail_on_unsafe: bool = false,
    pkg_count: usize = 0,
}

/// Parses `cap unlink` CLI arguments into `UnlinkCliArgs` and writes validated
/// positional package names into `out_pkgs`.
pub fn parseUnlinkCliArgs(
    argv: []const []const u8,
    out_pkgs: [][]const u8,
): PkgUnlinkError!UnlinkCliArgs {
    let res = UnlinkCliArgs{};
    let end_of_flags = false;
    let i: usize = 0;
    while (i < argv.len) {
        const arg = argv[i];
        if (!end_of_flags && ulBytesEql(arg, "--")) {
            end_of_flags = true;
            i += 1;
            continue;
        }
        if (!end_of_flags && arg.len > 0 && arg[0] == '-') {
            if (ulBytesEql(arg, "--dry-run") || ulBytesEql(arg, "-n")) {
                res.dry_run = true;
            } else if (ulBytesEql(arg, "--verbose") || ulBytesEql(arg, "-v")) {
                res.verbose = true;
            } else if (ulBytesEql(arg, "--strict")) {
                res.fail_on_unsafe = true;
            } else {
                return err.UnknownFlag;
            }
            i += 1;
            continue;
        }

        // Positional package name must pass strict validation
        * = parseUnlinkPackageName(arg)?;
        if (res.pkg_count >= out_pkgs.len) return err.CapacityExceeded;
        out_pkgs[res.pkg_count] = arg;
        res.pkg_count += 1;
        i += 1;
    }

    res.mode = if (res.pkg_count == 0)
        UnlinkMode.GlobalSelfUnlink
    else
        UnlinkMode.ProjectDepUnlink;
    return res;
}

fn ulOpKindLabel(kind: UnlinkOpKind): []const u8 {
    return match (kind) {
        UnlinkOpKind.RemovePackageSymlink => "UNLINK_PKG",
        UnlinkOpKind.RemoveBinSymlink => "UNLINK_BIN",
        UnlinkOpKind.RemoveEmptyScopeDir => "RMDIR_SCOPE",
        UnlinkOpKind.RestoreRegistryDep => "RESTORE_REG",
        UnlinkOpKind.SkipAlreadyUnlinked => "SKIP",
        UnlinkOpKind.BlockedUnsafeTarget => "BLOCKED",
    };
}

/// Formats a human-readable multi-line execution summary report for `cap unlink`.
pub fn formatPkgUnlinkReport(
    summary: UnlinkPlanSummary,
    ops: []const UnlinkPlannedOp,
    text_arena: []const u8,
    out: []u8,
): PkgUnlinkError!usize {
    let pos: usize = 0;
    ulAppendBytes(out, &pos, "cap unlink [mode=")?;
    ulAppendBytes(out, &pos, unlinkModeName(summary.mode))?;
    ulAppendBytes(out, &pos, "] unlinked=")?;
    ulAppendU64(out, &pos, @as(u64, summary.removed_pkg_symlinks))?;
    ulAppendBytes(out, &pos, ", bins=")?;
    ulAppendU64(out, &pos, @as(u64, summary.removed_bin_symlinks))?;
    ulAppendBytes(out, &pos, ", scopes_reclaimed=")?;
    ulAppendU64(out, &pos, @as(u64, summary.removed_empty_scopes))?;
    ulAppendBytes(out, &pos, ", restores=")?;
    ulAppendU64(out, &pos, @as(u64, summary.planned_registry_restores))?;
    ulAppendBytes(out, &pos, ", skipped=")?;
    ulAppendU64(out, &pos, @as(u64, summary.skipped_not_linked))?;
    ulAppendBytes(out, &pos, ", blocked=")?;
    ulAppendU64(out, &pos, @as(u64, summary.blocked_unsafe))?;
    ulAppendByte(out, &pos, '\n')?;

    const limit = if (summary.ops_count < ops.len) summary.ops_count else ops.len;
    let i: usize = 0;
    while (i < limit) {
        const op = ops[i];
        ulAppendBytes(out, &pos, "  - ")?;
        ulAppendBytes(out, &pos, ulOpKindLabel(op.kind))?;
        ulAppendBytes(out, &pos, " ")?;
        ulAppendBytes(out, &pos, getUnlinkOpText(op.package_name, text_arena))?;
        ulAppendBytes(out, &pos, " -> ")?;
        ulAppendBytes(out, &pos, getUnlinkOpText(op.path, text_arena))?;
        const det = getUnlinkOpText(op.detail, text_arena);
        if (det.len > 0) {
            ulAppendBytes(out, &pos, " (")?;
            ulAppendBytes(out, &pos, det)?;
            ulAppendByte(out, &pos, ')')?;
        }
        ulAppendByte(out, &pos, '\n')?;
        i += 1;
    }

    return pos;
}
