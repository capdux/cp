// Source: cp/lib/diff3/diff3.cp (Three-Way Text Merge & Conflict Resolution Engine)
// Pure CP zero-external-dependency 3-way merge engine based on Myers O(ND) alignment,
// supporting Standard and Diff3 conflict marker styles, identical-change resolution,
// and CRLF/LF line-ending preservation.

const std = @import("std");

/// Errors returned by 3-way merge operations.
pub err Diff3Error {
    OutOfMemory,
}

/// Conflict marker formatting style.
pub enum ConflictStyle: u8 {
    /// Standard 2-way conflict markers: <<<<<<< / ======= / >>>>>>>
    Standard = 0,
    /// Diff3 3-way conflict markers including base: <<<<<<< / ||||||| / ======= / >>>>>>>
    Diff3 = 1,
}

/// Lines involved in a single unresolved merge conflict.
pub struct ConflictBlock {
    base: []const []const u8 = [],
    ours: []const []const u8 = [],
    theirs: []const []const u8 = [],
}

/// A single hunk in a 3-way merge result, either cleanly merged lines or a conflict block.
pub enum MergeHunk {
    ok: []const []const u8,
    conflict: ConflictBlock,
}

/// Options controlling conflict marker labels, formatting style, and identical-change handling.
pub struct MergeOptions {
    ours_label: []const u8 = "ours",
    base_label: []const u8 = "base",
    theirs_label: []const u8 = "theirs",
    style: ConflictStyle = ConflictStyle.Standard,
    resolve_identical_changes: bool = true,
}

/// Result of a 3-way merge operation.
/// Caller must call `deinit(allocator)` to release owned hunks and `merged_text`.
pub struct MergeResult {
    has_conflicts: bool = false,
    conflict_count: usize = 0,
    hunks: []MergeHunk = [],
    merged_text: []u8 = [],

    /// Frees all memory owned by this `MergeResult`.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        for (hunk of _.hunks) {
            switch (hunk) {
                .ok lines => {
                    if (lines.len > 0) {
                        allocator.free(lines);
                    }
                },
                .conflict blk => {
                    if (blk.base.len > 0) {
                        allocator.free(blk.base);
                    }
                    if (blk.ours.len > 0) {
                        allocator.free(blk.ours);
                    }
                    if (blk.theirs.len > 0) {
                        allocator.free(blk.theirs);
                    }
                },
            }
        }
        if (_.hunks.len > 0) {
            allocator.free(_.hunks);
        }
        if (_.merged_text.len > 0) {
            allocator.free(_.merged_text);
        }
        _.hunks = [];
        _.merged_text = [];
    }
}

const MYERS_MAX_D: usize = 4096;

enum EditKind: u8 {
    Equal = 0,
    Insert = 1,
    Delete = 2,
}

struct LineEdit {
    kind: EditKind = EditKind.Equal,
    old_idx: usize = 0,
    new_idx: usize = 0,
}

struct ChangeHunk {
    base_start: usize = 0,
    base_end: usize = 0,
    rep_start: usize = 0,
    rep_end: usize = 0,
}

fn hashLine(s: []const u8): u64 {
    let h: u64 = 14695981039346656037;
    let i: usize = 0;
    while (i < s.len) {
        h = (h ^ u64(s[i])) *% 1099511628211;
        i += 1;
    }
    return h;
}

fn linesEqualAt(
    a: []const []const u8,
    ha: []const u64,
    ia: usize,
    b: []const []const u8,
    hb: []const u64,
    ib: usize,
): bool {
    if (ha[ia] != hb[ib]) {
        return false;
    }
    return std.mem.eql(u8, a[ia], b[ib]);
}

