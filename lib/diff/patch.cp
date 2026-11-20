// Source: cp/lib/diff/patch.cp (Unified Diff Generator, Strict Patch Applier & 3-Way Merge Engine)
// Pure CP zero-external-dependency Unified Diff generator, strict context-verifying
// patch applier, and 3-way merge conflict detector.

const std = @import("std");
import {
    DiffError,
    DiffOpKind,
    DiffEdit,
    DiffEngine,
    splitLinesAlloc,
} from "./myers.cp";

/// Result of a 3-way merge operation (`UnifiedDiff.merge3`).
pub struct MergeResult {
    clean: bool = true,
    conflict_count: usize = 0,
    merged_text: string = "",
}

enum PatchLineKind: u8 {
    Context = 0,
    Delete = 1,
    Insert = 2,
}

struct PatchLine {
    kind: PatchLineKind = PatchLineKind.Context,
    text: string = "",
}

struct ParsedHunk {
    old_start: usize = 0,
    old_len: usize = 0,
    new_start: usize = 0,
    new_len: usize = 0,
    lines_start: usize = 0,
    lines_end: usize = 0,
    no_nl_new: bool = false,
}

struct ChangeHunk {
    base_start: usize = 0,
    base_end: usize = 0,
    rep_start: usize = 0,
    rep_end: usize = 0,
}

fn appendUsize(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, val: usize) ! DiffError {
    let tmp: [32]u8 = undefined;
    const s = std.fmt.bufPrint(&tmp, "{d}", .{val}) catch return DiffError.OutOfMemory;
    buf.appendSlice(alloc, s) catch return DiffError.OutOfMemory;
}

fn appendStr(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, s: string) ! DiffError {
    buf.appendSlice(alloc, s) catch return DiffError.OutOfMemory;
}

fn parseUsize(s: string): usize? {
    if (s.len == 0) {
        return null;
    }
    let val: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < '0' || ch > '9') {
            return null;
        }
        val = val * 10 + usize(ch - '0');
        i += 1;
    }
    return val;
}

fn parseRangeSpec(spec: string): [usize, usize]? {
    if (spec.len == 0) {
        return null;
    }
    let comma_idx: usize? = null;
    let i: usize = 0;
    while (i < spec.len) {
        if (spec[i] == ',') {
            comma_idx = i;
            break;
        }
        i += 1;
    }
    if (cidx := comma_idx) {
        const start = parseUsize(spec[0..cidx]) ?? return null;
        const len = parseUsize(spec[cidx + 1 .. spec.len]) ?? return null;
        return [start, len];
    }
    const start = parseUsize(spec) ?? return null;
    return [start, 1];
}

fn parseHunkHeader(line: string): [usize, usize, usize, usize]? {
    if (!std.mem.startsWith(u8, line, "@@ -")) {
        return null;
    }
    const rest = line[4..line.len];
    let space_idx: usize? = null;
    let i: usize = 0;
    while (i < rest.len) {
        if (rest[i] == ' ') {
            space_idx = i;
            break;
        }
        i += 1;
    }
    const sp = space_idx ?? return null;
    const old_spec = rest[0..sp];
    const [old_start, old_len] = parseRangeSpec(old_spec) ?? return null;

    const after_space = rest[sp + 1 .. rest.len];
    if (!std.mem.startsWith(u8, after_space, "+")) {
        return null;
    }
    const new_part = after_space[1..after_space.len];
    let end_idx: usize? = null;
    let j: usize = 0;
    while (j < new_part.len) {
        if (new_part[j] == ' ') {
            end_idx = j;
            break;
        }
        j += 1;
    }
    const ej = end_idx ?? return null;
    const new_spec = new_part[0..ej];
    const [new_start, new_len] = parseRangeSpec(new_spec) ?? return null;

    const tail = new_part[ej + 1 .. new_part.len];
    if (!std.mem.startsWith(u8, tail, "@@")) {
        return null;
    }
    return [old_start, old_len, new_start, new_len];
}

