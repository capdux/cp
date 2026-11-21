// Source: native-cp-stdlib (Myers SES Line Diff & Unified Diff)
const std = @import("std");

type StrSlice = []const u8;

export enum DiffOpKind {
    Equal,
    Insert,
    Delete,
}

export struct DiffOp {
    kind: DiffOpKind,
    line: []const u8,
    old_line_no: usize?,
    new_line_no: usize?,
}

export struct DiffResult {
    ops: []DiffOp,
    added: usize,
    deleted: usize,

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        alloc.free(_.ops);
        _.ops = _.ops[0..0];
        _.added = 0;
        _.deleted = 0;
    }
}

fn splitLines(alloc: std.mem.Allocator, text: []const u8): ![]StrSlice {
    if (text.len == 0) {
        return try alloc.alloc(StrSlice, 0);
    }
    let list = std.ArrayList(StrSlice).empty;
    errdefer list.deinit(alloc);

    let start: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        if (text[i] == '\n') {
            try list.append(alloc, text[start..i]);
            start = i + 1;
        }
        i = i + 1;
    }
    try list.append(alloc, text[start..text.len]);
    return try list.toOwnedSlice(alloc);
}

fn traceAt(trace: []const isize, d: usize, k: isize): isize {
    const d_i: isize = @intCast(d);
    const base: usize = d * d;
    const rel: usize = @intCast(k + d_i);
    return trace[base + rel];
}

