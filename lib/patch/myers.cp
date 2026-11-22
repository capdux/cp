// Source: Absorbed from /data/dev/bun/src/patch/patch.cp | Synced: 2026-11-22T12:49:17Z
// lib/patch/myers.cp
// Line-Level Myers O(ND) Text Differ, DiffOp Sequence & Standard Unified Diff Generator
// Absorbed from Bun src/patch/

const std = @import("std");

pub enum DiffOpKind: u8 {
    Equal = 0,
    Insert = 1,
    Delete = 2,
}

pub struct DiffOp {
    kind: DiffOpKind = DiffOpKind.Equal,
    line: []const u8 = "",
    old_line_no: usize = 0,
    new_line_no: usize = 0,
}

const STACK_MAX_LINES: usize = 256;
const STACK_MAX_D: usize = 48;
const STACK_V_LEN: usize = 99;
const STACK_TRACE_STARTS_LEN: usize = 49;
// Sum_{d=0..48} (2*d + 1) = 49^2 = 2401
const STACK_TRACE_CAP: usize = 2500;

pub struct LineDiffer {
    pub static fn splitLines(text: []const u8, out_lines: [][]const u8): usize {
        if (text.len == 0) {
            return 0;
        }
        let count: usize = 0;
        let iter = std.mem.splitScalar(u8, text, '\n');
        while (raw := iter.next()) {
            const line = std.mem.trimEnd(u8, raw, "\r");
            // Skip trailing empty segment after final newline
            if (line.len == 0 && iter.peek() == null) {
                break;
            }
            if (count >= out_lines.len) {
                break;
            }
            out_lines[count] = line;
            count += 1;
        }
        return count;
    }

    pub static fn splitLinesAlloc(allocator: std.mem.Allocator, text: []const u8, has_trailing_nl: *bool): std.ArrayListUnmanaged(string)! {
        let list: std.ArrayListUnmanaged(string) = .empty;
        errdefer list.deinit(allocator);
        has_trailing_nl.* = text.len > 0 && text[text.len - 1] == '\n';
        if (text.len == 0) {
            return list;
        }
        let iter = std.mem.splitScalar(u8, text, '\n');
        while (raw := iter.next()) {
            const line = std.mem.trimEnd(u8, raw, "\r");
            if (line.len == 0 && iter.peek() == null) {
                break;
            }
            try list.append(allocator, line);
        }
        return list;
    }