fn lineSlicesEqual(a: []const []const u8, b: []const []const u8): bool {
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

fn diffLineSlices(
    alloc: std.mem.Allocator,
    old_lines: []const []const u8,
    new_lines: []const []const u8,
): ([]LineEdit) ! Diff3Error {
    let edits: std.ArrayListUnmanaged(LineEdit) = .empty;
    errdefer edits.deinit(alloc);

    if (old_lines.len == 0 && new_lines.len == 0) {
        return edits.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
    }

    // 1. Trim common prefix
    let prefix: usize = 0;
    while (prefix < old_lines.len && prefix < new_lines.len && std.mem.eql(u8, old_lines[prefix], new_lines[prefix])) {
        prefix += 1;
    }

    // 2. Trim common suffix
    let old_rem: usize = old_lines.len - prefix;
    let new_rem: usize = new_lines.len - prefix;
    let suffix: usize = 0;
    while (suffix < old_rem && suffix < new_rem) {
        const oi = old_lines.len - 1 - suffix;
        const ni = new_lines.len - 1 - suffix;
        if (!std.mem.eql(u8, old_lines[oi], new_lines[ni])) {
            break;
        }
        suffix += 1;
    }

    // Emit prefix equals
    let p: usize = 0;
    while (p < prefix) {
        edits.append(alloc, LineEdit{
            kind: EditKind.Equal,
            old_idx: p,
            new_idx: p,
        }) catch return Diff3Error.OutOfMemory;
        p += 1;
    }

    const mid_old = old_lines[prefix .. old_lines.len - suffix];
    const mid_new = new_lines[prefix .. new_lines.len - suffix];
    const n = mid_old.len;
    const m = mid_new.len;

    if (n == 0) {
        let j: usize = 0;
        while (j < m) {
            edits.append(alloc, LineEdit{
                kind: EditKind.Insert,
                old_idx: prefix,
                new_idx: prefix + j,
            }) catch return Diff3Error.OutOfMemory;
            j += 1;
        }
    } else if (m == 0) {
        let i: usize = 0;
        while (i < n) {
            edits.append(alloc, LineEdit{
                kind: EditKind.Delete,
                old_idx: prefix + i,
                new_idx: prefix,
            }) catch return Diff3Error.OutOfMemory;
            i += 1;
        }
    } else {
        // Compute FNV-1a hashes for fast comparison
        let ha = alloc.alloc(u64, n) catch return Diff3Error.OutOfMemory;
        defer alloc.free(ha);
        let hb = alloc.alloc(u64, m) catch return Diff3Error.OutOfMemory;
        defer alloc.free(hb);

        let hi: usize = 0;
        while (hi < n) {
            ha[hi] = hashLine(mid_old[hi]);
            hi += 1;
        }
        hi = 0;
        while (hi < m) {
            hb[hi] = hashLine(mid_new[hi]);
            hi += 1;
        }

        const max_d = @min(n + m, MYERS_MAX_D);
        const v_len = 2 * max_d + 1;
        const offset: isize = @intCast(max_d);

        let v = alloc.alloc(isize, v_len) catch return Diff3Error.OutOfMemory;
        defer alloc.free(v);
        @memset(v, -1);

        let trace: std.ArrayListUnmanaged([]isize) = .empty;
        defer {
            for (snap of trace.items) {
                alloc.free(snap);
            }
            trace.deinit(alloc);
        }

        const start_idx: usize = @intCast(offset + 1);
        v[start_idx] = 0;
        let found_d: usize? = null;
        let d: usize = 0;
        while (d <= max_d) {
            let snap = alloc.alloc(isize, v_len) catch return Diff3Error.OutOfMemory;
            @memcpy(snap, v);
            trace.append(alloc, snap) catch {
                alloc.free(snap);
                return Diff3Error.OutOfMemory;
            };

            const d_isize: isize = @intCast(d);
            let k: isize = -d_isize;
            while (k <= d_isize) {
                const idx = k + offset;
                const idx_m1: usize = @intCast(idx - 1);
                const idx_p1: usize = @intCast(idx + 1);
                let x: isize = 0;
                if (k == -d_isize || (k != d_isize && v[idx_m1] < v[idx_p1])) {
                    x = v[idx_p1];
                } else {
                    x = v[idx_m1] + 1;
                }
                let y = x - k;
                while (x >= 0 && y >= 0 && @as(usize, @intCast(x)) < n && @as(usize, @intCast(y)) < m &&
                    linesEqualAt(mid_old, ha, @as(usize, @intCast(x)), mid_new, hb, @as(usize, @intCast(y))))
                {
                    x += 1;
                    y += 1;
                }
                const idx_u: usize = @intCast(idx);
                v[idx_u] = x;
                if (@as(usize, @intCast(x)) >= n && @as(usize, @intCast(y)) >= m) {
                    found_d = d;
                    break;
                }
                k += 2;
            }
            if (found_d != null) {
                break;
            }
            d += 1;
        }

        if (final_d := found_d) {
            let rev_edits: std.ArrayListUnmanaged(LineEdit) = .empty;
            defer rev_edits.deinit(alloc);

            let cx: isize = @intCast(n);
            let cy: isize = @intCast(m);
            let step: isize = @intCast(final_d);
            while (step > 0) {
                const step_u: usize = @intCast(step);
                const snap = trace.items[step_u];
                const k = cx - cy;
                const idx = k + offset;
                const idx_m1: usize = @intCast(idx - 1);
                const idx_p1: usize = @intCast(idx + 1);
                let prev_k: isize = 0;
                if (k == -step || (k != step && snap[idx_m1] < snap[idx_p1])) {
                    prev_k = k + 1;
                } else {
                    prev_k = k - 1;
                }
                const prev_idx_u: usize = @intCast(prev_k + offset);
                const prev_x = snap[prev_idx_u];
                const prev_y = prev_x - prev_k;

                while (cx > prev_x && cy > prev_y) {
                    cx -= 1;
                    cy -= 1;
                    rev_edits.append(alloc, LineEdit{
                        kind: EditKind.Equal,
                        old_idx: prefix + @as(usize, @intCast(cx)),
                        new_idx: prefix + @as(usize, @intCast(cy)),
                    }) catch return Diff3Error.OutOfMemory;
                }
                if (cx > prev_x) {
                    cx -= 1;
                    rev_edits.append(alloc, LineEdit{
                        kind: EditKind.Delete,
                        old_idx: prefix + @as(usize, @intCast(cx)),
                        new_idx: prefix + @as(usize, @intCast(cy)),
                    }) catch return Diff3Error.OutOfMemory;
                } else if (cy > prev_y) {
                    cy -= 1;
                    rev_edits.append(alloc, LineEdit{
                        kind: EditKind.Insert,
                        old_idx: prefix + @as(usize, @intCast(cx)),
                        new_idx: prefix + @as(usize, @intCast(cy)),
                    }) catch return Diff3Error.OutOfMemory;
                }
                step -= 1;
            }
            while (cx > 0 && cy > 0) {
                cx -= 1;
                cy -= 1;
                rev_edits.append(alloc, LineEdit{
                    kind: EditKind.Equal,
                    old_idx: prefix + @as(usize, @intCast(cx)),
                    new_idx: prefix + @as(usize, @intCast(cy)),
                }) catch return Diff3Error.OutOfMemory;
            }

            // Reverse into edits
            let ri = rev_edits.items.len;
            while (ri > 0) {
                ri -= 1;
                edits.append(alloc, rev_edits.items[ri]) catch return Diff3Error.OutOfMemory;
            }
        } else {
            // Fallback for huge diffs exceeding MYERS_MAX_D
            let i: usize = 0;
            while (i < n) {
                edits.append(alloc, LineEdit{
                    kind: EditKind.Delete,
                    old_idx: prefix + i,
                    new_idx: prefix,
                }) catch return Diff3Error.OutOfMemory;
                i += 1;
            }
            let j: usize = 0;
            while (j < m) {
                edits.append(alloc, LineEdit{
                    kind: EditKind.Insert,
                    old_idx: prefix + n,
                    new_idx: prefix + j,
                }) catch return Diff3Error.OutOfMemory;
                j += 1;
            }
        }
    }

    // Emit suffix equals
    let s: usize = 0;
    while (s < suffix) {
        edits.append(alloc, LineEdit{
            kind: EditKind.Equal,
            old_idx: old_lines.len - suffix + s,
            new_idx: new_lines.len - suffix + s,
        }) catch return Diff3Error.OutOfMemory;
        s += 1;
    }

    return edits.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
}

fn extractChangeHunks(alloc: std.mem.Allocator, edits: []const LineEdit): ([]ChangeHunk) ! Diff3Error {
    let hunks: std.ArrayListUnmanaged(ChangeHunk) = .empty;
    errdefer hunks.deinit(alloc);

    let i: usize = 0;
    let base_cursor: usize = 0;
    let rep_cursor: usize = 0;

    while (i < edits.len) {
        if (edits[i].kind == EditKind.Equal) {
            base_cursor = edits[i].old_idx + 1;
            rep_cursor = edits[i].new_idx + 1;
            i += 1;
            continue;
        }

        const base_start = base_cursor;
        const rep_start = rep_cursor;
        let base_end = base_cursor;
        let rep_end = rep_cursor;

        while (i < edits.len && edits[i].kind != EditKind.Equal) {
            if (edits[i].kind == EditKind.Delete) {
                base_end = edits[i].old_idx + 1;
                base_cursor = base_end;
            } else {
                rep_end = edits[i].new_idx + 1;
                rep_cursor = rep_end;
            }
            i += 1;
        }

        hunks.append(alloc, ChangeHunk{
            base_start: base_start,
            base_end: base_end,
            rep_start: rep_start,
            rep_end: rep_end,
        }) catch return Diff3Error.OutOfMemory;
    }

    return hunks.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
}

fn hunksOverlap(a: ChangeHunk, b: ChangeHunk): bool {
    if (a.base_end < b.base_start || b.base_end < a.base_start) {
        return false;
    }
    if (a.base_end == b.base_start) {
        return a.base_start == a.base_end && b.base_start == b.base_end;
    }
    if (b.base_end == a.base_start) {
        return a.base_start == a.base_end && b.base_start == b.base_end;
    }
    return true;
}

fn rangeOverlapsHunk(win_start: usize, win_end: usize, h: ChangeHunk): bool {
    const w = ChangeHunk{
        base_start: win_start,
        base_end: win_end,
        rep_start: 0,
        rep_end: 0,
    };
    return hunksOverlap(w, h);
}

fn buildRegionLines(
    alloc: std.mem.Allocator,
    base_lines: []const []const u8,
    side_lines: []const []const u8,
    hunks: []const ChangeHunk,
    h_start: usize,
    h_end: usize,
    win_start: usize,
    win_end: usize,
): ([]const []const u8) ! Diff3Error {
    let out: std.ArrayListUnmanaged(string) = .empty;
    errdefer out.deinit(alloc);

    let pos = win_start;
    let hi = h_start;
    while (hi < h_end) {
        const h = hunks[hi];
        while (pos < h.base_start) {
            out.append(alloc, base_lines[pos]) catch return Diff3Error.OutOfMemory;
            pos += 1;
        }
        let ri = h.rep_start;
        while (ri < h.rep_end) {
            out.append(alloc, side_lines[ri]) catch return Diff3Error.OutOfMemory;
            ri += 1;
        }
        pos = h.base_end;
        hi += 1;
    }
    while (pos < win_end) {
        out.append(alloc, base_lines[pos]) catch return Diff3Error.OutOfMemory;
        pos += 1;
    }

    return out.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
}

fn flushOkBuf(
    alloc: std.mem.Allocator,
    hunks: *std.ArrayListUnmanaged(MergeHunk),
    ok_buf: *std.ArrayListUnmanaged(string),
): void ! Diff3Error {
    if (ok_buf.items.len == 0) {
        return;
    }
    const owned = ok_buf.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
    hunks.append(alloc, MergeHunk{ .ok = owned }) catch {
        alloc.free(owned);
        return Diff3Error.OutOfMemory;
    };
}

fn appendMarkerLine(
    buf: *std.ArrayListUnmanaged(u8),
    alloc: std.mem.Allocator,
    prefix: []const u8,
    label: []const u8,
    eol: []const u8,
): void ! Diff3Error {
    buf.appendSlice(alloc, prefix) catch return Diff3Error.OutOfMemory;
    if (label.len > 0) {
        buf.append(alloc, ' ') catch return Diff3Error.OutOfMemory;
        buf.appendSlice(alloc, label) catch return Diff3Error.OutOfMemory;
    }
    buf.appendSlice(alloc, eol) catch return Diff3Error.OutOfMemory;
}

fn renderMergedText(
    alloc: std.mem.Allocator,
    hunks: []const MergeHunk,
    opts: MergeOptions,
    eol: []const u8,
    trailing_eol: bool,
): ([]u8) ! Diff3Error {
    let buf: std.ArrayListUnmanaged(u8) = .empty;
    errdefer buf.deinit(alloc);

    let total_lines: usize = 0;
    for (hunk of hunks) {
        switch (hunk) {
            .ok lines => {
                total_lines += lines.len;
            },
            .conflict blk => {
                total_lines += blk.ours.len + blk.theirs.len + 3;
                if (opts.style == ConflictStyle.Diff3) {
                    total_lines += blk.base.len + 1;
                }
            },
        }
    }

    if (total_lines == 0) {
        return buf.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
    }

    let emitted: usize = 0;
    for (hunk of hunks) {
        switch (hunk) {
            .ok lines => {
                for (line of lines) {
                    buf.appendSlice(alloc, line) catch return Diff3Error.OutOfMemory;
                    emitted += 1;
                    if (emitted < total_lines || trailing_eol) {
                        buf.appendSlice(alloc, eol) catch return Diff3Error.OutOfMemory;
                    }
                }
            },
            .conflict blk => {
                appendMarkerLine(&buf, alloc, "<<<<<<<", opts.ours_label, eol)?;
                emitted += 1;
                for (line of blk.ours) {
                    buf.appendSlice(alloc, line) catch return Diff3Error.OutOfMemory;
                    buf.appendSlice(alloc, eol) catch return Diff3Error.OutOfMemory;
                    emitted += 1;
                }
                if (opts.style == ConflictStyle.Diff3) {
                    appendMarkerLine(&buf, alloc, "|||||||", opts.base_label, eol)?;
                    emitted += 1;
                    for (line of blk.base) {
                        buf.appendSlice(alloc, line) catch return Diff3Error.OutOfMemory;
                        buf.appendSlice(alloc, eol) catch return Diff3Error.OutOfMemory;
                        emitted += 1;
                    }
                }
                buf.appendSlice(alloc, "=======") catch return Diff3Error.OutOfMemory;
                buf.appendSlice(alloc, eol) catch return Diff3Error.OutOfMemory;
                emitted += 1;
                for (line of blk.theirs) {
                    buf.appendSlice(alloc, line) catch return Diff3Error.OutOfMemory;
                    buf.appendSlice(alloc, eol) catch return Diff3Error.OutOfMemory;
                    emitted += 1;
                }
                buf.appendSlice(alloc, ">>>>>>>") catch return Diff3Error.OutOfMemory;
                if (opts.theirs_label.len > 0) {
                    buf.append(alloc, ' ') catch return Diff3Error.OutOfMemory;
                    buf.appendSlice(alloc, opts.theirs_label) catch return Diff3Error.OutOfMemory;
                }
                emitted += 1;
                if (emitted < total_lines || trailing_eol) {
                    buf.appendSlice(alloc, eol) catch return Diff3Error.OutOfMemory;
                }
            },
        }
    }

    return buf.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
}

fn freeHunkList(alloc: std.mem.Allocator, list: *std.ArrayListUnmanaged(MergeHunk)) {
    for (hunk of list.items) {
        switch (hunk) {
            .ok lines => {
                if (lines.len > 0) {
                    alloc.free(lines);
                }
            },
            .conflict blk => {
                if (blk.base.len > 0) {
                    alloc.free(blk.base);
                }
                if (blk.ours.len > 0) {
                    alloc.free(blk.ours);
                }
                if (blk.theirs.len > 0) {
                    alloc.free(blk.theirs);
                }
            },
        }
    }
    list.deinit(alloc);
}

fn mergeLinesInternal(
    allocator: std.mem.Allocator,
    base_lines: []const []const u8,
    ours_lines: []const []const u8,
    theirs_lines: []const []const u8,
    opts: MergeOptions,
    eol: []const u8,
    trailing_eol: bool,
): MergeResult ! Diff3Error {
    const ours_edits = diffLineSlices(allocator, base_lines, ours_lines)?;
    defer allocator.free(ours_edits);
    const theirs_edits = diffLineSlices(allocator, base_lines, theirs_lines)?;
    defer allocator.free(theirs_edits);

    const ours_hunks = extractChangeHunks(allocator, ours_edits)?;
    defer allocator.free(ours_hunks);
    const theirs_hunks = extractChangeHunks(allocator, theirs_edits)?;
    defer allocator.free(theirs_hunks);

    let hunks: std.ArrayListUnmanaged(MergeHunk) = .empty;
    errdefer freeHunkList(allocator, &hunks);

    let ok_buf: std.ArrayListUnmanaged(string) = .empty;
    defer ok_buf.deinit(allocator);

    let base_pos: usize = 0;
    let oi: usize = 0;
    let ti: usize = 0;
    let conflict_count: usize = 0;

    while (oi < ours_hunks.len && ti < theirs_hunks.len) {
        const oh = ours_hunks[oi];
        const th = theirs_hunks[ti];

        if (!hunksOverlap(oh, th)) {
            const pick_ours = (oh.base_end < th.base_start) ||
                (oh.base_end == th.base_start && !(oh.base_start == oh.base_end && th.base_start == th.base_end) &&
                    (oh.base_start < th.base_start || oh.base_end <= th.base_start));
            if (pick_ours) {
                while (base_pos < oh.base_start) {
                    ok_buf.append(allocator, base_lines[base_pos]) catch return Diff3Error.OutOfMemory;
                    base_pos += 1;
                }
                let ri = oh.rep_start;
                while (ri < oh.rep_end) {
                    ok_buf.append(allocator, ours_lines[ri]) catch return Diff3Error.OutOfMemory;
                    ri += 1;
                }
                base_pos = oh.base_end;
                oi += 1;
            } else {
                while (base_pos < th.base_start) {
                    ok_buf.append(allocator, base_lines[base_pos]) catch return Diff3Error.OutOfMemory;
                    base_pos += 1;
                }
                let ri = th.rep_start;
                while (ri < th.rep_end) {
                    ok_buf.append(allocator, theirs_lines[ri]) catch return Diff3Error.OutOfMemory;
                    ri += 1;
                }
                base_pos = th.base_end;
                ti += 1;
            }
            continue;
        }

        // Expand overlapping conflict window across both sides
        let win_start = @min(oh.base_start, th.base_start);
        let win_end = @max(oh.base_end, th.base_end);
        let oi_end = oi + 1;
        let ti_end = ti + 1;

        let expanded = true;
        while (expanded) {
            expanded = false;
            while (oi_end < ours_hunks.len && rangeOverlapsHunk(win_start, win_end, ours_hunks[oi_end])) {
                if (ours_hunks[oi_end].base_start < win_start) {
                    win_start = ours_hunks[oi_end].base_start;
                }
                if (ours_hunks[oi_end].base_end > win_end) {
                    win_end = ours_hunks[oi_end].base_end;
                }
                oi_end += 1;
                expanded = true;
            }
            while (ti_end < theirs_hunks.len && rangeOverlapsHunk(win_start, win_end, theirs_hunks[ti_end])) {
                if (theirs_hunks[ti_end].base_start < win_start) {
                    win_start = theirs_hunks[ti_end].base_start;
                }
                if (theirs_hunks[ti_end].base_end > win_end) {
                    win_end = theirs_hunks[ti_end].base_end;
                }
                ti_end += 1;
                expanded = true;
            }
        }

        while (base_pos < win_start) {
            ok_buf.append(allocator, base_lines[base_pos]) catch return Diff3Error.OutOfMemory;
            base_pos += 1;
        }

        const ours_reg = buildRegionLines(allocator, base_lines, ours_lines, ours_hunks, oi, oi_end, win_start, win_end)?;
        const theirs_reg = buildRegionLines(allocator, base_lines, theirs_lines, theirs_hunks, ti, ti_end, win_start, win_end) catch {
            if (ours_reg.len > 0) {
                allocator.free(ours_reg);
            }
            return Diff3Error.OutOfMemory;
        };

        if (opts.resolve_identical_changes && lineSlicesEqual(ours_reg, theirs_reg)) {
            defer {
                if (ours_reg.len > 0) {
                    allocator.free(ours_reg);
                }
                if (theirs_reg.len > 0) {
                    allocator.free(theirs_reg);
                }
            }
            for (line of ours_reg) {
                ok_buf.append(allocator, line) catch return Diff3Error.OutOfMemory;
            }
        } else {
            flushOkBuf(allocator, &hunks, &ok_buf) catch {
                if (ours_reg.len > 0) {
                    allocator.free(ours_reg);
                }
                if (theirs_reg.len > 0) {
                    allocator.free(theirs_reg);
                }
                return Diff3Error.OutOfMemory;
            };

            const base_reg = allocator.dupe(string, base_lines[win_start..win_end]) catch {
                if (ours_reg.len > 0) {
                    allocator.free(ours_reg);
                }
                if (theirs_reg.len > 0) {
                    allocator.free(theirs_reg);
                }
                return Diff3Error.OutOfMemory;
            };

            conflict_count += 1;
            hunks.append(allocator, MergeHunk{
                .conflict = ConflictBlock{
                    base: base_reg,
                    ours: ours_reg,
                    theirs: theirs_reg,
                },
            }) catch {
                if (base_reg.len > 0) {
                    allocator.free(base_reg);
                }
                if (ours_reg.len > 0) {
                    allocator.free(ours_reg);
                }
                if (theirs_reg.len > 0) {
                    allocator.free(theirs_reg);
                }
                return Diff3Error.OutOfMemory;
            };
        }

        base_pos = win_end;
        oi = oi_end;
        ti = ti_end;
    }

    while (oi < ours_hunks.len) {
        const oh = ours_hunks[oi];
        while (base_pos < oh.base_start) {
            ok_buf.append(allocator, base_lines[base_pos]) catch return Diff3Error.OutOfMemory;
            base_pos += 1;
        }
        let ri = oh.rep_start;
        while (ri < oh.rep_end) {
            ok_buf.append(allocator, ours_lines[ri]) catch return Diff3Error.OutOfMemory;
            ri += 1;
        }
        base_pos = oh.base_end;
        oi += 1;
    }

    while (ti < theirs_hunks.len) {
        const th = theirs_hunks[ti];
        while (base_pos < th.base_start) {
            ok_buf.append(allocator, base_lines[base_pos]) catch return Diff3Error.OutOfMemory;
            base_pos += 1;
        }
        let ri = th.rep_start;
        while (ri < th.rep_end) {
            ok_buf.append(allocator, theirs_lines[ri]) catch return Diff3Error.OutOfMemory;
            ri += 1;
        }
        base_pos = th.base_end;
        ti += 1;
    }

    while (base_pos < base_lines.len) {
        ok_buf.append(allocator, base_lines[base_pos]) catch return Diff3Error.OutOfMemory;
        base_pos += 1;
    }

    flushOkBuf(allocator, &hunks, &ok_buf)?;

    const merged_text = renderMergedText(allocator, hunks.items, opts, eol, trailing_eol)?;
    const owned_hunks = hunks.toOwnedSlice(allocator) catch {
        if (merged_text.len > 0) {
            allocator.free(merged_text);
        }
        return Diff3Error.OutOfMemory;
    };

    return MergeResult{
        has_conflicts: conflict_count > 0,
        conflict_count: conflict_count,
        hunks: owned_hunks,
        merged_text: merged_text,
    };
}

/// Performs a 3-way merge on pre-split line slices.
/// Caller owns the returned `MergeResult` and must call `result.deinit(allocator)`.
pub fn mergeLines(
    allocator: std.mem.Allocator,
    base_lines: []const []const u8,
    ours_lines: []const []const u8,
    theirs_lines: []const []const u8,
    opts: MergeOptions,
): MergeResult ! Diff3Error {
    return mergeLinesInternal(allocator, base_lines, ours_lines, theirs_lines, opts, "\n", true);
}

fn trimTrailingCr(s: []const u8): []const u8 {
    if (s.len > 0 && s[s.len - 1] == '\r') {
        return s[0 .. s.len - 1];
    }
    return s;
}

fn splitLinesZeroCopy(alloc: std.mem.Allocator, text: []const u8): ([]const []const u8) ! Diff3Error {
    let list: std.ArrayListUnmanaged(string) = .empty;
    errdefer list.deinit(alloc);

    if (text.len == 0) {
        return list.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
    }

    let start: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        if (text[i] == '\n') {
            list.append(alloc, trimTrailingCr(text[start..i])) catch return Diff3Error.OutOfMemory;
            start = i + 1;
        }
        i += 1;
    }

    if (start < text.len) {
        list.append(alloc, trimTrailingCr(text[start..text.len])) catch return Diff3Error.OutOfMemory;
    }

    return list.toOwnedSlice(alloc) catch return Diff3Error.OutOfMemory;
}