export fn diffLines(alloc: std.mem.Allocator, old_text: []const u8, new_text: []const u8): !DiffResult {
    const old_lines = try splitLines(alloc, old_text);
    defer alloc.free(old_lines);
    const new_lines = try splitLines(alloc, new_text);
    defer alloc.free(new_lines);

    const n = old_lines.len;
    const m = new_lines.len;

    if (n == 0 and m == 0) {
        const empty_ops = try alloc.alloc(DiffOp, 0);
        return DiffResult{
            .ops = empty_ops,
            .added = 0,
            .deleted = 0,
        };
    }

    if (n == 0) {
        let ops = try alloc.alloc(DiffOp, m);
        let j: usize = 0;
        while (j < m) {
            ops[j] = DiffOp{
                .kind = DiffOpKind.Insert,
                .line = new_lines[j],
                .old_line_no = null,
                .new_line_no = j + 1,
            };
            j = j + 1;
        }
        return DiffResult{
            .ops = ops,
            .added = m,
            .deleted = 0,
        };
    }

    if (m == 0) {
        let ops = try alloc.alloc(DiffOp, n);
        let i: usize = 0;
        while (i < n) {
            ops[i] = DiffOp{
                .kind = DiffOpKind.Delete,
                .line = old_lines[i],
                .old_line_no = i + 1,
                .new_line_no = null,
            };
            i = i + 1;
        }
        return DiffResult{
            .ops = ops,
            .added = 0,
            .deleted = n,
        };
    }

    const max_d: usize = n + m;
    const offset: isize = @intCast(max_d);
    const n_i: isize = @intCast(n);
    const m_i: isize = @intCast(m);

    let v = try alloc.alloc(isize, 2 * max_d + 1);
    defer alloc.free(v);
    let vi: usize = 0;
    while (vi < v.len) {
        v[vi] = 0;
        vi = vi + 1;
    }

    let trace = std.ArrayList(isize).empty;
    defer trace.deinit(alloc);

    let found_d: usize = 0;
    let found = false;
    let d: usize = 0;
    while (d <= max_d and !found) {
        const d_i: isize = @intCast(d);
        let snap_k: isize = -d_i;
        while (snap_k <= d_i) {
            const snap_idx: usize = @intCast(offset + snap_k);
            try trace.append(alloc, v[snap_idx]);
            snap_k = snap_k + 1;
        }

        let k: isize = -d_i;
        while (k <= d_i) {
            let x: isize = 0;
            const k_minus_1_idx: usize = @intCast(offset + k - 1);
            const k_plus_1_idx: usize = @intCast(offset + k + 1);
            if (k == -d_i or (k != d_i and v[k_minus_1_idx] < v[k_plus_1_idx])) {
                x = v[k_plus_1_idx];
            } else {
                x = v[k_minus_1_idx] + 1;
            }
            let y: isize = x - k;
            while (x < n_i and y < m_i) {
                const xu: usize = @intCast(x);
                const yu: usize = @intCast(y);
                if (!std.mem.eql(u8, old_lines[xu], new_lines[yu])) {
                    break;
                }
                x = x + 1;
                y = y + 1;
            }
            const k_idx: usize = @intCast(offset + k);
            v[k_idx] = x;

            if (x >= n_i and y >= m_i) {
                found_d = d;
                found = true;
                break;
            }
            k = k + 2;
        }
        d = d + 1;
    }

    let rev_ops = std.ArrayList(DiffOp).empty;
    defer rev_ops.deinit(alloc);

    let x: isize = n_i;
    let y: isize = m_i;
    let d_back: usize = found_d;
    const trace_slice = trace.items;

    while (d_back > 0) {
        const d_back_i: isize = @intCast(d_back);
        const k: isize = x - y;
        let prev_k: isize = 0;
        if (k == -d_back_i or (k != d_back_i and traceAt(trace_slice, d_back, k - 1) < traceAt(trace_slice, d_back, k + 1))) {
            prev_k = k + 1;
        } else {
            prev_k = k - 1;
        }
        const prev_x: isize = traceAt(trace_slice, d_back, prev_k);
        const prev_y: isize = prev_x - prev_k;

        while (x > prev_x and y > prev_y) {
            x = x - 1;
            y = y - 1;
            const xu: usize = @intCast(x);
            const yu: usize = @intCast(y);
            try rev_ops.append(alloc, DiffOp{
                .kind = DiffOpKind.Equal,
                .line = old_lines[xu],
                .old_line_no = xu + 1,
                .new_line_no = yu + 1,
            });
        }

        if (x == prev_x) {
            const pyu: usize = @intCast(prev_y);
            try rev_ops.append(alloc, DiffOp{
                .kind = DiffOpKind.Insert,
                .line = new_lines[pyu],
                .old_line_no = null,
                .new_line_no = pyu + 1,
            });
        } else {
            const pxu: usize = @intCast(prev_x);
            try rev_ops.append(alloc, DiffOp{
                .kind = DiffOpKind.Delete,
                .line = old_lines[pxu],
                .old_line_no = pxu + 1,
                .new_line_no = null,
            });
        }

        x = prev_x;
        y = prev_y;
        d_back = d_back - 1;
    }

    while (x > 0 and y > 0) {
        x = x - 1;
        y = y - 1;
        const xu: usize = @intCast(x);
        const yu: usize = @intCast(y);
        try rev_ops.append(alloc, DiffOp{
            .kind = DiffOpKind.Equal,
            .line = old_lines[xu],
            .old_line_no = xu + 1,
            .new_line_no = yu + 1,
        });
    }

    const total_ops = rev_ops.items.len;
    let ops = try alloc.alloc(DiffOp, total_ops);
    let added: usize = 0;
    let deleted: usize = 0;
    let idx: usize = 0;
    while (idx < total_ops) {
        const rev_idx: usize = total_ops - 1 - idx;
        const op = rev_ops.items[rev_idx];
        ops[idx] = op;
        if (op.kind == DiffOpKind.Insert) {
            added = added + 1;
        } else if (op.kind == DiffOpKind.Delete) {
            deleted = deleted + 1;
        }
        idx = idx + 1;
    }

    return DiffResult{
        .ops = ops,
        .added = added,
        .deleted = deleted,
    };
}

