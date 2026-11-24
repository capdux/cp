// Reference-hardened against mitsuhiko/similar (Apache-2.0)
// Origin: Pure-CP Stdlib (AI Coding Agent Unified Diff Hunk Parser, Generator & Zero-Alloc Patch Applier)
// Unified Diff (@@ -a,b +c,d @@) generator, strict parser, context verifier, and two-pass atomic patch applier
// accelerated with 16-lane @Vector(16, u8) SIMD newline and line equality scanning.

const std = @import("std");
import {
    DiffError,
    DiffKind,
    DiffOptions,
    LineDiff,
    diffLines,
    simdBytesEqual,
    simdFindNewline,
} from "./myersDiff.cp";

pub const UNIFIED_DIFF_MAX_HUNK_LINES: u32 = 1_000_000;

/// Parsed Unified Diff hunk range header (`@@ -old_start,old_count +new_start,new_count @@`).
pub struct DiffHunkHeader {
    old_start: u32 = 0;
    old_count: u32 = 0;
    new_start: u32 = 0;
    new_count: u32 = 0;
}

/// Operation kind for a single line inside a Unified Diff hunk.
pub enum DiffLineKind {
    Context,
    Remove,
    Add,
}

/// Zero-copy view of a single Unified Diff hunk operation line.
pub struct DiffOp {
    kind: DiffLineKind = DiffLineKind.Context;
    text: string = "";
}

/// Deterministic status returned by `applyUnifiedPatch`.
pub enum PatchApplyStatus {
    Ok,
    ContextMismatch,
    LineOutOfRange,
    InvalidHunk,
    BufferTooSmall,
}

/// Summary and output byte length produced by `applyUnifiedPatch`.
/// On any non-`Ok` status, `written_bytes` is `0` and `out_buf` is never mutated.
pub struct PatchApplyResult {
    status: PatchApplyStatus = PatchApplyStatus.Ok;
    written_bytes: usize = 0;
    hunks_applied: u32 = 0;
    lines_added: u32 = 0;
    lines_removed: u32 = 0;
}

pub err UnifiedDiffError {
    ContextMismatch,
    LineOutOfRange,
    InvalidHunk,
    OutOfMemory,
}

fn stripTrailingCr(s: string): string {
    if (s.len > 0 && s[s.len - 1] == '\r') {
        return s[0..(s.len - 1)];
    }
    return s;
}

fn parseU32At(s: string, pos: *usize): u32? {
    const start = pos.*;
    if (start >= s.len) return null;
    if (s[start] < '0' || s[start] > '9') return null;

    let acc: u64 = 0;
    let p: usize = start;
    while (p < s.len && s[p] >= '0' && s[p] <= '9') : (p += 1) {
        const digit: u64 = (s[p] - '0') as u64;
        acc = (acc * 10) + digit;
        if (acc > 4294967295) return null;
    }
    pos.* = p;
    return acc as u32;
}

/// Parses a Unified Diff hunk header line such as `@@ -10,4 +12,6 @@ fn foo()`
/// or abbreviated forms omitting `,count` (defaulting count to `1`), such as `@@ -10 +10,2 @@`.
/// Enforces overflow guards, non-empty hunk bounds, and 1-based start validity.
/// Returns `null` for malformed headers.
pub fn parseHunkHeader(raw_line: string): DiffHunkHeader? {
    const line = stripTrailingCr(raw_line);
    if (line.len < 11) return null;
    if (line[0] != '@' || line[1] != '@' || line[2] != ' ' || line[3] != '-') {
        return null;
    }

    let pos: usize = 4;
    const old_start = parseU32At(line, *pos) ?? return null;
    let old_count: u32 = 1;
    if (pos < line.len && line[pos] == ',') {
        pos += 1;
        old_count = parseU32At(line, *pos) ?? return null;
    }

    if (pos + 1 >= line.len || line[pos] != ' ' || line[pos + 1] != '+') {
        return null;
    }
    pos += 2;

    const new_start = parseU32At(line, *pos) ?? return null;
    let new_count: u32 = 1;
    if (pos < line.len && line[pos] == ',') {
        pos += 1;
        new_count = parseU32At(line, *pos) ?? return null;
    }

    if (pos + 2 >= line.len || line[pos] != ' ' || line[pos + 1] != '@' || line[pos + 2] != '@') {
        return null;
    }
    pos += 3;
    // After closing "@@", must be end-of-line or whitespace-delimited optional section context
    if (pos < line.len && line[pos] != ' ' && line[pos] != '\t') {
        return null;
    }

    // Reject 0-line empty hunks, excessive counts, invalid 0 starts with count > 0, and u32 range overflows
    if (old_count == 0 && new_count == 0) return null;
    if (old_count > UNIFIED_DIFF_MAX_HUNK_LINES || new_count > UNIFIED_DIFF_MAX_HUNK_LINES) return null;
    if (old_count > 0 && old_start == 0) return null;
    if (new_count > 0 && new_start == 0) return null;
    if ((old_start as u64) + (old_count as u64) > 4294967295) return null;
    if ((new_start as u64) + (new_count as u64) > 4294967295) return null;
    if (old_count == 0 && old_start == 4294967295) return null;

    return DiffHunkHeader{
        old_start: old_start,
        old_count: old_count,
        new_start: new_start,
        new_count: new_count,
    };
}

