// Source: Absorbed from /data/dev/bun/src/runtime/node/assert/myers_diff.cp | Synced: 2026-11-22T06:39:08Z
// Myers O(ND) shortest-edit-script diff engine for lines and bytes.

pub err DiffError {
    InputsTooLarge,
    DiffTooLarge,
    OutOfMemory,
}

pub enum DiffKind: u8 {
    equal = 0,
    insert = 1,
    delete = 2,

    fn prefixByte(): u8 {
        return switch (_) {
            .equal => ' ',
            .insert => '+',
            .delete => '-',
        };
    }
}

pub struct LineDiff {
    kind: DiffKind,
    value: string,
}

pub struct ByteDiff {
    kind: DiffKind,
    value: u8,
}

pub struct DiffOptions {
    check_comma_disparity: bool = false,
}

pub struct LineDiffResult {
    items: []LineDiff,
    allocator: std.mem.Allocator,

    fn len(): usize {
        return _.items.len;
    }

    fn countKind(kind: DiffKind): usize {
        let n: usize = 0;
        for (it of _.items) {
            if (it.kind == kind) {
                n += 1;
            }
        }
        return n;
    }

    fn editDistance(): usize {
        return _.countKind(.insert) + _.countKind(.delete);
    }

    fn isIdentical(): bool {
        return _.editDistance() == 0;
    }

    ~init() {
        if (_.items.len > 0) {
            _.allocator.free(_.items);
            _.items = [];
        }
    }
}

pub struct ByteDiffResult {
    items: []ByteDiff,
    allocator: std.mem.Allocator,

    fn len(): usize {
        return _.items.len;
    }

    fn editDistance(): usize {
        let n: usize = 0;
        for (it of _.items) {
            if (it.kind != .equal) {
                n += 1;
            }
        }
        return n;
    }

    fn isIdentical(): bool {
        return _.editDistance() == 0;
    }

    ~init() {
        if (_.items.len > 0) {
            _.allocator.free(_.items);
            _.items = [];
        }
    }
}

const MAX_DIFF_LEN: usize = 0x3FFF_FFFF;

pub fn areLinesEqual(a: string, b: string, check_comma_disparity: bool): bool {
    if (!check_comma_disparity) {
        return std.mem.eql(u8, a, b);
    }
    if (a.len == b.len) {
        return std.mem.eql(u8, a, b);
    }
    if (a.len == b.len + 1) {
        return a[a.len - 1] == ',' && std.mem.eql(u8, a[0..b.len], b);
    }
    if (b.len == a.len + 1) {
        return b[b.len - 1] == ',' && std.mem.eql(u8, b[0..a.len], a);
    }
    return false;
}

pub fn splitLines(allocator: std.mem.Allocator, text: string): ([]string) ! DiffError {
    if (text.len == 0) {
        let empty: []string = [];
        return empty;
    }
    let count: usize = 1;
    for (ch of text) {
        if (ch == '\n') {
            count += 1;
        }
    }
    let out = allocator.alloc(string, count) catch return error.OutOfMemory;
    let idx: usize = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i < text.len) : (i += 1) {
        if (text[i] == '\n') {
            out[idx] = text[start..i];
            idx += 1;
            start = i + 1;
        }
    }
    out[idx] = text[start..text.len];
    return out;
}