fn isPatchHeaderMeta(line: string): bool {
    return std.mem.startsWith(u8, line, "diff ") ||
           std.mem.startsWith(u8, line, "index ") ||
           std.mem.startsWith(u8, line, "--- ") ||
           std.mem.startsWith(u8, line, "+++ ") ||
           std.mem.startsWith(u8, line, "new file mode ") ||
           std.mem.startsWith(u8, line, "deleted file mode ");
}

fn extractChangeHunks(alloc: std.mem.Allocator, edits: []const DiffEdit): ([]ChangeHunk) ! DiffError {
    let hunks: std.ArrayListUnmanaged(ChangeHunk) = .empty;
    errdefer hunks.deinit(alloc);

    let base_pos: usize = 0;
    let rep_pos: usize = 0;
    let i: usize = 0;

    while (i < edits.len) {
        if (edits[i].kind == .Equal) {
            base_pos += 1;
            rep_pos += 1;
            i += 1;
            continue;
        }
        const b_start = base_pos;
        const r_start = rep_pos;
        while (i < edits.len && edits[i].kind != .Equal) {
            if (edits[i].kind == .Delete) {
                base_pos += 1;
            } else {
                rep_pos += 1;
            }
            i += 1;
        }
        hunks.append(alloc, ChangeHunk{
            base_start: b_start,
            base_end: base_pos,
            rep_start: r_start,
            rep_end: rep_pos,
        }) catch return DiffError.OutOfMemory;
    }

    return hunks.toOwnedSlice(alloc) catch return DiffError.OutOfMemory;
}

fn hunksOverlap(a: ChangeHunk, b: ChangeHunk): bool {
    if (a.base_start == a.base_end && b.base_start == b.base_end) {
        return a.base_start == b.base_start;
    }
    if (a.base_start == a.base_end) {
        return a.base_start >= b.base_start && a.base_start <= b.base_end;
    }
    if (b.base_start == b.base_end) {
        return b.base_start >= a.base_start && b.base_start <= a.base_end;
    }
    return a.base_start < b.base_end && b.base_start < a.base_end;
}

fn buildRegionLines(
    alloc: std.mem.Allocator,
    base_lines: []const string,
    side_lines: []const string,
    hunks: []const ChangeHunk,
    h_start: usize,
    h_end: usize,
    win_start: usize,
    win_end: usize
): ([]string) ! DiffError {
    let out: std.ArrayListUnmanaged(string) = .empty;
    errdefer out.deinit(alloc);

    let pos = win_start;
    let hi = h_start;
    while (hi < h_end) {
        const h = hunks[hi];
        while (pos < h.base_start) {
            out.append(alloc, base_lines[pos]) catch return DiffError.OutOfMemory;
            pos += 1;
        }
        let ri = h.rep_start;
        while (ri < h.rep_end) {
            out.append(alloc, side_lines[ri]) catch return DiffError.OutOfMemory;
            ri += 1;
        }
        pos = h.base_end;
        hi += 1;
    }
    while (pos < win_end) {
        out.append(alloc, base_lines[pos]) catch return DiffError.OutOfMemory;
        pos += 1;
    }

    return out.toOwnedSlice(alloc) catch return DiffError.OutOfMemory;
}