export fn reconstructNew(alloc: std.mem.Allocator, ops: []const DiffOp): ![]u8 {
    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    let first = true;
    for (op of ops) {
        if (op.kind == DiffOpKind.Delete) {
            continue;
        }
        if (!first) {
            try out.append(alloc, '\n');
        }
        try out.appendSlice(alloc, op.line);
        first = false;
    }
    return try out.toOwnedSlice(alloc);
}

struct HunkRange {
    start: usize,
    end: usize,
}

export fn formatUnifiedDiff(
    alloc: std.mem.Allocator,
    old_name: []const u8,
    new_name: []const u8,
    old_text: []const u8,
    new_text: []const u8,
    context_lines: usize,
): ![]u8 {
    let res = try diffLines(alloc, old_text, new_text);
    defer res.deinit(alloc);

    if (res.added == 0 and res.deleted == 0) {
        return try alloc.alloc(u8, 0);
    }

    const ops = res.ops;
    let hunks = std.ArrayList(HunkRange).empty;
    defer hunks.deinit(alloc);

    let i: usize = 0;
    while (i < ops.len) {
        if (ops[i].kind != DiffOpKind.Equal) {
            let h_start: usize = 0;
            if (i > context_lines) {
                h_start = i - context_lines;
            }
            let h_end: usize = i + context_lines;
            if (h_end >= ops.len) {
                h_end = ops.len - 1;
            }

            if (hunks.items.len > 0) {
                const last_idx: usize = hunks.items.len - 1;
                if (h_start <= hunks.items[last_idx].end + 1) {
                    if (h_end > hunks.items[last_idx].end) {
                        hunks.items[last_idx].end = h_end;
                    }
                } else {
                    try hunks.append(alloc, HunkRange{ .start = h_start, .end = h_end });
                }
            } else {
                try hunks.append(alloc, HunkRange{ .start = h_start, .end = h_end });
            }
        }
        i = i + 1;
    }

    let out = std.ArrayList(u8).empty;
    errdefer out.deinit(alloc);

    try out.appendSlice(alloc, "--- ");
    try out.appendSlice(alloc, old_name);
    try out.append(alloc, '\n');
    try out.appendSlice(alloc, "+++ ");
    try out.appendSlice(alloc, new_name);
    try out.append(alloc, '\n');

    for (h of hunks.items) {
        let old_count: usize = 0;
        let new_count: usize = 0;
        let old_start: usize = 0;
        let new_start: usize = 0;
        let has_old = false;
        let has_new = false;

        let hi: usize = h.start;
        while (hi <= h.end) {
            const op = ops[hi];
            if (op.kind == DiffOpKind.Equal or op.kind == DiffOpKind.Delete) {
                old_count = old_count + 1;
                if (!has_old) {
                    if (line_no := op.old_line_no) {
                        old_start = line_no;
                        has_old = true;
                    }
                }
            }
            if (op.kind == DiffOpKind.Equal or op.kind == DiffOpKind.Insert) {
                new_count = new_count + 1;
                if (!has_new) {
                    if (line_no := op.new_line_no) {
                        new_start = line_no;
                        has_new = true;
                    }
                }
            }
            hi = hi + 1;
        }

        const header = try std.fmt.allocPrint(
            alloc,
            "@@ -{d},{d} +{d},{d} @@\n",
            .{ old_start, old_count, new_start, new_count },
        );
        defer alloc.free(header);
        try out.appendSlice(alloc, header);

        let pj: usize = h.start;
        while (pj <= h.end) {
            const op = ops[pj];
            if (op.kind == DiffOpKind.Equal) {
                try out.append(alloc, ' ');
            } else if (op.kind == DiffOpKind.Delete) {
                try out.append(alloc, '-');
            } else {
                try out.append(alloc, '+');
            }
            try out.appendSlice(alloc, op.line);
            try out.append(alloc, '\n');
            pj = pj + 1;
        }
    }

    return try out.toOwnedSlice(alloc);
}
