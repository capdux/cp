// Source: Absorbed from /data/dev/bun/src/install/isolated_install/FileCopier.cp | Synced: 2026-11-23T20:24:05Z
// ============================================================================
// lib/pkg/fileMaterialize.cp — Package Tree Materialization, Diff & Exec-Bit Planner
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven package tree materializer for `cap install`:
//   1. Entry filtering & symlink loop guard (`shouldSkipMaterializeEntry`):
//      skips `.git`, `node_modules`, `cap_modules`, `.DS_Store`, `Thumbs.db`,
//      `desktop.ini`, caller-defined skip dirs, and paths exceeding `max_depth`;
//   2. `cap:simd`-accelerated relative path normalization & depth calculation
//      (`normalizeMaterializeRelPath`, `countMaterializePathDepth`) blocking
//      absolute paths, `..` traversal, NUL bytes, and Windows-illegal chars;
//   3. Incremental diff & byte comparison (`diffMaterializeEntries`,
//      `compareMaterializeContentBytes`) skipping unchanged `(size, mtime, hash)`
//      files and isolating mode-only updates;
//   4. Zero-heap topological batch planner (`FileMaterializePlanner`) emitting
//      `MkdirAll` -> `CloneFile`/`HardlinkFile`/`CopyFile` -> `ApplyExecBit`
//      with deterministic cross-device `EXDEV` fallback state tracking.

import { Simd } from "cap:simd";

/// Default maximum directory / symlink nesting depth to guard against symlink loops.
pub const FILE_MAT_DEFAULT_MAX_DEPTH: u32 = 32;

/// Maximum normalized relative path length handled by internal stack scratch buffers.
pub const FILE_MAT_MAX_PATH_LEN: usize = 512;

/// Standard POSIX mode for non-executable materialized files (rw-r--r--).
pub const FILE_MAT_MODE_FILE_DEFAULT: u32 = 0o644;

/// Standard POSIX executable bit mask (u+x, g+x, o+x).
pub const FILE_MAT_MODE_EXEC_MASK: u32 = 0o111;

/// Standard POSIX mode for executable files / bin targets (rwxr-xr-x).
pub const FILE_MAT_MODE_EXEC_DEFAULT: u32 = 0o755;

/// Standard POSIX mode for materialized directories (rwxr-xr-x).
pub const FILE_MAT_MODE_DIR_DEFAULT: u32 = 0o755;

/// Error set for file materialization planning, path validation, and diffing.
pub err FileMaterializeError {
    InvalidRelPath,
    PathTraversalDetected,
    SymlinkDepthExceeded,
    BufferTooSmall,
    CapacityExceeded,
}

// ============================================================================
// Internal Zero-Alloc Byte, Comparison, and Formatting Helpers
// ============================================================================

fn fmBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn fmLexLess(a: []const u8, b: []const u8): bool {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return a[i] < b[i];
        i += 1;
    }
    return a.len < b.len;
}

fn fmStartsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    return fmBytesEql(s[0..prefix.len], prefix);
}

fn fmAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn fmAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn fmAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        fmAppendByte(out, pos, '0')?;
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
        fmAppendByte(out, pos, tmp[n])?;
    }
}

fn fmHasWindowsIllegalChars(seg: []const u8): bool {
    if (Simd.indexOfAny4(seg, '*', '?', '"', '<') != null) return true;
    if (Simd.indexOfAny4(seg, '>', '|', ':', 0) != null) return true;
    return false;
}

fn fmIsDefaultSkipDir(seg: []const u8): bool {
    return fmBytesEql(seg, ".git") ||
        fmBytesEql(seg, "node_modules") ||
        fmBytesEql(seg, "cap_modules");
}

fn fmIsOsMetadataFile(name: []const u8): bool {
    return fmBytesEql(name, ".DS_Store") ||
        fmBytesEql(name, "Thumbs.db") ||
        fmBytesEql(name, "desktop.ini");
}

fn fmBasename(norm_path: []const u8): []const u8 {
    let i = norm_path.len;
    while (i > 0) {
        i -= 1;
        if (norm_path[i] == '/') return norm_path[i + 1..];
    }
    return norm_path;
}