    /// Computes a minimal sequence of line-level `DiffOp` entries between `old_text` and `new_text` into `out_ops`.
    /// Uses common-prefix/suffix trimming + zero-heap Myers O(ND) shortest edit script for D <= 48.
    pub static fn diffLines(old_text: []const u8, new_text: []const u8, out_ops: []DiffOp): usize {
        let old_lines: [STACK_MAX_LINES][]const u8 = undefined;
        let new_lines: [STACK_MAX_LINES][]const u8 = undefined;
        const old_len = LineDiffer.splitLines(old_text, old_lines[0..STACK_MAX_LINES]);
        const new_len = LineDiffer.splitLines(new_text, new_lines[0..STACK_MAX_LINES]);

        let prefix: usize = 0;
        while (prefix < old_len && prefix < new_len && std.mem.eql(u8, old_lines[prefix], new_lines[prefix])) {
            prefix += 1;
        }

        let suffix: usize = 0;
        while ((prefix + suffix < old_len) &&
               (prefix + suffix < new_len) &&
               std.mem.eql(u8, old_lines[old_len - 1 - suffix], new_lines[new_len - 1 - suffix]))
        {
            suffix += 1;
        }

        let op_count: usize = 0;
        let i: usize = 0;
        while (i < prefix && op_count < out_ops.len) {
            out_ops[op_count] = DiffOp {
                kind: DiffOpKind.Equal,
                line: old_lines[i],
                old_line_no: i + 1,
                new_line_no: i + 1,
            };
            op_count += 1;
            i += 1;
        }

        const old_mid = old_lines[prefix .. old_len - suffix];
        const new_mid = new_lines[prefix .. new_len - suffix];

        if (old_mid.len == 0) {
            let ni: usize = 0;
            while (ni < new_mid.len && op_count < out_ops.len) {
                out_ops[op_count] = DiffOp {
                    kind: DiffOpKind.Insert,
                    line: new_mid[ni],
                    old_line_no: 0,
                    new_line_no: prefix + ni + 1,
                };
                op_count += 1;
                ni += 1;
            }
        } else if (new_mid.len == 0) {
            let oi: usize = 0;
            while (oi < old_mid.len && op_count < out_ops.len) {
                out_ops[op_count] = DiffOp {
                    kind: DiffOpKind.Delete,
                    line: old_mid[oi],
                    old_line_no: prefix + oi + 1,
                    new_line_no: 0,
                };
                op_count += 1;
                oi += 1;
            }
        } else {
            // Try zero-heap Myers O(ND)
            let mid_ops: [STACK_MAX_LINES]DiffOp = undefined;
            const mid_res = myersStack(old_mid, new_mid, prefix, mid_ops[0..STACK_MAX_LINES]);
            if (mid_count := mid_res) {
                let m: usize = 0;
                while (m < mid_count && op_count < out_ops.len) {
                    out_ops[op_count] = mid_ops[m];
                    op_count += 1;
                    m += 1;
                }
            } else {
                // Fallback if edit distance > STACK_MAX_D
                let oi: usize = 0;
                while (oi < old_mid.len && op_count < out_ops.len) {
                    out_ops[op_count] = DiffOp {
                        kind: DiffOpKind.Delete,
                        line: old_mid[oi],
                        old_line_no: prefix + oi + 1,
                        new_line_no: 0,
                    };
                    op_count += 1;
                    oi += 1;
                }
                let ni: usize = 0;
                while (ni < new_mid.len && op_count < out_ops.len) {
                    out_ops[op_count] = DiffOp {
                        kind: DiffOpKind.Insert,
                        line: new_mid[ni],
                        old_line_no: 0,
                        new_line_no: prefix + ni + 1,
                    };
                    op_count += 1;
                    ni += 1;
                }
            }
        }

        const old_mid_end = old_len - suffix;
        const new_mid_end = new_len - suffix;
        let s: usize = 0;
        while (s < suffix && op_count < out_ops.len) {
            const old_idx = old_mid_end + s;
            const new_idx = new_mid_end + s;
            out_ops[op_count] = DiffOp {
                kind: DiffOpKind.Equal,
                line: old_lines[old_idx],
                old_line_no: old_idx + 1,
                new_line_no: new_idx + 1,
            };
            op_count += 1;
            s += 1;
        }

        return op_count;
    }