/// Parses a single hunk body line (`' '`, `'-'`, `'+'`) into a zero-copy `DiffOp`.
/// Returns `null` if the line does not begin with a valid hunk operator.
pub fn parseDiffOp(raw_line: string): DiffOp? {
    const line = stripTrailingCr(raw_line);
    if (line.len == 0) return null;
    const prefix = line[0];
    if (prefix == ' ') {
        return DiffOp{ kind: DiffLineKind.Context, text: line[1..] };
    }
    if (prefix == '-') {
        return DiffOp{ kind: DiffLineKind.Remove, text: line[1..] };
    }
    if (prefix == '+') {
        return DiffOp{ kind: DiffLineKind.Add, text: line[1..] };
    }
    return null;
}

struct LineSlice {
    text: string = "";
    has_cr: bool = false;
    has_nl: bool = false;
    ok: bool = false;
}

fn readNextLine(src: string, pos: *usize): LineSlice {
    const start = pos.*;
    if (start >= src.len) {
        return LineSlice{ text: "", has_cr: false, has_nl: false, ok: false };
    }
    if (nl_pos := simdFindNewline(src, start)) {
        pos.* = nl_pos + 1;
        const raw = src[start..nl_pos];
        if (raw.len > 0 && raw[raw.len - 1] == '\r') {
            return LineSlice{
                text: raw[0..(raw.len - 1)],
                has_cr: true,
                has_nl: true,
                ok: true,
            };
        }
        return LineSlice{
            text: raw,
            has_cr: false,
            has_nl: true,
            ok: true,
        };
    }
    pos.* = src.len;
    const raw = src[start..src.len];
    if (raw.len > 0 && raw[raw.len - 1] == '\r') {
        return LineSlice{
            text: raw[0..(raw.len - 1)],
            has_cr: true,
            has_nl: false,
            ok: true,
        };
    }
    return LineSlice{
        text: raw,
        has_cr: false,
        has_nl: false,
        ok: true,
    };
}

fn startsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    return simdBytesEqual(s[0..prefix.len], prefix);
}

fn isIgnorableHeaderLine(line: string): bool {
    if (line.len == 0) return true;
    if (startsWith(line, "--- ")) return true;
    if (startsWith(line, "+++ ")) return true;
    if (startsWith(line, "diff ")) return true;
    if (startsWith(line, "index ")) return true;
    if (startsWith(line, "old mode ")) return true;
    if (startsWith(line, "new mode ")) return true;
    if (startsWith(line, "new file mode ")) return true;
    if (startsWith(line, "deleted file mode ")) return true;
    return false;
}

fn emitBytes(out_buf: *[]u8, out_pos: *usize, data: string, write_enabled: bool): bool {
    const cur = out_pos.*;
    if (write_enabled) {
        if (data.len > out_buf.len - cur) return false;
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            out_buf[cur + i] = data[i];
        }
    }
    out_pos.* = cur + data.len;
    return true;
}