fn fmDirname(norm_path: []const u8): []const u8 {
    let i = norm_path.len;
    while (i > 0) {
        i -= 1;
        if (norm_path[i] == '/') return norm_path[0..i];
    }
    return "";
}

// ============================================================================
// Section 1: SIMD Path Normalization, Depth Guard & Skip Filter
// ============================================================================

/// Filesystem entry classification for materialization inputs.
pub enum FileMatEntryKind {
    Directory,
    File,
    Symlink,
    Other,
}

/// Filter verdict returned by `shouldSkipMaterializeEntry`.
pub enum FileMatSkipReason {
    Keep,
    IgnoredDirName,
    IgnoredMetaFile,
    UnsupportedKind,
    DepthExceeded,
    UnsafePath,
}

/// Normalizes a package-relative path into canonical forward-slash form using
/// `cap:simd` (`Simd.indexOfAny2`, `Simd.indexOfAny4`). Rejects empty paths,
/// absolute paths, NUL bytes, Windows-illegal characters, and `..` traversal.
pub fn normalizeMaterializeRelPath(raw: []const u8, out: []u8): FileMaterializeError!usize {
    if (raw.len == 0) return err.InvalidRelPath;
    if (raw[0] == '/' || raw[0] == '\\') return err.InvalidRelPath;
    if (Simd.indexOfByte(raw, 0) != null) return err.InvalidRelPath;

    let pos: usize = 0;
    let out_pos: usize = 0;

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

        if (seg.len == 1 && seg[0] == '.') continue;
        if (seg.len == 2 && seg[0] == '.' && seg[1] == '.') {
            return err.PathTraversalDetected;
        }
        if (fmHasWindowsIllegalChars(seg)) {
            return err.InvalidRelPath;
        }

        if (out_pos > 0) {
            fmAppendByte(out, &out_pos, '/')?;
        }
        fmAppendBytes(out, &out_pos, seg)?;
    }

    if (out_pos == 0) return err.InvalidRelPath;
    return out_pos;
}

/// Counts nesting depth of a normalized relative path via `Simd.countByte(..., '/')`.
pub fn countMaterializePathDepth(norm_path: []const u8): u32 {
    if (norm_path.len == 0) return 0;
    return @as(u32, @intCast(Simd.countByte(norm_path, '/')));
}

/// Evaluates whether a source entry should be skipped during package materialization
/// (`.git`, `node_modules`, `cap_modules`, `.DS_Store`, `Thumbs.db`, `desktop.ini`,
/// caller-specified `extra_skip_dirs`, symlink/directory depth limits, or unsafe paths).
pub fn shouldSkipMaterializeEntry(
    rel_path: []const u8,
    kind: FileMatEntryKind,
    symlink_depth: u32,
    max_depth: u32,
    extra_skip_dirs: []const []const u8,
): FileMatSkipReason {
    if (kind == FileMatEntryKind.Other) {
        return FileMatSkipReason.UnsupportedKind;
    }

    let norm_buf: [FILE_MAT_MAX_PATH_LEN]u8 = [0; FILE_MAT_MAX_PATH_LEN];
    const norm_len = normalizeMaterializeRelPath(rel_path, norm_buf[0..]) catch {
        return FileMatSkipReason.UnsafePath;
    };
    const norm = norm_buf[0..norm_len];

    const limit = if (max_depth == 0) FILE_MAT_DEFAULT_MAX_DEPTH else max_depth;
    const depth = countMaterializePathDepth(norm);
    if (depth > limit || symlink_depth > limit) {
        return FileMatSkipReason.DepthExceeded;
    }

    // Check every path component against ignored directory names
    let pos: usize = 0;
    while (pos < norm.len) {
        let seg_end = norm.len;
        if (slash := Simd.indexOfByte(norm[pos..], '/')) {
            seg_end = pos + slash;
        }
        const seg = norm[pos..seg_end];
        const is_last = seg_end == norm.len;

        if (!is_last || kind == FileMatEntryKind.Directory) {
            if (fmIsDefaultSkipDir(seg)) {
                return FileMatSkipReason.IgnoredDirName;
            }
            let e: usize = 0;
            while (e < extra_skip_dirs.len) {
                if (fmBytesEql(seg, extra_skip_dirs[e])) {
                    return FileMatSkipReason.IgnoredDirName;
                }
                e += 1;
            }
        }
        pos = seg_end + 1;
    }

    if (kind == FileMatEntryKind.File && fmIsOsMetadataFile(fmBasename(norm))) {
        return FileMatSkipReason.IgnoredMetaFile;
    }

    return FileMatSkipReason.Keep;
}