fn lineSlicesEqual(a: []const string, b: []const string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (!std.mem.eql(u8, a[i], b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn joinLinesAlloc(alloc: std.mem.Allocator, lines: []const string, trailing_nl: bool): string ! DiffError {
    if (lines.len == 0) {
        return alloc.alloc(u8, 0) catch return DiffError.OutOfMemory;
    }
    let buf: std.ArrayListUnmanaged(u8) = .empty;
    errdefer buf.deinit(alloc);

    let i: usize = 0;
    while (i < lines.len) {
        buf.appendSlice(alloc, lines[i]) catch return DiffError.OutOfMemory;
        if (i + 1 < lines.len || trailing_nl) {
            buf.append(alloc, '\n') catch return DiffError.OutOfMemory;
        }
        i += 1;
    }
    return buf.toOwnedSlice(alloc) catch return DiffError.OutOfMemory;
}

/// Standard Unified Diff generator, strict patch applier, and 3-way merge engine.
pub static class UnifiedDiff {
    /// Generates a standard Unified Diff patch between `old_text` and `new_text`.
    /// Returns an empty string when `old_text` and `new_text` are byte-identical.
    pub fn createPatch(
        alloc: std.mem.Allocator,
        old_name: string,
        new_name: string,
        old_text: string,
        new_text: string,
        context_lines: usize
    ): string ! DiffError {
        if (std.mem.eql(u8, old_text, new_text)) {
            return alloc.alloc(u8, 0) catch return DiffError.OutOfMemory;
        }

        const old_has_nl = old_text.len > 0 && old_text[old_text.len - 1] == '\n';
        const new_has_nl = new_text.len > 0 && new_text[new_text.len - 1] == '\n';

        const old_lines = splitLinesAlloc(alloc, old_text)?;
        defer alloc.free(old_lines);
        const new_lines = splitLinesAlloc(alloc, new_text)?;
        defer alloc.free(new_lines);

        const raw_edits = DiffEngine.diffLines(alloc, old_text, new_text)?;
        defer alloc.free(raw_edits);

        // Copy into a mutable list so we can split the final Equal line when only trailing newline differs.
        let edits: std.ArrayListUnmanaged(DiffEdit) = .empty;
        defer edits.deinit(alloc);
        edits.appendSlice(alloc, raw_edits) catch return DiffError.OutOfMemory;

        let has_change = false;
        let ci: usize = 0;
        while (ci < edits.items.len) {
            if (edits.items[ci].kind != .Equal) {
                has_change = true;
                break;
            }
            ci += 1;
        }

        if (!has_change && old_has_nl != new_has_nl && edits.items.len > 0) {
            const last_idx = edits.items.len - 1;
            const last_edit = edits.items[last_idx];
            edits.items[last_idx] = DiffEdit{
                kind: DiffOpKind.Delete,
                text: last_edit.text,
                old_line: last_edit.old_line,
                new_line: 0,
            };
            edits.append(alloc, DiffEdit{
                kind: DiffOpKind.Insert,
                text: last_edit.text,
                old_line: 0,
                new_line: last_edit.new_line,
            }) catch return DiffError.OutOfMemory;
        }

        let buf: std.ArrayListUnmanaged(u8) = .empty;
        errdefer buf.deinit(alloc);

        // Emit file headers
        appendStr(&buf, alloc, "--- ")?;
        if (!std.mem.startsWith(u8, old_name, "a/") && !std.mem.eql(u8, old_name, "/dev/null")) {
            appendStr(&buf, alloc, "a/")?;
        }
        appendStr(&buf, alloc, old_name)?;
        appendStr(&buf, alloc, "\n+++ ")?;
        if (!std.mem.startsWith(u8, new_name, "b/") && !std.mem.eql(u8, new_name, "/dev/null")) {
            appendStr(&buf, alloc, "b/")?;
        }
        appendStr(&buf, alloc, new_name)?;
        appendStr(&buf, alloc, "\n")?;

        // Group non-Equal runs into Unified Diff hunks
        let idx: usize = 0;
        while (idx < edits.items.len) {
            if (edits.items[idx].kind == .Equal) {
                idx += 1;
                continue;
            }

            const h_start = if (idx > context_lines) idx - context_lines else 0;
            let h_end = idx;

            while (h_end < edits.items.len) {
                while (h_end < edits.items.len && edits.items[h_end].kind != .Equal) {
                    h_end += 1;
                }
                // Count trailing Equal lines until next change or end
                let eq_run: usize = 0;
                while (h_end + eq_run < edits.items.len && edits.items[h_end + eq_run].kind == .Equal) {
                    eq_run += 1;
                }
                if (h_end + eq_run >= edits.items.len) {
                    const take = if (eq_run < context_lines) eq_run else context_lines;
                    h_end += take;
                    break;
                }
                if (eq_run <= context_lines * 2) {
                    h_end += eq_run;
                } else {
                    h_end += context_lines;
                    break;
                }
            }

            // Compute old_before / new_before up to h_start
            let old_before: usize = 0;
            let new_before: usize = 0;
            let bi: usize = 0;
            while (bi < h_start) {
                if (edits.items[bi].kind != .Insert) {
                    old_before += 1;
                }
                if (edits.items[bi].kind != .Delete) {
                    new_before += 1;
                }
                bi += 1;
            }

            let old_count: usize = 0;
            let new_count: usize = 0;
            let hi = h_start;
            while (hi < h_end) {
                if (edits.items[hi].kind != .Insert) {
                    old_count += 1;
                }
                if (edits.items[hi].kind != .Delete) {
                    new_count += 1;
                }
                hi += 1;
            }

            const old_start = if (old_count == 0) old_before else old_before + 1;
            const new_start = if (new_count == 0) new_before else new_before + 1;

            appendStr(&buf, alloc, "@@ -")?;
            appendUsize(&buf, alloc, old_start)?;
            appendStr(&buf, alloc, ",")?;
            appendUsize(&buf, alloc, old_count)?;
            appendStr(&buf, alloc, " +")?;
            appendUsize(&buf, alloc, new_start)?;
            appendStr(&buf, alloc, ",")?;
            appendUsize(&buf, alloc, new_count)?;
            appendStr(&buf, alloc, " @@\n")?;

            hi = h_start;
            while (hi < h_end) {
                const e = edits.items[hi];
                switch (e.kind) {
                    .Equal => {
                        appendStr(&buf, alloc, " ")?;
                        appendStr(&buf, alloc, e.text)?;
                        appendStr(&buf, alloc, "\n")?;
                        if (e.old_line == old_lines.len && !old_has_nl && e.new_line == new_lines.len && !new_has_nl) {
                            appendStr(&buf, alloc, "\\ No newline at end of file\n")?;
                        }
                    },
                    .Delete => {
                        appendStr(&buf, alloc, "-")?;
                        appendStr(&buf, alloc, e.text)?;
                        appendStr(&buf, alloc, "\n")?;
                        if (e.old_line == old_lines.len && !old_has_nl) {
                            appendStr(&buf, alloc, "\\ No newline at end of file\n")?;
                        }
                    },
                    .Insert => {
                        appendStr(&buf, alloc, "+")?;
                        appendStr(&buf, alloc, e.text)?;
                        appendStr(&buf, alloc, "\n")?;
                        if (e.new_line == new_lines.len && !new_has_nl) {
                            appendStr(&buf, alloc, "\\ No newline at end of file\n")?;
                        }
                    },
                }
                hi += 1;
            }

            idx = h_end;
        }

        return buf.toOwnedSlice(alloc) catch return DiffError.OutOfMemory;
    }

    /// Strictly parses and applies a Unified Diff `patch_text` to `original_text`.
    /// Validates hunk headers, line counts, and exact context/deleted line contents.
    pub fn applyPatch(alloc: std.mem.Allocator, original_text: string, patch_text: string): string ! DiffError {
        if (patch_text.len == 0) {
            return alloc.dupe(u8, original_text) catch return DiffError.OutOfMemory;
        }

        const orig_has_nl = original_text.len > 0 && original_text[original_text.len - 1] == '\n';
        const orig_lines = splitLinesAlloc(alloc, original_text)?;
        defer alloc.free(orig_lines);

        const patch_lines = splitLinesAlloc(alloc, patch_text)?;
        defer alloc.free(patch_lines);

        let hunks: std.ArrayListUnmanaged(ParsedHunk) = .empty;
        defer hunks.deinit(alloc);
        let plines: std.ArrayListUnmanaged(PatchLine) = .empty;
        defer plines.deinit(alloc);

        let pi: usize = 0;
        while (pi < patch_lines.len) {
            const line = patch_lines[pi];
            if (!std.mem.startsWith(u8, line, "@@ ")) {
                if (isPatchHeaderMeta(line) || line.len == 0) {
                    pi += 1;
                    continue;
                }
                return DiffError.InvalidPatch;
            }

            const [old_start, old_len, new_start, new_len] = parseHunkHeader(line) ?? return DiffError.InvalidPatch;
            pi += 1;

            const l_start = plines.items.len;
            let seen_old: usize = 0;
            let seen_new: usize = 0;
            let last_kind: PatchLineKind? = null;
            let no_nl_new = false;

            while (pi < patch_lines.len) {
                const body_line = patch_lines[pi];
                if (std.mem.startsWith(u8, body_line, "@@ ") || isPatchHeaderMeta(body_line)) {
                    break;
                }
                if (seen_old == old_len && seen_new == new_len && !std.mem.startsWith(u8, body_line, "\\")) {
                    if (body_line.len == 0) {
                        pi += 1;
                        continue;
                    }
                    break;
                }
                if (body_line.len == 0) {
                    return DiffError.InvalidPatch;
                }

                const prefix = body_line[0];
                const content = body_line[1..body_line.len];
                if (prefix == ' ') {
                    plines.append(alloc, PatchLine{ kind: PatchLineKind.Context, text: content }) catch return DiffError.OutOfMemory;
                    seen_old += 1;
                    seen_new += 1;
                    last_kind = PatchLineKind.Context;
                } else if (prefix == '-') {
                    plines.append(alloc, PatchLine{ kind: PatchLineKind.Delete, text: content }) catch return DiffError.OutOfMemory;
                    seen_old += 1;
                    last_kind = PatchLineKind.Delete;
                } else if (prefix == '+') {
                    plines.append(alloc, PatchLine{ kind: PatchLineKind.Insert, text: content }) catch return DiffError.OutOfMemory;
                    seen_new += 1;
                    last_kind = PatchLineKind.Insert;
                } else if (prefix == '\\') {
                    if (lk := last_kind) {
                        if (lk == .Insert || lk == .Context) {
                            no_nl_new = true;
                        }
                    }
                } else {
                    return DiffError.InvalidPatch;
                }
                pi += 1;
            }

            if (seen_old != old_len || seen_new != new_len) {
                return DiffError.InvalidPatch;
            }

            hunks.append(alloc, ParsedHunk{
                old_start: old_start,
                old_len: old_len,
                new_start: new_start,
                new_len: new_len,
                lines_start: l_start,
                lines_end: plines.items.len,
                no_nl_new: no_nl_new,
            }) catch return DiffError.OutOfMemory;
        }

        if (hunks.items.len == 0) {
            return DiffError.InvalidPatch;
        }

        let out_lines: std.ArrayListUnmanaged(string) = .empty;
        defer out_lines.deinit(alloc);

        let cursor: usize = 0;
        let touched_eof = false;
        let eof_no_nl = false;

        let hi: usize = 0;
        while (hi < hunks.items.len) {
            const h = hunks.items[hi];
            let start_idx: usize = 0;
            if (h.old_len == 0) {
                start_idx = h.old_start;
            } else {
                if (h.old_start == 0) {
                    return DiffError.HunkMismatch;
                }
                start_idx = h.old_start - 1;
            }

            if (start_idx < cursor || start_idx + h.old_len > orig_lines.len) {
                return DiffError.HunkMismatch;
            }

            while (cursor < start_idx) {
                out_lines.append(alloc, orig_lines[cursor]) catch return DiffError.OutOfMemory;
                cursor += 1;
            }

            let orig_idx = start_idx;
            let li = h.lines_start;
            while (li < h.lines_end) {
                const pl = plines.items[li];
                switch (pl.kind) {
                    .Context => {
                        if (!std.mem.eql(u8, orig_lines[orig_idx], pl.text)) {
                            return DiffError.HunkMismatch;
                        }
                        out_lines.append(alloc, pl.text) catch return DiffError.OutOfMemory;
                        orig_idx += 1;
                    },
                    .Delete => {
                        if (!std.mem.eql(u8, orig_lines[orig_idx], pl.text)) {
                            return DiffError.HunkMismatch;
                        }
                        orig_idx += 1;
                    },
                    .Insert => {
                        out_lines.append(alloc, pl.text) catch return DiffError.OutOfMemory;
                    },
                }
                li += 1;
            }

            cursor = start_idx + h.old_len;
            if (cursor == orig_lines.len) {
                touched_eof = true;
                eof_no_nl = h.no_nl_new;
            }
            hi += 1;
        }

        while (cursor < orig_lines.len) {
            out_lines.append(alloc, orig_lines[cursor]) catch return DiffError.OutOfMemory;
            cursor += 1;
        }

        const out_has_nl = if (touched_eof) !eof_no_nl else orig_has_nl;
        return joinLinesAlloc(alloc, out_lines.items, out_has_nl);
    }

    /// Performs a 3-way merge of `ours_text` and `theirs_text` against common ancestor `base_text`.
    /// Non-overlapping or identical changes merge cleanly; overlapping divergent changes emit
    /// standard `<<<<<<< ours` / `=======` / `>>>>>>> theirs` conflict markers.
    pub fn merge3(
        alloc: std.mem.Allocator,
        base_text: string,
        ours_text: string,
        theirs_text: string
    ): MergeResult ! DiffError {
        const base_has_nl = base_text.len > 0 && base_text[base_text.len - 1] == '\n';
        const ours_has_nl = ours_text.len > 0 && ours_text[ours_text.len - 1] == '\n';
        const theirs_has_nl = theirs_text.len > 0 && theirs_text[theirs_text.len - 1] == '\n';
        const out_has_nl = base_has_nl || ours_has_nl || theirs_has_nl;

        const base_lines = splitLinesAlloc(alloc, base_text)?;
        defer alloc.free(base_lines);
        const ours_lines = splitLinesAlloc(alloc, ours_text)?;
        defer alloc.free(ours_lines);
        const theirs_lines = splitLinesAlloc(alloc, theirs_text)?;
        defer alloc.free(theirs_lines);

        const ours_edits = DiffEngine.diffLines(alloc, base_text, ours_text)?;
        defer alloc.free(ours_edits);
        const theirs_edits = DiffEngine.diffLines(alloc, base_text, theirs_text)?;
        defer alloc.free(theirs_edits);

        const ours_hunks = extractChangeHunks(alloc, ours_edits)?;
        defer alloc.free(ours_hunks);
        const theirs_hunks = extractChangeHunks(alloc, theirs_edits)?;
        defer alloc.free(theirs_hunks);

        let out_lines: std.ArrayListUnmanaged(string) = .empty;
        defer out_lines.deinit(alloc);

        let base_pos: usize = 0;
        let oi: usize = 0;
        let ti: usize = 0;
        let conflict_count: usize = 0;

        while (oi < ours_hunks.len && ti < theirs_hunks.len) {
            const oh = ours_hunks[oi];
            const th = theirs_hunks[ti];

            if (!hunksOverlap(oh, th)) {
                if (oh.base_end <= th.base_start && (oh.base_start < th.base_start || oh.base_end < th.base_end)) {
                    while (base_pos < oh.base_start) {
                        out_lines.append(alloc, base_lines[base_pos]) catch return DiffError.OutOfMemory;
                        base_pos += 1;
                    }
                    let ri = oh.rep_start;
                    while (ri < oh.rep_end) {
                        out_lines.append(alloc, ours_lines[ri]) catch return DiffError.OutOfMemory;
                        ri += 1;
                    }
                    base_pos = oh.base_end;
                    oi += 1;
                } else {
                    while (base_pos < th.base_start) {
                        out_lines.append(alloc, base_lines[base_pos]) catch return DiffError.OutOfMemory;
                        base_pos += 1;
                    }
                    let ri = th.rep_start;
                    while (ri < th.rep_end) {
                        out_lines.append(alloc, theirs_lines[ri]) catch return DiffError.OutOfMemory;
                        ri += 1;
                    }
                    base_pos = th.base_end;
                    ti += 1;
                }
                continue;
            }

            // Expand overlapping conflict window across both sides
            const win_start = if (oh.base_start < th.base_start) oh.base_start else th.base_start;
            let win_end = if (oh.base_end > th.base_end) oh.base_end else th.base_end;
            let oi_end = oi + 1;
            let ti_end = ti + 1;

            let expanded = true;
            while (expanded) {
                expanded = false;
                while (oi_end < ours_hunks.len && ours_hunks[oi_end].base_start <= win_end) {
                    if (ours_hunks[oi_end].base_end > win_end) {
                        win_end = ours_hunks[oi_end].base_end;
                    }
                    oi_end += 1;
                    expanded = true;
                }
                while (ti_end < theirs_hunks.len && theirs_hunks[ti_end].base_start <= win_end) {
                    if (theirs_hunks[ti_end].base_end > win_end) {
                        win_end = theirs_hunks[ti_end].base_end;
                    }
                    ti_end += 1;
                    expanded = true;
                }
            }

            while (base_pos < win_start) {
                out_lines.append(alloc, base_lines[base_pos]) catch return DiffError.OutOfMemory;
                base_pos += 1;
            }

            const ours_reg = buildRegionLines(alloc, base_lines, ours_lines, ours_hunks, oi, oi_end, win_start, win_end)?;
            defer alloc.free(ours_reg);
            const theirs_reg = buildRegionLines(alloc, base_lines, theirs_lines, theirs_hunks, ti, ti_end, win_start, win_end)?;
            defer alloc.free(theirs_reg);

            if (lineSlicesEqual(ours_reg, theirs_reg)) {
                let k: usize = 0;
                while (k < ours_reg.len) {
                    out_lines.append(alloc, ours_reg[k]) catch return DiffError.OutOfMemory;
                    k += 1;
                }
            } else {
                conflict_count += 1;
                out_lines.append(alloc, "<<<<<<< ours") catch return DiffError.OutOfMemory;
                let k: usize = 0;
                while (k < ours_reg.len) {
                    out_lines.append(alloc, ours_reg[k]) catch return DiffError.OutOfMemory;
                    k += 1;
                }
                out_lines.append(alloc, "=======") catch return DiffError.OutOfMemory;
                k = 0;
                while (k < theirs_reg.len) {
                    out_lines.append(alloc, theirs_reg[k]) catch return DiffError.OutOfMemory;
                    k += 1;
                }
                out_lines.append(alloc, ">>>>>>> theirs") catch return DiffError.OutOfMemory;
            }

            base_pos = win_end;
            oi = oi_end;
            ti = ti_end;
        }

        while (oi < ours_hunks.len) {
            const oh = ours_hunks[oi];
            while (base_pos < oh.base_start) {
                out_lines.append(alloc, base_lines[base_pos]) catch return DiffError.OutOfMemory;
                base_pos += 1;
            }
            let ri = oh.rep_start;
            while (ri < oh.rep_end) {
                out_lines.append(alloc, ours_lines[ri]) catch return DiffError.OutOfMemory;
                ri += 1;
            }
            base_pos = oh.base_end;
            oi += 1;
        }

        while (ti < theirs_hunks.len) {
            const th = theirs_hunks[ti];
            while (base_pos < th.base_start) {
                out_lines.append(alloc, base_lines[base_pos]) catch return DiffError.OutOfMemory;
                base_pos += 1;
            }
            let ri = th.rep_start;
            while (ri < th.rep_end) {
                out_lines.append(alloc, theirs_lines[ri]) catch return DiffError.OutOfMemory;
                ri += 1;
            }
            base_pos = th.base_end;
            ti += 1;
        }

        while (base_pos < base_lines.len) {
            out_lines.append(alloc, base_lines[base_pos]) catch return DiffError.OutOfMemory;
            base_pos += 1;
        }

        const merged = joinLinesAlloc(alloc, out_lines.items, out_has_nl)?;
        return MergeResult{
            clean: conflict_count == 0,
            conflict_count: conflict_count,
            merged_text: merged,
        };
    }
}
