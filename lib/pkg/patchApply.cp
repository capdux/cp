// Source: Absorbed from /data/dev/bun/src/install/patch_install.cp | Synced: 2026-11-22T12:19:29Z
// ============================================================================
// lib/pkg/patchApply.cp — Industrial Zero-Alloc Unified Diff / Git Patch Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven Unified Diff & Git Patch parser,
// path-traversal sanitizer, hunk applier (forward + reverse rollback),
// bounded offset-window context locator, dry-run validator, summary reporter,
// and atomic patch-install pipeline state machine.

/// Default bounded line-offset search window radius when exact hunk line fails.
pub const PATCH_DEFAULT_MAX_FUZZ_OFFSET: u32 = 64;

/// Maximum safe relative path length accepted in patch headers.
pub const PATCH_MAX_PATH_LEN: usize = 4096;

/// Conflict reason codes stored in `PatchConflictInfo.reason_code`.
pub const PATCH_CONFLICT_NONE: u8 = 0;
pub const PATCH_CONFLICT_CONTEXT_MISMATCH: u8 = 1;
pub const PATCH_CONFLICT_OUT_OF_BOUNDS: u8 = 2;
pub const PATCH_CONFLICT_BINARY_UNSUPPORTED: u8 = 3;
pub const PATCH_CONFLICT_FILE_ALREADY_EXISTS: u8 = 4;
pub const PATCH_CONFLICT_FILE_NOT_FOUND: u8 = 5;
pub const PATCH_CONFLICT_BUFFER_TOO_SMALL: u8 = 6;

/// Unified diff / Git patch parsing, validation, and application error set.
pub err PatchApplyError {
    EmptyPatch,
    InvalidDiffHeader,
    InvalidHunkHeader,
    InvalidHunkLine,
    HunkLineCountMismatch,
    BinaryPatchUnsupported,
    UnsafePatchPath,
    BufferTooSmall,
    SliceCapacityExceeded,
    ContextMismatch,
    FileAlreadyExists,
    FileNotFound,
    HunkOutOfBounds,
    InvalidLineRange,
    InvalidStageTransition,
}

/// Classification of a file entry inside a multi-file Unified Diff / Git patch.
pub enum PatchFileKind: u8 {
    Modify = 0,
    Create = 1,
    Delete = 2,
    Rename = 3,
    ModeChange = 4,
    Binary = 5,
}

/// Line-level operation inside a unified diff hunk.
pub enum PatchLineOpKind: u8 {
    Context = 0,
    Delete = 1,
    Add = 2,
}

/// Single line operation inside a hunk (zero-copy slice into patch text).
pub struct PatchLineOp {
    kind: PatchLineOpKind = PatchLineOpKind.Context,
    text: []const u8 = "",
    no_newline: bool = false,
    patch_line_no: u32 = 0,

    static fn empty(): PatchLineOp {
        return PatchLineOp{};
    }
}

/// Parsed `@@ -old_start,old_count +new_start,new_count @@` hunk descriptor.
pub struct PatchHunk {
    old_start: u32 = 0,
    old_count: u32 = 0,
    new_start: u32 = 0,
    new_count: u32 = 0,
    header_context: []const u8 = "",
    ops_start: u32 = 0,
    ops_len: u32 = 0,
    patch_line_no: u32 = 0,

    static fn empty(): PatchHunk {
        return PatchHunk{};
    }