fn emitByte(out_buf: *[]u8, out_pos: *usize, b: u8, write_enabled: bool): bool {
    const cur = out_pos.*;
    if (write_enabled) {
        if (cur >= out_buf.len) return false;
        out_buf[cur] = b;
    }
    out_pos.* = cur + 1;
    return true;
}

fn emitLineWithEndings(
    out_buf: *[]u8,
    out_pos: *usize,
    text: string,
    has_cr: bool,
    has_nl: bool,
    write_enabled: bool,
): bool {
    if (!emitBytes(out_buf, out_pos, text, write_enabled)) return false;
    if (has_cr) {
        if (!emitByte(out_buf, out_pos, '\r', write_enabled)) return false;
    }
    if (has_nl) {
        if (!emitByte(out_buf, out_pos, '\n', write_enabled)) return false;
    }
    return true;
}

fn runPatchPass(
    original: string,
    patch: string,
    out_buf: *[]u8,
    write_enabled: bool,
): PatchApplyResult {
    let orig_pos: usize = 0;
    let orig_line_no: u32 = 1;
    let patch_pos: usize = 0;
    let out_pos: usize = 0;

    let hunks_applied: u32 = 0;
    let lines_added: u32 = 0;
    let lines_removed: u32 = 0;

    while (true) {
        const pl = readNextLine(patch, *patch_pos);
        if (!pl.ok) break;

        if (!startsWith(pl.text, "@@")) {
            if (isIgnorableHeaderLine(pl.text)) {
                continue;
            }
            return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
        }

        const hdr_opt = parseHunkHeader(pl.text);
        if (hdr_opt == null) {
            return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
        }
        const hdr = hdr_opt ?? DiffHunkHeader{};

        // Determine target 1-based original line before which unchanged lines must be copied
        let target_orig_line: u32 = 1;
        if (hdr.old_count == 0) {
            // Pure insertion: old_start==0 inserts before line 1; old_start>0 inserts after line old_start
            target_orig_line = hdr.old_start + 1;
        } else {
            if (hdr.old_start == 0) {
                return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
            }
            target_orig_line = hdr.old_start;
        }

        if (target_orig_line < orig_line_no) {
            return PatchApplyResult{ status: PatchApplyStatus.LineOutOfRange };
        }

        // Copy untouched original lines up to target_orig_line
        while (orig_line_no < target_orig_line) {
            const ol = readNextLine(original, *orig_pos);
            if (!ol.ok) {
                return PatchApplyResult{ status: PatchApplyStatus.LineOutOfRange };
            }
            if (!emitLineWithEndings(out_buf, *out_pos, ol.text, ol.has_cr, ol.has_nl, write_enabled)) {
                return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
            }
            orig_line_no += 1;
        }

        // Apply hunk operations
        let seen_old: u32 = 0;
        let seen_new: u32 = 0;

        while (seen_old < hdr.old_count || seen_new < hdr.new_count) {
            const op_line = readNextLine(patch, *patch_pos);
            if (!op_line.ok) {
                return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
            }

            // Treat empty hunk line as an empty Context line for compatibility with stripped-space diffs
            let op_kind = DiffLineKind.Context;
            let op_text: string = "";
            if (op_line.text.len == 0) {
                op_kind = DiffLineKind.Context;
                op_text = "";
            } else if (parsed := parseDiffOp(op_line.text)) {
                op_kind = parsed.kind;
                op_text = parsed.text;
            } else {
                return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
            }

            // Peek for an immediately following "\ No newline at end of file" marker
            let no_nl_marker = false;
            const saved_patch_pos = patch_pos;
            const peek_line = readNextLine(patch, *patch_pos);
            if (peek_line.ok && startsWith(peek_line.text, "\\ ")) {
                no_nl_marker = true;
            } else {
                patch_pos = saved_patch_pos;
            }

            if (op_kind == DiffLineKind.Context) {
                if (seen_old >= hdr.old_count || seen_new >= hdr.new_count) {
                    return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
                }
                const ol = readNextLine(original, *orig_pos);
                if (!ol.ok) {
                    return PatchApplyResult{ status: PatchApplyStatus.LineOutOfRange };
                }
                if (!simdBytesEqual(ol.text, op_text)) {
                    return PatchApplyResult{ status: PatchApplyStatus.ContextMismatch };
                }
                const emit_nl = if (no_nl_marker) false else ol.has_nl;
                if (!emitLineWithEndings(out_buf, *out_pos, ol.text, ol.has_cr, emit_nl, write_enabled)) {
                    return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
                }
                orig_line_no += 1;
                seen_old += 1;
                seen_new += 1;
            } else if (op_kind == DiffLineKind.Remove) {
                if (seen_old >= hdr.old_count) {
                    return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
                }
                const ol = readNextLine(original, *orig_pos);
                if (!ol.ok) {
                    return PatchApplyResult{ status: PatchApplyStatus.LineOutOfRange };
                }
                if (!simdBytesEqual(ol.text, op_text)) {
                    return PatchApplyResult{ status: PatchApplyStatus.ContextMismatch };
                }
                orig_line_no += 1;
                seen_old += 1;
                lines_removed += 1;
            } else {
                // DiffLineKind.Add
                if (seen_new >= hdr.new_count) {
                    return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
                }
                const emit_nl = !no_nl_marker;
                if (!emitLineWithEndings(out_buf, *out_pos, op_text, op_line.has_cr, emit_nl, write_enabled)) {
                    return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
                }
                seen_new += 1;
                lines_added += 1;
            }
        }

        hunks_applied += 1;
    }

    // Copy any remaining untouched lines from original
    while (orig_pos < original.len) {
        const ol = readNextLine(original, *orig_pos);
        if (!ol.ok) break;
        if (!emitLineWithEndings(out_buf, *out_pos, ol.text, ol.has_cr, ol.has_nl, write_enabled)) {
            return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
        }
    }

    return PatchApplyResult{
        status: PatchApplyStatus.Ok,
        written_bytes: out_pos,
        hunks_applied: hunks_applied,
        lines_added: lines_added,
        lines_removed: lines_removed,
    };
}