    static fn myersStack(
        a: []const []const u8,
        b: []const []const u8,
        line_offset: usize,
        out_mid: []DiffOp,
    ): usize? {
        const n = a.len;
        const m = b.len;
        const max_d = if (n + m < STACK_MAX_D) n + m else STACK_MAX_D;

        let v: [STACK_V_LEN]isize = [0; STACK_V_LEN];
        let trace: [STACK_TRACE_CAP]isize = [0; STACK_TRACE_CAP];
        let trace_starts: [STACK_TRACE_STARTS_LEN]usize = [0; STACK_TRACE_STARTS_LEN];
        const off: isize = (STACK_MAX_D as isize) + 1;

        v[(off + 1) as usize] = 0;
        let trace_pos: usize = 0;
        let found_d: usize? = null;

        let d: usize = 0;
        while (d <= max_d) {
            const d_i: isize = d as isize;
            const row_len = 2 * d + 1;
            if (trace_pos + row_len > trace.len) {
                return null;
            }
            trace_starts[d] = trace_pos;
            let k: isize = -d_i;
            while (k <= d_i) {
                let x: isize = 0;
                if (k == -d_i || (k != d_i && v[(off + k - 1) as usize] < v[(off + k + 1) as usize])) {
                    x = v[(off + k + 1) as usize];
                } else {
                    x = v[(off + k - 1) as usize] + 1;
                }
                let y: isize = x - k;
                while (x < (n as isize) && y < (m as isize) && std.mem.eql(u8, a[x as usize], b[y as usize])) {
                    x += 1;
                    y += 1;
                }
                v[(off + k) as usize] = x;
                const row_idx = ((k + d_i) as usize);
                trace[trace_pos + row_idx] = x;
                if (x >= (n as isize) && y >= (m as isize)) {
                    found_d = d;
                    break;
                }
                k += 2;
            }
            trace_pos += row_len;
            if (found_d != null) break;
            d += 1;
        }

        if (found_d == null) return null;

        // Backtrack from found_d down to 0 in reverse
        let rev_count: usize = 0;
        let cx: isize = n as isize;
        let cy: isize = m as isize;
        let bd: usize = found_d.?;

        while (bd > 0) {
            const k: isize = cx - cy;
            const prev_d_i: isize = (bd - 1) as isize;
            const prev_row_start = trace_starts[bd - 1];

            let prev_k: isize = 0;
            if (k == -(bd as isize) ||
                (k != (bd as isize) &&
                 trace[prev_row_start + ((k - 1 + prev_d_i) as usize)] < trace[prev_row_start + ((k + 1 + prev_d_i) as usize)]))
            {
                prev_k = k + 1;
            } else {
                prev_k = k - 1;
            }

            const prev_x = trace[prev_row_start + ((prev_k + prev_d_i) as usize)];
            const prev_y = prev_x - prev_k;

            while (cx > prev_x && cy > prev_y) {
                cx -= 1;
                cy -= 1;
                if (rev_count >= out_mid.len) return null;
                out_mid[rev_count] = DiffOp {
                    kind: DiffOpKind.Equal,
                    line: a[cx as usize],
                    old_line_no: line_offset + (cx as usize) + 1,
                    new_line_no: line_offset + (cy as usize) + 1,
                };
                rev_count += 1;
            }

            if (cx == prev_x) {
                // Insertion
                if (rev_count >= out_mid.len) return null;
                out_mid[rev_count] = DiffOp {
                    kind: DiffOpKind.Insert,
                    line: b[prev_y as usize],
                    old_line_no: 0,
                    new_line_no: line_offset + (prev_y as usize) + 1,
                };
                rev_count += 1;
            } else {
                // Deletion
                if (rev_count >= out_mid.len) return null;
                out_mid[rev_count] = DiffOp {
                    kind: DiffOpKind.Delete,
                    line: a[prev_x as usize],
                    old_line_no: line_offset + (prev_x as usize) + 1,
                    new_line_no: 0,
                };
                rev_count += 1;
            }

            cx = prev_x;
            cy = prev_y;
            bd -= 1;
        }

        while (cx > 0 && cy > 0) {
            cx -= 1;
            cy -= 1;
            if (rev_count >= out_mid.len) return null;
            out_mid[rev_count] = DiffOp {
                kind: DiffOpKind.Equal,
                line: a[cx as usize],
                old_line_no: line_offset + (cx as usize) + 1,
                new_line_no: line_offset + (cy as usize) + 1,
            };
            rev_count += 1;
        }

        // Reverse out_mid[0..rev_count] in-place
        let lo: usize = 0;
        let hi: usize = rev_count;
        while (lo + 1 < hi) {
            hi -= 1;
            const tmp = out_mid[lo];
            out_mid[lo] = out_mid[hi];
            out_mid[hi] = tmp;
            lo += 1;
        }

        return rev_count;
    }

