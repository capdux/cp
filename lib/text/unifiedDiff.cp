// Origin: Pure-CP Stdlib (AI Coding Agent Unified Diff Hunk Parser & Zero-Alloc Patch Applier) | Created: 2026-11-22
// Zero-heap-allocation Unified Diff (@@ -a,b +c,d @@) parser, strict context verifier,
// and two-pass atomic patch applier designed for AI coding agents.

const std = @import("std");

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
    has_nl: bool = false;
    ok: bool = false;
}

fn readNextLine(src: string, pos: *usize): LineSlice {
    const start = pos.*;
    if (start >= src.len) {
        return LineSlice{ text: "", has_nl: false, ok: false };
    }
    let p: usize = start;
    while (p < src.len && src[p] != '\n') : (p += 1) {}
    if (p < src.len && src[p] == '\n') {
        pos.* = p + 1;
        return LineSlice{
            text: stripTrailingCr(src[start..p]),
            has_nl: true,
            ok: true,
        };
    }
    pos.* = p;
    return LineSlice{
        text: stripTrailingCr(src[start..p]),
        has_nl: false,
        ok: true,
    };
}

fn startsWith(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    return std.mem.eql(u8, s[0..prefix.len], prefix);
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
    let last_emitted_had_nl: bool = false;

    while (true) {
        const pl = readNextLine(patch, *patch_pos);
        if (!pl.ok) break;

        if (!startsWith(pl.text, "@@")) {
            if (hunks_applied == 0 && isIgnorableHeaderLine(pl.text)) {
                continue;
            }
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
            if (!emitBytes(out_buf, *out_pos, ol.text, write_enabled)) {
                return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
            }
            if (ol.has_nl) {
                if (!emitByte(out_buf, *out_pos, '\n', write_enabled)) {
                    return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
                }
            }
            last_emitted_had_nl = ol.has_nl;
            orig_line_no += 1;
        }

        // Apply hunk operations
        let seen_old: u32 = 0;
        let seen_new: u32 = 0;
        let last_op_was_output: bool = false;

        while (seen_old < hdr.old_count || seen_new < hdr.new_count) {
            const op_line = readNextLine(patch, *patch_pos);
            if (!op_line.ok) {
                return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
            }

            if (startsWith(op_line.text, "\\ ")) {
                // "\ No newline at end of file" applies to the immediately preceding output line
                if (last_op_was_output && last_emitted_had_nl && out_pos > 0) {
                    out_pos -= 1;
                    last_emitted_had_nl = false;
                }
                continue;
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

            if (op_kind == DiffLineKind.Context) {
                if (seen_old >= hdr.old_count || seen_new >= hdr.new_count) {
                    return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
                }
                const ol = readNextLine(original, *orig_pos);
                if (!ol.ok) {
                    return PatchApplyResult{ status: PatchApplyStatus.LineOutOfRange };
                }
                if (!std.mem.eql(u8, ol.text, op_text)) {
                    return PatchApplyResult{ status: PatchApplyStatus.ContextMismatch };
                }
                if (!emitBytes(out_buf, *out_pos, ol.text, write_enabled)) {
                    return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
                }
                if (ol.has_nl) {
                    if (!emitByte(out_buf, *out_pos, '\n', write_enabled)) {
                        return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
                    }
                }
                last_emitted_had_nl = ol.has_nl;
                last_op_was_output = true;
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
                if (!std.mem.eql(u8, ol.text, op_text)) {
                    return PatchApplyResult{ status: PatchApplyStatus.ContextMismatch };
                }
                last_op_was_output = false;
                orig_line_no += 1;
                seen_old += 1;
                lines_removed += 1;
            } else {
                // DiffLineKind.Add
                if (seen_new >= hdr.new_count) {
                    return PatchApplyResult{ status: PatchApplyStatus.InvalidHunk };
                }
                if (!emitBytes(out_buf, *out_pos, op_text, write_enabled)) {
                    return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
                }
                if (!emitByte(out_buf, *out_pos, '\n', write_enabled)) {
                    return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
                }
                last_emitted_had_nl = true;
                last_op_was_output = true;
                seen_new += 1;
                lines_added += 1;
            }
        }

        // Consume optional trailing "\ No newline at end of file" right after the last hunk line
        const saved_patch_pos = patch_pos;
        const peek_line = readNextLine(patch, *patch_pos);
        if (peek_line.ok && startsWith(peek_line.text, "\\ ")) {
            if (last_op_was_output && last_emitted_had_nl && out_pos > 0) {
                out_pos -= 1;
                last_emitted_had_nl = false;
            }
        } else {
            patch_pos = saved_patch_pos;
        }

        hunks_applied += 1;
    }

    // Copy any remaining untouched lines from original
    while (orig_pos < original.len) {
        const ol = readNextLine(original, *orig_pos);
        if (!ol.ok) break;
        if (!emitBytes(out_buf, *out_pos, ol.text, write_enabled)) {
            return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
        }
        if (ol.has_nl) {
            if (!emitByte(out_buf, *out_pos, '\n', write_enabled)) {
                return PatchApplyResult{ status: PatchApplyStatus.BufferTooSmall };
            }
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