/// Applies a Unified Diff `patch` to `original` and writes the patched content into `out_buf`.
/// Uses a two-pass atomic design (Pass 1 validates all hunks, line ranges, context lines, and total
/// output length; Pass 2 writes into `out_buf`), guaranteeing zero heap allocations and zero
/// partial corruption of `out_buf` on failure.
pub fn applyUnifiedPatch(
    original: string,
    patch: string,
    out_buf: *[]u8,
): PatchApplyResult {
    // Pass 1: Validate all hunks and compute exact required byte length without touching out_buf
    const dry = runPatchPass(original, patch, out_buf, false);
    if (dry.status != PatchApplyStatus.Ok) {
        return PatchApplyResult{
            status: dry.status,
            written_bytes: 0,
            hunks_applied: 0,
            lines_added: 0,
            lines_removed: 0,
        };
    }
    if (dry.written_bytes > out_buf.len) {
        return PatchApplyResult{
            status: PatchApplyStatus.BufferTooSmall,
            written_bytes: 0,
            hunks_applied: 0,
            lines_added: 0,
            lines_removed: 0,
        };
    }

    // Pass 2: Commit validated patch into out_buf
    return runPatchPass(original, patch, out_buf, true);
}

/// Allocating wrapper around `applyUnifiedPatch` that returns the exact patched bytes or a typed error.
pub fn applyUnifiedDiff(
    allocator: std.mem.Allocator,
    original: string,
    patch: string,
): ([]u8) ! UnifiedDiffError {
    let dummy: [1]u8 = [0; 1];
    let dummy_slice: []u8 = dummy[0..0];
    const dry = runPatchPass(original, patch, dummy_slice, false);
    if (dry.status != PatchApplyStatus.Ok) {
        return switch (dry.status) {
            PatchApplyStatus.ContextMismatch => error.ContextMismatch,
            PatchApplyStatus.LineOutOfRange => error.LineOutOfRange,
            PatchApplyStatus.InvalidHunk => error.InvalidHunk,
            PatchApplyStatus.BufferTooSmall => error.OutOfMemory,
            PatchApplyStatus.Ok => error.InvalidHunk,
        };
    }
    let out = allocator.alloc(u8, dry.written_bytes) catch return error.OutOfMemory;
    errdefer allocator.free(out);
    const res = runPatchPass(original, patch, out, true);
    if (res.status != PatchApplyStatus.Ok) {
        return error.InvalidHunk;
    }
    return out;
}