fn containsCrlf(text: []const u8): bool {
    return std.mem.indexOf(u8, text, "\r\n") != null;
}

fn endsWithNewline(text: []const u8): bool {
    return text.len > 0 && text[text.len - 1] == '\n';
}

/// Performs a 3-way merge on raw text strings, automatically handling `\n` and `\r\n` line endings.
/// Caller owns the returned `MergeResult` and must call `result.deinit(allocator)`.
pub fn mergeTexts(
    allocator: std.mem.Allocator,
    base_text: []const u8,
    ours_text: []const u8,
    theirs_text: []const u8,
    opts: MergeOptions,
): MergeResult ! Diff3Error {
    const base_lines = splitLinesZeroCopy(allocator, base_text)?;
    defer if (base_lines.len > 0) allocator.free(base_lines);

    const ours_lines = splitLinesZeroCopy(allocator, ours_text)?;
    defer if (ours_lines.len > 0) allocator.free(ours_lines);

    const theirs_lines = splitLinesZeroCopy(allocator, theirs_text)?;
    defer if (theirs_lines.len > 0) allocator.free(theirs_lines);

    const is_crlf = containsCrlf(base_text) || containsCrlf(ours_text) || containsCrlf(theirs_text);
    const eol: []const u8 = if (is_crlf) "\r\n" else "\n";
    const trailing_eol = endsWithNewline(base_text) || endsWithNewline(ours_text) || endsWithNewline(theirs_text);

    return mergeLinesInternal(allocator, base_lines, ours_lines, theirs_lines, opts, eol, trailing_eol);
}

/// Returns `true` if a 3-way merge of `base_text`, `ours_text`, and `theirs_text` produces any conflicts.
pub fn hasConflicts(
    base_text: []const u8,
    ours_text: []const u8,
    theirs_text: []const u8,
): bool {
    let tmp_arena = std.heap.ArenaAllocator.init(std.heap.page_allocator);
    defer tmp_arena.deinit();
    const alloc = tmp_arena.allocator();

    let res = mergeTexts(alloc, base_text, ours_text, theirs_text, MergeOptions{}) catch return false;
    return res.has_conflicts;
}