/// Determines whether a materialized file requires executable permission bits,
/// checking original POSIX mode bits, `bin/` / `.bin/` placement, and `#!` Shebang.
pub fn needsExecPermission(rel_path: []const u8, posix_mode: u32, has_shebang: bool): bool {
    if ((posix_mode & FILE_MAT_MODE_EXEC_MASK) != 0) return true;
    if (has_shebang) return true;
    if (fmStartsWith(rel_path, "bin/") || fmStartsWith(rel_path, ".bin/")) return true;
    return false;
}

// ============================================================================
// Section 2: Byte-Level Content Diff & Incremental Skip Detection
// ============================================================================

/// Source file descriptor collected from the cached package archive/tree.
pub struct FileMatSourceEntry {
    rel_path: []const u8 = "",
    kind: FileMatEntryKind = FileMatEntryKind.File,
    size: u64 = 0,
    mtime_ns: i64 = 0,
    content_hash: u64 = 0,
    posix_mode: u32 = FILE_MAT_MODE_FILE_DEFAULT,
    has_shebang: bool = false,
    symlink_depth: u32 = 0,
}

/// Destination file metadata snapshot used for incremental skip checks.
pub struct FileMatExistingEntry {
    rel_path: []const u8 = "",
    size: u64 = 0,
    mtime_ns: i64 = 0,
    content_hash: u64 = 0,
    posix_mode: u32 = FILE_MAT_MODE_FILE_DEFAULT,
}

/// Per-file incremental diff classification.
pub enum FileMatDiffStatus {
    NewEntry,
    ModifiedContent,
    ModifiedModeOnly,
    SkipUnchanged,
}

/// Aggregate counters returned by `diffMaterializeEntries`.
pub struct FileMatDiffSummary {
    total_checked: usize = 0,
    unchanged_skipped: usize = 0,
    new_entries: usize = 0,
    modified_content: usize = 0,
    modified_mode_only: usize = 0,
    bytes_saved_by_skip: u64 = 0,
}

/// Returns the byte offset of the first difference between `a` and `b`, or `null` if identical.
pub fn compareMaterializeContentBytes(a: []const u8, b: []const u8): usize? {
    const min_len = if (a.len < b.len) a.len else b.len;
    let i: usize = 0;
    while (i < min_len) {
        if (a[i] != b[i]) return i;
        i += 1;
    }
    if (a.len != b.len) return min_len;
    return null;
}