struct SplitTextLines {
    lines: []string,
    ends_with_nl: bool,
}

fn splitTextForUnified(allocator: std.mem.Allocator, text: string): SplitTextLines ! DiffError {
    if (text.len == 0) {
        let empty: []string = [];
        return SplitTextLines{ lines: empty, ends_with_nl: true };
    }
    const ends_with_nl = text[text.len - 1] == '\n';
    const effective = if (ends_with_nl) text[0..(text.len - 1)] else text;
    let count: usize = 1;
    let scan: usize = 0;
    while (nl_pos := simdFindNewline(effective, scan)) {
        count += 1;
        scan = nl_pos + 1;
    }
    let out = allocator.alloc(string, count) catch return error.OutOfMemory;
    let idx: usize = 0;
    let start: usize = 0;
    while (nl_pos := simdFindNewline(effective, start)) {
        out[idx] = effective[start..nl_pos];
        idx += 1;
        start = nl_pos + 1;
    }
    out[idx] = effective[start..effective.len];
    return SplitTextLines{ lines: out, ends_with_nl: ends_with_nl };
}

fn appendUsizeDecimal(allocator: std.mem.Allocator, buf: *std.ArrayList(u8), val: usize): DiffError!void {
    let tmp: [24]u8 = [0; 24];
    const rendered = std.fmt.bufPrint(&tmp, "{d}", .{val}) catch return error.OutOfMemory;
    buf.appendSlice(allocator, rendered) catch return error.OutOfMemory;
}