    fn addedCount(ops: []const PatchLineOp): u32 {
        let count: u32 = 0;
        let i: u32 = 0;
        while (i < _.ops_len) {
            const idx: usize = @intCast(_.ops_start + i);
            if (idx < ops.len && ops[idx].kind == PatchLineOpKind.Add) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    fn deletedCount(ops: []const PatchLineOp): u32 {
        let count: u32 = 0;
        let i: u32 = 0;
        while (i < _.ops_len) {
            const idx: usize = @intCast(_.ops_start + i);
            if (idx < ops.len && ops[idx].kind == PatchLineOpKind.Delete) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }

    fn contextCount(ops: []const PatchLineOp): u32 {
        let count: u32 = 0;
        let i: u32 = 0;
        while (i < _.ops_len) {
            const idx: usize = @intCast(_.ops_start + i);
            if (idx < ops.len && ops[idx].kind == PatchLineOpKind.Context) {
                count += 1;
            }
            i += 1;
        }
        return count;
    }
}

/// Parsed per-file patch descriptor referencing a slice range in caller's `[]PatchHunk`.
pub struct PatchFileEntry {
    kind: PatchFileKind = PatchFileKind.Modify,
    old_path: []const u8 = "",
    new_path: []const u8 = "",
    old_mode: u32 = 0,
    new_mode: u32 = 0,
    is_binary: bool = false,
    hunks_start: u32 = 0,
    hunks_len: u32 = 0,
    added_lines: u32 = 0,
    deleted_lines: u32 = 0,
    patch_line_no: u32 = 0,

    static fn empty(): PatchFileEntry {
        return PatchFileEntry{};
    }

    /// Returns the primary target path for this file entry.
    fn effectivePath(): []const u8 {
        if (_.kind == PatchFileKind.Delete) {
            return _.old_path;
        }
        if (_.new_path.len > 0) {
            return _.new_path;
        }
        return _.old_path;
    }

    /// Returns true if the file entry carries a POSIX mode change.
    fn hasModeChange(): bool {
        return _.old_mode != 0 && _.new_mode != 0 && _.old_mode != _.new_mode;
    }
}

/// Parser behavior options for `parseUnifiedPatch`.
pub struct PatchParseOptions {
    allow_binary_markers: bool = false,
    strip_prefix_level: u8 = 1,
    strict_hunk_counts: bool = true,

    static fn defaults(): PatchParseOptions {
        return PatchParseOptions{};
    }
}

/// Aggregate statistics and 64-bit content digest of a parsed patch.
pub struct PatchSummary {
    total_files: u32 = 0,
    created_files: u32 = 0,
    deleted_files: u32 = 0,
    modified_files: u32 = 0,
    renamed_files: u32 = 0,
    mode_only_files: u32 = 0,
    binary_files: u32 = 0,
    total_hunks: u32 = 0,
    total_added_lines: u32 = 0,
    total_deleted_lines: u32 = 0,
    content_hash64: u64 = 0,

    static fn empty(): PatchSummary {
        return PatchSummary{};
    }
}

/// Zero-alloc handle returned by `parseUnifiedPatch`.
pub struct ParsedUnifiedPatch {
    files_len: usize = 0,
    hunks_len: usize = 0,
    ops_len: usize = 0,
    summary: PatchSummary = PatchSummary.empty(),

    fn fileCount(): usize {
        return _.files_len;
    }

    fn hunkCount(): usize {
        return _.hunks_len;
    }

    fn opCount(): usize {
        return _.ops_len;
    }
}

/// Options controlling patch application, rollback, and offset search.
pub struct PatchApplyOptions {
    max_offset_lines: u32 = PATCH_DEFAULT_MAX_FUZZ_OFFSET,
    reverse: bool = false,
    ignore_trailing_cr: bool = true,
    preserve_line_ending: bool = true,

    static fn forward(): PatchApplyOptions {
        return PatchApplyOptions{
            max_offset_lines: PATCH_DEFAULT_MAX_FUZZ_OFFSET,
            reverse: false,
            ignore_trailing_cr: true,
            preserve_line_ending: true,
        };
    }

    static fn reverseMode(): PatchApplyOptions {
        return PatchApplyOptions{
            max_offset_lines: PATCH_DEFAULT_MAX_FUZZ_OFFSET,
            reverse: true,
            ignore_trailing_cr: true,
            preserve_line_ending: true,
        };
    }

    static fn strictExact(): PatchApplyOptions {
        return PatchApplyOptions{
            max_offset_lines: 0,
            reverse: false,
            ignore_trailing_cr: true,
            preserve_line_ending: true,
        };
    }
}

/// Precise conflict diagnostic populated when a hunk fails to apply.
pub struct PatchConflictInfo {
    has_conflict: bool = false,
    file_index: u32 = 0,
    hunk_index: u32 = 0,
    patch_line_no: u32 = 0,
    expected_target_line: u32 = 0,
    applied_offset: i32 = 0,
    reason_code: u8 = PATCH_CONFLICT_NONE,

    static fn none(): PatchConflictInfo {
        return PatchConflictInfo{};
    }
}

/// Result metrics produced by `applyFilePatch` / `applyUnifiedPatch` / `reverseUnifiedPatch`.
pub struct PatchApplyResult {
    output_len: usize = 0,
    hunks_applied: u32 = 0,
    lines_added: u32 = 0,
    lines_deleted: u32 = 0,
    max_abs_offset: u32 = 0,
    has_trailing_newline: bool = false,
    conflict: PatchConflictInfo = PatchConflictInfo.none(),

    static fn empty(): PatchApplyResult {
        return PatchApplyResult{};
    }
}

/// Non-mutating validation report returned by `dryRunPatch`.
pub struct PatchDryRunReport {
    can_apply: bool = false,
    hunks_checked: u32 = 0,
    hunks_matched_exact: u32 = 0,
    hunks_matched_with_offset: u32 = 0,
    estimated_output_bytes: usize = 0,
    conflict: PatchConflictInfo = PatchConflictInfo.none(),

    static fn empty(): PatchDryRunReport {
        return PatchDryRunReport{};
    }
}

/// Zero-copy line slice descriptor for caller-provided line scratch buffers.
pub struct SourceLineSpan {
    text: []const u8 = "",
    has_cr: bool = false,
    has_lf: bool = false,

    static fn empty(): SourceLineSpan {
        return SourceLineSpan{};
    }
}

/// Six-stage atomic patch installation pipeline state (modeled after Bun `PatchTask.apply`).
pub enum PatchInstallStage: u8 {
    Init = 0,
    Parsed = 1,
    Staged = 2,
    Applied = 3,
    Tagged = 4,
    Committed = 5,
    Failed = 6,
}

/// Pure-CP state machine coordinating patch hash calculation, dry-run validation,
/// and atomic staging-to-cache commit transitions.
pub struct PatchInstallSession {
    stage: PatchInstallStage = PatchInstallStage.Init,
    patch_hash: u64 = 0,
    files_total: u32 = 0,
    files_applied: u32 = 0,
    hunks_applied: u32 = 0,
    last_conflict: PatchConflictInfo = PatchConflictInfo.none(),

    static fn init(): PatchInstallSession {
        return PatchInstallSession{};
    }

    fn markParsed(*_, parsed: ParsedUnifiedPatch)! {
        if (_.stage != PatchInstallStage.Init) {
            return PatchApplyError.InvalidStageTransition;
        }
        _.patch_hash = parsed.summary.content_hash64;
        _.files_total = parsed.summary.total_files;
        _.stage = PatchInstallStage.Parsed;
    }

    fn markStaged(*_)! {
        if (_.stage != PatchInstallStage.Parsed) {
            return PatchApplyError.InvalidStageTransition;
        }
        _.stage = PatchInstallStage.Staged;
    }

    fn recordFileApplied(*_, res: PatchApplyResult)! {
        if (_.stage != PatchInstallStage.Staged && _.stage != PatchInstallStage.Applied) {
            return PatchApplyError.InvalidStageTransition;
        }
        _.files_applied += 1;
        _.hunks_applied += res.hunks_applied;
        if (_.files_applied >= _.files_total) {
            _.stage = PatchInstallStage.Applied;
        }
    }

    fn markTagged(*_)! {
        if (_.stage != PatchInstallStage.Applied) {
            return PatchApplyError.InvalidStageTransition;
        }
        _.stage = PatchInstallStage.Tagged;
    }

    fn markCommitted(*_)! {
        if (_.stage != PatchInstallStage.Tagged) {
            return PatchApplyError.InvalidStageTransition;
        }
        _.stage = PatchInstallStage.Committed;
    }

    fn markFailed(*_, conflict: PatchConflictInfo) {
        _.last_conflict = conflict;
        _.stage = PatchInstallStage.Failed;
    }

    fn isCommitted(): bool {
        return _.stage == PatchInstallStage.Committed;
    }
}

// ============================================================================
// Internal Byte & String Utilities (Zero-Alloc)
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

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (s.len < suffix.len) return false;
    const off = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[off + i] != suffix[i]) return false;
        i += 1;
    }
    return true;
}

fn indexOfByte(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn indexOfSubstr(haystack: []const u8, needle: []const u8): usize? {
    if (needle.len == 0) return 0;
    if (haystack.len < needle.len) return null;
    const limit = haystack.len - needle.len;
    let i: usize = 0;
    while (i <= limit) {
        let j: usize = 0;
        let matched = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                matched = false;
                break;
            }
            j += 1;
        }
        if (matched) return i;
        i += 1;
    }
    return null;
}

fn trimRightCr(line: []const u8): []const u8 {
    if (line.len > 0 && line[line.len - 1] == '\r') {
        return line[0 .. line.len - 1];
    }
    return line;
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r')) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        end -= 1;
    }
    return s[start..end];
}