fn fmFindExisting(
    norm_path: []const u8,
    existing: []const FileMatExistingEntry,
): usize? {
    let norm_ex: [FILE_MAT_MAX_PATH_LEN]u8 = [0; FILE_MAT_MAX_PATH_LEN];
    let i: usize = 0;
    while (i < existing.len) {
        const elen = normalizeMaterializeRelPath(existing[i].rel_path, norm_ex[0..]) catch {
            i += 1;
            continue;
        };
        if (fmBytesEql(norm_path, norm_ex[0..elen])) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Compares `sources` against destination `existing` snapshots to mark files whose
/// `(size, mtime_ns, content_hash)` and executable mode already match as `SkipUnchanged`.
pub fn diffMaterializeEntries(
    sources: []const FileMatSourceEntry,
    existing: []const FileMatExistingEntry,
    out_statuses: []FileMatDiffStatus,
): FileMaterializeError!FileMatDiffSummary {
    if (out_statuses.len < sources.len) return err.BufferTooSmall;
    let summary = FileMatDiffSummary{ total_checked: sources.len };

    let norm_buf: [FILE_MAT_MAX_PATH_LEN]u8 = [0; FILE_MAT_MAX_PATH_LEN];
    let i: usize = 0;
    while (i < sources.len) {
        const src = sources[i];
        const nlen = normalizeMaterializeRelPath(src.rel_path, norm_buf[0..]) catch {
            out_statuses[i] = FileMatDiffStatus.NewEntry;
            summary.new_entries += 1;
            i += 1;
            continue;
        };
        const norm = norm_buf[0..nlen];

        if (ex_idx := fmFindExisting(norm, existing)) {
            const ex = existing[ex_idx];
            const same_content = (src.size == ex.size) &&
                (src.mtime_ns == ex.mtime_ns) &&
                (src.content_hash == ex.content_hash);
            if (!same_content) {
                out_statuses[i] = FileMatDiffStatus.ModifiedContent;
                summary.modified_content += 1;
            } else {
                const want_exec = needsExecPermission(norm, src.posix_mode, src.has_shebang);
                const have_exec = (ex.posix_mode & FILE_MAT_MODE_EXEC_MASK) != 0;
                if (want_exec != have_exec) {
                    out_statuses[i] = FileMatDiffStatus.ModifiedModeOnly;
                    summary.modified_mode_only += 1;
                } else {
                    out_statuses[i] = FileMatDiffStatus.SkipUnchanged;
                    summary.unchanged_skipped += 1;
                    summary.bytes_saved_by_skip += src.size;
                }
            }
        } else {
            out_statuses[i] = FileMatDiffStatus.NewEntry;
            summary.new_entries += 1;
        }
        i += 1;
    }

    return summary;
}

// ============================================================================
// Section 3: Zero-Heap Topological Batch Planner & EXDEV Fallback Engine
// ============================================================================

/// Materialization strategy for transferring cached package files to destination.
pub enum FileMatStrategy {
    CloneFile,
    HardlinkFile,
    CopyFile,
}

/// Returns the canonical ASCII name of a `FileMatStrategy`.
pub fn fileMatStrategyName(s: FileMatStrategy): []const u8 {
    return match (s) {
        FileMatStrategy.CloneFile => "clonefile",
        FileMatStrategy.HardlinkFile => "hardlink",
        FileMatStrategy.CopyFile => "copyfile",
    };
}

/// Atomic filesystem operation emitted by `FileMaterializePlanner.planBatch`.
pub enum FileMatOpKind {
    MkdirAll,
    CloneFile,
    HardlinkFile,
    CopyFile,
    ApplyExecBit,
    SkipUnchanged,
    SkipFiltered,
}

/// Compact 8-byte `(offset, len)` handle into caller-provided `path_arena`.
pub struct FileMatSpan {
    offset: u32 = 0,
    len: u32 = 0,

    static fn empty(): FileMatSpan {
        return FileMatSpan{ offset: 0, len: 0 };
    }
}

/// Resolves a `FileMatSpan` against `path_arena`.
pub fn getFileMatOpPath(span: FileMatSpan, path_arena: []const u8): []const u8 {
    if (span.len == 0) return "";
    const off = @as(usize, span.offset);
    const l = @as(usize, span.len);
    if (off > path_arena.len || l > path_arena.len - off) return "";
    return path_arena[off..off + l];
}

/// Single planned materialization operation.
pub struct FileMatOp {
    kind: FileMatOpKind = FileMatOpKind.SkipUnchanged,
    rel_path: FileMatSpan = FileMatSpan.empty(),
    depth: u32 = 0,
    target_mode: u32 = FILE_MAT_MODE_FILE_DEFAULT,
    size_bytes: u64 = 0,

    static fn empty(): FileMatOp {
        return FileMatOp{};
    }
}

/// Options governing `FileMaterializePlanner.planBatch`.
pub struct FileMatPlanOptions {
    max_depth: u32 = FILE_MAT_DEFAULT_MAX_DEPTH,
    preserve_exec_bits: bool = true,
    skip_unchanged: bool = true,

    static fn defaults(): FileMatPlanOptions {
        return FileMatPlanOptions{};
    }
}

/// Aggregate telemetry returned by `FileMaterializePlanner.planBatch`.
pub struct FileMatPlanSummary {
    active_strategy: FileMatStrategy = FileMatStrategy.HardlinkFile,
    ops_count: usize = 0,
    mkdir_ops: usize = 0,
    file_ops: usize = 0,
    chmod_ops: usize = 0,
    skipped_unchanged: usize = 0,
    skipped_filtered: usize = 0,
    total_bytes_to_materialize: u64 = 0,
    bytes_saved_by_skip: u64 = 0,
    exdev_fallbacks: u32 = 0,
    arena_bytes_used: usize = 0,
}

fn fmArenaPut(arena: []u8, pos: *usize, s: []const u8): FileMaterializeError!FileMatSpan {
    if (s.len == 0) return FileMatSpan.empty();
    if (s.len > arena.len - pos.*) return err.BufferTooSmall;
    const start = pos.*;
    fmAppendBytes(arena, pos, s)?;
    return FileMatSpan{
        offset: @as(u32, @intCast(start)),
        len: @as(u32, @intCast(s.len)),
    };
}

fn fmEmitOp(out_ops: []FileMatOp, count: *usize, op: FileMatOp)! {
    if (count.* >= out_ops.len) return err.CapacityExceeded;
    out_ops[count.*] = op;
    count.* += 1;
}

fn fmDirAlreadyPlanned(out_ops: []const FileMatOp, count: usize, path_arena: []const u8, dir_path: []const u8): bool {
    let i: usize = 0;
    while (i < count) {
        if (out_ops[i].kind == FileMatOpKind.MkdirAll) {
            if (fmBytesEql(getFileMatOpPath(out_ops[i].rel_path, path_arena), dir_path)) {
                return true;
            }
        }
        i += 1;
    }
    return false;
}

fn fmEntryBefore(a: FileMatSourceEntry, b: FileMatSourceEntry): bool {
    let na: [FILE_MAT_MAX_PATH_LEN]u8 = [0; FILE_MAT_MAX_PATH_LEN];
    let nb: [FILE_MAT_MAX_PATH_LEN]u8 = [0; FILE_MAT_MAX_PATH_LEN];
    const la = normalizeMaterializeRelPath(a.rel_path, na[0..]) catch 0;
    const lb = normalizeMaterializeRelPath(b.rel_path, nb[0..]) catch 0;
    const da = countMaterializePathDepth(na[0..la]);
    const db = countMaterializePathDepth(nb[0..lb]);
    if (da != db) return da < db;
    const a_dir = a.kind == FileMatEntryKind.Directory;
    const b_dir = b.kind == FileMatEntryKind.Directory;
    if (a_dir != b_dir) return a_dir;
    return fmLexLess(na[0..la], nb[0..lb]);
}

fn fmFileOpForStrategy(strategy: FileMatStrategy): FileMatOpKind {
    return match (strategy) {
        FileMatStrategy.CloneFile => FileMatOpKind.CloneFile,
        FileMatStrategy.HardlinkFile => FileMatOpKind.HardlinkFile,
        FileMatStrategy.CopyFile => FileMatOpKind.CopyFile,
    };
}

/// Stateful, zero-heap package file materialization planner with automatic
/// cross-device (`EXDEV`) strategy downgrade (`CloneFile -> HardlinkFile -> CopyFile`).
pub struct FileMaterializePlanner {
    strategy: FileMatStrategy = FileMatStrategy.HardlinkFile,
    exdev_fallbacks: u32 = 0,

    static fn init(initial_strategy: FileMatStrategy): FileMaterializePlanner {
        return FileMaterializePlanner{
            strategy: initial_strategy,
            exdev_fallbacks: 0,
        };
    }

    /// Downgrades the active strategy upon encountering a cross-device `EXDEV`
    /// or unsupported filesystem error, returning the newly active strategy.
    fn onCrossDeviceExdev(*_): FileMatStrategy {
        if (_.strategy == FileMatStrategy.CloneFile) {
            _.strategy = FileMatStrategy.HardlinkFile;
            _.exdev_fallbacks += 1;
        } else if (_.strategy == FileMatStrategy.HardlinkFile) {
            _.strategy = FileMatStrategy.CopyFile;
            _.exdev_fallbacks += 1;
        }
        return _.strategy;
    }

    /// Generates a topologically ordered batch of `MkdirAll` -> file transfer ->
    /// `ApplyExecBit` operations into caller-provided `out_ops` and `path_arena`.
    fn planBatch(
        sources: []const FileMatSourceEntry,
        existing: []const FileMatExistingEntry,
        extra_skip_dirs: []const []const u8,
        opts: FileMatPlanOptions,
        scratch_order: []usize,
        out_ops: []FileMatOp,
        path_arena: []u8,
    ): FileMaterializeError!FileMatPlanSummary {
        if (scratch_order.len < sources.len) return err.BufferTooSmall;

        let summary = FileMatPlanSummary{
            active_strategy: _.strategy,
            exdev_fallbacks: _.exdev_fallbacks,
        };
        let ops_count: usize = 0;
        let arena_pos: usize = 0;

        // Pass 1: Filter skipped entries and collect kept indices
        let kept_count: usize = 0;
        let i: usize = 0;
        while (i < sources.len) {
            const src = sources[i];
            const verdict = shouldSkipMaterializeEntry(
                src.rel_path,
                src.kind,
                src.symlink_depth,
                opts.max_depth,
                extra_skip_dirs,
            );
            if (verdict != FileMatSkipReason.Keep) {
                summary.skipped_filtered += 1;
                i += 1;
                continue;
            }
            scratch_order[kept_count] = i;
            kept_count += 1;
            i += 1;
        }

        // Pass 2: Topological depth-first sort (shallowest dirs first, then files)
        let s: usize = 1;
        while (s < kept_count) {
            const key_idx = scratch_order[s];
            const key_entry = sources[key_idx];
            let j = s;
            while (j > 0 && fmEntryBefore(key_entry, sources[scratch_order[j - 1]])) {
                scratch_order[j] = scratch_order[j - 1];
                j -= 1;
            }
            scratch_order[j] = key_idx;
            s += 1;
        }

        // Pass 3: Emit deduplicated MkdirAll + file transfer + ApplyExecBit ops
        let norm_buf: [FILE_MAT_MAX_PATH_LEN]u8 = [0; FILE_MAT_MAX_PATH_LEN];
        let k: usize = 0;
        while (k < kept_count) {
            const src = sources[scratch_order[k]];
            const nlen = normalizeMaterializeRelPath(src.rel_path, norm_buf[0..])?;
            const norm = norm_buf[0..nlen];
            const depth = countMaterializePathDepth(norm);

            if (src.kind == FileMatEntryKind.Directory) {
                if (!fmDirAlreadyPlanned(out_ops, ops_count, path_arena, norm)) {
                    const span = fmArenaPut(path_arena, &arena_pos, norm)?;
                    fmEmitOp(out_ops, &ops_count, FileMatOp{
                        kind: FileMatOpKind.MkdirAll,
                        rel_path: span,
                        depth: depth,
                        target_mode: FILE_MAT_MODE_DIR_DEFAULT,
                        size_bytes: 0,
                    })?;
                    summary.mkdir_ops += 1;
                }
                k += 1;
                continue;
            }

            // Ensure parent directory is planned before creating nested file
            const parent = fmDirname(norm);
            if (parent.len > 0 && !fmDirAlreadyPlanned(out_ops, ops_count, path_arena, parent)) {
                const pspan = fmArenaPut(path_arena, &arena_pos, parent)?;
                fmEmitOp(out_ops, &ops_count, FileMatOp{
                    kind: FileMatOpKind.MkdirAll,
                    rel_path: pspan,
                    depth: if (depth > 0) depth - 1 else 0,
                    target_mode: FILE_MAT_MODE_DIR_DEFAULT,
                    size_bytes: 0,
                })?;
                summary.mkdir_ops += 1;
            }

            // Incremental skip check against existing destination entries
            const want_exec = opts.preserve_exec_bits && needsExecPermission(norm, src.posix_mode, src.has_shebang);
            const target_mode: u32 = if (want_exec)
                (src.posix_mode | FILE_MAT_MODE_EXEC_DEFAULT)
            else
                FILE_MAT_MODE_FILE_DEFAULT;

            if (opts.skip_unchanged) {
                if (ex_idx := fmFindExisting(norm, existing)) {
                    const ex = existing[ex_idx];
                    const same_content = (src.size == ex.size) &&
                        (src.mtime_ns == ex.mtime_ns) &&
                        (src.content_hash == ex.content_hash);
                    if (same_content) {
                        const have_exec = (ex.posix_mode & FILE_MAT_MODE_EXEC_MASK) != 0;
                        const span = fmArenaPut(path_arena, &arena_pos, norm)?;
                        if (want_exec != have_exec) {
                            fmEmitOp(out_ops, &ops_count, FileMatOp{
                                kind: FileMatOpKind.ApplyExecBit,
                                rel_path: span,
                                depth: depth,
                                target_mode: target_mode,
                                size_bytes: 0,
                            })?;
                            summary.chmod_ops += 1;
                        } else {
                            fmEmitOp(out_ops, &ops_count, FileMatOp{
                                kind: FileMatOpKind.SkipUnchanged,
                                rel_path: span,
                                depth: depth,
                                target_mode: target_mode,
                                size_bytes: src.size,
                            })?;
                            summary.skipped_unchanged += 1;
                            summary.bytes_saved_by_skip += src.size;
                        }
                        k += 1;
                        continue;
                    }
                }
            }

            // Emit file transfer op (CloneFile / HardlinkFile / CopyFile)
            const fspan = fmArenaPut(path_arena, &arena_pos, norm)?;
            fmEmitOp(out_ops, &ops_count, FileMatOp{
                kind: fmFileOpForStrategy(_.strategy),
                rel_path: fspan,
                depth: depth,
                target_mode: target_mode,
                size_bytes: src.size,
            })?;
            summary.file_ops += 1;
            summary.total_bytes_to_materialize += src.size;

            // Emit ApplyExecBit when executable permission must be preserved
            if (want_exec) {
                fmEmitOp(out_ops, &ops_count, FileMatOp{
                    kind: FileMatOpKind.ApplyExecBit,
                    rel_path: fspan,
                    depth: depth,
                    target_mode: target_mode,
                    size_bytes: 0,
                })?;
                summary.chmod_ops += 1;
            }

            k += 1;
        }

        summary.ops_count = ops_count;
        summary.arena_bytes_used = arena_pos;
        return summary;
    }
}

// ============================================================================
// Section 4: Diagnostic Summary Report Formatter
// ============================================================================

/// Formats a concise diagnostic summary of materialization batch statistics.
pub fn formatFileMaterializeReport(summary: FileMatPlanSummary, out: []u8): FileMaterializeError!usize {
    let pos: usize = 0;
    fmAppendBytes(out, &pos, "FileMaterialize(strategy=")?;
    fmAppendBytes(out, &pos, fileMatStrategyName(summary.active_strategy))?;
    fmAppendBytes(out, &pos, ", mkdir=")?;
    fmAppendU64(out, &pos, @as(u64, summary.mkdir_ops))?;
    fmAppendBytes(out, &pos, ", files=")?;
    fmAppendU64(out, &pos, @as(u64, summary.file_ops))?;
    fmAppendBytes(out, &pos, ", chmod=")?;
    fmAppendU64(out, &pos, @as(u64, summary.chmod_ops))?;
    fmAppendBytes(out, &pos, ", skipped=")?;
    fmAppendU64(out, &pos, @as(u64, summary.skipped_unchanged))?;
    fmAppendBytes(out, &pos, ", filtered=")?;
    fmAppendU64(out, &pos, @as(u64, summary.skipped_filtered))?;
    fmAppendBytes(out, &pos, ", bytes=")?;
    fmAppendU64(out, &pos, summary.total_bytes_to_materialize)?;
    fmAppendBytes(out, &pos, ", saved=")?;
    fmAppendU64(out, &pos, summary.bytes_saved_by_skip)?;
    fmAppendByte(out, &pos, ')')?;
    return pos;
}