/// Generates a standard Unified Diff (`---`/`+++`/`@@ -a,b +c,d @@`) transforming `old_text` into `new_text`.
/// Handles empty files, CRLF/LF preservation, and `\ No newline at end of file` markers so that
/// `applyUnifiedDiff(alloc, old_text, generateUnifiedDiff(...)) == new_text` holds byte-for-byte.
pub fn generateUnifiedDiff(
    allocator: std.mem.Allocator,
    old_text: string,
    new_text: string,
    old_label: string,
    new_label: string,
    context_lines: usize,
): ([]u8) ! DiffError {
    if (simdBytesEqual(old_text, new_text)) {
        let empty: []u8 = [];
        return empty;
    }

    const old_split = try splitTextForUnified(allocator, old_text);
    defer if (old_split.lines.len > 0) allocator.free(old_split.lines);
    const new_split = try splitTextForUnified(allocator, new_text);
    defer if (new_split.lines.len > 0) allocator.free(new_split.lines);

    // Note: diffLines(actual=new, expected=old) yields .insert for new-only lines and .delete for old-only lines
    let diff_res = try diffLines(allocator, new_split.lines, old_split.lines, DiffOptions{});
    // Build editable op list, splitting any equal line whose newline termination differs between old and new
    let ops: std.ArrayList(LineDiff) = .empty;
    defer ops.deinit(allocator);
    let o_scan: usize = 0;
    let n_scan: usize = 0;
    for (it of diff_res.items) {
        switch (it.kind) {
            .equal => {
                const old_has_nl = (o_scan + 1 < old_split.lines.len) || old_split.ends_with_nl;
                const new_has_nl = (n_scan + 1 < new_split.lines.len) || new_split.ends_with_nl;
                if (old_has_nl != new_has_nl) {
                    ops.append(allocator, LineDiff{ kind: .delete, value: it.value }) catch return error.OutOfMemory;
                    ops.append(allocator, LineDiff{ kind: .insert, value: it.value }) catch return error.OutOfMemory;
                } else {
                    ops.append(allocator, it) catch return error.OutOfMemory;
                }
                o_scan += 1;
                n_scan += 1;
            },
            .delete => {
                ops.append(allocator, it) catch return error.OutOfMemory;
                o_scan += 1;
            },
            .insert => {
                ops.append(allocator, it) catch return error.OutOfMemory;
                n_scan += 1;
            },
        }
    }

    const n = ops.items.len;
    if (n == 0) {
        let empty: []u8 = [];
        return empty;
    }

    // Precompute 1-based old/new line indices at each op position
    let old_idx = allocator.alloc(usize, n + 1) catch return error.OutOfMemory;
    defer allocator.free(old_idx);
    let new_idx = allocator.alloc(usize, n + 1) catch return error.OutOfMemory;
    defer allocator.free(new_idx);
    let o_cur: usize = 0;
    let n_cur: usize = 0;
    let idx: usize = 0;
    while (idx < n) : (idx += 1) {
        old_idx[idx] = o_cur;
        new_idx[idx] = n_cur;
        switch (ops.items[idx].kind) {
            .equal => {
                o_cur += 1;
                n_cur += 1;
            },
            .delete => {
                o_cur += 1;
            },
            .insert => {
                n_cur += 1;
            },
        }
    }
    old_idx[n] = o_cur;
    new_idx[n] = n_cur;

    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    out.appendSlice(allocator, "--- ") catch return error.OutOfMemory;
    out.appendSlice(allocator, old_label) catch return error.OutOfMemory;
    out.append(allocator, '\n') catch return error.OutOfMemory;
    out.appendSlice(allocator, "+++ ") catch return error.OutOfMemory;
    out.appendSlice(allocator, new_label) catch return error.OutOfMemory;
    out.append(allocator, '\n') catch return error.OutOfMemory;

    // Group non-equal ops into unified hunks separated by > 2 * context_lines equal lines
    let pos: usize = 0;
    while (pos < n) {
        while (pos < n && ops.items[pos].kind == .equal) : (pos += 1) {}
        if (pos >= n) break;

        const hunk_start = if (pos > context_lines) pos - context_lines else 0;
        let last_change = pos;
        let scan = pos;
        while (scan < n) {
            if (ops.items[scan].kind != .equal) {
                last_change = scan;
                scan += 1;
            } else {
                const eq_start = scan;
                while (scan < n && ops.items[scan].kind == .equal) : (scan += 1) {}
                const eq_len = scan - eq_start;
                if (scan >= n || eq_len > (2 * context_lines)) {
                    break;
                }
            }
        }
        const hunk_end_raw = last_change + 1 + context_lines;
        const hunk_end = if (hunk_end_raw < n) hunk_end_raw else n;

        const old_count = old_idx[hunk_end] - old_idx[hunk_start];
        const new_count = new_idx[hunk_end] - new_idx[hunk_start];
        const old_start = if (old_count == 0) old_idx[hunk_start] else old_idx[hunk_start] + 1;
        const new_start = if (new_count == 0) new_idx[hunk_start] else new_idx[hunk_start] + 1;

        out.appendSlice(allocator, "@@ -") catch return error.OutOfMemory;
        try appendUsizeDecimal(allocator, *out, old_start);
        out.append(allocator, ',') catch return error.OutOfMemory;
        try appendUsizeDecimal(allocator, *out, old_count);
        out.appendSlice(allocator, " +") catch return error.OutOfMemory;
        try appendUsizeDecimal(allocator, *out, new_start);
        out.append(allocator, ',') catch return error.OutOfMemory;
        try appendUsizeDecimal(allocator, *out, new_count);
        out.appendSlice(allocator, " @@\n") catch return error.OutOfMemory;

        let hi = hunk_start;
        while (hi < hunk_end) : (hi += 1) {
            const op = ops.items[hi];
            out.append(allocator, op.kind.prefixByte()) catch return error.OutOfMemory;
            out.appendSlice(allocator, op.value) catch return error.OutOfMemory;
            out.append(allocator, '\n') catch return error.OutOfMemory;

            if (op.kind == .delete && !old_split.ends_with_nl && (old_idx[hi] + 1 == old_split.lines.len)) {
                out.appendSlice(allocator, "\\ No newline at end of file\n") catch return error.OutOfMemory;
            } else if (op.kind == .insert && !new_split.ends_with_nl && (new_idx[hi] + 1 == new_split.lines.len)) {
                out.appendSlice(allocator, "\\ No newline at end of file\n") catch return error.OutOfMemory;
            } else if (op.kind == .equal && !new_split.ends_with_nl && (new_idx[hi] + 1 == new_split.lines.len)) {
                out.appendSlice(allocator, "\\ No newline at end of file\n") catch return error.OutOfMemory;
            }
        }

        pos = hunk_end;
    }

    return out.toOwnedSlice(allocator) catch return error.OutOfMemory;
}