fn fnv1a64Update(hash: u64, data: []const u8): u64 {
    let h = hash;
    let i: usize = 0;
    while (i < data.len) {
        h = h ^ @as(u64, data[i]);
        h = h *% 1099511628211;
        i += 1;
    }
    return h;
}

fn parseU32Decimal(s: []const u8): u32? {
    if (s.len == 0) return null;
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        val = val * 10 + @as(u64, c - '0');
        if (val > 4294967295) return null;
        i += 1;
    }
    return @intCast(val);
}

/// Parses an octal POSIX file mode such as `"100644"` or `"100755"`.
pub fn parseFileModeOctal(raw: []const u8): u32? {
    const s = trimSpaces(raw);
    if (s.len == 0 || s.len > 8) return null;
    let val: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '7') return null;
        val = (val << 3) | @as(u32, c - '0');
        i += 1;
    }
    return val;
}

// ============================================================================
// Path Safety & Header Path Sanitization
// ============================================================================

/// Validates that a relative path extracted from a patch header cannot escape
/// the target package directory. Rejects empty paths, leading slashes, backslashes,
/// NUL bytes, Windows drive prefixes (`C:`), `.` / `..` segments, and oversized paths.
pub fn isSafePatchRelPath(path: []const u8): bool {
    if (path.len == 0 || path.len > PATCH_MAX_PATH_LEN) return false;
    if (path[0] == '/' || path[0] == '\\') return false;
    // Reject Windows drive prefix like "C:/foo" or "C:foo"
    if (path.len >= 2 && path[1] == ':') {
        const c0 = path[0];
        if ((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z')) {
            return false;
        }
    }

    let seg_start: usize = 0;
    let i: usize = 0;
    while (i <= path.len) {
        const at_end = (i == path.len);
        if (!at_end) {
            const c = path[i];
            if (c == 0 || c == '\\' || c == '\r' || c == '\n') {
                return false;
            }
        }
        if (at_end || path[i] == '/') {
            const seg = path[seg_start..i];
            if (seg.len == 0) return false;
            if (bytesEqual(seg, ".") || bytesEqual(seg, "..")) return false;
            seg_start = i + 1;
        }
        i += 1;
    }
    return true;
}

/// Strips optional tab timestamp metadata, surrounding quotes, and `-pN` prefix
/// components from a `---`/`+++`/`rename` header path.
/// Returns `""` when the path is `/dev/null`; otherwise validates safety strictly.
pub fn sanitizePatchHeaderPath(raw: []const u8, strip_level: u8): ([]const u8)! {
    let s = trimRightCr(raw);
    // Strip optional timestamp after first TAB (standard unified diff: `--- a/file\t2026-...`)
    if (tab_idx := indexOfByte(s, '\t')) {
        s = s[0..tab_idx];
    }
    s = trimSpaces(s);
    if (s.len == 0) {
        return PatchApplyError.UnsafePatchPath;
    }
    // Strip surrounding double quotes if present
    if (s.len >= 2 && s[0] == '"' && s[s.len - 1] == '"') {
        s = s[1 .. s.len - 1];
    }
    if (bytesEqual(s, "/dev/null") || bytesEqual(s, "dev/null")) {
        return "";
    }
    // Reject explicit absolute paths before stripping (e.g. `--- /etc/passwd`)
    if (s[0] == '/' || s[0] == '\\') {
        return PatchApplyError.UnsafePatchPath;
    }

    // Apply -pN stripping (e.g. strip_level=1 strips `a/` or `b/`)
    let stripped = s;
    let level: u8 = 0;
    while (level < strip_level) {
        if (slash_idx := indexOfByte(stripped, '/')) {
            const prefix_seg = stripped[0..slash_idx];
            if (bytesEqual(prefix_seg, "..")) {
                return PatchApplyError.UnsafePatchPath;
            }
            stripped = stripped[slash_idx + 1 ..];
            level += 1;
        } else {
            break;
        }
    }

    // Also strip redundant leading "./" after -p stripping if present
    while (startsWith(stripped, "./")) {
        stripped = stripped[2..];
    }

    if (!isSafePatchRelPath(stripped)) {
        return PatchApplyError.UnsafePatchPath;
    }
    return stripped;
}

// ============================================================================
// Hunk Header Parser (`@@ -old_start,old_count +new_start,new_count @@ ...`)
// ============================================================================

fn parseRangePair(spec: []const u8): ([2]u32)! {
    if (spec.len == 0) {
        return PatchApplyError.InvalidHunkHeader;
    }
    if (comma := indexOfByte(spec, ',')) {
        const start_opt = parseU32Decimal(spec[0..comma]);
        const count_opt = parseU32Decimal(spec[comma + 1 ..]);
        if (start_opt == null || count_opt == null) {
            return PatchApplyError.InvalidHunkHeader;
        }
        return [start_opt.?, count_opt.?];
    } else {
        const start_opt = parseU32Decimal(spec);
        if (start_opt == null) {
            return PatchApplyError.InvalidHunkHeader;
        }
        // Per unified diff specification, omitted `,count` defaults to 1.
        return [start_opt.?, 1];
    }
}

/// Parses a single `@@ -old_start[,old_count] +new_start[,new_count] @@[ context]` header line.
pub fn parseHunkHeaderLine(raw_line: []const u8, patch_line_no: u32): PatchHunk! {
    const line = trimRightCr(raw_line);
    if (!startsWith(line, "@@ -")) {
        return PatchApplyError.InvalidHunkHeader;
    }
    const rest = line[4..];
    const close_rel = indexOfSubstr(rest, " @@");
    if (close_rel == null) {
        return PatchApplyError.InvalidHunkHeader;
    }
    const range_part = rest[0..close_rel.?];
    const after_close = rest[close_rel.? + 3 ..];

    const plus_rel = indexOfSubstr(range_part, " +");
    if (plus_rel == null) {
        return PatchApplyError.InvalidHunkHeader;
    }
    const old_spec = range_part[0..plus_rel.?];
    const new_spec = range_part[plus_rel.? + 2 ..];

    const old_pair = parseRangePair(old_spec)?;
    const new_pair = parseRangePair(new_spec)?;

    let ctx: []const u8 = "";
    if (after_close.len > 0) {
        if (after_close[0] == ' ') {
            ctx = after_close[1..];
        } else {
            ctx = after_close;
        }
    }

    return PatchHunk{
        old_start: old_pair[0],
        old_count: old_pair[1],
        new_start: new_pair[0],
        new_count: new_pair[1],
        header_context: ctx,
        ops_start: 0,
        ops_len: 0,
        patch_line_no: patch_line_no,
    };
}

// ============================================================================
// Multi-File Unified Diff / Git Patch Parser
// ============================================================================

struct PatchLineIter {
    data: []const u8,
    pos: usize = 0,
    line_no: u32 = 0,

    fn next(*_): ([]const u8)? {
        if (_.pos >= _.data.len) return null;
        const start = _.pos;
        while (_.pos < _.data.len && _.data[_.pos] != '\n') {
            _.pos += 1;
        }
        const end = _.pos;
        if (_.pos < _.data.len && _.data[_.pos] == '\n') {
            _.pos += 1;
        }
        _.line_no += 1;
        return trimRightCr(_.data[start..end]);
    }
}

fn parseDiffGitPaths(line: []const u8, strip_level: u8): ([2][]const u8)! {
    // Format: `diff --git a/path1 b/path2`
    const rest = trimSpaces(line[11..]);
    if (rest.len == 0) {
        return PatchApplyError.InvalidDiffHeader;
    }
    // Locate separator `" b/"` when standard git prefix is used, otherwise split on first space.
    let split_idx: usize? = null;
    if (b_pos := indexOfSubstr(rest, " b/")) {
        split_idx = b_pos;
    } else if (sp_pos := indexOfByte(rest, ' ')) {
        split_idx = sp_pos;
    }
    if (split_idx == null) {
        return PatchApplyError.InvalidDiffHeader;
    }
    const raw_a = rest[0..split_idx.?];
    const raw_b = trimSpaces(rest[split_idx.? + 1 ..]);
    const path_a = sanitizePatchHeaderPath(raw_a, strip_level)?;
    const path_b = sanitizePatchHeaderPath(raw_b, strip_level)?;
    return [path_a, path_b];
}

fn finalizeFileEntry(entry: *PatchFileEntry, summary: *PatchSummary) {
    if (entry.is_binary) {
        entry.kind = PatchFileKind.Binary;
        summary.binary_files += 1;
    } else if (entry.kind == PatchFileKind.Create) {
        summary.created_files += 1;
    } else if (entry.kind == PatchFileKind.Delete) {
        summary.deleted_files += 1;
    } else if (entry.kind == PatchFileKind.Rename) {
        summary.renamed_files += 1;
    } else if (entry.hunks_len == 0 && entry.hasModeChange()) {
        entry.kind = PatchFileKind.ModeChange;
        summary.mode_only_files += 1;
    } else {
        summary.modified_files += 1;
    }
    summary.total_files += 1;
    summary.total_hunks += entry.hunks_len;
    summary.total_added_lines += entry.added_lines;
    summary.total_deleted_lines += entry.deleted_lines;
}

/// Parses a complete multi-file Unified Diff or Git Patch into caller-owned slices.
/// Performs zero heap allocations and enforces strict path-traversal protection.
pub fn parseUnifiedPatch(
    patch_text: []const u8,
    opts: PatchParseOptions,
    files_out: []PatchFileEntry,
    hunks_out: []PatchHunk,
    ops_out: []PatchLineOp,
): ParsedUnifiedPatch! {
    if (trimSpaces(patch_text).len == 0) {
        return PatchApplyError.EmptyPatch;
    }

    let iter = PatchLineIter{ data: patch_text };
    let files_len: usize = 0;
    let hunks_len: usize = 0;
    let ops_len: usize = 0;
    let summary = PatchSummary.empty();
    summary.content_hash64 = fnv1a64Update(14695981039346656037, patch_text);

    let has_active_file = false;
    let cur_file = PatchFileEntry.empty();

    let in_hunk = false;
    let cur_hunk = PatchHunk.empty();
    let seen_old: u32 = 0;
    let seen_new: u32 = 0;

    while (line := iter.next()) {
        const line_no = iter.line_no;

        // If currently collecting hunk body lines, check whether hunk is still expecting lines
        if (in_hunk) {
            const need_more = (seen_old < cur_hunk.old_count) || (seen_new < cur_hunk.new_count);
            if (startsWith(line, "\\ No newline at end of file")) {
                if (ops_len == 0 || ops_len <= @as(usize, cur_hunk.ops_start)) {
                    return PatchApplyError.InvalidHunkLine;
                }
                let last_op = ops_out[ops_len - 1];
                last_op.no_newline = true;
                ops_out[ops_len - 1] = last_op;
                continue;
            }
            if (need_more) {
                let op_kind = PatchLineOpKind.Context;
                let op_text: []const u8 = "";
                if (line.len == 0) {
                    // Empty line in unified diff body is treated as an empty context line.
                    op_kind = PatchLineOpKind.Context;
                    op_text = "";
                    seen_old += 1;
                    seen_new += 1;
                } else {
                    const tag = line[0];
                    if (tag == ' ') {
                        op_kind = PatchLineOpKind.Context;
                        op_text = line[1..];
                        seen_old += 1;
                        seen_new += 1;
                    } else if (tag == '-') {
                        op_kind = PatchLineOpKind.Delete;
                        op_text = line[1..];
                        seen_old += 1;
                        cur_file.deleted_lines += 1;
                    } else if (tag == '+') {
                        op_kind = PatchLineOpKind.Add;
                        op_text = line[1..];
                        seen_new += 1;
                        cur_file.added_lines += 1;
                    } else {
                        return PatchApplyError.InvalidHunkLine;
                    }
                }

                if (ops_len >= ops_out.len) {
                    return PatchApplyError.SliceCapacityExceeded;
                }
                ops_out[ops_len] = PatchLineOp{
                    kind: op_kind,
                    text: op_text,
                    no_newline: false,
                    patch_line_no: line_no,
                };
                ops_len += 1;
                cur_hunk.ops_len += 1;

                if (seen_old > cur_hunk.old_count || seen_new > cur_hunk.new_count) {
                    if (opts.strict_hunk_counts) {
                        return PatchApplyError.HunkLineCountMismatch;
                    }
                }
                continue;
            } else {
                // Hunk satisfied; commit hunk and fall through to process `line` as next header/hunk.
                if (opts.strict_hunk_counts && (seen_old != cur_hunk.old_count || seen_new != cur_hunk.new_count)) {
                    return PatchApplyError.HunkLineCountMismatch;
                }
                if (hunks_len >= hunks_out.len) {
                    return PatchApplyError.SliceCapacityExceeded;
                }
                hunks_out[hunks_len] = cur_hunk;
                hunks_len += 1;
                cur_file.hunks_len += 1;
                in_hunk = false;
            }
        }

        // Handle `diff --git a/... b/...`
        if (startsWith(line, "diff --git ")) {
            if (has_active_file) {
                if (files_len >= files_out.len) {
                    return PatchApplyError.SliceCapacityExceeded;
                }
                finalizeFileEntry(&cur_file, &summary);
                files_out[files_len] = cur_file;
                files_len += 1;
            }
            const paths = parseDiffGitPaths(line, opts.strip_prefix_level)?;
            cur_file = PatchFileEntry.empty();
            cur_file.old_path = paths[0];
            cur_file.new_path = paths[1];
            cur_file.hunks_start = @intCast(hunks_len);
            cur_file.patch_line_no = line_no;
            has_active_file = true;
            continue;
        }

        // Handle git extended header metadata lines
        if (startsWith(line, "old mode ")) {
            if (!has_active_file) return PatchApplyError.InvalidDiffHeader;
            if (m := parseFileModeOctal(line[9..])) {
                cur_file.old_mode = m;
            } else {
                return PatchApplyError.InvalidDiffHeader;
            }
            continue;
        }
        if (startsWith(line, "new mode ")) {
            if (!has_active_file) return PatchApplyError.InvalidDiffHeader;
            if (m := parseFileModeOctal(line[9..])) {
                cur_file.new_mode = m;
            } else {
                return PatchApplyError.InvalidDiffHeader;
            }
            continue;
        }
        if (startsWith(line, "new file mode ")) {
            if (!has_active_file) return PatchApplyError.InvalidDiffHeader;
            if (m := parseFileModeOctal(line[14..])) {
                cur_file.new_mode = m;
                cur_file.kind = PatchFileKind.Create;
            } else {
                return PatchApplyError.InvalidDiffHeader;
            }
            continue;
        }
        if (startsWith(line, "deleted file mode ")) {
            if (!has_active_file) return PatchApplyError.InvalidDiffHeader;
            if (m := parseFileModeOctal(line[18..])) {
                cur_file.old_mode = m;
                cur_file.kind = PatchFileKind.Delete;
            } else {
                return PatchApplyError.InvalidDiffHeader;
            }
            continue;
        }
        if (startsWith(line, "rename from ")) {
            if (!has_active_file) return PatchApplyError.InvalidDiffHeader;
            cur_file.old_path = sanitizePatchHeaderPath(line[12..], 0)?;
            cur_file.kind = PatchFileKind.Rename;
            continue;
        }
        if (startsWith(line, "rename to ")) {
            if (!has_active_file) return PatchApplyError.InvalidDiffHeader;
            cur_file.new_path = sanitizePatchHeaderPath(line[10..], 0)?;
            cur_file.kind = PatchFileKind.Rename;
            continue;
        }
        if (startsWith(line, "Binary files ") || bytesEqual(line, "GIT binary patch")) {
            if (!opts.allow_binary_markers) {
                return PatchApplyError.BinaryPatchUnsupported;
            }
            if (!has_active_file) {
                cur_file = PatchFileEntry.empty();
                cur_file.hunks_start = @intCast(hunks_len);
                cur_file.patch_line_no = line_no;
                has_active_file = true;
            }
            cur_file.is_binary = true;
            cur_file.kind = PatchFileKind.Binary;
            continue;
        }

        // Handle `--- <old_path>` (supports standalone unified diff without `diff --git`)
        if (startsWith(line, "--- ")) {
            const p = sanitizePatchHeaderPath(line[4..], opts.strip_prefix_level)?;
            // If we already finished hunks on an active file, this starts a new standalone file entry.
            if (has_active_file && (cur_file.hunks_len > 0 || cur_file.is_binary)) {
                if (files_len >= files_out.len) {
                    return PatchApplyError.SliceCapacityExceeded;
                }
                finalizeFileEntry(&cur_file, &summary);
                files_out[files_len] = cur_file;
                files_len += 1;
                has_active_file = false;
            }
            if (!has_active_file) {
                cur_file = PatchFileEntry.empty();
                cur_file.hunks_start = @intCast(hunks_len);
                cur_file.patch_line_no = line_no;
                has_active_file = true;
            }
            cur_file.old_path = p;
            if (p.len == 0) {
                cur_file.kind = PatchFileKind.Create;
            }
            continue;
        }

        // Handle `+++ <new_path>`
        if (startsWith(line, "+++ ")) {
            if (!has_active_file) return PatchApplyError.InvalidDiffHeader;
            const p = sanitizePatchHeaderPath(line[4..], opts.strip_prefix_level)?;
            cur_file.new_path = p;
            if (p.len == 0) {
                cur_file.kind = PatchFileKind.Delete;
            }
            continue;
        }

        // Handle `@@ -old +new @@` hunk header
        if (startsWith(line, "@@ -")) {
            if (!has_active_file) {
                return PatchApplyError.InvalidDiffHeader;
            }
            cur_hunk = parseHunkHeaderLine(line, line_no)?;
            cur_hunk.ops_start = @intCast(ops_len);
            cur_hunk.ops_len = 0;
            seen_old = 0;
            seen_new = 0;
            in_hunk = true;
            continue;
        }

        // Ignore benign Git preamble lines (`index ...`, `similarity index ...`, commit comments, blank lines)
    }

    // Flush trailing hunk if open
    if (in_hunk) {
        if (opts.strict_hunk_counts && (seen_old != cur_hunk.old_count || seen_new != cur_hunk.new_count)) {
            return PatchApplyError.HunkLineCountMismatch;
        }
        if (hunks_len >= hunks_out.len) {
            return PatchApplyError.SliceCapacityExceeded;
        }
        hunks_out[hunks_len] = cur_hunk;
        hunks_len += 1;
        cur_file.hunks_len += 1;
    }

    // Flush trailing file entry if open
    if (has_active_file) {
        if (files_len >= files_out.len) {
            return PatchApplyError.SliceCapacityExceeded;
        }
        finalizeFileEntry(&cur_file, &summary);
        files_out[files_len] = cur_file;
        files_len += 1;
    }

    if (files_len == 0) {
        return PatchApplyError.InvalidDiffHeader;
    }

    return ParsedUnifiedPatch{
        files_len: files_len,
        hunks_len: hunks_len,
        ops_len: ops_len,
        summary: summary,
    };
}

// ============================================================================
// Source Line Splitting & Hunk Context Matching Engine
// ============================================================================

/// Splits `src` into zero-copy line spans stored in `lines_out`, preserving CRLF/LF metadata.
pub fn splitSourceLines(src: []const u8, lines_out: []SourceLineSpan): usize! {
    if (src.len == 0) return 0;
    let count: usize = 0;
    let pos: usize = 0;
    while (pos < src.len) {
        if (count >= lines_out.len) {
            return PatchApplyError.SliceCapacityExceeded;
        }
        const start = pos;
        while (pos < src.len && src[pos] != '\n') {
            pos += 1;
        }
        const has_lf = (pos < src.len && src[pos] == '\n');
        let line_end = pos;
        let has_cr = false;
        if (line_end > start && src[line_end - 1] == '\r') {
            has_cr = true;
            line_end -= 1;
        }
        lines_out[count] = SourceLineSpan{
            text: src[start..line_end],
            has_cr: has_cr,
            has_lf: has_lf,
        };
        count += 1;
        if (has_lf) {
            pos += 1;
        }
    }
    return count;
}

fn lineMatches(src_line: SourceLineSpan, op_text: []const u8, ignore_trailing_cr: bool): bool {
    const target = if (ignore_trailing_cr) trimRightCr(op_text) else op_text;
    return bytesEqual(src_line.text, target);
}

/// Checks whether the hunk's expected input lines match `src_lines` starting at 0-based `start_idx`.
fn hunkMatchesAt(
    src_lines: []const SourceLineSpan,
    start_idx: usize,
    hunk: PatchHunk,
    ops: []const PatchLineOp,
    reverse: bool,
    ignore_trailing_cr: bool,
): bool {
    let cur = start_idx;
    let i: u32 = 0;
    while (i < hunk.ops_len) {
        const op = ops[@as(usize, hunk.ops_start + i)];
        const is_input_line =
            (op.kind == PatchLineOpKind.Context) ||
            (!reverse && op.kind == PatchLineOpKind.Delete) ||
            (reverse && op.kind == PatchLineOpKind.Add);
        if (is_input_line) {
            if (cur >= src_lines.len) return false;
            if (!lineMatches(src_lines[cur], op.text, ignore_trailing_cr)) {
                return false;
            }
            cur += 1;
        }
        i += 1;
    }
    return true;
}

struct HunkMatchLocation {
    found: bool = false,
    start_idx: usize = 0,
    offset: i32 = 0,
    abs_offset: u32 = 0,
}

/// Locates where a hunk matches in `src_lines`, starting at `expected_idx` and expanding
/// symmetrically up to `max_offset_lines` without overlapping `min_start_idx`.
fn locateHunkMatch(
    src_lines: []const SourceLineSpan,
    min_start_idx: usize,
    expected_idx: usize,
    hunk: PatchHunk,
    ops: []const PatchLineOp,
    opts: PatchApplyOptions,
): HunkMatchLocation {
    const base_idx = if (expected_idx < min_start_idx) min_start_idx else expected_idx;
    if (base_idx <= src_lines.len && hunkMatchesAt(src_lines, base_idx, hunk, ops, opts.reverse, opts.ignore_trailing_cr)) {
        const off_i64 = @as(i64, @intCast(base_idx)) - @as(i64, @intCast(expected_idx));
        const abs_off: u32 = if (off_i64 < 0) @intCast(-off_i64) else @intCast(off_i64);
        return HunkMatchLocation{
            found: true,
            start_idx: base_idx,
            offset: @intCast(off_i64),
            abs_offset: abs_off,
        };
    }

    let delta: u32 = 1;
    while (delta <= opts.max_offset_lines) {
        const d_usize: usize = @intCast(delta);
        // Search forward: expected_idx + delta
        const fwd = expected_idx + d_usize;
        if (fwd >= min_start_idx && fwd <= src_lines.len) {
            if (hunkMatchesAt(src_lines, fwd, hunk, ops, opts.reverse, opts.ignore_trailing_cr)) {
                return HunkMatchLocation{
                    found: true,
                    start_idx: fwd,
                    offset: @intCast(delta),
                    abs_offset: delta,
                };
            }
        }
        // Search backward: expected_idx - delta
        if (expected_idx >= d_usize) {
            const bwd = expected_idx - d_usize;
            if (bwd >= min_start_idx && bwd <= src_lines.len) {
                if (hunkMatchesAt(src_lines, bwd, hunk, ops, opts.reverse, opts.ignore_trailing_cr)) {
                    return HunkMatchLocation{
                        found: true,
                        start_idx: bwd,
                        offset: -@as(i32, @intCast(delta)),
                        abs_offset: delta,
                    };
                }
            }
        }
        delta += 1;
    }

    return HunkMatchLocation{};
}

fn appendBytes(out_buf: []u8, pos: *usize, data: []const u8)! {
    if (pos.* + data.len > out_buf.len) {
        return PatchApplyError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < data.len) {
        out_buf[pos.* + i] = data[i];
        i += 1;
    }
    pos.* += data.len;
}

fn appendLine(out_buf: []u8, pos: *usize, text: []const u8, use_crlf: bool, emit_newline: bool)! {
    appendBytes(out_buf, pos, text)?;
    if (emit_newline) {
        if (use_crlf) {
            appendBytes(out_buf, pos, "\r\n")?;
        } else {
            appendBytes(out_buf, pos, "\n")?;
        }
    }
}

fn detectPreferCrlf(src_lines: []const SourceLineSpan): bool {
    if (src_lines.len == 0) return false;
    return src_lines[0].has_cr;
}

fn computeExpectedStartIndex(hunk: PatchHunk, reverse: bool, cumulative_delta: i64): usize {
    const raw_start: u32 = if (reverse) hunk.new_start else hunk.old_start;
    const zero_based: i64 = if (raw_start > 0) @as(i64, @intCast(raw_start - 1)) else 0;
    const shifted = zero_based + cumulative_delta;
    if (shifted < 0) return 0;
    return @intCast(shifted);
}

// ============================================================================
// Core Forward Apply, Reverse Rollback & Dry-Run Validator
// ============================================================================

/// Applies a single `PatchFileEntry` (forward or reverse) to `source` and writes the
/// resulting file bytes into `out_buf` with zero heap allocation.
/// On context conflict or bounds error, fills `conflict_out` with exact hunk/line diagnostics.
pub fn applyFilePatch(
    source: []const u8,
    file_entry: PatchFileEntry,
    hunks: []const PatchHunk,
    ops: []const PatchLineOp,
    opts: PatchApplyOptions,
    line_scratch: []SourceLineSpan,
    out_buf: []u8,
    conflict_out: *PatchConflictInfo,
): PatchApplyResult! {
    conflict_out.* = PatchConflictInfo.none();

    if (file_entry.is_binary || file_entry.kind == PatchFileKind.Binary) {
        conflict_out.* = PatchConflictInfo{
            has_conflict: true,
            file_index: 0,
            hunk_index: 0,
            patch_line_no: file_entry.patch_line_no,
            expected_target_line: 1,
            applied_offset: 0,
            reason_code: PATCH_CONFLICT_BINARY_UNSUPPORTED,
        };
        return PatchApplyError.BinaryPatchUnsupported;
    }

    // Validate Create / Delete preconditions
    if (!opts.reverse && file_entry.kind == PatchFileKind.Create && source.len > 0) {
        conflict_out.* = PatchConflictInfo{
            has_conflict: true,
            patch_line_no: file_entry.patch_line_no,
            expected_target_line: 1,
            reason_code: PATCH_CONFLICT_FILE_ALREADY_EXISTS,
        };
        return PatchApplyError.FileAlreadyExists;
    }
    if (opts.reverse && file_entry.kind == PatchFileKind.Delete && source.len > 0) {
        conflict_out.* = PatchConflictInfo{
            has_conflict: true,
            patch_line_no: file_entry.patch_line_no,
            expected_target_line: 1,
            reason_code: PATCH_CONFLICT_FILE_ALREADY_EXISTS,
        };
        return PatchApplyError.FileAlreadyExists;
    }

    const src_line_count = splitSourceLines(source, line_scratch)?;
    const src_lines = line_scratch[0..src_line_count];
    const prefer_crlf = opts.preserve_line_ending && detectPreferCrlf(src_lines);

    let out_pos: usize = 0;
    let cursor_line: usize = 0;
    let cumulative_delta: i64 = 0;
    let max_abs_offset: u32 = 0;
    let total_added: u32 = 0;
    let total_deleted: u32 = 0;
    let hunks_applied: u32 = 0;
    let last_emitted_newline = false;

    let hi: u32 = 0;
    while (hi < file_entry.hunks_len) {
        const hunk_idx: usize = @intCast(file_entry.hunks_start + hi);
        if (hunk_idx >= hunks.len) {
            return PatchApplyError.HunkOutOfBounds;
        }
        const hunk = hunks[hunk_idx];
        const expected_idx = computeExpectedStartIndex(hunk, opts.reverse, cumulative_delta);

        const loc = locateHunkMatch(src_lines, cursor_line, expected_idx, hunk, ops, opts);
        if (!loc.found) {
            conflict_out.* = PatchConflictInfo{
                has_conflict: true,
                file_index: 0,
                hunk_index: hi,
                patch_line_no: hunk.patch_line_no,
                expected_target_line: @intCast(expected_idx + 1),
                applied_offset: 0,
                reason_code: PATCH_CONFLICT_CONTEXT_MISMATCH,
            };
            return PatchApplyError.ContextMismatch;
        }

        if (loc.abs_offset > max_abs_offset) {
            max_abs_offset = loc.abs_offset;
        }

        // Emit untouched source lines before this hunk
        while (cursor_line < loc.start_idx) {
            const sl = src_lines[cursor_line];
            appendLine(out_buf, &out_pos, sl.text, sl.has_cr, sl.has_lf)?;
            last_emitted_newline = sl.has_lf;
            cursor_line += 1;
        }

        // Apply hunk operations
        let consumed_in_hunk: usize = 0;
        let emitted_in_hunk: usize = 0;
        let oi: u32 = 0;
        while (oi < hunk.ops_len) {
            const op_idx: usize = @intCast(hunk.ops_start + oi);
            if (op_idx >= ops.len) {
                return PatchApplyError.HunkOutOfBounds;
            }
            const op = ops[op_idx];

            if (op.kind == PatchLineOpKind.Context) {
                const sl = src_lines[cursor_line];
                const emit_nl = if (op.no_newline) false else sl.has_lf;
                appendLine(out_buf, &out_pos, sl.text, sl.has_cr, emit_nl)?;
                last_emitted_newline = emit_nl;
                cursor_line += 1;
                consumed_in_hunk += 1;
                emitted_in_hunk += 1;
            } else if ((!opts.reverse && op.kind == PatchLineOpKind.Delete) || (opts.reverse && op.kind == PatchLineOpKind.Add)) {
                cursor_line += 1;
                consumed_in_hunk += 1;
                total_deleted += 1;
            } else {
                // Emitted added line (Add in forward mode, Delete in reverse mode)
                const emit_nl = !op.no_newline;
                appendLine(out_buf, &out_pos, op.text, prefer_crlf, emit_nl)?;
                last_emitted_newline = emit_nl;
                emitted_in_hunk += 1;
                total_added += 1;
            }
            oi += 1;
        }

        cumulative_delta += @as(i64, @intCast(emitted_in_hunk)) - @as(i64, @intCast(consumed_in_hunk));
        hunks_applied += 1;
        hi += 1;
    }

    // Emit remaining source lines after the last hunk
    while (cursor_line < src_lines.len) {
        const sl = src_lines[cursor_line];
        appendLine(out_buf, &out_pos, sl.text, sl.has_cr, sl.has_lf)?;
        last_emitted_newline = sl.has_lf;
        cursor_line += 1;
    }

    return PatchApplyResult{
        output_len: out_pos,
        hunks_applied: hunks_applied,
        lines_added: total_added,
        lines_deleted: total_deleted,
        max_abs_offset: max_abs_offset,
        has_trailing_newline: last_emitted_newline,
        conflict: PatchConflictInfo.none(),
    };
}

/// Forward-applies a single parsed file patch onto `source` with default offset window.
pub fn applyUnifiedPatch(
    source: []const u8,
    file_entry: PatchFileEntry,
    hunks: []const PatchHunk,
    ops: []const PatchLineOp,
    line_scratch: []SourceLineSpan,
    out_buf: []u8,
    conflict_out: *PatchConflictInfo,
): PatchApplyResult! {
    return applyFilePatch(
        source,
        file_entry,
        hunks,
        ops,
        PatchApplyOptions.forward(),
        line_scratch,
        out_buf,
        conflict_out,
    );
}

/// Reverse-applies (rolls back) a single parsed file patch from `patched` back to original source.
pub fn reverseUnifiedPatch(
    patched: []const u8,
    file_entry: PatchFileEntry,
    hunks: []const PatchHunk,
    ops: []const PatchLineOp,
    line_scratch: []SourceLineSpan,
    out_buf: []u8,
    conflict_out: *PatchConflictInfo,
): PatchApplyResult! {
    return applyFilePatch(
        patched,
        file_entry,
        hunks,
        ops,
        PatchApplyOptions.reverseMode(),
        line_scratch,
        out_buf,
        conflict_out,
    );
}

/// Performs a zero-write dry-run validation of a file patch against `source`.
/// Computes exact vs offset hunk match counts, estimated output byte length,
/// and exact conflict coordinates without modifying any output buffer.
pub fn dryRunPatch(
    source: []const u8,
    file_entry: PatchFileEntry,
    hunks: []const PatchHunk,
    ops: []const PatchLineOp,
    opts: PatchApplyOptions,
    line_scratch: []SourceLineSpan,
): PatchDryRunReport {
    let report = PatchDryRunReport.empty();

    if (file_entry.is_binary || file_entry.kind == PatchFileKind.Binary) {
        report.can_apply = false;
        report.conflict = PatchConflictInfo{
            has_conflict: true,
            patch_line_no: file_entry.patch_line_no,
            expected_target_line: 1,
            reason_code: PATCH_CONFLICT_BINARY_UNSUPPORTED,
        };
        return report;
    }

    if (!opts.reverse && file_entry.kind == PatchFileKind.Create && source.len > 0) {
        report.can_apply = false;
        report.conflict = PatchConflictInfo{
            has_conflict: true,
            patch_line_no: file_entry.patch_line_no,
            expected_target_line: 1,
            reason_code: PATCH_CONFLICT_FILE_ALREADY_EXISTS,
        };
        return report;
    }

    const src_line_count = splitSourceLines(source, line_scratch) catch {
        report.can_apply = false;
        report.conflict = PatchConflictInfo{
            has_conflict: true,
            patch_line_no: file_entry.patch_line_no,
            expected_target_line: 1,
            reason_code: PATCH_CONFLICT_BUFFER_TOO_SMALL,
        };
        return report;
    };
    const src_lines = line_scratch[0..src_line_count];
    const prefer_crlf = opts.preserve_line_ending && detectPreferCrlf(src_lines);
    const nl_bytes: usize = if (prefer_crlf) 2 else 1;

    let cursor_line: usize = 0;
    let cumulative_delta: i64 = 0;
    let est_bytes: usize = 0;

    let hi: u32 = 0;
    while (hi < file_entry.hunks_len) {
        const hunk_idx: usize = @intCast(file_entry.hunks_start + hi);
        if (hunk_idx >= hunks.len) {
            report.can_apply = false;
            report.conflict = PatchConflictInfo{
                has_conflict: true,
                hunk_index: hi,
                reason_code: PATCH_CONFLICT_OUT_OF_BOUNDS,
            };
            return report;
        }
        const hunk = hunks[hunk_idx];
        report.hunks_checked += 1;

        const expected_idx = computeExpectedStartIndex(hunk, opts.reverse, cumulative_delta);
        const loc = locateHunkMatch(src_lines, cursor_line, expected_idx, hunk, ops, opts);
        if (!loc.found) {
            report.can_apply = false;
            report.conflict = PatchConflictInfo{
                has_conflict: true,
                hunk_index: hi,
                patch_line_no: hunk.patch_line_no,
                expected_target_line: @intCast(expected_idx + 1),
                reason_code: PATCH_CONFLICT_CONTEXT_MISMATCH,
            };
            return report;
        }

        if (loc.abs_offset == 0) {
            report.hunks_matched_exact += 1;
        } else {
            report.hunks_matched_with_offset += 1;
        }

        while (cursor_line < loc.start_idx) {
            const sl = src_lines[cursor_line];
            est_bytes += sl.text.len + (if (sl.has_cr) @as(usize, 1) else 0) + (if (sl.has_lf) @as(usize, 1) else 0);
            cursor_line += 1;
        }

        let consumed_in_hunk: usize = 0;
        let emitted_in_hunk: usize = 0;
        let oi: u32 = 0;
        while (oi < hunk.ops_len) {
            const op = ops[@as(usize, hunk.ops_start + oi)];
            if (op.kind == PatchLineOpKind.Context) {
                const sl = src_lines[cursor_line];
                const emit_nl = if (op.no_newline) false else sl.has_lf;
                est_bytes += sl.text.len + (if (emit_nl) (if (sl.has_cr) @as(usize, 2) else 1) else 0);
                cursor_line += 1;
                consumed_in_hunk += 1;
                emitted_in_hunk += 1;
            } else if ((!opts.reverse && op.kind == PatchLineOpKind.Delete) || (opts.reverse && op.kind == PatchLineOpKind.Add)) {
                cursor_line += 1;
                consumed_in_hunk += 1;
            } else {
                const emit_nl = !op.no_newline;
                est_bytes += op.text.len + (if (emit_nl) nl_bytes else 0);
                emitted_in_hunk += 1;
            }
            oi += 1;
        }

        cumulative_delta += @as(i64, @intCast(emitted_in_hunk)) - @as(i64, @intCast(consumed_in_hunk));
        hi += 1;
    }

    while (cursor_line < src_lines.len) {
        const sl = src_lines[cursor_line];
        est_bytes += sl.text.len + (if (sl.has_cr) @as(usize, 1) else 0) + (if (sl.has_lf) @as(usize, 1) else 0);
        cursor_line += 1;
    }

    report.can_apply = true;
    report.estimated_output_bytes = est_bytes;
    return report;
}

// ============================================================================
// Zero-Alloc Summary Report Formatter
// ============================================================================

fn appendU32Decimal(out_buf: []u8, pos: *usize, value: u32)! {
    let digits: [16]u8 = [0; 16];
    let len: usize = 0;
    let v = value;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = @intCast('0' + (v % 10));
            v = v / 10;
            len += 1;
        }
    }
    if (pos.* + len > out_buf.len) {
        return PatchApplyError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < len) {
        out_buf[pos.* + i] = digits[len - 1 - i];
        i += 1;
    }
    pos.* += len;
}