    /// Unbounded heap-backed Myers O(ND) line diff for arbitrary file lengths.
    pub static fn diffLinesAlloc(allocator: std.mem.Allocator, old_text: []const u8, new_text: []const u8): std.ArrayListUnmanaged(DiffOp)! {
        let old_nl = false;
        let new_nl = false;
        let old_list = try LineDiffer.splitLinesAlloc(allocator, old_text, &old_nl);
        defer old_list.deinit(allocator);
        let new_list = try LineDiffer.splitLinesAlloc(allocator, new_text, &new_nl);
        defer new_list.deinit(allocator);

        const old_lines = old_list.items;
        const new_lines = new_list.items;

        let prefix: usize = 0;
        while (prefix < old_lines.len && prefix < new_lines.len && std.mem.eql(u8, old_lines[prefix], new_lines[prefix])) {
            prefix += 1;
        }

        let suffix: usize = 0;
        while ((prefix + suffix < old_lines.len) &&
               (prefix + suffix < new_lines.len) &&
               std.mem.eql(u8, old_lines[old_lines.len - 1 - suffix], new_lines[new_lines.len - 1 - suffix]))
        {
            suffix += 1;
        }

        let ops: std.ArrayListUnmanaged(DiffOp) = .empty;
        errdefer ops.deinit(allocator);

        let i: usize = 0;
        while (i < prefix) {
            try ops.append(allocator, DiffOp {
                kind: DiffOpKind.Equal,
                line: old_lines[i],
                old_line_no: i + 1,
                new_line_no: i + 1,
            });
            i += 1;
        }

        const a = old_lines[prefix .. old_lines.len - suffix];
        const b = new_lines[prefix .. new_lines.len - suffix];

        if (a.len == 0) {
            let ni: usize = 0;
            while (ni < b.len) {
                try ops.append(allocator, DiffOp {
                    kind: DiffOpKind.Insert,
                    line: b[ni],
                    old_line_no: 0,
                    new_line_no: prefix + ni + 1,
                });
                ni += 1;
            }
        } else if (b.len == 0) {
            let oi: usize = 0;
            while (oi < a.len) {
                try ops.append(allocator, DiffOp {
                    kind: DiffOpKind.Delete,
                    line: a[oi],
                    old_line_no: prefix + oi + 1,
                    new_line_no: 0,
                });
                oi += 1;
            }
        } else {
            const n = a.len;
            const m = b.len;
            const max_d = n + m;
            const v_len = 2 * max_d + 3;
            let v = try allocator.alloc(isize, v_len);
            defer allocator.free(v);
            @memset(v, 0);

            let trace: std.ArrayListUnmanaged(isize) = .empty;
            defer trace.deinit(allocator);
            let trace_starts: std.ArrayListUnmanaged(usize) = .empty;
            defer trace_starts.deinit(allocator);

            const off: isize = (max_d as isize) + 1;
            v[(off + 1) as usize] = 0;
            let found_d: usize = max_d;

            let d: usize = 0;
            while (d <= max_d) {
                const d_i: isize = d as isize;
                try trace_starts.append(allocator, trace.items.len);
                const row_len = 2 * d + 1;
                try trace.ensureUnusedCapacity(allocator, row_len);

                let k: isize = -d_i;
                while (k <= d_i) {
                    let x: isize = 0;
                    if (k == -d_i || (k != d_i && v[(off + k - 1) as usize] < v[(off + k + 1) as usize])) {
                        x = v[(off + k + 1) as usize];
                    } else {
                        x = v[(off + k - 1) as usize] + 1;
                    }
                    let y: isize = x - k;
                    while (x < (n as isize) && y < (m as isize) && std.mem.eql(u8, a[x as usize], b[y as usize])) {
                        x += 1;
                        y += 1;
                    }
                    v[(off + k) as usize] = x;
                    // Fill trace slot sequentially for k = -d..d step 2
                    try trace.append(allocator, x);
                    if (k + 1 <= d_i) {
                        try trace.append(allocator, 0);
                    }
                    if (x >= (n as isize) && y >= (m as isize)) {
                        found_d = d;
                        break;
                    }
                    k += 2;
                }
                if (v[(off + ((n as isize) - (m as isize))) as usize] >= (n as isize) && found_d == d) {
                    break;
                }
                d += 1;
            }

            let mid_rev: std.ArrayListUnmanaged(DiffOp) = .empty;
            defer mid_rev.deinit(allocator);

            let cx: isize = n as isize;
            let cy: isize = m as isize;
            let bd: usize = found_d;
            while (bd > 0) {
                const k: isize = cx - cy;
                const prev_d_i: isize = (bd - 1) as isize;
                const prev_row_start = trace_starts.items[bd - 1];

                let prev_k: isize = 0;
                if (k == -(bd as isize) ||
                    (k != (bd as isize) &&
                     trace.items[prev_row_start + ((k - 1 + prev_d_i) as usize)] < trace.items[prev_row_start + ((k + 1 + prev_d_i) as usize)]))
                {
                    prev_k = k + 1;
                } else {
                    prev_k = k - 1;
                }

                const prev_x = trace.items[prev_row_start + ((prev_k + prev_d_i) as usize)];
                const prev_y = prev_x - prev_k;

                while (cx > prev_x && cy > prev_y) {
                    cx -= 1;
                    cy -= 1;
                    try mid_rev.append(allocator, DiffOp {
                        kind: DiffOpKind.Equal,
                        line: a[cx as usize],
                        old_line_no: prefix + (cx as usize) + 1,
                        new_line_no: prefix + (cy as usize) + 1,
                    });
                }

                if (cx == prev_x) {
                    try mid_rev.append(allocator, DiffOp {
                        kind: DiffOpKind.Insert,
                        line: b[prev_y as usize],
                        old_line_no: 0,
                        new_line_no: prefix + (prev_y as usize) + 1,
                    });
                } else {
                    try mid_rev.append(allocator, DiffOp {
                        kind: DiffOpKind.Delete,
                        line: a[prev_x as usize],
                        old_line_no: prefix + (prev_x as usize) + 1,
                        new_line_no: 0,
                    });
                }

                cx = prev_x;
                cy = prev_y;
                bd -= 1;
            }

            while (cx > 0 && cy > 0) {
                cx -= 1;
                cy -= 1;
                try mid_rev.append(allocator, DiffOp {
                    kind: DiffOpKind.Equal,
                    line: a[cx as usize],
                    old_line_no: prefix + (cx as usize) + 1,
                    new_line_no: prefix + (cy as usize) + 1,
                });
            }

            let r: usize = mid_rev.items.len;
            while (r > 0) {
                r -= 1;
                try ops.append(allocator, mid_rev.items[r]);
            }
        }

        const old_mid_end = old_lines.len - suffix;
        const new_mid_end = new_lines.len - suffix;
        let s: usize = 0;
        while (s < suffix) {
            const old_idx = old_mid_end + s;
            const new_idx = new_mid_end + s;
            try ops.append(allocator, DiffOp {
                kind: DiffOpKind.Equal,
                line: old_lines[old_idx],
                old_line_no: old_idx + 1,
                new_line_no: new_idx + 1,
            });
            s += 1;
        }

        return ops;
    }