/// Computes the Myers O(ND) shortest edit script between `actual` and `expected` line slices.
/// Results are returned in forward chronological order.
pub fn diffLines(
    allocator: std.mem.Allocator,
    actual: []const string,
    expected: []const string,
    opts: DiffOptions,
): LineDiffResult ! DiffError {
    const total = actual.len + expected.len;
    if (total > MAX_DIFF_LEN) return error.InputsTooLarge;
    const graph_size = (2 * total) + 1;
    if (graph_size > MAX_DIFF_LEN) return error.DiffTooLarge;

    if (total == 0) {
        let empty: []LineDiff = [];
        return LineDiffResult{ items: empty, allocator: allocator };
    }

    let graph = allocator.alloc(u32, graph_size) catch return error.OutOfMemory;
    defer allocator.free(graph);
    @memset(graph, 0);

    let trace_frames = allocator.alloc([]u32, total + 1) catch return error.OutOfMemory;
    let trace_len: usize = 0;
    defer {
        let ti: usize = 0;
        while (ti < trace_len) : (ti += 1) {
            allocator.free(trace_frames[ti]);
        }
        allocator.free(trace_frames);
    }

    const max_i: i64 = i64(total);
    let d_usize: usize = 0;
    let found = false;
    while (d_usize <= total) : (d_usize += 1) {
        const frame = allocator.dupe(u32, graph) catch return error.OutOfMemory;
        trace_frames[trace_len] = frame;
        trace_len += 1;

        const d: i64 = i64(d_usize);
        let k: i64 = 0 - d;
        while (k <= d) : (k += 2) {
            const offset: usize = usize(k + max_i);
            let x: usize = 0;
            if (k == (0 - d) || (k != d && graph[offset - 1] < graph[offset + 1])) {
                x = usize(graph[offset + 1]);
            } else {
                x = usize(graph[offset - 1]) + 1;
            }
            let y: usize = usize(i64(x) - k);
            while (x < actual.len && y < expected.len && areLinesEqual(actual[x], expected[y], opts.check_comma_disparity)) {
                x += 1;
                y += 1;
            }
            graph[offset] = u32(x);
            if (x >= actual.len && y >= expected.len) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    let rev_list: std.ArrayList(LineDiff) = .empty;
    defer rev_list.deinit(allocator);

    let x_i: i64 = i64(actual.len);
    let y_i: i64 = i64(expected.len);
    let level: usize = trace_len;
    while (level > 0) {
        level -= 1;
        const g = trace_frames[level];
        const k: i64 = x_i - y_i;
        const off: usize = usize(k + max_i);
        const lv_i: i64 = i64(level);
        const prev_k: i64 = if (k == (0 - lv_i) || (k != lv_i && g[off - 1] < g[off + 1]))
            k + 1
        else
            k - 1;
        const prev_x: i64 = i64(g[usize(prev_k + max_i)]);
        const prev_y: i64 = prev_x - prev_k;

        while (x_i > prev_x && y_i > prev_y) {
            const ax = actual[usize(x_i - 1)];
            let val = ax;
            if (opts.check_comma_disparity && ax.len > 0 && ax[ax.len - 1] != ',') {
                val = expected[usize(y_i - 1)];
            }
            rev_list.append(allocator, LineDiff{ kind: .equal, value: val }) catch return error.OutOfMemory;
            x_i -= 1;
            y_i -= 1;
        }
        if (level > 0) {
            if (x_i > prev_x) {
                rev_list.append(allocator, LineDiff{ kind: .insert, value: actual[usize(x_i - 1)] }) catch return error.OutOfMemory;
                x_i -= 1;
            } else {
                rev_list.append(allocator, LineDiff{ kind: .delete, value: expected[usize(y_i - 1)] }) catch return error.OutOfMemory;
                y_i -= 1;
            }
        }
    }

    const n = rev_list.items.len;
    let out = allocator.alloc(LineDiff, n) catch return error.OutOfMemory;
    let ri: usize = 0;
    while (ri < n) : (ri += 1) {
        out[ri] = rev_list.items[n - 1 - ri];
    }
    return LineDiffResult{ items: out, allocator: allocator };
}

/// Computes the Myers O(ND) shortest edit script between two byte slices.
pub fn diffBytes(
    allocator: std.mem.Allocator,
    actual: []const u8,
    expected: []const u8,
): ByteDiffResult ! DiffError {
    const total = actual.len + expected.len;
    if (total > MAX_DIFF_LEN) return error.InputsTooLarge;
    const graph_size = (2 * total) + 1;
    if (graph_size > MAX_DIFF_LEN) return error.DiffTooLarge;

    if (total == 0) {
        let empty: []ByteDiff = [];
        return ByteDiffResult{ items: empty, allocator: allocator };
    }

    let graph = allocator.alloc(u32, graph_size) catch return error.OutOfMemory;
    defer allocator.free(graph);
    @memset(graph, 0);

    let trace_frames = allocator.alloc([]u32, total + 1) catch return error.OutOfMemory;
    let trace_len: usize = 0;
    defer {
        let ti: usize = 0;
        while (ti < trace_len) : (ti += 1) {
            allocator.free(trace_frames[ti]);
        }
        allocator.free(trace_frames);
    }

    const max_i: i64 = i64(total);
    let d_usize: usize = 0;
    let found = false;
    while (d_usize <= total) : (d_usize += 1) {
        const frame = allocator.dupe(u32, graph) catch return error.OutOfMemory;
        trace_frames[trace_len] = frame;
        trace_len += 1;

        const d: i64 = i64(d_usize);
        let k: i64 = 0 - d;
        while (k <= d) : (k += 2) {
            const offset: usize = usize(k + max_i);
            let x: usize = 0;
            if (k == (0 - d) || (k != d && graph[offset - 1] < graph[offset + 1])) {
                x = usize(graph[offset + 1]);
            } else {
                x = usize(graph[offset - 1]) + 1;
            }
            let y: usize = usize(i64(x) - k);
            while (x < actual.len && y < expected.len && actual[x] == expected[y]) {
                x += 1;
                y += 1;
            }
            graph[offset] = u32(x);
            if (x >= actual.len && y >= expected.len) {
                found = true;
                break;
            }
        }
        if (found) break;
    }

    let rev_list: std.ArrayList(ByteDiff) = .empty;
    defer rev_list.deinit(allocator);

    let x_i: i64 = i64(actual.len);
    let y_i: i64 = i64(expected.len);
    let level: usize = trace_len;
    while (level > 0) {
        level -= 1;
        const g = trace_frames[level];
        const k: i64 = x_i - y_i;
        const off: usize = usize(k + max_i);
        const lv_i: i64 = i64(level);
        const prev_k: i64 = if (k == (0 - lv_i) || (k != lv_i && g[off - 1] < g[off + 1]))
            k + 1
        else
            k - 1;
        const prev_x: i64 = i64(g[usize(prev_k + max_i)]);
        const prev_y: i64 = prev_x - prev_k;

        while (x_i > prev_x && y_i > prev_y) {
            rev_list.append(allocator, ByteDiff{ kind: .equal, value: actual[usize(x_i - 1)] }) catch return error.OutOfMemory;
            x_i -= 1;
            y_i -= 1;
        }
        if (level > 0) {
            if (x_i > prev_x) {
                rev_list.append(allocator, ByteDiff{ kind: .insert, value: actual[usize(x_i - 1)] }) catch return error.OutOfMemory;
                x_i -= 1;
            } else {
                rev_list.append(allocator, ByteDiff{ kind: .delete, value: expected[usize(y_i - 1)] }) catch return error.OutOfMemory;
                y_i -= 1;
            }
        }
    }

    const n = rev_list.items.len;
    let out = allocator.alloc(ByteDiff, n) catch return error.OutOfMemory;
    let ri: usize = 0;
    while (ri < n) : (ri += 1) {
        out[ri] = rev_list.items[n - 1 - ri];
    }
    return ByteDiffResult{ items: out, allocator: allocator };
}

/// Formats line diff items into a readable `+`/`-`/` ` prefixed unified block.
pub fn formatLineDiff(allocator: std.mem.Allocator, diffs: []const LineDiff): ([]u8) ! DiffError {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(allocator);
    for (d of diffs) {
        buf.append(allocator, d.kind.prefixByte()) catch return error.OutOfMemory;
        buf.append(allocator, ' ') catch return error.OutOfMemory;
        buf.appendSlice(allocator, d.value) catch return error.OutOfMemory;
        buf.append(allocator, '\n') catch return error.OutOfMemory;
    }
    return buf.toOwnedSlice(allocator) catch return error.OutOfMemory;
}