/// Formats a human-readable patch summary line:
/// `files=<N> (create=<C> mod=<M> del=<D> ren=<R>) hunks=<H> +<A> -<X>`
pub fn formatPatchSummaryReport(summary: PatchSummary, out_buf: []u8): usize! {
    let pos: usize = 0;
    appendBytes(out_buf, &pos, "files=")?;
    appendU32Decimal(out_buf, &pos, summary.total_files)?;
    appendBytes(out_buf, &pos, " (create=")?;
    appendU32Decimal(out_buf, &pos, summary.created_files)?;
    appendBytes(out_buf, &pos, " mod=")?;
    appendU32Decimal(out_buf, &pos, summary.modified_files)?;
    appendBytes(out_buf, &pos, " del=")?;
    appendU32Decimal(out_buf, &pos, summary.deleted_files)?;
    appendBytes(out_buf, &pos, " ren=")?;
    appendU32Decimal(out_buf, &pos, summary.renamed_files)?;
    appendBytes(out_buf, &pos, ") hunks=")?;
    appendU32Decimal(out_buf, &pos, summary.total_hunks)?;
    appendBytes(out_buf, &pos, " +")?;
    appendU32Decimal(out_buf, &pos, summary.total_added_lines)?;
    appendBytes(out_buf, &pos, " -")?;
    appendU32Decimal(out_buf, &pos, summary.total_deleted_lines)?;
    return pos;
}