    pub static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    /// Formats a slice of `DiffOp` operations into a unified diff body (` `, `-`, `+` lines) inside `out_buf`.
    pub static fn formatUnifiedToBuf(ops: []const DiffOp, out_buf: []u8): []const u8 {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < ops.len && pos < out_buf.len) {
            const op = ops[i];
            let prefix_ch: u8 = ' ';
            if (op.kind == DiffOpKind.Insert) {
                prefix_ch = '+';
            } else if (op.kind == DiffOpKind.Delete) {
                prefix_ch = '-';
            }
            if (pos + 1 + op.line.len + 1 > out_buf.len) {
                break;
            }
            out_buf[pos] = prefix_ch;
            pos += 1;
            pos = LineDiffer.appendSlice(out_buf, pos, op.line);
            out_buf[pos] = '\n';
            pos += 1;
            i += 1;
        }
        return out_buf[0..pos];
    }

    /// Generates a complete standard Unified Diff string (`--- a/...`, `+++ b/...`, `@@ -l,s +l,s @@`, hunks with context).
    pub static fn generateUnifiedDiff(
        allocator: std.mem.Allocator,
        old_text: []const u8,
        new_text: []const u8,
        old_path: []const u8,
        new_path: []const u8,
        context_lines: usize,
    ): []u8! {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        const old_has_nl = old_text.len > 0 && old_text[old_text.len - 1] == '\n';
        const new_has_nl = new_text.len > 0 && new_text[new_text.len - 1] == '\n';

        let ops_list = try LineDiffer.diffLinesAlloc(allocator, old_text, new_text);
        defer ops_list.deinit(allocator);
        const ops = ops_list.items;

        let has_changes = false;
        for (op of ops) {
            if (op.kind != DiffOpKind.Equal) {
                has_changes = true;
                break;
            }
        }
        if (!has_changes && old_has_nl == new_has_nl) {
            return try out.toOwnedSlice(allocator);
        }

        // Header lines
        if (old_text.len == 0) {
            try out.appendSlice(allocator, "--- /dev/null\n");
        } else {
            try out.appendSlice(allocator, "--- a/");
            try out.appendSlice(allocator, old_path);
            try out.append(allocator, '\n');
        }
        if (new_text.len == 0) {
            try out.appendSlice(allocator, "+++ /dev/null\n");
        } else {
            try out.appendSlice(allocator, "+++ b/");
            try out.appendSlice(allocator, new_path);
            try out.append(allocator, '\n');
        }

        // Handle pure newline difference with identical text lines
        if (!has_changes && ops.len > 0) {
            const last_idx = ops.len - 1;
            const start_idx = if (last_idx > context_lines) last_idx - context_lines else 0;
            const count = ops.len - start_idx;
            const start_no = start_idx + 1;
            let hdr_buf: [128]u8 = undefined;
            const hdr = std.fmt.bufPrint(&hdr_buf, "@@ -{d},{d} +{d},{d} @@\n", .{ start_no, count, start_no, count }) catch "";
            try out.appendSlice(allocator, hdr);
            let k: usize = start_idx;
            while (k < last_idx) {
                try out.append(allocator, ' ');
                try out.appendSlice(allocator, ops[k].line);
                try out.append(allocator, '\n');
                k += 1;
            }
            try out.append(allocator, '-');
            try out.appendSlice(allocator, ops[last_idx].line);
            try out.append(allocator, '\n');
            if (!old_has_nl) {
                try out.appendSlice(allocator, "\\ No newline at end of file\n");
            }
            try out.append(allocator, '+');
            try out.appendSlice(allocator, ops[last_idx].line);
            try out.append(allocator, '\n');
            if (!new_has_nl) {
                try out.appendSlice(allocator, "\\ No newline at end of file\n");
            }
            return try out.toOwnedSlice(allocator);
        }

        // Group ops into hunks with context_lines
        let idx: usize = 0;
        while (idx < ops.len) {
            if (ops[idx].kind == DiffOpKind.Equal) {
                idx += 1;
                continue;
            }

            const hunk_start = if (idx > context_lines) idx - context_lines else 0;
            let hunk_end = idx + 1;
            let scan = idx + 1;
            while (scan < ops.len) {
                if (ops[scan].kind != DiffOpKind.Equal) {
                    hunk_end = scan + 1;
                    scan += 1;
                } else {
                    // Check if another change exists within 2 * context_lines
                    let eq_run: usize = 0;
                    let look = scan;
                    while (look < ops.len && ops[look].kind == DiffOpKind.Equal) {
                        eq_run += 1;
                        look += 1;
                    }
                    if (look < ops.len && eq_run <= context_lines * 2) {
                        hunk_end = look + 1;
                        scan = look + 1;
                    } else {
                        break;
                    }
                }
            }
            const trailing_ctx = if (hunk_end + context_lines < ops.len) hunk_end + context_lines else ops.len;
            hunk_end = trailing_ctx;

            // Compute old_start, old_count, new_start, new_count
            let old_count: usize = 0;
            let new_count: usize = 0;
            let first_old: usize? = null;
            let first_new: usize? = null;
            let prev_old: usize = 0;
            let prev_new: usize = 0;
            let p: usize = 0;
            while (p < hunk_start) {
                if (ops[p].kind != DiffOpKind.Insert) prev_old += 1;
                if (ops[p].kind != DiffOpKind.Delete) prev_new += 1;
                p += 1;
            }

            let h: usize = hunk_start;
            while (h < hunk_end) {
                const op = ops[h];
                if (op.kind != DiffOpKind.Insert) {
                    if (first_old == null) first_old = op.old_line_no;
                    old_count += 1;
                }
                if (op.kind != DiffOpKind.Delete) {
                    if (first_new == null) first_new = op.new_line_no;
                    new_count += 1;
                }
                h += 1;
            }

            const old_start = if (old_count == 0) prev_old else (first_old orelse 1);
            const new_start = if (new_count == 0) prev_new else (first_new orelse 1);

            let hdr_buf: [128]u8 = undefined;
            const hdr = std.fmt.bufPrint(&hdr_buf, "@@ -{d},{d} +{d},{d} @@\n", .{ old_start, old_count, new_start, new_count }) catch "";
            try out.appendSlice(allocator, hdr);

            // Emit hunk body with accurate "\ No newline at end of file" pragmas
            const touches_eof = (hunk_end == ops.len);
            let last_old_idx: usize? = null;
            let last_new_idx: usize? = null;
            if (touches_eof) {
                let r: usize = hunk_end;
                while (r > hunk_start) {
                    r -= 1;
                    if (last_old_idx == null && ops[r].kind != DiffOpKind.Insert) last_old_idx = r;
                    if (last_new_idx == null && ops[r].kind != DiffOpKind.Delete) last_new_idx = r;
                    if (last_old_idx != null && last_new_idx != null) break;
                }
            }

            h = hunk_start;
            while (h < hunk_end) {
                const op = ops[h];
                let ch: u8 = ' ';
                if (op.kind == DiffOpKind.Insert) ch = '+';
                if (op.kind == DiffOpKind.Delete) ch = '-';
                try out.append(allocator, ch);
                try out.appendSlice(allocator, op.line);
                try out.append(allocator, '\n');

                if (touches_eof) {
                    if (op.kind == DiffOpKind.Equal) {
                        if (!old_has_nl && !new_has_nl && last_old_idx != null && h == last_old_idx.?) {
                            try out.appendSlice(allocator, "\\ No newline at end of file\n");
                        }
                    } else if (op.kind == DiffOpKind.Delete) {
                        if (!old_has_nl && last_old_idx != null && h == last_old_idx.?) {
                            try out.appendSlice(allocator, "\\ No newline at end of file\n");
                        }
                    } else if (op.kind == DiffOpKind.Insert) {
                        if (!new_has_nl && last_new_idx != null && h == last_new_idx.?) {
                            try out.appendSlice(allocator, "\\ No newline at end of file\n");
                        }
                    }
                }
                h += 1;
            }

            idx = hunk_end;
        }

        return try out.toOwnedSlice(allocator);
    }

    /// Computes the Myers edit distance D (number of inserted + deleted lines) from a `DiffOp` slice.
    pub static fn editDistance(ops: []const DiffOp): usize {
        let dist: usize = 0;
        let i: usize = 0;
        while (i < ops.len) {
            if (ops[i].kind != DiffOpKind.Equal) {
                dist += 1;
            }
            i += 1;
        }
        return dist;
    }
}
